#ifndef STAGE_BACK_H
#define STAGE_BACK_H

#include "primitive.h"
#include "framebuffer.h"
#include "rdp_memory.h"

/*
 * Raster coordinates address the scaled grid; RDRAM addresses do not. Both
 * framebuffers are indexed by the hardware pixel containing the raster pixel,
 * with the sample index picking the slot inside it. At scale 1 these are the
 * plain expressions they replaced.
 */
static SR_ALWAYS_INLINE uint32_t stage_pixel_index(const rdp_primitive_state *primitive,
                                                   uint32_t raster_x, uint32_t raster_y)
{
    return sr_raster_to_pixel(raster_y) * primitive->framebuffer.color_image.width +
           sr_raster_to_pixel(raster_x);
}

/*
 * Both frame buffers are addressed by the RDRAM address of the hardware pixel
 * the raster pixel belongs to; the sample picks which of its samples. Aliasing
 * between render targets therefore behaves exactly as it does on hardware,
 * because the addressing is the hardware's.
 */
static SR_ALWAYS_INLINE uint32_t stage_color_slot(const sr_memory *memory,
                                                  const rdp_primitive_state *primitive,
                                                  uint32_t raster_x, uint32_t raster_y)
{
    return mask_addr(memory, primitive->framebuffer.color_image.address +
                     stage_pixel_index(primitive, raster_x, raster_y) *
                     primitive->framebuffer.bytes_per_pixel);
}

static SR_ALWAYS_INLINE uint32_t stage_depth_slot(const sr_memory *memory,
                                                  const rdp_primitive_state *primitive,
                                                  uint32_t raster_x, uint32_t raster_y)
{
    return mask_addr(memory, primitive->fragment.depth.image_address +
                     stage_pixel_index(primitive, raster_x, raster_y) * 2u);
}

/* Shared by RGB and alpha dither: the "magic square" pattern for even dither
 * modes, the Bayer pattern for odd ones. Indexed by ((y & 3) << 2) | (x & 3). */
static inline const uint8_t *stage_dither_matrix(bool bayer)
{
    static const uint8_t magic[16] = {
        0, 6, 1, 7, 4, 2, 5, 3, 3, 5, 2, 4, 7, 1, 6, 0
    };
    static const uint8_t bayer_matrix[16] = {
        0, 4, 1, 5, 4, 0, 5, 1, 3, 7, 2, 6, 7, 3, 6, 2
    };
    return bayer ? bayer_matrix : magic;
}

static SR_ALWAYS_INLINE uint8_t stage_dither_component(uint8_t value, uint8_t threshold)
{
    if ((value & 7u) <= threshold) return value;
    return value > 247u ? 255u : (uint8_t)((value & 0xf8u) + 8u);
}

/* x and y are hardware pixel coordinates: dither is a pattern on the display
 * grid, not on the raster grid. */
static SR_ALWAYS_INLINE rdp_color stage_dither_rgb(rdp_color color, uint8_t mode,
                                                   uint32_t x, uint32_t y,
                                                   uint32_t noise)
{
    if (mode == 3u) return color;
    if (mode == 2u) {
        color.r = stage_dither_component(color.r, (uint8_t)(noise & 7u));
        color.g = stage_dither_component(color.g, (uint8_t)((noise >> 3) & 7u));
        color.b = stage_dither_component(color.b, (uint8_t)((noise >> 6) & 7u));
    } else {
        const uint8_t threshold =
            stage_dither_matrix(mode != 0u)[((y & 3u) << 2) | (x & 3u)];
        color.r = stage_dither_component(color.r, threshold);
        color.g = stage_dither_component(color.g, threshold);
        color.b = stage_dither_component(color.b, threshold);
    }
    return color;
}

static SR_ALWAYS_INLINE rdp_color stage_blend_color_source(uint8_t code,
                                                           const rdp_blend_state *s,
                                                           rdp_color pixel,
                                                           rdp_color memory)
{
    switch (code & 3u) {
    case 0: return pixel;
    case 1: return memory;
    case 2: return s->blend_color;
    default:return s->fog_color;
    }
}

static SR_ALWAYS_INLINE uint16_t stage_blend_factor_a(uint8_t code,
                                                      const rdp_blend_state *s,
                                                      uint16_t pixel_alpha,
                                                      uint8_t shade_alpha)
{
    switch (code & 3u) {
    case 0: return pixel_alpha;
    case 1: return s->fog_color.a;
    case 2: return shade_alpha;
    default:return 0u;
    }
}

static SR_ALWAYS_INLINE uint16_t stage_blend_factor_b(uint8_t code, uint16_t a,
                                                      rdp_color memory)
{
    switch (code & 3u) {
    case 0: return a >= 0x100u ? 0u : 0xffu - a;
    case 1: return memory.a;
    case 2: return 0xffu;
    default:return 0u;
    }
}

/* The blender divider's quotient for factor sum d, in steps of four, and
 * eleven-bit numerator n, at [(d << 11) | n]; see blend_divider.c. */
extern const uint8_t stage_blend_divider_table[0x8000];

static SR_ALWAYS_INLINE rdp_color stage_blend_cycle(const rdp_blend_state *s,
                                          const rdp_blender_cycle *c,
                                          rdp_blender_op operation,
                                          rdp_color pixel, uint16_t pixel_alpha,
                                          rdp_color memory,
                                          uint8_t shade_alpha,
                                          bool final_cycle,
                                          uint8_t blend_shift)
{
    rdp_color x;
    rdp_color y;
    uint16_t raw_a;
    switch (operation) {
    case RDP_BLEND_OP_FOG_SHADE_ALPHA:
        x = s->fog_color;
        y = pixel;
        raw_a = shade_alpha;
        break;
    case RDP_BLEND_OP_FOG_ALPHA:
        x = pixel;
        y = s->fog_color;
        raw_a = s->fog_color.a;
        break;
    case RDP_BLEND_OP_FOG_CONSTANT_ALPHA:
        x = s->fog_color;
        y = pixel;
        raw_a = s->fog_color.a;
        break;
    default:
        /* Other compiled operations are planning metadata. The generic
         * evaluator intentionally retains the selector implementation as its
         * exact fallback rather than growing another set of equation paths. */
        x = stage_blend_color_source(c->color_a, s, pixel, memory);
        y = stage_blend_color_source(c->color_b, s, pixel, memory);
        raw_a = stage_blend_factor_a(c->factor_a, s, pixel_alpha, shade_alpha);
        break;
    }
    if (final_cycle && c->factor_a == 0u && c->factor_b == 0u &&
        pixel_alpha >= 0xffu)
        return x;
    uint32_t a = raw_a >> 3;
    uint32_t raw_b = stage_blend_factor_b(c->factor_b, raw_a, memory) >> 3;
    /*
     * Memory alpha as the B factor is the framebuffer-blend case, and the
     * hardware does not use the factors at full resolution: it shifts both by
     * the amounts the depth stage derives from the delta exponents, keeps A a
     * multiple of four and forces the low two bits of B. The forced bits are
     * one step, invisible in a single draw and decisive in a fade that blends
     * a buffer with itself repeatedly -- without them the result lands one
     * quantization step below the value the hardware reaches.
     */
    if ((c->factor_b & 3u) == 1u) {
        a = (a >> (blend_shift & 15u)) & 0x3cu;
        raw_b = (raw_b >> (blend_shift >> 4)) | 3u;
    }
    const uint32_t b = raw_b + 1u;
    const uint32_t red = (uint32_t)x.r * a + (uint32_t)y.r * b;
    const uint32_t green = (uint32_t)x.g * a + (uint32_t)y.g * b;
    const uint32_t blue = (uint32_t)x.b * a + (uint32_t)y.b * b;
    if (!final_cycle || s->force_blend) {
        return (rdp_color){
            (uint8_t)(red >> 5),
            (uint8_t)(green >> 5),
            (uint8_t)(blue >> 5),
            pixel.a
        };
    }
    /* The normalising divide works on the factor sum in steps of four and the
     * upper eleven bits of each product. A sum of eight - every blend whose
     * factors add up to one - divides exactly; other sums take the circuit's
     * quotients from its table. */
    const uint32_t sum = (a >> 2) + (raw_b >> 2) + 1u;
    if (sum == 8u)
        return (rdp_color){ (uint8_t)(red >> 5), (uint8_t)(green >> 5),
                            (uint8_t)(blue >> 5), pixel.a };
    return (rdp_color){
        stage_blend_divider_table[(sum << 11) | ((red >> 2) & 0x7ffu)],
        stage_blend_divider_table[(sum << 11) | ((green >> 2) & 0x7ffu)],
        stage_blend_divider_table[(sum << 11) | ((blue >> 2) & 0x7ffu)],
        pixel.a
    };
}

static SR_ALWAYS_INLINE rdp_color stage_blend(const rdp_blend_state *s,
                                    rdp_color pixel,
                                    uint16_t pixel_alpha,
                                    rdp_color memory,
                                    uint8_t shade_alpha,
                                    bool blend_enable,
                                    bool color_on_coverage,
                                    bool coverage_wrap,
                                    uint8_t blend_shift)
{
    const rdp_blender_cycle *final = &s->program.cycle[s->final_cycle];
    if (s->cycle_count == 2u) {
        /* Cycle 0 is part of the two-cycle color pipeline (commonly fog), not
         * conditional framebuffer blending. Only cycle 1 may be bypassed by
         * blend_enable. */
        pixel = stage_blend_cycle(s, &s->program.cycle[0],
                                  (rdp_blender_op)s->program.operation[0], pixel,
                                  pixel_alpha, memory, shade_alpha, false, blend_shift);
        if (color_on_coverage && !coverage_wrap)
            return stage_blend_color_source(final->color_b, s, pixel, memory);
        if (!blend_enable)
            return stage_blend_color_source(final->color_a, s, pixel, memory);
        return stage_blend_cycle(s, &s->program.cycle[1],
                                 (rdp_blender_op)s->program.operation[1], pixel,
                                 pixel_alpha, memory, shade_alpha, true, blend_shift);
    }
    if (color_on_coverage && !coverage_wrap)
        return stage_blend_color_source(final->color_b, s, pixel, memory);
    if (!blend_enable)
        return stage_blend_color_source(final->color_a, s, pixel, memory);
    return stage_blend_cycle(s, &s->program.cycle[0],
                             (rdp_blender_op)s->program.operation[0], pixel,
                             pixel_alpha, memory, shade_alpha, true, blend_shift);
}

/*
 * Draw-constant part of the alpha stage. Alpha dither modes 0/1 use the matrix
 * chosen by the low bit of the RGB dither mode, with mode 1 inverting the value;
 * mode 2 uses per-pixel noise, mode 3 disables it.
 */
typedef struct stage_alpha_setup {
    const uint8_t *dither_matrix;
    bool dither_invert;
    bool dither_noise;
    bool two_cycle_compare;
    uint8_t dither_y_shift;
} stage_alpha_setup;

static inline stage_alpha_setup stage_alpha_prepare(const rdp_fragment_state *state)
{
    stage_alpha_setup setup = {
        .dither_matrix = NULL,
        .dither_invert = false,
        .dither_noise = state->alpha_dither == 2u,
        .two_cycle_compare = state->blend.alpha_compare &&
                             state->blend.cycle_count == 2u,
        .dither_y_shift = state->dither_y_shift
    };
    if (state->alpha_dither < 2u) {
        setup.dither_matrix = stage_dither_matrix((state->rgb_dither & 1u) != 0u);
        setup.dither_invert = state->alpha_dither == 1u;
    }
    return setup;
}

static SR_ALWAYS_INLINE uint32_t stage_alpha_dither(const stage_alpha_setup *setup,
                                                    uint32_t noise,
                                                    uint32_t x, uint32_t y)
{
    if (setup->dither_noise) return noise & 7u;
    if (!setup->dither_matrix) return 0u;
    const uint32_t value =
        setup->dither_matrix[(((y >> setup->dither_y_shift) & 3u) << 2) | (x & 3u)];
    return setup->dither_invert ? (~value & 7u) : value;
}

/* The shade alpha the blender reads: the alpha dither is added to it as it is
 * to the combiner's alpha, and the sum saturates. */
static SR_ALWAYS_INLINE uint8_t stage_blend_shade_alpha(uint8_t shade_alpha,
                                                        uint32_t dither)
{
    const uint32_t value = (uint32_t)shade_alpha + dither;
    return value > 0xffu ? 0xffu : (uint8_t)value;
}

typedef struct stage_alpha_result {
    uint16_t alpha;
    uint8_t coverage;
    bool accept;
} stage_alpha_result;

/*
 * Combiner alpha and coverage in, blender alpha and pixel coverage out.
 * cycle0_alpha is the clamped first-cycle alpha, read only by the two-cycle
 * alpha compare. x and y are hardware pixel coordinates.
 */
static SR_ALWAYS_INLINE stage_alpha_result stage_alpha_split(const rdp_fragment_state *state,
                                                             const stage_alpha_setup *setup,
                                                             uint16_t combined_alpha,
                                                             uint16_t cycle0_alpha,
                                                             uint8_t coverage,
                                                             uint8_t compare_coverage,
                                                             uint16_t noise,
                                                             uint32_t x, uint32_t y,
                                                             int32_t key_alpha)
{
    stage_alpha_result result = { combined_alpha, coverage, true };
    /* Two cycles compare the first cycle's alpha; the second cycle's never
     * reaches the comparator. Same coverage and dither rules, but computed
     * before CVG_X_ALPHA rewrites the coverage this depends on. */
    uint32_t compare_reference = 0u;
    if (setup->two_cycle_compare) {
        compare_reference = cycle0_alpha;
        if (compare_reference == 0xffu) compare_reference = 0x100u;
        if (state->alpha_cvg_select) {
            compare_reference = state->cvg_times_alpha
                ? ((compare_reference * compare_coverage + 4u) >> 3)
                : ((uint32_t)compare_coverage << 5);
            if (compare_reference > 0xffu) compare_reference = 0xffu;
        } else {
            compare_reference += stage_alpha_dither(setup, noise, x, y);
            if (compare_reference & 0x100u) compare_reference = 0xffu;
        }
    }
    const uint32_t combiner_alpha = combined_alpha == 0xffu ? 0x100u : combined_alpha;
    const uint32_t scaled_alpha = (combiner_alpha * coverage + 4u) >> 3;
    if (state->cvg_times_alpha) {
        result.coverage = (uint8_t)((scaled_alpha >> 5) & 0xfu);
        /* With antialiasing disabled the blender gates writes with the
         * original coverage bit, not the alpha-scaled coverage count. */
        if (state->antialias && result.coverage == 0u) {
            result.accept = false;
            return result;
        }
    }
    if (state->alpha_cvg_select) {
        /* Only ALPHA_CVG_SELECT routes the coverage-scaled value into alpha. On
         * its own CVG_X_ALPHA updates the coverage and leaves alpha as the
         * combiner produced it. */
        const uint32_t value = state->cvg_times_alpha
            ? scaled_alpha : ((uint32_t)result.coverage << 5);
        result.alpha = (uint16_t)(value > 0xffu ? 0xffu : value);
    } else if (key_alpha >= 0) {
        /* A chroma-keyed pixel takes the key alpha, undithered
         * (stage_chroma_key). */
        result.alpha = (uint16_t)key_alpha;
    } else if (setup->dither_matrix || setup->dither_noise) {
        const uint32_t dithered =
            (uint32_t)result.alpha + stage_alpha_dither(setup, noise, x, y);
        result.alpha = dithered > 0xffu ? 0xffu : (uint16_t)dithered;
    }
    if (state->blend.alpha_compare) {
        /* Dithered alpha compare replaces the blend-colour threshold with a
         * per-pixel noise value. */
        const uint8_t threshold = state->blend.alpha_compare_dither
            ? (uint8_t)(noise & 0xffu)
            : state->blend.blend_color.a;
        if ((setup->two_cycle_compare ? compare_reference : result.alpha) < threshold)
            result.accept = false;
    }
    return result;
}

/* The alpha stage when the two-cycle compare sees the same pixel's cycle 0.
 * The kernel feeds the pipelined neighbour through stage_alpha_split. */
static SR_ALWAYS_INLINE stage_alpha_result stage_alpha(const rdp_fragment_state *state,
                                                       const stage_alpha_setup *setup,
                                                       uint16_t combined_alpha,
                                                       uint16_t cycle0_alpha,
                                                       uint8_t coverage,
                                                       uint16_t noise,
                                                       uint32_t x, uint32_t y)
{
    return stage_alpha_split(state, setup, combined_alpha, cycle0_alpha, coverage,
                             coverage, noise, x, y, -1);
}

typedef struct stage_depth_result {
    bool pass;
    bool update;
    bool farther;
    uint32_t address;
    uint32_t sample;
    uint16_t compressed;
    uint8_t hidden;
    /* The coverage interpenetrating mode rescaled the pixel to, or 0xff. */
    uint8_t coverage;
    uint8_t blend_shift;
} stage_depth_result;

static SR_ALWAYS_INLINE uint32_t stage_depth_decompress(uint16_t stored)
{
    static const uint8_t shift[8] = { 6u, 5u, 4u, 3u, 2u, 1u, 0u, 0u };
    static const uint32_t add[8] = {
        0x00000u, 0x20000u, 0x30000u, 0x38000u,
        0x3c000u, 0x3e000u, 0x3f000u, 0x3f800u
    };
    const uint32_t encoded = (stored >> 2) & 0x3fffu;
    const uint32_t exponent = encoded >> 11;
    return (((encoded & 0x7ffu) << shift[exponent]) + add[exponent]) & 0x3ffffu;
}

static SR_ALWAYS_INLINE uint16_t stage_depth_compress(uint32_t depth)
{
    depth &= 0x3ffffu;
    if (depth < 0x20000u) return (uint16_t)((depth >> 4) & 0x1ffcu);
    if (depth < 0x30000u) return (uint16_t)(((depth >> 3) & 0x1ffcu) | 0x2000u);
    if (depth < 0x38000u) return (uint16_t)(((depth >> 2) & 0x1ffcu) | 0x4000u);
    if (depth < 0x3c000u) return (uint16_t)(((depth >> 1) & 0x1ffcu) | 0x6000u);
    if (depth < 0x3e000u) return (uint16_t)((depth & 0x1ffcu) | 0x8000u);
    if (depth < 0x3f000u) return (uint16_t)(((depth << 1) & 0x1ffcu) | 0xa000u);
    if (depth < 0x3f800u) return (uint16_t)(((depth << 2) & 0x1ffcu) | 0xc000u);
    return (uint16_t)(((depth << 2) & 0x1ffcu) | 0xe000u);
}

/*
 * The depth test. It decides and prepares the store; stage_depth_commit writes
 * it once the pixel has been accepted by every later stage.
 */
static SR_ALWAYS_INLINE sr_result stage_depth_test(sr_memory *memory,
                                         const rdp_primitive_state *primitive,
                                         int32_t depth_snapped,
                                         uint32_t addr,
                                         uint32_t color_addr,
                                         uint32_t sample,
                                         uint8_t current_coverage,
                                         stage_depth_result *result,
                                         const bool special)
{
    const rdp_depth_state *depth = &primitive->fragment.depth;
    *result = (stage_depth_result){ .pass = true, .farther = true,
                                    .coverage = 0xffu,
                                    .blend_shift = depth->blend_shift };
    if ((!primitive->triangle.has_depth && !depth->source_primitive) ||
        addr == FB_SLOT_NONE) {
        return SR_OK;
    }

    uint16_t old_stored = 0xffffu;
    uint32_t new_depth;
    if (depth->source_primitive) {
        new_depth = ((uint32_t)depth->primitive_depth << 3) & 0x3ffffu;
    } else {
        /* Depth arrives already centroid-snapped from span setup. Only the
         * 19-bit overflow fold is left. */
        const uint32_t scaled = (uint32_t)depth_snapped & 0x7ffffu;
        const uint32_t overflow = (scaled >> 17) & 3u;
        new_depth = overflow < 2u ? scaled & 0x3ffffu :
                    overflow == 2u ? 0x3ffffu : 0u;
    }
    if (depth->compare)
        old_stored = sr_memory_read_be16_sample(memory, addr, sample);
    if (depth->compare) {
        const uint32_t old_depth = stage_depth_decompress(old_stored);
        const bool maximum = old_depth == 0x3ffffu;
        const bool in_front = new_depth < old_depth;
        /* The four-bit stored delta exponent: upper two bits in the halfword,
         * lower two in the hidden plane. Both the stored and the incoming delta
         * are single bits, so the hardware's comparator - the highest set bit of
         * their union - is a max on the exponents, and the whole comparison
         * stays out of the shifted domain until the final tolerance. */
        const uint32_t raw_memory_delta = ((old_stored & 3u) << 2) |
            sr_memory_read_hidden_depth(memory, addr, sample);
        /* The blender shifts the factor of whichever surface has the larger
         * delta, by the exponent difference up to four. */
        if (special && depth->blend_shift_live) {
            const int32_t exponent_gap =
                (int32_t)depth->delta_exponent - (int32_t)raw_memory_delta;
            result->blend_shift = (uint8_t)(
                (exponent_gap > 4 ? 4 : exponent_gap < 0 ? 0 : exponent_gap) |
                ((exponent_gap < -4 ? 4 : exponent_gap > 0 ? 0 : -exponent_gap) << 4));
        }
        /* The stored value's own exponent field. Depth compressed with a coarse
         * mantissa needs the tolerance widened to match, and a memory delta
         * already at its maximum there means the surfaces cannot be separated
         * at all: the comparison is forced coplanar. */
        const uint32_t precision_factor = (old_stored >> 13) & 7u;
        bool force_coplanar = false;
        uint32_t memory_exponent = raw_memory_delta;
        if (precision_factor < 3u) {
            if (raw_memory_delta != 15u) {
                const uint32_t floor_exponent = 4u - precision_factor;
                memory_exponent = raw_memory_delta + 1u;
                if (memory_exponent < floor_exponent)
                    memory_exponent = floor_exponent;
            } else {
                force_coplanar = true;
                memory_exponent = 15u;
            }
        }
        uint32_t delta = 1u << (memory_exponent + 3u);
        const uint32_t incoming_delta = (uint32_t)depth->pixel_delta << 3u;
        if (incoming_delta > delta) delta = incoming_delta;
        const bool nearer = force_coplanar || new_depth <= old_depth + delta;
        const bool farther = force_coplanar || new_depth + delta >= old_depth;
        result->farther = farther;
        /* current_coverage is the pixel coverage as the alpha stage leaves it,
         * which under CVG_X_ALPHA is not the geometric coverage span setup
         * produced - hence the late depth order, which moves this whole stage
         * after that rewrite. Full coverage overflows against every possible
         * memory value, so it needs neither the read nor the sum. */
        bool overflow = true;
        if ((depth->mode & 3u) < 2u && current_coverage != 8u) {
            uint8_t memory_coverage = 7u;
            if (primitive->fragment.blend.image_read &&
                primitive->framebuffer.store_size != RDP_SIZE_8BPP) {
                rdp_memory_pixel memory_pixel;
                const sr_result read_result = framebuffer_read_memory_address(memory,
                    primitive->framebuffer.store_size, color_addr, true,
                    sample, &memory_pixel);
                if (read_result != SR_OK) return read_result;
                memory_coverage = memory_pixel.coverage;
            }
            overflow = ((current_coverage + memory_coverage) & 8u) != 0u;
        }
        switch (depth->mode & 3u) {
        case 0u: result->pass = maximum || (overflow ? in_front : nearer); break;
        case 1u:
            /* A surface in front but within the delta of an overflowing one
             * intersects it: it passes, with its coverage scaled by how far in
             * front it is, in units of the combined delta. */
            if (special && in_front && farther && overflow) {
                const uint32_t combined = (delta >> 3) & 0xffffu;
                const uint32_t shift = combined ? 31u - (uint32_t)__builtin_clz(combined) : 0u;
                const uint32_t scale = ((old_depth >> shift) - (new_depth >> shift)) & 15u;
                result->coverage = (uint8_t)(((scale * current_coverage) >> 3) & 15u);
                /* With antialiasing the blender writes only covered pixels,
                 * so a surface scaled down to nothing is dropped. */
                result->pass = result->coverage != 0u || !primitive->fragment.antialias;
            } else {
                result->pass = maximum || (overflow ? in_front : nearer);
            }
            break;
        case 2u: result->pass = maximum || in_front; break;
        default: result->pass = !maximum && nearer && farther; break;
        }
    }
    if (result->pass && depth->update) {
        result->update = true;
        result->address = addr;
        result->sample = sample;
        result->compressed = (uint16_t)(stage_depth_compress(new_depth) |
                                        ((depth->delta_exponent >> 2) & 3u));
        result->hidden = (uint8_t)(depth->delta_exponent & 3u);
    }
    return SR_OK;
}

static SR_ALWAYS_INLINE sr_result stage_depth_commit(sr_memory *memory,
                                                     const stage_depth_result *depth,
                                                     bool fragment_accepted)
{
    if (!depth->update) return SR_OK;
    if (!fragment_accepted || depth->address == FB_SLOT_NONE) {
        return SR_OK;
    }
    sr_memory_write_be16_sample(memory, depth->address, depth->compressed,
                                depth->sample);
    sr_memory_write_hidden_sample(memory, depth->address, depth->hidden,
                                  depth->sample);
    return SR_OK;
}

/*
 * When the framebuffer read is needed. It is the most expensive part of the
 * write and only matters when something downstream consumes the result.
 *
 * The memory color reaches the blender through the compiled input mask, which
 * already covers the color_a/color_b/factor_b selectors of every evaluated
 * cycle - including the bypass paths taken when blending is disabled, which
 * still return the memory color source.
 *
 * The memory coverage feeds two consumers. COVERAGE_DEST uses it directly for
 * wrap (1) and save (3), and for clamp (0) whenever the blend is enabled -
 * which without antialiasing means force_blend. It also feeds the overflow
 * flag, but that is only observable through blend_enable (antialias) and
 * coverage_wrap (color_on_cvg), and with full coverage the sum overflows for
 * every possible memory value, so the read cannot change the flag.
 */
typedef struct stage_write_setup {
    bool read_unless_full_coverage;
    bool read_always;
} stage_write_setup;

/*
 * A fully covered pixel overflows against every memory coverage, so unless
 * blending is forced it is written through the blend-disabled path: cycle 0
 * (in two-cycle mode) and then the final cycle's colour_a source, with
 * coverage clamp or full not reading memory either. Memory then matters only
 * if one of those selects it, or the coverage mode is wrap or save. Any
 * memory selector counts, so the test can only skip reads nobody uses.
 */
static inline bool stage_full_coverage_reads_memory(const rdp_fragment_state *state)
{
    const rdp_blend_state *blend = &state->blend;
    const uint8_t coverage_dest = (uint8_t)(state->coverage_dest & 3u);
    if (blend->force_blend || coverage_dest == 1u || coverage_dest == 3u) return true;
    if ((blend->program.cycle[blend->final_cycle].color_a & 3u) == 1u) return true;
    if (blend->cycle_count == 2u) {
        const rdp_blender_cycle *first = &blend->program.cycle[0];
        if ((first->color_a & 3u) == 1u || (first->color_b & 3u) == 1u ||
            (first->factor_b & 3u) == 1u)
            return true;
    }
    return false;
}

static inline stage_write_setup stage_write_prepare(const rdp_fragment_state *state)
{
    const uint8_t coverage_dest = (uint8_t)(state->coverage_dest & 3u);
    const bool needs_memory_color =
        (state->blend.input_mask & RDP_BLENDER_INPUT_MEMORY) != 0u;
    const bool needs_memory_coverage =
        coverage_dest == 1u || coverage_dest == 3u ||
        (coverage_dest == 0u && state->blend.force_blend);
    const bool needs_overflow = state->antialias || state->color_on_cvg;
    return (stage_write_setup){
        .read_unless_full_coverage = state->blend.image_read &&
            (needs_memory_color || needs_memory_coverage || needs_overflow),
        .read_always = state->blend.image_read &&
            (needs_memory_color || needs_memory_coverage) &&
            stage_full_coverage_reads_memory(state)
    };
}

/* x and y are raster coordinates. */
static SR_ALWAYS_INLINE sr_result stage_write_color(sr_memory *memory,
                                                    const rdp_primitive_state *primitive,
                                                    const stage_write_setup *setup,
                                                    rdp_texture_size size,
                                                    uint32_t color_address,
                                                    uint32_t x, uint32_t y,
                                                    rdp_color pixel,
                                                    uint16_t alpha,
                                                    uint8_t shade_alpha,
                                                    uint8_t coverage,
                                                    const stage_depth_result *depth,
                                                    uint16_t noise,
                                                    rdp_hidden8_writer *writer)
{
    const rdp_fragment_state *state = &primitive->fragment;
    rdp_memory_pixel memory_pixel;
    const bool need_read = coverage < 8u
        ? setup->read_unless_full_coverage : setup->read_always;
    const uint32_t sample = sr_sample_of(x, y);
    const sr_result result = framebuffer_read_memory_address(
        memory, size, color_address, need_read, sample, &memory_pixel);
    if (result != SR_OK) return result;
    const uint8_t memory_coverage = memory_pixel.coverage;
    const bool overflow = ((coverage + memory_coverage) & 8u) != 0u;
    /* The coverage blend also requires the depth test's `farther`: an edge
     * pixel in front of what is already there is written opaque. */
    const bool blend_enable = state->blend.force_blend ||
                              (state->antialias && !overflow && depth->farther);
    /* COLOR_ON_CVG affects only the final blender cycle. In two-cycle mode
     * cycle 0 must still produce the final cycle's pixel input. */
    pixel = stage_blend(&state->blend, pixel, alpha, memory_pixel.color,
                        shade_alpha, blend_enable, state->color_on_cvg, overflow,
                        depth->blend_shift);
    /* The overflow and blend decisions above use the pixel's coverage; the
     * stored coverage is the one interpenetrating mode rescaled it to. */
    const uint32_t stored_coverage =
        depth->coverage == 0xffu ? coverage : depth->coverage;
    uint32_t final_coverage;
    switch (state->coverage_dest & 3u) {
    case 0:
        final_coverage = blend_enable
            ? stored_coverage + memory_coverage
            : stored_coverage - 1u;
        if (final_coverage > 7u) final_coverage = 7u;
        break;
    case 1: final_coverage = (stored_coverage + memory_coverage) & 7u; break;
    case 2: final_coverage = 7u; break;
    default: final_coverage = memory_coverage; break;
    }
    pixel.a = (uint8_t)((final_coverage << 5) | 0x1fu);
    /* The 8bpp lane split keys on the hardware pixel, not the raster one. */
    const uint32_t pixel_index =
        sr_raster_to_pixel(y) * primitive->framebuffer.color_image.width +
        sr_raster_to_pixel(x);
    /* The blender dithers its output for every image size that stores colour,
     * not only 16bpp. */
    if (size != RDP_SIZE_4BPP)
        pixel = stage_dither_rgb(pixel, state->rgb_dither, sr_raster_to_pixel(x),
                                 sr_raster_to_pixel(y) >> state->dither_y_shift,
                                 noise);
    if (size == RDP_SIZE_8BPP)
        return framebuffer_write_color8(memory, color_address, pixel_index,
                                        sample, pixel, writer);
    /* A 4bpp image only ever receives zero bytes. */
    if (size == RDP_SIZE_4BPP) {
        if (color_address != FB_SLOT_NONE)
            sr_memory_write_u8_sample(memory, color_address, 0u, sample);
        return SR_OK;
    }
    return framebuffer_write_color_address(memory, size, color_address,
                                           pixel_index, sample, pixel);
}

/*
 * The colour stage_write_color would store to an 8bpp image, blended and
 * dithered, without storing it. The 8bpp span instance needs it for pixels it
 * writes later in the hardware's walking order, and for rejected ones, whose
 * dithered green still reaches the ninth bits of an odd byte.
 */
static SR_ALWAYS_INLINE sr_result stage_color8(sr_memory *memory,
                                               const rdp_primitive_state *primitive,
                                               const stage_write_setup *setup,
                                               uint32_t color_address,
                                               uint32_t x, uint32_t y,
                                               rdp_color pixel,
                                               uint16_t alpha,
                                               uint8_t shade_alpha,
                                               uint8_t coverage,
                                               const stage_depth_result *depth,
                                               uint16_t noise,
                                               rdp_color *out)
{
    const rdp_fragment_state *state = &primitive->fragment;
    rdp_memory_pixel memory_pixel;
    const bool need_read = coverage < 8u
        ? setup->read_unless_full_coverage : setup->read_always;
    const sr_result result = framebuffer_read_memory_address(
        memory, RDP_SIZE_8BPP, color_address, need_read, sr_sample_of(x, y),
        &memory_pixel);
    if (result != SR_OK) return result;
    const bool overflow = ((coverage + memory_pixel.coverage) & 8u) != 0u;
    const bool blend_enable = state->blend.force_blend ||
                              (state->antialias && !overflow && depth->farther);
    pixel = stage_blend(&state->blend, pixel, alpha, memory_pixel.color,
                        shade_alpha, blend_enable, state->color_on_cvg, overflow,
                        depth->blend_shift);
    *out = stage_dither_rgb(pixel, state->rgb_dither, sr_raster_to_pixel(x),
                            sr_raster_to_pixel(y) >> state->dither_y_shift, noise);
    return SR_OK;
}

#endif
