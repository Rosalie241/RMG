#ifndef RMG_Sdl3GuidWithIndex_HPP
#define RMG_Sdl3GuidWithIndex_HPP

#include <string>
#include <vector>
#include <SDL3/SDL.h>

namespace Utilities
{
    std::string Sdl3GuidWithIndex(std::vector<std::string>& guidsWithoutIndex, SDL_JoystickID joystickId);
} // namespace Utilities

#endif //RMG_Sdl3GuidWithIndex_H
