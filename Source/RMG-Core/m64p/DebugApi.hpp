/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef M64P_DEBUGAPI_HPP
#define M64P_DEBUGAPI_HPP

#include "api/m64p_common.h"
#include "api/m64p_debugger.h"

#include <string>

namespace m64p
{
class DebugApi
{
  public:
    DebugApi();
    ~DebugApi();

    bool Hook(m64p_dynlib_handle handle);
    bool Unhook(void);
    bool IsHooked(void);

    m64p_dynlib_handle GetHandle(void);
    std::string GetLastError(void);

    ptr_DebugSetCallbacks SetCallbacks;
    ptr_DebugSetRunState SetRunState;
    ptr_DebugGetState GetState;
    ptr_DebugStep Step;
    ptr_DebugDecodeOp DecodeOp;
    ptr_DebugMemGetRecompInfo MemGetRecompInfo;
    ptr_DebugMemGetMemInfo MemGetMemInfo;
    ptr_DebugMemGetPointer MemGetPointer;
    ptr_DebugMemRead64 MemRead64;
    ptr_DebugMemRead32 MemRead32;
    ptr_DebugMemRead16 MemRead16;
    ptr_DebugMemRead8 MemRead8;
    ptr_DebugMemWrite64 MemWrite64;
    ptr_DebugMemWrite32 MemWrite32;
    ptr_DebugMemWrite16 MemWrite16;
    ptr_DebugMemWrite8 MemWrite8;
    ptr_DebugGetCPUDataPtr GetCPUDataPtr;
    ptr_DebugBreakpointLookup BreakpointLookup;
    ptr_DebugBreakpointCommand BreakpointCommand;

  private:
    bool hooked = false;

    std::string errorMessage;
    m64p_dynlib_handle handle;
};
} // namespace m64p

#endif // M64P_DEBUGAPI_HPP
