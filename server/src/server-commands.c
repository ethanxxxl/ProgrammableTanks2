#include "server-commands.h"
#include "stdbool.h"

const struct command server_commands[] = {
    {"quit", &cmd_quit},
};

struct command_line_args server_command_line_args = {
    .documentation_file = "documentation/server-commands.org",

    .command_list = server_commands,
    .num_commands = sizeof(server_commands) / sizeof(struct command),

    .run_program = &g_run_server
};

void cmd_quit(int argc, char** argv, struct error *e) {
    (void)argc;
    (void)argv;
    (void)e;
    
    g_run_server = false;
}
