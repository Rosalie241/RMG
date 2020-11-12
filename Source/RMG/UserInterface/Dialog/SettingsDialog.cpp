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

using namespace UserInterface::Dialog;
using namespace M64P::Wrapper;

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
    this->setupUi(this);

    this->treeWidget->topLevelItem(0)->setSelected(true);
    this->treeWidget->expandAll();

    for (int i = 0; i < this->stackedWidget->count(); i++)
        this->reloadSettings(i);

    int width = 0, height = 0;
    g_MupenApi.Config.GetOption(SETTINGS_SECTION, "Settings Window Width", &width);
    g_MupenApi.Config.GetOption(SETTINGS_SECTION, "Settings Window Height", &height);

    if (width != 0 && height != 0)
    {
        // center current dialog
        this->setGeometry(
            QStyle::alignedRect( 
                Qt::LeftToRight,
                Qt::AlignCenter,
                QSize(width, height),
                parent->geometry()
            )
        );
    }
}

SettingsDialog::~SettingsDialog(void)
{
    g_MupenApi.Config.SetOption(SETTINGS_SECTION, "Settings Window Width",
                                this->size().width());
    g_MupenApi.Config.SetOption(SETTINGS_SECTION, "Settings Window Height",
                                this->size().height());
}

void SettingsDialog::restoreDefaults(int stackedWidgetIndex)
{
    switch (stackedWidgetIndex)
    {
    default:
    case 0:
        loadCoreSettings();
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

    g_MupenApi.Config.GetOption(QString("Core"), QString("DisableExtraMem"), &disableExtraMem);
    g_MupenApi.Config.GetOption(QString("Core"), QString("CountPerOp"), &counterFactor);
    g_MupenApi.Config.GetOption(QString("Core"), QString("R4300Emulator"), &cpuEmulator);
    g_MupenApi.Config.GetOption(QString("Core"), QString("SiDmaDuration"), &siDmaDuration);
    g_MupenApi.Config.GetOption(QString("Core"), QString("RandomizeInterrupt"), &randomizeInterrupt);
    g_MupenApi.Config.GetOption(QString("Core"), QString("EnableDebugger"), &debugger);

    this->coreCpuEmulator->setCurrentIndex(cpuEmulator);
    this->coreRandomizeTiming->setChecked(randomizeInterrupt);
    this->coreDebugger->setChecked(debugger);

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
    QList<Plugin_t> pluginList;
    QComboBox *comboBoxArray[4] = {this->videoPlugins, this->audioPlugins, this->inputPlugins, this->rspPlugins};
    PluginType type;
    QComboBox *comboBox;
    Plugin_t plugin_t;

    for (int i = 0; i < 4; i++)
    {
        type = (PluginType)i;
        comboBox = comboBoxArray[i];
        comboBox->clear();

        pluginList.clear();
        pluginList = g_MupenApi.Core.GetPlugins((PluginType)i);

        int pluginIndex = -1, stopCounting = 0;
        for (Plugin_t &p : pluginList)
        {
            comboBox->addItem(p.Name);

            if (stopCounting == 1)
                continue;

            pluginIndex++;

            if (g_MupenApi.Core.GetCurrentPlugin(type, &plugin_t) && plugin_t.FileName == p.FileName)
            {
                stopCounting = 1;
            }
        }

        comboBox->setCurrentIndex(pluginIndex);
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

    g_MupenApi.Config.GetOption("Core", "ScreenshotPath", &screenshotDir);
    g_MupenApi.Config.GetOption("Core", "SaveStatePath", &saveStateDir);
    g_MupenApi.Config.GetOption("Core", "SaveSRAMPath", &saveSramDir);
    g_MupenApi.Config.GetOption("Core", "SharedDataPath", &sharedDataDir);

    g_MupenApi.Config.GetOption(SETTINGS_SECTION, "CoreOverrideUserDirectories", &overrideUserDirs);
    g_MupenApi.Config.GetOption(SETTINGS_SECTION, "CoreUserDataDirectory", &userDataDir);
    g_MupenApi.Config.GetOption(SETTINGS_SECTION, "CoreUserCacheDirectory", &userCacheDir);

    this->screenshotDirectory->setText(screenshotDir);
    this->saveStateDirectory->setText(saveStateDir);
    this->saveSramDirectory->setText(saveSramDir);
    this->sharedDataDirectory->setText(sharedDataDir);
    this->overrideUserDirectories->setChecked(overrideUserDirs);
    this->userDataDirectory->setText(userDataDir);
    this->userCacheDirectory->setText(userCacheDir);
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
    this->screenshotDirectory->setText("Screenshots");
    this->saveStateDirectory->setText("Save/State");
    this->saveSramDirectory->setText("Save/Game");
    this->sharedDataDirectory->setText("Data");
    this->overrideUserDirectories->setChecked(true);
    this->userDataDirectory->setText("Data");
    this->userCacheDirectory->setText("Cache");
}

void SettingsDialog::saveSettings(void)
{
    this->saveCoreSettings();
    if (g_EmuThread->isRunning())
        this->saveGameSettings();
    this->savePluginSettings();
    this->saveDirectorySettings();
}

void SettingsDialog::saveCoreSettings(void)
{
    bool disableExtraMem = (this->coreMemorySize->currentIndex() == 0);
    int counterFactor = this->coreCounterFactor->currentIndex();
    int cpuEmulator = this->coreCpuEmulator->currentIndex();
    int siDmaDuration = this->coreSiDmaDuration->value();
    bool randomizeInterrupt = this->coreRandomizeTiming->isChecked();
    bool debugger = this->coreDebugger->isChecked();

    g_MupenApi.Config.SetOption(QString("Core"), QString("R4300Emulator"), cpuEmulator);
    g_MupenApi.Config.SetOption(QString("Core"), QString("RandomizeInterrupt"), randomizeInterrupt);
    g_MupenApi.Config.SetOption(QString("Core"), QString("EnableDebugger"), debugger);

    if (!this->coreOverrideGameSettingsGroup->isChecked())
    {
        disableExtraMem = false;
        counterFactor = 0;
        siDmaDuration = -1;
    }

    g_MupenApi.Config.SetOption(QString("Core"), QString("DisableExtraMem"), disableExtraMem);
    g_MupenApi.Config.SetOption(QString("Core"), QString("CountPerOp"), counterFactor);
    g_MupenApi.Config.SetOption(QString("Core"), QString("SiDmaDuration"), siDmaDuration);
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
    QList<Plugin_t> pluginList;
    QComboBox *comboBoxArray[4] = {this->videoPlugins, this->audioPlugins, this->inputPlugins, this->rspPlugins};
    QString settingsArray[4] = {SETTINGS_PLUGIN_GFX, SETTINGS_PLUGIN_AUDIO, SETTINGS_PLUGIN_INPUT, SETTINGS_PLUGIN_RSP};
    PluginType type;
    QComboBox *comboBox;

    for (int i = 0; i < 4; i++)
    {
        type = (PluginType)i;
        comboBox = comboBoxArray[i];

        pluginList.clear();
        pluginList = g_MupenApi.Core.GetPlugins((PluginType)i);

        for (Plugin_t &p : pluginList)
        {
            if (p.Name == comboBox->currentText())
            {
                g_MupenApi.Config.SetOption(SETTINGS_SECTION, settingsArray[i], p.FileName);
                g_MupenApi.Core.SetPlugin(p);
                break;
            }
        }
    }
}

void SettingsDialog::saveDirectorySettings(void)
{
    g_MupenApi.Config.SetOption("Core", "ScreenshotPath", this->screenshotDirectory->text());
    g_MupenApi.Config.SetOption("Core", "SaveStatePath", this->saveStateDirectory->text());
    g_MupenApi.Config.SetOption("Core", "SaveSRAMPath", this->saveSramDirectory->text());
    g_MupenApi.Config.SetOption("Core", "SharedDataPath", this->sharedDataDirectory->text());

    g_MupenApi.Config.SetOption(SETTINGS_SECTION, "CoreOverrideUserDirectories", this->overrideUserDirectories->isChecked());
    g_MupenApi.Config.SetOption(SETTINGS_SECTION, "CoreUserDataDirectory", this->userDataDirectory->text());
    g_MupenApi.Config.SetOption(SETTINGS_SECTION, "CoreUserCacheDirectory", this->userCacheDirectory->text());
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
