/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#define CORE_INTERNAL
#include "Screenshot.hpp"
#include "Library.hpp"
#include "Error.hpp"

#include "m64p/Api.hpp"

#include <string>

//
// Exported Functions
//

CORE_EXPORT bool CoreTakeScreenshot(void)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    ret = m64p::Core.DoCommand(M64CMD_TAKE_NEXT_SCREENSHOT, 0, nullptr);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreTakeScreenshot M64P::Core.DoCommand(M64CMD_TAKE_NEXT_SCREENSHOT) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}
