#include "NetplaySessionDialog.hpp"
#include "NetplayCommon.hpp"
#include "Utilities/QtMessageBox.hpp"

#include <QJsonDocument>
#include <QPushButton>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonArray>

#include <RMG-Core/Core.hpp>

using namespace UserInterface::Dialog;
using namespace Utilities;

NetplaySessionDialog::NetplaySessionDialog(QWidget *parent, QWebSocket* webSocket, QJsonObject json, QString sessionFile, QJsonArray cheats) : QDialog(parent)
{
    this->setupUi(this);

    this->webSocket = webSocket;
    this->cheats = cheats;
    
    QJsonObject session = json.value("room").toObject();

    this->nickName    = json.value("player_name").toString();
    this->sessionPort = session.value("port").toInt();
    this->sessionName = session.value("room_name").toString();
    this->sessionFile = sessionFile;

    // Store cheats
    QJsonObject featuresObject = session.value("features").toObject();
    QString cheatsString = featuresObject.value("cheats").toString();
    QJsonDocument cheatsDoc = QJsonDocument::fromJson(cheatsString.toUtf8());
    if (cheatsDoc.isArray()) {
        this->cheats = cheatsDoc.array();
    }

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
}

NetplaySessionDialog::~NetplaySessionDialog(void)
{
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
        emit OnPlayGame(this->sessionFile, this->webSocket->peerAddress().toString(), this->sessionPort, this->sessionNumber, this->cheats);
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

    webSocket->sendTextMessage(QJsonDocument(json).toJson());
    this->chatLineEdit->clear();
}

void NetplaySessionDialog::accept()
{
    QPushButton* startButton = this->buttonBox->button(QDialogButtonBox::Ok);
    startButton->setEnabled(false);

    QJsonObject json;
    QJsonObject session;
    session.insert("port", this->sessionPort);
    json.insert("type", "request_begin_game");
    json.insert("room", session);
    NetplayCommon::AddCommonJson(json);

    webSocket->sendTextMessage(QJsonDocument(json).toJson());
}