#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "scheduler.h"
#include "sched_impl.h"
#include "fifo_impl.h"
#include "rr_impl.h"

/***************************************
  Worker thread ops block
  All functions here should be independent of scheduling strategy.
****************************************/
static void init_thread_info(thread_info_t *info, sched_queue_t *queue) {
  // TODO: initialize info

  info->queue = queue;
  info->queue_elem = NULL;

  info->yield_cpu = malloc(sizeof(pthread_mutex_t));
  info->has_cpu = malloc(sizeof(pthread_mutex_t));
  
  /* The initializer is a weird macro, so we need this hackey stuff... */
  pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
  *(info->yield_cpu) = m1;
  pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;
  *(info->has_cpu) = m2;
  if(pthread_mutex_lock(info->has_cpu)) {
    /* Handle mutex lock failure */
    perror("worker thread wait for cpu");
  }
}

static void destroy_thread_info(thread_info_t *info) {
  info->queue = NULL;
  info->queue_elem = NULL;
  
  if(pthread_mutex_destroy(info->has_cpu)
     || pthread_mutex_destroy(info->yield_cpu)) {
    /* Handle errors on mutex destruction */
    perror("worker thread mutex destruction");
  }

  free(info->has_cpu);
  free(info->yield_cpu);
  info->has_cpu = NULL;
  info->yield_cpu = NULL;
}

static void enter_sched_queue(thread_info_t *info) {
  /* Create list element wrapper for info */
  info->queue_elem = malloc(sizeof(list_elem_t));
  list_elem_init(info->queue_elem, info);

  /* Block until there's space in the queue */
  if(!sem_wait(info->queue->production)) {

    /* Block while another thread is accessing the queue */
    if(!pthread_mutex_lock(info->queue->access_mutex)) {
      list_insert_tail(info->queue->list, info->queue_elem);

      /* Having added an element to the queue, unlock consumption semaphore */
      if(sem_post(info->queue->consumption)) {
        /* Handle semaphore unlock failure */
        perror("scheduling queue consumption unlock");
      }

      if(pthread_mutex_unlock(info->queue->access_mutex)) {
        /* Handle mutex unlock failure */
        perror("scheduling queue access mutex unlock");
      }
    } else {
      /* Handle mutex lock failure */
      perror("scheduling queue access mutex lock");
    }
  } else {
    /* Handle semaphore wait failure */
    perror("scheduling queue element insertion");
  }
}

static void leave_sched_queue(thread_info_t *info) {
  /* Lock consumption semaphore. This should never block. */
  if(!sem_trywait(info->queue->consumption)) {
    /* Block while another thread is accessing the queue */
    if(!pthread_mutex_lock(info->queue->access_mutex)) {
      list_remove_elem(info->queue->list, info->queue_elem);
      free(info->queue_elem);

      /* Having freed up space in the queue, unlock production semaphore */
      if(sem_post(info->queue->production)) {
        /* Handle semaphore unlock failure */
        perror("scheduling queue production unlock");
      }

      if(pthread_mutex_unlock(info->queue->access_mutex)) {
        /* Handle mutex unlock failure */
        perror("scheduling queue access mutex unlock");
      }
    } else {
      /* Handle mutex lock failure */
      perror("scheduling queue access mutex lock");
    }
  } else {
    /* Handle semaphore lock failure */
    /* Most likely, the given item wasn't in the queue... */
    perror("scheduling queue element removal");
  }
}

static void wait_for_cpu(thread_info_t *info) {
  /* Wait for CPU lock to release */
  if(pthread_mutex_lock(info->has_cpu)) {
    /* Handle mutex lock failure */
    perror("worker thread wait for cpu");
  }
}

static void release_cpu(thread_info_t *info) {
  /* Unlock thread yield */
  if(pthread_mutex_unlock(info->yield_cpu)) {
    /* Handle mutex unlock failure */
    perror("worker thread release cpu");
  }
}

static void wake_worker(thread_info_t *info) {
  if(pthread_mutex_unlock(info->has_cpu)) {
    /* Handle mutex unlock error */
    perror("wake worker");
  }
}
/* End worker thread ops block */

/***************************************
  Scheduler ops block
  All functions here should be independent of scheduling strategy.
****************************************/
static void init_sched_queue(sched_queue_t *queue, int queue_size) {
  queue->list = malloc(sizeof(list_t));
  list_init(queue->list);

  queue->production = malloc(sizeof(sem_t));
  queue->consumption = malloc(sizeof(sem_t));
  if(sem_init(queue->production, 0, queue_size) == -1
     || sem_init(queue->consumption, 0, 0) == -1) {
    /* Handle errors on semaphore initialization */
    perror("scheduling queue semaphore initialization");
    exit(1);
  }

  queue->access_mutex = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
  *(queue->access_mutex) = m;
}

static void destroy_sched_queue(sched_queue_t *queue) {

  /* Destroy and free each queue element */
  list_elem_t* e;
  while((e = list_get_head(queue->list))) {
    list_remove_elem(queue->list, e);
    destroy_thread_info((thread_info_t*) e->datum);
    free(e);
  }

  /* Close semaphore */
  if(sem_destroy(queue->production) || sem_destroy(queue->consumption)) {
    /* Handle errors on semaphore destruction */
    perror("scheduling queue semaphore destruction");
  }

  /* Close queue access mutex */
  if(pthread_mutex_destroy(queue->access_mutex)) {
    /* Handle errors on mutex destruction */
    perror("scheduling queue access mutex destruction");
  }

  /* Free remaining resources */
  free(queue->access_mutex);
  free(queue->production);
  free(queue->consumption);
  free(queue->list);
}

static thread_info_t* next_worker(sched_queue_t *queue) {
  /* On failure to retrieve worker, return NULL */
  thread_info_t* next = NULL;
  
  /* Block while another thread is accessing the queue */
  if(!pthread_mutex_lock(queue->access_mutex)) {
    list_elem_t* head = list_get_head(queue->list);

    /* If the queue is empty, head will be NULL */
    if(head) {
      next = (thread_info_t*) head->datum;
    }

    if(pthread_mutex_unlock(queue->access_mutex)) {
      /* Handle mutex unlock failure */
      perror("scheduling queue access mutex unlock");
    }
  } else {
    /* Handle mutex lock failure */
    perror("scheduling queue access mutex lock");
  }

  return next;
}

static void wait_for_queue(sched_queue_t *queue) {
  /* Block until there is an element in the queue */
  if(!sem_wait(queue->consumption)) {
    /* As soon as we lock the consumption semaphore, unlock it again */
    if(sem_post(queue->consumption)) {
      /* Handle semaphore unlock failure */
      perror("schedule queue waiting");
    }
  } else {
    /* Handle failure during blocking */
    perror("schedule queue waiting");
  }
}
/* End scheduler ops block */

/* Static initialization of scheduling strategies */
sched_impl_t sched_fifo = {
  { init_thread_info, destroy_thread_info, enter_sched_queue, leave_sched_queue,
    wait_for_cpu, release_cpu }, 
  { init_sched_queue, destroy_sched_queue, wake_worker, fifo_wait,
    next_worker, wait_for_queue } },
sched_rr = {
  { init_thread_info, destroy_thread_info, enter_sched_queue, leave_sched_queue,
    wait_for_cpu, release_cpu },
  { init_sched_queue, destroy_sched_queue, wake_worker, rr_wait,
    next_worker, wait_for_queue } };
