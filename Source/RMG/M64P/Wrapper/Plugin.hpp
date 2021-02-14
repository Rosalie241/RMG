/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef M64P_WRAPPER_PLUGIN_HPP
#define M64P_WRAPPER_PLUGIN_HPP

#include "../PluginApi.hpp"
#include "../api/m64p_types.h"
#include "Types.hpp"

#include <QString>

namespace M64P
{
namespace Wrapper
{
class Plugin
{
  public:
    Plugin(void);
    ~Plugin(void);

    bool Init(QString, m64p_dynlib_handle);

    bool Startup(void);
    bool Shutdown(void);

    bool HasConfig(void);
    bool OpenConfig(void);

    QString GetName(void);
    QString GetFileName(void);
    int GetVersion(void);
    int GetApiVersion(void);
    int GetCapabilities(void);

    m64p_dynlib_handle GetHandle(void);
    m64p_plugin_type GetType(void);

    QString GetLastError(void);

  private:
    QString error_Message;

    QString name;
    QString fileName;
    int version;
    int apiVersion;
    int capabilities;

    m64p_dynlib_handle handle;
    m64p_plugin_type type;

    m64p_dynlib_handle coreHandle;

    M64P::PluginApi plugin;
};
} // namespace Wrapper
} // namespace M64P

#endif // M64P_WRAPPER_PLUGIN_HPP
