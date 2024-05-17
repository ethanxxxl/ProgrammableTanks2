#include "client-commands.h"
#include "client-gfx.h"
#include "game-manager.h"

#include "message.h"
#include "vector.h"
#include "scenario.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>

extern bool g_run_program;
extern bool g_server_connected;
extern int g_server_sock;

extern bool g_gfx_running;

extern char g_username[50];
extern bool g_print_msg;


extern int g_bg_color;
extern pthread_t g_gfx_pid;
extern pthread_t g_read_msg_pid;

extern struct vector *g_players;

// FIXME I don't think that this is the best place for this function.
void *read_msg_thread(void *arg) {
    (void)arg; // arg is unused.

    g_print_msg = false;
    
    struct message msg = {0};
    struct vector* msg_buf = make_vector(sizeof(char), 30);
    fcntl(g_server_sock, F_SETFL, O_NONBLOCK);

    while (g_run_program) {
        if (!g_server_connected)
            continue;

        int status = message_recv(g_server_sock, &msg, msg_buf);

        if (status < 0)
            continue;

        switch (msg.type) {
        case MSG_RESPONSE_SCENARIO_TICK: {

            struct scenario_tick body = msg.scenario_tick;
            for (size_t u = 0; u < vec_len(body.username_vecs); u++) {
                char *username =
                    vec_dat(((struct vector *)vec_ref(body.username_vecs, u)));

                struct vector *tanks = vec_ref(body.tank_positions, u);
               
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

    
    free_vector(msg_buf);
    return NULL;
}

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
    vec_pushn(msg.user_credentials.username, argv[1], strlen(argv[1]));
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
    for (size_t p = 0; p < vec_len(g_players); p++) {
        vec_at(g_players, p, &player);
        if (strcmp(player.username, g_username) == 0)
            goto update_tank_send_update;
    }
    printf("ERROR: you don't have any tanks to update!\n");
    return;
    
 update_tank_send_update:;
    int index = atoi(argv[1]);
    int x = atoi(argv[2]);
    int y = atoi(argv[3]);

    if (index < 0 || index >= (int)vec_len(player.tanks)) {
        printf("ERROR: you must index a valid tank!\n");
        return;
    }
    
    struct tank* tank = vec_ref(player.tanks, index);
    tank->move_to_x = x;
    tank->move_to_y = y;
    return;
 }

void propose_update(int argc, char **argv) {
    (void)argc;
    (void)argv;

    // find yourself in player list
    struct player player;
    for (size_t p = 0; p < vec_len(g_players); p++) {
        vec_at(g_players, p, &player);
        if (strcmp(player.username, g_username) == 0)
            goto propose_update_tank_send_update;
    }
    printf("ERROR: you don't have any tanks to update!\n");
    return;

 propose_update_tank_send_update:;
    struct message msg;
    make_message(&msg, MSG_REQUEST_PLAYER_UPDATE);

    for (size_t t = 0; t < vec_len(player.tanks); t++) {
        struct tank tank;
        vec_at(player.tanks, t, &tank);
        
        enum tank_command cmd = TANK_MOVE;
        struct coordinate coord = { .x = tank.move_to_x, .y = tank.move_to_y };
           
        vec_push(msg.player_update.tank_instructions, &cmd);
        vec_push(msg.player_update.tank_target_coords, &coord);
        vec_push(msg.player_update.tank_position_coords, &coord);
    }

    debug_send_msg(msg);

    free_message(msg);
    return;
}

void list_tanks(int argc, char **argv) {
    (void)argc; (void)argv;    
    for (size_t p = 0; p < vec_len(g_players); p++) {
        struct player *player = vec_ref(g_players, p);
        printf("[tanks for %s]\n", player->username);
        for (size_t t = 0; t < vec_len(player->tanks); t++) {
            struct tank *tank = vec_ref(player->tanks, t);
            printf("  [%zu] x: %d y: %d\n", t, tank->x, tank->y);
        }
    }
}

void message_server(int argc, char **argv) {
    struct message msg;
    make_message(&msg, MSG_REQUEST_DEBUG);
    msg.text = make_vector(sizeof(char), 50);

    for (int i = 1; i < argc; i++) {
        vec_pushn(msg.text, argv[i], strlen(argv[i]));
        vec_push(msg.text, " ");
    }
    vec_push(msg.text, "\0");

    debug_send_msg(msg);
}

void quit(int argc, char **argv) {
    (void)argc; (void)argv;    
    g_run_program = false;
}

void dummy(int argc, char **argv) {
    (void)argc; (void)argv;    
}

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
