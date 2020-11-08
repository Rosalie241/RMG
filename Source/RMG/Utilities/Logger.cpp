/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Logger.hpp"

#include <QDir>

using namespace Utilities;

Logger::Logger(void)
{
}

Logger::~Logger(void)
{
    if (!this->init_Failed)
    {
        this->logfile.flush();
        this->logfile.close();
    }
}

bool Logger::Init(void)
{
    if (!QDir().exists(APP_LOG_DIR))
    {
        if (!QDir().mkdir(APP_LOG_DIR))
        {
            this->error_Message = "QDir::mkdir Failed";
            return false;
        }
    }

    this->logfile.setFileName(APP_LOG_FILE);

    if (!this->logfile.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        this->error_Message = "QFile::Open Failed";
        return false;
    }

    this->init_Failed = false;
    return true;
}

void Logger::AddText(QString text)
{
    if (this->init_Failed)
        return;

    if (text[text.size() - 1] != "\n")
        text += "\n";

    this->logfile.write(text.toUtf8());
    this->logfile.flush();
}

QString Logger::GetLastError()
{
    return this->error_Message;
}
