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
#include "Core.hpp"
#include "Config.hpp"
#include "m64p/Api.hpp"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <filesystem>

//
// Local Variables
//

static std::filesystem::path l_LibraryPathOverride;
static std::filesystem::path l_CorePathOverride;
static std::filesystem::path l_PluginPathOverride;
static std::filesystem::path l_SharedDataPathOverride;

//
// Local Functions
//

#ifndef PORTABLE_INSTALL
static std::filesystem::path get_var_directory(std::string var, std::string append, std::string fallbackVar, std::string fallbackAppend)
{
    std::filesystem::path directory;

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

static std::filesystem::path get_command_output(std::string command)
{
    std::string output;
    char buf[2048];
    FILE* pipe = nullptr;

    pipe = popen(command.c_str(), "r");
    if (pipe == nullptr)
    {
        return std::string();
    }

    while (fgets(buf, sizeof(buf), pipe) != nullptr)
    {
        output += buf;
    }

    pclose(pipe);

    // strip newline
    if (output.back() == '\n')
    {
        output.pop_back();
    }

    return output;
}
#endif // PORTABLE_INSTALL

//
// Exported Functions
//

bool CoreCreateDirectories(void)
{
    std::string error;

    std::filesystem::path directories[] = 
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
        CoreGetSaveStateDirectory(),
        CoreGetScreenshotDirectory()
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
            error += dir.string();
            error += "' directory!";
            CoreSetError(error);
            return false;
        }
    }

    return true;
}

std::filesystem::path CoreGetLibraryDirectory(void)
{
    std::filesystem::path directory;
#ifdef PORTABLE_INSTALL
    directory = ".";
#else // Not Portable
    if (!l_LibraryPathOverride.empty())
    {
        directory = l_LibraryPathOverride;
    }
    else
    {
        directory = CORE_INSTALL_PREFIX;
        directory += "/lib/RMG/";
    }
#endif // PORTABLE_INSTALL
    return directory;
}

std::filesystem::path CoreGetCoreDirectory(void)
{
    std::filesystem::path directory;
#ifdef PORTABLE_INSTALL
    directory = "Core";
#else // Not Portable
    if (!l_CorePathOverride.empty())
    {
        directory = l_CorePathOverride;
    }
    else
    {
        directory = CORE_INSTALL_PREFIX;
        directory += "/lib/RMG/Core";
    }
#endif // PORTABLE_INSTALL
    return directory;
}

std::filesystem::path CoreGetPluginDirectory(void)
{
    std::filesystem::path directory;
#ifdef PORTABLE_INSTALL
    directory = "Plugin";
#else // Not Portable
    if (!l_PluginPathOverride.empty())
    {
        directory = l_PluginPathOverride;
    }
    else
    {
        directory = CORE_INSTALL_PREFIX;
        directory += "/lib/RMG/Plugin";
    }
#endif // PORTABLE_INSTALL
    return directory;
}

std::filesystem::path CoreGetUserConfigDirectory(void)
{
    std::filesystem::path directory;
#ifdef PORTABLE_INSTALL
    directory = "Config";
#else // Not Portable
    directory = get_var_directory("XDG_CONFIG_HOME", "/RMG", "HOME", "/.config/RMG");
#endif // PORTABLE_INSTALL
    return directory;
}

std::filesystem::path CoreGetDefaultUserDataDirectory(void)
{
    std::filesystem::path directory;
#ifdef PORTABLE_INSTALL
    directory = "Data";
#else // Not Portable
    directory = get_var_directory("XDG_DATA_HOME", "/RMG", "HOME", "/.local/share/RMG");
#endif // PORTABLE_INSTALL
    return directory;
}

std::filesystem::path CoreGetDefaultUserCacheDirectory(void)
{
    std::filesystem::path directory;
#ifdef PORTABLE_INSTALL
    directory = "Cache";
#else // Not Portable
    directory = get_var_directory("XDG_CACHE_HOME", "/RMG", "HOME", "/.cache/RMG");
#endif // PORTABLE_INSTALL
    return directory;
}

std::filesystem::path CoreGetDefaultSaveDirectory(void)
{
    std::filesystem::path directory;
#ifdef PORTABLE_INSTALL
    directory = "Save/Game";
#else // Not Portable
    directory = CoreGetDefaultUserDataDirectory();
    directory += "/Save/Game";
#endif // PORTABLE_INSTALL
    return directory;
}

std::filesystem::path CoreGetDefaultSaveStateDirectory(void)
{
    std::filesystem::path directory;
#ifdef PORTABLE_INSTALL
    directory = "Save/State";
#else // Not Portable
    directory = CoreGetDefaultUserDataDirectory();
    directory += "/Save/State";
#endif // PORTABLE_INSTALL
    return directory;
}

std::filesystem::path CoreGetDefaultScreenshotDirectory(void)
{
    std::filesystem::path directory;
#ifdef PORTABLE_INSTALL
    directory = "Screenshots";
#else // Not Portable
    directory = get_command_output("xdg-user-dir PICTURES");
    if (!directory.empty())
    {
        directory += "/RMG";
    }
    else
    {
        directory = get_var_directory("XDG_PICTURES_DIR", "/RMG", "HOME", "/Pictures/RMG");
    }
#endif // PORTABLE_INSTALL
    return directory;
}

std::filesystem::path CoreGetUserDataDirectory(void)
{
    return std::filesystem::path(m64p::Config.GetUserDataPath());
}

std::filesystem::path CoreGetUserCacheDirectory(void)
{
    return std::filesystem::path(m64p::Config.GetUserCachePath());
}

std::filesystem::path CoreGetSharedDataDirectory(void)
{
    std::filesystem::path directory;
#ifdef PORTABLE_INSTALL
    directory = "Data";
#else // Not Portable
    if (!l_SharedDataPathOverride.empty())
    {
        directory = l_SharedDataPathOverride;
    }
    else
    {
        directory = CORE_INSTALL_PREFIX;
        directory += "/share/RMG";
    }
#endif // PORTABLE_INSTALL
    return directory;
}

std::filesystem::path CoreGetSaveDirectory(void)
{
    return CoreSettingsGetStringValue(SettingsID::Core_SaveSRAMPath);
}

std::filesystem::path CoreGetSaveStateDirectory(void)
{
    return CoreSettingsGetStringValue(SettingsID::Core_SaveStatePath);
}

std::filesystem::path CoreGetScreenshotDirectory(void)
{
    return CoreSettingsGetStringValue(SettingsID::Core_ScreenshotPath);
}

#ifndef PORTABLE_INSTALL
void CoreSetLibraryPathOverride(std::filesystem::path path)
{
    l_LibraryPathOverride = path;
}

void CoreSetCorePathOverride(std::filesystem::path path)
{
    l_CorePathOverride = path;
}

void CoreSetPluginPathOverride(std::filesystem::path path)
{
    l_PluginPathOverride = path;
}

void CoreSetSharedDataPathOverride(std::filesystem::path path)
{
    l_SharedDataPathOverride = path;
}
#endif // PORTABLE_INSTALL
