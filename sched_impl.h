#ifndef	__SCHED_IMPL__H__
#define	__SCHED_IMPL__H__

#include <semaphore.h>
#include <pthread.h>

#include "list.h"

struct thread_info {
  /* Scheduling queue */
  sched_queue_t* queue;
  list_elem_t* queue_elem;
};

struct sched_queue {
  /* Actual data structure */
  list_t* list;

  /* Semaphores to synchronize queue capacity */
  sem_t* production, * consumption;

  /* Mutex lock to synchronize actual read/write operations on queue */
  pthread_mutex_t* access_mutex;
};

#endif /* __SCHED_IMPL__H__ */
