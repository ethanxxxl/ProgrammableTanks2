/* Client Specific Includes */
#include "client-commands.h"
#include "client-gfx.h"
#include "game-manager.h"

/* Project Library Includes */
#include "scenario.h"
#include "vector.h"
#include "message.h"
#include "command-line.h"

/* STD Lib Includes */
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// TODO FIXME BUG program segfaults when you enter blank username.

/* GLOBAL VARIABLES
 */

struct coord coord;

pthread_t g_gfx_pid;
pthread_t g_read_msg_pid;

bool g_run_program;
bool g_server_connected;
int g_server_sock;

bool g_gfx_running;

char g_username[50];
bool g_print_msg;

char g_bg_color;

struct vector* g_players;

/*
 * Command Master List
 */
// master list of all commands in the client program. In the future, I
// may integrate the documentation into this structure as well, and
// make the help function generate the help text automatically.

struct command COMMANDS[] = {
    {"connect", &connect_serv},
    {"start-gfx", &start_gfx},
    {"list-tanks", &list_tanks},
    {"update", &propose_update},
    {"update-tank", &update_tank},
    {"request", &request_server_update},
    {"auth", &authenticate},
    {"change-state", &change_state},
    {"list-scenarios", &list_scenarios},

    {"msg", &message_server},
    {"color", &change_bg_color},

    {"debug-messages", &enable_print_messages},

    //{"\n", &dummy},
    {"quit", &quit},
};

struct command_line_args COMMAND_LINE_ENVIRONMENT = {
    .documentation_file = "documentation/client-commands.org",
    .command_list = COMMANDS,
    .num_commands = sizeof(COMMANDS)/sizeof(struct command),
    .run_program = &g_run_program,
};

char g_welcome_message[] = "\
--Welcome to  Programable Tanks 2!--\n\
\n\
This client estabilishes a connection between the scenario servers and your\n\
control code. Included also is a graphical display of the scenario, allowing you\n\
to monitor the progress of the match. This program is a work in progress.\n\
\n\
The `help' command will provide a listing of all commands available to you,\n\
along with a brief description of their operation.\n\
\n\
This program is written and maintained by Ethan Smith.\n";

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    g_gfx_pid = 0;
    g_read_msg_pid = 0;

    g_run_program = true;
    g_server_connected = false;
    g_gfx_running = false;

    g_players = make_vector(sizeof(struct player), 5);

    puts(g_welcome_message);

    pthread_t command_line_pid = 0;
    pthread_create(&command_line_pid,
                   NULL,
                   &command_line_thread,
                   &COMMAND_LINE_ENVIRONMENT);

    printf("quitting program...\n");

    if (command_line_pid != 0)
        pthread_join(command_line_pid, NULL);
    
    if (g_gfx_pid != 0)
        pthread_join(g_gfx_pid, NULL);

    if (g_read_msg_pid != 0)
        pthread_join(g_read_msg_pid, NULL);

    exit(EXIT_SUCCESS);
}
