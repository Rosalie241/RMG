/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "UserInterface/Dialog/Cheats/CheatsCommon.hpp"
#include "UserInterface/Dialog/Cheats/CheatsDialog.hpp"
#include "Utilities/QtMessageBox.hpp"
#include "NetplaySessionDialog.hpp"
#include "NetplayCommon.hpp"

#include <QJsonDocument>
#include <QPushButton>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonArray>

#include <RMG-Core/Error.hpp>
#include <RMG-Core/Rom.hpp>

using namespace UserInterface::Dialog;
using namespace Utilities;

NetplaySessionDialog::NetplaySessionDialog(QWidget *parent, QWebSocket* webSocket, QJsonObject json, QString sessionFile) : QDialog(parent)
{
    this->setupUi(this);
    this->setWindowIcon(QIcon(":Resource/RMG.png"));
    this->setWindowFlags(this->windowFlags() | Qt::WindowMinimizeButtonHint);

    this->webSocket = webSocket;

    QJsonObject session = json.value("room").toObject();

    this->sessionJson = json.value("room").toObject();
    this->nickName    = json.value("player_name").toString();
    this->sessionPort = session.value("port").toInt();
    this->sessionName = session.value("room_name").toString();
    this->sessionFile = sessionFile;

    this->sessionNameLineEdit->setText(this->sessionName);
    this->gameNameLineEdit->setText(session.value("game_name").toString());

    connect(this->webSocket, &QWebSocket::textMessageReceived, this, &NetplaySessionDialog::on_webSocket_textMessageReceived);

    // reset json objects
    session = {};
    json    = {};

    // request server motd
    json.insert("type", "request_motd");
    json.insert("room_name", this->sessionName);
    NetplayCommon::AddCommonJson(json);
    webSocket->sendTextMessage(QJsonDocument(json).toJson());

    // request players
    json = {};
    session.insert("port", this->sessionPort);
    json.insert("type", "request_players");
    json.insert("room", session);
    NetplayCommon::AddCommonJson(json);
    webSocket->sendTextMessage(QJsonDocument(json).toJson());

    QPushButton* startButton = this->buttonBox->button(QDialogButtonBox::Ok);
    startButton->setText("Start");
    startButton->setEnabled(false);

    QPushButton* cheatsButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);
    cheatsButton->setText("Cheats");
    cheatsButton->setIcon(QIcon::fromTheme("code-box-line"));

    this->updateCheatsTreeWidget();
}

NetplaySessionDialog::~NetplaySessionDialog(void)
{
    if (this->webSocket->isValid())
    {
        this->webSocket->close();
    }
}

bool NetplaySessionDialog::getCheats(std::vector<CoreCheat>& cheats, QJsonArray& cheatsArray)
{
    QJsonObject session  = this->sessionJson;
    QJsonObject features = session.value("features").toObject();
    QString cheatJson    = features["cheats"].toString();

    if (cheatJson.isEmpty())
    {
        return false;
    }

    QJsonDocument cheatDocument = QJsonDocument::fromJson(cheatJson.toUtf8());
    cheatsArray = cheatDocument.array();

    if (!CheatsCommon::ParseCheatJson(cheatsArray, cheats))
    {
        QString error = "Failed to parse cheats json: " + QString(cheatDocument.toJson());
        QtMessageBox::Error(this, "CheatsCommon::ParseCheatJson() Failed", error);
        return false;
    }

    return true;
}

bool NetplaySessionDialog::applyCheats(void)
{
    std::vector<CoreCheat> cheats;
    QJsonArray cheatsArray;

    if (!this->getCheats(cheats, cheatsArray))
    {
        return false;
    }

    if (!CoreSetNetplayCheats(cheats))
    {
        QtMessageBox::Error(this, "CoreSetNetplayCheats() Failed", QString::fromStdString(CoreGetError()));
        return false;
    }

    return true;
}

void NetplaySessionDialog::updateCheatsTreeWidget(void)
{
    std::vector<CoreCheat> cheats;
    QJsonArray cheatsArray;

    if (!this->getCheats(cheats, cheatsArray))
    {
        // always clear UI on failure
        this->cheatsTreeWidget->clear();
        return;
    }

    CheatsCommon::AddCheatsToTreeWidget(true, cheatsArray, this->sessionFile, cheats, this->cheatsTreeWidget, true);    
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
            QJsonArray names = json.value("player_names").toArray();
            for (int i = 0; i < 4; i++)
            {
                name = names.at(i).toString();

                // add read-only item to UI
                QListWidgetItem* item = new QListWidgetItem();
                item->setText(name);
                item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
                this->listWidget->addItem(item);

                if (!name.isEmpty())
                {
                    bool nameMatch = this->nickName == name;
                    if (nameMatch)
                    {
                        this->sessionNumber = i + 1;
                    }
                    if (!this->started && i == 0)
                    {
                        QPushButton* startButton = this->buttonBox->button(QDialogButtonBox::Ok);
                        QPushButton* cheatsButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);
                        startButton->setEnabled(nameMatch);
                        cheatsButton->setEnabled(nameMatch);
                    }
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
        if (json.value("accept").toInt() == 0)
        {
            this->started = true;
            this->applyCheats();
            emit OnPlayGame(this->sessionFile, this->webSocket->peerAddress().toString(), this->sessionPort, this->sessionNumber);
        }
        else
        {
            // allow user to try again
            QPushButton* startButton = this->buttonBox->button(QDialogButtonBox::Ok);
            QPushButton* cheatsButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);
            startButton->setEnabled(true);
            cheatsButton->setEnabled(true);

            QtMessageBox::Error(this, "Server Error", json.value("message").toString());
        }
    }
    else if (type == "reply_motd")
    {
        QString message = "<b>MOTD:</b> " + json.value("message").toString();
        this->chatPlainTextEdit->appendHtml(message);
        this->chatPlainTextEdit->setTextInteractionFlags(Qt::TextBrowserInteraction);
    }
    else if (type == "reply_edit_room")
    {
        if (json.value("accept").toInt() == 0)
        {
            this->sessionJson = json.value("room").toObject();
            this->updateCheatsTreeWidget();
        }
        else
        {
            QtMessageBox::Error(this, "Server Error", json.value("message").toString());
        }
    }
}

void NetplaySessionDialog::on_chatLineEdit_textChanged(QString text)
{
    this->sendPushButton->setEnabled(!text.startsWith(' ') && !text.trimmed().isEmpty() && text.size() <= 256);
    this->sendPushButton->setDefault(this->sendPushButton->isEnabled());
}

void NetplaySessionDialog::on_sendPushButton_clicked(void)
{
    QJsonObject json;
    QJsonObject session;
    session.insert("port", this->sessionPort);
    json.insert("type", "request_chat_message");
    json.insert("player_name", this->nickName);
    json.insert("message", this->chatLineEdit->text());
    json.insert("room", session);
    NetplayCommon::AddCommonJson(json);

    this->webSocket->sendTextMessage(QJsonDocument(json).toJson());
    this->chatLineEdit->clear();
}

void NetplaySessionDialog::on_buttonBox_clicked(QAbstractButton* button)
{
    QPushButton* pushButton = (QPushButton*)button;
    QPushButton* cheatsButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);

    if (pushButton == cheatsButton)
    {
        QString cheatsJson = this->sessionJson.value("features").toObject().value("cheats").toString();
        QJsonDocument cheatsDocument = QJsonDocument::fromJson(cheatsJson.toUtf8());

        // show cheats dialog to user
        Dialog::CheatsDialog dialog(this, this->sessionFile, true, cheatsDocument.array());
        dialog.exec();

        // request an update to the existing session with the new cheats
        cheatsDocument.setArray(dialog.GetJson());
        QJsonObject session = this->sessionJson;
        QJsonObject features = session.value("features").toObject();
        if (!cheatsDocument.array().empty())
        { // only add cheats object when it's not empty
            features["cheats"] = QString(cheatsDocument.toJson(QJsonDocument::Compact));
        }
        else
        { // else remove it
            features.remove("cheats");
        }
        session.insert("features", features);

        // only request update when needed
        if (this->sessionJson != session)
        {
            QJsonObject json;
            json.insert("type", "request_edit_room");
            json.insert("player_name", this->nickName);
            json.insert("room", session);
            NetplayCommon::AddCommonJson(json);

            this->webSocket->sendTextMessage(QJsonDocument(json).toJson());
        }
    }
}

void NetplaySessionDialog::accept()
{
    QPushButton* startButton = this->buttonBox->button(QDialogButtonBox::Ok);
    QPushButton* cheatsButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);
    startButton->setEnabled(false);
    cheatsButton->setEnabled(false);

    QJsonObject json;
    QJsonObject session;
    session.insert("port", this->sessionPort);
    json.insert("type", "request_begin_game");
    json.insert("room", session);
    NetplayCommon::AddCommonJson(json);

    this->webSocket->sendTextMessage(QJsonDocument(json).toJson());
}

void NetplaySessionDialog::reject(void)
{
    // when we're in-game we should keep
    // the dialog alive so that the player
    // can re-open the dialog and continue chatting
    if (this->started)
    {
        this->hide();
        return;
    }

    if (this->webSocket->isValid())
    {
        this->webSocket->close();
    }

    QDialog::reject();
}
