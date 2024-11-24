#ifndef UNIT_TEST_H
#define UNIT_TEST_H

#include <stddef.h>
#include <unistd.h>

#include "error.h"

typedef struct result_void (* unit_test_fn)(void);

/**
 * Main structure for unit testing.
 *
 * Associates a given test with a name. In the future, other data may be added,
 * but as of now, this simple structure will suffice.
 *
 * the function will return a pointer to an error message (c-str) that describes
 * how the test failed, or NULL.
 */
struct test {
    char* name;
    unit_test_fn unit_test;
};

/**
 * A common error type for unit tests to return in general cases.
 */
extern const char GENERAL_ERROR[];
extern const char TEST_NOT_IMPLEMENTED_ERROR[];

struct failed_test {
    int num;
    const char* name;
    struct error error;
};

struct result_void fail_msg(const char *msg, ...);

/** returns a non-error status result struct. */
struct result_void no_error();

/**
 * "main" function for all test suites.
 *
 * As tests in `tests` are run, they will be printed out in a neatly formatted
 * and aligned table. To indicate progress, before a test is ran, an
 * unhighlighted entry in the table is appended. Once the test is finished, that
 * entry is updated to match the test result.
 *
 * @param[in] tests an array of test structurs to be run.
 * @param[in] num_tests number of tests within `tests`
 * @param[in] test_suite_name name of the entire test suite. will be drawn in
 *            the table header.
 *
 * @return None.
 */
void run_test_suite(const struct test tests[], size_t num_tests,
                    const char *test_suite_name);

#endif
