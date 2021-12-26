/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "MainDialog.hpp"
#include <RMG-Core/Core.hpp>

using namespace UserInterface;

MainDialog::MainDialog(QWidget* parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
    this->setupUi(this);

    this->volumeSlider->setValue(CoreSettingsGetIntValue(SettingsID::Audio_Volume));
    this->mutedCheckbox->setChecked(CoreSettingsGetBoolValue(SettingsID::Audio_Muted));
}

MainDialog::~MainDialog()
{
}

void MainDialog::on_volumeSlider_valueChanged(int value)
{
    this->volumeLabel->setText(QString::number(value) + "%");
}

void MainDialog::on_buttonBox_clicked(QAbstractButton* button)
{
    QPushButton *pushButton = (QPushButton *)button;
    QPushButton *okButton = this->buttonBox->button(QDialogButtonBox::Ok);

    int volume = this->volumeSlider->value();
    bool muted = this->mutedCheckbox->isChecked();

    if (pushButton == okButton)
    {
        CoreSettingsSetValue(SettingsID::Audio_Volume, volume);
        CoreSettingsSetValue(SettingsID::Audio_Muted, muted);
        CoreSettingsSave();
    }
}
