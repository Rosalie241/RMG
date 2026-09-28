#ifndef VI_H
#define VI_H

#include "sr_host.h"

typedef struct sr_memory sr_memory;

#define VI_MAX_OUTPUT_WIDTH 640u
#define VI_MAX_OUTPUT_HEIGHT 576u
#define VI_MAX_SOURCE_WIDTH 4096u
#define VI_MAX_SCANLINE_REGS 32u

/* H_START and X_SCALE as written for the lines from vi_line on. vi_line counts
 * in V_START units: the first active line is VI_V_OFFSET_*, two per output
 * line. */
typedef struct vi_scanline_regs {
    uint32_t vi_line;
    uint32_t h_start;
    uint32_t x_scale;
} vi_scanline_regs;

typedef struct vi_state {
    uint32_t control;
    uint32_t origin;
    uint32_t width;
    uint32_t current;
    uint32_t v_sync;
    uint32_t h_start;
    uint32_t v_start;
    uint32_t x_scale;
    uint32_t y_scale;
    bool disable_dither_filter;
    bool disable_divot_filter;
    bool disable_gamma_dither;
    bool disable_aa;
    /* Mid-frame H_START/X_SCALE writes, in line order; empty for a frame that
     * keeps one horizontal window. Set by sr_set_vi_scanline_registers and
     * cleared by the scanout that uses them. */
    uint32_t scanline_count;
    vi_scanline_regs scanline[VI_MAX_SCANLINE_REGS];
} vi_state;

typedef enum vi_output_transform {
    VI_OUTPUT_IDENTITY = 0,
    VI_OUTPUT_GAMMA,
    VI_OUTPUT_GAMMA_DITHER,
    VI_OUTPUT_DITHER_ONLY
} vi_output_transform;

typedef enum vi_scanout_state {
    VI_SCANOUT_BLANK = 0,
    VI_SCANOUT_READY,
    VI_SCANOUT_INVALID_MEMORY,
    /* Valid pixel type but the frame cannot be produced this refresh (e.g. the
     * horizontal range is not programmed yet). The presenter should hold the last
     * frame instead of flashing black */
    VI_SCANOUT_HOLD
} vi_scanout_state;

/* The horizontal walk of one output row: which columns carry the framebuffer
 * and where they sample it (see the fields of the same names below). */
typedef struct vi_x_window {
    uint32_t sample_x_start;
    uint32_t sample_x_add;
    uint32_t active_x_begin;
    uint32_t active_x_end;
    uint32_t replicate_fetch_mask;
} vi_x_window;

typedef struct vi_scanout_plan {
    vi_scanout_state state;
    uint32_t origin;
    uint32_t source_stride;
    uint32_t source_width;
    uint32_t bytes_per_pixel;
    /* Hardware output dimensions before internal-resolution scaling. */
    uint32_t output_width;
    uint32_t output_height;
    /* The pixel buffer handed to the caller, which is what sr_get_vi_frame_info
     * reports and what vi_execute_scanout validates against: one pixel per
     * sample, so the supersampled image leaves the renderer intact and the
     * presenter scales it to the same picture geometry display_width and
     * display_height describe. */
    uint32_t scanout_width;
    uint32_t scanout_height;
    /* Source sample walk: scaling changes the start, not the step. */
    uint32_t sample_x_start;
    uint32_t sample_x_add;
    uint32_t sample_y_start;
    uint32_t sample_y_add;
    /* Columns [active_x_begin, active_x_end) carry framebuffer content; the
     * guard-band columns outside are blanked, as the hardware VI blanks the
     * left/right border where its filter window would read out of bounds. */
    uint32_t active_x_begin;
    uint32_t active_x_end;
    uint32_t active_y_begin;
    uint32_t active_y_end;
    uint32_t display_width;
    uint32_t display_height;
    uint32_t aa_mode;
    bool serrate;
    vi_output_transform output_transform;
    uint32_t dither_frame;
    bool divot_enable;
    bool dither_filter_enable;
    uint32_t replicate_fetch_mask;
    /* windows[0] repeats the horizontal fields above. With per_row_x, the
     * frame's mid-frame H_START/X_SCALE windows follow it and row_window picks
     * one per hardware output row. */
    bool per_row_x;
    vi_x_window windows[VI_MAX_SCANLINE_REGS + 1u];
    uint8_t row_window[VI_MAX_OUTPUT_HEIGHT];
} vi_scanout_plan;

void vi_init(vi_state *vi);
void vi_latch_registers(vi_state *vi, const sr_host_interface *host);
void vi_build_scanout_plan(const vi_state *vi, const sr_memory *memory,
                           vi_scanout_plan *plan);
sr_result vi_execute_scanout(const vi_scanout_plan *plan,
                             const sr_memory *memory,
                             sr_framebuffer *out);
/* Synchronous row-band dispatch. Source memory must remain stable until this
 * returns; output pixels must not alias it. False keeps execution on caller. */
sr_result vi_execute_scanout_threaded(const vi_scanout_plan *plan,
                                     const sr_memory *memory,
                                     sr_framebuffer *out, bool threaded);

#endif
