#ifndef SR_INTERNAL_H
#define SR_INTERNAL_H

/*
 * The renderer context's layout. Private to the core: sr.c owns it, and the
 * conformance hooks (tests/conformance/softrdp_test_hooks.c) read a few
 * fields through it. Both include this header so a field added or moved in
 * one place cannot silently misalign the other.
 */

#include "sr.h"
#include "sr_async.h"
#include "rdp_memory.h"
#include "rdp_state.h"
#include "sr_threads.h"
#include "tmem.h"
#include "vi.h"

typedef struct prepared_range { uint32_t begin, end; } prepared_range;

typedef struct command_batch {
    sr_context *ctx;
    uint32_t *words;
    uint32_t count, first;
    uint32_t color_addr, depth_addr, color_bytes, depth_bytes;
} command_batch;

struct sr_context {
    void *allocation;
    sr_host_interface host;
    sr_memory memory;
    rdp_state rdp;
    tmem_state tmem;
    vi_state vi;
    vi_scanout_plan vi_plan;
    bool vi_plan_prepared;
    sr_debug_stats debug;
    sr_debug_stats render_debug;
    sr_async *executor;
    uint32_t worker_count;
    command_batch *segments;
    uint32_t segment_count, segment_capacity, segment_start;
    uint32_t batch_target_words;
    uint32_t epoch_batches;
    uint64_t epoch_words;
#if SOFTRDP_SCALE > 1
    prepared_range *prepared;
    uint32_t prepared_count, prepared_capacity;
    bool reference_fallback;
#endif

    /* Producer storage: sealed at complete-command cuts and worker hazards. */
    uint32_t *cmd_buffer;
    uint32_t cmd_buffer_words;
    uint32_t cmd_buffer_capacity;

    /* Framebuffer-into-texture read-after-write hazard tracking */
    uint32_t fb_addr;
    uint32_t fb_width;
    uint32_t fb_bpp;
    uint32_t fb_depth_addr;
    uint32_t tex_addr;
    uint32_t scissor_yhi;
    /*
     * How far down the bound colour image primitives have actually reached,
     * in hardware rows. A colour image states no height, and the scissor is
     * only an upper bound - taking the scissor instead over-covers, and an
     * over-covered import range reaches into whatever buffer follows this one.
     * Reset whenever the target changes.
     */
    uint32_t fb_height;
    bool fb_color_write_pending;
    bool fb_depth_write_pending;
    bool fb_depth_access_pending;
    bool fb_z_update;
    bool fb_z_compare;

    /* Secondary worker states are seeded on demand on first multi-threaded
     * dispatch, and thereafter updated in lockstep with the primary state by
     * having every worker replay every command in the batch. */
    bool workers_seeded;

    rdp_state rdp_worker[SR_THREADS_MAX];
    tmem_state tmem_worker[SR_THREADS_MAX];

    /* Stateful command reader */
    uint32_t cmd_words[SR_MAX_COMMAND_WORDS];
    uint32_t cmd_word_count;
    uint32_t cmd_words_loaded;
    uint32_t cmd_current_address;
    uint8_t cmd_id;
    /* Kept at the tail so adding VI temporal state does not perturb the hot
     * renderer-state layout. Captured into vi_plan at frame preparation. */
    uint32_t vi_frame_count;
};

#endif
