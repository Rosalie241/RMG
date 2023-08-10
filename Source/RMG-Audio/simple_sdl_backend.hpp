/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef SIMPLE_SDL_BACKEND_HPP
#define SIMPLE_SDL_BACKEND_HPP

#include <cstdlib>
#include <cstdint>

bool init_simple_sdl_backend(void);

void release_simple_sdl_backend(void);

void simple_sdl_push_samples(const void* src, size_t size);

void simple_sdl_set_frequency(unsigned int frequency);

void simple_sdl_set_volume(int volume);

void simple_sdl_set_speed_factor(uint32_t speed_factor);


#endif // SIMPLE_SDL_BACKEND_HPP