#include "framebuffer.h"

#include <string.h>

#if SOFTRDP_SCALE == 1
/* The destination is 32-bit aligned before entering this loop. Keeping the
 * loop as one restrict-qualified repeated store lets the compiler vectorize
 * it without understanding RDRAM byte swapping or pixel formats. */
static inline void fill_repeated_words(uint8_t *rdram, uint32_t address,
                                       uint32_t value, uint32_t count)
{
    uint32_t *dst = (uint32_t *)(void *)&rdram[address];
    for (uint32_t i = 0; i < count; i++) {
        dst[i] = value;
    }
}

static bool fill_rect_8(sr_memory *memory, const rdp_framebuffer_state *state,
                        uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1,
                        uint32_t offset, uint32_t stride)
{
    const uint32_t width = state->color_image.width;
    const uint32_t count = x1 - x0 + 1u;
    const uint32_t stored_word = state->fill_color;
    for (uint32_t y = y0; y <= y1; y++) {
        if ((y % stride) != offset) continue;
        framebuffer_fill8_hidden_span(memory, state, y, (int)x0, (int)x1, true, NULL);
        uint32_t address = state->color_image.address + y * width + x0;
        uint32_t remaining = count;
        while (remaining && (address & 3u)) {
            const uint32_t shift = (address & 3u) ^ 3u;
            if (!sr_memory_write_u8(memory, address,
                                    (uint8_t)(state->fill_color >> (shift * 8u)))) return false;
            address++;
            remaining--;
        }
        const uint32_t words = remaining >> 2;
        fill_repeated_words(memory->rdram, address, stored_word, words);
        address += words * 4u;
        remaining -= words * 4u;
        while (remaining) {
            const uint32_t shift = (address & 3u) ^ 3u;
            if (!sr_memory_write_u8(memory, address,
                                    (uint8_t)(state->fill_color >> (shift * 8u)))) return false;
            address++;
            remaining--;
        }
    }
    return true;
}

static bool fill_rect_16(sr_memory *memory, const rdp_framebuffer_state *state,
                         uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1,
                         uint32_t offset, uint32_t stride)
{
    const uint32_t width = state->color_image.width;
    const uint32_t count = x1 - x0 + 1u;
    /* Coverage for filled pixels comes from the stored LSB replicated into the
     * hidden bits. The two halfwords of fill_color can differ, so only a run
     * whose halves agree can be cleared with a single memset. */
    const uint8_t hidden_lo = ((uint16_t)state->fill_color & 1u) ? 3u : 0u;
    const uint8_t hidden_hi = ((uint16_t)(state->fill_color >> 16) & 1u) ? 3u : 0u;
    for (uint32_t y = y0; y <= y1; y++) {
        if ((y % stride) != offset) continue;
        uint32_t pixel = y * width + x0;
        uint32_t address = state->color_image.address + pixel * 2u;
        if (memory->hidden) {
            const uint32_t base = mask_addr(memory, address) >> 1;
            if (hidden_lo == hidden_hi) {
                memset(&memory->hidden[base], hidden_lo, count);
            } else {
                for (uint32_t i = 0; i < count; i++)
                    memory->hidden[base + i] =
                        ((pixel + i) & 1u) ? hidden_lo : hidden_hi;
            }
        }
        uint32_t remaining = count;
        if (remaining && (address & 3u)) {
            const uint16_t value = (pixel & 1u) ? (uint16_t)state->fill_color
                                                : (uint16_t)(state->fill_color >> 16);
            if (!sr_memory_write_be16(memory, address, value)) return false;
            pixel++;
            address += 2u;
            remaining--;
        }
        uint32_t pair = (pixel & 1u) ?
                        (state->fill_color << 16) | (state->fill_color >> 16) :
                        state->fill_color;
        const uint32_t words = remaining >> 1;
        fill_repeated_words(memory->rdram, address, pair, words);
        pixel += words * 2u;
        address += words * 4u;
        remaining -= words * 2u;
        if (remaining) {
            const uint16_t value = (pixel & 1u) ? (uint16_t)state->fill_color
                                                : (uint16_t)(state->fill_color >> 16);
            if (!sr_memory_write_be16(memory, address, value)) return false;
        }
    }
    return true;
}

static bool fill_rect_32(sr_memory *memory, const rdp_framebuffer_state *state,
                         uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1,
                         uint32_t offset, uint32_t stride)
{
    const uint32_t width = state->color_image.width;
    const uint32_t count = x1 - x0 + 1u;
    const uint32_t stored_word = state->fill_color;
    for (uint32_t y = y0; y <= y1; y++) {
        if ((y % stride) != offset) continue;
        const uint32_t address = state->color_image.address +
                                 (y * width + x0) * 4u;
        if (address & 3u) {
            for (uint32_t x = 0; x < count; x++)
                if (!sr_memory_write_be32(memory, address + x * 4u,
                                          state->fill_color)) return false;
        } else {
            fill_repeated_words(memory->rdram, address, stored_word, count);
        }
        if (memory->hidden) {
            const uint8_t hidden_hi = (stored_word & 0x00010000u) ? 3u : 0u;
            const uint8_t hidden_lo = (stored_word & 0x00000001u) ? 3u : 0u;
            uint32_t hidden = mask_addr(memory, address) >> 1;
            for (uint32_t x = 0; x < count; x++) {
                memory->hidden[hidden++] = hidden_hi;
                memory->hidden[hidden++] = hidden_lo;
            }
        }
    }
    return true;
}

#endif /* SOFTRDP_SCALE == 1 */

#if SOFTRDP_SCALE > 1
/*
 * Scaled fill. The run-length paths above write one hardware pixel per store
 * and cannot express the samples, so above scale 1 the rectangle is walked in
 * raster coordinates and each sample is filled in its own right. Fill is a
 * whole-buffer operation once a frame rather than a per-primitive one, so this
 * is not the loop that decides frame time.
 */
static sr_result fill_rect_samples(sr_memory *memory,
                                   const rdp_framebuffer_state *state,
                                   uint32_t x0, uint32_t y0, uint32_t x1,
                                   uint32_t y1, uint32_t offset, uint32_t stride)
{
    for (uint32_t y = y0; y <= y1; y++) {
        if ((y % stride) != offset) continue;
        if (state->color_image.size == RDP_SIZE_8BPP)
            framebuffer_fill8_hidden_span(memory, state, y, (int)x0, (int)x1, true, NULL);
        for (uint32_t x = x0; x <= x1; x++) {
            const sr_result result = framebuffer_write_fill_pixel(memory, state, x, y);
            if (result != SR_OK) return result;
        }
    }
    return SR_OK;
}
#endif

sr_result framebuffer_fill_rect(sr_memory *memory, const rdp_framebuffer_state *state,
                                uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1,
                                uint32_t worker_offset, uint32_t worker_stride)
{
    if (!memory || !state || state->color_image.width == 0)
        return SR_ERROR_INVALID_ARGUMENT;
#if SOFTRDP_SCALE == 1
    if (!memory->rdram) return SR_ERROR_INVALID_ARGUMENT;
#endif
    /* Scanline ownership keys on the absolute line, matching every other render
     * path, so workers fill disjoint lines instead of each filling the whole
     * rectangle. */
    if (worker_stride == 0u) worker_stride = 1u;
    if (state->color_image.size < RDP_SIZE_8BPP ||
        state->color_image.size > RDP_SIZE_32BPP) return SR_ERROR_UNSUPPORTED;

    /* The clamp is on the raster grid, which is where the rectangle now is. */
    const uint32_t raster_width =
        (uint32_t)sr_pixel_to_raster((int32_t)state->color_image.width);
    /* Fill addresses the image linearly and its right edge is inclusive, so a
     * span ending at the image width writes that column into column 0 of the
     * next scanline. No clamp to the width; the RDRAM bound still holds. */
    if (x1 < x0 || y1 < y0 || x0 >= raster_width) return SR_OK;

#if SOFTRDP_SCALE > 1
    return fill_rect_samples(memory, state, x0, y0, x1, y1, worker_offset,
                             worker_stride);
#else
    const uint32_t bytes_per_pixel = 1u << (state->color_image.size - 1u);
    const uint64_t first_pixel = (uint64_t)y0 * state->color_image.width + x0;
    const uint64_t last_pixel = (uint64_t)y1 * state->color_image.width + x1;
    const uint64_t first_address = state->color_image.address + first_pixel * bytes_per_pixel;
    const uint64_t last_address = state->color_image.address +
                                  last_pixel * bytes_per_pixel + bytes_per_pixel - 1u;
    if (first_address >= memory->rdram_size || last_address >= memory->rdram_size)
        return SR_OK;

    switch (state->color_image.size) {
    case RDP_SIZE_8BPP:
        fill_rect_8(memory, state, x0, y0, x1, y1, worker_offset, worker_stride);
        break;
    case RDP_SIZE_16BPP:
        fill_rect_16(memory, state, x0, y0, x1, y1, worker_offset, worker_stride);
        break;
    case RDP_SIZE_32BPP:
        fill_rect_32(memory, state, x0, y0, x1, y1, worker_offset, worker_stride);
        break;
    default: return SR_ERROR_UNSUPPORTED;
    }
    return SR_OK;
#endif
}
