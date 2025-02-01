/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef DOWNLOADUPDATEDIALOG_HPP
#define DOWNLOADUPDATEDIALOG_HPP

#include <QWidget>
#include <QDialog>
#include <QJsonObject>
#include <QNetworkReply>

#include "ui_DownloadUpdateDialog.h"

namespace UserInterface
{
namespace Dialog
{
class DownloadUpdateDialog : public QDialog, private Ui::DownloadUpdateDialog
{
    Q_OBJECT

  private:
    QNetworkReply *reply;
    QString temporaryDirectory;
    QString filename;

  public:
    DownloadUpdateDialog(QWidget *parent, QUrl url, QString filename);
    ~DownloadUpdateDialog(void);

    QString GetTempDirectory(void);
    QString GetFileName(void);

  private slots:
    void on_reply_downloadProgress(qint64 size, qint64 total);
    void on_reply_finished(void);
};
} // namespace Dialog
} // namespace UserInterface

#endif // DOWNLOADUPDATEDIALOG_HPP