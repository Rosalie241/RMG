/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef OPTIONSDIALOG_HPP
#define OPTIONSDIALOG_HPP

#include <QDialog>
#include <string>

#include <SDL3/SDL.h>

#include "ui_OptionsDialog.h"

struct OptionsDialogSettings
{
    // controller settings
    int     ControllerPak = 0;
    std::string GameboyRom;
    std::string GameboySave;

    // UI settings
    bool RemoveDuplicateMappings = false;
    bool FilterEventsForButtons  = true;
    bool FilterEventsForAxis     = true;
};

namespace UserInterface
{
class OptionsDialog : public QDialog, private Ui::OptionsDialog
{
Q_OBJECT

public:
    OptionsDialog(QWidget *parent, OptionsDialogSettings settings, 
                  SDL_Joystick* joystick, SDL_Gamepad* gamepad);

    OptionsDialogSettings GetSettings();

protected:
    void accept() Q_DECL_OVERRIDE;

private:
    OptionsDialogSettings settings;

    SDL_Joystick* currentJoystick = nullptr;
    SDL_Gamepad* currentGamepad = nullptr;

    void setIconsForEmulationInfoText(void);
    void hideEmulationInfoText(void);

private slots:
    void on_controllerPakComboBox_currentIndexChanged(int index);

    void on_changeGameboyRomButton_clicked();
    void on_clearGameboyRomButton_clicked();

    void on_changeGameboySaveButton_clicked();
    void on_clearGameboySaveButton_clicked();

    void on_testRumbleButton_clicked();
};
}

#endif // OPTIONSDIALOG_HPP
