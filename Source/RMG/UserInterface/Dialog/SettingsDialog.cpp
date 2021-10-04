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
#include "Utilities/SettingsID.hpp"
#include "Globals.hpp"

#include <QFileDialog>

using namespace UserInterface::Dialog;
using namespace M64P::Wrapper;

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
    this->setupUi(this);

    this->inGame = g_EmuThread->isRunning();
    if (inGame)
    {
        g_MupenApi.Core.GetRomInfo(&this->gameInfo);
        g_MupenApi.Core.GetDefaultRomInfo(&this->defaultGameInfo);
        this->gameSection = this->gameInfo.Settings.MD5;
    }
    else
    {
        this->hideEmulationInfoText();
        this->tabWidget->setTabEnabled(1, false);
    }

    pluginList = g_Plugins.GetAvailablePlugins();

    for (int i = 0; i < 8; i++)
    {
        this->reloadSettings(i);
    }

    int width = g_Settings.GetIntValue(SettingsID::GUI_SettingsDialogWidth);
    int height = g_Settings.GetIntValue(SettingsID::GUI_SettingsDialogHeight);

    if (width != 0 && height != 0)
    {
        // center current dialog
        this->setGeometry(
            QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, QSize(width, height), parent->geometry()));
    }
}

SettingsDialog::~SettingsDialog(void)
{
    g_Settings.SetValue(SettingsID::GUI_SettingsDialogWidth, this->size().width());
    g_Settings.SetValue(SettingsID::GUI_SettingsDialogHeight, this->size().height());
}

int SettingsDialog::currentIndex(void)
{
    int currentIndex = this->tabWidget->currentIndex();

    if (currentIndex == 1)
    { // game tab
        currentIndex += this->innerGameTabWidget->currentIndex();
    } else if (currentIndex > 1)
    { // above game tab
        currentIndex += this->innerGameTabWidget->count() - 1;
    }

    return currentIndex;
}

void SettingsDialog::restoreDefaults(int stackedWidgetIndex)
{
    switch (stackedWidgetIndex)
    {
    default:
    case 0:
        loadDefaultCoreSettings();
        break;
    case 1:
        loadDefaultGameSettings();
        break;
    case 2:
        loadDefaultGameCoreSettings();
        break;
    case 3:
        loadDefaultGamePluginSettings();
        break;
    case 4:
        loadPluginSettings();
        break;
    case 5:
        loadDefaultDirectorySettings();
        break;
    case 6:
        loadDefaultHotkeySettings();
        break;
    case 7:
        loadDefaultBehaviorSettings();
        break;
    }
}

void SettingsDialog::reloadSettings(int stackedWidgetIndex)
{
    switch (stackedWidgetIndex)
    {
    default:
    case 0:
        loadCoreSettings();
        break;
    case 1:
        loadGameSettings();
        break;
    case 2:
        loadGameCoreSettings();
        break;
    case 3:
        loadGamePluginSettings();
        break;
    case 4:
        loadPluginSettings();
        break;
    case 5:
        loadDirectorySettings();
        break;
    case 6:
        loadHotkeySettings();
        break;
    case 7:
        loadBehaviorSettings();
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

    disableExtraMem = g_Settings.GetBoolValue(SettingsID::Core_DisableExtraMem);
    counterFactor = g_Settings.GetIntValue(SettingsID::Core_CountPerOp);
    cpuEmulator = g_Settings.GetIntValue(SettingsID::Core_CPU_Emulator);
    siDmaDuration = g_Settings.GetIntValue(SettingsID::Core_SiDmaDuration);
    randomizeInterrupt = g_Settings.GetBoolValue(SettingsID::Core_RandomizeInterrupt);
    debugger = g_Settings.GetBoolValue(SettingsID::Core_EnableDebugger);
    overrideGameSettings = g_Settings.GetBoolValue(SettingsID::Core_OverrideGameSpecificSettings);

    this->coreCpuEmulatorComboBox->setCurrentIndex(cpuEmulator);
    this->coreRandomizeTimingCheckBox->setChecked(randomizeInterrupt);
    this->coreDebuggerCheckBox->setChecked(debugger);

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
    this->gameGoodNameLineEdit->setText(this->gameInfo.Settings.goodname);
    this->gameMemorySizeComboBox->setCurrentIndex(!this->gameInfo.Settings.disableextramem);
    this->gameSaveTypeComboBox->setCurrentIndex(this->gameInfo.Settings.savetype);
    this->gameCounterFactorComboBox->setCurrentIndex(this->gameInfo.Settings.countperop - 1);
    this->gameSiDmaDurationSpinBox->setValue(this->gameInfo.Settings.sidmaduration);
}

void SettingsDialog::loadGameCoreSettings(void)
{
    bool overrideEnabled, randomizeInterrupt;
    int cpuEmulator = 0;

    overrideEnabled = g_Settings.GetBoolValue(SettingsID::Game_OverrideCoreSettings, this->gameSection);
    cpuEmulator = g_Settings.GetIntValue(SettingsID::Game_CPU_Emulator, this->gameSection);
    randomizeInterrupt = g_Settings.GetBoolValue(SettingsID::Game_RandomizeInterrupt, this->gameSection);

    gameOverrideCoreSettingsGroupBox->setChecked(overrideEnabled);
    gameCoreCpuEmulatorComboBox->setCurrentIndex(cpuEmulator);
    gameRandomizeTimingCheckBox->setChecked(randomizeInterrupt);
}

void SettingsDialog::loadGamePluginSettings(void)
{
    QComboBox *comboBoxArray[4] = {this->gameVideoPluginsComboBox, this->gameAudioPluginsComboBox,
                                   this->gameInputPluginsComboBox, this->gameRspPluginsComboBox};
    SettingsID settingsId[4] = {SettingsID::Game_GFX_Plugin, SettingsID::Game_AUDIO_Plugin,
                                SettingsID::Game_INPUT_Plugin, SettingsID::Game_RSP_Plugin};
    QComboBox *comboBox;

    for (QComboBox *comboBox : comboBoxArray)
    {
        comboBox->clear();
        comboBox->addItem("**Use Core Plugin Settings**");
    }

    for (const auto &p : this->pluginList)
    {
        comboBox = comboBoxArray[(int)p.Type];
        comboBox->addItem(p.Name, p.FileName);
        if (g_Settings.GetStringValue(settingsId[(int)p.Type], this->gameSection) == p.FileName)
        {
            comboBox->setCurrentText(p.Name);
        }
    }
}

void SettingsDialog::loadPluginSettings(void)
{
    QComboBox *comboBoxArray[] = {this->videoPluginsComboBox, this->audioPluginsComboBox, this->inputPluginsComboBox,
                                  this->rspPluginsComboBox};
    SettingsID settingsIdArray[] = {SettingsID::Core_GFX_Plugin, SettingsID::Core_AUDIO_Plugin,
                                    SettingsID::Core_INPUT_Plugin, SettingsID::Core_RSP_Plugin};

    PluginType type;
    QComboBox *comboBox;
    QString pluginFileName;
    int index = 0;

    for (const auto &p : this->pluginList)
    {
        comboBox = comboBoxArray[(int)p.Type];
        pluginFileName = g_Settings.GetStringValue(settingsIdArray[(int)p.Type]);

        comboBox->addItem(p.Name, p.FileName);

        if (pluginFileName == p.FileName)
        {
            comboBox->setCurrentText(p.Name);
        }
    }
}

void SettingsDialog::loadDirectorySettings(void)
{
    // these need to be static, otherwise Qt will segfault
    static QString screenshotDir;
    static QString saveStateDir;
    static QString saveSramDir;
    static QString sharedDataDir;
    static bool overrideUserDirs = false;
    static QString userDataDir;
    static QString userCacheDir;

    screenshotDir = g_Settings.GetStringValue(SettingsID::Core_ScreenshotPath);
    saveStateDir = g_Settings.GetStringValue(SettingsID::Core_SaveStatePath);
    saveSramDir = g_Settings.GetStringValue(SettingsID::Core_SaveSRAMPath);
    sharedDataDir = g_Settings.GetStringValue(SettingsID::Core_SharedDataPath);

    overrideUserDirs = g_Settings.GetBoolValue(SettingsID::Core_OverrideUserDirs);
    userDataDir = g_Settings.GetStringValue(SettingsID::Core_UserDataDirOverride);
    userCacheDir = g_Settings.GetStringValue(SettingsID::Core_UserCacheDirOverride);

    this->screenshotDirLineEdit->setText(screenshotDir);
    this->saveStateDirLineEdit->setText(saveStateDir);
    this->saveSramDirLineEdit->setText(saveSramDir);
    this->sharedDataDirLineEdit->setText(sharedDataDir);
    this->overrideUserDirsGroupBox->setChecked(overrideUserDirs);
    this->userDataDirLineEdit->setText(userDataDir);
    this->userCacheDirLineEdit->setText(userCacheDir);
}

void SettingsDialog::loadHotkeySettings(void)
{
    this->commonHotkeySettings(0);
}

void SettingsDialog::loadBehaviorSettings(void)
{
    this->manualResizingCheckBox->setChecked(g_Settings.GetBoolValue(SettingsID::GUI_AllowManualResizing));
    this->hideCursorCheckBox->setChecked(g_Settings.GetBoolValue(SettingsID::GUI_HideCursorInEmulation));
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

    disableExtraMem = g_Settings.GetDefaultBoolValue(SettingsID::Core_DisableExtraMem);
    counterFactor = g_Settings.GetDefaultIntValue(SettingsID::Core_CountPerOp);
    cpuEmulator = g_Settings.GetDefaultIntValue(SettingsID::Core_CPU_Emulator);
    siDmaDuration = g_Settings.GetDefaultIntValue(SettingsID::Core_SiDmaDuration);
    randomizeInterrupt = g_Settings.GetDefaultBoolValue(SettingsID::Core_RandomizeInterrupt);
    debugger = g_Settings.GetDefaultBoolValue(SettingsID::Core_EnableDebugger);
    overrideGameSettings = g_Settings.GetDefaultBoolValue(SettingsID::Core_OverrideGameSpecificSettings);

    this->coreCpuEmulatorComboBox->setCurrentIndex(cpuEmulator);
    this->coreRandomizeTimingCheckBox->setChecked(randomizeInterrupt);
    this->coreDebuggerCheckBox->setChecked(debugger);

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
    this->gameGoodNameLineEdit->setText(this->gameInfo.Settings.goodname);
    this->gameMemorySizeComboBox->setCurrentIndex(!this->gameInfo.Settings.disableextramem);
    this->gameSaveTypeComboBox->setCurrentIndex(this->gameInfo.Settings.savetype);
    this->gameCounterFactorComboBox->setCurrentIndex(this->gameInfo.Settings.countperop - 1);
    this->gameSiDmaDurationSpinBox->setValue(this->gameInfo.Settings.sidmaduration);
}

void SettingsDialog::loadDefaultGameCoreSettings(void)
{
    bool overrideEnabled, randomizeInterrupt;
    int cpuEmulator = 0;

    overrideEnabled = g_Settings.GetDefaultBoolValue(SettingsID::Game_OverrideCoreSettings);
    cpuEmulator = g_Settings.GetDefaultIntValue(SettingsID::Game_CPU_Emulator);
    randomizeInterrupt = g_Settings.GetDefaultBoolValue(SettingsID::Game_RandomizeInterrupt);

    gameOverrideCoreSettingsGroupBox->setChecked(overrideEnabled);
    gameCoreCpuEmulatorComboBox->setCurrentIndex(cpuEmulator);
    gameRandomizeTimingCheckBox->setChecked(randomizeInterrupt);
}

void SettingsDialog::loadDefaultGamePluginSettings(void)
{
    QComboBox *comboBoxArray[4] = {this->gameVideoPluginsComboBox, this->gameAudioPluginsComboBox,
                                   this->gameInputPluginsComboBox, this->gameRspPluginsComboBox};

    for (QComboBox *comboBox : comboBoxArray)
    {
        comboBox->setCurrentIndex(0);
    }
}

void SettingsDialog::loadDefaultDirectorySettings(void)
{
    this->screenshotDirLineEdit->setText(g_Settings.GetDefaultStringValue(SettingsID::Core_ScreenshotPath));
    this->saveStateDirLineEdit->setText(g_Settings.GetDefaultStringValue(SettingsID::Core_SaveStatePath));
    this->saveSramDirLineEdit->setText(g_Settings.GetDefaultStringValue(SettingsID::Core_SaveSRAMPath));
    this->sharedDataDirLineEdit->setText(g_Settings.GetDefaultStringValue(SettingsID::Core_SharedDataPath));
    this->overrideUserDirsGroupBox->setChecked(g_Settings.GetDefaultBoolValue(SettingsID::Core_OverrideUserDirs));
    this->userDataDirLineEdit->setText(g_Settings.GetDefaultStringValue(SettingsID::Core_UserDataDirOverride));
    this->userCacheDirLineEdit->setText(g_Settings.GetDefaultStringValue(SettingsID::Core_UserCacheDirOverride));
}

void SettingsDialog::loadDefaultHotkeySettings(void)
{
    this->commonHotkeySettings(1);
}

void SettingsDialog::loadDefaultBehaviorSettings(void)
{
    this->manualResizingCheckBox->setChecked(g_Settings.GetDefaultBoolValue(SettingsID::GUI_AllowManualResizing));
    this->hideCursorCheckBox->setChecked(g_Settings.GetDefaultBoolValue(SettingsID::GUI_HideCursorInEmulation));
}

void SettingsDialog::saveSettings(void)
{
    this->saveCoreSettings();
    if (inGame)
    {
        // clean 'game settings'
        g_MupenApi.Config.DeleteSection(this->gameSection);
        this->saveGameSettings();
        this->saveGameCoreSettings();
        this->saveGamePluginSettings();
    }
    this->savePluginSettings();
    this->saveDirectorySettings();
    this->saveHotkeySettings();
    this->saveBehaviorSettings();
}

void SettingsDialog::saveCoreSettings(void)
{
    bool disableExtraMem = (this->coreMemorySizeComboBox->currentIndex() == 0);
    int counterFactor = this->coreCounterFactorComboBox->currentIndex() + 1;
    int cpuEmulator = this->coreCpuEmulatorComboBox->currentIndex();
    int siDmaDuration = this->coreSiDmaDurationSpinBox->value();
    bool randomizeInterrupt = this->coreRandomizeTimingCheckBox->isChecked();
    bool debugger = this->coreDebuggerCheckBox->isChecked();
    bool overrideGameSettings = this->coreOverrideGameSettingsGroup->isChecked();

    g_Settings.SetValue(SettingsID::Core_CPU_Emulator, cpuEmulator);
    g_Settings.SetValue(SettingsID::Core_RandomizeInterrupt, randomizeInterrupt);
    g_Settings.SetValue(SettingsID::Core_EnableDebugger, debugger);
    g_Settings.SetValue(SettingsID::Core_OverrideGameSpecificSettings, overrideGameSettings);

    if (!overrideGameSettings)
    {
        disableExtraMem = false;
        counterFactor = 0;
        siDmaDuration = -1;
    }

    g_Settings.SetValue(SettingsID::Core_DisableExtraMem, disableExtraMem);
    g_Settings.SetValue(SettingsID::Core_CountPerOp, counterFactor);
    g_Settings.SetValue(SettingsID::Core_SiDmaDuration, siDmaDuration);
}

void SettingsDialog::saveGameSettings(void)
{
    bool disableExtraMem = this->gameMemorySizeComboBox->currentIndex() == 0;
    int saveType = this->gameSaveTypeComboBox->currentIndex();
    int countPerOp = this->gameCounterFactorComboBox->currentIndex() + 1;
    int siDmaDuration = this->gameSiDmaDurationSpinBox->value();

    if (this->defaultGameInfo.Settings.disableextramem != (unsigned char)disableExtraMem)
    {
        g_Settings.SetValue(SettingsID::Game_DisableExtraMem, this->gameSection, disableExtraMem);
    }
    if (this->defaultGameInfo.Settings.savetype != saveType)
    {
        g_Settings.SetValue(SettingsID::Game_SaveType, this->gameSection, saveType);
    }
    if (this->defaultGameInfo.Settings.countperop != countPerOp)
    {
        g_Settings.SetValue(SettingsID::Game_CountPerOp, this->gameSection, countPerOp);
    }
    if (this->defaultGameInfo.Settings.sidmaduration != siDmaDuration)
    {
        g_Settings.SetValue(SettingsID::Game_SiDmaDuration, this->gameSection, siDmaDuration);
    }
}

void SettingsDialog::saveGameCoreSettings(void)
{
    bool overrideEnabled, randomizeInterrupt;
    bool defaultOverrideEnabled, defaultRandomizeInterrupt;
    int cpuEmulator = 0, defaultCpuEmulator;

    overrideEnabled = gameOverrideCoreSettingsGroupBox->isChecked();
    cpuEmulator = gameCoreCpuEmulatorComboBox->currentIndex();
    randomizeInterrupt = gameRandomizeTimingCheckBox->isChecked();

    defaultOverrideEnabled = g_Settings.GetDefaultBoolValue(SettingsID::Game_OverrideCoreSettings);
    defaultRandomizeInterrupt = g_Settings.GetDefaultBoolValue(SettingsID::Game_RandomizeInterrupt);
    defaultCpuEmulator = g_Settings.GetDefaultIntValue(SettingsID::Game_CPU_Emulator);

    if (defaultOverrideEnabled != overrideEnabled)
        g_Settings.SetValue(SettingsID::Game_OverrideCoreSettings, this->gameSection, overrideEnabled);
    if (defaultCpuEmulator != cpuEmulator)
        g_Settings.SetValue(SettingsID::Game_CPU_Emulator, this->gameSection, cpuEmulator);
    if (defaultRandomizeInterrupt != randomizeInterrupt)
        g_Settings.SetValue(SettingsID::Game_RandomizeInterrupt, this->gameSection, randomizeInterrupt);
}

void SettingsDialog::saveGamePluginSettings(void)
{
    QComboBox *comboBoxArray[4] = {this->gameVideoPluginsComboBox, this->gameAudioPluginsComboBox,
                                   this->gameInputPluginsComboBox, this->gameRspPluginsComboBox};
    SettingsID settingsIdArray[4] = {SettingsID::Game_GFX_Plugin, SettingsID::Game_AUDIO_Plugin,
                                     SettingsID::Game_INPUT_Plugin, SettingsID::Game_RSP_Plugin};
    QComboBox *comboBox;
    SettingsID id;

    for (int i = 0; i < 4; i++)
    {
        comboBox = comboBoxArray[i];
        id = settingsIdArray[i];

        if (comboBox->currentIndex() != 0)
        {
            g_Settings.SetValue(id, this->gameSection, comboBox->currentData().toString());
        }
    }
}

void SettingsDialog::savePluginSettings(void)
{
    QComboBox *comboBoxArray[4] = {this->videoPluginsComboBox, this->audioPluginsComboBox, this->inputPluginsComboBox,
                                   this->rspPluginsComboBox};
    QComboBox *comboBox;

    for (const auto &p : this->pluginList)
    {
        comboBox = comboBoxArray[(int)p.Type];

        if (p.FileName == comboBox->currentData().toString())
        {
            g_Plugins.ChangePlugin(p);
        }
    }
}

void SettingsDialog::saveDirectorySettings(void)
{
    g_Settings.SetValue(SettingsID::Core_ScreenshotPath, this->screenshotDirLineEdit->text());
    g_Settings.SetValue(SettingsID::Core_SaveStatePath, this->saveStateDirLineEdit->text());
    g_Settings.SetValue(SettingsID::Core_SaveSRAMPath, this->saveSramDirLineEdit->text());
    g_Settings.SetValue(SettingsID::Core_SharedDataPath, this->sharedDataDirLineEdit->text());

    g_Settings.SetValue(SettingsID::Core_OverrideUserDirs, this->overrideUserDirsGroupBox->isChecked());
    g_Settings.SetValue(SettingsID::Core_UserDataDirOverride, this->userDataDirLineEdit->text());
    g_Settings.SetValue(SettingsID::Core_UserCacheDirOverride, this->userCacheDirLineEdit->text());
}

void SettingsDialog::saveHotkeySettings(void)
{
    this->commonHotkeySettings(2);
}

void SettingsDialog::saveBehaviorSettings(void)
{
    g_Settings.SetValue(SettingsID::GUI_AllowManualResizing, this->manualResizingCheckBox->isChecked());
    g_Settings.SetValue(SettingsID::GUI_HideCursorInEmulation, this->hideCursorCheckBox->isChecked());
}

void SettingsDialog::commonHotkeySettings(int action)
{
    struct
    {
        KeyBindButton *button;
        SettingsID settingId;
    } keybindings[] =
    {
        { this->openRomKeyButton, SettingsID::KeyBinding_OpenROM },
        { this->openComboKeyButton, SettingsID::KeyBinding_OpenCombo },
        { this->startEmuKeyButton, SettingsID::KeyBinding_StartEmulation },
        { this->endEmuKeyButton, SettingsID::KeyBinding_EndEmulation },
        { this->refreshRomListKeyButton, SettingsID::KeyBinding_RefreshROMList },
        { this->exitKeyButton, SettingsID::KeyBinding_Exit },
        { this->softResetKeyButton, SettingsID::KeyBinding_SoftReset },
        { this->hardResetKeyButton, SettingsID::KeyBinding_HardReset },
        { this->pauseKeyButton, SettingsID::KeyBinding_Resume },
        { this->generateBitmapKeyButton, SettingsID::KeyBinding_GenerateBitmap },
        { this->limitFPSKeyButton, SettingsID::KeyBinding_LimitFPS },
        { this->swapDiskKeyButton, SettingsID::KeyBinding_SwapDisk },
        { this->saveStateKeyButton, SettingsID::KeyBinding_SaveState },
        { this->saveAsKeyButton, SettingsID::KeyBinding_SaveAs },
        { this->loadStateKeyButton, SettingsID::KeyBinding_LoadState },
        { this->loadKeyButton, SettingsID::KeyBinding_Load },
        { this->cheatsKeyButton, SettingsID::KeyBinding_Cheats },
        { this->gsButtonKeyButton, SettingsID::KeyBinding_GSButton },
        { this->fullscreenKeyButton, SettingsID::KeyBinding_Fullscreen },
        { this->settingsKeyButton, SettingsID::KeyBinding_Settings },
    };

    for (const auto& keybinding : keybindings)
    {
        switch (action)
        {
        default:
        case 0:
            keybinding.button->setText(g_Settings.GetStringValue(keybinding.settingId));
            break;
        case 1:
            keybinding.button->setText(g_Settings.GetDefaultStringValue(keybinding.settingId));
            break;
        case 2:
            g_Settings.SetValue(keybinding.settingId, keybinding.button->text());
            break;
        }
    }
}

void SettingsDialog::hideEmulationInfoText(void)
{
    QHBoxLayout *layouts[] = {this->emulationInfoLayout_0, this->emulationInfoLayout_1, this->emulationInfoLayout_2};

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
    QFileDialog dialog;
    int ret;

    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);

    ret = dialog.exec();
    if (!ret)
    {
        return;
    }

    lineEdit->setText(dialog.directory().path());
}

void SettingsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    QPushButton *pushButton = (QPushButton *)button;
    QPushButton *defaultButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);
    QPushButton *cancelButton = this->buttonBox->button(QDialogButtonBox::Cancel);
    QPushButton *okButton = this->buttonBox->button(QDialogButtonBox::Ok);

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
        this->saveSettings();
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

void SettingsDialog::on_changeSharedDataDirButton_clicked(void)
{
    this->chooseDirectory(this->sharedDataDirLineEdit);
}

void SettingsDialog::on_changeUserDataDirButton_clicked(void)
{
    this->chooseDirectory(this->userDataDirLineEdit);
}

void SettingsDialog::on_changeUserCacheDirButton_clicked(void)
{
    this->chooseDirectory(this->userCacheDirLineEdit);
}
