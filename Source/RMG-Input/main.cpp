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
#include "Thread/HotkeysThread.hpp"
#include "Utilities/InputDevice.hpp"
#include "common.hpp"
#include "main.hpp"
#ifdef VRU
#include "VRU.hpp"
#endif // VRU

#include <RMG-Core/Core.hpp>

#include <QGuiApplication>
#include <QApplication>
#include <SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>

//
// Local Defines
//

#define NUM_CONTROLLERS    4
#define N64_AXIS_PEAK      85
#define MAX_DIAGONAL_VALUE 69

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

    InputDeviceType DeviceType = InputDeviceType::Mouse;

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

    // hotkeys
    bool Hotkey_Shutdown_Pressed = false;
    InputMapping Hotkey_Shutdown;
    bool Hotkey_Exit_Pressed = false;
    InputMapping Hotkey_Exit;
    bool Hotkey_SoftReset_Pressed = false;
    InputMapping Hotkey_SoftReset;
    bool Hotkey_HardReset_Pressed = false;
    InputMapping Hotkey_HardReset;
    bool Hotkey_Resume_Pressed = false;
    InputMapping Hotkey_Resume;
    bool Hotkey_Screenshot_Pressed = false;
    InputMapping Hotkey_Screenshot;
    bool Hotkey_LimitFPS_Pressed = false;
    InputMapping Hotkey_LimitFPS;
    bool Hotkey_SpeedFactor_Pressed = false;
    InputMapping Hotkey_SpeedFactor25;
    InputMapping Hotkey_SpeedFactor50;
    InputMapping Hotkey_SpeedFactor75;
    InputMapping Hotkey_SpeedFactor100;
    InputMapping Hotkey_SpeedFactor125;
    InputMapping Hotkey_SpeedFactor150;
    InputMapping Hotkey_SpeedFactor175;
    InputMapping Hotkey_SpeedFactor200;
    InputMapping Hotkey_SpeedFactor225;
    InputMapping Hotkey_SpeedFactor250;
    InputMapping Hotkey_SpeedFactor275;
    InputMapping Hotkey_SpeedFactor300;
    bool Hotkey_SaveState_Pressed = false;
    InputMapping Hotkey_SaveState;
    bool Hotkey_LoadState_Pressed = false;
    InputMapping Hotkey_LoadState;
    bool Hotkey_GSButton_Pressed = false;
    InputMapping Hotkey_GSButton;
    bool Hotkey_SaveStateSlot_Pressed = false;
    InputMapping Hotkey_SaveStateSlot0;
    InputMapping Hotkey_SaveStateSlot1;
    InputMapping Hotkey_SaveStateSlot2;
    InputMapping Hotkey_SaveStateSlot3;
    InputMapping Hotkey_SaveStateSlot4;
    InputMapping Hotkey_SaveStateSlot5;
    InputMapping Hotkey_SaveStateSlot6;
    InputMapping Hotkey_SaveStateSlot7;
    InputMapping Hotkey_SaveStateSlot8;
    InputMapping Hotkey_SaveStateSlot9;
    bool Hotkey_IncreaseSaveStateSlot_Pressed = false;
    InputMapping Hotkey_IncreaseSaveStateSlot;
    bool Hotkey_DecreaseSaveStateSlot_Pressed = false;
    InputMapping Hotkey_DecreaseSaveStateSlot;
    bool Hotkey_MemoryPak_Pressed = false;
    InputMapping Hotkey_MemoryPak;
    bool Hotkey_RumblePak_Pressed = false;
    InputMapping Hotkey_RumblePak;
    bool Hotkey_NoPak_Pressed = false;
    InputMapping Hotkey_NoPak;
    bool Hotkey_Fullscreen_Pressed = false;
    InputMapping Hotkey_Fullscreen;
};

struct MouseMovement
{
    int x;
    int y;
};

//
// Local variables
//

// SDL thread
static Thread::SDLThread *l_SDLThread = nullptr;

// Hotkeys thread (for when paused)
static Thread::HotkeysThread *l_HotkeysThread = nullptr;

// input profiles
static InputProfile l_InputProfiles[NUM_CONTROLLERS];

// whether we have mupen64plus' control info
static bool l_HasControlInfo = false;

// mupen64plus control info
static CONTROL_INFO l_ControlInfo;

// mupen64plus debug callback
static void (*l_DebugCallback)(void *, int, const char *) = nullptr;
static void *l_DebugCallContext                           = nullptr;

// keyboard state
static bool l_KeyboardState[SDL_NUM_SCANCODES];

// mouse movements
static std::mutex l_MouseMutex;
static std::vector<MouseMovement> l_MouseMovements;
static bool l_MouseButtonState[2];
static ptr_ResetMousePositionCallback l_ResetMousPositionCallback = nullptr;

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
        profile->DeviceType = (InputDeviceType)CoreSettingsGetIntValue(SettingsID::Input_InputType, section);
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

#define LOAD_INPUT_MAPPING(mapping, setting) \
        load_inputmapping_settings(&profile->mapping, section, SettingsID::setting##_Name, SettingsID::setting##_InputType, SettingsID::setting##_Data, SettingsID::setting##_ExtraData)

        // load inputmapping settings
        LOAD_INPUT_MAPPING(Button_A,            Input_A);
        LOAD_INPUT_MAPPING(Button_B,            Input_B);
        LOAD_INPUT_MAPPING(Button_Start,        Input_Start);
        LOAD_INPUT_MAPPING(Button_DpadUp,       Input_DpadUp);
        LOAD_INPUT_MAPPING(Button_DpadDown,     Input_DpadDown);
        LOAD_INPUT_MAPPING(Button_DpadLeft,     Input_DpadLeft);
        LOAD_INPUT_MAPPING(Button_DpadRight,    Input_DpadRight);
        LOAD_INPUT_MAPPING(Button_CButtonUp,    Input_CButtonUp);
        LOAD_INPUT_MAPPING(Button_CButtonDown,  Input_CButtonDown);
        LOAD_INPUT_MAPPING(Button_CButtonLeft,  Input_CButtonLeft);
        LOAD_INPUT_MAPPING(Button_CButtonRight, Input_CButtonRight);
        LOAD_INPUT_MAPPING(Button_LeftTrigger,  Input_LeftTrigger);
        LOAD_INPUT_MAPPING(Button_RightTrigger, Input_RightTrigger);
        LOAD_INPUT_MAPPING(Button_ZTrigger,     Input_ZTrigger);
        LOAD_INPUT_MAPPING(AnalogStick_Up,      Input_AnalogStickUp);
        LOAD_INPUT_MAPPING(AnalogStick_Down,    Input_AnalogStickDown);
        LOAD_INPUT_MAPPING(AnalogStick_Left,    Input_AnalogStickLeft);
        LOAD_INPUT_MAPPING(AnalogStick_Right,   Input_AnalogStickRight);

        // load hotkeys settings
        LOAD_INPUT_MAPPING(Hotkey_Shutdown,       Input_Hotkey_Shutdown);
        LOAD_INPUT_MAPPING(Hotkey_Exit,           Input_Hotkey_Exit);
        LOAD_INPUT_MAPPING(Hotkey_SoftReset,      Input_Hotkey_SoftReset);
        LOAD_INPUT_MAPPING(Hotkey_HardReset,      Input_Hotkey_HardReset);
        LOAD_INPUT_MAPPING(Hotkey_Resume,         Input_Hotkey_Resume);
        LOAD_INPUT_MAPPING(Hotkey_Screenshot,     Input_Hotkey_Screenshot);
        LOAD_INPUT_MAPPING(Hotkey_LimitFPS,       Input_Hotkey_LimitFPS);
        LOAD_INPUT_MAPPING(Hotkey_SpeedFactor25,  Input_Hotkey_SpeedFactor25);
        LOAD_INPUT_MAPPING(Hotkey_SpeedFactor50,  Input_Hotkey_SpeedFactor50);
        LOAD_INPUT_MAPPING(Hotkey_SpeedFactor75,  Input_Hotkey_SpeedFactor75);
        LOAD_INPUT_MAPPING(Hotkey_SpeedFactor100, Input_Hotkey_SpeedFactor100);
        LOAD_INPUT_MAPPING(Hotkey_SpeedFactor125, Input_Hotkey_SpeedFactor125);
        LOAD_INPUT_MAPPING(Hotkey_SpeedFactor150, Input_Hotkey_SpeedFactor150);
        LOAD_INPUT_MAPPING(Hotkey_SpeedFactor175, Input_Hotkey_SpeedFactor175);
        LOAD_INPUT_MAPPING(Hotkey_SpeedFactor200, Input_Hotkey_SpeedFactor200);
        LOAD_INPUT_MAPPING(Hotkey_SpeedFactor225, Input_Hotkey_SpeedFactor225);
        LOAD_INPUT_MAPPING(Hotkey_SpeedFactor250, Input_Hotkey_SpeedFactor250);
        LOAD_INPUT_MAPPING(Hotkey_SpeedFactor275, Input_Hotkey_SpeedFactor275);
        LOAD_INPUT_MAPPING(Hotkey_SpeedFactor300, Input_Hotkey_SpeedFactor300);
        LOAD_INPUT_MAPPING(Hotkey_SaveState,      Input_Hotkey_SaveState);
        LOAD_INPUT_MAPPING(Hotkey_LoadState,      Input_Hotkey_LoadState);
        LOAD_INPUT_MAPPING(Hotkey_GSButton,       Input_Hotkey_GSButton);
        LOAD_INPUT_MAPPING(Hotkey_SaveStateSlot0, Input_Hotkey_SaveStateSlot0);
        LOAD_INPUT_MAPPING(Hotkey_SaveStateSlot1, Input_Hotkey_SaveStateSlot1);
        LOAD_INPUT_MAPPING(Hotkey_SaveStateSlot2, Input_Hotkey_SaveStateSlot2);
        LOAD_INPUT_MAPPING(Hotkey_SaveStateSlot3, Input_Hotkey_SaveStateSlot3);
        LOAD_INPUT_MAPPING(Hotkey_SaveStateSlot4, Input_Hotkey_SaveStateSlot4);
        LOAD_INPUT_MAPPING(Hotkey_SaveStateSlot5, Input_Hotkey_SaveStateSlot5);
        LOAD_INPUT_MAPPING(Hotkey_SaveStateSlot6, Input_Hotkey_SaveStateSlot6);
        LOAD_INPUT_MAPPING(Hotkey_SaveStateSlot7, Input_Hotkey_SaveStateSlot7);
        LOAD_INPUT_MAPPING(Hotkey_SaveStateSlot8, Input_Hotkey_SaveStateSlot8);
        LOAD_INPUT_MAPPING(Hotkey_SaveStateSlot9, Input_Hotkey_SaveStateSlot9);
        LOAD_INPUT_MAPPING(Hotkey_IncreaseSaveStateSlot, Input_Hotkey_IncreaseSaveStateSlot);
        LOAD_INPUT_MAPPING(Hotkey_DecreaseSaveStateSlot, Input_Hotkey_DecreaseSaveStateSlot);
        LOAD_INPUT_MAPPING(Hotkey_MemoryPak,  Input_Hotkey_MemoryPak);
        LOAD_INPUT_MAPPING(Hotkey_RumblePak,  Input_Hotkey_RumblePak);
        LOAD_INPUT_MAPPING(Hotkey_NoPak,      Input_Hotkey_NoPak);
        LOAD_INPUT_MAPPING(Hotkey_Fullscreen, Input_Hotkey_Fullscreen);

#undef LOAD_INPUT_MAPPING
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
        bool emulateVRU = (profile->DeviceNum == (int)InputDeviceType::EmulateVRU);

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

#ifdef VRU
        // attempt to try initializing VRU when needed,
        // if it fails, unplug the VRU
        if (emulateVRU && !IsVRUInit() && !InitVRU())
        {
            profile->PluggedIn = false;
        }
#else
        // always unplug VRU when RMG-Input was built
        // without VRU support
        if (emulateVRU)
        {
            profile->PluggedIn = false;
        }
#endif // VRU

        l_ControlInfo.Controls[i].Present = profile->PluggedIn ? 1 : 0;
        l_ControlInfo.Controls[i].Plugin  = emulateVRU ? PLUGIN_NONE : plugin;
        l_ControlInfo.Controls[i].RawData = 0;
        l_ControlInfo.Controls[i].Type    = emulateVRU ? CONT_TYPE_VRU : CONT_TYPE_STANDARD;

        if (i == 0)
        {
            l_ControlInfo.Controls[i].Type = CONT_TYPE_MOUSE;
        }
        else
        {
            l_ControlInfo.Controls[i].Type = CONT_TYPE_STANDARD;
        }

        l_ControlInfo.Controls[i].Type    = (profile->DeviceType != InputDeviceType::Mouse ? CONT_TYPE_STANDARD : CONT_TYPE_MOUSE);
    }
}

static void switch_controller_pak(InputProfile* profile, const int Control, const int pak)
{
    const int currentPak = l_ControlInfo.Controls[Control].Plugin;

    // we don't need to do anything,
    // when the current pak matches
    if (currentPak == pak)
    {
        return;
    }

    l_ControlInfo.Controls[Control].Plugin = pak;

    // when switching from rumble pak to a different pak,
    // ensure that we stop the controller's rumble
    if (currentPak == PLUGIN_RAW)
    {
        profile->InputDevice.StopRumble();
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

static int get_button_state(InputProfile* profile, const InputMapping* inputMapping, const bool allPressed = false)
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
                if (allPressed && i > 0)
                {
                    state &= SDL_GameControllerGetButton(profile->InputDevice.GetGameControllerHandle(), (SDL_GameControllerButton)data);
                }
                else
                {
                    state |= SDL_GameControllerGetButton(profile->InputDevice.GetGameControllerHandle(), (SDL_GameControllerButton)data);
                }
            } break;
            case InputType::GamepadAxis:
            {
                int axis_value = SDL_GameControllerGetAxis(profile->InputDevice.GetGameControllerHandle(), (SDL_GameControllerAxis)data);
                if (allPressed && i > 0)
                {
                    state &= (abs(axis_value) >= (SDL_AXIS_PEAK / 2) && (extraData ? axis_value > 0 : axis_value < 0)) ? 1 : 0;
                }
                else
                {
                    state |= (abs(axis_value) >= (SDL_AXIS_PEAK / 2) && (extraData ? axis_value > 0 : axis_value < 0)) ? 1 : 0;
                }
            } break;
            case InputType::JoystickButton:
            {
                if (allPressed && i > 0)
                {
                    state &= SDL_JoystickGetButton(profile->InputDevice.GetJoystickHandle(), data);
                }
                else
                {
                    state |= SDL_JoystickGetButton(profile->InputDevice.GetJoystickHandle(), data);
                }
            } break;
            case InputType::JoystickHat:
            {
                if (allPressed && i > 0)
                {
                    state &= (SDL_JoystickGetHat(profile->InputDevice.GetJoystickHandle(), data) & extraData) ? 1 : 0;
                }
                else
                {
                    state |= (SDL_JoystickGetHat(profile->InputDevice.GetJoystickHandle(), data) & extraData) ? 1 : 0;
                }
            } break;
            case InputType::JoystickAxis:
            {
                int axis_value = SDL_JoystickGetAxis(profile->InputDevice.GetJoystickHandle(), data);
                if (allPressed && i > 0)
                {
                    state &= (abs(axis_value) >= (SDL_AXIS_PEAK / 2) && (extraData ? axis_value > 0 : axis_value < 0)) ? 1 : 0;
                }
                else
                {
                    state |= (abs(axis_value) >= (SDL_AXIS_PEAK / 2) && (extraData ? axis_value > 0 : axis_value < 0)) ? 1 : 0;
                }
            } break;
            case InputType::Keyboard:
            {
                if (allPressed && i > 0)
                {
                    state &= l_KeyboardState[data] ? 1 : 0;
                }
                else
                {
                    state |= l_KeyboardState[data] ? 1 : 0;
                }
            } break;
            default:
                break;
        }

        // early return when needed
        if (allPressed && !state)
        {
            return state;
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
                if (axis_value < -32767.0) axis_value = -32767.0;
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
            case InputType::JoystickHat:
            {
                button_state |= (SDL_JoystickGetHat(profile->InputDevice.GetJoystickHandle(), data) & extraData) ? 1 : 0;
            } break;
            case InputType::JoystickAxis:
            {
                double axis_value = SDL_JoystickGetAxis(profile->InputDevice.GetJoystickHandle(), data);
                if (axis_value < -32767.0) axis_value = -32767.0;
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

// maps a value in one range to a value in another
static double map_range_to_range(const double value, const double fromLower, const double fromUpper, const double toLower, const double toUpper)
{
    const double fromDelta = fromUpper - fromLower;
    const double toDelta = toUpper - toLower;
    const double toUnitsPerFromUnit = toDelta / fromDelta;
    const double fromUnits = value - fromLower;

    return toLower + fromUnits * toUnitsPerFromUnit;
}

// applies square deadzone, then scales result such that the edge of the deadzone is 0
static double apply_deadzone(const double input, const double deadzone)
{
    const double inputAbsolute = std::abs(input);

    if (inputAbsolute <= deadzone)
    {
        return 0;
    }

    return std::copysign(map_range_to_range(inputAbsolute, deadzone, 1.0, 0.0, 1.0), input);
}

// Credit: MerryMage, fzurita & kev4cards
static void simulate_octagon(const double deadzone, const double inputX, const double inputY, int& outputX, int& outputY)
{
    const double maxAxis     = N64_AXIS_PEAK;
    const double maxDiagonal = MAX_DIAGONAL_VALUE;
    const double maxInputRadius = sqrt(2) * (maxDiagonal + deadzone * (maxAxis - maxDiagonal));
    // scale to [-maxInputRadius, maxInputRadius]
    double ax = inputX * maxInputRadius;
    double ay = inputY * maxInputRadius;

    // check whether (ax, ay) is within the circle of radius maxInputRadius
    double distance = std::hypot(ax, ay);
    if (distance > maxInputRadius)
    {
        // scale ax and ay to stay on the same line, but at the edge of the circle
        const double scale = maxInputRadius / distance;
        ax *= scale;
        ay *= scale;
    }

    // bound diagonals to an octagonal range [-maxDiagonal, maxDiagonal]
    if (ax != 0.0 && ay != 0.0)
    {
        const double slope = ay / ax;
        double edgex = std::copysign(maxAxis / (std::abs(slope) + (maxAxis - maxDiagonal) / maxDiagonal), ax);
        const double edgey = std::copysign(std::min(std::abs(edgex * slope), maxAxis / (1.0 / std::abs(slope) + (maxAxis - maxDiagonal) / maxDiagonal)), ay);
        edgex = edgey / slope;

        distance = std::hypot(ax, ay);
        const double distanceToOctagonalEdge = std::hypot(edgex, edgey);
        if (distance > distanceToOctagonalEdge)
        {
            ax = edgex;
            ay = edgey;
        }
    }

    // keep cardinal input within positive and negative bounds of maxAxis
    if (std::abs(ax) > maxAxis) ax = std::copysign(maxAxis, ax);
    if (std::abs(ay) > maxAxis) ay = std::copysign(maxAxis, ay);

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

static bool check_hotkeys(int Control)
{
    InputProfile* profile = &l_InputProfiles[Control];

    int state = 0;

    // we only have to check for hotkeys
    // when there's a controller opened
    if (!profile->InputDevice.HasOpenDevice())
    {
        return false;
    }

#define DEFINE_HOTKEY(mapping, pressed, function, function2) \
    state = get_button_state(profile, &profile->mapping, true); \
    if (state) \
    { \
        if (!profile->pressed) \
        { \
            profile->pressed = true; \
            function; \
        } \
        return true; \
    } \
    else if (!state && profile->pressed) \
    { \
        function2; \
        profile->pressed = false; \
    }

    DEFINE_HOTKEY(Hotkey_Shutdown,              Hotkey_Shutdown_Pressed,      CoreStopEmulation(), );
    DEFINE_HOTKEY(Hotkey_Exit,                  Hotkey_Exit_Pressed,          QGuiApplication::quit(), );
    DEFINE_HOTKEY(Hotkey_SoftReset,             Hotkey_SoftReset_Pressed,     CoreResetEmulation(false), );
    DEFINE_HOTKEY(Hotkey_Resume,                Hotkey_Resume_Pressed,        CoreIsEmulationPaused() ? CoreResumeEmulation() : CorePauseEmulation(), );
    DEFINE_HOTKEY(Hotkey_Screenshot,            Hotkey_Screenshot_Pressed,    CoreTakeScreenshot(), );
    DEFINE_HOTKEY(Hotkey_LimitFPS,              Hotkey_LimitFPS_Pressed,      CoreSetSpeedLimiterState(!CoreIsSpeedLimiterEnabled()), );
    DEFINE_HOTKEY(Hotkey_SpeedFactor25,         Hotkey_SpeedFactor_Pressed,   CoreSetSpeedFactor(25), );
    DEFINE_HOTKEY(Hotkey_SpeedFactor50,         Hotkey_SpeedFactor_Pressed,   CoreSetSpeedFactor(50), );
    DEFINE_HOTKEY(Hotkey_SpeedFactor75,         Hotkey_SpeedFactor_Pressed,   CoreSetSpeedFactor(75), );
    DEFINE_HOTKEY(Hotkey_SpeedFactor100,        Hotkey_SpeedFactor_Pressed,   CoreSetSpeedFactor(100), );
    DEFINE_HOTKEY(Hotkey_SpeedFactor125,        Hotkey_SpeedFactor_Pressed,   CoreSetSpeedFactor(125), );
    DEFINE_HOTKEY(Hotkey_SpeedFactor150,        Hotkey_SpeedFactor_Pressed,   CoreSetSpeedFactor(150), );
    DEFINE_HOTKEY(Hotkey_SpeedFactor175,        Hotkey_SpeedFactor_Pressed,   CoreSetSpeedFactor(175), );
    DEFINE_HOTKEY(Hotkey_SpeedFactor200,        Hotkey_SpeedFactor_Pressed,   CoreSetSpeedFactor(200), );
    DEFINE_HOTKEY(Hotkey_SpeedFactor225,        Hotkey_SpeedFactor_Pressed,   CoreSetSpeedFactor(225), );
    DEFINE_HOTKEY(Hotkey_SpeedFactor250,        Hotkey_SpeedFactor_Pressed,   CoreSetSpeedFactor(250), );
    DEFINE_HOTKEY(Hotkey_SpeedFactor275,        Hotkey_SpeedFactor_Pressed,   CoreSetSpeedFactor(275), );
    DEFINE_HOTKEY(Hotkey_SpeedFactor300,        Hotkey_SpeedFactor_Pressed,   CoreSetSpeedFactor(300), );
    DEFINE_HOTKEY(Hotkey_SaveState,             Hotkey_SaveState_Pressed,     CoreSaveState(), );
    DEFINE_HOTKEY(Hotkey_LoadState,             Hotkey_LoadState_Pressed,     CoreLoadSaveState(), );
    DEFINE_HOTKEY(Hotkey_GSButton,              Hotkey_GSButton_Pressed,      CorePressGamesharkButton(true), CorePressGamesharkButton(false));
    DEFINE_HOTKEY(Hotkey_SaveStateSlot0,        Hotkey_SaveStateSlot_Pressed, CoreSetSaveStateSlot(0), );
    DEFINE_HOTKEY(Hotkey_SaveStateSlot1,        Hotkey_SaveStateSlot_Pressed, CoreSetSaveStateSlot(1), );
    DEFINE_HOTKEY(Hotkey_SaveStateSlot2,        Hotkey_SaveStateSlot_Pressed, CoreSetSaveStateSlot(2), );
    DEFINE_HOTKEY(Hotkey_SaveStateSlot3,        Hotkey_SaveStateSlot_Pressed, CoreSetSaveStateSlot(3), );
    DEFINE_HOTKEY(Hotkey_SaveStateSlot4,        Hotkey_SaveStateSlot_Pressed, CoreSetSaveStateSlot(4), );
    DEFINE_HOTKEY(Hotkey_SaveStateSlot5,        Hotkey_SaveStateSlot_Pressed, CoreSetSaveStateSlot(5), );
    DEFINE_HOTKEY(Hotkey_SaveStateSlot6,        Hotkey_SaveStateSlot_Pressed, CoreSetSaveStateSlot(6), );
    DEFINE_HOTKEY(Hotkey_SaveStateSlot7,        Hotkey_SaveStateSlot_Pressed, CoreSetSaveStateSlot(7), );
    DEFINE_HOTKEY(Hotkey_SaveStateSlot8,        Hotkey_SaveStateSlot_Pressed, CoreSetSaveStateSlot(8), );
    DEFINE_HOTKEY(Hotkey_SaveStateSlot9,        Hotkey_SaveStateSlot_Pressed, CoreSetSaveStateSlot(9), );
    DEFINE_HOTKEY(Hotkey_IncreaseSaveStateSlot, Hotkey_IncreaseSaveStateSlot_Pressed, CoreIncreaseSaveStateSlot(), );
    DEFINE_HOTKEY(Hotkey_DecreaseSaveStateSlot, Hotkey_DecreaseSaveStateSlot_Pressed, CoreDecreaseSaveStateSlot(), );
    DEFINE_HOTKEY(Hotkey_MemoryPak,             Hotkey_MemoryPak_Pressed,     switch_controller_pak(profile, Control, PLUGIN_MEMPAK), );
    DEFINE_HOTKEY(Hotkey_RumblePak,             Hotkey_RumblePak_Pressed,     switch_controller_pak(profile, Control, PLUGIN_RAW), );
    DEFINE_HOTKEY(Hotkey_NoPak,                 Hotkey_NoPak_Pressed,         switch_controller_pak(profile, Control, PLUGIN_NONE), );
    DEFINE_HOTKEY(Hotkey_Fullscreen,            Hotkey_Fullscreen_Pressed,    CoreToggleFullscreen(), );

#undef DEFINE_HOTKEY
    return false;
}

static void sdl_init()
{
    std::filesystem::path gameControllerDbPath;
    std::string debugMessage;
    int ret = -1;

    for (const int subsystem : {SDL_INIT_GAMECONTROLLER, SDL_INIT_AUDIO, SDL_INIT_VIDEO, SDL_INIT_HAPTIC})
    {
        if (!SDL_WasInit(subsystem))
        {
            SDL_InitSubSystem(subsystem);
        }
    }

    gameControllerDbPath = CoreGetSharedDataDirectory();
    gameControllerDbPath += "/gamecontrollerdb.txt";

    // try to load SDL_GameControllerDB when the file exists
    if (std::filesystem::is_regular_file(gameControllerDbPath))
    {
        ret = SDL_GameControllerAddMappingsFromFile(gameControllerDbPath.string().c_str());
        if (ret == -1)
        {
            debugMessage = "sdl_init(): SDL_GameControllerAddMappingsFromFile Failed: ";
            debugMessage += SDL_GetError();
            PluginDebugMessage(M64MSG_WARNING, debugMessage);
        }
    }
    else
    {
        debugMessage = "sdl_init(): Cannot find SDL_GameControllerDB at \"";
        debugMessage += gameControllerDbPath.string().c_str();
        debugMessage += "\"!";
        PluginDebugMessage(M64MSG_WARNING, debugMessage);
    }
}

static void sdl_quit()
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

    // setup debug callback
    l_DebugCallback    = DebugCallback;
    l_DebugCallContext = Context;

    sdl_init();

    l_SDLThread = new Thread::SDLThread(nullptr);
    l_SDLThread->start();

    for (int i = 0; i < NUM_CONTROLLERS; i++)
    {
        l_InputProfiles[i].InputDevice.SetSDLThread(l_SDLThread);
    }

    l_HotkeysThread = new Thread::HotkeysThread(check_hotkeys, nullptr);
    l_HotkeysThread->start();

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

    l_HotkeysThread->StopLoop();
    l_HotkeysThread->deleteLater();
    l_HotkeysThread = nullptr;

    sdl_quit();

    // clear debug callback
    l_DebugCallback    = nullptr;
    l_DebugCallContext = nullptr;

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

    // when PluginShutdown() is called during PluginConfig2(),
    // we shouldn't do anything here anymore because
    // l_SDLThread will be nullptr, causing a crash
    if (l_SDLThread == nullptr)
    {
        return M64ERR_SUCCESS;
    }

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
            if (l_ControlInfo.Controls[Control].Plugin == PLUGIN_RAW)
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
            if (l_ControlInfo.Controls[Control].Plugin == PLUGIN_RAW)
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

#include <iostream>
EXPORT void CALL GetKeys(int Control, BUTTONS* Keys)
{
    InputProfile* profile = &l_InputProfiles[Control];

    if (!profile->PluggedIn)
    {
        return;
    }

#ifdef VRU
    // when we're emulating the VRU,
    // we need to check the mic state
    if (profile->DeviceNum == (int)InputDeviceType::EmulateVRU)
    {
        if (GetVRUMicState())
        {
            Keys->Value = 0x0020;
        }
        else
        {
            Keys->Value = 0x0000;
        }
        return;
    }
#endif // VRU

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

    // when we've matched a hotkey,
    // we don't need to check anything
    // else
    if (check_hotkeys(Control))
    {
        return;
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

    if (Control == 0)
    {
        // mouse
        if (profile->DeviceType == InputDeviceType::Mouse)
        { // n64 mouse
            l_MouseMutex.lock();

            // set left & right button state
            Keys->A_BUTTON = l_MouseButtonState[0];
            Keys->B_BUTTON = l_MouseButtonState[1];

            if (!l_MouseMovements.empty())
            {
                // calculate how much the mouse has moved
                const int x = l_MouseMovements.front().x - l_MouseMovements.back().x;
                const int y = l_MouseMovements.front().y - l_MouseMovements.back().y;            

                // set axis state
                Keys->X_AXIS = -x;
                Keys->Y_AXIS = y;

                l_MouseMovements.clear();
            }

            l_MouseMutex.unlock();

            // request front-end to reset mouse position
            if (l_ResetMousPositionCallback != nullptr)
            {
                l_ResetMousPositionCallback();
            }

            return;
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

    // take deadzone into account
    const double deadzone = profile->DeadzoneValue / 100.0;
    inputX = apply_deadzone(inputX, deadzone);
    inputY = apply_deadzone(inputY, deadzone);

    // take sensitivity into account
    const double sensitivityRatio = profile->SensitivityValue / 100.0;
    const double lowerInputLimit = std::max(-1.0, -sensitivityRatio);
    const double upperInputLimit = std::min(1.0, sensitivityRatio);
    inputX = std::clamp(inputX * sensitivityRatio, lowerInputLimit, upperInputLimit);
    inputY = std::clamp(inputY * sensitivityRatio, lowerInputLimit, upperInputLimit);

    int octagonX = 0, octagonY = 0;
    simulate_octagon(
        deadzone, // deadzone
        inputX, // inputX
        inputY, // inputY
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
    l_HotkeysThread->SetState(HotkeysThreadState::RomOpened);
    return 1;
}

EXPORT void CALL RomClosed(void)
{
    l_HotkeysThread->SetState(HotkeysThreadState::RomClosed);
    l_HasControlInfo = false;
    close_controllers();
#ifdef VRU
    QuitVRU();
#endif // VRU
}

EXPORT void CALL SDL_KeyDown(int keymod, int keysym)
{
    l_KeyboardState[keysym] = true;
}

EXPORT void CALL SDL_KeyUp(int keymod, int keysym)
{
    l_KeyboardState[keysym] = false;
}

EXPORT void CALL MouseMove(int x, int y)
{
    l_MouseMutex.lock();
    l_MouseMovements.push_back({x, y});
    l_MouseMutex.unlock();
}

EXPORT void CALL MouseButton(int left, int right)
{
    l_MouseMutex.lock();
    l_MouseButtonState[0] = left;
    l_MouseButtonState[1] = right;
    l_MouseMutex.unlock();    
}

EXPORT void CALL SetResetMousePositionCallback(ptr_ResetMousePositionCallback callback)
{
    l_ResetMousPositionCallback = callback;
}
