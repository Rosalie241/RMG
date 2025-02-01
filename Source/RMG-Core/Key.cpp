/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Error.hpp"
#include "Key.hpp"

#include "m64p/Api.hpp"

#include <string>

//
// Exported Functions
//

bool CoreSetKeyUp(int key, int mod)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    ret = m64p::Core.DoCommand(M64CMD_SEND_SDL_KEYUP, (mod << 16) + key, NULL);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreSetKeyUp M64P::Core.DoCommand(M64CMD_SEND_SDL_KEYUP) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

bool CoreSetKeyDown(int key, int mod)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    ret = m64p::Core.DoCommand(M64CMD_SEND_SDL_KEYDOWN, (mod << 16) + key, NULL);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreSetKeyDown M64P::Core.DoCommand(M64CMD_SEND_SDL_KEYDOWN) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}
