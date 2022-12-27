/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#define CORE_INTERNAL
#include "Core.hpp"
#ifdef DISCORD_RPC
#include "DiscordRpc.hpp"
#endif // DISCORD_RPC
#include "osal/osal_dynlib.hpp"
#include "m64p/Api.hpp"
#include "m64p/api/version.h"
#include <filesystem>
#include <string>
#include <cstring>

//
// Local Variables
//

static osal_dynlib_lib_handle l_CoreLibHandle;
static char l_CoreContextString[20];

//
// Local Functions
//

std::string find_core_lib(void)
{
    for (const auto& entry : std::filesystem::recursive_directory_iterator(CoreGetCoreDirectory()))
    {
        std::string path = entry.path().string();
        if (path.ends_with(OSAL_DYNLIB_LIB_EXT_STR))
        {
            return path;
        }
    }

    return std::string();
}

bool config_override_user_dirs(void)
{
    std::string error;
    m64p_error  ret;
    std::string dataDir;
    std::string cacheDir;
    bool        overrideUserDirs;

    dataDir = CoreSettingsGetStringValue(SettingsID::Core_UserDataDirOverride);
    cacheDir = CoreSettingsGetStringValue(SettingsID::Core_UserCacheDirOverride);
    overrideUserDirs = CoreSettingsGetBoolValue(SettingsID::Core_OverrideUserDirs);

    if (!overrideUserDirs)
    {
        return true;
    }

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

bool CoreInit(void)
{
    std::string error;
    std::string core_file;
    m64p_error  m64p_ret;
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

    l_CoreLibHandle = osal_dynlib_open(core_file.c_str());
    if (l_CoreLibHandle == nullptr)
    {
        error = "osal_dynlib_open Failed: ";
        error += osal_dynlib_strerror();
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

    m64p_ret = m64p::Core.Startup(FRONTEND_API_VERSION, CoreGetUserConfigDirectory().string().c_str(), CoreGetSharedDataDirectory().string().c_str(), (void*)l_CoreContextString, CoreDebugCallback, nullptr, CoreStateCallback);
    if (m64p_ret != M64ERR_SUCCESS)
    {
        error = "CoreInit M64P::Core.Startup() Failed: ";
        error += m64p::Core.ErrorMessage(m64p_ret);
        CoreSetError(error);
        return false;
    }

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

#ifdef DISCORD_RPC
    CoreDiscordRpcInit();
    CoreDiscordRpcUpdate(false);
#endif // DISCORD_RPC

    return true;
}

bool CoreInit(m64p_dynlib_handle handle)
{
    std::string error;
    bool ret = false;

    ret = m64p::Core.Hook(handle);
    if (!ret)
    {
        error = m64p::Core.GetLastError();
        CoreSetError(error);
        return false;
    }

    ret = m64p::Config.Hook(handle);
    if (!ret)
    {
        error = m64p::Config.GetLastError();
        CoreSetError(error);
        return false;
    }

    return true;
}

void CoreShutdown(void)
{
    CorePluginsShutdown();

    CoreSaveRomHeaderAndSettingsCache();

#ifdef DISCORD_RPC
    CoreDiscordRpcShutdown();
#endif // DISCORD_RPC

    m64p::Core.Unhook();
    m64p::Config.Unhook();

    osal_dynlib_close(l_CoreLibHandle);
}
