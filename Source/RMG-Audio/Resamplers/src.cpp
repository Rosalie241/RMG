/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-sdl-audio - src.c                                         *
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

#include <samplerate.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <RMG-Core/m64p/api/m64p_types.h>

#define ARRAY_SIZE(x) sizeof((x)) / sizeof((x)[0])

struct fbuffer
{
    float* data;
    size_t size;
};

static void grow_fbuffer(struct fbuffer* fbuffer, size_t new_size)
{
    if (fbuffer->size < new_size) {
        fbuffer->data = (float*)realloc(fbuffer->data, new_size);
        fbuffer->size = new_size;
    }
}

static void free_fbuffer(struct fbuffer* fbuffer)
{
    free(fbuffer->data);
    fbuffer->data = NULL;
    fbuffer->size = 0;
}



struct src_resampler
{
    SRC_STATE* state;

    /* 2 intermediate buffers are needed for float/int conversion */
    struct fbuffer fbuffers[2];
};

static void* src_init_from_id(const char* resampler_id)
{
    size_t i;
    int error = 0;

    static const struct {
        const char* name;
        int converter_type;
    } types[] =
    {
        { "src-sinc-best-quality", SRC_SINC_BEST_QUALITY },
        { "src-sinc-medium-quality", SRC_SINC_MEDIUM_QUALITY },
        { "src-sinc-fastest", SRC_SINC_FASTEST },
        { "src-zero-order-hold", SRC_ZERO_ORDER_HOLD },
        { "src-linear",  SRC_LINEAR }
    };

    /* select resampler configuration */
    for (i = 0; i < ARRAY_SIZE(types); ++i) {
        if (strcmp(types[i].name, resampler_id) == 0) {
            break;
        }
    }

    /* handle unknown configuration */
    if (i >= ARRAY_SIZE(types)) {
        i = 1;

        DebugMessage(M64MSG_WARNING,
            "Unknown RESAMPLE configuration %s; use %s resampler",
            resampler_id, types[i].name);
    }

    /* init src resampler */
    struct src_resampler* src_resampler = (struct src_resampler*)malloc(sizeof(*src_resampler));
    if (src_resampler == NULL) {
        DebugMessage(M64MSG_ERROR, "Failed to allocate memory for SRC resampler");
        return NULL;
    }

    /* lazy-alloc of fbuffers */
    memset(src_resampler, 0, sizeof(*src_resampler));

    src_resampler->state = src_new(types[i].converter_type, 2, &error);
    if (error != 0) {
        DebugMessage(M64MSG_ERROR, "SRC error: %s", src_strerror(error));
        free(src_resampler);
        return NULL;
    }

    return src_resampler;
}

static void src_release(void* resampler)
{
    size_t i;

    struct src_resampler* src_resampler = (struct src_resampler*)resampler;

    if (src_resampler == NULL)
        return;

    src_delete(src_resampler->state);

    for(i = 0; i < 2; ++i) {
        free_fbuffer(&src_resampler->fbuffers[i]);
    }
}

static size_t src_resample(void* resampler,
                           const void* src, size_t src_size, unsigned int src_freq,
                           void* dst, size_t dst_size, unsigned int dst_freq)
{
    struct src_resampler* src_resampler = (struct src_resampler*)resampler;

    /* High quality resamplers needs more input than what
     * the sample rate ratio would indicate to work properly, hence the src/dst>1 ratio
     *
     * Limit src_size to avoid too much short-float-short conversion time
     */
    if (src_size > dst_size * 5 / 2) {
        src_size = dst_size * 5 / 2;
    }

    /* grow float buffers if necessary */
    if (src_size > 0) {
        grow_fbuffer(&src_resampler->fbuffers[0], src_size*2);
    }
    if (dst_size > 0) {
        grow_fbuffer(&src_resampler->fbuffers[1], dst_size*2);
    }

    src_short_to_float_array((short*)src, src_resampler->fbuffers[0].data, src_size/2);

    /* perform resampling */
    SRC_DATA src_data;

    src_data.data_in = src_resampler->fbuffers[0].data;
    src_data.input_frames = src_size/4;

    src_data.data_out = src_resampler->fbuffers[1].data;
    src_data.output_frames = dst_size/4;

    src_data.src_ratio = (float)dst_freq / src_freq;
    src_data.end_of_input = 0;

    int error = src_process(src_resampler->state, &src_data);

    /* in case of error, display error, zero output buffer and discard input buffer */
    if (error)
    {
        DebugMessage(M64MSG_ERROR, "SRC error: %s", src_strerror(error));
        memset(dst, 0, dst_size);
        return src_size;
    }

    if (dst_size != src_data.output_frames_gen*4) {
        DebugMessage(M64MSG_WARNING, "dst_size = %u != output_frames_gen*4 = %u",
                (uint32_t) dst_size, (uint32_t) src_data.output_frames_gen*4);
    }

    src_float_to_short_array(src_resampler->fbuffers[1].data, (short*)dst, src_data.output_frames_gen*2);
    memset((char*)dst + src_data.output_frames_gen*4, 0, dst_size - src_data.output_frames_gen*4);

    return src_data.input_frames_used * 4;
}

const struct resampler_interface g_src_iresampler = {
    "src",
    src_init_from_id,
    src_release,
    src_resample
};
