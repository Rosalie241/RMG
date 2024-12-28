/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef NETPLAYSESSIONDIALOG_HPP
#define NETPLAYSESSIONDIALOG_HPP

#include <QJsonObject>
#include <QWebSocket>
#include <QDialog>
#include <QString>
#include <QJsonArray>

#include "ui_NetplaySessionDialog.h"

namespace UserInterface
{
namespace Dialog
{
class NetplaySessionDialog : public QDialog, private Ui::NetplaySessionDialog
{
    Q_OBJECT

  public:
    NetplaySessionDialog(QWidget *parent, QWebSocket* webSocket, QJsonObject json, QString sessionFile, QJsonArray cheats);
    ~NetplaySessionDialog(void);

  private:
    QString sessionFile;
    QString nickName;
    QString sessionName;
    int sessionPort = -1;
    int sessionNumber = -1;

  	QWebSocket* webSocket;
    QJsonArray cheats; 

  private slots:
  	void on_webSocket_textMessageReceived(QString message);

    void on_chatLineEdit_textChanged(QString text);
    void on_sendPushButton_clicked(void);

  	void accept(void) Q_DECL_OVERRIDE;

  signals:
    void OnPlayGame(QString file, QString address, int port, int player, QJsonArray cheats);
};
} // namespace Dialog
} // namespace UserInterface

#endif // NETPLAYSESSIONDIALOG_HPP
