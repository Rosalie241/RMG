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

    HOOK_FUNC(handle, Debug, SetCallbacks);
    HOOK_FUNC(handle, Debug, SetRunState);
    HOOK_FUNC(handle, Debug, GetState);
    HOOK_FUNC(handle, Debug, Step);
    HOOK_FUNC(handle, Debug, DecodeOp);
    HOOK_FUNC(handle, Debug, MemGetRecompInfo);
    HOOK_FUNC(handle, Debug, MemGetMemInfo);
    HOOK_FUNC(handle, Debug, MemGetPointer);
    HOOK_FUNC(handle, Debug, MemRead64);
    HOOK_FUNC(handle, Debug, MemRead32);
    HOOK_FUNC(handle, Debug, MemRead16);
    HOOK_FUNC(handle, Debug, MemRead8);
    HOOK_FUNC(handle, Debug, MemWrite64);
    HOOK_FUNC(handle, Debug, MemWrite32);
    HOOK_FUNC(handle, Debug, MemWrite16);
    HOOK_FUNC(handle, Debug, MemWrite8);
    HOOK_FUNC(handle, Debug, GetCPUDataPtr);
    HOOK_FUNC(handle, Debug, BreakpointLookup);
    HOOK_FUNC(handle, Debug, BreakpointCommand);

    this->handle = handle;
    this->hooked = true;
    return true;
}

bool DebugApi::Unhook(void)
{
    UNHOOK_FUNC(Debug, SetCallbacks);
    UNHOOK_FUNC(Debug, SetRunState);
    UNHOOK_FUNC(Debug, GetState);
    UNHOOK_FUNC(Debug, Step);
    UNHOOK_FUNC(Debug, DecodeOp);
    UNHOOK_FUNC(Debug, MemGetRecompInfo);
    UNHOOK_FUNC(Debug, MemGetMemInfo);
    UNHOOK_FUNC(Debug, MemGetPointer);
    UNHOOK_FUNC(Debug, MemRead64);
    UNHOOK_FUNC(Debug, MemRead32);
    UNHOOK_FUNC(Debug, MemRead16);
    UNHOOK_FUNC(Debug, MemRead8);
    UNHOOK_FUNC(Debug, MemWrite64);
    UNHOOK_FUNC(Debug, MemWrite32);
    UNHOOK_FUNC(Debug, MemWrite16);
    UNHOOK_FUNC(Debug, MemWrite8);
    UNHOOK_FUNC(Debug, GetCPUDataPtr);
    UNHOOK_FUNC(Debug, BreakpointLookup);
    UNHOOK_FUNC(Debug, BreakpointCommand);

    this->handle = nullptr;
    this->hooked = false;
    return true;
}

bool DebugApi::IsHooked(void)
{
    return this->hooked;
}

m64p_dynlib_handle DebugApi::GetHandle(void)
{
    return this->handle;
}

std::string DebugApi::GetLastError(void)
{
    return this->errorMessage;
}
