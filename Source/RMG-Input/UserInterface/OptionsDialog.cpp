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

#include <QFileDialog>
#include <QMessageBox>
#include <RMG-Core/Core.hpp>

#include <SDL.h>

using namespace UserInterface;

OptionsDialog::OptionsDialog(QWidget* parent, OptionsDialogSettings settings,
                             SDL_Joystick* joystick, SDL_GameController* controller) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
    this->setupUi(this);

    this->setIconsForEmulationInfoText();

    this->controllerPakComboBox->setCurrentIndex(settings.ControllerPak);
    this->gameboyRomLineEdit->setText(QString::fromStdString(settings.GameboyRom));
    this->gameboySaveLineEdit->setText(QString::fromStdString(settings.GameboySave));
    this->removeDuplicateMappingsCheckbox->setChecked(settings.RemoveDuplicateMappings);
    this->filterEventsForButtonsCheckBox->setChecked(settings.FilterEventsForButtons);
    this->filterEventsForAxisCheckBox->setChecked(settings.FilterEventsForAxis);

    // global settings
    this->sdlControllerModeComboBox->setCurrentIndex(CoreSettingsGetIntValue(SettingsID::Input_ControllerMode));

    if (!CoreIsEmulationRunning() && !CoreIsEmulationPaused())
    {
        this->hideEmulationInfoText();
    }

    this->currentJoystick   = joystick;
    this->currentController = controller;

    this->testRumbleButton->setVisible(settings.ControllerPak == 1);
    this->testRumbleButton->setEnabled(this->currentJoystick != nullptr || this->currentController != nullptr);
}

OptionsDialogSettings OptionsDialog::GetSettings()
{
    return this->settings;
}

void OptionsDialog::setIconsForEmulationInfoText(void)
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

void OptionsDialog::hideEmulationInfoText(void)
{
    QHBoxLayout *layouts[] = {this->emulationInfoLayout_1};

    for (const auto &layout : layouts)
    {
        for (int i = 0; i < layout->count(); i++)
        {
            QWidget *widget = layout->itemAt(i)->widget();
            widget->hide();
        }
    }
}

void OptionsDialog::accept()
{
    this->settings.ControllerPak = this->controllerPakComboBox->currentIndex();
    this->settings.GameboyRom = this->gameboyRomLineEdit->text().toStdString();
    this->settings.GameboySave = this->gameboySaveLineEdit->text().toStdString();
    this->settings.RemoveDuplicateMappings = this->removeDuplicateMappingsCheckbox->isChecked();
    this->settings.FilterEventsForButtons = this->filterEventsForButtonsCheckBox->isChecked();
    this->settings.FilterEventsForAxis = this->filterEventsForAxisCheckBox->isChecked();

    // save global settings now
    CoreSettingsSetValue(SettingsID::Input_ControllerMode, this->sdlControllerModeComboBox->currentIndex());

    QDialog::accept();
}

void OptionsDialog::on_controllerPakComboBox_currentIndexChanged(int index)
{
    this->testRumbleButton->setVisible(index == 1);
}

void OptionsDialog::on_changeGameboyRomButton_clicked()
{
    QString gameBoyRom;
    gameBoyRom = QFileDialog::getOpenFileName(this, "", "", "Gameboy ROM (*.gb *.gbc)");

    if (!gameBoyRom.isEmpty())
    {
        this->gameboyRomLineEdit->setText(QDir::toNativeSeparators(gameBoyRom));
    }
}

void OptionsDialog::on_changeGameboySaveButton_clicked()
{
    QString gameBoySave;
    gameBoySave = QFileDialog::getOpenFileName(this, "", "", "Gameboy save (*.sav *.ram)");

    if (!gameBoySave.isEmpty())
    {
        this->gameboySaveLineEdit->setText(QDir::toNativeSeparators(gameBoySave));
    }
}

void OptionsDialog::on_testRumbleButton_clicked()
{
#if SDL_VERSION_ATLEAST(2,0,18)
    if ((this->currentJoystick != nullptr   && SDL_JoystickHasRumble(this->currentJoystick) != SDL_TRUE) ||
        (this->currentController != nullptr && SDL_GameControllerHasRumble(this->currentController) != SDL_TRUE))
    {
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.setWindowTitle("Error");
        msgBox.setText("Controller doesn't support rumble");
        msgBox.addButton(QMessageBox::Ok);
        msgBox.exec();
        return;
    }
#endif

    if (this->currentJoystick != nullptr)
    {
        SDL_JoystickRumble(this->currentJoystick, 0xFFFF, 0xFFFF, 1500);
    }
    else
    {
        SDL_GameControllerRumble(this->currentController, 0xFFFF, 0xFFFF, 1500);
    }
}
