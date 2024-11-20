#ifndef SERVER_COMMANDS_H
#define SERVER_COMMANDS_H

#include "command-line.h"

command_fn cmd_quit;

extern bool g_run_server;

extern const struct command server_commands[];
extern struct command_line_args server_command_line_args;

#endif
