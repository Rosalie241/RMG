/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "NetplaySessionBrowserDialog.hpp"
#include "NetplaySessionPasswordDialog.hpp"
#include "Utilities/QtMessageBox.hpp"
#include "NetplayCommon.hpp"

#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QNetworkDatagram>
#include <QJsonDocument>
#include <QInputDialog>
#include <QPushButton>
#include <QFileDialog>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QFile>

#include <RMG-Core/Settings.hpp>
#include <RMG-Core/Rom.hpp>

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
    refreshButton->setText("Refresh");
    refreshButton->setIcon(QIcon::fromTheme("refresh-line"));
    refreshButton->setEnabled(false);

    // set validator for nickname
    QRegularExpression re(NETPLAYCOMMON_NICKNAME_REGEX);
    this->nickNameLineEdit->setValidator(new QRegularExpressionValidator(re, this));
    this->nickNameLineEdit->setText(QString::fromStdString(CoreSettingsGetStringValue(SettingsID::Netplay_Nickname)));

    // configure dispatcher network access manager
    this->dispatcherNetworkAccessManager = new QNetworkAccessManager(this);
    this->dispatcherNetworkAccessManager->setTransferTimeout(15000);
    connect(this->dispatcherNetworkAccessManager, &QNetworkAccessManager::finished, this, &NetplaySessionBrowserDialog::on_dispatcherRetrieveServers_Finished);

    // request server list
    QString serverUrl = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::Netplay_ServerJsonUrl));
    if (!serverUrl.isEmpty())
    {
        QFile qFile(serverUrl);
        if (qFile.exists())
        {
            if (qFile.open(QIODevice::ReadOnly))
            {
                NetplayCommon::AddServers(this->serverComboBox, 
                                          QJsonDocument::fromJson(qFile.readAll()));   
            }
            else
            {
                QtMessageBox::Error(this, "Server Error", "Failed to open server list json: " + qFile.errorString());
            }
        }
        else if (QUrl(serverUrl).isValid())
        {
            QNetworkAccessManager* networkAccessManager = new QNetworkAccessManager(this);
            connect(networkAccessManager, &QNetworkAccessManager::finished, this, &NetplaySessionBrowserDialog::on_jsonServerListDownload_Finished);
            networkAccessManager->setTransferTimeout(15000);
            networkAccessManager->get(QNetworkRequest(QUrl(serverUrl)));
        }
    }

    QString dispatcherUrl = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::Netplay_DispatcherUrl));
    if (!dispatcherUrl.isEmpty() && QUrl(dispatcherUrl).isValid())
    {
        this->dispatcherUrl = dispatcherUrl;

        QNetworkAccessManager* networkAccessManager = new QNetworkAccessManager(this);
        connect(networkAccessManager, &QNetworkAccessManager::finished, this, &NetplaySessionBrowserDialog::on_dispatcherRegionListDownload_Finished);
        networkAccessManager->setTransferTimeout(15000);

        QNetworkRequest networkRequest(QUrl(this->dispatcherUrl + "/getRegions"));
        networkRequest.setRawHeader("netplay-id", "RMG");

        networkAccessManager->get(networkRequest);
    }

    this->validateJoinButton();
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

void NetplaySessionBrowserDialog::toggleUI(bool enable, bool enableJoinButton)
{
    QPushButton* joinButton = this->buttonBox->button(QDialogButtonBox::Ok);
    joinButton->setEnabled(enableJoinButton);

    QPushButton* refreshButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);
    refreshButton->setEnabled(enable);

    this->serverComboBox->setEnabled(enable);
    this->nickNameLineEdit->setReadOnly(!enable);
}

void NetplaySessionBrowserDialog::refreshSessions(void)
{
    // disable refresh button while refreshing
    QPushButton* refreshButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);
    refreshButton->setEnabled(false);

    // disable join button while refreshing
    QPushButton* joinButton = this->buttonBox->button(QDialogButtonBox::Ok);
    joinButton->setEnabled(false);

    if (NetplayCommon::IsServerDispatcher(this->serverComboBox))
    {
        this->on_serverComboBox_currentIndexChanged(this->serverComboBox->currentIndex());
    }
    else
    {
        this->on_webSocket_connected();
    }
}

void NetplaySessionBrowserDialog::joinSession(void)
{
    QJsonObject json;
    QJsonObject session;
    session.insert("port", sessionData.Port);
    session.insert("password", this->sessionPassword);
    session.insert("MD5", this->sessionData.MD5);

    json.insert("type", "request_join_room");
    json.insert("player_name", this->nickNameLineEdit->text());
    json.insert("room", session);
    NetplayCommon::AddCommonJson(json);

    this->webSocket->sendTextMessage(QJsonDocument(json).toJson());
}

void NetplaySessionBrowserDialog::resetDispatcherState(void)
{
    if (this->dispatcherTimerId != -1)
    {
        this->killTimer(this->dispatcherTimerId);
        this->dispatcherTimerId = -1;
    }

    if (this->dispatcherTimeoutTimerId != -1)
    {
        this->killTimer(this->dispatcherTimeoutTimerId);
        this->dispatcherTimeoutTimerId = -1;
    }

    if (this->dispatcherNetworkReply != nullptr)
    {
        this->dispatcherNetworkReply->abort();
        this->dispatcherNetworkReply = nullptr;
    }

    this->dispatcherMoveThroughList  = false;
    this->dispatcherJoinSession      = false;
    this->dispatcherAddressListIndex = false;
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
    else if (event->timerId() == this->dispatcherTimerId)
    {
        if (this->dispatcherMoveThroughList)
        {
            this->dispatcherAddressListIndex++;

            if (this->dispatcherAddressListIndex < this->dispatcherAddressList.size())
            {
                QString address = this->dispatcherAddressList.at(this->dispatcherAddressListIndex);

                if (!NetplayCommon::ConnectToIPv4Server(address, this->webSocket))
                {
                    this->sessionBrowserWidget->Reset();
                    QtMessageBox::Error(this, "Failed to find IPv4 address of server");
                }

                this->dispatcherMoveThroughList = false;

                // start socket timeout
                if (this->dispatcherTimeoutTimerId != -1)
                {
                    this->killTimer(this->dispatcherTimeoutTimerId);
                    this->dispatcherTimeoutTimerId = -1;
                }

                this->dispatcherTimeoutTimerId = this->startTimer(15000);
            }
            else
            {
                this->sessionBrowserWidget->RefreshDone();
                this->resetDispatcherState();
            }
        }
    }
    else if (event->timerId() == this->dispatcherTimeoutTimerId)
    {
        this->webSocket->close();
        this->dispatcherMoveThroughList = true;

        this->killTimer(this->dispatcherTimeoutTimerId);
        this->dispatcherTimeoutTimerId = -1;

    }
}

void NetplaySessionBrowserDialog::on_webSocket_connected(void)
{
    if (!this->webSocket->isValid())
    {
        QtMessageBox::Error(this, "Server Error", "Connection Failed");
        return;
    }

    bool dispatcher = NetplayCommon::IsServerDispatcher(this->serverComboBox);

    if (dispatcher && this->dispatcherJoinSession)
    {
        this->joinSession();
        this->dispatcherJoinSession = false;
        return;
    }

    // clear sessions when we're not using the dispatcher server
    if (!dispatcher)
    {
        this->sessionBrowserWidget->StartRefresh();
    }

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

    bool dispatcher = NetplayCommon::IsServerDispatcher(this->serverComboBox);

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
                                                            session.value("features").toObject().value("gfx_plugin").toString(),
                                                            this->webSocket->requestUrl());
            }

            // we're done refreshing the sessions
            if (!dispatcher)
            {
                this->sessionBrowserWidget->RefreshDone();
            }
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
            this->toggleUI(true, this->validate());
        }
    }

    if (dispatcher)
    {
        this->dispatcherMoveThroughList = true;
    }
}

void NetplaySessionBrowserDialog::on_webSocket_pong(quint64 elapsedTime, const QByteArray&)
{
    if (!NetplayCommon::IsServerDispatcher(this->serverComboBox))
    {
        this->pingLineEdit->setText(QString::number(elapsedTime) + " ms");
    }
}

void NetplaySessionBrowserDialog::on_webSocket_disconnected()
{
    if (!NetplayCommon::IsServerDispatcher(this->serverComboBox))
    {
        this->sessionBrowserWidget->Reset();
    }
}

void NetplaySessionBrowserDialog::on_broadcastSocket_readyRead(void)
{
    while (this->broadcastSocket.hasPendingDatagrams())
    {
        QNetworkDatagram datagram = this->broadcastSocket.receiveDatagram();
        QByteArray incomingData = datagram.data();

        NetplayCommon::AddServers(this->serverComboBox, QJsonDocument::fromJson(incomingData));
    }

    NetplayCommon::RestoreSelectedServer(this->serverComboBox);
}

void NetplaySessionBrowserDialog::on_jsonServerListDownload_Finished(QNetworkReply* reply)
{
    if (reply->error())
    {
        this->sessionBrowserWidget->Reset();
        QtMessageBox::Error(this, "Server Error", "Failed to retrieve server list json: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    NetplayCommon::AddServers(this->serverComboBox, 
                              QJsonDocument::fromJson(reply->readAll()));

    reply->deleteLater();
}

void NetplaySessionBrowserDialog::on_dispatcherRegionListDownload_Finished(QNetworkReply* reply)
{
    if (reply->error())
    {
        this->sessionBrowserWidget->Reset();
        QtMessageBox::Error(this, "Server Error", "Failed to retrieve region list: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    NetplayCommon::AddServers(this->serverComboBox, 
                              QJsonDocument::fromJson(reply->readAll()), true);

    reply->deleteLater();
}

void NetplaySessionBrowserDialog::on_dispatcherRetrieveServers_Finished(QNetworkReply* reply)
{
    // don't show an error when the request has been aborted
    if (reply->error() == QNetworkReply::OperationCanceledError)
    {
        reply->deleteLater();
        return;
    }

    if (reply->error())
    {
        this->sessionBrowserWidget->Reset();
        QtMessageBox::Error(this, "Server Error", "Failed to retrieve server list: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QJsonDocument jsonDocument = QJsonDocument::fromJson(reply->readAll());
    QJsonObject jsonObject = jsonDocument.object();
    QStringList jsonKeys = jsonObject.keys();

    this->sessionBrowserWidget->StartRefresh();
    this->resetDispatcherState();

    // we only care about the addresses
    this->dispatcherAddressList.clear();
    for (int i = 0; i < jsonKeys.size(); i++)
    {
        QString address = jsonObject[jsonKeys.at(i)].toString();
        this->dispatcherAddressList.append(address);
    }

    this->dispatcherAddressListIndex = -1;
    this->dispatcherMoveThroughList = true;
    this->dispatcherTimerId = this->startTimer(250);

    reply->deleteLater();
}

void NetplaySessionBrowserDialog::on_serverComboBox_currentIndexChanged(int index)
{
    QPushButton* refreshButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);
    refreshButton->setEnabled(index != -1);

    if (index == -1)
    {
        return;
    }

    bool dispatcher = NetplayCommon::IsServerDispatcher(this->serverComboBox, index);

    if (this->pingTimerId != -1)
    {
        this->killTimer(this->pingTimerId);
        this->pingTimerId = -1;
    }

    this->resetDispatcherState();
    this->pingLineEdit->setText(dispatcher ? "N/A" : "Calculating...");
    this->sessionBrowserWidget->StartRefresh();

    if (dispatcher)
    {
        QString region = NetplayCommon::GetServerData(this->serverComboBox, index);

        QUrl url(this->dispatcherUrl + "/getServers");

        QUrlQuery urlQuery;
        urlQuery.addQueryItem("region", region);
        url.setQuery(urlQuery);

        QNetworkRequest networkRequest(url);
        networkRequest.setRawHeader("netplay-id", "RMG");

        // sadly we have to force HTTP/1 here due to a Qt bug,
        // we abort the connection which sadly in Qt6 causes an error and eventually segfault,
        // so to workaround this, force HTTP/1 until it's fixed in Qt6
        //
        // thank you MapLibre devs for figuring out this issue:
        // https://github.com/maplibre/maplibre-native/issues/3644
        networkRequest.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

        this->dispatcherNetworkReply = this->dispatcherNetworkAccessManager->get(networkRequest);
    }
    else
    {
        this->pingTimerId = this->startTimer(2000);

        QString address = NetplayCommon::GetServerData(this->serverComboBox, index);
        this->webSocket->open(QUrl(address));
    }
}

void NetplaySessionBrowserDialog::on_sessionBrowserWidget_OnSessionChanged(bool valid)
{
    this->validateJoinButton();
}

void NetplaySessionBrowserDialog::on_sessionBrowserWidget_OnRefreshDone(void)
{
    QPushButton* refreshButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);
    refreshButton->setEnabled(true);
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
        this->refreshSessions();
    }
}

void NetplaySessionBrowserDialog::accept()
{
    if (!NetplayCommon::IsServerDispatcher(this->serverComboBox) &&
        !this->webSocket->isValid())
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
    this->toggleUI(false, false);

    // request password when needed
    if (sessionData.PasswordProtected)
    {
        NetplaySessionPasswordDialog dialog(this);
        dialog.exec();
        this->sessionPassword = dialog.GetPassword();

        // do nothing if password is empty
        if (this->sessionPassword.isEmpty())
        {
            this->toggleUI(true, true);
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
        this->toggleUI(true, true);
        return;
    }
    if (sessionData.GfxPlugin != pluginNames[1])
    {
        details  = "Expected GFX Plugin: " + sessionData.GfxPlugin + "\n";
        details += "Current  GFX Plugin: " + pluginNames[1];
        QtMessageBox::Error(this, "GFX Plugin Mismatch", details);
        this->toggleUI(true, true);
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
            this->toggleUI(true, true);
            return;
        }
    }

    this->sessionData = sessionData;

    if (NetplayCommon::IsServerDispatcher(this->serverComboBox))
    {
        this->dispatcherJoinSession = true;
        this->webSocket->open(sessionData.Address);
    }
    else
    {
        this->joinSession();
    }
}
