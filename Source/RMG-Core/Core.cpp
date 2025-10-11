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
#include "CachedRomHeaderAndSettings.hpp"
#include "Directories.hpp"
#include "MediaLoader.hpp"
#include "Callback.hpp"
#include "Settings.hpp"
#include "Library.hpp"
#include "Plugins.hpp"
#include "Error.hpp"
#include "Core.hpp"

#include "m64p/Api.hpp"
#include "m64p/api/version.h"

#include <filesystem>
#include <string>
#include <cstring>

//
// Local Variables
//

static CoreLibraryHandle l_CoreLibHandle;
static char l_CoreContextString[20];

//
// Local Functions
//

static std::filesystem::path find_core_lib(void)
{
    for (const auto& entry : std::filesystem::recursive_directory_iterator(CoreGetCoreDirectory()))
    {
        std::filesystem::path path = entry.path();
        if (path.has_extension() && 
            path.extension() == CORE_LIBRARY_EXT_STR)
        {
            return path;
        }
    }

    return std::filesystem::path();
}

static bool config_override_user_dirs(void)
{
    std::string error;
    m64p_error  ret;
    const std::string dataDir = CoreGetUserDataDirectory().string();
    const std::string cacheDir = CoreGetUserCacheDirectory().string();

    ret = m64p::Config.OverrideUserPaths(dataDir.c_str(), cacheDir.c_str());
    if (ret != M64ERR_SUCCESS)
    {
        error = "config_override_user_dirs m64p::Config.OverrideUserPaths() Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

//
// Exported Functions
//

CORE_EXPORT bool CoreInit(void)
{
    std::string           error;
    std::filesystem::path core_file;
    m64p_error            m64p_ret;
    bool ret = false;

    // initialize context string
    std::strcpy(l_CoreContextString, "[CORE]  ");

    core_file = find_core_lib();
    if (core_file.empty())
    {
        error = "no core lib found";
        CoreSetError(error);
        return false;
    }

    l_CoreLibHandle = CoreOpenLibrary(core_file);
    if (l_CoreLibHandle == nullptr)
    {
        error = "CoreOpenLibrary Failed: ";
        error += CoreGetLibraryError();
        CoreSetError(error);
        return false;
    }

    ret = m64p::Core.Hook(l_CoreLibHandle);
    if (!ret)
    {
        error = m64p::Core.GetLastError();
        CoreSetError(error);
        return false;
    }

    ret = m64p::Config.Hook(l_CoreLibHandle);
    if (!ret)
    {
        error = m64p::Config.GetLastError();
        CoreSetError(error);
        return false;
    }

    m64p_ret = m64p::Core.Startup(FRONTEND_API_VERSION, 
                                    CoreGetUserConfigDirectory().string().c_str(), 
                                    CoreGetSharedDataDirectory().string().c_str(), 
                                    l_CoreContextString,
                                    CoreDebugCallback, 
                                    nullptr,
                                    CoreStateCallback);
    if (m64p_ret != M64ERR_SUCCESS)
    {
        error = "CoreInit m64p::Core.Startup() Failed: ";
        error += m64p::Core.ErrorMessage(m64p_ret);
        CoreSetError(error);
        return false;
    }

    CoreAddCallbackMessage(CoreDebugMessageType::Info, "Initialized " + core_file.filename().string());

    ret = CoreSetupMediaLoader();
    if (!ret)
    {
        return false;
    }

    ret = CoreSettingsUpgrade();
    if (!ret)
    {
        return false;
    }

    ret = CoreSettingsSetupDefaults();
    if (!ret)
    {
        return false;
    }

    ret = config_override_user_dirs();
    if (!ret)
    {
        return false;
    }

    ret = CoreCreateDirectories();
    if (!ret)
    {
        return false;
    }

    CoreReadRomHeaderAndSettingsCache();
    return true;
}

CORE_EXPORT void CoreShutdown(void)
{
    CorePluginsShutdown();

    CoreSaveRomHeaderAndSettingsCache();

    m64p::Core.Shutdown();

    m64p::Core.Unhook();
    m64p::Config.Unhook();

    CoreCloseLibrary(l_CoreLibHandle);
}
