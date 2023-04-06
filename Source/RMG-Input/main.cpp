/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#define CORE_PLUGIN
#define M64P_PLUGIN_PROTOTYPES 1
#define INPUT_PLUGIN_API_VERSION 0x020100

#include <UserInterface/MainDialog.hpp>
#include "Thread/SDLThread.hpp"
#include "Utilities/InputDevice.hpp"
#include "common.hpp"

#include <RMG-Core/Core.hpp>

#include <QApplication>
#include <SDL.h>

#include <algorithm>
#include <chrono>

//
// Local Defines
//

#define NUM_CONTROLLERS    4
#define N64_AXIS_PEAK      85
#define MAX_DIAGONAL_VALUE 69
#define DEADZONE_VALUE     7

#define RD_GETSTATUS        0x00   // get status
#define RD_READKEYS         0x01   // read button values
#define RD_READPAK          0x02   // read from controllerpack
#define RD_WRITEPAK         0x03   // write to controllerpack
#define RD_RESETCONTROLLER  0xff   // reset controller
#define RD_READEEPROM       0x04   // read eeprom
#define RD_WRITEEPROM       0x05   // write eeprom

#define PAK_IO_RUMBLE       0xC000 // the address where rumble-commands are sent to

//
// Local Structures
//

struct InputMapping
{
    std::vector<std::string> Name;
    std::vector<int>         Type;
    std::vector<int>         Data;
    std::vector<int>         ExtraData;
    int                      Count = 0;
};

struct InputProfile
{
    bool PluggedIn    = false;
    int DeadzoneValue = 0;
    int SensitivityValue = 100;

    N64ControllerPak ControllerPak = N64ControllerPak::None;

    // input device information
    std::string DeviceName;
    int DeviceNum = -1;
    std::chrono::time_point<std::chrono::high_resolution_clock> LastDeviceCheckTime = std::chrono::high_resolution_clock::now();

    // Gameboy information
    std::string GameboyRom;
    std::string GameboySave;

    // input device
    Utilities::InputDevice InputDevice;

    // buttons
    InputMapping Button_A;
    InputMapping Button_B;
    InputMapping Button_Start;
    InputMapping Button_DpadUp;
    InputMapping Button_DpadDown;
    InputMapping Button_DpadLeft;
    InputMapping Button_DpadRight;
    InputMapping Button_CButtonUp;
    InputMapping Button_CButtonDown;
    InputMapping Button_CButtonLeft;
    InputMapping Button_CButtonRight;
    InputMapping Button_LeftTrigger;
    InputMapping Button_RightTrigger;
    InputMapping Button_ZTrigger;

    // analog stick
    InputMapping AnalogStick_Up;
    InputMapping AnalogStick_Down;
    InputMapping AnalogStick_Left;
    InputMapping AnalogStick_Right;
};

//
// Local variables
//

// SDL thread
static Thread::SDLThread *l_SDLThread = nullptr;

// input profiles
static InputProfile l_InputProfiles[NUM_CONTROLLERS];

// whether we have mupen64plus' control info
static bool l_HasControlInfo = false;

// mupen64plus' control info
static CONTROL_INFO l_ControlInfo;

// keyboard state
static bool l_KeyboardState[SDL_NUM_SCANCODES];

//
// Local Functions
//

static void load_inputmapping_settings(InputMapping* mapping, std::string section,
    SettingsID inputNameSettingsId, SettingsID inputTypeSettingsId, 
    SettingsID dataSettingsId, SettingsID extraDataSettingsId)
{
    mapping->Name = CoreSettingsGetStringListValue(inputNameSettingsId, section);
    mapping->Type = CoreSettingsGetIntListValue(inputTypeSettingsId, section);
    mapping->Data = CoreSettingsGetIntListValue(dataSettingsId, section);
    mapping->ExtraData = CoreSettingsGetIntListValue(extraDataSettingsId, section);
    mapping->Count = std::min(mapping->Type.size(), std::min(mapping->Data.size(), mapping->ExtraData.size()));

    // check if mapping is old profile type,
    // it should be true when the minimum of the lengths
    // of all int lists is 0, and the name isn't empty or whitespace
    if (mapping->Count == 0 &&
        !mapping->Name.empty() && !mapping->Name.at(0).empty() &&
        mapping->Name.at(0).find_first_not_of(' ') != std::string::npos)
    {
        mapping->Type.push_back(CoreSettingsGetIntValue(inputTypeSettingsId, section));
        mapping->Data.push_back(CoreSettingsGetIntValue(dataSettingsId, section));
        mapping->ExtraData.push_back(CoreSettingsGetIntValue(extraDataSettingsId, section));
        mapping->Count = 1;
    }
}

static void load_settings(void)
{
    std::string gameId;
    CoreRomSettings romSettings;

    std::string userProfileSectionBase = "Rosalie's Mupen GUI - Input Plugin User Profile";
    std::vector<std::string> userProfiles;
    std::vector<std::string>::iterator userProfilesIter;

    // try to retrieve current ROM settings
    if (CoreGetCurrentRomSettings(romSettings))
    {
        gameId = romSettings.MD5;
    }

    // retrieve all user profiles
    userProfiles = CoreSettingsGetStringListValue(SettingsID::Input_Profiles);

    for (int i = 0; i < NUM_CONTROLLERS; i++)
    {
        InputProfile* profile = &l_InputProfiles[i];
        std::string section     = "Rosalie's Mupen GUI - Input Plugin Profile " + std::to_string(i);
        std::string gameSection = section + " Game " + gameId;
        std::string userProfileSection;
        std::string userProfileName;

        // when a user profile has been specified,
        // ensure it exists in the profile list and
        // in the settings, if it does,
        // use that user profile
        userProfileName = CoreSettingsGetStringValue(SettingsID::Input_UseProfile, section);
        if (!userProfileName.empty())
        {
            userProfilesIter = std::find(userProfiles.begin(), userProfiles.end(), userProfileName);
            if (userProfilesIter != userProfiles.end())
            {
                userProfileSection = userProfileSectionBase;
                userProfileSection += " \"" + userProfileName + "\"";
                if (CoreSettingsSectionExists(userProfileSection))
                {
                    section = userProfileSection;
                }
            }
        }

        // if game ID was retrieved,
        // check if game section exists,
        // if it does, check if the 'UseGameProfile' key exists,
        // if it doesn't then use the profile, else
        // check if the value is true
        if (!gameId.empty())
        {
            if (CoreSettingsSectionExists(gameSection))
            {
                if (!CoreSettingsKeyExists(gameSection, "UseGameProfile") ||
                    CoreSettingsGetBoolValue(SettingsID::Input_UseGameProfile, gameSection))
                {
                    section = gameSection;
                }
            }
        }

        // when the settings section doesn't exist,
        // disable profile
        if (!CoreSettingsSectionExists(section))
        {
            profile->PluggedIn = false;
            continue;
        }

        profile->PluggedIn = CoreSettingsGetBoolValue(SettingsID::Input_PluggedIn, section);
        profile->DeadzoneValue = CoreSettingsGetIntValue(SettingsID::Input_Deadzone, section);
        profile->ControllerPak = (N64ControllerPak)CoreSettingsGetIntValue(SettingsID::Input_Pak, section);
        profile->DeviceName = CoreSettingsGetStringValue(SettingsID::Input_DeviceName, section);
        profile->DeviceNum = CoreSettingsGetIntValue(SettingsID::Input_DeviceNum, section);
        profile->GameboyRom = CoreSettingsGetStringValue(SettingsID::Input_GameboyRom, section);
        profile->GameboySave = CoreSettingsGetStringValue(SettingsID::Input_GameboySave, section);

        // keep compatibility with profiles before version v0.3.9
        if (CoreSettingsKeyExists(section, "Sensitivity"))
        {
            profile->SensitivityValue = CoreSettingsGetIntValue(SettingsID::Input_Sensitivity, section);
        }
        else
        {
            profile->SensitivityValue = 100;
        }

        // load inputmapping settings
        load_inputmapping_settings(&profile->Button_A, section, SettingsID::Input_A_Name, SettingsID::Input_A_InputType, SettingsID::Input_A_Data, SettingsID::Input_A_ExtraData);
        load_inputmapping_settings(&profile->Button_B, section, SettingsID::Input_B_Name, SettingsID::Input_B_InputType, SettingsID::Input_B_Data, SettingsID::Input_B_ExtraData);
        load_inputmapping_settings(&profile->Button_Start, section, SettingsID::Input_Start_Name, SettingsID::Input_Start_InputType, SettingsID::Input_Start_Data, SettingsID::Input_Start_ExtraData);
        load_inputmapping_settings(&profile->Button_DpadUp, section, SettingsID::Input_DpadUp_Name, SettingsID::Input_DpadUp_InputType, SettingsID::Input_DpadUp_Data, SettingsID::Input_DpadUp_ExtraData);
        load_inputmapping_settings(&profile->Button_DpadDown, section, SettingsID::Input_DpadDown_Name, SettingsID::Input_DpadDown_InputType, SettingsID::Input_DpadDown_Data, SettingsID::Input_DpadDown_ExtraData);
        load_inputmapping_settings(&profile->Button_DpadLeft, section, SettingsID::Input_DpadLeft_Name, SettingsID::Input_DpadLeft_InputType, SettingsID::Input_DpadLeft_Data, SettingsID::Input_DpadLeft_ExtraData);
        load_inputmapping_settings(&profile->Button_DpadRight, section, SettingsID::Input_DpadRight_Name, SettingsID::Input_DpadRight_InputType, SettingsID::Input_DpadRight_Data, SettingsID::Input_DpadRight_ExtraData);
        load_inputmapping_settings(&profile->Button_CButtonUp, section, SettingsID::Input_CButtonUp_Name, SettingsID::Input_CButtonUp_InputType, SettingsID::Input_CButtonUp_Data, SettingsID::Input_CButtonUp_ExtraData);
        load_inputmapping_settings(&profile->Button_CButtonDown, section, SettingsID::Input_CButtonDown_Name, SettingsID::Input_CButtonDown_InputType, SettingsID::Input_CButtonDown_Data, SettingsID::Input_CButtonDown_ExtraData);
        load_inputmapping_settings(&profile->Button_CButtonLeft, section, SettingsID::Input_CButtonLeft_Name, SettingsID::Input_CButtonLeft_InputType, SettingsID::Input_CButtonLeft_Data, SettingsID::Input_CButtonLeft_ExtraData);
        load_inputmapping_settings(&profile->Button_CButtonRight, section, SettingsID::Input_CButtonRight_Name, SettingsID::Input_CButtonRight_InputType, SettingsID::Input_CButtonRight_Data, SettingsID::Input_CButtonRight_ExtraData);
        load_inputmapping_settings(&profile->Button_LeftTrigger, section, SettingsID::Input_LeftTrigger_Name, SettingsID::Input_LeftTrigger_InputType, SettingsID::Input_LeftTrigger_Data, SettingsID::Input_LeftTrigger_ExtraData);
        load_inputmapping_settings(&profile->Button_RightTrigger, section, SettingsID::Input_RightTrigger_Name, SettingsID::Input_RightTrigger_InputType, SettingsID::Input_RightTrigger_Data, SettingsID::Input_RightTrigger_ExtraData);
        load_inputmapping_settings(&profile->Button_ZTrigger, section, SettingsID::Input_ZTrigger_Name, SettingsID::Input_ZTrigger_InputType, SettingsID::Input_ZTrigger_Data, SettingsID::Input_ZTrigger_ExtraData);
        load_inputmapping_settings(&profile->AnalogStick_Up, section, SettingsID::Input_AnalogStickUp_Name, SettingsID::Input_AnalogStickUp_InputType, SettingsID::Input_AnalogStickUp_Data, SettingsID::Input_AnalogStickUp_ExtraData);
        load_inputmapping_settings(&profile->AnalogStick_Down, section, SettingsID::Input_AnalogStickDown_Name, SettingsID::Input_AnalogStickDown_InputType, SettingsID::Input_AnalogStickDown_Data, SettingsID::Input_AnalogStickDown_ExtraData);
        load_inputmapping_settings(&profile->AnalogStick_Left, section, SettingsID::Input_AnalogStickLeft_Name, SettingsID::Input_AnalogStickLeft_InputType, SettingsID::Input_AnalogStickLeft_Data, SettingsID::Input_AnalogStickLeft_ExtraData);
        load_inputmapping_settings(&profile->AnalogStick_Right, section, SettingsID::Input_AnalogStickRight_Name, SettingsID::Input_AnalogStickRight_InputType, SettingsID::Input_AnalogStickRight_Data, SettingsID::Input_AnalogStickRight_ExtraData);
    }
}

static void apply_controller_profiles(void)
{
    // only apply profile when 
    // we have the control info
    if (!l_HasControlInfo)
    {
        return;
    }

    for (int i = 0; i < NUM_CONTROLLERS; i++)
    {
        InputProfile* profile = &l_InputProfiles[i];
        int plugin = PLUGIN_NONE;

        switch (profile->ControllerPak)
        {
            case N64ControllerPak::MemoryPak:
                plugin = PLUGIN_MEMPAK;
                break;
            case N64ControllerPak::RumblePak:
                plugin = PLUGIN_RAW;
                break;
             case N64ControllerPak::TransferPak:
                plugin = PLUGIN_TRANSFER_PAK;
                break;
            default:
                plugin = PLUGIN_NONE;
                break;
        }

        l_ControlInfo.Controls[i].Present = profile->PluggedIn ? 1 : 0;
        l_ControlInfo.Controls[i].Plugin  = plugin;
        l_ControlInfo.Controls[i].RawData = 0;
        l_ControlInfo.Controls[i].Type    = CONT_TYPE_STANDARD;
    }
}

static void apply_gameboy_settings(void)
{
    SettingsID gameBoyRomSettings[] =
    {
        SettingsID::Core_Gameboy_P1_Rom,
        SettingsID::Core_Gameboy_P2_Rom,
        SettingsID::Core_Gameboy_P3_Rom,
        SettingsID::Core_Gameboy_P4_Rom,
    };

    SettingsID gameBoySaveSettings[] =
    {
        SettingsID::Core_Gameboy_P1_Save,
        SettingsID::Core_Gameboy_P2_Save,
        SettingsID::Core_Gameboy_P3_Save,
        SettingsID::Core_Gameboy_P4_Save,
    };

    for (int i = 0; i < NUM_CONTROLLERS; i++)
    {
        InputProfile* profile = &l_InputProfiles[i];

        CoreSettingsSetValue(gameBoyRomSettings[i], profile->GameboyRom);
        CoreSettingsSetValue(gameBoySaveSettings[i], profile->GameboySave);
    }

    CoreSettingsSave();
}

static void setup_device_automatic(int num, InputProfile* profile)
{
    static int previousSdlDeviceNum = -1;

    // reset variable when needed
    if (num == 0)
    {
        previousSdlDeviceNum = -1;
    }

    bool foundSdlDevice                   = false;
    SDL_GameController* sdlGameController = nullptr;
    SDL_Joystick* sdlJoystick             = nullptr;
    std::string sdlDeviceName;

    for (int i = (previousSdlDeviceNum + 1); i < SDL_NumJoysticks(); i++)
    {
        if (SDL_IsGameController(i))
        {
            sdlGameController = SDL_GameControllerOpen(i);
            if (sdlGameController != nullptr)
            {
                sdlDeviceName = SDL_GameControllerName(sdlGameController);
                SDL_GameControllerClose(sdlGameController);
                profile->DeviceNum   = i;
                profile->DeviceName  = sdlDeviceName;
                previousSdlDeviceNum = i;
                foundSdlDevice       = true; 
                break;
            }
        }
        else
        {
            sdlJoystick = SDL_JoystickOpen(i);
            if (sdlJoystick != nullptr)
            {
                sdlDeviceName = SDL_JoystickName(sdlJoystick);
                SDL_JoystickClose(sdlJoystick);
                profile->DeviceNum   = i;
                profile->DeviceName  = sdlDeviceName;
                previousSdlDeviceNum = i;
                foundSdlDevice       = true; 
                break;
            }
        }
    }

    if (!foundSdlDevice)
    { // fallback to keyboard
        if (num == 0)
        {
            profile->DeviceNum = (int)InputDeviceType::Keyboard;
        }
        else
        {
            profile->PluggedIn = false;

            // only override present in core
            // when we have the control info
            if (l_HasControlInfo)
            {
                l_ControlInfo.Controls[num].Present = 0;
            }
        }
    }
}

static void open_controllers(void)
{
    // force re-fresh joystick list
    SDL_JoystickUpdate();

    for (int i = 0; i < NUM_CONTROLLERS; i++)
    {
        InputProfile* profile = &l_InputProfiles[i];

        profile->InputDevice.CloseDevice();

        if (profile->DeviceNum == (int)InputDeviceType::Automatic)
        {
            setup_device_automatic(i, profile);
        }

        if (profile->DeviceNum != (int)InputDeviceType::Keyboard)
        {
            profile->InputDevice.OpenDevice(profile->DeviceName, profile->DeviceNum);
        }
    }
}

static void close_controllers(void)
{
    for (int i = 0; i < NUM_CONTROLLERS; i++)
    {
        InputProfile* profile = &l_InputProfiles[i];

        profile->InputDevice.CloseDevice();
    }
}

static int get_button_state(InputProfile* profile, const InputMapping* inputMapping)
{
    int state = 0;

    for (int i = 0; i < inputMapping->Count; i++)
    {
        const int data = inputMapping->Data.at(i);
        const int extraData = inputMapping->ExtraData.at(i);

        switch ((InputType)inputMapping->Type[i])
        {
            case InputType::GamepadButton:
            {
                state |= SDL_GameControllerGetButton(profile->InputDevice.GetGameControllerHandle(), (SDL_GameControllerButton)data);
            } break;
            case InputType::GamepadAxis:
            {
                int axis_value = SDL_GameControllerGetAxis(profile->InputDevice.GetGameControllerHandle(), (SDL_GameControllerAxis)data);
                state |= (abs(axis_value) >= (SDL_AXIS_PEAK / 2) && (extraData ? axis_value > 0 : axis_value < 0)) ? 1 : 0;
            } break;
            case InputType::JoystickButton:
            {
                state |= SDL_JoystickGetButton(profile->InputDevice.GetJoystickHandle(), data);
            } break;
            case InputType::JoystickAxis:
            {
                int axis_value = SDL_JoystickGetAxis(profile->InputDevice.GetJoystickHandle(), data);
                state |= (abs(axis_value) >= (SDL_AXIS_PEAK / 2) && (extraData ? axis_value > 0 : axis_value < 0)) ? 1 : 0;
            } break;
            case InputType::Keyboard:
            {
                state |= l_KeyboardState[data] ? 1 : 0;
            } break;
            default:
                break;
        }
    }

    return state;
}

// returns axis input scaled to the range [-1, 1]
static double get_axis_state(InputProfile* profile, const InputMapping* inputMapping, const int direction, const double value, bool& useButtonMapping)
{
    double axis_state   = value;
    bool   button_state = false;

    for (int i = 0; i < inputMapping->Count; i++)
    {
        const int data = inputMapping->Data.at(i);
        const int extraData = inputMapping->ExtraData.at(i);

        switch ((InputType)inputMapping->Type[i])
        {
            case InputType::GamepadButton:
            {
                button_state |= SDL_GameControllerGetButton(profile->InputDevice.GetGameControllerHandle(), (SDL_GameControllerButton)data);;
            } break;
            case InputType::GamepadAxis:
            {
                double axis_value = SDL_GameControllerGetAxis(profile->InputDevice.GetGameControllerHandle(), (SDL_GameControllerAxis)data);
                if (extraData ? axis_value > 0 : axis_value < 0)
                {
                    axis_value = (axis_value / SDL_AXIS_PEAK);
                    axis_value = abs(axis_value) * direction;
                    axis_state = axis_value;
                }
            } break;
            case InputType::JoystickButton:
            {
                button_state |= SDL_JoystickGetButton(profile->InputDevice.GetJoystickHandle(), data);;
            } break;
            case InputType::JoystickAxis:
            {
                double axis_value = SDL_JoystickGetAxis(profile->InputDevice.GetJoystickHandle(), data);
                if (extraData ? axis_value > 0 : axis_value < 0)
                {
                    axis_value = (axis_value / SDL_AXIS_PEAK);
                    axis_value = abs(axis_value) * direction;
                    axis_state = axis_value;
                }
            } break;
            case InputType::Keyboard:
            {
                button_state |= l_KeyboardState[data];
            } break;
            default:
                break;
        }
    }

    // when a button has been mapped
    // to an axis, we should prioritize 
    // the button when it's been pressed
    if (button_state)
    {
        useButtonMapping = true;
        return direction;
    }
    else if (!useButtonMapping)
    {
        return axis_state;
    } 
    else
    {
        return value;
    }
}

static double simulate_deadzone(const double n64InputAxis, const double maxAxis, const int deadzone, const double axisRange)
{
    double axisAbsolute = std::abs(n64InputAxis);

    if (axisAbsolute <= deadzone)
    {
        axisAbsolute = 0; // No input when inside deadzone
    }
    else
    {
        // Create linear scaling factor from 0 at inner deadzone to MAX_AXIS at outer limit
        axisAbsolute = (axisAbsolute - deadzone) * maxAxis / axisRange / axisAbsolute;
    }

    return axisAbsolute;
}

// Credit: MerryMage & fzurita
static void simulate_octagon(const double inputX, const double inputY, const double deadzoneFactor, const double sensitivityFactor, int& outputX, int& outputY)
{
    // don't increase emulated range at higher than 100% sensitivity
    const double maxAxis     = N64_AXIS_PEAK * std::min(sensitivityFactor, 1.0);
    const double maxDiagonal = MAX_DIAGONAL_VALUE * std::min(sensitivityFactor, 1.0);
    const int    deadzone    = (int)(deadzoneFactor * N64_AXIS_PEAK * sensitivityFactor);
    const double axisRange   = maxAxis - deadzone;
    // scale to [-maxAxis, maxAxis]
    double ax = std::min(inputX * sensitivityFactor, 1.0) * maxAxis;
    double ay = std::min(inputY * sensitivityFactor, 1.0) * maxAxis;

    // check whether (ax, ay) is within the circle of radius MAX_AXIS
    double len = std::sqrt(ax*ax + ay*ay);
    if (len <= maxAxis)
    {
        // scale inputs
        ax *= simulate_deadzone(ax, maxAxis, deadzone, axisRange);
        ay *= simulate_deadzone(ay, maxAxis, deadzone, axisRange);
    }
    else
    {
        // scale ax and ay to stay on the same line, but at the edge of the circle
        len = maxAxis / len;
        ax *= len;
        ay *= len;
    }

    // bound diagonals to an octagonal range [-69, 69]
    if (ax != 0.0 && ay != 0.0)
    {
        const double slope = ay / ax;
        double edgex = std::copysign(maxAxis / (std::abs(slope) + (maxAxis - maxDiagonal) / maxDiagonal), ax);
        const double edgey = std::copysign(std::min(std::abs(edgex * slope), maxAxis / (1.0 / std::abs(slope) + (maxAxis - maxDiagonal) / maxDiagonal)), ay);
        edgex = edgey / slope;

        const double scale = std::sqrt(edgex*edgex + edgey*edgey) / maxAxis;
        ax *= scale;
        ay *= scale;
    }

    outputX = (int)ax;
    outputY = (int)ay;
}

static unsigned char data_crc(unsigned char *data, int length)
{
    unsigned char remainder = data[0];

    int byte = 1;
    unsigned char bit = 0;

    while (byte <= length)
    {
        int highBit = ((remainder & 0x80) != 0);
        remainder = remainder << 1;

        remainder += (byte < length && data[byte] & (0x80 >> bit )) ? 1 : 0;

        remainder ^= (highBit) ? 0x85 : 0;

        bit++;
        byte += bit/8;
        bit %= 8;
    }

    return remainder;
}

void sdl_init()
{
    std::filesystem::path gameControllerDbPath;

    for (const int subsystem : {SDL_INIT_GAMECONTROLLER, SDL_INIT_VIDEO, SDL_INIT_HAPTIC})
    {
        if (!SDL_WasInit(subsystem))
        {
            SDL_InitSubSystem(subsystem);
        }
    }

    gameControllerDbPath = CoreGetSharedDataDirectory();
    gameControllerDbPath += "/gamecontrollerdb.txt";

    // load mappings from file
    SDL_GameControllerAddMappingsFromFile(gameControllerDbPath.string().c_str());
}

void sdl_quit()
{
    for (const int subsystem : {SDL_INIT_GAMECONTROLLER, SDL_INIT_HAPTIC})
    {
        if (SDL_WasInit(subsystem))
        {
            SDL_QuitSubSystem(subsystem);
        }
    }
}

//
// Basic Plugin Functions
//

EXPORT m64p_error CALL PluginStartup(m64p_dynlib_handle CoreLibHandle, void *Context, void (*DebugCallback)(void *, int, const char *))
{
    if (l_SDLThread != nullptr)
    {
        return M64ERR_ALREADY_INIT;
    }

    if (!CoreInit(CoreLibHandle))
    {
        return M64ERR_SYSTEM_FAIL;
    }

    sdl_init();

    l_SDLThread = new Thread::SDLThread(nullptr);
    l_SDLThread->start();

    for (int i = 0; i < NUM_CONTROLLERS; i++)
    {
        l_InputProfiles[i].InputDevice.SetSDLThread(l_SDLThread);
    }

    load_settings();

    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginShutdown(void)
{
    if (l_SDLThread == nullptr)
    {
        return M64ERR_NOT_INIT;
    }

    close_controllers();

    l_SDLThread->StopLoop();
    l_SDLThread->deleteLater();
    l_SDLThread = nullptr;

    sdl_quit();

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
        *pluginNamePtr = "Rosalie's Mupen GUI - Input Plugin";
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

EXPORT m64p_error CALL PluginConfig2(int romConfig)
{
    if (l_SDLThread == nullptr)
    {
        return M64ERR_NOT_INIT;
    }

    // close controllers
    close_controllers();

    l_SDLThread->SetAction(SDLThreadAction::SDLPumpEvents);

    UserInterface::MainDialog dialog(nullptr, l_SDLThread, romConfig);
    dialog.exec();

    l_SDLThread->SetAction(SDLThreadAction::None);

    // wait until it's done pumping events
    while (l_SDLThread->GetCurrentAction() == SDLThreadAction::SDLPumpEvents)
    {
        QThread::msleep(5);
    }

    // reload settings
    load_settings();

    // apply profiles
    apply_controller_profiles();

    // apply gameboy settings
    apply_gameboy_settings();

    // open controllers
    open_controllers();

    return M64ERR_SUCCESS;
}

EXPORT int CALL PluginConfig2HasRomConfig(void)
{
    return 1;
}

//
// Input Plugin Functions
//

EXPORT void CALL ControllerCommand(int Control, unsigned char* Command)
{
    unsigned char* data = &Command[5];

    if (Control == -1)
    {
        return;
    }

    InputProfile* profile = &l_InputProfiles[Control];

    switch (Command[2])
    {
        case RD_GETSTATUS:
        case RD_READKEYS:
            break;
        case RD_READPAK:
            if (profile->ControllerPak == N64ControllerPak::RumblePak)
            {
                unsigned int dwAddress = (Command[3] << 8) + (Command[4] & 0xE0);

                if ((dwAddress >= 0x8000 ) && (dwAddress < 0x9000 ))
                {
                    memset(data, 0x80, 32);
                }
                else
                {
                    memset(data, 0x00, 32);
                }

                data[32] = data_crc(data, 32);
            }
            break;
        case RD_WRITEPAK:
            if (profile->ControllerPak == N64ControllerPak::RumblePak)
            {
                unsigned int dwAddress = (Command[3] << 8) + (Command[4] & 0xE0);
                if (dwAddress == PAK_IO_RUMBLE) 
                {
                    if (*data) 
                    {
                        profile->InputDevice.StartRumble();
                    }
                    else
                    {
                        profile->InputDevice.StopRumble();
                    }
                }
                data[32] = data_crc( data, 32 );
            }
            break;
        case RD_RESETCONTROLLER:
        case RD_READEEPROM:
        case RD_WRITEEPROM:
            break;
    }
}

EXPORT void CALL GetKeys(int Control, BUTTONS* Keys)
{
    InputProfile* profile = &l_InputProfiles[Control];

    if (!profile->PluggedIn)
    {
        return;
    }

    // check if device has been disconnected,
    // if it has, try to open it again,
    // only do this every 2 seconds to prevent lag
    const auto currentTime  = std::chrono::high_resolution_clock::now();
    const int secondsPassed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - profile->LastDeviceCheckTime).count();
    if (secondsPassed >= 2)
    {
        profile->LastDeviceCheckTime = currentTime;

        if (profile->DeviceNum != (int)InputDeviceType::Keyboard)
        {
            if (profile->InputDevice.IsOpeningDevice())
            {
                return;
            }

            if (!profile->InputDevice.HasOpenDevice() || !profile->InputDevice.IsAttached())
            {
                profile->InputDevice.OpenDevice(profile->DeviceName, profile->DeviceNum);
            }
        }
    }

    Keys->A_BUTTON     = get_button_state(profile, &profile->Button_A);
    Keys->B_BUTTON     = get_button_state(profile, &profile->Button_B);
    Keys->START_BUTTON = get_button_state(profile, &profile->Button_Start);
    Keys->U_DPAD       = get_button_state(profile, &profile->Button_DpadUp);
    Keys->D_DPAD       = get_button_state(profile, &profile->Button_DpadDown);
    Keys->L_DPAD       = get_button_state(profile, &profile->Button_DpadLeft);
    Keys->R_DPAD       = get_button_state(profile, &profile->Button_DpadRight);
    Keys->U_CBUTTON    = get_button_state(profile, &profile->Button_CButtonUp);
    Keys->D_CBUTTON    = get_button_state(profile, &profile->Button_CButtonDown);
    Keys->L_CBUTTON    = get_button_state(profile, &profile->Button_CButtonLeft);
    Keys->R_CBUTTON    = get_button_state(profile, &profile->Button_CButtonRight);
    Keys->L_TRIG       = get_button_state(profile, &profile->Button_LeftTrigger);
    Keys->R_TRIG       = get_button_state(profile, &profile->Button_RightTrigger);
    Keys->Z_TRIG       = get_button_state(profile, &profile->Button_ZTrigger);

    double inputX = 0, inputY = 0;
    bool useButtonMapping = false;
    inputY = get_axis_state(profile, &profile->AnalogStick_Up,    1, inputY, useButtonMapping);
    inputY = get_axis_state(profile, &profile->AnalogStick_Down, -1, inputY, useButtonMapping);
    inputX = get_axis_state(profile, &profile->AnalogStick_Left, -1, inputX, useButtonMapping);
    inputX = get_axis_state(profile, &profile->AnalogStick_Right, 1, inputX, useButtonMapping);

    int octagonX = 0, octagonY = 0;
    simulate_octagon(
        inputX, // inputX
        inputY, // inputY
        profile->DeadzoneValue / 100.0, // deadzoneFactor
        profile->SensitivityValue / 100.0, // sensitivityFactor
        octagonX, // outputX
        octagonY  // outputY
    );

    Keys->X_AXIS = octagonX;
    Keys->Y_AXIS = octagonY;
}

EXPORT void CALL InitiateControllers(CONTROL_INFO ControlInfo)
{
    for (int i = 0; i < SDL_NUM_SCANCODES; i++)
    {
        l_KeyboardState[i] = 0;
    }

    l_ControlInfo    = ControlInfo;
    l_HasControlInfo = true;

    // load settings
    load_settings();
    // apply profiles
    apply_controller_profiles();
    // apply gameboy settings
    apply_gameboy_settings();
    // open controllers
    open_controllers();
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
    l_HasControlInfo = false;
    close_controllers();
}

EXPORT void CALL SDL_KeyDown(int keymod, int keysym)
{
    l_KeyboardState[keysym] = true;
}

EXPORT void CALL SDL_KeyUp(int keymod, int keysym)
{
    l_KeyboardState[keysym] = false;
}
