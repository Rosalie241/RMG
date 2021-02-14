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
#include <QDirIterator>

using namespace Utilities;

Plugins::Plugins()
{
}

Plugins::~Plugins()
{
}

void Plugins::LoadSettings()
{
    QString settingValue;
    SettingsID settingsID;

    this->FindAvailablePlugins();

    for (Plugin *p : this->GetAvailablePlugins())
    {
        settingsID = this->pluginTypeToSettingsId(p->GetType());
        settingValue = g_Settings.GetStringValue(settingsID);
        if (settingValue == p->GetFileName() || settingValue.isEmpty())
        {
            this->ChangePlugin(p);
        }
    }
}

#include <iostream>
void Plugins::FindAvailablePlugins(void)
{
    QDirIterator pluginIt("Plugin", QStringList() << "*." SO_EXT, QDir::Files, QDirIterator::Subdirectories);

    for (Plugin *p : this->plugins)
    {
        if (p != nullptr)
        {
            p->Shutdown();
            delete p;
        }
    }

    this->plugins.clear();

    std::cout << "aaaa: " << std::endl;

    while (pluginIt.hasNext())
    {
        QString pluginFileName = pluginIt.next();

        std::cout << "FindAvailablePlugins: " << pluginFileName.toStdString() << std::endl;
        Plugin *plugin = new Plugin();

        if (!plugin->Init(pluginFileName, g_MupenApi.Core.GetHandle()))
        {
            plugin->Shutdown();
            std::cout << "Plugin INIT FAILED!!" << std::endl;
            continue;
        }

        this->plugins.append(plugin);
    }
}

QList<Plugin *> Plugins::GetAvailablePlugins(void)
{
    return this->plugins;
}

#include <iostream>
bool Plugins::ChangePlugin(Plugin *plugin)
{
    Plugin *oldPlugin;

    std::cout << "ChangePlugin" << std::endl;

    if (plugin == nullptr)
    {
        std::cout << "ChangePlugin: plugin == nullptr!" << std::endl;
        return false;
    }

    if (!plugin->Startup())
    {
        std::cout << "ChangePlugin: !plugin->Startup()!" << std::endl;
        std::cout << plugin->GetLastError().toStdString() << std::endl;
        return false;
    }

    // pointersssssss
    // aaaaaaaaaaaaaaaaaaaaa

    switch (plugin->GetType())
    {
    default:
    case m64p_plugin_type::M64PLUGIN_GFX:
        this->plugin_Gfx = plugin;
        break;
    case m64p_plugin_type::M64PLUGIN_AUDIO:
        this->plugin_Audio = plugin;
        oldPlugin = this->plugin_Audio;
        break;
    case m64p_plugin_type::M64PLUGIN_INPUT:
        this->plugin_Input = plugin;
        oldPlugin = this->plugin_Input;
        break;
    case m64p_plugin_type::M64PLUGIN_RSP:
        this->plugin_Rsp = plugin;
        oldPlugin = this->plugin_Rsp;
        break;
    }

  /*  if (oldPlugin != nullptr)
    {
        oldPlugin->Shutdown();
    } 

    oldPlugin = plugin; */

    if (plugin == nullptr)
    {
        std::cout << "Plugins::ChangePlugin plugin is NULLPTRA ASDASDASD" << std::endl;
    }

    g_MupenApi.Core.AttachPlugin(plugin);

    return true;
}

Plugin *Plugins::GetCurrentPlugin(PluginType type)
{
    Plugin *plugin;

    switch (type)
    {
    default:
    case PluginType::Gfx:
        plugin = this->plugin_Gfx;
        break;
    case PluginType::Audio:
        plugin = this->plugin_Audio;
        break;
    case PluginType::Input:
        plugin = this->plugin_Input;
        break;
    case PluginType::Rsp:
        plugin = this->plugin_Rsp;
        break;
    }

    return plugin;
}

SettingsID Plugins::pluginTypeToSettingsId(m64p_plugin_type type)
{
    SettingsID id;

    switch (type)
    {
    default:
    case m64p_plugin_type::M64PLUGIN_GFX:
        id = SettingsID::Core_GFX_Plugin;
        break;
    case m64p_plugin_type::M64PLUGIN_AUDIO:
        id = SettingsID::Core_AUDIO_Plugin;
        break;
    case m64p_plugin_type::M64PLUGIN_INPUT:
        id = SettingsID::Core_INPUT_Plugin;
        break;
    case m64p_plugin_type::M64PLUGIN_RSP:
        id = SettingsID::Core_RSP_Plugin;
        break;
    }

    return id;
}
