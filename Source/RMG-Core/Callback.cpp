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
#include "Core.hpp"

//
// Local Variables
//

static bool l_SetupCallbacks = false;
static std::function<void(enum CoreDebugMessageType, std::string)> l_DebugCallbackFunc;

//
// Internal Functions
//

void CoreDebugCallback(void* context, int level, const char* message)
{
    if (!l_SetupCallbacks)
    {
        return;
    }

    l_DebugCallbackFunc((CoreDebugMessageType)level, std::string(message));
}

void CoreStateCallback(void* context, m64p_core_param param, int value)
{
    // TODO
}

//
// Exported Functions
//

bool CoreSetupCallbacks(std::function<void(enum CoreDebugMessageType, std::string)> debugCallbackFunc)
{
    l_DebugCallbackFunc = debugCallbackFunc;
    l_SetupCallbacks = true;
    return true;
}
