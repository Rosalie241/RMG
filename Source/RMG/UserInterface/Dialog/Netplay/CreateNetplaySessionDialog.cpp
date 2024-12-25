/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "CreateNetplaySessionDialog.hpp"
#include "NetplayCommon.hpp"

#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QNetworkDatagram>
#include <QJsonDocument>
#include <QPushButton>
#include <QMessageBox>
#include <QJsonObject>
#include <QFileInfo>

#include <RMG-Core/Core.hpp>

using namespace UserInterface::Dialog;

//
// Local Structs
//

struct NetplayRomData_t
{
    QString GoodName;
    QString MD5;
    QString File;
};

Q_DECLARE_METATYPE(NetplayRomData_t);

//
// Exported Functions
//


CreateNetplaySessionDialog::CreateNetplaySessionDialog(QWidget *parent, QWebSocket* webSocket, QMap<QString, CoreRomSettings> modelData) : QDialog(parent)
{
    qRegisterMetaType<NetplayRomData_t>();

    this->setupUi(this);

    // prepare web socket
    this->webSocket = webSocket;
    connect(this->webSocket, &QWebSocket::textMessageReceived, this, &CreateNetplaySessionDialog::on_webSocket_textMessageReceived);

    // prepare broadcast
    broadcastSocket.bind(QHostAddress(QHostAddress::AnyIPv4), 0);
    connect(&this->broadcastSocket, &QUdpSocket::readyRead, this, &CreateNetplaySessionDialog::on_broadcastSocket_readyRead);
    QByteArray multirequest;
    multirequest.append(1);
    broadcastSocket.writeDatagram(multirequest, QHostAddress::Broadcast, 45000);

    // change ok button name
    QPushButton* createButton = this->buttonBox->button(QDialogButtonBox::Ok);
    createButton->setText("Create");
    createButton->setEnabled(false);

    // set validator for nickname
    QRegularExpression nicknameRe(NETPLAYCOMMON_NICKNAME_REGEX);
    this->nickNameLineEdit->setValidator(new QRegularExpressionValidator(nicknameRe, this));
    this->nickNameLineEdit->setText(QString::fromStdString(CoreSettingsGetStringValue(SettingsID::Netplay_Nickname)));

    // set validator for session
    QRegularExpression sessionRe(NETPLAYCOMMON_SESSION_REGEX);
    this->sessionNameLineEdit->setValidator(new QRegularExpressionValidator(sessionRe, this));

    // set validator for password
    QRegularExpression passwordRe(NETPLAYCOMMON_PASSWORD_REGEX);
    this->passwordLineEdit->setValidator(new QRegularExpressionValidator(passwordRe, this));

    // transform model data to data we can use
    QList<NetplayRomData_t> romData;
    romData.reserve(modelData.size());
    for (auto it = modelData.begin(); it != modelData.end(); it++)
    {
        romData.append(
        {
            QString::fromStdString(it.value().GoodName), 
            QString::fromStdString(it.value().MD5), 
            it.key()
        });
    }
    // add data to list widget
    for (const NetplayRomData_t& data : romData)
    {
        QListWidgetItem* item = new QListWidgetItem();
        item->setData(Qt::UserRole, QVariant::fromValue(data));
        item->setText(this->getGameName(data.GoodName, data.File));
        this->listWidget->addItem(item);
    }
    this->listWidget->sortItems();

    this->validateCreateButton();

    // request server list
    QString serverUrl = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::Netplay_ServerJsonUrl));
    if (!serverUrl.isEmpty() && QUrl(serverUrl).isValid())
    {
        QNetworkAccessManager* networkAccessManager = new QNetworkAccessManager(this);
        connect(networkAccessManager, &QNetworkAccessManager::finished, this, &CreateNetplaySessionDialog::on_networkAccessManager_Finished);
        networkAccessManager->get(QNetworkRequest(QUrl(serverUrl)));
    }
}

CreateNetplaySessionDialog::~CreateNetplaySessionDialog(void)
{
    QString nickname = this->nickNameLineEdit->text();
    if (!nickname.isEmpty())
    {
        CoreSettingsSetValue(SettingsID::Netplay_Nickname, nickname.toStdString());
    }
}

QJsonObject CreateNetplaySessionDialog::GetSessionJson(void)
{
    return this->sessionJson;
}

QString CreateNetplaySessionDialog::GetSessionFile(void)
{
    return this->sessionFile;
}

void CreateNetplaySessionDialog::showErrorMessage(QString error, QString details)
{
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Icon::Critical);
    msgBox.setWindowTitle("Error");
    msgBox.setText(error);
    msgBox.setDetailedText(details);
    msgBox.addButton(QMessageBox::Ok);
    msgBox.exec();
}

QString CreateNetplaySessionDialog::getGameName(QString goodName, QString file)
{
    QString gameName = goodName;

    if (gameName.endsWith("(unknown rom)") ||
        gameName.endsWith("(unknown disk)"))
    {
        gameName = QFileInfo(file).fileName();
    }

    return gameName;
}

bool CreateNetplaySessionDialog::validate(void)
{
    if (this->nickNameLineEdit->text().isEmpty() ||
        this->nickNameLineEdit->text().size() > 128)
    {
        return false;
    }

    if (this->sessionNameLineEdit->text().isEmpty() ||
        this->sessionNameLineEdit->text().size() > 128)
    {
        return false;
    }

    if (this->listWidget->count() == 0 ||
        this->serverComboBox->count() == 0)
    {
        return false;
    }

    if (this->listWidget->currentItem() == nullptr)
    {
        return false;
    }

    return true;
}

void CreateNetplaySessionDialog::validateCreateButton(void)
{
    QPushButton* createButton = this->buttonBox->button(QDialogButtonBox::Ok);
    createButton->setEnabled(this->validate());
}

void CreateNetplaySessionDialog::on_webSocket_textMessageReceived(QString message)
{
    QJsonDocument jsonDocument = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject json = jsonDocument.object();

    if (json.value("type").toString() == "reply_create_room")
    {
        if (json.value("accept").toInt() == 0)
        {
            this->sessionJson = json;
            QDialog::accept();
        }
        else
        {
            this->showErrorMessage("Server Error", json.value("message").toString());
            this->validateCreateButton();
        }
    }
}

void CreateNetplaySessionDialog::on_broadcastSocket_readyRead()
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

void CreateNetplaySessionDialog::on_networkAccessManager_Finished(QNetworkReply* reply)
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

    // TODO: custom server support
    //this->serverComboBox->addItem(QString("Custom"), QString("Custom"));
    reply->deleteLater();
}

void CreateNetplaySessionDialog::on_serverComboBox_currentIndexChanged(int index)
{
    if (index == -1)
    {
        return;
    }

    QString address = this->serverComboBox->itemData(index).toString();
    this->webSocket->open(QUrl(address));
}

void CreateNetplaySessionDialog::on_nickNameLineEdit_textChanged(void)
{
    this->validateCreateButton();
}

void CreateNetplaySessionDialog::on_sessionNameLineEdit_textChanged(void)
{
    this->validateCreateButton();
}

void CreateNetplaySessionDialog::on_passwordLineEdit_textChanged(void)
{
    this->validateCreateButton();
}

void CreateNetplaySessionDialog::on_listWidget_currentRowChanged(int index)
{
    this->validateCreateButton();
}

void CreateNetplaySessionDialog::accept()
{
    if (!this->webSocket->isValid())
    {
        this->showErrorMessage("Server Error", "Connection Failed");
        return;
    }

    // disable create button while we're processing the request
    QPushButton* createButton = this->buttonBox->button(QDialogButtonBox::Ok);
    createButton->setEnabled(false);

    QListWidgetItem* item    = this->listWidget->currentItem();
    NetplayRomData_t romData = item->data(Qt::UserRole).value<NetplayRomData_t>();

    this->sessionFile = romData.File;

    QList<QString> plugins = NetplayCommon::GetPluginNames(romData.MD5);

    QJsonObject jsonFeatures;
    jsonFeatures.insert("cpu_emulator", NetplayCommon::GetCpuEmulator(romData.MD5));
    jsonFeatures.insert("rsp_plugin", plugins[0]);
    jsonFeatures.insert("gfx_plugin", plugins[1]);

    QJsonObject json;
    json.insert("type", "request_create_room");
    json.insert("room_name", this->sessionNameLineEdit->text());
    json.insert("player_name", this->nickNameLineEdit->text());
    json.insert("password", this->passwordLineEdit->text());
    json.insert("MD5", romData.MD5);
    json.insert("game_name", this->getGameName(romData.GoodName, romData.File));
    json.insert("features",  jsonFeatures);
    NetplayCommon::AddCommonJson(json);

    this->webSocket->sendTextMessage(QJsonDocument(json).toJson());
}
