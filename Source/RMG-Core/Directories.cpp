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
#include "Config.hpp"
#include "Error.hpp"
#include "Core.hpp"
#include "m64p/Api.hpp"

#include <cstdio>
#include <cstdlib>
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

static std::string get_command_output(std::string command)
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
    directory = CORE_INSTAlL_PREFIX;
    directory += "/lib/RMG/Core";
#endif // PORTABLE_INSTALL
    return directory;
}

std::string CoreGetPluginDirectory(void)
{
    std::string directory;
#ifdef PORTABLE_INSTALL
    directory = "Plugin";
#else // Not Portable
    directory = CORE_INSTAlL_PREFIX;
    directory += "/lib/RMG/Plugin";
#endif // PORTABLE_INSTALL
    return directory;
}

std::string CoreGetUserConfigDirectory(void)
{
    std::string directory;
#ifdef PORTABLE_INSTALL
    directory = "Config";
#else // Not Portable
    directory = get_var_directory("XDG_CONFIG_HOME", "/RMG", "HOME", "/.config/RMG");
#endif // PORTABLE_INSTALL
    return directory;
}

std::string CoreGetDefaultUserDataDirectory(void)
{
    std::string directory;
#ifdef PORTABLE_INSTALL
    directory = "Data";
#else // Not Portable
    directory = get_var_directory("XDG_DATA_HOME", "/RMG", "HOME", "/.local/share/RMG");
#endif // PORTABLE_INSTALL
    return directory;
}

std::string CoreGetDefaultUserCacheDirectory(void)
{
    std::string directory;
#ifdef PORTABLE_INSTALL
    directory = "Cache";
#else // Not Portable
    directory = get_var_directory("XDG_CACHE_HOME", "/RMG", "HOME", "/.cache/RMG");
#endif // PORTABLE_INSTALL
    return directory;
}

std::string CoreGetDefaultSaveDirectory(void)
{
    std::string directory;
#ifdef PORTABLE_INSTALL
    directory = "Save/Game";
#else // Not Portable
    directory = CoreGetUserDataDirectory();
    directory += "/Save/Game";
#endif // PORTABLE_INSTALL
    return directory;
}

std::string CoreGetDefaultSaveStateDirectory(void)
{
    std::string directory;
#ifdef PORTABLE_INSTALL
    directory = "Save/State";
#else // Not Portable
    directory = CoreGetUserDataDirectory();
    directory += "/Save/State";
#endif // PORTABLE_INSTALL
    return directory;
}

std::string CoreGetDefaultScreenshotDirectory(void)
{
    std::string directory;
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

std::string CoreGetUserDataDirectory(void)
{
    return std::string(m64p::Config.GetUserDataPath());
}

std::string CoreGetUserCacheDirectory(void)
{
    return std::string(m64p::Config.GetUserCachePath());
}

std::string CoreGetSharedDataDirectory(void)
{
    std::string directory;
#ifdef PORTABLE_INSTALL
    directory = "Data";
#else // Not Portable
    directory = CORE_INSTAlL_PREFIX;
    directory += "/share/RMG";
#endif // PORTABLE_INSTALL
    return directory;
}

std::string CoreGetSaveDirectory(void)
{
    return CoreSettingsGetStringValue(SettingsID::Core_SaveSRAMPath);
}

std::string CoreGetSaveStateDirectory(void)
{
    return CoreSettingsGetStringValue(SettingsID::Core_SaveStatePath);
}

std::string CoreGetScreenshotDirectory(void)
{
    return CoreSettingsGetStringValue(SettingsID::Core_ScreenshotPath);
}
