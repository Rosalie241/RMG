#include "sr_async.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0601
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
typedef CRITICAL_SECTION mutex;
typedef CONDITION_VARIABLE condition;
#define mutex_init(m) InitializeCriticalSection(m)
#define mutex_free(m) DeleteCriticalSection(m)
#define lock(m) EnterCriticalSection(m)
#define unlock(m) LeaveCriticalSection(m)
#define cond_init(c) InitializeConditionVariable(c)
#define wake(c) WakeAllConditionVariable(c)
#define await(c,m) SleepConditionVariableCS(c,m,INFINITE)
#else
#include <pthread.h>
typedef pthread_mutex_t mutex;
typedef pthread_cond_t condition;
#define mutex_init(m) pthread_mutex_init(m,NULL)
#define mutex_free(m) pthread_mutex_destroy(m)
#define lock(m) pthread_mutex_lock(m)
#define unlock(m) pthread_mutex_unlock(m)
#define cond_init(c) pthread_cond_init(c,NULL)
#define wake(c) pthread_cond_broadcast(c)
#define await(c,m) pthread_cond_wait(c,m)
#endif

enum { QUEUE_SIZE = 8 };
struct sr_async {
    mutex mutex;
    condition changed;
    struct { void (*fn)(void *); void *payload; } jobs[QUEUE_SIZE];
    uint64_t submitted, completed;
    bool stop;
#ifdef _WIN32
    HANDLE thread;
#else
    pthread_t thread;
#endif
};

#ifdef _WIN32
static DWORD WINAPI execute(void *opaque)
#else
static void *execute(void *opaque)
#endif
{
    sr_async *q = opaque;
    lock(&q->mutex);
    for (;;) {
        while (q->completed == q->submitted && !q->stop)
            await(&q->changed, &q->mutex);
        if (q->stop) break;
        unsigned slot = (unsigned)(q->completed % QUEUE_SIZE);
        void (*fn)(void *) = q->jobs[slot].fn;
        void *payload = q->jobs[slot].payload;
        unlock(&q->mutex);
        fn(payload);
        lock(&q->mutex);
        q->completed++;
        wake(&q->changed);
    }
    unlock(&q->mutex);
    return 0;
}

sr_async *sr_async_create(void)
{
    sr_async *q = calloc(1, sizeof(*q));
    if (!q) return NULL;
    mutex_init(&q->mutex);
    cond_init(&q->changed);
#ifdef _WIN32
    q->thread = CreateThread(NULL, 0, execute, q, 0, NULL);
    if (q->thread) return q;
#else
    if (pthread_create(&q->thread, NULL, execute, q) == 0) return q;
    pthread_cond_destroy(&q->changed);
#endif
    mutex_free(&q->mutex);
    free(q);
    return NULL;
}

void sr_async_submit(sr_async *q, void (*fn)(void *), void *payload)
{
    if (!q) { fn(payload); return; }
    lock(&q->mutex);
    while (q->submitted - q->completed == QUEUE_SIZE)
        await(&q->changed, &q->mutex);
    unsigned slot = (unsigned)(q->submitted % QUEUE_SIZE);
    q->jobs[slot].fn = fn;
    q->jobs[slot].payload = payload;
    q->submitted++;
    wake(&q->changed);
    unlock(&q->mutex);
}

void sr_async_wait(sr_async *q)
{
    if (!q) return;
    lock(&q->mutex);
    while (q->completed != q->submitted) await(&q->changed, &q->mutex);
    unlock(&q->mutex);
}
void sr_async_destroy(sr_async *q)
{
    if (!q) return;
    sr_async_wait(q);
    lock(&q->mutex);
    q->stop = true;
    wake(&q->changed);
    unlock(&q->mutex);
#ifdef _WIN32
    WaitForSingleObject(q->thread, INFINITE);
    CloseHandle(q->thread);
#else
    pthread_join(q->thread, NULL);
    pthread_cond_destroy(&q->changed);
#endif
    mutex_free(&q->mutex);
    free(q);
}
