#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include <stddef.h>
#include <stdbool.h>

typedef void command_fn(int, char**);

struct command {
    const char* name;
    command_fn* callback;
};

struct command_line_args {
    const char* documentation_file;

    const struct command* command_list;
    size_t num_commands;

    bool* run_program;
};

/**
 * Command line */
void* command_line_thread(void* arg);

#endif
