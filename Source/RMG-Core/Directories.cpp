/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Directories.hpp"
#include "Error.hpp"

#include <cstdio>
#include <iostream>
#include <filesystem>

//
// Local Functions
//

static std::string get_var_directory(std::string var, std::string append, std::string fallbackVar, std::string fallbackAppend)
{
    std::string directory;

    const char* env = std::getenv(var.c_str());
    if (env == nullptr)
    {
        env = std::getenv(fallbackVar.c_str());
        if (env == nullptr)
        {
            std::cerr << "get_var_directory: fallbackVar: $" << fallbackVar << " cannot be nullptr!" << std::endl;
            std::terminate();
        }
        directory = env;
        directory += fallbackAppend;
    }
    else
    {
        directory = env;
        directory += append;
    }

    return directory;
}

//
// Exported Functions
//

bool CoreCreateDirectories(void)
{
    std::string error;

    std::string directories[] = 
    {
#ifdef PORTABLE_INSTALL
        CoreGetCoreDirectory(),
        CoreGetPluginDirectory(),
        CoreGetSharedDataDirectory(),
#endif // PORTABLE_INSTALL
        CoreGetUserConfigDirectory(),
        CoreGetUserDataDirectory(),
        CoreGetUserCacheDirectory(),
        CoreGetSaveDirectory(),
        CoreGetSaveStateDirectory()
    };

    for (const auto& dir : directories)
    {
        try
        {
            std::filesystem::create_directories(dir);
        }
        catch (...)
        {
            error = "CoreCreateDirectories Failed: cannot create the '";
            error += dir;
            error += "' directory!";
            CoreSetError(error);
            return false;
        }
    }

    return true;
}

std::string CoreGetCoreDirectory(void)
{
    std::string directory;
#ifdef PORTABLE_INSTALL
    directory = "Core";
#else // Not Portable
#ifdef _WIN32
    directory = "Core";
#else // Unix
    directory = "/usr/local/lib/RMG/Core";
#endif // _WIN32
#endif // PORTABLE_INSTALL
    return directory;
}

std::string CoreGetPluginDirectory(void)
{
    std::string directory;
#ifdef PORTABLE_INSTALL
    directory = "Plugin";
#else // Not Portable
#ifdef _WIN32
    directory = "Plugin";
#else // Unix
    directory = "/usr/local/lib/RMG/Plugin";
#endif // _WIN32
#endif // PORTABLE_INSTALL
    return directory;
}

std::string CoreGetUserConfigDirectory(void)
{
    std::string directory;
#ifdef PORTABLE_INSTALL
    directory = "Config";
#else // Not Portable
#ifdef _WIN32
    directory = get_var_directory("APPDATA", "/RMG/Config", "HOME", "/RMG/Config");
#else // Unix
    directory = get_var_directory("XDG_CONFIG_HOME", "/RMG", "HOME", "/.config/RMG");
#endif // _WIN32
#endif // PORTABLE_INSTALL
    return directory;
}

std::string CoreGetUserDataDirectory(void)
{
    std::string directory;
#ifdef PORTABLE_INSTALL
    directory = "Data";
#else // Not Portable
#ifdef _WIN32
    directory = get_var_directory("APPDATA", "/RMG/Data", "HOME", "/RMG/Data");
#else // Unix
    directory = get_var_directory("XDG_DATA_HOME", "/RMG", "HOME", "/.local/share/RMG");
#endif // _WIN32
#endif // PORTABLE_INSTALL
    return directory;
}

std::string CoreGetUserCacheDirectory(void)
{
    std::string directory;
#ifdef PORTABLE_INSTALL
    directory = "Cache";
#else // Not Portable
#ifdef _WIN32
    directory = get_var_directory("APPDATA", "/RMG/Cache", "HOME", "/RMG/Cache");
#else // Unix
    directory = get_var_directory("XDG_CACHE_HOME", "/RMG", "HOME", "/.cache/RMG");
#endif // _WIN32
#endif // PORTABLE_INSTALL
    return directory;
}

std::string CoreGetSharedDataDirectory(void)
{
    std::string directory;
#ifdef PORTABLE_INSTALL
    directory = "Data";
#else // Not Portable
#ifdef _WIN32
    directory = "Data";
#else // Unix
    directory = "/usr/share/RMG";
#endif // _WIN32
#endif // PORTABLE_INSTALL
    return directory;
}

std::string CoreGetSaveDirectory(void)
{
    std::string directory;
#ifdef PORTABLE_INSTALL
    directory = "Save/Game";
#else
    directory = CoreGetUserDataDirectory();
    directory += "/Save/Game";
#endif // PORTABLE_INSTALL
    return directory;
}

std::string CoreGetSaveStateDirectory(void)
{
    std::string directory;
#ifdef PORTABLE_INSTALL
    directory = "Save/State";
#else
    directory = CoreGetUserDataDirectory();
    directory += "/Save/State";
#endif // PORTABLE_INSTALL
    return directory;
}
