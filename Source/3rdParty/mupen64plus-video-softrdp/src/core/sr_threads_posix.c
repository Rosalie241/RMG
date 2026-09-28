#include "sr_threads.h"

/* POSIX threading is isolated to this translation unit. It is the direct
 * counterpart of sr_threads_win32.c: same pool shape, same ids, same
 * caller-runs-worker-0 rule. The auto-reset events that file uses map onto
 * unnamed semaphores one for one - sem_post releases exactly one wait and the
 * count is consumed by it, which is what "auto-reset" means. */

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* macOS has <semaphore.h>, but unnamed POSIX semaphores are not implemented
 * there: sem_init always fails with ENOSYS. Building the pool on it would make
 * every worker creation fail and silently drop the emulator to a single
 * thread. GCD semaphores have the same auto-reset counting behaviour and are
 * the supported replacement. */
#if defined(__APPLE__)
#include <dispatch/dispatch.h>
#include <sys/sysctl.h>

typedef dispatch_semaphore_t sr_sem_t;

static int sr_sem_init(sr_sem_t *s)
{
    *s = dispatch_semaphore_create(0);
    return *s != NULL ? 0 : -1;
}
static void sr_sem_destroy(sr_sem_t *s)
{
    if (*s) dispatch_release(*s);
    *s = NULL;
}
static void sr_sem_post(sr_sem_t *s) { dispatch_semaphore_signal(*s); }
/* dispatch_semaphore_wait does not return early on signal delivery, so this is
 * already the uninterruptible wait the sem_t path has to emulate. */
static void sr_sem_wait(sr_sem_t *s)
{
    dispatch_semaphore_wait(*s, DISPATCH_TIME_FOREVER);
}
#else
#include <semaphore.h>

typedef sem_t sr_sem_t;

static int sr_sem_init(sr_sem_t *s) { return sem_init(s, 0, 0); }
static void sr_sem_destroy(sr_sem_t *s) { sem_destroy(s); }
static void sr_sem_post(sr_sem_t *s) { sem_post(s); }

/* sem_wait is the one call here that can return early: a signal delivered to
 * this thread aborts the wait with EINTR, and treating that as a wake-up would
 * run the dispatch function without a dispatch. Emulators install signal
 * handlers, so this is not theoretical. */
static void sr_sem_wait(sr_sem_t *s)
{
    while (sem_wait(s) != 0) {
        /* errno == EINTR is the only documented spurious failure for an
         * initialised semaphore; retrying on anything else would spin, so
         * only that case loops. */
        if (errno != EINTR) return;
    }
}
#endif

typedef struct sr_worker {
    pthread_t thread;
    sr_sem_t start; /* posted to release the worker */
    sr_sem_t done;  /* posted when the worker finishes */
    uint32_t id;
    bool live;
} sr_worker;

static struct {
    sr_worker workers[SR_THREADS_MAX];  /* index 0 unused: worker 0 is the caller */
    uint32_t count;
    volatile int shutting_down;
    void (*fn)(void *, uint32_t, uint32_t);
    void *ctx;
} g_pool;

static void *worker_main(void *param)
{
    sr_worker *w = (sr_worker *)param;
    for (;;) {
        sr_sem_wait(&w->start);
        if (g_pool.shutting_down) {
            sr_sem_post(&w->done);
            return NULL;
        }
        /* g_pool.fn/ctx were published before start was posted; the semaphore
         * wait is an acquire barrier, so they are visible here. */
        g_pool.fn(g_pool.ctx, w->id, g_pool.count);
        sr_sem_post(&w->done);
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
        if (sr_sem_init(&w->start) != 0) {
            worker_count = i;  /* fall back to what we successfully built */
            break;
        }
        if (sr_sem_init(&w->done) != 0) {
            sr_sem_destroy(&w->start);
            worker_count = i;
            break;
        }
        if (pthread_create(&w->thread, NULL, worker_main, w) != 0) {
            sr_sem_destroy(&w->start);
            sr_sem_destroy(&w->done);
            worker_count = i;
            break;
        }
        w->live = true;
    }
    g_pool.count = worker_count;
}

uint32_t sr_threads_count(void)
{
    return g_pool.count == 0u ? 1u : g_pool.count;
}

#if !defined(__APPLE__)
/* Read a single integer out of a one-line sysfs file. */
static bool read_sysfs_int(const char *path, int *out)
{
    FILE *f = fopen(path, "r");
    if (!f) return false;
    const bool ok = fscanf(f, "%d", out) == 1;
    fclose(f);
    return ok;
}
#endif

uint32_t sr_threads_physical_cores(void)
{
#if defined(__APPLE__)
    /* Darwin has no /sys, so the topology scan below would find nothing and
     * report one core. sysctl answers directly: hw.perflevel0.physicalcpu is
     * the performance-core count on Apple silicon (perflevel1 is the
     * efficiency cluster, which is not worth scheduling raster work onto),
     * and hw.physicalcpu covers Intel Macs and older kernels. */
    int cores = 0;
    size_t len = sizeof(cores);
    if (sysctlbyname("hw.perflevel0.physicalcpu", &cores, &len, NULL, 0) != 0 ||
        cores < 1) {
        len = sizeof(cores);
        if (sysctlbyname("hw.physicalcpu", &cores, &len, NULL, 0) != 0)
            cores = 0;
    }
    return cores > 0 ? (uint32_t)cores : 1u;
#else
    /* sysconf(_SC_NPROCESSORS_ONLN) reports logical CPUs, which would
     * oversubscribe the pool by the SMT factor on most machines. The topology
     * directory is the only place the kernel exposes the physical grouping:
     * two logical CPUs sharing a (package, core) pair are SMT siblings of one
     * core, so counting distinct pairs counts physical cores. */
    struct { int package; int core; } seen[SR_THREADS_MAX * 8u];
    uint32_t seen_count = 0u;

    const long logical = sysconf(_SC_NPROCESSORS_ONLN);
    if (logical < 1) return 1u;

    /* CPU numbering can be sparse (offlined CPUs leave gaps), so scan a range
     * wider than the online count and stop once that many have been found. */
    long found = 0;
    for (int cpu = 0; cpu < 4096 && found < logical; cpu++) {
        char path[128];
        int package = 0;
        int core = 0;

        snprintf(path, sizeof(path),
                 "/sys/devices/system/cpu/cpu%d/topology/physical_package_id", cpu);
        if (!read_sysfs_int(path, &package)) continue;

        snprintf(path, sizeof(path),
                 "/sys/devices/system/cpu/cpu%d/topology/core_id", cpu);
        if (!read_sysfs_int(path, &core)) continue;

        found++;

        bool duplicate = false;
        for (uint32_t i = 0u; i < seen_count; i++) {
            if (seen[i].package == package && seen[i].core == core) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) continue;

        /* Past the array there is nothing useful left to learn: the pool is
         * capped at SR_THREADS_MAX and the count already exceeds it. */
        if (seen_count >= sizeof(seen) / sizeof(seen[0])) break;
        seen[seen_count].package = package;
        seen[seen_count].core = core;
        seen_count++;
    }

    /* No sysfs (a container with /sys masked, or a kernel without topology
     * support) degrades to single-threaded rather than to a logical count. */
    return seen_count == 0u ? 1u : seen_count;
#endif
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
    for (uint32_t i = 1u; i < count; i++) {
        if (g_pool.workers[i].live) sr_sem_post(&g_pool.workers[i].start);
    }

    for (uint32_t i = 1u; i < count; i++) {
        sr_worker *w = &g_pool.workers[i];
        if (!w->live) continue;
        /* Join on the thread, not on its done semaphore: the semaphore only
         * says the worker decided to leave, while pthread_join also covers it
         * actually finishing. Destroying the semaphores it waits on - or
         * unmapping this code - before that point is what has to be ruled
         * out. */
        pthread_join(w->thread, NULL);
        sr_sem_destroy(&w->start);
        sr_sem_destroy(&w->done);
        w->live = false;
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
        sr_sem_post(&g_pool.workers[i].start);

    fn(ctx, 0u, count);

    for (uint32_t i = 1u; i < count; i++)
        sr_sem_wait(&g_pool.workers[i].done);
}
