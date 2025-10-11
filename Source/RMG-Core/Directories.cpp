/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#define CORE_INTERNAL
#include "Directories.hpp"
#include "Settings.hpp"
#include "Library.hpp"
#include "Config.hpp"
#include "Error.hpp"

#include "m64p/Api.hpp"

#include <filesystem>
#include <stdexcept>
#include <optional>
#include <cstdlib>
#include <cstdio>
#ifdef _WIN32
#include <Windows.h>
#include <shlobj.h>
#endif // _WIN32

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

#ifdef PORTABLE_INSTALL
static std::filesystem::path get_exe_directory(void)
{
    static std::filesystem::path directory;
#ifdef _WIN32
    wchar_t buffer[MAX_PATH] = {0};
#else
    std::error_code errorCode;
#endif // _WIN32

    if (!directory.empty())
    {
        return directory.make_preferred();
    }

#ifdef _WIN32
    if (GetModuleFileNameW(nullptr, buffer, MAX_PATH) == 0)
    {
        MessageBoxA(nullptr, "get_exe_directory: GetModuleFileNameW() Failed!", "Error", MB_OK | MB_ICONERROR);
        throw std::runtime_error("get_exe_directory: GetModuleFileNameW() failed");
    }
    directory = std::filesystem::path(buffer).parent_path();
#else // _WIN32
    directory =  std::filesystem::canonical("/proc/self/exe", errorCode).parent_path();
    if (errorCode)
    {
        throw std::runtime_error("get_exe_directory: std::filesystem::canonical(\"/proc/self/exe\") failed: " + errorCode.message());
    }
#endif // _WIN32
    return directory.make_preferred();
}
#endif // PORTABLE_INSTALL

#ifdef _WIN32
static std::filesystem::path get_appdata_directory(std::filesystem::path directory)
{
    static std::filesystem::path appdataDirectory;
    std::filesystem::path fullDirectory;

    if (appdataDirectory.empty())
    {
        wchar_t buffer[MAX_PATH];
        LPITEMIDLIST pidl;

        if (SHGetSpecialFolderLocation(nullptr, CSIDL_APPDATA, &pidl) != S_OK ||
            !SHGetPathFromIDListW(pidl, buffer))
        { // fallback to executable directory
            appdataDirectory = get_exe_directory();
        }
        else
        {
            appdataDirectory = std::filesystem::path(buffer);
        }
    }

    fullDirectory = appdataDirectory;
    fullDirectory += "/RMG/";
    fullDirectory += directory;

    return fullDirectory;
}
#else // _WIN32
static std::filesystem::path get_var_directory(std::string var, std::string append, std::string fallbackVar, std::string fallbackAppend)
{
    std::filesystem::path directory;

    const char* env = std::getenv(var.c_str());
    if (env == nullptr)
    {
        env = std::getenv(fallbackVar.c_str());
        if (env == nullptr)
        {
            throw std::runtime_error("get_var_directory: fallbackVar: $" + fallbackVar + " cannot be non-existent");
        }
        directory = env;
        directory += fallbackAppend;
    }
    else
    {
        directory = env;
        directory += append;
    }

    return directory.make_preferred();
}
#endif // _WIN32

//
// Exported Functions
//

CORE_EXPORT bool CoreCreateDirectories(void)
{
    std::string error;
    std::error_code errorCode;

    const std::filesystem::path directories[] = 
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

    for (const std::filesystem::path& directory : directories)
    {
        if (!std::filesystem::is_directory(directory) &&
            !std::filesystem::create_directories(directory, errorCode))
        {
            error = "CoreCreateDirectories Failed: cannot create the '";
            error += directory.string();
            error += "' directory: ";
            error += errorCode.message();
            CoreSetError(error);
            return false;
        }
    }

    return true;
}

CORE_EXPORT bool CoreGetPortableDirectoryMode(void)
{
#ifdef PORTABLE_INSTALL
    static std::optional<bool> portable;

    if (portable.has_value())
    {
        return portable.value();
    }

    std::filesystem::path exeDirectory;
    std::filesystem::path configFile;
    std::filesystem::path portableFile;

    exeDirectory      = get_exe_directory();
    configFile        = exeDirectory;
    configFile        += "/Config/mupen64plus.cfg";
    portableFile      = exeDirectory;
    portableFile      += "/portable.txt";

    portable = std::filesystem::is_regular_file(portableFile) ||
               std::filesystem::is_regular_file(configFile);
    return portable.value();
#else // Linux install
    return false;
#endif // PORTABLE_INSTALL
}

CORE_EXPORT std::filesystem::path CoreGetLibraryDirectory(void)
{
    std::filesystem::path directory;
#ifdef PORTABLE_INSTALL
    if (CoreGetPortableDirectoryMode())
    {
        directory = ".";
    }
    else
    {
        directory = get_exe_directory();
    }
#else // Linux install
    if (!l_LibraryPathOverride.empty())
    {
        directory = l_LibraryPathOverride;
    }
    else
    {
        directory = CORE_INSTALL_LIBDIR;
        directory += "/RMG";
    }
#endif // PORTABLE_INSTALL
    return directory.make_preferred();
}

CORE_EXPORT std::filesystem::path CoreGetCoreDirectory(void)
{
    std::filesystem::path directory;
#ifdef PORTABLE_INSTALL
    if (CoreGetPortableDirectoryMode())
    {
        directory = "Core";
    }
    else
    {
        directory = get_exe_directory();
        directory += "/Core";
    }
#else // Linux install
    if (!l_CorePathOverride.empty())
    {
        directory = l_CorePathOverride;
    }
    else
    {
        directory = CoreGetLibraryDirectory();
        directory += "/Core";
    }
#endif // PORTABLE_INSTALL
    return directory.make_preferred();
}

CORE_EXPORT std::filesystem::path CoreGetPluginDirectory(void)
{
    std::filesystem::path directory;
#ifdef PORTABLE_INSTALL
    if (CoreGetPortableDirectoryMode())
    {
        directory = "Plugin";
    }
    else
    {
        directory = get_exe_directory();
        directory += "/Plugin";
    }
#else // Linux install
    if (!l_PluginPathOverride.empty())
    {
        directory = l_PluginPathOverride;
    }
    else
    {
        directory = CoreGetLibraryDirectory();
        directory += "/Plugin";
    }
#endif // PORTABLE_INSTALL
    return directory.make_preferred();
}

CORE_EXPORT std::filesystem::path CoreGetUserConfigDirectory(void)
{
    std::filesystem::path directory;
#ifdef PORTABLE_INSTALL
    if (CoreGetPortableDirectoryMode())
    {
        directory = "Config";
    }
    else
#endif // PORTABLE_INSTALL
    {
#ifdef _WIN32
        directory = get_appdata_directory("Config");

#else
        directory = get_var_directory("XDG_CONFIG_HOME", "/RMG", "HOME", "/.config/RMG");
#endif // _WIN32
    }
    return directory.make_preferred();
}

CORE_EXPORT std::filesystem::path CoreGetDefaultSaveDirectory(void)
{
    std::filesystem::path directory;
#ifdef PORTABLE_INSTALL
    if (CoreGetPortableDirectoryMode())
    {
        directory = "Save/Game";
    }
    else
#endif // PORTABLE_INSTALL
    {
#ifdef _WIN32
        directory = get_appdata_directory("Save/Game");

#else
        directory = CoreGetUserDataDirectory();
        directory += "/Save/Game";
#endif // _WIN32
    }
    return directory.make_preferred();
}

CORE_EXPORT std::filesystem::path CoreGetDefaultSaveStateDirectory(void)
{
    std::filesystem::path directory;
#ifdef PORTABLE_INSTALL
    if (CoreGetPortableDirectoryMode())
    {
        directory = "Save/State";
    }
    else
#endif // PORTABLE_INSTALL
    {
#ifdef _WIN32
        directory = get_appdata_directory("Save/State");

#else
        directory = CoreGetUserDataDirectory();
        directory += "/Save/State";
#endif // _WIN32
    }
    return directory.make_preferred();
}

CORE_EXPORT std::filesystem::path CoreGetDefaultScreenshotDirectory(void)
{
    std::filesystem::path directory;
#ifdef PORTABLE_INSTALL
    if (CoreGetPortableDirectoryMode())
    {
        directory = "Screenshots";
    }
    else
#endif // PORTABLE_INSTALL
    {
#ifdef _WIN32
        directory = get_appdata_directory("Screenshots");

#else
        directory = CoreGetUserDataDirectory();
        directory += "/Screenshots";
#endif // _WIN32
    }
    return directory.make_preferred();
}

CORE_EXPORT std::filesystem::path CoreGetUserDataDirectory(void)
{
    std::filesystem::path directory;
#ifdef PORTABLE_INSTALL
    if (CoreGetPortableDirectoryMode())
    {
        directory = "Data";
    }
    else
#endif // PORTABLE_INSTALL
    {
#ifdef _WIN32
        directory = get_appdata_directory("Data");
#else
        directory = get_var_directory("XDG_DATA_HOME", "/RMG", "HOME", "/.local/share/RMG");
#endif // _WIN32
    }
    return directory.make_preferred();
}

CORE_EXPORT std::filesystem::path CoreGetUserCacheDirectory(void)
{
    std::filesystem::path directory;
#ifdef PORTABLE_INSTALL
    if (CoreGetPortableDirectoryMode())
    {
        directory = "Cache";
    }
    else
#endif // PORTABLE_INSTALL
    {
#ifdef _WIN32
        directory = get_appdata_directory("Cache");

#else
        directory = get_var_directory("XDG_CACHE_HOME", "/RMG", "HOME", "/.cache/RMG");
#endif // _WIN32
    }
    return directory.make_preferred();
}

CORE_EXPORT std::filesystem::path CoreGetSharedDataDirectory(void)
{
    std::filesystem::path directory;
#ifdef PORTABLE_INSTALL
    if (CoreGetPortableDirectoryMode())
    {
        directory = "Data";
    }
    else
    {
        directory = get_exe_directory();
        directory += "/Data";
    }
#else // Linux install
    if (!l_SharedDataPathOverride.empty())
    {
        directory = l_SharedDataPathOverride;
    }
    else
    {
        directory = CORE_INSTALL_DATADIR;
        directory += "/RMG";
    }
#endif // PORTABLE_INSTALL
    return directory.make_preferred();
}

CORE_EXPORT std::filesystem::path CoreGetSaveDirectory(void)
{
    return CoreSettingsGetStringValue(SettingsID::Core_SaveSRAMPath);
}

CORE_EXPORT std::filesystem::path CoreGetSaveStateDirectory(void)
{
    return CoreSettingsGetStringValue(SettingsID::Core_SaveStatePath);
}

CORE_EXPORT std::filesystem::path CoreGetScreenshotDirectory(void)
{
    return CoreSettingsGetStringValue(SettingsID::Core_ScreenshotPath);
}

#ifndef PORTABLE_INSTALL
CORE_EXPORT void CoreSetLibraryPathOverride(std::filesystem::path path)
{
    l_LibraryPathOverride = path;
}

CORE_EXPORT void CoreSetCorePathOverride(std::filesystem::path path)
{
    l_CorePathOverride = path;
}

CORE_EXPORT void CoreSetPluginPathOverride(std::filesystem::path path)
{
    l_PluginPathOverride = path;
}

CORE_EXPORT void CoreSetSharedDataPathOverride(std::filesystem::path path)
{
    l_SharedDataPathOverride = path;
}
#endif // PORTABLE_INSTALL
