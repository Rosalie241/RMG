/*
 * Portions of the internal-resolution scaling implementation are adapted
 * from Parallel-RDP.
 *
 * Copyright (c) 2020 Themaister
 * Used under the MIT License
 */

/*
 * Copyright (c) 2026 PixelMechanic0
 * Licensed under the MIT License.
 *
 * Project: https://github.com/PixelMechanic0/softrdp
 */

#include "raster.h"
#include <string.h>

#include "framebuffer.h"

#include <stdint.h>


static int32_t sign_extend(uint32_t value, unsigned bits)
{
    const uint32_t mask = 1u << (bits - 1u);
    return (int32_t)((value ^ mask) - mask);
}

static raster_triangle_setup decode_triangle_setup(const rdp_command *cmd)
{
    const uint32_t *w = cmd->words;
    raster_triangle_setup setup;

    setup.flip = (w[0] & 0x00800000u) != 0;
    setup.tile = (uint8_t)((w[0] >> 16) & 7u);
    setup.max_level = (uint8_t)((w[0] >> 19) & 7u);
    setup.yl = (int16_t)sign_extend(w[0] & 0x3fffu, 14);
    setup.ym = (int16_t)sign_extend((w[1] >> 16) & 0x3fffu, 14);
    setup.yh = (int16_t)sign_extend(w[1] & 0x3fffu, 14);
    setup.xl = sign_extend(w[2] & 0x0fffffffu, 28);
    setup.dxldy = sign_extend(w[3] & 0x3fffffffu, 30);
    setup.xh = sign_extend(w[4] & 0x0fffffffu, 28);
    setup.dxhdy = sign_extend(w[5] & 0x3fffffffu, 30);
    setup.xm = sign_extend(w[6] & 0x0fffffffu, 28);
    setup.dxmdy = sign_extend(w[7] & 0x3fffffffu, 30);

    return setup;
}

static int32_t join_hi_lo(uint32_t hi_word, unsigned hi_shift, uint32_t lo_word, unsigned lo_shift)
{
    return (int32_t)(((hi_word >> hi_shift) & 0xffff0000u) |
                      ((lo_word >> lo_shift) & 0x0000ffffu));
}

static raster_shade_setup decode_shade_setup(const uint32_t *w)
{
    raster_shade_setup setup;

    setup.r = join_hi_lo(w[0], 0, w[4], 16);
    setup.g = (int32_t)((w[0] << 16) | (w[4] & 0xffffu));
    setup.b = join_hi_lo(w[1], 0, w[5], 16);
    setup.a = (int32_t)((w[1] << 16) | (w[5] & 0xffffu));

    setup.drdx = join_hi_lo(w[2], 0, w[6], 16);
    setup.dgdx = (int32_t)((w[2] << 16) | (w[6] & 0xffffu));
    setup.dbdx = join_hi_lo(w[3], 0, w[7], 16);
    setup.dadx = (int32_t)((w[3] << 16) | (w[7] & 0xffffu));

    setup.drde = join_hi_lo(w[8], 0, w[12], 16);
    setup.dgde = (int32_t)((w[8] << 16) | (w[12] & 0xffffu));
    setup.dbde = join_hi_lo(w[9], 0, w[13], 16);
    setup.dade = (int32_t)((w[9] << 16) | (w[13] & 0xffffu));

    setup.drdy = join_hi_lo(w[10], 0, w[14], 16);
    setup.dgdy = (int32_t)((w[10] << 16) | (w[14] & 0xffffu));
    setup.dbdy = join_hi_lo(w[11], 0, w[15], 16);
    setup.dady = (int32_t)((w[11] << 16) | (w[15] & 0xffffu));

    return setup;
}

static raster_texture_setup decode_texture_setup(const uint32_t *w)
{
    raster_texture_setup setup;

    setup.s = join_hi_lo(w[0], 0, w[4], 16);
    setup.t = (int32_t)((w[0] << 16) | (w[4] & 0xffffu));
    setup.w = join_hi_lo(w[1], 0, w[5], 16);

    setup.dsdx = join_hi_lo(w[2], 0, w[6], 16);
    setup.dtdx = (int32_t)((w[2] << 16) | (w[6] & 0xffffu));
    setup.dwdx = join_hi_lo(w[3], 0, w[7], 16);

    setup.dsde = join_hi_lo(w[8], 0, w[12], 16);
    setup.dtde = (int32_t)((w[8] << 16) | (w[12] & 0xffffu));
    setup.dwde = join_hi_lo(w[9], 0, w[13], 16);

    setup.dsdy = join_hi_lo(w[10], 0, w[14], 16);
    setup.dtdy = (int32_t)((w[10] << 16) | (w[14] & 0xffffu));
    setup.dwdy = join_hi_lo(w[11], 0, w[15], 16);

    return setup;
}

static raster_depth_setup decode_depth_setup(const uint32_t *w)
{
    raster_depth_setup setup;

    setup.z = (int32_t)w[0];
    setup.dzdx = (int32_t)w[1];
    setup.dzde = (int32_t)w[2];
    setup.dzdy = (int32_t)w[3];
    return setup;
}

/* A triangle without a z block still runs the z pipe; othermode decides
 * whether it compares and writes. The coefficient loader latches the
 * triangle's own header doubleword into both z-block slots:
 * z = dzde = hi32, dzdx = dzdy = lo32. */
static raster_depth_setup decode_header_depth_setup(const uint32_t *w)
{
    raster_depth_setup setup;

    setup.z = setup.dzde = (int32_t)w[0];
    setup.dzdx = setup.dzdy = (int32_t)w[1];
    return setup;
}

/*
 * Lift a triangle setup into raster space.
 *
 * The y edges count quarter-scanlines and the x edges are 16.16 pixel
 * coordinates, so both scale with the resolution. The dy slopes do not: they
 * are already per quarter-scanline, and raster space has proportionally more
 * quarter-scanlines per hardware scanline in exactly the same ratio as it has
 * more x units per hardware pixel.
 *
 * The `& ~1` truncation the hardware applies to an x origin is an artefact at
 * hardware precision, so it happens before the scaling rather than after, where
 * it would be a no-op.
 */
static void scale_triangle_setup(raster_triangle_setup *setup)
{
#if SOFTRDP_SCALE > 1
    setup->yh = (int16_t)(setup->yh * SOFTRDP_SCALE);
    setup->ym = (int16_t)(setup->ym * SOFTRDP_SCALE);
    setup->yl = (int16_t)(setup->yl * SOFTRDP_SCALE);
    setup->xh = (setup->xh & ~1) * SOFTRDP_SCALE;
    setup->xm = (setup->xm & ~1) * SOFTRDP_SCALE;
    setup->xl = (setup->xl & ~1) * SOFTRDP_SCALE;
#else
    (void)setup;
#endif
}

/* Scissor edges are 10.2 fixed point in hardware pixels. */
static inline uint32_t scale_scissor(uint32_t edge)
{
    return edge * SOFTRDP_SCALE;
}

static int fixed_floor_div(int64_t value, int64_t scale)
{
    int64_t quotient = value / scale;
    const int64_t remainder = value % scale;
    if (remainder && value < 0) {
        quotient--;
    }
    return (int)quotient;
}

static int fixed_ceil_div(int64_t value, int64_t scale)
{
    return -fixed_floor_div(-value, scale);
}


typedef struct raster_span {
    int y;
    int x0;
    int x1;
    raster_coverage_span coverage;
} raster_span;

typedef struct raster_edge_cursor {
    int64_t major[4];
    int64_t upper[4];
    int64_t lower[4];
} raster_edge_cursor;

static inline int32_t edge_origin(int32_t x)
{
    return x & ~1;
}

static inline int32_t edge_step(int32_t slope)
{
    return (slope >> 2) & ~1;
}

static void raster_edge_cursor_init(raster_edge_cursor *cursor,
                                    const raster_triangle_setup *setup,
                                    int y)
{
    /* Snapped at hardware precision: `& ~3` means "a whole hardware
     * scanline", and applying it after scaling would snap to half of one,
     * displacing every scanline of every triangle whose yh happens to
     * carry the bits the mask discards. */
    const int64_t yh_base = sr_snap_scaled(setup->yh, 2u);
    const int32_t step_major = edge_step(setup->dxhdy);
    const int32_t step_upper = edge_step(setup->dxmdy);
    const int32_t step_lower = edge_step(setup->dxldy);
    for (int row = 0; row < 4; row++) {
        const int64_t y_sub = (int64_t)y * 4 + row;
        const int64_t dy_h = y_sub - yh_base;
        cursor->major[row] = (int64_t)edge_origin(setup->xh) + dy_h * step_major;
        cursor->upper[row] = (int64_t)edge_origin(setup->xm) + dy_h * step_upper;
        /* The low edge takes over at ym, where the hardware reloads the
         * accumulator with xl and switches to the dxldy step. */
        cursor->lower[row] = (int64_t)edge_origin(setup->xl) +
            (y_sub - setup->ym) * step_lower;
    }
}

static inline void raster_edge_cursor_advance(raster_edge_cursor *cursor,
                                              const raster_triangle_setup *setup)
{
    /* One scanline is four sub-scanline steps of the truncated slope, not one
     * step of the untruncated one. */
    const int64_t step_major = 4 * (int64_t)edge_step(setup->dxhdy);
    const int64_t step_upper = 4 * (int64_t)edge_step(setup->dxmdy);
    const int64_t step_lower = 4 * (int64_t)edge_step(setup->dxldy);
    for (int row = 0; row < 4; row++) {
        cursor->major[row] += step_major;
        cursor->upper[row] += step_upper;
        cursor->lower[row] += step_lower;
    }
}

static inline bool raster_minor_uses_lower(const raster_triangle_setup *setup,
                                           int64_t y_sub)
{
    /* Minor-edge selection begins with xm/dxmdy at the rounded triangle start.
     * The xl/dxldy segment becomes active only when ym is reached during the
     * primitive, so an earlier ym cannot preselect the lower segment. */
    const int64_t y_start = sr_snap_scaled(setup->yh, 2u);
    return setup->ym >= y_start && y_sub >= setup->ym;
}

static inline bool scissor_accepts_raster_y(const rdp_state *state, uint32_t y)
{
    return !state->scissor_field ||
           ((sr_raster_to_pixel(y) & 1u) ==
            (uint32_t)state->scissor_keep_odd);
}

/*
 * Two properties of the fixed-width raster coordinates that a wide accumulator with a
 * numeric clamp does not have, and that only show up once a slope is steep
 * enough to matter:
 *
 *  - the edge lives in a register of fixed width, so a runaway slope wraps
 *    rather than growing without bound, and
 *  - edges are snapped to eighths of a pixel, with a sticky bit standing in for
 *    everything finer, BEFORE they are compared with each other or the scissor.
 *
 * Together they end a runaway edge (it reads as having crossed its partner) and
 * then let it re-enter from the other side, instead of pinning it to the
 * scissor and filling every remaining scanline.
 *
 */
#define RASTER_EDGE_BITS (28u + SR_SCALE_LOG2)

static inline int32_t raster_edge_truncate(int64_t x)
{
    const uint64_t mask = (1ull << RASTER_EDGE_BITS) - 1ull;
    const uint64_t sign = 1ull << (RASTER_EDGE_BITS - 1u);
    return (int32_t)(int64_t)(((((uint64_t)x) & mask) ^ sign) - sign);
}

/* 16.16 to eighths of a pixel; bit 0 means "and something below an eighth". */
static inline int32_t raster_edge_snap(int32_t x)
{
    return (x >> 13) | ((x & 0x1fff) != 0 ? 1 : 0);
}

/* Rectangles carry their edges already clipped, so they only need the plain
 * numeric clamp; the snapping above models how the hardware steps triangle
 * edges. */
static int32_t clamp_edge_x(int64_t x, int32_t minimum, int32_t maximum)
{
    if (x < minimum) return minimum;
    if (x > maximum) return maximum;
    return (int32_t)x;
}

static bool triangle_span_for_y(const raster_triangle_setup *setup,
                                const rdp_state *state,
                                const raster_edge_cursor *cursor,
                                int y,
                                raster_span *span)
{
    static const int sample_min[4] = { 0, 2, 0, 2 };
    static const int sample_max[4] = { 4, 6, 4, 6 };
    /* An empty scissor (any high edge <= its low edge) clips the whole primitive
     * away. Never expand a zero high edge into a large default: that is the same
     * hazard that let a degenerate FILL_RECT scribble over RDRAM. */
    if (state->scissor_x1 <= state->scissor_x0 ||
        state->scissor_y1 <= state->scissor_y0) {
        return false;
    }
    const int scissor_y0 = (int)scale_scissor(state->scissor_y0);
    const int scissor_y1 = (int)scale_scissor(state->scissor_y1);
    /* Eighths of a pixel on the RASTER grid: the scale rides on the scissor,
     * so the edge itself stays the width of the register being modelled. */
    const int32_t lo_scissor =
        (int32_t)(state->scissor_x0 << 1) * (int32_t)SOFTRDP_SCALE;
    const int32_t hi_scissor =
        (int32_t)(state->scissor_x1 << 1) * (int32_t)SOFTRDP_SCALE;
    int min_edge = 0x3fffffff;
    int max_edge = -0x3fffffff;
    int full_x0 = -0x3fffffff;
    int full_x1 = 0x3fffffff;
    bool all_over = true;
    bool all_under = true;

    *span = (raster_span){ .y = y };
    for (int row = 0; row < 4; row++) {
        const int64_t y_sub = (int64_t)y * 4 + row;
        int32_t left = RASTER_COVERAGE_NEVER_LEFT;
        int32_t right = RASTER_COVERAGE_NEVER_RIGHT;

        const int32_t major =
            raster_edge_snap(raster_edge_truncate(cursor->major[row]));
        const int32_t minor = raster_edge_snap(raster_edge_truncate(
            raster_minor_uses_lower(setup, y_sub) ?
                cursor->lower[row] : cursor->upper[row]));
        int32_t edge_l = setup->flip ? major : minor;
        int32_t edge_r = setup->flip ? minor : major;

        /* Scanlines outside the y range still take part in these: leaving the
         * scissor on one side drops the scanline rather than drawing it pinned
         * to that edge, and that is what breaks a sweeping edge into bands. */
        all_over = all_over && (edge_l < edge_r ? edge_l : edge_r) >= hi_scissor;
        all_under = all_under && (edge_l > edge_r ? edge_l : edge_r) < lo_scissor;

        if (edge_l < lo_scissor) edge_l = lo_scissor;
        if (edge_l > hi_scissor) edge_l = hi_scissor;
        if (edge_r < lo_scissor) edge_r = lo_scissor;
        if (edge_r > hi_scissor) edge_r = hi_scissor;

        /* Comparing with the sticky bit dropped puts this at quarter-pixel
         * resolution, which is what lets a wrapped edge read as crossed. A row
         * that fails it is left as the empty interval, exactly like a row
         * outside the y range - both mean "contributes no samples". */
        if (y_sub >= setup->yh && y_sub < setup->yl &&
            y_sub >= scissor_y0 && y_sub < scissor_y1 &&
            (edge_l >> 1) <= (edge_r >> 1)) {
            left = (int32_t)((int64_t)edge_l << 13);
            right = (int32_t)((int64_t)edge_r << 13);
        }

        span->coverage.left[row] = left;
        span->coverage.right[row] = right;

        /* The empty interval is the identity for all four reductions below, so
         * untouched rows fall out on their own: they cannot narrow the span
         * extent, and they drive the fully covered range empty, which is what
         * the old valid_rows != 0x0f case used to force by hand. */
        if (left < min_edge) min_edge = left;
        if (right > max_edge) max_edge = right;

        const int row_full_x0 = fixed_ceil_div(
            (int64_t)left - (int64_t)sample_min[row] * 8192, 65536);
        const int row_full_x1 = fixed_floor_div(
            (int64_t)right - 1 - (int64_t)sample_max[row] * 8192, 65536);
        if (row_full_x0 > full_x0) full_x0 = row_full_x0;
        if (row_full_x1 < full_x1) full_x1 = row_full_x1;
    }

    /* Every sample clipped the same way means the scanline missed the scissor
     * entirely on that side; the hardware drops it rather than drawing a span
     * pinned to the edge. */
    if (all_over || all_under) return false;
    if (min_edge > max_edge) return false;
    span->coverage.full_x0 = full_x0;
    span->coverage.full_x1 = full_x1;
    span->x0 = fixed_floor_div(min_edge, 65536);
    span->x1 = fixed_floor_div(max_edge, 65536);
    return span->x0 <= span->x1;
}

static void raster_setup_texel1_peek(const rdp_primitive_state *primitive,
                                     const rdp_state *state,
                                     const raster_edge_cursor *next_cursor,
                                     int y,
                                     int yl,
                                     rdp_span *work);
static sr_result triangle_rows_peek(sr_memory *memory,
                                    const rdp_state *state,
                                    const rdp_primitive_state *primitive,
                                    const raster_decoded_triangle *decoded,
                                    int yh, int yl, int terminal_y,
                                    bool fill_span, uint32_t stride,
                                    uint32_t offset);
static inline __attribute__((always_inline)) sr_result triangle_rows(
    sr_memory *memory, const rdp_state *state, const rdp_primitive_state *primitive,
    const raster_decoded_triangle *decoded, int yh, int yl, int terminal_y,
    bool fill_span, uint32_t stride, uint32_t offset, const bool peek);

static bool fill_triangle_span_for_y(const raster_triangle_setup *setup,
                                     const rdp_state *state,
                                     const raster_edge_cursor *cursor,
                                     int y,
                                     raster_span *span)
{
    *span = (raster_span){ .y = y };
    /* Empty scissor clips the fill triangle away; see triangle_span_for_y. Do
     * not expand a zero high edge into a full-screen default. */
    if (state->scissor_x1 <= state->scissor_x0 ||
        state->scissor_y1 <= state->scissor_y0) {
        return false;
    }
    /*
     * Fill mode has no coverage, so the span is the UNION of what the four
     * sub-scanlines touch, each truncated to a whole pixel - not the single
     * edge pair at the scanline centre. Sampling the centre and rounding the
     * leading edge up loses the one or two pixels the first and last
     * sub-scanline reach past it, which shows as a ragged edge one to three
     * pixels inside the hardware's on every sloped fill triangle.
     */
    const int sub_y0 = (int)scale_scissor(state->scissor_y0);
    const int sub_y1 = (int)scale_scissor(state->scissor_y1);
    const int32_t lo_scissor =
        (int32_t)(state->scissor_x0 << 1) * (int32_t)SOFTRDP_SCALE;
    const int32_t hi_scissor =
        (int32_t)(state->scissor_x1 << 1) * (int32_t)SOFTRDP_SCALE;
    int major_lo = 0x3fffffff, major_hi = -0x3fffffff;
    int minor_lo = 0x3fffffff, minor_hi = -0x3fffffff;
    bool all_over = true, all_under = true;
    for (int row = 0; row < 4; row++) {
        const int64_t y_sub = (int64_t)y * 4 + row;
        int32_t major_edge = raster_edge_snap(
            raster_edge_truncate(cursor->major[row]));
        int32_t minor_edge = raster_edge_snap(raster_edge_truncate(
            raster_minor_uses_lower(setup, y_sub) ?
                cursor->lower[row] : cursor->upper[row]));
        const int32_t unclipped_major = major_edge;
        const int32_t unclipped_minor = minor_edge;
        const int32_t edge_l = setup->flip ? major_edge : minor_edge;
        const int32_t edge_r = setup->flip ? minor_edge : major_edge;
        const int32_t near_edge = edge_l < edge_r ? edge_l : edge_r;
        all_over = all_over && near_edge >= hi_scissor;
        all_under = all_under && (edge_l > edge_r ? edge_l : edge_r) < lo_scissor;
        /* Horizontal rejection is a scanline-wide reduction. Subrows outside
         * the vertical interval still affect that reduction, although they do
         * not contribute endpoints to the resulting span. */
        if (y_sub < setup->yh || y_sub >= setup->yl) continue;
        if (y_sub < sub_y0 || y_sub >= sub_y1) continue;
        if (major_edge < lo_scissor) major_edge = lo_scissor;
        if (major_edge > hi_scissor) major_edge = hi_scissor;
        if (minor_edge < lo_scissor) minor_edge = lo_scissor;
        if (minor_edge > hi_scissor) minor_edge = hi_scissor;
        /* Crossed pairs are invalid sub-scanlines, not endpoints to fold into
         * the fill span. Check both sides of clipping: clipping must not turn
         * a crossed pair into a boundary pixel, while the clipped comparison
         * still catches pairs that cross at a scissor edge. */
        const bool crossed_before_clip =
            (setup->flip ? ((unclipped_minor >> 1) < (unclipped_major >> 1)) :
                           ((unclipped_major >> 1) < (unclipped_minor >> 1)));
        if (crossed_before_clip ||
            (setup->flip ? ((minor_edge >> 1) < (major_edge >> 1)) :
                           ((major_edge >> 1) < (minor_edge >> 1)))) {
            continue;
        }
        const int major = fixed_floor_div(major_edge, 8);
        const int minor = fixed_floor_div(minor_edge, 8);
        if (major < major_lo) major_lo = major;
        if (major > major_hi) major_hi = major;
        if (minor < minor_lo) minor_lo = minor;
        if (minor > minor_hi) minor_hi = minor;
    }
    if (major_lo > major_hi || all_over || all_under) return false;
    if (setup->flip) {
        span->x0 = major_lo;
        span->x1 = minor_hi;
    } else {
        span->x0 = minor_lo;
        span->x1 = major_hi;
    }
    /* Fill mode writes whole hardware pixels - there is no coverage to carry a
     * partly covered one. Widening the span to hardware-pixel boundaries is
     * what keeps a scaled render agreeing with the scale-1 image, which is the
     * hardware result; without it a span that starts on an odd raster column
     * leaves the pixel's leading sample clear and the resolve drops it. */
    span->x0 = sr_pixel_to_raster((int32_t)sr_raster_to_pixel((uint32_t)span->x0));
    span->x1 = sr_pixel_to_raster((int32_t)sr_raster_to_pixel((uint32_t)span->x1)) +
               (int)SOFTRDP_SCALE - 1;
    /* The hardware clamps an out-of-range edge TO the scissor boundary and then
     * truncates, so the pixel the boundary falls in is drawn at both ends -
     * the same rule the sampled path gets from clamping in fixed point. The
     * y range needs no separate test: the sub-scanline loop above already
     * dropped every sub-scanline outside the scissor. */
    const int min_x = (int)(scale_scissor(state->scissor_x0) >> 2);
    const int max_x = (int)(scale_scissor(state->scissor_x1) >> 2);
    if (span->x0 < min_x) span->x0 = min_x;
    if (span->x1 > max_x) span->x1 = max_x;
    /* Fill mode covers every pixel of the span, so the fully covered range spans
     * it end to end and the per-sample edges are never consulted. They still get
     * the empty interval so the structure carries no accidental coverage. */
    span->coverage.full_x0 = span->x0;
    span->coverage.full_x1 = span->x1;
    for (int row = 0; row < 4; row++) {
        span->coverage.left[row] = RASTER_COVERAGE_NEVER_LEFT;
        span->coverage.right[row] = RASTER_COVERAGE_NEVER_RIGHT;
    }
    return span->x0 <= span->x1;
}

sr_result raster_decode_triangle(const rdp_command *cmd, raster_decoded_triangle *out)
{
    if (!cmd || !out) {
        return SR_ERROR_INVALID_ARGUMENT;
    }

    *out = (raster_decoded_triangle){0};
    out->position = decode_triangle_setup(cmd);

    switch (cmd->id) {
    case RDP_CMD_FILL_TRIANGLE:
    case RDP_CMD_TEXTURE_TRIANGLE:
    case RDP_CMD_SHADE_TRIANGLE:
    case RDP_CMD_SHADE_TEXTURE_TRIANGLE:
        out->has_depth = true;
        out->depth = decode_header_depth_setup(&cmd->words[0]);
        break;
    default:
        break;
    }

    switch (cmd->id) {
    case RDP_CMD_FILL_TRIANGLE:                   return SR_OK;
    case RDP_CMD_FILL_ZBUFFER_TRIANGLE:           out->has_depth = true; out->depth = decode_depth_setup(&cmd->words[8]); return SR_OK;
    case RDP_CMD_TEXTURE_TRIANGLE:                out->has_texture = true; out->texture = decode_texture_setup(&cmd->words[8]); return SR_OK;
    case RDP_CMD_TEXTURE_ZBUFFER_TRIANGLE:        out->has_texture = out->has_depth = true; out->texture = decode_texture_setup(&cmd->words[8]); out->depth = decode_depth_setup(&cmd->words[24]); return SR_OK;
    case RDP_CMD_SHADE_TRIANGLE:                  out->has_shade = true; out->shade = decode_shade_setup(&cmd->words[8]); return SR_OK;
    case RDP_CMD_SHADE_ZBUFFER_TRIANGLE:          out->has_shade = out->has_depth = true; out->shade = decode_shade_setup(&cmd->words[8]); out->depth = decode_depth_setup(&cmd->words[24]); return SR_OK;
    case RDP_CMD_SHADE_TEXTURE_TRIANGLE:          out->has_shade = out->has_texture = true; out->shade = decode_shade_setup(&cmd->words[8]); out->texture = decode_texture_setup(&cmd->words[24]); return SR_OK;
    case RDP_CMD_SHADE_TEXTURE_ZBUFFER_TRIANGLE:  out->has_shade = out->has_texture = out->has_depth = true; out->shade = decode_shade_setup(&cmd->words[8]); out->texture = decode_texture_setup(&cmd->words[24]); out->depth = decode_depth_setup(&cmd->words[40]); return SR_OK;
    default:                                      return SR_ERROR_BAD_COMMAND;
    }
}

typedef struct raster_rect {
    uint32_t x0;
    uint32_t y0;
    uint32_t x1;
    uint32_t y1;
} raster_rect;

/*
 * Lift a clipped rectangle into raster space. One hardware pixel becomes a
 * SOFTRDP_SCALE square, so the high edge grows to the last raster pixel still
 * inside it. Applied after scissoring, which already rounds to whole hardware
 * pixels and so loses nothing by running unscaled.
 */
/*
 * Whether texture rectangles render with native-resolution coordinates. Both
 * halves of the condition matter: at scale 1 the two are the same thing, and
 * SOFTRDP_NATIVE_TEX_RECT can turn it off to see the difference.
 *
 * FILL_RECTANGLE needs no equivalent. Its bounds are whole hardware pixels
 * (clip_rect_to_scissor runs before scale_rect, in hardware units), scale_rect
 * maps them to whole sample blocks, and the colour does not vary across the
 * rectangle - so it is already exactly what scale 1 would produce, magnified.
 */
#if SOFTRDP_SCALE > 1 && SOFTRDP_NATIVE_TEX_RECT
#define SR_RECT_NATIVE 1
#else
#define SR_RECT_NATIVE 0
#endif

static inline void scale_rect(raster_rect *rect)
{
#if SOFTRDP_SCALE > 1
    rect->x0 *= SOFTRDP_SCALE;
    rect->y0 *= SOFTRDP_SCALE;
    rect->x1 = rect->x1 * SOFTRDP_SCALE + (SOFTRDP_SCALE - 1u);
    rect->y1 = rect->y1 * SOFTRDP_SCALE + (SOFTRDP_SCALE - 1u);
#else
    (void)rect;
#endif
}

/* Every primitive drawn to an 8bpp image drops bit 1 of two phases of the
 * memory interface's latched ninth-bit pair. */
static void clear_hidden_latch_phases(sr_memory *memory, const rdp_state *state)
{
    if (state->color_image.size != RDP_SIZE_8BPP) return;
    for (uint32_t sample = 0; sample < SR_SCALE_SAMPLES; sample++) {
        memory->copy_hidden_latch[sample][0] &= 1u;
        memory->copy_hidden_latch[sample][4] &= 1u;
    }
}

static bool clip_rect_to_scissor(raster_rect *rect, const rdp_state *state)
{
    /* Scissor high edges are literal 10.2 fixed-point values. A high edge of 0
     * (or any x1 <= x0 / y1 <= y0) describes an EMPTY clip region, not an
     * unbounded one. Treating 0 as "no limit" let a degenerate FILL_RECT with a
     * huge rectangle write across all of RDRAM -- including the CPU exception
     * vectors -- crashing games that legitimately scissor to nothing. */
    if (state->scissor_x1 <= state->scissor_x0 ||
        state->scissor_y1 <= state->scissor_y0) {
        return false;
    }
    /* Fill and copy keep any pixel or line that a low scissor edge only
     * partially clips, and treat the high x edge as inclusive. */
    const bool whole_pixels = state->other_modes.cycle_type == RDP_CYCLE_FILL ||
                              state->other_modes.cycle_type == RDP_CYCLE_COPY;
    const uint32_t round_up = whole_pixels ? 0u : 3u;
    if (state->scissor_x0 > rect->x0 << 2) {
        rect->x0 = (state->scissor_x0 + round_up) >> 2;
    }
    if (state->scissor_y0 > rect->y0 << 2) {
        rect->y0 = (state->scissor_y0 + round_up) >> 2;
    }
    const uint32_t x1_limit = whole_pixels ? state->scissor_x1 >> 2
                                           : (state->scissor_x1 - 1u) >> 2;
    if (rect->x1 > x1_limit) {
        rect->x1 = x1_limit;
    }
    if (state->scissor_y1 <= rect->y1 << 2) {
        rect->y1 = (state->scissor_y1 - 1u) >> 2;
    }

    return rect->x0 <= rect->x1 && rect->y0 <= rect->y1;
}

sr_result raster_submit_triangle(sr_memory *memory,
                                 tmem_state *tmem,
                                 const rdp_state *state,
                                 const rdp_command *cmd)
{
    if (!cmd) {
        return SR_ERROR_INVALID_ARGUMENT;
    }
    raster_decoded_triangle decoded = cmd->decoded.triangle;
    scale_triangle_setup(&decoded.position);

    /*
     * The opcode selects the payload size, not the rasterization rule. Builders
     * may emit the full shade/texture/depth payload even when fill cycle ignores
     * all attributes. Conversely, a short 0x08 payload in one- or two-cycle mode
     * still uses normal sampled coverage and the configured color pipeline.
     */
    const bool fill_cycle = state->other_modes.cycle_type == RDP_CYCLE_FILL;
    const bool fill_span = fill_cycle;
    const bool byte_copy =
        state->other_modes.cycle_type == RDP_CYCLE_COPY &&
        state->color_image.size <= RDP_SIZE_8BPP;
    /* Byte-pair latch state is command-ordered, so scanlines using it cannot
     * be split between workers. The lead worker reconstructs the full draw. */
    const bool ordered_hidden = byte_copy ||
        state->color_image.size == RDP_SIZE_8BPP;
    if (ordered_hidden && state->worker_stride > 1u &&
        state->worker_offset != 0u)
        return SR_OK;
    clear_hidden_latch_phases(memory, state);
    /* A scanline participates if ANY of its four sub-scanlines falls inside
     * [yh, yl) - the same rule in fill mode as everywhere else, since the fill
     * span is the union of the sub-scanlines rather than the centre sample.
     * Rows that turn out to contribute nothing are dropped by the span itself. */
    const int yh = fixed_floor_div(decoded.position.yh, 4);
    const int yl = fixed_ceil_div(decoded.position.yl, 4);
    const int scissor_last_y =
        ((int)scale_scissor(state->scissor_y1) - 1) >> 2;
    const int terminal_y = yl - 1 < scissor_last_y ? yl - 1 : scissor_last_y;

    if (yl <= yh) {
        return SR_OK;
    }

    const rdp_primitive_state *primitive =
        primitive_compile_triangle_cached(state, tmem, &decoded, fill_cycle);

    /* Scanline-parallel worker assignment: this call renders only the
     * scanlines owned by its worker. Default stride 1 / offset 0 renders every
     * scanline (single-threaded). The edge cursor is walked for every scanline
     * regardless, so each worker independently reconstructs edge state. */
    const uint32_t stride = ordered_hidden ? 1u
        : (state->worker_stride ? state->worker_stride : 1u);
    const uint32_t offset = ordered_hidden ? 0u : state->worker_offset;

    const bool texel1_peek = !fill_span && decoded.has_texture &&
        primitive->color.next_texel && !primitive->color.two_cycle &&
        (primitive->plan.stages & RDP_DRAW_TEXTURE) != 0u;
    return texel1_peek
        ? triangle_rows_peek(memory, state, primitive, &decoded, yh, yl, terminal_y,
                             fill_span, stride, offset)
        : triangle_rows(memory, state, primitive, &decoded, yh, yl, terminal_y,
                        fill_span, stride, offset, false);
}

/* The scanlines of a triangle. Only the instance with peek sets up the
 * one-cycle TEXEL1 hand-over, so the common loop carries none of it. */
static inline __attribute__((always_inline)) sr_result triangle_rows(
    sr_memory *memory, const rdp_state *state, const rdp_primitive_state *primitive,
    const raster_decoded_triangle *decoded, int yh, int yl, int terminal_y,
    bool fill_span, uint32_t stride, uint32_t offset, const bool peek)
{
    raster_edge_cursor edge_cursor;
    raster_edge_cursor_init(&edge_cursor, &decoded->position, yh);

    for (int y = yh; y < yl; y++) {
        raster_span span;
        const bool has_span = fill_span
            ? fill_triangle_span_for_y(&decoded->position, state, &edge_cursor, y, &span)
            : triangle_span_for_y(&decoded->position, state, &edge_cursor, y, &span);
        raster_edge_cursor_advance(&edge_cursor, &decoded->position);
        if (!has_span) {
            continue;
        }
        if (!scissor_accepts_raster_y(state, (uint32_t)y)) {
            continue;
        }
        /* Ownership must key on the ABSOLUTE scanline, not one relative to this
         * primitive's start. A relative key flips parity with each primitive's
         * yh, so two workers can write the same screen line concurrently - there
         * is no barrier between commands. */
        if (((uint32_t)y % stride) != offset) {
            continue;
        }

        rdp_span work;
        raster_setup_triangle_span(primitive, span.x0, span.x1, y, &work);
        work.copy_primitive_terminal = y == terminal_y;
        work.coverage = span.coverage;
        if (peek)
            raster_setup_texel1_peek(primitive, state, &edge_cursor, y, yl, &work);
        const sr_result result = kernel_render_span(memory, primitive, &work);
        if (result != SR_OK) {
            return result;
        }
    }

    return SR_OK;
}

static __attribute__((noinline)) sr_result triangle_rows_peek(
    sr_memory *memory, const rdp_state *state, const rdp_primitive_state *primitive,
    const raster_decoded_triangle *decoded, int yh, int yl, int terminal_y,
    bool fill_span, uint32_t stride, uint32_t offset)
{
    return triangle_rows(memory, state, primitive, decoded, yh, yl, terminal_y,
                         fill_span, stride, offset, true);
}

/*
 * Rectangle edge coverage.
 *
 * A rectangle is the degenerate triangle whose two edges are vertical, so its
 * sub-pixel span is identical on every scanline. That is built once per
 * rectangle; per scanline only the set of live sub-rows can change, and only on
 * the first and last one.
 */
typedef struct raster_rect_coverage {
    raster_coverage_span all_rows;
    uint32_t y_top;     /* quarter scanlines, inclusive */
    uint32_t y_bottom;  /* quarter scanlines, exclusive */
} raster_rect_coverage;

static void rectangle_coverage_setup(raster_rect_coverage *out,
                                     const rdp_state *state,
                                     const rdp_rect_cmd *cmd)
{
    static const int sample_min[4] = { 0, 2, 0, 2 };
    static const int sample_max[4] = { 4, 6, 4, 6 };

    /*
     * Every bound is lifted into raster space before it is compared, the same
     * way triangle_span_for_y does it. The span built here is evaluated against
     * raster x in the pixel loop and raster sub-scanlines in
     * rectangle_coverage_for_y, while the rectangle edges, the scissor and the
     * sub-scanline bounds all arrive in hardware units. Leaving any of them
     * unscaled collapses the fully covered range above scale 1, which rejects
     * every lane and draws nothing.
     */
    /* Quarter pixels to 16.16, in raster space. */
    const int32_t scissor_x0 = (int32_t)scale_scissor(state->scissor_x0) << 14;
    const int32_t scissor_x1 = (int32_t)scale_scissor(state->scissor_x1) << 14;
    const int32_t left = clamp_edge_x(((int64_t)cmd->xh * SOFTRDP_SCALE) << 14,
                                      scissor_x0, scissor_x1);
    const int32_t right = clamp_edge_x(((int64_t)cmd->xl * SOFTRDP_SCALE) << 14,
                                       scissor_x0, scissor_x1);
    const bool empty = left >= right;

    /* Quarter scanlines, likewise in raster space. */
    const uint32_t rect_top = (uint32_t)cmd->yh * SOFTRDP_SCALE;
    const uint32_t rect_bottom = (uint32_t)cmd->yl * SOFTRDP_SCALE;
    const uint32_t scissor_y0 = scale_scissor(state->scissor_y0);
    const uint32_t scissor_y1 = scale_scissor(state->scissor_y1);
    out->y_top = rect_top > scissor_y0 ? rect_top : scissor_y0;
    out->y_bottom = rect_bottom < scissor_y1 ? rect_bottom : scissor_y1;

    int full_x0 = -0x3fffffff;
    int full_x1 = 0x3fffffff;
    for (int row = 0; row < 4; row++) {
        out->all_rows.left[row] = empty ? RASTER_COVERAGE_NEVER_LEFT : left;
        out->all_rows.right[row] = empty ? RASTER_COVERAGE_NEVER_RIGHT : right;
        const int row_full_x0 = fixed_ceil_div(
            (int64_t)out->all_rows.left[row] - (int64_t)sample_min[row] * 8192, 65536);
        const int row_full_x1 = fixed_floor_div(
            (int64_t)out->all_rows.right[row] - 1 - (int64_t)sample_max[row] * 8192, 65536);
        if (row_full_x0 > full_x0) full_x0 = row_full_x0;
        if (row_full_x1 < full_x1) full_x1 = row_full_x1;
    }
    out->all_rows.full_x0 = full_x0;
    out->all_rows.full_x1 = full_x1;
}

static void rectangle_coverage_for_y(raster_coverage_span *coverage,
                                     const raster_rect_coverage *source,
                                     uint32_t y)
{
    *coverage = source->all_rows;

    const uint32_t y_sub = y * 4u;
    /* Interior scanlines are the overwhelming majority and reuse the prebuilt
     * span untouched, which keeps the fully covered range intact so their
     * pixels never reach the per-sample evaluation at all. */
    if (y_sub >= source->y_top && y_sub + 4u <= source->y_bottom) {
        return;
    }

    for (uint32_t row = 0; row < 4u; row++) {
        const uint32_t sub = y_sub + row;
        if (sub < source->y_top || sub >= source->y_bottom) {
            coverage->left[row] = RASTER_COVERAGE_NEVER_LEFT;
            coverage->right[row] = RASTER_COVERAGE_NEVER_RIGHT;
        }
    }
    /* Dropping sub-rows lowers coverage across the whole scanline, interior
     * included, so the fully covered shortcut has to go. */
    coverage->full_x0 = 1;
    coverage->full_x1 = 0;
}

static sr_result submit_texture_rectangle(sr_memory *memory,
                                          tmem_state *tmem,
                                          const rdp_state *state,
                                          const rdp_command *cmd)
{
    const rdp_rect_cmd *rect_cmd = &cmd->decoded.rect;
    const uint32_t tile_index = rect_cmd->tile_index;
    const bool copy = state->other_modes.cycle_type == RDP_CYCLE_COPY;
    /* A copy-mode fill's zeroed (s, t, w) goes through the perspective
     * divide, and the copy pipe has no clamp stage: the w <= 0 carry
     * saturates both coordinates to +0x7fff, which fetches texel
     * (1023, 1023). */
    const bool fill_saturates = cmd->id == RDP_CMD_FILL_RECTANGLE &&
        state->other_modes.perspective;
    const int32_t s0 = (fill_saturates ? 0x7fff : rect_cmd->s0) * 32;
    const int32_t t0 = (fill_saturates ? 0x7fff : rect_cmd->t0) * 32;
    const bool flip = rect_cmd->flip;
    const bool byte_copy = copy && state->color_image.size <= RDP_SIZE_8BPP;
    /* See the triangle path: this latch is shared by the whole command. */
    const bool ordered_hidden = byte_copy ||
        state->color_image.size == RDP_SIZE_8BPP;
    if (ordered_hidden && state->worker_stride > 1u &&
        state->worker_offset != 0u)
        return SR_OK;
    clear_hidden_latch_phases(memory, state);
    /* TexRectFlip transposes the command derivatives: command dsdx advances S
     * vertically and command dtdy advances T horizontally.  In copy mode the
     * horizontal derivative is applied once per 64-bit copy group. */
    const int32_t dsdy = flip ? rect_cmd->dsdx : 0;
    const int32_t dtdx = flip ? rect_cmd->dtdy : 0;
    const int32_t dsdx = flip ? 0 : rect_cmd->dsdx;
    const int32_t dtdy = flip ? 0 : rect_cmd->dtdy;
    /* Outside copy mode the coordinates are interpolated from the exact xh
     * rather than from its whole pixel. */
    const int64_t xfrac = copy ? 0 : (int64_t)(rect_cmd->xh & 3u);
    const int32_t s_start = s0 + (int32_t)((-(int64_t)dsdx * xfrac) >> 2);
    const int32_t t_start = t0 + (int32_t)((-(int64_t)dtdx * xfrac) >> 2);

    raster_rect rect = {
        .x0 = rect_cmd->x0,
        .y0 = rect_cmd->y0,
        .x1 = rect_cmd->x1,
        .y1 = rect_cmd->y1
    };
    if (state->other_modes.cycle_type == RDP_CYCLE_1 ||
        state->other_modes.cycle_type == RDP_CYCLE_2) {
        if (rect_cmd->xl == 0u || rect_cmd->yl == 0u) return SR_OK;
        rect.x1 = (rect_cmd->xl - 1u) >> 2;
        rect.y1 = (rect_cmd->yl - 1u) >> 2;
    }
    if (!clip_rect_to_scissor(&rect, state)) {
        return SR_OK;
    }
    /* The texture origin is anchored at the unclipped rectangle, so it scales
     * with the geometry rather than being derived from the clipped bounds. */
    const uint32_t base_x = (uint32_t)rect_cmd->x0 * SOFTRDP_SCALE;
    const uint32_t base_y = (uint32_t)rect_cmd->y0 * SOFTRDP_SCALE;
    scale_rect(&rect);

    rdp_primitive_state primitive;
    primitive_compile_rectangle(&primitive, state, tmem, tile_index);

    /* Only one- and two-cycle rectangles carry edge coverage. Copy and fill
     * snap to whole samples on the hardware, and leaving the span untouched
     * makes raster_setup_rectangle_span's "everything is covered" default
     * stand. */
    const bool edge_coverage = state->other_modes.cycle_type == RDP_CYCLE_1 ||
                               state->other_modes.cycle_type == RDP_CYCLE_2;
    raster_rect_coverage rect_coverage;
    if (edge_coverage) {
        rectangle_coverage_setup(&rect_coverage, state, rect_cmd);
    }

    /* Render only this worker's scanlines; otherwise every worker would render
     * the whole rectangle and blended rectangles would be composited twice. */
    const uint32_t stride = ordered_hidden ? 1u
        : (state->worker_stride ? state->worker_stride : 1u);
    const uint32_t offset = ordered_hidden ? 0u : state->worker_offset;

    for (uint32_t y = rect.y0; y <= rect.y1; y++) {
        if (!scissor_accepts_raster_y(state, y)) {
            continue;
        }
        if ((y % stride) != offset) {
            continue;
        }
        /* dx and dy count raster steps into a derivative expressed per hardware
         * pixel, so they go through the same snapped accumulation the triangle
         * attributes use. */
        /* Copy groups count from the clipped span start, so clipping does not
         * advance the texture coordinate. */
        const int32_t dx = copy ? 0 : (int32_t)(rect.x0 - base_x);
        const int32_t dy = (int32_t)(y - base_y);
#if SR_RECT_NATIVE
        /* Native-resolution coordinates: drop the raster steps within a
         * hardware pixel and keep the full per-hardware-pixel derivative, so
         * every sample of that pixel samples the texel scale 1 would have. */
        const int32_t s_fixed = (int32_t)(s_start + sr_native_steps(dsdx, dx) +
                                               sr_native_steps(dsdy, dy));
        const int32_t t_fixed = (int32_t)(t_start + sr_native_steps(dtdx, dx) +
                                               sr_native_steps(dtdy, dy));
        const int32_t span_dsdx = dsdx;
        const int32_t span_dtdx = dtdx;
#else
        const int32_t s_fixed = (int32_t)(s_start + sr_scaled_steps(dsdx, dx) +
                                               sr_scaled_steps(dsdy, dy));
        const int32_t t_fixed = (int32_t)(t_start + sr_scaled_steps(dtdx, dx) +
                                               sr_scaled_steps(dtdy, dy));
        const int32_t span_dsdx = sr_scaled_derivative(dsdx);
        const int32_t span_dtdx = sr_scaled_derivative(dtdx);
#endif
        rdp_span work;

        raster_setup_rectangle_span((int)rect.x0,
                                      (int)rect.x1,
                                      (int)y,
                                      s_fixed,
                                      t_fixed,
                                      span_dsdx,
                                      span_dtdx,
                                      &work);
        if (edge_coverage) {
            rectangle_coverage_for_y(&work.coverage, &rect_coverage, y);
        }
        work.texture_coord_shift = 5u;
#if SR_RECT_NATIVE
        /* rect.x0 is a scaled hardware pixel, so the span's column offset shifts
         * down to a hardware column exactly. */
        work.texture_step_shift = (uint8_t)SR_SCALE_LOG2;
#endif
        if (copy) {
            work.dsdx_fixed = dsdx;
            work.dtdx_fixed = dtdx;
            work.texture_step_shift = (uint8_t)SR_SCALE_LOG2;
        }
        sr_result result = kernel_render_span(memory, &primitive, &work);
        if (result != SR_OK) {
            return result;
        }
    }

    return SR_OK;
}

static sr_result raster_submit_rectangle_internal(sr_memory *memory,
                                                  tmem_state *tmem,
                                                  const rdp_state *state,
                                                  const rdp_command *cmd)
{
    raster_rect rect;

    /* Fill_Rect in copy mode runs the copy pipe with all-zero texture
     * attributes (tile 0, s = t = 0, no derivatives), which decode_rect
     * leaves in place; they are not the ones a previous command latched. */
    const bool texture_rect = cmd->id == RDP_CMD_TEXTURE_RECTANGLE ||
                              cmd->id == RDP_CMD_TEXTURE_RECTANGLE_FLIP;
    const bool fill_cycle = state->other_modes.cycle_type == RDP_CYCLE_FILL;
    if ((texture_rect && !fill_cycle) ||
        (cmd->id == RDP_CMD_FILL_RECTANGLE &&
         state->other_modes.cycle_type == RDP_CYCLE_COPY)) {
        return submit_texture_rectangle(memory, tmem, state, cmd);
    }

    /* In fill mode a Texture_Rectangle is drawn like a Fill_Rect: the fill
     * pipe writes the fill colour over the same bounds and ignores the
     * texture. */
    if (!texture_rect && cmd->id != RDP_CMD_FILL_RECTANGLE) {
        return SR_OK;
    }
    /* Keep the command-global byte latch on the lead worker. */
    const bool ordered_hidden = state->color_image.size == RDP_SIZE_8BPP;
    if (ordered_hidden && state->worker_stride > 1u &&
        state->worker_offset != 0u)
        return SR_OK;
    clear_hidden_latch_phases(memory, state);

    rect.x0 = cmd->decoded.rect.x0;
    rect.y0 = cmd->decoded.rect.y0;
    rect.x1 = cmd->decoded.rect.x1;
    rect.y1 = cmd->decoded.rect.y1;

    if (state->other_modes.cycle_type == RDP_CYCLE_FILL ||
        state->other_modes.cycle_type == RDP_CYCLE_COPY) {
        rect.y1 = ((cmd->words[0] & 0xfffu) | 3u) >> 2;
    }

    if (!clip_rect_to_scissor(&rect, state)) {
        return SR_OK;
    }

    if (state->other_modes.cycle_type != RDP_CYCLE_FILL &&
        state->other_modes.cycle_type != RDP_CYCLE_COPY) {
        rdp_primitive_state primitive;
        primitive_compile_color_rectangle(&primitive, state, tmem);
        raster_rect_coverage rect_coverage;
        rectangle_coverage_setup(&rect_coverage, state, &cmd->decoded.rect);
        const uint32_t stride = state->worker_stride ? state->worker_stride : 1u;
        const uint32_t offset = state->worker_offset;
        scale_rect(&rect);
        for (uint32_t y = rect.y0; y <= rect.y1; y++) {
            if (!scissor_accepts_raster_y(state, y)) {
                continue;
            }
            if ((y % stride) != offset) {
                continue;
            }
            rdp_span work;
            raster_setup_rectangle_span((int)rect.x0, (int)rect.x1, (int)y,
                                          0, 0, 0, 0, &work);
            rectangle_coverage_for_y(&work.coverage, &rect_coverage, y);
            const sr_result result = kernel_render_span(memory, &primitive, &work);
            if (result != SR_OK) return result;
        }
        return SR_OK;
    }

    rdp_framebuffer_state framebuffer;
    primitive_compile_framebuffer(&framebuffer, state);
    /* Fill takes raster coordinates like every other render path, so each
     * sample of a covered hardware pixel is filled in its own right. A no-op at
     * scale 1, where the two grids are the same. */
    scale_rect(&rect);
    if (state->scissor_field) {
        for (uint32_t y = rect.y0; y <= rect.y1; y++) {
            if (!scissor_accepts_raster_y(state, y)) {
                continue;
            }
            const sr_result result = framebuffer_fill_rect(
                memory, &framebuffer, rect.x0, y, rect.x1, y,
                ordered_hidden ? 0u : state->worker_offset,
                ordered_hidden ? 1u : state->worker_stride);
            if (result != SR_OK) return result;
        }
        return SR_OK;
    }
    return framebuffer_fill_rect(memory,
                                 &framebuffer,
                                 rect.x0,
                                 rect.y0,
                                  rect.x1,
                                  rect.y1,
                                  ordered_hidden ? 0u : state->worker_offset,
                                  ordered_hidden ? 1u : state->worker_stride);
}

sr_result raster_submit_rectangle(sr_memory *memory,
                                  tmem_state *tmem,
                                  const rdp_state *state,
                                  const rdp_command *cmd)
{
    return raster_submit_rectangle_internal(memory, tmem, state, cmd);
}

/*
 * Span-buffer stale read. The command processor runs ahead of the pixel
 * pipeline: a span of W columns at cyc cycles per pixel takes
 * L = max(cyc*W + cyc - 1, 4) clocks, and the processor leads by
 * D = min(3L - 2, 25). When D > L, a rectangle's framebuffer reads precede its
 * predecessor's commit of the same pixels and see the image from before the
 * predecessor ran, so a stack of repeats advances every other primitive.
 * Only the measured shape is modelled: identical single-row 1-/2-cycle
 * Fill_Rects issued back to back into a 16bpp image with image_read set and
 * atomic_prim clear. Returns the samples of the footprint this worker renders,
 * or 0 when the rectangle is not eligible.
 */
static uint32_t rect_stale_footprint(const sr_memory *memory, const rdp_state *state,
                                     const rdp_command *cmd, uint32_t *slots,
                                     uint32_t *samples)
{
    const rdp_other_modes *modes = &state->other_modes;
    const rdp_rect_cmd *r = &cmd->decoded.rect;
    if ((modes->cycle_type != RDP_CYCLE_1 && modes->cycle_type != RDP_CYCLE_2) ||
        !modes->image_read || modes->atomic_prim ||
        state->color_image.size != RDP_SIZE_16BPP)
        return 0u;
    const uint32_t w = (uint32_t)(r->xl >> 2) - (uint32_t)(r->xh >> 2);
    const uint32_t h = (uint32_t)(r->yl >> 2) - (uint32_t)(r->yh >> 2);
    const uint32_t cyc = modes->cycle_type == RDP_CYCLE_2 ? 2u : 1u;
    const uint32_t span = cyc * w + cyc - 1u;
    const uint32_t l = span < 4u ? 4u : span;
    const uint32_t d = 3u * l - 2u < 25u ? 3u * l - 2u : 25u;
    if (h != 1u || w < 1u || w > 32u || d <= l) return 0u;

    raster_rect rect = { .x0 = r->x0, .y0 = r->y0, .x1 = r->x1, .y1 = r->y1 };
    if (!clip_rect_to_scissor(&rect, state)) return 0u;
    scale_rect(&rect);
    rdp_framebuffer_state framebuffer;
    primitive_compile_framebuffer(&framebuffer, state);
    const uint32_t stride = state->worker_stride ? state->worker_stride : 1u;
    uint32_t count = 0u;
    for (uint32_t y = rect.y0; y <= rect.y1; y++) {
        if (!scissor_accepts_raster_y(state, y) || (y % stride) != state->worker_offset)
            continue;
        for (uint32_t x = rect.x0; x <= rect.x1; x++) {
            const uint32_t slot = framebuffer_slot(memory, &framebuffer, x, y);
            if (slot == FB_SLOT_NONE) continue;
            if (count == RDP_RECT_STALE_MAX) return 0u;
            slots[count] = slot;
            samples[count] = sr_sample_of(x, y);
            count++;
        }
    }
    return count;
}

sr_result raster_submit_fill_rectangle(sr_memory *memory, tmem_state *tmem,
                                       rdp_state *state, const rdp_command *cmd)
{
    rdp_rect_stale *stale = &state->rect_stale;
    uint32_t slots[RDP_RECT_STALE_MAX], samples[RDP_RECT_STALE_MAX];
    const uint32_t count = memory
        ? rect_stale_footprint(memory, state, cmd, slots, samples) : 0u;
    if (count == 0u) {
        stale->valid = false;
        return raster_submit_rectangle(memory, tmem, state, cmd);
    }
    const bool repeat = stale->valid && stale->count == count &&
        stale->words[0] == cmd->words[0] && stale->words[1] == cmd->words[1];
    uint16_t image[RDP_RECT_STALE_MAX];
    for (uint32_t i = 0; i < count; i++)
        image[i] = (uint16_t)framebuffer_load_raw(memory, RDP_SIZE_16BPP,
                                                  slots[i], samples[i]);
    /* A repeat reads its predecessor's pre-image: put it back, draw over it,
     * and pass the predecessor's output on as the next repeat's pre-image. */
    if (repeat)
        for (uint32_t i = 0; i < count; i++)
            framebuffer_store_raw(memory, RDP_SIZE_16BPP, slots[i], stale->pre[i],
                                  samples[i]);
    const sr_result result = raster_submit_rectangle(memory, tmem, state, cmd);
    stale->valid = true;
    stale->count = count;
    stale->words[0] = cmd->words[0];
    stale->words[1] = cmd->words[1];
    memcpy(stale->pre, image, count * sizeof(image[0]));
    return result;
}

/* The attribute latch of scanline y: its value at the major edge pixel, which
 * is returned in base_x_out. The same arithmetic as interpolate_attribute,
 * kept apart so that function's per-span inlining is untouched; only the
 * TEXEL1 peek calls this. */
static __attribute__((noinline)) int64_t interpolate_attribute_line(const raster_decoded_triangle *decoded,
                                          int32_t base,
                                          int32_t ddx,
                                          int32_t dde,
                                          int32_t ddy,
                                          int y,
                                          bool suppress_x,
                                          int *base_x_out)
{
    const bool sign_dxhdy = decoded->position.dxhdy < 0;
    const bool do_offset = sign_dxhdy == decoded->position.flip;
    /* Both of these model hardware quantisations, so they snap at hardware
     * precision and the result is carried back into raster units. Snapping the
     * scaled value directly quantises to a fraction of a scanline and shifts
     * every attribute by a raster row for half of all triangles. */
    const int y_base = sr_shift_scaled(decoded->position.yh, 2u);
    const int dy = y - y_base;
    /* The offset latches the varyings at the last sub-row of the HARDWARE
     * scanline, so the count of sub-rows scales with the resolution. */
    const int ldflag = do_offset ? sr_hardware_subrows(3) : 0;
    const int subpixel_steps = (y << 2) + ldflag -
                               sr_snap_scaled(decoded->position.yh, 2u);
    /* X advances at quarter-pixel precision and truncates the slope on every
     * step. A direct dy*dxhdy reconstruction can cross an attribute boundary
     * even when the covered pixels are otherwise identical. */
    int64_t xh = (decoded->position.xh & ~1) +
                 (int64_t)subpixel_steps * ((decoded->position.dxhdy >> 2) & ~1);
    int32_t diff = 0;

    if (do_offset) {
        /* Three quarters of a HARDWARE scanline, and deliberately not scaled.
         * The offset it corrects for is expressed in the x nudge above, which
         * is scaled instead; scaling both would apply it twice. */
        const int32_t ddeh = dde & ~0x1ff;
        const int32_t ddyh = ddy & ~0x1ff;
        diff = ddeh - (ddeh >> 2) - ddyh + (ddyh >> 2);
    }

    const int base_x = (int)(xh >> 16);
    const int xfrac = suppress_x ? 0 : (int)((xh >> 8) & 0xff);
    /* dy and (x - base_x) count raster steps, which are fractions of the
     * hardware step these derivatives describe. sr_scaled_steps keeps the whole
     * hardware steps at full precision instead of halving the derivative up
     * front and losing a bit on every one of them. xfrac is a fraction of a
     * raster pixel, so its correction uses the per-raster-step derivative. */
    int64_t value = (int64_t)base + sr_scaled_steps(dde, dy);
    value = ((value & ~0x1ffll) + diff -
             (int64_t)xfrac * ((sr_scaled_derivative(ddx) >> 8) & ~1)) & ~0x3ffll;
    *base_x_out = base_x;
    return value;
}

static int32_t interpolate_attribute(const raster_decoded_triangle *decoded,
                                     int32_t base,
                                     int32_t ddx,
                                     int32_t dde,
                                     int32_t ddy,
                                     int x,
                                     int y,
                                     bool truncate_dx,
                                     bool suppress_x)
{
    const bool sign_dxhdy = decoded->position.dxhdy < 0;
    const bool do_offset = sign_dxhdy == decoded->position.flip;
    /* Both of these model hardware quantisations, so they snap at hardware
     * precision and the result is carried back into raster units. Snapping the
     * scaled value directly quantises to a fraction of a scanline and shifts
     * every attribute by a raster row for half of all triangles. */
    const int y_base = sr_shift_scaled(decoded->position.yh, 2u);
    const int dy = y - y_base;
    /* The offset latches the varyings at the last sub-row of the HARDWARE
     * scanline, so the count of sub-rows scales with the resolution. */
    const int ldflag = do_offset ? sr_hardware_subrows(3) : 0;
    const int subpixel_steps = (y << 2) + ldflag -
                               sr_snap_scaled(decoded->position.yh, 2u);
    /* X advances at quarter-pixel precision and truncates the slope on every
     * step. A direct dy*dxhdy reconstruction can cross an attribute boundary
     * even when the covered pixels are otherwise identical. */
    int64_t xh = (decoded->position.xh & ~1) +
                 (int64_t)subpixel_steps * ((decoded->position.dxhdy >> 2) & ~1);
    int32_t diff = 0;

    if (do_offset) {
        /* Three quarters of a HARDWARE scanline, and deliberately not scaled.
         * The offset it corrects for is expressed in the x nudge above, which
         * is scaled instead; scaling both would apply it twice. */
        const int32_t ddeh = dde & ~0x1ff;
        const int32_t ddyh = ddy & ~0x1ff;
        diff = ddeh - (ddeh >> 2) - ddyh + (ddyh >> 2);
    }

    const int base_x = (int)(xh >> 16);
    const int xfrac = suppress_x ? 0 : (int)((xh >> 8) & 0xff);
    /* dy and (x - base_x) count raster steps, which are fractions of the
     * hardware step these derivatives describe. sr_scaled_steps keeps the whole
     * hardware steps at full precision instead of halving the derivative up
     * front and losing a bit on every one of them. xfrac is a fraction of a
     * raster pixel, so its correction uses the per-raster-step derivative. */
    int64_t value = (int64_t)base + sr_scaled_steps(dde, dy);
    value = ((value & ~0x1ffll) + diff -
             (int64_t)xfrac * ((sr_scaled_derivative(ddx) >> 8) & ~1)) & ~0x3ffll;
    /* Integer displacement from the attribute latch uses the quantized step
     * when travelling in its forward direction. A reverse traversal retains
     * the decoded derivative, matching the subsequent per-pixel steps. */
    const int32_t x_step = truncate_dx ? ddx & ~0x1f : ddx;
    if (!suppress_x)
        value += sr_scaled_steps(x_step, (int64_t)(x - base_x));
    return (int32_t)(uint32_t)value;
}

/*
 * The end-of-span TEXEL1 peek (see rdp_span). The span length counts from the
 * attribute latch to the far end in walking order; the next scanline is live
 * when it produces a span of its own. next_cursor is positioned at y + 1.
 */
static void raster_setup_texel1_peek(const rdp_primitive_state *primitive,
                                     const rdp_state *state,
                                     const raster_edge_cursor *next_cursor,
                                     int y,
                                     int yl,
                                     rdp_span *work)
{
    const raster_decoded_triangle *decoded = &primitive->triangle;
    int base_x;
    (void)interpolate_attribute_line(decoded, decoded->texture.s, decoded->texture.dsdx,
                                     decoded->texture.dsde, decoded->texture.dsdy,
                                     y, false, &base_x);
    const int length = decoded->position.flip ? work->x_end - base_x
                                              : base_x - work->x_begin;
    const int next_y = y + 1;
    raster_span next;
    if (length < 8 || next_y >= yl ||
        !scissor_accepts_raster_y(state, (uint32_t)next_y) ||
        !triangle_span_for_y(&decoded->position, state, next_cursor, next_y, &next))
        return;
    work->texel1_peek = true;
    work->peek_s = (int32_t)(uint32_t)interpolate_attribute_line(decoded,
        decoded->texture.s, decoded->texture.dsdx, decoded->texture.dsde,
        decoded->texture.dsdy, next_y, false, &base_x);
    work->peek_t = (int32_t)(uint32_t)interpolate_attribute_line(decoded,
        decoded->texture.t, decoded->texture.dtdx, decoded->texture.dtde,
        decoded->texture.dtdy, next_y, false, &base_x);
    if (primitive->texture.perspective)
        work->peek_w = (int32_t)(uint32_t)interpolate_attribute_line(decoded,
            decoded->texture.w, decoded->texture.dwdx, decoded->texture.dwde,
            decoded->texture.dwdy, next_y, false, &base_x);
}

void raster_setup_triangle_span(const rdp_primitive_state *primitive,
                                  int x_begin,
                                  int x_end,
                                  int y,
                                  rdp_span *work)
{
    if (!primitive || !work) {
        return;
    }

    const raster_decoded_triangle *decoded = &primitive->triangle;
    memset(work, 0, sizeof(*work));
    work->x_begin = x_begin;
    work->x_end = x_end;
    work->y = y;
    work->coverage.full_x0 = x_begin;
    work->coverage.full_x1 = x_end;
    /* Default: the whole span counts as fully covered, so the per-sample edges
     * are unreachable and carry the empty interval. Callers that have real edge
     * data overwrite work->coverage wholesale. */
    for (uint32_t row = 0; row < 4u; row++) {
        work->coverage.left[row] = RASTER_COVERAGE_NEVER_LEFT;
        work->coverage.right[row] = RASTER_COVERAGE_NEVER_RIGHT;
    }

    if (decoded->has_depth) {
        /* Preserve the endpoint at which the fixed-point depth latch is
         * evaluated. Reconstructing the opposite endpoint directly is
         * algebraically equivalent, but changes results after quantization. */
        const int anchor_x = decoded->position.flip ? x_begin : x_end;
        const int32_t anchor = interpolate_attribute(decoded,
                                                     decoded->depth.z,
                                                     decoded->depth.dzdx,
                                                     decoded->depth.dzde,
                                                     decoded->depth.dzdy,
                                                     anchor_x,
                                                     y,
                                                     decoded->position.flip,
                                                     false);
        work->depth_fixed = decoded->position.flip ? anchor :
            (int32_t)((uint32_t)anchor -
                (uint32_t)sr_scaled_steps(decoded->depth.dzdx,
                                          (int64_t)(x_end - x_begin)));
    }

    if (decoded->has_texture &&
        (primitive->color.needs_texel0 || primitive->color.needs_texel1 ||
         primitive->color.needs_lod_fraction ||
         primitive->color.cycle_type == RDP_CYCLE_COPY)) {
        const bool copy = primitive->color.cycle_type == RDP_CYCLE_COPY;
        work->s_fixed = interpolate_attribute(decoded, decoded->texture.s,
                                              decoded->texture.dsdx,
                                              decoded->texture.dsde,
                                              decoded->texture.dsdy,
                                              x_begin, y, true, copy);
        work->t_fixed = interpolate_attribute(decoded, decoded->texture.t,
                                              decoded->texture.dtdx,
                                              decoded->texture.dtde,
                                              decoded->texture.dtdy,
                                              x_begin, y, true, copy);
        if (primitive->texture.perspective) {
            work->w_fixed = interpolate_attribute(decoded, decoded->texture.w,
                                                  decoded->texture.dwdx,
                                                  decoded->texture.dwde,
                                                  decoded->texture.dwdy,
                                                  x_begin, y, true, copy);
        }
    }

    if (decoded->has_shade) {
        work->shade = decoded->shade;
        work->shade.r = interpolate_attribute(decoded, work->shade.r, work->shade.drdx, work->shade.drde, work->shade.drdy, x_begin, y, true, false);
        work->shade.g = interpolate_attribute(decoded, work->shade.g, work->shade.dgdx, work->shade.dgde, work->shade.dgdy, x_begin, y, true, false);
        work->shade.b = interpolate_attribute(decoded, work->shade.b, work->shade.dbdx, work->shade.dbde, work->shade.dbdy, x_begin, y, true, false);
        work->shade.a = interpolate_attribute(decoded, work->shade.a, work->shade.dadx, work->shade.dade, work->shade.dady, x_begin, y, true, false);
    }
}

void raster_setup_rectangle_span(int x_begin,
                                   int x_end,
                                   int y,
                                   int32_t s_fixed,
                                   int32_t t_fixed,
                                   int32_t dsdx_fixed,
                                   int32_t dtdx_fixed,
                                   rdp_span *work)
{
    if (!work) {
        return;
    }

    memset(work, 0, sizeof(*work));
    work->x_begin = x_begin;
    work->x_end = x_end;
    work->y = y;
    work->s_fixed = s_fixed;
    work->t_fixed = t_fixed;
    work->dsdx_fixed = dsdx_fixed;
    work->dtdx_fixed = dtdx_fixed;
}
