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
#include "UserInterface/NoFocusDelegate.hpp"
#include "NetplaySessionPasswordDialog.hpp"
#include "NetplayCommon.hpp"

#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QNetworkDatagram>
#include <QJsonDocument>
#include <QInputDialog>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonObject>

#include <RMG-Core/Core.hpp>

using namespace UserInterface::Dialog;

//
// Local Structs
//

struct NetplaySessionData_t
{
    QString SessionName;
    QString GameName;
    QString MD5;
    bool PasswordProtected = false;
    int  Port = 0;
    QString CpuEmulator;
    QString RspPlugin;
    QString GfxPlugin;
};

Q_DECLARE_METATYPE(NetplaySessionData_t);

//
// Exported Functions
//

NetplaySessionBrowserDialog::NetplaySessionBrowserDialog(QWidget *parent, QWebSocket* webSocket, QMap<QString, CoreRomSettings> modelData) : QDialog(parent)
{
    qRegisterMetaType<NetplaySessionData_t>();

    this->setupUi(this);

    // prepare web socket
    this->webSocket = webSocket;
    connect(this->webSocket, &QWebSocket::connected, this, &NetplaySessionBrowserDialog::on_webSocket_connected);
    connect(this->webSocket, &QWebSocket::textMessageReceived, this, &NetplaySessionBrowserDialog::on_webSocket_textMessageReceived);

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
    refreshButton->setIcon(QIcon());
    refreshButton->setEnabled(false);

    // set validator for nickname
    QRegularExpression re(NETPLAYCOMMON_NICKNAME_REGEX);
    this->nickNameLineEdit->setValidator(new QRegularExpressionValidator(re, this));
    this->nickNameLineEdit->setText(QString::fromStdString(CoreSettingsGetStringValue(SettingsID::Netplay_Nickname)));

    // prepare session browser
    this->tableWidget->setFrameStyle(QFrame::NoFrame);
    this->tableWidget->setItemDelegate(new NoFocusDelegate(this));
    this->tableWidget->setWordWrap(false);
    this->tableWidget->setShowGrid(false);
    this->tableWidget->setSortingEnabled(true);
    this->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->tableWidget->setSelectionBehavior(QTableView::SelectRows);
    this->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    this->tableWidget->setVerticalScrollMode(QAbstractItemView::ScrollMode::ScrollPerPixel);
    this->tableWidget->verticalHeader()->hide();
    this->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    this->tableWidget->horizontalHeader()->setSectionsMovable(true);
    this->tableWidget->horizontalHeader()->setFirstSectionMovable(true);
    this->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    this->tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    this->tableWidget->horizontalHeader()->setSortIndicatorShown(false);
    this->tableWidget->horizontalHeader()->setHighlightSections(false);

    // request server list
    QString serverUrl = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::Netplay_ServerJsonUrl));
    if (!serverUrl.isEmpty() && QUrl(serverUrl).isValid())
    {
        QNetworkAccessManager* networkAccessManager = new QNetworkAccessManager(this);
        connect(networkAccessManager, &QNetworkAccessManager::finished, this, &NetplaySessionBrowserDialog::on_networkAccessManager_Finished);
        networkAccessManager->get(QNetworkRequest(QUrl(serverUrl)));
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
}

QJsonObject NetplaySessionBrowserDialog::GetSessionJson(void)
{
    return this->sessionJson;
}

QString NetplaySessionBrowserDialog::GetSessionFile(void)
{
    return this->sessionFile;
}

void NetplaySessionBrowserDialog::showErrorMessage(QString error, QString details)
{
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Icon::Critical);
    msgBox.setWindowTitle("Error");
    msgBox.setText(error);
    msgBox.setDetailedText(details);
    msgBox.addButton(QMessageBox::Ok);
    msgBox.exec();
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
            this->showErrorMessage("Incorrect ROM Selected", details);
            return "";
        }
    }

    return file;
}

bool NetplaySessionBrowserDialog::validate(void)
{
    if (this->nickNameLineEdit->text().isEmpty() ||
        this->nickNameLineEdit->text().contains(' ') ||
        this->nickNameLineEdit->text().size() > 256)
    {
        return false;
    }

    if (this->tableWidget->currentItem() == nullptr ||
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

void NetplaySessionBrowserDialog::addSessionData(QString name, QString game, QString md5, bool password, int port,
                                                 QString cpuEmulator, QString rspPlugin, QString gfxPlugin)
{
    const NetplaySessionData_t sessionData =
    {
        name,
        game,
        md5,
        password,
        port,
        cpuEmulator,
        rspPlugin,
        gfxPlugin
    };

    int row = this->tableWidget->rowCount();
    this->tableWidget->insertRow(row);

    // Session name
    QTableWidgetItem* tableWidgetItem1 = new QTableWidgetItem(name);
    tableWidgetItem1->setData(Qt::UserRole, QVariant::fromValue<NetplaySessionData_t>(sessionData));
    this->tableWidget->setItem(row, 0, tableWidgetItem1);

    // Game
    QTableWidgetItem* tableWidgetItem2 = new QTableWidgetItem(game);
    this->tableWidget->setItem(row, 1, tableWidgetItem2);

    // MD5
    QTableWidgetItem* tableWidgetItem3 = new QTableWidgetItem(md5);
    this->tableWidget->setItem(row, 2, tableWidgetItem3);

    // Password
    QTableWidgetItem* tableWidgetItem4 = new QTableWidgetItem(password ? "Yes" : "No");
    this->tableWidget->setItem(row, 3, tableWidgetItem4);
}

void NetplaySessionBrowserDialog::on_webSocket_connected(void)
{
    if (!this->webSocket->isValid())
    {
        return;
    }

    // clear sessions from the table
    this->tableWidget->model()->removeRows(0, this->tableWidget->rowCount());

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
            this->addSessionData(json.value("room_name").toString(), 
                                    json.value("game_name").toString(), 
                                    json.value("MD5").toString(), 
                                    json.value("protected").toBool(),
                                    json.value("port").toInt(),
                                    json.value("features").toObject().value("cpu_emulator").toString(),
                                    json.value("features").toObject().value("rsp_plugin").toString(),
                                    json.value("features").toObject().value("gfx_plugin").toString());
        }
        else
        {
            this->showErrorMessage("Server Error", json.value("message").toString());
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
            this->showErrorMessage("Server Error", json.value("message").toString());
            this->validateJoinButton();
        }
    }
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
}

void NetplaySessionBrowserDialog::on_networkAccessManager_Finished(QNetworkReply* reply)
{
    if (reply->error())
    {
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
}

void NetplaySessionBrowserDialog::on_serverComboBox_currentIndexChanged(int index)
{
    QPushButton* refreshButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);
    refreshButton->setEnabled(index != -1);

    if (index == -1)
    {
        return;
    }

    // clear sessions from the table
    this->tableWidget->model()->removeRows(0, this->tableWidget->rowCount());

    QString address = this->serverComboBox->itemData(index).toString();
    this->webSocket->open(QUrl(address));
}

void NetplaySessionBrowserDialog::on_tableWidget_currentItemChanged(QTableWidgetItem* current, QTableWidgetItem* previous)
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
        this->showErrorMessage("Server Error", "Connection Failed");
        return;
    }

    // disable join button while we're processing the request
    QPushButton* joinButton = this->buttonBox->button(QDialogButtonBox::Ok);
    joinButton->setEnabled(false);

    // retrieve information from row
    QTableWidgetItem* item = this->tableWidget->item(this->tableWidget->currentRow(), 0);
    NetplaySessionData_t sessionData = item->data(Qt::UserRole).value<NetplaySessionData_t>();

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
    QString cpuEmulator        = NetplayCommon::GetCpuEmulator(sessionData.MD5);
    QString details;
    if (sessionData.CpuEmulator != cpuEmulator)
    {
        details  = "Expected CPU Emulator: " + NetplayCommon::GetCpuEmulatorName(sessionData.CpuEmulator) + "\n";
        details += "Current  CPU Emulator: " + NetplayCommon::GetCpuEmulatorName(cpuEmulator);
        this->showErrorMessage("CPU Emulator Mismatch", details);
        joinButton->setEnabled(true);
        return;
    }
    if (sessionData.RspPlugin != pluginNames[0])
    {
        details  = "Expected RSP Plugin: " + sessionData.RspPlugin + "\n";
        details += "Current  RSP Plugin: " + pluginNames[0];
        this->showErrorMessage("RSP Plugin Mismatch", details);
        joinButton->setEnabled(true);
        return;
    }
    if (sessionData.GfxPlugin != pluginNames[1])
    {
        details  = "Expected GFX Plugin: " + sessionData.GfxPlugin + "\n";
        details += "Current  GFX Plugin: " + pluginNames[1];
        this->showErrorMessage("GFX Plugin Mismatch", details);
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
    json.insert("type", "request_join_room");
    json.insert("port", sessionData.Port);
    json.insert("player_name", this->nickNameLineEdit->text());
    json.insert("password", password);
    json.insert("MD5", QString::fromStdString(md5));
    NetplayCommon::AddCommonJson(json);

    this->webSocket->sendTextMessage(QJsonDocument(json).toJson());
}
