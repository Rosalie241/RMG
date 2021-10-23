/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef M64P_COREAPI_HPP
#define M64P_COREAPI_HPP

#include "api/m64p_common.h"
#include "api/m64p_frontend.h"

#include <iostream>

namespace m64p
{
class CoreApi
{
  public:
    CoreApi();
    ~CoreApi();

    bool Hook(m64p_dynlib_handle handle);
    bool IsHooked(void);

    std::string GetLastError(void);

    ptr_CoreStartup Startup;
    ptr_CoreShutdown Shutdown;
    ptr_CoreAttachPlugin AttachPlugin;
    ptr_CoreDetachPlugin DetachPlugin;
    ptr_CoreDoCommand DoCommand;
    ptr_CoreOverrideVidExt OverrideVidExt;
    ptr_CoreAddCheat AddCheat;
    ptr_CoreCheatEnabled CheatEnabled;
    ptr_CoreGetRomSettings GetRomSettings;
    ptr_CoreGetAPIVersions GetAPIVersions;
    ptr_CoreErrorMessage ErrorMessage;

  private:
    bool hooked = false;

    std::string errorMessage;
};
} // namespace m64p

#endif // M64P_COREAPI_HPP
