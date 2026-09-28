/*
 * Scale selection.
 *
 * The core is linked in twice, once compiled for each scale, with its symbols
 * renamed by sr_namespace.h. This file provides the public sr_* API and forwards
 * each call to the copy that owns the context.
 *
 * Why this shape rather than a runtime scale inside one core: inside a core the
 * scale is a compile-time constant, so every expression that depends on it is
 * resolved by the preprocessor and the machine code is bit-for-bit what a
 * single-scale build produces. Threading the scale through as a runtime value
 * instead makes correctness easy and speed conditional on the optimiser
 * reconstructing the constant across every function boundary - which it does
 * until one function stops being inlined, with no diagnostic when it fails.
 *
 * The cost is one indirect call per API entry - a handful per frame, not per
 * pixel - and a binary carrying both cores.
 */

#include "sr.h"
#include "sr_scale.h"

#include <stdlib.h>
#include <string.h>

/* Both cores, under their namespaced names. Declared by hand rather than by
 * including sr.h twice, because sr.h describes the API, not these copies. */
#define SR_DECLARE_CORE(suffix)                                                \
    sr_context *sr_create##suffix(const sr_host_interface *host);              \
    void sr_destroy##suffix(sr_context *ctx);                                  \
    void sr_set_host##suffix(sr_context *ctx, const sr_host_interface *host);  \
    sr_result sr_process_rdp_list##suffix(sr_context *ctx);                    \
    void sr_flush##suffix(sr_context *ctx);                                    \
    uint32_t sr_debug_read_sample##suffix(const sr_context *ctx,               \
                                          uint32_t address, uint32_t sample);  \
    sr_result sr_get_vi_frame_info##suffix(sr_context *ctx,                    \
                                           sr_vi_frame_info *info);            \
    sr_result sr_update_screen##suffix(sr_context *ctx, sr_framebuffer *out);  \
    sr_debug_stats sr_get_debug_stats##suffix(const sr_context *ctx);          \
    size_t sr_state_snapshot_size##suffix(void);                               \
    sr_result sr_save_state##suffix(const sr_context *ctx, void *data,         \
                                    size_t size);                              \
    sr_result sr_load_state##suffix(sr_context *ctx, const void *data,         \
                                    size_t size);                              \
    sr_result sr_set_vi_scanline_registers##suffix(sr_context *ctx,            \
        const sr_vi_scanline_regs *regs, uint32_t count);

SR_DECLARE_CORE(_s1)
SR_DECLARE_CORE(_s2)

typedef struct sr_core_api {
    sr_context *(*create)(const sr_host_interface *);
    void (*destroy)(sr_context *);
    void (*set_host)(sr_context *, const sr_host_interface *);
    sr_result (*process_rdp_list)(sr_context *);
    void (*flush)(sr_context *);
    uint32_t (*debug_read_sample)(const sr_context *, uint32_t, uint32_t);
    sr_result (*get_vi_frame_info)(sr_context *, sr_vi_frame_info *);
    sr_result (*update_screen)(sr_context *, sr_framebuffer *);
    sr_debug_stats (*get_debug_stats)(const sr_context *);
    size_t (*state_snapshot_size)(void);
    sr_result (*save_state)(const sr_context *, void *, size_t);
    sr_result (*load_state)(sr_context *, const void *, size_t);
    sr_result (*set_vi_scanline_registers)(sr_context *,
                                           const sr_vi_scanline_regs *, uint32_t);
} sr_core_api;

#define SR_CORE_TABLE(suffix) {                                                \
    sr_create##suffix, sr_destroy##suffix, sr_set_host##suffix,                \
    sr_process_rdp_list##suffix, sr_flush##suffix,                             \
    sr_debug_read_sample##suffix, sr_get_vi_frame_info##suffix,                \
    sr_update_screen##suffix, sr_get_debug_stats##suffix,                      \
    sr_state_snapshot_size##suffix, sr_save_state##suffix,                     \
    sr_load_state##suffix, sr_set_vi_scanline_registers##suffix }

static const sr_core_api sr_core_1x = SR_CORE_TABLE(_s1);
static const sr_core_api sr_core_2x = SR_CORE_TABLE(_s2);

/*
 * A context handle that remembers which core made it, so a caller holding two
 * contexts at different scales stays correct. sr_context is opaque to callers,
 * so wrapping it costs them nothing.
 */
typedef struct sr_context_wrapper {
    const sr_core_api *api;
    sr_context *inner;
} sr_context_wrapper;

static const sr_core_api *core_for_scale(uint32_t scale)
{
    /* 0 means "no preference"; anything above the build maximum is clamped
     * rather than rejected, so a stale config still starts. */
    return scale >= 2u ? &sr_core_2x : &sr_core_1x;
}

sr_context *sr_create(const sr_host_interface *host)
{
    sr_context_wrapper *wrapper = calloc(1, sizeof(*wrapper));
    if (!wrapper) return NULL;
    wrapper->api = core_for_scale(host ? host->scale : 0u);
    wrapper->inner = wrapper->api->create(host);
    if (!wrapper->inner) {
        free(wrapper);
        return NULL;
    }
    return (sr_context *)wrapper;
}

void sr_destroy(sr_context *ctx)
{
    sr_context_wrapper *wrapper = (sr_context_wrapper *)ctx;
    if (!wrapper) return;
    wrapper->api->destroy(wrapper->inner);
    free(wrapper);
}

/*
 * The scale is fixed when the context is created, because it decides which core
 * allocated the sample planes. A later host with a different scale is honoured
 * for everything else and ignored for the scale; the plugins recreate the
 * context at RomOpen, which is where a scale change actually takes effect.
 */
void sr_set_host(sr_context *ctx, const sr_host_interface *host)
{
    sr_context_wrapper *wrapper = (sr_context_wrapper *)ctx;
    if (!wrapper) return;
    wrapper->api->set_host(wrapper->inner, host);
}

sr_result sr_process_rdp_list(sr_context *ctx)
{
    sr_context_wrapper *wrapper = (sr_context_wrapper *)ctx;
    if (!wrapper) return SR_ERROR_INVALID_ARGUMENT;
    return wrapper->api->process_rdp_list(wrapper->inner);
}

void sr_flush(sr_context *ctx)
{
    sr_context_wrapper *wrapper = (sr_context_wrapper *)ctx;
    if (wrapper) wrapper->api->flush(wrapper->inner);
}

uint32_t sr_debug_read_sample(const sr_context *ctx, uint32_t address,
                              uint32_t sample)
{
    const sr_context_wrapper *wrapper = (const sr_context_wrapper *)ctx;
    if (!wrapper) return 0u;
    return wrapper->api->debug_read_sample(wrapper->inner, address, sample);
}

sr_result sr_get_vi_frame_info(sr_context *ctx, sr_vi_frame_info *info)
{
    sr_context_wrapper *wrapper = (sr_context_wrapper *)ctx;
    if (!wrapper) return SR_ERROR_INVALID_ARGUMENT;
    return wrapper->api->get_vi_frame_info(wrapper->inner, info);
}

sr_result sr_update_screen(sr_context *ctx, sr_framebuffer *out)
{
    sr_context_wrapper *wrapper = (sr_context_wrapper *)ctx;
    if (!wrapper) return SR_ERROR_INVALID_ARGUMENT;
    return wrapper->api->update_screen(wrapper->inner, out);
}

sr_result sr_set_vi_scanline_registers(sr_context *ctx,
                                       const sr_vi_scanline_regs *regs,
                                       uint32_t count)
{
    sr_context_wrapper *wrapper = (sr_context_wrapper *)ctx;
    if (!wrapper) return SR_ERROR_INVALID_ARGUMENT;
    return wrapper->api->set_vi_scanline_registers(wrapper->inner, regs, count);
}

sr_debug_stats sr_get_debug_stats(const sr_context *ctx)
{
    const sr_context_wrapper *wrapper = (const sr_context_wrapper *)ctx;
    if (!wrapper) {
        sr_debug_stats empty;
        memset(&empty, 0, sizeof(empty));
        return empty;
    }
    return wrapper->api->get_debug_stats(wrapper->inner);
}

/* Both cores share one snapshot layout, so either may answer. */
size_t sr_state_snapshot_size(void)
{
    return sr_core_1x.state_snapshot_size();
}

sr_result sr_save_state(const sr_context *ctx, void *data, size_t size)
{
    const sr_context_wrapper *wrapper = (const sr_context_wrapper *)ctx;
    if (!wrapper) return SR_ERROR_INVALID_ARGUMENT;
    return wrapper->api->save_state(wrapper->inner, data, size);
}

sr_result sr_load_state(sr_context *ctx, const void *data, size_t size)
{
    sr_context_wrapper *wrapper = (sr_context_wrapper *)ctx;
    if (!wrapper) return SR_ERROR_INVALID_ARGUMENT;
    return wrapper->api->load_state(wrapper->inner, data, size);
}
