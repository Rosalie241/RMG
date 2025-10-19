/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>

#include <SDL3/SDL.h>

#define SDL_AXIS_PEAK 32767

enum class N64ControllerButton
{
    A = 0,
    B,
    Start,
    DpadUp,
    DpadDown,
    DpadLeft,
    DpadRight,
    CButtonUp,
    CButtonDown,
    CButtonLeft,
    CButtonRight,
    LeftShoulder,
    RightShoulder,
    ZTrigger,
    Invalid
};

enum class InputDeviceType
{
    None = 0,
    EmulateVRU,
    Automatic,
    Keyboard,
    Joystick,
    Invalid
};

enum class InputType
{
    Keyboard = -1,
    GamepadButton,
    GamepadAxis,
    JoystickButton,
    JoystickAxis,
    JoystickHat,
    Invalid
};

enum class InputAxisDirection
{
    Up = 0,
    Down,
    Left,
    Right,
    Invalid
};

enum class N64ControllerPak
{
    MemoryPak = 0,
    RumblePak,
    TransferPak,
    None,
};

struct InputDevice
{
    InputDeviceType type = InputDeviceType::Invalid;

    std::string name;
    std::string path;
    std::string serial;
    SDL_JoystickID id = 0;

    bool operator== (const InputDevice& other) const
    {
        return other.type == type &&
                other.name == name &&
                other.path == path &&
                other.serial == serial &&
                other.id == id;
    }
};

#endif // COMMON_HPP
