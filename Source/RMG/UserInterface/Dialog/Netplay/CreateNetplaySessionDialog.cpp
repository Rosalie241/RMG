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
#include "Utilities/QtMessageBox.hpp"

#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QNetworkDatagram>
#include <QJsonDocument>
#include <QPushButton>
#include <QJsonObject>
#include <QFileInfo>
#include <QJsonArray>

#include <RMG-Core/Core.hpp>

using namespace UserInterface::Dialog;
using namespace Utilities;


//
// Exported Functions
//


CreateNetplaySessionDialog::CreateNetplaySessionDialog(QWidget *parent, QWebSocket* webSocket, QMap<QString, CoreRomSettings> modelData) : QDialog(parent)
{
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

    // add data to widget
    for (auto it = modelData.begin(); it != modelData.end(); it++)
    {
        this->romListWidget->AddRomData(this->getGameName(QString::fromStdString(it.value().GoodName), it.key()),
                                        QString::fromStdString(it.value().MD5),
                                        it.key());
    }
    this->romListWidget->RefreshDone();

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

    if (!this->romListWidget->IsCurrentRomValid() ||
        this->serverComboBox->count() == 0)
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
            QtMessageBox::Error(this, "Server Error", json.value("message").toString());
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

void CreateNetplaySessionDialog::on_romListWidget_OnRomChanged(bool valid)
{
    this->validateCreateButton();
}

void CreateNetplaySessionDialog::accept()
{
    if (!this->webSocket->isValid())
    {
        QtMessageBox::Error(this, "Server Error", "Connection Failed");
        return;
    }

    NetplayRomData romData;
    if (!this->romListWidget->GetCurrentRom(romData))
    {
        return;
    }

    // Disable create button while processing the request
    QPushButton* createButton = this->buttonBox->button(QDialogButtonBox::Ok);
    createButton->setEnabled(false);

    this->sessionFile = romData.File;

    QList<QString> plugins = NetplayCommon::GetPluginNames(romData.MD5);

    QJsonObject jsonFeatures;
    jsonFeatures.insert("rsp_plugin", plugins[0]);
    jsonFeatures.insert("gfx_plugin", plugins[1]);

    // Retrieve and format cheats
    QJsonArray cheatsArray = GetSessionCheats();
    if (!cheatsArray.isEmpty()) {
        jsonFeatures.insert("cheats", cheatsArray);
    }

    QJsonObject json;
    QJsonObject session;
    session.insert("room_name", this->sessionNameLineEdit->text());
    session.insert("password", this->passwordLineEdit->text());
    session.insert("MD5", romData.MD5);
    session.insert("game_name", this->getGameName(romData.GoodName, romData.File));
    session.insert("features", jsonFeatures);
    json.insert("type", "request_create_room");
    json.insert("player_name", this->nickNameLineEdit->text());
    json.insert("room", session);
    NetplayCommon::AddCommonJson(json);

    this->webSocket->sendTextMessage(QJsonDocument(json).toJson());
}

QJsonArray CreateNetplaySessionDialog::GetSessionCheats()
{
    std::vector<CoreCheat> coreCheats;
    QJsonArray cheatsArray;

    if (CoreGetCurrentCheats(coreCheats)) {
        for (const auto& cheat : coreCheats) {
            if (CoreIsCheatEnabled(cheat)) {
                for (const auto& code : cheat.CheatCodes) {
                    QString codeStr = FormatCheatCode(cheat, code);
                    if (!codeStr.isEmpty()) {
                        cheatsArray.append(codeStr);
                    }
                }
            }
        }
    }

    return cheatsArray;
}

QString CreateNetplaySessionDialog::FormatCheatCode(const CoreCheat& cheat, const CoreCheatCode& code)
{
    QString codeStr;
    if (code.UseOptions) {
        CoreCheatOption currentOption;
        if (CoreGetCheatOption(cheat, currentOption)) {
            QString codeValueString = QString("%1").arg(code.Value, 4, 16, QChar('0')).toUpper();
            QString optionValueString = QString("%1").arg(currentOption.Value, code.OptionSize * 2, 16, QChar('0')).toUpper();

            if (optionValueString.size() == code.OptionSize * 2) {
                codeValueString.replace(code.OptionIndex * 2, code.OptionSize * 2, optionValueString);
                codeStr = QString("%1 %2").arg(code.Address, 8, 16, QChar('0')).arg(codeValueString).toUpper();
            }
        }
    } else {
        codeStr = QString("%1 %2").arg(code.Address, 8, 16, QChar('0')).arg(code.Value, 4, 16, QChar('0')).toUpper();
    }
    return codeStr;
}