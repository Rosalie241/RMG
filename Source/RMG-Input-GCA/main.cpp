/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#define INPUT_PLUGIN_API_VERSION 0x020100

#define M64P_PLUGIN_PROTOTYPES 1
#include <RMG-Core/m64p/api/m64p_common.h>
#include <RMG-Core/m64p/api/m64p_plugin.h>
#include <RMG-Core/m64p/api/m64p_custom.h>
#include <RMG-Core/m64p/api/m64p_types.h>

#include <RMG-Core/Settings.hpp>
#include <libusb.h>

#include "UserInterface/MainDialog.hpp"

//
// Local Defines
//

#define NUM_CONTROLLERS    4
#define N64_AXIS_PEAK      85

#define GCA_VENDOR_ID  0x057e
#define GCA_PRODUCT_ID 0x0337

#define GCA_ENDPOINT_IN  0x81
#define GCA_ENDPOINT_OUT 0x02

#define GCA_COMMAND_POLL 0x13

//
// Local Structures
//

struct SettingsProfile
{
    double DeadzoneValue = 0.09;
    double SensitivityValue = 1.0;

    double TriggerTreshold = 0.5;
    double CButtonTreshold = 0.4;
};

struct GameCubeAdapterControllerState
{
    uint8_t Status;

    union
    {
        uint8_t Buttons1;
        struct
        {
            bool A : 1;
            bool B : 1;
            bool X : 1;
            bool Y : 1;

            bool DpadLeft  : 1;
            bool DpadRight : 1;
            bool DpadDown  : 1;
            bool DpadUp    : 1;
        };
    };
    

    union
    {
        uint8_t Buttons2;
        struct
        {
            bool Start : 1;
            bool Z : 1;
            bool R : 1;
            bool L : 1;
        };
    };

    uint8_t LeftStickX;
    uint8_t LeftStickY;
    uint8_t RightStickX;
    uint8_t RightStickY;

    uint8_t LeftTrigger;
    uint8_t RightTrigger;
};

//
// Local variables
//

// libusb variables
static bool l_UsbInitialized = false;

// GCA variables
static libusb_device_handle* l_DeviceHandle = nullptr;
static GameCubeAdapterControllerState l_ControllerState[4] = {0};
static SettingsProfile l_Settings = {0};

// mupen64plus debug callback
static void (*l_DebugCallback)(void *, int, const char *) = nullptr;
static void *l_DebugCallContext                           = nullptr;

//
// Custom Internal Plugin Functions
//

void PluginDebugMessage(int level, std::string message)
{
    if (l_DebugCallback == nullptr)
    {
        return;
    }

    l_DebugCallback(l_DebugCallContext, level, message.c_str());
}

//
// Internal Functions
//

static bool usb_init(void)
{
    std::string debugMessage;

    int ret = libusb_init(nullptr);
    if (ret != LIBUSB_SUCCESS)
    {
        debugMessage = "usb_init(): failed to initialize libusb: ";
        debugMessage += libusb_error_name(ret);
        PluginDebugMessage(M64MSG_ERROR, debugMessage);
        return false;
    }

    l_UsbInitialized = true;
    return true;
}

static void usb_quit(void)
{
    if (l_UsbInitialized)
    {
        libusb_exit(nullptr);
        l_UsbInitialized = false;
    }
}

static bool gca_init(void)
{
    std::string debugMessage;
    int ret;

    // reset state
    for (int i = 0; i < NUM_CONTROLLERS; i++)
    {
        l_ControllerState[i] = {0};
    }

    // attempt open device
    l_DeviceHandle = libusb_open_device_with_vid_pid(nullptr, GCA_VENDOR_ID, GCA_PRODUCT_ID);
    if (l_DeviceHandle == nullptr)
    {
        debugMessage = "gca_init(): failed to open adapter!";
        PluginDebugMessage(M64MSG_ERROR, debugMessage);
        return false;
    }

    // according to dolphin, this makes
    // the Nyko-brand adapters work
    libusb_control_transfer(l_DeviceHandle, 0x21, 11, 0x0001, 0, nullptr, 0, 1000);

    // only detach kernel driver when required
    if (libusb_kernel_driver_active(l_DeviceHandle, 0) == 1)
    {
        ret = libusb_detach_kernel_driver(l_DeviceHandle, 0);
        if (ret != LIBUSB_SUCCESS)
        {
            libusb_close(l_DeviceHandle);
            debugMessage = "gca_init(): failed to detach kernel driver: ";
            debugMessage += libusb_error_name(ret);
            PluginDebugMessage(M64MSG_ERROR, debugMessage);
            return false;
        }
    }

    ret = libusb_claim_interface(l_DeviceHandle, 0);
    if (ret != LIBUSB_SUCCESS)
    {
        libusb_close(l_DeviceHandle);
        debugMessage = "gca_init(): failed to claim interface: ";
        debugMessage += libusb_error_name(ret);
        PluginDebugMessage(M64MSG_ERROR, debugMessage);
        return false;
    }

    // attempt to begin polling
    uint8_t cmd = GCA_COMMAND_POLL;
    ret = libusb_interrupt_transfer(l_DeviceHandle, GCA_ENDPOINT_OUT, &cmd, sizeof(cmd), nullptr, 16);
    if (ret != LIBUSB_SUCCESS)
    {
        libusb_release_interface(l_DeviceHandle, 0);
        libusb_close(l_DeviceHandle);
        debugMessage = "gca_init(): failed to send polling cmd: ";
        debugMessage += libusb_error_name(ret);
        PluginDebugMessage(M64MSG_ERROR, debugMessage);
        return false;
    }

    debugMessage = "gca_init(): successfully opened adapter";
    PluginDebugMessage(M64MSG_INFO, debugMessage);
    return true;
}

static void gca_quit(void)
{
    if (l_DeviceHandle != nullptr)
    {
        libusb_release_interface(l_DeviceHandle, 0);
        libusb_close(l_DeviceHandle);
        l_DeviceHandle = nullptr;
    }
}

static bool gca_poll_input(int index, GameCubeAdapterControllerState& state)
{
    if (index == 0 && l_DeviceHandle != nullptr)
    {
        uint8_t readBuf[37] = {0};
        int transferred = 0;
        int ret = libusb_interrupt_transfer(l_DeviceHandle, GCA_ENDPOINT_IN, readBuf, sizeof(readBuf), &transferred, 16);
        if (ret != LIBUSB_SUCCESS || transferred != sizeof(readBuf))
        {
            std::string debugMessage;
            debugMessage = "gca_poll_input(): failed to retrieve input buffer: ";
            debugMessage += libusb_error_name(ret);
            PluginDebugMessage(M64MSG_WARNING, debugMessage);
            return false;
        }

        for (int i = 0; i < NUM_CONTROLLERS; i++)
        {
            int offset = (i * 9);
            l_ControllerState[i].Status       = readBuf[offset + 1];
            l_ControllerState[i].Buttons1     = readBuf[offset + 2];
            l_ControllerState[i].Buttons2     = readBuf[offset + 3];
            l_ControllerState[i].LeftStickX   = readBuf[offset + 4];
            l_ControllerState[i].LeftStickY   = readBuf[offset + 5];
            l_ControllerState[i].RightStickX  = readBuf[offset + 6];
            l_ControllerState[i].RightStickY  = readBuf[offset + 7];
            l_ControllerState[i].LeftTrigger  = readBuf[offset + 8];
            l_ControllerState[i].RightTrigger = readBuf[offset + 9];
        }
    }

    state = l_ControllerState[index];
    return true;
}

static void load_settings(void)
{
    l_Settings.DeadzoneValue = static_cast<double>(CoreSettingsGetIntValue(SettingsID::GCAInput_Deadzone)) / 100.0;
    l_Settings.SensitivityValue = static_cast<double>(CoreSettingsGetIntValue(SettingsID::GCAInput_Sensitivity)) / 100.0;
    l_Settings.CButtonTreshold = static_cast<double>(CoreSettingsGetIntValue(SettingsID::GCAInput_CButtonTreshold)) / 100.0;
    l_Settings.TriggerTreshold = static_cast<double>(CoreSettingsGetIntValue(SettingsID::GCAInput_TriggerTreshold)) / 100.0;
}

static double apply_deadzone(const double input, const double deadzone)
{
    const double inputAbsolute = std::abs(input) / INT8_MAX;

    if (inputAbsolute <= deadzone)
    {
        return 0;
    }

    return input;
}

//
// Basic Plugin Functions
//

EXPORT m64p_error CALL PluginStartup(m64p_dynlib_handle CoreLibHandle, void *Context, void (*DebugCallback)(void *, int, const char *))
{
    // setup debug callback
    l_DebugCallback    = DebugCallback;
    l_DebugCallContext = Context;

    if (!usb_init())
    {
        return M64ERR_SYSTEM_FAIL;
    }

    load_settings();
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginShutdown(void)
{
    // clear debug callback
    l_DebugCallback    = nullptr;
    l_DebugCallContext = nullptr;

    usb_quit();

    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginGetVersion(m64p_plugin_type *pluginType, int *pluginVersion, 
    int *apiVersion, const char **pluginNamePtr, int *capabilities)
{
    if (pluginType != nullptr)
    {
        *pluginType = M64PLUGIN_INPUT;
    }

    if (pluginVersion != nullptr)
    {
        *pluginVersion = 0x010000;
    }

    if (apiVersion != nullptr)
    {
        *apiVersion = INPUT_PLUGIN_API_VERSION;
    }

    if (pluginNamePtr != nullptr)
    {
        *pluginNamePtr = "Rosalie's Mupen GUI - GameCube Adapter Input Plugin";
    }

    if (capabilities != nullptr)
    {
        *capabilities = 0;
    }

    return M64ERR_SUCCESS;
}

//
// Custom Plugin Functions
//

EXPORT m64p_error CALL PluginConfig(void* parent)
{
    UserInterface::MainDialog dialog((QWidget*)parent);
    dialog.exec();

    // re-load settings
    load_settings();

    return M64ERR_SUCCESS;
}

//
// Input Plugin Functions
//

EXPORT void CALL ControllerCommand(int Control, unsigned char* Command)
{
}

EXPORT void CALL GetKeys(int Control, BUTTONS* Keys)
{
    GameCubeAdapterControllerState state;
    if (!gca_poll_input(Control, state) || !state.Status)
    {
        Keys->Value = 0;
        return;
    }

    Keys->A_BUTTON = state.A;
    Keys->B_BUTTON = state.B;
    Keys->START_BUTTON = state.Start;
    Keys->L_DPAD = state.DpadLeft;
    Keys->R_DPAD = state.DpadRight;
    Keys->D_DPAD = state.DpadDown;
    Keys->U_DPAD = state.DpadUp;
    Keys->Z_TRIG = state.Z;

    const int triggerTreshold = INT8_MAX * l_Settings.TriggerTreshold;
    const int cStickTreshold  = INT8_MAX * l_Settings.CButtonTreshold;
    const int8_t cX = static_cast<int8_t>(state.RightStickX + 128);
    const int8_t cY = static_cast<int8_t>(state.RightStickY + 128);

    Keys->L_TRIG = state.LeftTrigger > triggerTreshold;
    Keys->R_TRIG = state.RightTrigger > triggerTreshold;

    Keys->L_CBUTTON = cX < -cStickTreshold;
    Keys->R_CBUTTON = cX > cStickTreshold;
    Keys->U_CBUTTON = cY > cStickTreshold;
    Keys->D_CBUTTON = cY < -cStickTreshold;

    const int8_t x = static_cast<int8_t>(state.LeftStickX + 128);
    const int8_t y = static_cast<int8_t>(state.LeftStickY + 128);

    double modX = (static_cast<double>(x) / static_cast<double>(INT8_MAX)) * N64_AXIS_PEAK * l_Settings.SensitivityValue;
    double modY = (static_cast<double>(y) / static_cast<double>(INT8_MAX)) * N64_AXIS_PEAK * l_Settings.SensitivityValue;

    modX = apply_deadzone(modX, l_Settings.DeadzoneValue);
    modY = apply_deadzone(modY, l_Settings.DeadzoneValue);

    Keys->X_AXIS = static_cast<int>(modX);
    Keys->Y_AXIS = static_cast<int>(modY);
}

EXPORT void CALL InitiateControllers(CONTROL_INFO ControlInfo)
{
    if (!gca_init())
    {
        return;
    }

    for (int i = 0; i < NUM_CONTROLLERS; i++)
    {
        GameCubeAdapterControllerState state;
        if (!gca_poll_input(i, state))
        {
            return;
        }

        ControlInfo.Controls[i].Present = (state.Status > 0) ? 1 : 0;
    }

    // load settings
    load_settings();
}

EXPORT void CALL ReadController(int Control, unsigned char *Command)
{
}

EXPORT int CALL RomOpen(void)
{
    return 1;
}

EXPORT void CALL RomClosed(void)
{
    gca_quit();
}

EXPORT void CALL SDL_KeyDown(int keymod, int keysym)
{
}

EXPORT void CALL SDL_KeyUp(int keymod, int keysym)
{
}
