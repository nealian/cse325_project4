#ifndef	__SCHEDULER__H__
#define	__SCHEDULER__H__

struct thread_info; /* To be defined in sched_impl.h */
typedef struct thread_info thread_info_t;

struct sched_queue; /* To be defined in sched_impl.h */
typedef struct sched_queue sched_queue_t;

/* A worker thread must be able to call the following operations.
 * See scheduler.c:worker_proc() for an example of usage. */
typedef struct worker_thread_ops {
	/* Initialize a thread_info_t */
	void (*init_thread_info)    (thread_info_t *info, sched_queue_t *queue);
	/* Release the resources associated with a thread_info_t */
	void (*destroy_thread_info) (thread_info_t *info);
	/* Block until the thread can enter the scheduler queue. */
	void (*enter_sched_queue)   (thread_info_t *info);
	/* Remove the thread from the scheduler queue. */
	void (*leave_sched_queue)   (thread_info_t *info);
	/* While on the scheduler queue, block until thread is scheduled. */
	void (*wait_for_cpu)        (thread_info_t *info);
	/* Voluntarily relinquish the CPU when this thread's timeslice is
	 * over (cooperative multithreading). */
	void (*release_cpu)         (thread_info_t *info);
} worker_thread_ops_t;

/* The scheduler thread must be able to call the following operations.
 * See scheduler.c:sched_proc() for an example of usage. */
typedef struct sched_ops {
	/* Initialize a sched_queue_t */
	void            (*init_sched_queue)    (sched_queue_t *queue, int queue_size);
	/* Release the resources associated with a sched_queue_t */
	void            (*destroy_sched_queue) (sched_queue_t *queue);
	/* Allow a worker thread to execute. */
	void            (*wake_up_worker)      (thread_info_t *info);
	/* Block until the current worker thread relinquishes the CPU. */
	void            (*wait_for_worker)     (sched_queue_t *queue);
	/* Select the next worker thread to execute.
	 * Returns NULL if the scheduler queue is empty. */
	thread_info_t * (*next_worker)         (sched_queue_t *queue);
	/* Block until at least one worker thread is in the scheduler queue. */
	void            (*wait_for_queue)      (sched_queue_t *queue);
} sched_ops_t;

/* A scheduler implementation consists of the above sets of operations. */
typedef struct sched_impl {
	worker_thread_ops_t worker_ops;
	sched_ops_t         sched_ops;
} sched_impl_t;

/* For this MP you will implement two different schedulers.
 * Hint: most of the operations will be the same in both. */
extern sched_impl_t sched_fifo, sched_rr; /* To be defined in sched_impl.c */
extern sched_impl_t sched_dummy; /* Defined in dummy_impl.c */

int smp4_main(int argc, const char **argv);

#endif /* __SCHEDULER__H__ */
