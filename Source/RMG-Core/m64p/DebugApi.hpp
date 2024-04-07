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

    std::string GetLastError(void);

    ptr_DebugSetCoreCompare SetCoreCompare;
    ptr_DebugGetCPUDataPtr GetCPUDataPtr;

  private:
    bool hooked = false;

    std::string errorMessage;
    m64p_dynlib_handle handle;
};
} // namespace m64p

#endif // M64P_DEBUGAPI_HPP
