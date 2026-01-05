/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 * Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3.
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef KAILLERASESSIONMANAGER_HPP
#define KAILLERASESSIONMANAGER_HPP

#include <QObject>
#include <QString>
#include <QWidget>

//
// KailleraSessionManager
//
// Qt wrapper around Kaillera that bridges C callbacks to Qt signals
// Provides thread-safe interface for Kaillera integration
//
class KailleraSessionManager : public QObject
{
    Q_OBJECT

public:
    KailleraSessionManager(QWidget* parent = nullptr);
    ~KailleraSessionManager();

    // Show Kaillera's server selection dialog
    // Returns true if user connected to server, false if cancelled
    bool showServerDialog();

    // Get current game state
    bool isGameActive() const;
    QString getCurrentGame() const;
    int getPlayerNumber() const;
    int getTotalPlayers() const;

signals:
    // Emitted when a game starts (after joining/creating in Kaillera dialog)
    void gameStarted(QString gameName, int playerNum, int totalPlayers);

    // Emitted when a chat message is received
    void chatReceived(QString nickname, QString message);

    // Emitted when a player disconnects
    void playerDropped(QString nickname, int playerNum);

    // Emitted when the game ends
    void gameEnded();

public slots:
    // Send a chat message to other players
    void sendChatMessage(QString message);

    // End the current game session
    void endGame();

private:
    // Callback handlers (invoked from Kaillera thread via QMetaObject::invokeMethod)
    void handleGameStart(QString game, int player, int numPlayers);
    void handleChatReceived(QString nick, QString text);
    void handlePlayerDropped(QString nick, int playerNum);

    // State variables
    bool m_gameActive;
    QString m_currentGame;
    int m_playerNumber;
    int m_totalPlayers;
    QWidget* m_parentWidget;
};

#endif // KAILLERASESSIONMANAGER_HPP
