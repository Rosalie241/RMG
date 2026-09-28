#ifndef TMEM_H
#define TMEM_H

#include "rdp_state.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct rdp_command rdp_command;
typedef struct sr_memory sr_memory;

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "SoftRDP TMEM requires a little-endian host"
#endif

/* Physical 4 KiB SRAM with native views for the texture hot paths. */
typedef struct tmem_state {
    union {
        _Alignas(64) uint8_t bytes[SR_TMEM_SIZE];
        uint16_t words[SR_TMEM_SIZE / 2u];
        uint32_t dwords[SR_TMEM_SIZE / 4u];
        uint64_t qwords[SR_TMEM_SIZE / 8u];
    };
    /* The 256 palette entries of upper TMEM as their first copy holds them,
     * decoded as RGBA5551 [0] and as IA88 [1]: the dense table the one-copy
     * samplers read. palette_lanes decodes every halfword as RGBA5551, so
     * entry i's four TMEM bank copies are lanes 4i..4i+3 there, for the banked
     * sampler. palette_known holds each upper-TMEM word as it was last
     * decoded, so tmem_palette_refresh decodes only the entries a write
     * changed. The snapshot stores only the bytes and rebuilds these on
     * load. */
    rdp_color palette[2][256];
    rdp_color palette_lanes[1024];
    uint64_t palette_known[256];
    /* Bit b is set when an entry of palette block b (entries 16b..16b+15)
     * differs between its four TMEM bank copies. LoadTLUT writes the copies
     * identically; any other write to upper TMEM need not. The decoded
     * palettes above hold one copy, so only unmixed blocks may use them. */
    uint16_t palette_mixed;
    /*
     * Pads the state to a whole number of 4 KiB pages. The renderer context
     * holds one TMEM for itself and one per worker, and everything after them
     * would otherwise move relative to the RDP state by a non-multiple of 4 KiB
     * whenever this struct changes size. The per-command path stores to the
     * debug block and then loads RDP state, and a load whose address matches a
     * preceding store in the low 12 bits stalls on this CPU family: a 2 KiB
     * shift measurably slowed command-bound dumps by up to 20%.
     */
    uint8_t page_pad[3u * SR_TMEM_SIZE - sizeof(rdp_color[2][256]) -
                     sizeof(rdp_color[1024]) - sizeof(uint64_t[256]) -
                     sizeof(uint16_t)];
} tmem_state;

_Static_assert(sizeof(tmem_state) % 4096u == 0u,
               "tmem_state keeps the context layout stable modulo 4 KiB");

typedef struct tmem_texel_address {
    uint32_t byte;
    uint32_t byte2;
    uint8_t subtexel;
    uint8_t bytes;
} tmem_texel_address;

extern rdp_color tmem_rgba16_decode_table[UINT16_MAX + 1u];

void tmem_init(tmem_state *tmem);
/* Re-decodes the palette entries after TMEM was written directly. */
void tmem_palette_refresh(tmem_state *tmem);
sr_result tmem_load_tile(tmem_state *tmem, sr_memory *memory, rdp_state *state, const rdp_command *cmd);

static inline int32_t sign16_coord(int32_t value)
{
    return (int16_t)(value & 0xffff);
}

static inline int32_t shift_tile_coord(int32_t coord, uint8_t shift)
{
    if (shift < 11u) {
        return sign16_coord(coord) >> shift;
    }
    if (shift < 16u) {
        return sign16_coord((int32_t)((uint32_t)coord << (16u - shift)));
    }
    return sign16_coord(coord);
}

static inline int32_t shift_tile_coord_fixed5(int32_t coord, uint8_t shift)
{
    if (shift < 11u) {
        return sign16_coord(coord) >> shift;
    }
    if (shift < 16u) {
        return sign16_coord((int32_t)((uint32_t)coord << (16u - shift)));
    }
    return sign16_coord(coord);
}

static inline uint32_t tmem_align_row_stride(uint32_t bytes)
{
    return (bytes + 7u) & ~7u;
}

static inline uint32_t tmem_physical_byte(uint32_t byte)
{
    return byte ^ 3u;
}

static inline uint32_t tmem_physical_word_byte(uint32_t byte)
{
    return ((byte >> 1) ^ 1u) << 1;
}

static inline uint16_t tmem_read_native16(const tmem_state *tmem, uint32_t byte)
{
    return tmem->words[(byte & 0xfffu) >> 1];
}

static inline void tmem_write_native16(tmem_state *tmem, uint32_t byte, uint16_t value)
{
    tmem->words[(byte & 0xfffu) >> 1] = value;
}

/* Palette entry `index` as its first TMEM copy holds it, the copy the copy
 * pipe reads. The one-copy samplers use it for 1- and 2-cycle draws only
 * while the entry's copies are identical; see palette_mixed. */
static inline rdp_color tmem_palette_entry(const tmem_state *tmem, bool ia,
                                           uint32_t index)
{
    return tmem->palette[ia ? 1 : 0][index];
}

static inline int32_t fixed5_floor_to_texel(int32_t coord)
{
    return coord >> 5;
}

static inline bool resolve_tile_axis(int32_t coord,
                                     int32_t lo,
                                     int32_t hi,
                                     uint32_t extent,
                                     bool clamp,
                                     bool mirror,
                                     uint8_t mask_bits,
                                     uint32_t *local)
{
    int32_t relative;

    if (clamp) {
        if (coord < lo) {
            relative = 0;
        } else if (coord > hi) {
            relative = (int32_t)extent - 1;
        } else {
            relative = coord - lo;
        }
    } else {
        relative = coord - lo;
    }

    if (mask_bits) {
        const int32_t period = 1 << mask_bits;
        const int32_t repeat_mask = mirror ? (period << 1) - 1 : period - 1;
        relative &= repeat_mask;
        if (mirror) {
            if (relative & period) {
                relative = ((period << 1) - 1) - relative;
            }
        }
    } else if (!clamp && (relative < 0 || relative >= (int32_t)extent)) {
        return false;
    }

    if (relative < 0 || relative >= (int32_t)extent) {
        return false;
    }

    *local = (uint32_t)relative;
    return true;
}

static inline bool tmem_resolve_rgba16_address_raw(const rdp_tile *tile,
                                                   uint32_t stride,
                                                   uint32_t local_s,
                                                   uint32_t local_t,
                                                   tmem_texel_address *address)
{
    if (!tile || !address) {
        return false;
    }

    /*
     * RGBA16 fetches words from TMEM in four-word groups. Odd rows flip word
     * address bit 1, which is the first piece of the real TMEM lane addressing
     * model that matters for nearest RGBA16 sampling.
     */
    const uint32_t logical = tile->tmem + local_t * stride + local_s * 2u;
    address->byte = tmem_physical_word_byte(
        (logical ^ ((local_t & 1u) ? 4u : 0u)) & 0xfffu);
    address->byte2 = 0;
    address->subtexel = 0;
    address->bytes = 2;
    return address->byte + 1u < SR_TMEM_SIZE;
}

static inline bool tmem_resolve_rgba32_address_raw(const rdp_tile *tile,
                                                   uint32_t stride,
                                                   uint32_t local_s,
                                                   uint32_t local_t,
                                                   tmem_texel_address *address)
{
    if (!tile || !address) {
        return false;
    }

    const uint32_t logical = tile->tmem + local_t * stride + local_s * 2u;
    /* RGBA32 is split across the two 2 KiB TMEM banks. The tile address
     * selects within a bank; it must not linearly run past the end of TMEM. */
    address->byte = tmem_physical_word_byte(
        logical ^ ((local_t & 1u) ? 4u : 0u)) & 0x7ffu;
    address->byte2 = address->byte | 0x800u;
    address->subtexel = 0;
    address->bytes = 4;
    return true;
}

static inline bool tmem_tile_extent_from_descriptor(const rdp_tile *tile, uint32_t *width, uint32_t *height)
{
    if (!tile || !width || !height) {
        return false;
    }

    *width = (((tile->sh >> 2) - (tile->sl >> 2)) & 0x3ffu) + 1u;
    *height = (((tile->th >> 2) - (tile->tl >> 2)) & 0x3ffu) + 1u;
    return *width != 0 && *height != 0;
}

static inline uint32_t tmem_derived_row_stride(const rdp_tile *tile, uint32_t width)
{
    const uint32_t bytes_per_texel = tile->size == RDP_SIZE_32BPP ? 4u :
                                     tile->size == RDP_SIZE_16BPP ? 2u :
                                     tile->size == RDP_SIZE_8BPP ? 1u : 0u;
    const uint32_t row_bytes = bytes_per_texel ? width * bytes_per_texel
                                               : (width + 1u) >> 1;
    return tmem_align_row_stride(row_bytes);
}

static inline bool tmem_tile_sample_layout(const tmem_state *tmem,
                                           const rdp_texture_sample_state *sample,
                                           uint32_t *width,
                                           uint32_t *height,
                                           uint32_t *stride)
{
    if (!tmem || !sample || !width || !height || !stride || sample->tile_index >= 8) {
        return false;
    }

    const rdp_tile *tile = &sample->tile;
    if (sample->width != 0 && sample->height != 0) {
        *width = sample->width;
        *height = sample->height;
        *stride = sample->stride;
        return true;
    }
    if (!tmem_tile_extent_from_descriptor(tile, width, height)) {
        return false;
    }
    *stride = tile->line;
    return true;
}

static inline bool tmem_resolve_texel_address_raw(const rdp_tile *tile,
                                                  uint32_t stride,
                                                  uint32_t local_s,
                                                  uint32_t local_t,
                                                  tmem_texel_address *address)
{
    if (!tile || !address) {
        return false;
    }

    const uint32_t row_xor = (local_t & 1u) ? 4u : 0u;
    switch (tile->size) {
    case RDP_SIZE_4BPP:
        address->byte = tmem_physical_byte(
            (tile->tmem + local_t * stride + ((local_s >> 1) ^ row_xor)) & 0xfffu);
        address->byte2 = 0;
        address->subtexel = (uint8_t)(local_s & 1u);
        address->bytes = 1;
        return address->byte < SR_TMEM_SIZE;
    case RDP_SIZE_8BPP:
        address->byte = tmem_physical_byte(
            (tile->tmem + local_t * stride + (local_s ^ row_xor)) & 0xfffu);
        address->byte2 = 0;
        address->subtexel = 0;
        address->bytes = 1;
        return address->byte < SR_TMEM_SIZE;
    case RDP_SIZE_16BPP:
        return tmem_resolve_rgba16_address_raw(tile, stride, local_s, local_t, address);
    case RDP_SIZE_32BPP:
        /* CI, IA and I have no split-bank layout at 32bpp: the texture unit
         * reads one 16-bit word per texel, addressed like RGBA16 across all
         * of TMEM. */
        if (tile->format == RDP_FORMAT_CI || tile->format == RDP_FORMAT_IA ||
            tile->format == RDP_FORMAT_I)
            return tmem_resolve_rgba16_address_raw(tile, stride, local_s, local_t, address);
        return tmem_resolve_rgba32_address_raw(tile, stride, local_s, local_t, address);
    }

    return false;
}

/* When TLUT lookup is enabled, the texture fetch is confined to TMEM's lower
 * 2 KiB bank.  The upper bank is then addressed separately by the palette
 * lookup.  In particular, a SetTile TMEM value of 0x100 (byte address 0x800)
 * aliases byte zero for the source texel; several games rely on this. */
static inline uint32_t tmem_tlut_texel_byte(uint32_t byte)
{
    return byte & 0x7ffu;
}

static inline bool tmem_resolve_texel_coord(const tmem_state *tmem,
                                            const rdp_texture_sample_state *sample,
                                            int32_t s,
                                            int32_t t,
                                            uint32_t *local_s,
                                            uint32_t *local_t)
{
    if (!tmem || !sample || !local_s || !local_t || sample->tile_index >= 8) {
        return false;
    }

    const rdp_tile *tile = &sample->tile;
    const rdp_tile_bounds *bounds = &sample->bounds;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    if (!tmem_tile_sample_layout(tmem, sample, &width, &height, &stride)) {
        return false;
    }

    s = shift_tile_coord(s, tile->shift_s);
    t = shift_tile_coord(t, tile->shift_t);

    return resolve_tile_axis(s,
                             bounds->sl,
                             bounds->sh,
                             width,
                             tile->clamp_s != 0 || tile->mask_s == 0,
                             tile->mirror_s != 0,
                             tile->mask_s,
                             local_s) &&
           resolve_tile_axis(t,
                             bounds->tl,
                             bounds->th,
                             height,
                             tile->clamp_t != 0 || tile->mask_t == 0,
                             tile->mirror_t != 0,
                             tile->mask_t,
                             local_t);
}

static inline bool tmem_resolve_texel_coord_fixed5(const tmem_state *tmem,
                                                   const rdp_texture_sample_state *sample,
                                                   int32_t s_fixed,
                                                   int32_t t_fixed,
                                                   uint32_t *local_s,
                                                   uint32_t *local_t)
{
    if (!tmem || !sample || !local_s || !local_t || sample->tile_index >= 8) {
        return false;
    }

    const rdp_tile *tile = &sample->tile;
    const rdp_tile_bounds *bounds = &sample->bounds;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    if (!tmem_tile_sample_layout(tmem, sample, &width, &height, &stride)) {
        return false;
    }
    (void)stride;

    s_fixed = shift_tile_coord_fixed5(s_fixed, tile->shift_s);
    t_fixed = shift_tile_coord_fixed5(t_fixed, tile->shift_t);

    const int32_t s_relative = s_fixed - ((int32_t)tile->sl << 3);
    const int32_t t_relative = t_fixed - ((int32_t)tile->tl << 3);
    const int32_t s_texel = fixed5_floor_to_texel(s_relative);
    const int32_t t_texel = fixed5_floor_to_texel(t_relative);

    return resolve_tile_axis(s_texel,
                             0,
                             bounds->sh >= bounds->sl ? (int32_t)(bounds->sh - bounds->sl) : 0,
                             width,
                             tile->clamp_s != 0 || tile->mask_s == 0,
                             tile->mirror_s != 0,
                             tile->mask_s,
                             local_s) &&
           resolve_tile_axis(t_texel,
                             0,
                             bounds->th >= bounds->tl ? (int32_t)(bounds->th - bounds->tl) : 0,
                             height,
                             tile->clamp_t != 0 || tile->mask_t == 0,
                             tile->mirror_t != 0,
                             tile->mask_t,
                             local_t);
}

static inline bool tmem_resolve_texel_address(const tmem_state *tmem,
                                              const rdp_texture_sample_state *sample,
                                              uint32_t local_s,
                                              uint32_t local_t,
                                              tmem_texel_address *address)
{
    if (!tmem || !sample || !address || sample->tile_index >= 8) {
        return false;
    }

    const rdp_tile *tile = &sample->tile;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    if (!tmem_tile_sample_layout(tmem, sample, &width, &height, &stride)) {
        return false;
    }

    if (local_s >= width || local_t >= height) {
        return false;
    }

    return tmem_resolve_texel_address_raw(tile,
                                          stride,
                                          local_s,
                                          local_t,
                                          address);
}

static inline bool tmem_sample_rgba5551(const tmem_state *tmem, const rdp_texture_sample_state *sample, int32_t s, int32_t t, uint16_t *texel)
{
    if (!texel) {
        return false;
    }

    uint32_t local_s;
    uint32_t local_t;
    if (!tmem_resolve_texel_coord(tmem, sample, s, t, &local_s, &local_t)) {
        return false;
    }

    if (!sample || sample->tile.format != RDP_FORMAT_RGBA ||
        sample->tile.size != RDP_SIZE_16BPP) {
        return false;
    }

    tmem_texel_address address;
    if (!tmem_resolve_texel_address(tmem, sample, local_s, local_t, &address)) {
        return false;
    }

    if (address.bytes != 2) {
        return false;
    }

    *texel = tmem_read_native16(tmem, address.byte);
    return true;
}

static inline uint8_t expand_4_to_8(uint32_t value)
{
    value &= 0xfu;
    return (uint8_t)((value << 4) | value);
}

static inline uint8_t lerp_u8(uint8_t a, uint8_t b, uint32_t frac5)
{
    return (uint8_t)(((uint32_t)a * (32u - frac5) + (uint32_t)b * frac5 + 16u) >> 5);
}

static inline rdp_color lerp_color(rdp_color a, rdp_color b, uint32_t frac5)
{
    return (rdp_color){
        lerp_u8(a.r, b.r, frac5),
        lerp_u8(a.g, b.g, frac5),
        lerp_u8(a.b, b.b, frac5),
        lerp_u8(a.a, b.a, frac5)
    };
}

static inline uint8_t clamp_i32_to_u8(int32_t value)
{
    return value < 0 ? 0u : (value > 255 ? 255u : (uint8_t)value);
}

static inline __attribute__((always_inline)) uint8_t bilerp_3tap_u8(uint8_t base, uint8_t edge_s, uint8_t edge_t, uint32_t frac_s, uint32_t frac_t)
{
    int32_t value = ((int32_t)edge_s - (int32_t)base) * (int32_t)frac_s;
    value += ((int32_t)edge_t - (int32_t)base) * (int32_t)frac_t;
    value = (value + 0x10) >> 5;
    value += base;
    return clamp_i32_to_u8(value);
}

static inline __attribute__((always_inline)) rdp_color bilerp_3tap_color(rdp_color base, rdp_color edge_s, rdp_color edge_t, uint32_t frac_s, uint32_t frac_t)
{
    return (rdp_color){
        bilerp_3tap_u8(base.r, edge_s.r, edge_t.r, frac_s, frac_t),
        bilerp_3tap_u8(base.g, edge_s.g, edge_t.g, frac_s, frac_t),
        bilerp_3tap_u8(base.b, edge_s.b, edge_t.b, frac_s, frac_t),
        bilerp_3tap_u8(base.a, edge_s.a, edge_t.a, frac_s, frac_t)
    };
}

/* A YUV16 texel: luma from upper TMEM at its own column, chroma from the lower
 * half at chroma column chroma_x, which a pair of texels shares. U and V are
 * signed in eight bits; tmem_texel_output widens them. */
static inline void tmem_fetch_yuv_local(const tmem_state *tmem,
                                        const rdp_texture_sample_state *sample,
                                        uint32_t local_s, uint32_t local_t,
                                        uint32_t chroma_x, rdp_color *color)
{
    const uint32_t row_base = sample->tile.tmem + local_t * sample->stride;
    const uint32_t row_xor = (local_t & 1u) ? 4u : 0u;
    const uint32_t uv_addr = tmem_physical_word_byte(
        (row_base + chroma_x * 2u) ^ row_xor) & 0x7ffu;
    const uint32_t y_addr = (tmem_physical_byte(
        (row_base + local_s) ^ row_xor) & 0x7ffu) | 0x800u;
    const uint16_t uv = tmem_read_native16(tmem, uv_addr);
    color->r = (uint8_t)((int32_t)(uint8_t)(uv >> 8) - 128);
    color->g = (uint8_t)((int32_t)(uint8_t)uv - 128);
    color->b = tmem->bytes[y_addr];
    color->a = color->b;
}

static inline bool tmem_fetch_color_local(const tmem_state *tmem,
                                          const rdp_texture_sample_state *sample,
                                          uint32_t local_s,
                                          uint32_t local_t,
                                          rdp_color *color)
{
    if (!color || !sample || sample->tile_index >= 8) {
        return false;
    }

    const rdp_tile *tile = &sample->tile;
    tmem_texel_address address;
    /* Compiled coordinates are already clamped and masked; the texel beyond a
     * tile's edge is read wherever TMEM holds it. */
    const bool resolved = sample->width && sample->height
        ? tmem_resolve_texel_address_raw(&sample->tile, sample->stride,
                                         local_s, local_t, &address)
        : tmem_resolve_texel_address(tmem, sample, local_s, local_t, &address);
    if (!resolved) {
        return false;
    }

    /* TLUT-enabled IA16 rectangle effects use the upper texel byte as their
     * palette index. */
    const bool indexed_tlut = sample->tlut_enable &&
        tile->format != RDP_FORMAT_YUV &&
        (tile->size == RDP_SIZE_4BPP || tile->size == RDP_SIZE_8BPP ||
         sample->tlut_wide_index);
    if (indexed_tlut) {
        address.byte = tmem_tlut_texel_byte(address.byte);
        address.byte2 = tmem_tlut_texel_byte(address.byte2);
        uint32_t index;
        if (tile->size == RDP_SIZE_4BPP && address.bytes == 1) {
            const uint8_t packed = tmem->bytes[address.byte];
            index = ((uint32_t)tile->palette << 4) |
                    (address.subtexel ? (packed & 0xfu) : (packed >> 4));
        } else if (tile->size == RDP_SIZE_8BPP && address.bytes == 1) {
            index = tmem->bytes[address.byte];
        } else if (tile->size >= RDP_SIZE_16BPP && address.bytes >= 2) {
            index = tmem_read_native16(tmem, address.byte) >> 8;
        } else {
            return false;
        }

        const uint32_t palette_logical = 0x800u + index * 8u;
        const uint32_t palette_addr = palette_logical;
        if (palette_addr + 1u >= SR_TMEM_SIZE) return false;
        const uint16_t entry = tmem_read_native16(tmem, palette_addr);
        if (sample->tlut_ia) {
            const uint8_t intensity = (uint8_t)(entry >> 8);
            *color = (rdp_color){ intensity, intensity, intensity, (uint8_t)entry };
        } else {
            *color = tmem_rgba16_decode_table[entry];
        }
        return true;
    }

    if (tile->format == RDP_FORMAT_IA) {
        switch (tile->size) {
        case RDP_SIZE_4BPP: {
            if (address.bytes != 1) {
                return false;
            }
            const uint8_t packed = tmem->bytes[address.byte];
            const uint8_t texel = address.subtexel ? (packed & 0xfu) : (packed >> 4);
            const uint8_t intensity_bits = texel & 0xeu;
            const uint8_t intensity = (uint8_t)((intensity_bits << 4) |
                                                (intensity_bits << 1) |
                                                (intensity_bits >> 2));
            const uint8_t alpha = (texel & 1u) ? 0xffu : 0u;
            *color = (rdp_color){ intensity, intensity, intensity, alpha };
            return true;
        }
        case RDP_SIZE_8BPP: {
            if (address.bytes != 1) {
                return false;
            }
            const uint8_t texel = tmem->bytes[address.byte];
            const uint8_t intensity = expand_4_to_8(texel >> 4);
            const uint8_t alpha = expand_4_to_8(texel);
            *color = (rdp_color){ intensity, intensity, intensity, alpha };
            return true;
        }
        case RDP_SIZE_16BPP:
            if (address.bytes != 2) {
                return false;
            }
            const uint16_t texel = tmem_read_native16(tmem, address.byte);
            const uint8_t intensity = (uint8_t)(texel >> 8);
            *color = (rdp_color){ intensity, intensity, intensity, (uint8_t)texel };
            return true;
        case RDP_SIZE_32BPP: {
            /* IA32 has no decode of its own: the unit exposes the 16-bit
             * word as R,G,B,A = hi,lo,hi,lo, as for CI32. */
            if (address.bytes < 2) {
                return false;
            }
            const uint16_t word = tmem_read_native16(tmem, address.byte);
            const uint8_t high = (uint8_t)(word >> 8);
            const uint8_t low = (uint8_t)word;
            *color = (rdp_color){ high, low, high, low };
            return true;
        }
        default:
            return false;
        }
    }

    if (tile->format == RDP_FORMAT_I) {
        if (tile->size == RDP_SIZE_4BPP && address.bytes == 1) {
            const uint8_t packed = tmem->bytes[address.byte];
            const uint8_t nibble = address.subtexel ? (packed & 0xfu) : (packed >> 4);
            const uint8_t intensity = expand_4_to_8(nibble);
            *color = (rdp_color){ intensity, intensity, intensity, intensity };
            return true;
        }
        if (tile->size == RDP_SIZE_8BPP && address.bytes == 1) {
            const uint8_t intensity = tmem->bytes[address.byte];
            *color = (rdp_color){ intensity, intensity, intensity, intensity };
            return true;
        }
        /* The RDP does not reject the nominally invalid I16 and I32
         * encodings. Its texture fetch unit exposes the 16-bit word as
         * R,G,B,A = hi,lo,hi,lo. */
        if ((tile->size == RDP_SIZE_16BPP || tile->size == RDP_SIZE_32BPP) &&
            address.bytes >= 2) {
            const uint16_t texel = tmem_read_native16(tmem, address.byte);
            const uint8_t high = (uint8_t)(texel >> 8);
            const uint8_t low = (uint8_t)texel;
            *color = (rdp_color){ high, low, high, low };
            return true;
        }
        return false;
    }

    /* With TLUT disabled, CI is not rejected. The texture unit exposes the
     * index directly, matching the corresponding raw-width RGBA wiring.
     * Yoshi's Story uses CI8 this way while constructing auxiliary buffers. */
    if (tile->format == RDP_FORMAT_CI) {
        if (tile->size == RDP_SIZE_4BPP && address.bytes == 1u) {
            const uint8_t packed = tmem->bytes[address.byte];
            uint8_t index = address.subtexel ? (packed & 0xfu) : (packed >> 4);
            index |= (uint8_t)(tile->palette << 4);
            *color = (rdp_color){index, index, index, index};
            return true;
        }
        if (tile->size == RDP_SIZE_8BPP && address.bytes == 1u) {
            const uint8_t index = tmem->bytes[address.byte];
            *color = (rdp_color){index, index, index, index};
            return true;
        }
        if ((tile->size == RDP_SIZE_16BPP || tile->size == RDP_SIZE_32BPP) &&
            address.bytes >= 2u) {
            const uint16_t texel = tmem_read_native16(tmem, address.byte);
            const uint8_t high = (uint8_t)(texel >> 8);
            const uint8_t low = (uint8_t)texel;
            *color = (rdp_color){high, low, high, low};
            return true;
        }
        return false;
    }

    if (tile->format == RDP_FORMAT_YUV) {
        if (tile->size == RDP_SIZE_32BPP) {
            /* YUV at size 3 is illegal; the RDP addresses it at 32bpp texel
             * rate. A 32-bit unit holds a UYVY pair, so the pair index is
             * s >> 1, the line term doubles, and the odd-line swap applies to
             * the pair index. Address bit 1 of the in-row word is the OR of
             * the 16-bit-rate and 32bpp-rate drivers. Chroma is the addressed
             * low-bank word; luma is byte (s & 1) of that word in the high
             * bank. The halfword index equals the byte row base numerically. */
            const uint32_t m = ((local_s >> 1) ^ ((local_t & 1u) << 1)) & 0x1ffu;
            const uint32_t inrow = ((m >> 1) << 2) |
                                   (((m | (m >> 1)) & 1u) << 1) | (m & 1u);
            const uint32_t word = (sample->tile.tmem + local_t * sample->stride +
                                   inrow) & 0x3ffu;
            const uint16_t uv = tmem_read_native16(tmem,
                tmem_physical_word_byte(word << 1) & 0x7ffu);
            const uint32_t y_addr = (tmem_physical_byte(
                ((word << 1) | (local_s & 1u)) & 0x7ffu) & 0x7ffu) | 0x800u;
            color->r = (uint8_t)((int32_t)(uint8_t)(uv >> 8) - 128);
            color->g = (uint8_t)((int32_t)(uint8_t)uv - 128);
            color->b = tmem->bytes[y_addr];
            color->a = color->b;
            return true;
        }
        if (tile->size != RDP_SIZE_16BPP) {
            return false;
        }

        tmem_fetch_yuv_local(tmem, sample, local_s, local_t, local_s >> 1, color);
        return true;
    }

    if (tile->format != RDP_FORMAT_RGBA) {
        return false;
    }

    switch (tile->size) {
    /* RGBA has no 4- or 8-bit colour decode; the texture unit exposes such a
     * texel as intensity in every channel, exactly like I4 and I8. */
    case RDP_SIZE_4BPP:
        if (address.bytes != 1) {
            return false;
        }
        {
            const uint8_t packed = tmem->bytes[address.byte];
            const uint8_t intensity = expand_4_to_8(address.subtexel
                ? (packed & 0xfu) : (packed >> 4));
            *color = (rdp_color){ intensity, intensity, intensity, intensity };
        }
        return true;
    case RDP_SIZE_8BPP:
        if (address.bytes != 1) {
            return false;
        }
        {
            const uint8_t intensity = tmem->bytes[address.byte];
            *color = (rdp_color){ intensity, intensity, intensity, intensity };
        }
        return true;
    case RDP_SIZE_16BPP: {
        if (address.bytes != 2) {
            return false;
        }
        const uint16_t texel = tmem_read_native16(tmem, address.byte);
        *color = rdp_color_from_rgba5551(texel);
        return true;
    }
    case RDP_SIZE_32BPP:
        if (address.bytes != 4 || address.byte + 1u >= SR_TMEM_SIZE || address.byte2 + 1u >= SR_TMEM_SIZE) {
            return false;
        }
        {
            const uint16_t rg = tmem_read_native16(tmem, address.byte);
            const uint16_t ba = tmem_read_native16(tmem, address.byte2);
            *color = (rdp_color){(uint8_t)(rg >> 8), (uint8_t)rg,
                                 (uint8_t)(ba >> 8), (uint8_t)ba};
        }
        return true;
    default:
        return false;
    }
}

/*
 * A fetched texel as the combiner receives it, in nine-bit channels, for the
 * draws whose texel output is not plain. Raw YUV carries signed U and V. With
 * bi_lerp clear the filter's multipliers run the K0-K3 colour conversion
 * instead, and its result is not clamped. Only the full texture unit reaches
 * it.
 */
static inline void tmem_texel_output(const rdp_texture_sample_state *sample,
                                     rdp_color color, uint16_t out[4])
{
    const bool yuv = sample->tile.format == RDP_FORMAT_YUV;
    const int32_t u = yuv ? (int8_t)color.r : color.r;
    const int32_t v = yuv ? (int8_t)color.g : color.g;
    const int32_t y = color.b;
    if (sample->texel_output == RDP_TEXEL_CONVERT) {
        out[0] = (uint16_t)(y + ((sample->convert_k0_tf * v + 0x80) >> 8)) & 0x1ffu;
        out[1] = (uint16_t)(y + ((sample->convert_k1_tf * u +
                                  sample->convert_k2_tf * v + 0x80) >> 8)) & 0x1ffu;
        out[2] = (uint16_t)(y + ((sample->convert_k3_tf * u + 0x80) >> 8)) & 0x1ffu;
        out[3] = (uint16_t)y;
    } else {
        out[0] = (uint16_t)u & 0x1ffu;
        out[1] = (uint16_t)v & 0x1ffu;
        out[2] = color.b;
        out[3] = color.a;
    }
}

static inline bool tmem_sample_color(const tmem_state *tmem, const rdp_texture_sample_state *sample, int32_t s, int32_t t, rdp_color *color)
{
    if (!color || !sample || sample->tile_index >= 8) {
        return false;
    }

    uint32_t local_s;
    uint32_t local_t;
    if (!tmem_resolve_texel_coord(tmem, sample, s, t, &local_s, &local_t)) {
        return false;
    }

    return tmem_fetch_color_local(tmem, sample, local_s, local_t, color);
}

/*
 * The texture unit's clamp and mask for one axis, reduced to draw constants.
 * The clamp compares the shifted coordinate with the tile's far edge at its
 * full 10.2 precision, so it fires at offset clamp_limit, and pins it to that
 * edge's whole texel. A missing mask forces the clamp. The mirror fold
 * (2p - 1) - r equals r ^ (2p - 1) for the r in [p, 2p) it applies to.
 * Without a mask, repeat is all ones.
 */
static inline void tmem_compile_axes(rdp_texture_sample_state *sample)
{
    sample->axes_ready = sample->width != 0u && sample->height != 0u;
    for (uint32_t index = 0; index < 2u; index++) {
        const bool s_axis = index == 0u;
        const rdp_tile *tile = &sample->tile;
        rdp_texture_axis *axis = &sample->axis[index];
        const int32_t lo = (int32_t)(s_axis ? tile->sl : tile->tl);
        const int32_t hi = (int32_t)(s_axis ? tile->sh : tile->th);
        const uint8_t mask = s_axis ? tile->mask_s : tile->mask_t;
        const bool mirror = (s_axis ? tile->mirror_s : tile->mirror_t) != 0 && mask;
        const int32_t period = mask ? 1 << mask : 0;
        axis->origin = lo * 8;
        axis->clamp_limit = (hi - lo) * 8;
        axis->clamp_offset = (int32_t)(s_axis ? sample->bounds.sh : sample->bounds.th) << 5;
        axis->extent = s_axis ? sample->width : sample->height;
        axis->repeat = !mask ? -1 : mirror ? (period << 1) - 1 : period - 1;
        axis->mirror_bit = mirror ? period : 0;
        axis->mirror_fold = mirror ? (period << 1) - 1 : 0;
        axis->shift = s_axis ? tile->shift_s : tile->shift_t;
        axis->clamp = (s_axis ? tile->clamp_s : tile->clamp_t) != 0 || mask == 0;
    }
}

static inline __attribute__((always_inline)) uint32_t tmem_axis_mask(
    const rdp_texture_axis *axis, int32_t texel)
{
    texel &= axis->repeat;
    texel ^= (0 - (int32_t)((texel & axis->mirror_bit) != 0)) & axis->mirror_fold;
    return (uint32_t)texel;
}

/* The tile-relative 10.5 offset after the clamp. A clamped offset has no
 * fraction. */
static inline __attribute__((always_inline)) int32_t tmem_axis_clamp(
    const rdp_texture_axis *axis, int32_t shifted)
{
    const int32_t offset = shifted - axis->origin;
    if (axis->clamp) {
        if (offset >= axis->clamp_limit) return axis->clamp_offset;
        if (offset < 0) return 0;
    }
    return offset;
}

/* T addresses TMEM rows with its low eight bits only; the second row keeps its
 * distance from the first, except that a wrap lands on row 0. */
static inline __attribute__((always_inline)) void tmem_axis_rows(uint32_t *t0,
                                                                 uint32_t *t1)
{
    int32_t step = (int32_t)(*t1 - *t0);
    if (step < -255) step = -255;
    *t0 &= 0xffu;
    *t1 = *t0 + (uint32_t)step;
}

static inline bool tmem_resolve_compiled_axis_shifted_fixed5(
    const rdp_texture_sample_state *sample, int32_t shifted, bool s_axis,
    uint32_t *local)
{
    const rdp_texture_axis *axis = &sample->axis[s_axis ? 0 : 1];
    const uint32_t texel = tmem_axis_mask(
        axis, fixed5_floor_to_texel(tmem_axis_clamp(axis, shifted)));
    *local = s_axis ? texel : texel & 0xffu;
    return true;
}

static inline bool tmem_resolve_compiled_axis_fixed5(const rdp_texture_sample_state *sample,
                                                      int32_t fixed, bool s_axis,
                                                      uint32_t *local)
{
    const uint8_t shift = s_axis ? sample->tile.shift_s : sample->tile.shift_t;
    return tmem_resolve_compiled_axis_shifted_fixed5(
        sample, shift_tile_coord_fixed5(fixed, shift), s_axis, local);
}

static inline bool tmem_resolve_compiled_coord_fixed5(const rdp_texture_sample_state *sample,
                                                       int32_t s_fixed, int32_t t_fixed,
                                                       uint32_t *local_s, uint32_t *local_t)
{
    return tmem_resolve_compiled_axis_fixed5(sample, s_fixed, true, local_s) &&
           tmem_resolve_compiled_axis_fixed5(sample, t_fixed, false, local_t);
}

/* Both bilerp taps on one axis and the fraction between them. The second tap
 * is the first plus one, masked but not clamped, so at a clamped edge it is
 * the texel beyond the tile. */
static inline __attribute__((always_inline)) bool tmem_resolve_axis_pair_fixed5(
    const rdp_texture_sample_state *sample, int32_t fixed, bool s_axis,
    uint32_t *local0, uint32_t *local1, uint32_t *frac)
{
    const rdp_texture_axis *axis = &sample->axis[s_axis ? 0 : 1];
    const int32_t offset = tmem_axis_clamp(axis,
                                           shift_tile_coord_fixed5(fixed, axis->shift));
    const int32_t texel = fixed5_floor_to_texel(offset);
    *frac = (uint32_t)offset & 31u;
    *local0 = tmem_axis_mask(axis, texel);
    *local1 = tmem_axis_mask(axis, texel + 1);
    if (!s_axis) tmem_axis_rows(local0, local1);
    return true;
}

static inline bool tmem_sample_point_compiled_fixed5(const tmem_state *tmem,
                                                      const rdp_texture_sample_state *sample,
                                                      int32_t s_fixed, int32_t t_fixed,
                                                      rdp_color *color)
{
    uint32_t local_s, local_t;
    return color && tmem_resolve_compiled_coord_fixed5(sample, s_fixed, t_fixed,
                                                        &local_s, &local_t) &&
           tmem_fetch_color_local(tmem, sample, local_s, local_t, color);
}

/* Copy fetches select a member of a horizontal group after coordinate shift,
 * but before mask, mirror and address routing. Applying the member offset to
 * the incoming fixed-point coordinate would incorrectly scale it through the
 * tile's S shift. */
static inline int32_t tmem_copy_mask_coordinate(int32_t coordinate,
                                                uint8_t mask, bool mirror)
{
    if (mask == 0u) return coordinate;
    const int32_t period = 1 << mask;
    coordinate &= mirror ? (period << 1) - 1 : period - 1;
    if (mirror && (coordinate & period) != 0)
        coordinate = ((period << 1) - 1) - coordinate;
    return coordinate;
}

static inline uint16_t tmem_copy_native_word(const tmem_state *tmem,
                                             const rdp_tile *tile,
                                             uint32_t stride, int32_t s,
                                             int32_t t, bool lower_bank)
{
    const uint32_t size_shift = tile->size > RDP_SIZE_16BPP
        ? RDP_SIZE_16BPP : tile->size;
    uint32_t nibble = ((tile->tmem + (uint32_t)t * stride) * 2u +
                       ((uint32_t)s << size_shift)) & 0x1fffu;
    nibble ^= ((uint32_t)t & 1u) * 8u;
    uint32_t index = nibble >> 2u;
    index &= lower_bank ? 0x3ffu : 0x7ffu;
    return tmem_read_native16(tmem, (index ^ 1u) << 1u);
}

static inline bool tmem_fetch_copy_word_fixed5(
    const tmem_state *tmem, const rdp_texture_sample_state *sample,
    int32_t s_fixed, int32_t t_fixed, uint32_t word_offset,
    uint16_t *word)
{
    if (!tmem || !sample || !word) return false;
    const int32_t shifted_s = shift_tile_coord_fixed5(s_fixed,
                                                       sample->tile.shift_s);
    const int32_t shifted_t = shift_tile_coord_fixed5(t_fixed,
                                                       sample->tile.shift_t);
    const rdp_tile *tile = &sample->tile;
    int32_t local_s = fixed5_floor_to_texel(
        shifted_s - ((int32_t)tile->sl << 3));
    int32_t local_t = fixed5_floor_to_texel(
        shifted_t - ((int32_t)tile->tl << 3));
    local_t = tmem_copy_mask_coordinate(local_t, tile->mask_t,
                                         tile->mirror_t != 0);

    const bool paired_compact = word_offset < 2u &&
        tile->size != RDP_SIZE_16BPP && !sample->tlut_enable;
    if (paired_compact) {
        const int32_t first = local_s + (int32_t)(2u * word_offset);
        uint8_t bytes[2];
        for (uint32_t lane = 0; lane < 2u; lane++) {
            const int32_t s = tmem_copy_mask_coordinate(first + (int32_t)lane,
                tile->mask_s, tile->mirror_s != 0);
            tmem_texel_address address;
            if (!tmem_resolve_texel_address_raw(tile, sample->stride,
                    (uint32_t)s, (uint32_t)local_t, &address)) return false;
            const uint8_t packed = tmem->bytes[address.byte];
            if (tile->size == RDP_SIZE_4BPP) {
                const uint8_t nibble = address.subtexel
                    ? packed & 0xfu : packed >> 4;
                bytes[lane] = (uint8_t)(nibble * 0x11u);
            } else if (tile->size == RDP_SIZE_8BPP) {
                bytes[lane] = packed;
            } else {
                bytes[lane] = (uint8_t)(
                    tmem_copy_native_word(tmem, tile, sample->stride, s,
                                          local_t, true) >> 8);
            }
        }
        *word = (uint16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
        return true;
    }

    local_s = tmem_copy_mask_coordinate(local_s + (int32_t)word_offset,
                                         tile->mask_s,
                                         tile->mirror_s != 0);
    uint16_t fetched = tmem_copy_native_word(tmem, tile, sample->stride,
        local_s, local_t,
        tile->size == RDP_SIZE_32BPP || sample->tlut_enable);
    if (sample->tlut_enable) {
        uint32_t index;
        if (tile->size == RDP_SIZE_4BPP) {
            const uint32_t shift = 12u - 4u * ((uint32_t)local_s & 3u);
            index = ((fetched >> shift) & 0xfu) |
                    ((uint32_t)tile->palette << 4u);
        } else {
            const uint32_t shift = tile->size == RDP_SIZE_8BPP
                ? 8u - 8u * ((uint32_t)local_s & 1u) : 8u;
            index = (fetched >> shift) & 0xffu;
        }
        index = (index << 2u) + word_offset;
        fetched = tmem_read_native16(tmem,
            ((((index | 0x400u) & 0x7ffu) ^ 1u) << 1u));
    }
    *word = fetched;
    return true;
}

static inline bool tmem_fetch_rgba16_local(const tmem_state *tmem,
                                            const rdp_texture_sample_state *sample,
                                            uint32_t local_s, uint32_t local_t,
                                            rdp_color *color)
{
    tmem_texel_address address;
    if (!tmem_resolve_rgba16_address_raw(&sample->tile, sample->stride,
                                         local_s, local_t, &address) ||
        address.byte + 1u >= SR_TMEM_SIZE) return false;
    const uint16_t texel = tmem_read_native16(tmem, address.byte);
    *color = tmem_rgba16_decode_table[texel];
    return true;
}

/* IA16 shares the RGBA16 word layout: intensity in the high byte, alpha in
 * the low one, as tmem_fetch_color_local reads it with TLUT disabled. */
static inline bool tmem_fetch_ia16_local(const tmem_state *tmem,
                                          const rdp_texture_sample_state *sample,
                                          uint32_t local_s, uint32_t local_t,
                                          rdp_color *color)
{
    tmem_texel_address address;
    if (!tmem_resolve_rgba16_address_raw(&sample->tile, sample->stride,
                                         local_s, local_t, &address) ||
        address.byte + 1u >= SR_TMEM_SIZE) return false;
    const uint16_t texel = tmem_read_native16(tmem, address.byte);
    const uint8_t intensity = (uint8_t)(texel >> 8);
    *color = (rdp_color){ intensity, intensity, intensity, (uint8_t)texel };
    return true;
}

static inline bool tmem_sample_rgba16_point_fixed5(const tmem_state *tmem,
                                                    const rdp_texture_sample_state *sample,
                                                    int32_t s_fixed, int32_t t_fixed,
                                                    rdp_color *color)
{
    uint32_t local_s, local_t;
    return color && tmem_resolve_compiled_coord_fixed5(sample, s_fixed, t_fixed,
                                                        &local_s, &local_t) &&
           tmem_fetch_rgba16_local(tmem, sample, local_s, local_t, color);
}

static inline __attribute__((always_inline)) bool tmem_fetch_compact_class(
                                            const tmem_state *tmem,
                                            const rdp_sampler_class cls,
                                            const rdp_texture_sample_state *sample,
                                            uint32_t local_s, uint32_t local_t,
                                            rdp_color *color)
{
    /* Each compact class is only assigned to its own texel size (I4 and CI4
     * are 4bpp, the others 8bpp), so this is tmem_resolve_texel_address_raw
     * with its size switch folded. Both sizes always yield one in-range byte. */
    const bool four_bit = cls == RDP_SAMPLER_I4_BILERP ||
                          cls == RDP_SAMPLER_CI4_TLUT_BILERP;
    const uint32_t row_xor = (local_t & 1u) ? 4u : 0u;
    tmem_texel_address address;
    address.byte = tmem_physical_byte((sample->tile.tmem + local_t * sample->stride +
        ((four_bit ? local_s >> 1 : local_s) ^ row_xor)) & 0xfffu);
    address.byte2 = 0;
    address.subtexel = (uint8_t)(four_bit ? (local_s & 1u) : 0u);
    address.bytes = 1;
    if (cls == RDP_SAMPLER_I4_BILERP) {
        const uint8_t packed = tmem->bytes[address.byte];
        const uint8_t intensity = expand_4_to_8(
            address.subtexel ? (packed & 0xfu) : (packed >> 4));
        *color = (rdp_color){ intensity, intensity, intensity, intensity };
        return true;
    }
    if (cls == RDP_SAMPLER_CI4_TLUT_BILERP) {
        address.byte = tmem_tlut_texel_byte(address.byte);
        const uint8_t packed = tmem->bytes[address.byte];
        const uint32_t index = ((uint32_t)sample->tile.palette << 4) |
            (address.subtexel ? (packed & 0xfu) : (packed >> 4));
        *color = tmem_palette_entry(tmem, sample->tlut_ia, index);
        return true;
    }
    if (cls == RDP_SAMPLER_I8_BILERP) {
        const uint8_t intensity = tmem->bytes[address.byte];
        *color = (rdp_color){ intensity, intensity, intensity, intensity };
        return true;
    }
    if (cls == RDP_SAMPLER_IA8_BILERP) {
        const uint8_t texel = tmem->bytes[address.byte];
        const uint8_t intensity = expand_4_to_8(texel >> 4);
        const uint8_t alpha = expand_4_to_8(texel);
        *color = (rdp_color){ intensity, intensity, intensity, alpha };
        return true;
    }
    *color = tmem_palette_entry(tmem, sample->tlut_ia,
                                tmem->bytes[tmem_tlut_texel_byte(address.byte)]);
    return true;
}

static inline __attribute__((always_inline)) bool tmem_sample_compact_bilerp_class(
                                                     const tmem_state *tmem,
                                                     const rdp_sampler_class cls,
                                                     const rdp_texture_sample_state *sample,
                                                     int32_t s_fixed, int32_t t_fixed,
                                                     rdp_color *color)
{
    uint32_t s0, t0, s1, t1, frac_s, frac_t;
    if (!color || !tmem_resolve_axis_pair_fixed5(sample, s_fixed, true, &s0, &s1, &frac_s) ||
        !tmem_resolve_axis_pair_fixed5(sample, t_fixed, false, &t0, &t1, &frac_t))
        return false;
    rdp_color c00, c10, c01, c11;
    if (sample->mid_texel && frac_s == 16u && frac_t == 16u) {
        if (tmem_fetch_compact_class(tmem, cls, sample,s0, t0, &c00) &&
            tmem_fetch_compact_class(tmem, cls, sample,s1, t0, &c10) &&
            tmem_fetch_compact_class(tmem, cls, sample,s0, t1, &c01) &&
            tmem_fetch_compact_class(tmem, cls, sample,s1, t1, &c11)) {
            *color = (rdp_color){
                (uint8_t)(((uint32_t)c00.r + c10.r + c01.r + c11.r + 2u) >> 2),
                (uint8_t)(((uint32_t)c00.g + c10.g + c01.g + c11.g + 2u) >> 2),
                (uint8_t)(((uint32_t)c00.b + c10.b + c01.b + c11.b + 2u) >> 2),
                (uint8_t)(((uint32_t)c00.a + c10.a + c01.a + c11.a + 2u) >> 2)
            };
            return true;
        }
    } else if (frac_s + frac_t >= 32u) {
        if (tmem_fetch_compact_class(tmem, cls, sample,s1, t1, &c11) &&
            tmem_fetch_compact_class(tmem, cls, sample,s1, t0, &c10) &&
            tmem_fetch_compact_class(tmem, cls, sample,s0, t1, &c01)) {
            *color = bilerp_3tap_color(c11, c10, c01, 32u - frac_t, 32u - frac_s);
            return true;
        }
    } else {
        if (tmem_fetch_compact_class(tmem, cls, sample,s0, t0, &c00) &&
            tmem_fetch_compact_class(tmem, cls, sample,s1, t0, &c10) &&
            tmem_fetch_compact_class(tmem, cls, sample,s0, t1, &c01)) {
            *color = bilerp_3tap_color(c00, c10, c01, frac_s, frac_t);
            return true;
        }
    }
    return tmem_fetch_compact_class(tmem, cls, sample,s0, t0, color);
}

/* One out-of-line instance per compact class, so the format is decided once
 * per sample rather than once per tap without growing the span loop. */
#define TMEM_COMPACT_BILERP_INSTANCE(name, cls)                                  \
    static __attribute__((noinline)) bool name(                           \
        const tmem_state *tmem, const rdp_texture_sample_state *sample,          \
        int32_t s_fixed, int32_t t_fixed, rdp_color *color)                      \
    {                                                                            \
        return tmem_sample_compact_bilerp_class(tmem, cls, sample,               \
                                                s_fixed, t_fixed, color);        \
    }
TMEM_COMPACT_BILERP_INSTANCE(tmem_sample_i4_bilerp, RDP_SAMPLER_I4_BILERP)
TMEM_COMPACT_BILERP_INSTANCE(tmem_sample_ci4_bilerp, RDP_SAMPLER_CI4_TLUT_BILERP)
TMEM_COMPACT_BILERP_INSTANCE(tmem_sample_i8_bilerp, RDP_SAMPLER_I8_BILERP)
TMEM_COMPACT_BILERP_INSTANCE(tmem_sample_ia8_bilerp, RDP_SAMPLER_IA8_BILERP)
TMEM_COMPACT_BILERP_INSTANCE(tmem_sample_ci8_bilerp, RDP_SAMPLER_CI8_TLUT_BILERP)
#undef TMEM_COMPACT_BILERP_INSTANCE

/* Every class not named falls to the fetch's last branch, the 8-bit palette
 * path. */
static inline bool tmem_sample_compact_bilerp_fixed5(const tmem_state *tmem,
                                                     const rdp_texture_sample_state *sample,
                                                     int32_t s_fixed, int32_t t_fixed,
                                                     rdp_color *color)
{
    switch (sample->sampler_class) {
    case RDP_SAMPLER_I4_BILERP:
        return tmem_sample_i4_bilerp(tmem, sample, s_fixed, t_fixed, color);
    case RDP_SAMPLER_CI4_TLUT_BILERP:
        return tmem_sample_ci4_bilerp(tmem, sample, s_fixed, t_fixed, color);
    case RDP_SAMPLER_I8_BILERP:
        return tmem_sample_i8_bilerp(tmem, sample, s_fixed, t_fixed, color);
    case RDP_SAMPLER_IA8_BILERP:
        return tmem_sample_ia8_bilerp(tmem, sample, s_fixed, t_fixed, color);
    default:
        return tmem_sample_ci8_bilerp(tmem, sample, s_fixed, t_fixed, color);
    }
}

static inline bool tmem_sample_rgba16_bilerp_fixed5(const tmem_state *tmem,
                                                     const rdp_texture_sample_state *sample,
                                                     int32_t s_fixed, int32_t t_fixed,
                                                     rdp_color *color)
{
    uint32_t s0, t0, s1, t1, frac_s, frac_t;
    if (!color || !tmem_resolve_axis_pair_fixed5(sample, s_fixed, true, &s0, &s1, &frac_s) ||
        !tmem_resolve_axis_pair_fixed5(sample, t_fixed, false, &t0, &t1, &frac_t))
        return false;
    rdp_color c00, c10, c01, c11;
    if (sample->mid_texel && frac_s == 16u && frac_t == 16u) {
        if (tmem_fetch_rgba16_local(tmem, sample, s0, t0, &c00) &&
            tmem_fetch_rgba16_local(tmem, sample, s1, t0, &c10) &&
            tmem_fetch_rgba16_local(tmem, sample, s0, t1, &c01) &&
            tmem_fetch_rgba16_local(tmem, sample, s1, t1, &c11)) {
            *color = (rdp_color){
                (uint8_t)(((uint32_t)c00.r + c10.r + c01.r + c11.r + 2u) >> 2),
                (uint8_t)(((uint32_t)c00.g + c10.g + c01.g + c11.g + 2u) >> 2),
                (uint8_t)(((uint32_t)c00.b + c10.b + c01.b + c11.b + 2u) >> 2),
                (uint8_t)(((uint32_t)c00.a + c10.a + c01.a + c11.a + 2u) >> 2)
            };
            return true;
        }
    } else if (frac_s + frac_t >= 32u) {
        if (tmem_fetch_rgba16_local(tmem, sample, s1, t1, &c11) &&
            tmem_fetch_rgba16_local(tmem, sample, s1, t0, &c10) &&
            tmem_fetch_rgba16_local(tmem, sample, s0, t1, &c01)) {
            *color = bilerp_3tap_color(c11, c10, c01, 32u - frac_t, 32u - frac_s);
            return true;
        }
    } else {
        if (tmem_fetch_rgba16_local(tmem, sample, s0, t0, &c00) &&
            tmem_fetch_rgba16_local(tmem, sample, s1, t0, &c10) &&
            tmem_fetch_rgba16_local(tmem, sample, s0, t1, &c01)) {
            *color = bilerp_3tap_color(c00, c10, c01, frac_s, frac_t);
            return true;
        }
    }
    return tmem_fetch_rgba16_local(tmem, sample, s0, t0, color);
}

/* The RGBA16 bilerp with the IA16 decode. A separate copy rather than a shared
 * template, which measurably perturbed the RGBA16 path; out of line like the
 * compact classes, so it does not grow the span loop. */
static __attribute__((noinline)) bool tmem_sample_ia16_bilerp_fixed5(
    const tmem_state *tmem, const rdp_texture_sample_state *sample,
    int32_t s_fixed, int32_t t_fixed, rdp_color *color)
{
    uint32_t s0, t0, s1, t1, frac_s, frac_t;
    if (!color || !tmem_resolve_axis_pair_fixed5(sample, s_fixed, true, &s0, &s1, &frac_s) ||
        !tmem_resolve_axis_pair_fixed5(sample, t_fixed, false, &t0, &t1, &frac_t))
        return false;
    rdp_color c00, c10, c01, c11;
    if (sample->mid_texel && frac_s == 16u && frac_t == 16u) {
        if (tmem_fetch_ia16_local(tmem, sample, s0, t0, &c00) &&
            tmem_fetch_ia16_local(tmem, sample, s1, t0, &c10) &&
            tmem_fetch_ia16_local(tmem, sample, s0, t1, &c01) &&
            tmem_fetch_ia16_local(tmem, sample, s1, t1, &c11)) {
            *color = (rdp_color){
                (uint8_t)(((uint32_t)c00.r + c10.r + c01.r + c11.r + 2u) >> 2),
                (uint8_t)(((uint32_t)c00.g + c10.g + c01.g + c11.g + 2u) >> 2),
                (uint8_t)(((uint32_t)c00.b + c10.b + c01.b + c11.b + 2u) >> 2),
                (uint8_t)(((uint32_t)c00.a + c10.a + c01.a + c11.a + 2u) >> 2)
            };
            return true;
        }
    } else if (frac_s + frac_t >= 32u) {
        if (tmem_fetch_ia16_local(tmem, sample, s1, t1, &c11) &&
            tmem_fetch_ia16_local(tmem, sample, s1, t0, &c10) &&
            tmem_fetch_ia16_local(tmem, sample, s0, t1, &c01)) {
            *color = bilerp_3tap_color(c11, c10, c01, 32u - frac_t, 32u - frac_s);
            return true;
        }
    } else {
        if (tmem_fetch_ia16_local(tmem, sample, s0, t0, &c00) &&
            tmem_fetch_ia16_local(tmem, sample, s1, t0, &c10) &&
            tmem_fetch_ia16_local(tmem, sample, s0, t1, &c01)) {
            *color = bilerp_3tap_color(c00, c10, c01, frac_s, frac_t);
            return true;
        }
    }
    return tmem_fetch_ia16_local(tmem, sample, s0, t0, color);
}

typedef struct tmem_bilerp_footprint {
    rdp_color c00, c10, c01, c11;
    uint8_t frac_s, frac_t;
} tmem_bilerp_footprint;

/* The footprint's texel coordinates, s0/t0 and their neighbours s1/t1, and
 * the two bilerp fractions. */
static inline bool tmem_footprint_coords_fixed5(
    const rdp_texture_sample_state *sample, int32_t s_fixed, int32_t t_fixed,
    uint32_t *s0, uint32_t *t0, uint32_t *s1, uint32_t *t1,
    uint8_t *frac_s, uint8_t *frac_t)
{
    uint32_t fs, ft;
    tmem_resolve_axis_pair_fixed5(sample, s_fixed, true, s0, s1, &fs);
    tmem_resolve_axis_pair_fixed5(sample, t_fixed, false, t0, t1, &ft);
    *frac_s = (uint8_t)fs;
    *frac_t = (uint8_t)ft;
    return true;
}

/* Resolve and fetch the texture-unit footprint once. Normal bilerp and the
 * coupled convert_one cycle differ only in how these four taps are reduced. */
static inline bool tmem_fetch_bilerp_footprint_fixed5(
    const tmem_state *tmem, const rdp_texture_sample_state *sample,
    int32_t s_fixed, int32_t t_fixed, tmem_bilerp_footprint *footprint)
{
    uint32_t local_s, local_t, local_s1, local_t1;
    return footprint &&
           tmem_footprint_coords_fixed5(sample, s_fixed, t_fixed,
                                        &local_s, &local_t, &local_s1, &local_t1,
                                        &footprint->frac_s, &footprint->frac_t) &&
           tmem_fetch_color_local(tmem, sample, local_s, local_t,
                                  &footprint->c00) &&
           tmem_fetch_color_local(tmem, sample, local_s1, local_t,
                                  &footprint->c10) &&
           tmem_fetch_color_local(tmem, sample, local_s, local_t1,
                                  &footprint->c01) &&
           tmem_fetch_color_local(tmem, sample, local_s1, local_t1,
                                  &footprint->c11);
}

/* The palette index a TLUT tile's texel carries: its nibble under the tile's
 * palette at 4bpp, its byte at 8bpp, the upper byte of its 16-bit word above.
 * The texel comes from lower TMEM only. */
static inline bool tmem_tlut_index_local(const tmem_state *tmem,
                                         const rdp_texture_sample_state *sample,
                                         uint32_t local_s, uint32_t local_t,
                                         uint32_t *index)
{
    tmem_texel_address address;
    if (!tmem_resolve_texel_address_raw(&sample->tile, sample->stride,
                                        local_s, local_t, &address))
        return false;
    const uint32_t byte = tmem_tlut_texel_byte(address.byte);
    switch (sample->tile.size) {
    case RDP_SIZE_4BPP: {
        const uint8_t packed = tmem->bytes[byte];
        *index = ((uint32_t)sample->tile.palette << 4) |
                 (address.subtexel ? (packed & 0xfu) : (packed >> 4));
        return true;
    }
    case RDP_SIZE_8BPP:
        *index = tmem->bytes[byte];
        return true;
    default:
        *index = (uint32_t)tmem_read_native16(tmem, byte) >> 8;
        return true;
    }
}

/* Palette entry `index` as TMEM bank `bank` holds it. The four copies of an
 * entry are the halfwords of one TMEM word; the upper triangle of the bilerp
 * footprint reads the bank lanes crossed. */
static inline rdp_color tmem_tlut_bank_entry(const tmem_state *tmem,
                                             const rdp_texture_sample_state *sample,
                                             uint32_t index, uint32_t bank,
                                             bool upper)
{
    const uint32_t lane = (((index << 2) + bank) ^ (upper ? 2u : 1u)) & 0x3ffu;
    if (sample->tlut_ia) {
        const uint16_t entry = tmem->words[0x400u + lane];
        const uint8_t intensity = (uint8_t)(entry >> 8);
        return (rdp_color){ intensity, intensity, intensity, (uint8_t)entry };
    }
    return tmem->palette_lanes[lane];
}

/*
 * A paletted sample whose palette copies differ between the four TMEM banks.
 * Tap t00 reads bank 0, t10 bank 1, t01 bank 2 and t11 bank 3, and the upper
 * triangle's base tap is t11. Without sample_quad all four taps are the texel
 * at s0/t0, still read across the banks and filtered.
 */
static inline bool tmem_sample_tlut_banks_fixed5(const tmem_state *tmem,
                                                 const rdp_texture_sample_state *sample,
                                                 int32_t s_fixed, int32_t t_fixed,
                                                 rdp_color *color)
{
    uint32_t s0, t0, s1, t1;
    uint8_t frac_s, frac_t;
    if (!tmem_footprint_coords_fixed5(sample, s_fixed, t_fixed, &s0, &t0, &s1, &t1,
                                      &frac_s, &frac_t))
        return false;
    if (!sample->sample_quad) {
        s1 = s0;
        t1 = t0;
    }
    const bool upper = (uint32_t)frac_s + frac_t >= 32u;
    const bool center = sample->mid_texel && sample->bilerp &&
                        frac_s == 16u && frac_t == 16u;
    const bool upper_base = upper && !center;

    uint32_t index;
    if (!tmem_tlut_index_local(tmem, sample, upper_base ? s1 : s0,
                               upper_base ? t1 : t0, &index))
        return false;
    const rdp_color base = tmem_tlut_bank_entry(tmem, sample, index,
                                                upper_base ? 3u : 0u, upper);
    if (!sample->bilerp) {
        *color = base;
        return true;
    }

    uint32_t index10, index01;
    if (!tmem_tlut_index_local(tmem, sample, s1, t0, &index10) ||
        !tmem_tlut_index_local(tmem, sample, s0, t1, &index01))
        return false;
    const rdp_color c10 = tmem_tlut_bank_entry(tmem, sample, index10, 1u, upper);
    const rdp_color c01 = tmem_tlut_bank_entry(tmem, sample, index01, 2u, upper);
    if (center) {
        uint32_t index11;
        if (!tmem_tlut_index_local(tmem, sample, s1, t1, &index11))
            return false;
        const rdp_color c11 = tmem_tlut_bank_entry(tmem, sample, index11, 3u, upper);
        *color = (rdp_color){
            (uint8_t)(((uint32_t)base.r + c10.r + c01.r + c11.r + 2u) >> 2),
            (uint8_t)(((uint32_t)base.g + c10.g + c01.g + c11.g + 2u) >> 2),
            (uint8_t)(((uint32_t)base.b + c10.b + c01.b + c11.b + 2u) >> 2),
            (uint8_t)(((uint32_t)base.a + c10.a + c01.a + c11.a + 2u) >> 2)
        };
    } else if (upper_base) {
        *color = bilerp_3tap_color(base, c10, c01, 32u - frac_t, 32u - frac_s);
    } else {
        *color = bilerp_3tap_color(base, c10, c01, frac_s, frac_t);
    }
    return true;
}

/* The palette index of a 4bpp or 8bpp TLUT texel, with the address folded as
 * tmem_fetch_compact_class does. */
static inline __attribute__((always_inline)) uint32_t tmem_compact_tlut_index(
    const tmem_state *tmem, const rdp_texture_sample_state *sample,
    bool four_bit, uint32_t local_s, uint32_t local_t)
{
    const uint32_t row_xor = (local_t & 1u) ? 4u : 0u;
    const uint8_t packed = tmem->bytes[tmem_tlut_texel_byte(tmem_physical_byte(
        (sample->tile.tmem + local_t * sample->stride +
         ((four_bit ? local_s >> 1 : local_s) ^ row_xor)) & 0xfffu))];
    if (!four_bit) return packed;
    return ((uint32_t)sample->tile.palette << 4) |
           ((local_s & 1u) ? (packed & 0xfu) : (packed >> 4));
}

/* tmem_sample_tlut_banks_fixed5 for its common shape: a 4bpp or 8bpp index
 * filtered over a quad, with compiled axes. */
static inline bool tmem_sample_tlut_banks_quad_fixed5(const tmem_state *tmem,
                                                      const rdp_texture_sample_state *sample,
                                                      int32_t s_fixed, int32_t t_fixed,
                                                      rdp_color *color)
{
    uint32_t s0, t0, s1, t1, frac_s, frac_t;
    tmem_resolve_axis_pair_fixed5(sample, s_fixed, true, &s0, &s1, &frac_s);
    tmem_resolve_axis_pair_fixed5(sample, t_fixed, false, &t0, &t1, &frac_t);
    const bool four_bit = sample->tile.size == RDP_SIZE_4BPP;
    const bool upper = frac_s + frac_t >= 32u;
    const bool center = sample->mid_texel && frac_s == 16u && frac_t == 16u;
    const bool upper_base = upper && !center;

    const rdp_color base = tmem_tlut_bank_entry(tmem, sample,
        tmem_compact_tlut_index(tmem, sample, four_bit,
                                upper_base ? s1 : s0, upper_base ? t1 : t0),
        upper_base ? 3u : 0u, upper);
    const rdp_color c10 = tmem_tlut_bank_entry(tmem, sample,
        tmem_compact_tlut_index(tmem, sample, four_bit, s1, t0), 1u, upper);
    const rdp_color c01 = tmem_tlut_bank_entry(tmem, sample,
        tmem_compact_tlut_index(tmem, sample, four_bit, s0, t1), 2u, upper);
    if (center) {
        const rdp_color c11 = tmem_tlut_bank_entry(tmem, sample,
            tmem_compact_tlut_index(tmem, sample, four_bit, s1, t1), 3u, upper);
        *color = (rdp_color){
            (uint8_t)(((uint32_t)base.r + c10.r + c01.r + c11.r + 2u) >> 2),
            (uint8_t)(((uint32_t)base.g + c10.g + c01.g + c11.g + 2u) >> 2),
            (uint8_t)(((uint32_t)base.b + c10.b + c01.b + c11.b + 2u) >> 2),
            (uint8_t)(((uint32_t)base.a + c10.a + c01.a + c11.a + 2u) >> 2)
        };
    } else if (upper_base) {
        *color = bilerp_3tap_color(base, c10, c01, 32u - frac_t, 32u - frac_s);
    } else {
        *color = bilerp_3tap_color(base, c10, c01, frac_s, frac_t);
    }
    return true;
}

static inline bool tmem_sample_bilerp_compiled_fixed5(
    const tmem_state *tmem, const rdp_texture_sample_state *sample,
    int32_t s_fixed, int32_t t_fixed, rdp_color *color)
{
    /* IA16 is tested here rather than in tmem_sample_color_fixed5_raw, which
     * is inlined into the span loop: a test there changed the loop's code
     * and cost ~1.5% on dumps that never sample IA16. */
    if (sample->sampler_class == RDP_SAMPLER_IA16_BILERP)
        return tmem_sample_ia16_bilerp_fixed5(tmem, sample, s_fixed, t_fixed, color);
    tmem_bilerp_footprint f;
    if (!color || !tmem_fetch_bilerp_footprint_fixed5(
            tmem, sample, s_fixed, t_fixed, &f)) {
        uint32_t local_s, local_t;
        return color && tmem_resolve_compiled_coord_fixed5(
                   sample, s_fixed, t_fixed, &local_s, &local_t) &&
               tmem_fetch_color_local(tmem, sample, local_s, local_t, color);
    }
    if (sample->mid_texel && f.frac_s == 16u && f.frac_t == 16u) {
        *color = (rdp_color){
            (uint8_t)(((uint32_t)f.c00.r + f.c10.r + f.c01.r + f.c11.r + 2u) >> 2),
            (uint8_t)(((uint32_t)f.c00.g + f.c10.g + f.c01.g + f.c11.g + 2u) >> 2),
            (uint8_t)(((uint32_t)f.c00.b + f.c10.b + f.c01.b + f.c11.b + 2u) >> 2),
            (uint8_t)(((uint32_t)f.c00.a + f.c10.a + f.c01.a + f.c11.a + 2u) >> 2)
        };
    } else if ((uint32_t)f.frac_s + f.frac_t >= 32u) {
        *color = bilerp_3tap_color(f.c11, f.c10, f.c01,
                                  32u - f.frac_t, 32u - f.frac_s);
    } else {
        *color = bilerp_3tap_color(f.c00, f.c10, f.c01,
                                  f.frac_s, f.frac_t);
    }
    return true;
}

/* The generic point path for RDP_SAMPLER_TLUT_POINT, without the format
 * dispatch: a 4/8bpp texel is always a palette index there. */
static inline bool tmem_sample_tlut_point_fixed5(const tmem_state *tmem,
                                                 const rdp_texture_sample_state *sample,
                                                 int32_t s_fixed, int32_t t_fixed,
                                                 rdp_color *color)
{
    uint32_t local_s, local_t;
    tmem_texel_address address;
    if (!tmem_resolve_compiled_coord_fixed5(sample, s_fixed, t_fixed, &local_s, &local_t) ||
        !tmem_resolve_texel_address_raw(&sample->tile, sample->stride,
                                        local_s, local_t, &address))
        return false;
    const uint8_t packed = tmem->bytes[tmem_tlut_texel_byte(address.byte)];
    const uint32_t index = sample->tile.size == RDP_SIZE_4BPP
        ? (((uint32_t)sample->tile.palette << 4) |
           (address.subtexel ? (packed & 0xfu) : (packed >> 4)))
        : packed;
    *color = tmem_palette_entry(tmem, sample->tlut_ia, index);
    return true;
}

static inline bool tmem_sample_color_fixed5_raw(const tmem_state *tmem, const rdp_texture_sample_state *sample, int32_t s_fixed, int32_t t_fixed, rdp_color *color)
{
    if (!color || !sample || sample->tile_index >= 8) return false;
    if (sample->width && sample->height) {
        if (sample->sampler_class == RDP_SAMPLER_TLUT_POINT)
            return tmem_sample_tlut_point_fixed5(tmem, sample, s_fixed, t_fixed, color);
        if (sample->sampler_class == RDP_SAMPLER_RGBA16_BILERP)
            return tmem_sample_rgba16_bilerp_fixed5(tmem, sample, s_fixed, t_fixed, color);
        if (sample->sampler_class == RDP_SAMPLER_RGBA16_POINT)
            return tmem_sample_rgba16_point_fixed5(tmem, sample, s_fixed, t_fixed, color);
        if (sample->sampler_class == RDP_SAMPLER_I4_BILERP ||
            sample->sampler_class == RDP_SAMPLER_CI4_TLUT_BILERP ||
            sample->sampler_class == RDP_SAMPLER_CI8_TLUT_BILERP ||
            sample->sampler_class == RDP_SAMPLER_I8_BILERP ||
            sample->sampler_class == RDP_SAMPLER_IA8_BILERP)
            return tmem_sample_compact_bilerp_fixed5(tmem, sample, s_fixed, t_fixed, color);
        return sample->bilerp && sample->sample_quad
            ? tmem_sample_bilerp_compiled_fixed5(tmem, sample, s_fixed, t_fixed, color)
            : tmem_sample_point_compiled_fixed5(tmem, sample, s_fixed, t_fixed, color);
    }
    uint32_t local_s, local_t;
    if (!tmem_resolve_texel_coord_fixed5(tmem, sample, s_fixed, t_fixed, &local_s, &local_t)) return false;
    return tmem_fetch_color_local(tmem, sample, local_s, local_t, color);
}

/* One plane of the three-tap filter on signed values: the lower triangle based
 * on t00, the upper one on t11. */
static inline int32_t tmem_bilerp_3tap_plane(int32_t t00, int32_t t10, int32_t t01,
                                             int32_t t11, uint32_t frac_s,
                                             uint32_t frac_t)
{
    const bool upper = frac_s + frac_t >= 32u;
    const int32_t base = upper ? t11 : t00;
    const int32_t weight_s = (int32_t)(upper ? 32u - frac_t : frac_s);
    const int32_t weight_t = (int32_t)(upper ? 32u - frac_s : frac_t);
    return base + (((t10 - base) * weight_s + (t01 - base) * weight_t + 0x10) >> 5);
}

/* Without bilerp, a quad footprint still decides which texel the unit hands
 * on: the base of the filter triangle, t11 in the upper one. */
static inline bool tmem_sample_quad_base_fixed5(const tmem_state *tmem,
                                                const rdp_texture_sample_state *sample,
                                                int32_t s_fixed, int32_t t_fixed,
                                                rdp_color *color)
{
    uint32_t s0, s1, t0, t1, frac_s, frac_t;
    tmem_resolve_axis_pair_fixed5(sample, s_fixed, true, &s0, &s1, &frac_s);
    tmem_resolve_axis_pair_fixed5(sample, t_fixed, false, &t0, &t1, &frac_t);
    const bool upper = frac_s + frac_t >= 32u;
    return tmem_fetch_color_local(tmem, sample, upper ? s1 : s0, upper ? t1 : t0,
                                  color);
}

/*
 * YUV16 over a quad footprint. A texel pair shares its chroma, so the second
 * column reads chroma column (2 s1 - s0) / 2, and chroma filters with an S
 * fraction of its own that carries the position within the pair. Luma and
 * chroma choose their filter triangle - or, without bilerp, their base texel -
 * separately.
 */
static inline bool tmem_sample_yuv_quad_fixed5(const tmem_state *tmem,
                                               const rdp_texture_sample_state *sample,
                                               int32_t s_fixed, int32_t t_fixed,
                                               rdp_color *color)
{
    uint32_t s0, s1, t0, t1, frac_s, frac_t;
    tmem_resolve_axis_pair_fixed5(sample, s_fixed, true, &s0, &s1, &frac_s);
    tmem_resolve_axis_pair_fixed5(sample, t_fixed, false, &t0, &t1, &frac_t);
    const uint32_t chroma0 = s0 >> 1;
    const uint32_t chroma1 = (uint32_t)((int32_t)(2u * s1 - s0) >> 1);
    const uint32_t chroma_frac = ((s0 & 1u) << 4) | (frac_s >> 1);
    rdp_color c00, c10, c01, c11;
    tmem_fetch_yuv_local(tmem, sample, s0, t0, chroma0, &c00);
    tmem_fetch_yuv_local(tmem, sample, s1, t0, chroma1, &c10);
    tmem_fetch_yuv_local(tmem, sample, s0, t1, chroma0, &c01);
    tmem_fetch_yuv_local(tmem, sample, s1, t1, chroma1, &c11);
    int32_t u, v, y;
    if (!sample->bilerp) {
        const rdp_color *chroma = chroma_frac + frac_t >= 32u ? &c11 : &c00;
        const rdp_color *luma = frac_s + frac_t >= 32u ? &c11 : &c00;
        u = (int8_t)chroma->r;
        v = (int8_t)chroma->g;
        y = luma->b;
    } else {
        if (sample->mid_texel && chroma_frac == 16u && frac_t == 16u) {
            u = ((int8_t)c00.r + (int8_t)c10.r + (int8_t)c01.r + (int8_t)c11.r + 2) >> 2;
            v = ((int8_t)c00.g + (int8_t)c10.g + (int8_t)c01.g + (int8_t)c11.g + 2) >> 2;
        } else {
            u = tmem_bilerp_3tap_plane((int8_t)c00.r, (int8_t)c10.r, (int8_t)c01.r,
                                       (int8_t)c11.r, chroma_frac, frac_t);
            v = tmem_bilerp_3tap_plane((int8_t)c00.g, (int8_t)c10.g, (int8_t)c01.g,
                                       (int8_t)c11.g, chroma_frac, frac_t);
        }
        if (sample->mid_texel && frac_s == 16u && frac_t == 16u)
            y = (c00.b + c10.b + c01.b + c11.b + 2) >> 2;
        else
            y = tmem_bilerp_3tap_plane(c00.b, c10.b, c01.b, c11.b, frac_s, frac_t);
    }
    *color = (rdp_color){ (uint8_t)u, (uint8_t)v, (uint8_t)y, (uint8_t)y };
    return true;
}

/* The texel fetch of the full texture unit: palettes read per TMEM bank where
 * the draw's palette copies differ, the ordinary fetch otherwise. */
static inline bool tmem_sample_color_full_fixed5(const tmem_state *tmem,
                                                 const rdp_texture_sample_state *sample,
                                                 int32_t s_fixed, int32_t t_fixed,
                                                 rdp_color *color)
{
    if (sample->sampler_class == RDP_SAMPLER_TLUT_BANKS)
        return sample->axes_ready && sample->bilerp && sample->sample_quad &&
               sample->tile.size <= RDP_SIZE_8BPP
            ? tmem_sample_tlut_banks_quad_fixed5(tmem, sample, s_fixed, t_fixed, color)
            : tmem_sample_tlut_banks_fixed5(tmem, sample, s_fixed, t_fixed, color);
    if (sample->sample_quad && sample->axes_ready) {
        if (sample->tile.format == RDP_FORMAT_YUV &&
            sample->tile.size == RDP_SIZE_16BPP)
            return tmem_sample_yuv_quad_fixed5(tmem, sample, s_fixed, t_fixed, color);
        if (!sample->bilerp)
            return tmem_sample_quad_base_fixed5(tmem, sample, s_fixed, t_fixed, color);
    }
    return tmem_sample_color_fixed5_raw(tmem, sample, s_fixed, t_fixed, color);
}

static inline int32_t tmem_sign_extend_9(uint32_t value)
{
    value &= 0x1ffu;
    return (int32_t)((value ^ 0x100u) - 0x100u);
}

static inline uint8_t tmem_color_component(const rdp_color *color,
                                           uint32_t component)
{
    return component == 0u ? color->r : component == 1u ? color->g :
           component == 2u ? color->b : color->a;
}

/* The convert_one cycle-1 sample, from the cycle-0 texel pr/pg/pb in its nine
 * combiner bits. convert_one is a reduction mode for the shared bilerp
 * footprint: cycle-0 RGB replaces the ordinary S/T weights, so the two samples
 * form one coupled texture-pipeline operation rather than two independent
 * sampler calls. */
static inline bool tmem_cycle1_convert_one(
    const tmem_state *tmem, const rdp_texture_sample_state *sample,
    int32_t s_fixed, int32_t t_fixed, int32_t pr, int32_t pg, int32_t pb,
    uint16_t out[4])
{
    if (!sample->bilerp) {
        out[0] = (uint16_t)(pb + ((sample->convert_k0_tf * pg + 0x80) >> 8)) & 0x1ffu;
        out[1] = (uint16_t)(pb + ((sample->convert_k1_tf * pr +
                                   sample->convert_k2_tf * pg + 0x80) >> 8)) & 0x1ffu;
        out[2] = (uint16_t)(pb + ((sample->convert_k3_tf * pr + 0x80) >> 8)) & 0x1ffu;
        out[3] = (uint16_t)pb & 0x1ffu;
        return true;
    }

    /* With no quad footprint the dynamic factors have no differences to
     * weight; every component is the cycle-0 blue value. */
    if (!sample->sample_quad) {
        for (uint32_t component = 0; component < 4u; component++)
            out[component] = (uint16_t)pb & 0x1ffu;
        return true;
    }

    /* YUV takes its chroma from the pair's shared column and picks the chroma
     * plane's triangle and centre with the chroma fraction, as the YUV quad
     * sample does; luma keeps the ordinary ones. */
    if (sample->tile.format == RDP_FORMAT_YUV && sample->tile.size == RDP_SIZE_16BPP) {
        uint32_t s0, s1, t0, t1, frac_s, frac_t;
        tmem_resolve_axis_pair_fixed5(sample, s_fixed, true, &s0, &s1, &frac_s);
        tmem_resolve_axis_pair_fixed5(sample, t_fixed, false, &t0, &t1, &frac_t);
        const uint32_t chroma0 = s0 >> 1;
        const uint32_t chroma1 = (uint32_t)((int32_t)(2u * s1 - s0) >> 1);
        const uint32_t chroma_frac = ((s0 & 1u) << 4) | (frac_s >> 1);
        rdp_color q[4];
        tmem_fetch_yuv_local(tmem, sample, s0, t0, chroma0, &q[0]);
        tmem_fetch_yuv_local(tmem, sample, s1, t0, chroma1, &q[1]);
        tmem_fetch_yuv_local(tmem, sample, s0, t1, chroma0, &q[2]);
        tmem_fetch_yuv_local(tmem, sample, s1, t1, chroma1, &q[3]);
        const bool center_rg = sample->mid_texel && chroma_frac == 16u && frac_t == 16u;
        const bool center_ba = sample->mid_texel && frac_s == 16u && frac_t == 16u;
        const bool upper_rg = !center_rg && chroma_frac + frac_t >= 32u;
        const bool upper_ba = !center_ba && frac_s + frac_t >= 32u;
        for (uint32_t component = 0; component < 4u; component++) {
            const bool chroma = component < 2u;
            int32_t c[4];
            for (uint32_t tap = 0; tap < 4u; tap++) {
                const uint8_t raw = tmem_color_component(&q[tap], component);
                c[tap] = chroma ? (int8_t)raw : raw;
            }
            const bool center = chroma ? center_rg : center_ba;
            const bool upper = chroma ? upper_rg : upper_ba;
            int32_t reduced;
            if (center) {
                reduced = pr * (c[2] - c[3]) + pg * (c[1] - c[3]) + ((c[0] - c[3]) << 6);
            } else {
                const int32_t base = upper ? c[3] : c[0];
                reduced = (upper ? pg : pr) * (c[1] - base) +
                          (upper ? pr : pg) * (c[2] - base);
            }
            out[component] = (uint16_t)(pb + ((reduced + 0x80) >> 8)) & 0x1ffu;
        }
        return true;
    }

    tmem_bilerp_footprint f;
    if (!tmem_fetch_bilerp_footprint_fixed5(
            tmem, sample, s_fixed, t_fixed, &f))
        return false;
    const bool center = sample->mid_texel &&
                        f.frac_s == 16u && f.frac_t == 16u;
    const bool upper = (uint32_t)f.frac_s + f.frac_t >= 32u;
    for (uint32_t component = 0; component < 4u; component++) {
        const int32_t c00 = tmem_color_component(&f.c00, component);
        const int32_t c10 = tmem_color_component(&f.c10, component);
        const int32_t c01 = tmem_color_component(&f.c01, component);
        const int32_t c11 = tmem_color_component(&f.c11, component);
        int32_t reduced;
        if (center) {
            reduced = pr * (c01 - c11) + pg * (c10 - c11) +
                      ((c00 - c11) << 6);
        } else {
            const int32_t base = upper ? c11 : c00;
            const int32_t weight_s = upper ? pg : pr;
            const int32_t weight_t = upper ? pr : pg;
            reduced = weight_s * (c10 - base) +
                      weight_t * (c01 - base);
        }
        const int32_t value = pb + ((reduced + 0x80) >> 8);
        out[component] = (uint16_t)value & 0x1ffu;
    }
    return true;
}

static inline bool tmem_sample_color_cycle1_fixed5(
    const tmem_state *tmem, const rdp_texture_sample_state *sample,
    int32_t s_fixed, int32_t t_fixed, const rdp_color *previous,
    uint16_t out[4])
{
    if (!out || !previous) return false;
    if (!sample->convert_one) {
        rdp_color color;
        if (!tmem_sample_color_fixed5_raw(tmem, sample, s_fixed, t_fixed, &color))
            return false;
        out[0] = color.r; out[1] = color.g;
        out[2] = color.b; out[3] = color.a;
        return true;
    }
    return tmem_cycle1_convert_one(tmem, sample, s_fixed, t_fixed,
                                   tmem_sign_extend_9(previous->r),
                                   tmem_sign_extend_9(previous->g),
                                   tmem_sign_extend_9(previous->b), out);
}

/* The cycle-1 sample of the full texture unit, whose texels are nine bits
 * wide on both sides. */
static inline bool tmem_sample_color_cycle1_wide(
    const tmem_state *tmem, const rdp_texture_sample_state *sample,
    int32_t s_fixed, int32_t t_fixed, const uint16_t previous[4],
    uint16_t out[4])
{
    if (!sample->convert_one) {
        rdp_color color;
        if (!tmem_sample_color_full_fixed5(tmem, sample, s_fixed, t_fixed, &color))
            return false;
        if (sample->texel_output != RDP_TEXEL_PLAIN) {
            tmem_texel_output(sample, color, out);
        } else {
            out[0] = color.r; out[1] = color.g;
            out[2] = color.b; out[3] = color.a;
        }
        return true;
    }
    return tmem_cycle1_convert_one(tmem, sample, s_fixed, t_fixed,
                                   tmem_sign_extend_9(previous[0]),
                                   tmem_sign_extend_9(previous[1]),
                                   tmem_sign_extend_9(previous[2]), out);
}

/* A texel as fetched, before tmem_texel_output. */
static inline bool tmem_sample_color_fixed5(const tmem_state *tmem, const rdp_texture_sample_state *sample, int32_t s_fixed, int32_t t_fixed, rdp_color *color)
{
    return tmem_sample_color_fixed5_raw(tmem, sample, s_fixed, t_fixed, color);
}

#endif
