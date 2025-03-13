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
#include "Volume.hpp"

#include "Library.hpp"
#include "Error.hpp"

#include "m64p/Api.hpp"

//
// Exported Functions
//

CORE_EXPORT int CoreGetVolume(void)
{
    std::string error;
    m64p_error ret;
    int volume = -1;

    if (!m64p::Core.IsHooked())
    {
        return volume;
    }

    ret = m64p::Core.DoCommand(M64CMD_CORE_STATE_QUERY, M64CORE_AUDIO_VOLUME, &volume);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreGetVolume: m64p::Core.DoCommand(M64CMD_CORE_STATE_QUERY) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return volume;
}

CORE_EXPORT bool CoreSetVolume(int value)
{
    std::string error;
    m64p_error ret;
    int volume = value;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    ret = m64p::Core.DoCommand(M64CMD_CORE_STATE_SET, M64CORE_AUDIO_VOLUME, &volume);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreSetVolume: m64p::Core.DoCommand(M64CMD_CORE_STATE_SET) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}

CORE_EXPORT bool CoreIncreaseVolume(void)
{
    std::string error;
    int volume = 0;

    volume = CoreGetVolume();
    if (volume == -1)
    {
        return false;
    }
    else if (volume > 90)
    {
        error = "CoreIncreaseVolume Failed: cannot increase volume!";
        CoreSetError(error);
        return false;
    }

    return CoreSetVolume(volume + 10);
}

CORE_EXPORT bool CoreDecreaseVolume(void)
{
    std::string error;
    int volume = 0;

    volume = CoreGetVolume();
    if (volume == -1)
    {
        return false;
    }
    else if (volume < 10)
    {
        error = "CoreIncreaseVolume Failed: cannot decrease volume!";
        CoreSetError(error);
        return false;
    }

    return CoreSetVolume(volume - 10);
}

CORE_EXPORT bool CoreToggleMuteVolume(void)
{
    std::string error;
    m64p_error ret;
    int muted = 0;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    ret = m64p::Core.DoCommand(M64CMD_CORE_STATE_QUERY, M64CORE_AUDIO_MUTE, &muted);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreGetVolume: m64p::Core.DoCommand(M64CMD_CORE_STATE_QUERY) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
        return false;
    }

    muted = !muted;

    ret = m64p::Core.DoCommand(M64CMD_CORE_STATE_SET, M64CORE_AUDIO_MUTE, &muted);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreGetVolume: m64p::Core.DoCommand(M64CMD_CORE_STATE_SET) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    return ret == M64ERR_SUCCESS;
}