/**
 * rr_impl.c
 *
 * Implementation of functionality specific to round-robin scheduling.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "rr_impl.h"
#include "list.h"
#include "sched_impl.h"

void rr_wait(sched_queue_t *queue) {
  list_elem_t *head;
  thread_info_t *worker;

  /* Get head of queue */
  if(!pthread_mutex_lock(queue->access_mutex)) {
    head = list_get_head(queue->list);
    if(head) {
      worker = (thread_info_t*) head->datum;
      list_remove_elem(queue->list, head);
      list_insert_tail(queue->list, head);
    } else {
      /* Queue is empty. Just leave then. */
      pthread_mutex_unlock(queue->access_mutex);
      return;
    }
    pthread_mutex_unlock(queue->access_mutex);
  } else {
    /* Handle queue access lock failure */
  }

  /* Block until worker has finished */
  if(!pthread_mutex_lock(worker->yield_cpu)) {
    /* Error handling */
  }
}
