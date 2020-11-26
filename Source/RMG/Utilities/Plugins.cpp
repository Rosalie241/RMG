/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Plugins.hpp"
#include "../Globals.hpp"
#include "Utilities/SettingsID.hpp"

#include <QDir>

using namespace Utilities;

Plugins::Plugins()
{
}

Plugins::~Plugins()
{
}

void Plugins::LoadSettings()
{
    SettingsID settingIds[]{SettingsID::Core_GFX_Plugin, SettingsID::Core_RSP_Plugin, SettingsID::Core_AUDIO_Plugin,
                            SettingsID::Core_INPUT_Plugin};

    QString settingValue;
    for (const Plugin_t &p : this->GetAvailablePlugins())
    {
        settingValue = g_Settings.GetStringValue(settingIds[(int)p.Type]);
        if (settingValue == p.FileName || settingValue.isEmpty())
        {
            this->ChangePlugin(p);
        }
    }
}

QList<Plugin_t> Plugins::GetAvailablePlugins()
{
    QList<Plugin_t> plugins;

    plugins.append(g_MupenApi.Core.GetPlugins(PluginType::Gfx));
    plugins.append(g_MupenApi.Core.GetPlugins(PluginType::Rsp));
    plugins.append(g_MupenApi.Core.GetPlugins(PluginType::Audio));
    plugins.append(g_MupenApi.Core.GetPlugins(PluginType::Input));

    return plugins;
}

bool Plugins::ChangePlugin(Plugin_t plugin)
{
    SettingsID settingIds[]{SettingsID::Core_GFX_Plugin, SettingsID::Core_RSP_Plugin, SettingsID::Core_AUDIO_Plugin,
                            SettingsID::Core_INPUT_Plugin};
    bool ret;

    ret = g_MupenApi.Core.SetPlugin(plugin);
    if (ret)
    {
        g_Settings.SetValue(settingIds[(int)plugin.Type], plugin.FileName);
    }

    return ret;
}

Plugin_t Plugins::GetCurrentPlugin(PluginType type)
{
    Plugin_t plugin = {0};
    g_MupenApi.Core.GetCurrentPlugin(type, &plugin);
    return plugin;
}