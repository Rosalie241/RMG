#ifndef RDP_MEMORY_H
#define RDP_MEMORY_H

#include "sr_host.h"
#include "sr_scale.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * RDRAM carries a ninth bit per byte that the RDP uses as a side plane: the low
 * two coverage bits of a 16bpp pixel and the low two bits of the depth delta
 * live there, with the third bit in the stored halfword itself. RGBA5551 has no
 * spare bit for them, so without this plane a three-bit coverage survives a
 * framebuffer round trip as one bit, and the resulting overflow test misfires
 * along polygon edges.
 *
 * This uses a parallel host array rather than genuine 9-bit memory: one byte per
 * 16-bit word, indexed by the logical word number (no halfword swap - the swap
 * applies to the data array only).
 */
#define SR_HIDDEN_CLEAN 4u

typedef struct sr_memory {
    uint8_t *rdram;
    uint32_t rdram_size;
    uint8_t *dmem;
    uint8_t *hidden;
    uint32_t hidden_size;
    bool hidden_owned;
    /* The byte-write interface retains the last committed hidden pair for
     * each phase of its eight-word rotation. Partial 8-bit framebuffer writes
     * consume this state even though the visible byte is otherwise independent. */
    uint8_t copy_hidden_latch[SR_SCALE_SAMPLES][8];
#if SOFTRDP_SCALE > 1
    /*
     * Samples 1..SR_SCALE_SAMPLES-1 of every pixel, as whole extra copies of
     * RDRAM laid end to end. Sample 0 is not here: it is the RDRAM pixel
     * itself, so a scaled build writes the hardware-resolution image to the
     * same place a scale-1 build does and RDRAM is correct by construction -
     * no resolve pass, and texture loads from the frame buffer keep working.
     *
     * Addressed by the RDRAM address, which is the whole point. Render targets
     * on this machine alias freely: a game binds one buffer at two widths, or
     * renders a view into memory that overlaps another buffer and lets the
     * overlap composite. Anything keyed per buffer cannot express that; keyed
     * by address it needs no expressing.
     */
    uint8_t *sample_pixels;
    uint32_t sample_pixels_size;
    uint8_t *sample_hidden;
    uint32_t sample_hidden_size;
    /*
     * RDRAM as the renderer last left it. Anything differing from it was
     * written by someone else - a CPU store, a DMA, the RSP - and exists in
     * sample 0 alone, because nothing outside this renderer knows the other
     * samples are there. Comparing against this is how such a write reaches
     * them; without it CPU-drawn content shows on one pixel in four.
     */
    uint8_t *reference;
    uint32_t reference_size;
#endif
} sr_memory;

/*
 * `mem` must be zeroed or already initialised: the hidden plane is only resized
 * when the host's RDRAM size changes, so init reads the previous size and frees
 * the previous pointer.
 */
void sr_memory_init(sr_memory *mem, const sr_host_interface *host);
void sr_memory_release(sr_memory *mem);

/*
 * Bring the extra samples of [addr, addr + bytes) up to date with anything
 * written to RDRAM from outside the renderer, and take the range as the new
 * reference. Call it before reading a range that something else may have
 * touched - before rendering into a buffer, and before scanning one out.
 *
 * sr_memory_take_reference records completed renderer ranges at the completion
 * fence. It is deliberately a bulk copy, not an extra store in the pixel loop.
 * VI may consume a different completed buffer while RDP writes the next one.
 *
 * Both are no-ops at scale 1, where there are no other samples.
 */
void sr_memory_import_foreign(sr_memory *mem, uint32_t addr, uint32_t bytes);
void sr_memory_take_reference(sr_memory *mem, uint32_t addr, uint32_t bytes);

static inline uint32_t mask_addr(const sr_memory *mem, uint32_t addr)
{
    return addr & (mem->rdram_size - 1u);
}

/*
 * `word` is the halfword already read from the same address. Memory the RDP
 * never wrote coverage to - CPU stores, DMA, a framebuffer the game cleared
 * itself - keeps the clean marker, and the low bits are reconstructed from the
 * stored LSB so such a pixel reads back consistently covered or not.
 *
 * A null plane (a caller that built sr_memory itself, or a failed allocation)
 * takes the same reconstruction path, which is the pre-hidden-plane behaviour.
 */
static inline uint8_t sr_memory_read_hidden_fast(const sr_memory *mem,
                                                 uint32_t addr, uint16_t word)
{
    const uint8_t *hidden = mem->hidden;
    if (!hidden) return (word & 1u) ? 3u : 0u;
    const uint8_t stored = hidden[addr >> 1];
    if (stored & SR_HIDDEN_CLEAN) return (word & 1u) ? 3u : 0u;
    return (uint8_t)(stored & 3u);
}

static inline void sr_memory_write_hidden_fast(sr_memory *mem, uint32_t addr,
                                               uint8_t value)
{
    if (mem->hidden) mem->hidden[addr >> 1] = (uint8_t)(value & 3u);
}

static inline bool sr_memory_read_u8(const sr_memory *mem, uint32_t addr, uint8_t *value)
{
    if (!mem || !mem->rdram || !value) return false;
    addr = mask_addr(mem, addr);
    *value = mem->rdram[addr ^ 3u];
    return true;
}

static inline bool sr_memory_read_be16(const sr_memory *mem, uint32_t addr, uint16_t *value)
{
    if (!mem || !mem->rdram || !value) return false;
    addr = mask_addr(mem, addr);
    *value = *(const uint16_t *)(const void *)&mem->rdram[addr ^ 2u];
    return true;
}

/*
 * mask_addr() bounds the first byte of an access; a 32-bit one touches four.
 * An address in the last three bytes of RDRAM therefore reads or writes past
 * the end - which is exactly what a texture load from the top of memory does.
 * The 8- and 16-bit accessors are safe on their own: their ^3 and ^2 keep the
 * access inside the aligned word the masked address already lands in.
 *
 * RDRAM wraps, so the bytes that fall off the end come from the start rather
 * than being clamped. The byte order below reproduces the native load in the
 * aligned path, which this renderer's x86 target makes little-endian.
 */
static inline bool sr_addr32_crosses_end(const sr_memory *mem, uint32_t addr)
{
    return addr + 4u > mem->rdram_size;
}

static inline uint32_t sr_read32_wrapped(const sr_memory *mem, uint32_t addr)
{
    return (uint32_t)mem->rdram[mask_addr(mem, addr)] |
           ((uint32_t)mem->rdram[mask_addr(mem, addr + 1u)] << 8) |
           ((uint32_t)mem->rdram[mask_addr(mem, addr + 2u)] << 16) |
           ((uint32_t)mem->rdram[mask_addr(mem, addr + 3u)] << 24);
}

static inline void sr_write32_wrapped(sr_memory *mem, uint32_t addr, uint32_t value)
{
    mem->rdram[mask_addr(mem, addr)] = (uint8_t)value;
    mem->rdram[mask_addr(mem, addr + 1u)] = (uint8_t)(value >> 8);
    mem->rdram[mask_addr(mem, addr + 2u)] = (uint8_t)(value >> 16);
    mem->rdram[mask_addr(mem, addr + 3u)] = (uint8_t)(value >> 24);
}

static inline bool sr_memory_read_be32(const sr_memory *mem, uint32_t addr, uint32_t *value)
{
    if (!mem || !mem->rdram || !value) return false;
    addr = mask_addr(mem, addr);
    *value = sr_addr32_crosses_end(mem, addr)
        ? sr_read32_wrapped(mem, addr)
        : *(const uint32_t *)(const void *)&mem->rdram[addr];
    return true;
}

static inline bool sr_memory_write_u8(sr_memory *mem, uint32_t addr, uint8_t value)
{
    if (!mem || !mem->rdram) return false;
    addr = mask_addr(mem, addr);
    mem->rdram[addr ^ 3u] = value;
    return true;
}

static inline bool sr_memory_write_be16(sr_memory *mem, uint32_t addr, uint16_t value)
{
    if (!mem || !mem->rdram) return false;
    addr = mask_addr(mem, addr);
    *(uint16_t *)(void *)&mem->rdram[addr ^ 2u] = value;
    return true;
}

static inline bool sr_memory_write_be32(sr_memory *mem, uint32_t addr, uint32_t value)
{
    if (!mem || !mem->rdram) return false;
    addr = mask_addr(mem, addr);
    if (sr_addr32_crosses_end(mem, addr)) {
        sr_write32_wrapped(mem, addr, value);
    } else {
        *(uint32_t *)(void *)&mem->rdram[addr] = value;
    }
    return true;
}

/* For addresses already wrapped by mask_addr(). */
static inline uint8_t sr_memory_read_u8_fast(const sr_memory *mem, uint32_t addr)
{
    return mem->rdram[addr ^ 3u];
}

static inline uint16_t sr_memory_read_be16_fast(const sr_memory *mem, uint32_t addr)
{
    return *(const uint16_t *)(const void *)&mem->rdram[addr ^ 2u];
}

/*
 * Requires a four-byte aligned address, which every caller has by construction:
 * both the color image and the VI origin are aligned to their pixel size where
 * they are latched, so base + pixel * 4 cannot straddle the end of RDRAM. That
 * makes the bounds check the general accessors need unnecessary here, which
 * matters because these run once per pixel.
 */
static inline uint32_t sr_memory_read_be32_fast(const sr_memory *mem, uint32_t addr)
{
    return *(const uint32_t *)(const void *)&mem->rdram[addr];
}

static inline void sr_memory_write_u8_fast(sr_memory *mem, uint32_t addr, uint8_t value)
{
    mem->rdram[addr ^ 3u] = value;
}

static inline void sr_memory_write_be16_fast(sr_memory *mem, uint32_t addr, uint16_t value)
{
    *(uint16_t *)(void *)&mem->rdram[addr ^ 2u] = value;
}

/* Aligned, like sr_memory_read_be32_fast. */
static inline void sr_memory_write_be32_fast(sr_memory *mem, uint32_t addr, uint32_t value)
{
    *(uint32_t *)(void *)&mem->rdram[addr] = value;
}

/*
 * Sample addressing.
 *
 * Sample 0 is RDRAM. Every other sample is the same address in its own plane,
 * so the byte swizzle, the wrap and the aliasing all behave exactly as they do
 * for the hardware-resolution pixel - there is no second addressing scheme to
 * keep consistent with the first.
 */
#if SOFTRDP_SCALE > 1
static inline uint8_t *sr_sample_plane(sr_memory *mem, uint32_t sample)
{
    return mem->sample_pixels + (sample - 1u) * mem->rdram_size;
}

static inline const uint8_t *sr_sample_plane_const(const sr_memory *mem,
                                                   uint32_t sample)
{
    return mem->sample_pixels + (sample - 1u) * mem->rdram_size;
}
#endif

/*
 * `base` selects RDRAM for sample 0 and the sample's plane otherwise. A failed
 * allocation leaves sample_pixels null and every sample collapses onto RDRAM,
 * which costs the extra detail but keeps each accessor defined.
 */
#if SOFTRDP_SCALE > 1
#define SR_SAMPLE_BASE(mem, sample) \
    (((sample) == 0u || !(mem)->sample_pixels) ? (mem)->rdram \
                                               : sr_sample_plane((mem), (sample)))
#define SR_SAMPLE_BASE_CONST(mem, sample) \
    (((sample) == 0u || !(mem)->sample_pixels) \
        ? (const uint8_t *)(mem)->rdram : sr_sample_plane_const((mem), (sample)))
#else
#define SR_SAMPLE_BASE(mem, sample) ((void)(sample), (mem)->rdram)
#define SR_SAMPLE_BASE_CONST(mem, sample) ((void)(sample), (const uint8_t *)(mem)->rdram)
#endif

static inline uint8_t sr_memory_read_u8_sample(const sr_memory *mem, uint32_t addr,
                                               uint32_t sample)
{
    return SR_SAMPLE_BASE_CONST(mem, sample)[addr ^ 3u];
}

static inline uint16_t sr_memory_read_be16_sample(const sr_memory *mem, uint32_t addr,
                                                  uint32_t sample)
{
    return *(const uint16_t *)(const void *)&SR_SAMPLE_BASE_CONST(mem, sample)[addr ^ 2u];
}

static inline uint32_t sr_memory_read_be32_sample(const sr_memory *mem, uint32_t addr,
                                                  uint32_t sample)
{
    return *(const uint32_t *)(const void *)&SR_SAMPLE_BASE_CONST(mem, sample)[addr];
}

static inline void sr_memory_write_u8_sample(sr_memory *mem, uint32_t addr,
                                             uint8_t value, uint32_t sample)
{
    SR_SAMPLE_BASE(mem, sample)[addr ^ 3u] = value;
}

static inline void sr_memory_write_be16_sample(sr_memory *mem, uint32_t addr,
                                               uint16_t value, uint32_t sample)
{
    *(uint16_t *)(void *)&SR_SAMPLE_BASE(mem, sample)[addr ^ 2u] = value;
}

static inline void sr_memory_write_be32_sample(sr_memory *mem, uint32_t addr,
                                               uint32_t value, uint32_t sample)
{
    *(uint32_t *)(void *)&SR_SAMPLE_BASE(mem, sample)[addr] = value;
}

/* The hidden plane carries one byte per 16-bit word, so a sample's plane is
 * half the size of a pixel plane and indexed the same way. */
static inline uint8_t sr_memory_read_hidden_sample(const sr_memory *mem,
                                                   uint32_t addr, uint16_t word,
                                                   uint32_t sample)
{
#if SOFTRDP_SCALE > 1
    const bool base = sample == 0u || !mem->sample_hidden;
    const uint8_t *hidden = base ? mem->hidden
        : mem->sample_hidden + (sample - 1u) * (mem->rdram_size >> 1);
#else
    (void)sample;
    const uint8_t *hidden = mem->hidden;
#endif
    if (!hidden) return (word & 1u) ? 3u : 0u;
    const uint8_t stored = hidden[addr >> 1];
    if (stored & SR_HIDDEN_CLEAN) return (word & 1u) ? 3u : 0u;
    return (uint8_t)(stored & 3u);
}

/*
 * The depth buffer's use of the same plane: the low two bits of the delta-Z
 * exponent, with the upper two in the stored halfword.
 *
 * This cannot share sr_memory_read_hidden_sample, whose clean-marker fallback
 * reconstructs the bits from the stored LSB. That reconstruction is specific to
 * coverage; on a depth word the LSB is a delta-Z *high* bit, so it would feed
 * the high bit back in as the low ones. Depth memory the RDP never wrote reads
 * as zero instead, which is what a foreign clear leaves on the hardware.
 */
static inline uint8_t sr_memory_read_hidden_depth(const sr_memory *mem,
                                                  uint32_t addr, uint32_t sample)
{
#if SOFTRDP_SCALE > 1
    const bool base = sample == 0u || !mem->sample_hidden;
    const uint8_t *hidden = base ? mem->hidden
        : mem->sample_hidden + (sample - 1u) * (mem->rdram_size >> 1);
#else
    (void)sample;
    const uint8_t *hidden = mem->hidden;
#endif
    if (!hidden) return 0u;
    const uint8_t stored = hidden[addr >> 1];
    return (stored & SR_HIDDEN_CLEAN) ? 0u : (uint8_t)(stored & 3u);
}

static inline void sr_memory_write_hidden_sample(sr_memory *mem, uint32_t addr,
                                                 uint8_t value, uint32_t sample)
{
#if SOFTRDP_SCALE > 1
    const bool base = sample == 0u || !mem->sample_hidden;
    uint8_t *hidden = base ? mem->hidden
        : mem->sample_hidden + (sample - 1u) * (mem->rdram_size >> 1);
#else
    (void)sample;
    uint8_t *hidden = mem->hidden;
#endif
    if (hidden) hidden[addr >> 1] = (uint8_t)(value & 3u);
}

#endif
