/*
 * Portions of the internal-resolution scaling implementation are adapted
 * from Parallel-RDP.
 *
 * Copyright (c) 2020 Themaister
 * Used under the MIT License
 */

#include "rdp_memory.h"

#include <stdlib.h>
#include <string.h>

void sr_memory_init(sr_memory *mem, const sr_host_interface *host)
{
    mem->rdram = host ? host->rdram : NULL;
    mem->rdram_size = host ? host->rdram_size : 0;
    mem->dmem = host ? host->dmem : NULL;

    /* One hidden byte per 16-bit word. mask_addr() wraps every address to
     * rdram_size, so `addr >> 1` lands in range by construction and the
     * per-pixel accessors need no bounds check of their own. Reallocated only
     * when the host hands us a differently sized RDRAM. */
    const uint32_t needed = mem->rdram_size >> 1;
    const bool borrow_hidden = host && host->hidden_rdram &&
                               host->hidden_rdram_size >= needed;
    if (borrow_hidden) {
        if (mem->hidden_owned) free(mem->hidden);
        mem->hidden = host->hidden_rdram;
        mem->hidden_size = needed;
        mem->hidden_owned = false;
    } else if (!mem->hidden_owned || needed != mem->hidden_size) {
        if (mem->hidden_owned) free(mem->hidden);
        mem->hidden = needed ? (uint8_t *)malloc(needed) : NULL;
        mem->hidden_size = mem->hidden ? needed : 0u;
        mem->hidden_owned = mem->hidden != NULL;
        if (mem->hidden) memset(mem->hidden, SR_HIDDEN_CLEAN, mem->hidden_size);
    }
    memset(mem->copy_hidden_latch, 0, sizeof(mem->copy_hidden_latch));

#if SOFTRDP_SCALE > 1
    /* One extra copy of RDRAM per sample beyond the first. Sized off the same
     * rdram_size, so an address that mask_addr() accepted is in range here for
     * the same reason. */
    const uint32_t pixels_needed =
        (uint32_t)SR_SCALE_SIDE_SAMPLES * mem->rdram_size;
    if (pixels_needed != mem->sample_pixels_size) {
        free(mem->sample_pixels);
        mem->sample_pixels = pixels_needed ? (uint8_t *)malloc(pixels_needed) : NULL;
        mem->sample_pixels_size = mem->sample_pixels ? pixels_needed : 0u;
    }
    if (mem->sample_pixels) memset(mem->sample_pixels, 0, mem->sample_pixels_size);

    const uint32_t hidden_needed =
        (uint32_t)SR_SCALE_SIDE_SAMPLES * (mem->rdram_size >> 1);
    if (hidden_needed != mem->sample_hidden_size) {
        free(mem->sample_hidden);
        mem->sample_hidden = hidden_needed ? (uint8_t *)malloc(hidden_needed) : NULL;
        mem->sample_hidden_size = mem->sample_hidden ? hidden_needed : 0u;
    }
    if (mem->sample_hidden)
        memset(mem->sample_hidden, SR_HIDDEN_CLEAN, mem->sample_hidden_size);

    if (mem->rdram_size != mem->reference_size) {
        free(mem->reference);
        mem->reference = mem->rdram_size ? (uint8_t *)malloc(mem->rdram_size) : NULL;
        mem->reference_size = mem->reference ? mem->rdram_size : 0u;
    }
    /*
     * Deliberately NOT seeded from RDRAM. Every word then differs on the first
     * import over a range, so a buffer that was already sitting in memory when
     * this context appeared reaches the extra samples too, rather than staying
     * blank in three of every four.
     */
    if (mem->reference) memset(mem->reference, 0, mem->reference_size);
#endif
}

void sr_memory_release(sr_memory *mem)
{
    if (!mem) return;
    if (mem->hidden_owned) free(mem->hidden);
    mem->hidden = NULL;
    mem->hidden_size = 0u;
    mem->hidden_owned = false;
#if SOFTRDP_SCALE > 1
    free(mem->sample_pixels);
    mem->sample_pixels = NULL;
    mem->sample_pixels_size = 0u;
    free(mem->sample_hidden);
    mem->sample_hidden = NULL;
    mem->sample_hidden_size = 0u;
    free(mem->reference);
    mem->reference = NULL;
    mem->reference_size = 0u;
#endif
}

#if SOFTRDP_SCALE > 1
/*
 * Both passes walk 32-bit words. The sample planes are byte-identical copies of
 * RDRAM, so a differing word is copied wholesale with no per-pixel work, and the
 * hidden plane's two bytes for that word come along with it.
 *
 * The range is clamped rather than wrapped. RDP addressing does wrap, but the
 * extents handed here are upper bounds on a buffer's size, and letting an
 * overhang wrap pulls unrelated memory at the start of RDRAM into the import.
 */
static uint32_t clamp_words(const sr_memory *mem, uint32_t addr, uint32_t bytes,
                            uint32_t *first_word)
{
    if (!mem->rdram || !mem->reference || !mem->sample_pixels ||
        mem->rdram_size == 0u)
        return 0u;
    const uint32_t start = mask_addr(mem, addr) & ~3u;
    uint32_t words = (bytes + 3u) >> 2;
    const uint32_t available = (mem->rdram_size - start) >> 2;
    if (words > available) words = available;
    *first_word = start >> 2;
    return words;
}

void sr_memory_import_foreign(sr_memory *mem, uint32_t addr, uint32_t bytes)
{
    if (!mem) return;
    uint32_t first = 0u;
    const uint32_t words = clamp_words(mem, addr, bytes, &first);
    uint32_t *const live = (uint32_t *)(void *)mem->rdram;
    uint32_t *const reference = (uint32_t *)(void *)mem->reference;
    const uint32_t plane_words = mem->rdram_size >> 2;
    const uint32_t hidden_plane = mem->rdram_size >> 1;

    for (uint32_t i = 0; i < words; i++) {
        const uint32_t word = first + i;
        if (live[word] == reference[word]) continue;
        const uint32_t hidden_index = word * 2u;
        for (uint32_t sample = 1u; sample < (uint32_t)SR_SCALE_SAMPLES; sample++) {
            ((uint32_t *)(void *)mem->sample_pixels)[word +
                (sample - 1u) * plane_words] = live[word];
            if (mem->sample_hidden && mem->hidden) {
                uint8_t *plane = mem->sample_hidden + (sample - 1u) * hidden_plane;
                plane[hidden_index] = mem->hidden[hidden_index];
                plane[hidden_index + 1u] = mem->hidden[hidden_index + 1u];
            }
        }
        reference[word] = live[word];
    }
}

void sr_memory_take_reference(sr_memory *mem, uint32_t addr, uint32_t bytes)
{
    if (!mem) return;
    uint32_t first = 0u;
    const uint32_t words = clamp_words(mem, addr, bytes, &first);
    if (words)
        memcpy(&mem->reference[first * 4u], &mem->rdram[first * 4u], words * 4u);
}
#endif
