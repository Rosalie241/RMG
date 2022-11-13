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

#include "ui_OptionsDialog.h"

namespace UserInterface
{
class OptionsDialog : public QDialog, private Ui::OptionsDialog
{
Q_OBJECT

public:
    OptionsDialog(QWidget *parent, QString mainSettingsSection, QString settingsSection);

private:
    QString settingsSection;

private slots:
    void on_buttonBox_clicked(QAbstractButton *);
};
}

#endif // OPTIONSDIALOG_HPP
