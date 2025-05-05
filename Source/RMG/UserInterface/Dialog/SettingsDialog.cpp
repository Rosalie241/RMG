/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "UserInterface/Dialog/Netplay/NetplayCommon.hpp"
#include "UserInterface/Widget/KeybindButton.hpp"
#include "Utilities/QtMessageBox.hpp"
#include "OnScreenDisplay.hpp"
#include "SettingsDialog.hpp"

#include <QRegularExpressionValidator>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QFileDialog>
#include <QColorDialog>
#include <QDirIterator>
#include <QLabel>

#include <RMG-Core/CachedRomHeaderAndSettings.hpp>
#include <RMG-Core/Directories.hpp>
#include <RMG-Core/Emulation.hpp>
#include <RMG-Core/Settings.hpp>
#include <RMG-Core/Error.hpp>
#include <RMG-Core/Rom.hpp>

using namespace UserInterface::Dialog;
using namespace Utilities;

//
// Local Enums
//

enum class SettingsDialogTab
{
    InterfaceGeneral    = 0,
    InterfaceEmulation  = 1,
    InterfaceRomBrowser = 2,
    InterfaceLog = 3,
    InterfaceOSD = 4,
    InterfaceNetplay = 5,
    Hotkey     = 6,
    Core       = 7,
    Game       = 8,
    GameCore   = 9,
    GamePlugin = 10,
    Plugin     = 11,
    Directory  = 12,
    N64DD      = 13,
    Invalid    = 14
};


//
// Exported Functions
//

SettingsDialog::SettingsDialog(QWidget *parent, QString file) : QDialog(parent)
{
    this->setupUi(this);

    this->setIconsForEmulationInfoText();

    // if ROM is open, we should retrieve the current settings,
    // if it's not opened but we got a filename, try to 
    // retrieve cached entries
    if (CoreHasRomOpen())
    {
        std::filesystem::path romPath;
        // only show game tab once retrieving info succeeds
        this->showGameSettings = CoreGetRomPath(romPath) && 
                                    CoreGetRomType(this->currentGameType) &&
                                    CoreGetCurrentRomHeader(this->currentGameHeader) &&
                                    CoreGetCurrentRomSettings(this->currentGameSettings) && 
                                    CoreGetCurrentDefaultRomSettings(this->defaultGameSettings);
        this->currentGameFile = QString::fromStdU32String(romPath.u32string());
    }
    else if (!file.isEmpty())
    {
        // only show game tab once retrieving cached entry succeeds
        this->showGameSettings = CoreGetCachedRomHeaderAndSettings(file.toStdU32String(), &this->currentGameType, 
                                    &this->currentGameHeader, &this->defaultGameSettings, &this->currentGameSettings);
        if (this->showGameSettings)
        {
            this->currentGameFile = file;
        }
    }

    if (this->showGameSettings)
    {
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
        this->tabWidget->setTabEnabled(3, false);
    }

    pluginList = CoreGetAllPlugins();

    for (int i = 0; i < (int)SettingsDialogTab::Invalid; i++)
    {
        this->loadSettings(i);
    }

    // connect hotkey settings to slot
    this->commonHotkeySettings(SettingsDialogAction::ConnectSignals);

#ifndef NETPLAY
    this->innerInterfaceTabWidget->setTabVisible((int)SettingsDialogTab::InterfaceNetplay, false);
#endif // !NETPLAY

#ifndef UPDATER
    this->checkForUpdatesCheckBox->setHidden(true);
#endif // !UPDATER

#ifndef DISCORD_RPC
    this->discordRpcCheckBox->setHidden(true);
#endif // !DISCORD_RPC
}

SettingsDialog::~SettingsDialog(void)
{
}

void SettingsDialog::ShowGameTab(void)
{
    this->tabWidget->setCurrentIndex(3);
}

int SettingsDialog::currentIndex(void)
{
    int currentIndex = this->tabWidget->currentIndex();
    int index = currentIndex;

    if (currentIndex == 0)
    { // interface tab
        index += this->innerInterfaceTabWidget->currentIndex();
    }
    else if (currentIndex > 0)
    { // above interface tab
        index += this->innerInterfaceTabWidget->count() - 1;
    }

    if (currentIndex == 3)
    { // game tab
        index += this->innerGameTabWidget->currentIndex();
    }
    else if (currentIndex > 3)
    { // above game tab
        index += this->innerGameTabWidget->count() - 1;
    }

    return index;
}

void SettingsDialog::restoreDefaults(int stackedWidgetIndex)
{
    if (stackedWidgetIndex < 0 || 
        stackedWidgetIndex > (int)SettingsDialogTab::Invalid)
    {
        return;
    }

    switch ((SettingsDialogTab)stackedWidgetIndex)
    {
    default:
        break;
    case SettingsDialogTab::InterfaceGeneral:
        this->loadDefaultInterfaceGeneralSettings();
        break;
    case SettingsDialogTab::InterfaceEmulation:
        this->loadDefaultInterfaceEmulationSettings();
        break;
    case SettingsDialogTab::InterfaceRomBrowser:
        this->loadDefaultInterfaceRomBrowserSettings();
        break;
    case SettingsDialogTab::InterfaceLog:
        this->loadDefaultInterfaceLogSettings();
        break;
    case SettingsDialogTab::InterfaceOSD:
        this->loadDefaultInterfaceOSDSettings();
        break;
    case SettingsDialogTab::InterfaceNetplay:
        this->loadDefaultInterfaceNetplaySettings();
        break;
    case SettingsDialogTab::Hotkey:
        this->loadDefaultHotkeySettings();
        break;
    case SettingsDialogTab::Core:
        this->loadDefaultCoreSettings();
        break;
    case SettingsDialogTab::Game:
        this->loadDefaultGameSettings();
        break;
    case SettingsDialogTab::GameCore:
        this->loadDefaultGameCoreSettings();
        break;
    case SettingsDialogTab::GamePlugin:
        this->loadDefaultGamePluginSettings();
        break;
    case SettingsDialogTab::Plugin:
        this->loadDefaultPluginSettings();
        break;
    case SettingsDialogTab::Directory:
        this->loadDefaultDirectorySettings();
        break;
    case SettingsDialogTab::N64DD:
        this->loadDefault64DDSettings();
        break;
    }
}

void SettingsDialog::loadSettings(int stackedWidgetIndex)
{
    if (stackedWidgetIndex < 0 || 
        stackedWidgetIndex > (int)SettingsDialogTab::Invalid)
    {
        return;
    }

    switch ((SettingsDialogTab)stackedWidgetIndex)
    {
    default:
        break;
    case SettingsDialogTab::InterfaceGeneral:
        this->loadInterfaceGeneralSettings();
        break;
    case SettingsDialogTab::InterfaceEmulation:
        this->loadInterfaceEmulationSettings();
        break;
    case SettingsDialogTab::InterfaceRomBrowser:
        this->loadInterfaceRomBrowserSettings();
        break;
    case SettingsDialogTab::InterfaceLog:
        this->loadInterfaceLogSettings();
        break;
    case SettingsDialogTab::InterfaceOSD:
        this->loadInterfaceOSDSettings();
        break;
    case SettingsDialogTab::InterfaceNetplay:
        this->loadInterfaceNetplaySettings();
        break;
    case SettingsDialogTab::Hotkey:
        this->loadHotkeySettings();
        break;
    case SettingsDialogTab::Core:
        this->loadCoreSettings();
        break;
    case SettingsDialogTab::Game:
        this->loadGameSettings();
        break;
    case SettingsDialogTab::GameCore:
        this->loadGameCoreSettings();
        break;
    case SettingsDialogTab::GamePlugin:
        this->loadGamePluginSettings();
        break;
    case SettingsDialogTab::Plugin:
        this->loadPluginSettings();
        break;
    case SettingsDialogTab::Directory:
        this->loadDirectorySettings();
        break;
    case SettingsDialogTab::N64DD:
        this->load64DDSettings();
        break;
    }
}

void SettingsDialog::loadCoreSettings(void)
{
    bool disableExtraMem = false;
    int counterFactor = 0;
    int cpuEmulator = 0;
    int saveFilenameFormat = 0;
    int siDmaDuration = -1;
    bool randomizeInterrupt = true;
    bool usePIFROM = false;
    QString ntscPifROM;
    QString palPifRom;
    bool overrideGameSettings = false;

    disableExtraMem = CoreSettingsGetBoolValue(SettingsID::CoreOverlay_DisableExtraMem);
    counterFactor = CoreSettingsGetIntValue(SettingsID::CoreOverlay_CountPerOp);
    cpuEmulator = CoreSettingsGetIntValue(SettingsID::CoreOverlay_CPU_Emulator);
    saveFilenameFormat = CoreSettingsGetIntValue(SettingsID::CoreOverLay_SaveFileNameFormat);
    siDmaDuration = CoreSettingsGetIntValue(SettingsID::CoreOverlay_SiDmaDuration);
    randomizeInterrupt = CoreSettingsGetBoolValue(SettingsID::CoreOverlay_RandomizeInterrupt);
    usePIFROM = CoreSettingsGetBoolValue(SettingsID::Core_PIF_Use);
    ntscPifROM = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::Core_PIF_NTSC));
    palPifRom = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::Core_PIF_PAL));
    overrideGameSettings = CoreSettingsGetBoolValue(SettingsID::Core_OverrideGameSpecificSettings);

    this->coreCpuEmulatorComboBox->setCurrentIndex(cpuEmulator);
    this->coreSaveFilenameFormatComboBox->setCurrentIndex(saveFilenameFormat);
    this->coreRandomizeTimingCheckBox->setChecked(randomizeInterrupt);

    this->usePifRomGroupBox->setChecked(usePIFROM);
    this->ntscPifRomLineEdit->setText(ntscPifROM);
    this->palPifRomLineEdit->setText(palPifRom);

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
    this->gameTransferPakComboBox->setCurrentIndex(this->currentGameSettings.TransferPak);
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
    gameOverclockingFactorSpinBox->setValue(overclockingFactor);
    gameRandomizeTimingCheckBox->setChecked(randomizeInterrupt);
}

void SettingsDialog::loadGamePluginSettings(void)
{
    QComboBox *comboBoxArray[] = {this->gameRspPluginsComboBox, this->gameVideoPluginsComboBox,
                                   this->gameAudioPluginsComboBox, this->gameInputPluginsComboBox};
    SettingsID settingsId[] = {SettingsID::Game_RSP_Plugin, SettingsID::Game_GFX_Plugin, 
                                    SettingsID::Game_AUDIO_Plugin, SettingsID::Game_INPUT_Plugin};
    bool pluginFound[] = {false, false, false, false};
    QString pluginFileNames[4];
    QString pluginFileName;
    QString pluginName;

    QComboBox *comboBox;
    int index = 0;

    for (int i = 0; i < 4; i++)
    {
        pluginFileName = QString::fromStdString(CoreSettingsGetStringValue(settingsId[i], this->gameSection));

        if (!pluginFileName.isEmpty())
        {
            // account for full path (<v0.3.5 we used the full path)
            pluginFileName = QFileInfo(pluginFileName).fileName();
            pluginFileNames[i] = pluginFileName;
        }
    }

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

        if (pluginFileNames[index] == QString::fromStdString(p.File))
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
            pluginName = pluginFileNames[i] + " (not found)";

            comboBox->addItem(pluginName, pluginFileNames[i]);
            comboBox->setCurrentText(pluginName);
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

void SettingsDialog::loadInterfaceGeneralSettings(void)
{
    // find stylesheets and add them to the UI
    QString directory;
    directory = QString::fromStdString(CoreGetSharedDataDirectory().string());
    directory += CORE_DIR_SEPERATOR_STR;
    directory += "Styles";
    directory += CORE_DIR_SEPERATOR_STR;

    QStringList filter;
    filter << "*.qss";

    QDirIterator stylesDirectoryIter(directory, filter, QDir::Files, QDirIterator::NoIteratorFlags);
    while (stylesDirectoryIter.hasNext())
    {
        QFileInfo fileInfo(stylesDirectoryIter.next());
        this->themeComboBox->addItem(fileInfo.fileName());
    }

    // select currently chosen theme in UI
    this->themeComboBox->setCurrentText(QString::fromStdString(CoreSettingsGetStringValue(SettingsID::GUI_Theme)));
    this->iconThemeComboBox->setCurrentText(QString::fromStdString(CoreSettingsGetStringValue(SettingsID::GUI_IconTheme)));
#ifdef UPDATER
    this->checkForUpdatesCheckBox->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_CheckForUpdates));
#endif // UPDATER
#ifdef DISCORD_RPC
    this->discordRpcCheckBox->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_DiscordRpc));
#endif // DISCORD_RPC
}

void SettingsDialog::loadInterfaceEmulationSettings(void)
{
    this->pauseEmulationOnFocusCheckbox->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_PauseEmulationOnFocusLoss));
    this->resumeEmulationOnFocusCheckBox->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_ResumeEmulationOnFocus));
    this->hideCursorCheckBox->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_HideCursorInEmulation));
    this->hideCursorFullscreenCheckBox->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_HideCursorInFullscreenEmulation));
    this->automaticFullscreenCheckbox->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_AutomaticFullscreen));
    this->confirmDragDropCheckBox->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_ConfirmDragDrop));
    this->statusBarMessageDurationSpinBox->setValue(CoreSettingsGetIntValue(SettingsID::GUI_StatusbarMessageDuration));
    this->openglTypeComboBox->setCurrentIndex(CoreSettingsGetBoolValue(SettingsID::GUI_OpenGLES));
}

void SettingsDialog::loadInterfaceRomBrowserSettings(void)
{
    this->searchSubDirectoriesCheckbox->setChecked(CoreSettingsGetBoolValue(SettingsID::RomBrowser_Recursive));
    this->romSearchLimitSpinBox->setValue(CoreSettingsGetIntValue(SettingsID::RomBrowser_MaxItems));
}

void SettingsDialog::loadInterfaceLogSettings(void)
{
    this->showVerboseLogMessagesCheckBox->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_ShowVerboseLogMessages));
}

void SettingsDialog::loadInterfaceOSDSettings(void)
{
    this->osdEnabledCheckBox->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_OnScreenDisplayEnabled));
    this->osdLocationComboBox->setCurrentIndex(CoreSettingsGetIntValue(SettingsID::GUI_OnScreenDisplayLocation));
    this->osdVerticalPaddingSpinBox->setValue(CoreSettingsGetIntValue(SettingsID::GUI_OnScreenDisplayPaddingY));
    this->osdHorizontalPaddingSpinBox->setValue(CoreSettingsGetIntValue(SettingsID::GUI_OnScreenDisplayPaddingX));
    this->osdDurationSpinBox->setValue(CoreSettingsGetIntValue(SettingsID::GUI_OnScreenDisplayDuration));

    std::vector<int> backgroundColor = CoreSettingsGetIntListValue(SettingsID::GUI_OnScreenDisplayBackgroundColor);
    std::vector<int> textColor = CoreSettingsGetIntListValue(SettingsID::GUI_OnScreenDisplayTextColor);
    if (backgroundColor.size() == 4)
    {
        this->currentBackgroundColor = QColor(backgroundColor.at(0), backgroundColor.at(1), backgroundColor.at(2), backgroundColor.at(3));
        this->chooseColor(this->changeBackgroundColorButton, &this->currentBackgroundColor, true);
    }
    if (textColor.size() == 4)
    {
        this->currentTextColor = QColor(textColor.at(0), textColor.at(1), textColor.at(2), textColor.at(3));
        this->chooseColor(this->changeTextColorButton, &this->currentTextColor, true);
    }
}

void SettingsDialog::loadInterfaceNetplaySettings(void)
{
    // TODO: maybe add initialize functions for each tab?
    static bool initialized = false;
    if (!initialized)
    { // set regexp for nickname
        QRegularExpression re(NETPLAYCOMMON_NICKNAME_REGEX);
        this->netplayNicknameLineEdit->setValidator(new QRegularExpressionValidator(re, this));
        initialized = true;
    }

    this->netplayNicknameLineEdit->setText(QString::fromStdString(CoreSettingsGetStringValue(SettingsID::Netplay_Nickname)));
    this->netplayServerUrlLineEdit->setText(QString::fromStdString(CoreSettingsGetStringValue(SettingsID::Netplay_ServerJsonUrl)));
}

void SettingsDialog::loadDefaultCoreSettings(void)
{
    bool disableExtraMem = false;
    int counterFactor = 0;
    int cpuEmulator = 0;
    int saveFilenameFormat = 0;
    int siDmaDuration = -1;
    bool randomizeInterrupt = true;
    bool usePIFROM;
    QString ntscPifROM;
    QString palPifRom;
    bool overrideGameSettings;

    disableExtraMem = CoreSettingsGetDefaultBoolValue(SettingsID::CoreOverlay_DisableExtraMem);
    counterFactor = CoreSettingsGetDefaultIntValue(SettingsID::CoreOverlay_CountPerOp);
    cpuEmulator = CoreSettingsGetDefaultIntValue(SettingsID::CoreOverlay_CPU_Emulator);
    siDmaDuration = CoreSettingsGetDefaultIntValue(SettingsID::CoreOverlay_SiDmaDuration);
    saveFilenameFormat = CoreSettingsGetDefaultIntValue(SettingsID::CoreOverLay_SaveFileNameFormat);
    randomizeInterrupt = CoreSettingsGetDefaultBoolValue(SettingsID::CoreOverlay_RandomizeInterrupt);
    usePIFROM = CoreSettingsGetDefaultBoolValue(SettingsID::Core_PIF_Use);
    ntscPifROM = QString::fromStdString(CoreSettingsGetDefaultStringValue(SettingsID::Core_PIF_NTSC));
    palPifRom = QString::fromStdString(CoreSettingsGetDefaultStringValue(SettingsID::Core_PIF_PAL));
    overrideGameSettings = CoreSettingsGetDefaultBoolValue(SettingsID::Core_OverrideGameSpecificSettings);

    this->coreCpuEmulatorComboBox->setCurrentIndex(cpuEmulator);
    this->coreSaveFilenameFormatComboBox->setCurrentIndex(saveFilenameFormat);
    this->coreRandomizeTimingCheckBox->setChecked(randomizeInterrupt);

    this->usePifRomGroupBox->setChecked(usePIFROM);
    this->ntscPifRomLineEdit->setText(ntscPifROM);
    this->palPifRomLineEdit->setText(palPifRom);

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
    this->gameTransferPakComboBox->setCurrentIndex(this->defaultGameSettings.TransferPak);
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
    gameOverclockingFactorSpinBox->setValue(overclockingFactor);
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

void SettingsDialog::loadDefaultInterfaceGeneralSettings(void)
{
    this->themeComboBox->setCurrentText(QString::fromStdString(CoreSettingsGetDefaultStringValue(SettingsID::GUI_Theme)));
    this->iconThemeComboBox->setCurrentText(QString::fromStdString(CoreSettingsGetDefaultStringValue(SettingsID::GUI_IconTheme)));
#ifdef UPDATER
    this->checkForUpdatesCheckBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::GUI_CheckForUpdates));
#endif // UPDATER
#ifdef DISCORD_RPC
    this->discordRpcCheckBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::GUI_DiscordRpc));
#endif // DISCORD_RPC
}

void SettingsDialog::loadDefaultInterfaceEmulationSettings(void)
{
    this->pauseEmulationOnFocusCheckbox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::GUI_PauseEmulationOnFocusLoss));
    this->resumeEmulationOnFocusCheckBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::GUI_ResumeEmulationOnFocus));
    this->hideCursorCheckBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::GUI_HideCursorInEmulation));
    this->hideCursorFullscreenCheckBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::GUI_HideCursorInFullscreenEmulation));
    this->automaticFullscreenCheckbox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::GUI_AutomaticFullscreen));
    this->confirmDragDropCheckBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::GUI_ConfirmDragDrop));
    this->statusBarMessageDurationSpinBox->setValue(CoreSettingsGetDefaultIntValue(SettingsID::GUI_StatusbarMessageDuration));
    this->openglTypeComboBox->setCurrentIndex(CoreSettingsGetDefaultBoolValue(SettingsID::GUI_OpenGLES));
}

void SettingsDialog::loadDefaultInterfaceRomBrowserSettings(void)
{
    this->searchSubDirectoriesCheckbox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::RomBrowser_Recursive));
    this->romSearchLimitSpinBox->setValue(CoreSettingsGetDefaultIntValue(SettingsID::RomBrowser_MaxItems));
}

void SettingsDialog::loadDefaultInterfaceLogSettings(void)
{
    this->showVerboseLogMessagesCheckBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::GUI_ShowVerboseLogMessages));
}

void SettingsDialog::loadDefaultInterfaceOSDSettings(void)
{
    this->osdEnabledCheckBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::GUI_OnScreenDisplayEnabled));
    this->osdLocationComboBox->setCurrentIndex(CoreSettingsGetDefaultIntValue(SettingsID::GUI_OnScreenDisplayLocation));
    this->osdVerticalPaddingSpinBox->setValue(CoreSettingsGetDefaultIntValue(SettingsID::GUI_OnScreenDisplayPaddingY));
    this->osdHorizontalPaddingSpinBox->setValue(CoreSettingsGetDefaultIntValue(SettingsID::GUI_OnScreenDisplayPaddingX));
    this->osdDurationSpinBox->setValue(CoreSettingsGetDefaultIntValue(SettingsID::GUI_OnScreenDisplayDuration));

    std::vector<int> backgroundColor = CoreSettingsGetDefaultIntListValue(SettingsID::GUI_OnScreenDisplayBackgroundColor);
    std::vector<int> textColor = CoreSettingsGetDefaultIntListValue(SettingsID::GUI_OnScreenDisplayTextColor);
    if (backgroundColor.size() == 4)
    {
        this->currentBackgroundColor = QColor(backgroundColor.at(0), backgroundColor.at(1), backgroundColor.at(2), backgroundColor.at(3));
        this->chooseColor(this->changeBackgroundColorButton, &this->currentBackgroundColor, true);
    }
    if (textColor.size() == 4)
    {
        this->currentTextColor = QColor(textColor.at(0), textColor.at(1), textColor.at(2), textColor.at(3));
        this->chooseColor(this->changeTextColorButton, &this->currentTextColor, true);
    }
}

void SettingsDialog::loadDefaultInterfaceNetplaySettings(void)
{
    this->netplayNicknameLineEdit->setText(QString::fromStdString(CoreSettingsGetDefaultStringValue(SettingsID::Netplay_Nickname)));
    this->netplayServerUrlLineEdit->setText(QString::fromStdString(CoreSettingsGetDefaultStringValue(SettingsID::Netplay_ServerJsonUrl)));
}

void SettingsDialog::saveSettings(void)
{
    this->saveCoreSettings();
    if (this->showGameSettings)
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
    this->saveInterfaceGeneralSettings();
    this->saveInterfaceEmulationSettings();
    this->saveInterfaceRomBrowserSettings();
    this->saveInterfaceLogSettings();
    this->saveInterfaceOSDSettings();
    this->saveInterfaceNetplaySettings();
    CoreSettingsSave();
}

void SettingsDialog::saveCoreSettings(void)
{
    bool disableExtraMem = (this->coreMemorySizeComboBox->currentIndex() == 0);
    int counterFactor = this->coreCounterFactorComboBox->currentIndex() + 1;
    int cpuEmulator = this->coreCpuEmulatorComboBox->currentIndex();
    int saveFilenameFormat = this->coreSaveFilenameFormatComboBox->currentIndex();
    int siDmaDuration = this->coreSiDmaDurationSpinBox->value();
    bool randomizeInterrupt = this->coreRandomizeTimingCheckBox->isChecked();
    bool usePIF = this->usePifRomGroupBox->isChecked();
    QString ntscPifROM = this->ntscPifRomLineEdit->text();
    QString palPifROM = this->palPifRomLineEdit->text();
    bool overrideGameSettings = this->coreOverrideGameSettingsGroup->isChecked();

    CoreSettingsSetValue(SettingsID::CoreOverlay_CPU_Emulator, cpuEmulator);
    CoreSettingsSetValue(SettingsID::CoreOverLay_SaveFileNameFormat, saveFilenameFormat);
    CoreSettingsSetValue(SettingsID::CoreOverlay_RandomizeInterrupt, randomizeInterrupt);
    CoreSettingsSetValue(SettingsID::Core_PIF_Use, usePIF);
    CoreSettingsSetValue(SettingsID::Core_PIF_NTSC, ntscPifROM.toStdString());
    CoreSettingsSetValue(SettingsID::Core_PIF_PAL, palPifROM.toStdString());
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
    bool transferPak = this->gameTransferPakComboBox->currentIndex() != 0;
    int siDmaDuration = this->gameSiDmaDurationSpinBox->value();

    if ((this->defaultGameSettings.DisableExtraMem != disableExtraMem) ||
        (this->defaultGameSettings.SaveType != saveType) ||
        (this->defaultGameSettings.CountPerOp != countPerOp) ||
        (this->defaultGameSettings.TransferPak != transferPak) ||
        (this->defaultGameSettings.SiDMADuration != siDmaDuration))
    {
        CoreSettingsSetValue(SettingsID::Game_OverrideSettings, this->gameSection, true);
        CoreSettingsSetValue(SettingsID::Game_TransferPak, this->gameSection, transferPak);
        CoreSettingsSetValue(SettingsID::Game_DisableExtraMem, this->gameSection, disableExtraMem);
        CoreSettingsSetValue(SettingsID::Game_SaveType, this->gameSection, saveType);
        CoreSettingsSetValue(SettingsID::Game_CountPerOp, this->gameSection, countPerOp);
        CoreSettingsSetValue(SettingsID::Game_SiDmaDuration, this->gameSection, siDmaDuration);
    }

    // update cache when needed
    if (!this->currentGameFile.isEmpty())
    {
        CoreRomSettings romSettings = this->currentGameSettings;
        romSettings.SaveType = saveType;
        romSettings.DisableExtraMem = disableExtraMem;
        romSettings.CountPerOp = countPerOp;
        romSettings.TransferPak = transferPak;
        romSettings.SiDMADuration = siDmaDuration;
        CoreUpdateCachedRomHeaderAndSettings(this->currentGameFile.toStdU32String(), this->currentGameType, this->currentGameHeader, this->defaultGameSettings, romSettings);
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
    overclockingFactor = gameOverclockingFactorSpinBox->value();
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

void SettingsDialog::saveInterfaceGeneralSettings(void)
{
    CoreSettingsSetValue(SettingsID::GUI_Theme, this->themeComboBox->currentText().toStdString());
    CoreSettingsSetValue(SettingsID::GUI_IconTheme, this->iconThemeComboBox->currentText().toStdString());
#ifdef UPDATER
    CoreSettingsSetValue(SettingsID::GUI_CheckForUpdates, this->checkForUpdatesCheckBox->isChecked());
#endif // UPDATER
#ifdef DISCORD_RPC
    CoreSettingsSetValue(SettingsID::GUI_DiscordRpc, this->discordRpcCheckBox->isChecked());
#endif // DISCORD_RPC
}

void SettingsDialog::saveInterfaceEmulationSettings(void)
{
    CoreSettingsSetValue(SettingsID::GUI_HideCursorInEmulation, this->hideCursorCheckBox->isChecked());
    CoreSettingsSetValue(SettingsID::GUI_HideCursorInFullscreenEmulation, this->hideCursorFullscreenCheckBox->isChecked());
    CoreSettingsSetValue(SettingsID::GUI_PauseEmulationOnFocusLoss, this->pauseEmulationOnFocusCheckbox->isChecked());
    CoreSettingsSetValue(SettingsID::GUI_ResumeEmulationOnFocus, this->resumeEmulationOnFocusCheckBox->isChecked());
    CoreSettingsSetValue(SettingsID::GUI_AutomaticFullscreen, this->automaticFullscreenCheckbox->isChecked());
    CoreSettingsSetValue(SettingsID::GUI_ConfirmDragDrop, this->confirmDragDropCheckBox->isChecked());
    CoreSettingsSetValue(SettingsID::GUI_StatusbarMessageDuration, this->statusBarMessageDurationSpinBox->value());
    CoreSettingsSetValue(SettingsID::GUI_OpenGLES, this->openglTypeComboBox->currentIndex() == 1);
}

void SettingsDialog::saveInterfaceRomBrowserSettings(void)
{
    CoreSettingsSetValue(SettingsID::RomBrowser_Recursive, this->searchSubDirectoriesCheckbox->isChecked());
    CoreSettingsSetValue(SettingsID::RomBrowser_MaxItems, this->romSearchLimitSpinBox->value());
}

void SettingsDialog::saveInterfaceLogSettings(void)
{
    CoreSettingsSetValue(SettingsID::GUI_ShowVerboseLogMessages, this->showVerboseLogMessagesCheckBox->isChecked());
}

void SettingsDialog::saveInterfaceOSDSettings(void)
{
    CoreSettingsSetValue(SettingsID::GUI_OnScreenDisplayEnabled, this->osdEnabledCheckBox->isChecked());
    CoreSettingsSetValue(SettingsID::GUI_OnScreenDisplayLocation, this->osdLocationComboBox->currentIndex());
    CoreSettingsSetValue(SettingsID::GUI_OnScreenDisplayPaddingY, this->osdVerticalPaddingSpinBox->value());
    CoreSettingsSetValue(SettingsID::GUI_OnScreenDisplayPaddingX, this->osdHorizontalPaddingSpinBox->value());
    CoreSettingsSetValue(SettingsID::GUI_OnScreenDisplayDuration, this->osdDurationSpinBox->value());
    CoreSettingsSetValue(SettingsID::GUI_OnScreenDisplayBackgroundColor, std::vector<int>({ this->currentBackgroundColor.red(),
                                                                                            this->currentBackgroundColor.green(),
                                                                                            this->currentBackgroundColor.blue(),
                                                                                            this->currentBackgroundColor.alpha()
                                                                                           }));
    CoreSettingsSetValue(SettingsID::GUI_OnScreenDisplayTextColor, std::vector<int>({ this->currentTextColor.red(),
                                                                                      this->currentTextColor.green(),
                                                                                      this->currentTextColor.blue(),
                                                                                      this->currentTextColor.alpha()
                                                                                    }));
}

void SettingsDialog::saveInterfaceNetplaySettings(void)
{
    CoreSettingsSetValue(SettingsID::Netplay_Nickname, this->netplayNicknameLineEdit->text().toStdString());
    CoreSettingsSetValue(SettingsID::Netplay_ServerJsonUrl, this->netplayServerUrlLineEdit->text().toStdString());
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

    std::vector<keybinding> keybindings_SpeedFactor =
    {
        { this->speedFactor25KeyButton, SettingsID::KeyBinding_SpeedFactor25 },
        { this->speedFactor50KeyButton, SettingsID::KeyBinding_SpeedFactor50 },
        { this->speedFactor75KeyButton, SettingsID::KeyBinding_SpeedFactor75 },
        { this->speedFactor100KeyButton, SettingsID::KeyBinding_SpeedFactor100 },
        { this->speedFactor125KeyButton, SettingsID::KeyBinding_SpeedFactor125 },
        { this->speedFactor150KeyButton, SettingsID::KeyBinding_SpeedFactor150 },
        { this->speedFactor175KeyButton, SettingsID::KeyBinding_SpeedFactor175 },
        { this->speedFactor200KeyButton, SettingsID::KeyBinding_SpeedFactor200 },
        { this->speedFactor225KeyButton, SettingsID::KeyBinding_SpeedFactor225 },
        { this->speedFactor250KeyButton, SettingsID::KeyBinding_SpeedFactor250 },
        { this->speedFactor275KeyButton, SettingsID::KeyBinding_SpeedFactor275 },
        { this->speedFactor300KeyButton, SettingsID::KeyBinding_SpeedFactor300 },
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

    std::vector<keybinding> keybindings_Audio =
    {
        { this->increaseVolumeKeyButton, SettingsID::KeyBinding_IncreaseVolume },
        { this->decreaseVolumeKeyButton, SettingsID::KeyBinding_DecreaseVolume },
        { this->muteVolumeKeyButton, SettingsID::KeyBinding_ToggleMuteVolume },
    };

    std::vector<keybinding> keybindings_Settings =
    {
        { this->graphicsSettingsKeyButton, SettingsID::KeyBinding_GraphicsSettings },
        { this->audioSettingsKeyButton, SettingsID::KeyBinding_AudioSettings },
        { this->rspSettingsKeyButton, SettingsID::KeyBinding_RspSettings },
        { this->inputSettingsKeyButton, SettingsID::KeyBinding_InputSettings },
        { this->settingsKeyButton, SettingsID::KeyBinding_Settings },
    };

    std::vector<keybinding> keybindings_View =
    {
        { this->fullscreenKeyButton, SettingsID::KeyBinding_Fullscreen },
        { this->logKeyButton, SettingsID::Keybinding_ViewLog },
        { this->refreshRomListKeyButton, SettingsID::KeyBinding_RefreshROMList },
        { this->searchKeyButton, SettingsID::KeyBinding_ViewSearch }
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
            keybindings.insert(keybindings.end(), keybindings_SpeedFactor.begin(), keybindings_SpeedFactor.end());
            break;
        case 2:
            keybindings.insert(keybindings.end(), keybindings_CurrentSaveState.begin(), keybindings_CurrentSaveState.end());
            break;
        case 3:
            keybindings.insert(keybindings.end(), keybindings_Audio.begin(), keybindings_Audio.end());
            break;
        case 4:
            keybindings.insert(keybindings.end(), keybindings_Settings.begin(), keybindings_Settings.end());
            break;
        case 5:
            keybindings.insert(keybindings.end(), keybindings_View.begin(), keybindings_View.end());
            break;
        }
    }
    else
    {
        keybindings.insert(keybindings.end(), keybindings_System.begin(), keybindings_System.end());
        keybindings.insert(keybindings.end(), keybindings_SpeedFactor.begin(), keybindings_SpeedFactor.end());
        keybindings.insert(keybindings.end(), keybindings_CurrentSaveState.begin(), keybindings_CurrentSaveState.end());
        keybindings.insert(keybindings.end(), keybindings_Audio.begin(), keybindings_Audio.end());
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
            connect(keybinding.button, &KeybindButton::KeybindingChanged, this, &SettingsDialog::on_KeybindButton_KeybindingChanged);
            connect(keybinding.button, &KeybindButton::Clicked, this, &SettingsDialog::on_KeybindButton_Clicked);
            break;
        case SettingsDialogAction::LoadSettings:
            keybinding.button->SetText(QString::fromStdString(CoreSettingsGetStringValue(keybinding.settingId)));
            break;
        case SettingsDialogAction::LoadDefaultSettings:
            keybinding.button->SetText(QString::fromStdString(CoreSettingsGetDefaultStringValue(keybinding.settingId)));
            break;
        case SettingsDialogAction::SaveSettings:
            CoreSettingsSetValue(keybinding.settingId, keybinding.button->GetCurrentText().toStdString());
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
    QString pluginFileNames[4];

    QComboBox *comboBox;
    QString pluginName;
    QString pluginFileName;
    int index = 0;

    // clear combobox items
    for (const auto& c : comboBoxArray)
    {
        c->clear();
    }

    for (int i = 0; i < 4; i++)
    {
        pluginFileName = action == SettingsDialogAction::LoadSettings ? 
                            QString::fromStdString(CoreSettingsGetStringValue(settingsIdArray[i])) :
                            QString::fromStdString(CoreSettingsGetDefaultStringValue(settingsIdArray[i]));

        // account for full path (<v0.3.5 we used the full path)
        pluginFileName = QFileInfo(pluginFileName).fileName();
        
        pluginFileNames[i] = pluginFileName;
    }

    for (const auto &p : this->pluginList)
    {
        index = ((int)p.Type - 1);
        comboBox = comboBoxArray[index];
        pluginFileName = pluginFileNames[index];
        pluginName = QString::fromStdString(p.Name);

        comboBox->addItem(pluginName, QString::fromStdString(p.File));

        if (pluginFileName == QString::fromStdString(p.File))
        {
            comboBox->setCurrentText(pluginName);
            pluginFound[index] = true;
        }
    }

    for (int i = 0; i < 4; i++)
    {
        comboBox = comboBoxArray[i];
        if (!pluginFound[i])
        {
            pluginName = pluginFileNames[i] + " (not found)";

            comboBox->addItem(pluginName, pluginFileNames[i]);
            comboBox->setCurrentText(pluginName);
        }
        else
        { // find duplicates and append filename
            for (int i = 0; i < comboBox->count(); i++)
            {
                pluginName = comboBox->itemText(i);
                bool foundDuplicate = false;
                for (int x = i + 1; x < comboBox->count(); x++)
                {
                    if (comboBox->itemText(x) == pluginName)
                    {
                        pluginFileName = QFileInfo(comboBox->itemData(x).toString()).fileName();
                        comboBox->setItemText(x, pluginName + " (" + pluginFileName + ")");
                        foundDuplicate = true;
                    }
                }
                if (foundDuplicate)
                {
                    pluginFileName = QFileInfo(comboBox->itemData(i).toString()).fileName();
                    comboBox->setItemText(i, pluginName + " (" + pluginFileName + ")");
                }
            }
        }
    }
}

void SettingsDialog::setIconsForEmulationInfoText(void)
{
    QLabel* labels[] = {
        this->infoIconLabel_0, this->infoIconLabel_1, this->infoIconLabel_2,
        this->infoIconLabel_3, this->infoIconLabel_4, this->infoIconLabel_5,
        this->infoIconLabel_7, this->infoIconLabel_6, this->infoIconLabel_8,
        this->infoIconLabel_9
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

void SettingsDialog::chooseDirectory(QLineEdit *lineEdit, QString caption)
{
    QString dir = QFileDialog::getExistingDirectory(this, caption);
    if (dir.isEmpty())
    {
        return;
    }

    lineEdit->setText(QDir::toNativeSeparators(dir));
}

void SettingsDialog::chooseFile(QLineEdit *lineEdit, QString caption, QString filter, QString md5)
{
    QString file = QFileDialog::getOpenFileName(this, caption, "", filter);
    if (file.isEmpty())
    {
        return;
    }

    if (!md5.isEmpty())
    {
        QFile qFile(file);
        if (!qFile.open(QFile::ReadOnly))
        {
            QtMessageBox::Error(this, "Failed to open file", "QFile::open() Failed");
            return;
        }

        QCryptographicHash hash(QCryptographicHash::Algorithm::Md5);
        if (hash.addData(&qFile))
        {
            QString md5Hash = QString(hash.result().toHex());
            if (md5Hash != md5)
            {
                QtMessageBox::Error(this, "MD5 mismatch", "Expected file with MD5: \"" + md5 + "\"");
                return;
            }
        }
    }

    lineEdit->setText(QDir::toNativeSeparators(file));
}

void SettingsDialog::chooseColor(QPushButton *pushButton, QColor *initialColor, bool skipChoice)
{
    QColor color = QColor::Invalid;

    if (skipChoice)
    {
        color = *initialColor;
    }
    else
    {
        color = QColorDialog::getColor(*initialColor, this, "", QColorDialog::ColorDialogOption::ShowAlphaChannel);
    }

    if (!color.isValid())
    {
        return;
    }

    pushButton->setStyleSheet("background-color: " + color.name());
    *initialColor = color;
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
            QtMessageBox::Error(this, "CoreApplyPluginSettings() Failed", QString::fromStdString(CoreGetError()));
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

void SettingsDialog::timerEvent(QTimerEvent* event)
{
    this->keybindButtonTimeLeft--;

    if (this->currentKeybindButton != nullptr)
    {
        this->currentKeybindButton->SetSecondsLeft(this->keybindButtonTimeLeft);
    }

    if (this->keybindButtonTimeLeft == 0)
    {
        this->killTimer(this->keybindButtonTimerId);
        this->keybindButtonTimerId = -1;
        this->currentKeybindButton = nullptr;
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
    this->chooseDirectory(this->screenshotDirLineEdit, tr("Select Screenshot Directory"));
}

void SettingsDialog::on_changeSaveStateDirButton_clicked(void)
{
    this->chooseDirectory(this->saveStateDirLineEdit, tr("Select Save (State) Directory"));
}

void SettingsDialog::on_changeSaveSramDirButton_clicked(void)
{
    this->chooseDirectory(this->saveSramDirLineEdit, tr("Select Save (SRAM) Directory"));
}

void SettingsDialog::on_changeUserDataDirButton_clicked(void)
{
    this->chooseDirectory(this->userDataDirLineEdit, tr("Select User Data Directory"));
}

void SettingsDialog::on_changeUserCacheDirButton_clicked(void)
{
    this->chooseDirectory(this->userCacheDirLineEdit, tr("Select User Cache Directory"));
}

void SettingsDialog::on_changeJapaneseIPLRomPathButton_clicked(void)
{
    this->chooseFile(this->japaneseIPLRomLineEdit, tr("Open Japanese Retail 64DD IPL"), "IPL ROMs (*.n64)");
}

void SettingsDialog::on_changeAmericanIPLRomPathButton_clicked(void)
{
    this->chooseFile(this->americanIPLRomLineEdit, tr("Open American Retail 64DD IPL"), "IPL ROMs (*.n64)");
}

void SettingsDialog::on_changeDevelopmentIPLRomPathButton_clicked(void)
{
    this->chooseFile(this->developmentIPLRomLineEdit, tr("Open Japanese Development 64DD IPL"), "IPL ROMs (*.n64)");
}

void SettingsDialog::on_changeBackgroundColorButton_clicked(void)
{
    this->chooseColor(this->changeBackgroundColorButton, &this->currentBackgroundColor);
}

void SettingsDialog::on_changeTextColorButton_clicked(void)
{
    this->chooseColor(this->changeTextColorButton, &this->currentTextColor);
}

void SettingsDialog::on_KeybindButton_KeybindingChanged(KeybindButton* button)
{
    if (this->keybindButtonTimerId != -1)
    {
        this->killTimer(this->keybindButtonTimerId);
        this->keybindButtonTimerId = -1;
    }

    if (this->currentKeybindButton != nullptr)
    {
        this->currentKeybindButton->Reset();
        this->currentKeybindButton = nullptr;
    }
   
    if (!this->removeDuplicateHotkeysCheckBox->isChecked())
    {
        return;
    }

    QString text = button->GetCurrentText();
    if (text.isEmpty())
    {
        return;
    }

    QList<KeybindButton*> rombrowserKeybindButtons =
    {
        this->startRomKeyButton,
        this->startComboKeyButton,
        this->shutdownKeyButton,
        this->exitKeyButton,
        this->graphicsSettingsKeyButton,
        this->audioSettingsKeyButton,
        this->rspSettingsKeyButton,
        this->inputSettingsKeyButton,
        this->settingsKeyButton,
        this->logKeyButton,
        this->refreshRomListKeyButton,
    };

    QList<KeybindButton*> emulationKeybindButtons =
    {
        this->shutdownKeyButton,
        this->softResetKeyButton,
        this->hardResetKeyButton,
        this->pauseKeyButton,
        this->generateBitmapKeyButton,
        this->limitFPSKeyButton,
        this->speedFactor25KeyButton,
        this->speedFactor50KeyButton,
        this->speedFactor75KeyButton,
        this->speedFactor100KeyButton,
        this->speedFactor125KeyButton,
        this->speedFactor150KeyButton,
        this->speedFactor175KeyButton,
        this->speedFactor200KeyButton,
        this->speedFactor225KeyButton,
        this->speedFactor250KeyButton,
        this->speedFactor275KeyButton,
        this->speedFactor300KeyButton,
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
        this->increaseVolumeKeyButton,
        this->decreaseVolumeKeyButton,
        this->muteVolumeKeyButton,
        this->exitKeyButton,
        this->graphicsSettingsKeyButton,
        this->audioSettingsKeyButton,
        this->rspSettingsKeyButton,
        this->inputSettingsKeyButton,
        this->settingsKeyButton,
        this->logKeyButton,
        this->fullscreenKeyButton,
    };

    QList<KeybindButton*> keybindButtons;

    if (rombrowserKeybindButtons.contains(button))
    {
        keybindButtons.append(rombrowserKeybindButtons);
    }
    if (emulationKeybindButtons.contains(button))
    {
        keybindButtons.append(emulationKeybindButtons);
    }

    for (KeybindButton* keybindButton : keybindButtons)
    {
        if (keybindButton != button)
        {
            if (keybindButton->GetCurrentText() == text)
            {
                keybindButton->Clear();
            }
        }
    }
}

void SettingsDialog::on_KeybindButton_Clicked(KeybindButton* button)
{
    if (this->currentKeybindButton == button)
    {
        return;
    }

    if (this->currentKeybindButton != nullptr)
    {
        this->currentKeybindButton->Reset();
        this->currentKeybindButton = nullptr;
    }

    if (this->keybindButtonTimerId != -1)
    {
        this->killTimer(this->keybindButtonTimerId);
        this->keybindButtonTimerId = -1;
    }

    this->currentKeybindButton  = button;
    this->keybindButtonTimeLeft = 5;
    this->keybindButtonTimerId  = this->startTimer(1000);

    // notify button
    this->currentKeybindButton->SetSecondsLeft(5);
}

void SettingsDialog::on_coreCpuEmulatorComboBox_currentIndexChanged(int index)
{
    // hide PIF ROM options when using dynamic recompiler
    // because the dynarec crashes when using a PIF ROM
    this->usePifRomGroupBox->setVisible(index != 2);
}

void SettingsDialog::on_changeNTSCPifRomButton_clicked(void)
{
    this->chooseFile(this->ntscPifRomLineEdit, tr("Open NTSC PIF ROM"), "PIF ROMs (*.rom)", "5c124e7948ada85da603a522782940d0");
}

void SettingsDialog::on_changePALPifRomButton_clicked(void)
{
    this->chooseFile(this->palPifRomLineEdit, tr("Open PAL PIF ROM"), "PIF ROMs (*.rom)", "d4232dc935cad0650ac2664d52281f3a");
}
