#include "vi.h"

#include "raster.h"
#include "rdp_memory.h"
#include "sr_threads.h"

#include <string.h>

#define VI_TYPE_MASK 3u
#define VI_TYPE_RGBA5551 2u
#define VI_TYPE_RGBA8888 3u
#define VI_CONTROL_GAMMA_DITHER (1u << 2)
#define VI_CONTROL_GAMMA (1u << 3)
#define VI_CONTROL_DIVOT (1u << 4)
#define VI_CONTROL_SERRATE (1u << 6)
#define VI_CONTROL_AA_MODE_SHIFT 8u
#define VI_CONTROL_DITHER_FILTER (1u << 16)
#define VI_AA_RESAMP_ONLY 2u
#define VI_AA_REPLICATE 3u
#define VI_V_SYNC_NTSC 525u


static const uint8_t vi_gamma_flat[256] = {
      0,  16,  22,  26,  32,  34,  38,  42,  44,  48,  50,  52,  54,  56,  58,  60,
     64,  64,  66,  68,  70,  72,  74,  76,  78,  80,  80,  82,  84,  86,  86,  88,
     90,  90,  92,  94,  96,  96,  98,  98, 100, 102, 102, 104, 106, 106, 108, 108,
    110, 112, 112, 114, 114, 116, 116, 118, 118, 120, 120, 122, 122, 124, 124, 126,
    128, 128, 128, 130, 130, 132, 132, 134, 134, 136, 136, 138, 138, 140, 140, 142,
    142, 144, 144, 144, 146, 146, 148, 148, 150, 150, 150, 152, 152, 154, 154, 154,
    156, 156, 158, 158, 160, 160, 160, 162, 162, 162, 164, 164, 166, 166, 166, 168,
    168, 170, 170, 170, 172, 172, 172, 174, 174, 176, 176, 176, 178, 178, 178, 180,
    180, 180, 182, 182, 182, 184, 184, 184, 186, 186, 186, 188, 188, 188, 190, 190,
    192, 192, 192, 192, 194, 194, 194, 196, 196, 196, 198, 198, 198, 200, 200, 200,
    202, 202, 202, 204, 204, 204, 206, 206, 206, 208, 208, 208, 208, 210, 210, 210,
    212, 212, 212, 214, 214, 214, 214, 216, 216, 216, 218, 218, 218, 218, 220, 220,
    220, 222, 222, 222, 224, 224, 224, 224, 226, 226, 226, 226, 228, 228, 228, 230,
    230, 230, 230, 232, 232, 232, 234, 234, 234, 234, 236, 236, 236, 236, 238, 238,
    238, 240, 240, 240, 240, 242, 242, 242, 242, 244, 244, 244, 244, 246, 246, 246,
    246, 248, 248, 248, 248, 250, 250, 250, 250, 252, 252, 252, 252, 254, 254, 254,
};
#define VI_H_OFFSET_NTSC 108
#define VI_H_OFFSET_PAL 128
#define VI_V_OFFSET_NTSC 34
#define VI_V_OFFSET_PAL 44

/*
 * Two source rows are live at once for vertical interpolation, and each is
 * decoded once per sub-row, so the cache grows with the scale rather than
 * thrashing between the sub-rows of one source line.
 */
#define VI_ROW_CACHE_SLOTS (2u * SOFTRDP_SCALE)

typedef struct vi_row_cache {
    sr_rgba8 rows[VI_ROW_CACHE_SLOTS][VI_MAX_SOURCE_WIDTH * SOFTRDP_SCALE];
    uint32_t row_index[VI_ROW_CACHE_SLOTS];
    uint32_t sub_row[VI_ROW_CACHE_SLOTS];
    bool fetch_bug[VI_ROW_CACHE_SLOTS];
    bool valid[VI_ROW_CACHE_SLOTS];
    uint32_t next_slot;
    /* The raster rows held in decode_row_filtered's three texel buffers. Kept
     * here, reset with the cache, so no texel outlives the scanout it was
     * fetched for. */
    int32_t texel_row[3];
    bool texel_valid[3];
} vi_row_cache;

void vi_init(vi_state *vi)
{
    if (vi) memset(vi, 0, sizeof(*vi));
}

void vi_latch_registers(vi_state *vi, const sr_host_interface *host)
{
    if (!vi || !host) return;
#define LATCH(reg) (host->vi_regs[reg] ? *host->vi_regs[reg] : 0u)
    vi->control = LATCH(SR_VI_STATUS);
    vi->origin = LATCH(SR_VI_ORIGIN);
    vi->width = LATCH(SR_VI_WIDTH);
    vi->current = LATCH(SR_VI_CURRENT);
    vi->v_sync = LATCH(SR_VI_V_SYNC);
    vi->h_start = LATCH(SR_VI_H_START);
    vi->v_start = LATCH(SR_VI_V_START);
    vi->x_scale = LATCH(SR_VI_X_SCALE);
    vi->y_scale = LATCH(SR_VI_Y_SCALE);
    vi->disable_dither_filter = host->disable_vi_dither_filter;
    vi->disable_divot_filter = host->disable_vi_divot_filter;
    vi->disable_gamma_dither = host->disable_vi_gamma_dither;
    vi->disable_aa = host->disable_vi_aa;
#undef LATCH
}

static void vi_build_blank_canvas(const vi_state *vi, bool pal,
                                  vi_scanout_plan *plan)
{
    const uint32_t canvas_height = pal ? 288u : 240u;
    const bool serrate = (vi->control & VI_CONTROL_SERRATE) != 0;
    plan->state = VI_SCANOUT_READY;
    plan->output_width = VI_MAX_OUTPUT_WIDTH;
    plan->output_height = canvas_height;
    plan->scanout_width = plan->output_width * (uint32_t)SOFTRDP_SCALE;
    plan->scanout_height = plan->output_height * (uint32_t)SOFTRDP_SCALE;
    plan->display_width = plan->output_width;
    plan->display_height = (uint32_t)(((uint64_t)plan->output_height *
                           2u * VI_V_SYNC_NTSC) /
                           ((vi->v_sync & 0x3ffu) ?
                            (vi->v_sync & 0x3ffu) : VI_V_SYNC_NTSC));
    if (!plan->display_height)
        plan->display_height = plan->output_height;
    plan->serrate = serrate;
}

/*
 * The horizontal window one H_START/X_SCALE pair describes, as the frame's own
 * is built below: the columns that carry the framebuffer, without the 8/7-pixel
 * guard bands unless that edge is clamped to the screen, and the sampling walk
 * across them. Returns the largest source x an active column reads.
 */
static uint32_t vi_compute_x_window(uint32_t h_start_reg, uint32_t x_scale_reg,
                                    int32_t h_offset, uint32_t type,
                                    uint32_t aa_mode, vi_x_window *window)
{
    const uint32_t x_add = x_scale_reg & 0xfffu;
    uint32_t x_start = (x_scale_reg >> 16) & 0xfffu;
    const int32_t h_start_raw = (int32_t)((h_start_reg >> 16) & 0x3ffu);
    int32_t h_start = h_start_raw - h_offset;
    int32_t h_end = (int32_t)(h_start_reg & 0x3ffu) - h_offset;
    bool left_clamp = false;
    bool right_clamp = false;
    if (h_start < 0) {
        x_start += x_add * (uint32_t)(-h_start);
        h_start = 0;
        left_clamp = true;
    }
    if (h_end > (int32_t)VI_MAX_OUTPUT_WIDTH) {
        h_end = VI_MAX_OUTPUT_WIDTH;
        right_clamp = true;
    }
    int32_t begin = h_start + (left_clamp ? 0 : 8);
    int32_t end = h_end - (right_clamp ? 0 : 7);
    if (begin < 0) begin = 0;
    if (end < 0) end = 0;
    if (begin > (int32_t)VI_MAX_OUTPUT_WIDTH) begin = VI_MAX_OUTPUT_WIDTH;
    if (end > (int32_t)VI_MAX_OUTPUT_WIDTH) end = VI_MAX_OUTPUT_WIDTH;
    if (end < begin) end = begin;
    window->active_x_begin = (uint32_t)begin;
    window->active_x_end = (uint32_t)end;
    window->sample_x_start = (x_start - (uint32_t)h_start * x_add) *
                             (uint32_t)SOFTRDP_SCALE;
    window->sample_x_add = x_add;
    window->replicate_fetch_mask = 0u;
    if (aa_mode == VI_AA_REPLICATE && x_add <= 0x200u) {
        const uint32_t fetch_mask = type == VI_TYPE_RGBA5551 ? 0x40u : 0x20u;
        if (h_start_raw < (int32_t)(fetch_mask << 1))
            window->replicate_fetch_mask = fetch_mask;
    }
    if (end <= begin) return 0u;
    return (x_start + (uint32_t)(end - 1 - h_start) * x_add) >> 10;
}

void vi_build_scanout_plan(const vi_state *vi, const sr_memory *memory,
                           vi_scanout_plan *plan)
{
    memset(plan, 0, sizeof(*plan));
    if (!vi || !memory || !memory->rdram) return;

    const uint32_t type = vi->control & VI_TYPE_MASK;
    /* Aligned to the pixel size, like the color image: scanout indexes whole
     * pixels, and the aligned base is what lets the per-pixel fetch skip the
     * end-of-RDRAM check. */
    const uint32_t origin_align = (vi->control & VI_TYPE_MASK) == VI_TYPE_RGBA8888 ? 3u : 1u;
    const uint32_t origin = ((vi->origin & 0x00ffffffu) & (memory->rdram_size - 1u)) &
                            ~origin_align;
    const uint32_t source_stride = vi->width & 0xfffu;
    const uint32_t x_add = vi->x_scale & 0xfffu;
    const uint32_t y_add = vi->y_scale & 0xfffu;
    uint32_t x_start = (vi->x_scale >> 16) & 0xfffu;
    uint32_t y_start = (vi->y_scale >> 16) & 0xfffu;
    int32_t h_start = (int32_t)((vi->h_start >> 16) & 0x3ffu);
    int32_t h_end = (int32_t)(vi->h_start & 0x3ffu);
    int32_t v_start = (int32_t)((vi->v_start >> 16) & 0x3ffu);
    int32_t v_end = (int32_t)(vi->v_start & 0x3ffu);
    const int32_t v_start_raw = v_start;
    const int32_t h_start_raw = h_start;
    const bool pal = (vi->v_sync & 0x3ffu) > VI_V_SYNC_NTSC + 25u;
    const int32_t h_offset = pal ? VI_H_OFFSET_PAL : VI_H_OFFSET_NTSC;
    const int32_t v_offset = pal ? VI_V_OFFSET_PAL : VI_V_OFFSET_NTSC;

    /* A non-RGBA pixel type is a genuine blank signal: leave state BLANK so the
     * presenter shows black. */
    if (type != VI_TYPE_RGBA5551 && type != VI_TYPE_RGBA8888) {
        return;
    }
    /* Valid pixel type but the frame is not renderable this refresh (framebuffer
     * or timing not fully programmed, e.g. H_START still zero). Hold the last
     * frame rather than flashing black. source_stride is the row pitch, not the
     * sampled width, so a wide stride is fine; the sampled extent is bounded
     * later (max_x < VI_MAX_SOURCE_WIDTH) and validated by the memory check. */
    if (origin == 0 || source_stride == 0 || x_add == 0 || y_add == 0 ||
        h_end <= h_start || v_end == v_start) {
        plan->state = VI_SCANOUT_HOLD;
        return;
    }

    h_start -= h_offset;
    h_end -= h_offset;
    bool left_clamp = false;
    bool right_clamp = false;
    if (h_start < 0) {
        x_start += x_add * (uint32_t)(-h_start);
        h_start = 0;
        left_clamp = true;
    }
    if (h_end > (int32_t)VI_MAX_OUTPUT_WIDTH) {
        h_end = VI_MAX_OUTPUT_WIDTH;
        right_clamp = true;
    }

    const int32_t window_x_begin = h_start;
    const int32_t window_width = h_end - h_start;
    const bool serrate = (vi->control & VI_CONTROL_SERRATE) != 0;
    const bool wrapped_v_window = serrate && v_end < v_start;
    const int32_t canvas_height = pal ? 288 : 240;
    int32_t field_height = (v_end - v_start_raw) / 2;
    /* A wrapped V_VIDEO window continues to the bottom of the active video
     * region. Give it a full-canvas candidate height, then let the same clamp
     * used by every window trim it to the space below its starting line. This
     * avoids encoding Perfect Dark's bottom placement as a separate height
     * formula. */
    if (wrapped_v_window)
        field_height = canvas_height;
    v_start = (v_start - v_offset) / 2;
    if (v_start < 0) {
        y_start += y_add * (uint32_t)(-v_start);
        v_start = 0;
    }
    if (field_height > canvas_height - v_start)
        field_height = canvas_height - v_start;
    const int32_t output_width = (int32_t)VI_MAX_OUTPUT_WIDTH;
    const int32_t output_height = canvas_height;
    if (window_width <= 0 ||
        window_x_begin >= (int32_t)VI_MAX_OUTPUT_WIDTH || h_end <= 0) {
        plan->state = VI_SCANOUT_HOLD;
        return;
    }
    if (field_height <= 0) {
        vi_build_blank_canvas(vi, pal, plan);
        return;
    }

    plan->origin = origin;
    plan->source_stride = source_stride;
    plan->bytes_per_pixel = type == VI_TYPE_RGBA5551 ? 2u : 4u;
    plan->output_width = (uint32_t)output_width;
    plan->output_height = (uint32_t)output_height;
    plan->scanout_width = plan->output_width * (uint32_t)SOFTRDP_SCALE;
    plan->scanout_height = plan->output_height * (uint32_t)SOFTRDP_SCALE;
    /* The VI blanks an 8-pixel left and 7-pixel right guard band unless that
     * edge is already clamped to the screen border. This is where the filter
     * window would sample past the framebuffer, so it also keeps the fetch from
     * reading beyond WIDTH into the next scanline. */
    const uint32_t left_border = left_clamp ? 0u : 8u;
    const uint32_t right_border = right_clamp ? 0u : 7u;
    int32_t active_x_begin = window_x_begin + (int32_t)left_border;
    int32_t active_x_end = h_end - (int32_t)right_border;
    if (active_x_begin < 0) active_x_begin = 0;
    if (active_x_end < 0) active_x_end = 0;
    if (active_x_begin > (int32_t)plan->output_width)
        active_x_begin = (int32_t)plan->output_width;
    if (active_x_end > (int32_t)plan->output_width)
        active_x_end = (int32_t)plan->output_width;
    plan->active_x_begin = (uint32_t)active_x_begin;
    plan->active_x_end = (uint32_t)active_x_end;
    if (plan->active_x_end < plan->active_x_begin)
        plan->active_x_end = plan->active_x_begin;
    plan->active_y_begin = (uint32_t)v_start;
    plan->active_y_end = plan->active_y_begin + (uint32_t)field_height;
    if (plan->active_y_end > (uint32_t)output_height)
        plan->active_y_end = (uint32_t)output_height;
    plan->display_width = plan->output_width;
    plan->display_height = (uint32_t)(((uint64_t)plan->output_height *
                           2u * VI_V_SYNC_NTSC) /
                           ((vi->v_sync & 0x3ffu) ?
                            (vi->v_sync & 0x3ffu) : VI_V_SYNC_NTSC));
    if (!plan->display_height) plan->display_height = plan->output_height;
    plan->aa_mode = (vi->control >> VI_CONTROL_AA_MODE_SHIFT) & 3u;
    /* Without AA the edge pass is skipped but the resample stays bilinear. */
    if (vi->disable_aa && plan->aa_mode < VI_AA_RESAMP_ONLY)
        plan->aa_mode = VI_AA_RESAMP_ONLY;
    plan->serrate = serrate;
    const bool gamma = (vi->control & VI_CONTROL_GAMMA) != 0;
    const bool gamma_dither = !vi->disable_gamma_dither &&
        (vi->control & VI_CONTROL_GAMMA_DITHER) != 0;
    plan->output_transform = gamma
        ? (gamma_dither ? VI_OUTPUT_GAMMA_DITHER : VI_OUTPUT_GAMMA)
        : (gamma_dither ? VI_OUTPUT_DITHER_ONLY : VI_OUTPUT_IDENTITY);
    plan->divot_enable = !vi->disable_divot_filter &&
        (vi->control & VI_CONTROL_DIVOT) != 0;
    plan->dither_filter_enable = !vi->disable_dither_filter &&
        (vi->control & VI_CONTROL_DITHER_FILTER) != 0;
    if (plan->aa_mode == VI_AA_REPLICATE && x_add <= 0x200u) {
        const uint32_t fetch_mask = type == VI_TYPE_RGBA5551 ? 0x40u : 0x20u;
        if (h_start_raw < (int32_t)(fetch_mask << 1))
            plan->replicate_fetch_mask = fetch_mask;
    }

    /* Scaled start, unchanged step: see sample_x_start in vi.h. */
    plan->sample_x_start = (x_start - (uint32_t)window_x_begin * x_add) *
                           (uint32_t)SOFTRDP_SCALE;
    plan->sample_x_add = x_add;
    plan->sample_y_start = y_start * (uint32_t)SOFTRDP_SCALE;
    plan->sample_y_add = y_add;

    uint32_t max_x = 0;
    uint32_t max_y = 0;
    for (uint32_t x = 0; x < plan->output_width; x++) {
        const uint32_t coordinate = x_start +
            (x - (uint32_t)window_x_begin) * x_add;
        const uint32_t source_x = (uint16_t)(coordinate >> 10);
        /* Only active columns are fetched, so blanked guard-band columns must
         * not widen the source extent or the memory range check. */
        if (x >= plan->active_x_begin && x < plan->active_x_end &&
            source_x > max_x)
            max_x = source_x;
    }
    plan->windows[0] = (vi_x_window){
        plan->sample_x_start, plan->sample_x_add,
        plan->active_x_begin, plan->active_x_end, plan->replicate_fetch_mask
    };
    /* Mid-frame H_START/X_SCALE writes: each window holds from its line to the
     * next one's, the frame's registers above it. */
    if (vi->scanline_count) {
        plan->per_row_x = true;
        const uint32_t count = vi->scanline_count < VI_MAX_SCANLINE_REGS
            ? vi->scanline_count : VI_MAX_SCANLINE_REGS;
        for (uint32_t i = 0; i < count; i++) {
            const uint32_t window_max = vi_compute_x_window(
                vi->scanline[i].h_start, vi->scanline[i].x_scale, h_offset, type,
                plan->aa_mode, &plan->windows[i + 1u]);
            if (window_max > max_x) max_x = window_max;
            int32_t row = ((int32_t)vi->scanline[i].vi_line - v_offset) / 2;
            if (row < 0) row = 0;
            for (uint32_t y = (uint32_t)row; y < plan->output_height; y++)
                plan->row_window[y] = (uint8_t)(i + 1u);
        }
    }
    for (uint32_t y = plan->active_y_begin; y < plan->active_y_end; y++) {
        const uint32_t field_y = y - plan->active_y_begin;
        const uint32_t coordinate = y_start +
            field_y * y_add;
        const uint32_t source_y = (uint16_t)(coordinate >> 10);
        if (source_y > max_y) max_y = source_y;
    }

    const bool interpolate = plan->aa_mode != VI_AA_REPLICATE;
    if (interpolate) {
        max_x++;
        max_y++;
    }
    if (max_x >= VI_MAX_SOURCE_WIDTH) {
        plan->state = VI_SCANOUT_HOLD;
        return;
    }
    plan->source_width = max_x + 1u;

    const uint64_t last_pixel = (uint64_t)max_y * source_stride + max_x;
    const uint64_t last_byte = (uint64_t)origin +
                               last_pixel * plan->bytes_per_pixel +
                               plan->bytes_per_pixel - 1u;
    if (last_byte >= memory->rdram_size) {
        plan->state = VI_SCANOUT_INVALID_MEMORY;
        return;
    }
    plan->state = VI_SCANOUT_READY;
}

/* One-time decode table for RGBA5551 texels (indexed by big-endian halfword). */
static sr_rgba8 vi_rgba5551_table[65536];
static bool vi_rgba5551_table_ready;

static void vi_build_rgba5551_table(void)
{
    for (uint32_t v = 0; v < 65536u; v++) {
        vi_rgba5551_table[v] = (sr_rgba8){
            (uint8_t)((v >> 8) & 0xf8u),
            (uint8_t)((v >> 3) & 0xf8u),
            (uint8_t)((v << 2) & 0xf8u),
            (uint8_t)((v & 1u) ? 0xffu : 0u)
        };
    }
    vi_rgba5551_table_ready = true;
}
typedef struct vi_aa_texel {
    uint8_t r, g, b;
    uint8_t coverage;
} vi_aa_texel;

/*
 * One AA tap, addressed in RASTER coordinates.
 *
 * The filter runs on the sample grid, so a neighbour is one SAMPLE away rather
 * than one hardware pixel - parallel-rdp does the same by scaling the VI's
 * max_x/max_y up before it builds the AA image, which puts its whole post chain
 * on the upscaled grid. The address still comes from the hardware pixel that
 * contains the sample; sr_sample_of picks which of its samples to read.
 *
 * Signed, because taps step off the left edge and above the first row. The
 * arithmetic shift floors into the previous hardware pixel, and mask_addr wraps
 * the result - RDRAM wraps too, and the pixel that lands there is the one the
 * hardware would read. At scale 1 this is exactly the old linear arithmetic:
 * the shifts vanish and the sample folds to 0.
 */
static inline vi_aa_texel vi_fetch_texel(const vi_scanout_plan *plan,
                                         const sr_memory *memory,
                                         int32_t raster_x, int32_t raster_y)
{
    const int32_t pixel_x = raster_x >> SR_SCALE_LOG2;
    const int32_t pixel_y = raster_y >> SR_SCALE_LOG2;
    const uint32_t pixel =
        (uint32_t)(pixel_y * (int32_t)plan->source_stride + pixel_x);
    const uint32_t sample =
        sr_sample_of((uint32_t)raster_x, (uint32_t)raster_y);

    vi_aa_texel texel;
    if (plan->bytes_per_pixel == 2u) {
        const uint32_t address = mask_addr(memory, plan->origin + pixel * 2u);
        const uint16_t raw = sr_memory_read_be16_sample(memory, address, sample);
        const sr_rgba8 color = vi_rgba5551_table[raw];
        texel.r = color.r;
        texel.g = color.g;
        texel.b = color.b;
        texel.coverage = (uint8_t)(((raw & 1u) << 2) |
            sr_memory_read_hidden_sample(memory, address, raw, sample));
    } else {
        const uint32_t address = mask_addr(memory, plan->origin + pixel * 4u);
        const uint32_t raw = sr_memory_read_be32_sample(memory, address, sample);
        texel.r = (uint8_t)(raw >> 24);
        texel.g = (uint8_t)(raw >> 16);
        texel.b = (uint8_t)(raw >> 8);
        /* 32bpp keeps all three coverage bits in the alpha byte. */
        texel.coverage = (uint8_t)((raw >> 5) & 7u);
    }
    return texel;
}

/*
 * Fold one neighbour into the running lowest/highest and second lowest/highest.
 * Tracking both incrementally costs four min/max per channel and avoids
 * collecting the candidates first and scanning them twice.
 */
static inline void vi_aa_fold(vi_aa_texel neighbour, uint8_t lo[3], uint8_t hi[3],
                              uint8_t second_lo[3], uint8_t second_hi[3])
{
    if (neighbour.coverage != 7u) return;
    const uint8_t value[3] = { neighbour.r, neighbour.g, neighbour.b };
    for (uint32_t c = 0; c < 3u; c++) {
        const uint8_t v = value[c];
        const uint8_t raised = v > lo[c] ? v : lo[c];
        const uint8_t lowered = v < hi[c] ? v : hi[c];
        if (raised < second_lo[c]) second_lo[c] = raised;
        if (lowered > second_hi[c]) second_hi[c] = lowered;
        if (v < lo[c]) lo[c] = v;
        if (v > hi[c]) hi[c] = v;
    }
}

/* The same six taps the hardware reads, now one sample apart rather than one
 * pixel: left/right up, two out horizontally, left/right down. */
static const int32_t vi_aa_tap_x[6] = { -1, +1, -2, +2, -1, +1 };
static const int32_t vi_aa_tap_y[6] = { -1, -1,  0,  0, +1, +1 };

/*
 * Filter one partially covered pixel in place from its six unfiltered
 * neighbours. The centre colour is whatever the decode already produced, so
 * this never re-reads or re-decodes it.
 */
static void vi_aa_resolve(const vi_aa_texel taps[6], uint32_t coverage,
                          sr_rgba8 *restrict out)
{
    const uint8_t centre[3] = { out->r, out->g, out->b };
    uint8_t lo[3] = { centre[0], centre[1], centre[2] };
    uint8_t hi[3] = { centre[0], centre[1], centre[2] };
    uint8_t second_lo[3] = { centre[0], centre[1], centre[2] };
    uint8_t second_hi[3] = { centre[0], centre[1], centre[2] };
    for (uint32_t i = 0; i < 6u; i++)
        vi_aa_fold(taps[i], lo, hi, second_lo, second_hi);

    /* Unsigned throughout: the offset may borrow, but the final truncation to
     * eight bits makes the shift's sign irrelevant. */
    const uint32_t coeff = 7u - coverage;
    uint8_t result[3];
    for (uint32_t c = 0; c < 3u; c++) {
        const uint32_t offset = (uint32_t)second_lo[c] + second_hi[c] -
                                ((uint32_t)centre[c] << 1);
        result[c] = (uint8_t)(((offset * coeff + 4u) >> 3) + centre[c]);
    }
    out->r = result[0];
    out->g = result[1];
    out->b = result[2];
}

/* The per-pixel AA path of the unfiltered decode loops: the neighbours come
 * straight from RDRAM, so they are unfiltered however far the row has got. */
static void vi_filter_pixel(const vi_scanout_plan *plan, const sr_memory *memory,
                            int32_t raster_x, int32_t raster_y, uint32_t coverage,
                            bool fetch_bug,
                            sr_rgba8 *restrict out)
{
    vi_aa_texel taps[6];
    for (uint32_t i = 0; i < 6u; i++) {
        const uint32_t tap = fetch_bug && i >= 4u ? i - 2u : i;
        taps[i] = vi_fetch_texel(plan, memory, raster_x + vi_aa_tap_x[tap],
                                 raster_y + vi_aa_tap_y[tap]);
    }
    vi_aa_resolve(taps, coverage, out);
}

typedef struct vi_filtered_texel {
    sr_rgba8 color;
    uint8_t coverage;
} vi_filtered_texel;

static inline int32_t vi_clamp_unit(int32_t value)
{
    return value < -1 ? -1 : (value > 1 ? 1 : value);
}

/*
 * The filter stage reads the unfiltered texels of three raster rows - the
 * pixel's own and the ones above and below - reaching VI_ROW_MARGIN pixels past
 * either end: two for the AA taps, one more for the divot neighbour's taps.
 * `rows` point at x = 0 of each, so negative indices are the left margin.
 */
#define VI_ROW_MARGIN 3

/*
 * Dither reconstruction from the 3x3 neighbourhood, for raster pixels
 * [first, last] of a row at once. Each channel adds the clamped 5-bit
 * difference to each neighbour; the centre's own term is always zero. Written
 * as a plain loop over the texel bytes so the compiler vectorizes it - the
 * coverage lane is computed along with the colours and never read.
 */
static void vi_dither_row(const vi_aa_texel *const rows[3], int32_t first,
                          int32_t last, bool fetch_bug,
                          uint8_t *restrict out)
{
    const uint8_t *restrict above = (const uint8_t *)(rows[0] + first);
    const uint8_t *restrict mid = (const uint8_t *)(rows[1] + first);
    const uint8_t *restrict below =
        (const uint8_t *)(rows[fetch_bug ? 1 : 2] + first);
    const int32_t bytes = (last - first + 1) * (int32_t)sizeof(vi_aa_texel);
    const int32_t step = (int32_t)sizeof(vi_aa_texel);
    for (int32_t i = 0; i < bytes; i++) {
        const int32_t base = mid[i] >> 3;
        const int32_t sum =
            vi_clamp_unit((above[i - step] >> 3) - base) +
            vi_clamp_unit((above[i] >> 3) - base) +
            vi_clamp_unit((above[i + step] >> 3) - base) +
            vi_clamp_unit((mid[i - step] >> 3) - base) +
            vi_clamp_unit((mid[i + step] >> 3) - base) +
            vi_clamp_unit((below[i - step] >> 3) - base) +
            vi_clamp_unit((below[i] >> 3) - base) +
            vi_clamp_unit((below[i + step] >> 3) - base);
        out[i] = (uint8_t)((mid[i] & 0xf8u) + sum);
    }
}

/* AA for a partially covered pixel, dither reconstruction for a fully covered
 * one, whichever the plan enables. `dither` is vi_dither_row's output for this
 * row, starting at x = -1; unread when the dither filter is off. */
static inline vi_filtered_texel vi_filter_stage(const vi_scanout_plan *plan,
                                                const vi_aa_texel *const rows[3],
                                                const uint8_t *dither,
                                                int32_t x, bool fetch_bug)
{
    const vi_aa_texel source = rows[1][x];
    vi_filtered_texel result = {
        {source.r, source.g, source.b, 0xffu}, source.coverage
    };
    if (plan->aa_mode < VI_AA_RESAMP_ONLY && source.coverage != 7u) {
        vi_aa_texel taps[6];
        for (uint32_t i = 0; i < 6u; i++) {
            const uint32_t tap = fetch_bug && i >= 4u ? i - 2u : i;
            taps[i] = rows[1 + vi_aa_tap_y[tap]][x + vi_aa_tap_x[tap]];
        }
        vi_aa_resolve(taps, source.coverage, &result.color);
    } else if (plan->dither_filter_enable && source.coverage == 7u) {
        const uint8_t *d = dither + (x + 1) * (int32_t)sizeof(vi_aa_texel);
        result.color = (sr_rgba8){ d[0], d[1], d[2], 0xffu };
    }
    return result;
}

static inline uint8_t vi_median3(uint8_t a, uint8_t b, uint8_t c)
{
    if (a > b) { const uint8_t t = a; a = b; b = t; }
    if (b > c) { const uint8_t t = b; b = c; c = t; }
    if (a > b) b = a;
    return b;
}

/*
 * Plan is VI_SCANOUT_READY, so every address touched here was proven in range
 * during vi_build_scanout_plan(); use the unchecked fast accessors.
 *
 * Coverage is derived from the same word the colour comes from, so enabling AA
 * costs one hidden-plane byte per pixel at 16bpp and nothing at all at 32bpp,
 * where the three bits already sit in the word. Splitting the filter out into
 * its own pass over the row would mean re-reading and re-decoding every centre
 * pixel just to learn its coverage.
 *
 * `apply_aa` is a constant at both call sites, so each pixel format compiles to
 * a filtered and an unfiltered loop with no per-pixel test.
 */
static inline void decode_row_16(const vi_scanout_plan *plan,
                                 const sr_memory *memory, uint32_t source_y,
                                 uint32_t sub_row, sr_rgba8 *restrict row,
                                 bool apply_aa, bool fetch_bug)
{
    const uint32_t row_base = plan->origin + source_y * plan->source_stride * 2u;
    const uint32_t width = plan->source_width;
    const sr_rgba8 *restrict table = vi_rgba5551_table;
    for (uint32_t x = 0; x < width; x++) {
        const uint32_t address = mask_addr(memory, row_base + x * 2u);
        for (uint32_t sx = 0; sx < (uint32_t)SOFTRDP_SCALE; sx++) {
            const uint32_t sample = sub_row * (uint32_t)SOFTRDP_SCALE + sx;
            const uint32_t slot = x * (uint32_t)SOFTRDP_SCALE + sx;
            const uint16_t raw = sr_memory_read_be16_sample(memory, address, sample);
            row[slot] = table[raw];
            if (apply_aa) {
                const uint32_t coverage = (uint32_t)((raw & 1u) << 2) |
                    sr_memory_read_hidden_sample(memory, address, raw, sample);
                if (coverage != 7u)
                    vi_filter_pixel(plan, memory,
                                    (int32_t)(x * (uint32_t)SOFTRDP_SCALE + sx),
                                    (int32_t)(source_y * (uint32_t)SOFTRDP_SCALE +
                                              sub_row),
                                    coverage, fetch_bug, &row[slot]);
            }
        }
    }
}

static inline void decode_row_32(const vi_scanout_plan *plan,
                                 const sr_memory *memory, uint32_t source_y,
                                 uint32_t sub_row, sr_rgba8 *restrict row,
                                 bool apply_aa, bool fetch_bug)
{
    const uint32_t row_base = plan->origin + source_y * plan->source_stride * 4u;
    const uint32_t width = plan->source_width;
    for (uint32_t x = 0; x < width; x++) {
        const uint32_t address = mask_addr(memory, row_base + x * 4u);
        for (uint32_t sx = 0; sx < (uint32_t)SOFTRDP_SCALE; sx++) {
            const uint32_t sample = sub_row * (uint32_t)SOFTRDP_SCALE + sx;
            const uint32_t slot = x * (uint32_t)SOFTRDP_SCALE + sx;
            const uint32_t raw = sr_memory_read_be32_sample(memory, address, sample);
            row[slot] = (sr_rgba8){(uint8_t)(raw >> 24), (uint8_t)(raw >> 16),
                                   (uint8_t)(raw >> 8), (uint8_t)raw};
            if (apply_aa) {
                const uint32_t coverage = (raw >> 5) & 7u;
                if (coverage != 7u)
                    vi_filter_pixel(plan, memory,
                                    (int32_t)(x * (uint32_t)SOFTRDP_SCALE + sx),
                                    (int32_t)(source_y * (uint32_t)SOFTRDP_SCALE +
                                              sub_row),
                                    coverage, fetch_bug, &row[slot]);
            }
        }
    }
}

/*
 * A row with the divot and/or dither filter on. The filters read the raster
 * rows above and below as well, and rows are decoded top to bottom, so the
 * three texel buffers form a ring: each raster row is fetched from RDRAM once
 * per scanout and serves as the lower, centre and upper row in turn. The
 * filter stage (AA or dither reconstruction) then runs once per raster pixel,
 * and divot takes its median over neighbours from that finished row instead of
 * filtering each of them again. The stage entries past either end are the
 * neighbours the edge pixels' divot reads.
 */
static void decode_row_filtered(const vi_scanout_plan *plan,
                                const sr_memory *memory, uint32_t source_y,
                                uint32_t sub_row, sr_rgba8 *restrict row,
                                vi_row_cache *cache, bool fetch_bug)
{
    enum { ROW_TEXELS = VI_MAX_SOURCE_WIDTH * SOFTRDP_SCALE + 2 * VI_ROW_MARGIN };
    static _Thread_local vi_aa_texel texels[3][ROW_TEXELS];
    static _Thread_local vi_filtered_texel stage[VI_MAX_SOURCE_WIDTH * SOFTRDP_SCALE + 2u];
    const int32_t raster_y = (int32_t)(source_y * (uint32_t)SOFTRDP_SCALE + sub_row);
    const int32_t count = (int32_t)(plan->source_width * (uint32_t)SOFTRDP_SCALE);

    int32_t slot_of[3] = { -1, -1, -1 };
    bool taken[3] = { false, false, false };
    for (int32_t r = 0; r < 3; r++)
        for (int32_t s = 0; s < 3; s++)
            if (cache->texel_valid[s] && cache->texel_row[s] == raster_y - 1 + r) {
                slot_of[r] = s;
                taken[s] = true;
            }
    for (int32_t r = 0; r < 3; r++) {
        if (slot_of[r] >= 0) continue;
        int32_t s = 0;
        while (taken[s]) s++;
        taken[s] = true;
        slot_of[r] = s;
        for (int32_t x = -VI_ROW_MARGIN; x < count + VI_ROW_MARGIN; x++)
            texels[s][x + VI_ROW_MARGIN] =
                vi_fetch_texel(plan, memory, x, raster_y - 1 + r);
        cache->texel_row[s] = raster_y - 1 + r;
        cache->texel_valid[s] = true;
    }
    const vi_aa_texel *const rows[3] = {
        texels[slot_of[0]] + VI_ROW_MARGIN,
        texels[slot_of[1]] + VI_ROW_MARGIN,
        texels[slot_of[2]] + VI_ROW_MARGIN
    };
    /* Pixels -1..count: the divot neighbours of the edge pixels included. */
    static _Thread_local uint8_t dither[(VI_MAX_SOURCE_WIDTH * SOFTRDP_SCALE + 2u) *
                                        sizeof(vi_aa_texel)];
    if (plan->dither_filter_enable)
        vi_dither_row(rows, -1, count, fetch_bug, dither);
    if (!plan->divot_enable) {
        for (int32_t x = 0; x < count; x++)
            row[x] = vi_filter_stage(plan, rows, dither, x, fetch_bug).color;
        return;
    }
    for (int32_t x = -1; x <= count; x++)
        stage[x + 1] = vi_filter_stage(plan, rows, dither, x, fetch_bug);
    for (int32_t x = 0; x < count; x++) {
        const vi_filtered_texel left = stage[x];
        const vi_filtered_texel center = stage[x + 1];
        const vi_filtered_texel right = stage[x + 2];
        sr_rgba8 color = center.color;
        if ((left.coverage & center.coverage & right.coverage) != 7u) {
            color.r = vi_median3(left.color.r, center.color.r, right.color.r);
            color.g = vi_median3(left.color.g, center.color.g, right.color.g);
            color.b = vi_median3(left.color.b, center.color.b, right.color.b);
        }
        row[x] = color;
    }
}

static void decode_row(const vi_scanout_plan *plan,
                       const sr_memory *memory, uint32_t source_y,
                       uint32_t sub_row, sr_rgba8 *restrict row,
                       vi_row_cache *cache, bool fetch_bug)
{
    if (plan->dither_filter_enable || plan->divot_enable) {
        decode_row_filtered(plan, memory, source_y, sub_row, row, cache,
                            fetch_bug);
        return;
    }
    /*
     * Runs at every scale. The RDP writes coverage PER SAMPLE, so a sample on a
     * polygon edge still carries a partial value above scale 1 and the filter
     * still has something real to act on; suppressing it would make a scaled
     * frame look unlike both the hardware and the unscaled path.
     */
    const bool apply_aa = plan->aa_mode < VI_AA_RESAMP_ONLY;
    if (plan->bytes_per_pixel == 2u) {
        decode_row_16(plan, memory, source_y, sub_row, row, apply_aa, fetch_bug);
    } else {
        decode_row_32(plan, memory, source_y, sub_row, row, apply_aa, fetch_bug);
    }
}

static const sr_rgba8 *get_cached_row(vi_row_cache *cache,
                                      const vi_scanout_plan *plan,
                                      const sr_memory *memory,
                                      uint32_t source_y, uint32_t sub_row,
                                      bool fetch_bug,
                                      const sr_rgba8 *protected_row)
{
    for (uint32_t slot = 0; slot < VI_ROW_CACHE_SLOTS; slot++) {
        if (cache->valid[slot] && cache->row_index[slot] == source_y &&
            cache->sub_row[slot] == sub_row &&
            cache->fetch_bug[slot] == fetch_bug)
            return cache->rows[slot];
    }
    uint32_t slot;
    do {
        slot = cache->next_slot++ % VI_ROW_CACHE_SLOTS;
    } while (cache->rows[slot] == protected_row);
    decode_row(plan, memory, source_y, sub_row, cache->rows[slot], cache,
               fetch_bug);
    cache->valid[slot] = true;
    cache->row_index[slot] = source_y;
    cache->sub_row[slot] = sub_row;
    cache->fetch_bug[slot] = fetch_bug;
    return cache->rows[slot];
}

/*
 * The VI's lerp, a + ((b - a) * fraction + 16) >> 5 on each channel with a
 * five-bit fraction. Evaluated in unsigned 32-bit arithmetic and truncated to
 * eight bits, it gives exactly the floor an arithmetic shift of the signed
 * difference gives, so 16-bit lanes compute the same bytes - all four
 * channels at once.
 */
typedef uint8_t vi_u8x4 __attribute__((vector_size(4)));
typedef int16_t vi_i16x4 __attribute__((vector_size(8)));

static inline sr_rgba8 lerp_color(sr_rgba8 a, sr_rgba8 b, uint32_t fraction)
{
    vi_u8x4 pa, pb;
    memcpy(&pa, &a, sizeof(pa));
    memcpy(&pb, &b, sizeof(pb));
    const vi_i16x4 wa = __builtin_convertvector(pa, vi_i16x4);
    const vi_i16x4 wb = __builtin_convertvector(pb, vi_i16x4);
    const vi_i16x4 blend = wa + (((wb - wa) * (int16_t)fraction + 16) >> 5);
    const vi_u8x4 packed = __builtin_convertvector(blend, vi_u8x4);
    sr_rgba8 out;
    memcpy(&out, &packed, sizeof(out));
    return out;
}

/*
 * Whether every output pixel lands on a whole source pixel: the step has no
 * fractional bits and the start's five-bit fraction is zero. Every horizontal
 * lerp then has fraction 0, which returns its first input, so the row is a
 * plain gather.
 */
static inline bool vi_whole_x_steps(const vi_x_window *window)
{
    return (window->sample_x_add & 0x3ffu) == 0u &&
           ((window->sample_x_start >> 5) & 31u) == 0u;
}

/* The horizontal window of scaled output row output_y. */
static inline const vi_x_window *vi_row_window(const vi_scanout_plan *plan,
                                               uint32_t output_y)
{
    return &plan->windows[plan->per_row_x
        ? plan->row_window[output_y / (uint32_t)SOFTRDP_SCALE] : 0u];
}

static inline uint32_t vi_output_y_coordinate(const vi_scanout_plan *plan,
                                              uint32_t output_y,
                                              uint32_t active_y_begin)
{
    const uint32_t local_y = output_y - active_y_begin;
    return plan->sample_y_start + local_y * plan->sample_y_add;
}

static inline bool vi_next_row_fetch_bug(const vi_scanout_plan *plan,
                                         uint32_t output_y,
                                         uint32_t active_y_begin)
{
    if (output_y == active_y_begin) return false;
    if (plan->serrate) return false;
    const uint32_t previous = vi_output_y_coordinate(plan, output_y - 1u,
                                                      active_y_begin);
    const uint32_t current = vi_output_y_coordinate(plan, output_y,
                                                     active_y_begin);
    const uint32_t next = vi_output_y_coordinate(plan, output_y + 1u,
                                                  active_y_begin);
    return (previous >> 10) == (current >> 10) &&
           (current >> 10) != (next >> 10);
}

static inline bool vi_replicate_fetch(const vi_x_window *window,
                                      uint32_t coordinate,
                                      uint32_t source_width,
                                      uint32_t *index)
{
    uint32_t at = coordinate >> 10;
    const uint32_t mask = window->replicate_fetch_mask;
    if (mask) {
        if (at & mask) return false;
        at &= mask - 1u;
    }
    *index = at < source_width ? at : source_width - 1u;
    return true;
}

/*
 * The vertical half of the filter for a whole source row: every output pixel
 * of a row shares the vertical fraction, so it is blended once per row rather
 * than twice per output pixel, in a plain byte loop the compiler vectorizes.
 * The same arithmetic as lerp_color.
 */
static const sr_rgba8 *vi_blend_rows(const sr_rgba8 *restrict row0,
                                     const sr_rgba8 *restrict row1,
                                     uint32_t fraction, uint32_t width,
                                     sr_rgba8 *restrict out)
{
    const uint8_t *restrict a = (const uint8_t *)row0;
    const uint8_t *restrict b = (const uint8_t *)row1;
    uint8_t *restrict o = (uint8_t *)out;
    const int32_t f = (int32_t)fraction;
    for (uint32_t i = 0; i < width * (uint32_t)sizeof(sr_rgba8); i++)
        o[i] = (uint8_t)(a[i] + ((((int32_t)b[i] - (int32_t)a[i]) * f + 16) >> 5));
    return out;
}

/*
 * The per-pixel dither noise, for pixels [begin, end) of one row. The noise is
 * the top half of `a` after three rounds of
 *
 *     a, b, c = ((a >> 8) ^ b) * P, ((b >> 8) ^ c) * P, ((c >> 8) ^ a) * P
 *
 * starting from (x, y, frame), P = 1103515245. Only `a` is returned, and it
 * depends on the pixel through three multiplies: y and the frame are constant
 * along the row, and x >> 8 changes only every 256 pixels, so the rest of the
 * nine is hoisted out. The per-pixel part is a loop the compiler vectorizes.
 */
static void vi_row_noise(uint32_t y, uint32_t frame, uint32_t begin,
                         uint32_t end, uint16_t *restrict out)
{
    const uint32_t prime = 1103515245u;
    const uint32_t b1 = ((y >> 8) ^ frame) * prime;
    const uint32_t b1_shifted = b1 >> 8;
    const uint32_t frame_shifted = frame >> 8;
    for (uint32_t x = begin; x < end; ) {
        const uint32_t block_end = ((x >> 8) + 1u) << 8;
        const uint32_t stop = block_end < end ? block_end : end;
        const uint32_t a1 = ((x >> 8) ^ y) * prime;
        const uint32_t a2_shifted = ((((a1 >> 8) ^ b1) * prime) >> 8);
        for (; x < stop; x++) {
            const uint32_t c1 = (frame_shifted ^ x) * prime;
            const uint32_t b2 = (b1_shifted ^ c1) * prime;
            out[x] = (uint16_t)(((a2_shifted ^ b2) * prime) >> 16);
        }
    }
}

static inline sr_rgba8 vi_transform_gamma(sr_rgba8 color)
{
    color.r = vi_gamma_flat[color.r];
    color.g = vi_gamma_flat[color.g];
    color.b = vi_gamma_flat[color.b];
    return color;
}

static uint32_t vi_isqrt(uint32_t value)
{
    uint32_t low = 0u;
    uint32_t high = 128u;
    while (low + 1u < high) {
        const uint32_t middle = low + ((high - low) >> 1);
        if (middle * middle <= value)
            low = middle;
        else
            high = middle;
    }
    return low;
}

static inline sr_rgba8 vi_transform_gamma_dither(sr_rgba8 color, uint16_t noise)
{
    color.r = (uint8_t)(2u * vi_isqrt(((uint32_t)color.r << 6) +
                                      (noise & 0x3fu)));
    color.g = (uint8_t)(2u * vi_isqrt(((uint32_t)color.g << 6) +
                                      ((noise >> 6) & 0x3fu)));
    color.b = (uint8_t)(2u * vi_isqrt(((uint32_t)color.b << 6) +
                                      (((noise >> 9) & 0x38u) | (noise & 7u))));
    return color;
}

static inline sr_rgba8 vi_transform_dither(sr_rgba8 color, uint16_t noise)
{
    const uint32_t r = (uint32_t)color.r + (noise & 1u);
    const uint32_t g = (uint32_t)color.g + ((noise >> 1) & 1u);
    const uint32_t b = (uint32_t)color.b + ((noise >> 2) & 1u);
    color.r = (uint8_t)(r > 255u ? 255u : r);
    color.g = (uint8_t)(g > 255u ? 255u : g);
    color.b = (uint8_t)(b > 255u ? 255u : b);
    return color;
}

typedef struct vi_scanout_job {
    const vi_scanout_plan *plan;
    const sr_memory *memory;
    sr_rgba8 *pixels;
    uint32_t width, height, stride;
} vi_scanout_job;

static void vi_execute_identity_rows(const vi_scanout_job *job,
                                     uint32_t begin, uint32_t end)
{
    const vi_scanout_plan *plan = job->plan;
    const sr_memory *memory = job->memory;
    const uint32_t width = job->width, stride = job->stride;

    vi_row_cache cache;
    memset(&cache, 0, sizeof(cache));
    const bool interpolate = plan->aa_mode != VI_AA_REPLICATE;
    const uint32_t source_width = plan->source_width;
    const sr_rgba8 border = {0, 0, 0, 0};
    const uint32_t active_y_begin =
        plan->active_y_begin * (uint32_t)SOFTRDP_SCALE;
    const uint32_t active_y_end =
        plan->active_y_end * (uint32_t)SOFTRDP_SCALE;

    const uint32_t scaled_source_width = source_width * (uint32_t)SOFTRDP_SCALE;

    for (uint32_t y = begin; y < end; y++) {
        sr_rgba8 *restrict destination = job->pixels + y * stride;
        if (y < active_y_begin || y >= active_y_end) {
            for (uint32_t x = 0; x < width; x++) destination[x] = border;
            continue;
        }
        /* The row's horizontal window. The guard band is stated in hardware
         * output pixels and blanks the whole scaled square of each. */
        const vi_x_window *window = vi_row_window(plan, y);
        const uint32_t band_begin = window->active_x_begin * (uint32_t)SOFTRDP_SCALE;
        const uint32_t band_end = window->active_x_end * (uint32_t)SOFTRDP_SCALE;
        const uint32_t active_begin = band_begin < width ? band_begin : width;
        const uint32_t active_end = band_end < width ? band_end : width;
        const uint32_t y_coordinate =
            vi_output_y_coordinate(plan, y, active_y_begin);
        const uint32_t scaled_row = y_coordinate >> 10;
        const uint32_t y_fraction_scaled = (y_coordinate >> 5) & 31u;
        const uint32_t next_scaled_row = scaled_row + 1u;
        const sr_rgba8 *restrict row0 = get_cached_row(&cache, plan, memory,
            scaled_row / (uint32_t)SOFTRDP_SCALE,
            scaled_row % (uint32_t)SOFTRDP_SCALE, false, NULL);
        const bool fetch_bug = vi_next_row_fetch_bug(plan, y, active_y_begin);
        const sr_rgba8 *restrict row1 = (interpolate && y_fraction_scaled) ?
            get_cached_row(&cache, plan, memory,
                           next_scaled_row / (uint32_t)SOFTRDP_SCALE,
                           next_scaled_row % (uint32_t)SOFTRDP_SCALE,
                           fetch_bug, row0) : NULL;
        for (uint32_t x = 0; x < active_begin; x++) destination[x] = border;
        for (uint32_t x = active_end; x < width; x++) destination[x] = border;

        if (!interpolate) {
            /* Point sampling: straight gather, no per-pixel branching. */
            for (uint32_t x = active_begin; x < active_end; x++) {
                uint32_t at;
                if (vi_replicate_fetch(window, window->sample_x_start +
                                       x * window->sample_x_add,
                                       scaled_source_width, &at))
                    destination[x] = row0[at];
                else
                    destination[x] = border;
            }
            continue;
        }

        /* Vertical blend once per row, then the horizontal lerp per pixel. A
         * zero fraction leaves lerp_color's first input unchanged, so neither
         * needs a special case. */
        static _Thread_local sr_rgba8 blended[VI_MAX_SOURCE_WIDTH * SOFTRDP_SCALE];
        const sr_rgba8 *restrict source = row1
            ? vi_blend_rows(row0, row1, y_fraction_scaled, scaled_source_width, blended)
            : row0;
        if (vi_whole_x_steps(window)) {
            for (uint32_t x = active_begin; x < active_end; x++) {
                const uint32_t at = (window->sample_x_start + x * window->sample_x_add) >> 10;
                destination[x] = source[at < scaled_source_width ? at : scaled_source_width - 1u];
            }
            continue;
        }
        for (uint32_t x = active_begin; x < active_end; x++) {
            const uint32_t coordinate = window->sample_x_start +
                                        x * window->sample_x_add;
            const uint32_t fraction = (coordinate >> 5) & 31u;
            uint32_t at = coordinate >> 10;
            if (at >= scaled_source_width) at = scaled_source_width - 1u;
            const uint32_t at_next = at + 1u < scaled_source_width ? at + 1u : at;
            destination[x] = lerp_color(source[at], source[at_next], fraction);
        }
    }
}

typedef void (*vi_filtered_row_fn)(const vi_scanout_plan *plan,
                                   const sr_rgba8 *restrict row0,
                                   const sr_rgba8 *restrict row1,
                                   uint32_t y_fraction, uint32_t source_width,
                                   uint32_t y, uint32_t begin, uint32_t end,
                                   sr_rgba8 *restrict destination);

#define VI_APPLY_GAMMA(color, noise) \
    vi_transform_gamma(color)
#define VI_APPLY_GAMMA_DITHER(color, noise) \
    vi_transform_gamma_dither(color, noise)
#define VI_APPLY_DITHER(color, noise) \
    vi_transform_dither(color, noise)

/* `noisy` is constant per instance: the dithering transforms get the row's
 * noise precomputed by vi_row_noise, the others never read it. */
#define VI_DEFINE_POINT_ROW(name, apply, noisy)                               \
static void name(const vi_scanout_plan *plan,                                 \
                 const sr_rgba8 *restrict row0,                               \
                 const sr_rgba8 *restrict row1, uint32_t y_fraction,          \
                 uint32_t source_width, uint32_t y, uint32_t begin,           \
                 uint32_t end, sr_rgba8 *restrict destination)                \
{                                                                             \
    static _Thread_local uint16_t noise[VI_MAX_OUTPUT_WIDTH * SOFTRDP_SCALE]; \
    (void)row1; (void)y_fraction;                                             \
    const vi_x_window *window = vi_row_window(plan, y);                       \
    if (noisy) vi_row_noise(y, plan->dither_frame, begin, end, noise);        \
    for (uint32_t x = begin; x < end; x++) {                                  \
        uint32_t at;                                                           \
        const bool fetched = vi_replicate_fetch(window,                       \
            window->sample_x_start + x * window->sample_x_add,                \
            source_width, &at);                                               \
        sr_rgba8 color = fetched ? row0[at] : (sr_rgba8){0, 0, 0, 0};         \
        destination[x] = apply(color, noise[x]);                              \
    }                                                                         \
}

/* Vertical blend once per row, then the horizontal lerp per pixel; see the
 * identity path for why neither fraction needs a zero case. */
#define VI_DEFINE_INTERP_ROW(name, apply, noisy)                              \
static void name(const vi_scanout_plan *plan,                                 \
                 const sr_rgba8 *restrict row0,                               \
                 const sr_rgba8 *restrict row1, uint32_t y_fraction,          \
                 uint32_t source_width, uint32_t y, uint32_t begin,           \
                 uint32_t end, sr_rgba8 *restrict destination)                \
{                                                                             \
    static _Thread_local uint16_t noise[VI_MAX_OUTPUT_WIDTH * SOFTRDP_SCALE]; \
    static _Thread_local sr_rgba8 blended[VI_MAX_SOURCE_WIDTH * SOFTRDP_SCALE]; \
    const vi_x_window *window = vi_row_window(plan, y);                       \
    if (noisy) vi_row_noise(y, plan->dither_frame, begin, end, noise);        \
    const sr_rgba8 *restrict source = row1                                    \
        ? vi_blend_rows(row0, row1, y_fraction, source_width, blended)        \
        : row0;                                                               \
    if (vi_whole_x_steps(window)) {                                           \
        for (uint32_t x = begin; x < end; x++) {                              \
            const uint32_t at = (window->sample_x_start +                     \
                                 x * window->sample_x_add) >> 10;             \
            destination[x] = apply(source[at < source_width ? at              \
                                          : source_width - 1u], noise[x]);    \
        }                                                                     \
        return;                                                               \
    }                                                                         \
    for (uint32_t x = begin; x < end; x++) {                                  \
        const uint32_t coordinate = window->sample_x_start +                  \
                                    x * window->sample_x_add;                 \
        const uint32_t fraction = (coordinate >> 5) & 31u;                    \
        uint32_t at = coordinate >> 10;                                       \
        if (at >= source_width) at = source_width - 1u;                       \
        const uint32_t next = at + 1u < source_width ? at + 1u : at;          \
        destination[x] = apply(lerp_color(source[at], source[next], fraction), \
                               noise[x]);                                     \
    }                                                                         \
}

VI_DEFINE_POINT_ROW(vi_point_gamma, VI_APPLY_GAMMA, 0)
VI_DEFINE_POINT_ROW(vi_point_gamma_dither, VI_APPLY_GAMMA_DITHER, 1)
VI_DEFINE_POINT_ROW(vi_point_dither, VI_APPLY_DITHER, 1)
VI_DEFINE_INTERP_ROW(vi_interp_gamma, VI_APPLY_GAMMA, 0)
VI_DEFINE_INTERP_ROW(vi_interp_gamma_dither, VI_APPLY_GAMMA_DITHER, 1)
VI_DEFINE_INTERP_ROW(vi_interp_dither, VI_APPLY_DITHER, 1)

#undef VI_DEFINE_POINT_ROW
#undef VI_DEFINE_INTERP_ROW
#undef VI_APPLY_GAMMA
#undef VI_APPLY_GAMMA_DITHER
#undef VI_APPLY_DITHER

static vi_filtered_row_fn vi_select_filtered_row(vi_output_transform transform,
                                                  bool interpolate)
{
    if (interpolate) {
        if (transform == VI_OUTPUT_GAMMA) return vi_interp_gamma;
        if (transform == VI_OUTPUT_GAMMA_DITHER) return vi_interp_gamma_dither;
        return vi_interp_dither;
    }
    if (transform == VI_OUTPUT_GAMMA) return vi_point_gamma;
    if (transform == VI_OUTPUT_GAMMA_DITHER) return vi_point_gamma_dither;
    return vi_point_dither;
}

static __attribute__((noinline)) void vi_execute_filtered_rows(
                                     const vi_scanout_job *job,
                                     uint32_t begin, uint32_t end)
{
    const vi_scanout_plan *plan = job->plan;
    const sr_memory *memory = job->memory;
    const uint32_t width = job->width, stride = job->stride;

    vi_row_cache cache;
    memset(&cache, 0, sizeof(cache));
    const bool interpolate = plan->aa_mode != VI_AA_REPLICATE;
    const vi_filtered_row_fn write_row =
        vi_select_filtered_row(plan->output_transform, interpolate);
    const uint32_t scaled_source_width =
        plan->source_width * (uint32_t)SOFTRDP_SCALE;
    const sr_rgba8 border = {0, 0, 0, 0};
    const uint32_t active_y_begin =
        plan->active_y_begin * (uint32_t)SOFTRDP_SCALE;
    const uint32_t active_y_end =
        plan->active_y_end * (uint32_t)SOFTRDP_SCALE;

    for (uint32_t y = begin; y < end; y++) {
        sr_rgba8 *restrict destination = job->pixels + y * stride;
        if (y < active_y_begin || y >= active_y_end) {
            for (uint32_t x = 0; x < width; x++) destination[x] = border;
            continue;
        }
        const uint32_t y_coordinate =
            vi_output_y_coordinate(plan, y, active_y_begin);
        const uint32_t scaled_row = y_coordinate >> 10;
        const uint32_t y_fraction = (y_coordinate >> 5) & 31u;
        const uint32_t next_scaled_row = scaled_row + 1u;
        const sr_rgba8 *restrict row0 = get_cached_row(&cache, plan, memory,
            scaled_row / (uint32_t)SOFTRDP_SCALE,
            scaled_row % (uint32_t)SOFTRDP_SCALE, false, NULL);
        const bool fetch_bug = vi_next_row_fetch_bug(plan, y, active_y_begin);
        const sr_rgba8 *restrict row1 = (interpolate && y_fraction)
            ? get_cached_row(&cache, plan, memory,
                next_scaled_row / (uint32_t)SOFTRDP_SCALE,
                next_scaled_row % (uint32_t)SOFTRDP_SCALE, fetch_bug, row0)
            : NULL;
        /* The row's horizontal window, in hardware output pixels. */
        const vi_x_window *window = vi_row_window(plan, y);
        const uint32_t band_begin = window->active_x_begin * (uint32_t)SOFTRDP_SCALE;
        const uint32_t band_end = window->active_x_end * (uint32_t)SOFTRDP_SCALE;
        const uint32_t active_begin = band_begin < width ? band_begin : width;
        const uint32_t active_end = band_end < width ? band_end : width;
        for (uint32_t x = 0; x < active_begin; x++) destination[x] = border;
        for (uint32_t x = active_end; x < width; x++) destination[x] = border;
        write_row(plan, row0, row1, y_fraction, scaled_source_width, y,
                  active_begin, active_end, destination);
    }
}

static void vi_scanout_worker(void *opaque, uint32_t id, uint32_t count)
{
    const vi_scanout_job *job = opaque;
    /* Adjacent output rows reuse decoded source rows. Keep each band together
     * and let its execution own a private row cache. Absolute y coordinates
     * preserve sampling and dither regardless of the partition. */
    const uint32_t begin = (uint32_t)((uint64_t)job->height * id / count);
    const uint32_t end = (uint32_t)((uint64_t)job->height * (id + 1u) / count);
    if (begin == end) return;
    if (job->plan->output_transform == VI_OUTPUT_IDENTITY)
        vi_execute_identity_rows(job, begin, end);
    else
        vi_execute_filtered_rows(job, begin, end);
}

sr_result vi_execute_scanout_threaded(const vi_scanout_plan *plan,
                                     const sr_memory *memory,
                                     sr_framebuffer *out, bool threaded)
{
    if (!plan || !memory || !out) return SR_ERROR_INVALID_ARGUMENT;
    out->valid = false;
    if (plan->state != VI_SCANOUT_READY) {
        out->width = plan->scanout_width;
        out->height = plan->scanout_height;
        return SR_OK;
    }
    const uint32_t width = out->width ? out->width : plan->scanout_width;
    const uint32_t height = out->height ? out->height : plan->scanout_height;
    const uint32_t stride = out->stride_pixels ? out->stride_pixels : width;
    if (!out->pixels || width != plan->scanout_width ||
        height != plan->scanout_height || stride < width)
        return SR_ERROR_INVALID_ARGUMENT;

    /* Shared tables are fully published before any helper reads them. Frame
     * metadata is likewise caller-owned and published only after completion. */
    if (plan->bytes_per_pixel == 2u && !vi_rgba5551_table_ready)
        vi_build_rgba5551_table();
    vi_scanout_job job = {plan, memory, out->pixels, width, height, stride};
    if (threaded && height > 1u) {
        if (sr_vi_threads_count() == 1u)
            sr_vi_threads_init(3u);
        sr_vi_threads_run(vi_scanout_worker, &job);
    } else {
        vi_scanout_worker(&job, 0u, 1u);
    }
    out->width = width;
    out->height = height;
    out->stride_pixels = stride;
    out->valid = true;
    return SR_OK;
}

sr_result vi_execute_scanout(const vi_scanout_plan *plan,
                             const sr_memory *memory, sr_framebuffer *out)
{
    return vi_execute_scanout_threaded(plan, memory, out, false);
}
