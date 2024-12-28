/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_SETTINGS_HPP
#define CORE_SETTINGS_HPP

#include <string>
#include <vector>

enum class SettingsID
{
    // GUI Settings
    GUI_HideCursorInEmulation = 0,
    GUI_HideCursorInFullscreenEmulation,
    GUI_StatusbarMessageDuration,
    GUI_PauseEmulationOnFocusLoss,
    GUI_ResumeEmulationOnFocus,
    GUI_AutomaticFullscreen,
    GUI_ConfirmDragDrop,
    GUI_OpenGLES,
    GUI_ShowVerboseLogMessages,
    GUI_OnScreenDisplayEnabled,
    GUI_OnScreenDisplayLocation,
    GUI_OnScreenDisplayPaddingX,
    GUI_OnScreenDisplayPaddingY,
    GUI_OnScreenDisplayBackgroundColor,
    GUI_OnScreenDisplayTextColor,
    GUI_OnScreenDisplayDuration,
    GUI_Toolbar,
    GUI_ToolbarArea,
    GUI_StatusBar,
    GUI_Theme,
    GUI_IconTheme,
    GUI_CheckForUpdates,
    GUI_LastUpdateCheck,
    GUI_DiscordRpc,
    GUI_Version,

    // Netplay Settings
    Netplay_Nickname,
    Netplay_ServerJsonUrl,

    // Core Plugin Settings
    Core_GFX_Plugin,
    Core_AUDIO_Plugin,
    Core_INPUT_Plugin,
    Core_RSP_Plugin,
    Core_EXECUTION_Plugin,

    // Core User Directory Settings
    Core_OverrideUserDirs,
    Core_UserDataDirOverride,
    Core_UserCacheDirOverride,

    // Core 64DD ROM Settings
    Core_64DD_JapaneseIPL,
    Core_64DD_AmericanIPL,
    Core_64DD_DevelopmentIPL,
    Core_64DD_SaveDiskFormat,

    // Core PIF ROM Settings
    Core_PIF_Use,
    Core_PIF_NTSC,
    Core_PIF_PAL,

    // Core Gameboy ROM settings
    Core_Gameboy_P1_Rom,
    Core_Gameboy_P1_Save,
    Core_Gameboy_P2_Rom,
    Core_Gameboy_P2_Save,
    Core_Gameboy_P3_Rom,
    Core_Gameboy_P3_Save,
    Core_Gameboy_P4_Rom,
    Core_Gameboy_P4_Save,

    // (mupen64plus) Core Settings
    Core_OverrideGameSpecificSettings,
    Core_RandomizeInterrupt,
    Core_CPU_Emulator,
    Core_DisableExtraMem,
    Core_EnableDebugger,
    Core_CountPerOp,
    Core_CountPerOpDenomPot,
    Core_SiDmaDuration,
    Core_SaveFileNameFormat,

    // (mupen64plus) Overlay Core Settings
    CoreOverlay_RandomizeInterrupt,
    CoreOverlay_CPU_Emulator,
    CoreOverlay_DisableExtraMem,
    CoreOverlay_EnableDebugger,
    CoreOverlay_CountPerOp,
    CoreOverlay_CountPerOpDenomPot,
    CoreOverlay_SiDmaDuration,
    CoreOverLay_SaveFileNameFormat,

    // (mupen64plus) Core Directory Settings
    Core_ScreenshotPath,
    Core_SaveStatePath,
    Core_SaveSRAMPath,

    // Game Specific Settings
    Game_OverrideSettings,
    Game_DisableExtraMem,
    Game_TransferPak,
    Game_SaveType,
    Game_CountPerOp,
    Game_SiDmaDuration,

    // Game Core Override Settings
    Game_OverrideCoreSettings,
    Game_CPU_Emulator,
    Game_CountPerOpDenomPot,
    Game_RandomizeInterrupt,

    // Game Plugin Settings
    Game_GFX_Plugin,
    Game_AUDIO_Plugin,
    Game_INPUT_Plugin,
    Game_RSP_Plugin,
    Game_EXECUTION_Plugin,

    // GUI KeyBindings
    KeyBinding_RemoveDuplicates,
    KeyBinding_StartROM,
    KeyBinding_StartCombo,
    KeyBinding_Shutdown,
    KeyBinding_RefreshROMList,
    KeyBinding_Exit,
    KeyBinding_SoftReset,
    KeyBinding_HardReset,
    KeyBinding_Resume,
    KeyBinding_Screenshot,
    KeyBinding_LimitFPS,
    KeyBinding_SpeedFactor25,
    KeyBinding_SpeedFactor50,
    KeyBinding_SpeedFactor75,
    KeyBinding_SpeedFactor100,
    KeyBinding_SpeedFactor125,
    KeyBinding_SpeedFactor150,
    KeyBinding_SpeedFactor175,
    KeyBinding_SpeedFactor200,
    KeyBinding_SpeedFactor225,
    KeyBinding_SpeedFactor250,
    KeyBinding_SpeedFactor275,
    KeyBinding_SpeedFactor300,
    KeyBinding_SaveState,
    KeyBinding_SaveAs,
    KeyBinding_LoadState,
    KeyBinding_Load,
    KeyBinding_Cheats,
    KeyBinding_GSButton,
    KeyBinding_SaveStateSlot0,
    KeyBinding_SaveStateSlot1,
    KeyBinding_SaveStateSlot2,
    KeyBinding_SaveStateSlot3,
    KeyBinding_SaveStateSlot4,
    KeyBinding_SaveStateSlot5,
    KeyBinding_SaveStateSlot6,
    KeyBinding_SaveStateSlot7,
    KeyBinding_SaveStateSlot8,
    KeyBinding_SaveStateSlot9,
    KeyBinding_Fullscreen,
    Keybinding_ViewLog,
    KeyBinding_GraphicsSettings,
    KeyBinding_AudioSettings,
    KeyBinding_RspSettings,
    KeyBinding_InputSettings,
    KeyBinding_Settings,
    KeyBinding_IncreaseVolume,
    KeyBinding_DecreaseVolume,
    KeyBinding_ToggleMuteVolume,

    // RomBrowser Settings
    RomBrowser_Directory,
    RomBrowser_Geometry,
    RomBrowser_Maximized,
    RomBrowser_Recursive,
    RomBrowser_MaxItems,
    RomBrowser_ColumnVisibility,
    RomBrowser_ColumnOrder,
    RomBrowser_ColumnSizes,
    RomBrowser_SortAfterSearch,
    RomBrowser_ViewMode,
    RomBrowser_ListViewSortSection,
    RomBrowser_ListViewSortOrder,
    RomBrowser_GridViewIconWidth,
    RomBrowser_GridViewIconHeight,
    RomBrowser_GridViewUniformItemSizes,
    
    // Settings Settings
    Settings_HasForceUsedSetOnce,

    // Audio Plugin Settings
    Audio_DefaultFrequency,
    Audio_SwapChannels,
    Audio_PrimaryBufferSize,
    Audio_PrimaryBufferTarget,
    Audio_SecondaryBufferSize,
    Audio_Resampler,
    Audio_Volume,
    Audio_Muted,
    Audio_Synchronize,
    Audio_SimpleBackend,

    // HLE RSP Plugin Settings
    RSP_Version,
    RSP_Fallback,
    RSP_GraphicsHLE,
    RSP_AudioHLE,

    // Input Plugin Settings
    Input_Profiles,
    Input_ControllerMode,
    Input_UseProfile,
    Input_UseGameProfile,
    Input_PluggedIn,
    Input_DeviceType,
    Input_DeviceName,
    Input_DeviceNum,
    Input_Deadzone,
    Input_Sensitivity,
    Input_Pak,
    Input_GameboyRom,
    Input_GameboySave,
    Input_RemoveDuplicateMappings,
    Input_FilterEventsForButtons,
    Input_FilterEventsForAxis,
    Input_A_InputType,
    Input_A_Name,
    Input_A_Data,
    Input_A_ExtraData,
    Input_B_InputType,
    Input_B_Name,
    Input_B_Data,
    Input_B_ExtraData,
    Input_Start_InputType,
    Input_Start_Name,
    Input_Start_Data,
    Input_Start_ExtraData,
    Input_DpadUp_InputType,
    Input_DpadUp_Name,
    Input_DpadUp_Data,
    Input_DpadUp_ExtraData,
    Input_DpadDown_InputType,
    Input_DpadDown_Name,
    Input_DpadDown_Data,
    Input_DpadDown_ExtraData,
    Input_DpadLeft_InputType,
    Input_DpadLeft_Name,
    Input_DpadLeft_Data,
    Input_DpadLeft_ExtraData,
    Input_DpadRight_InputType,
    Input_DpadRight_Name,
    Input_DpadRight_Data,
    Input_DpadRight_ExtraData,
    Input_CButtonUp_InputType,
    Input_CButtonUp_Name,
    Input_CButtonUp_Data,
    Input_CButtonUp_ExtraData,
    Input_CButtonDown_InputType,
    Input_CButtonDown_Name,
    Input_CButtonDown_Data,
    Input_CButtonDown_ExtraData,
    Input_CButtonLeft_InputType,
    Input_CButtonLeft_Name,
    Input_CButtonLeft_Data,
    Input_CButtonLeft_ExtraData,
    Input_CButtonRight_InputType,
    Input_CButtonRight_Name,
    Input_CButtonRight_Data,
    Input_CButtonRight_ExtraData,
    Input_LeftTrigger_InputType,
    Input_LeftTrigger_Name,
    Input_LeftTrigger_Data,
    Input_LeftTrigger_ExtraData,
    Input_RightTrigger_InputType,
    Input_RightTrigger_Name,
    Input_RightTrigger_Data,
    Input_RightTrigger_ExtraData,
    Input_ZTrigger_InputType,
    Input_ZTrigger_Name,
    Input_ZTrigger_Data,
    Input_ZTrigger_ExtraData,
    Input_AnalogStickUp_InputType,
    Input_AnalogStickUp_Name,
    Input_AnalogStickUp_Data,
    Input_AnalogStickUp_ExtraData,
    Input_AnalogStickDown_InputType,
    Input_AnalogStickDown_Name,
    Input_AnalogStickDown_Data,
    Input_AnalogStickDown_ExtraData,
    Input_AnalogStickLeft_InputType,
    Input_AnalogStickLeft_Name,
    Input_AnalogStickLeft_Data,
    Input_AnalogStickLeft_ExtraData,
    Input_AnalogStickRight_InputType,
    Input_AnalogStickRight_Name,
    Input_AnalogStickRight_Data,
    Input_AnalogStickRight_ExtraData,
    Input_Hotkey_Shutdown_InputType,
    Input_Hotkey_Shutdown_Name,
    Input_Hotkey_Shutdown_Data,
    Input_Hotkey_Shutdown_ExtraData,
    Input_Hotkey_Exit_InputType,
    Input_Hotkey_Exit_Name,
    Input_Hotkey_Exit_Data,
    Input_Hotkey_Exit_ExtraData,
    Input_Hotkey_SoftReset_InputType,
    Input_Hotkey_SoftReset_Name,
    Input_Hotkey_SoftReset_Data,
    Input_Hotkey_SoftReset_ExtraData,
    Input_Hotkey_HardReset_InputType,
    Input_Hotkey_HardReset_Name,
    Input_Hotkey_HardReset_Data,
    Input_Hotkey_HardReset_ExtraData,
    Input_Hotkey_Resume_InputType,
    Input_Hotkey_Resume_Name,
    Input_Hotkey_Resume_Data,
    Input_Hotkey_Resume_ExtraData,
    Input_Hotkey_Screenshot_InputType,
    Input_Hotkey_Screenshot_Name,
    Input_Hotkey_Screenshot_Data,
    Input_Hotkey_Screenshot_ExtraData,
    Input_Hotkey_LimitFPS_InputType,
    Input_Hotkey_LimitFPS_Name,
    Input_Hotkey_LimitFPS_Data,
    Input_Hotkey_LimitFPS_ExtraData,
    Input_Hotkey_SpeedFactor25_InputType,
    Input_Hotkey_SpeedFactor25_Name,
    Input_Hotkey_SpeedFactor25_Data,
    Input_Hotkey_SpeedFactor25_ExtraData,
    Input_Hotkey_SpeedFactor50_InputType,
    Input_Hotkey_SpeedFactor50_Name,
    Input_Hotkey_SpeedFactor50_Data,
    Input_Hotkey_SpeedFactor50_ExtraData,
    Input_Hotkey_SpeedFactor75_InputType,
    Input_Hotkey_SpeedFactor75_Name,
    Input_Hotkey_SpeedFactor75_Data,
    Input_Hotkey_SpeedFactor75_ExtraData,
    Input_Hotkey_SpeedFactor100_InputType,
    Input_Hotkey_SpeedFactor100_Name,
    Input_Hotkey_SpeedFactor100_Data,
    Input_Hotkey_SpeedFactor100_ExtraData,
    Input_Hotkey_SpeedFactor125_InputType,
    Input_Hotkey_SpeedFactor125_Name,
    Input_Hotkey_SpeedFactor125_Data,
    Input_Hotkey_SpeedFactor125_ExtraData,
    Input_Hotkey_SpeedFactor150_InputType,
    Input_Hotkey_SpeedFactor150_Name,
    Input_Hotkey_SpeedFactor150_Data,
    Input_Hotkey_SpeedFactor150_ExtraData,
    Input_Hotkey_SpeedFactor175_InputType,
    Input_Hotkey_SpeedFactor175_Name,
    Input_Hotkey_SpeedFactor175_Data,
    Input_Hotkey_SpeedFactor175_ExtraData,
    Input_Hotkey_SpeedFactor200_InputType,
    Input_Hotkey_SpeedFactor200_Name,
    Input_Hotkey_SpeedFactor200_Data,
    Input_Hotkey_SpeedFactor200_ExtraData,
    Input_Hotkey_SpeedFactor225_InputType,
    Input_Hotkey_SpeedFactor225_Name,
    Input_Hotkey_SpeedFactor225_Data,
    Input_Hotkey_SpeedFactor225_ExtraData,
    Input_Hotkey_SpeedFactor250_InputType,
    Input_Hotkey_SpeedFactor250_Name,
    Input_Hotkey_SpeedFactor250_Data,
    Input_Hotkey_SpeedFactor250_ExtraData,
    Input_Hotkey_SpeedFactor275_InputType,
    Input_Hotkey_SpeedFactor275_Name,
    Input_Hotkey_SpeedFactor275_Data,
    Input_Hotkey_SpeedFactor275_ExtraData,
    Input_Hotkey_SpeedFactor300_InputType,
    Input_Hotkey_SpeedFactor300_Name,
    Input_Hotkey_SpeedFactor300_Data,
    Input_Hotkey_SpeedFactor300_ExtraData,
    Input_Hotkey_SaveState_InputType,
    Input_Hotkey_SaveState_Name,
    Input_Hotkey_SaveState_Data,
    Input_Hotkey_SaveState_ExtraData,
    Input_Hotkey_LoadState_InputType,
    Input_Hotkey_LoadState_Name,
    Input_Hotkey_LoadState_Data,
    Input_Hotkey_LoadState_ExtraData,
    Input_Hotkey_GSButton_InputType,
    Input_Hotkey_GSButton_Name,
    Input_Hotkey_GSButton_Data,
    Input_Hotkey_GSButton_ExtraData,
    Input_Hotkey_SaveStateSlot0_InputType,
    Input_Hotkey_SaveStateSlot0_Name,
    Input_Hotkey_SaveStateSlot0_Data,
    Input_Hotkey_SaveStateSlot0_ExtraData,
    Input_Hotkey_SaveStateSlot1_InputType,
    Input_Hotkey_SaveStateSlot1_Name,
    Input_Hotkey_SaveStateSlot1_Data,
    Input_Hotkey_SaveStateSlot1_ExtraData,
    Input_Hotkey_SaveStateSlot2_InputType,
    Input_Hotkey_SaveStateSlot2_Name,
    Input_Hotkey_SaveStateSlot2_Data,
    Input_Hotkey_SaveStateSlot2_ExtraData,
    Input_Hotkey_SaveStateSlot3_InputType,
    Input_Hotkey_SaveStateSlot3_Name,
    Input_Hotkey_SaveStateSlot3_Data,
    Input_Hotkey_SaveStateSlot3_ExtraData,
    Input_Hotkey_SaveStateSlot4_InputType,
    Input_Hotkey_SaveStateSlot4_Name,
    Input_Hotkey_SaveStateSlot4_Data,
    Input_Hotkey_SaveStateSlot4_ExtraData,
    Input_Hotkey_SaveStateSlot5_InputType,
    Input_Hotkey_SaveStateSlot5_Name,
    Input_Hotkey_SaveStateSlot5_Data,
    Input_Hotkey_SaveStateSlot5_ExtraData,
    Input_Hotkey_SaveStateSlot6_InputType,
    Input_Hotkey_SaveStateSlot6_Name,
    Input_Hotkey_SaveStateSlot6_Data,
    Input_Hotkey_SaveStateSlot6_ExtraData,
    Input_Hotkey_SaveStateSlot7_InputType,
    Input_Hotkey_SaveStateSlot7_Name,
    Input_Hotkey_SaveStateSlot7_Data,
    Input_Hotkey_SaveStateSlot7_ExtraData,
    Input_Hotkey_SaveStateSlot8_InputType,
    Input_Hotkey_SaveStateSlot8_Name,
    Input_Hotkey_SaveStateSlot8_Data,
    Input_Hotkey_SaveStateSlot8_ExtraData,
    Input_Hotkey_SaveStateSlot9_InputType,
    Input_Hotkey_SaveStateSlot9_Name,
    Input_Hotkey_SaveStateSlot9_Data,
    Input_Hotkey_SaveStateSlot9_ExtraData,
    Input_Hotkey_IncreaseSaveStateSlot_InputType,
    Input_Hotkey_IncreaseSaveStateSlot_Name,
    Input_Hotkey_IncreaseSaveStateSlot_Data,
    Input_Hotkey_IncreaseSaveStateSlot_ExtraData,
    Input_Hotkey_DecreaseSaveStateSlot_InputType,
    Input_Hotkey_DecreaseSaveStateSlot_Name,
    Input_Hotkey_DecreaseSaveStateSlot_Data,
    Input_Hotkey_DecreaseSaveStateSlot_ExtraData,
    Input_Hotkey_MemoryPak_InputType,
    Input_Hotkey_MemoryPak_Name,
    Input_Hotkey_MemoryPak_Data,
    Input_Hotkey_MemoryPak_ExtraData,
    Input_Hotkey_RumblePak_InputType,
    Input_Hotkey_RumblePak_Name,
    Input_Hotkey_RumblePak_Data,
    Input_Hotkey_RumblePak_ExtraData,
    Input_Hotkey_NoPak_InputType,
    Input_Hotkey_NoPak_Name,
    Input_Hotkey_NoPak_Data,
    Input_Hotkey_NoPak_ExtraData,
    Input_Hotkey_Fullscreen_InputType,
    Input_Hotkey_Fullscreen_Name,
    Input_Hotkey_Fullscreen_Data,
    Input_Hotkey_Fullscreen_ExtraData,

    Invalid
};

// saves settings to file
bool CoreSettingsSave(void);

// upgrades existing settings to new version
bool CoreSettingsUpgrade(void);

// setup default settings
bool CoreSettingsSetupDefaults(void);

// returns whether a section exists
bool CoreSettingsSectionExists(std::string section);

// reverts section to the one from the settings
bool CoreSettingsRevertSection(std::string section);

// deletes given section
bool CoreSettingsDeleteSection(std::string section);

// returns whether a key in the given section exists
bool CoreSettingsKeyExists(std::string section, std::string key);

// sets setting as int value
bool CoreSettingsSetValue(SettingsID settingId, int value);
// sets setting as bool value
bool CoreSettingsSetValue(SettingsID settingId, bool value);
// sets setting as float value
bool CoreSettingsSetValue(SettingsID settingId, float value);
// sets setting as string value
bool CoreSettingsSetValue(SettingsID settingId, std::string value);
// sets setting as int list value
bool CoreSettingsSetValue(SettingsID settingId, std::vector<int> value);
// sets setting as string list value
bool CoreSettingsSetValue(SettingsID settingId, std::vector<std::string> value);

// sets setting in section as int value
bool CoreSettingsSetValue(SettingsID settingId, std::string section, int value);
// sets setting in section as bool value
bool CoreSettingsSetValue(SettingsID settingId, std::string section, bool value);
// sets setting in section as float value
bool CoreSettingsSetValue(SettingsID settingId, std::string section, float value);
// sets setting in section as string value
bool CoreSettingsSetValue(SettingsID settingId, std::string section, std::string value);
// sets setting in section as int list value
bool CoreSettingsSetValue(SettingsID settingId, std::string section, std::vector<int> value);
// sets setting as section as string list value
bool CoreSettingsSetValue(SettingsID settingId, std::string section, std::vector<std::string> value);

#ifdef CORE_INTERNAL
// sets key in section as int value
bool CoreSettingsSetValue(std::string section, std::string key, int value);
// sets key in section as bool value
bool CoreSettingsSetValue(std::string section, std::string key, bool value);
// sets key in section as float value
bool CoreSettingsSetValue(std::string section, std::string key, float value);
// sets key in section as string value
bool CoreSettingsSetValue(std::string section, std::string key, std::string value);
// sets key in section as int list value
bool CoreSettingsSetValue(std::string section, std::string key, std::vector<int> value);
// sets key in section as string list value
bool CoreSettingsSetValue(std::string section, std::string key, std::vector<std::string> value);
#endif // CORE_INTERNAL

// retrieves default setting as int
int CoreSettingsGetDefaultIntValue(SettingsID settingId);
// retrieves default setting as bool
bool CoreSettingsGetDefaultBoolValue(SettingsID settingId);
// retrieves default setting as float
float CoreSettingsGetDefaultFloatValue(SettingsID settingId);
// retrieves default setting as string
std::string CoreSettingsGetDefaultStringValue(SettingsID settingId);
// retrieves default setting as int list
std::vector<int> CoreSettingsGetDefaultIntListValue(SettingsID settingId);
// retrieves default setting as string list
std::vector<std::string> CoreSettingsGetDefaultStringListValue(SettingsID settingId);

// retrieves setting as int
int CoreSettingsGetIntValue(SettingsID settingId);
// retrieves setting as bool
bool CoreSettingsGetBoolValue(SettingsID settingId);
// retrieves setting as float
float CoreSettingsGetFloatValue(SettingsID settingId);
// retrieves setting as string
std::string CoreSettingsGetStringValue(SettingsID settingId);
// retrieves setting as int list
std::vector<int> CoreSettingsGetIntListValue(SettingsID settingId);
// retrieves setting as string list
std::vector<std::string> CoreSettingsGetStringListValue(SettingsID settingId);

// retrieves setting in section as int
int CoreSettingsGetIntValue(SettingsID settingId, std::string section);
// retrieves setting in section as bool
bool CoreSettingsGetBoolValue(SettingsID settingId, std::string section);
// retrieves setting in section as float
float CoreSettingsGetFloatValue(SettingsID settingId, std::string section);
// retrieves setting in section as string
std::string CoreSettingsGetStringValue(SettingsID settingId, std::string section);
// retrieves setting in section as int list
std::vector<int> CoreSettingsGetIntListValue(SettingsID settingId, std::string section);
// retrieves setting in section as string list
std::vector<std::string> CoreSettingsGetStringListValue(SettingsID settingId, std::string section);

#ifdef CORE_INTERNAL
// retrieves key in section as int
int CoreSettingsGetIntValue(std::string section, std::string key, int defaultValue);
// retrieves key in section as bool
bool CoreSettingsGetBoolValue(std::string section, std::string key, bool defaultValue);
// retrieves key in section as float
float CoreSettingsGetFloatValue(std::string section, std::string key, float defaultValue);
// retrieves key in section as string
std::string CoreSettingsGetStringValue(std::string section, std::string key);
// retrieves key in section as int list
std::vector<int> CoreSettingsGetIntListValue(std::string section, std::string key);
// retrieves key in section as string list
std::vector<std::string> CoreSettingsGetStringListValue(std::string section, std::string key);
#endif // CORE_INTERNAL

#endif // CORE_SETTINGS_HPP
