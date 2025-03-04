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
#ifdef _WIN32
#define _CRT_RAND_S
#include <cstdlib>
#endif // _WIN32
#include "Netplay.hpp"
#include "Library.hpp"
#include "Error.hpp"

#include "m64p/Api.hpp"


//
// Local Variables
//

static bool l_HasInitNetplay = false;

//
// Exported Functions
//

CORE_EXPORT bool CoreInitNetplay(std::string address, int port, int player)
{
#ifdef NETPLAY
    std::string error;
    m64p_error ret;
    uint32_t id = 0;

    // initialize random ID
    while (id == 0)
    {
#ifdef _WIN32
        rand_s(&id);
#else
        id = rand();
#endif
        id &= ~0x7;
        id |= player;
    }

    uint32_t version;
    ret = m64p::Core.DoCommand(M64CMD_NETPLAY_GET_VERSION, 0x010001, &version);
    if (ret != M64ERR_SUCCESS)
    { 
        error = "CoreInitNetplay m64p::Core.DoCommand(M64CMD_NETPLAY_GET_VERSION) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
        return false;
    }

    ret = m64p::Core.DoCommand(M64CMD_NETPLAY_INIT, port, const_cast<char*>(address.c_str()));
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreInitNetplay m64p::Core.DoCommand(M64CMD_NETPLAY_INIT) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
        return false;
    }

    ret = m64p::Core.DoCommand(M64CMD_NETPLAY_CONTROL_PLAYER, player, &id);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreInitNetplay m64p::Core.DoCommand(M64CMD_NETPLAY_CONTROL_PLAYER) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
        CoreShutdownNetplay();
        return false;
    }

    l_HasInitNetplay = true;
    return true;
#else
    return false;
#endif // NETPLAY
}

CORE_EXPORT bool CoreHasInitNetplay(void)
{
#ifdef NETPLAY
    return l_HasInitNetplay;
#else
    return false;
#endif // NETPLAY
}

CORE_EXPORT bool CoreShutdownNetplay(void)
{
#ifdef NETPLAY
    std::string error;
    m64p_error ret;

    ret = m64p::Core.DoCommand(M64CMD_NETPLAY_CLOSE, 0, nullptr);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreShutdownNetplay m64p::Core.DoCommand(M64CMD_NETPLAY_CLOSE) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
        return false;
    }

    l_HasInitNetplay = false;
    return true;
#else
    return false;
#endif // NETPLAY
}