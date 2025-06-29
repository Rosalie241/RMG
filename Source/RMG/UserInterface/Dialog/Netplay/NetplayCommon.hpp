/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef NETPLAYCOMMON_HPP
#define NETPLAYCOMMON_HPP

#include <QJsonDocument>
#include <QJsonObject>
#ifdef NETPLAY
#include <QWebSocket>
#endif // NETPLAY
#include <QComboBox>
#include <QString>

namespace NetplayCommon
{
    #define NETPLAYCOMMON_SESSION_REGEX "[a-zA-Z0-9 ]+"
    #define NETPLAYCOMMON_NICKNAME_REGEX "[a-zA-Z0-9]+"
    #define NETPLAYCOMMON_PASSWORD_REGEX "[a-zA-Z0-9,.\\/<>?;:[\\]{}\\-=_+`~!@#$%^&*()]+"

    // Adds common json emulator and auth info
    void AddCommonJson(QJsonObject& json);

    // Retrieves RSP and GFX plugin names
    QList<QString> GetPluginNames(QString md5QString);

    // Adds servers from json to combobox
    void AddServers(QComboBox* comboBox, QJsonDocument document);

    // Restores previously selected server
    void RestoreSelectedServer(QComboBox* comboBox);

#ifdef NETPLAY
    // Attempts to connect the web socket to the given address using IPv4
    bool ConnectToIPv4Server(QString address, QWebSocket* webSocket);
#endif // #ifdef NETPLAY
}

#endif // NETPLAYCOMMON_HPP
