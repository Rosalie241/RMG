/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Sdl3ButtonLabelToString.hpp"

#include <SDL3/SDL.h>

using namespace Utilities;

QString Utilities::Sdl3ButtonLabelToString(SDL_GamepadButtonLabel label)
{
	QString buttonName;

	switch (label)
    {
    case SDL_GAMEPAD_BUTTON_LABEL_A:
        buttonName = "a";
        break;
    case SDL_GAMEPAD_BUTTON_LABEL_B:
        buttonName = "b";
        break;
    case SDL_GAMEPAD_BUTTON_LABEL_X:
        buttonName = "x";
        break;
    case SDL_GAMEPAD_BUTTON_LABEL_Y:
        buttonName = "y";
        break;

    case SDL_GAMEPAD_BUTTON_LABEL_CROSS:
        buttonName = "cross";
        break;
    case SDL_GAMEPAD_BUTTON_LABEL_CIRCLE:
        buttonName = "circle";
        break;
    case SDL_GAMEPAD_BUTTON_LABEL_SQUARE:
        buttonName = "square";
        break;
    case SDL_GAMEPAD_BUTTON_LABEL_TRIANGLE:
        buttonName = "triangle";
        break;

    default:
        break;
    }

    return buttonName;
}
