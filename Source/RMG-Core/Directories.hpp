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

#include <filesystem>

#ifdef _WIN32
#define CORE_DIR_SEPERATOR_STR "\\"
#else // Unix
#define CORE_DIR_SEPERATOR_STR "/"
#endif // _WIN32

// tries to create the needed directories,
// returns false when failed
bool CoreCreateDirectories(void);

// returns whether we're currently in portable
// directory mode or not
bool CoreGetPortableDirectoryMode(void);

// returns the core directory where
// the mupen64plus library should be
// located
std::filesystem::path CoreGetCoreDirectory(void);

// returns the library directory where
// misc libraries should be located
std::filesystem::path CoreGetLibraryDirectory(void);

// returns the plugin directory where
// plugins should be located
std::filesystem::path CoreGetPluginDirectory(void);

// returns the default user data directory where
// data files should be located
std::filesystem::path CoreGetDefaultUserDataDirectory(void);

// returns the default user cache directory where
// cache files should be located
std::filesystem::path CoreGetDefaultUserCacheDirectory(void);

// returns the default save directory where
// game saves should be located
std::filesystem::path CoreGetDefaultSaveDirectory(void);

// returns the default save state directory where
// save states should be located
std::filesystem::path CoreGetDefaultSaveStateDirectory(void);

// returns the default screenshot directory where
// screenshots should be located
std::filesystem::path CoreGetDefaultScreenshotDirectory(void);

// returns the user config directory where
// configuration files should be located
std::filesystem::path CoreGetUserConfigDirectory(void);

// returns the user data directory where
// data files should be located
std::filesystem::path CoreGetUserDataDirectory(void);

// returns the user cache directory where
// cache files should be located
std::filesystem::path CoreGetUserCacheDirectory(void);

// returns the shared data directory where
// shared data files should be located
std::filesystem::path CoreGetSharedDataDirectory(void);

// returns the save directory where
// game saves should be located
std::filesystem::path CoreGetSaveDirectory(void);

// returns the save state directory where
// save states should be located
std::filesystem::path CoreGetSaveStateDirectory(void);

// returns the screenshot directory where
// screenshots should be located
std::filesystem::path CoreGetScreenshotDirectory(void);

// changes the library path to the given path
void CoreSetLibraryPathOverride(std::filesystem::path path);

// changes the core path to the given path
void CoreSetCorePathOverride(std::filesystem::path path);

// changes the plugin path to the given path
void CoreSetPluginPathOverride(std::filesystem::path path);

// changes the shared data path to the given path
void CoreSetSharedDataPathOverride(std::filesystem::path path);

#endif // DIRECTORIES_HPP
