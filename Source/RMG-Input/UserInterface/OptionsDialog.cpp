/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "OptionsDialog.hpp"

#include <RMG-Core/Core.hpp>

using namespace UserInterface;

OptionsDialog::OptionsDialog(QWidget* parent, QString settingsSection) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
    this->setupUi(this);

    this->settingsSection = settingsSection;

    std::string section = settingsSection.toStdString();

    this->removeDuplicateMappingsCheckbox->setChecked(CoreSettingsGetBoolValue(SettingsID::Input_RemoveDuplicateMappings, section));
    this->controllerPakComboBox->setCurrentIndex(CoreSettingsGetIntValue(SettingsID::Input_Pak, section));
}

void OptionsDialog::on_buttonBox_clicked(QAbstractButton *button)
{

    QPushButton *pushButton = (QPushButton *)button;
    QPushButton *okButton = this->buttonBox->button(QDialogButtonBox::Ok);

    if (pushButton == okButton)
    {
        std::string section = this->settingsSection.toStdString();

        CoreSettingsSetValue(SettingsID::Input_RemoveDuplicateMappings, section, this->removeDuplicateMappingsCheckbox->isChecked());
        CoreSettingsSetValue(SettingsID::Input_Pak, section, this->controllerPakComboBox->currentIndex());
    }
    
    this->accept();
}
