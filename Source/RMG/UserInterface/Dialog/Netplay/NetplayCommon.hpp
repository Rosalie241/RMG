/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef NETPLAYCOMMON_HPP
#define NETPLAYCOMMON_HPP

#include <QJsonObject>
#include <QJsonArray>
#include <QString>

#include <RMG-Core/m64p/api/m64p_types.h>

namespace NetplayCommon
{
    #define NETPLAYCOMMON_SESSION_REGEX "[a-zA-Z0-9 ]+"
    #define NETPLAYCOMMON_NICKNAME_REGEX "[a-zA-Z0-9]+"
    #define NETPLAYCOMMON_PASSWORD_REGEX "[a-zA-Z0-9,.\\/<>?;:[\\]{}\\-=_+`~!@#$%^&*()]+"

    // Adds common json emulator and auth info
    void AddCommonJson(QJsonObject& json);

    // Retrieves RSP and GFX plugin names
    QList<QString> GetPluginNames(QString md5QString);
}

m64p_error loadROM(QString filename);

#endif // NETPLAYCOMMON_HPP
