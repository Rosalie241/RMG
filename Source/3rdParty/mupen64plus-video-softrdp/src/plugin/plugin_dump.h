#ifndef PLUGIN_DUMP_H
#define PLUGIN_DUMP_H

/*
 * F11 frame dump driver for the Windows plugins.
 *
 * The file format lives in src/core/sr_dump.c and knows nothing about a host.
 * This layer is the part that is genuinely plugin-shaped: the hotkey, the lock
 * ProcessRDPList and the present callback race over, the frame countdown, and
 * reading the registers out of a GFX_INFO. Both plugins share one copy of it;
 * all that differs is the pointer table each one fills in.
 */

#include <stdbool.h>
#include <stdint.h>

typedef struct sr_context sr_context;

/*
 * Off unless the build asks for it (make SOFTRDP_DUMP=1). It is a development
 * tool: nothing in the render path needs it, and a front-end that wants dumps
 * is already a debugging build. When it is off every entry point below is an
 * empty inline and neither half of the recorder is compiled at all.
 */
#ifndef SOFTRDP_ENABLE_DUMP
#define SOFTRDP_ENABLE_DUMP 0
#endif

/*
 * The driver is Windows-only - the hotkey and the lock are Win32 - so asking
 * for it elsewhere is a build configuration error rather than something to
 * silently ignore. The file format writer (src/core/sr_dump.c) is portable;
 * a non-Windows host drives that directly instead of going through this.
 */
#if SOFTRDP_ENABLE_DUMP && !defined(_WIN32)
#error "SOFTRDP_DUMP=1 needs Windows; call sr_dump.c directly on other hosts"
#endif

/*
 * What the recorder needs from a host, resolved once at attach by
 * plugin_common.c from the sr_plugin_machine. The register pointers are the
 * host's own, read live on every list, in sr_dp_register / sr_vi_register
 * order.
 */
typedef struct plugin_dump_host {
    const uint8_t *rdram;
    const uint8_t *dmem;          /* may be NULL; XBUS lists are skipped without it */
    uint32_t rdram_size;
    bool rdram_bswapped;
    const uint32_t *dp[8];
    const uint32_t *vi[14];
    /* Plain message, already formatted: varargs cannot be forwarded. */
    void (*log)(const char *message);
} plugin_dump_host;


#if SOFTRDP_ENABLE_DUMP

/*
 * Set once a recording has been requested or is running. ProcessRDPList reads
 * it on every list, so it is the entire cost the non-recording path pays; the
 * locking and register reads live behind it.
 */
extern volatile bool plugin_dump_armed;

/* Copies the descriptor; the pointers inside it must outlive the session. */
void plugin_dump_attach(const plugin_dump_host *host, sr_context *ctx);
void plugin_dump_detach(void);

/* Once per presented frame. Arms a recording when the hotkey goes down. */
void plugin_dump_poll_hotkey(void);

/* Guard each of these with plugin_dump_armed. */
void plugin_dump_before_list(void);
void plugin_dump_after_list(void);
void plugin_dump_on_present(void);

#else

#define plugin_dump_armed false

static inline void plugin_dump_attach(const plugin_dump_host *host, sr_context *ctx)
{
    (void)host;
    (void)ctx;
}
static inline void plugin_dump_detach(void) {}
static inline void plugin_dump_poll_hotkey(void) {}
static inline void plugin_dump_before_list(void) {}
static inline void plugin_dump_after_list(void) {}
static inline void plugin_dump_on_present(void) {}

#endif

#endif
