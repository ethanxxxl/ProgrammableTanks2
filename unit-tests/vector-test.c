#include <stdio.h>

#include "message.h"
#include "scenario.h"

// HACK these are externs referenced in scenario.c which are defined in the
// server translation unit. In order for the tests to compile, they need to
// provide a defnition for this global variable.
extern struct scenario g_scenario;
struct scenario g_scenario;

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printf("vector-test\n");
    return 0;
}
