/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef NETPLAYSESSIONPASSWORDDIALOG_HPP
#define NETPLAYSESSIONPASSWORDDIALOG_HPP

#include <QDialog>
#include <QString>

#include "ui_NetplaySessionPasswordDialog.h"

namespace UserInterface
{
namespace Dialog
{
class NetplaySessionPasswordDialog : public QDialog, private Ui::NetplaySessionPasswordDialog
{
    Q_OBJECT

  public:
    NetplaySessionPasswordDialog(QWidget *parent);
    ~NetplaySessionPasswordDialog(void);

    QString GetPassword(void);

  private:
    QString password;

  private slots:
    void accept(void) Q_DECL_OVERRIDE;
};
} // namespace Dialog
} // namespace UserInterface

#endif // NETPLAYSESSIONPASSWORDDIALOG_HPP
