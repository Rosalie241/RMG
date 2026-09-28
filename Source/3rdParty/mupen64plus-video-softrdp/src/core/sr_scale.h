/*
 * Portions of the internal-resolution scaling implementation are adapted
 * from Parallel-RDP.
 *
 * Copyright (c) 2020 Themaister
 * Used under the MIT License
 */
 
#ifndef SR_SCALE_H
#define SR_SCALE_H

/*
 * Internal resolution scale. 1 renders at the hardware's resolution; 2 gives
 * every RDRAM pixel a 2x2 grid of samples.
 *
 * Override at build time (-DSOFTRDP_SCALE=2). This is a compile-time constant
 * on purpose: it multiplies out into address arithmetic on the hottest paths in
 * the pixel pipeline, and every derived expression below folds to nothing at
 * scale 1, so a 1x build carries no trace of the scaling support.
 */
#ifndef SOFTRDP_SCALE
#define SOFTRDP_SCALE 1
#endif

#if SOFTRDP_SCALE != 1 && SOFTRDP_SCALE != 2
#error "SOFTRDP_SCALE must be 1 or 2"
#endif

/*
 * Give TEX_RECT and TEX_RECT_FLIP native-resolution texture coordinates when
 * scaling: the rectangle still covers the whole upscaled grid, but the samples
 * of one hardware pixel all carry that pixel's scale-1 texel.
 *
 * TEX_RECT is the machine's 2D blit. A game emitting one has chosen dsdx so
 * that one texel lands on one hardware pixel, and the bilinear filter's
 * half-texel offset cancels at exactly that ratio. Stepping the coordinate per
 * raster pixel puts the intermediate samples BETWEEN texels, and the filter
 * then averages neighbours that were never meant to meet: the next sprite in
 * an atlas bleeds across every border, glyph edges soften, and at a tile
 * boundary the second tap is decided by the wrap mode. There is nothing finer
 * to interpolate towards here - the texel grid is the source resolution - so
 * upscaling the coordinate is wrong rather than merely imprecise.
 *
 */
#ifndef SOFTRDP_NATIVE_TEX_RECT
#define SOFTRDP_NATIVE_TEX_RECT 1
#endif

/*
 * The largest scale this build can be asked for at runtime. The core is
 * compiled once per scale and both copies are linked in, so SOFTRDP_SCALE below
 * is only this build's DEFAULT - it does not bound what the binary can render.
 * Use SR_SCALE_MAX to size anything that must hold either scale, and to clamp
 * configuration.
 */
#define SR_SCALE_MAX 2

/* Samples per hardware pixel: the 2x2 grid at scale 2, one at scale 1. */
#define SR_SCALE_SAMPLES (SOFTRDP_SCALE * SOFTRDP_SCALE)

/* Samples held outside RDRAM. Sample 0 is the RDRAM pixel itself. */
#define SR_SCALE_SIDE_SAMPLES (SR_SCALE_SAMPLES - 1)

#if SOFTRDP_SCALE == 2
#define SR_SCALE_LOG2 1
#else
#define SR_SCALE_LOG2 0
#endif

#include <stdbool.h>
#include <stdint.h>

/*
 * Raster space is the scaled grid rasterization and the span kernels work in:
 * x and y count 2x pixels at scale 2, hardware pixels at scale 1.
 *
 * A raster pixel maps to the frame buffer pixel that contains it.
 */
static inline uint32_t sr_raster_to_pixel(uint32_t raster_coord)
{
    return raster_coord >> SR_SCALE_LOG2;
}

static inline int32_t sr_pixel_to_raster(int32_t pixel_coord)
{
    return pixel_coord << SR_SCALE_LOG2;
}

/*
 * Which sample of its hardware pixel a raster pixel is. Sample 0 is the
 * top-left, and it is the one that lives in RDRAM.
 */
static inline uint32_t sr_sample_of(uint32_t raster_x, uint32_t raster_y)
{
#if SOFTRDP_SCALE == 1
    (void)raster_x;
    (void)raster_y;
    return 0u;
#else
    return ((raster_y & 1u) << 1) | (raster_x & 1u);
#endif
}

/*
 * A hardware-precision snap applied to a value that has already been lifted
 * into raster space.
 *
 * Masking off the low bits of a hardware value quantises it to a whole
 * hardware unit - a scanline, a pixel. Applying the same mask to the scaled
 * value quantises to a FRACTION of that unit instead, which is a different
 * operation and leaves the result depending on the low bits the mask was
 * supposed to discard. Scaling multiplies by 2^SR_SCALE_LOG2, so the
 * equivalent mask is that many bits wider.
 *
 * `hardware_bits` is how many low bits the hardware expression masks off:
 * `& ~3` is two.
 */
static inline int32_t sr_snap_scaled(int32_t scaled, uint32_t hardware_bits)
{
    const uint32_t mask = (1u << (hardware_bits + SR_SCALE_LOG2)) - 1u;
    return scaled & ~(int32_t)mask;
}

/*
 * The same reasoning for a shift: `value >> n` at hardware precision, with the
 * result still in raster units.
 */
static inline int32_t sr_shift_scaled(int32_t scaled, uint32_t hardware_bits)
{
    return (scaled >> (hardware_bits + SR_SCALE_LOG2)) << SR_SCALE_LOG2;
}

/* A count of hardware sub-rows, expressed in raster sub-rows. */
static inline int32_t sr_hardware_subrows(int32_t subrows)
{
    return subrows << SR_SCALE_LOG2;
}

/*
 * Derivatives in a triangle setup describe one hardware pixel or one hardware
 * scanline. Raster space steps in fractions of those, so a step count cannot
 * simply be multiplied by the derivative.
 *
 * Splitting the count into whole hardware steps and the remainder keeps the
 * whole steps at full precision and only the remainder at reduced precision.
 * Halving the derivative up front instead would drop a bit on every step, and
 * with it the property that sample 0 reproduces the hardware-resolution value.
 */
/* The derivative for a single raster step, used where a fraction of a hardware
 * step is needed directly rather than as part of a step count. */
static inline int32_t sr_scaled_derivative(int32_t derivative)
{
    return derivative >> SR_SCALE_LOG2;
}

/*
 * The same step count evaluated at hardware precision: the raster steps within
 * a hardware pixel are dropped, so every sample of that pixel gets the value it
 * would have had at scale 1. The counterpart to sr_scaled_steps for the paths
 * that must not upscale at all.
 */
static inline int64_t sr_native_steps(int32_t derivative, int64_t raster_steps)
{
    return (raster_steps >> SR_SCALE_LOG2) * (int64_t)derivative;
}

static inline int64_t sr_scaled_steps(int32_t derivative, int64_t raster_steps)
{
#if SOFTRDP_SCALE == 1
    return (int64_t)derivative * raster_steps;
#else
    const int64_t whole = raster_steps >> SR_SCALE_LOG2;
    const int64_t part = raster_steps & (SOFTRDP_SCALE - 1);
    return whole * (int64_t)derivative + part * (int64_t)(derivative >> SR_SCALE_LOG2);
#endif
}

#endif
