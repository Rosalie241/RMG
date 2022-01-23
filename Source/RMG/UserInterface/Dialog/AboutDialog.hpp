/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef ABOUTDIALOG_HPP
#define ABOUTDIALOG_HPP

#include <QDialog>

#include "ui_AboutDialog.h"

namespace UserInterface
{
namespace Dialog
{
class AboutDialog : public QDialog, private Ui::AboutDialog
{
    Q_OBJECT

  private slots:
    void on_okButton_clicked(void);

  public:
    AboutDialog();
};
} // namespace Dialog
} // namespace UserInterface

#endif // ABOUTDIALOG_HPP 
