#ifndef SERVER_COMMANDS_H
#define SERVER_COMMANDS_H

#include "command-line.h"

void cmd_quit(int, char**);

extern bool g_run_server;

extern const struct command server_commands[];
extern struct command_line_args server_command_line_args;

#endif
