#include <stdlib.h>

#include "scheduler.h"
#include "sched_impl.h"
#include "fifo_impl.h"
#include "rr_impl.h"


/***************************************
  Worker thread ops block
  All functions here should be independent of scheduling strategy.
****************************************/
static void init_thread_info(thread_info_t *info, sched_queue_t *queue) {
  /*...Code goes here...*/
  // TODO
}

static void destroy_thread_info(thread_info_t *info) {
  /*...Code goes here...*/
  // TODO
}

static void wait_for_cpu(thread_info_t *info) {
  // TODO
}

static void release_cpu(thread_info_t *info) {
  // TODO
}
/* End worker thread ops block */

/***************************************
  Scheduler ops block
  All functions here should be independent of scheduling strategy.
****************************************/
static void init_sched_queue(sched_queue_t *queue, int queue_size) {
  /*...Code goes here...*/
  // TODO
}

static void destroy_sched_queue(sched_queue_t *queue) {
  /*...Code goes here...*/
  // TODO
}

static thread_info_t* next_worker(sched_queue_t *queue) {
  // TODO
  return NULL;
}

static void wait_for_queue(sched_queue_t *queue) {
  // TODO
}
/* End scheduler ops block */

/* Static initialization of scheduling strategies */
sched_impl_t sched_fifo = {
  { init_thread_info, destroy_thread_info, fifo_enter_queue, fifo_leave_queue,
    wait_for_cpu, release_cpu }, 
  { init_sched_queue, destroy_sched_queue, fifo_wake_worker, fifo_wait,
    next_worker, wait_for_queue } },
sched_rr = {
  { init_thread_info, destroy_thread_info, rr_enter_queue, rr_leave_queue,
    wait_for_cpu, release_cpu },
  { init_sched_queue, destroy_sched_queue, rr_wake_worker, rr_wait,
    next_worker, wait_for_queue } };
