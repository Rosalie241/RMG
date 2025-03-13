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
#include "SpeedFactor.hpp"
#include "Library.hpp"
#include "Error.hpp"

#include "m64p/Api.hpp"

#include <string>

//
// Exported Functions
//

CORE_EXPORT int CoreGetSpeedFactor(void)
{
    std::string error;
    m64p_error ret;
    int value = 0;

    if (!m64p::Core.IsHooked())
    {
        return 100;
    }

    ret = m64p::Core.DoCommand(M64CMD_CORE_STATE_QUERY, M64CORE_SPEED_FACTOR, &value);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreGetSpeedFactor: m64p::Core.DoCommand(M64CMD_CORE_STATE_QUERY) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return value;
}

CORE_EXPORT bool CoreSetSpeedFactor(int factor)
{
    std::string error;
    m64p_error ret;
    int value = factor;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    ret = m64p::Core.DoCommand(M64CMD_CORE_STATE_SET, M64CORE_SPEED_FACTOR, &value);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreSetSpeedFactor: m64p::Core.DoCommand(M64CMD_CORE_STATE_SET) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}
