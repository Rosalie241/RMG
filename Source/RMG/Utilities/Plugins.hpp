/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef PLUGINS_HPP
#define PLUGINS_HPP

#include "M64P/Wrapper/Types.hpp"
#include "M64P/Wrapper/Plugin.hpp"
#include "SettingsID.hpp"

#include <QList>

using namespace M64P::Wrapper;

namespace Utilities
{
class Plugins
{
  public:
    Plugins();
    ~Plugins();

    void LoadSettings();

    void FindAvailablePlugins(void);

    QList<Plugin *> GetAvailablePlugins(void);
    QList<Plugin *> GetAvailablePlugins(PluginType);

    bool ChangePlugin(Plugin *);
    Plugin *GetCurrentPlugin(PluginType);

  private:
    Plugin *plugin_Rsp = nullptr;
    Plugin *plugin_Gfx = nullptr;
    Plugin *plugin_Audio = nullptr;
    Plugin *plugin_Input = nullptr;

    QList<Plugin *> plugins;

    SettingsID pluginTypeToSettingsId(m64p_plugin_type);
};
}; // namespace Utilities

#endif // PLUGINS_HPP
