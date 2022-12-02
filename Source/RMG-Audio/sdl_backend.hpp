/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-sdl-audio - sdl_backend.h                                 *
 *   Mupen64Plus homepage: https://mupen64plus.org/                        *
 *   Copyright (C) 2017 Bobby Smiles                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef M64P_SDL_BACKEND_H
#define M64P_SDL_BACKEND_H

#include <cstdlib>

struct sdl_backend;

struct sdl_backend* init_sdl_backend(void);

void sdl_apply_settings(struct sdl_backend* sdl_backend);

void release_sdl_backend(struct sdl_backend* sdl_backend);

void sdl_set_frequency(struct sdl_backend* sdl_backend, unsigned int frequency);

void sdl_push_samples(struct sdl_backend* sdl_backend, const void* src, size_t size);

void sdl_synchronize_audio(struct sdl_backend* sdl_backend);

void sdl_set_speed_factor(struct sdl_backend* sdl_backend, unsigned int speed_factor);

#endif
