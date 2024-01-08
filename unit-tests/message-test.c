#include <stdio.h>
#include "scenario.h"
#include "unit-test.h"
#include "message.h"


// HACK these are externs referenced in scenario.c which are defined in the
// server translation unit. In order for the tests to compile, they need to
// provide a defnition for this global variable.
extern struct scenario g_scenario;
struct scenario g_scenario;

// data-carrying message types.
const char* tst_text_msg_serde(void) {
    return TEST_NOT_IMPLEMENTED_ERROR;
}
const char* tst_user_credentials_serde(void) {
    return TEST_NOT_IMPLEMENTED_ERROR;
}
const char* tst_player_update_serde(void) {
    return TEST_NOT_IMPLEMENTED_ERROR;
}
const char* tst_scenario_tick_serde(void) {
    return TEST_NOT_IMPLEMENTED_ERROR;
}

// TODO make tests for the rest of the message types.

struct test g_all_tests[] = {
    {"serialization: text", &tst_text_msg_serde},
    {"serialization: user credentials", &tst_user_credentials_serde},
    {"serialization: player update", &tst_player_update_serde},
    {"serialization: scenario tick", &tst_scenario_tick_serde},
};

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    size_t num_tests = sizeof(g_all_tests)/sizeof(struct test);
    const char header[] = "message test";
    run_test_suite(g_all_tests, num_tests, header);
    
    return 0;
}
