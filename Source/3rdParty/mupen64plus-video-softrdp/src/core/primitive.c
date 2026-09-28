#include "primitive.h"

#include "tmem.h"

#include <stddef.h>
#include <string.h>

static uint8_t combiner_cycle_mask(const rdp_combiner_cycle *cycle)
{
    uint8_t mask = 0u;
    const uint8_t *sources = (const uint8_t *)cycle;
    for (uint32_t i = 0; i < sizeof(*cycle); i++) {
        switch ((rdp_combiner_source)sources[i]) {
        case RDP_COMBINER_TEXEL0_RGB:
        case RDP_COMBINER_TEXEL0_ALPHA: mask |= RDP_COMBINER_INPUT_TEXEL0; break;
        case RDP_COMBINER_TEXEL1_RGB:
        case RDP_COMBINER_TEXEL1_ALPHA: mask |= RDP_COMBINER_INPUT_TEXEL1; break;
        case RDP_COMBINER_SHADE_RGB:
        case RDP_COMBINER_SHADE_ALPHA: mask |= RDP_COMBINER_INPUT_SHADE; break;
        case RDP_COMBINER_LOD_FRACTION: mask |= RDP_COMBINER_INPUT_LOD_FRACTION; break;
        case RDP_COMBINER_NOISE: mask |= RDP_COMBINER_INPUT_NOISE; break;
        default: break;
        }
    }
    return mask;
}

static uint16_t normalize_depth_delta(uint32_t value)
{
    if (value >= 0x8000u) return 0x8000u;
    if (value == 0u) return 0u;
    uint32_t normalized = 1u;
    while (value >>= 1u) normalized <<= 1u;
    return (uint16_t)normalized;
}

static uint16_t compile_depth_delta(const rdp_depth_state *depth,
                                    const raster_decoded_triangle *triangle)
{
    uint32_t value;
    if (depth->source_primitive) {
        value = depth->primitive_delta_z;
    } else {
        /* Deliberately the hardware derivatives, not the per-raster-step ones
         * the interpolation uses. This value is the RDP's depth-comparison
         * tolerance and is stored in the frame buffer, so scaling it changes
         * how coplanar surfaces and decals resolve - for sample 0 as well,
         * which is supposed to reproduce the hardware-resolution result.
         * Measured on a real frame: scaling it doubles the deviation from the
         * scale-1 image (5.75% of pixels against 2.92%). */
        const uint32_t dx = (uint32_t)triangle->depth.dzdx >> 16;
        const uint32_t dy = (uint32_t)triangle->depth.dzdy >> 16;
        const uint32_t abs_dx = (dx & 0x8000u) ? (~dx & 0x7fffu) : dx;
        const uint32_t abs_dy = (dy & 0x8000u) ? (~dy & 0x7fffu) : dy;
        value = (abs_dx + abs_dy) & 0xffffu;
        if (value & 0xc000u) return 0x8000u;
        if (value == 0u) return 1u;
        if (value == 1u) return 3u;
        value <<= 1u;
    }
    return normalize_depth_delta(value);
}

/* Both fields the depth stage needs, so the exponent's loop runs once per
 * primitive instead of once per depth-tested pixel. */
static void compile_depth_delta_state(rdp_depth_state *depth,
                                      const raster_decoded_triangle *triangle)
{
    const uint16_t value = compile_depth_delta(depth, triangle);
    depth->pixel_delta = value;
    /* The stored delta exponent is that of the delta the comparison uses; a
     * primitive delta is not rescaled first.*/
    uint32_t exponent = 0u;
    for (uint32_t bits = value; bits >>= 1u;) exponent++;
    depth->delta_exponent = (uint8_t)exponent;
    depth->blend_shift = (uint8_t)((exponent < 11u ? 4u : 15u - exponent) << 4);
}

void primitive_tile_bounds(const rdp_state *state,
                                  const tmem_state *tmem,
                                  uint32_t tile_index,
                                  rdp_tile_bounds *bounds)
{
    if (!bounds) {
        return;
    }
    if (!state || tile_index >= 8) {
        memset(bounds, 0, sizeof(*bounds));
        return;
    }

    const rdp_tile *tile = &state->tiles[tile_index];
    (void)tmem;
    bounds->sl = 0u;
    bounds->tl = 0u;
    bounds->sh = ((tile->sh >> 2) - (tile->sl >> 2)) & 0x3ffu;
    bounds->th = ((tile->th >> 2) - (tile->tl >> 2)) & 0x3ffu;
}

void primitive_compile_framebuffer(rdp_framebuffer_state *framebuffer,
                                  const rdp_state *registers)
{
    if (!framebuffer || !registers) {
        return;
    }

    framebuffer->color_image = registers->color_image;
    framebuffer->fill_color = registers->fill_color;
    framebuffer->bytes_per_pixel = registers->color_image.size == RDP_SIZE_32BPP ? 4u :
                                   registers->color_image.size == RDP_SIZE_16BPP ? 2u : 1u;
    framebuffer->store_size =
        registers->color_image.size == RDP_SIZE_16BPP &&
        registers->color_image.format == RDP_FORMAT_IA
        ? RDP_STORE_IA16 : registers->color_image.size;
    framebuffer->color_image.address &= ~(uint32_t)(framebuffer->bytes_per_pixel - 1u);
}

static void primitive_compile_texture(rdp_texture_sample_state *texture,
                                     const rdp_state *registers,
                                     const tmem_state *tmem,
                                     uint32_t tile_index,
                                     bool second_cycle)
{
    memset(texture, 0, sizeof(*texture));
    texture->tile_index = (uint8_t)(tile_index & 7u);
    texture->tile = registers->tiles[texture->tile_index];
    texture->perspective = registers->other_modes.perspective;
    texture->tlut_enable = registers->other_modes.tlut_enable;
    texture->tlut_ia = registers->other_modes.tlut_ia;
    /* TLUT lookup is a texture-unit property, not a rectangle special case.
     * CI16/IA16-style encodings use the upper fetched byte as their index. */
    texture->tlut_wide_index = texture->tlut_enable &&
                               texture->tile.size >= RDP_SIZE_16BPP;
    texture->bilerp = second_cycle ? registers->other_modes.bilerp1
                                   : registers->other_modes.bilerp0;
    texture->sample_quad = registers->other_modes.sample_quad;
    texture->mid_texel = registers->other_modes.mid_texel;
    texture->convert_one = second_cycle && registers->other_modes.convert_one;
    /* A cycle that does not spend its filter on bilerp converts instead, in
     * every format; convert_one takes its input from the previous cycle. */
    texture->texel_output = !texture->bilerp && !texture->convert_one
        ? RDP_TEXEL_CONVERT
        : texture->tile.format == RDP_FORMAT_YUV ? RDP_TEXEL_YUV : RDP_TEXEL_PLAIN;
    texture->convert_k0_tf = registers->convert_k0_tf;
    texture->convert_k1_tf = registers->convert_k1_tf;
    texture->convert_k2_tf = registers->convert_k2_tf;
    texture->convert_k3_tf = registers->convert_k3_tf;
    primitive_tile_bounds(registers, tmem, texture->tile_index, &texture->bounds);
    uint32_t texture_width;
    uint32_t texture_height;
    uint32_t texture_stride;
    if (tmem_tile_sample_layout(tmem, texture, &texture_width,
                                &texture_height, &texture_stride)) {
        /* Masking addresses its period directly. Width and height are derived
         * only from the current render tile; loads do not create textures. */
        if (!texture->tile.clamp_s && texture->tile.mask_s) {
            const uint32_t period = 1u << texture->tile.mask_s;
            if (texture_width < period) texture_width = period;
        }
        if (!texture->tile.clamp_t && texture->tile.mask_t) {
            const uint32_t period = 1u << texture->tile.mask_t;
            if (texture_height < period) texture_height = period;
        }
        texture->width = (uint16_t)texture_width;
        texture->height = (uint16_t)texture_height;
        texture->stride = (uint16_t)texture_stride;
    }
    /* The other paletted samplers read one decoded copy of each entry, which
     * is exact only while the copies the draw may index are identical. */
    const bool tlut_banks = tmem && texture->tlut_enable &&
        texture->tile.format != RDP_FORMAT_YUV &&
        registers->other_modes.cycle_type != RDP_CYCLE_COPY &&
        (texture->tile.size == RDP_SIZE_4BPP
             ? ((tmem->palette_mixed >> (texture->tile.palette & 15u)) & 1u) != 0u
             : tmem->palette_mixed != 0u);
    if (tlut_banks) {
        texture->sampler_class = RDP_SAMPLER_TLUT_BANKS;
    } else if (texture->tile.format == RDP_FORMAT_RGBA &&
        texture->tile.size == RDP_SIZE_16BPP && !texture->tlut_enable) {
        texture->sampler_class = texture->bilerp && texture->sample_quad
            ? RDP_SAMPLER_RGBA16_BILERP : RDP_SAMPLER_RGBA16_POINT;
    } else if (!(texture->bilerp && texture->sample_quad) && texture->tlut_enable &&
               texture->tile.format != RDP_FORMAT_YUV &&
               (texture->tile.size == RDP_SIZE_4BPP ||
                texture->tile.size == RDP_SIZE_8BPP)) {
        texture->sampler_class = RDP_SAMPLER_TLUT_POINT;
    } else if (texture->bilerp && texture->sample_quad &&
               texture->tile.format == RDP_FORMAT_CI &&
               texture->tile.size == RDP_SIZE_4BPP && texture->tlut_enable) {
        texture->sampler_class = RDP_SAMPLER_CI4_TLUT_BILERP;
    } else if (texture->bilerp && texture->sample_quad &&
               texture->tile.format == RDP_FORMAT_I && texture->tile.size == RDP_SIZE_4BPP) {
        texture->sampler_class = RDP_SAMPLER_I4_BILERP;
    } else if (texture->bilerp && texture->sample_quad &&
               texture->tile.format != RDP_FORMAT_YUV &&
               texture->tile.size == RDP_SIZE_8BPP &&
               texture->tlut_enable) {
        texture->sampler_class = RDP_SAMPLER_CI8_TLUT_BILERP;
    } else if (texture->bilerp && texture->sample_quad &&
               texture->tile.format == RDP_FORMAT_I && texture->tile.size == RDP_SIZE_8BPP) {
        texture->sampler_class = RDP_SAMPLER_I8_BILERP;
    } else if (texture->bilerp && texture->sample_quad &&
               texture->tile.format == RDP_FORMAT_IA && texture->tile.size == RDP_SIZE_8BPP) {
        texture->sampler_class = RDP_SAMPLER_IA8_BILERP;
    } else if (texture->bilerp && texture->sample_quad && !texture->tlut_enable &&
               texture->tile.format == RDP_FORMAT_IA && texture->tile.size == RDP_SIZE_16BPP) {
        texture->sampler_class = RDP_SAMPLER_IA16_BILERP;
    }
    tmem_compile_axes(texture);
}

static void primitive_compile_common(rdp_primitive_state *primitive,
                                    const rdp_state *registers,
                                    const tmem_state *tmem,
                                    uint32_t tile_index)
{
    memset(primitive, 0, offsetof(rdp_primitive_state, lod_textures));
    primitive_compile_framebuffer(&primitive->framebuffer, registers);
    primitive_compile_texture(&primitive->texture, registers, tmem, tile_index, false);
    primitive->lod_base_tile = primitive->texture.tile_index;
    primitive->lod_min_level = registers->primitive_min_lod & 0x1fu;
    primitive->texture_lod = registers->other_modes.texture_lod;
    primitive->sharpen_lod = registers->other_modes.sharpen_lod;
    primitive->detail_lod = registers->other_modes.detail_lod;
    primitive->fragment.depth.image_address = registers->depth_image_address;
    primitive->fragment.depth.primitive_depth = registers->primitive_depth;
    primitive->fragment.depth.primitive_delta_z = registers->primitive_delta_z;
    primitive->fragment.depth.mode = registers->other_modes.z_mode;
    primitive->fragment.depth.compare = registers->other_modes.z_compare;
    primitive->fragment.depth.update = registers->other_modes.z_update;
    primitive->fragment.depth.source_primitive = registers->other_modes.z_source_primitive;
    primitive->color.program = registers->combiner;
    primitive->color.primitive_color = registers->primitive_color;
    primitive->color.environment_color = registers->environment_color;
    primitive->color.key_center = registers->key_center;
    primitive->color.key_scale = registers->key_scale;
    primitive->color.key_enable = registers->key_enable;
    for (uint32_t i = 0; i < 3u; i++)
        primitive->color.key_width[i] = registers->key_width[i];
    primitive->color.cycle_type = registers->other_modes.cycle_type;
    primitive->color.two_cycle = registers->other_modes.cycle_type == RDP_CYCLE_2;
    primitive->color.primitive_lod_fraction = registers->primitive_lod_fraction;
    const uint8_t cycle1_inputs =
        combiner_cycle_mask(&registers->combiner.cycle[1]);
    const uint8_t cycle0_inputs = primitive->color.two_cycle
        ? combiner_cycle_mask(&registers->combiner.cycle[0]) : 0u;
    const uint8_t active_combiner_inputs = cycle0_inputs | cycle1_inputs;
    /* Match ParallelRDP's two-cycle model: cycle 1 promotes the physical
     * tile-1 sample to TEXEL0 and aliases TEXEL1 back to physical TEXEL0. */
    primitive->color.needs_texel0 = primitive->color.two_cycle
        ? ((cycle0_inputs & RDP_COMBINER_INPUT_TEXEL0) != 0u ||
           (cycle1_inputs & RDP_COMBINER_INPUT_TEXEL1) != 0u)
        : (cycle1_inputs & RDP_COMBINER_INPUT_TEXEL0) != 0u;
    primitive->color.needs_texel1 = primitive->color.two_cycle
        ? ((cycle0_inputs & RDP_COMBINER_INPUT_TEXEL1) != 0u ||
           (cycle1_inputs & RDP_COMBINER_INPUT_TEXEL0) != 0u)
        : (cycle1_inputs & RDP_COMBINER_INPUT_TEXEL1) != 0u;
    /*
     * Cycle 1 of the two-cycle combiner does not read TEXEL1 from this pixel.
     * The hardware pipeline shifts the texel registers before evaluating it:
     * TEXEL0 takes the physical tile-1 sample of this pixel (modelled above)
     * and TEXEL1 takes the physical tile-0 sample of the NEXT pixel. One-cycle
     * programs that read TEXEL1 see the same successor sample.
     */
    primitive->color.next_texel =
        (cycle1_inputs & RDP_COMBINER_INPUT_TEXEL1) != 0u;
    if (primitive->color.next_texel)
        primitive->color.needs_texel0 = true;
    primitive->color.needs_lod_fraction =
        (active_combiner_inputs & RDP_COMBINER_INPUT_LOD_FRACTION) != 0;
    primitive->texture_cycle1 = primitive->texture;
    if (primitive->color.two_cycle && primitive->color.needs_texel1)
        primitive_compile_texture(&primitive->texture_cycle1, registers, tmem,
                                 (tile_index + 1u) & 7u, true);
    if (primitive->texture_cycle1.convert_one && primitive->color.needs_texel1)
        primitive->color.needs_texel0 = true;
    primitive->color.convert_k4 = registers->convert_k4;
    primitive->color.convert_k5 = registers->convert_k5;
    primitive->color.primitive_counter = registers->primitive_counter;
    primitive->fragment.blend.program = registers->blender;
    primitive->fragment.blend.fog_color = registers->fog_color;
    primitive->fragment.blend.blend_color = registers->blend_color;
    primitive->fragment.blend.cycle_type = registers->other_modes.cycle_type;
    primitive->fragment.blend.force_blend = registers->other_modes.force_blend;
    /* A 4bpp image reads back as black with full coverage - exactly the memory
     * pixel an unread framebuffer supplies - so its draws never read. */
    primitive->fragment.blend.image_read = registers->other_modes.image_read &&
        registers->color_image.size != RDP_SIZE_4BPP;
    primitive->fragment.blend.alpha_compare = registers->other_modes.alpha_compare;
    primitive->fragment.blend.alpha_compare_dither = registers->other_modes.alpha_compare_dither;
    primitive->fragment.blend.cycle_count = registers->other_modes.cycle_type == RDP_CYCLE_2 ? 2u : 1u;
    primitive->fragment.blend.final_cycle = registers->other_modes.cycle_type == RDP_CYCLE_2 ? 1u : 0u;
    primitive->fragment.blend.input_mask = registers->blender.input_mask[0];
    if (primitive->fragment.blend.cycle_count == 2u)
        primitive->fragment.blend.input_mask |= registers->blender.input_mask[1];
    primitive->fragment.alpha_cvg_select = registers->other_modes.alpha_cvg_select;
    primitive->fragment.cvg_times_alpha = registers->other_modes.cvg_times_alpha;
    primitive->fragment.antialias = registers->other_modes.antialias;
    primitive->fragment.color_on_cvg = registers->other_modes.color_on_cvg;
    primitive->fragment.coverage_dest = registers->other_modes.coverage_dest;
    primitive->fragment.rgb_dither = registers->other_modes.rgb_dither;
    primitive->fragment.alpha_dither = registers->other_modes.alpha_dither;
    /* An interlaced (field) scissor indexes the dither matrix with row bits
     * [2:1] instead of [1:0]; noise dither keeps the real coordinates. */
    primitive->fragment.dither_y_shift = registers->scissor_field ? 1u : 0u;
    /* Dither mode 2 is the noise mode for both channels, and a dithered alpha
     * compare draws its threshold from the same hash. Any of them makes the
     * kernel compute it. */
    primitive->color.needs_noise =
        (active_combiner_inputs & RDP_COMBINER_INPUT_NOISE) != 0u ||
        registers->other_modes.rgb_dither == 2u ||
        registers->other_modes.alpha_dither == 2u ||
        (registers->other_modes.alpha_compare &&
         registers->other_modes.alpha_compare_dither);
    primitive->tmem = tmem;
}

/* Copy fetches do not apply the ordinary tile clamp, and a mask wider than 10
 * bits acts as 10. An unmasked axis stays unmasked: a copy group's later
 * members step past texel 1023 and address on into TMEM, which wraps. Keep
 * this normalization in compilation so both copy renderers receive an
 * already-specialized, immutable texture state. */
static void primitive_compile_copy_texture(rdp_texture_sample_state *texture)
{
    if (!texture) return;
    texture->tile.clamp_s = 0u;
    texture->tile.clamp_t = 0u;
    if (texture->tile.mask_s > 10u) texture->tile.mask_s = 10u;
    if (texture->tile.mask_t > 10u) texture->tile.mask_t = 10u;
    if (texture->tile.mask_s) texture->width = (uint16_t)(1u << texture->tile.mask_s);
    if (texture->tile.mask_t) texture->height = (uint16_t)(1u << texture->tile.mask_t);
    tmem_compile_axes(texture);
}

/* Whether a texture state needs the full texture unit: nine-bit texels, or a
 * palette read per TMEM bank. */
static bool texture_needs_full_unit(const rdp_texture_sample_state *texture)
{
    return texture->texel_output != RDP_TEXEL_PLAIN ||
           texture->sampler_class == RDP_SAMPLER_TLUT_BANKS;
}

/* Whether any texture state the kernel may sample needs the full texture unit.
 * lod_tiles counts the LOD tiles compiled from lod_base_tile on; the others
 * hold whatever an earlier primitive left there. */
static bool primitive_needs_full_unit(const rdp_primitive_state *primitive,
                                      uint32_t lod_tiles)
{
    if (texture_needs_full_unit(&primitive->texture) ||
        texture_needs_full_unit(&primitive->texture_cycle1))
        return true;
    for (uint32_t offset = 0; offset < lod_tiles; offset++) {
        const uint32_t tile = (primitive->lod_base_tile + offset) & 7u;
        if ((primitive->color.needs_texel0 &&
             texture_needs_full_unit(&primitive->lod_textures[tile])) ||
            (primitive->color.needs_texel1 &&
             texture_needs_full_unit(&primitive->lod_textures_cycle1[tile])))
            return true;
    }
    return false;
}

static void primitive_compile_plan(rdp_primitive_state *primitive,
                                        bool has_texture,
                                        bool has_shade,
                                        bool has_depth,
                                        uint32_t lod_tiles)
{
    rdp_draw_plan *plan = &primitive->plan;
    const rdp_depth_state *depth = &primitive->fragment.depth;
    plan->stages = 0u;
    /* The shifts reach the result only through the memory-alpha B factor of a
     * cycle that runs: the first of two always, the final one only when
     * blending can be enabled at all. */
    const rdp_blend_state *blend = &primitive->fragment.blend;
    const bool two_cycle_blend = blend->cycle_count == 2u;
    primitive->fragment.depth.blend_shift_live =
        (two_cycle_blend && (blend->program.cycle[0].factor_b & 3u) == 1u) ||
        ((blend->force_blend || primitive->fragment.antialias) &&
         (blend->program.cycle[two_cycle_blend ? 1u : 0u].factor_b & 3u) == 1u);
    /* Address 0 is an ordinary depth image: the RDP has no "unset" value, and
     * a depth image aliased onto the color image at 0 is drawn through. */
    if ((depth->compare || depth->update) &&
        (has_depth || depth->source_primitive)) {
        plan->stages |= RDP_DRAW_DEPTH;
        /* Only a compare consults coverage, and only z-modes 0 and 1 consult the
         * overflow it produces, so every other depth draw keeps the early test. */
        if (depth->compare && primitive->fragment.cvg_times_alpha &&
            (depth->mode & 3u) < 2u)
            plan->stages |= RDP_DRAW_DEPTH_LATE;
    }
    /* The command payload only controls whether texture coordinates are
     * supplied. The texture unit is enabled by combiner demand; untextured
     * triangles can still sample with the zero coordinates from span setup. */
    if ((has_texture && primitive->color.cycle_type == RDP_CYCLE_COPY) ||
        ((has_texture || has_shade) &&
         (primitive->color.needs_texel0 || primitive->color.needs_texel1)) ||
        (has_texture && primitive->color.needs_lod_fraction)) {
        plan->stages |= RDP_DRAW_TEXTURE;
        plan->texture_unit =
            primitive->color.cycle_type != RDP_CYCLE_COPY &&
            primitive_needs_full_unit(primitive, lod_tiles)
            ? RDP_TEXTURE_UNIT_FULL
            : primitive->texture.sampler_class == RDP_SAMPLER_RGBA16_BILERP
            ? RDP_TEXTURE_UNIT_RGBA16_BILERP
            : primitive->texture.sampler_class == RDP_SAMPLER_RGBA16_POINT
                ? RDP_TEXTURE_UNIT_RGBA16_POINT : RDP_TEXTURE_UNIT_GENERIC;
    } else {
        plan->texture_unit = RDP_TEXTURE_UNIT_NONE;
    }
    if ((plan->stages & RDP_DRAW_TEXTURE) &&
        (primitive->texture_lod || primitive->sharpen_lod || primitive->detail_lod ||
         primitive->color.needs_lod_fraction))
        plan->stages |= RDP_DRAW_LOD;
    if (primitive->fragment.blend.alpha_compare)
        plan->stages |= RDP_DRAW_ALPHA_COMPARE;
    if (primitive->fill_mode)
        plan->stages |= RDP_DRAW_FILL;
}

void primitive_compile_triangle(rdp_primitive_state *primitive,
                               const rdp_state *registers,
                               const tmem_state *tmem,
                               const raster_decoded_triangle *triangle,
                               bool fill_mode)
{
    if (!primitive || !registers || !triangle) {
        return;
    }

    primitive_compile_common(primitive,
                            registers,
                            tmem,
                            triangle->position.tile);
    primitive->triangle = *triangle;
    primitive->lod_max_level = triangle->position.max_level;
    uint32_t tile_count = 0u;
    if (triangle->has_texture && (primitive->texture_lod || primitive->sharpen_lod ||
                                  primitive->detail_lod ||
                                  primitive->color.needs_lod_fraction)) {
        tile_count = primitive->texture_lod
            ? (uint32_t)primitive->lod_max_level + 2u : 1u;
        if (tile_count > 8u) tile_count = 8u;
        for (uint32_t offset = 0; offset < tile_count; offset++) {
            const uint32_t tile = (primitive->lod_base_tile + offset) & 7u;
            if (primitive->color.needs_texel0)
                primitive_compile_texture(&primitive->lod_textures[tile], registers, tmem, tile, false);
            if (primitive->color.needs_texel1)
                primitive_compile_texture(&primitive->lod_textures_cycle1[tile], registers, tmem, tile, true);
        }
    }
    compile_depth_delta_state(&primitive->fragment.depth, triangle);
    /* FILL_TRIANGLE describes the triangle payload (no shade/texture/depth),
     * not an unconditional framebuffer fill. In one/two-cycle mode it still
     * runs through the configured combiner and blender. */
    primitive->fill_mode = fill_mode &&
                           registers->other_modes.cycle_type == RDP_CYCLE_FILL;
    const bool copy = registers->other_modes.cycle_type == RDP_CYCLE_COPY;
    primitive->kernel = copy ? RDP_KERNEL_TEXTURE_TRIANGLE_COPY
                                  : RDP_KERNEL_TRIANGLE;
    if (copy) primitive_compile_copy_texture(&primitive->texture);
    primitive_compile_plan(primitive, triangle->has_texture,
                                triangle->has_shade, triangle->has_depth, tile_count);
}

static _Thread_local rdp_primitive_state cache_primitive;
static _Thread_local uint32_t cache_tile;
static _Thread_local uint8_t cache_max_level;
static _Thread_local bool cache_has_texture;
static _Thread_local bool cache_has_shade;
static _Thread_local bool cache_fill_mode;
static _Thread_local bool cache_valid;
static _Thread_local bool cache_dirty;
static _Atomic uint32_t cache_generation = 1u;
static _Thread_local uint32_t cache_seen_generation;

void primitive_cache_invalidate(void)
{
    cache_dirty = true;
}

void primitive_cache_reset(void)
{
    cache_generation++;
    cache_dirty = true;
}

const rdp_primitive_state *primitive_compile_triangle_cached(
    const rdp_state *registers,
    const tmem_state *tmem,
    const raster_decoded_triangle *triangle,
    bool fill_mode)
{
    if (cache_seen_generation != cache_generation) {
        cache_seen_generation = cache_generation;
        cache_valid = false;
    }

    const bool hit = cache_valid && !cache_dirty &&
        cache_tile == triangle->position.tile &&
        cache_max_level == triangle->position.max_level &&
        cache_has_texture == triangle->has_texture &&
        cache_has_shade == triangle->has_shade &&
        cache_fill_mode == fill_mode;

    if (!hit) {
        primitive_compile_triangle(&cache_primitive, registers, tmem, triangle, fill_mode);
        cache_valid = true;
        cache_dirty = false;
        cache_tile = triangle->position.tile;
        cache_max_level = triangle->position.max_level;
        cache_has_texture = triangle->has_texture;
        cache_has_shade = triangle->has_shade;
        cache_fill_mode = fill_mode;
        return &cache_primitive;
    }

    /* Material is still valid; refresh only the per-draw fields. */
    cache_primitive.triangle = *triangle;
    cache_primitive.lod_max_level = triangle->position.max_level;
    cache_primitive.color.primitive_counter = registers->primitive_counter;
    compile_depth_delta_state(&cache_primitive.fragment.depth, triangle);
    return &cache_primitive;
}

/* A rectangle has no z slopes. Its depth delta - the stored dz, the compare
 * tolerance and the blend shift - comes from the primitive depth, or is the
 * smallest delta, exactly as for a triangle whose slopes are zero. */
static const raster_decoded_triangle rectangle_depth_slopes;

void primitive_compile_rectangle(rdp_primitive_state *primitive,
                                const rdp_state *registers,
                                const tmem_state *tmem,
                                uint32_t tile_index)
{
    if (!primitive || !registers) {
        return;
    }

    primitive_compile_common(primitive, registers, tmem, tile_index);
    compile_depth_delta_state(&primitive->fragment.depth, &rectangle_depth_slopes);
    const bool copy = registers->other_modes.cycle_type == RDP_CYCLE_COPY;
    primitive->kernel = copy ? RDP_KERNEL_TEXTURE_RECTANGLE_COPY
                                  : RDP_KERNEL_TEXTURE_RECTANGLE;
    if (copy) primitive_compile_copy_texture(&primitive->texture);
    primitive_compile_plan(primitive, true, false, false, 0u);
}

void primitive_compile_color_rectangle(rdp_primitive_state *primitive,
                                      const rdp_state *registers,
                                      const tmem_state *tmem)
{
    if (!primitive || !registers) return;
    primitive_compile_common(primitive, registers, tmem, 0u);
    compile_depth_delta_state(&primitive->fragment.depth, &rectangle_depth_slopes);
    primitive->kernel = RDP_KERNEL_TEXTURE_RECTANGLE;
    primitive_compile_plan(primitive, false, false, false, 0u);
}
