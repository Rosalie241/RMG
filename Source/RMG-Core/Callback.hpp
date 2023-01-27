/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_CALLBACK_HPP
#define CORE_CALLBACK_HPP

#include <functional>
#include <string>

// internal mupen64plus callbacks
#ifdef CORE_INTERNAL

#include "m64p/Api.hpp"

void CoreDebugCallback(void* context, int level, const char* message);
void CoreStateCallback(void* context, m64p_core_param param, int value);

void ResetMousePositionCallback(void);

#endif // CORE_INTERNAL

enum class CoreDebugMessageType
{
    Error   = 1,
    Warning = 2,
    Info    = 3,
    Status  = 4,
    Verbose = 5
};

enum class CoreStateCallbackType
{
    EmulationState = 1,
    VideoMode,
    SaveStateSlot,
    SpeedFactor,
    SpeedLimiter,
    VideoSize,
    AudioVolume,
    AudioMute,
    GamesharkInput,
    SaveStateLoaded,
    SaveStateSaved,
    ScreenshotCaptured,
};

// attempts to setup callbacks with the provided functions
bool CoreSetupCallbacks(std::function<void(enum CoreDebugMessageType, std::string, std::string)> debugCallbackFunc,
                        std::function<void(enum CoreStateCallbackType, int)> stateCallbackFunc);

// sets whether the debug callbacks will be printed to stdout
void CoreSetPrintDebugCallback(bool enabled);

// sends message to the debug callback
void CoreAddCallbackMessage(CoreDebugMessageType type, std::string message);

#endif // CORE_CALLBACK_HPP
