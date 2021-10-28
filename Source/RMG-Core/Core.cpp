/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Core.hpp"

#include "osal/osal_dynlib.hpp"
#include "m64p/Api.hpp"

#include <filesystem>

//
// Local Variables
//

static osal_dynlib_lib_handle l_CoreLibHandle;

//
// Local Functions
//

std::string find_core_lib(void)
{
    for (const auto& entry : std::filesystem::recursive_directory_iterator("Core"))
    {
        std::string path = entry.path().string();
        if (path.ends_with(OSAL_DYNLIB_LIB_EXT_STR))
        {
            return path;
        }
    }

    return std::string();
}

//
// Exported Functions
//

bool CoreInit(void)
{
    std::string error;
    std::string core_file;
    bool ret = false;

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

    ret = CoreSettingsSetupDefaults();
    if (!ret)
    {
        return false;
    }

    ret = CoreApplyPluginSettings();
    if (!ret)
    {
        return false;
    }

    return true;
}

void CoreShutdown(void)
{
    CorePluginsShutdown();

    osal_dynlib_close(l_CoreLibHandle);
}
