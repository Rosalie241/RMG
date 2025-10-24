/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef UICOMMON_HPP
#define UICOMMON_HPP

#include <SDL3/SDL.h>
#include <QString>

namespace UICommon
{

// attempts to retrieve native button text for button from gamepad
QString GetGamepadButtonText(SDL_Gamepad* gamepad, SDL_GamepadButton button);

};

#endif // UICOMMON_HPP
