/**
 * rr_impl.h
 *
 * Interface to the implementation of round-robin (RR) specific
 * scheduler functionality.
 */

#ifndef __RR_IMPL__H__
#define __RR_IMPL__H__

#include "scheduler.h"

void rr_wake_worker(thread_info_t *info);
void rr_wait(sched_queue_t *queue);

#endif
