/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "NetplayCommon.hpp"

#include <QCryptographicHash>
#include <QByteArray>
#include <QFileInfo>
#include <QDateTime>
#include <QProcess>
#include <QFile>

#include <RMG-Core/Core.hpp>
#include <RMG-Core/m64p/Api.hpp>

using namespace NetplayCommon;

void NetplayCommon::AddCommonJson(QJsonObject& json)
{
    QCryptographicHash hash = QCryptographicHash(QCryptographicHash::Sha256);
    QByteArray currentTime = QByteArray::number(QDateTime::currentMSecsSinceEpoch());
    hash.addData(currentTime);
    hash.addData(QStringLiteral("RMG").toUtf8());

    json.insert("client_sha", QString::fromStdString(CoreGetVersion()));
    json.insert("emulator", "RMG");
    json.insert("auth", QString(hash.result().toHex()));
    json.insert("authTime", QString(currentTime));
    json.insert("netplay_version", 17);
}

QList<QString> NetplayCommon::GetPluginNames(QString md5QString)
{
    QList<QString> pluginNames(2);
    const std::string md5 = md5QString.toStdString();
    const std::vector<CorePlugin> pluginList = CoreGetAllPlugins();
    const SettingsID settingsId[] =
    {
        SettingsID::Core_RSP_Plugin, SettingsID::Core_GFX_Plugin, 
    };
    const SettingsID gameSettingsId[] =
    {
        SettingsID::Game_RSP_Plugin, SettingsID::Game_GFX_Plugin, 
    };
    QString pluginFileNames[4];
    QString pluginFileName;

    int index = 0;

    for (int i = 0; i < 2; i++)
    {
        pluginFileName = QString::fromStdString(CoreSettingsGetStringValue(gameSettingsId[i], md5));
        if (pluginFileName.isEmpty())
        { // fallback to main plugin settings
            pluginFileName = QString::fromStdString(CoreSettingsGetStringValue(settingsId[i]));
        }

        if (!pluginFileName.isEmpty())
        {
            // account for full path (<v0.3.5 we used the full path)
            pluginFileName = QFileInfo(pluginFileName).fileName();
            pluginFileNames[i] = pluginFileName;
        }
    }

    for (const auto &p : pluginList)
    {
        index = ((int)p.Type - 1);

        if (pluginFileNames[index] == QString::fromStdString(p.File))
        {
            pluginNames[index] = QString::fromStdString(p.Name);
        }
    }

    return pluginNames;
}

m64p_error loadROM(QString filename)
{
    char *ROM_buffer = NULL;
    size_t romlength = 0;

    if (filename.endsWith(".7z") || filename.endsWith(".zip") || filename.endsWith(".ZIP"))
    {
        QProcess process;
        process.setProgram("7za");
        QStringList arguments = { "e", "-so", filename, "*64"  };
        process.setArguments(arguments);
        process.start();
        process.waitForFinished(-1);
        QByteArray data = process.readAllStandardOutput();
        romlength = data.size();
        if (romlength == 0)
        {
            printf("couldn't open file '%s' for reading.", filename.toUtf8().constData());
            return M64ERR_INVALID_STATE;
        }
        ROM_buffer = (char *) malloc(romlength);
        memcpy(ROM_buffer, data.constData(), romlength);
    }
    else
    {
        /* load ROM image */
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly))
        {
            printf("couldn't open ROM file '%s' for reading.", filename.toUtf8().constData());
            return M64ERR_INVALID_STATE;
        }

        romlength = file.size();
        QDataStream in(&file);
        ROM_buffer = (char *) malloc(romlength);
        if (in.readRawData(ROM_buffer, romlength) == -1)
        {
            printf("couldn't read %li bytes from ROM image file '%s'.", romlength, filename.toUtf8().constData());
            free(ROM_buffer);
            file.close();
            return M64ERR_INVALID_STATE;
        }
        file.close();
    }

    /* Try to load the ROM image into the core */
    if (m64p::Core.DoCommand(M64CMD_ROM_OPEN, (int) romlength, ROM_buffer) != M64ERR_SUCCESS)
    {
        printf("core failed to open ROM image file '%s'.", filename.toUtf8().constData());
        free(ROM_buffer);
        return M64ERR_INVALID_STATE;
    }
    free(ROM_buffer); /* the core copies the ROM image, so we can release this buffer immediately */

    return M64ERR_SUCCESS;
}