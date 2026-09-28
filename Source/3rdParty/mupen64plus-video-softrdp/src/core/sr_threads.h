#ifndef SR_THREADS_H
#define SR_THREADS_H

#include <stdint.h>

/* Persistent worker pool for scanline-parallel rendering. The platform
 * threading API is confined to the sr_threads_<platform>.c implementing this
 * header; the rest of the core calls only this interface. Workers are created
 * once and reused for every dispatch, so there is no per-primitive thread
 * creation cost.
 *
 * The pool is process-wide and not itself thread-safe: init/run/shutdown are
 * expected to be called from one thread, the one that drives rendering. */

/* Hard upper bound on workers, whatever the machine reports. Also sizes the
 * per-worker replica arrays in sr.c, so raising it costs memory in every
 * renderer context. Past roughly this width the batch replay each worker
 * duplicates (texture loads, primitive compiles, edge stepping over every
 * scanline) outweighs what another core adds. */
#define SR_THREADS_MAX 12u

void sr_vi_threads_init(uint32_t worker_count);
uint32_t sr_vi_threads_count(void);
void sr_vi_threads_run(void (*fn)(void *, uint32_t, uint32_t), void *ctx);
void sr_vi_threads_shutdown(void);


/* Idempotent. Creates worker_count workers (clamped to [1, SR_THREADS_MAX]) on
 * first call; later calls are no-ops until sr_threads_shutdown. Worker 0 always
 * runs on the caller, so a count of 1 creates no thread at all. */
void sr_threads_init(uint32_t worker_count);

/* Number of workers the pool dispatches to (1 if uninitialized). */
uint32_t sr_threads_count(void);

/* Physical cores, counting the SMT/hyperthread siblings of one core only once -
 * the natural width to build the pool at. Returns 1 if the topology cannot be
 * determined; never a logical-CPU count, so a failed query degrades to
 * single-threaded rather than oversubscribing.
 *
 * Touches no pool state, and on some systems is the more awkward half to port:
 * where a thread API is a mechanical translation, physical-core counting
 * differs per OS (on Linux it means reading sysfs, since sysconf reports
 * logical CPUs). A port may leave this returning 1 and still have a working
 * pool. */
uint32_t sr_threads_physical_cores(void);

/* Tear the pool down and join every worker, leaving it re-initialisable. Safe
 * to call when uninitialized.
 *
 * Callers must reach this on any path that can unload the code the workers run
 * in: a worker still running inside an unmapped shared library crashes, and
 * that is a plugin-swap bug, not a shutdown bug. It must NOT be called from a
 * library unload/detach callback, though - joining threads while the loader
 * lock is held deadlocks. Call it from ordinary teardown instead. */
void sr_threads_shutdown(void);

/* Run fn on every worker id in [0, count) and block until all return. Worker 0
 * executes on the calling thread. fn must be reentrant across workers: the
 * common ctx is shared read-only and any writes must target worker-disjoint
 * memory.
 *
 * Implementations must guarantee that everything the caller wrote before this
 * call is visible to every worker, and everything the workers wrote is visible
 * to the caller once it returns - i.e. the release of a worker carries release
 * semantics and its pickup acquire semantics. Signalling primitives (events,
 * semaphores, condition variables) provide this on their own; an
 * implementation built on raw atomics or spin-waiting has to place the fences
 * itself. */
void sr_threads_run(void (*fn)(void *ctx, uint32_t id, uint32_t count),
                    void *ctx);

/* Resolve a configured worker count:
 * 0 = auto (1 on single-core, 2 on dual-core, physical cores - 1 up to max 6 on higher counts).
 * 1 = single-threaded serial execution (replaces threaded=0).
 * 2+ = specifically set worker count (clamped to SR_THREADS_MAX). */
static inline uint32_t sr_resolve_worker_count(uint32_t config_workers)
{
    if (config_workers == 1u) {
        return 1u;
    }
    if (config_workers == 0u) {
        const uint32_t cores = sr_threads_physical_cores();
        if (cores <= 1u) return 1u;
        if (cores == 2u) return 2u;
        const uint32_t count = cores - 1u;
        return count > 6u ? 6u : count;
    }
    return config_workers > SR_THREADS_MAX ? SR_THREADS_MAX : config_workers;
}

#endif
