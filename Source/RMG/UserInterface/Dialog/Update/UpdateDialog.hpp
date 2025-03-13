/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef UPDATEDIALOG_HPP
#define UPDATEDIALOG_HPP

#include <QWidget>
#include <QDialog>
#include <QJsonObject>
#include <QNetworkReply>

#include "ui_UpdateDialog.h"

namespace UserInterface
{
namespace Dialog
{
class UpdateDialog : public QDialog, private Ui::UpdateDialog
{
    Q_OBJECT

  private:
    QJsonObject jsonObject;
    QString filename;
    QUrl url;
#ifdef _WIN32
    bool isWin32Setup = false;
#endif // _WIN32

  public:
    UpdateDialog(QWidget *parent, QJsonObject jsonObject, bool forced);
    ~UpdateDialog(void);

    QString GetFileName(void);
    QUrl GetUrl(void);

  private slots:
    void on_disableUpdateCheckCheckBox_stateChanged(int state);

    void accept(void) Q_DECL_OVERRIDE;
};
} // namespace Dialog
} // namespace UserInterface

#endif // UPDATEDIALOG_HPP