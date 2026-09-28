#include "plugin_common.h"
#include "plugin_dump.h"

#include "../core/sr_threads.h"
#include "../core/vi.h"

#include <stdlib.h>
#include <string.h>

/*
 * The scanout buffer carries one pixel per sample, so the bound follows the
 * largest scale selectable at runtime, not the build default. The VI never
 * produces more than its output window, so this is a limit the renderer
 * cannot exceed rather than a policy.
 */
#define SR_PLUGIN_MAX_FRAME_WIDTH (VI_MAX_OUTPUT_WIDTH * SR_SCALE_MAX)
#define SR_PLUGIN_MAX_FRAME_HEIGHT (VI_MAX_OUTPUT_HEIGHT * SR_SCALE_MAX)

/* The renderer sets the MI bit itself before calling this; the host only has
 * to notice it. */
static void raise_mi_interrupt(void *userdata)
{
    const sr_plugin_machine *machine = userdata;
    if (machine && machine->check_interrupts) machine->check_interrupts();
}

sr_context *sr_plugin_context_create(const sr_plugin_machine *machine,
                                     const sr_plugin_config *config,
                                     void (*dump_log)(const char *message))
{
    if (!machine || !config) return NULL;

    sr_host_interface host;
    memset(&host, 0, sizeof(host));
    host.scale = config->scale;
    host.workers = config->workers;
    host.disable_vi_dither_filter = config->disable_vi_dither_filter;
    host.disable_vi_divot_filter = config->disable_vi_divot_filter;
    host.disable_vi_gamma_dither = config->disable_vi_gamma_dither;
    host.disable_vi_aa = config->disable_vi_aa;
    host.rdram = machine->rdram;
    host.rdram_size = machine->rdram_size;
    host.dmem = machine->dmem;
    host.mi_intr_reg = machine->mi_intr_reg;
    host.raise_mi_interrupt = raise_mi_interrupt;
    host.userdata = (void *)machine;
    for (uint32_t i = 0; i < SR_DP_REGISTER_COUNT; i++) host.dp_regs[i] = machine->dp[i];
    for (uint32_t i = 0; i < SR_VI_REGISTER_COUNT; i++) host.vi_regs[i] = machine->vi[i];

    sr_context *ctx = sr_create(&host);
    if (!ctx) return NULL;

#if SOFTRDP_ENABLE_DUMP
    plugin_dump_host dump_host;
    memset(&dump_host, 0, sizeof(dump_host));
    dump_host.rdram = machine->rdram;
    dump_host.dmem = machine->dmem;
    dump_host.rdram_size = machine->rdram_size;
    dump_host.rdram_bswapped = machine->rdram_bswapped;
    dump_host.log = dump_log;
    for (uint32_t i = 0; i < SR_DP_REGISTER_COUNT; i++) dump_host.dp[i] = machine->dp[i];
    for (uint32_t i = 0; i < SR_VI_REGISTER_COUNT; i++) dump_host.vi[i] = machine->vi[i];
    plugin_dump_attach(&dump_host, ctx);
#else
    (void)dump_log;
#endif
    return ctx;
}

void sr_plugin_context_destroy(sr_context *ctx)
{
    plugin_dump_detach();
    sr_destroy(ctx);
    sr_threads_shutdown();
}

sr_result sr_plugin_process_list(sr_context *ctx)
{
    if (!ctx) return SR_ERROR_INVALID_ARGUMENT;
    const bool dump_armed = plugin_dump_armed;
    if (dump_armed) plugin_dump_before_list();
    const sr_result result = sr_process_rdp_list(ctx);
    if (dump_armed) plugin_dump_after_list();
    return result;
}

static bool ensure_frame_storage(sr_plugin_screen *screen, uint32_t width,
                                 uint32_t height)
{
    if (width == 0u || height == 0u || width > SR_PLUGIN_MAX_FRAME_WIDTH ||
        height > SR_PLUGIN_MAX_FRAME_HEIGHT)
        return false;
    const uint32_t pixels = width * height;
    if (pixels <= screen->capacity_pixels) return true;
    sr_rgba8 *grown = realloc(screen->pixels, pixels * sizeof(*grown));
    if (!grown) return false;
    screen->pixels = grown;
    screen->capacity_pixels = pixels;
    /* The old buffer is gone, and with it whatever was shown from it. */
    screen->shown_pixels = NULL;
    return true;
}

sr_plugin_present_result sr_plugin_present(sr_plugin_screen *screen,
                                           sr_context *ctx,
                                           sr_present *present)
{
    plugin_dump_poll_hotkey();
    if (plugin_dump_armed) plugin_dump_on_present();

    if (!screen || !ctx || !present || !present->ready) return SR_PLUGIN_SKIPPED;

    sr_vi_frame_info info;
    memset(&info, 0, sizeof(info));
    const bool have_info = sr_get_vi_frame_info(ctx, &info) == SR_OK;

    if (have_info && info.display &&
        ensure_frame_storage(screen, info.width, info.height)) {
        sr_framebuffer fb;
        memset(&fb, 0, sizeof(fb));
        fb.pixels = screen->pixels;
        fb.width = info.width;
        fb.height = info.height;
        fb.stride_pixels = info.width;

        const sr_rgba8 *pixels;
        uint32_t height;
        if (sr_update_screen(ctx, &fb) == SR_OK && fb.valid &&
            sr_field_output_compose(&screen->field_output, fb.pixels, fb.width,
                                    fb.height, fb.stride_pixels, info.interlaced,
                                    info.field, &pixels, &height)) {
            sr_present_set_display_size(present,
                info.display_width ? info.display_width : info.width,
                info.display_height ? info.display_height : info.height);
            if (sr_present_upload_rgba8(present, pixels, fb.width, height, fb.width)) {
                screen->shown_pixels = pixels;
                screen->shown_width = fb.width;
                screen->shown_height = height;
                screen->blank = false;
                return SR_PLUGIN_PRESENTED;
            }
        }
    }

    /* Valid pixel type but no renderable frame this refresh (e.g. H_START not
     * programmed yet): redraw the retained frame rather than flash black. A
     * blank screen is retained as blank. */
    if (have_info && info.hold && !screen->blank) {
        sr_present_draw(present);
        return SR_PLUGIN_HELD;
    }

    sr_field_output_reset(&screen->field_output);
    screen->shown_pixels = NULL;
    screen->blank = true;
    sr_present_clear(present);
    return SR_PLUGIN_BLANKED;
}

void sr_plugin_screen_reset(sr_plugin_screen *screen)
{
    if (!screen) return;
    sr_field_output_reset(&screen->field_output);
    free(screen->pixels);
    memset(screen, 0, sizeof(*screen));
}
