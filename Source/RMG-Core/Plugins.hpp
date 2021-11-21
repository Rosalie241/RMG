/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_PLUGINS_HPP
#define CORE_PLUGINS_HPP

#include <string>
#include <vector>

enum class CorePluginType
{
    Rsp = 1,
    Gfx = 2,
    Audio = 3,
    Input = 4,
    Invalid,
};

struct CorePlugin
{
    std::string    File;
    std::string    Name;
    CorePluginType Type;
};

// retrieves all available plugins
std::vector<CorePlugin> CoreGetAllPlugins(void);

// applies updated plugin settings,
// i.e when you've changed a plugin
bool CoreApplyPluginSettings(void);

// applies ROM plugin settings,
// i.e when you launch a ROM
bool CoreApplyRomPluginSettings(void);

// checks wether all plugins are
// hooked and ready for emulation
bool CoreArePluginsReady(void);

// returns wether the currently used plugin
// of given type has a config GUI
bool CorePluginsHasConfig(CorePluginType type);

// opens the config GUI for the currently
// used plugin of given type
bool CorePluginsOpenConfig(CorePluginType type);

// attaches all used plugins
bool CoreAttachPlugins(void);

// detaches all used plugins
bool CoreDetachPlugins(void);

// shuts down all currently used plugins
bool CorePluginsShutdown(void);

#endif // CORE_PLUGINS_HPP
