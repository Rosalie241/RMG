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

    // TODO
    if (!g_EmuThread->isRunning())
    {
        this->treeWidget->topLevelItem(0)->setSelected(true);
        this->treeWidget->topLevelItem(1)->setDisabled(true);
    }
    else
    {
        this->treeWidget->topLevelItem(1)->setSelected(true);
        M64P::Wrapper::RomInfo_t info;

        g_MupenApi.Core.GetRomInfo(&info);

        this->lineEdit->setText(info.Settings.goodname);
        this->comboBox->setCurrentIndex(info.Settings.disableextramem == 1 ? 0 : 1);
        this->comboBox_2->setCurrentIndex(info.Settings.savetype);
        this->comboBox_3->setCurrentIndex(info.Settings.countperop);
    }   


    this->setFixedSize(this->size());
}

SettingsDialog::~SettingsDialog(void)
{
}
