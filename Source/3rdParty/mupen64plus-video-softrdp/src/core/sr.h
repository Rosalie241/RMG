#ifndef SR_H
#define SR_H

#include "sr_defs.h"
#include "sr_host.h"
#include <stddef.h>

typedef struct sr_context sr_context;

typedef struct sr_vi_frame_info {
    uint32_t width;
    uint32_t height;
    uint32_t display_width;
    uint32_t display_height;
    bool display;
    bool interlaced;
    bool field;
    /* Frame not renderable this refresh but not a genuine blank: the presenter
     * should hold the previous frame instead of showing black. */
    bool hold;
} sr_vi_frame_info;

typedef struct sr_debug_stats {
    uint32_t last_list_current;
    uint32_t last_list_end;
    uint32_t last_list_bytes;
    uint32_t last_command_address;
    uint32_t last_command_id;
    sr_result last_result;
    uint32_t color_image_format;
    uint32_t color_image_size;
    uint32_t color_image_width;
    uint32_t color_image_address;
    uint32_t last_texture_image_format;
    uint32_t last_texture_image_size;
    uint32_t last_texture_image_width;
    uint32_t last_texture_image_address;
    uint32_t last_tile_index;
    uint32_t last_tile_format;
    uint32_t last_tile_size;
    uint32_t last_tile_tmem;
    uint32_t last_tile_line;
    uint32_t last_tile_sl;
    uint32_t last_tile_tl;
    uint32_t last_tile_sh;
    uint32_t last_tile_th;
    uint32_t last_load_sl;
    uint32_t last_load_tl;
    uint32_t last_load_sh;
    uint32_t last_load_th;
    int32_t last_rect_s0;
    int32_t last_rect_t0;
    int32_t last_rect_dsdx;
    int32_t last_rect_dtdy;
    /*
     * Addresses of the offscreen surfaces the last dispatched batch was bound
     * to, or 0 where no surface was bound. Zero throughout at scale 1, which
     * renders into RDRAM and has no surfaces. Equal to each other when the
     * depth buffer is the colour buffer, which is what a depth clear looks
     * like.
     */
    uint32_t surface_color_address;
    uint32_t surface_depth_address;
    /*
     * What scanout was asked to display and whether it found it. The VI origin
     * is the buffer the game is showing; scanout_surface_found says whether any
     * surface carries that address. A displayed frame needs the two to agree,
     * and a mismatch is the difference between a picture and a black screen.
     */
    uint32_t scanout_origin;
    uint32_t scanout_state;
    uint32_t scanout_width;
    uint32_t scanout_height;
    uint32_t submitted_batches;
} sr_debug_stats;

sr_context *sr_create(const sr_host_interface *host);
void sr_destroy(sr_context *ctx);

void sr_set_host(sr_context *ctx, const sr_host_interface *host);
sr_result sr_process_rdp_list(sr_context *ctx);

/* Submit buffered commands and wait for all queued rendering. SYNC_FULL does
 * this before raising the host interrupt. With threading enabled, complete
 * chunks may execute before this call; callers reading output must still wait
 * for completion. VI captures its input independently and does not call this. */
void sr_flush(sr_context *ctx);

/*
 * One sample of the pixel at an RDRAM address. Sample 0 is what RDRAM itself
 * holds; the rest live in the side planes and are otherwise invisible. Reads a
 * halfword, which covers 16bpp colour and depth. Diagnostic only.
 */
uint32_t sr_debug_read_sample(const sr_context *ctx, uint32_t address,
                              uint32_t sample);
sr_result sr_get_vi_frame_info(sr_context *ctx, sr_vi_frame_info *info);

/*
 * Mid-frame H_START/X_SCALE writes for the next scanout. From each entry's
 * vi_line on (V_START units: the first active line is VI_V_OFFSET_*, two per
 * output line), its values replace the H_START and X_SCALE registers until the
 * next entry. Entries in line order, at most 32. The table applies to one
 * scanout: sr_update_screen clears it.
 */
typedef struct sr_vi_scanline_regs {
    uint32_t vi_line;
    uint32_t h_start;
    uint32_t x_scale;
} sr_vi_scanline_regs;
sr_result sr_set_vi_scanline_registers(sr_context *ctx,
                                       const sr_vi_scanline_regs *regs,
                                       uint32_t count);
sr_result sr_update_screen(sr_context *ctx, sr_framebuffer *out);
sr_debug_stats sr_get_debug_stats(const sr_context *ctx);

/* Opaque, versioned RDP/TMEM state used by diagnostic frame dumps. */
size_t sr_state_snapshot_size(void);
sr_result sr_save_state(const sr_context *ctx, void *data, size_t size);
sr_result sr_load_state(sr_context *ctx, const void *data, size_t size);

#endif
