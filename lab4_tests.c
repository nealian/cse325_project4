/*************** YOU SHOULD NOT MODIFY ANYTHING IN THIS FILE ***************/
#define _GNU_SOURCE
#include <stdio.h>
#undef _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "testrunner.h"
#include "list.h"
#include "scheduler.h"

#define quit_if(cond) do {if (cond) exit(EXIT_FAILURE);} while(0)

/* Prepare input, reroute file descriptors, and run the program. */
void run_test(int argc, const char **argv)
{
	int fork_pid = fork();
	if (fork_pid == 0) {
		/* Reroute standard file descriptors */
		freopen("smp4.out", "w", stdout);
		/* Run the program */
		exit(smp4_main(argc, argv));
		fclose(stdout);
	} else if (fork_pid > 0) {
		waitpid(fork_pid, 0, 0);
	} else {
		fprintf(stderr, "run_test: fork() error\n");
	}
}

void read_header(FILE *in, int *workers, int *queue_size, int *iterations)
{
	char buf[1024];
	*workers = *queue_size = *iterations = 0;
	while (!feof(in)) {
		int workers_tmp, queue_size_tmp, iterations_tmp;
		fgets(buf, 1024, in);
		if (sscanf(buf, "Main: running %d workers on %d queue_size for %d iterations\n",
		                &workers_tmp, &queue_size_tmp, &iterations_tmp) == 3) {
			*workers    = workers_tmp;
			*queue_size = queue_size_tmp;
			*iterations = iterations_tmp;
		}
	}
}

int check_for_done(FILE *in)
{
	char buf[1024];
	int done = 0;
	while (!feof(in)) {
		fgets(buf, 1024, in);
		if (!feof(in)) {
			done = !strcmp(buf, "Scheduler: done!\n");
		}
	}
	return done;
}

void compute_queue_size(FILE *in, int *final_threads, int *threads_max, int *threads_min, int *admitted_threads)
{
	char buf[1024];
	*final_threads = *threads_max = *threads_min = *admitted_threads = 0;
	while (!feof(in)) {
		unsigned long thread_id;
		char nl;
		fgets(buf, 1024, in);
		if (sscanf(buf, "Thread %lu: in scheduler queue%c", &thread_id, &nl) == 2) {
			admitted_threads[0]++;
			final_threads[0]++;
			if (*final_threads > *threads_max)
				*threads_max = *final_threads;
		} else if (sscanf(buf, "Thread %lu: exiting%c", &thread_id, &nl) == 2) {
			final_threads[0]--;
			if (*final_threads < *threads_min)
				*threads_min = *final_threads;
		}
	}
}

unsigned long *lookup_bucket(unsigned long *buckets, int id)
{
	int i;
	for (i = 2; i < buckets[0]*2+2; i += 2) {
		if (buckets[i] == id)
			return &buckets[i+1];
	}
	if (buckets[0] < buckets[1]) {
		buckets[buckets[0]*2+2] = id;
		buckets[buckets[0]*2+3] = 0;
		buckets[0]++;
		return &buckets[(buckets[0]-1)*2+3];
	} else {
		return NULL;
	}
}

int bucket_exists(unsigned long *buckets, int with_value)
{
	int i;
	for (i = 2; i < buckets[0]*2+2; i += 2) {
		if (buckets[i+1] == with_value)
			return 1;
	}
	return 0;
}

int check_executed(FILE *in, int num_threads, int num_iterations)
{
	char buf[1024];
	unsigned long buckets[2+num_threads*2];
	buckets[0] = 0;
	buckets[1] = num_threads;
	while (!feof(in)) {
		unsigned long thread_id;
		int iter;
		char nl;
		fgets(buf, 1024, in);
		if (sscanf(buf, "Thread %lu: in scheduler queue%c", &thread_id, &nl) == 2) {
			unsigned long *b = lookup_bucket(buckets, thread_id);
			if (b == NULL)
				return 0;
			b[0] = 0;
		} else if (sscanf(buf, "Thread %lu: loop %i%c", &thread_id, &iter, &nl) == 3) {
			unsigned long *b = lookup_bucket(buckets, thread_id);
			if (b == NULL)
				return 0;
			if (b[0] != iter)
				return 0;
			b[0]++;
		} else if (sscanf(buf, "Thread %lu: exiting%c", &thread_id, &nl) == 2) {
			unsigned long *b = lookup_bucket(buckets, thread_id);
			if (b == NULL)
				return 0;
			if (b[0] != num_iterations)
				return 0;
		}
	}
	return buckets[0] == buckets[1];
}

int check_executed_fifo(FILE *in, int num_threads, int num_iterations)
{
	char buf[1024];
	unsigned long prev_thread = 0;
	int counter = 0;
	while (!feof(in)) {
		unsigned long thread_id;
		int iter;
		char nl;
		fgets(buf, 1024, in);
		if (sscanf(buf, "Thread %lu: loop %i%c", &thread_id, &iter, &nl) == 3) {
			if (prev_thread == 0)
				prev_thread = thread_id;
			else if (prev_thread != thread_id)
				return 0;
			counter++;
		} else if (sscanf(buf, "Thread %lu: exiting%c", &thread_id, &nl) == 2) {
			if (counter != num_iterations)
				return 0;
			prev_thread = 0;
			counter = 0;
		}
	}
	return 1;
}

int check_executed_rr(FILE *in, int num_threads, int num_iterations)
{
	char buf[1024];
	unsigned long buckets[2+num_threads*2];
	int stepnum = 1;
	list_t rrq;
	list_init(&rrq);
	buckets[0] = 0;
	buckets[1] = num_threads;
	while (!feof(in)) {
			unsigned long thread_id;
		int iter;
		char nl;
		fgets(buf, 1024, in);
		if (sscanf(buf, "Thread %lu: in scheduler queue%c", &thread_id, &nl) == 2) {
			unsigned long *b = lookup_bucket(buckets, thread_id);
			if (b == NULL)
				return 0;
			b[0] = stepnum;
		} else if (sscanf(buf, "Thread %lu: loop %i%c", &thread_id, &iter, &nl) == 3) {
			unsigned long *b = lookup_bucket(buckets, thread_id);
			list_elem_t *elt;
			if (b == NULL)
				return 0;
			if (b[0] > stepnum) {
				// Check if anyone's left at stepnum
				if (bucket_exists(buckets, stepnum)) {
					return 0;
				}
				// Otherwise move on to the next step
				stepnum++;
			}
			b[0] = stepnum+1;
			if (iter == 0) {
				elt = (list_elem_t *) malloc(sizeof(list_elem_t));
				if (elt == NULL)
					return 0;
				list_elem_init(elt, (void *) thread_id);
				list_insert_tail(&rrq, elt);
			} else {
				elt = list_get_head(&rrq);
				if (elt == NULL)
					return 0;
				if (elt->datum != (void *) thread_id)
					return 0;
				list_remove_elem(&rrq, elt);
				list_insert_tail(&rrq, elt);
			}
		} else if (sscanf(buf, "Thread %lu: exiting%c", &thread_id, &nl) == 2) {
			unsigned long *b = lookup_bucket(buckets, thread_id);
			list_elem_t *elt = list_get_head(&rrq);
			if (b == NULL)
				return 0;
			b[0] = 0;
			if (elt == NULL)
				return 0;
			if (elt->datum != (void *) thread_id)
				return 0;
			list_remove_elem(&rrq, elt);
			free(elt);
		}
	}
	return 1;
}

int check_rudimentary(FILE *in, int workers, int queue_size, int iterations)
{
	int final_threads, threads_max, threads_min, admitted_threads;
	int actual_workers, actual_queue_size, actual_iterations;
	rewind(in);
	read_header(in, &actual_workers, &actual_queue_size, &actual_iterations);
	if (!((workers == actual_workers) && (queue_size == actual_queue_size) && (iterations == actual_iterations)))
		return 0;
	rewind(in);
	if (!check_for_done(in))
		return 0;
	rewind(in);
	compute_queue_size(in, &final_threads, &threads_max, &threads_min, &admitted_threads);
	if (!((threads_min == 0) && (threads_max <= queue_size) && (final_threads == 0) && (admitted_threads == workers)))
		return 0;
	rewind(in);
	if (!check_executed(in, workers, iterations))
		return 0;
	return 1;
}

int check_fifo(FILE *in, int queue_size, int workers, int iterations)
{
	if (!check_rudimentary(in, workers, queue_size, iterations))
		return 0;
	rewind(in);
	if (!check_executed_fifo(in, workers, iterations))
		return 0;
	return 1;
}

int check_rr(FILE *in, int queue_size, int workers, int iterations)
{
	if (!check_rudimentary(in, workers, queue_size, iterations))
		return 0;
	rewind(in);
	if (!check_executed_rr(in, workers, iterations))
		return 0;
	return 1;
}

void test_whatever(const char *impl, int queue_size, int workers, int iterations)
{
	char qs[16], wk[16], it[16];
	const char *args[] = { "./scheduler", impl, qs, wk, it, NULL };
	sprintf(qs, "%d", queue_size);
	sprintf(wk, "%d", workers);
	sprintf(it, "%d", iterations);
	run_test(5, args);
}

int test_fifo(int queue_size, int workers, int iterations)
{
	FILE *out;
	test_whatever("-fifo", queue_size, workers, iterations);
	out = fopen("smp4.out", "r");
	quit_if(!check_fifo(out, queue_size, workers, iterations));
	return EXIT_SUCCESS;
}

int test_rr(int queue_size, int workers, int iterations)
{
	FILE *out;
	test_whatever("-rr", queue_size, workers, iterations);
	out = fopen("smp4.out", "r");
	quit_if(!check_rr(out, queue_size, workers, iterations));
	return EXIT_SUCCESS;
}

int test_fifo_var(int argc, const char **argv)
{
	quit_if(argc != 4);
	return test_fifo(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
}

int test_rr_var(int argc, const char **argv)
{
	quit_if(argc != 4);
	return test_rr(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
}

int test_fifo_1_2_3(int argc, const char **argv)
{
	return test_fifo(1, 2, 3);
}

int test_fifo_10_2_3(int argc, const char **argv)
{
	return test_fifo(10, 2, 3);
}

int test_fifo_7_1_30(int argc, const char **argv)
{
	return test_fifo(7, 1, 30);
}

int test_fifo_7_5_5(int argc, const char **argv)
{
	return test_fifo(7, 5, 5);
}

int test_fifo_5_7_5(int argc, const char **argv)
{
	return test_fifo(5, 7, 5);
}

int test_rr_1_2_3(int argc, const char **argv)
{
	return test_rr(1, 2, 3);
}

int test_rr_10_2_3(int argc, const char **argv)
{
	return test_rr(10, 2, 3);
}

int test_rr_7_1_30(int argc, const char **argv)
{
	return test_rr(7, 1, 30);
}

int test_rr_7_5_5(int argc, const char **argv)
{
	return test_rr(7, 5, 5);
}

int test_rr_5_7_5(int argc, const char **argv)
{
	return test_rr(5, 7, 5);
}

/*
 * Main entry point for SMP4 test harness
 */
int run_smp4_tests(int argc, const char **argv)
{
	/* Tests can be invoked by matching their name or their suite name
	 * or 'all' */
	testentry_t tests[] = {
		{"fifo_var",    "var",  test_fifo_var   },
		{"rr_var",      "var",  test_rr_var     },
		{"fifo_1_2_3",  "fifo", test_fifo_1_2_3 },
		{"fifo_10_2_3", "fifo", test_fifo_10_2_3},
		{"fifo_7_1_30", "fifo", test_fifo_7_1_30},
		{"fifo_7_5_5",  "fifo", test_fifo_7_5_5 },
		{"fifo_5_7_5",  "fifo", test_fifo_5_7_5 },
		{"rr_1_2_3",    "rr",   test_rr_1_2_3   },
		{"rr_10_2_3",   "rr",   test_rr_10_2_3  },
		{"rr_7_1_30",   "rr",   test_rr_7_1_30  },
		{"rr_7_5_5",    "rr",   test_rr_7_5_5   },
		{"rr_5_7_5",    "rr",   test_rr_5_7_5   }
	};
	int result = run_testrunner(argc, argv, tests, sizeof(tests) / sizeof(testentry_t));
	unlink("smp4.out");
	return result;
}

/* The real main function.  */
int main(int argc, const char **argv)
{
	if (argc > 1 && !strcmp(argv[1], "-test")) {
		return run_smp4_tests(argc - 1, argv + 1);
	} else {
		return smp4_main(argc, argv);
	}
}
