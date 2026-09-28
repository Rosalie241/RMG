#ifndef STAGE_FRONT_H
#define STAGE_FRONT_H

#include "primitive.h"
#include "tmem.h"

static SR_ALWAYS_INLINE int32_t sr_wrap_add(int32_t a, int32_t b)
{
    return (int32_t)((uint32_t)a + (uint32_t)b);
}

/* One colour with headroom: texels leave the texture unit with up to nine
 * significant bits per channel. */
typedef struct sr_rgba { uint16_t r, g, b, a; } sr_rgba;

static SR_ALWAYS_INLINE sr_rgba sr_rgba_from_color(rdp_color color)
{
    return (sr_rgba){ color.r, color.g, color.b, color.a };
}

static SR_ALWAYS_INLINE sr_rgba sr_rgba_from_wide(const uint16_t value[4])
{
    return (sr_rgba){ value[0], value[1], value[2], value[3] };
}

/* The interpolants of one pixel. Stepping wraps like the hardware's
 * accumulators. */
typedef struct sr_attr { int32_t r, g, b, a, z, s, t, w; } sr_attr;

static SR_ALWAYS_INLINE sr_attr sr_attr_step(sr_attr value, sr_attr step)
{
    return (sr_attr){
        sr_wrap_add(value.r, step.r), sr_wrap_add(value.g, step.g),
        sr_wrap_add(value.b, step.b), sr_wrap_add(value.a, step.a),
        sr_wrap_add(value.z, step.z), sr_wrap_add(value.s, step.s),
        sr_wrap_add(value.t, step.t), sr_wrap_add(value.w, step.w)
    };
}

static SR_ALWAYS_INLINE uint8_t stage_shade_clamp9(int32_t component)
{
    component &= 0x1ff;
    if (component & 0x100) {
        return (component & 0x80) ? 0u : 255u;
    }
    return (uint8_t)component;
}

static SR_ALWAYS_INLINE uint8_t stage_shade_u8(int32_t interpolated)
{
    return stage_shade_clamp9(interpolated >> 16);
}

/*
 * Coverage-centroid adjustment for triangle attributes (shade, depth).
 *
 * `first_x` and `first_y` measure the offset of the first covered sub-sample
 * from the pixel origin, in QUARTER pixels (0..3) -- not centred on the pixel.
 * Attribute centroid correction adds `off * (derivative >> n)` at a coarse
 * scale, so a centred offset would inject a constant half-pixel bias that has
 * nothing to do with where the coverage actually lies.
 */
static SR_ALWAYS_INLINE uint8_t stage_shade_centroid_u8(int32_t value,
                                                        int32_t ddx, int32_t ddy,
                                                        uint8_t first_x, uint8_t first_y)
{
    int32_t snapped = (int32_t)((uint32_t)(value >> 14) << 2);
    snapped += (int32_t)first_x * (ddx >> 14) + (int32_t)first_y * (ddy >> 14);
    snapped >>= 4;
    return stage_shade_clamp9(snapped);
}

static SR_ALWAYS_INLINE int32_t stage_depth_centroid(int32_t z,
                                                     int32_t dzdx, int32_t dzdy,
                                                     uint8_t first_x, uint8_t first_y)
{
    int32_t snapped = (int32_t)((uint32_t)(z >> 10) << 2);
    snapped += (int32_t)first_x * (dzdx >> 10) + (int32_t)first_y * (dzdy >> 10);
    return snapped >> 5;
}

static SR_ALWAYS_INLINE int32_t stage_texcoord_direct(int32_t interpolated)
{
    return (int16_t)((uint32_t)interpolated >> 16);
}

static SR_ALWAYS_INLINE int32_t stage_rect_texcoord(int32_t base, int32_t step,
                                                    uint32_t index, uint8_t shift,
                                                    uint8_t step_shift)
{
    const int64_t accumulated = (int64_t)base + (int64_t)step * (index >> step_shift);
    return shift ? (int32_t)(accumulated >> shift) : (int32_t)accumulated;
}

/* Divider ROM: 64 line segments over the normalized W mantissa. */
#define PERSP_RECIP(d) ((int32_t)((1048576u + (d) / 2u) / (d)))
/* Two adjacent intervals use the ROM's quantized line fit rather than the
 * rounded reciprocal endpoints. */
#define PERSP_BASE(i) (PERSP_RECIP(64u + (i)) - ((i) == 6u))
#define PERSP_SLOPE(i) (4 * (PERSP_RECIP(65u + (i)) - PERSP_RECIP(64u + (i))) \
                        - 4 * ((i) == 5u) + 4 * ((i) == 6u))
#define PERSP_ROM8(m, i) m(i), m(i + 1u), m(i + 2u), m(i + 3u), \
                         m(i + 4u), m(i + 5u), m(i + 6u), m(i + 7u)
#define PERSP_ROM64(m) PERSP_ROM8(m, 0u), PERSP_ROM8(m, 8u), \
                       PERSP_ROM8(m, 16u), PERSP_ROM8(m, 24u), \
                       PERSP_ROM8(m, 32u), PERSP_ROM8(m, 40u), \
                       PERSP_ROM8(m, 48u), PERSP_ROM8(m, 56u)

static SR_ALWAYS_INLINE int32_t stage_perspective_reciprocal(uint32_t normalized)
{
    static const int32_t base[64] = { PERSP_ROM64(PERSP_BASE) };
    static const int32_t slope[64] = { PERSP_ROM64(PERSP_SLOPE) };
    const uint32_t interval = normalized >> 8;
    const int32_t fraction = (int32_t)(normalized & 0xffu);
    return base[interval] + (slope[interval] * fraction >> 10);
}

#undef PERSP_ROM64
#undef PERSP_ROM8
#undef PERSP_SLOPE
#undef PERSP_BASE
#undef PERSP_RECIP

static SR_ALWAYS_INLINE int32_t stage_perspective_clamp(int32_t divided)
{
    return divided < -0x10000 ? -0x10000 : divided > 0xffff ? 0xffff : divided;
}

/* Returns true when the result is unusable for LOD: W was not positive or a
 * coordinate clamped. */
static SR_ALWAYS_INLINE bool stage_perspective_divide(int32_t s, int32_t t, int32_t w,
                                                      int32_t *out_s, int32_t *out_t)
{
    /* |coord| <= 2^15 and the reciprocal <= 2^14, so the doubled product fits
     * in 32 bits. */
    const int32_t quantized_s = (int16_t)((uint32_t)s >> 16);
    const int32_t quantized_t = (int16_t)((uint32_t)t >> 16);
    const int32_t quantized_w = (int16_t)((uint32_t)w >> 16);
    const bool invalid = quantized_w <= 0;
    const uint32_t magnitude = invalid ? 1u : (uint32_t)quantized_w;

    const int32_t top_bit = 31 - __builtin_clz(magnitude);
    const uint32_t normalized = (magnitude << (14 - top_bit)) & 0x3fffu;
    const int32_t reciprocal = stage_perspective_reciprocal(normalized);

    const int32_t divided_s = (quantized_s * reciprocal * 2) >> top_bit;
    const int32_t divided_t = (quantized_t * reciprocal * 2) >> top_bit;
    const int32_t clamped_s = stage_perspective_clamp(divided_s);
    const int32_t clamped_t = stage_perspective_clamp(divided_t);
    *out_s = invalid ? 0x7fff : clamped_s;
    *out_t = invalid ? 0x7fff : clamped_t;
    return invalid || clamped_s != divided_s || clamped_t != divided_t;
}

typedef struct stage_lod_result {
    uint16_t fraction;
    uint8_t tile0;
    uint8_t tile1;
} stage_lod_result;

static SR_ALWAYS_INLINE int32_t stage_lod_sign_extend_17(int32_t value)
{
    const uint32_t bits = (uint32_t)value & 0x1ffffu;
    return (int32_t)((bits ^ 0x10000u) - 0x10000u);
}

static SR_ALWAYS_INLINE uint32_t stage_lod_delta(int32_t scurr, int32_t snext,
                                                 int32_t tcurr, int32_t tnext,
                                                 uint32_t previous)
{
    int32_t ds = stage_lod_sign_extend_17(snext) - stage_lod_sign_extend_17(scurr);
    int32_t dt = stage_lod_sign_extend_17(tnext) - stage_lod_sign_extend_17(tcurr);
    if (((uint32_t)ds & 0x20000u) != 0u) ds = (int32_t)(~ds & 0x1ffff);
    if (((uint32_t)dt & 0x20000u) != 0u) dt = (int32_t)(~dt & 0x1ffff);
    uint32_t delta = (uint32_t)(ds > dt ? ds : dt);
    if (previous > delta) delta = previous;
    uint32_t lod = delta & 0x7fffu;
    if (delta & 0x1c000u) lod |= 0x4000u;
    return lod;
}

static SR_ALWAYS_INLINE uint32_t stage_lod_log2(uint32_t value)
{
    uint32_t level = 0u;
    while (value >>= 1u) level++;
    return level;
}

static SR_ALWAYS_INLINE stage_lod_result stage_lod_resolve(const rdp_primitive_state *primitive,
                                                 int32_t s, int32_t t,
                                                 int32_t sx, int32_t tx,
                                                 int32_t sy, int32_t ty,
                                                 bool lod_clamp)
{
    uint32_t lod = 0u;
    if (!lod_clamp) {
        lod = stage_lod_delta(s, sx, t, tx, 0u);
        lod = stage_lod_delta(s, sy, t, ty, lod);
    }

    uint32_t level = 0u;
    bool magnify;
    bool distant;
    uint16_t fraction;
    if ((lod & 0x4000u) || lod_clamp) {
        magnify = false;
        distant = true;
        fraction = 0xffu;
    } else if (lod < primitive->lod_min_level) {
        magnify = true;
        distant = primitive->lod_max_level == 0u;
        if (!primitive->sharpen_lod && !primitive->detail_lod)
            fraction = distant ? 0xffu : 0u;
        else {
            fraction = (uint16_t)(primitive->lod_min_level << 3);
            if (primitive->sharpen_lod) fraction |= 0x100u;
        }
    } else if (lod < 32u) {
        magnify = true;
        distant = primitive->lod_max_level == 0u;
        if (!primitive->sharpen_lod && !primitive->detail_lod)
            fraction = distant ? 0xffu : 0u;
        else {
            fraction = (uint16_t)(lod << 3);
            if (primitive->sharpen_lod) fraction |= 0x100u;
        }
    } else {
        magnify = false;
        level = stage_lod_log2((lod >> 5) & 0xffu);
        distant = primitive->lod_max_level
            ? ((lod & 0x6000u) != 0u || level >= primitive->lod_max_level)
            : true;
        fraction = (!primitive->sharpen_lod && !primitive->detail_lod && distant)
            ? 0xffu : (uint16_t)(((lod << 3) >> level) & 0xffu);
    }

    uint8_t tile0 = primitive->lod_base_tile;
    uint8_t tile1 = tile0;
    if (primitive->texture_lod) {
        if (distant) level = primitive->lod_max_level;
        if (!primitive->detail_lod) {
            tile0 = (uint8_t)((primitive->lod_base_tile + level) & 7u);
            tile1 = (distant || (!primitive->sharpen_lod && magnify))
                ? tile0 : (uint8_t)((tile0 + 1u) & 7u);
        } else {
            tile0 = (uint8_t)((primitive->lod_base_tile + level +
                               (magnify ? 0u : 1u)) & 7u);
            tile1 = (uint8_t)((primitive->lod_base_tile + level +
                               (!distant && !magnify ? 2u : 1u)) & 7u);
        }
    }
    return (stage_lod_result){ fraction, tile0, tile1 };
}

static SR_ALWAYS_INLINE bool stage_texel_sample(const tmem_state *tmem,
                                                const rdp_texture_sample_state *sample,
                                                int32_t s, int32_t t,
                                                rdp_color *color,
                                                rdp_texture_unit sampler)
{
    if (sampler == RDP_TEXTURE_UNIT_RGBA16_BILERP)
        return tmem_sample_rgba16_bilerp_fixed5(tmem, sample, s, t, color);
    if (sampler == RDP_TEXTURE_UNIT_RGBA16_POINT)
        return tmem_sample_rgba16_point_fixed5(tmem, sample, s, t, color);
    return tmem_sample_color_fixed5_raw(tmem, sample, s, t, color);
}

/*
 * Whether the tile-1 sample has to be taken at all, or already exists as the
 * tile-0 sample. In one-cycle mode primitive_state leaves texture_cycle1 a byte
 * copy of texture -- it is only recompiled for the two-cycle tile pair -- so
 * sampling it returns the value texel0 already holds, and the fetch is pure
 * duplicate work. The LOD path is the exception: it selects tile1 independently
 * of tile0, so the two samples genuinely differ.
 */
static inline bool stage_texel1_separate(const rdp_primitive_state *primitive,
                                         bool uses_lod)
{
    const rdp_combine_state *color = &primitive->color;
    if (!color->needs_texel1) return false;
    if (color->next_texel && !color->two_cycle) return false;
    /* Without a tile-0 sample there is nothing to copy from. */
    if (!color->needs_texel0) return true;
    return color->two_cycle || (uses_lod && primitive->texture_lod);
}

typedef struct stage_combiner_inputs {
    sr_rgba shade;
    sr_rgba texel0;
    sr_rgba texel1;
    rdp_color primitive;
    rdp_color environment;
    rdp_color key_center;
    rdp_color key_scale;
    uint16_t lod_fraction;
    uint8_t primitive_lod_fraction;
    uint16_t k4;
    uint16_t k5;
    uint16_t noise;
} stage_combiner_inputs;

/* Running combiner value: 9-bit channels between cycles, before the clamp. */
typedef struct stage_combined { int32_t r, g, b, a; } stage_combined;

static SR_ALWAYS_INLINE uint16_t stage_combiner_noise(uint16_t noise)
{
    return (uint16_t)(((uint32_t)(noise & 7u) << 6) | 0x20u);
}

static SR_ALWAYS_INLINE int32_t stage_combiner_rgba(sr_rgba color, uint32_t component)
{
    return component == 0 ? color.r : component == 1 ? color.g : color.b;
}

static SR_ALWAYS_INLINE int32_t stage_combiner_color(rdp_color color, uint32_t component)
{
    return component == 0 ? color.r : component == 1 ? color.g : color.b;
}

static SR_ALWAYS_INLINE int32_t stage_combiner_source(rdp_combiner_source source,
                                                      const stage_combiner_inputs *in,
                                                      const stage_combined *combined,
                                                      uint32_t component)
{
    switch (source) {
    case RDP_COMBINER_COMBINED_RGB:     return component == 0 ? combined->r : component == 1 ? combined->g : combined->b;
    case RDP_COMBINER_COMBINED_ALPHA:   return combined->a;
    case RDP_COMBINER_TEXEL0_RGB:       return stage_combiner_rgba(in->texel0, component);
    case RDP_COMBINER_TEXEL0_ALPHA:     return in->texel0.a;
    case RDP_COMBINER_TEXEL1_RGB:       return stage_combiner_rgba(in->texel1, component);
    case RDP_COMBINER_TEXEL1_ALPHA:     return in->texel1.a;
    case RDP_COMBINER_PRIMITIVE_RGB:    return stage_combiner_color(in->primitive, component);
    case RDP_COMBINER_PRIMITIVE_ALPHA:  return in->primitive.a;
    case RDP_COMBINER_SHADE_RGB:        return stage_combiner_rgba(in->shade, component);
    case RDP_COMBINER_SHADE_ALPHA:      return in->shade.a;
    case RDP_COMBINER_ENVIRONMENT_RGB:  return stage_combiner_color(in->environment, component);
    case RDP_COMBINER_ENVIRONMENT_ALPHA:return in->environment.a;
    case RDP_COMBINER_KEY_CENTER:       return stage_combiner_color(in->key_center, component);
    case RDP_COMBINER_KEY_SCALE:        return stage_combiner_color(in->key_scale, component);
    case RDP_COMBINER_LOD_FRACTION:     return in->lod_fraction;
    case RDP_COMBINER_PRIMITIVE_LOD_FRACTION: return in->primitive_lod_fraction;
    case RDP_COMBINER_K4:               return in->k4;
    case RDP_COMBINER_K5:               return in->k5;
    case RDP_COMBINER_NOISE:            return in->noise;
    case RDP_COMBINER_ONE:              return 0x100;
    default:                            return 0;
    }
}

static SR_ALWAYS_INLINE int32_t stage_combiner_extend9(int32_t value)
{
    value &= 0x1ff;
    return (value & 0x180) == 0x180 ? value | ~0x1ff : value;
}

/*
 * The multiplier is the one combiner input the hardware sign-extends as an
 * ordinary nine-bit number, so bit 8 alone is its sign. The other three take
 * the rule above, which only goes negative once the top two bits are both set.
 *
 * The difference decides whether a product overflows upwards or downwards. A
 * combined value above 255 fed back as the multiplier is negative here and
 * positive under the other rule, and the clamp turns the first into black and
 * the second into white - so getting this wrong replaces a darkening pass with
 * a saturating bright one.
 */
static SR_ALWAYS_INLINE int32_t stage_combiner_extend_multiplier9(int32_t value)
{
    value &= 0x1ff;
    return (value ^ 0x100) - 0x100;
}

static SR_ALWAYS_INLINE int32_t stage_combiner_rgb_equation(int32_t a, int32_t b,
                                                            int32_t c, int32_t d)
{
    return ((stage_combiner_extend9(a) - stage_combiner_extend9(b)) *
            stage_combiner_extend_multiplier9(c) +
            stage_combiner_extend9(d) * 256 + 0x80) & 0x1ffff;
}

static SR_ALWAYS_INLINE int32_t stage_combiner_alpha_equation(int32_t a, int32_t b,
                                                              int32_t c, int32_t d)
{
    return (((stage_combiner_extend9(a) - stage_combiner_extend9(b)) *
             stage_combiner_extend_multiplier9(c) +
             stage_combiner_extend9(d) * 256 + 0x80) >> 8) & 0x1ff;
}

static SR_ALWAYS_INLINE uint8_t stage_combiner_clamp9(int32_t value)
{
    value &= 0x1ff;
    switch ((value >> 7) & 3) {
    case 0: case 1: return (uint8_t)(value & 0xff);
    case 2:         return 0xffu;
    default:        return 0u;
    }
}

static SR_ALWAYS_INLINE void stage_combine_cycle(const rdp_combiner_cycle *cycle,
                                                 const stage_combiner_inputs *inputs,
                                                 stage_combined *combined)
{
    stage_combined next;
    for (uint32_t component = 0; component < 3u; component++) {
        const int32_t raw = stage_combiner_rgb_equation(
            stage_combiner_source((rdp_combiner_source)cycle->rgb_a, inputs, combined, component),
            stage_combiner_source((rdp_combiner_source)cycle->rgb_b, inputs, combined, component),
            stage_combiner_source((rdp_combiner_source)cycle->rgb_c, inputs, combined, component),
            stage_combiner_source((rdp_combiner_source)cycle->rgb_d, inputs, combined, component));
        if (component == 0) next.r = (raw >> 8) & 0x1ff;
        else if (component == 1) next.g = (raw >> 8) & 0x1ff;
        else next.b = (raw >> 8) & 0x1ff;
    }
    next.a = stage_combiner_alpha_equation(
        stage_combiner_source((rdp_combiner_source)cycle->alpha_a, inputs, combined, 3u),
        stage_combiner_source((rdp_combiner_source)cycle->alpha_b, inputs, combined, 3u),
        stage_combiner_source((rdp_combiner_source)cycle->alpha_c, inputs, combined, 3u),
        stage_combiner_source((rdp_combiner_source)cycle->alpha_d, inputs, combined, 3u));
    *combined = next;
}

/*
 * The combiner inputs as a register file: one four-lane row per source, lanes
 * r, g, b and the value the alpha equation sees. A kernel fills the constant
 * rows once per span and rewrites only the per-pixel ones, so an operand is a
 * load instead of a switch. The RGB and alpha equations reduce to the same bit
 * expression, which lets all four lanes share one.
 */
enum { STAGE_COMBINER_SOURCES = RDP_COMBINER_K5 + 1 };

typedef struct stage_combiner_regs {
    int32_t v[STAGE_COMBINER_SOURCES][4];
} stage_combiner_regs;

/* One cycle's operand rows, a/b/c/d: lanes 0-2 use rgb, lane 3 alpha. */
typedef struct stage_combiner_ops {
    uint8_t rgb[4];
    uint8_t alpha[4];
} stage_combiner_ops;

static inline uint8_t stage_combiner_row(uint8_t source)
{
    return source < STAGE_COMBINER_SOURCES ? source : (uint8_t)RDP_COMBINER_ZERO;
}

static inline stage_combiner_ops stage_combiner_ops_prepare(const rdp_combiner_cycle *cycle)
{
    return (stage_combiner_ops){
        { stage_combiner_row(cycle->rgb_a), stage_combiner_row(cycle->rgb_b),
          stage_combiner_row(cycle->rgb_c), stage_combiner_row(cycle->rgb_d) },
        { stage_combiner_row(cycle->alpha_a), stage_combiner_row(cycle->alpha_b),
          stage_combiner_row(cycle->alpha_c), stage_combiner_row(cycle->alpha_d) }
    };
}

/* The rows a cycle reads, one bit per source. A row nobody reads need not be
 * kept current. */
static inline uint32_t stage_combiner_ops_reads(const stage_combiner_ops *ops)
{
    uint32_t reads = 0u;
    for (uint32_t i = 0; i < 4u; i++)
        reads |= (1u << ops->rgb[i]) | (1u << ops->alpha[i]);
    return reads;
}

/* An RGB source and the ALPHA source that follows it. */
#define STAGE_COMBINER_PAIR(rgb) (3u << (rgb))

/* Writes an RGB row and the ALPHA row that follows it in the source order. */
static SR_ALWAYS_INLINE void stage_combiner_set_rgba(stage_combiner_regs *regs,
                                                     rdp_combiner_source rgb,
                                                     sr_rgba color)
{
    int32_t *row = regs->v[rgb];
    row[0] = color.r; row[1] = color.g; row[2] = color.b; row[3] = color.b;
    row += 4;
    row[0] = color.a; row[1] = color.a; row[2] = color.a; row[3] = color.a;
}

static SR_ALWAYS_INLINE void stage_combiner_set_scalar(stage_combiner_regs *regs,
                                                       rdp_combiner_source source,
                                                       int32_t value)
{
    int32_t *row = regs->v[source];
    row[0] = value; row[1] = value; row[2] = value; row[3] = value;
}

/*
 * Every row as stage_combiner_source would produce it: RGB sources as r, g, b
 * and b again for the alpha lane, everything else in all four lanes. The
 * per-pixel rows start at zero.
 */
static inline void stage_combiner_regs_prepare(stage_combiner_regs *regs,
                                               const stage_combiner_inputs *inputs)
{
    __builtin_memset(regs, 0, sizeof(*regs));
    stage_combiner_set_scalar(regs, RDP_COMBINER_ONE, 0x100);
    stage_combiner_set_rgba(regs, RDP_COMBINER_PRIMITIVE_RGB,
                            sr_rgba_from_color(inputs->primitive));
    stage_combiner_set_rgba(regs, RDP_COMBINER_ENVIRONMENT_RGB,
                            sr_rgba_from_color(inputs->environment));
    stage_combiner_set_scalar(regs, RDP_COMBINER_LOD_FRACTION, inputs->lod_fraction);
    stage_combiner_set_scalar(regs, RDP_COMBINER_PRIMITIVE_LOD_FRACTION,
                              inputs->primitive_lod_fraction);
    stage_combiner_set_scalar(regs, RDP_COMBINER_NOISE, inputs->noise);
    const rdp_color center = inputs->key_center, scale = inputs->key_scale;
    int32_t *row = regs->v[RDP_COMBINER_KEY_CENTER];
    row[0] = center.r; row[1] = center.g; row[2] = center.b; row[3] = center.b;
    row = regs->v[RDP_COMBINER_KEY_SCALE];
    row[0] = scale.r; row[1] = scale.g; row[2] = scale.b; row[3] = scale.b;
    stage_combiner_set_scalar(regs, RDP_COMBINER_K4, inputs->k4);
    stage_combiner_set_scalar(regs, RDP_COMBINER_K5, inputs->k5);
    stage_combiner_set_rgba(regs, RDP_COMBINER_SHADE_RGB, inputs->shade);
    stage_combiner_set_rgba(regs, RDP_COMBINER_TEXEL0_RGB, inputs->texel0);
    stage_combiner_set_rgba(regs, RDP_COMBINER_TEXEL1_RGB, inputs->texel1);
}

static SR_ALWAYS_INLINE void stage_combiner_set_combined(stage_combiner_regs *regs,
                                                         stage_combined combined)
{
    int32_t *row = regs->v[RDP_COMBINER_COMBINED_RGB];
    row[0] = combined.r; row[1] = combined.g; row[2] = combined.b; row[3] = combined.b;
    row += 4;
    row[0] = combined.a; row[1] = combined.a; row[2] = combined.a; row[3] = combined.a;
}

/* The four lanes of one cycle run as one vector: the same operations as the
 * scalar equations above, lane for lane. */
typedef int32_t stage_v4 __attribute__((vector_size(16)));

static SR_ALWAYS_INLINE stage_v4 stage_combiner_operand(const stage_combiner_regs *regs,
                                                        uint8_t rgb, uint8_t alpha)
{
    stage_v4 value, alpha_row;
    __builtin_memcpy(&value, regs->v[rgb], sizeof(value));
    __builtin_memcpy(&alpha_row, regs->v[alpha], sizeof(alpha_row));
    value[3] = alpha_row[3];
    return value;
}

static SR_ALWAYS_INLINE stage_v4 stage_combiner_extend9_v4(stage_v4 value)
{
    value &= 0x1ff;
    const stage_v4 negative = (value & 0x180) == 0x180;
    return value | (negative & ~0x1ff);
}

static SR_ALWAYS_INLINE stage_v4 stage_combiner_extend_multiplier9_v4(stage_v4 value)
{
    return ((value & 0x1ff) ^ 0x100) - 0x100;
}

/* stage_combiner_clamp9 per lane, without its branches. */
static SR_ALWAYS_INLINE stage_v4 stage_combiner_clamp9_v4(stage_v4 value)
{
    value &= 0x1ff;
    const stage_v4 range = value >> 7;
    return ((value & 0xff) & (range < 2)) | (0xff & (range == 2));
}

static SR_ALWAYS_INLINE stage_v4 stage_combine_cycle_regs_v4(const stage_combiner_ops *ops,
                                                             const stage_combiner_regs *regs)
{
    const stage_v4 a = stage_combiner_extend9_v4(
        stage_combiner_operand(regs, ops->rgb[0], ops->alpha[0]));
    const stage_v4 b = stage_combiner_extend9_v4(
        stage_combiner_operand(regs, ops->rgb[1], ops->alpha[1]));
    const stage_v4 c = stage_combiner_extend_multiplier9_v4(
        stage_combiner_operand(regs, ops->rgb[2], ops->alpha[2]));
    const stage_v4 d = stage_combiner_extend9_v4(
        stage_combiner_operand(regs, ops->rgb[3], ops->alpha[3]));
    return (((a - b) * c + d * 256 + 0x80) >> 8) & 0x1ff;
}

static SR_ALWAYS_INLINE stage_combined stage_combine_cycle_regs(const stage_combiner_ops *ops,
                                                                const stage_combiner_regs *regs)
{
    const stage_v4 out = stage_combine_cycle_regs_v4(ops, regs);
    return (stage_combined){ out[0], out[1], out[2], out[3] };
}

/*
 * Chroma keying in the final cycle. The RGB accumulator (a - b) * c + d, before
 * the shift and clamp, is each channel's distance from the key centre; the key
 * alpha is the smallest (width << 4) - |distance|, clamped to 0..255, with a
 * positive distance whose low nibble is 8 rounded 16 closer. The colour is the
 * A input itself, clamped (the key bypass). Returns the key alpha and writes
 * the bypassed colour into pixel's RGB.
 */
static inline int32_t stage_chroma_key(const stage_combiner_ops *ops,
                                       const stage_combiner_regs *regs,
                                       const uint16_t width[3],
                                       rdp_color *pixel)
{
    const stage_v4 raw_a = stage_combiner_operand(regs, ops->rgb[0], ops->alpha[0]);
    const stage_v4 a = stage_combiner_extend9_v4(raw_a);
    const stage_v4 b = stage_combiner_extend9_v4(
        stage_combiner_operand(regs, ops->rgb[1], ops->alpha[1]));
    const stage_v4 c = stage_combiner_extend_multiplier9_v4(
        stage_combiner_operand(regs, ops->rgb[2], ops->alpha[2]));
    const stage_v4 d = stage_combiner_extend9_v4(
        stage_combiner_operand(regs, ops->rgb[3], ops->alpha[3]));
    const stage_v4 accumulator = ((a - b) * c + d * 256 + 0x80) & 0x1ffff;
    int32_t key = 0;
    for (uint32_t i = 0; i < 3u; i++) {
        int32_t distance = (accumulator[i] ^ 0x10000) - 0x10000;
        if (distance > 0)
            distance = (distance & 0xf) == 8 ? 0x10 - distance : -distance;
        const int32_t channel = ((int32_t)width[i] << 4) + distance;
        if (i == 0u || channel < key) key = channel;
    }
    pixel->r = stage_combiner_clamp9(raw_a[0]);
    pixel->g = stage_combiner_clamp9(raw_a[1]);
    pixel->b = stage_combiner_clamp9(raw_a[2]);
    return key < 0 ? 0 : key > 0xff ? 0xff : key;
}

#endif
