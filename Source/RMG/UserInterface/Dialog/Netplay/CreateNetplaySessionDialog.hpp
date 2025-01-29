/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CREATENETPLAYSESSIONDIALOG_HPP
#define CREATENETPLAYSESSIONDIALOG_HPP

#include <QNetworkReply>
#include <QJsonObject>
#include <QTimerEvent>
#include <QWebSocket>
#include <QUdpSocket>
#include <QDialog>
#include <QString>

#include "ui_CreateNetplaySessionDialog.h"

#include <RMG-Core/Core.hpp>

namespace UserInterface
{
namespace Dialog
{
class CreateNetplaySessionDialog : public QDialog, private Ui::CreateNetplaySessionDialog
{
    Q_OBJECT

  public:
    CreateNetplaySessionDialog(QWidget *parent, QWebSocket* webSocket, QMap<QString, CoreRomSettings> data);
    ~CreateNetplaySessionDialog(void);

    QJsonObject GetSessionJson(void);
    QString     GetSessionFile(void);

  private:
  	QWebSocket* webSocket;
    QUdpSocket broadcastSocket;

    int pingTimerId = -1;

  	QJsonObject sessionJson;
    QString sessionFile;

    QString getGameName(QString goodName, QString file);

    bool validate(void);
    void validateCreateButton(void);

  protected:
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;

  private slots:
  	void on_webSocket_textMessageReceived(QString message);
    void on_webSocket_pong(quint64 elapsedTime, const QByteArray&);
    void on_broadcastSocket_readyRead(void);
    void on_networkAccessManager_Finished(QNetworkReply* reply);

    void on_serverComboBox_currentIndexChanged(int index);

    void on_nickNameLineEdit_textChanged(void);
    void on_sessionNameLineEdit_textChanged(void);
    void on_passwordLineEdit_textChanged(void);

    void on_romListWidget_OnRomChanged(bool valid);

  	void accept(void) Q_DECL_OVERRIDE;
};
} // namespace Dialog
} // namespace UserInterface

#endif // CREATENETPLAYSESSIONDIALOG_HPP
