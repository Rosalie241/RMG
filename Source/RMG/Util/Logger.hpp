/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <Config.hpp>

#include <QString>
#include <QFile>

namespace Util
{
    class Logger
    {
    public:
        Logger(void);
        ~Logger(void);

        bool Init(void);

        void AddText(QString);

        QString GetLastError(void);

    private:
        bool init_Failed = true;

        QString error_Message;

        QFile logfile;
    };

} // namespace Util

extern Util::Logger g_Logger;

#endif // LOGGER_HPP