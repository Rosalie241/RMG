/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-sdl-audio - speex.c                                       *
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

#include <speex/speex_resampler.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

#define CORE_PLUGIN
#include <RMG-Core/Core.hpp>

#define ARRAY_SIZE(x) sizeof((x)) / sizeof((x)[0])

/* assume 2x16bit interleaved channels */
enum { BYTES_PER_SAMPLE = 4 };

static void* speex_init_from_id(const char* resampler_id)
{
    size_t i;
    int error;
    static const char *types[] =
    {
        "speex-fixed-0",
        "speex-fixed-1",
        "speex-fixed-2",
        "speex-fixed-3",
        "speex-fixed-4",
        "speex-fixed-5",
        "speex-fixed-6",
        "speex-fixed-7",
        "speex-fixed-8",
        "speex-fixed-9",
        "speex-fixed-10",
    };

    /* select resampler configuration */
    for (i = 0; i < ARRAY_SIZE(types); ++i) {
        if (strcmp(types[i], resampler_id) == 0) {
            break;
        }
    }

    /* handle unknown configuration */
    if (i >= ARRAY_SIZE(types)) {
        i = 4;

        DebugMessage(M64MSG_WARNING,
            "Unknown RESAMPLE configuration %s; use %s resampler",
            resampler_id, types[i]);
    }

    /* init speex object with dummy frequencies (will be set later) */
    SpeexResamplerState* spx_state =  speex_resampler_init(2, 44100, 44100, (int)i,  &error);

    if (error != RESAMPLER_ERR_SUCCESS) {
        DebugMessage(M64MSG_ERROR, "Speex error: %s", speex_resampler_strerror(error));
    }

    return spx_state;
}

static void speex_release(void* resampler)
{
    SpeexResamplerState* spx_state = (SpeexResamplerState*)resampler;

    if (spx_state == NULL) {
        return;
    }

    speex_resampler_destroy(spx_state);
}

static size_t speex_resample(void* resampler,
                             const void* src, size_t src_size, unsigned int src_freq,
                             void* dst, size_t dst_size, unsigned int dst_freq)
{
    SpeexResamplerState* spx_state = (SpeexResamplerState*)resampler;

    /* update resampling rates */
    speex_resampler_set_rate(spx_state, src_freq, dst_freq);

    /* perform resampling */
    spx_uint32_t in_len = src_size / BYTES_PER_SAMPLE;
    spx_uint32_t out_len = dst_size / BYTES_PER_SAMPLE;
    int error = speex_resampler_process_interleaved_int(spx_state, (const spx_int16_t *)src, &in_len, (spx_int16_t *)dst, &out_len);

    /* in case of error, display error, zero output buffer and discard input buffer */
    if (error != RESAMPLER_ERR_SUCCESS)
    {
        DebugMessage(M64MSG_ERROR, "Speex error: %s", speex_resampler_strerror(error));
        memset(dst, 0, dst_size);
        return src_size;
    }

    if (dst_size != out_len * BYTES_PER_SAMPLE) {
        DebugMessage(M64MSG_WARNING, "dst_size = %u != outlen*4 = %u",
                (uint32_t) dst_size, out_len * BYTES_PER_SAMPLE);
    }
    memset((char*)dst + out_len * BYTES_PER_SAMPLE, 0, dst_size - out_len * BYTES_PER_SAMPLE);

    return in_len * BYTES_PER_SAMPLE;
}

const struct resampler_interface g_speex_iresampler = {
    "speex",
    speex_init_from_id,
    speex_release,
    speex_resample
};
