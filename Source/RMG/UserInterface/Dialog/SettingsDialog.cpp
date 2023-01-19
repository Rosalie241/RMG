/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "SettingsDialog.hpp"
#include "OnScreenDisplay.hpp"
#include "RMG-Core/DiscordRpc.hpp"
#include "RMG-Core/Settings/Settings.hpp"
#include "UserInterface/Widget/KeybindButton.hpp"

#include <QFileDialog>
#include <QMessageBox>
#include <QDirIterator>
#include <QLabel>

using namespace UserInterface::Dialog;

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    this->setupUi(this);

    this->setIconsForEmulationInfoText();

    this->romOpened = CoreHasRomOpen();
    if (romOpened)
    {
        CoreGetCurrentRomSettings(this->currentGameSettings);
        CoreGetCurrentDefaultRomSettings(this->defaultGameSettings);
        this->gameSection = this->currentGameSettings.MD5;

        // no need to show emulation info text,
        // when we're not running/paused
        if (!CoreIsEmulationRunning() && !CoreIsEmulationPaused())
        {
            this->hideEmulationInfoText();
        }
    }
    else
    {
        this->hideEmulationInfoText();
        this->tabWidget->setTabEnabled(1, false);
    }

    pluginList = CoreGetAllPlugins();

    for (int i = 0; i < 14; i++)
    {
        this->reloadSettings(i);
    }

    // connect hotkey settings to slot
    this->commonHotkeySettings(SettingsDialogAction::ConnectSignals);

#ifndef UPDATER
    this->checkForUpdatesCheckBox->setHidden(true);
#endif // !UPDATER

#ifndef DISCORD_RPC
    this->discordRpcCheckBox->setHidden(true);

    // if both DISCORD_RPC & UPDATER
    // aren't defined, hide the tab
    // with the settings for those
#ifndef UPDATER
    this->innerInterfaceTabWidget->removeTab(5);
#endif // !UPDATER
#endif // !DISCORD_RPC

#ifndef _WIN32
    this->innerInterfaceTabWidget->removeTab(4);
#endif

    int width = CoreSettingsGetIntValue(SettingsID::GUI_SettingsDialogWidth);
    int height = CoreSettingsGetIntValue(SettingsID::GUI_SettingsDialogHeight);

    if (width != 0 && height != 0)
    {
        // center current dialog
        this->setGeometry(
            QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, QSize(width, height), parent->geometry()));
    }
}

SettingsDialog::~SettingsDialog(void)
{
    CoreSettingsSetValue(SettingsID::GUI_SettingsDialogWidth, this->size().width());
    CoreSettingsSetValue(SettingsID::GUI_SettingsDialogHeight, this->size().height());
    CoreSettingsSave();
}

void SettingsDialog::ShowGameTab(void)
{
    this->tabWidget->setCurrentIndex(1);
}

int SettingsDialog::currentIndex(void)
{
    int currentIndex = this->tabWidget->currentIndex();

    if (currentIndex == 1)
    { // game tab
        currentIndex += this->innerGameTabWidget->currentIndex();
    }
    else if (currentIndex > 1)
    { // above game tab
        currentIndex += this->innerGameTabWidget->count() - 1;
    }

    if (currentIndex == 8)
    { // interface tab
        currentIndex += this->innerInterfaceTabWidget->currentIndex();
    }

    return currentIndex;
}

void SettingsDialog::restoreDefaults(int stackedWidgetIndex)
{
    switch (stackedWidgetIndex)
    {
    default:
    case 0:
        this->loadDefaultCoreSettings();
        break;
    case 1:
        this->loadDefaultGameSettings();
        break;
    case 2:
        this->loadDefaultGameCoreSettings();
        break;
    case 3:
        this->loadDefaultGamePluginSettings();
        break;
    case 4:
        this->loadDefaultPluginSettings();
        break;
    case 5:
        this->loadDefaultDirectorySettings();
        break;
    case 6:
        this->loadDefault64DDSettings();
        break;
    case 7:
        this->loadDefaultHotkeySettings();
        break;
    case 8:
        this->loadDefaultInterfaceEmulationSettings();
        break;
    case 9:
        this->loadDefaultInterfaceRomBrowserSettings();
        break;
    case 10:
        this->loadDefaultInterfaceLogWindowSettings();
        break;
    case 11:
        this->loadDefaultInterfaceOSDSettings();
        break;
    case 12:
        this->loadDefaultInterfaceStyleSettings();
        break;
    case 13:
        this->loadDefaultInterfaceMiscSettings();
        break;
    }
}

void SettingsDialog::reloadSettings(int stackedWidgetIndex)
{
    switch (stackedWidgetIndex)
    {
    default:
    case 0:
        this->loadCoreSettings();
        break;
    case 1:
        this->loadGameSettings();
        break;
    case 2:
        this->loadGameCoreSettings();
        break;
    case 3:
        this->loadGamePluginSettings();
        break;
    case 4:
        this->loadPluginSettings();
        break;
    case 5:
        this->loadDirectorySettings();
        break;
    case 6:
        this->load64DDSettings();
        break;
    case 7:
        this->loadHotkeySettings();
        break;
    case 8:
        this->loadInterfaceEmulationSettings();
        break;
    case 9:
        this->loadInterfaceRomBrowserSettings();
        break;
    case 10:
        this->loadInterfaceLogWindowSettings();
        break;
    case 11:
        this->loadInterfaceOSDSettings();
        break;
    case 12:
        this->loadInterfaceStyleSettings();
        break;
    case 13:
        this->loadInterfaceMiscSettings();
        break;
    }
}

void SettingsDialog::loadCoreSettings(void)
{
    bool disableExtraMem = false;
    int counterFactor = 0;
    int cpuEmulator = 0;
    int siDmaDuration = -1;
    bool randomizeInterrupt = true;
    bool debugger = false;
    bool overrideGameSettings = false;

    disableExtraMem = CoreSettingsGetBoolValue(SettingsID::CoreOverlay_DisableExtraMem);
    counterFactor = CoreSettingsGetIntValue(SettingsID::CoreOverlay_CountPerOp);
    cpuEmulator = CoreSettingsGetIntValue(SettingsID::CoreOverlay_CPU_Emulator);
    siDmaDuration = CoreSettingsGetIntValue(SettingsID::CoreOverlay_SiDmaDuration);
    randomizeInterrupt = CoreSettingsGetBoolValue(SettingsID::CoreOverlay_RandomizeInterrupt);
    debugger = CoreSettingsGetBoolValue(SettingsID::CoreOverlay_EnableDebugger);
    overrideGameSettings = CoreSettingsGetBoolValue(SettingsID::Core_OverrideGameSpecificSettings);

    this->coreCpuEmulatorComboBox->setCurrentIndex(cpuEmulator);
    this->coreRandomizeTimingCheckBox->setChecked(randomizeInterrupt);
    //this->coreDebuggerCheckBox->setChecked(debugger);

    this->coreOverrideGameSettingsGroup->setChecked(overrideGameSettings);

    if (!overrideGameSettings)
    {
        disableExtraMem = false;
        counterFactor = 2;
        siDmaDuration = 2304;
    }

    this->coreMemorySizeComboBox->setCurrentIndex(!disableExtraMem);
    this->coreCounterFactorComboBox->setCurrentIndex(counterFactor - 1);
    this->coreSiDmaDurationSpinBox->setValue(siDmaDuration);
}

void SettingsDialog::loadGameSettings(void)
{
    this->gameGoodNameLineEdit->setText(QString::fromStdString(this->currentGameSettings.GoodName));
    this->gameMemorySizeComboBox->setCurrentIndex(!this->currentGameSettings.DisableExtraMem);
    this->gameSaveTypeComboBox->setCurrentIndex(this->currentGameSettings.SaveType);
    this->gameCounterFactorComboBox->setCurrentIndex(this->currentGameSettings.CountPerOp - 1);
    this->gameSiDmaDurationSpinBox->setValue(this->currentGameSettings.SiDMADuration);
}

void SettingsDialog::loadGameCoreSettings(void)
{
    bool overrideEnabled, randomizeInterrupt;
    int cpuEmulator = 0, overclockingFactor = 0;

    overrideEnabled = CoreSettingsGetBoolValue(SettingsID::Game_OverrideCoreSettings, this->gameSection);
    cpuEmulator = CoreSettingsGetIntValue(SettingsID::Game_CPU_Emulator, this->gameSection);
    overclockingFactor = CoreSettingsGetIntValue(SettingsID::Game_CountPerOpDenomPot, this->gameSection);
    randomizeInterrupt = CoreSettingsGetBoolValue(SettingsID::Game_RandomizeInterrupt, this->gameSection);

    gameOverrideCoreSettingsGroupBox->setChecked(overrideEnabled);
    gameCoreCpuEmulatorComboBox->setCurrentIndex(cpuEmulator);
    gameOverclockingFactorComboBox->setCurrentIndex(overclockingFactor);
    gameRandomizeTimingCheckBox->setChecked(randomizeInterrupt);
}

void SettingsDialog::loadGamePluginSettings(void)
{
    QComboBox *comboBoxArray[] = {this->gameRspPluginsComboBox, this->gameVideoPluginsComboBox,
                                   this->gameAudioPluginsComboBox, this->gameInputPluginsComboBox};
    SettingsID settingsId[] = {SettingsID::Game_RSP_Plugin, SettingsID::Game_GFX_Plugin, 
                                    SettingsID::Game_AUDIO_Plugin, SettingsID::Game_INPUT_Plugin};
    bool pluginFound[] = {false, false, false, false};

    QComboBox *comboBox;
    int index = 0;

    for (QComboBox *comboBox : comboBoxArray)
    {
        comboBox->clear();
        comboBox->addItem("**Use Core Plugin Settings**");
    }

    for (const auto &p : this->pluginList)
    {
        index = ((int)p.Type - 1);

        comboBox = comboBoxArray[index];
        comboBox->addItem(QString::fromStdString(p.Name), QString::fromStdString(p.File));

        if (CoreSettingsGetStringValue(settingsId[index], this->gameSection) == p.File)
        {
            comboBox->setCurrentText(QString::fromStdString(p.Name));
            pluginFound[index] = true;
        }
    }

    for (int i = 0; i < 4; i++)
    {
        comboBox = comboBoxArray[i];

        if (CoreSettingsGetStringValue(settingsId[i], this->gameSection).empty())
        { // skip empty values
            continue;
        }

        if (!pluginFound[i])
        {
            comboBox->addItem("", "");
            comboBox->setCurrentText("");
        }
    }
}

void SettingsDialog::loadPluginSettings(void)
{
    this->commonPluginSettings(SettingsDialogAction::LoadSettings);
}

void SettingsDialog::loadDirectorySettings(void)
{
    // these need to be static, otherwise Qt will segfault
    static std::string screenshotDir;
    static std::string saveStateDir;
    static std::string saveSramDir;
    static bool overrideUserDirs = false;
    static std::string userDataDir;
    static std::string userCacheDir;

    screenshotDir = CoreSettingsGetStringValue(SettingsID::Core_ScreenshotPath);
    saveStateDir = CoreSettingsGetStringValue(SettingsID::Core_SaveStatePath);
    saveSramDir = CoreSettingsGetStringValue(SettingsID::Core_SaveSRAMPath);

    overrideUserDirs = CoreSettingsGetBoolValue(SettingsID::Core_OverrideUserDirs);
    userDataDir = CoreSettingsGetStringValue(SettingsID::Core_UserDataDirOverride);
    userCacheDir = CoreSettingsGetStringValue(SettingsID::Core_UserCacheDirOverride);

    this->screenshotDirLineEdit->setText(QString::fromStdString(screenshotDir));
    this->saveStateDirLineEdit->setText(QString::fromStdString(saveStateDir));
    this->saveSramDirLineEdit->setText(QString::fromStdString(saveSramDir));
    this->overrideUserDirsGroupBox->setChecked(overrideUserDirs);
    this->userDataDirLineEdit->setText(QString::fromStdString(userDataDir));
    this->userCacheDirLineEdit->setText(QString::fromStdString(userCacheDir));
}

void SettingsDialog::load64DDSettings(void)
{
    std::string japaneseIPLRom;
    std::string americanIPlRom;
    std::string developmentIPLRom;
    int saveDiskFormat = 0;

    japaneseIPLRom = CoreSettingsGetStringValue(SettingsID::Core_64DD_JapaneseIPL);
    americanIPlRom = CoreSettingsGetStringValue(SettingsID::Core_64DD_AmericanIPL);
    developmentIPLRom = CoreSettingsGetStringValue(SettingsID::Core_64DD_DevelopmentIPL);
    saveDiskFormat = CoreSettingsGetIntValue(SettingsID::Core_64DD_SaveDiskFormat);

    this->japaneseIPLRomLineEdit->setText(QString::fromStdString(japaneseIPLRom));
    this->americanIPLRomLineEdit->setText(QString::fromStdString(americanIPlRom));
    this->developmentIPLRomLineEdit->setText(QString::fromStdString(developmentIPLRom));
    this->diskSaveTypeComboBox->setCurrentIndex(saveDiskFormat);
}

void SettingsDialog::loadHotkeySettings(void)
{
    this->commonHotkeySettings(SettingsDialogAction::LoadSettings);
}

void SettingsDialog::loadInterfaceEmulationSettings(void)
{
    this->manualResizingCheckBox->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_AllowManualResizing));
    this->pauseEmulationOnFocusCheckbox->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_PauseEmulationOnFocusLoss));
    this->resumeEmulationOnFocusCheckBox->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_ResumeEmulationOnFocus));
    this->hideCursorCheckBox->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_HideCursorInEmulation));
    this->hideCursorFullscreenCheckBox->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_HideCursorInFullscreenEmulation));
    this->automaticFullscreenCheckbox->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_AutomaticFullscreen));
    this->statusBarMessageDurationSpinBox->setValue(CoreSettingsGetIntValue(SettingsID::GUI_StatusbarMessageDuration));
}

void SettingsDialog::loadInterfaceRomBrowserSettings(void)
{
    this->searchSubDirectoriesCheckbox->setChecked(CoreSettingsGetBoolValue(SettingsID::RomBrowser_Recursive));
    this->romSearchLimitSpinBox->setValue(CoreSettingsGetIntValue(SettingsID::RomBrowser_MaxItems));
}

void SettingsDialog::loadInterfaceLogWindowSettings(void)
{
    this->showVerboseLogMessagesCheckBox->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_ShowVerboseLogMessages));
}

void SettingsDialog::loadInterfaceOSDSettings(void)
{
    this->osdEnabledCheckBox->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_OnScreenDisplayEnabled));
    this->osdLocationComboBox->setCurrentIndex(CoreSettingsGetIntValue(SettingsID::GUI_OnScreenDisplayLocation));
    this->osdVerticalPaddingSpinBox->setValue(CoreSettingsGetIntValue(SettingsID::GUI_OnScreenDisplayPaddingY));
    this->osdHorizontalPaddingSpinBox->setValue(CoreSettingsGetIntValue(SettingsID::GUI_OnScreenDisplayPaddingX));
    this->osdOpacitySpinBox->setValue(CoreSettingsGetFloatValue(SettingsID::GUI_OnScreenDisplayOpacity));
    this->osdDurationSpinBox->setValue(CoreSettingsGetIntValue(SettingsID::GUI_OnScreenDisplayDuration));
}

void SettingsDialog::loadInterfaceStyleSettings(void)
{
    this->commonInterfaceStyleSettings(SettingsDialogAction::LoadSettings);
}

void SettingsDialog::loadInterfaceMiscSettings(void)
{
#ifdef UPDATER
    this->checkForUpdatesCheckBox->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_CheckForUpdates));
#endif // UPDATER
#ifdef DISCORD_RPC
    this->discordRpcCheckBox->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_DiscordRpc));
#endif // DISCORD_RPC
}

void SettingsDialog::loadDefaultCoreSettings(void)
{
    bool disableExtraMem = false;
    int counterFactor = 0;
    int cpuEmulator = 0;
    int siDmaDuration = -1;
    bool randomizeInterrupt = true;
    bool debugger = false;
    bool overrideGameSettings;

    disableExtraMem = CoreSettingsGetDefaultBoolValue(SettingsID::CoreOverlay_DisableExtraMem);
    counterFactor = CoreSettingsGetDefaultIntValue(SettingsID::CoreOverlay_CountPerOp);
    cpuEmulator = CoreSettingsGetDefaultIntValue(SettingsID::CoreOverlay_CPU_Emulator);
    siDmaDuration = CoreSettingsGetDefaultIntValue(SettingsID::CoreOverlay_SiDmaDuration);
    randomizeInterrupt = CoreSettingsGetDefaultBoolValue(SettingsID::CoreOverlay_RandomizeInterrupt);
    debugger = CoreSettingsGetDefaultBoolValue(SettingsID::CoreOverlay_EnableDebugger);
    overrideGameSettings = CoreSettingsGetDefaultBoolValue(SettingsID::Core_OverrideGameSpecificSettings);

    this->coreCpuEmulatorComboBox->setCurrentIndex(cpuEmulator);
    this->coreRandomizeTimingCheckBox->setChecked(randomizeInterrupt);
    //this->coreDebuggerCheckBox->setChecked(debugger);

    this->coreOverrideGameSettingsGroup->setChecked(overrideGameSettings);

    if (!this->coreOverrideGameSettingsGroup->isChecked())
    {
        disableExtraMem = false;
        counterFactor = 2;
        siDmaDuration = 2304;
    }

    this->coreMemorySizeComboBox->setCurrentIndex(!disableExtraMem);
    this->coreCounterFactorComboBox->setCurrentIndex(counterFactor - 1);
    this->coreSiDmaDurationSpinBox->setValue(siDmaDuration);
}

void SettingsDialog::loadDefaultGameSettings(void)
{
    this->gameGoodNameLineEdit->setText(QString::fromStdString(this->defaultGameSettings.GoodName));
    this->gameMemorySizeComboBox->setCurrentIndex(!this->defaultGameSettings.DisableExtraMem);
    this->gameSaveTypeComboBox->setCurrentIndex(this->defaultGameSettings.SaveType);
    this->gameCounterFactorComboBox->setCurrentIndex(this->defaultGameSettings.CountPerOp - 1);
    this->gameSiDmaDurationSpinBox->setValue(this->defaultGameSettings.SiDMADuration);
}

void SettingsDialog::loadDefaultGameCoreSettings(void)
{
    bool overrideEnabled, randomizeInterrupt;
    int cpuEmulator = 0, overclockingFactor = 0;

    overrideEnabled = CoreSettingsGetDefaultBoolValue(SettingsID::Game_OverrideCoreSettings);
    cpuEmulator = CoreSettingsGetDefaultIntValue(SettingsID::Game_CPU_Emulator);
    overclockingFactor = CoreSettingsGetDefaultIntValue(SettingsID::Game_CountPerOpDenomPot);
    randomizeInterrupt = CoreSettingsGetDefaultBoolValue(SettingsID::Game_RandomizeInterrupt);

    gameOverrideCoreSettingsGroupBox->setChecked(overrideEnabled);
    gameCoreCpuEmulatorComboBox->setCurrentIndex(cpuEmulator);
    gameOverclockingFactorComboBox->setCurrentIndex(overclockingFactor);
    gameRandomizeTimingCheckBox->setChecked(randomizeInterrupt);
}

void SettingsDialog::loadDefaultGamePluginSettings(void)
{
    QComboBox *comboBoxArray[] = {this->gameVideoPluginsComboBox, this->gameAudioPluginsComboBox,
                                   this->gameInputPluginsComboBox, this->gameRspPluginsComboBox};

    for (QComboBox *comboBox : comboBoxArray)
    {
        comboBox->setCurrentIndex(0);
    }
}

void SettingsDialog::loadDefaultPluginSettings(void)
{
    this->commonPluginSettings(SettingsDialogAction::LoadDefaultSettings);
}

void SettingsDialog::loadDefaultDirectorySettings(void)
{
    this->screenshotDirLineEdit->setText(QString::fromStdString(CoreSettingsGetDefaultStringValue(SettingsID::Core_ScreenshotPath)));
    this->saveStateDirLineEdit->setText(QString::fromStdString(CoreSettingsGetDefaultStringValue(SettingsID::Core_SaveStatePath)));
    this->saveSramDirLineEdit->setText(QString::fromStdString(CoreSettingsGetDefaultStringValue(SettingsID::Core_SaveSRAMPath)));
    this->overrideUserDirsGroupBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::Core_OverrideUserDirs));
    this->userDataDirLineEdit->setText(QString::fromStdString(CoreSettingsGetDefaultStringValue(SettingsID::Core_UserDataDirOverride)));
    this->userCacheDirLineEdit->setText(QString::fromStdString(CoreSettingsGetDefaultStringValue(SettingsID::Core_UserCacheDirOverride)));
}

void SettingsDialog::loadDefault64DDSettings(void)
{
    this->japaneseIPLRomLineEdit->setText(QString::fromStdString(CoreSettingsGetDefaultStringValue(SettingsID::Core_64DD_JapaneseIPL)));
    this->americanIPLRomLineEdit->setText(QString::fromStdString(CoreSettingsGetDefaultStringValue(SettingsID::Core_64DD_AmericanIPL)));
    this->developmentIPLRomLineEdit->setText(QString::fromStdString(CoreSettingsGetDefaultStringValue(SettingsID::Core_64DD_DevelopmentIPL)));
    this->diskSaveTypeComboBox->setCurrentIndex(CoreSettingsGetDefaultIntValue(SettingsID::Core_64DD_SaveDiskFormat));
}

void SettingsDialog::loadDefaultHotkeySettings(void)
{
    this->commonHotkeySettings(SettingsDialogAction::LoadDefaultSettings);
}

void SettingsDialog::loadDefaultInterfaceEmulationSettings(void)
{
    this->manualResizingCheckBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::GUI_AllowManualResizing));
    this->pauseEmulationOnFocusCheckbox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::GUI_PauseEmulationOnFocusLoss));
    this->resumeEmulationOnFocusCheckBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::GUI_ResumeEmulationOnFocus));
    this->hideCursorCheckBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::GUI_HideCursorInEmulation));
    this->hideCursorFullscreenCheckBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::GUI_HideCursorInFullscreenEmulation));
    this->automaticFullscreenCheckbox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::GUI_AutomaticFullscreen));
    this->statusBarMessageDurationSpinBox->setValue(CoreSettingsGetDefaultIntValue(SettingsID::GUI_StatusbarMessageDuration));
}

void SettingsDialog::loadDefaultInterfaceRomBrowserSettings(void)
{
    this->searchSubDirectoriesCheckbox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::RomBrowser_Recursive));
    this->romSearchLimitSpinBox->setValue(CoreSettingsGetDefaultIntValue(SettingsID::RomBrowser_MaxItems));
}

void SettingsDialog::loadDefaultInterfaceLogWindowSettings(void)
{
    this->showVerboseLogMessagesCheckBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::GUI_ShowVerboseLogMessages));
}

void SettingsDialog::loadDefaultInterfaceOSDSettings(void)
{
    this->osdEnabledCheckBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::GUI_OnScreenDisplayEnabled));
    this->osdLocationComboBox->setCurrentIndex(CoreSettingsGetDefaultIntValue(SettingsID::GUI_OnScreenDisplayLocation));
    this->osdVerticalPaddingSpinBox->setValue(CoreSettingsGetDefaultIntValue(SettingsID::GUI_OnScreenDisplayPaddingY));
    this->osdHorizontalPaddingSpinBox->setValue(CoreSettingsGetDefaultIntValue(SettingsID::GUI_OnScreenDisplayPaddingX));
    this->osdOpacitySpinBox->setValue(CoreSettingsGetDefaultFloatValue(SettingsID::GUI_OnScreenDisplayOpacity));
    this->osdDurationSpinBox->setValue(CoreSettingsGetDefaultIntValue(SettingsID::GUI_OnScreenDisplayDuration));
}

void SettingsDialog::loadDefaultInterfaceStyleSettings(void)
{
    this->commonInterfaceStyleSettings(SettingsDialogAction::LoadDefaultSettings);
}

void SettingsDialog::loadDefaultInterfaceMiscSettings(void)
{
#ifdef UPDATER
    this->checkForUpdatesCheckBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::GUI_CheckForUpdates));
#endif // UPDATER
#ifdef DISCORD_RPC
    this->discordRpcCheckBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::GUI_DiscordRpc));
#endif // DISCORD_RPC
}

void SettingsDialog::saveSettings(void)
{
    this->saveCoreSettings();
    if (romOpened)
    {
        // clean 'game settings'
        CoreSettingsDeleteSection(this->gameSection);
        this->saveGameSettings();
        this->saveGameCoreSettings();
        this->saveGamePluginSettings();
    }
    this->savePluginSettings();
    this->saveDirectorySettings();
    this->save64DDSettings();
    this->saveHotkeySettings();
    this->saveInterfaceEmulationSettings();
    this->saveInterfaceRomBrowserSettings();
    this->saveInterfaceLogWindowSettings();
    this->saveInterfaceOSDSettings();
    this->saveInterfaceStyleSettings();
    this->saveInterfaceMiscSettings();
}

void SettingsDialog::saveCoreSettings(void)
{
    bool disableExtraMem = (this->coreMemorySizeComboBox->currentIndex() == 0);
    int counterFactor = this->coreCounterFactorComboBox->currentIndex() + 1;
    int cpuEmulator = this->coreCpuEmulatorComboBox->currentIndex();
    int siDmaDuration = this->coreSiDmaDurationSpinBox->value();
    bool randomizeInterrupt = this->coreRandomizeTimingCheckBox->isChecked();
    //bool debugger = this->coreDebuggerCheckBox->isChecked();
    bool overrideGameSettings = this->coreOverrideGameSettingsGroup->isChecked();

    CoreSettingsSetValue(SettingsID::CoreOverlay_CPU_Emulator, cpuEmulator);
    CoreSettingsSetValue(SettingsID::CoreOverlay_RandomizeInterrupt, randomizeInterrupt);
    //CoreSettingsSetValue(SettingsID::CoreOverlay_EnableDebugger, debugger);
    CoreSettingsSetValue(SettingsID::Core_OverrideGameSpecificSettings, overrideGameSettings);

    if (!overrideGameSettings)
    {
        disableExtraMem = false;
        counterFactor = 0;
        siDmaDuration = -1;
    }

    CoreSettingsSetValue(SettingsID::CoreOverlay_DisableExtraMem, disableExtraMem);
    CoreSettingsSetValue(SettingsID::CoreOverlay_CountPerOp, counterFactor);
    CoreSettingsSetValue(SettingsID::CoreOverlay_SiDmaDuration, siDmaDuration);
}

void SettingsDialog::saveGameSettings(void)
{
    bool disableExtraMem = this->gameMemorySizeComboBox->currentIndex() == 0;
    int saveType = this->gameSaveTypeComboBox->currentIndex();
    int countPerOp = this->gameCounterFactorComboBox->currentIndex() + 1;
    int siDmaDuration = this->gameSiDmaDurationSpinBox->value();

    if ((this->defaultGameSettings.DisableExtraMem != disableExtraMem) ||
        (this->defaultGameSettings.SaveType != saveType) ||
        (this->defaultGameSettings.CountPerOp != countPerOp) ||
        (this->defaultGameSettings.SiDMADuration != siDmaDuration))
    {
        CoreSettingsSetValue(SettingsID::Game_OverrideSettings, this->gameSection, true);
        CoreSettingsSetValue(SettingsID::Game_DisableExtraMem, this->gameSection, disableExtraMem);
        CoreSettingsSetValue(SettingsID::Game_SaveType, this->gameSection, saveType);
        CoreSettingsSetValue(SettingsID::Game_CountPerOp, this->gameSection, countPerOp);
        CoreSettingsSetValue(SettingsID::Game_SiDmaDuration, this->gameSection, siDmaDuration);
    }
}

void SettingsDialog::saveGameCoreSettings(void)
{
    bool overrideEnabled, randomizeInterrupt;
    bool defaultOverrideEnabled, defaultRandomizeInterrupt;
    int cpuEmulator = 0, defaultCpuEmulator;
    int overclockingFactor = 0, defaultoverclockingFactor;

    overrideEnabled = gameOverrideCoreSettingsGroupBox->isChecked();
    cpuEmulator = gameCoreCpuEmulatorComboBox->currentIndex();
    overclockingFactor = gameOverclockingFactorComboBox->currentIndex();
    randomizeInterrupt = gameRandomizeTimingCheckBox->isChecked();

    defaultOverrideEnabled = CoreSettingsGetDefaultBoolValue(SettingsID::Game_OverrideCoreSettings);
    defaultRandomizeInterrupt = CoreSettingsGetDefaultBoolValue(SettingsID::Game_RandomizeInterrupt);
    defaultCpuEmulator = CoreSettingsGetDefaultIntValue(SettingsID::Game_CPU_Emulator);
    defaultoverclockingFactor = CoreSettingsGetDefaultIntValue(SettingsID::Game_CountPerOpDenomPot);

    if ((defaultOverrideEnabled != overrideEnabled) ||
        (defaultCpuEmulator != cpuEmulator) ||
        (defaultoverclockingFactor != overclockingFactor) ||
        (defaultRandomizeInterrupt != randomizeInterrupt))
    {
        CoreSettingsSetValue(SettingsID::Game_OverrideCoreSettings, this->gameSection, overrideEnabled);
        CoreSettingsSetValue(SettingsID::Game_CPU_Emulator, this->gameSection, cpuEmulator);
        CoreSettingsSetValue(SettingsID::Game_CountPerOpDenomPot, this->gameSection, overclockingFactor);
        CoreSettingsSetValue(SettingsID::Game_RandomizeInterrupt, this->gameSection, randomizeInterrupt);
    }
}

void SettingsDialog::saveGamePluginSettings(void)
{
    QComboBox *comboBoxArray[] = {this->gameVideoPluginsComboBox, this->gameAudioPluginsComboBox,
                                   this->gameInputPluginsComboBox, this->gameRspPluginsComboBox};
    SettingsID settingsIdArray[] = {SettingsID::Game_GFX_Plugin, SettingsID::Game_AUDIO_Plugin,
                                     SettingsID::Game_INPUT_Plugin, SettingsID::Game_RSP_Plugin};
    QComboBox *comboBox;
    SettingsID id;

    for (int i = 0; i < 4; i++)
    {
        comboBox = comboBoxArray[i];
        id = settingsIdArray[i];

        if (comboBox->currentIndex() != 0 )
        {
            CoreSettingsSetValue(id, this->gameSection, comboBox->currentData().toString().toStdString());
        }
    }
}

void SettingsDialog::savePluginSettings(void)
{
    QComboBox *comboBoxArray[] = {this->videoPluginsComboBox, this->audioPluginsComboBox, this->inputPluginsComboBox,
                                   this->rspPluginsComboBox};
    SettingsID settings[] = {SettingsID::Core_GFX_Plugin, SettingsID::Core_AUDIO_Plugin, 
                                SettingsID::Core_INPUT_Plugin, SettingsID::Core_RSP_Plugin};
    QComboBox *comboBox;
    SettingsID settingId;

    for (int i = 0; i < 4; i++)
    {
        comboBox = comboBoxArray[i];
        settingId = settings[i];

        CoreSettingsSetValue(settingId, comboBox->currentData().toString().toStdString());
    }
}

void SettingsDialog::saveDirectorySettings(void)
{
    CoreSettingsSetValue(SettingsID::Core_ScreenshotPath, this->screenshotDirLineEdit->text().toStdString());
    CoreSettingsSetValue(SettingsID::Core_SaveStatePath, this->saveStateDirLineEdit->text().toStdString());
    CoreSettingsSetValue(SettingsID::Core_SaveSRAMPath, this->saveSramDirLineEdit->text().toStdString());

    CoreSettingsSetValue(SettingsID::Core_OverrideUserDirs, this->overrideUserDirsGroupBox->isChecked());
    CoreSettingsSetValue(SettingsID::Core_UserDataDirOverride, this->userDataDirLineEdit->text().toStdString());
    CoreSettingsSetValue(SettingsID::Core_UserCacheDirOverride, this->userCacheDirLineEdit->text().toStdString());
}

void SettingsDialog::save64DDSettings(void)
{
    CoreSettingsSetValue(SettingsID::Core_64DD_JapaneseIPL, this->japaneseIPLRomLineEdit->text().toStdString());
    CoreSettingsSetValue(SettingsID::Core_64DD_AmericanIPL, this->americanIPLRomLineEdit->text().toStdString());
    CoreSettingsSetValue(SettingsID::Core_64DD_DevelopmentIPL, this->developmentIPLRomLineEdit->text().toStdString());
    CoreSettingsSetValue(SettingsID::Core_64DD_SaveDiskFormat, this->diskSaveTypeComboBox->currentIndex());
}

void SettingsDialog::saveHotkeySettings(void)
{
    this->commonHotkeySettings(SettingsDialogAction::SaveSettings);
}

void SettingsDialog::saveInterfaceEmulationSettings(void)
{
    CoreSettingsSetValue(SettingsID::GUI_AllowManualResizing, this->manualResizingCheckBox->isChecked());
    CoreSettingsSetValue(SettingsID::GUI_HideCursorInEmulation, this->hideCursorCheckBox->isChecked());
    CoreSettingsSetValue(SettingsID::GUI_HideCursorInFullscreenEmulation, this->hideCursorFullscreenCheckBox->isChecked());
    CoreSettingsSetValue(SettingsID::GUI_PauseEmulationOnFocusLoss, this->pauseEmulationOnFocusCheckbox->isChecked());
    CoreSettingsSetValue(SettingsID::GUI_ResumeEmulationOnFocus, this->resumeEmulationOnFocusCheckBox->isChecked());
    CoreSettingsSetValue(SettingsID::GUI_AutomaticFullscreen, this->automaticFullscreenCheckbox->isChecked());
    CoreSettingsSetValue(SettingsID::GUI_StatusbarMessageDuration, this->statusBarMessageDurationSpinBox->value());
}

void SettingsDialog::saveInterfaceRomBrowserSettings(void)
{
    CoreSettingsSetValue(SettingsID::RomBrowser_Recursive, this->searchSubDirectoriesCheckbox->isChecked());
    CoreSettingsSetValue(SettingsID::RomBrowser_MaxItems, this->romSearchLimitSpinBox->value());
}

void SettingsDialog::saveInterfaceLogWindowSettings(void)
{
    CoreSettingsSetValue(SettingsID::GUI_ShowVerboseLogMessages, this->showVerboseLogMessagesCheckBox->isChecked());
}

void SettingsDialog::saveInterfaceOSDSettings(void)
{
    CoreSettingsSetValue(SettingsID::GUI_OnScreenDisplayEnabled, this->osdEnabledCheckBox->isChecked());
    CoreSettingsSetValue(SettingsID::GUI_OnScreenDisplayLocation, this->osdLocationComboBox->currentIndex());
    CoreSettingsSetValue(SettingsID::GUI_OnScreenDisplayPaddingY, this->osdVerticalPaddingSpinBox->value());
    CoreSettingsSetValue(SettingsID::GUI_OnScreenDisplayPaddingX, this->osdHorizontalPaddingSpinBox->value());
    CoreSettingsSetValue(SettingsID::GUI_OnScreenDisplayOpacity, (float)this->osdOpacitySpinBox->value());
    CoreSettingsSetValue(SettingsID::GUI_OnScreenDisplayDuration, this->osdDurationSpinBox->value());
}

void SettingsDialog::saveInterfaceStyleSettings(void)
{
#ifdef _WIN32
    CoreSettingsSetValue(SettingsID::GUI_Style, this->styleComboBox->currentData().toString().toStdString());
    CoreSettingsSetValue(SettingsID::GUI_IconTheme, this->iconThemeComboBox->currentText().toStdString());
#endif // _WIN32
}

void SettingsDialog::saveInterfaceMiscSettings(void)
{
#ifdef UPDATER
    CoreSettingsSetValue(SettingsID::GUI_CheckForUpdates, this->checkForUpdatesCheckBox->isChecked());
#endif // UPDATER
#ifdef DISCORD_RPC
    CoreSettingsSetValue(SettingsID::GUI_DiscordRpc, this->discordRpcCheckBox->isChecked());
#endif // DISCORD_RPC
}

void SettingsDialog::commonHotkeySettings(SettingsDialogAction action)
{
    struct keybinding
    {
        KeybindButton* button;
        SettingsID settingId;
    };

    std::vector<keybinding> keybindings;

    std::vector<keybinding> keybindings_System =
    {
        { this->startRomKeyButton, SettingsID::KeyBinding_StartROM },
        { this->startComboKeyButton, SettingsID::KeyBinding_StartCombo },
        { this->shutdownKeyButton, SettingsID::KeyBinding_Shutdown },
        { this->refreshRomListKeyButton, SettingsID::KeyBinding_RefreshROMList },
        { this->exitKeyButton, SettingsID::KeyBinding_Exit },
        { this->softResetKeyButton, SettingsID::KeyBinding_SoftReset },
        { this->hardResetKeyButton, SettingsID::KeyBinding_HardReset },
        { this->pauseKeyButton, SettingsID::KeyBinding_Resume },
        { this->generateBitmapKeyButton, SettingsID::KeyBinding_Screenshot },
        { this->limitFPSKeyButton, SettingsID::KeyBinding_LimitFPS },
        { this->saveStateKeyButton, SettingsID::KeyBinding_SaveState },
        { this->saveAsKeyButton, SettingsID::KeyBinding_SaveAs },
        { this->loadStateKeyButton, SettingsID::KeyBinding_LoadState },
        { this->loadKeyButton, SettingsID::KeyBinding_Load },
        { this->cheatsKeyButton, SettingsID::KeyBinding_Cheats },
        { this->gsButtonKeyButton, SettingsID::KeyBinding_GSButton },
    };

    std::vector<keybinding> keybindings_CurrentSaveState =
    {
        { this->saveState0KeyButton, SettingsID::KeyBinding_SaveStateSlot0 },
        { this->saveState1KeyButton, SettingsID::KeyBinding_SaveStateSlot1 },
        { this->saveState2KeyButton, SettingsID::KeyBinding_SaveStateSlot2 },
        { this->saveState3KeyButton, SettingsID::KeyBinding_SaveStateSlot3 },
        { this->saveState4KeyButton, SettingsID::KeyBinding_SaveStateSlot4 },
        { this->saveState5KeyButton, SettingsID::KeyBinding_SaveStateSlot5 },
        { this->saveState6KeyButton, SettingsID::KeyBinding_SaveStateSlot6 },
        { this->saveState7KeyButton, SettingsID::KeyBinding_SaveStateSlot7 },
        { this->saveState8KeyButton, SettingsID::KeyBinding_SaveStateSlot8 },
        { this->saveState9KeyButton, SettingsID::KeyBinding_SaveStateSlot9 },
    };

    std::vector<keybinding> keybindings_Settings =
    {
        { this->settingsKeyButton, SettingsID::KeyBinding_Settings },
    };

    std::vector<keybinding> keybindings_View =
    {
        { this->fullscreenKeyButton, SettingsID::KeyBinding_Fullscreen },
    };


    if (action == SettingsDialogAction::LoadDefaultSettings)
    {
        int index = this->keybindingsMenuTabWidget->currentIndex();
        switch (index)
        {
        default:
        case 0:
            keybindings.insert(keybindings.end(), keybindings_System.begin(), keybindings_System.end());
            break;
        case 1:
            keybindings.insert(keybindings.end(), keybindings_CurrentSaveState.begin(), keybindings_CurrentSaveState.end());
            break;
        case 2:
            keybindings.insert(keybindings.end(), keybindings_Settings.begin(), keybindings_Settings.end());
            break;
        case 3:
            keybindings.insert(keybindings.end(), keybindings_View.begin(), keybindings_View.end());
            break;
        }
    }
    else
    {
        keybindings.insert(keybindings.end(), keybindings_System.begin(), keybindings_System.end());
        keybindings.insert(keybindings.end(), keybindings_CurrentSaveState.begin(), keybindings_CurrentSaveState.end());
        keybindings.insert(keybindings.end(), keybindings_Settings.begin(), keybindings_Settings.end());
        keybindings.insert(keybindings.end(), keybindings_View.begin(), keybindings_View.end());
    }

    switch (action)
    {
        default:
            break;
        case SettingsDialogAction::LoadSettings:
            this->removeDuplicateHotkeysCheckBox->setChecked(CoreSettingsGetBoolValue(SettingsID::KeyBinding_RemoveDuplicates));
            break;
        case SettingsDialogAction::LoadDefaultSettings:
            this->removeDuplicateHotkeysCheckBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::KeyBinding_RemoveDuplicates));
            break;
        case SettingsDialogAction::SaveSettings:
            CoreSettingsSetValue(SettingsID::KeyBinding_RemoveDuplicates, this->removeDuplicateHotkeysCheckBox->isChecked());
            break;
    }

    for (const auto& keybinding : keybindings)
    {
        switch (action)
        {
        default:
        case SettingsDialogAction::ConnectSignals:
            connect(keybinding.button, &KeybindButton::on_KeybindButton_KeybindingChanged, this, &SettingsDialog::on_KeybindButton_KeybindingChanged);
        case SettingsDialogAction::LoadSettings:
            keybinding.button->setText(QString::fromStdString(CoreSettingsGetStringValue(keybinding.settingId)));
            break;
        case SettingsDialogAction::LoadDefaultSettings:
            keybinding.button->setText(QString::fromStdString(CoreSettingsGetDefaultStringValue(keybinding.settingId)));
            break;
        case SettingsDialogAction::SaveSettings:
            CoreSettingsSetValue(keybinding.settingId, keybinding.button->text().toStdString());
            break;
        }
    }
}

void SettingsDialog::commonPluginSettings(SettingsDialogAction action)
{
    QComboBox *comboBoxArray[] = {this->rspPluginsComboBox, this->videoPluginsComboBox, 
                                    this->audioPluginsComboBox, this->inputPluginsComboBox};
    SettingsID settingsIdArray[] = {SettingsID::Core_RSP_Plugin, SettingsID::Core_GFX_Plugin, 
                                    SettingsID::Core_AUDIO_Plugin, SettingsID::Core_INPUT_Plugin};
    bool pluginFound[] = {false, false, false, false};

    QComboBox *comboBox;
    std::string pluginFileName;
    int index = 0;

    // clear combobox items
    for (const auto& c : comboBoxArray)
    {
        c->clear();
    }

    for (const auto &p : this->pluginList)
    {
        index = ((int)p.Type - 1);
        comboBox = comboBoxArray[index];

        pluginFileName = action == SettingsDialogAction::LoadSettings ? 
                            CoreSettingsGetStringValue(settingsIdArray[index]) :
                            CoreSettingsGetDefaultStringValue(settingsIdArray[index]);

        comboBox->addItem(QString::fromStdString(p.Name), QString::fromStdString(p.File));

        if (pluginFileName == p.File)
        {
            comboBox->setCurrentText(QString::fromStdString(p.Name));
            pluginFound[index] = true;
        }
    }

    for (int i = 0; i < 4; i++)
    {
        comboBox = comboBoxArray[i];
        if (!pluginFound[i])
        {
            comboBox->addItem("", "");
            comboBox->setCurrentText("");
        }
    }
}

void SettingsDialog::commonInterfaceStyleSettings(SettingsDialogAction action)
{
#ifdef _WIN32
    this->styleComboBox->clear();
    this->styleComboBox->addItem("None", "");

    QString currentStyle = action == SettingsDialogAction::LoadSettings ?
        QString::fromStdString(CoreSettingsGetStringValue(SettingsID::GUI_Style)) :
        QString::fromStdString(CoreSettingsGetDefaultStringValue(SettingsID::GUI_Style));

    QString directory;
    directory = QString::fromStdString(CoreGetSharedDataDirectory().string());
    directory += "\\Styles\\";

    QStringList filter;
    filter << "*.qss";

    bool styleFound = false;
    QDirIterator stylesDirectoryIter(directory, filter, QDir::Files, QDirIterator::NoIteratorFlags);
    while (stylesDirectoryIter.hasNext())
    {
        QString filePath = stylesDirectoryIter.next();
        QFileInfo fileInfo(filePath);

        this->styleComboBox->addItem(fileInfo.baseName(), filePath);
        if (filePath == currentStyle)
        {
            styleFound = true;
            this->styleComboBox->setCurrentText(fileInfo.baseName());
        }
    }

    if (currentStyle.isEmpty())
    {
        this->styleComboBox->setCurrentText("None");
        styleFound = true;
    }

    if (!styleFound)
    {
        this->styleComboBox->addItem("", "");
        this->styleComboBox->setCurrentText("");
    }

    QString currentIconTheme = action == SettingsDialogAction::LoadSettings ?
                QString::fromStdString(CoreSettingsGetStringValue(SettingsID::GUI_IconTheme)) :
                QString::fromStdString(CoreSettingsGetDefaultStringValue(SettingsID::GUI_IconTheme));
    this->iconThemeComboBox->setCurrentText(currentIconTheme);
#endif // _WIN32
}

void SettingsDialog::setIconsForEmulationInfoText(void)
{
    QLabel* labels[] = {
        this->infoIconLabel_0, this->infoIconLabel_1, this->infoIconLabel_2,
        this->infoIconLabel_3, this->infoIconLabel_4, this->infoIconLabel_5,
        this->infoIconLabel_6, this->infoIconLabel_7
    };

    QIcon infoIcon = QIcon::fromTheme("information-line");
    QPixmap infoIconPixmap = infoIcon.pixmap(16, 16);

    for (QLabel* label : labels)
    {
        label->setPixmap(infoIconPixmap);
    }
}

void SettingsDialog::hideEmulationInfoText(void)
{
    QHBoxLayout *layouts[] = {this->emulationInfoLayout_0, this->emulationInfoLayout_1, 
                                this->emulationInfoLayout_2, this->emulationInfoLayout_3,
                                this->emulationInfoLayout_9};

    for (const auto &layout : layouts)
    {
        for (int i = 0; i < layout->count(); i++)
        {
            QWidget *widget = layout->itemAt(i)->widget();
            widget->hide();
        }
    }
}

void SettingsDialog::chooseDirectory(QLineEdit *lineEdit)
{
    QString dir;

    dir = QFileDialog::getExistingDirectory(this);
    if (dir.isEmpty())
    {
        return;
    }

    lineEdit->setText(dir);
}

void SettingsDialog::chooseIPLRom(QLineEdit *lineEdit)
{
    QString file;

    file = QFileDialog::getOpenFileName(this, "", "", "IPL ROMs (*.n64 *.v64 *.z64)");
    if (file.isEmpty())
    {
        return;
    }

    lineEdit->setText(file);
}

bool SettingsDialog::applyPluginSettings(void)
{
    // attempt to apply plugin settings when emulation
    // isn't running, when it fails, show the user the error and
    // don't allow the user to save invalid settings
    if (!CoreIsEmulationPaused() && !CoreIsEmulationRunning())
    {
        if (!CoreApplyPluginSettings())
        {
            QMessageBox msgBox(this);
            msgBox.setIcon(QMessageBox::Icon::Critical);
            msgBox.setWindowTitle("Error");
            msgBox.setText("CoreApplyPluginSettings() Failed");
            msgBox.setDetailedText(QString::fromStdString(CoreGetError()));
            msgBox.addButton(QMessageBox::Ok);
            msgBox.exec();
            return false;
        }
    }
    return true;
}

void SettingsDialog::closeEvent(QCloseEvent* event)
{
    if (this->applyPluginSettings())
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void SettingsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    QPushButton *pushButton = (QPushButton *)button;
    QPushButton *defaultButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);
    QPushButton *cancelButton = this->buttonBox->button(QDialogButtonBox::Cancel);
    QPushButton *okButton = this->buttonBox->button(QDialogButtonBox::Ok);


    if (pushButton == cancelButton || pushButton == okButton)
    {
        if (pushButton == okButton)
        {
            this->saveSettings();

            // re-load OSD settings
            OnScreenDisplayLoadSettings();
        }

        if (!this->applyPluginSettings())
        {
            return;
        }
    }

    if (pushButton == defaultButton)
    {
        this->restoreDefaults(this->currentIndex());
    }
    else if (pushButton == cancelButton)
    {
        this->reject();
    }
    else if (pushButton == okButton)
    {
        this->accept();
    }
}

void SettingsDialog::on_changeScreenShotDirButton_clicked(void)
{
    this->chooseDirectory(this->screenshotDirLineEdit);
}

void SettingsDialog::on_changeSaveStateDirButton_clicked(void)
{
    this->chooseDirectory(this->saveStateDirLineEdit);
}

void SettingsDialog::on_changeSaveSramDirButton_clicked(void)
{
    this->chooseDirectory(this->saveSramDirLineEdit);
}

void SettingsDialog::on_changeUserDataDirButton_clicked(void)
{
    this->chooseDirectory(this->userDataDirLineEdit);
}

void SettingsDialog::on_changeUserCacheDirButton_clicked(void)
{
    this->chooseDirectory(this->userCacheDirLineEdit);
}

void SettingsDialog::on_changeJapaneseIPLRomPathButton_clicked(void)
{
    this->chooseIPLRom(this->japaneseIPLRomLineEdit);
}

void SettingsDialog::on_changeAmericanIPLRomPathButton_clicked(void)
{
    this->chooseIPLRom(this->americanIPLRomLineEdit);
}

void SettingsDialog::on_changeDevelopmentIPLRomPathButton_clicked(void)
{
    this->chooseIPLRom(this->developmentIPLRomLineEdit);
}

void SettingsDialog::on_KeybindButton_KeybindingChanged(KeybindButton* button)
{
    if (!this->removeDuplicateHotkeysCheckBox->isChecked())
    {
        return;
    }

    QString text = button->text();
    if (text.isEmpty())
    {
        return;
    }

    KeybindButton* keybindButtons[] = 
    {
        this->startRomKeyButton,
        this->startComboKeyButton,
        this->shutdownKeyButton,
        this->refreshRomListKeyButton,
        this->exitKeyButton,
        this->softResetKeyButton,
        this->hardResetKeyButton,
        this->pauseKeyButton, 
        this->generateBitmapKeyButton,
        this->limitFPSKeyButton,
        this->saveStateKeyButton,
        this->saveAsKeyButton, 
        this->loadStateKeyButton,
        this->loadKeyButton,
        this->cheatsKeyButton, 
        this->gsButtonKeyButton,
        this->saveState0KeyButton,
        this->saveState1KeyButton,
        this->saveState2KeyButton,
        this->saveState3KeyButton,
        this->saveState4KeyButton,
        this->saveState5KeyButton,
        this->saveState6KeyButton,
        this->saveState7KeyButton,
        this->saveState8KeyButton,
        this->saveState9KeyButton,
        this->settingsKeyButton,
        this->fullscreenKeyButton,
    };
    
    for (KeybindButton* keybindButton : keybindButtons)
    {
        if (keybindButton != button)
        {
            if (keybindButton->text() == text)
            {
                keybindButton->Clear();
            }
        }
    }
}
