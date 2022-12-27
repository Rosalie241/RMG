/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
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

#include "ui_OptionsDialog.h"

struct OptionsDialogSettings
{
    // controller settings
    int     ControllerPak = 0;
    std::string GameboyRom;
    std::string GameboySave;

    // UI settings
    bool RemoveDuplicateMappings = false;
    bool InvertAxis = false;
};

namespace UserInterface
{
class OptionsDialog : public QDialog, private Ui::OptionsDialog
{
Q_OBJECT

public:
    OptionsDialog(QWidget *parent, OptionsDialogSettings settings);

    bool HasSaved();
    OptionsDialogSettings GetSettings();

private:
    bool hasSaved = false;
    OptionsDialogSettings settings;

    void setIconsForEmulationInfoText(void);
    void hideEmulationInfoText(void);

private slots:
    void on_buttonBox_clicked(QAbstractButton *);

    void on_changeGameboyRomButton_clicked();
    void on_changeGameboySaveButton_clicked();
};
}

#endif // OPTIONSDIALOG_HPP
