#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <unistd.h>

#include "scheduler.h"
#include "sched_impl.h"

/* Used to pass thread arguments to worker_proc() */
typedef struct worker_args {
	pthread_t            thread_id;
	int                  iterations;
	worker_thread_ops_t *ops;
	thread_info_t        info;
} worker_args_t;

/* Used to pass thread arguments to sched_proc() */
typedef struct sched_args {
	sched_queue_t *queue;
	sched_ops_t   *sched_ops;
} sched_args_t;

/* The scheduler uses this counter to keep track of whether it should wait
 * for more threads to schedule or exit.  If your scheduler implementation
 * is written correctly, it will provide all of the synchronization
 * necessary to mediate access to the num_workers_remaining variable. */
static int num_workers_remaining = 0;

/* Procedure implementing a worker thread. */
static void *worker_proc(void *arg)
{
	int i;
	worker_args_t *wa = (worker_args_t *) arg;

	/* Compete with other threads to enter the scheduler queue. */
	wa->ops->enter_sched_queue(&wa->info);
	printf("Thread %lu: in scheduler queue\n", (unsigned long) wa->thread_id);

	for (i = 0; i < wa->iterations; i++) {
		/* Don't do anything until the scheduler tells us. */
		wa->ops->wait_for_cpu(&wa->info);

		/* Do some meaningless work... */
		usleep(30000);
		printf("Thread %lu: loop %d\n", (unsigned long) wa->thread_id, i);

		/* Let another worker have a chance. */
		wa->ops->release_cpu(&wa->info);
	}

	/* Leave the scheduler queue to make room for someone else
	 * and decrement the count of remaining threads. */
	wa->ops->wait_for_cpu(&wa->info);
	printf("Thread %lu: exiting\n", (unsigned long) wa->thread_id);
	wa->ops->leave_sched_queue(&wa->info);
	num_workers_remaining--;
	wa->ops->release_cpu(&wa->info);

	pthread_exit(0);
}

/* Procedure implementing the outline of the scheduler. */
static void *sched_proc(void *arg)
{
	sched_args_t  *sa        = (sched_args_t *) arg;
	sched_queue_t *queue     = sa->queue;
	sched_ops_t   *sched_ops = sa->sched_ops;

	/* Wait until there's at least one worker thread to schedule. */
	sched_ops->wait_for_queue(queue);

	/* Keep scheduling worker threads until we're done. */
	while (num_workers_remaining > 0) {
		thread_info_t *info = sched_ops->next_worker(queue);
		/* next_worker() returns NULL if the scheduler queue is empty. */
		if (info) {
			sched_ops->wake_up_worker(info);
			sched_ops->wait_for_worker(queue);
		} else {
			/* Wait for someone to enter the queue. */
			sched_ops->wait_for_queue(queue);
		}
	}
	printf("Scheduler: done!\n");

	return NULL;
}

/* Print out a message describing usage of the program. */
static void print_help(const char *progname)
{
	printf("\nusage: %s <sched_impl> <queue_size> <num_threads> [iterations]\n", progname);
	printf("\tsched_impl : -fifo or -rr or -dummy\n");
	printf("\tqueue_size : the number of threads that can be in the scheduler at one time\n");
	printf("\tnum_threads: the number of worker threads to run\n");
	printf("\titerations (optional): the number of loops each worker thread runs\n\n");
}

/* Display an error and terminate. */
static void exit_error(int err_num)
{
	fprintf(stderr, "failure: %s\n", strerror(err_num));
	exit(1);
}

/* Create thread_count-many worker threads, each of which run for iterations-many cycles. */
static worker_args_t *create_workers(worker_thread_ops_t *ops, int thread_count, int iterations, sched_queue_t *queue)
{
	int i = 0;
	/* Allocate all thread arguments in one big array (makes it easy to deallocate). */
	worker_args_t *argses = (worker_args_t *) malloc(thread_count * sizeof(worker_args_t));

	if (argses == NULL)
		exit_error(errno);

	/* Initialize counter used by sched_proc() to determine when to stop.
	 * This is safe assuming sched_ops->wait_for_queue() blocks in sched_proc(). */
	num_workers_remaining = thread_count;

	for (i = 0; i < thread_count; i++) {
		int err = 0;

		/* Set up worker thread arguments. */
		argses[i].iterations = iterations;
		argses[i].ops = ops;
		ops->init_thread_info(&argses[i].info, queue);

		/* Create worker thread. */
		err = pthread_create(&argses[i].thread_id, NULL, worker_proc, (void *) &argses[i]);
		if (err)
			exit_error(err);

		/* Detach worker thread. */
		printf("Main: detaching worker thread %lu\n", (unsigned long) argses[i].thread_id);
		pthread_detach(argses[i].thread_id);
	}

	/* Arguments can only be deallocated when we're sure the worker
	 * threads are done with them.  And besides, it contains a handy
	 * list of the thread_info_t's which need to be cleaned up at the
	 * end.  So return the arguments array. */
	return argses;
}

/* Launch the scheduler thread. */
static int start_scheduler(pthread_t *thread_id, sched_queue_t *queue, sched_ops_t *ops)
{
	/* Make arguments static so they're still valid after this function returns. */
	static sched_args_t sa;
	int err;

	/* Set up scheduler thread arguments. */
	sa.queue     = queue;
	sa.sched_ops = ops;

	/* Create the scheduler thread.	*/
	err = pthread_create(thread_id, NULL, sched_proc, &sa);
	if (err)
		exit_error(err);

	return err;
}

/* Deallocate all resources (called at end of program). */
static void clean_up(sched_impl_t *sched, int thread_count, worker_args_t *argses, sched_queue_t *queue)
{
	int i;
	for (i = 0; i < thread_count; i++)
		sched->worker_ops.destroy_thread_info(&argses[i].info);
	free(argses);
	sched->sched_ops.destroy_sched_queue(queue);
}

#define WORKER_ITERATIONS 5
int smp4_main(int argc, const char **argv)
{
	int thread_count = 0;
	int queue_size = 0;
	pthread_t sched_thread;
	int iterations = WORKER_ITERATIONS;
	worker_args_t *argses;
	sched_impl_t *sched;
	sched_queue_t queue;

	/* Collect command-line arguments (or exit on error). */
	if (argc < 4) {
		print_help(argv[0]);
		exit(0);
	}

	if (!strcmp("-fifo", argv[1])) {
		sched = &sched_fifo;
	} else if (!strcmp("-rr", argv[1])) {
		sched = &sched_rr;
	} else if (!strcmp("-dummy", argv[1])) {
		sched = &sched_dummy;
	} else {
		printf("\nApologies, I'm unfamiliar with the \"%s\" scheduling policy.\n", argv[1]);
		print_help(argv[0]);
		exit(0);
	}

	queue_size = atoi(argv[2]);
	thread_count = atoi(argv[3]);
	if (argc >= 5) {
		iterations = atoi(argv[4]);
	}

	/* Begin execution proper. */
	printf("Main: running %d workers on %d queue_size for %d iterations\n", thread_count, queue_size, iterations);

	/* Initialize anything that needs to be done for the scheduler queue. */
	sched->sched_ops.init_sched_queue(&queue, queue_size);

	/* Create a thread for the scheduler. */
	start_scheduler(&sched_thread, &queue, &sched->sched_ops);

	/* Create the worker threads and return. */
	argses = create_workers(&sched->worker_ops, thread_count, iterations, &queue);

	printf("Main: waiting for scheduler %lu\n", (unsigned long) sched_thread);
	/* Wait for scheduler to finish. */
	pthread_join(sched_thread, NULL);

	/* Clean up our resources. */
	clean_up(sched, thread_count, argses, &queue);

	/* This will wait for all other threads. */
	pthread_exit(0);
}
