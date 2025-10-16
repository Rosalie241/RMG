/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "MainDialog.hpp"

#include <RMG-Core/Settings.hpp>

using namespace UserInterface;

MainDialog::MainDialog(QWidget* parent) : QDialog(parent)
{
    this->setupUi(this);

    this->deadZoneSlider->setValue(CoreSettingsGetIntValue(SettingsID::GCAInput_Deadzone));
    this->sensitivitySlider->setValue(CoreSettingsGetIntValue(SettingsID::GCAInput_Sensitivity));
    this->triggerTresholdSlider->setValue(CoreSettingsGetIntValue(SettingsID::GCAInput_TriggerTreshold));
    this->cButtonTresholdSlider->setValue(CoreSettingsGetIntValue(SettingsID::GCAInput_CButtonTreshold));
    this->swapZLCheckBox->setChecked(CoreSettingsGetBoolValue(SettingsID::GCAInput_SwapZL));
}

MainDialog::~MainDialog()
{
}

void MainDialog::on_buttonBox_clicked(QAbstractButton* button)
{
    QPushButton *pushButton = (QPushButton *)button;
    QPushButton *okButton = this->buttonBox->button(QDialogButtonBox::Ok);
    QPushButton *defaultButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);

    if (pushButton == okButton)
    {
        CoreSettingsSetValue(SettingsID::GCAInput_Deadzone, this->deadZoneSlider->value());
        CoreSettingsSetValue(SettingsID::GCAInput_Sensitivity, this->sensitivitySlider->value());
        CoreSettingsSetValue(SettingsID::GCAInput_TriggerTreshold, this->triggerTresholdSlider->value());
        CoreSettingsSetValue(SettingsID::GCAInput_CButtonTreshold, this->cButtonTresholdSlider->value());
        CoreSettingsSetValue(SettingsID::GCAInput_SwapZL, this->swapZLCheckBox->isChecked());
        CoreSettingsSave();
    }
    else if (pushButton == defaultButton)
    {
        this->deadZoneSlider->setValue(CoreSettingsGetDefaultIntValue(SettingsID::GCAInput_Deadzone));
        this->sensitivitySlider->setValue(CoreSettingsGetDefaultIntValue(SettingsID::GCAInput_Sensitivity));
        this->triggerTresholdSlider->setValue(CoreSettingsGetDefaultIntValue(SettingsID::GCAInput_TriggerTreshold));
        this->cButtonTresholdSlider->setValue(CoreSettingsGetDefaultIntValue(SettingsID::GCAInput_CButtonTreshold));
        this->swapZLCheckBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::GCAInput_SwapZL));
    }
}

void MainDialog::on_deadZoneSlider_valueChanged(int value)
{
    QString title;
    title = "Deadzone: ";
    title += QString::number(value);
    title += "%";

    this->deadZoneGroupBox->setTitle(title);
}

void MainDialog::on_sensitivitySlider_valueChanged(int value)
{
    QString title;
    title = "Sensitivity: ";
    title += QString::number(value);
    title += "%";

    this->sensitivityGroupBox->setTitle(title);
}

void MainDialog::on_triggerTresholdSlider_valueChanged(int value)
{
    QString title;
    title = "Trigger treshold: ";
    title += QString::number(value);
    title += "%";

    this->triggerTresholdGroupBox->setTitle(title);
}

void MainDialog::on_cButtonTresholdSlider_valueChanged(int value)
{
    QString title;
    title = "C button treshold: ";
    title += QString::number(value);
    title += "%";

    this->cButtonTresholdGroupBox->setTitle(title);
}
