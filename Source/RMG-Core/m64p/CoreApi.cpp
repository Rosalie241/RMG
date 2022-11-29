/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "CoreApi.hpp"
#include "Macros.hpp"

using namespace m64p;

CoreApi::CoreApi(void)
{
    this->Unhook();
}

CoreApi::~CoreApi(void)
{
}

bool CoreApi::Hook(m64p_dynlib_handle handle)
{
    this->errorMessage = "CoreApi::Hook Failed: ";

    HOOK_FUNC(handle, Core, Startup);
    HOOK_FUNC(handle, Core, Shutdown);
    HOOK_FUNC(handle, Core, AttachPlugin);
    HOOK_FUNC(handle, Core, DetachPlugin);
    HOOK_FUNC(handle, Core, DoCommand);
    HOOK_FUNC(handle, Core, OverrideVidExt);
    HOOK_FUNC(handle, Core, AddCheat);
    HOOK_FUNC(handle, Core, CheatEnabled);
    HOOK_FUNC(handle, Core, GetRomSettings);
    HOOK_FUNC(handle, Core, GetAPIVersions);
    HOOK_FUNC(handle, Core, ErrorMessage);

    this->handle = handle;
    this->hooked = true;
    return true;
}

bool CoreApi::Unhook(void)
{
    UNHOOK_FUNC(Core, Startup);
    UNHOOK_FUNC(Core, Shutdown);
    UNHOOK_FUNC(Core, AttachPlugin);
    UNHOOK_FUNC(Core, DetachPlugin);
    UNHOOK_FUNC(Core, DoCommand);
    UNHOOK_FUNC(Core, OverrideVidExt);
    UNHOOK_FUNC(Core, AddCheat);
    UNHOOK_FUNC(Core, CheatEnabled);
    UNHOOK_FUNC(Core, GetRomSettings);
    UNHOOK_FUNC(Core, GetAPIVersions);
    UNHOOK_FUNC(Core, ErrorMessage);

    this->handle = nullptr;
    this->hooked = false;
    return true;
}

bool CoreApi::IsHooked(void)
{
    return this->hooked;
}

m64p_dynlib_handle CoreApi::GetHandle(void)
{
    return this->handle;
}

std::string CoreApi::GetLastError(void)
{
    return this->errorMessage;
}
