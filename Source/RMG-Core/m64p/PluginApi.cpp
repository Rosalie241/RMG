/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "PluginApi.hpp"
#include "Macros.hpp"

using namespace m64p;

PluginApi::PluginApi(void)
{
}

PluginApi::~PluginApi(void)
{
}

bool PluginApi::Hook(m64p_dynlib_handle handle)
{
    this->errorMessage = "PluginApi::Hook Failed: ";

    HOOK_FUNC(handle, Plugin, Startup);
    HOOK_FUNC(handle, Plugin, Shutdown);
    HOOK_FUNC_OPT(handle, Plugin, Config);
    HOOK_FUNC(handle, Plugin, GetVersion);

    return true;
}

std::string PluginApi::GetLastError(void)
{
    return this->errorMessage;
}
