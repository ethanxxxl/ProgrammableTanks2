#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include "error.h"

#include <stddef.h>
#include <stdbool.h>

typedef void command_fn(int, char**, struct error *);

/** describes a command that can be called on the command line.

    The command function must take your typical argc and argv argument
    parameters.  It additionally will be passed a pointer to an error structure
    that may be filled out if an error occurs.  If an error is returned in the
    command, then the error should be written to this location.
*/
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
