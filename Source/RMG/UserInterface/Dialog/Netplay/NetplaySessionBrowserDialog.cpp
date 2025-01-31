/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "NetplaySessionBrowserDialog.hpp"
#include "NetplaySessionPasswordDialog.hpp"
#include "NetplayCommon.hpp"
#include "Utilities/QtMessageBox.hpp"

#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QNetworkDatagram>
#include <QJsonDocument>
#include <QInputDialog>
#include <QPushButton>
#include <QFileDialog>
#include <QJsonObject>
#include <QJsonArray>

#include <RMG-Core/Core.hpp>

using namespace UserInterface::Dialog;
using namespace Utilities;

//
// Exported Functions
//

NetplaySessionBrowserDialog::NetplaySessionBrowserDialog(QWidget *parent, QWebSocket* webSocket, QMap<QString, CoreRomSettings> modelData) : QDialog(parent)
{
    this->setupUi(this);

    // prepare web socket
    this->webSocket = webSocket;
    connect(this->webSocket, &QWebSocket::connected, this, &NetplaySessionBrowserDialog::on_webSocket_connected);
    connect(this->webSocket, &QWebSocket::disconnected, this, &NetplaySessionBrowserDialog::on_webSocket_disconnected);
    connect(this->webSocket, &QWebSocket::textMessageReceived, this, &NetplaySessionBrowserDialog::on_webSocket_textMessageReceived);
    connect(this->webSocket, &QWebSocket::pong, this, &NetplaySessionBrowserDialog::on_webSocket_pong);

    // copy rom data for later
    this->romData = modelData;

    // prepare broadcast
    broadcastSocket.bind(QHostAddress(QHostAddress::AnyIPv4), 0);
    connect(&this->broadcastSocket, &QUdpSocket::readyRead, this, &NetplaySessionBrowserDialog::on_broadcastSocket_readyRead);
    QByteArray multirequest;
    multirequest.append(1);
    broadcastSocket.writeDatagram(multirequest, QHostAddress::Broadcast, 45000);

    // change ok button name
    QPushButton* joinButton = this->buttonBox->button(QDialogButtonBox::Ok);
    joinButton->setText("Join");
    joinButton->setEnabled(false);

    // change restore defaults button name
    QPushButton* refreshButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);
    refreshButton->setText(tr("Refresh"));
    refreshButton->setIcon(QIcon::fromTheme("refresh-line"));
    refreshButton->setEnabled(false);

    // set validator for nickname
    QRegularExpression re(NETPLAYCOMMON_NICKNAME_REGEX);
    this->nickNameLineEdit->setValidator(new QRegularExpressionValidator(re, this));
    this->nickNameLineEdit->setText(QString::fromStdString(CoreSettingsGetStringValue(SettingsID::Netplay_Nickname)));

    // request server list
    QString serverUrl = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::Netplay_ServerJsonUrl));
    if (!serverUrl.isEmpty() && QUrl(serverUrl).isValid())
    {
        QNetworkAccessManager* networkAccessManager = new QNetworkAccessManager(this);
        connect(networkAccessManager, &QNetworkAccessManager::finished, this, &NetplaySessionBrowserDialog::on_networkAccessManager_Finished);
        networkAccessManager->setTransferTimeout(15000);
        networkAccessManager->get(QNetworkRequest(QUrl(serverUrl)));
    }

    this->validateJoinButton();

    this->pingTimerId = this->startTimer(2000);
}

NetplaySessionBrowserDialog::~NetplaySessionBrowserDialog(void)
{
    QString nickname = this->nickNameLineEdit->text();
    if (!nickname.isEmpty())
    {
        CoreSettingsSetValue(SettingsID::Netplay_Nickname, nickname.toStdString());
    }

    QString server = this->serverComboBox->currentText();
    if (!server.isEmpty())
    {
        CoreSettingsSetValue(SettingsID::Netplay_SelectedServer, server.toStdString());
    }
}

QJsonObject NetplaySessionBrowserDialog::GetSessionJson(void)
{
    return this->sessionJson;
}

QString NetplaySessionBrowserDialog::GetSessionFile(void)
{
    return this->sessionFile;
}

QString NetplaySessionBrowserDialog::showROMDialog(QString name, QString md5)
{
    QString title = "Open " + name;
    QString file = QFileDialog::getOpenFileName(this, title, "", "N64 ROMs (*.n64 *.z64 *.v64 *.zip *.7z)");
    CoreRomSettings romSettings;

    if (!file.isEmpty())
    {
        if (!CoreOpenRom(file.toStdU32String()) ||
            !CoreGetCurrentRomSettings(romSettings))
        {
            CoreCloseRom();
            return "";
        }

        CoreCloseRom();

        if (md5.toStdString() != romSettings.MD5)
        {
            QString details = "Expected MD5: " + md5 + "\n";
            details        += "Received MD5: " + QString::fromStdString(romSettings.MD5);
            QtMessageBox::Error(this, "Incorrect ROM Selected", details);
            return "";
        }
    }

    return file;
}

bool NetplaySessionBrowserDialog::validate(void)
{
    if (this->nickNameLineEdit->text().isEmpty() ||
        this->nickNameLineEdit->text().contains(' ') ||
        this->nickNameLineEdit->text().size() > 128)
    {
        return false;
    }

    if (!this->sessionBrowserWidget->IsCurrentSessionValid() ||
        this->serverComboBox->count() == 0)
    {
        return false;
    }

    return true;
}

void NetplaySessionBrowserDialog::validateJoinButton(void)
{
    QPushButton* joinButton = this->buttonBox->button(QDialogButtonBox::Ok);
    joinButton->setEnabled(this->validate());
}

void NetplaySessionBrowserDialog::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == this->pingTimerId)
    {
        if (this->webSocket->isValid())
        {
            this->webSocket->ping();
        }
    }
}

void NetplaySessionBrowserDialog::on_webSocket_connected(void)
{
    if (!this->webSocket->isValid())
    {
        QtMessageBox::Error(this, "Server Error", "Connection Failed");
        return;
    }

    // disable refresh button while refreshing
    QPushButton* refreshButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);
    refreshButton->setEnabled(false);

    // disable join button while refreshing
    QPushButton* joinButton = this->buttonBox->button(QDialogButtonBox::Ok);
    joinButton->setEnabled(false);

    // clear sessions
    this->sessionBrowserWidget->StartRefresh();

    // request session list from server
    QJsonObject json;
    json.insert("type", "request_get_rooms");
    NetplayCommon::AddCommonJson(json);

    this->webSocket->sendTextMessage(QJsonDocument(json).toJson());
}

void NetplaySessionBrowserDialog::on_webSocket_textMessageReceived(QString message)
{
    QJsonDocument jsonDocument = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject json = jsonDocument.object();

    QString type = json.value("type").toString();

    if (type == "reply_get_rooms")
    {
        if (json.value("accept").toInt() == 0)
        {
            QJsonArray  sessions = json.value("rooms").toArray();
            QJsonObject session;
            for (int i = 0; i < sessions.size(); i++)
            {
                session = sessions.at(i).toObject();

                this->sessionBrowserWidget->AddSessionData(session.value("room_name").toString(), 
                                                            session.value("game_name").toString(), 
                                                            session.value("MD5").toString(), 
                                                            session.value("protected").toBool(),
                                                            session.value("port").toInt(),
                                                            session.value("features").toObject().value("cpu_emulator").toString(),
                                                            session.value("features").toObject().value("rsp_plugin").toString(),
                                                            session.value("features").toObject().value("gfx_plugin").toString());
            }

            // we're done refreshing the sessions
            this->sessionBrowserWidget->RefreshDone();
            // enable refresh button when refreshing is done
            QPushButton* refreshButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);
            refreshButton->setEnabled(true);
            // re-validate join button after refresh
            this->validateJoinButton();
        }
        else
        {
            this->sessionBrowserWidget->Reset();
            QtMessageBox::Error(this, "Server Error", json.value("message").toString());
        }
    }
    else if (type == "reply_join_room")
    {
        if (json.value("accept").toInt() == 0)
        {
            this->sessionJson = json;
            QDialog::accept();
        }
        else
        {
            QtMessageBox::Error(this, "Server Error", json.value("message").toString());
            this->validateJoinButton();
        }
    }
}

void NetplaySessionBrowserDialog::on_webSocket_pong(quint64 elapsedTime, const QByteArray&)
{
    this->pingLineEdit->setText(QString::number(elapsedTime) + " ms");
}

void NetplaySessionBrowserDialog::on_webSocket_disconnected()
{
    this->sessionBrowserWidget->Reset();
}

void NetplaySessionBrowserDialog::on_broadcastSocket_readyRead(void)
{
    while (this->broadcastSocket.hasPendingDatagrams())
    {
        QNetworkDatagram datagram = this->broadcastSocket.receiveDatagram();
        QByteArray incomingData = datagram.data();
        QJsonDocument json_doc  = QJsonDocument::fromJson(incomingData);
        QJsonObject json        = json_doc.object();
        QStringList servers     = json.keys();

        for (int i = 0; i < servers.size(); i++)
        {
            this->serverComboBox->addItem(servers.at(i), json.value(servers.at(i)).toString());
        }
    }

    NetplayCommon::RestoreSelectedServer(this->serverComboBox);
}

void NetplaySessionBrowserDialog::on_networkAccessManager_Finished(QNetworkReply* reply)
{
    if (reply->error())
    {
        this->sessionBrowserWidget->Reset();
        QtMessageBox::Error(this, "Server Error", "Failed to retrieve server list json: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QJsonDocument jsonDocument = QJsonDocument::fromJson(reply->readAll());
    QJsonObject jsonObject     = jsonDocument.object();
    QStringList jsonServers    = jsonObject.keys();

    for (int i = 0; i < jsonServers.size(); i++)
    {
        this->serverComboBox->addItem(jsonServers.at(i), jsonObject.value(jsonServers.at(i)).toString());
    }

    reply->deleteLater();

    NetplayCommon::RestoreSelectedServer(this->serverComboBox);
}

void NetplaySessionBrowserDialog::on_serverComboBox_currentIndexChanged(int index)
{
    QPushButton* refreshButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);
    refreshButton->setEnabled(index != -1);

    if (index == -1)
    {
        return;
    }

    this->pingLineEdit->setText("Calculating...");

    this->sessionBrowserWidget->StartRefresh();

    QString address = this->serverComboBox->itemData(index).toString();
    this->webSocket->open(QUrl(address));
}

void NetplaySessionBrowserDialog::on_sessionBrowserWidget_OnSessionChanged(bool valid)
{
    this->validateJoinButton();
}

void NetplaySessionBrowserDialog::on_nickNameLineEdit_textChanged()
{
    this->validateJoinButton();
}

void NetplaySessionBrowserDialog::on_buttonBox_clicked(QAbstractButton* button)
{
    // refresh session list when refresh button has been pressed
    if (button == this->buttonBox->button(QDialogButtonBox::RestoreDefaults))
    {
        this->on_webSocket_connected();
    }
}

void NetplaySessionBrowserDialog::accept()
{
    if (!this->webSocket->isValid())
    {
        QtMessageBox::Error(this, "Server Error", "Connection Failed");
        return;
    }

    // retrieve session data
    NetplaySessionData sessionData;
    if (!this->sessionBrowserWidget->GetCurrentSession(sessionData))
    {
        return;
    }

    // disable join button while we're processing the request
    QPushButton* joinButton = this->buttonBox->button(QDialogButtonBox::Ok);
    joinButton->setEnabled(false);

    // request password when needed
    QString password;
    if (sessionData.PasswordProtected)
    {
        NetplaySessionPasswordDialog dialog(this);
        dialog.exec();
        password = dialog.GetPassword();

        // do nothing if password is empty
        if (password.isEmpty())
        {
            joinButton->setEnabled(true);
            return;
        }
    }

    // ensure features match to what we currently have
    QList<QString> pluginNames = NetplayCommon::GetPluginNames(sessionData.MD5);
    QString details;
    if (sessionData.RspPlugin != pluginNames[0])
    {
        details  = "Expected RSP Plugin: " + sessionData.RspPlugin + "\n";
        details += "Current  RSP Plugin: " + pluginNames[0];
        QtMessageBox::Error(this, "RSP Plugin Mismatch", details);
        joinButton->setEnabled(true);
        return;
    }
    if (sessionData.GfxPlugin != pluginNames[1])
    {
        details  = "Expected GFX Plugin: " + sessionData.GfxPlugin + "\n";
        details += "Current  GFX Plugin: " + pluginNames[1];
        QtMessageBox::Error(this, "GFX Plugin Mismatch", details);
        joinButton->setEnabled(true);
        return;
    }


    // attempt to find ROM from the ROM browser data
    std::string md5 = sessionData.MD5.toStdString();
    for (auto it = this->romData.begin(); it != this->romData.end(); it++)
    {
        if (it.value().MD5 == md5)
        {
            this->sessionFile = it.key();
            break;
        }
    }

    // show ROM dialog when we haven't found the ROM
    if (this->sessionFile.isEmpty())
    {
        this->sessionFile = this->showROMDialog(sessionData.GameName, sessionData.MD5);
        if (this->sessionFile.isEmpty())
        {
            joinButton->setEnabled(true);
            return;
        }
    }

    QJsonObject json;
    QJsonObject session;
    session.insert("port", sessionData.Port);
    session.insert("password", password);
    session.insert("MD5", QString::fromStdString(md5));

    json.insert("type", "request_join_room");
    json.insert("player_name", this->nickNameLineEdit->text());
    json.insert("room", session);
    NetplayCommon::AddCommonJson(json);

    this->webSocket->sendTextMessage(QJsonDocument(json).toJson());
}
