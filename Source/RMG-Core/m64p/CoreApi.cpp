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

    this->hooked = true;
    return true;
}

bool CoreApi::IsHooked(void)
{
    return this->hooked;
}

std::string CoreApi::GetLastError(void)
{
    return this->errorMessage;
}
