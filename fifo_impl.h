/**
 * fifo_impl.h
 *
 * Interface to the implementation of first come first serve (FIFO) specific
 * scheduler functionality.
 */

#ifndef __FIFO_IMPL__H__
#define __FIFO_IMPL__H__

#include "scheduler.h"

void fifo_enter_queue(thread_info_t *info);
void fifo_leave_queue(thread_info_t *info);

void fifo_wake_worker(thread_info_t *info);
void fifo_wait(sched_queue_t *queue);

#endif
