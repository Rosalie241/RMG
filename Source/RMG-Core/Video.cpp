/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Video.hpp"

#include "Error.hpp"
#include "m64p/Api.hpp"

#include <string>

//
// Exported Functions
//

bool CoreSetVideoSize(int width, int height)
{
    std::string error;
    m64p_error ret;
    int size;

    size = (width << 16) + height;

    ret = m64p::Core.DoCommand(M64CMD_CORE_STATE_SET, M64CORE_VIDEO_SIZE, &size);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreSetVideoSize m64p::Core.DoCommand(M64CMD_CORE_STATE_SET) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

bool CoreToggleFullscreen(void)
{
    std::string error;
    m64p_error ret;
    m64p_video_mode mode;

    ret = m64p::Core.DoCommand(M64CMD_CORE_STATE_QUERY, M64CORE_VIDEO_MODE, &mode);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreToggleFullscreen m64p::Core.DoCommand(M64CMD_CORE_STATE_QUERY) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
        return false;
    }

    if (mode == M64VIDEO_FULLSCREEN)
    {
        mode = M64VIDEO_WINDOWED;
    }
    else
    {
        mode = M64VIDEO_FULLSCREEN;
    }

    ret = m64p::Core.DoCommand(M64CMD_CORE_STATE_SET, M64CORE_VIDEO_MODE, &mode);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreToggleFullscreen m64p::Core.DoCommand(M64CMD_CORE_STATE_SET) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}
