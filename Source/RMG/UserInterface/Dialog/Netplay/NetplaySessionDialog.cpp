/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "NetplaySessionDialog.hpp"
#include "NetplayCommon.hpp"

#include <QJsonDocument>
#include <QPushButton>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonArray>

#include <RMG-Core/Core.hpp>

using namespace UserInterface::Dialog;

NetplaySessionDialog::NetplaySessionDialog(QWidget *parent, QWebSocket* webSocket, QJsonObject sessionJson, QString sessionFile) : QDialog(parent)
{
    this->setupUi(this);

    this->webSocket = webSocket;

    this->nickName    = sessionJson.value("player_name").toString();
    this->sessionPort = sessionJson.value("port").toInt();
    this->sessionName = sessionJson.value("room_name").toString();
    this->sessionFile = sessionFile;

    this->sessionNameLineEdit->setText(this->sessionName);
    this->gameNameLineEdit->setText(sessionJson.value("game_name").toString());

    connect(this->webSocket, &QWebSocket::textMessageReceived, this, &NetplaySessionDialog::on_webSocket_textMessageReceived);

    // request server motd
    QJsonObject json;
    json.insert("type", "request_motd");
    json.insert("room_name", this->sessionName);
    NetplayCommon::AddCommonJson(json);
    webSocket->sendTextMessage(QJsonDocument(json).toJson());

    // request players
    json.insert("type", "request_players");
    json.insert("port", this->sessionPort);
    NetplayCommon::AddCommonJson(json);
    webSocket->sendTextMessage(QJsonDocument(json).toJson());

    QPushButton* startButton = this->buttonBox->button(QDialogButtonBox::Ok);
    startButton->setText("Start");
    startButton->setEnabled(false);
}

NetplaySessionDialog::~NetplaySessionDialog(void)
{
}

void NetplaySessionDialog::showErrorMessage(QString error, QString details)
{
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Icon::Critical);
    msgBox.setWindowTitle("Error");
    msgBox.setText(error);
    msgBox.setDetailedText(details);
    msgBox.addButton(QMessageBox::Ok);
    msgBox.exec();
}

void NetplaySessionDialog::on_webSocket_textMessageReceived(QString message)
{
    QJsonDocument jsonDocument = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject json = jsonDocument.object();
    QString     type = json.value("type").toString();

    if (type == "reply_players")
    {
        if (json.contains("player_names"))
        {
            this->listWidget->clear();
            QString name;
            for (int i = 0; i < 4; ++i)
            {
                name = json.value("player_names").toArray().at(i).toString();
                if (!name.isEmpty())
                {
                    this->listWidget->addItem(name);
                    if (this->nickName == name)
                    {
                        this->sessionNumber = i + 1;
                    }
                }
                if (i == 0)
                {
                    QPushButton* startButton = this->buttonBox->button(QDialogButtonBox::Ok);
                    startButton->setEnabled(this->nickName == name);
                }
            }
        }
    }
    else if (type == "reply_chat_message")
    {
        this->chatPlainTextEdit->appendPlainText(json.value("message").toString());
    }
    else if (type == "reply_begin_game")
    {
        emit OnPlayGame(this->sessionFile, this->webSocket->peerAddress().toString(), this->sessionPort, this->sessionNumber);
        QDialog::accept();
    }
    else if (type == "reply_motd")
    {
        QString message = "<b>MOTD:</b> " + json.value("message").toString();
        this->chatPlainTextEdit->appendHtml(message);
        this->chatPlainTextEdit->setTextInteractionFlags(Qt::TextBrowserInteraction);
    }
}

void NetplaySessionDialog::on_chatLineEdit_textChanged(QString text)
{
    this->sendPushButton->setEnabled(!text.isEmpty() && text.size() <= 256);
}

void NetplaySessionDialog::on_sendPushButton_clicked(void)
{
    QJsonObject json;
    json.insert("type", "request_chat_message");
    json.insert("port", this->sessionPort);
    json.insert("player_name", this->nickName);
    json.insert("message", this->chatLineEdit->text());
    NetplayCommon::AddCommonJson(json);

    webSocket->sendTextMessage(QJsonDocument(json).toJson());
    this->chatLineEdit->clear();
}

void NetplaySessionDialog::accept()
{
    QPushButton* startButton = this->buttonBox->button(QDialogButtonBox::Ok);
    startButton->setEnabled(false);

    QJsonObject json;
    json.insert("type", "request_begin_game");
    json.insert("port", this->sessionPort);
    NetplayCommon::AddCommonJson(json);

    webSocket->sendTextMessage(QJsonDocument(json).toJson());
}
