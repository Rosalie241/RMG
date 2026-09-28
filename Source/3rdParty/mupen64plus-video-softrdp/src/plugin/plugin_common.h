#ifndef SR_PLUGIN_COMMON_H
#define SR_PLUGIN_COMMON_H

/*
 * What the host adapters share once the host ABI has been unpacked: the
 * machine both build from their GFX_INFO, the renderer context's lifetime,
 * the frame dump hooks around a list, and the frame path from scanout to
 * the presenter. An adapter keeps only its exports, its configuration source
 * and its window.
 */

#include "../core/sr.h"
#include "../present/sr_present.h"
#include "../present/sr_field_output.h"
#include "sr_config.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * The registers a host hands over, in sr_dp_register / sr_vi_register order.
 * Both GFX_INFOs spell the fields identically and store them as 32-bit
 * unsigned (DWORD on Project64, unsigned int on mupen64plus), so each adapter
 * expands these once over its own struct with SR_PLUGIN_MACHINE_FROM_GFX.
 */
#define SR_PLUGIN_DP_REG_LIST(X)                                               \
    X(DPC_START_REG) X(DPC_END_REG) X(DPC_CURRENT_REG) X(DPC_STATUS_REG)      \
    X(DPC_CLOCK_REG) X(DPC_BUFBUSY_REG) X(DPC_PIPEBUSY_REG) X(DPC_TMEM_REG)

#define SR_PLUGIN_VI_REG_LIST(X)                                               \
    X(VI_STATUS_REG) X(VI_ORIGIN_REG) X(VI_WIDTH_REG) X(VI_INTR_REG)          \
    X(VI_V_CURRENT_LINE_REG) X(VI_TIMING_REG) X(VI_V_SYNC_REG)                \
    X(VI_H_SYNC_REG) X(VI_LEAP_REG) X(VI_H_START_REG) X(VI_V_START_REG)       \
    X(VI_V_BURST_REG) X(VI_X_SCALE_REG) X(VI_Y_SCALE_REG)

/*
 * The emulated machine as the renderer sees it. The adapter that fills it must
 * keep it alive for as long as the context it creates: the renderer's
 * interrupt callback reads check_interrupts through it.
 */
typedef struct sr_plugin_machine {
    uint8_t *rdram;
    uint8_t *dmem;
    uint32_t rdram_size;
    bool rdram_bswapped;
    uint32_t *mi_intr_reg;
    uint32_t *dp[SR_DP_REGISTER_COUNT];
    uint32_t *vi[SR_VI_REGISTER_COUNT];
    void (*check_interrupts)(void);
} sr_plugin_machine;

#define SR_PLUGIN_MACHINE_FROM_GFX(machine, gfx)                              \
    do {                                                                       \
        sr_plugin_machine *const machine_ = (machine);                         \
        const __typeof__(*(gfx)) *const gfx_ = (gfx);                          \
        uint32_t index_ = 0u;                                                  \
        machine_->rdram = gfx_->RDRAM;                                         \
        machine_->dmem = gfx_->DMEM;                                           \
        machine_->mi_intr_reg = (uint32_t *)(void *)gfx_->MI_INTR_REG;         \
        machine_->check_interrupts = gfx_->CheckInterrupts;                    \
        SR_PLUGIN_DP_REG_LIST(SR_PLUGIN_TAKE_DP_)                              \
        index_ = 0u;                                                           \
        SR_PLUGIN_VI_REG_LIST(SR_PLUGIN_TAKE_VI_)                              \
    } while (0)
/* The helpers read the do-block's locals: a macro argument is not substituted
 * into another macro's body. */
#define SR_PLUGIN_TAKE_DP_(name) \
    machine_->dp[index_++] = (uint32_t *)(void *)gfx_->name;
#define SR_PLUGIN_TAKE_VI_(name) \
    machine_->vi[index_++] = (uint32_t *)(void *)gfx_->name;

/*
 * Create the renderer for this machine and configuration and, in a dump
 * build, attach the frame dump recorder to it. dump_log receives the
 * recorder's messages and may be NULL. Returns NULL when the renderer could
 * not be created.
 */
sr_context *sr_plugin_context_create(const sr_plugin_machine *machine,
                                     const sr_plugin_config *config,
                                     void (*dump_log)(const char *message));

/*
 * Detach the recorder, destroy the context and join the worker pool. The pool
 * is process-wide and its width is fixed once created, so joining it here is
 * what lets a changed Workers setting take effect on the next ROM, and what
 * keeps a worker from outliving this library when the host unloads it. Safe
 * with a NULL context.
 */
void sr_plugin_context_destroy(sr_context *ctx);

/* One RDP list, with the recorder's before/after hooks around it. */
sr_result sr_plugin_process_list(sr_context *ctx);

/*
 * The frame path's storage: the scanout buffer, the interlaced field weave,
 * and what the presenter was last handed, for a host that wants to read the
 * screen back. Zero-initialise; release with sr_plugin_screen_reset.
 */
typedef struct sr_plugin_screen {
    sr_rgba8 *pixels;
    uint32_t capacity_pixels;
    sr_field_output field_output;
    /* The image on screen after the last sr_plugin_present, or NULL when the
     * screen is blank. Points into this struct's own buffers. */
    const sr_rgba8 *shown_pixels;
    uint32_t shown_width;
    uint32_t shown_height;
    bool blank;
} sr_plugin_screen;

typedef enum sr_plugin_present_result {
    /* A new frame was scanned out and drawn. */
    SR_PLUGIN_PRESENTED = 0,
    /* No frame this refresh but not a blank signal: the previous frame was
     * redrawn. */
    SR_PLUGIN_HELD,
    /* A blank signal or a scanout failure: the screen was cleared. VI scanout
     * is authoritative, so raw RDRAM is never shown as a fallback. */
    SR_PLUGIN_BLANKED,
    /* Nothing was drawn: no context, or the presenter is not ready. */
    SR_PLUGIN_SKIPPED
} sr_plugin_present_result;

/*
 * One host refresh: poll the recorder's hotkey, scan out the VI frame, weave
 * fields and hand the result to the presenter. Owned-context presenters swap
 * inside; an external-context host swaps afterwards itself.
 */
sr_plugin_present_result sr_plugin_present(sr_plugin_screen *screen,
                                           sr_context *ctx,
                                           sr_present *present);

void sr_plugin_screen_reset(sr_plugin_screen *screen);

#endif
