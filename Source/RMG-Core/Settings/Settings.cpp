/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Settings.hpp"

#include "Directories.hpp"
#include "Version.hpp"
#include "m64p/Api.hpp"
#include "Error.hpp"
#include "m64p/api/m64p_types.h"

#include <sstream>
#include <algorithm>

//
// Local Defines
//

#define STR_SIZE 4096

//
// Local Structures
//

struct l_DynamicValue
{
    int intValue;
    bool boolValue;
    float floatValue;
    std::string stringValue;
    std::vector<int> intListValue;

    m64p_type valueType;

    l_DynamicValue() {}

    l_DynamicValue(int value)
    {
        intValue = value;
        valueType = M64TYPE_INT;
    }

    l_DynamicValue(bool value)
    {
        boolValue = value;
        valueType = M64TYPE_BOOL;
    }

    l_DynamicValue(float value)
    {
        floatValue = value;
        valueType = M64TYPE_FLOAT;
    }

    l_DynamicValue(const char* value)
    {
        stringValue = std::string(value);
        valueType = M64TYPE_STRING;
    }

    l_DynamicValue(std::string value)
    {
        stringValue = value;
        valueType = M64TYPE_STRING;
    }

    l_DynamicValue(std::filesystem::path value)
    {
        stringValue = value.string();
        valueType = M64TYPE_STRING;
    }

    l_DynamicValue(std::vector<int> value)
    {
        intListValue = value;
        // convert list to string
        std::string value_str;
        for (int num : value)
        {
            value_str += std::to_string(num);
            value_str += ";";
        }
        stringValue = value_str;
        valueType = M64TYPE_STRING;
    }
};

struct l_Setting
{
    std::string Section;
    std::string Key;
    l_DynamicValue DefaultValue;
    std::string Description = "";
    bool ForceUseSetOnce = false;
};

//
// Local Variables
//

static m64p_handle              l_sectionHandle = nullptr;
static std::vector<std::string> l_sectionList;
static std::vector<std::string> l_keyList;

//
// Local Functions
//

#define SETTING_SECTION_GUI         "Rosalie's Mupen GUI"
#define SETTING_SECTION_CORE        SETTING_SECTION_GUI  " Core"
#define SETTING_SECTION_OVERLAY     SETTING_SECTION_CORE " Overlay"
#define SETTING_SECTION_KEYBIND     SETTING_SECTION_GUI  " KeyBindings"
#define SETTING_SECTION_ROMBROWSER  SETTING_SECTION_GUI  " RomBrowser"
#define SETTING_SECTION_SETTINGS    SETTING_SECTION_CORE " Settings"
#define SETTING_SECTION_64DD        SETTING_SECTION_CORE " 64DD"
#define SETTING_SECTION_GB          SETTING_SECTION_CORE " Gameboy"
#define SETTING_SECTION_M64P        "Core"
#define SETTING_SECTION_AUDIO       SETTING_SECTION_GUI  " - Audio Plugin"

// retrieves l_Setting from settingId
static l_Setting get_setting(SettingsID settingId)
{
    l_Setting setting;

    switch (settingId)
    {
    default:
        setting = {"", "", "", "", false};
        break;

    case SettingsID::GUI_SettingsDialogWidth:
        setting = {SETTING_SECTION_GUI, "SettingsDialogWidth", 0};
        break;
    case SettingsID::GUI_SettingsDialogHeight:
        setting = {SETTING_SECTION_GUI, "SettingsDialogHeight", 0};
        break;
    case SettingsID::GUI_HideCursorInEmulation:
        setting = {SETTING_SECTION_GUI, "HideCursorInEmulation", false};
        break;
    case SettingsID::GUI_HideCursorInFullscreenEmulation:
        setting = {SETTING_SECTION_GUI, "HideCursorInFullscreenEmulation", true};
        break;
    case SettingsID::GUI_StatusbarMessageDuration:
        setting = {SETTING_SECTION_GUI, "StatusbarMessageDuration", 3};
        break;
    case SettingsID::GUI_PauseEmulationOnFocusLoss:
        setting = {SETTING_SECTION_GUI, "PauseEmulationOnFocusLoss", true};
        break;
    case SettingsID::GUI_ResumeEmulationOnFocus:
        setting = {SETTING_SECTION_GUI, "ResumeEmulationOnFocus", true};
        break;
    case SettingsID::GUI_AutomaticFullscreen:
        setting = {SETTING_SECTION_GUI, "AutomaticFullscreen", false};
        break;
    case SettingsID::GUI_ShowVerboseLogMessages:
        setting = {SETTING_SECTION_GUI, "ShowVerboseLogMessages", false};
        break;
    case SettingsID::GUI_OnScreenDisplayEnabled:
        setting = {SETTING_SECTION_GUI, "OnScreenDisplayEnabled", true};
        break;
    case SettingsID::GUI_OnScreenDisplayLocation:
        setting = {SETTING_SECTION_GUI, "OnScreenDisplayLocation", 0};
        break;
    case SettingsID::GUI_OnScreenDisplayPaddingX:
        setting = {SETTING_SECTION_GUI, "OnScreenDisplayPaddingX", 20};
        break;
    case SettingsID::GUI_OnScreenDisplayPaddingY:
        setting = {SETTING_SECTION_GUI, "OnScreenDisplayPaddingY", 20};
        break;
    case SettingsID::GUI_OnScreenDisplayOpacity:
        setting = {SETTING_SECTION_GUI, "OnScreenDisplayOpacity", 0.5f};
        break;
    case SettingsID::GUI_OnScreenDisplayDuration:
        setting = {SETTING_SECTION_GUI, "OnScreenDisplayDuration", 3};
        break;
    case SettingsID::GUI_Toolbar:
        setting = {SETTING_SECTION_GUI, "Toolbar", true};
        break;
    case SettingsID::GUI_StatusBar:
        setting = {SETTING_SECTION_GUI, "StatusBar", true};
        break;
    case SettingsID::GUI_Theme:
        setting = {SETTING_SECTION_GUI, "Theme", "Native"};
        break;
    case SettingsID::GUI_CheckForUpdates:
        setting = {SETTING_SECTION_GUI, "CheckForUpdates", true};
        break;
    case SettingsID::GUI_DiscordRpc:
        setting = {SETTING_SECTION_GUI, "DiscordRpc", true};
        break;
    case SettingsID::GUI_Version:
        setting = {SETTING_SECTION_GUI, "Version", CoreGetVersion()};
        break;

    case SettingsID::Core_GFX_Plugin:
        setting = {SETTING_SECTION_CORE, "GFX_Plugin", 
#ifdef _WIN32
                    CoreGetPluginDirectory().string() + "\\GFX\\mupen64plus-video-GLideN64.dll",
#else
                    CoreGetPluginDirectory().string() + "/GFX/mupen64plus-video-GLideN64.so", 
#endif // _WIN32
                  };
        break;
    case SettingsID::Core_AUDIO_Plugin:
        setting = {SETTING_SECTION_CORE, "AUDIO_Plugin", 
#ifdef _WIN32
                    CoreGetPluginDirectory().string() + "\\Audio\\RMG-Audio.dll",
#else
                    CoreGetPluginDirectory().string() + "/Audio/RMG-Audio.so",
#endif // _WIN32
                  };
        break;
    case SettingsID::Core_INPUT_Plugin:
        setting = {SETTING_SECTION_CORE, "INPUT_Plugin", 
#ifdef _WIN32
                    CoreGetPluginDirectory().string() + "\\Input\\RMG-Input.dll",
#else
                    CoreGetPluginDirectory().string() + "/Input/RMG-Input.so",
#endif // _WIN32
                  };
        break;
    case SettingsID::Core_RSP_Plugin:
        setting = {SETTING_SECTION_CORE, "RSP_Plugin", 
#ifdef _WIN32
                    CoreGetPluginDirectory().string() + "\\RSP\\mupen64plus-rsp-hle.dll",
#else
                    CoreGetPluginDirectory().string() + "/RSP/mupen64plus-rsp-hle.so",
#endif // _WIN32
                  };
        break;

    case SettingsID::Core_OverrideUserDirs:
        setting = {SETTING_SECTION_CORE, "OverrideUserDirectories", true};
        break;
    case SettingsID::Core_UserDataDirOverride:
        setting = {SETTING_SECTION_CORE, "UserDataDirectory", CoreGetDefaultUserDataDirectory()};
        break;
    case SettingsID::Core_UserCacheDirOverride:
        setting = {SETTING_SECTION_CORE, "UserCacheDirectory", CoreGetDefaultUserCacheDirectory()};
        break;

    case SettingsID::Core_OverrideGameSpecificSettings:
        setting = {SETTING_SECTION_CORE, "OverrideGameSpecificSettings", false};
        break;

    case SettingsID::Core_RandomizeInterrupt:
        setting = {SETTING_SECTION_M64P, "RandomizeInterrupt", true};
        break;
    case SettingsID::Core_CPU_Emulator:
        setting = {SETTING_SECTION_M64P, "R4300Emulator", 2};
        break;
    case SettingsID::Core_DisableExtraMem:
        setting = {SETTING_SECTION_M64P, "DisableExtraMem", false};
        break;
    case SettingsID::Core_EnableDebugger:
        setting = {SETTING_SECTION_M64P, "EnableDebugger", false};
        break;
    case SettingsID::Core_CountPerOp:
        setting = {SETTING_SECTION_M64P, "CountPerOp", 0};
        break;
    case SettingsID::Core_CountPerOpDenomPot:
        setting = {SETTING_SECTION_M64P, "CountPerOpDenomPot", 0};
        break;
    case SettingsID::Core_SiDmaDuration:
        setting = {SETTING_SECTION_M64P, "SiDmaDuration", -1};
        break;

    case SettingsID::CoreOverlay_RandomizeInterrupt:
        setting = {SETTING_SECTION_OVERLAY, "RandomizeInterrupt", true};
        break;
    case SettingsID::CoreOverlay_CPU_Emulator:
        setting = {SETTING_SECTION_OVERLAY, "CPU_Emulator", 2};
        break;
    case SettingsID::CoreOverlay_DisableExtraMem:
        setting = {SETTING_SECTION_OVERLAY, "DisableExtraMem", false};
        break;
    case SettingsID::CoreOverlay_EnableDebugger:
        setting = {SETTING_SECTION_OVERLAY, "EnableDebugger", false};
        break;
    case SettingsID::CoreOverlay_CountPerOp:
        setting = {SETTING_SECTION_OVERLAY, "CountPerOp", 0};
        break;
    case SettingsID::CoreOverlay_CountPerOpDenomPot:
        setting = {SETTING_SECTION_OVERLAY, "CountPerOpDenomPot", 0};
        break;
    case SettingsID::CoreOverlay_SiDmaDuration:
        setting = {SETTING_SECTION_OVERLAY, "SiDmaDuration", -1};
        break;

    case SettingsID::Core_ScreenshotPath:
        setting = {SETTING_SECTION_M64P, "ScreenshotPath", CoreGetDefaultScreenshotDirectory(), "", true};
        break;
    case SettingsID::Core_SaveStatePath:
        setting = {SETTING_SECTION_M64P, "SaveStatePath", CoreGetDefaultSaveStateDirectory(), "", true};
        break;
    case SettingsID::Core_SaveSRAMPath:
        setting = {SETTING_SECTION_M64P, "SaveSRAMPath", CoreGetDefaultSaveDirectory(), "", true};
        break;

    case SettingsID::Core_64DD_JapaneseIPL:
        setting = {SETTING_SECTION_64DD, "64DD_JapaneseIPL", ""};
        break;
    case SettingsID::Core_64DD_AmericanIPL:
        setting = {SETTING_SECTION_64DD, "64DD_AmericanIPL", ""};
        break;
    case SettingsID::Core_64DD_DevelopmentIPL:
        setting = {SETTING_SECTION_64DD, "64DD_DevelopmentIPL", ""};
        break;
    case SettingsID::Core_64DD_SaveDiskFormat:
        setting = {SETTING_SECTION_M64P, "SaveDiskFormat", 1};
        break;

    case SettingsID::Core_Gameboy_P1_Rom:
        setting = {SETTING_SECTION_GB, "Gameboy_P1_Rom", ""};
        break;
    case SettingsID::Core_Gameboy_P1_Save:
        setting = {SETTING_SECTION_GB, "Gameboy_P1_Save", ""};
        break;
    case SettingsID::Core_Gameboy_P2_Rom:
        setting = {SETTING_SECTION_GB, "Gameboy_P2_Rom", ""};
        break;
    case SettingsID::Core_Gameboy_P2_Save:
        setting = {SETTING_SECTION_GB, "Gameboy_P2_Save", ""};
        break;
    case SettingsID::Core_Gameboy_P3_Rom:
        setting = {SETTING_SECTION_GB, "Gameboy_P3_Rom", ""};
        break;
    case SettingsID::Core_Gameboy_P3_Save:
        setting = {SETTING_SECTION_GB, "Gameboy_P3_Save", ""};
        break;
    case SettingsID::Core_Gameboy_P4_Rom:
        setting = {SETTING_SECTION_GB, "Gameboy_P4_Rom", ""};
        break;
    case SettingsID::Core_Gameboy_P4_Save:
        setting = {SETTING_SECTION_GB, "Gameboy_P4_Save", ""};
        break;

    case SettingsID::Game_OverrideSettings:
        setting = {"", "OverrideSettings", false};
        break;
    case SettingsID::Game_DisableExtraMem:
        setting = {"", "DisableExtraMem", false};
        break;
    case SettingsID::Game_SaveType:
        setting = {"", "SaveType", 0};
        break;
    case SettingsID::Game_CountPerOp:
        setting = {"", "CountPerOp", 2};
        break;
    case SettingsID::Game_SiDmaDuration:
        setting = {"", "SiDmaDuration", 2304};
        break;

    case SettingsID::Game_OverrideCoreSettings:
        setting = {"", "OverrideCoreSettings", false};
        break;
    case SettingsID::Game_CPU_Emulator:
        setting = {"", "CPU_Emulator", 2};
        break;
    case SettingsID::Game_CountPerOpDenomPot:
        setting = {"", "CountPerOpDenomPot", 0};
        break;
    case SettingsID::Game_RandomizeInterrupt:
        setting = {"", "RandomizeInterrupt", true};
        break;

    case SettingsID::Game_GFX_Plugin:
        setting = {"", "GFX_Plugin", ""};
        break;
    case SettingsID::Game_AUDIO_Plugin:
        setting = {"", "AUDIO_Plugin", ""};
        break;
    case SettingsID::Game_INPUT_Plugin:
        setting = {"", "INPUT_Plugin", ""};
        break;
    case SettingsID::Game_RSP_Plugin:
        setting = {"", "RSP_Plugin", ""};
        break;

    case SettingsID::KeyBinding_RemoveDuplicates:
        setting = {SETTING_SECTION_KEYBIND, "RemoveDuplicates", true};
        break;
    case SettingsID::KeyBinding_StartROM:
        setting = {SETTING_SECTION_KEYBIND, "StartROM", "Ctrl+O"};
        break;
    case SettingsID::KeyBinding_StartCombo:
        setting = {SETTING_SECTION_KEYBIND, "StartCombo", "Ctrl+Shift+O"};
        break;
    case SettingsID::KeyBinding_Shutdown:
        setting = {SETTING_SECTION_KEYBIND, "Shutdown", "F12"};
        break;
    case SettingsID::KeyBinding_RefreshROMList:
        setting = {SETTING_SECTION_KEYBIND, "RefreshROMList", "F5"};
        break;
    case SettingsID::KeyBinding_Exit:
        setting = {SETTING_SECTION_KEYBIND, "Exit", "Alt+F4"};
        break;
    case SettingsID::KeyBinding_SoftReset:
        setting = {SETTING_SECTION_KEYBIND, "SoftReset", "F1"};
        break;
    case SettingsID::KeyBinding_HardReset:
        setting = {SETTING_SECTION_KEYBIND, "HardReset", "Shift+F1"};
        break;
    case SettingsID::KeyBinding_Resume:
        setting = {SETTING_SECTION_KEYBIND, "Resume", "F2"};
        break;
    case SettingsID::KeyBinding_Screenshot:
        setting = {SETTING_SECTION_KEYBIND, "Screenshot", "F3"};
        break;
    case SettingsID::KeyBinding_LimitFPS:
        setting = {SETTING_SECTION_KEYBIND, "LimitFPS", "F4"};
        break;
    case SettingsID::KeyBinding_SaveState:
        setting = {SETTING_SECTION_KEYBIND, "SaveState", "F5"};
        break;
    case SettingsID::KeyBinding_SaveAs:
        setting = {SETTING_SECTION_KEYBIND, "SaveAs", "Ctrl+S"};
        break;
    case SettingsID::KeyBinding_LoadState:
        setting = {SETTING_SECTION_KEYBIND, "LoadState", "F7"};
        break;
    case SettingsID::KeyBinding_Load:
        setting = {SETTING_SECTION_KEYBIND, "Load", "Ctrl+L"};
        break;
    case SettingsID::KeyBinding_Cheats:
        setting = {SETTING_SECTION_KEYBIND, "Cheats", "Ctrl+C"};
        break;
    case SettingsID::KeyBinding_GSButton:
        setting = {SETTING_SECTION_KEYBIND, "GSButton", "F9"};
        break;
    case SettingsID::KeyBinding_SaveStateSlot0:
        setting = {SETTING_SECTION_KEYBIND, "SaveStateSlot0", "Ctrl+0"};
        break;
    case SettingsID::KeyBinding_SaveStateSlot1:
        setting = {SETTING_SECTION_KEYBIND, "SaveStateSlot1", "Ctrl+1"};
        break;
    case SettingsID::KeyBinding_SaveStateSlot2:
        setting = {SETTING_SECTION_KEYBIND, "SaveStateSlot2", "Ctrl+2"};
        break;
    case SettingsID::KeyBinding_SaveStateSlot3:
        setting = {SETTING_SECTION_KEYBIND, "SaveStateSlot3", "Ctrl+3"};
        break;
    case SettingsID::KeyBinding_SaveStateSlot4:
        setting = {SETTING_SECTION_KEYBIND, "SaveStateSlot4", "Ctrl+4"};
        break;
    case SettingsID::KeyBinding_SaveStateSlot5:
        setting = {SETTING_SECTION_KEYBIND, "SaveStateSlot5", "Ctrl+5"};
        break;
    case SettingsID::KeyBinding_SaveStateSlot6:
        setting = {SETTING_SECTION_KEYBIND, "SaveStateSlot6", "Ctrl+6"};
        break;
    case SettingsID::KeyBinding_SaveStateSlot7:
        setting = {SETTING_SECTION_KEYBIND, "SaveStateSlot7", "Ctrl+7"};
        break;
    case SettingsID::KeyBinding_SaveStateSlot8:
        setting = {SETTING_SECTION_KEYBIND, "SaveStateSlot8", "Ctrl+8"};
        break;
    case SettingsID::KeyBinding_SaveStateSlot9:
        setting = {SETTING_SECTION_KEYBIND, "SaveStateSlot9", "Ctrl+9"};
        break;
    case SettingsID::KeyBinding_Fullscreen:
        setting = {SETTING_SECTION_KEYBIND, "Fullscreen", "Alt+Return"};
        break;
    case SettingsID::KeyBinding_Settings:
        setting = {SETTING_SECTION_KEYBIND, "Settings", "Ctrl+T"};
        break;

    case SettingsID::RomBrowser_Directory:
        setting = {SETTING_SECTION_ROMBROWSER, "Directory", ""};
        break;
    case SettingsID::RomBrowser_Geometry:
        setting = {SETTING_SECTION_ROMBROWSER, "Geometry", ""};
        break;
    case SettingsID::RomBrowser_Maximized:
        setting = {SETTING_SECTION_ROMBROWSER, "Maximized", false};
        break;
    case SettingsID::RomBrowser_Recursive:
        setting = {SETTING_SECTION_ROMBROWSER, "Recursive", true};
        break;
    case SettingsID::RomBrowser_MaxItems:
        setting = {SETTING_SECTION_ROMBROWSER, "MaxItems", 250};
        break;
    case SettingsID::RomBrowser_Columns:
        setting = {SETTING_SECTION_ROMBROWSER, "Columns", std::vector<int>({0, 1, 2})};
        break;
    case SettingsID::RomBrowser_ColumnSizes:
        setting = {SETTING_SECTION_ROMBROWSER, "ColumnSizes", std::vector<int>({-1, -1, -1})};
        break;
    case SettingsID::RomBrowser_SortAfterSearch:
        setting = {SETTING_SECTION_ROMBROWSER, "SortAfterSearch", true};
        break;
    case SettingsID::RomBrowser_ViewMode:
        setting = {SETTING_SECTION_ROMBROWSER, "ViewMode", 0};
        break;
    case SettingsID::RomBrowser_ListViewSortSection:
        setting = {SETTING_SECTION_ROMBROWSER, "ListViewSortSection", 0};
        break;
    case SettingsID::RomBrowser_ListViewSortOrder:
        setting = {SETTING_SECTION_ROMBROWSER, "ListViewSortOrder", 0};
        break;
    case SettingsID::RomBrowser_GridViewIconWidth:
        setting = {SETTING_SECTION_ROMBROWSER, "GridViewIconWidth", 180};
        break;
    case SettingsID::RomBrowser_GridViewIconHeight:
        setting = {SETTING_SECTION_ROMBROWSER, "GridViewIconHeight", 126};
        break;
    case SettingsID::RomBrowser_GridViewUniformItemSizes:
        setting = {SETTING_SECTION_ROMBROWSER, "GridViewUniformItemSizes", true};
        break;

    case SettingsID::Settings_HasForceUsedSetOnce:
        setting = {SETTING_SECTION_SETTINGS, "HasForceUsedSetOnce", false};
        break;

    case SettingsID::Audio_DefaultFrequency:
        setting = {SETTING_SECTION_AUDIO, "DefaultFrequency", 33600};
        break;
    case SettingsID::Audio_SwapChannels:
        setting = {SETTING_SECTION_AUDIO, "SwapChannels", false};
        break;
    case SettingsID::Audio_PrimaryBufferSize:
        setting = {SETTING_SECTION_AUDIO, "PrimaryBufferSize", 16384};
        break;
    case SettingsID::Audio_PrimaryBufferTarget:
        setting = {SETTING_SECTION_AUDIO, "PrimaryBufferTarget", 2048};
        break;
    case SettingsID::Audio_SecondaryBufferSize:
        setting = {SETTING_SECTION_AUDIO, "SecondaryBufferSize", 1024};
        break;
    case SettingsID::Audio_Resampler:
        setting = {SETTING_SECTION_AUDIO, "Resampler", std::string("trivial")};
        break;
    case SettingsID::Audio_Volume:
        setting = {SETTING_SECTION_AUDIO, "Volume", 100};
        break;
    case SettingsID::Audio_Muted:
        setting = {SETTING_SECTION_AUDIO, "Muted", false};
        break;
    case SettingsID::Audio_Synchronize:
        setting = {SETTING_SECTION_AUDIO, "Synchronize", false};
        break;

    case SettingsID::Input_PluggedIn:
        setting = {"", "PluggedIn"};
        break;
    case SettingsID::Input_DeviceType:
        setting = {"", "DeviceType"};
        break;
    case SettingsID::Input_DeviceName:
        setting = {"", "DeviceName"};
        break;
    case SettingsID::Input_DeviceNum:
        setting = {"", "DeviceNum"};
        break;
    case SettingsID::Input_Deadzone:
        setting = {"", "Deadzone"};
        break;
    case SettingsID::Input_Pak:
        setting = {"", "Pak"};
        break;
    case SettingsID::Input_GameboyRom:
        setting = {"", "GameboyRom"};
        break;
    case SettingsID::Input_GameboySave:
        setting = {"", "GameboySave"};
        break;
    case SettingsID::Input_RemoveDuplicateMappings:
        setting = {"", "RemoveDuplicateMappings"};
        break;
    case SettingsID::Input_A_InputType:
        setting = {"", "A_InputType"};
        break;
    case SettingsID::Input_A_Name:
        setting = {"", "A_Name"};
        break;
    case SettingsID::Input_A_Data:
        setting = {"", "A_Data"};
        break;
    case SettingsID::Input_A_ExtraData:
        setting = {"", "A_ExtraData"};
        break;
    case SettingsID::Input_B_InputType:
        setting = {"", "B_InputType"};
        break;
    case SettingsID::Input_B_Name:
        setting = {"", "B_Name"};
        break;
    case SettingsID::Input_B_Data:
        setting = {"", "B_Data"};
        break;
    case SettingsID::Input_B_ExtraData:
        setting = {"", "B_ExtraData"};
        break;
    case SettingsID::Input_Start_InputType:
        setting = {"", "Start_InputType"};
        break;
    case SettingsID::Input_Start_Name:
        setting = {"", "Start_Name"};
        break;
    case SettingsID::Input_Start_Data:
        setting = {"", "Start_Data"};
        break;
    case SettingsID::Input_Start_ExtraData:
        setting = {"", "Start_ExtraData"};
        break;
    case SettingsID::Input_DpadUp_InputType:
        setting = {"", "DpadUp_InputType"};
        break;
    case SettingsID::Input_DpadUp_Name:
        setting = {"", "DpadUp_Name"};
        break;
    case SettingsID::Input_DpadUp_Data:
        setting = {"", "DpadUp_Data"};
        break;
    case SettingsID::Input_DpadUp_ExtraData:
        setting = {"", "DpadUp_ExtraData"};
        break;
    case SettingsID::Input_DpadDown_InputType:
        setting = {"", "DpadDown_InputType"};
        break;
    case SettingsID::Input_DpadDown_Name:
        setting = {"", "DpadDown_Name"};
        break;
    case SettingsID::Input_DpadDown_Data:
        setting = {"", "DpadDown_Data"};
        break;
    case SettingsID::Input_DpadDown_ExtraData:
        setting = {"", "DpadDown_ExtraData"};
        break;
    case SettingsID::Input_DpadLeft_InputType:
        setting = {"", "DpadLeft_InputType"};
        break;
    case SettingsID::Input_DpadLeft_Name:
        setting = {"", "DpadLeft_Name"};
        break;
    case SettingsID::Input_DpadLeft_Data:
        setting = {"", "DpadLeft_Data"};
        break;
    case SettingsID::Input_DpadLeft_ExtraData:
        setting = {"", "DpadLeft_ExtraData"};
        break;
    case SettingsID::Input_DpadRight_InputType:
        setting = {"", "DpadRight_InputType"};
        break;
    case SettingsID::Input_DpadRight_Name:
        setting = {"", "DpadRight_Name"};
        break;
    case SettingsID::Input_DpadRight_Data:
        setting = {"", "DpadRight_Data"};
        break;
    case SettingsID::Input_DpadRight_ExtraData:
        setting = {"", "DpadRight_ExtraData"};
        break;
    case SettingsID::Input_CButtonUp_InputType:
        setting = {"", "CButtonUp_InputType"};
        break;
    case SettingsID::Input_CButtonUp_Name:
        setting = {"", "CButtonUp_Name"};
        break;
    case SettingsID::Input_CButtonUp_Data:
        setting = {"", "CButtonUp_Data"};
        break;
    case SettingsID::Input_CButtonUp_ExtraData:
        setting = {"", "CButtonUp_ExtraData"};
        break;
    case SettingsID::Input_CButtonDown_InputType:
        setting = {"", "CButtonDown_InputType"};
        break;
    case SettingsID::Input_CButtonDown_Name:
        setting = {"", "CButtonDown_Name"};
        break;
    case SettingsID::Input_CButtonDown_Data:
        setting = {"", "CButtonDown_Data"};
        break;
    case SettingsID::Input_CButtonDown_ExtraData:
        setting = {"", "CButtonDown_ExtraData"};
        break;
    case SettingsID::Input_CButtonLeft_InputType:
        setting = {"", "CButtonLeft_InputType"};
        break;
    case SettingsID::Input_CButtonLeft_Name:
        setting = {"", "CButtonLeft_Name"};
        break;
    case SettingsID::Input_CButtonLeft_Data:
        setting = {"", "CButtonLeft_Data"};
        break;
    case SettingsID::Input_CButtonLeft_ExtraData:
        setting = {"", "CButtonLeft_ExtraData"};
        break;
    case SettingsID::Input_CButtonRight_InputType:
        setting = {"", "CButtonRight_InputType"};
        break;
    case SettingsID::Input_CButtonRight_Name:
        setting = {"", "CButtonRight_Name"};
        break;
    case SettingsID::Input_CButtonRight_Data:
        setting = {"", "CButtonRight_Data"};
        break;
    case SettingsID::Input_CButtonRight_ExtraData:
        setting = {"", "CButtonRight_ExtraData"};
        break;
    case SettingsID::Input_LeftTrigger_InputType:
        setting = {"", "LeftTrigger_InputType"};
        break;
    case SettingsID::Input_LeftTrigger_Name:
        setting = {"", "LeftTrigger_Name"};
        break;
    case SettingsID::Input_LeftTrigger_Data:
        setting = {"", "LeftTrigger_Data"};
        break;
    case SettingsID::Input_LeftTrigger_ExtraData:
        setting = {"", "LeftTrigger_ExtraData"};
        break;
    case SettingsID::Input_RightTrigger_InputType:
        setting = {"", "RightTrigger_InputType"};
        break;
    case SettingsID::Input_RightTrigger_Name:
        setting = {"", "RightTrigger_Name"};
        break;
    case SettingsID::Input_RightTrigger_Data:
        setting = {"", "RightTrigger_Data"};
        break;
    case SettingsID::Input_RightTrigger_ExtraData:
        setting = {"", "RightTrigger_ExtraData"};
        break;
    case SettingsID::Input_ZTrigger_InputType:
        setting = {"", "ZTrigger_InputType"};
        break;
    case SettingsID::Input_ZTrigger_Name:
        setting = {"", "ZTrigger_Name"};
        break;
    case SettingsID::Input_ZTrigger_Data:
        setting = {"", "ZTrigger_Data"};
        break;
    case SettingsID::Input_ZTrigger_ExtraData:
        setting = {"", "ZTrigger_ExtraData"};
        break;
    case SettingsID::Input_AnalogStickUp_InputType:
        setting = {"", "AnalogStickUp_InputType"};
        break;
    case SettingsID::Input_AnalogStickUp_Name:
        setting = {"", "AnalogStickUp_Name"};
        break;
    case SettingsID::Input_AnalogStickUp_Data:
        setting = {"", "AnalogStickUp_Data"};
        break;
    case SettingsID::Input_AnalogStickUp_ExtraData:
        setting = {"", "AnalogStickUp_ExtraData"};
        break;
    case SettingsID::Input_AnalogStickDown_InputType:
        setting = {"", "AnalogStickDown_InputType"};
        break;
    case SettingsID::Input_AnalogStickDown_Name:
        setting = {"", "AnalogStickDown_Name"};
        break;
    case SettingsID::Input_AnalogStickDown_Data:
        setting = {"", "AnalogStickDown_Data"};
        break;
    case SettingsID::Input_AnalogStickDown_ExtraData:
        setting = {"", "AnalogStickDown_ExtraData"};
        break;
    case SettingsID::Input_AnalogStickLeft_InputType:
        setting = {"", "AnalogStickLeft_InputType"};
        break;
    case SettingsID::Input_AnalogStickLeft_Name:
        setting = {"", "AnalogStickLeft_Name"};
        break;
    case SettingsID::Input_AnalogStickLeft_Data:
        setting = {"", "AnalogStickLeft_Data"};
        break;
    case SettingsID::Input_AnalogStickLeft_ExtraData:
        setting = {"", "AnalogStickLeft_ExtraData"};
        break;
    case SettingsID::Input_AnalogStickRight_InputType:
        setting = {"", "AnalogStickRight_InputType"};
        break;
    case SettingsID::Input_AnalogStickRight_Name:
        setting = {"", "AnalogStickRight_Name"};
        break;
    case SettingsID::Input_AnalogStickRight_Data:
        setting = {"", "AnalogStickRight_Data"};
        break;
    case SettingsID::Input_AnalogStickRight_ExtraData:
        setting = {"", "AnalogStickRight_ExtraData"};
        break;
    }

    return setting;
}

static void config_listsections_callback(void* context, const char* section)
{
    l_sectionList.emplace_back(std::string(section));
}

static bool config_section_exists(std::string section)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Config.IsHooked())
    {
        return false;
    }

    l_sectionList.clear();

    ret = m64p::Config.ListSections(nullptr, &config_listsections_callback);
    if (ret != M64ERR_SUCCESS)
    {
        error = "config_section_exists m64p::Config.ListSections Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
        return false;
    }

    return std::find(l_sectionList.begin(), l_sectionList.end(), section) != l_sectionList.end();
}

static bool config_section_open(std::string section)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Config.IsHooked())
    {
        return false;
    }

    if (section.empty())
    {
        error = "config_section_open Failed: cannot open empty section!";
        CoreSetError(error);
        return false;
    }

    ret = m64p::Config.OpenSection(section.c_str(), &l_sectionHandle);
    if (ret != M64ERR_SUCCESS)
    {
        error = "config_section_open m64p::Config.OpenSection Failed: ";
        error = m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

static void config_listkeys_callback(void* context, const char* key, m64p_type type)
{
    l_keyList.push_back(std::string(key));
}

static bool config_key_exists(std::string section, std::string key)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Config.IsHooked())
    {
        return false;
    }

    if (!config_section_open(section))
    {
        return false;
    }

    l_keyList.clear();

    ret = m64p::Config.ListParameters(l_sectionHandle, nullptr, &config_listkeys_callback);
    if (ret != M64ERR_SUCCESS)
    {
        error = "config_key_exists m64p::Config.ListParameters Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
        return false;
    }

    return std::find(l_keyList.begin(), l_keyList.end(), key) != l_keyList.end();;
}

static bool config_option_set(std::string section, std::string key, m64p_type type, void *value)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Config.IsHooked())
    {
        return false;
    }

    if (!config_section_open(section))
    {
        return false;
    }

    ret = m64p::Config.SetParameter(l_sectionHandle, key.c_str(), type, value);
    if (ret != M64ERR_SUCCESS)
    {
        error = "config_option_set m64p::Config.SetParameter Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

static bool config_option_get(std::string section, std::string key, m64p_type type, void *value, int size)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Config.IsHooked())
    {
        return false;
    }

    if (!config_section_exists(section))
    {
        error = "config_option_get Failed: cannot open non-existent section!";
        CoreSetError(error);
        return false;
    }

    if (!config_section_open(section))
    {
        return false;
    }

    ret = m64p::Config.GetParameter(l_sectionHandle, key.c_str(), type, value, size);
    if (ret != M64ERR_SUCCESS)
    {
        error = "config_option_get m64p::Config.GetParameter Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

static bool config_option_default_set(std::string section, std::string key, m64p_type type, void *value, const char* description)
{
    std::string error;
    m64p_error ret;

    if (!config_section_open(section))
    {
        return false;
    }

    switch (type)
    {
        default:
        {
            ret = M64ERR_INPUT_INVALID;
            error = "config_option_default_set Failed: invalid type parameter!";
        } break;
        case M64TYPE_INT:
        {
            ret = m64p::Config.SetDefaultInt(l_sectionHandle, key.c_str(), *(int*)value, description);
            error = "config_option_default_set m64p::Config.SetDefaultInt Failed: ";
            error += m64p::Core.ErrorMessage(ret);
        } break;
        case M64TYPE_BOOL:
        {
            ret = m64p::Config.SetDefaultBool(l_sectionHandle, key.c_str(), *(bool*)value, description);
            error = "config_option_default_set m64p::Config.SetDefaultBool Failed: ";
            error += m64p::Core.ErrorMessage(ret);
        } break;
        case M64TYPE_FLOAT:
        {
            ret = m64p::Config.SetDefaultFloat(l_sectionHandle, key.c_str(), *(float*)value, description);
            error = "config_option_default_set m64p::Config.SetDefaultFloat Failed: ";
            error += m64p::Core.ErrorMessage(ret);
        } break;
        case M64TYPE_STRING:
        {
            ret = m64p::Config.SetDefaultString(l_sectionHandle, key.c_str(), (char*)value, description);
            error = "config_option_default_set m64p::Config.SetDefaultString Failed: ";
            error += m64p::Core.ErrorMessage(ret);
        } break;
    }

    if (ret != M64ERR_SUCCESS)
    {
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

static bool int_list_to_string(std::vector<int> intList, std::string& string)
{
    for (int i = 0; i < intList.size(); i++)
    {
        int num = intList.at(i);
        string += std::to_string(num);

        // don't add seperator at end
        // of the list
        if (i < (intList.size() - 1))
        {
            string += ";";
        }
    }
    return true;
}

static bool string_to_int_list(std::string string, std::vector<int>& intList)
{
    std::string error;

    // split string by ';'
    // and append list with each item
    std::stringstream value_str_stream(string);
    std::string tmp_str;
    while (std::getline(value_str_stream, tmp_str, ';'))
    {
        try
        {
            intList.emplace_back(std::stoi(tmp_str));
        }
        catch (...)
        {
            error = "string_to_int_list: std::stroi threw an exception!";
            CoreSetError(error);
            return false;
        }
    }

    return true;
}

static bool string_list_to_string(std::vector<std::string> stringList, std::string& string)
{
    std::string error;

    for (int i = 0; i < stringList.size(); i++)
    {
        std::string str = stringList.at(i);

        // ensure the string doesn't contain
        // the separator character
        if (str.find(";") != std::string::npos)
        {
            error = "string_list_to_string: string cannot contain ';'!";
            CoreSetError(error);
            return false;
        }

        string += str;

        // don't add separator at end
        // of the list
        if (i < (stringList.size() - 1))
        {
            string += ";";
        }
    }

    return true;
}

static bool string_to_string_list(std::string string, std::vector<std::string>& stringList)
{
    std::string error;

    // split string by ';'
    // and append list with each item
    std::stringstream value_str_stream(string);
    std::string tmp_str;
    while (std::getline(value_str_stream, tmp_str, ';'))
    {
        stringList.emplace_back(tmp_str);
    }

    return true;
}

//
// Exported Functions
//

bool CoreSettingsSave(void)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Config.IsHooked())
    {
        return false;
    }

    ret = m64p::Config.SaveFile();
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreSettingsSave m64p::Config.SaveFile Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

bool CoreSettingsUpgrade(void)
{
    std::string settingsVersion;
    std::string settingsString;

    settingsVersion = CoreSettingsGetStringValue(SettingsID::GUI_Version);

    // we don't need to do anything
    // when the core version and the settings version match
    if (settingsVersion == CoreGetVersion())
    {
        return true;
    }

    if (settingsVersion.empty())
    { // settings version was introduced in >v0.1.5

#ifndef PORTABLE_INSTALL // only applies to non-portable installs (i.e flatpak)
        // sadly v0.1.5 introduced an issue,
        // in v0.1.4 the screenshot directory was set to 'Screenshots',
        // in v0.1.5 it was changed to '$HOME/Pictures/RMG'
        // and it was included in CoreCreateDirectories(),
        // which, when someone upgraded to v0.1.5 would fail
        // because we lacked permissions to create the 'Screenshots' directory
        settingsString = CoreGetScreenshotDirectory();
        if (settingsString == "Screenshots")
        {
            CoreSettingsSetValue(SettingsID::Core_ScreenshotPath, CoreGetDefaultScreenshotDirectory());
        }
#endif
    }

    if (settingsVersion == "v0.2.1" ||
        settingsVersion == "v0.2.2" ||
        settingsVersion == "v0.2.3")
    {
        // sadly when introducing the new RMG-Audio based on mupen64plus-audio-sdl
        // 1) sync audio seems to cause crackling for some users
        // 2) sync audio breaks the 'Limit FPS' option
        // so to fix those issues, we'll disable the sync audio option
        // if it's enabled
        if (CoreSettingsGetBoolValue(SettingsID::Audio_Synchronize))
        {
            CoreSettingsSetValue(SettingsID::Audio_Synchronize, false);
        }
    }

    // save core version
    CoreSettingsSetValue(SettingsID::GUI_Version, CoreGetVersion());
    CoreSettingsSave();
    return true;
}

bool CoreSettingsSetupDefaults(void)
{
    l_Setting setting;
    bool ret, hasForceUsedSetOnce;

    hasForceUsedSetOnce = CoreSettingsGetBoolValue(SettingsID::Settings_HasForceUsedSetOnce);

    for (int i = 0; i < (int)SettingsID::Invalid; i++)
    {
        setting = get_setting((SettingsID)i);

        if (setting.Section.empty())
        {
            continue;
        }

        switch (setting.DefaultValue.valueType)
        {
        case M64TYPE_STRING:
        {
            if (setting.ForceUseSetOnce && !hasForceUsedSetOnce)
            {
                ret = config_option_set(setting.Section, setting.Key, M64TYPE_STRING, (void*)setting.DefaultValue.stringValue.c_str());
            }
            else if (!setting.ForceUseSetOnce)
            {
                ret = config_option_default_set(setting.Section, setting.Key, M64TYPE_STRING, (void*)setting.DefaultValue.stringValue.c_str(), setting.Description.c_str());
            }
        } break;
        case M64TYPE_INT:
            ret = config_option_default_set(setting.Section, setting.Key, M64TYPE_INT, &setting.DefaultValue.intValue, setting.Description.c_str());
            break;
        case M64TYPE_BOOL:
            ret = config_option_default_set(setting.Section, setting.Key, M64TYPE_BOOL, &setting.DefaultValue.boolValue, setting.Description.c_str());
            break;
        case M64TYPE_FLOAT:
            ret = config_option_default_set(setting.Section, setting.Key, M64TYPE_FLOAT, &setting.DefaultValue.floatValue, setting.Description.c_str());
            break;
        }

        if (!ret)
        {
            return false;
        }
    }

    if (!hasForceUsedSetOnce)
    {
        CoreSettingsSetValue(SettingsID::Settings_HasForceUsedSetOnce, true);
    }

    return true;
}

bool CoreSettingsSectionExists(std::string section)
{
    return config_section_exists(section);
}

bool CoreSettingsDeleteSection(std::string section)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Config.IsHooked())
    {
        return false;
    }

    if (!config_section_exists(section))
    {
        error = "CoreSettingsDeleteSection Failed: cannot non-existent section!";
        CoreSetError(error);
        return false;
    }

    ret = m64p::Config.DeleteSection(section.c_str());
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreSettingsDeleteSection m64p::Config.DeleteSection() Failed: ";
        error = m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

bool CoreSettingsKeyExists(std::string section, std::string key)
{
    return config_key_exists(section, key);
}

bool CoreSettingsSetValue(SettingsID settingId, int value)
{
    l_Setting setting = get_setting(settingId);
    return config_option_set(setting.Section, setting.Key, M64TYPE_INT, &value);
}

bool CoreSettingsSetValue(SettingsID settingId, bool value)
{
    l_Setting setting = get_setting(settingId);
    int intValue = value ? 1 : 0;
    return config_option_set(setting.Section, setting.Key, M64TYPE_BOOL, &intValue);
}

bool CoreSettingsSetValue(SettingsID settingId, float value)
{
    l_Setting setting = get_setting(settingId);
    return config_option_set(setting.Section, setting.Key, M64TYPE_FLOAT, &value);
}

bool CoreSettingsSetValue(SettingsID settingId, std::string value)
{
    l_Setting setting = get_setting(settingId);
    return config_option_set(setting.Section, setting.Key, M64TYPE_STRING, (void*)value.c_str());
}

bool CoreSettingsSetValue(SettingsID settingId, std::vector<int> value)
{
    std::string value_str;
    if (!int_list_to_string(value, value_str))
    {
        return false;
    }
    return CoreSettingsSetValue(settingId, value_str);
}

bool CoreSettingsSetValue(SettingsID settingId, std::vector<std::string> value)
{
    std::string value_str;
    if (!string_list_to_string(value, value_str))
    {
        return false;
    }
    return CoreSettingsSetValue(settingId, value_str);
}

bool CoreSettingsSetValue(SettingsID settingId, std::string section, int value)
{
    l_Setting setting = get_setting(settingId);
    return config_option_set(section, setting.Key, M64TYPE_INT, &value);
}

bool CoreSettingsSetValue(SettingsID settingId, std::string section, bool value)
{
    l_Setting setting = get_setting(settingId);
    int intValue = value ? 1 : 0;
    return config_option_set(section, setting.Key, M64TYPE_BOOL, &intValue);
}

bool CoreSettingsSetValue(SettingsID settingId, std::string section, float value)
{
    l_Setting setting = get_setting(settingId);
    return config_option_set(section, setting.Key, M64TYPE_FLOAT, &value);
}

bool CoreSettingsSetValue(SettingsID settingId, std::string section, std::string value)
{
    l_Setting setting = get_setting(settingId);
    return config_option_set(section, setting.Key, M64TYPE_STRING, (void*)value.c_str());
}

bool CoreSettingsSetValue(SettingsID settingId, std::string section, std::vector<int> value)
{
    std::string value_str;
    if (!int_list_to_string(value, value_str))
    {
        return false;
    }
    return CoreSettingsSetValue(settingId, section, value_str);
}

bool CoreSettingsSetValue(SettingsID settingId, std::string section, std::vector<std::string> value)
{
    std::string value_str;
    if (!string_list_to_string(value, value_str))
    {
        return false;
    }
    return CoreSettingsSetValue(settingId, section, value_str);
}

bool CoreSettingsSetValue(std::string section, std::string key, int value)
{
    return config_option_set(section, key, M64TYPE_INT, &value);
}

bool CoreSettingsSetValue(std::string section, std::string key, bool value)
{
    int intValue = value ? 1 : 0;
    return config_option_set(section, key, M64TYPE_BOOL, &intValue);
}

bool CoreSettingsSetValue(std::string section, std::string key, float value)
{
    return config_option_set(section, key, M64TYPE_FLOAT, &value);
}

bool CoreSettingsSetValue(std::string section, std::string key, std::string value)
{
    return config_option_set(section, key, M64TYPE_STRING, (void*)value.c_str());
}

bool CoreSettingsSetValue(std::string section, std::string key, std::vector<int> value)
{
    std::string value_str;
    if (!int_list_to_string(value, value_str))
    {
        return false;
    }
    return CoreSettingsSetValue(section, key, value_str);
}

bool CoreSettingsSetValue(std::string section, std::string key, std::vector<std::string> value)
{
    std::string value_str;
    if (!string_list_to_string(value, value_str))
    {
        return false;
    }
    return CoreSettingsSetValue(section, key, value_str);
}

int CoreSettingsGetDefaultIntValue(SettingsID settingId)
{
    l_Setting setting = get_setting(settingId);
    return setting.DefaultValue.intValue;
}

bool CoreSettingsGetDefaultBoolValue(SettingsID settingId)
{
    l_Setting setting = get_setting(settingId);
    return setting.DefaultValue.boolValue;
}

float CoreSettingsGetDefaultFloatValue(SettingsID settingId)
{
    l_Setting setting = get_setting(settingId);
    return setting.DefaultValue.floatValue;
}

std::string CoreSettingsGetDefaultStringValue(SettingsID settingId)
{
    l_Setting setting = get_setting(settingId);
    return setting.DefaultValue.stringValue;
}

std::vector<int> CoreSettingsGetDefaultIntListValue(SettingsID settingId)
{
    l_Setting setting = get_setting(settingId);
    return setting.DefaultValue.intListValue;
}

int CoreSettingsGetIntValue(SettingsID settingId)
{
    l_Setting setting = get_setting(settingId);
    int value = setting.DefaultValue.intValue;
    config_option_get(setting.Section, setting.Key, M64TYPE_INT, &value, sizeof(value));
    return value;
}

bool CoreSettingsGetBoolValue(SettingsID settingId)
{
    l_Setting setting = get_setting(settingId);
    int value = setting.DefaultValue.boolValue ? 1 : 0;
    config_option_get(setting.Section, setting.Key, M64TYPE_BOOL, &value, sizeof(value));
    return value > 0;
}

float CoreSettingsGetFloatValue(SettingsID settingId)
{
    l_Setting setting = get_setting(settingId);
    float value = setting.DefaultValue.floatValue;
    config_option_get(setting.Section, setting.Key, M64TYPE_FLOAT, &value, sizeof(value));
    return value;
}

std::string CoreSettingsGetStringValue(SettingsID settingId)
{
    l_Setting setting = get_setting(settingId);
    char value[STR_SIZE] = {0};
    config_option_get(setting.Section, setting.Key, M64TYPE_STRING, (char*)value, sizeof(value));
    return std::string(value);
}

std::vector<int> CoreSettingsGetIntListValue(SettingsID settingId)
{
    l_Setting setting = get_setting(settingId);
    return CoreSettingsGetIntListValue(settingId, setting.Section);
}

std::vector<std::string> CoreSettingsGetStringListValue(SettingsID settingId)
{
    l_Setting setting = get_setting(settingId);
    return CoreSettingsGetStringListValue(settingId, setting.Section);
}

int CoreSettingsGetIntValue(SettingsID settingId, std::string section)
{
    l_Setting setting = get_setting(settingId);
    int value = setting.DefaultValue.intValue;
    config_option_get(section, setting.Key, M64TYPE_INT, &value, sizeof(value));
    return value;
}

bool CoreSettingsGetBoolValue(SettingsID settingId, std::string section)
{
    l_Setting setting = get_setting(settingId);
    int value = setting.DefaultValue.boolValue;
    config_option_get(section, setting.Key, M64TYPE_BOOL, &value, sizeof(value));
    return value;
}

float CoreSettingsGetFloatValue(SettingsID settingId, std::string section)
{
    l_Setting setting = get_setting(settingId);
    float value = setting.DefaultValue.floatValue;
    config_option_get(section, setting.Key, M64TYPE_FLOAT, &value, sizeof(value));
    return value;
}

std::string CoreSettingsGetStringValue(SettingsID settingId, std::string section)
{
    l_Setting setting = get_setting(settingId);
    char value[STR_SIZE] = {0};
    config_option_get(section, setting.Key, M64TYPE_STRING, (char*)value, sizeof(value));
    return std::string(value);
}

std::vector<int> CoreSettingsGetIntListValue(SettingsID settingId, std::string section)
{
    l_Setting setting = get_setting(settingId);
    std::vector<int> value;

    std::string value_str;
    value_str = CoreSettingsGetStringValue(settingId, section);

    if (!string_to_int_list(value_str, value))
    {
        return std::vector<int>();
    }

    return value;
}

std::vector<std::string> CoreSettingsGetStringListValue(SettingsID settingId, std::string section)
{
    l_Setting setting = get_setting(settingId);
    std::vector<std::string> value;

    std::string value_str;
    value_str = CoreSettingsGetStringValue(settingId, section);

    if (!string_to_string_list(value_str, value))
    {
        return std::vector<std::string>();
    }

    return value;
}


int CoreSettingsGetIntValue(std::string section, std::string key, int defaultValue)
{
    int value = defaultValue;
    config_option_get(section, key, M64TYPE_INT, &value, sizeof(value));
    return value;
}

bool CoreSettingsGetBoolValue(std::string section, std::string key, bool defaultValue)
{
    int value = defaultValue;
    config_option_get(section, key, M64TYPE_BOOL, &value, sizeof(value));
    return value;
}

float CoreSettingsGetFloatValue(std::string section, std::string key, float defaultValue)
{
    float value = defaultValue;
    config_option_get(section, key, M64TYPE_FLOAT, &value, sizeof(value));
    return value;
}

std::string CoreSettingsGetStringValue(std::string section, std::string key)
{
    char value[STR_SIZE] = {0};
    config_option_get(section, key, M64TYPE_STRING, (char*)value, sizeof(value));
    return std::string(value);
}

std::vector<int> CoreSettingsGetIntListValue(std::string section, std::string key)
{
    std::vector<int> value;

    std::string value_str;
    value_str = CoreSettingsGetStringValue(section, key);

    if (!string_to_int_list(value_str, value))
    {
        return std::vector<int>();
    }

    return value;
}

std::vector<std::string> CoreSettingsGetStringListValue(std::string section, std::string key)
{
    std::vector<std::string> value;

    std::string value_str;
    value_str = CoreSettingsGetStringValue(section, key);

    if (!string_to_string_list(value_str, value))
    {
        return std::vector<std::string>();
    }

    return value;
}
