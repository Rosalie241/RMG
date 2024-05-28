/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Key.hpp"
#include "Error.hpp"
#include "m64p/Api.hpp"

#include <string>

//
// Exported Functions
//

bool CoreSetMouseMove(int x, int y)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    ret = m64p::Core.DoCommand(M64CMD_SET_MOUSE_MOVE, (y << 16) + x, nullptr);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreSetMouseMove M64P::Core.DoCommand(M64CMD_SET_MOUSE_MOVE) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

bool CoreSetMouseButton(int left, int right)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    ret = m64p::Core.DoCommand(M64CMD_SET_MOUSE_BUTTON, (right << 16) + left, nullptr);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreSetMouseMove M64P::Core.DoCommand(M64CMD_SET_MOUSE_MOVE) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}
