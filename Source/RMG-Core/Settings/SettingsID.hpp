#ifndef CORE_SETTINGSID_HPP
#define CORE_SETTINGSID_HPP

enum class SettingsID
{
    // GUI Settings
    GUI_SettingsDialogWidth = 0,
    GUI_SettingsDialogHeight,
    GUI_AllowManualResizing,
    GUI_HideCursorInEmulation,
    GUI_HideCursorInFullscreenEmulation,
    GUI_StatusbarMessageDuration,
    GUI_PauseEmulationOnFocusLoss,
    GUI_ResumeEmulationOnFocus,
    GUI_Version,
    
    // Core Plugin Settings
    Core_GFX_Plugin,
    Core_AUDIO_Plugin,
    Core_INPUT_Plugin,
    Core_RSP_Plugin,

    // Core User Directory Settings
    Core_OverrideUserDirs,
    Core_UserDataDirOverride,
    Core_UserCacheDirOverride,

    // Core 64DD ROM Settings
    Core_64DD_JapaneseIPL,
    Core_64DD_AmericanIPL,
    Core_64DD_DevelopmentIPL,
    Core_64DD_SaveDiskFormat,

    // (mupen64plus) Core Settings
    Core_OverrideGameSpecificSettings,
    Core_RandomizeInterrupt,
    Core_CPU_Emulator,
    Core_DisableExtraMem,
    Core_EnableDebugger,
    Core_CountPerOp,
    Core_SiDmaDuration,

    // (mupen64plus) Overlay Core Settings
    CoreOverlay_RandomizeInterrupt,
    CoreOverlay_CPU_Emulator,
    CoreOverlay_DisableExtraMem,
    CoreOverlay_EnableDebugger,
    CoreOverlay_CountPerOp,
    CoreOverlay_SiDmaDuration,

    // (mupen64plus) Core Directory Settings
    Core_ScreenshotPath,
    Core_SaveStatePath,
    Core_SaveSRAMPath,
    Core_SharedDataPath,

    // Game Specific Settings
    Game_DisableExtraMem,
    Game_SaveType,
    Game_CountPerOp,
    Game_SiDmaDuration,

    // Game Core Override Settings
    Game_OverrideCoreSettings,
    Game_CPU_Emulator,
    Game_RandomizeInterrupt,

    // Game Plugin Settings
    Game_GFX_Plugin,
    Game_AUDIO_Plugin,
    Game_INPUT_Plugin,
    Game_RSP_Plugin,

    // GUI KeyBindings
    KeyBinding_OpenROM,
    KeyBinding_OpenCombo,
    KeyBinding_StartEmulation,
    KeyBinding_EndEmulation,
    KeyBinding_RefreshROMList,
    KeyBinding_Exit,
    KeyBinding_SoftReset,
    KeyBinding_HardReset,
    KeyBinding_Resume,
    KeyBinding_GenerateBitmap,
    KeyBinding_LimitFPS,
    KeyBinding_SwapDisk,
    KeyBinding_SaveState,
    KeyBinding_SaveAs,
    KeyBinding_LoadState,
    KeyBinding_Load,
    KeyBinding_Cheats,
    KeyBinding_GSButton,
    KeyBinding_Fullscreen,
    KeyBinding_Settings,

    // RomBrowser Settings
    RomBrowser_Directory,
    RomBrowser_Geometry,
    RomBrowser_Recursive,
    RomBrowser_MaxItems,
    RomBrowser_Columns,
    RomBrowser_ColumnSizes,

    // Settings Settings
    Settings_HasForceUsedSetOnce,

    // Audio Plugin Settings
    Audio_Volume,
    Audio_Muted,

    // Input Plugin Settings
    Input_PluggedIn,
    Input_DeviceType,
    Input_DeviceName,
    Input_DeviceNum,
    Input_Range,
    Input_Deadzone,
    Input_Pak,
    Input_RemoveDuplicateMappings,
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

    Invalid
};

#endif // CORE_SETTINGSID_HPP
