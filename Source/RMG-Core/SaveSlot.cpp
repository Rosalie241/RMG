/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "SaveSlot.hpp"
#include "Error.hpp"

#include "m64p/Api.hpp"

//
// Exported Functions
//

bool CoreSetSaveSlot(int slot)
{
    std::string error;
    m64p_error ret;

    ret = m64p::Core.DoCommand(M64CMD_CORE_STATE_SET, M64CORE_SAVESTATE_SLOT, &slot);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreSetSaveSlot M64P::Core.DoCommand(M64CMD_CORE_STATE_SET, M64CORE_SAVESTATE_SLOT) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

int CoreGetSaveSlot(void)
{
    std::string error;
    m64p_error ret;
    int slot = -1;

    ret = m64p::Core.DoCommand(M64CMD_CORE_STATE_QUERY, M64CORE_SAVESTATE_SLOT, &slot);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreGetSaveSlot M64P::Core.DoCommand(M64CMD_CORE_STATE_QUERY, M64CORE_SAVESTATE_SLOT) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return slot;
}
