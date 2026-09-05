/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2026 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Sdl3GuidWithIndex.hpp"
#include <string>
#include <vector>
#include <SDL3/SDL.h>

using namespace Utilities;

std::string Utilities::Sdl3GuidWithIndex(std::vector<std::string>& guidsWithoutIndex, SDL_JoystickID joystickId)
{
    char guidBuffer[33]{};
    SDL_GUIDToString(SDL_GetGamepadGUIDForID(joystickId), guidBuffer, sizeof(guidBuffer));
    std::string guidWithoutIndex = std::string(guidBuffer);

    int guidIndex = 0;
    for(auto& otherDeviceGuidWithoutIndex : guidsWithoutIndex) {
        if(otherDeviceGuidWithoutIndex == guidWithoutIndex) guidIndex++;
    }
    guidsWithoutIndex.push_back(guidWithoutIndex);
    std::string guidWithIndex = guidWithoutIndex + "/" + std::to_string(guidIndex);

    return guidWithIndex;
}
