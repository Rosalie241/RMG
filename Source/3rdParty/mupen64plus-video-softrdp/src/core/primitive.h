#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include "rdp_commands.h"
#include "sr_scale.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum rdp_kernel_kind {
    RDP_KERNEL_INVALID = 0,
    RDP_KERNEL_TRIANGLE,
    RDP_KERNEL_TEXTURE_TRIANGLE_COPY,
    RDP_KERNEL_TEXTURE_RECTANGLE,
    RDP_KERNEL_TEXTURE_RECTANGLE_COPY
} rdp_kernel_kind;

typedef enum rdp_draw_stage {
    RDP_DRAW_TEXTURE       = 1u << 0,
    RDP_DRAW_DEPTH         = 1u << 1,
    RDP_DRAW_ALPHA_COMPARE = 1u << 2,
    RDP_DRAW_FILL          = 1u << 3,
    RDP_DRAW_LOD           = 1u << 4,
    /*
     * Run the depth stage after the alpha stage instead of before it. The depth
     * test's overflow term is the sum of pixel and memory coverage, and CVG_X_ALPHA
     * rewrites pixel coverage from the combiner's alpha - so on the hardware, which
     * evaluates the combiner first, the depth test sees the scaled value. Testing
     * early is worth the reordering everywhere else, because it rejects pixels
     * before the texture and combiner stages run; this flag marks the draws where
     * that would change the result.
     */
    RDP_DRAW_DEPTH_LATE    = 1u << 5
} rdp_draw_stage;

typedef enum rdp_texture_unit {
    RDP_TEXTURE_UNIT_NONE = 0,
    RDP_TEXTURE_UNIT_GENERIC,
    RDP_TEXTURE_UNIT_RGBA16_POINT,
    RDP_TEXTURE_UNIT_RGBA16_BILERP,
    /* The complete texture unit, for states the others cannot represent:
     * nine-bit texels (the K0-K3 conversion, raw YUV) and palettes read per
     * TMEM bank. Kept apart so the other units carry none of it. */
    RDP_TEXTURE_UNIT_FULL
} rdp_texture_unit;

/* Draw-constant execution metadata the kernels read once per span. */
typedef struct rdp_draw_plan {
    uint32_t stages;
    rdp_texture_unit texture_unit;
} rdp_draw_plan;

/*
 * A draw-local snapshot of register-derived state. The command processor owns
 * mutable RDP registers; span rendering only sees this immutable value.
 */
typedef struct rdp_primitive_state {
    rdp_framebuffer_state framebuffer;
    rdp_texture_sample_state texture;
    rdp_texture_sample_state texture_cycle1;
    uint8_t lod_base_tile;
    uint8_t lod_max_level;
    uint8_t lod_min_level;
    bool texture_lod;
    bool sharpen_lod;
    bool detail_lod;
    rdp_combine_state color;
    rdp_fragment_state fragment;
    const tmem_state *tmem;
    raster_decoded_triangle triangle;
    rdp_draw_plan plan;
    rdp_kernel_kind kernel;
    bool fill_mode;
    /* Kept last so non-LOD primitives do not clear this large cold storage. */
    rdp_texture_sample_state lod_textures[8];
    rdp_texture_sample_state lod_textures_cycle1[8];
} rdp_primitive_state;

void primitive_compile_framebuffer(rdp_framebuffer_state *framebuffer,
                                   const rdp_state *registers);

void primitive_compile_triangle(rdp_primitive_state *primitive,
                                const rdp_state *registers,
                                const tmem_state *tmem,
                                const raster_decoded_triangle *triangle,
                                bool fill_mode);

/* Cached variant: returns a pointer to thread-local storage valid until the
 * next call on this thread. Reuses the compiled material across triangles that
 * share state, refreshing only per-draw fields. Call primitive_cache_invalidate()
 * whenever compile-relevant state changes (any non-triangle command). */
const rdp_primitive_state *primitive_compile_triangle_cached(
    const rdp_state *registers,
    const tmem_state *tmem,
    const raster_decoded_triangle *triangle,
    bool fill_mode);

void primitive_cache_invalidate(void);

/* Invalidate every thread's compile cache. Call when a renderer context is
 * created so stale material from a previous context is never reused. */
void primitive_cache_reset(void);

void primitive_compile_rectangle(rdp_primitive_state *primitive,
                                 const rdp_state *registers,
                                 const tmem_state *tmem,
                                 uint32_t tile_index);
void primitive_compile_color_rectangle(rdp_primitive_state *primitive,
                                       const rdp_state *registers,
                                       const tmem_state *tmem);

void primitive_tile_bounds(const rdp_state *state, const tmem_state *tmem,
                           uint32_t tile_index, rdp_tile_bounds *bounds);

#ifdef __cplusplus
}
#endif

#endif
