/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef M64P_WRAPPER_API
#define M64P_WRAPPER_API

#include "Core.hpp"
#include "Config.hpp"
#include "Plugin.hpp"
#include "Types.hpp"

#include <QString>
#include <QObject>
#include <QOpenGLWidget>

namespace M64P
{
    namespace Wrapper
    {
        class Api : public QObject
        {
            Q_OBJECT

        public:
            Api(void);
            ~Api(void);

            M64P::Wrapper::Core Core;
            M64P::Wrapper::Config Config;

            bool Init(QString);

            QString GetLastError(void);

        private:
            QString error_Message;

            m64p_dynlib_handle core_Handle;
            bool core_Handle_Opened;
            bool core_Handle_Open(QString);
            void core_Handle_Close(void);
            bool core_VidExt_Override(void);
        };
    } // namespace Wrapper
} // namespace M64P

#endif // M64P_WRAPPER_API
