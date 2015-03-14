/**
 * fifo_impl.c
 *
 * Implementation of functionality specific to first-come-first-serve
 * scheduling.
 */

#include <stdio.h>

#include "fifo_impl.h"
#include "list.h"
#include "sched_impl.h"

void fifo_wait(sched_queue_t *queue) {
  /* Get current worker from head of queue */
  list_elem_t* head = list_get_head(queue->list);
  if(!head) {
    /* No workers active, so... Just return */
    return;
  }

  thread_info_t* worker = (thread_info_t*) head->datum;

  /* Block until worker has finished */
  if(pthread_mutex_lock(worker->yield_cpu)) {
    /* Handle error locking cpu yield */
    perror("scheduling policy waiting on worker");
  }
}
