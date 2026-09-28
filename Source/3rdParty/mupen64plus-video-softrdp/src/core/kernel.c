/*
 * Copyright (c) 2026 PixelMechanic0
 * Licensed under the MIT License.
 *
 * Project: https://github.com/PixelMechanic0/softrdp
 */

#include "raster.h"
#include "framebuffer.h"
#include "stage_back.h"
#include "stage_front.h"
#include "finish.h"
#include "tmem.h"

#include <limits.h>
#include <string.h>

#ifndef SOFTRDP_CENSUS
#define SOFTRDP_CENSUS 0
#endif
#if SOFTRDP_CENSUS
#include "kernel_census.h"
#define CENSUS_COUNT(counts, field) ((counts).field++)
#else
#define CENSUS_COUNT(counts, field) ((void)0)
typedef struct census_counts { char unused; } census_counts;
#endif

/* Everything a kernel holds constant for one span. */
typedef struct kernel_span {
    const rdp_primitive_state *primitive;
    const rdp_span *work;
    sr_attr step;
    /* Offset from a pixel to its LOD y neighbour. One-cycle mode has no second
     * cycle to reach the next scanline: it measures the pipelined pair P+1 and
     * P+2 in walking order. At the second-to-last walked pixel of a span whose
     * four sublines are all valid (lod_end_x) there is no P+2, and it measures
     * the centred pair P-1, P+1 instead (lod_centre is the P-1 offset). */
    sr_attr lod_y;
    sr_attr lod_centre;
    int lod_end_x;
    int32_t shade_dy[4];
    int32_t dzdy;
    stage_combiner_regs regs;
    stage_combiner_ops ops[2];
    /* Rows read by the first evaluated cycle, by cycle 1 after the two-cycle
     * register shift, and by either. */
    uint32_t reads_first;
    uint32_t reads_second;
    uint32_t reads_any;
    stage_alpha_setup alpha;
    stage_write_setup write;
    rdp_texture_unit sampler;
    bool texture;
    bool lod;
    bool perspective;
    bool one_cycle;
    bool next_texel;
    bool texel1_separate;
    bool depth;
    bool depth_late;
    /* The depth compare may rescale coverage (interpenetrating mode) or feed
     * the blender its shifts; such spans take render_span_special. */
    bool special;
    /* The blender reads shade alpha and alpha dither is on, so it reads the
     * dithered value. */
    bool blend_shade_dither;
    /* Chroma keying is on: only render_span_full's instances (full) compile
     * the key, so render_span sends these spans there. */
    bool key;
    uint8_t finish;
} kernel_span;

typedef struct texture_sample {
    sr_rgba texel0;
    sr_rgba texel1;
    uint16_t lod_fraction;
} texture_sample;

/* sample_texels for the full texture unit: the complete fetch, palettes read
 * per TMEM bank included, with texels kept in nine bits from the fetch to the
 * combiner. Only render_span_full's instances contain it. */
static SR_ALWAYS_INLINE texture_sample sample_texels_full(
    const kernel_span *span, const rdp_texture_sample_state *texture0,
    const rdp_texture_sample_state *texture1, int32_t s, int32_t t,
    uint16_t lod_fraction)
{
    const rdp_primitive_state *primitive = span->primitive;
    const rdp_combine_state *color = &primitive->color;
    texture_sample out = { { 0u, 0u, 0u, 0u }, { 0u, 0u, 0u, 0u }, lod_fraction };
    rdp_color fetched = { 0u, 0u, 0u, 0u };
    uint16_t texel1[4];
    const bool ok0 = !color->needs_texel0 ||
        tmem_sample_color_full_fixed5(primitive->tmem, texture0, s, t, &fetched);
    uint16_t texel0[4] = { fetched.r, fetched.g, fetched.b, fetched.a };
    if (texture0->texel_output != RDP_TEXEL_PLAIN)
        tmem_texel_output(texture0, fetched, texel0);
    const bool ok1 = !span->texel1_separate ||
        tmem_sample_color_cycle1_wide(primitive->tmem, texture1, s, t, texel0, texel1);
    if (!ok0 || !ok1) return out;
    if (color->needs_texel0) out.texel0 = sr_rgba_from_wide(texel0);
    if (span->texel1_separate) out.texel1 = sr_rgba_from_wide(texel1);
    else if (color->needs_texel0) out.texel1 = out.texel0;
    if (!color->needs_texel0 && span->texel1_separate) out.texel0 = out.texel1;
    return out;
}

/*
 * A texture fetch cannot fail on hardware: TMEM is 4 KiB and every address
 * wraps inside it, so a degenerate tile descriptor still yields some texel.
 * Our samplers report failure when the tile layout has no extent at all, which
 * is a property of this implementation, not of the RDP. Such a pixel gets
 * defined zero texels and the combiner still runs.
 */
static SR_ALWAYS_INLINE texture_sample sample_texels(const kernel_span *span,
                                                     const rdp_texture_sample_state *texture0,
                                                     const rdp_texture_sample_state *texture1,
                                                     int32_t s, int32_t t,
                                                     uint16_t lod_fraction,
                                                     const bool full)
{
    if (full)
        return sample_texels_full(span, texture0, texture1, s, t, lod_fraction);
    const rdp_primitive_state *primitive = span->primitive;
    const rdp_combine_state *color = &primitive->color;
    texture_sample out = { { 0u, 0u, 0u, 0u }, { 0u, 0u, 0u, 0u }, lod_fraction };
    rdp_color texel0 = { 0u, 0u, 0u, 0u };
    uint16_t texel1[4];
    const bool ok0 = !color->needs_texel0 ||
        stage_texel_sample(primitive->tmem, texture0, s, t, &texel0, span->sampler);
    const bool ok1 = !span->texel1_separate ||
        tmem_sample_color_cycle1_fixed5(primitive->tmem, texture1, s, t, &texel0, texel1);
    if (!ok0 || !ok1) return out;
    if (color->needs_texel0) out.texel0 = sr_rgba_from_color(texel0);
    if (span->texel1_separate) out.texel1 = sr_rgba_from_wide(texel1);
    else if (color->needs_texel0) out.texel1 = out.texel0;
    if (!color->needs_texel0 && span->texel1_separate) out.texel0 = out.texel1;
    return out;
}

/* The LOD path divides the x neighbour, whose inputs are exactly the next
 * pixel's own; this keeps that result for when pixel x is reached. */
typedef struct divide_cache {
    int x;
    int32_t s, t;
    bool clamped;
} divide_cache;

static SR_ALWAYS_INLINE texture_sample triangle_texture(const kernel_span *span,
                                                        sr_attr a, int x,
                                                        divide_cache *cache,
                                                        const bool full)
{
    const rdp_primitive_state *primitive = span->primitive;
    int32_t s, t;
    bool lod_clamp = false;
    bool own_clamped = false;
    if (span->perspective) {
        if (span->lod && cache->x == x) {
            s = cache->s;
            t = cache->t;
            own_clamped = cache->clamped;
        } else {
            own_clamped = stage_perspective_divide(a.s, a.t, a.w, &s, &t);
        }
        lod_clamp = span->lod && !span->one_cycle && own_clamped;
    } else {
        s = stage_texcoord_direct(a.s);
        t = stage_texcoord_direct(a.t);
    }
    if (!span->lod)
        return sample_texels(span, &primitive->texture, &primitive->texture_cycle1,
                             s, t, 0u, full);

    /* The x neighbour is the next pixel in walking order, which is the one to
     * the left in a non-flipped triangle. Its divide is then the previous
     * pixel's own, so that is what the cache keeps. */
    const bool back = !primitive->triangle.position.flip;
    const int32_t xs = back ? (int32_t)((uint32_t)a.s - (uint32_t)span->step.s)
                            : sr_wrap_add(a.s, span->step.s);
    const int32_t xt = back ? (int32_t)((uint32_t)a.t - (uint32_t)span->step.t)
                            : sr_wrap_add(a.t, span->step.t);
    const int32_t xw = back ? (int32_t)((uint32_t)a.w - (uint32_t)span->step.w)
                            : sr_wrap_add(a.w, span->step.w);
    const sr_attr lod_y = x == span->lod_end_x ? span->lod_centre : span->lod_y;
    const int32_t ys = sr_wrap_add(a.s, lod_y.s);
    const int32_t yt = sr_wrap_add(a.t, lod_y.t);
    const int32_t yw = sr_wrap_add(a.w, lod_y.w);
    int32_t nx_s, nx_t, ny_s, ny_t;
    if (span->perspective) {
        bool clamped_x;
        if (back && cache->x == x - 1) {
            nx_s = cache->s;
            nx_t = cache->t;
            clamped_x = cache->clamped;
        } else {
            clamped_x = stage_perspective_divide(xs, xt, xw, &nx_s, &nx_t);
        }
        *cache = back ? (divide_cache){ x, s, t, own_clamped }
                      : (divide_cache){ x + 1, nx_s, nx_t, clamped_x };
        const bool clamped_y = stage_perspective_divide(ys, yt, yw, &ny_s, &ny_t);
        lod_clamp = lod_clamp || clamped_x || clamped_y;
    } else {
        nx_s = stage_texcoord_direct(xs);
        nx_t = stage_texcoord_direct(xt);
        ny_s = stage_texcoord_direct(ys);
        ny_t = stage_texcoord_direct(yt);
    }
    const stage_lod_result lod = span->one_cycle
        ? stage_lod_resolve(primitive, nx_s, nx_t, ny_s, ny_t, nx_s, nx_t, lod_clamp)
        : stage_lod_resolve(primitive, s, t, nx_s, nx_t, ny_s, ny_t, lod_clamp);
    /* LOD_FRACTION can be consumed by the combiner while texture LOD selection
     * itself is disabled. In that mode the two-cycle texture pipeline still
     * samples the primitive's base tile pair; only the fraction is computed. */
    if (!primitive->texture_lod)
        return sample_texels(span, &primitive->texture, &primitive->texture_cycle1,
                             s, t, lod.fraction, full);
    return sample_texels(span, &primitive->lod_textures[lod.tile0],
                         &primitive->lod_textures_cycle1[lod.tile1], s, t,
                         lod.fraction, full);
}

static SR_ALWAYS_INLINE texture_sample rectangle_texture(const kernel_span *span,
                                                         uint32_t index,
                                                         const bool full)
{
    const rdp_span *work = span->work;
    const int32_t s = stage_rect_texcoord(work->s_fixed, work->dsdx_fixed, index,
                                          work->texture_coord_shift,
                                          work->texture_step_shift);
    const int32_t t = stage_rect_texcoord(work->t_fixed, work->dtdx_fixed, index,
                                          work->texture_coord_shift,
                                          work->texture_step_shift);
    return sample_texels(span, &span->primitive->texture,
                         &span->primitive->texture_cycle1, s, t, 0u, full);
}

static SR_ALWAYS_INLINE texture_sample fetch_texture(const kernel_span *span,
                                                     bool rectangle,
                                                     uint32_t index, sr_attr a,
                                                     int x, divide_cache *cache,
                                                     const bool full)
{
    return rectangle ? rectangle_texture(span, index, full)
                     : triangle_texture(span, a, x, cache, full);
}

static sr_result fill_span(sr_memory *memory,
                           const rdp_primitive_state *primitive,
                           const rdp_span *work)
{
    enum { MAX_SPAN = 4096 * SOFTRDP_SCALE };
    uint16_t written[MAX_SPAN / 16];
    const int total = work->x_end - work->x_begin + 1;
    if (total > MAX_SPAN) return SR_ERROR_INVALID_ARGUMENT;
    memset(written, 0, sizeof(uint16_t) * (size_t)((total + 15) / 16));

    for (int index = 0; index < total; index++) {
        const raster_coverage coverage =
            raster_coverage_evaluate(&work->coverage, work->x_begin + index);
        if (coverage.count == 0u ||
            (!primitive->fragment.antialias && !(coverage.mask & 1u)))
            continue;
        written[index >> 4] |= (uint16_t)(1u << (index & 15));
    }

    const rdp_framebuffer_state *framebuffer = &primitive->framebuffer;
    if (framebuffer->color_image.size == RDP_SIZE_8BPP)
        framebuffer_fill8_hidden_span(memory, framebuffer, (uint32_t)work->y,
                                      work->x_begin, work->x_end,
                                      primitive->triangle.position.flip, written);
    for (int index = 0; index < total; index++) {
        if (!(written[index >> 4] & (1u << (index & 15)))) continue;
        const sr_result result = framebuffer_write_fill_pixel(memory, framebuffer,
            (uint32_t)(work->x_begin + index), (uint32_t)work->y);
        if (result != SR_OK) return result;
    }
    return SR_OK;
}

/* A rectangle has no shade or depth interpolants: its attributes stay zero and
 * only its texture coordinates are generated differently. */
static void kernel_span_prepare(kernel_span *span,
                                const rdp_primitive_state *primitive,
                                const rdp_span *work,
                                bool rectangle)
{
    const raster_decoded_triangle *decoded = &primitive->triangle;
    const rdp_combine_state *color = &primitive->color;
    const uint32_t stages = primitive->plan.stages;
    memset(span, 0, sizeof(*span));
    span->primitive = primitive;
    span->work = work;
    span->one_cycle = color->cycle_type == RDP_CYCLE_1;
    if (!rectangle) {
        /* One pixel is one raster step. The span start arrives exactly snapped
         * from span setup; within the span the per-step derivative is used
         * directly, which costs the low bit per step at scale 2 - the same
         * order as the hardware's own per-step truncation. */
        span->step = (sr_attr){
            sr_scaled_derivative(decoded->shade.drdx & ~0x1f),
            sr_scaled_derivative(decoded->shade.dgdx & ~0x1f),
            sr_scaled_derivative(decoded->shade.dbdx & ~0x1f),
            sr_scaled_derivative(decoded->shade.dadx & ~0x1f),
            sr_scaled_derivative(decoded->depth.dzdx),
            sr_scaled_derivative(decoded->texture.dsdx & ~0x1f),
            sr_scaled_derivative(decoded->texture.dtdx & ~0x1f),
            sr_scaled_derivative(decoded->texture.dwdx & ~0x1f)
        };
        /* The walk runs right to left in a non-flipped triangle. */
        const bool back = !decoded->position.flip;
        const sr_attr walk = {
            0, 0, 0, 0, 0,
            back ? (int32_t)(0u - (uint32_t)span->step.s) : span->step.s,
            back ? (int32_t)(0u - (uint32_t)span->step.t) : span->step.t,
            back ? (int32_t)(0u - (uint32_t)span->step.w) : span->step.w
        };
        span->lod_y = span->one_cycle
            ? (sr_attr){ 0, 0, 0, 0, 0,
                         sr_wrap_add(walk.s, walk.s),
                         sr_wrap_add(walk.t, walk.t),
                         sr_wrap_add(walk.w, walk.w) }
            /* The LOD unit steps to the next scanline with the low 15 bits of
             * the y derivatives cleared, as the x steps lose their low 5. */
            : (sr_attr){ 0, 0, 0, 0, 0,
                         sr_scaled_derivative(decoded->texture.dsdy & ~0x7fff),
                         sr_scaled_derivative(decoded->texture.dtdy & ~0x7fff),
                         sr_scaled_derivative(decoded->texture.dwdy & ~0x7fff) };
        span->lod_centre = (sr_attr){ 0, 0, 0, 0, 0,
            (int32_t)(0u - (uint32_t)walk.s),
            (int32_t)(0u - (uint32_t)walk.t),
            (int32_t)(0u - (uint32_t)walk.w) };
        /* Partial spans (any invalid subline) keep the pipelined pair. */
        bool all_sublines = span->one_cycle;
        for (uint32_t row = 0; row < 4u; row++)
            all_sublines = all_sublines &&
                work->coverage.left[row] != RASTER_COVERAGE_NEVER_LEFT;
        span->lod_end_x = !all_sublines ? INT32_MIN
                        : back ? work->x_begin + 1 : work->x_end - 1;
        span->shade_dy[0] = sr_scaled_derivative(decoded->shade.drdy);
        span->shade_dy[1] = sr_scaled_derivative(decoded->shade.dgdy);
        span->shade_dy[2] = sr_scaled_derivative(decoded->shade.dbdy);
        span->shade_dy[3] = sr_scaled_derivative(decoded->shade.dady);
        span->dzdy = sr_scaled_derivative(decoded->depth.dzdy);
        span->lod = (stages & RDP_DRAW_LOD) != 0u;
        span->perspective =
            primitive->texture.perspective;
    }
    const stage_combiner_inputs constants = {
        .primitive = color->primitive_color,
        .environment = color->environment_color,
        .key_center = color->key_center,
        .key_scale = color->key_scale,
        .primitive_lod_fraction = color->primitive_lod_fraction,
        .k4 = (uint16_t)color->convert_k4,
        .k5 = (uint16_t)color->convert_k5
    };
    stage_combiner_regs_prepare(&span->regs, &constants);
    span->ops[0] = stage_combiner_ops_prepare(&color->program.cycle[0]);
    span->ops[1] = stage_combiner_ops_prepare(&color->program.cycle[1]);
    const uint32_t reads1 = stage_combiner_ops_reads(&span->ops[1]);
    span->reads_first = color->two_cycle ? stage_combiner_ops_reads(&span->ops[0]) : reads1;
    span->reads_second = color->two_cycle ? reads1 : 0u;
    span->reads_any = span->reads_first | span->reads_second;
    span->alpha = stage_alpha_prepare(&primitive->fragment);
    span->write = stage_write_prepare(&primitive->fragment);
    span->texture = (stages & RDP_DRAW_TEXTURE) != 0u;
    /* LOD picks its tiles per pixel, which the specialised units cannot; the
     * convert unit samples generically already. */
    span->sampler = span->lod && primitive->plan.texture_unit != RDP_TEXTURE_UNIT_FULL
        ? RDP_TEXTURE_UNIT_GENERIC : primitive->plan.texture_unit;
    span->next_texel = color->next_texel;
    span->texel1_separate = stage_texel1_separate(primitive, span->lod);
    span->depth = (stages & RDP_DRAW_DEPTH) != 0u;
    span->depth_late = (stages & RDP_DRAW_DEPTH_LATE) != 0u;
    span->special = span->depth && primitive->fragment.depth.compare &&
        ((primitive->fragment.depth.mode & 3u) == 1u ||
         primitive->fragment.depth.blend_shift_live);
    span->blend_shade_dither =
        (primitive->fragment.blend.input_mask & RDP_BLENDER_INPUT_SHADE_ALPHA) != 0u &&
        (primitive->fragment.alpha_dither & 3u) != 3u;
    span->finish = (uint8_t)finish_select(primitive, span->depth_late);
    span->key = color->key_enable;
}

/* One pixel back: the attributes of x - 1 from those of x. */
static SR_ALWAYS_INLINE sr_attr sr_attr_unstep(sr_attr value, sr_attr step)
{
    return (sr_attr){
        (int32_t)((uint32_t)value.r - (uint32_t)step.r),
        (int32_t)((uint32_t)value.g - (uint32_t)step.g),
        (int32_t)((uint32_t)value.b - (uint32_t)step.b),
        (int32_t)((uint32_t)value.a - (uint32_t)step.a),
        (int32_t)((uint32_t)value.z - (uint32_t)step.z),
        (int32_t)((uint32_t)value.s - (uint32_t)step.s),
        (int32_t)((uint32_t)value.t - (uint32_t)step.t),
        (int32_t)((uint32_t)value.w - (uint32_t)step.w)
    };
}

/* The step that walks the other way. */
static SR_ALWAYS_INLINE sr_attr sr_attr_negate(sr_attr step)
{
    return (sr_attr){
        (int32_t)(0u - (uint32_t)step.r), (int32_t)(0u - (uint32_t)step.g),
        (int32_t)(0u - (uint32_t)step.b), (int32_t)(0u - (uint32_t)step.a),
        (int32_t)(0u - (uint32_t)step.z), (int32_t)(0u - (uint32_t)step.s),
        (int32_t)(0u - (uint32_t)step.t), (int32_t)(0u - (uint32_t)step.w)
    };
}

/* n pixels on: the attributes after n steps, as stepping n times yields them. */
static SR_ALWAYS_INLINE sr_attr sr_attr_advance(sr_attr value, sr_attr step, int n)
{
    const uint32_t k = (uint32_t)n;
    return (sr_attr){
        (int32_t)((uint32_t)value.r + (uint32_t)step.r * k),
        (int32_t)((uint32_t)value.g + (uint32_t)step.g * k),
        (int32_t)((uint32_t)value.b + (uint32_t)step.b * k),
        (int32_t)((uint32_t)value.a + (uint32_t)step.a * k),
        (int32_t)((uint32_t)value.z + (uint32_t)step.z * k),
        (int32_t)((uint32_t)value.s + (uint32_t)step.s * k),
        (int32_t)((uint32_t)value.t + (uint32_t)step.t * k),
        (int32_t)((uint32_t)value.w + (uint32_t)step.w * k)
    };
}

/*
 * Cycle 0 of pixel nx, the next pixel in walking order, for the two-cycle
 * alpha compare when the span loop has not just run it for that pixel itself:
 * its centroid shade, its texels and its noise. The loop needs this only after
 * a pixel it skipped, so it stays out of line.
 */
static __attribute__((noinline)) stage_combined ac2_neighbour_cycle0(
    kernel_span *span, sr_attr nattr, int nx, uint32_t nindex, uint32_t pixel_y,
    bool rectangle, bool full, raster_coverage ncov, divide_cache *divides)
{
    const rdp_primitive_state *primitive = span->primitive;
    const rdp_combine_state *color = &primitive->color;
    stage_combiner_regs *regs = &span->regs;
    const uint32_t reads_first = span->reads_first;
    if (span->reads_any & STAGE_COMBINER_PAIR(RDP_COMBINER_SHADE_RGB)) {
        const sr_rgba nshade = ncov.count == 8u
            ? (sr_rgba){ stage_shade_u8(nattr.r), stage_shade_u8(nattr.g),
                         stage_shade_u8(nattr.b), stage_shade_u8(nattr.a) }
            : (sr_rgba){
                stage_shade_centroid_u8(nattr.r, span->step.r, span->shade_dy[0],
                                        ncov.first_x, ncov.first_y),
                stage_shade_centroid_u8(nattr.g, span->step.g, span->shade_dy[1],
                                        ncov.first_x, ncov.first_y),
                stage_shade_centroid_u8(nattr.b, span->step.b, span->shade_dy[2],
                                        ncov.first_x, ncov.first_y),
                stage_shade_centroid_u8(nattr.a, span->step.a, span->shade_dy[3],
                                        ncov.first_x, ncov.first_y) };
        stage_combiner_set_rgba(regs, RDP_COMBINER_SHADE_RGB, nshade);
    }
    if (span->texture) {
        const texture_sample ntex = full
            ? fetch_texture(span, rectangle, nindex, nattr, nx, divides, true)
            : fetch_texture(span, rectangle, nindex, nattr, nx, divides, false);
        if (reads_first & STAGE_COMBINER_PAIR(RDP_COMBINER_TEXEL0_RGB))
            stage_combiner_set_rgba(regs, RDP_COMBINER_TEXEL0_RGB, ntex.texel0);
        if (reads_first & STAGE_COMBINER_PAIR(RDP_COMBINER_TEXEL1_RGB))
            stage_combiner_set_rgba(regs, RDP_COMBINER_TEXEL1_RGB, ntex.texel1);
        if (span->reads_any & (1u << RDP_COMBINER_LOD_FRACTION))
            stage_combiner_set_scalar(regs, RDP_COMBINER_LOD_FRACTION, ntex.lod_fraction);
    }
    if (span->reads_any & (1u << RDP_COMBINER_NOISE))
        stage_combiner_set_scalar(regs, RDP_COMBINER_NOISE, stage_combiner_noise(
            color->needs_noise
                ? rdp_pixel_noise(sr_raster_to_pixel((uint32_t)nx), pixel_y,
                                  color->primitive_counter)
                : 0u));
    if (reads_first & STAGE_COMBINER_PAIR(RDP_COMBINER_COMBINED_RGB))
        stage_combiner_set_combined(regs, (stage_combined){ 0, 0, 0, 0 });
    return stage_combine_cycle_regs(&span->ops[0], regs);
}

/* Loop state that crosses from one segment of a span into the next. */
typedef struct span_carry {
    sr_attr attr;
    texture_sample texel;
    int texel_x;
    divide_cache divides;
    /* The last pixel's own cycle 0 for the two-cycle alpha compare (see
     * render_segment), kept across segments so their boundaries do not
     * recompute it. */
    int c0_x;
    uint16_t c0_alpha;
    uint8_t c0_cov;
} span_carry;

/*
 * Pixels x_first..x_last of a span. The interior of a span is fully covered by
 * construction, so its instance takes the coverage as a constant and folds
 * away everything that only partial coverage needs.
 */
static SR_ALWAYS_INLINE sr_result render_segment(sr_memory *memory,
                                                 const rdp_primitive_state *primitive,
                                                 const rdp_span *work,
                                                 kernel_span *span,
                                                 rdp_hidden8_writer *writer,
                                                 int x_first, int x_last,
                                                 span_carry *state,
                                                 census_counts *census,
                                                 const bool rectangle,
                                                 const bool interior,
                                                 const int finish,
                                                 const bool full,
                                                 const bool special,
                                                 const bool hidden8,
                                                 const bool next,
                                                 const bool descending,
                                                 uint16_t *records8)
{
    static const raster_coverage full_coverage = { 0xffu, 8u, 0u, 0u };
    const rdp_combine_state *color = &primitive->color;
    const rdp_fragment_state *fragment = &primitive->fragment;
    const rdp_texture_size size = primitive->framebuffer.store_size;
    const uint32_t y = (uint32_t)work->y;
    const uint32_t pixel_y = sr_raster_to_pixel(y);
    sr_attr attr = state->attr;
    texture_sample carry = state->texel;
    int carry_x = state->texel_x;
    divide_cache divides = state->divides;
    /* The last pixel's own clamped cycle-0 alpha and its coverage, which the
     * two-cycle alpha compare reads as its next pixel's (see below). */
    int c0_x = state->c0_x;
    uint16_t c0_alpha = state->c0_alpha;
    uint8_t c0_cov = state->c0_cov;
    /* Only the instances built with next read the next pixel's texel; the
     * common span loop folds all of it away. */
    const bool next_texel = next && span->next_texel;
    (void)census;

    /* A descending segment runs x_last down to x_first; state->attr then holds
     * the attributes of x_last. Only the instances that read the next pixel
     * pass a runtime direction; everywhere else it is a constant false and
     * the loop is the plain ascending one. */
    const int dx = descending ? -1 : 1;
    const sr_attr dstep = descending ? sr_attr_negate(span->step) : span->step;
    for (int x = descending ? x_last : x_first;
         descending ? x >= x_first : x <= x_last;
         x += dx, attr = sr_attr_step(attr, dstep)) {
        const uint32_t index = (uint32_t)(x - work->x_begin);
        const raster_coverage coverage = interior ? full_coverage
            : x >= work->coverage.full_x0 && x <= work->coverage.full_x1
                ? full_coverage : raster_coverage_evaluate(&work->coverage, x);
        /* The 8bpp instance carries a rejected pixel through the pipeline: its
         * blender output still reaches the ninth bits of an odd byte. */
        bool rejected = false;
        if (coverage.count == 0u ||
            (!fragment->antialias && !(coverage.mask & 1u))) {
            if (!hidden8) continue;
            rejected = true;
        }
        CENSUS_COUNT(*census, covered);
        if (coverage.count == 8u) CENSUS_COUNT(*census, full);

        /* Texture coordinates remain at the pixel position. The RDP applies
         * coverage-centroid correction to shade and Z only. */
        sr_rgba shade;
        if (coverage.count == 8u) {
            shade = (sr_rgba){ stage_shade_u8(attr.r), stage_shade_u8(attr.g),
                               stage_shade_u8(attr.b), stage_shade_u8(attr.a) };
        } else {
            shade = (sr_rgba){
                stage_shade_centroid_u8(attr.r, span->step.r, span->shade_dy[0],
                                        coverage.first_x, coverage.first_y),
                stage_shade_centroid_u8(attr.g, span->step.g, span->shade_dy[1],
                                        coverage.first_x, coverage.first_y),
                stage_shade_centroid_u8(attr.b, span->step.b, span->shade_dy[2],
                                        coverage.first_x, coverage.first_y),
                stage_shade_centroid_u8(attr.a, span->step.a, span->shade_dy[3],
                                        coverage.first_x, coverage.first_y)
            };
        }
        const int32_t depth_snapped = stage_depth_centroid(attr.z, span->step.z,
            span->dzdy, coverage.first_x, coverage.first_y);
        const uint32_t color_address = stage_color_slot(memory, primitive,
                                                        (uint32_t)x, y);
        const uint32_t pixel_x = sr_raster_to_pixel((uint32_t)x);
        const uint16_t noise = color->needs_noise
            ? rdp_pixel_noise(pixel_x, pixel_y, color->primitive_counter) : 0u;

        stage_depth_result depth = { .pass = true, .farther = true, .coverage = 0xffu,
                                     .blend_shift = fragment->depth.blend_shift };
        if (span->depth && !span->depth_late) {
            const sr_result result = stage_depth_test(memory, primitive,
                depth_snapped, stage_depth_slot(memory, primitive, (uint32_t)x, y),
                color_address, sr_sample_of((uint32_t)x, y), coverage.count, &depth,
                special);
            if (result != SR_OK) return result;
            if (!depth.pass) {
                CENSUS_COUNT(*census, early_reject);
                if (!hidden8) continue;
                rejected = true;
            }
        }

        texture_sample texture = { { 0u, 0u, 0u, 0u }, { 0u, 0u, 0u, 0u }, 0u };
        sr_rgba next_texel0 = { 0u, 0u, 0u, 0u };
        if (span->texture) {
            texture = carry_x == x ? carry
                                   : fetch_texture(span, rectangle, index, attr,
                                                   x, &divides, full);
            /* The next pixel in walking order: x + 1 for rectangles and
             * flipped triangles, x - 1 otherwise. A walk that runs past the
             * span end steps on beyond it, unless the end-of-span peek hands
             * over to the next scanline. */
            if (next_texel) {
                if (rectangle) {
                    if (x < work->x_end) {
                        carry = fetch_texture(span, true, index + 1u, attr,
                                              x + 1, &divides, full);
                        carry_x = x + 1;
                        next_texel0 = carry.texel0;
                    } else {
                        next_texel0 = texture.texel0;
                    }
                } else if (work->texel1_peek &&
                           x == (primitive->triangle.position.flip ? work->x_end
                                                                   : work->x_begin)) {
                    const sr_attr line = { 0, 0, 0, 0, 0,
                                           work->peek_s, work->peek_t, work->peek_w };
                    next_texel0 = fetch_texture(span, false, index, line,
                                                work->x_begin - 3, &divides,
                                                full).texel0;
                    if (!primitive->triangle.position.flip) {
                        carry = texture;
                        carry_x = x;
                    }
                } else if (primitive->triangle.position.flip) {
                    carry = fetch_texture(span, false, index + 1u,
                                          sr_attr_step(attr, span->step),
                                          x + 1, &divides, full);
                    carry_x = x + 1;
                    next_texel0 = carry.texel0;
                } else {
                    const sr_attr back = { 0, 0, 0, 0, 0,
                        (int32_t)((uint32_t)attr.s - (uint32_t)span->step.s),
                        (int32_t)((uint32_t)attr.t - (uint32_t)span->step.t),
                        (int32_t)((uint32_t)attr.w - (uint32_t)span->step.w) };
                    next_texel0 = carry_x == x - 1 ? carry.texel0
                        : fetch_texture(span, false, index, back, x - 1,
                                        &divides, full).texel0;
                    carry = texture;
                    carry_x = x;
                }
            }
        }

        stage_combiner_regs *regs = &span->regs;
        const uint32_t reads_first = span->reads_first;
        const uint32_t reads_second = span->reads_second;
        if (span->reads_any & STAGE_COMBINER_PAIR(RDP_COMBINER_SHADE_RGB))
            stage_combiner_set_rgba(regs, RDP_COMBINER_SHADE_RGB, shade);
        if (reads_first & STAGE_COMBINER_PAIR(RDP_COMBINER_TEXEL0_RGB))
            stage_combiner_set_rgba(regs, RDP_COMBINER_TEXEL0_RGB, texture.texel0);
        if (reads_first & STAGE_COMBINER_PAIR(RDP_COMBINER_TEXEL1_RGB))
            stage_combiner_set_rgba(regs, RDP_COMBINER_TEXEL1_RGB,
                                    next_texel && !color->two_cycle
                                        ? next_texel0 : texture.texel1);
        if (span->reads_any & (1u << RDP_COMBINER_LOD_FRACTION))
            stage_combiner_set_scalar(regs, RDP_COMBINER_LOD_FRACTION, texture.lod_fraction);
        if (span->reads_any & (1u << RDP_COMBINER_NOISE))
            stage_combiner_set_scalar(regs, RDP_COMBINER_NOISE, stage_combiner_noise(noise));
        stage_combined combined = { 0, 0, 0, 0 };
        uint16_t cycle0_alpha = 0u;
        if (color->two_cycle) {
            /* The combined rows stay zero in one-cycle mode, where nothing
             * writes them; here they carry the previous pixel's result. */
            if (reads_first & STAGE_COMBINER_PAIR(RDP_COMBINER_COMBINED_RGB))
                stage_combiner_set_combined(regs, combined);
            combined = stage_combine_cycle_regs(&span->ops[0], regs);
            cycle0_alpha = stage_combiner_clamp9(combined.a);
            /* The texel registers shift between the cycles: TEXEL0 takes this
             * pixel's tile-1 sample and TEXEL1 the tile-0 sample, of the next
             * pixel when the pipeline exposes it. */
            if (reads_second & STAGE_COMBINER_PAIR(RDP_COMBINER_TEXEL0_RGB))
                stage_combiner_set_rgba(regs, RDP_COMBINER_TEXEL0_RGB, texture.texel1);
            if (reads_second & STAGE_COMBINER_PAIR(RDP_COMBINER_TEXEL1_RGB))
                stage_combiner_set_rgba(regs, RDP_COMBINER_TEXEL1_RGB,
                                        next_texel ? next_texel0 : texture.texel0);
            if (reads_second & STAGE_COMBINER_PAIR(RDP_COMBINER_COMBINED_RGB))
                stage_combiner_set_combined(regs, combined);
        }
        const stage_v4 final = stage_combiner_clamp9_v4(
            stage_combine_cycle_regs_v4(&span->ops[1], regs));
        rdp_color pixel = {
            (uint8_t)final[0], (uint8_t)final[1], (uint8_t)final[2], (uint8_t)final[3]
        };
        /* Chroma keying rewrites the colour and the alpha the alpha stage uses;
         * only the full instances contain it (kernel_span.key). */
        int32_t key_alpha = -1;
        if (full && span->key)
            key_alpha = stage_chroma_key(&span->ops[1], regs, color->key_width, &pixel);

        const uint8_t blend_shade = span->blend_shade_dither
            ? stage_blend_shade_alpha((uint8_t)shade.a,
                  stage_alpha_dither(&span->alpha, noise, pixel_x, pixel_y))
            : (uint8_t)shade.a;
        if (finish != FINISH_GENERIC) {
            sr_result result = finish_write(memory, primitive, (finish_kind)finish,
                color_address, (uint32_t)x, y, pixel, blend_shade, noise,
                special ? depth.coverage : 0xffu);
            if (result != SR_OK) return result;
            result = stage_depth_commit(memory, &depth, true);
            if (result != SR_OK) return result;
            CENSUS_COUNT(*census, written);
            continue;
        }

        /* The two-cycle alpha compare reads cycle 0 of the NEXT pixel in
         * walking order: the pipeline has already taken that pixel through
         * cycle 0 when this one reaches the comparator. Its coverage comes with
         * it (none past the span end); the alpha dither stays this pixel's.
         * Such spans run against their walking order (render_span_next), so
         * that cycle 0 is normally the one the previous pixel ran for itself. */
        uint8_t compare_coverage = coverage.count;
        if (next && span->alpha.two_cycle_compare) {
            const bool forward = rectangle || primitive->triangle.position.flip;
            const int nx = forward ? x + 1 : x - 1;
            const uint16_t own_cycle0 = cycle0_alpha;
            if (c0_x == nx) {
                cycle0_alpha = c0_alpha;
                compare_coverage = c0_cov;
            } else if (nx < work->x_begin || nx > work->x_end) {
                /* The last pixel in walking order has no next pixel: the
                 * hardware compares an undefined value there (cen64 notes
                 * "garbage"). The pixel's own cycle 0 keeps it a plain alpha
                 * test and costs nothing; the conformance reference is patched
                 * to match (tests/angrylion_wrapper.c). */
            } else {
                const raster_coverage ncov = raster_coverage_evaluate(&work->coverage, nx);
                cycle0_alpha = stage_combiner_clamp9(ac2_neighbour_cycle0(span,
                    forward ? sr_attr_step(attr, span->step)
                            : sr_attr_unstep(attr, span->step),
                    nx, forward ? index + 1u : index - 1u, pixel_y, rectangle, full,
                    ncov, &divides).a);
                compare_coverage = ncov.count;
            }
            c0_x = x;
            c0_alpha = own_cycle0;
            c0_cov = coverage.count;
        }
        const stage_alpha_result alpha = stage_alpha_split(fragment, &span->alpha,
            pixel.a, cycle0_alpha, coverage.count, compare_coverage, noise,
            pixel_x, pixel_y, key_alpha);
        if (!alpha.accept) {
            CENSUS_COUNT(*census, alpha_reject);
            if (!hidden8) continue;
            rejected = true;
        }

        if (span->depth && span->depth_late) {
            const sr_result result = stage_depth_test(memory, primitive,
                depth_snapped, stage_depth_slot(memory, primitive, (uint32_t)x, y),
                color_address, sr_sample_of((uint32_t)x, y), alpha.coverage, &depth,
                special);
            if (result != SR_OK) return result;
            if (!depth.pass) {
                CENSUS_COUNT(*census, late_reject);
                if (!hidden8) continue;
                rejected = true;
            }
        }

        if (hidden8) {
            rdp_color out;
            const sr_result result = stage_color8(memory, primitive, &span->write,
                color_address, (uint32_t)x, y, pixel, alpha.alpha, blend_shade,
                alpha.coverage, &depth, noise, &out);
            if (result != SR_OK) return result;
            if (color_address == FB_SLOT_NONE) continue;
            if (!rejected) {
                const uint32_t pixel_index =
                    pixel_y * primitive->framebuffer.color_image.width + pixel_x;
                /* records8 is render_span_hidden8's per-pixel result, indexed
                 * from x_begin: 0x100 | byte for a written pixel, 0x200 |
                 * green LSB for a rejected odd one. A parameter rather than a
                 * kernel_span field, which every span's stack frame carries. */
                records8[index] =
                    (uint16_t)(0x100u | ((pixel_index & 1u) ? out.g : out.r));
                const sr_result commit = stage_depth_commit(memory, &depth, true);
                if (commit != SR_OK) return commit;
                CENSUS_COUNT(*census, written);
            } else if (color_address & 1u) {
                records8[index] = (uint16_t)(0x200u | (out.g & 1u));
            }
            continue;
        }

        sr_result result = stage_write_color(memory, primitive, &span->write, size,
            color_address, (uint32_t)x, y, pixel, alpha.alpha, blend_shade,
            alpha.coverage, &depth, noise, writer);
        if (result != SR_OK) return result;
        result = stage_depth_commit(memory, &depth, true);
        if (result != SR_OK) return result;
        CENSUS_COUNT(*census, written);
    }

    state->attr = attr;
    state->texel = carry;
    state->texel_x = carry_x;
    state->divides = divides;
    state->c0_x = c0_x;
    state->c0_alpha = c0_alpha;
    state->c0_cov = c0_cov;
    return SR_OK;
}

/* The segments of a span: left edge, fully covered interior with one instance
 * per finish kind, right edge. A descending span runs them right to left. */
static SR_ALWAYS_INLINE sr_result render_segments(sr_memory *memory,
                                                  const rdp_primitive_state *primitive,
                                                  const rdp_span *work,
                                                  kernel_span *span,
                                                  rdp_hidden8_writer *writer,
                                                  span_carry *state,
                                                  census_counts *census,
                                                  const bool rectangle,
                                                  const bool full,
                                                  const bool special,
                                                  const bool hidden8,
                                                  const bool next,
                                                  const bool descending,
                                                  uint16_t *records8)
{
    const int interior_first = work->coverage.full_x0 > work->x_begin
        ? work->coverage.full_x0 : work->x_begin;
    const int interior_last = work->coverage.full_x1 < work->x_end
        ? work->coverage.full_x1 : work->x_end;
    if (interior_first > interior_last)
        return render_segment(memory, primitive, work, span, writer,
                              work->x_begin, work->x_end, state, census,
                              rectangle, false, FINISH_GENERIC, full, special, hidden8, next,
                              descending, records8);

    /* The edge run first and the edge run last, in the span's running order. */
    const int first_edge_x0 = descending ? interior_last + 1 : work->x_begin;
    const int first_edge_x1 = descending ? work->x_end : interior_first - 1;
    const int last_edge_x0 = descending ? work->x_begin : interior_last + 1;
    const int last_edge_x1 = descending ? interior_first - 1 : work->x_end;
    sr_result result = render_segment(memory, primitive, work, span, writer,
                                      first_edge_x0, first_edge_x1, state, census,
                                      rectangle, false, FINISH_GENERIC, full, special, hidden8, next,
                                      descending, records8);
    if (result != SR_OK) return result;
    /* One interior instance per finish kind. Written out rather than
     * generated from FINISH_KINDS, which measured slower; the check below
     * breaks the build when a kind is added without its case. */
    _Static_assert(FINISH_COUNT == 3, "a finish kind has no interior instance");
    /* The instances that read the next pixel serve rare spans (and the
     * two-cycle alpha compare needs the generic finish anyway): they take the
     * generic interior only, which is correct for every finish kind. */
    switch (next ? (int)FINISH_GENERIC : (int)span->finish) {
    case FINISH_STORE:
        result = render_segment(memory, primitive, work, span, writer,
                                interior_first, interior_last, state, census,
                                rectangle, true, FINISH_STORE, full, special, hidden8, next,
                                descending, records8);
        break;
    case FINISH_FOG:
        result = render_segment(memory, primitive, work, span, writer,
                                interior_first, interior_last, state, census,
                                rectangle, true, FINISH_FOG, full, special, hidden8, next,
                                descending, records8);
        break;
    default:
        result = render_segment(memory, primitive, work, span, writer,
                                interior_first, interior_last, state, census,
                                rectangle, true, FINISH_GENERIC, full, special, hidden8, next,
                                descending, records8);
        break;
    }
    if (result != SR_OK) return result;
    return render_segment(memory, primitive, work, span, writer,
                          last_edge_x0, last_edge_x1, state, census,
                          rectangle, false, FINISH_GENERIC, full, special, hidden8, next,
                          descending, records8);
}

/*
 * Whether a span runs against its walking order. A two-cycle alpha compare
 * reads cycle 0 of the next pixel in walking order; running a forward walk
 * backwards makes that the pixel just run, whose own cycle 0 render_segment
 * reuses. A descending span starts from x_end's attributes.
 */
static SR_ALWAYS_INLINE bool span_descending(const kernel_span *span,
                                             const rdp_primitive_state *primitive,
                                             const rdp_span *work,
                                             span_carry *state, bool rectangle)
{
    if (!span->alpha.two_cycle_compare ||
        !(rectangle || primitive->triangle.position.flip))
        return false;
    if (!rectangle)
        state->attr = sr_attr_advance(state->attr, span->step,
                                      work->x_end - work->x_begin);
    return true;
}

/* A span of the full texture unit, through its own instances of the span
 * segments. Out of line, so the other units' span loops are compiled without
 * any of it. */
static __attribute__((noinline, cold)) sr_result render_span_full(
    sr_memory *memory, const rdp_primitive_state *primitive, const rdp_span *work,
    kernel_span *span, rdp_hidden8_writer *writer, span_carry *state,
    census_counts *census, bool rectangle)
{
    /* Spans that read no next pixel keep an instance without any of that
     * code: it costs the loop even where it never runs. */
    if (!span->next_texel && !span->alpha.two_cycle_compare)
        return rectangle
            ? render_segments(memory, primitive, work, span, writer, state, census, true, true, true, false, false, false, NULL)
            : render_segments(memory, primitive, work, span, writer, state, census, false, true, true, false, false, false, NULL);
    const bool descending = span_descending(span, primitive, work, state, rectangle);
    return rectangle
        ? render_segments(memory, primitive, work, span, writer, state, census, true, true, true, false, true, descending, NULL)
        : render_segments(memory, primitive, work, span, writer, state, census, false, true, true, false, true, descending, NULL);
}

/* A span whose depth compare may rescale coverage or feed the blender its
 * shifts. Out of line for the same reason: the common span loops keep none of
 * that code. Not cold: some games draw most of a frame this way. */
static __attribute__((noinline)) sr_result render_span_special(
    sr_memory *memory, const rdp_primitive_state *primitive, const rdp_span *work,
    kernel_span *span, rdp_hidden8_writer *writer, span_carry *state,
    census_counts *census, bool rectangle)
{
    /* Spans that read no next pixel keep an instance without any of that
     * code: it cost bh, oot and xg2m 2-3% in aligned builds although they
     * never ran it. */
    if (!span->next_texel && !span->alpha.two_cycle_compare)
        return rectangle
            ? render_segments(memory, primitive, work, span, writer, state, census, true, false, true, false, false, false, NULL)
            : render_segments(memory, primitive, work, span, writer, state, census, false, false, true, false, false, false, NULL);
    const bool descending = span_descending(span, primitive, work, state, rectangle);
    return rectangle
        ? render_segments(memory, primitive, work, span, writer, state, census, true, false, true, false, true, descending, NULL)
        : render_segments(memory, primitive, work, span, writer, state, census, false, false, true, false, true, descending, NULL);
}

/* A span that reads the next pixel in walking order: its combiner reads that
 * pixel's texel as TEXEL1, or its two-cycle alpha compare sees that pixel's
 * cycle 0. Out of line so the common span loop carries none of it. Not cold:
 * two-cycle alpha compare is common, and some games (ts2, tsphere2) draw
 * much of a frame here. */
static __attribute__((noinline)) sr_result render_span_next(
    sr_memory *memory, const rdp_primitive_state *primitive, const rdp_span *work,
    kernel_span *span, rdp_hidden8_writer *writer, span_carry *state,
    census_counts *census, bool rectangle)
{
    const bool descending = span_descending(span, primitive, work, state, rectangle);
    return rectangle
        ? render_segments(memory, primitive, work, span, writer, state, census, true, false, false, false, true, descending, NULL)
        : render_segments(memory, primitive, work, span, writer, state, census, false, false, false, false, true, descending, NULL);
}

/* Records for the widest span a scissor allows. */
enum { HIDDEN8_SPAN_CAP = 1024 * SOFTRDP_SCALE + 8 };

/*
 * A span to an 8bpp image. Its byte writes reach the ninth bits of their
 * halfword through the memory interface's latch, so they depend on the order
 * the hardware walks the span and on the pixels it rejects. The segments record
 * each pixel's result; they are then stored in walking order, rejected odd
 * bytes included. Out of line: no other image size runs any of it.
 */
static __attribute__((noinline, cold)) sr_result render_span_hidden8(
    sr_memory *memory, const rdp_primitive_state *primitive, const rdp_span *work,
    kernel_span *span, rdp_hidden8_writer *writer, span_carry *state,
    census_counts *census, bool rectangle)
{
    const int count = work->x_end - work->x_begin + 1;
    if (count > HIDDEN8_SPAN_CAP)
        return render_segments(memory, primitive, work, span, writer, state, census,
                               rectangle, span->sampler == RDP_TEXTURE_UNIT_FULL || span->key,
                               true, false, true, false, NULL);
    uint16_t records[HIDDEN8_SPAN_CAP];
    memset(records, 0, (size_t)count * sizeof(records[0]));
    const bool full = span->sampler == RDP_TEXTURE_UNIT_FULL || span->key;
    const sr_result result = rectangle
        ? (full ? render_segments(memory, primitive, work, span, writer, state, census,
                                  true, true, true, true, true, false, records)
                : render_segments(memory, primitive, work, span, writer, state, census,
                                  true, false, true, true, true, false, records))
        : (full ? render_segments(memory, primitive, work, span, writer, state, census,
                                  false, true, true, true, true, false, records)
                : render_segments(memory, primitive, work, span, writer, state, census,
                                  false, false, true, true, true, false, records));
    if (result != SR_OK) return result;

    const uint32_t y = (uint32_t)work->y;
    for (int i = 0; i < count; i++) {
        const int x = writer->forward ? work->x_begin + i : work->x_end - i;
        const uint16_t record = records[x - work->x_begin];
        if (!record) continue;
        const uint32_t slot = stage_color_slot(memory, primitive, (uint32_t)x, y);
        if (slot == FB_SLOT_NONE) continue;
        const uint32_t sample = sr_sample_of((uint32_t)x, y);
        if (record & 0x100u) {
            framebuffer_hidden8_write(memory, slot, (uint8_t)record, writer, sample);
            sr_memory_write_u8_sample(memory, slot, (uint8_t)record, sample);
        } else {
            framebuffer_hidden8_reject(memory, slot, (record & 1u) != 0u, writer, sample);
        }
    }
    return SR_OK;
}

static SR_ALWAYS_INLINE sr_result render_span(sr_memory *memory,
                                              const rdp_primitive_state *primitive,
                                              const rdp_span *work,
                                              const bool rectangle,
                                              const bool image8)
{
    if (!memory || !primitive || !work) return SR_ERROR_INVALID_ARGUMENT;
    if (work->x_end < work->x_begin) return SR_OK;
    if (!rectangle && (primitive->plan.stages & RDP_DRAW_FILL))
        return fill_span(memory, primitive, work);

    kernel_span span;
    kernel_span_prepare(&span, primitive, work, rectangle);
#if SOFTRDP_CENSUS
    census_counts census = { 1u, 0u, 0u, 0u, 0u, 0u, 0u };
#else
    census_counts census = { 0 };
#endif
    const rdp_texture_size size = primitive->framebuffer.color_image.size;
    rdp_hidden8_writer hidden8;
    rdp_hidden8_writer *writer = NULL;
    if (size == RDP_SIZE_8BPP) {
        framebuffer_hidden8_begin(&hidden8,
            rectangle || primitive->triangle.position.flip);
        writer = &hidden8;
    }

    /* The next pixel's texture sample, taken early when the combiner reads it
     * as TEXEL1, is carried with the attributes and reused when that pixel is
     * reached, also across segments. */
    span_carry state = {
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { { 0u, 0u, 0u, 0u }, { 0u, 0u, 0u, 0u }, 0u },
        /* Nothing is carried or cached yet, so neither key may match a lookup:
         * x_begin - 1 is the first pixel's x neighbour in a non-flipped
         * two-cycle span and x_begin - 3 the end-of-span peek. A matching key
         * would hand them a zero texel (black TEXEL1) or s = t = 0 (wrong LOD). */
        INT_MIN,
        { INT_MIN, 0, 0, false },
        /* No pixel's cycle 0 is known yet; x_begin - 3 is nobody's neighbour. */
        work->x_begin - 3, 0u, 0u
    };
    if (!rectangle) {
        state.attr = (sr_attr){
            work->shade.r, work->shade.g, work->shade.b, work->shade.a,
            (int32_t)work->depth_fixed, work->s_fixed, work->t_fixed, work->w_fixed
        };
    }

    const sr_result result = image8
        ? render_span_hidden8(memory, primitive, work, &span, writer, &state, &census,
                              rectangle)
        : span.sampler == RDP_TEXTURE_UNIT_FULL || span.key
        ? render_span_full(memory, primitive, work, &span, writer, &state, &census,
                           rectangle)
        : span.special
        ? render_span_special(memory, primitive, work, &span, writer, &state, &census,
                              rectangle)
        : span.next_texel || span.alpha.two_cycle_compare
        ? render_span_next(memory, primitive, work, &span, writer, &state, &census,
                           rectangle)
        : render_segments(memory, primitive, work, &span, writer, &state, &census,
                          rectangle, false, false, false, false, false, NULL);
    if (result != SR_OK) return result;

#if SOFTRDP_CENSUS
    census_record(primitive, &census);
#endif
    if (writer) framebuffer_hidden8_end(memory, writer);
    return SR_OK;
}

/* The render_span instance for 8bpp images, out of line and chosen in
 * kernel_render_span, so kernel_triangle and kernel_rectangle stay the single
 * instance every other image uses. Inlining it beside that instance moved the
 * instance's code and cost about 2% on dumps that never draw to an 8bpp image. */
static __attribute__((noinline)) sr_result kernel_span_image8(
    sr_memory *memory, const rdp_primitive_state *primitive, const rdp_span *work,
    bool rectangle)
{
    return rectangle ? render_span(memory, primitive, work, true, true)
                     : render_span(memory, primitive, work, false, true);
}

static sr_result kernel_triangle(sr_memory *memory,
                               const rdp_primitive_state *primitive,
                               const rdp_span *work)
{
    return render_span(memory, primitive, work, false, false);
}

static sr_result kernel_rectangle(sr_memory *memory,
                                const rdp_primitive_state *primitive,
                                const rdp_span *work)
{
    return render_span(memory, primitive, work, true, false);
}

static inline uint32_t copy_group_pixels(const rdp_primitive_state *primitive)
{
    /* An 8bpp target takes four pixels a clock, like a 16bpp one. */
    return primitive->framebuffer.color_image.size == RDP_SIZE_4BPP ? 8u
        : primitive->framebuffer.color_image.size <= RDP_SIZE_16BPP ? 4u : 2u;
}

typedef struct rdp_copy_group {
    uint16_t value[8];
    uint8_t valid_mask;
    uint8_t count;
} rdp_copy_group;

typedef struct rdp_copy_hidden_pair {
    uint32_t pair;
    uint32_t x, y;
    uint16_t even_value, odd_value;
    bool valid, have_even, have_odd, primitive_terminal;
} rdp_copy_hidden_pair;

static void copy_flush_hidden_pair(sr_memory *memory,
                                            const rdp_primitive_state *primitive,
                                            rdp_copy_hidden_pair *pending)
{
    if (!pending->valid) return;
    if (primitive->framebuffer.color_image.size == RDP_SIZE_4BPP) {
        const uint32_t slot = framebuffer_slot(memory, &primitive->framebuffer,
                                               pending->x, pending->y);
        if (slot == FB_SLOT_NONE) {
            *pending = (rdp_copy_hidden_pair){0};
            return;
        }
        const uint32_t word_slot = slot & ~1u;
        const uint32_t sample = sr_sample_of(pending->x, pending->y);
        const uint16_t word = sr_memory_read_be16_sample(memory, word_slot, sample);
        uint8_t hidden = sr_memory_read_hidden_sample(memory, word_slot, word,
                                                       sample);
        const bool incomplete_terminal_group =
            !pending->have_odd && pending->primitive_terminal;
        if (pending->have_even && !incomplete_terminal_group)
            hidden = (uint8_t)((hidden & 1u) |
                               ((pending->even_value & 1u) << 1u));
        if (pending->have_odd)
            hidden = (uint8_t)((hidden & 2u) | (pending->odd_value & 1u));
        framebuffer_write_copy_hidden(memory, &primitive->framebuffer,
                                      pending->x, pending->y, hidden);
    } else if (pending->have_odd) {
        const uint32_t slot = framebuffer_slot(memory, &primitive->framebuffer,
                                               pending->x, pending->y);
        const uint32_t word_slot = slot & ~1u;
        const uint32_t sample = sr_sample_of(pending->x, pending->y);
        const uint16_t word = sr_memory_read_be16_sample(memory, word_slot, sample);
        const uint8_t previous = sr_memory_read_hidden_sample(memory, word_slot,
                                                               word, sample);
        const uint8_t replicated = (pending->odd_value & 1u) ? 3u : 0u;
        /* A complete byte pair commits the odd byte's ninth bit to both
         * lanes. An odd-only write changes just its own lane. */
        const uint8_t hidden = pending->have_even
            ? replicated : (uint8_t)((previous & 2u) | (replicated & 1u));
        framebuffer_write_copy_hidden(memory, &primitive->framebuffer,
                                      pending->x, pending->y, hidden);
        memory->copy_hidden_latch[sample][pending->pair & 7u] = replicated;
    } else if (pending->have_even) {
        const uint32_t slot = framebuffer_slot(memory, &primitive->framebuffer,
                                               pending->x, pending->y);
        const uint32_t word_slot = slot & ~1u;
        const uint32_t sample = sr_sample_of(pending->x, pending->y);
        const uint16_t word = sr_memory_read_be16_sample(memory, word_slot, sample);
        const uint8_t previous = sr_memory_read_hidden_sample(memory, word_slot,
                                                               word, sample);
        /* An even-only write keeps the adjacent lane and sources its ninth bit
         * from the memory interface's phase latch. */
        const uint8_t hidden = (uint8_t)((previous & 1u) |
            (memory->copy_hidden_latch[sample][pending->pair & 7u] & 2u));
        framebuffer_write_copy_hidden(memory, &primitive->framebuffer,
                                      pending->x, pending->y, hidden);
    }
    *pending = (rdp_copy_hidden_pair){0};
}

static void copy_accumulate_hidden_pair(
    sr_memory *memory, const rdp_primitive_state *primitive,
    rdp_copy_hidden_pair *pending, uint32_t x, uint32_t y,
    uint16_t value)
{
    const uint32_t slot = framebuffer_slot(memory, &primitive->framebuffer, x, y);
    if (slot == FB_SLOT_NONE) return;
    const uint32_t pair = slot >> 1;
    if (pending->valid && pending->pair != pair)
        copy_flush_hidden_pair(memory, primitive, pending);
    if (!pending->valid) {
        pending->valid = true;
        pending->pair = pair;
        pending->x = x;
        pending->y = y;
    }
    if (slot & 1u) {
        pending->have_odd = true;
        pending->odd_value = value;
        pending->x = x;
    } else {
        pending->have_even = true;
        pending->even_value = value;
    }
}

static void copy_fetch_group(const rdp_primitive_state *primitive,
                                      int first_x, int terminal_x, int direction,
                                      int32_t s_fixed, int32_t t_fixed,
                                      rdp_copy_group *group)
{
    const bool byte_target =
        primitive->framebuffer.color_image.size == RDP_SIZE_8BPP;
    const uint32_t capacity = copy_group_pixels(primitive);
    *group = (rdp_copy_group){0};
    while (group->count < capacity) {
        /* A copy lane is one hardware framebuffer pixel.  Raster coordinates
         * contain SOFTRDP_SCALE samples per pixel, so counting lanes directly
         * in raster space would consume a new texel for every subpixel. */
        const int pixel_x = first_x + direction *
            (int)(group->count * (uint32_t)SOFTRDP_SCALE);
        if (direction > 0 ? pixel_x > terminal_x : pixel_x < terminal_x) break;
        group->count++;
    }

    if (primitive->framebuffer.color_image.size == RDP_SIZE_4BPP) {
        group->valid_mask = group->count == 8u
            ? 0xffu : (uint8_t)((1u << group->count) - 1u);
        return;
    }

    for (uint32_t member = 0; member < group->count; member++) {
        uint16_t raw_word;
        if (!tmem_fetch_copy_word_fixed5(primitive->tmem,
                &primitive->texture, s_fixed, t_fixed, member, &raw_word))
            continue;
        /* An 8bpp pixel is the high byte of its copy word: the first two
         * words hold the replicated texels at S and S+2, the last two the raw
         * TMEM halfwords holding texels S+2 and S+3. */
        group->value[member] = byte_target ? (uint16_t)(raw_word >> 8) : raw_word;
        group->valid_mask |= (uint8_t)(1u << member);
    }
}

static sr_result copy_write_group(sr_memory *memory,
                                           const rdp_primitive_state *primitive,
                                           uint32_t y, int first_x, int terminal_x,
                                           int direction,
                                           const rdp_copy_group *group,
                                           rdp_copy_hidden_pair *pending)
{
    for (uint32_t lane = 0; lane < group->count; lane++) {
        if (!(group->valid_mask & (uint8_t)(1u << lane))) continue;
        const uint16_t copy_value = group->value[lane];
        const bool passes_alpha =
            !(primitive->plan.stages & RDP_DRAW_ALPHA_COMPARE) ||
            (primitive->framebuffer.color_image.size == RDP_SIZE_16BPP
                ? (copy_value & 1u) != 0u
                : copy_value >= primitive->fragment.blend.blend_color.a);
        if (!passes_alpha) continue;
        for (uint32_t sx = 0; sx < (uint32_t)SOFTRDP_SCALE; sx++) {
            const int pixel_x = first_x + direction *
                (int)(lane * (uint32_t)SOFTRDP_SCALE + sx);
            if (direction > 0 ? pixel_x > terminal_x : pixel_x < terminal_x)
                break;
            const sr_result result = framebuffer_write_copy(memory,
                &primitive->framebuffer, (uint32_t)pixel_x, y, copy_value);
            if (result != SR_OK) return result;
            if (pending && primitive->framebuffer.color_image.size <= RDP_SIZE_8BPP)
                copy_accumulate_hidden_pair(memory, primitive, pending,
                    (uint32_t)pixel_x, y, copy_value);
        }
    }
    return SR_OK;
}

static sr_result kernel_copy_triangle(sr_memory *memory,
                                             const rdp_primitive_state *primitive,
                                             const rdp_span *work)
{
    if (!memory || !primitive || !work) return SR_ERROR_INVALID_ARGUMENT;
    if (work->x_end < work->x_begin) return SR_OK;

    const raster_decoded_triangle *decoded = &primitive->triangle;
    const int direction = decoded->position.flip ? 1 : -1;
    const uint32_t group_pixels = copy_group_pixels(primitive);
    int x = direction > 0 ? work->x_begin : work->x_end;
    const int terminal = direction > 0 ? work->x_end : work->x_begin;
    int32_t s = work->s_fixed;
    int32_t t = work->t_fixed;
    int32_t w = work->w_fixed;
    const int32_t dsdx = sr_scaled_derivative(decoded->texture.dsdx & ~0x1f);
    const int32_t dtdx = sr_scaled_derivative(decoded->texture.dtdx & ~0x1f);
    const int32_t dwdx = sr_scaled_derivative(decoded->texture.dwdx & ~0x1f);
    rdp_copy_hidden_pair pending = {0};

    while (direction > 0 ? x <= terminal : x >= terminal) {
        int32_t base_s, base_t;
        if (primitive->texture.perspective) {
            stage_perspective_divide(s, t, w, &base_s, &base_t);
        } else {
            base_s = stage_texcoord_direct(s);
            base_t = stage_texcoord_direct(t);
        }
        rdp_copy_group group;
        copy_fetch_group(primitive, x, terminal, direction,
                                  base_s, base_t, &group);
        const sr_result result = copy_write_group(memory, primitive,
            (uint32_t)work->y, x, terminal, direction, &group, &pending);
        if (result != SR_OK) return result;
        x += direction * (int)(group_pixels * (uint32_t)SOFTRDP_SCALE);
        s = (int32_t)((uint32_t)s + direction * (uint32_t)dsdx);
        t = (int32_t)((uint32_t)t + direction * (uint32_t)dtdx);
        w = (int32_t)((uint32_t)w + direction * (uint32_t)dwdx);
    }
    pending.primitive_terminal = work->copy_primitive_terminal;
    copy_flush_hidden_pair(memory, primitive, &pending);
    return SR_OK;
}

static sr_result kernel_copy_rectangle(sr_memory *memory,
                                               const rdp_primitive_state *primitive,
                                               const rdp_span *work)
{
    if (!memory || !primitive || !work) return SR_ERROR_INVALID_ARGUMENT;
    const uint32_t group_pixels = copy_group_pixels(primitive);
    rdp_copy_hidden_pair pending = {0};
    /* The coordinate steps once per copy group, counted from the span start. */
    for (int x = work->x_begin; x <= work->x_end;
         x += (int)(group_pixels * (uint32_t)SOFTRDP_SCALE)) {
        const uint32_t group_index =
            ((uint32_t)(x - work->x_begin) >> work->texture_step_shift) / group_pixels;
        const int32_t s_fixed = stage_rect_texcoord(work->s_fixed,
            work->dsdx_fixed, group_index, work->texture_coord_shift, 0);
        const int32_t t_fixed = stage_rect_texcoord(work->t_fixed,
            work->dtdx_fixed, group_index, work->texture_coord_shift, 0);
        rdp_copy_group group;
        copy_fetch_group(primitive, x, work->x_end, 1,
                                  s_fixed, t_fixed, &group);
        const sr_result result = copy_write_group(memory, primitive,
            (uint32_t)work->y, x, work->x_end, 1, &group, &pending);
        if (result != SR_OK) return result;
    }
    copy_flush_hidden_pair(memory, primitive, &pending);
    return SR_OK;
}

sr_result kernel_render_span(sr_memory *memory,
                             const rdp_primitive_state *primitive,
                             const rdp_span *work)
{
    if (!memory || !primitive || !work) return SR_ERROR_INVALID_ARGUMENT;
#if SOFTRDP_CENSUS
    if (primitive->kernel == RDP_KERNEL_TEXTURE_TRIANGLE_COPY ||
        primitive->kernel == RDP_KERNEL_TEXTURE_RECTANGLE_COPY ||
        (primitive->kernel == RDP_KERNEL_TRIANGLE &&
         (primitive->plan.stages & RDP_DRAW_FILL)))
        census_record_bulk(primitive, work);
#endif
    switch (primitive->kernel) {
    case RDP_KERNEL_TRIANGLE:
        return primitive->framebuffer.color_image.size == RDP_SIZE_8BPP
            ? kernel_span_image8(memory, primitive, work, false)
            : kernel_triangle(memory, primitive, work);
    case RDP_KERNEL_TEXTURE_TRIANGLE_COPY:
        return kernel_copy_triangle(memory, primitive, work);
    case RDP_KERNEL_TEXTURE_RECTANGLE:
        return primitive->framebuffer.color_image.size == RDP_SIZE_8BPP
            ? kernel_span_image8(memory, primitive, work, true)
            : kernel_rectangle(memory, primitive, work);
    case RDP_KERNEL_TEXTURE_RECTANGLE_COPY:
        return kernel_copy_rectangle(memory, primitive, work);
    default:
        return SR_ERROR_INVALID_ARGUMENT;
    }
}
