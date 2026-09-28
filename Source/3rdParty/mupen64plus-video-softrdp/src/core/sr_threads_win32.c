#include "sr_threads.h"

/* Win32 threading is isolated to this translation unit. */
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
/* GetLogicalProcessorInformationEx needs Windows 7; MinGW's default target is
 * older and would hide the declaration. */
#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0601
#  undef _WIN32_WINNT
#  define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
#include <stdlib.h>

typedef struct sr_worker {
    HANDLE thread;
    HANDLE start;   /* auto-reset: signalled to release the worker */
    HANDLE done;    /* auto-reset: signalled when the worker finishes */
    uint32_t id;
} sr_worker;

static struct {
    sr_worker workers[SR_THREADS_MAX];  /* index 0 unused: worker 0 is the caller */
    uint32_t count;
    volatile LONG shutting_down;
    void (*fn)(void *, uint32_t, uint32_t);
    void *ctx;
} g_pool;

static DWORD WINAPI worker_main(LPVOID param)
{
    sr_worker *w = (sr_worker *)param;
    for (;;) {
        WaitForSingleObject(w->start, INFINITE);
        if (g_pool.shutting_down) {
            SetEvent(w->done);
            return 0;
        }
        /* g_pool.fn/ctx were published before start was signalled; the event
         * wait is an acquire barrier, so they are visible here. */
        g_pool.fn(g_pool.ctx, w->id, g_pool.count);
        SetEvent(w->done);
    }
}

void sr_threads_init(uint32_t worker_count)
{
    if (g_pool.count != 0u) return;  /* already initialised */
    if (worker_count < 1u) worker_count = 1u;
    if (worker_count > SR_THREADS_MAX) worker_count = SR_THREADS_MAX;

    /* Helper threads carry ids 1..count-1; worker 0 runs on the caller. */
    for (uint32_t i = 1u; i < worker_count; i++) {
        sr_worker *w = &g_pool.workers[i];
        w->id = i;
        w->start = CreateEvent(NULL, FALSE, FALSE, NULL);
        w->done = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!w->start || !w->done) {
            worker_count = i;  /* fall back to what we successfully built */
            break;
        }
        w->thread = CreateThread(NULL, 0, worker_main, w, 0, NULL);
        if (!w->thread) {
            CloseHandle(w->start);
            CloseHandle(w->done);
            worker_count = i;
            break;
        }
    }
    g_pool.count = worker_count;
}

uint32_t sr_threads_count(void)
{
    return g_pool.count == 0u ? 1u : g_pool.count;
}

uint32_t sr_threads_physical_cores(void)
{
    /* One RelationProcessorCore record per physical core, regardless of how
     * many logical processors it exposes - that is exactly the count wanted.
     * The Ex variant is used rather than GetSystemInfo/dwNumberOfProcessors
     * because the latter reports logical CPUs, and rather than the non-Ex
     * variant because that one cannot describe machines with more than one
     * processor group. */
    DWORD bytes = 0;
    if (GetLogicalProcessorInformationEx(RelationProcessorCore, NULL, &bytes) ||
        GetLastError() != ERROR_INSUFFICIENT_BUFFER || bytes == 0) {
        return 1u;
    }

    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buf = malloc(bytes);
    if (!buf) return 1u;
    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, buf, &bytes)) {
        free(buf);
        return 1u;
    }

    /* Records are variable-length; walk by each one's own Size. The bound must
     * be the header size, NOT sizeof the struct: that is the size of the whole
     * union (large enough for a RelationGroup record), so a buffer holding only
     * the shorter core records would look truncated from the very first one. */
    const DWORD header = FIELD_OFFSET(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, Processor);
    uint32_t cores = 0u;
    DWORD offset = 0;
    while (bytes - offset >= header) {
        const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *info =
            (const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)((const char *)buf + offset);
        /* Guard against a zero Size (would spin) and a Size running past the
         * buffer (would read out of bounds). */
        if (info->Size < header || info->Size > bytes - offset) break;
        if (info->Relationship == RelationProcessorCore) cores++;
        offset += info->Size;
    }
    free(buf);

    return cores == 0u ? 1u : cores;
}

void sr_threads_shutdown(void)
{
    const uint32_t count = g_pool.count;
    if (count <= 1u) {
        /* Nothing was ever created; just reopen the pool for a later init. */
        g_pool.count = 0u;
        return;
    }

    /* Release the helpers with the flag already set, so each wakes up, sees it
     * and returns instead of dispatching. */
    g_pool.shutting_down = 1;
    for (uint32_t i = 1u; i < count; i++)
        SetEvent(g_pool.workers[i].start);

    for (uint32_t i = 1u; i < count; i++) {
        sr_worker *w = &g_pool.workers[i];
        /* Join on the thread, not on its done event: the event only says the
         * worker decided to leave, while the thread handle also covers it
         * actually finishing. Closing the events it waits on - or unmapping
         * this code - before that point is what has to be ruled out. */
        WaitForSingleObject(w->thread, INFINITE);
        CloseHandle(w->thread);
        CloseHandle(w->start);
        CloseHandle(w->done);
        w->thread = NULL;
        w->start = NULL;
        w->done = NULL;
    }

    g_pool.shutting_down = 0;
    g_pool.fn = NULL;
    g_pool.ctx = NULL;
    g_pool.count = 0u;
}

void sr_threads_run(void (*fn)(void *ctx, uint32_t id, uint32_t count),
                    void *ctx)
{
    const uint32_t count = g_pool.count;
    if (count <= 1u) {
        fn(ctx, 0u, 1u);
        return;
    }

    g_pool.fn = fn;
    g_pool.ctx = ctx;

    /* Release helpers, run our own share, then wait for helpers to finish. */
    for (uint32_t i = 1u; i < count; i++)
        SetEvent(g_pool.workers[i].start);

    fn(ctx, 0u, count);

    for (uint32_t i = 1u; i < count; i++)
        WaitForSingleObject(g_pool.workers[i].done, INFINITE);
}
