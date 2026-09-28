#ifndef FINISH_H
#define FINISH_H

/*
 * Finish kinds: the fast paths of the back half.
 *
 * A fully covered pixel overflows against any memory coverage, so blending
 * runs only when forced and otherwise takes the final cycle's bypass colour.
 * Where the draw also has no alpha compare, no CVG_X_ALPHA and clamp or full
 * coverage, the alpha stage accepts, its alpha goes unused and the stored
 * coverage is 7. What is left of stage_write_color is a fixed colour path,
 * dither and the store.
 *
 * finish_select decides once per draw. The kernel runs the fully covered
 * interior of each span through the selected kind and every other pixel
 * through the generic stages. finish_write reuses the stage functions for the
 * work it keeps, and tests/test_finish.c checks every kind against the
 * generic path. A new kind is added to FINISH_KINDS, finish_select and
 * finish_write, and gets its interior instance in kernel.c, whose dispatch is
 * checked against FINISH_COUNT at compile time.
 */

#include "stage_back.h"

#define FINISH_KINDS(X)                                                        \
    X(GENERIC)  /* no fast path: the generic stages */                         \
    X(STORE)    /* the blender leaves the pixel as it is */                    \
    X(FOG)      /* cycle 0 fogs by shade alpha, the final cycle leaves it */

typedef enum finish_kind {
#define FINISH_ENUM(kind) FINISH_##kind,
    FINISH_KINDS(FINISH_ENUM)
#undef FINISH_ENUM
    FINISH_COUNT
} finish_kind;

static inline finish_kind finish_select(const rdp_primitive_state *primitive,
                                        bool depth_late)
{
    const rdp_fragment_state *state = &primitive->fragment;
    const rdp_blend_state *blend = &state->blend;
    const uint8_t coverage_dest = (uint8_t)(state->coverage_dest & 3u);
    if (primitive->framebuffer.store_size != RDP_SIZE_16BPP ||
        blend->alpha_compare || state->cvg_times_alpha || depth_late ||
        /* A coverage rescaled by interpenetrating mode adds to memory's
         * under a forced blend, which only the generic path reads. */
        (state->depth.compare && (state->depth.mode & 3u) == 1u &&
         coverage_dest == 0u && blend->force_blend) ||
        (coverage_dest != 0u && coverage_dest != 2u))
        return FINISH_GENERIC;
    /* The final cycle: blended when forced, which is the identity only for the
     * passthrough equation, and otherwise bypassed with its colour_a source. */
    const bool two_cycle = blend->cycle_count == 2u;
    const bool final_identity = blend->force_blend
        ? blend->program.operation[two_cycle ? 1u : 0u] == RDP_BLEND_OP_PIXEL_PASSTHROUGH
        : (blend->program.cycle[blend->final_cycle].color_a & 3u) == 0u;
    if (!final_identity) return FINISH_GENERIC;
    if (!two_cycle || blend->program.operation[0] == RDP_BLEND_OP_PIXEL_PASSTHROUGH)
        return FINISH_STORE;
    if (blend->program.operation[0] == RDP_BLEND_OP_FOG_SHADE_ALPHA &&
        (blend->program.cycle[0].factor_b & 3u) != 1u)
        return FINISH_FOG;
    return FINISH_GENERIC;
}

/* stage_write_color for a fully covered pixel of a draw with this finish kind.
 * x and y are raster coordinates. */
static SR_ALWAYS_INLINE sr_result finish_write(sr_memory *memory,
                                               const rdp_primitive_state *primitive,
                                               finish_kind finish,
                                               uint32_t color_address,
                                               uint32_t x, uint32_t y,
                                               rdp_color pixel,
                                               uint8_t shade_alpha,
                                               uint16_t noise,
                                               uint8_t rescaled_coverage)
{
    const rdp_fragment_state *state = &primitive->fragment;
    if (finish == FINISH_FOG)
        pixel = stage_blend_cycle(&state->blend, &state->blend.program.cycle[0],
                                  RDP_BLEND_OP_FOG_SHADE_ALPHA, pixel, 0u,
                                  (rdp_color){ 0u, 0u, 0u, 0u }, shade_alpha, false, 0u);
    pixel.a = 0xffu;
    /* Clamp coverage stores one less than the pixel's, which a rescale by
     * interpenetrating mode (0xff: none) can bring below full. */
    if (rescaled_coverage != 0xffu && (state->coverage_dest & 3u) == 0u) {
        const uint32_t stored = (uint32_t)rescaled_coverage - 1u;
        pixel.a = (uint8_t)(((stored > 7u ? 7u : stored) << 5) | 0x1fu);
    }
    const uint32_t pixel_x = sr_raster_to_pixel(x);
    const uint32_t pixel_y = sr_raster_to_pixel(y);
    pixel = stage_dither_rgb(pixel, state->rgb_dither, pixel_x,
                             pixel_y >> state->dither_y_shift, noise);
    return framebuffer_write_color_address(memory, RDP_SIZE_16BPP, color_address,
        pixel_y * primitive->framebuffer.color_image.width + pixel_x,
        sr_sample_of(x, y), pixel);
}

#endif
