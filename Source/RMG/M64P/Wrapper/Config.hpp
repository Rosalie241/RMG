/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef M64P_WRAPPER_CONFIG_HPP
#define M64P_WRAPPER_CONFIG_HPP

#include <M64P/ConfigApi.hpp>

#include <QString>

namespace M64P
{
    namespace Wrapper
    {
        class Config
        {
        public:
            Config(void);
            ~Config(void);

            bool Init();

            bool SetOption(QString section, QString key, int value);
            bool SetOption(QString section, QString key, float value);
            bool SetOption(QString section, QString key, bool value);
            bool SetOption(QString section, QString key, QString value);
            bool SetOption(QString section, QString key, char* value);

            bool GetOption(QString section, QString key, int* value);
            bool GetOption(QString section, QString key, float* value);
            bool GetOption(QString section, QString key, bool* value);
            bool GetOption(QString section, QString key, QString* value);

            QString GetLastError(void);

        private:
            QString error_Message;

            m64p_handle handle;
            m64p_handle section_Handle;

            bool section_Open(QString);
            bool value_Set(QString, m64p_type, void*);
            bool value_Get(QString, m64p_type, void*, int);
        };
    } // namespace Wrapper
} // namespace M64P

#endif // M64P_WRAPPER_CONFIG_HPP
