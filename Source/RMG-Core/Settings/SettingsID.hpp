#ifndef CORE_SETTINGSID_HPP
#define CORE_SETTINGSID_HPP

enum class SettingsID
{
    // GUI Settings
    GUI_SettingsDialogWidth = 0,
    GUI_SettingsDialogHeight,
    GUI_AllowManualResizing,
    GUI_HideCursorInEmulation,
    GUI_StatusbarMessageDuration,

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
    Core_64DD_RomFile,

    // (mupen64plus) Core Settings
    Core_OverrideGameSpecificSettings,
    Core_RandomizeInterrupt,
    Core_CPU_Emulator,
    Core_DisableExtraMem,
    Core_EnableDebugger,
    Core_CountPerOp,
    Core_SiDmaDuration,

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

    Invalid
};

#endif // CORE_SETTINGSID_HPP
