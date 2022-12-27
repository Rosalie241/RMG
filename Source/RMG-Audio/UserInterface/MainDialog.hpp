/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QDialog>
#include <QAbstractButton>

#include "ui_MainDialog.h"

namespace UserInterface
{
class MainDialog : public QDialog, private Ui::MainDialog
{
Q_OBJECT
public:
    MainDialog(QWidget *parent);
    ~MainDialog(void);

private:
    void setIconsForEmulationInfoText(void);
    void hideEmulationInfoText(void);

private slots:
    void on_volumeSlider_valueChanged(int value);

    void on_buttonBox_clicked(QAbstractButton *);
};
}

#endif // MAINWINDOW_HPP