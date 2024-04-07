/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "DebugApi.hpp"
#include "Macros.hpp"

using namespace m64p;

DebugApi::DebugApi(void)
{
    this->Unhook();
}

DebugApi::~DebugApi(void)
{
}

bool DebugApi::Hook(m64p_dynlib_handle handle)
{
    this->errorMessage = "DebugApi::Hook Failed: ";

    HOOK_FUNC(handle, Debug, SetCoreCompare);
    HOOK_FUNC(handle, Debug, GetCPUDataPtr);

    this->hooked = true;
    return true;
}

bool DebugApi::Unhook(void)
{
    UNHOOK_FUNC(Debug, SetCoreCompare);
    UNHOOK_FUNC(Debug, GetCPUDataPtr);

    this->hooked = false;
    return true;
}

bool DebugApi::IsHooked(void)
{
    return this->hooked;
}

std::string DebugApi::GetLastError(void)
{
    return this->errorMessage;
}
