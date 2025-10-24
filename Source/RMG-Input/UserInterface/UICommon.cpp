/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Utilities/Sdl3ButtonLabelToString.hpp"
#include "UICommon.hpp"

QString UICommon::GetGamepadButtonText(SDL_Gamepad* gamepad, SDL_GamepadButton button)
{
    QString text = SDL_GetGamepadStringForButton(button);
    if (gamepad == nullptr)
    {
        return text;
    }

    // convert A/B/X/Y buttons to their native counterpart,
    // this makes the button text look better in the UI
    if (button == SDL_GAMEPAD_BUTTON_SOUTH ||
        button == SDL_GAMEPAD_BUTTON_EAST  ||
        button == SDL_GAMEPAD_BUTTON_WEST  ||
        button == SDL_GAMEPAD_BUTTON_NORTH)
    {
        SDL_GamepadButtonLabel buttonLabel = SDL_GetGamepadButtonLabel(gamepad, button);
        QString labelString = Utilities::Sdl3ButtonLabelToString(buttonLabel);
        if (!labelString.isEmpty())
        { // replace only when we have a name
            text = labelString;
        }
    }

    return text;
}
