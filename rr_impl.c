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
  list_elem_t *queue_elem;
  thread_info_t *new_head_info;
  if(!pthread_mutex_lock(queue->access_mutex)) {
    /* Current worker is moved to the back of the line */
    queue_elem = list_get_head(queue->list);

    /* If the queue is empty, the head elem will be NULL */
    if(queue_elem) {
      list_remove_elem(queue->list, queue_elem);
      list_insert_tail(queue->list, queue_elem);

      /* re-use queue_elem for the new head */

      queue_elem = list_get_head(queue->list);

      /* If the queue is empty, the head elem will be NULL */
      if(queue_elem) {
        new_head_info = (thread_info_t *) queue_elem->datum;

        if(pthread_mutex_unlock(queue->access_mutex)) {
          /* Handle mutex unlock failure */
          fprintf(stderr, "Couldn't unlock queue access mutex after rotation in RR\n");
        }

        if(pthread_mutex_lock(new_head_info->yield_cpu)) {
          /* Handle mutex lock failure */
          fprintf(stderr, "Couldn't lock cpu yield in RR\n");
        }
      } else {
        /* Where did it go in the last milliseconds?!? (Shouldn't happen) */
        /* return, I guess? */
        return;
      }

    } else {
      /* Uh, where'd our worker go? */
      /* return, I guess? */
      return;
    }
  } else {
    /* Handle mutex lock failure */
    fprintf(stderr, "Couldn't acquire queue access lock for rotation in RR\n");
  }
}
