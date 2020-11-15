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
    SettingsID id;
    QString settingValue;
    for (const Plugin_t &p : this->GetAvailablePlugins())
    {
        switch (p.Type)
        {
        default:
        case PluginType::Gfx:
            id = SettingsID::Core_GFX_Plugin;
            break;
        case PluginType::Rsp:
            id = SettingsID::Core_RSP_Plugin;
            break;
        case PluginType::Audio:
            id = SettingsID::Core_AUDIO_Plugin;
            break;
        case PluginType::Input:
            id = SettingsID::Core_INPUT_Plugin;
            break;
        }

        settingValue = g_Settings.GetStringValue(id);
        if (settingValue == p.FileName || settingValue.isEmpty())
            this->ChangePlugin(p);
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
    bool ret = false;
    ret = g_MupenApi.Core.SetPlugin(plugin);

    if (ret)
    {
        SettingsID id;
        switch (plugin.Type)
        {
        default:
        case PluginType::Gfx:
            id = SettingsID::Core_GFX_Plugin;
            break;
        case PluginType::Rsp:
            id = SettingsID::Core_RSP_Plugin;
            break;
        case PluginType::Audio:
            id = SettingsID::Core_AUDIO_Plugin;
            break;
        case PluginType::Input:
            id = SettingsID::Core_INPUT_Plugin;
            break;
        }

        g_Settings.SetValue(id, plugin.FileName);
    }

    return ret;
}

Plugin_t Plugins::GetCurrentPlugin(PluginType type)
{
    Plugin_t plugin = {0};
    g_MupenApi.Core.GetCurrentPlugin(type, &plugin);
    return plugin;
}