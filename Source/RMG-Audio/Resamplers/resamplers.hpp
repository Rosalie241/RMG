/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-audio-sdl - resamplers.h                                  *
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

#ifndef M64P_RESAMPLERS_RESAMPLERS_H
#define M64P_RESAMPLERS_RESAMPLERS_H

#include <cstddef>

struct resampler_interface
{
    const char* name;

    void* (*init_from_id)(const char* name);

    void (*release)(void* resampler);

    size_t (*resample)(void* resampler,
                       const void* src, size_t src_size, unsigned int src_freq,
                       void* dst, size_t dst_size, unsigned int dst_freq);
};

const struct resampler_interface* get_iresampler(const char* resampler_id, void** resampler);

extern const struct resampler_interface g_trivial_iresampler;
extern const struct resampler_interface g_speex_iresampler;
extern const struct resampler_interface g_src_iresampler;

#endif