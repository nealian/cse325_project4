#include "scheduler.h"

void dummy_ti_sq(thread_info_t *info, sched_queue_t *queue) { }
void dummy_ti(thread_info_t *info) { }
void dummy_sq_i(sched_queue_t *queue, int queue_size) { }
void dummy_sq(sched_queue_t *queue) { }
thread_info_t *ti_dummy_sq(sched_queue_t *queue) { return 0; }

sched_impl_t sched_dummy = {
	{ dummy_ti_sq, dummy_ti, dummy_ti, dummy_ti, dummy_ti, dummy_ti },
	{ dummy_sq_i, dummy_sq, dummy_ti, dummy_sq, ti_dummy_sq, dummy_sq }
};
