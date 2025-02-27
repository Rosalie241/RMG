/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#define CORE_INTERNAL
#include "Video.hpp"

#include "Library.hpp"
#include "Error.hpp"

#include "m64p/Api.hpp"

#include <string>
#include <mutex>

//
// Local Variables
//

static std::mutex l_VideoMutex;

//
// Exported Functions
//

CORE_EXPORT bool CoreGetVideoSize(int& width, int& height)
{
    const std::lock_guard<std::mutex> guard(l_VideoMutex);
    std::string error;
    m64p_error ret;
    int size = 0;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    ret = m64p::Core.DoCommand(M64CMD_CORE_STATE_QUERY, M64CORE_VIDEO_SIZE, &size);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreGetVideoSize m64p::Core.DoCommand(M64CMD_CORE_STATE_QUERY) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
        return false;
    }

    width  = (size >> 16) & 0xffff;
    height = size & 0xffff;
    return true;
}

CORE_EXPORT bool CoreSetVideoSize(int width, int height)
{
    const std::lock_guard<std::mutex> guard(l_VideoMutex);
    std::string error;
    m64p_error ret;
    int size;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

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

CORE_EXPORT bool CoreToggleFullscreen(void)
{
    std::string error;
    m64p_error ret;
    m64p_video_mode mode;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

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
