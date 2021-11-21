/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "VidExt.hpp"
#include "Error.hpp"

#include "m64p/Api.hpp"

//
// Exported Functions
//

bool CoreSetupVidExt(m64p_video_extension_functions functions)
{
    std::string error;
    m64p_error ret;

    ret = m64p::Core.OverrideVidExt(&functions);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreSetupVidExt m64p::Core.OverrideVidExt() Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}
