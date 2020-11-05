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

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
    this->setupUi(this);

    this->treeWidget->topLevelItem(0)->setSelected(true);
    this->loadCoreSettings();
    this->loadGameSettings();
    this->loadPluginSettings();
}

SettingsDialog::~SettingsDialog(void)
{
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
        loadGameSettings();
        break;
    case 2:
        loadPluginSettings();
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

    this->coreMemorySize->setCurrentIndex(!disableExtraMem);
    this->coreCounterFactor->setCurrentIndex(counterFactor);
    this->coreCpuEmulator->setCurrentIndex(cpuEmulator);
    this->coreSiDmaDuration->setValue(siDmaDuration);
    this->coreRandomizeTiming->setChecked(randomizeInterrupt);
    this->coreDebugger->setChecked(debugger);
}

void SettingsDialog::loadGameSettings(void)
{
    if (!g_EmuThread->isRunning())
    {
        this->treeWidget->topLevelItem(1)->setDisabled(true);
        return;
    }

    M64P::Wrapper::RomInfo_t gameInfo = {0};

    g_MupenApi.Core.GetRomInfo(&gameInfo);

    this->gameGoodName->setText(gameInfo.Settings.goodname);
    this->gameMemorySize->setCurrentIndex(!gameInfo.Settings.disableextramem);
    this->gameSaveType->setCurrentIndex(gameInfo.Settings.savetype);
    this->gameCounterFactor->setCurrentIndex(gameInfo.Settings.countperop);
    this->gameSiDmaDuration->setValue(gameInfo.Settings.sidmaduration);
}

void SettingsDialog::loadPluginSettings(void)
{
}

void SettingsDialog::saveCoreSettings(void)
{
    bool disableExtraMem = (this->coreMemorySize->currentIndex() == 0);
    int counterFactor = this->coreCounterFactor->currentIndex();
    int cpuEmulator = this->coreCpuEmulator->currentIndex();
    int siDmaDuration = this->coreSiDmaDuration->value();
    bool randomizeInterrupt = this->coreRandomizeTiming->isChecked();;
    bool debugger = this->coreDebugger->isChecked();


    g_MupenApi.Config.SetOption(QString("Core"), QString("DisableExtraMem"), disableExtraMem);
    g_MupenApi.Config.SetOption(QString("Core"), QString("CountPerOp"), counterFactor);
    g_MupenApi.Config.SetOption(QString("Core"), QString("R4300Emulator"), cpuEmulator);
    g_MupenApi.Config.SetOption(QString("Core"), QString("SiDmaDuration"), siDmaDuration);
    g_MupenApi.Config.SetOption(QString("Core"), QString("RandomizeInterrupt"), randomizeInterrupt);
    g_MupenApi.Config.SetOption(QString("Core"), QString("EnableDebugger"), debugger);
}

void SettingsDialog::saveGameSettings(void)
{

}

void SettingsDialog::savePluginSettings(void)
{

}

void SettingsDialog::on_treeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    int index = this->treeWidget->indexOfTopLevelItem(current);
    this->stackedWidget->setCurrentIndex(index);
}

void SettingsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    QPushButton *pushButton = (QPushButton *)button;
    QPushButton *defaultButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);
    QPushButton *saveButton = this->buttonBox->button(QDialogButtonBox::Save);
    QPushButton *cancelButton = this->buttonBox->button(QDialogButtonBox::Cancel);
    QPushButton *okButton = this->buttonBox->button(QDialogButtonBox::Ok);

    if (pushButton == defaultButton)
    {
        this->restoreDefaults(this->stackedWidget->currentIndex());
    }
    else if (pushButton == saveButton)
    {
        this->saveCoreSettings();
    }
    else if (pushButton == cancelButton)
    {
        this->reject();
    }
    else if (pushButton == okButton)
    {
        this->saveCoreSettings();
        this->accept();
    }
}
