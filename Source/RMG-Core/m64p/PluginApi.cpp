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
    this->Unhook();
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
    HOOK_FUNC_OPT(handle, Plugin, Config2);
    HOOK_FUNC(handle, Plugin, GetVersion);

    this->handle = handle;
    this->hooked = true;
    return true;
}

bool PluginApi::Unhook(void)
{
    UNHOOK_FUNC(Plugin, Startup);
    UNHOOK_FUNC(Plugin, Shutdown);
    UNHOOK_FUNC(Plugin, Config);
    UNHOOK_FUNC(Plugin, Config2);
    UNHOOK_FUNC(Plugin, GetVersion);

    this->handle = nullptr;
    this->hooked = false;
    return true;
}

bool PluginApi::IsHooked(void)
{
    return this->hooked;
}

m64p_dynlib_handle PluginApi::GetHandle(void)
{
    return this->handle;
}

std::string PluginApi::GetLastError(void)
{
    return this->errorMessage;
}

