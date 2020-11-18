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
#include "../../Globals.hpp"
#include "Utilities/SettingsID.hpp"

#include <QFileDialog>

using namespace UserInterface::Dialog;
using namespace M64P::Wrapper;

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
    this->setupUi(this);

    this->treeWidget->topLevelItem(0)->setSelected(true);
    this->treeWidget->expandAll();

    for (int i = 0; i < this->stackedWidget->count(); i++)
        this->reloadSettings(i);

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
        loadGameCoreSettings();
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

    this->coreCpuEmulator->setCurrentIndex(cpuEmulator);
    this->coreRandomizeTiming->setChecked(randomizeInterrupt);
    this->coreDebugger->setChecked(debugger);

    this->coreOverrideGameSettingsGroup->setChecked(overrideGameSettings);

    if (!overrideGameSettings)
    {
        disableExtraMem = false;
        counterFactor = 2;
        siDmaDuration = 2304;
    }

    this->coreMemorySize->setCurrentIndex(!disableExtraMem);
    this->coreCounterFactor->setCurrentIndex(counterFactor - 1);
    this->coreSiDmaDuration->setValue(siDmaDuration);
}

void SettingsDialog::loadGameSettings(void)
{
    if (!g_EmuThread->isRunning())
    {
        this->hideEmulationInfoText();
        this->treeWidget->topLevelItem(1)->setDisabled(true);
        return;
    }

    M64P::Wrapper::RomInfo_t gameInfo = {0};

    g_MupenApi.Core.GetRomInfo(&gameInfo);

    this->gameGoodName->setText(gameInfo.Settings.goodname);
    this->gameMemorySize->setCurrentIndex(!gameInfo.Settings.disableextramem);
    this->gameSaveType->setCurrentIndex(gameInfo.Settings.savetype);
    this->gameCounterFactor->setCurrentIndex(gameInfo.Settings.countperop - 1);
    this->gameSiDmaDuration->setValue(gameInfo.Settings.sidmaduration);
}

void SettingsDialog::loadGameCoreSettings(void)
{
}

void SettingsDialog::loadGamePluginSettings(void)
{
    QComboBox *comboBoxArray[4] = {this->pluginVideoPlugins, this->pluginAudioPlugins, this->pluginInputPlugins,
                                   this->pluginRspPlugins};

    for (QComboBox *comboBox : comboBoxArray)
    {
        comboBox->clear();
        comboBox->addItem("**Use Core Plugin Settings**");
    }
}

void SettingsDialog::loadPluginSettings(void)
{
    QComboBox *comboBoxArray[4] = {this->videoPlugins, this->audioPlugins, this->inputPlugins, this->rspPlugins};
    PluginType type;
    QComboBox *comboBox;
    Plugin_t plugin;
    int index = 0;

    for (const Plugin_t &p : g_Plugins.GetAvailablePlugins())
    {
        comboBox = comboBoxArray[(int)p.Type];
        plugin = g_Plugins.GetCurrentPlugin(p.Type);

        comboBox->addItem(p.Name);

        if (plugin.FileName == p.FileName)
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

void SettingsDialog::loadBehaviorSettings(void)
{
    bool pause = false, resume = false;

    /*
        pause = g_Settings.GetBoolValue(SettingsID::GUI_PauseEmulationOnFocusLoss);
        resume = g_Settings.GetBoolValue(SettingsID::GUI_ResumeEmulationOnFocus);
    */

    this->pauseEmulationOnFocusLoss->setChecked(pause);
    this->resumeEmulationOnFocus->setChecked(resume);
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

    this->coreCpuEmulator->setCurrentIndex(cpuEmulator);
    this->coreRandomizeTiming->setChecked(randomizeInterrupt);
    this->coreDebugger->setChecked(debugger);

    this->coreOverrideGameSettingsGroup->setChecked(overrideGameSettings);

    if (!this->coreOverrideGameSettingsGroup->isChecked())
    {
        disableExtraMem = false;
        counterFactor = 2;
        siDmaDuration = 2304;
    }

    this->coreMemorySize->setCurrentIndex(!disableExtraMem);
    this->coreCounterFactor->setCurrentIndex(counterFactor - 1);
    this->coreSiDmaDuration->setValue(siDmaDuration);
}

void SettingsDialog::loadDefaultGameSettings(void)
{
    RomInfo_t gameInfo;

    g_MupenApi.Core.GetDefaultRomInfo(&gameInfo);

    this->gameGoodName->setText(gameInfo.Settings.goodname);
    this->gameMemorySize->setCurrentIndex(!gameInfo.Settings.disableextramem);
    this->gameSaveType->setCurrentIndex(gameInfo.Settings.savetype);
    this->gameCounterFactor->setCurrentIndex(gameInfo.Settings.countperop - 1);
    this->gameSiDmaDuration->setValue(gameInfo.Settings.sidmaduration);
}

void SettingsDialog::loadDefaultGamePluginSettings(void)
{
    QComboBox *comboBoxArray[4] = {this->pluginVideoPlugins, this->pluginAudioPlugins, this->pluginInputPlugins,
                                   this->pluginRspPlugins};

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

void SettingsDialog::loadDefaultBehaviorSettings(void)
{
    bool pause = false, resume = false;

    /*
        pause = g_Settings.GetBoolValue(SettingsID::GUI_PauseEmulationOnFocusLoss);
        resume = g_Settings.GetBoolValue(SettingsID::GUI_ResumeEmulationOnFocus);
    */

    this->pauseEmulationOnFocusLoss->setChecked(pause);
    this->resumeEmulationOnFocus->setChecked(resume);
}

void SettingsDialog::saveSettings(void)
{
    this->saveCoreSettings();
    if (g_EmuThread->isRunning())
        this->saveGameSettings();
    this->savePluginSettings();
    this->saveDirectorySettings();
    this->saveBehaviorSettings();
}

void SettingsDialog::saveCoreSettings(void)
{
    bool disableExtraMem = (this->coreMemorySize->currentIndex() == 0);
    int counterFactor = this->coreCounterFactor->currentIndex() + 1;
    int cpuEmulator = this->coreCpuEmulator->currentIndex();
    int siDmaDuration = this->coreSiDmaDuration->value();
    bool randomizeInterrupt = this->coreRandomizeTiming->isChecked();
    bool debugger = this->coreDebugger->isChecked();
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
    M64P::Wrapper::RomInfo_t gameInfo = {0}, defaultGameInfo = {0};

    g_MupenApi.Core.GetRomInfo(&gameInfo);
    g_MupenApi.Core.GetDefaultRomInfo(&defaultGameInfo);

    QString section = QString(gameInfo.Settings.MD5);

    if (g_MupenApi.Config.SectionExists(section))
        g_MupenApi.Config.DeleteSection(section);

    bool disableExtraMem = this->gameMemorySize->currentIndex() == 0;
    int saveType = this->gameSaveType->currentIndex();
    int countPerOp = this->gameCounterFactor->currentIndex() + 1;
    int siDmaDuration = this->gameSiDmaDuration->value();

    if (defaultGameInfo.Settings.disableextramem != (unsigned char)disableExtraMem)
        g_MupenApi.Config.SetOption(section, "DisableExtraMem", disableExtraMem);
    if (defaultGameInfo.Settings.savetype != saveType)
        g_MupenApi.Config.SetOption(section, "SaveType", saveType);
    if (defaultGameInfo.Settings.countperop != countPerOp)
        g_MupenApi.Config.SetOption(section, "CountPerOp", countPerOp);
    if (defaultGameInfo.Settings.sidmaduration != siDmaDuration)
        g_MupenApi.Config.SetOption(section, "SiDmaDuration", siDmaDuration);
}

void SettingsDialog::savePluginSettings(void)
{
    QComboBox *comboBoxArray[4] = {this->videoPlugins, this->audioPlugins, this->inputPlugins, this->rspPlugins};
    QComboBox *comboBox;

    for (const Plugin_t &p : g_Plugins.GetAvailablePlugins())
    {
        comboBox = comboBoxArray[(int)p.Type];

        if (p.Name == comboBox->currentText())
            g_Plugins.ChangePlugin(p);
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

void SettingsDialog::saveBehaviorSettings(void)
{
    bool pause = false, resume = false;

    pause = this->pauseEmulationOnFocusLoss->isChecked();
    resume = this->resumeEmulationOnFocus->isChecked();
    /* TODO for someday
        g_Settings.SetValue(SettingsID::GUI_PauseEmulationOnFocusLoss, pause);
        g_Settings.SetValue(SettingsID::GUI_ResumeEmulationOnFocus, resume);
    */
}

void SettingsDialog::hideEmulationInfoText(void)
{
    QHBoxLayout *layouts[] = {this->emulationInfoLayout, this->emulationInfoLayout_2};

    for (const QHBoxLayout *layout : layouts)
    {
        for (int i = 0; i < layout->count(); i++)
        {
            QWidget *widget = layout->itemAt(i)->widget();
            widget->hide();
        }
    }
}

void SettingsDialog::chooseDirectory(QLineEdit* lineEdit)
{
    QFileDialog dialog;
    int ret;

    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);

    ret = dialog.exec();
    if (!ret)
        return;

    lineEdit->setText(dialog.directory().path());
}

void SettingsDialog::on_treeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    int topLevelCount = this->treeWidget->topLevelItemCount();
    int index = this->treeWidget->indexOfTopLevelItem(current);

    // count all children up until our item
    for (int i = 0; i < topLevelCount && i < index; i++)
    {
        index += this->treeWidget->topLevelItem(i)->childCount();
    }

    // if we're a child ourselves,
    // get index of parent and add that onto our own index
    if (index == -1)
    {
        index = this->treeWidget->indexOfTopLevelItem(current->parent());
        index += current->parent()->indexOfChild(current) + 1;
    }

    this->stackedWidget->setCurrentIndex(index);
}

void SettingsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    QPushButton *pushButton = (QPushButton *)button;
    QPushButton *resetButton = this->buttonBox->button(QDialogButtonBox::Reset);
    QPushButton *defaultButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);
    QPushButton *saveButton = this->buttonBox->button(QDialogButtonBox::Save);
    QPushButton *cancelButton = this->buttonBox->button(QDialogButtonBox::Cancel);
    QPushButton *okButton = this->buttonBox->button(QDialogButtonBox::Ok);

    if (pushButton == resetButton)
    {
        this->reloadSettings(this->stackedWidget->currentIndex());
    }
    else if (pushButton == defaultButton)
    {
        this->restoreDefaults(this->stackedWidget->currentIndex());
    }
    else if (pushButton == saveButton)
    {
        this->saveSettings();
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

