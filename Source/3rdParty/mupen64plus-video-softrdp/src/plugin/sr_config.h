#ifndef SR_PLUGIN_CONFIG_H
#define SR_PLUGIN_CONFIG_H

/*
 * Renderer options, and the one place their names, defaults and valid
 * ranges are stated. Each front-end reads them from whatever its host provides
 * - mupen64plus from its own config API, Project64 from an ini beside the DLL -
 * but both land here to be validated, so the two cannot drift.
 *
 * Header-only and free of any host dependency, so it adds no build rules.
 *
 * Options are read once per ROM, at RomOpen. They cannot change while a ROM runs:
 * the scale sizes the sample planes at context creation, and the worker pool is
 * built once. Reading at RomOpen rather than at plugin load is what makes a
 * changed setting take effect on ROM reload instead of an emulator restart.
 */

#include "../core/sr_scale.h"
#include "../core/sr_threads.h"

#include <stdbool.h>
#include <stdint.h>

/* Project64: the ini section beside the DLL. */
#define SR_CONFIG_SECTION "SoftRDP"

/*
 * mupen64plus: the section in mupen64plus.cfg. The "Video-" prefix is the
 * convention every video plugin follows, and it is what a front-end looks for
 * when it offers per-plugin settings.
 */
#define SR_CONFIG_M64P_SECTION "Video-SoftRDP"
#define SR_CONFIG_KEY_WORKERS "Workers"
#define SR_CONFIG_KEY_SCALE "Scale"
#define SR_CONFIG_KEY_DISABLE_VI_DITHER_FILTER "DisableVIDitherFilter"
#define SR_CONFIG_KEY_DISABLE_VI_DIVOT_FILTER "DisableVIDivotFilter"
#define SR_CONFIG_KEY_DISABLE_VI_GAMMA_DITHER "DisableVIGammaDither"
#define SR_CONFIG_KEY_DISABLE_VI_AA "DisableVIAA"

#define SR_CONFIG_DESC_WORKERS \
    "Rendering worker threads: 0 = auto, 1 = single-threaded, 2+ = fixed core count (up to 12)."
#define SR_CONFIG_DESC_SCALE \
    "Internal resolution: 1 = native, 2 = 2x per axis (4x the pixels)."
#define SR_CONFIG_DESC_DISABLE_VI_DITHER_FILTER \
    "Disable VI dither reconstruction even when requested by the game."
#define SR_CONFIG_DESC_DISABLE_VI_DIVOT_FILTER \
    "Disable the VI divot filter even when requested by the game."
#define SR_CONFIG_DESC_DISABLE_VI_GAMMA_DITHER \
    "Disable VI gamma dither even when requested by the game."
#define SR_CONFIG_DESC_DISABLE_VI_AA \
    "Disable VI anti-aliasing even when requested by the game (bilinear resampling is kept)."

#define SR_CONFIG_DEFAULT_WORKERS 0u
#define SR_CONFIG_DEFAULT_SCALE ((uint32_t)SOFTRDP_SCALE)
#define SR_CONFIG_DEFAULT_DISABLE_VI_DITHER_FILTER false
#define SR_CONFIG_DEFAULT_DISABLE_VI_DIVOT_FILTER false
#define SR_CONFIG_DEFAULT_DISABLE_VI_GAMMA_DITHER false
#define SR_CONFIG_DEFAULT_DISABLE_VI_AA false

typedef struct sr_plugin_config {
    uint32_t workers;
    uint32_t scale;
    bool disable_vi_dither_filter;
    bool disable_vi_divot_filter;
    bool disable_vi_gamma_dither;
    bool disable_vi_aa;
} sr_plugin_config;

static inline sr_plugin_config sr_config_defaults(void)
{
    const sr_plugin_config config = {
        .workers = SR_CONFIG_DEFAULT_WORKERS,
        .scale = SR_CONFIG_DEFAULT_SCALE,
        .disable_vi_dither_filter = SR_CONFIG_DEFAULT_DISABLE_VI_DITHER_FILTER,
        .disable_vi_divot_filter = SR_CONFIG_DEFAULT_DISABLE_VI_DIVOT_FILTER,
        .disable_vi_gamma_dither = SR_CONFIG_DEFAULT_DISABLE_VI_GAMMA_DITHER,
        .disable_vi_aa = SR_CONFIG_DEFAULT_DISABLE_VI_AA
    };
    return config;
}

/*
 * Out-of-range values are clamped, never rejected: a hand-edited ini or a
 * config file left over from a build with a higher SR_SCALE_MAX should still
 * start the emulator rather than fail to render.
 */
static inline uint32_t sr_config_clamp_scale(long value)
{
    if (value < 1) return 1u;
    if (value > SR_SCALE_MAX) return (uint32_t)SR_SCALE_MAX;
    return (uint32_t)value;
}

static inline uint32_t sr_config_clamp_workers(long value)
{
    if (value < 0) return 0u;
    if (value > (long)SR_THREADS_MAX) return SR_THREADS_MAX;
    return (uint32_t)value;
}

#endif
