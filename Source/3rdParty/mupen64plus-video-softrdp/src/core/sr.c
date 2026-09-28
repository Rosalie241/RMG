#include "sr.h"
#include "sr_internal.h"
#include "sr_async.h"

#include "rdp_commands.h"
#include "raster.h"
#include "rdp_memory.h"
#include "rdp_state.h"
#include "sr_threads.h"
#include "tmem.h"
#include "vi.h"

#include <stdlib.h>
#include <string.h>


#define SR_STATE_SNAPSHOT_MAGIC 0x31535253u /* "SRS1" */
typedef struct sr_state_snapshot {
    uint32_t magic;
    uint32_t size;
    rdp_state rdp;
    /* TMEM's bytes only, laid out exactly as the tmem_state the format was
     * defined with, so existing dumps keep loading. */
    _Alignas(64) uint8_t tmem[SR_TMEM_SIZE];
} sr_state_snapshot;

#define DP_STATUS_XBUS_DMA 0x001u
#define DP_INTERRUPT 0x20u

static uint32_t read_reg(uint32_t *const *regs, uint32_t index)
{
    return regs[index] ? *regs[index] : 0;
}

size_t sr_state_snapshot_size(void)
{
    return sizeof(sr_state_snapshot);
}

sr_result sr_save_state(const sr_context *ctx, void *data, size_t size)
{
    if (!ctx || !data || size != sizeof(sr_state_snapshot)) return SR_ERROR_INVALID_ARGUMENT;
    sr_async_wait(ctx->executor);
    sr_state_snapshot snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    snapshot.magic = SR_STATE_SNAPSHOT_MAGIC;
    snapshot.size = (uint32_t)sizeof(snapshot);
    snapshot.rdp = ctx->rdp;
    memcpy(snapshot.tmem, ctx->tmem.bytes, sizeof(snapshot.tmem));
    memcpy(data, &snapshot, sizeof(snapshot));
    return SR_OK;
}

sr_result sr_load_state(sr_context *ctx, const void *data, size_t size)
{
    if (!ctx || !data) return SR_ERROR_INVALID_ARGUMENT;
    sr_flush(ctx);
    if (size != sizeof(sr_state_snapshot)) return SR_ERROR_INVALID_ARGUMENT;
    sr_state_snapshot snapshot;
    memcpy(&snapshot, data, sizeof(snapshot));
    if (snapshot.magic != SR_STATE_SNAPSHOT_MAGIC || snapshot.size != sizeof(snapshot))
        return SR_ERROR_INVALID_ARGUMENT;
    ctx->rdp = snapshot.rdp;
    memcpy(ctx->tmem.bytes, snapshot.tmem, sizeof(snapshot.tmem));
    tmem_palette_refresh(&ctx->tmem);
    ctx->fb_z_update = ctx->rdp.other_modes.z_update;
    ctx->fb_z_compare = ctx->rdp.other_modes.z_compare;
    ctx->workers_seeded = false;
    primitive_cache_reset();
    return SR_OK;
}

static void write_reg(uint32_t *const *regs, uint32_t index, uint32_t value)
{
    if (regs[index]) {
        *regs[index] = value;
    }
}

static void capture_texture_debug(sr_debug_stats *debug, const rdp_state *state,
                                  const rdp_command *cmd);

static sr_result execute_rdp_command(sr_context *ctx, rdp_command *cmd)
{
    sr_result result = rdp_decode_command(cmd);
    if (result == SR_OK)
        result = rdp_execute_command(&ctx->memory, &ctx->tmem, &ctx->rdp, cmd);
    if (result != SR_OK) {
        capture_texture_debug(&ctx->render_debug, &ctx->rdp, cmd);
        return result;
    }
    ctx->render_debug.color_image_format = (uint32_t)ctx->rdp.color_image.format;
    ctx->render_debug.color_image_size = (uint32_t)ctx->rdp.color_image.size;
    ctx->render_debug.color_image_width = ctx->rdp.color_image.width;
    ctx->render_debug.color_image_address = ctx->rdp.color_image.address;
    capture_texture_debug(&ctx->render_debug, &ctx->rdp, cmd);
    return SR_OK;
}

static bool read_command_word(const sr_context *ctx, bool xbus_dma, uint32_t word_index, uint32_t *word)
{
    if (xbus_dma) {
        const uint32_t byte_addr = (word_index & 0x3ffu) << 2;
        const uint8_t *dmem = ctx->host.dmem;
        if (!dmem || byte_addr + 3u >= SR_DMEM_SIZE) {
            return false;
        }

        const uint32_t swizzle = ctx->host.dmem_big_endian ? 0u : 3u;
        *word = ((uint32_t)dmem[byte_addr ^ swizzle] << 24) |
                ((uint32_t)dmem[(byte_addr + 1u) ^ swizzle] << 16) |
                ((uint32_t)dmem[(byte_addr + 2u) ^ swizzle] << 8) |
                ((uint32_t)dmem[(byte_addr + 3u) ^ swizzle]);
        return true;
    }

    return sr_memory_read_be32(&ctx->memory, word_index << 2, word);
}

static void raise_dp_interrupt(sr_context *ctx)
{
    if (ctx->host.mi_intr_reg) {
        *ctx->host.mi_intr_reg |= DP_INTERRUPT;
    }
    if (ctx->host.raise_mi_interrupt) {
        ctx->host.raise_mi_interrupt(ctx->host.userdata);
    }
}

static void finish_rdp_list(sr_context *ctx)
{
    /* DPC_END is owned by the command producer and may change in the
     * SyncFull interrupt callback. Match the RDP/reference implementation:
     * acknowledge the list using the current END value, but never overwrite it.
     */
    const uint32_t end = read_reg(ctx->host.dp_regs, SR_DP_END);
    write_reg(ctx->host.dp_regs, SR_DP_START, end);
    write_reg(ctx->host.dp_regs, SR_DP_CURRENT, end);
}

typedef struct replay_job {
    sr_context *ctx;
    const uint32_t *words;
    uint32_t word_count;
    rdp_state *rdp[SR_THREADS_MAX];
    tmem_state *tmem[SR_THREADS_MAX];
    sr_result result[SR_THREADS_MAX];
} replay_job;

static void replay_run(void *user, uint32_t worker_id, uint32_t worker_count)
{
    replay_job *j = (replay_job *)user;
    sr_context *ctx = j->ctx;
    rdp_state *rdp = j->rdp[worker_id];
    tmem_state *tmem = j->tmem[worker_id];
    rdp->worker_offset = worker_id;
    rdp->worker_stride = worker_count;

    uint32_t word = 0u;
    const uint32_t end_word = j->word_count;
    sr_result result = SR_OK;

    while (word < end_word) {
        const uint32_t first_word = j->words[word];
        const uint8_t cid = (uint8_t)((first_word >> 24) & 0x3fu);
        const uint32_t wc = rdp_command_word_count((rdp_command_id)cid);
        if (wc == 0u || wc > SR_MAX_COMMAND_WORDS || word + wc > end_word) {
            break;
        }

        rdp_command cmd;
        cmd.id = (rdp_command_id)cid;
        cmd.word_count = (uint8_t)wc;
        for (uint32_t w = 0u; w < wc; w++)
            cmd.words[w] = j->words[word + w];
        word += wc;

        sr_result r;
        if (worker_id == 0u) {
            /* Lead worker owns the canonical state and debug side effects,
             * matching the single-threaded path exactly. */
            ctx->render_debug.last_command_id = (uint32_t)cid;
            r = execute_rdp_command(ctx, &cmd);
            ctx->render_debug.last_result = r;
        } else {
            r = rdp_decode_command(&cmd);
            if (r == SR_OK)
                r = rdp_execute_command(&ctx->memory, tmem, rdp, &cmd);
        }
        if (r != SR_OK)
            result = r;
    }
    j->result[worker_id] = result;
}

static bool cmdbuf_append(sr_context *ctx, const uint32_t *words, uint32_t count)
{
    if (ctx->cmd_buffer_words + count > ctx->cmd_buffer_capacity) {
        uint32_t cap = ctx->cmd_buffer_capacity ? ctx->cmd_buffer_capacity : 4096u;
        while (cap < ctx->cmd_buffer_words + count) cap *= 2u;
        uint32_t *grown = realloc(ctx->cmd_buffer, cap * sizeof(uint32_t));
        if (!grown) return false;
        ctx->cmd_buffer = grown;
        ctx->cmd_buffer_capacity = cap;
    }
    for (uint32_t w = 0u; w < count; w++)
        ctx->cmd_buffer[ctx->cmd_buffer_words + w] = words[w];
    ctx->cmd_buffer_words += count;
    return true;
}

static inline uint32_t image_bytes_per_pixel(rdp_texture_size size)
{
    switch (size) {
    case RDP_SIZE_8BPP: return 1u;
    case RDP_SIZE_16BPP: return 2u;
    case RDP_SIZE_32BPP: return 4u;
    default: return 0u;
    }
}

/*
 * `low_edge` is the primitive's bottom in quarter rows, the same units the
 * scissor uses. Clipping to the scissor first is what keeps a primitive that
 * runs off the bottom of the screen from inflating the buffer's height.
 */
static void fb_note_height(sr_context *ctx, uint32_t low_edge)
{
    const uint32_t limit = ctx->scissor_yhi;
    if (limit && low_edge > limit) low_edge = limit;
    const uint32_t rows = (low_edge + 3u) >> 2;
    if (rows > ctx->fb_height) ctx->fb_height = rows;
}

static void fb_note_draw(sr_context *ctx)
{
    ctx->fb_color_write_pending = true;
    if (ctx->fb_z_update) ctx->fb_depth_write_pending = true;
    if (ctx->fb_z_update || ctx->fb_z_compare) ctx->fb_depth_access_pending = true;
}

/* Height of the buffer, not of what has been drawn into it: the scissor is
 * the region the game declares it draws in, so the target is at least that
 * tall. Capped so a bogus scissor cannot widen the window arbitrarily. */
#define SR_FB_MAX_ROWS 512u

static bool tmem_load_needs_flush(const sr_context *ctx)
{
    const uint32_t mask = ctx->memory.rdram_size ? ctx->memory.rdram_size - 1u : 0u;
    uint32_t rows = ctx->scissor_yhi >> 2;
    if (rows == 0u) rows = 1u;
    if (rows > SR_FB_MAX_ROWS) rows = SR_FB_MAX_ROWS;

    if (ctx->fb_color_write_pending && ctx->fb_width != 0u && ctx->fb_bpp != 0u) {
        const uint32_t span = ctx->fb_width * rows * ctx->fb_bpp;
        if (((ctx->tex_addr - ctx->fb_addr) & mask) < span) return true;
    }
    if (ctx->fb_depth_write_pending && ctx->fb_width != 0u) {
        const uint32_t span = ctx->fb_width * rows * 2u;
        if (((ctx->tex_addr - ctx->fb_depth_addr) & mask) < span) return true;
    }
    return false;
}

/* Threaded batch replay. Sealed batches execute in FIFO order, and SyncFull
 * waits for their completion. Replay is split across the worker pool: every
 * worker replays every command of every batch from its own rdp/tmem state (so
 * state stays consistent) and renders only its interleaved scanlines. In-order
 * replay preserves the temporal coupling between texture loads and later draws.
 *
 * With threading off the dispatch is unchanged - the pool is simply one worker
 * wide, the same shape a single-core machine produces. Command buffering
 * applies either way. */

/* How wide to build the pool. The host's choice is a policy input here rather
 * than a second code path, so every configuration runs the same dispatch. */
static uint32_t replay_worker_count(const sr_context *ctx)
{
    if (ctx->worker_count <= 1u) return 1u;
    /* At most one worker per physical core. Hyperthread siblings share the cache and
     * the vector units the span loops lean on, and every worker additionally
     * replays the whole batch's setup work, so pairing workers onto one core
     * costs more in duplicated replay than it wins in occupancy.
     *
     * Sound at every scale: workers are partitioned by raster row, and a raster
     * row is a row of the render target at any scale, so two workers never hold
     * the same pixel. */
    return ctx->worker_count;
}

/* Render the accumulated batch: replay it across the worker pool, then reset. */
#if SOFTRDP_SCALE > 1
/*
 * The batch's render targets, in bytes. Height comes from the primitives that
 * have been queued, so it describes what this buffer is actually used for
 * rather than what the scissor permits.
 */
static void fb_extent(const sr_context *ctx, uint32_t *color, uint32_t *depth)
{
    const uint32_t rows = ctx->fb_height;
    *color = ctx->fb_width * rows * ctx->fb_bpp;
    *depth = ctx->fb_width * rows * 2u;
}
#endif


#if SOFTRDP_SCALE > 1
/* Prepare each address only once between completion fences. Storage cuts and
 * worker hazards do not re-import renderer output. Completed ranges are copied
 * to the reference at SyncFull, before the game may reuse/display them. */
static void prepare_render_range(sr_context *ctx, uint32_t addr, uint32_t bytes)
{
    if (!bytes || !ctx->memory.rdram_size) return;
    uint32_t first = mask_addr(&ctx->memory, addr) & ~3u;
    uint32_t count = (bytes + 3u) / 4u;
    if (count > (ctx->memory.rdram_size - first) / 4u)
        count = (ctx->memory.rdram_size - first) / 4u;
    uint32_t end = first + count * 4u;
    uint32_t cursor = first;
    for (uint32_t i = 0; i < ctx->prepared_count && cursor < end; i++) {
        prepared_range range = ctx->prepared[i];
        if (range.end <= cursor) continue;
        if (range.begin >= end) break;
        if (range.begin > cursor)
            sr_memory_import_foreign(&ctx->memory, cursor, range.begin - cursor);
        cursor = range.end;
    }
    if (cursor < end) sr_memory_import_foreign(&ctx->memory, cursor, end - cursor);

    uint32_t lo = 0;
    while (lo < ctx->prepared_count && ctx->prepared[lo].end < first) lo++;
    uint32_t hi = lo;
    while (hi < ctx->prepared_count && ctx->prepared[hi].begin <= end) {
        if (ctx->prepared[hi].begin < first) first = ctx->prepared[hi].begin;
        if (ctx->prepared[hi].end > end) end = ctx->prepared[hi].end;
        hi++;
    }
    if (hi == lo && ctx->prepared_count == ctx->prepared_capacity) {
        uint32_t capacity = ctx->prepared_capacity ? ctx->prepared_capacity * 2u : 16u;
        prepared_range *grown = realloc(ctx->prepared, capacity * sizeof(*grown));
        if (!grown) { ctx->reference_fallback = true; return; }
        ctx->prepared = grown;
        ctx->prepared_capacity = capacity;
    }
    memmove(ctx->prepared + lo + 1u, ctx->prepared + hi,
            (ctx->prepared_count - hi) * sizeof(*ctx->prepared));
    ctx->prepared[lo] = (prepared_range){first, end};
    ctx->prepared_count = ctx->prepared_count - (hi - lo) + 1u;
}
#endif

static void execute_batch(void *opaque)
{
    static sr_context *previous_context;
    command_batch *batch = opaque;
    sr_context *ctx = batch->ctx;
    if (previous_context != ctx) {
        primitive_cache_reset();
        previous_context = ctx;
    }
#if SOFTRDP_SCALE > 1
    prepare_render_range(ctx, batch->color_addr, batch->color_bytes);
    if (batch->depth_addr)
        prepare_render_range(ctx, batch->depth_addr, batch->depth_bytes);
#endif
    replay_job job;
    job.ctx = ctx;
    job.words = batch->words;
    job.word_count = batch->count;
    job.rdp[0] = &ctx->rdp;
    job.tmem[0] = &ctx->tmem;
    job.result[0] = SR_OK;

    sr_threads_init(replay_worker_count(ctx));
    const uint32_t workers = sr_threads_count();

    /* Seed the helper replicas once. Every worker then replays every command of
     * every batch from the same state, so they stay identical on their own.
     * Empty at one worker: there are no helpers, and worker 0 renders straight
     * from the canonical rdp/tmem installed above. */
    for (uint32_t i = 1u; i < workers; i++) {
        job.result[i] = SR_OK;
        if (!ctx->workers_seeded) {
            ctx->rdp_worker[i] = ctx->rdp;
            ctx->tmem_worker[i] = ctx->tmem;
        }
        job.rdp[i] = &ctx->rdp_worker[i];
        job.tmem[i] = &ctx->tmem_worker[i];
    }
    /* Only claim the replicas are seeded if there were any to seed, so the flag
     * keeps meaning "the helper replicas hold a valid copy". */
    if (workers > 1u) ctx->workers_seeded = true;

    /* Collapses to a direct call on the caller when the pool is one wide, so
     * this costs no events and no waits in the serial configurations. */
    sr_threads_run(replay_run, &job);

    /* replay_run leaves worker 0's split on the canonical state; restore the
     * whole-frame stride for anything reading rdp outside a replay. */
    ctx->rdp.worker_offset = 0u;
    ctx->rdp.worker_stride = 1u;
    for (uint32_t i = 0u; i < workers; i++)
        if (job.result[i] != SR_OK) ctx->render_debug.last_result = job.result[i];
#if SOFTRDP_SCALE > 1
    /* Only allocation failure loses epoch tracking. Preserve correctness with
     * the old bulk operation in that exceptional path, never pixel stores. */
    if (ctx->reference_fallback) {
        sr_memory_take_reference(&ctx->memory, batch->color_addr, batch->color_bytes);
        if (batch->depth_addr)
            sr_memory_take_reference(&ctx->memory, batch->depth_addr, batch->depth_bytes);
    }
#endif
}

typedef struct queued_batch {
    sr_context *ctx;
    uint32_t *words;
    command_batch *segments;
    uint32_t segment_count;
    command_batch tail;
} queued_batch;

static command_batch pending_segment(sr_context *ctx)
{
    command_batch segment = {0};
    segment.ctx = ctx;
    segment.first = ctx->segment_start;
    segment.count = ctx->cmd_buffer_words - ctx->segment_start;
    segment.color_addr = ctx->fb_addr;
    segment.depth_addr = ctx->fb_depth_addr;
#if SOFTRDP_SCALE > 1
    if (ctx->fb_color_write_pending)
        fb_extent(ctx, &segment.color_bytes, &segment.depth_bytes);
    if (!ctx->fb_depth_access_pending) segment.depth_bytes = 0;
#endif
    return segment;
}

static void clear_segment_writes(sr_context *ctx)
{
    ctx->fb_color_write_pending = false;
    ctx->fb_depth_write_pending = false;
    ctx->fb_depth_access_pending = false;
}

/* A worker barrier is a segment inside a job, not a producer queue dispatch. */
static bool cmdbuf_cut(sr_context *ctx)
{
    if (ctx->segment_start == ctx->cmd_buffer_words) return true;
    if (ctx->segment_count == ctx->segment_capacity) {
        uint32_t capacity = ctx->segment_capacity ? ctx->segment_capacity * 2u : 16u;
        command_batch *grown = realloc(ctx->segments, capacity * sizeof(*grown));
        if (!grown) return false;
        ctx->segments = grown;
        ctx->segment_capacity = capacity;
    }
    ctx->segments[ctx->segment_count++] = pending_segment(ctx);
    ctx->segment_start = ctx->cmd_buffer_words;
    clear_segment_writes(ctx);
    return true;
}

static void execute_queued_batch(void *opaque)
{
    queued_batch *batch = opaque;
    for (uint32_t i = 0; i < batch->segment_count; i++) {
        command_batch segment = batch->segments[i];
        segment.words = batch->words + segment.first;
        execute_batch(&segment);
    }
    if (batch->tail.count) {
        batch->tail.words = batch->words + batch->tail.first;
        execute_batch(&batch->tail);
    }
}

static void execute_owned_batch(void *opaque)
{
    queued_batch *batch = opaque;
    execute_queued_batch(batch);
    free(batch->segments);
    free(batch->words);
    free(batch);
}

static void cmdbuf_flush(sr_context *ctx)
{
    if (!ctx->cmd_buffer_words) return;
    queued_batch batch = {ctx, ctx->cmd_buffer, ctx->segments,
                          ctx->segment_count, pending_segment(ctx)};
    queued_batch *owned = ctx->executor ? malloc(sizeof(*owned)) : NULL;
    if (owned) {
        *owned = batch;
        ctx->cmd_buffer = NULL;
        ctx->cmd_buffer_capacity = 0;
        ctx->segments = NULL;
        ctx->segment_capacity = 0;
        ctx->epoch_batches++;
        ctx->debug.submitted_batches++;
        sr_async_submit(ctx->executor, execute_owned_batch, owned);
    } else {
        sr_async_wait(ctx->executor);
        if (ctx->executor) primitive_cache_reset();
        execute_queued_batch(&batch);
        if (ctx->executor) primitive_cache_reset();
    }
    ctx->cmd_buffer_words = 0;
    ctx->segment_count = ctx->segment_start = 0;
    clear_segment_writes(ctx);
}

/*
 * Whether this command ends the batch, asked WITHOUT touching the tracked
 * state. Seal the old target's immutable import extent before tracking the
 * following target. These cuts order RDP workers; they do not wait for the CPU
 * or refresh the 2x reference image.
 */
static bool hazard_needs_flush(const sr_context *ctx, const rdp_command *cmd)
{
    /* Switching either render target is itself a flush point; that is what lets
     * a single tracked framebuffer suffice, since pending writes then always
     * belong to the current target. The depth buffer counts too, so each batch
     * carries the correct destination range for first-use preparation. */
    switch (cmd->id) {
    case RDP_CMD_SET_COLOR_IMAGE:
        return cmd->decoded.set_color_image.address != ctx->fb_addr ||
               cmd->decoded.set_color_image.width != ctx->fb_width ||
               image_bytes_per_pixel(cmd->decoded.set_color_image.size) != ctx->fb_bpp;
    case RDP_CMD_SET_MASK_IMAGE:
        return cmd->decoded.set_mask_image.address != ctx->fb_depth_addr;
    /* A texture load overlapping a render target has to see the pending batch's
     * output. This lives here rather than in hazard_track because only the
     * answer matters - the load mutates none of the tracked state - and
     * hazard_track's return value is not consulted. */
    case RDP_CMD_LOAD_TILE:
    case RDP_CMD_LOAD_BLOCK:
    case RDP_CMD_LOAD_TLUT:
        return tmem_load_needs_flush(ctx);
    default:
        return false;
    }
}

/* Applied after any flush the command triggered, so the new state belongs to
 * the batch that follows it. */
static bool hazard_track(sr_context *ctx, const rdp_command *cmd)
{
    switch (cmd->id) {
    case RDP_CMD_SET_COLOR_IMAGE: {
        const uint32_t addr = cmd->decoded.set_color_image.address;
        const uint32_t width = cmd->decoded.set_color_image.width;
        const uint32_t bpp = image_bytes_per_pixel(cmd->decoded.set_color_image.size);
        const bool changed = addr != ctx->fb_addr || width != ctx->fb_width ||
                             bpp != ctx->fb_bpp;
        /* A different buffer has its own extent; nothing carries over. */
        if (changed) ctx->fb_height = 0u;
        ctx->fb_addr = addr;
        ctx->fb_width = width;
        ctx->fb_bpp = bpp;
        return changed;
    }
    case RDP_CMD_SET_TEXTURE_IMAGE:
        ctx->tex_addr = cmd->decoded.set_texture_image.address;
        return false;
    case RDP_CMD_SET_MASK_IMAGE: {
        const uint32_t addr = cmd->decoded.set_mask_image.address;
        const bool changed = addr != ctx->fb_depth_addr;
        ctx->fb_depth_addr = addr;
        return changed;
    }
    case RDP_CMD_SET_SCISSOR:
        ctx->scissor_yhi = cmd->decoded.set_scissor.y1;
        return false;
    case RDP_CMD_SET_OTHER_MODES:
        ctx->fb_z_update = cmd->decoded.set_other_modes.z_update;
        ctx->fb_z_compare = cmd->decoded.set_other_modes.z_compare;
        return false;

    case RDP_CMD_FILL_TRIANGLE:
    case RDP_CMD_FILL_ZBUFFER_TRIANGLE:
    case RDP_CMD_TEXTURE_TRIANGLE:
    case RDP_CMD_TEXTURE_ZBUFFER_TRIANGLE:
    case RDP_CMD_SHADE_TRIANGLE:
    case RDP_CMD_SHADE_ZBUFFER_TRIANGLE:
    case RDP_CMD_SHADE_TEXTURE_TRIANGLE:
    case RDP_CMD_SHADE_TEXTURE_ZBUFFER_TRIANGLE:
        /* The triangle's low edge, clipped by the scissor, in quarter rows. */
        fb_note_height(ctx, (uint32_t)cmd->decoded.triangle.position.yl);
        fb_note_draw(ctx);
        return false;
    case RDP_CMD_TEXTURE_RECTANGLE:
    case RDP_CMD_TEXTURE_RECTANGLE_FLIP:
    case RDP_CMD_FILL_RECTANGLE:
        /* Rectangle edges are whole pixels; scale to the same quarter rows. */
        fb_note_height(ctx, (cmd->decoded.rect.y1 + 1u) << 2);
        fb_note_draw(ctx);
        return false;

    case RDP_CMD_LOAD_TILE:
    case RDP_CMD_LOAD_BLOCK:
    case RDP_CMD_LOAD_TLUT:
        return tmem_load_needs_flush(ctx);

    default:
        return false;
    }
}

static void rdp_state_init(rdp_state *state)
{
    memset(state, 0, sizeof(*state));

    state->color_image.format = RDP_FORMAT_RGBA;
    state->color_image.size = RDP_SIZE_16BPP;
    state->color_image.width = 320;

    state->texture_image.format = RDP_FORMAT_RGBA;
    state->texture_image.size = RDP_SIZE_16BPP;
    state->texture_image.width = 1;

    state->scissor_x1 = 640u << 2;
    state->scissor_y1 = 480u << 2;
    state->worker_offset = 0u;
    state->worker_stride = 1u;
    state->other_modes.cycle_type = RDP_CYCLE_1;
    rdp_combiner_make_passthrough(&state->combiner,
                                  RDP_COMBINER_TEXEL0_RGB,
                                  RDP_COMBINER_TEXEL0_ALPHA);
}

static void capture_texture_debug(sr_debug_stats *debug, const rdp_state *state, const rdp_command *cmd)
{
    if (!debug || !state || !cmd) {
        return;
    }

    uint32_t tile_index = 0;
    debug->last_texture_image_format = (uint32_t)state->texture_image.format;
    debug->last_texture_image_size = (uint32_t)state->texture_image.size;
    debug->last_texture_image_width = state->texture_image.width;
    debug->last_texture_image_address = state->texture_image.address;
    debug->last_load_sl = 0;
    debug->last_load_tl = 0;
    debug->last_load_sh = 0;
    debug->last_load_th = 0;
    debug->last_rect_s0 = 0;
    debug->last_rect_t0 = 0;
    debug->last_rect_dsdx = 0;
    debug->last_rect_dtdy = 0;

    switch (cmd->id) {
    case RDP_CMD_LOAD_TLUT:
    case RDP_CMD_LOAD_BLOCK:
    case RDP_CMD_LOAD_TILE:
        tile_index = cmd->decoded.load.tile_index;
        debug->last_load_sl = cmd->decoded.load.sl;
        debug->last_load_tl = cmd->decoded.load.tl;
        debug->last_load_sh = cmd->decoded.load.sh;
        debug->last_load_th = cmd->decoded.load.th;
        break;
    case RDP_CMD_TEXTURE_RECTANGLE:
    case RDP_CMD_TEXTURE_RECTANGLE_FLIP:
        tile_index = cmd->decoded.rect.tile_index;
        debug->last_rect_s0 = cmd->decoded.rect.s0;
        debug->last_rect_t0 = cmd->decoded.rect.t0;
        debug->last_rect_dsdx = cmd->decoded.rect.dsdx;
        debug->last_rect_dtdy = cmd->decoded.rect.dtdy;
        break;
    case RDP_CMD_TEXTURE_TRIANGLE:
    case RDP_CMD_TEXTURE_ZBUFFER_TRIANGLE:
    case RDP_CMD_SHADE_TEXTURE_TRIANGLE:
    case RDP_CMD_SHADE_TEXTURE_ZBUFFER_TRIANGLE:
        tile_index = cmd->decoded.triangle.position.tile & 7u;
        break;
    default:
        return;
    }

    tile_index &= 7u;
    const rdp_tile *tile = &state->tiles[tile_index];
    debug->last_tile_index = tile_index;
    debug->last_tile_format = (uint32_t)tile->format;
    debug->last_tile_size = (uint32_t)tile->size;
    debug->last_tile_tmem = tile->tmem;
    debug->last_tile_line = tile->line;
    debug->last_tile_sl = tile->sl;
    debug->last_tile_tl = tile->tl;
    debug->last_tile_sh = tile->sh;
    debug->last_tile_th = tile->th;
}

/* Live renderer contexts. The worker pool is process-wide but exists only to
 * serve them, so it is torn down with the last one. This counts create/destroy
 * pairs - e.g. a plugin rebuilding its context on ROM switch - not concurrent
 * use; a context is not thread-safe to begin with. */
static uint32_t g_live_contexts;

sr_context *sr_create(const sr_host_interface *host)
{
    /* TMEM/state members can require wider alignment than malloc guarantees. */
    const size_t alignment = _Alignof(sr_context);
    void *allocation = calloc(1, sizeof(sr_context) + alignment - 1u);
    if (!allocation) {
        return NULL;
    }
    sr_context *ctx = (sr_context *)(((uintptr_t)allocation + alignment - 1u) &
                                    ~(uintptr_t)(alignment - 1u));
    ctx->allocation = allocation;
    ctx->batch_target_words = 8192u;

    sr_set_host(ctx, host);
    rdp_state_init(&ctx->rdp);
    tmem_init(&ctx->tmem);
    vi_init(&ctx->vi);
    ctx->scissor_yhi = 480u << 2;  /* matches rdp_state_init's scissor default */
    primitive_cache_reset();
    g_live_contexts++;
    if (ctx->worker_count > 1u) ctx->executor = sr_async_create();
    return ctx;
}

void sr_flush(sr_context *ctx)
{
    if (!ctx) return;
    cmdbuf_flush(ctx);
    sr_async_wait(ctx->executor);
#if SOFTRDP_SCALE > 1
    for (uint32_t i = 0; i < ctx->prepared_count; i++) {
        const prepared_range range = ctx->prepared[i];
        sr_memory_take_reference(&ctx->memory, range.begin, range.end - range.begin);
    }
    ctx->prepared_count = 0;
    ctx->reference_fallback = false;
#endif
    if (ctx->epoch_words) {
        uint64_t target = (ctx->epoch_words + 2u) / 3u;
        ctx->batch_target_words = target < 8192u ? 8192u :
                                  target > 32768u ? 32768u : (uint32_t)target;
        ctx->epoch_words = 0;
    }
    ctx->epoch_batches = 0;
}




uint32_t sr_debug_read_sample(const sr_context *ctx, uint32_t address,
                              uint32_t sample)
{
    if (!ctx || !ctx->memory.rdram) return 0u;
    return sr_memory_read_be16_sample(&ctx->memory,
                                      mask_addr(&ctx->memory, address), sample);
}

void sr_destroy(sr_context *ctx)
{
    if (!ctx) return;
    sr_async_destroy(ctx->executor);
    free(ctx->segments);
#if SOFTRDP_SCALE > 1
    free(ctx->prepared);
#endif
    free(ctx->cmd_buffer);
    sr_memory_release(&ctx->memory);
    free(ctx->allocation);

    /* Join the workers once nothing is left to render. Letting them run to
     * process exit is survivable on Windows, but this code ships as a plugin
     * that the host can unload: a worker parked in a library that has just been
     * unmapped takes the process down on the next plugin swap. */
    if (g_live_contexts > 0u && --g_live_contexts == 0u) {
        sr_threads_shutdown();
        sr_vi_threads_shutdown();
    }
}

void sr_set_host(sr_context *ctx, const sr_host_interface *host)
{
    if (!ctx) {
        return;
    }

    sr_async_wait(ctx->executor);
    if (host) {
        ctx->host = *host;
    } else {
        memset(&ctx->host, 0, sizeof(ctx->host));
    }

    sr_memory_init(&ctx->memory, &ctx->host);
    ctx->worker_count = sr_resolve_worker_count(ctx->host.workers);
#if SOFTRDP_SCALE > 1
    ctx->prepared_count = 0;
#endif
}

static sr_result sr_process_rdp_list_internal(sr_context *ctx)
{
    uint32_t current;
    uint32_t end;
    bool xbus_dma;

    current = read_reg(ctx->host.dp_regs, SR_DP_CURRENT) & ~7u;
    end = read_reg(ctx->host.dp_regs, SR_DP_END) & ~7u;
    xbus_dma = (read_reg(ctx->host.dp_regs, SR_DP_STATUS) & DP_STATUS_XBUS_DMA) != 0;
    ctx->debug.last_list_current = current;
    ctx->debug.last_list_end = end;
    ctx->debug.last_list_bytes = end > current ? end - current : 0u;
    ctx->debug.last_command_address = current;
    ctx->debug.last_command_id = 0u;
    ctx->debug.last_result = SR_OK;

    if (end <= current) {
        return SR_OK;
    }

    /* Accumulate complete commands into the batch buffer; the stateful reader
     * carries a command split across list boundaries. Rendering is deferred to
     * cmdbuf_flush, triggered by SYNC_FULL (or an explicit sr_flush). */
    while (current < end) {
        if (ctx->cmd_words_loaded == 0) {
            uint32_t first_word = 0;
            if (!read_command_word(ctx, xbus_dma, current >> 2, &first_word)) {
                ctx->debug.last_command_address = current;
                ctx->debug.last_result = SR_ERROR_BAD_COMMAND;
                finish_rdp_list(ctx);
                return SR_ERROR_BAD_COMMAND;
            }

            ctx->cmd_id = (uint8_t)((first_word >> 24) & 0x3fu);
            ctx->cmd_word_count = rdp_command_word_count((rdp_command_id)ctx->cmd_id);
            if (ctx->cmd_word_count == 0 || ctx->cmd_word_count > SR_MAX_COMMAND_WORDS) {
                ctx->debug.last_result = SR_ERROR_BAD_COMMAND;
                finish_rdp_list(ctx);
                return SR_ERROR_BAD_COMMAND;
            }

            ctx->cmd_words[0] = first_word;
            ctx->cmd_words_loaded = 1;
            ctx->cmd_current_address = current;
            ctx->debug.last_command_address = ctx->cmd_current_address;
            ctx->debug.last_command_id = (uint32_t)ctx->cmd_id;
            current += 4u;
        }

        while (ctx->cmd_words_loaded < ctx->cmd_word_count) {
            if (current >= end) {
                /* Command is truncated in this list. Wait for the next list. */
                finish_rdp_list(ctx);
                return SR_OK;
            }

            uint32_t word = 0;
            if (!read_command_word(ctx, xbus_dma, current >> 2, &word)) {
                ctx->debug.last_result = SR_ERROR_BAD_COMMAND;
                finish_rdp_list(ctx);
                return SR_ERROR_BAD_COMMAND;
            }

            ctx->cmd_words[ctx->cmd_words_loaded++] = word;
            current += 4u;
        }

        const rdp_command_id id = (rdp_command_id)ctx->cmd_id;

        if (ctx->host.trace_rdp_command) {
            ctx->host.trace_rdp_command(ctx->host.userdata,
                                        ctx->cmd_current_address,
                                        (uint32_t)id, ctx->cmd_words,
                                        ctx->cmd_word_count);
        }

        /* Decode a copy purely to inspect it: hazard tracking must see the
         * command before it is buffered, since the workers only execute it at
         * flush time. */
        rdp_command probe;
        probe.id = id;
        probe.word_count = (uint8_t)ctx->cmd_word_count;
        memcpy(probe.words, ctx->cmd_words, ctx->cmd_word_count * sizeof(uint32_t));
        if (rdp_decode_command(&probe) == SR_OK) {
            if (hazard_needs_flush(ctx, &probe)) {
                if (!ctx->executor || !cmdbuf_cut(ctx)) cmdbuf_flush(ctx);
            }
            hazard_track(ctx, &probe);
        }

        if (!cmdbuf_append(ctx, ctx->cmd_words, ctx->cmd_word_count)) {
            /* Out of memory: render what we have to bound the buffer, then
             * retry the append. */
            cmdbuf_flush(ctx);
            if (rdp_decode_command(&probe) == SR_OK) hazard_track(ctx, &probe);
            if (!cmdbuf_append(ctx, ctx->cmd_words, ctx->cmd_word_count)) {
                ctx->cmd_words_loaded = 0;
                ctx->debug.last_result = SR_ERROR_INVALID_ARGUMENT;
                finish_rdp_list(ctx);
                return SR_ERROR_INVALID_ARGUMENT;
            }
        }
        ctx->epoch_words += ctx->cmd_word_count;
        ctx->cmd_words_loaded = 0;

        /* Only a few coarse jobs per epoch; serial replay has no storage cuts.
         * Adapt to the previous epoch, keeping a 32 KiB minimum job size. */
        if (ctx->executor && ctx->epoch_batches < 2u &&
            ctx->cmd_buffer_words >= ctx->batch_target_words)
            cmdbuf_flush(ctx);

        if (id == RDP_CMD_SYNC_FULL) {
            /* SYNC_FULL is the LOW-profile flush point: render the batch, then
             * raise the interrupt so the CPU sees completed rendering. */
            sr_flush(ctx);
            raise_dp_interrupt(ctx);
        }
    }

    finish_rdp_list(ctx);
    return SR_OK;
}

sr_result sr_process_rdp_list(sr_context *ctx)
{
    if (!ctx) {
        return SR_ERROR_INVALID_ARGUMENT;
    }

    sr_result result = sr_process_rdp_list_internal(ctx);
    if (result == SR_OK) {
        vi_latch_registers(&ctx->vi, &ctx->host);
    }

    return result;
}

/* Recorded on every plan build so a black frame can be told apart from a frame
 * that was never asked for. */
/*
 * The buffer the VI is about to display, whoever wrote it. This is the path
 * that catches content the renderer never drew - a CPU-blitted HUD, a decoded
 * movie frame - including a small patch written into a buffer the renderer HAS
 * drawn, which nothing else here would notice.
 */
#if SOFTRDP_SCALE > 1
static void scanout_window(const sr_context *ctx, uint32_t *start, uint32_t *bytes)
{
    const vi_scanout_plan *plan = &ctx->vi_plan;
    const uint32_t size = ctx->memory.rdram_size;
    *start = *bytes = 0;
    if (plan->state != VI_SCANOUT_READY || !plan->output_height || !size) return;
    const uint32_t rows = (plan->active_y_end - plan->active_y_begin) * SOFTRDP_SCALE;
    if (!rows) return;
    const uint32_t last_coordinate = plan->sample_y_start + (rows - 1u) * plan->sample_y_add;
    int32_t first_row = (int32_t)(plan->sample_y_start >> 10);
    uint32_t last_row = last_coordinate >> 10;
    if (plan->aa_mode != 3u && ((last_coordinate >> 5) & 31u)) last_row++;
    const bool aa = plan->aa_mode < 2u;
    if (aa) { first_row--; last_row++; }
    first_row >>= SR_SCALE_LOG2;
    last_row >>= SR_SCALE_LOG2;
    const uint32_t x_border = aa ? 1u : 0u;
    const int64_t begin = (int64_t)plan->origin +
        ((int64_t)first_row * plan->source_stride - x_border) * plan->bytes_per_pixel;
    const int64_t end = (int64_t)plan->origin +
        ((int64_t)last_row * plan->source_stride + plan->source_width + x_border) *
        plan->bytes_per_pixel;
    const int64_t aligned_begin = begin & ~(int64_t)3;
    const uint64_t length = (uint64_t)(((end + 3) & ~(int64_t)3) - aligned_begin);
    *start = mask_addr(&ctx->memory, (uint32_t)aligned_begin);
    *bytes = length >= size ? size : (uint32_t)length;
}
#endif

static void import_scanout_range(sr_context *ctx)
{
#if SOFTRDP_SCALE > 1
    uint32_t start, remaining;
    scanout_window(ctx, &start, &remaining);
    while (remaining) {
        uint32_t available = ctx->memory.rdram_size - start;
        uint32_t bytes = remaining < available ? remaining : available;
        sr_memory_import_foreign(&ctx->memory, start, bytes);
        remaining -= bytes;
        start = 0;
    }
#else
    (void)ctx;
#endif
}

static void note_scanout(sr_context *ctx)
{
    ctx->debug.scanout_origin = ctx->vi_plan.origin;
    ctx->debug.scanout_state = ctx->vi_plan.state;
    ctx->debug.scanout_width = ctx->vi_plan.scanout_width;
    ctx->debug.scanout_height = ctx->vi_plan.scanout_height;
}

sr_result sr_update_screen(sr_context *ctx, sr_framebuffer *out)
{
    if (!ctx || !out) {
        return SR_ERROR_INVALID_ARGUMENT;
    }

    if (!ctx->vi_plan_prepared) {
        vi_latch_registers(&ctx->vi, &ctx->host);
        vi_build_scanout_plan(&ctx->vi, &ctx->memory, &ctx->vi_plan);
        ctx->vi_plan.dither_frame = ctx->vi_frame_count;
        note_scanout(ctx);
    }
    ctx->vi_plan_prepared = false;
    /* The CPU may have written since sr_get_vi_frame_info prepared the plan. */
    import_scanout_range(ctx);

    sr_result result = vi_execute_scanout_threaded(&ctx->vi_plan,
                                                  &ctx->memory,
                                                  out, ctx->worker_count > 1u);
    ctx->vi_frame_count++;
    /* Per-scanline registers describe one frame. */
    ctx->vi.scanline_count = 0u;
    return result;
}

sr_result sr_set_vi_scanline_registers(sr_context *ctx,
                                       const sr_vi_scanline_regs *regs,
                                       uint32_t count)
{
    if (!ctx || (count && !regs)) return SR_ERROR_INVALID_ARGUMENT;
    if (count > VI_MAX_SCANLINE_REGS) count = VI_MAX_SCANLINE_REGS;
    for (uint32_t i = 0; i < count; i++)
        ctx->vi.scanline[i] = (vi_scanline_regs){
            regs[i].vi_line, regs[i].h_start, regs[i].x_scale
        };
    ctx->vi.scanline_count = count;
    ctx->vi_plan_prepared = false;
    return SR_OK;
}

sr_result sr_get_vi_frame_info(sr_context *ctx, sr_vi_frame_info *info)
{
    if (!ctx || !info) return SR_ERROR_INVALID_ARGUMENT;
    vi_latch_registers(&ctx->vi, &ctx->host);
    vi_build_scanout_plan(&ctx->vi, &ctx->memory, &ctx->vi_plan);
    ctx->vi_plan.dither_frame = ctx->vi_frame_count;
    note_scanout(ctx);
    ctx->vi_plan_prepared = true;
    info->width = ctx->vi_plan.scanout_width;
    info->height = ctx->vi_plan.scanout_height;
    info->display_width = ctx->vi_plan.display_width;
    info->display_height = ctx->vi_plan.display_height;
    info->display = ctx->vi_plan.state == VI_SCANOUT_READY;
    info->interlaced = ctx->vi_plan.serrate;
    info->field = (ctx->vi.current & 1u) != 0;
    info->hold = ctx->vi_plan.state == VI_SCANOUT_HOLD ||
                 ctx->vi_plan.state == VI_SCANOUT_INVALID_MEMORY;
    return SR_OK;
}

sr_debug_stats sr_get_debug_stats(const sr_context *ctx)
{
    sr_debug_stats stats = {0};

    if (ctx) {
        sr_async_wait(ctx->executor);
        stats = ctx->render_debug;
        stats.submitted_batches = ctx->debug.submitted_batches;
        stats.last_list_current = ctx->debug.last_list_current;
        stats.last_list_end = ctx->debug.last_list_end;
        stats.last_list_bytes = ctx->debug.last_list_bytes;
        stats.last_command_address = ctx->debug.last_command_address;
        stats.scanout_origin = ctx->debug.scanout_origin;
        stats.scanout_state = ctx->debug.scanout_state;
        stats.scanout_width = ctx->debug.scanout_width;
        stats.scanout_height = ctx->debug.scanout_height;
        if (ctx->debug.last_result != SR_OK) stats.last_result = ctx->debug.last_result;
    }

    return stats;
}
