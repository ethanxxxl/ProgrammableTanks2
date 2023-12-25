#include <SDL2/SDL.h>
#include "vector.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <message.h>

/* GLOBAL VARIABLES
 */
bool g_run_program;
int g_server_sock;

int debug_send_msg(struct message msg) {
    printf("--SENDING--\n");
    print_message(msg);
    return message_send(g_server_sock, msg);    
}
/*
 * Command Callback Functions
 */

void propose_update(int argc, char **argv) {
    printf("proposing update...\n");
    printf("COMMAND FAILED: NOT IMPLEMENTED\n");
}
void request_server_update(int argc, char **argv) {
    printf("requesting info from server...\n");
    printf("COMMAND FAILED: NOT IMPLEMENTED\n");
}

void authenticate(int argc, char **argv) {
    struct message msg;
    make_message(&msg, MSG_REQUEST_AUTHENTICATE);
    vec_pushn(&msg.user_credentials.username, argv[1], strlen(argv[1]));
    debug_send_msg(msg);
}
void change_state(int argc, char **argv) {
    if (argc != 2) {
    printf("ERROR: valid options are \"scene\" or \"lobby\"\n");
    return;
    }

    struct message msg;
    if (strcmp(argv[1], "scene") == 0) {
        msg = (struct message){MSG_REQUEST_JOIN_SCENARIO};
    } else if (strcmp(argv[1], "lobby") == 0) {
    msg = (struct message){MSG_REQUEST_RETURN_TO_LOBBY};
    } else {
        printf("ERROR: valid options are \"scene\" or \"lobby\"\n");
    return;
    }

    debug_send_msg(msg);
    return;
}
void list_scenarios(int argc, char **argv) {
    struct message msg = {MSG_REQUEST_LIST_SCENARIOS};
    debug_send_msg(msg);
}

void add_tank(int argc, char **argv) {}

void help_page(int argc, char** argv);

void message_server(int argc, char **argv) {
    struct message msg = {MSG_REQUEST_DEBUG};
    make_vector(&msg.text, sizeof(char), 50);

    for (int i = 1; i < argc; i++) {
        vec_pushn(&msg.text, argv[i], strlen(argv[i]));
        vec_push(&msg.text, " ");
    }
    vec_push(&msg.text, "\0");

    debug_send_msg(msg);
}
void command_error(int argc, char **argv) {
    printf("ERROR: Unknown Command \"%s\"\n", argv[0]);
}
void quit(int argc, char **argv) {
    g_run_program = false;
}

void dummy(int arggc, char **argv) {}

int g_bg_color;
void change_bg_color(int argc, char **argv) {
    if (argc != 2) {
        printf("ERROR: there may only be a single argument to this function");
        return;
    }

    g_bg_color = atoi(argv[1]);
}

/*
 * Command Master List
 */
// master list of all commands in the client program. In the future, I
// may integrate the documentation into this structure as well, and
// make the help function generate the help text automatically.
const struct command {
    char name[20];
    void (*callback)(int, char**);
    char* docs;
} COMMANDS[] = {
    {"update", &propose_update,
     "NOT IMPLEMENTED"},
    {"add-tank", &add_tank,
     "NOT IMPLEMENTED"},
    {"request", &request_server_update,
     "NOT IMPLEMENTED"},
    {"auth", &authenticate,
     "takes a single argument, which is your username. registers you with the server."},
    {"change-state", &change_state,
     "valid options are 'lobby' or 'scene'."},
    {"list-scenarios", &list_scenarios,
     "asks the server for available scenarios to join (currently only one)."},
    {"help", &help_page,
     "displays this help page."},
    {"msg", &message_server,
     "sends the server a DEBUG_MESSAGE. sending a DEBUG_MESSAGE with the text\
'kill-serv' will cause the server to quit."},

    {"color", &change_bg_color,
     "changes the bg color of the SDL graphics window."},

    {"\n", &dummy},
    {"q", &quit, "quit this client"},
    {"quit", &quit, "quit this client"},
    {"exit", &quit, "quit this client"},
};

void help_page(int argc, char **arg) {
    int max_len = 0;
    const int num_cmds = sizeof(COMMANDS) / sizeof(struct command);

    for (int i = 0; i < num_cmds; i++) {
        if (strlen(COMMANDS[i].name) > max_len)
            max_len = strlen(COMMANDS[i].name);
    }

    printf("Here are all possible commands that you could enter:\n");
    for (int i = 0; i < sizeof(COMMANDS) / sizeof(struct command); i++) {
        if (COMMANDS[i].name[0] == '\n')
            continue;

        // add appropriate amount of spacing to right align docs
        for (int n = 0; n < max_len - strlen(COMMANDS[i].name); n++)
            printf(" ");

        printf(" \033[1m%s\033[0m", COMMANDS[i].name);

        if (COMMANDS[i].docs != NULL)
            printf(" :: %s", COMMANDS[i].docs);

        printf("\n");
    }
}

// call the appropriate callback function for the command passed in.
void manage_commands(int argc, char** argv) {
    struct command const* cmd = COMMANDS;
    const size_t N_ELEM = sizeof(COMMANDS)/sizeof(struct command);

    while (strcmp((cmd)->name, argv[0]) != 0
       && (cmd - COMMANDS) < N_ELEM) cmd++;
    
    if (cmd - COMMANDS >= N_ELEM) {
    command_error(argc, argv);
    return;
    }

    cmd->callback(argc, argv);
}

void* read_msg_thread(void* arg) {
    struct message msg = {0};
    struct vector msg_buf;
    make_vector(&msg_buf, sizeof(char), 30);
    fcntl(g_server_sock, F_SETFL, O_NONBLOCK);
    
    while (g_run_program) {
        int status = message_recv(g_server_sock, &msg, &msg_buf);

    if (status != -1) {
        print_message(msg);
        free_message(msg);
    }
    }

    return NULL;
}

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
void* gfx_thread(void* arg) {
    SDL_Window* window = NULL;
    SDL_Surface* screenSurface = NULL;
    g_bg_color = 0xff;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    } else {
        window = SDL_CreateWindow("SDL Tutorial",
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SCREEN_WIDTH,
                                  SCREEN_HEIGHT,
                                  SDL_WINDOW_SHOWN);
        if (window == NULL) {
            printf("SDL could not create a window! SDL_Error: %s\n",
                   SDL_GetError());
        } else {
            screenSurface = SDL_GetWindowSurface(window);
            SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format,
                                                         g_bg_color, g_bg_color, g_bg_color));
            SDL_UpdateWindowSurface(window);
            SDL_Event e;

            while (g_run_program) {
                SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format,
                                                             g_bg_color, g_bg_color, g_bg_color));
                SDL_UpdateWindowSurface(window);
                while( SDL_PollEvent( &e ) ) {
                    if ( e.type == SDL_QUIT )
                        g_run_program = false;
                }
            }

            SDL_DestroyWindow(window);
            SDL_Quit();
        }}

    return NULL;
}

int main(int argc, char** argv) {
    // create a socket
    g_server_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (socket < 0) {
        perror("ERROR: failed to open network socket\n");
        exit(EXIT_FAILURE);
    }

    // give the socket a name
    struct sockaddr_in name =
        { .sin_family = AF_INET,
          .sin_addr = {inet_addr("127.0.0.1")},
          .sin_port = 4444};

    int status = connect(g_server_sock,
             (struct sockaddr*) &name,
             sizeof(name));
    if (status < 0) {
        perror("ERROR: couldn't connect to server.\n");
        exit(EXIT_FAILURE);
    }
    
    g_run_program = true;
    char* buff = malloc(50);
    size_t buff_size = 50;

    pthread_t read_msg_pid;
    pthread_create(&read_msg_pid,
           NULL,
           &read_msg_thread,
           NULL);

    pthread_t gfx_pid;
    pthread_create(&gfx_pid,
                   NULL,
                   &gfx_thread,
                   NULL);
    
    
    while (g_run_program) {
        printf("\n> ");
        fflush(stdout);

        getline(&buff, &buff_size, stdin);

        char* argv[10];
    int i = 0;
    for (char* token = strtok(buff, " \n");
         token != NULL;
         token = strtok(NULL, " \n")) {
        argv[i++] = token;
    }

    manage_commands(i, argv);
    }

    printf("quitting program...\n");
    exit(EXIT_SUCCESS);
}
