/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef DIRECTORIES_HPP
#define DIRECTORIES_HPP

#include <string>

// tries to create the needed directories,
// returns false when failed
bool CoreCreateDirectories(void);

// returns the core directory where
// the mupen64plus library should be
// located
std::string CoreGetCoreDirectory(void);

// returns the plugin directory where
// plugins should be located
std::string CoreGetPluginDirectory(void);

// returns the user config directory where
// configuration files should be located
std::string CoreGetUserConfigDirectory(void);

// returns the user data directory where
// data files should be located
std::string CoreGetUserDataDirectory(void);

// returns the user cache directory where
// cache files should be located
std::string CoreGetUserCacheDirectory(void);

// returns the shared data directory where
// shared data files should be located
std::string CoreGetSharedDataDirectory(void);

// returns the save directory where
// game saves should be located
std::string CoreGetSaveDirectory(void);

// returns the save state directory where
// save states should be located
std::string CoreGetSaveStateDirectory(void);

#endif // DIRECTORIES_HPP
