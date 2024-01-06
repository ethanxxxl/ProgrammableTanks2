#include <SDL2/SDL.h>
#include "tank.h"
#include "vector.h"
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
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
#include <message.h>

// TODO FIXME BUG program segfaults when you enter blank username.

/* GLOBAL VARIABLES
 */

pthread_t g_gfx_pid;
pthread_t g_read_msg_pid;

bool g_run_program;
bool g_server_connected;
int g_server_sock;

bool g_gfx_running;

char g_username[50];
bool g_print_msg;

struct player {
    char username[50];
    struct vector tanks;
};

struct vector g_players;

void players_update_player(char *username, struct vector *tank_positions) {
    // see if the player exists in the structure.
    struct player *player = NULL;
    for (size_t p = 0; p < g_players.len; p++) {
        player = vec_ref(&g_players, p);
        int status = strcmp(username, player->username);

        if (status == 0)
            goto update_player;
    }
    // emulated for-else syntax from python
    // this is the else case, we didn't find the player.
    struct player new_player;
    strncpy(new_player.username, username, 50);
    make_vector(&new_player.tanks, sizeof(struct tank), 30);

    vec_push(&g_players, &new_player);
    player = vec_ref(&g_players, g_players.len - 1);

update_player: ; // can't have a declaration after a label in cstd < c2x
    vec_resize(&player->tanks, tank_positions->len);

    for (size_t t = 0; t < tank_positions->len; t++) {
        struct coordinate coord;
        vec_at(tank_positions, t, &coord);

        struct tank *tank = vec_ref(&player->tanks, t);
        tank->x = coord.x;
        tank->y = coord.y;
    }
    
    return;
}

/*
** forward declarations
*/
void* read_msg_thread(void* arg);
void* gfx_thread(void* arg);

/*
 * Command Callback Functions
 */
int debug_send_msg(struct message msg) {
    if (!g_server_connected) {
        printf("ERROR! you must connect to the server first!\n");
        return -1;
    }
    printf("--SENDING--\n");
    print_message(msg);
    return message_send(g_server_sock, msg);
}

void enable_print_messages(int argc, char **argv) {
    (void)argc; (void)argv;

    g_print_msg = !g_print_msg;
}

void request_server_update(int argc, char **argv) {
    (void)argc; (void)argv;    
    printf("requesting info from server...\n");
    printf("COMMAND FAILED: NOT IMPLEMENTED\n");
}

void authenticate(int argc, char **argv) {
    (void)argc; (void)argv;

    if (argc != 2) {
        printf("ERROR: second argument must be your username.\n");
        return;
    }

    // copy username into global username tracker.
    memcpy(&g_username, argv[1], strlen(argv[1]));
    
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
        make_message(&msg, MSG_REQUEST_JOIN_SCENARIO);
    } else if (strcmp(argv[1], "lobby") == 0) {
        make_message(&msg, MSG_REQUEST_RETURN_TO_LOBBY);
    } else {
        printf("ERROR: valid options are \"scene\" or \"lobby\"\n");
    return;
    }

    debug_send_msg(msg);
    return;
}
void list_scenarios(int argc, char **argv) {
    (void)argc; (void)argv;

    struct message msg;
    make_message(&msg, MSG_REQUEST_LIST_SCENARIOS);

    debug_send_msg(msg);
}

void update_tank(int argc, char **argv) {
    (void)argc; (void)argv;
    if (argc < 3) {
        printf("ERROR: arguments must be: update-tank: IDX X Y\n");
        return;
    }

    // find yourself in player list
    struct player player;
    for (size_t p = 0; p < g_players.len; p++) {
        vec_at(&g_players, p, &player);
        if (strcmp(player.username, g_username) == 0)
            goto update_tank_send_update;
    }
    printf("ERROR: you don't have any tanks to update!\n");
    return;
    
 update_tank_send_update:;
    int index = atoi(argv[1]);
    int x = atoi(argv[2]);
    int y = atoi(argv[3]);

    if (index < 0 || index >= (int)player.tanks.len) {
        printf("ERROR: you must index a valid tank!\n");
        return;
    }
    
    struct tank* tank = vec_ref(&player.tanks, index);
    tank->move_to_x = x;
    tank->move_to_y = y;
    return;
 }

void propose_update(int argc, char **argv) {
    (void)argc;
    (void)argv;

    // find yourself in player list
    struct player player;
    for (size_t p = 0; p < g_players.len; p++) {
        vec_at(&g_players, p, &player);
        if (strcmp(player.username, g_username) == 0)
            goto propose_update_tank_send_update;
    }
    printf("ERROR: you don't have any tanks to update!\n");
    return;

 propose_update_tank_send_update:;
    struct message msg;
    make_message(&msg, MSG_REQUEST_PLAYER_UPDATE);

    for (size_t t = 0; t < player.tanks.len; t++) {
        struct tank tank;
        vec_at(&player.tanks, t, &tank);
        
        enum tank_command cmd = TANK_MOVE;
        struct coordinate coord = { .x = tank.move_to_x, .y = tank.move_to_y };
           
        vec_push(&msg.player_update.tank_instructions, &cmd);
        vec_push(&msg.player_update.tank_target_coords, &coord);
        vec_push(&msg.player_update.tank_position_coords, &coord);
    }

    debug_send_msg(msg);

    free_message(msg);
    return;
}

void list_tanks(int argc, char **argv) {
    (void)argc; (void)argv;    
    for (size_t p = 0; p < g_players.len; p++) {
        struct player *player = vec_ref(&g_players, p);
        printf("[tanks for %s]\n", player->username);
        for (size_t t = 0; t < player->tanks.len; t++) {
            struct tank *tank = vec_ref(&player->tanks, t);
            printf("  [%zu] x: %d y: %d\n", t, tank->x, tank->y);
        }
    }
}

void help_page(int argc, char** argv);

void message_server(int argc, char **argv) {
    struct message msg;
    make_message(&msg, MSG_REQUEST_DEBUG);
    make_vector(&msg.text, sizeof(char), 50);

    for (int i = 1; i < argc; i++) {
        vec_pushn(&msg.text, argv[i], strlen(argv[i]));
        vec_push(&msg.text, " ");
    }
    vec_push(&msg.text, "\0");

    debug_send_msg(msg);
}
void command_error(int argc, char **argv) {
    (void)argc; (void)argv;

    printf("ERROR: Unknown Command \"%s\"\n", argv[0]);
}
void quit(int argc, char **argv) {
    (void)argc; (void)argv;    
    g_run_program = false;
}

void dummy(int argc, char **argv) {
    (void)argc; (void)argv;    
}

int g_bg_color;
void change_bg_color(int argc, char **argv) {
    if (argc != 2) {
        printf("ERROR: there may only be a single argument to this function\n");
        return;
    }

    g_bg_color = atoi(argv[1]);
}

void connect_serv(int argc, char **argv) {
    if (argc > 2) {
        printf("ERROR: there shouldn't be more than one parameter!\n");
        return;
    }

    char default_addr[] = "127.0.0.1:4444";
    char* address_port = default_addr;

    if (argc == 2) {
        address_port = argv[1];
    }

    char* address = strtok(address_port, ":");
    char* port_str = strtok(NULL, ":");
    int port = atoi(port_str);

        // create a socket
    g_server_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (g_server_sock < 0) {
        printf("ERROR: failed to open network socket\n");
        return;
    }

    // give the socket a name
    struct sockaddr_in name =
        { .sin_family = AF_INET,
          .sin_addr = {inet_addr(address)},
          .sin_port = port};

    int status = connect(g_server_sock,
             (struct sockaddr*) &name,
             sizeof(name));
    if (status < 0) {
        printf("ERROR: couldn't connect to server.\n");
        return;
    }

    // start the message reader thread
    pthread_create(&g_read_msg_pid,
           NULL,
           &read_msg_thread,
           NULL);

    g_server_connected = true;
    printf("connected to server on %s:%d\n", address, port);
    return;
}

void start_gfx(int argc, char **argv) {
    (void)argc; (void)argv;    
    pthread_create(&g_gfx_pid,
                   NULL,
                   &gfx_thread,
                   NULL);
    
    g_gfx_running = true;
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
    {"connect", &connect_serv,
     "connect to a server. For debugging purposes, an argument specifying the address \
and port is optional. (default is 127.0.0.1:4444)"},
    {"start-gfx", &start_gfx,
     "starts the SDL2 GUI."},
    {"list-tanks", &list_tanks, "lists tanks for each player currently connected"},
    {"update", &propose_update,
     "NOT IMPLEMENTED"},
    {"update-tank", &update_tank,
     "give the tank index, followed by an x and y coordinate, and that tank will move to that location."},
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
     "sends the server a DEBUG_MESSAGE. sending a DEBUG_MESSAGE with the text \
'kill-serv' will cause the server to quit."},

    {"color", &change_bg_color,
     "changes the bg color of the SDL graphics window."},

    {"debug-messages", &enable_print_messages,
     "toggles whether or not to print messages recieved from server."},

    {"\n", &dummy, NULL},
    {"q", &quit, "quit this client"},
    {"quit", &quit, "quit this client"},
    {"exit", &quit, "quit this client"},
};

void help_page(int argc, char **argv) {
    (void)argc; (void)argv;    
    size_t max_len = 0;
    const size_t num_cmds = sizeof(COMMANDS) / sizeof(struct command);

    for (size_t i = 0; i < num_cmds; i++) {
        if (strlen(COMMANDS[i].name) > max_len)
            max_len = strlen(COMMANDS[i].name);
    }

    printf("Here are all possible commands that you could enter:\n");
    for (size_t i = 0; i < num_cmds; i++) {
        if (COMMANDS[i].name[0] == '\n')
            continue;

        // add appropriate amount of spacing to right align docs
        for (size_t n = 0; n < max_len - strlen(COMMANDS[i].name); n++)
            printf(" ");

        printf(" \033[1m%s\033[0m", COMMANDS[i].name);

        if (COMMANDS[i].docs != NULL)
            printf(" :: %s", COMMANDS[i].docs);

        printf("\n");
    }
}

// call the appropriate callback function for the command passed in.
void manage_commands(int argc, char **argv) {
    struct command const *cmd = COMMANDS;
    const size_t N_ELEM = sizeof(COMMANDS)/sizeof(struct command);

    if (argc <= 0)
        return;

    while (strcmp((cmd)->name, argv[0]) != 0
           && (size_t)(cmd - COMMANDS) < N_ELEM) cmd++;
    
    if ((size_t)(cmd - COMMANDS) >= N_ELEM) {
        command_error(argc, argv);
        return;
    }

    cmd->callback(argc, argv);
}

void *read_msg_thread(void *arg) {
    (void)arg; // arg is unused.

    g_print_msg = false;
    
    struct message msg = {0};
    struct vector msg_buf;
    make_vector(&msg_buf, sizeof(char), 30);
    fcntl(g_server_sock, F_SETFL, O_NONBLOCK);

    while (g_run_program) {
        if (!g_server_connected)
            continue;

        int status = message_recv(g_server_sock, &msg, &msg_buf);

        if (status < 0)
            continue;

        switch (msg.type) {
        case MSG_RESPONSE_SCENARIO_TICK: {

            struct scenario_tick body = msg.scenario_tick;
            for (size_t u = 0; u < body.username_vecs.len; u++) {
                char *username =
                    ((struct vector *)vec_ref(&body.username_vecs, u))->data;

                struct vector *tanks = vec_ref(&body.tank_positions, u);
               
                players_update_player(username, tanks);
            }
        } break;            
        default:
            break;
        }

        if (g_print_msg) {
            print_message(msg);
        }
        
        free_message(msg);
    }

    
    free_vector(&msg_buf);
    return NULL;
}

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

int g_map_height;
int g_map_width;
/// Draws the grid with dimensions on the surface, but only the portion that cam
/// is viewing.
void gfx_render_rect(SDL_Renderer* renderer, SDL_Rect* cam, SDL_Rect* rect) {
    int l_edge = rect->x - cam->x;
    int t_edge = rect->y - cam->y;
    int r_edge = (rect->x + rect->w) - cam->x;
    int b_edge = (rect->y + rect->h) - cam->y;

    if (l_edge < 0) l_edge = 0;            // | bounds checking
    if (t_edge < 0) t_edge = 0;            // |
    if (r_edge > cam->w) r_edge = cam->w;  // |
    if (b_edge > cam->h) b_edge = cam->h;  // |

    SDL_Rect the_thing = {
        .x = l_edge,
        .y = t_edge,
        .w = r_edge - l_edge,
        .h = b_edge - t_edge
    };

    SDL_RenderFillRect(renderer, &the_thing);
}

void gfx_render_grid(SDL_Renderer* renderer, SDL_Rect* cam, const SDL_Rect* tile,
                     int spacing, int rows, int cols, uint8_t fg[3], uint8_t bg[3]) {
    SDL_Rect drawn_tile = *tile;
    SDL_Rect canvas = {
        .x = drawn_tile.x - 5,
        .y = drawn_tile.y - 5,
        .w = cols * (drawn_tile.w + spacing) + 10,
        .h = rows * (drawn_tile.h + spacing) + 10
    };

    // clear the ground first
    SDL_SetRenderDrawColor(renderer, bg[0], bg[1], bg[2], SDL_ALPHA_OPAQUE);
    gfx_render_rect(renderer, cam, &canvas);

    // now draw the grid
    SDL_SetRenderDrawColor(renderer, fg[0], fg[1], fg[2], SDL_ALPHA_OPAQUE);
    for (int x = 0; x < cols; x++) {
        for (int y = 0; y < rows; y++) {
            gfx_render_rect(renderer, cam, &drawn_tile);

            drawn_tile.y += drawn_tile.h + spacing;
        }

        drawn_tile.y = canvas.y + 5;
        drawn_tile.x += drawn_tile.w + spacing;
    }
}

void gfx_draw_tank(SDL_Renderer *renderer, SDL_Rect *cam, const SDL_Rect *tile,
                   int spacing, int x, int y) {
    SDL_Rect tank_tile = {
        .x = x * (tile->w + spacing),
        .y = y * (tile->h + spacing),
        .w = tile->w,
        .h = tile->h
    };

    // TODO: Render different color for each player
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    gfx_render_rect(renderer, cam, &tank_tile);    
}

void *gfx_thread(void *arg) {
    (void)arg;
    
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    g_bg_color = 0xff;
    g_map_height = 50;
    g_map_width = 50;

    /*
    ** INITIALIZE SDL
     */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize!\n");
        printf("SDL_Error: %s\n", SDL_GetError());
        return NULL;
    };

    window = SDL_CreateWindow("SDL Tutorial",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              SCREEN_WIDTH,
                              SCREEN_HEIGHT,
                              SDL_WINDOW_SHOWN);

    if (window == NULL) {
        printf("SDL could not create a window!\n");
        printf("SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return NULL;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("SDL could not create a renderer!\n");
        printf("SLD_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return NULL;
    }

    /*
    ** LOAD MEDIA
     */
    uint8_t bg_colors[3] = {47, 196, 97};
    uint8_t fg_colors[3] = {88, 119, 140};

    /*
    ** RUN GFX LOOP
     */
    SDL_Rect camera = { .x = 0, .y = 0, .h = SCREEN_HEIGHT, .w = SCREEN_WIDTH};
    SDL_Rect tile = {.x = 0, .y=0, .h = 5, .w = 5};

    SDL_Event e;
    g_gfx_running = true;
    while (g_gfx_running) {
        SDL_SetRenderDrawColor(renderer, g_bg_color, g_bg_color, g_bg_color,
                               SDL_ALPHA_OPAQUE);

        SDL_RenderClear(renderer);
        const int grid_spacing = 1;
        gfx_render_grid(renderer, &camera, &tile, grid_spacing,
                        g_map_height, g_map_width, bg_colors, fg_colors);

        for (size_t p = 0; p < g_players.len; p++) {
            struct player *player = vec_ref(&g_players, p);

            for (size_t t = 0; t < player->tanks.len; t++) {
                struct tank tank;
                vec_at(&player->tanks, t, &tank);
                gfx_draw_tank(renderer, &camera, &tile, grid_spacing, tank.x, tank.y);
            }
        }
        
        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_MOUSEMOTION:
                // see if the user is panning
                if ((e.motion.state & SDL_BUTTON_LMASK) != SDL_BUTTON_LMASK)
                    break;

                camera.x -= e.motion.xrel;
                camera.y -= e.motion.yrel;
                break;
            case SDL_MOUSEWHEEL:
                tile.h += e.wheel.y;
                tile.w += e.wheel.y;
                break;
            case SDL_QUIT:
                g_gfx_running = false;
                break;
            default:
                break;
            }
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return NULL;
}

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

    make_vector(&g_players, sizeof(struct player), 5);
    
    char* buff = malloc(50);
    size_t buff_size = 50;

    puts(g_welcome_message);
    
    while (g_run_program) {
        printf("> ");
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

    if (g_gfx_pid != 0)
        pthread_join(g_gfx_pid, NULL);

    if (g_read_msg_pid != 0)
        pthread_join(g_read_msg_pid, NULL);

    exit(EXIT_SUCCESS);
}
