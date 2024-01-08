#include "unit-test.h"
#include "message.h"
#include "scenario.h"
#include "unistd.h"

// HACK these are externs referenced in scenario.c which are defined in the
// server translation unit. In order for the tests to compile, they need to
// provide a defnition for this global variable.
extern struct scenario g_scenario;
struct scenario g_scenario;

int test1_fn(void) {
    sleep(1);
    return 0;
}
struct test g_test1 = {
    .name = "example",
    .unit_test = &test1_fn    
};

int test2_fn(void) {
    sleep(1);
    return 0;
}
struct test g_test2 = {
    .name = "maybe vec_len?",
    .unit_test = &test2_fn    
};

int test3_fn(void) {
    sleep(1);
    return -1;
}
struct test g_test3 = {
    .name = "vec_reserve",
    .unit_test = &test3_fn
};

struct test *g_all_tests[] = {
    &g_test1,
    &g_test2,
    &g_test3,
};

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    size_t num_tests = sizeof(g_all_tests)/sizeof(struct test *);
    run_test_suite(g_all_tests, num_tests, "vector tests");
    
    return 0;
}
