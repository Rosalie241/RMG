#include "plugin_dump.h"

#if SOFTRDP_ENABLE_DUMP

#include <windows.h>

#include "../core/sr.h"
#include "../core/sr_defs.h"
#include "../core/sr_dump.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

enum { FRAME_DUMP_PRESENT_COUNT = 64u };

volatile bool plugin_dump_armed = false;

static SRWLOCK g_lock = SRWLOCK_INIT;
static plugin_dump_host g_host;
static bool g_attached;
static sr_context *g_ctx;
static bool g_requested;
static sr_dump *g_dump;
static uint32_t g_presents_remaining;

static void dump_log(const char *fmt, ...)
{
    if (!g_host.log) return;
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    g_host.log(buf);
}

static uint32_t read_reg(const uint32_t *reg)
{
    return reg ? *reg : 0u;
}

static void refresh_armed_locked(void)
{
    plugin_dump_armed = g_requested || g_dump != NULL;
}

static void abort_locked(void)
{
    sr_dump_abort(g_dump);
    g_dump = NULL;
    g_requested = false;
    refresh_armed_locked();
}

static void start_recording_locked(void)
{
    g_requested = false;
    g_presents_remaining = FRAME_DUMP_PRESENT_COUNT;

    g_dump = sr_dump_open("frame_dump.bin", g_ctx, g_host.rdram,
                          g_host.rdram_size, g_host.rdram_bswapped);
    refresh_armed_locked();
    if (!g_dump) {
        dump_log("ERROR: Could not start frame_dump.bin (errno=%d)", errno);
        return;
    }
    dump_log("Starting frame dump recording for %u presented frames...",
             FRAME_DUMP_PRESENT_COUNT);
}

static void finish_recording_locked(void)
{
    if (!g_dump) return;
    const uint32_t lists = sr_dump_list_count(g_dump);
    const bool ok = sr_dump_close(g_dump);
    g_dump = NULL;
    refresh_armed_locked();

    if (ok)
        dump_log("Frame dump completed successfully! Recorded %u lists.", lists);
    else
        dump_log("ERROR: Frame dump incomplete after %u lists (errno=%d)", lists, errno);
}

void plugin_dump_attach(const plugin_dump_host *host, sr_context *ctx)
{
    AcquireSRWLockExclusive(&g_lock);
    abort_locked();
    g_host = *host;
    g_ctx = ctx;
    g_attached = host->rdram != NULL && host->rdram_size != 0u;
    ReleaseSRWLockExclusive(&g_lock);
}

void plugin_dump_detach(void)
{
    AcquireSRWLockExclusive(&g_lock);
    abort_locked();
    memset(&g_host, 0, sizeof(g_host));
    g_attached = false;
    g_ctx = NULL;
    ReleaseSRWLockExclusive(&g_lock);
}

void plugin_dump_poll_hotkey(void)
{
    static bool f11_was_down = false;
    const bool f11_is_down = (GetAsyncKeyState(VK_F11) & 0x8000) != 0;
    const bool pressed = f11_is_down && !f11_was_down;
    f11_was_down = f11_is_down;

    if (!pressed) {
        return;
    }

    AcquireSRWLockExclusive(&g_lock);
    if (g_attached && g_ctx && !g_dump) {
        g_requested = true;
        refresh_armed_locked();
        dump_log("F11 pressed: Requesting frame dump on next RDP list");
    }
    ReleaseSRWLockExclusive(&g_lock);
}

void plugin_dump_before_list(void)
{
    AcquireSRWLockExclusive(&g_lock);
    if (!g_attached) {
        ReleaseSRWLockExclusive(&g_lock);
        return;
    }

    if (g_requested) {
        start_recording_locked();
    }

    if (g_dump) {
        const uint32_t current = read_reg(g_host.dp[2]) & ~7u; /* DPC_CURRENT */
        const uint32_t end = read_reg(g_host.dp[1]) & ~7u;     /* DPC_END */
        const bool xbus_dma = (read_reg(g_host.dp[3]) & 0x001u) != 0u;

        /*
         * An XBUS list is addressed in DMEM and wraps, so it is unwrapped into a
         * flat buffer here; the writer takes bytes, not an address space.
         */
        uint8_t xbus_buffer[SR_DMEM_SIZE];
        const uint8_t *command = NULL;
        uint32_t size = 0;
        if (end > current) {
            if (xbus_dma && g_host.dmem && end - current <= SR_DMEM_SIZE) {
                size = end - current;
                for (uint32_t byte = 0; byte < size; byte++)
                    xbus_buffer[byte] = g_host.dmem[(current + byte) & (SR_DMEM_SIZE - 1u)];
                command = xbus_buffer;
            } else if (!xbus_dma && current < g_host.rdram_size &&
                       end <= g_host.rdram_size) {
                size = end - current;
                command = g_host.rdram + current;
            }
        }

        uint32_t dp[SR_DUMP_DP_REGS];
        uint32_t vi[SR_DUMP_VI_REGS];
        for (uint32_t i = 0; i < SR_DUMP_DP_REGS; i++) dp[i] = read_reg(g_host.dp[i]);
        for (uint32_t i = 0; i < SR_DUMP_VI_REGS; i++) vi[i] = read_reg(g_host.vi[i]);

        if (!sr_dump_record_list(g_dump, dp, vi, command, size)) {
            dump_log("ERROR: Could not write atomic frame dump record");
            abort_locked();
        }
    }
    ReleaseSRWLockExclusive(&g_lock);
}

void plugin_dump_after_list(void)
{
    AcquireSRWLockExclusive(&g_lock);
    /* RDP writes must be reproduced independently by each replay renderer,
     * not injected into the next record as host deltas. */
    sr_dump_sync_shadow(g_dump);
    ReleaseSRWLockExclusive(&g_lock);
}

void plugin_dump_on_present(void)
{
    AcquireSRWLockExclusive(&g_lock);
    if (g_dump) {
        if (g_presents_remaining > 0u) {
            g_presents_remaining--;
        }
        if (g_presents_remaining == 0u) {
            finish_recording_locked();
        }
    }
    ReleaseSRWLockExclusive(&g_lock);
}

#else

/* ISO C wants a declaration even when the whole driver compiles away. */
typedef int plugin_dump_unused;

#endif
