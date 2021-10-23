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

//
// Defines
//

#define CORE_LIB_FILENAME "Core/libmupen64plus.so.2.0.0"

//
// Local Variables
//

static std::string            l_ErrorMessage;
static osal_dynlib_lib_handle l_CoreLibHandle;

//
// Exported Functions
//

bool CoreInit(void)
{
    bool ret = false;

    l_CoreLibHandle = osal_dynlib_open(CORE_LIB_FILENAME);
    if (l_CoreLibHandle == nullptr)
    {
        l_ErrorMessage = "osal_dynlib_open Failed: ";
        l_ErrorMessage += osal_dynlib_strerror();
        return false;
    }

    ret = m64p::Core.Hook(l_CoreLibHandle);
    if (!ret)
    {
        l_ErrorMessage = m64p::Core.GetLastError();
        return false;
    }

    ret = m64p::Config.Hook(l_CoreLibHandle);
    if (!ret)
    {
        l_ErrorMessage = m64p::Core.GetLastError();
        return false;
    }

    return true;
}

void CoreShutdown(void)
{
    osal_dynlib_close(l_CoreLibHandle);
}

std::string CoreGetError(void)
{
    return l_ErrorMessage;
}
