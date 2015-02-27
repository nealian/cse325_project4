/*************** YOU SHOULD NOT MODIFY ANYTHING IN THIS FILE ***************/
typedef int (*test_fp) (int, const char **);

typedef struct {
    const char *name;
    const char *suite;
    test_fp test_function;

} testentry_t;

int run_testrunner(int argc, const char **argv, testentry_t * entries,
		   int entry_count);
void set_testrunner_default_timeout(int s);
void set_testrunner_timeout(int s);
