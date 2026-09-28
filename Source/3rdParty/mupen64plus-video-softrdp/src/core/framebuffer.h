#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "rdp_memory.h"
#include "rdp_state.h"
#include "sr_scale.h"

typedef struct rdp_memory_pixel {
    rdp_color color;
    uint8_t coverage;
} rdp_memory_pixel;

/*
 * Where a pixel lives: an RDRAM address, already wrapped by mask_addr, at every
 * scale. Which SAMPLE of that pixel is a separate value, and the two together
 * pick a byte - see sr_sample_of and the _sample accessors.
 *
 * FB_SLOT_NONE marks a target that cannot be written at all (no RDRAM).
 */
#define FB_SLOT_NONE 0xffffffffu

static inline uint32_t framebuffer_pixel_bytes(rdp_texture_size size)
{
    if (size <= RDP_SIZE_8BPP) return 1u;
    return 1u << ((uint32_t)size - 1u);
}

/*
 * These take raster coordinates, which count scaled pixels. The frame buffer
 * pixel index derived from them is the HARDWARE pixel: it drives the 8bpp lane
 * split and the fill colour's halfword selection, both of which are properties
 * of the hardware pixel and not of the sample inside it.
 */
static inline uint32_t framebuffer_raster_pixel(const rdp_framebuffer_state *state,
                                                uint32_t x, uint32_t y)
{
    return sr_raster_to_pixel(y) * state->color_image.width + sr_raster_to_pixel(x);
}

static inline uint32_t framebuffer_slot(const sr_memory *memory,
                                        const rdp_framebuffer_state *state,
                                        uint32_t x, uint32_t y)
{
    if (!memory->rdram) return FB_SLOT_NONE;
    return mask_addr(memory, state->color_image.address +
                     framebuffer_raster_pixel(state, x, y) *
                     framebuffer_pixel_bytes(state->color_image.size));
}

static inline sr_result framebuffer_read_memory_address(sr_memory *memory,
                                                         rdp_texture_size size,
                                                         uint32_t slot,
                                                         bool image_read,
                                                         uint32_t sample,
                                                         rdp_memory_pixel *pixel)
{
    if (!pixel) return SR_ERROR_INVALID_ARGUMENT;
    pixel->color = (rdp_color){0, 0, 0, 0xe0u};
    pixel->coverage = 7u;
    if (!image_read || slot == FB_SLOT_NONE) return SR_OK;
    if (size == RDP_SIZE_8BPP) {
        const uint8_t value = sr_memory_read_u8_sample(memory, slot, sample);
        pixel->color = (rdp_color){value, value, value, 0xe0u};
        pixel->coverage = 7u;
    } else if (size == RDP_SIZE_16BPP) {
        const uint16_t value = sr_memory_read_be16_sample(memory, slot, sample);
        const uint8_t hidden =
            sr_memory_read_hidden_sample(memory, slot, value, sample);
        /* Coverage bit 2 rides in the stored LSB, bits 1:0 in the hidden
         * plane. Reading only the LSB would round a three-bit coverage to
         * "0 or 7", which misfires the blender's overflow test on edges. */
        const uint8_t coverage = (uint8_t)(((value & 1u) << 2) | hidden);
        /* The blender sees the stored five color bits in the high bits of
         * each channel; unlike texture decode, framebuffer read does not
         * replicate them into the low three bits. */
        pixel->color = (rdp_color){
            (uint8_t)((value >> 8) & 0xf8u),
            (uint8_t)((value >> 3) & 0xf8u),
            (uint8_t)((value << 2) & 0xf8u),
            (uint8_t)(coverage << 5)
        };
        pixel->coverage = coverage;
    } else if (size == RDP_SIZE_32BPP) {
        const uint32_t value = sr_memory_read_be32_sample(memory, slot, sample);
        pixel->color = (rdp_color){(uint8_t)(value >> 24), (uint8_t)(value >> 16),
                                   (uint8_t)(value >> 8), (uint8_t)value & 0xe0u};
        pixel->coverage = (pixel->color.a >> 5) & 7u;
    } else if (size == RDP_STORE_IA16) {
        /* Intensity in the high byte stands for all three colours; coverage
         * sits in bits 7:5 and the hidden bits carry nothing. */
        const uint16_t value = sr_memory_read_be16_sample(memory, slot, sample);
        const uint8_t intensity = (uint8_t)(value >> 8);
        const uint8_t coverage = (uint8_t)((value >> 5) & 7u);
        pixel->color = (rdp_color){ intensity, intensity, intensity,
                                    (uint8_t)(coverage << 5) };
        pixel->coverage = coverage;
    } else return SR_ERROR_UNSUPPORTED;
    return SR_OK;
}

static inline sr_result framebuffer_write_color_address(sr_memory *memory,
                                                         rdp_texture_size size,
                                                         uint32_t slot,
                                                         uint32_t pixel_index,
                                                         uint32_t sample,
                                                         rdp_color color)
{
    if (slot == FB_SLOT_NONE) return SR_OK;
    switch (size) {
    case RDP_SIZE_8BPP: {
        const uint8_t value = (pixel_index & 1u) ? color.g : color.r;
        sr_memory_write_u8_sample(memory, slot, value, sample);
        if (pixel_index & 1u)
            sr_memory_write_hidden_sample(memory, slot,
                                          (value & 1u) ? 3u : 0u, sample);
        return SR_OK;
    }
    case RDP_SIZE_16BPP:
        /* rgba5551 packing already carries coverage bit 2 in the LSB, since
         * the caller hands us alpha as (coverage << 5) | 0x1f. */
        sr_memory_write_be16_sample(memory, slot,
                                    rdp_color_to_rgba5551(color), sample);
        sr_memory_write_hidden_sample(memory, slot,
                                      (uint8_t)((color.a >> 5) & 3u), sample);
        return SR_OK;
    case RDP_SIZE_32BPP:
        /* 32bpp stores coverage in the top three alpha bits and leaves the rest
         * zero. The caller's alpha is (coverage << 5) | 0x1f, where the low bits
         * only exist to carry coverage bit 2 into the 5551 LSB; storing them
         * here would put 0x1f in every written pixel's alpha byte. The read side
         * masks with the same 0xe0. */
        color.a &= 0xe0u;
        sr_memory_write_be32_sample(memory, slot,
                                    rdp_color_to_rgba8888(color), sample);
        /* The hidden bits of the first halfword copy the LSB of green, the
         * bit a 16bpp view of it would read as coverage; the second's clear. */
        sr_memory_write_hidden_sample(memory, slot, (color.g & 1u) ? 3u : 0u, sample);
        sr_memory_write_hidden_sample(memory, slot + 2u, 0u, sample);
        return SR_OK;
    default:
        if (size != RDP_STORE_IA16) return SR_ERROR_UNSUPPORTED;
        /* IA stores the red blender output as intensity and the coverage in
         * bits 7:5 of the low byte; the hidden bits clear. */
        sr_memory_write_be16_sample(memory, slot,
                                    (uint16_t)(((uint32_t)color.r << 8) | (color.a & 0xe0u)),
                                    sample);
        sr_memory_write_hidden_sample(memory, slot, 0u, sample);
        return SR_OK;
    }
}

#include <stdbool.h>
#include <stdint.h>

/* Fills only the scanlines owned by this worker (absolute line y where
 * y % worker_stride == worker_offset). Pass offset 0 / stride 1 to fill all.
 * Takes raster coordinates. */
sr_result framebuffer_fill_rect(sr_memory *memory,
                                const rdp_framebuffer_state *state,
                                uint32_t x0,
                                uint32_t y0,
                                uint32_t x1,
                                uint32_t y1,
                                uint32_t worker_offset,
                                uint32_t worker_stride);

/*
 * Colour and coverage for one 16bpp pixel.
 *
 * Copy and fill do not run the coverage unit: the stored LSB is replicated into
 * the hidden bits, so the pixel reads back as fully covered or not at all. That
 * is why this exists alongside framebuffer_write_color_address, which takes its
 * coverage from the blended alpha instead.
 */
static inline bool framebuffer_store_rgba5551(sr_memory *memory, uint32_t slot,
                                              uint16_t value, uint32_t sample)
{
    if (slot == FB_SLOT_NONE) return false;
    sr_memory_write_be16_sample(memory, slot, value, sample);
    sr_memory_write_hidden_sample(memory, slot, (value & 1u) ? 3u : 0u, sample);
    return true;
}

static inline uint32_t framebuffer_load_raw(const sr_memory *memory,
                                            rdp_texture_size size, uint32_t slot,
                                            uint32_t sample)
{
    if (slot == FB_SLOT_NONE) return 0u;
    if (size == RDP_SIZE_32BPP) return sr_memory_read_be32_sample(memory, slot, sample);
    if (size == RDP_SIZE_8BPP) return sr_memory_read_u8_sample(memory, slot, sample);
    return sr_memory_read_be16_sample(memory, slot, sample);
}

/*
 * Every colour write goes through here rather than sr_memory_write_*_fast.
 * A direct RDRAM store reaches sample 0 only, so at a scale above 1 it leaves
 * the other samples of the pixel holding whatever was there before. With a
 * constant `size` this folds to the same single store, so routing through it
 * costs nothing.
 */
static inline void framebuffer_store_raw(sr_memory *memory, rdp_texture_size size,
                                         uint32_t slot, uint32_t value,
                                         uint32_t sample)
{
    if (slot == FB_SLOT_NONE) return;
    if (size == RDP_SIZE_32BPP) sr_memory_write_be32_sample(memory, slot, value, sample);
    else if (size == RDP_SIZE_8BPP)
        sr_memory_write_u8_sample(memory, slot, (uint8_t)value, sample);
    else sr_memory_write_be16_sample(memory, slot, (uint16_t)value, sample);
}

static inline bool write_fill_pixel_16(sr_memory *memory, const rdp_framebuffer_state *state, uint32_t x, uint32_t y)
{
    const uint32_t pixel = framebuffer_raster_pixel(state, x, y);
    const uint16_t value = (pixel & 1u) ? (uint16_t)state->fill_color : (uint16_t)(state->fill_color >> 16);
    return framebuffer_store_rgba5551(memory, framebuffer_slot(memory, state, x, y),
                                      value, sr_sample_of(x, y));
}

static inline bool write_fill_pixel_32(sr_memory *memory, const rdp_framebuffer_state *state, uint32_t x, uint32_t y)
{
    const uint32_t slot = framebuffer_slot(memory, state, x, y);
    if (slot == FB_SLOT_NONE) return false;
    const uint32_t sample = sr_sample_of(x, y);
    framebuffer_store_raw(memory, RDP_SIZE_32BPP, slot, state->fill_color,
                          sample);
    sr_memory_write_hidden_sample(memory, slot,
        (state->fill_color & 0x00010000u) ? 3u : 0u, sample);
    sr_memory_write_hidden_sample(memory, slot + 2u,
        (state->fill_color & 0x00000001u) ? 3u : 0u, sample);
    return true;
}

static inline sr_result framebuffer_write_copy(sr_memory *memory,
                                                const rdp_framebuffer_state *state,
                                                uint32_t x, uint32_t y,
                                                uint16_t value)
{
    if (!memory || !state || state->color_image.width == 0u) return SR_OK;
    if (state->color_image.size == RDP_SIZE_32BPP) return SR_OK;
    const uint32_t slot = framebuffer_slot(memory, state, x, y);
    if (slot == FB_SLOT_NONE) return SR_OK;
    const uint32_t sample = sr_sample_of(x, y);
    if (state->color_image.size == RDP_SIZE_16BPP) {
        framebuffer_store_rgba5551(memory, slot, value, sample);
        return SR_OK;
    }
    if (state->color_image.size == RDP_SIZE_8BPP) {
        framebuffer_store_raw(memory, RDP_SIZE_8BPP, slot, value, sample);
        return SR_OK;
    }
    if (state->color_image.size == RDP_SIZE_4BPP) {
        framebuffer_store_raw(memory, RDP_SIZE_8BPP, slot, 0u, sample);
        return SR_OK;
    }
    return SR_ERROR_UNSUPPORTED;
}

static inline void framebuffer_write_copy_hidden(sr_memory *memory,
                                                  const rdp_framebuffer_state *state,
                                                  uint32_t x, uint32_t y,
                                                  uint8_t value)
{
    const uint32_t slot = framebuffer_slot(memory, state, x, y);
    if (slot != FB_SLOT_NONE)
        sr_memory_write_hidden_sample(memory, slot, value, sr_sample_of(x, y));
}

static inline sr_result framebuffer_write_rgba5551(sr_memory *memory, const rdp_framebuffer_state *state, uint32_t x, uint32_t y, uint16_t texel)
{
    if (!memory || !state || state->color_image.width == 0) return SR_OK;
    const uint32_t pixel = framebuffer_raster_pixel(state, x, y);
    const uint32_t slot = framebuffer_slot(memory, state, x, y);
    if (slot == FB_SLOT_NONE) return SR_OK;
    switch (state->color_image.size) {
    case RDP_SIZE_8BPP: {
        const rdp_color color = rdp_color_from_rgba5551(texel);
        framebuffer_store_raw(memory, RDP_SIZE_8BPP, slot,
                              (pixel & 1u) ? color.g : color.r, sr_sample_of(x, y));
        return SR_OK;
    }
    case RDP_SIZE_16BPP:
        framebuffer_store_rgba5551(memory, slot, texel, sr_sample_of(x, y));
        return SR_OK;
    case RDP_SIZE_32BPP:
        framebuffer_store_raw(memory, RDP_SIZE_32BPP, slot,
            rdp_color_to_rgba8888(rdp_color_from_rgba5551(texel)),
            sr_sample_of(x, y));
        return SR_OK;
    default:
        return SR_ERROR_UNSUPPORTED;
    }
}

static inline sr_result framebuffer_write_color(sr_memory *memory, const rdp_framebuffer_state *state, uint32_t x, uint32_t y, rdp_color color)
{
    if (!memory || !state || state->color_image.width == 0) return SR_OK;
    const uint32_t pixel = framebuffer_raster_pixel(state, x, y);
    const uint32_t slot = framebuffer_slot(memory, state, x, y);
    if (slot == FB_SLOT_NONE) return SR_OK;
    switch (state->color_image.size) {
    /* The 8-bit framebuffer packs the red and green blender outputs on the
     * two pixel lanes.  This is observable for scratch images. */
    case RDP_SIZE_8BPP: {
        const uint8_t value = (pixel & 1u) ? color.g : color.r;
        framebuffer_store_raw(memory, RDP_SIZE_8BPP, slot, value,
                              sr_sample_of(x, y));
        if (pixel & 1u)
            sr_memory_write_hidden_sample(memory, slot,
                                          (value & 1u) ? 3u : 0u,
                                          sr_sample_of(x, y));
        return SR_OK;
    }
    case RDP_SIZE_16BPP:
        framebuffer_store_rgba5551(memory, slot, rdp_color_to_rgba5551(color),
                                   sr_sample_of(x, y));
        return SR_OK;
    case RDP_SIZE_32BPP:
        framebuffer_store_raw(memory, RDP_SIZE_32BPP, slot,
                              rdp_color_to_rgba8888(color), sr_sample_of(x, y));
        return SR_OK;
    default:
        return SR_ERROR_UNSUPPORTED;
    }
}

static inline sr_result framebuffer_read_color(sr_memory *memory, const rdp_framebuffer_state *state,
                                                uint32_t x, uint32_t y, rdp_color *color)
{
    if (!color) return SR_ERROR_INVALID_ARGUMENT;
    *color = (rdp_color){0, 0, 0, 0xe0u};
    if (!memory || !state || state->color_image.width == 0) return SR_OK;
    const uint32_t slot = framebuffer_slot(memory, state, x, y);
    if (slot == FB_SLOT_NONE) return SR_OK;
    if (state->color_image.size == RDP_SIZE_8BPP) {
        const uint8_t value = (uint8_t)framebuffer_load_raw(memory, RDP_SIZE_8BPP,
                                                            slot, sr_sample_of(x, y));
        *color = (rdp_color){ value, value, value, 0xe0u };
        return SR_OK;
    }
    if (state->color_image.size == RDP_SIZE_16BPP) {
        const uint16_t value = (uint16_t)framebuffer_load_raw(
            memory, RDP_SIZE_16BPP, slot, sr_sample_of(x, y));
        *color = rdp_color_from_rgba5551(value);
        color->a &= 0xe0u;
        return SR_OK;
    }
    if (state->color_image.size == RDP_SIZE_32BPP) {
        const uint32_t value = framebuffer_load_raw(memory, RDP_SIZE_32BPP, slot,
                                                    sr_sample_of(x, y));
        *color = (rdp_color){ (uint8_t)(value >> 24), (uint8_t)(value >> 16),
                              (uint8_t)(value >> 8), (uint8_t)value & 0xe0u };
        return SR_OK;
    }
    return SR_ERROR_UNSUPPORTED;
}

static inline sr_result framebuffer_read_memory_pixel(sr_memory *memory,
                                                       const rdp_framebuffer_state *state,
                                                       uint32_t x, uint32_t y,
                                                       bool image_read,
                                                       rdp_memory_pixel *pixel)
{
    if (!pixel) return SR_ERROR_INVALID_ARGUMENT;
    pixel->color = (rdp_color){0, 0, 0, 0xe0u};
    pixel->coverage = 7u;
    if (!image_read) return SR_OK;
    const sr_result result = framebuffer_read_color(memory, state, x, y, &pixel->color);
    if (result != SR_OK) return result;
    pixel->coverage = (pixel->color.a >> 5) & 7u;
    return SR_OK;
}

static inline uint8_t *framebuffer_hidden_plane(sr_memory *memory, uint32_t sample)
{
#if SOFTRDP_SCALE > 1
    if (sample != 0u && memory->sample_hidden)
        return memory->sample_hidden + (sample - 1u) * (memory->rdram_size >> 1);
#else
    (void)sample;
#endif
    return memory->hidden;
}

typedef struct rdp_hidden8_writer {
    int64_t pending[SR_SCALE_SAMPLES];
    bool forward;
} rdp_hidden8_writer;

static inline void framebuffer_hidden8_begin(rdp_hidden8_writer *writer,
                                             bool forward)
{
    writer->forward = forward;
    for (uint32_t sample = 0; sample < SR_SCALE_SAMPLES; sample++)
        writer->pending[sample] = -1;
}

/* An 8bpp byte write reaches the ninth bits of its halfword through the memory
 * interface: an odd byte commits its own LSB, an even byte takes its bit from
 * the interface's latched pair. Left-to-right spans hold an even byte back until
 * its odd partner arrives, which then commits both lanes. */
static inline void framebuffer_hidden8_settle(sr_memory *memory, uint32_t addr,
                                              uint32_t sample)
{
    uint8_t *hidden = framebuffer_hidden_plane(memory, sample);
    if (!hidden) return;
    uint8_t *pair = &hidden[addr >> 1];
    *pair = (uint8_t)((*pair & ~2u) |
                      (memory->copy_hidden_latch[sample][(addr >> 1) & 7u] & 2u));
}

static inline void framebuffer_hidden8_write(sr_memory *memory, uint32_t addr,
                                             uint8_t value,
                                             rdp_hidden8_writer *writer,
                                             uint32_t sample)
{
    const bool forward = writer->forward;
    int64_t *pending = &writer->pending[sample];
    uint8_t *latch = &memory->copy_hidden_latch[sample][(addr >> 1) & 7u];
    const uint8_t replicated = (value & 1u) ? 3u : 0u;
    if (forward && *pending >= 0 && (uint32_t)*pending != addr) {
        framebuffer_hidden8_settle(memory, (uint32_t)*pending, sample);
        *pending = -1;
    }
    uint8_t *hidden = framebuffer_hidden_plane(memory, sample);
    if (hidden) {
        uint8_t *pair = &hidden[addr >> 1];
        const bool clean = (*pair & SR_HIDDEN_CLEAN) != 0u;
        const bool old_lsb =
            (sr_memory_read_be16_sample(memory, addr & ~1u, sample) & 1u) != 0u;
        if (!(addr & 1u)) {
            if (!forward)
                *pair = (uint8_t)((clean ? (old_lsb ? 1u : 0u) : (*pair & ~2u)) |
                                  (*latch & 2u));
            else if (clean)
                *pair = old_lsb ? 3u : 0u;
        } else if (forward && *pending >= 0) {
            *pair = replicated;
        } else {
            *pair = (uint8_t)((clean ? (old_lsb ? 2u : 0u) : (*pair & ~1u)) |
                              (replicated & 1u));
        }
    }
    if (addr & 1u) {
        *latch = replicated;
        *pending = -1;
    } else if (forward) {
        *pending = (int64_t)addr + 1;
    }
}

/* A pixel the blender rejects stores no byte. An odd one still hands its
 * dithered green LSB to the latch and, left to right, settles a held even
 * partner with it; an even one leaves everything alone. */
static inline void framebuffer_hidden8_reject(sr_memory *memory, uint32_t addr,
                                              bool green_lsb,
                                              rdp_hidden8_writer *writer,
                                              uint32_t sample)
{
    if (!(addr & 1u)) return;
    const uint8_t replicated = green_lsb ? 3u : 0u;
    int64_t *pending = &writer->pending[sample];
    if (writer->forward && *pending >= 0) {
        if ((uint32_t)*pending < addr) {
            framebuffer_hidden8_settle(memory, (uint32_t)*pending, sample);
        } else {
            uint8_t *hidden = framebuffer_hidden_plane(memory, sample);
            if (hidden) {
                uint8_t *pair = &hidden[addr >> 1];
                *pair = (uint8_t)((*pair & ~2u) | (replicated & 2u));
            }
        }
        *pending = -1;
    }
    memory->copy_hidden_latch[sample][(addr >> 1) & 7u] = replicated;
}

static inline void framebuffer_hidden8_end(sr_memory *memory,
                                           const rdp_hidden8_writer *writer)
{
    if (!writer->forward) return;
    for (uint32_t sample = 0; sample < SR_SCALE_SAMPLES; sample++)
        if (writer->pending[sample] >= 0)
            framebuffer_hidden8_settle(memory,
                (uint32_t)writer->pending[sample], sample);
}

static inline sr_result framebuffer_write_color8(sr_memory *memory, uint32_t slot,
                                                 uint32_t pixel_index, uint32_t sample,
                                                 rdp_color color,
                                                 rdp_hidden8_writer *writer)
{
    if (slot == FB_SLOT_NONE) return SR_OK;
    const uint8_t value = (pixel_index & 1u) ? color.g : color.r;
    rdp_hidden8_writer local;
    if (!writer) {
        framebuffer_hidden8_begin(&local, true);
        writer = &local;
    }
    framebuffer_hidden8_write(memory, slot, value, writer, sample);
    sr_memory_write_u8_sample(memory, slot, value, sample);
    return SR_OK;
}

/* Runs before the span's colour bytes are stored: a first write to clean
 * memory seeds the pair from the byte it replaces. `mask` selects written
 * pixels by offset from x0; NULL writes all of them. */
static inline void framebuffer_fill8_hidden_span(sr_memory *memory,
                                                 const rdp_framebuffer_state *state,
                                                 uint32_t y, int x0, int x1,
                                                 bool forward, const uint16_t *mask)
{
    if (!memory->hidden || state->color_image.width == 0) return;
    rdp_hidden8_writer writer;
    framebuffer_hidden8_begin(&writer, forward);
    const int step = forward ? 1 : -1;
    for (int x = forward ? x0 : x1; forward ? x <= x1 : x >= x0; x += step) {
        const uint32_t index = (uint32_t)(x - x0);
        if (mask && !(mask[index >> 4] & (1u << (index & 15u)))) continue;
        const uint32_t slot = framebuffer_slot(memory, state, (uint32_t)x, y);
        if (slot == FB_SLOT_NONE) continue;
        const uint32_t pixel = framebuffer_raster_pixel(state, (uint32_t)x, y);
        const uint32_t shift = ((state->color_image.address + pixel) & 3u) ^ 3u;
        const uint32_t sample = sr_sample_of((uint32_t)x, y);
        framebuffer_hidden8_write(memory, slot,
                                  (uint8_t)(state->fill_color >> (shift * 8u)),
                                  &writer, sample);
    }
    framebuffer_hidden8_end(memory, &writer);
}

static inline sr_result framebuffer_write_fill_pixel(sr_memory *memory, const rdp_framebuffer_state *state, uint32_t x, uint32_t y)
{
    if (!memory || !state || state->color_image.width == 0) return SR_OK;
    switch (state->color_image.size) {
    case RDP_SIZE_8BPP: {
        /* Which byte of the fill word a pixel takes is a property of its RDRAM
         * address, so it stays keyed on the hardware pixel even when the pixel
         * itself is stored offscreen. */
        const uint32_t pixel = framebuffer_raster_pixel(state, x, y);
        const uint32_t shift = ((state->color_image.address + pixel) & 3u) ^ 3u;
        framebuffer_store_raw(memory, RDP_SIZE_8BPP,
                              framebuffer_slot(memory, state, x, y),
                              (uint8_t)(state->fill_color >> (shift * 8u)),
                              sr_sample_of(x, y));
        return SR_OK;
    }
    case RDP_SIZE_16BPP:
        write_fill_pixel_16(memory, state, x, y);
        return SR_OK;
    case RDP_SIZE_32BPP:
        write_fill_pixel_32(memory, state, x, y);
        return SR_OK;
    default:
        return SR_ERROR_UNSUPPORTED;
    }
}

#endif
