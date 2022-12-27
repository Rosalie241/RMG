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

MainDialog::MainDialog(QWidget* parent) : QDialog(parent)
{
    this->setupUi(this);
    this->setWindowIcon(QIcon(":Resource/RMG.png"));

    this->setIconsForEmulationInfoText();

    this->volumeSlider->setValue(CoreSettingsGetIntValue(SettingsID::Audio_Volume));
    this->muteCheckBox->setChecked(CoreSettingsGetBoolValue(SettingsID::Audio_Muted));

    this->defaultFrequencySpinBox->setValue(CoreSettingsGetIntValue(SettingsID::Audio_DefaultFrequency));
    this->primaryBufferSizeSpinBox->setValue(CoreSettingsGetIntValue(SettingsID::Audio_PrimaryBufferSize));
    this->primaryBufferTargetSpinBox->setValue(CoreSettingsGetIntValue(SettingsID::Audio_PrimaryBufferTarget));
    this->secondaryBufferSizeSpinBox->setValue(CoreSettingsGetIntValue(SettingsID::Audio_SecondaryBufferSize));
    this->resamplerComboBox->setCurrentText(QString::fromStdString(CoreSettingsGetStringValue(SettingsID::Audio_Resampler)));
    this->swapChannelsCheckBox->setChecked(CoreSettingsGetBoolValue(SettingsID::Audio_SwapChannels));
    this->synchronizeAudioCheckBox->setChecked(CoreSettingsGetBoolValue(SettingsID::Audio_Synchronize));

    if (!CoreIsEmulationRunning() && !CoreIsEmulationPaused())
    {
        this->hideEmulationInfoText();
    }
}

MainDialog::~MainDialog()
{
}

void MainDialog::setIconsForEmulationInfoText(void)
{
    QLabel* labels[] = {
        this->infoIconLabel_0
    };

    QIcon infoIcon = QIcon::fromTheme("information-line");
    QPixmap infoIconPixmap = infoIcon.pixmap(16, 16);
    
    for (QLabel* label : labels)
    {
        label->setPixmap(infoIconPixmap);
    }
}

void MainDialog::hideEmulationInfoText(void)
{
    QHBoxLayout *layouts[] = {this->emulationInfoLayout};

    for (const auto &layout : layouts)
    {
        for (int i = 0; i < layout->count(); i++)
        {
            QWidget *widget = layout->itemAt(i)->widget();
            widget->hide();
        }
    }
}

void MainDialog::on_volumeSlider_valueChanged(int value)
{
    this->volumeLabel->setText(QString::number(value) + "%");
}

void MainDialog::on_buttonBox_clicked(QAbstractButton* button)
{
    QPushButton *pushButton = (QPushButton *)button;
    QPushButton *okButton = this->buttonBox->button(QDialogButtonBox::Ok);
    QPushButton *defaultButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);

    if (pushButton == okButton)
    {
        CoreSettingsSetValue(SettingsID::Audio_Volume, this->volumeSlider->value());
        CoreSettingsSetValue(SettingsID::Audio_Muted, this->muteCheckBox->isChecked());
    
        CoreSettingsSetValue(SettingsID::Audio_DefaultFrequency, this->defaultFrequencySpinBox->value());
        CoreSettingsSetValue(SettingsID::Audio_PrimaryBufferSize, this->primaryBufferSizeSpinBox->value());
        CoreSettingsSetValue(SettingsID::Audio_PrimaryBufferTarget, this->primaryBufferTargetSpinBox->value());
        CoreSettingsSetValue(SettingsID::Audio_SecondaryBufferSize, this->secondaryBufferSizeSpinBox->value());
        CoreSettingsSetValue(SettingsID::Audio_Resampler, this->resamplerComboBox->currentText().toStdString());
        CoreSettingsSetValue(SettingsID::Audio_SwapChannels, this->swapChannelsCheckBox->isChecked());
        CoreSettingsSetValue(SettingsID::Audio_Synchronize, this->synchronizeAudioCheckBox->isChecked());
        CoreSettingsSave();
    }
    else if (pushButton == defaultButton)
    {
        if (this->tabWidget->currentIndex() == 0)
        { // volume settings
            this->volumeSlider->setValue(CoreSettingsGetDefaultIntValue(SettingsID::Audio_Volume));
            this->muteCheckBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::Audio_Muted));
        }
        else
        { // advanced settings
            this->defaultFrequencySpinBox->setValue(CoreSettingsGetDefaultIntValue(SettingsID::Audio_DefaultFrequency));
            this->primaryBufferSizeSpinBox->setValue(CoreSettingsGetDefaultIntValue(SettingsID::Audio_PrimaryBufferSize));
            this->primaryBufferTargetSpinBox->setValue(CoreSettingsGetDefaultIntValue(SettingsID::Audio_PrimaryBufferTarget));
            this->secondaryBufferSizeSpinBox->setValue(CoreSettingsGetDefaultIntValue(SettingsID::Audio_SecondaryBufferSize));
            this->resamplerComboBox->setCurrentText(QString::fromStdString(CoreSettingsGetDefaultStringValue(SettingsID::Audio_Resampler)));
            this->swapChannelsCheckBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::Audio_SwapChannels));
            this->synchronizeAudioCheckBox->setChecked(CoreSettingsGetDefaultBoolValue(SettingsID::Audio_Synchronize));
        }
    }
}
