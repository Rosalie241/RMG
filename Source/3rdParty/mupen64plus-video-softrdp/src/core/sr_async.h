#ifndef SR_ASYNC_H
#define SR_ASYNC_H

/* A bounded FIFO with one persistent execution owner. Submission transfers
 * ownership of the payload to fn. Wait covers all submissions by this caller.
 * Only the producer and executor synchronize through this queue. VI reads
 * completed display memory independently under the guest's ownership rules. */
typedef struct sr_async sr_async;
sr_async *sr_async_create(void);
void sr_async_submit(sr_async *queue, void (*fn)(void *), void *payload);
void sr_async_wait(sr_async *queue);
void sr_async_destroy(sr_async *queue);
#endif
