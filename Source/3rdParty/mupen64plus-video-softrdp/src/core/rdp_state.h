#ifndef RDP_STATE_H
#define RDP_STATE_H

#include "sr_defs.h"

typedef enum rdp_cycle_type {
    RDP_CYCLE_1 = 0,
    RDP_CYCLE_2 = 1,
    RDP_CYCLE_COPY = 2,
    RDP_CYCLE_FILL = 3
} rdp_cycle_type;

typedef enum rdp_texture_format {
    RDP_FORMAT_RGBA = 0,
    RDP_FORMAT_YUV = 1,
    RDP_FORMAT_CI = 2,
    RDP_FORMAT_IA = 3,
    RDP_FORMAT_I = 4
} rdp_texture_format;

typedef enum rdp_texture_size {
    RDP_SIZE_4BPP = 0,
    RDP_SIZE_8BPP = 1,
    RDP_SIZE_16BPP = 2,
    RDP_SIZE_32BPP = 3
} rdp_texture_size;

typedef struct rdp_color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} rdp_color;

typedef struct rdp_image {
    uint32_t address;
    uint32_t width;
    rdp_texture_format format;
    rdp_texture_size size;
} rdp_image;

typedef struct rdp_tile {
    uint16_t sl;
    uint16_t tl;
    uint16_t sh;
    uint16_t th;
    uint16_t tmem;
    uint16_t line;
    uint8_t palette;
    uint8_t clamp_s;
    uint8_t clamp_t;
    uint8_t mirror_s;
    uint8_t mirror_t;
    uint8_t mask_s;
    uint8_t mask_t;
    uint8_t shift_s;
    uint8_t shift_t;
    rdp_texture_format format;
    rdp_texture_size size;
} rdp_tile;

typedef struct rdp_other_modes {
    rdp_cycle_type cycle_type;
    bool perspective;
    bool texture_lod;
    bool sharpen_lod;
    bool detail_lod;
    bool tlut_enable;
    bool tlut_ia;
    bool sample_quad;
    bool mid_texel;
    bool bilerp0;
    bool bilerp1;
    bool convert_one;
    bool force_blend;
    bool alpha_cvg_select;
    bool cvg_times_alpha;
    bool color_on_cvg;
    bool image_read;
    bool z_update;
    bool z_compare;
    bool z_source_primitive;
    bool antialias;
    bool alpha_compare;
    bool alpha_compare_dither;
    uint8_t coverage_dest;
    uint8_t z_mode;
    uint8_t rgb_dither;
    uint8_t alpha_dither;
    /* Stalls the command processor per primitive (raster_submit_fill_rectangle). */
    bool atomic_prim;
} rdp_other_modes;

typedef enum rdp_combiner_source {
    RDP_COMBINER_ZERO = 0,
    RDP_COMBINER_ONE,
    RDP_COMBINER_COMBINED_RGB,
    RDP_COMBINER_COMBINED_ALPHA,
    RDP_COMBINER_TEXEL0_RGB,
    RDP_COMBINER_TEXEL0_ALPHA,
    RDP_COMBINER_TEXEL1_RGB,
    RDP_COMBINER_TEXEL1_ALPHA,
    RDP_COMBINER_PRIMITIVE_RGB,
    RDP_COMBINER_PRIMITIVE_ALPHA,
    RDP_COMBINER_SHADE_RGB,
    RDP_COMBINER_SHADE_ALPHA,
    RDP_COMBINER_ENVIRONMENT_RGB,
    RDP_COMBINER_ENVIRONMENT_ALPHA,
    RDP_COMBINER_LOD_FRACTION,
    RDP_COMBINER_PRIMITIVE_LOD_FRACTION,
    RDP_COMBINER_NOISE,
    RDP_COMBINER_KEY_CENTER,
    RDP_COMBINER_KEY_SCALE,
    RDP_COMBINER_K4,
    RDP_COMBINER_K5
} rdp_combiner_source;

typedef struct rdp_combiner_cycle {
    uint8_t rgb_a, rgb_b, rgb_c, rgb_d;
    uint8_t alpha_a, alpha_b, alpha_c, alpha_d;
} rdp_combiner_cycle;

enum {
    RDP_COMBINER_INPUT_TEXEL0 = 1u << 0,
    RDP_COMBINER_INPUT_TEXEL1 = 1u << 1,
    RDP_COMBINER_INPUT_SHADE = 1u << 2,
    RDP_COMBINER_INPUT_LOD_FRACTION = 1u << 3,
    RDP_COMBINER_INPUT_NOISE = 1u << 4
};

typedef struct rdp_combiner_program {
    rdp_combiner_cycle cycle[2];
    uint8_t input_mask;
} rdp_combiner_program;

typedef struct rdp_blender_cycle {
    uint8_t color_a, factor_a, color_b, factor_b;
} rdp_blender_cycle;

typedef enum rdp_blender_op {
    RDP_BLEND_OP_GENERIC = 0,
    RDP_BLEND_OP_FOG_SHADE_ALPHA,
    RDP_BLEND_OP_FOG_ALPHA,
    RDP_BLEND_OP_FOG_CONSTANT_ALPHA,
    /* pixel * 0 + pixel * 1 */
    RDP_BLEND_OP_PIXEL_PASSTHROUGH,
    /* pixel * pixel_alpha + memory * (1 - pixel_alpha) */
    RDP_BLEND_OP_PIXEL_OVER_MEMORY,
    /* pixel * pixel_alpha + memory * memory_alpha */
    RDP_BLEND_OP_PIXEL_MEMORY_ALPHA
} rdp_blender_op;

enum {
    RDP_BLENDER_INPUT_MEMORY      = 1u << 0,
    RDP_BLENDER_INPUT_FOG         = 1u << 1,
    RDP_BLENDER_INPUT_SHADE_ALPHA = 1u << 2
};

typedef struct rdp_blender_program {
    rdp_blender_cycle cycle[2];
    uint8_t operation[2];
    uint8_t input_mask[2];
} rdp_blender_program;

typedef struct rdp_tile_bounds {
    uint32_t sl;
    uint32_t tl;
    uint32_t sh;
    uint32_t th;
} rdp_tile_bounds;

/* Two rows of up to 33 pixels at up to 2x2 samples. */
#define RDP_RECT_STALE_MAX 264u

typedef struct rdp_rect_stale {
    uint32_t words[2];
    uint32_t count;
    bool valid;
    uint16_t pre[RDP_RECT_STALE_MAX];
} rdp_rect_stale;

typedef struct rdp_state {
    rdp_image color_image;
    rdp_image texture_image;
    uint32_t depth_image_address;
    uint32_t fill_color;
    rdp_color fog_color;
    rdp_color blend_color;
    rdp_color primitive_color;
    rdp_color environment_color;
    uint8_t primitive_min_lod;
    uint8_t primitive_lod_fraction;
    uint16_t primitive_depth;
    uint16_t primitive_delta_z;
    uint16_t scissor_x0;
    uint16_t scissor_y0;
    uint16_t scissor_x1;
    uint16_t scissor_y1;
    bool scissor_field;
    bool scissor_keep_odd;
    rdp_other_modes other_modes;
    rdp_combiner_program combiner;
    rdp_blender_program blender;
    rdp_tile tiles[8];
    int32_t convert_k0_tf;
    int32_t convert_k1_tf;
    int32_t convert_k2_tf;
    int32_t convert_k3_tf;
    int32_t convert_k4;
    int32_t convert_k5;
    rdp_color key_center;
    rdp_color key_scale;
    /* Monotonic per-primitive index. Feeds the noise generator so overlapping
     * and successive draws get independent noise, and it advances across frames
     * on its own. */
    uint32_t primitive_counter;
    /* Scanline-parallel worker assignment. A worker renders scanline y when
     * (y - yh) % worker_stride == worker_offset. Default stride 1 / offset 0
     * renders every scanline (single-threaded). Set per worker before replay. */
    uint32_t worker_offset;
    uint32_t worker_stride;
    /* Chroma key: SET_KEY_R/GB's 12-bit widths and SET_OTHER_MODES bit 8. */
    uint16_t key_width[3];
    bool key_enable;
    /* Span-buffer stale read (raster_submit_fill_rectangle): the last eligible
     * Fill_Rect's command words and the framebuffer samples this worker drew
     * for it, as they were before it ran. Any other command clears it. */
    rdp_rect_stale rect_stale;
} rdp_state;

/*
 * Stateless per-pixel noise. Hashing (x, y, primitive) instead of advancing a
 * running seed keeps noise independent of evaluation order, so span rendering
 * can be split across threads without changing the result. The three-way mix
 * decorrelates all three inputs.
 */
static inline uint16_t rdp_pixel_noise(uint32_t x, uint32_t y, uint32_t primitive)
{
    const uint32_t prime = 1103515245u;
    uint32_t a = x, b = y, c = primitive;
    for (int round = 0; round < 3; round++) {
        /* Update all three from the current values, not sequentially: each mixes
         * with the next input, so a coordinate that starts in c still reaches a
         * over the three rounds. Overwriting a before c reads it would drop the
         * x dependence entirely (x >> 8 is 0 on screen) and leave only stripes. */
        const uint32_t na = ((a >> 8) ^ b) * prime;
        const uint32_t nb = ((b >> 8) ^ c) * prime;
        const uint32_t nc = ((c >> 8) ^ a) * prime;
        a = na;
        b = nb;
        c = nc;
    }
    return (uint16_t)(a >> 16);
}

/* Draw-local snapshots consumed by individual software pipeline stages. */
/* A colour image's stored layout where it differs from its size alone: 16bpp IA
 * keeps intensity and coverage, not RGBA5551. */
#define RDP_STORE_IA16 ((rdp_texture_size)4)

typedef struct rdp_framebuffer_state {
    rdp_image color_image;
    uint32_t fill_color;
    uint8_t bytes_per_pixel;
    /* color_image.size, or RDP_STORE_IA16: the layout pixel writes and reads
     * use, decided once per draw. */
    rdp_texture_size store_size;
} rdp_framebuffer_state;

typedef enum rdp_sampler_class {
    RDP_SAMPLER_GENERIC = 0,
    RDP_SAMPLER_RGBA16_POINT,
    RDP_SAMPLER_RGBA16_BILERP,
    RDP_SAMPLER_I4_BILERP,
    RDP_SAMPLER_CI4_TLUT_BILERP,
    RDP_SAMPLER_CI8_TLUT_BILERP,
    RDP_SAMPLER_I8_BILERP,
    RDP_SAMPLER_IA8_BILERP,
    /* Point-sampled 4/8bpp through the palette, any non-YUV format. */
    RDP_SAMPLER_TLUT_POINT,
    RDP_SAMPLER_IA16_BILERP,
    /* Paletted, with every tap reading its own TMEM bank's palette copy:
     * the copies of an entry the draw may index differ. */
    RDP_SAMPLER_TLUT_BANKS
} rdp_sampler_class;

/* One texture axis reduced to what the samplers need per tap; built by
 * tmem_compile_axes from the tile, bounds and extent. */
typedef struct rdp_texture_axis {
    int32_t origin;
    int32_t clamp_limit;
    int32_t clamp_offset;
    uint32_t extent;
    int32_t repeat;
    int32_t mirror_bit;
    int32_t mirror_fold;
    uint8_t shift;
    bool clamp;
} rdp_texture_axis;

/* What the texture unit hands the combiner, decided per draw. */
typedef enum rdp_texel_output {
    RDP_TEXEL_PLAIN = 0,  /* the fetched eight bits, unchanged */
    RDP_TEXEL_YUV,        /* raw YUV: U and V are signed */
    RDP_TEXEL_CONVERT     /* bi_lerp clear: the K0-K3 colour conversion */
} rdp_texel_output;

typedef struct rdp_texture_sample_state {
    rdp_tile tile;
    rdp_tile_bounds bounds;
    uint16_t width;
    uint16_t height;
    uint16_t stride;
    uint8_t tile_index;
    rdp_sampler_class sampler_class;
    bool perspective;
    bool tlut_enable;
    bool tlut_ia;
    bool tlut_wide_index;
    bool bilerp;
    bool sample_quad;
    bool mid_texel;
    bool convert_one;
    /* An rdp_texel_output, stored in the byte the per-pixel struct has room for. */
    uint8_t texel_output;
    int32_t convert_k0_tf;
    int32_t convert_k1_tf;
    int32_t convert_k2_tf;
    int32_t convert_k3_tf;
    /* S and T. Valid only when axes_ready; a hand-built state without them
     * takes the reference path. */
    rdp_texture_axis axis[2];
    bool axes_ready;
} rdp_texture_sample_state;

typedef struct rdp_depth_state {
    uint32_t image_address;
    uint16_t primitive_depth;
    uint16_t primitive_delta_z;
    uint16_t pixel_delta;
    /* Index of pixel_delta's highest set bit: the encoded delta Z. Bits
     * 3:2 are stored in the depth halfword's low bits, bits 1:0 in the hidden
     * plane. Compiled per primitive because it is one per draw, not per pixel. */
    uint8_t delta_exponent;
    /* The blender's memory-alpha factor shifts without a depth compare, A in
     * bits 3:0 and B in bits 7:4. A compare replaces them per pixel. */
    uint8_t blend_shift;
    /* Whether an evaluated blender cycle uses memory alpha as its B factor,
     * the only case in which the shifts matter. */
    bool blend_shift_live;
    uint8_t mode;
    bool compare;
    bool update;
    bool source_primitive;
} rdp_depth_state;

typedef struct rdp_combine_state {
    rdp_combiner_program program;
    rdp_color primitive_color;
    rdp_color environment_color;
    rdp_color key_center;
    rdp_color key_scale;
    /* Chroma keying in the final cycle (stage_chroma_key). */
    uint16_t key_width[3];
    bool key_enable;
    rdp_cycle_type cycle_type;
    bool two_cycle;
    uint8_t primitive_lod_fraction;
    bool needs_texel0;
    bool needs_texel1;
    /* The final combiner cycle reads TEXEL1, which the pipeline delivers as the
     * next pixel's tile-0 sample. */
    bool next_texel;
    bool needs_lod_fraction;
    /* Covers every consumer of the per-pixel hash, not just the combiner input:
     * the dither modes of the alpha and write stages read the same value. */
    bool needs_noise;
    int32_t convert_k4;
    int32_t convert_k5;
    uint32_t primitive_counter;
} rdp_combine_state;

typedef struct rdp_blend_state {
    rdp_blender_program program;
    rdp_color fog_color;
    rdp_color blend_color;
    rdp_cycle_type cycle_type;
    uint8_t input_mask;
    bool force_blend;
    bool image_read;
    bool alpha_compare;
    bool alpha_compare_dither;
    uint8_t cycle_count;
    uint8_t final_cycle;
} rdp_blend_state;

typedef struct rdp_fragment_state {
    rdp_blend_state blend;
    rdp_depth_state depth;
    bool alpha_cvg_select;
    bool cvg_times_alpha;
    bool antialias;
    bool color_on_cvg;
    uint8_t coverage_dest;
    uint8_t rgb_dither;
    uint8_t alpha_dither;
    /* Right shift of the row before it indexes the dither matrix: 1 under an
     * interlaced (field) scissor, which dithers on row bits [2:1]. */
    uint8_t dither_y_shift;
} rdp_fragment_state;

static inline uint8_t expand_5_to_8(uint32_t value)
{
    value &= 0x1fu;
    return (uint8_t)((value << 3) | (value >> 2));
}

static inline uint8_t shrink_8_to_5(uint8_t value)
{
    return (uint8_t)(value >> 3);
}

static inline rdp_color rdp_color_from_rgba5551(uint16_t value)
{
    return (rdp_color){ expand_5_to_8(value >> 11), expand_5_to_8(value >> 6), expand_5_to_8(value >> 1), (value & 1u) ? 255u : 0u };
}

static inline uint16_t rdp_color_to_rgba5551(rdp_color color)
{
    return (uint16_t)(((uint16_t)shrink_8_to_5(color.r) << 11) |
                      ((uint16_t)shrink_8_to_5(color.g) << 6) |
                      ((uint16_t)shrink_8_to_5(color.b) << 1) |
                      (color.a >= 128 ? 1u : 0u));
}

static inline uint32_t rdp_color_to_rgba8888(rdp_color color)
{
    return ((uint32_t)color.r << 24) | ((uint32_t)color.g << 16) | ((uint32_t)color.b << 8) | (uint32_t)color.a;
}

#endif
