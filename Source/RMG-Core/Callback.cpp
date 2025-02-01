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
#include "ConvertStringEncoding.hpp"
#include "Callback.hpp"

#include <iostream>

//
// Local Structs
//

struct l_DebugCallbackMessage
{
    std::string Context;
    int         Level = 0;
    std::string Message;
};

//
// Local Variables
//

static bool l_SetupCallbacks = false;
static std::function<void(enum CoreDebugMessageType, std::string, std::string)> l_DebugCallbackFunc;
static std::function<void(enum CoreStateCallbackType, int)> l_StateCallbackFunc;
static bool l_PrintCallbacks = false;
static std::vector<l_DebugCallbackMessage> l_PendingCallbacks;


//
// Internal Functions
//

void CoreDebugCallback(void* context, int level, const char* message)
{
    std::string contextString((const char*)context);
    std::string messageString(message);

    if (!l_SetupCallbacks)
    {
        l_PendingCallbacks.push_back({contextString, level, message});
        return;
    }

    if (l_PrintCallbacks)
    {
        std::cout << contextString << messageString << std::endl;
    }

    // convert string encoding accordingly
    if (messageString.starts_with("IS64:"))
    {
        messageString = CoreConvertStringEncoding(message, CoreStringEncoding::EUC_JP);
    }
    else if (contextString.starts_with("[CORE]"))
    {
        messageString = CoreConvertStringEncoding(message, CoreStringEncoding::Shift_JIS);
    }

    l_DebugCallbackFunc((CoreDebugMessageType)level, contextString, messageString);
}

void CoreStateCallback(void* context, m64p_core_param param, int value)
{
    if (!l_SetupCallbacks)
    {
        return;
    }

    l_StateCallbackFunc((CoreStateCallbackType)param, value);
}

//
// Exported Functions
//

bool CoreSetupCallbacks(std::function<void(enum CoreDebugMessageType, std::string, std::string)> debugCallbackFunc,
                        std::function<void(enum CoreStateCallbackType, int)> stateCallbackFunc)
{
    l_DebugCallbackFunc = debugCallbackFunc;
    l_StateCallbackFunc = stateCallbackFunc;
    l_SetupCallbacks = true;
    
    // send pending messages
    for (const auto& callback : l_PendingCallbacks)
    {
        CoreDebugCallback((void*)callback.Context.c_str(), callback.Level, callback.Message.c_str());
    }
    l_PendingCallbacks.clear();

    return true;
}

void CoreSetPrintDebugCallback(bool enabled)
{
    l_PrintCallbacks = enabled;
}

void CoreAddCallbackMessage(CoreDebugMessageType type, std::string message)
{
    CoreDebugCallback((void*)"[GUI]   ", (int)type, message.c_str());
}
