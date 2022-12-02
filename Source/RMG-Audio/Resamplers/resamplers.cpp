/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-audio-sdl - resamplers.c                                  *
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

#include "resamplers.hpp"

#include "main.hpp"

#include <cstring>

#define CORE_PLUGIN
#include <RMG-Core/Core.hpp>

#define ARRAY_SIZE(x) sizeof((x)) / sizeof((x)[0])

const struct resampler_interface* get_iresampler(const char* resampler_id, void** resampler)
{
    size_t i;

    static const struct {
        const struct resampler_interface* iresampler;
        const char* cmp_str;
    } resamplers[] = {
      //  { &g_trivial_iresampler, "trivial" },
        { &g_speex_iresampler, "speex-" },
        { &g_src_iresampler, "src-" }
    };

    /* search matching resampler */
    for(i = 0; i < ARRAY_SIZE(resamplers); ++i) {
        if (strncmp(resampler_id, resamplers[i].cmp_str, strlen(resamplers[i].cmp_str)) == 0) {
            DebugMessage(M64MSG_INFO, "Using resampler %s", resamplers[i].iresampler->name);
            break;
        }
    }

    /* handle not found case */
    if (i >= ARRAY_SIZE(resamplers)) {
        i = 0;

        DebugMessage(M64MSG_WARNING, "Could not find RESAMPLE configuration %s; use %s resampler",
            resampler_id, resamplers[i].iresampler->name);
    }

    /* instanciate resampler */
    *resampler = resamplers[i].iresampler->init_from_id(resampler_id);
    return resamplers[i].iresampler;
}
