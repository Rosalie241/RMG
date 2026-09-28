#ifndef RASTER_H
#define RASTER_H

#include "primitive.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A sub-row that the primitive does not touch is encoded as an empty interval
 * rather than flagged in a separate mask: no sample position can satisfy
 * `pos >= NEVER_LEFT && pos < NEVER_RIGHT`. That keeps the evaluation loop free
 * of control flow, and the two values are the identity elements of the min/max
 * that derive the span extent, so span setup needs no special case either.
 */
#define RASTER_COVERAGE_NEVER_LEFT  INT32_MAX
#define RASTER_COVERAGE_NEVER_RIGHT INT32_MIN

typedef struct raster_coverage_span {
    int32_t left[4];
    int32_t right[4];
    int full_x0;
    int full_x1;
} raster_coverage_span;

/*
 * first_x / first_y locate the first covered sub-sample in scanline order, in
 * QUARTER pixels measured from the pixel origin (both 0..3) -- not centred on
 * the pixel. Attribute centroid correction adds `off * (derivative >> n)` at a
 * coarse scale, so a centred offset would inject a constant half-pixel bias
 * that has nothing to do with where the coverage actually lies.
 *
 * The bit layout of `mask` is chosen so these fall straight out of the lowest
 * set bit:
 *
 *     0x01        0x02
 *           0x04        0x08
 *     0x10        0x20
 *           0x40        0x80
 *
 * Full coverage therefore yields first_x = first_y = 0, which makes the
 * correction term vanish on its own -- no special case is needed.
 */
typedef struct raster_coverage {
    uint8_t mask;
    uint8_t count;
    uint8_t first_x;
    uint8_t first_y;
} raster_coverage;

static inline raster_coverage raster_coverage_evaluate(const raster_coverage_span *span,
                                                       int x)
{
    raster_coverage result = { 0u, 0u, 0u, 0u };
    if (x >= span->full_x0 && x <= span->full_x1) {
        result.mask = 0xffu;
        result.count = 8u;
        return result;
    }

    /* A staggered 8-sample grid. Coordinates are eighth-pixels; only boundary
     * pixels execute this loop. Untouched sub-rows carry an empty interval, so
     * no row test is needed and the eight sample decisions stay branch-free. */
    static const int8_t sample_x[4][2] = {
        { 0, 4 }, { 2, 6 }, { 0, 4 }, { 2, 6 }
    };
    const int32_t pixel_base = x << 16;
    uint8_t mask = 0u;
    for (uint32_t row = 0; row < 4u; row++) {
        const int32_t left = span->left[row];
        const int32_t right = span->right[row];
        for (uint32_t sample = 0; sample < 2u; sample++) {
            const int32_t position = pixel_base + (int32_t)sample_x[row][sample] * 8192;
            const uint32_t covered = (uint32_t)(position >= left) &
                                     (uint32_t)(position < right);
            mask |= (uint8_t)(covered << (row * 2u + sample));
        }
    }
    result.mask = mask;
    result.count = (uint8_t)__builtin_popcount((unsigned)mask);
    if (mask) {
        const uint32_t first = (uint32_t)__builtin_ctz((unsigned)mask);
        /* Derived from the bit index alone; odd sub-rows are staggered half a
         * quarter-pixel to the right, which is the `first_y & 1` term. */
        result.first_y = (uint8_t)(first >> 1);
        result.first_x = (uint8_t)(((first & 1u) << 1) + ((first >> 1) & 1u));
    }
    return result;
}

/* Incremental values at the start of one scanline span. */
typedef struct rdp_span {
    int x_begin;
    int x_end;
    int y;
    int64_t depth_fixed;
    int32_t s_fixed;
    int32_t t_fixed;
    int32_t w_fixed;
    int32_t dsdx_fixed;
    int32_t dtdx_fixed;
    uint8_t texture_coord_shift;
    /*
     * How many low bits of the column offset to drop before applying dsdx/dtdx,
     * so a group of adjacent pixels shares one texture coordinate. Zero steps
     * every pixel. Set to SR_SCALE_LOG2 by the native-resolution rectangle path,
     * where the derivative is then the full per-hardware-pixel one.
     */
    uint8_t texture_step_shift;
    /* Last visible row of a copy primitive; its trailing memory transaction
     * does not receive another scanline's completion strobe. */
    bool copy_primitive_terminal;
    raster_shade_setup shade;
    raster_coverage_span coverage;
    /* A one-cycle TEXEL1 read takes the next pixel in walking order. At the
     * end of a span of eight or more pixels whose next scanline is live, the
     * pipeline has already moved on and that pixel is the next scanline's
     * attribute latch, carried here. Last, so the fields every pixel reads
     * keep their offsets. */
    bool texel1_peek;
    int32_t peek_s;
    int32_t peek_t;
    int32_t peek_w;
} rdp_span;

void raster_setup_triangle_span(const rdp_primitive_state *primitive,
                                int x_begin,
                                int x_end,
                                int y,
                                rdp_span *work);
void raster_setup_rectangle_span(int x_begin,
                                 int x_end,
                                 int y,
                                 int32_t s_fixed,
                                 int32_t t_fixed,
                                 int32_t dsdx_fixed,
                                 int32_t dtdx_fixed,
                                 rdp_span *work);

sr_result raster_decode_triangle(const rdp_command *cmd, raster_decoded_triangle *out);
sr_result raster_submit_triangle(sr_memory *memory, tmem_state *tmem, const rdp_state *state, const rdp_command *cmd);
sr_result raster_submit_rectangle(sr_memory *memory, tmem_state *tmem, const rdp_state *state, const rdp_command *cmd);
/* Fill_Rect, with the span-buffer stale read on back-to-back repeats. */
sr_result raster_submit_fill_rectangle(sr_memory *memory, tmem_state *tmem, rdp_state *state, const rdp_command *cmd);

/* Renders one span with the kernel its primitive selected (kernel.c). */
sr_result kernel_render_span(sr_memory *memory,
                             const rdp_primitive_state *primitive,
                             const rdp_span *work);

#ifdef __cplusplus
}
#endif

#endif
