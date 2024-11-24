#include "client-commands.h"
#include "client-gfx.h"
#include "error.h"
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

    struct error err;
    g_print_msg = false;
    
    sexp *msg = NULL;
    struct vector* msg_buf = make_vector(sizeof(char), 30);
    fcntl(g_server_sock, F_SETFL, O_NONBLOCK);

    while (g_run_program) {
        if (!g_server_connected)
            continue;

        struct result_sexp r = message_recv(g_server_sock, msg_buf);

        // handle any errors that may have occured during parsing, etc.
        if (r.status == RESULT_ERROR) {
            err = r.error;
            goto handle_error;
        } else {
            msg = r.ok;
        }

        // no message was received, continue to wait for a message.
        if (msg == NULL)
            continue;

        // a message was received, handle the message.
        switch (message_get_type(msg)) {
        case MSG_RESPONSE_SCENARIO_TICK: {
            struct scenario_tick tick;
            struct result_scenario_tick r = unwrap_scenario_tick_message(msg);

            // free resources and restart loop if error occured.
            if (r.status == RESULT_ERROR) {
                err = r.error;
                free_sexp(msg);
                goto handle_error;
            } else {
                tick = r.ok;
            }

            struct player_public_data* pd = vec_dat(tick.players_public_data);
            struct player_public_data* end = vec_last(tick.players_public_data);
            for (; pd <= end; pd++) {
                players_update_player(vec_dat(pd->username), pd->tank_positions);
            }

            free_scenario_tick(tick);
        } break;            
        default:
            break;
        }

        if (g_print_msg) {
            sexp_print(msg);
        }
        
        free_sexp(msg);

    handle_error:
        char *err_msg = describe_error(err);
        puts(err_msg);
        free(err_msg);
        free_error(err);
        continue;
    }
        
    free_vector(msg_buf);
    return NULL;
}

struct result_s32 debug_send_msg(sexp *msg) {
    if (!g_server_connected)
        RESULT_MSG_ERROR(s32, "ERROR! you must connect to the server first!\n");

    printf("--SENDING--\n");
    printf("sending message: ");
    sexp_println(msg);

    struct result_s32 r = message_send(g_server_sock, msg);
    if (r.status == RESULT_OK)
        printf("message sent.\n");
    else
        printf("message not sent.\n");
    
    return r;
}

void enable_print_messages(int argc, char **argv, struct error *e) {
    (void)argc; (void)argv; (void)e;

    g_print_msg = !g_print_msg;
}

void request_server_update(int argc, char **argv, struct error *e) {
    (void)argc; (void)argv; (void)e; (void)e;
    printf("requesting info from server...\n");

    *e = make_msg_error("Not Implemented");
}

void authenticate(int argc, char **argv, struct error *e) {
    (void)argc; (void)argv; (void)e;

    if (argc != 2) {
        *e = make_msg_error("ERROR: second argument must be your username.\n");
        return;
    }

    // copy username into global username tracker.
    memcpy(&g_username, argv[1], strlen(argv[1]));

    const char *username = argv[1];
    const char *password = NULL;
    struct result_sexp msg =
        make_user_credentials_message_str(username, password);

    if (msg.status == RESULT_ERROR) {
        *e = msg.error;
        return;
    }

    debug_send_msg(msg.ok);
    free_sexp(msg.ok);
}

void change_state(int argc, char **argv, struct error *e) {
    if (argc != 2) {
        *e = make_msg_error("ERROR: valid options are \"scene\" or \"lobby\"\n");
        return;
    }

    struct result_sexp msg;
    if (strcmp(argv[1], "scene") == 0) {
        msg = make_join_scenario_message("default");
    } else if (strcmp(argv[1], "lobby") == 0) {
        msg = make_return_to_lobby_message();
    } else {
        *e = make_msg_error("ERROR: valid options are \"scene\" or \"lobby\"\n");
        return;
    }

    if (msg.status == RESULT_ERROR) {
        *e = msg.error;
        return;
    }
    
    debug_send_msg(msg.ok);
    free_sexp(msg.ok);
    return;
}
void list_scenarios(int argc, char **argv, struct error *e) {
    (void)argc; (void)argv;

    struct result_sexp msg;
    msg = make_list_scenarios_message();
    if (msg.status == RESULT_ERROR) {
        *e = msg.error;
        return;
    }

    debug_send_msg(msg.ok);
    free_sexp(msg.ok);
    return;
}

void update_tank(int argc, char **argv, struct error *e) {
    (void)argc; (void)argv; (void)e;
    if (argc < 3) {
        *e = make_msg_error("ERROR: arguments must be: update-tank: IDX X Y\n");
        return;
    }

    // find yourself in player list
    struct player player;
    for (size_t p = 0; p < vec_len(g_players); p++) {
        vec_at(g_players, p, &player);
        if (strcmp(player.username, g_username) == 0)
            goto update_tank_send_update;
    }
    *e = make_msg_error("ERROR: you don't have any tanks to update!\n");
    return;
    
 update_tank_send_update:;
    int index = atoi(argv[1]);
    int x = atoi(argv[2]);
    int y = atoi(argv[3]);

    if (index < 0 || index >= (int)vec_len(player.tanks)) {
        *e = make_msg_error("ERROR: you must index a valid tank!\n");
        return;
    }
    
    struct tank* tank = vec_ref(player.tanks, index);
    tank->move_to.x = x;
    tank->move_to.y = y;
    return;
 }

void propose_update(int argc, char **argv, struct error *e) {
    (void)argc; (void)e;
    (void)argv;

    // find yourself in player list
    struct player player;
    for (size_t p = 0; p < vec_len(g_players); p++) {
        vec_at(g_players, p, &player);
        if (strcmp(player.username, g_username) == 0)
            goto propose_update_tank_send_update;
    }
    *e = make_msg_error("ERROR: you don't have any tanks to update!\n");
    return;

 propose_update_tank_send_update:
    // FIXME make_vector could fail here.
    struct player_update update = {
        .tank_instructions = make_vector(sizeof(enum tank_command),
                                         vec_len(player.tanks)),
        .tank_target_coords = make_vector(sizeof(struct coord),
                                          vec_len(player.tanks)),
    };
    
    for (size_t t = 0; t < vec_len(player.tanks); t++) {
        struct tank tank;
        vec_at(player.tanks, t, &tank);

        enum tank_command cmd = TANK_MOVE;

        vec_push(update.tank_instructions, &cmd);
        vec_push(update.tank_target_coords, &tank.move_to);
    }

    struct result_sexp msg;
    msg = make_player_update_message(&update);

    if (msg.status == RESULT_ERROR) {
        *e = msg.error;
        free_vector(update.tank_instructions);
        free_vector(update.tank_target_coords);
        return;
    }

    debug_send_msg(msg.ok);
    free_sexp(msg.ok);
    return;
}

void list_tanks(int argc, char **argv, struct error *e) {
    (void)argc; (void)argv;     (void)e;
    for (size_t p = 0; p < vec_len(g_players); p++) {
        struct player *player = vec_ref(g_players, p);
        printf("[tanks for %s]\n", player->username);
        for (size_t t = 0; t < vec_len(player->tanks); t++) {
            struct tank *tank = vec_ref(player->tanks, t);
            printf("  [%zu] x: %d y: %d\n", t, tank->pos.x, tank->pos.y);
        }
    }
}

void message_server(int argc, char **argv, struct error *e) {
    vector *txt = make_vector(sizeof(char), 50);
    for (int i = 1; i < argc; i++) {
        vec_pushn(txt, argv[i], strlen(argv[i]));
        vec_push(txt, " ");
    }
    vec_push(txt, "\0");

    struct result_sexp msg;
    msg = make_text_message(vec_dat(txt));
    free_vector(txt);
    
    if (msg.status == RESULT_ERROR) {
        *e = msg.error;
        return;
    }
    
    debug_send_msg(msg.ok);
    free_sexp(msg.ok);
    return;
}

void quit(int argc, char **argv, struct error *e) {
    (void)argc; (void)argv; (void)e;
    g_run_program = false;
}

void dummy(int argc, char **argv, struct error *e) {
    (void)argc; (void)argv; (void)e;
}

void change_bg_color(int argc, char **argv, struct error *e) {
    if (argc != 2) {
        *e = make_msg_error("ERROR: there may only be a single argument to this function\n");
        return;
    }

    g_bg_color = atoi(argv[1]);
}

void connect_serv(int argc, char **argv, struct error *e) {
    if (argc > 2) { 
        *e = make_msg_error("ERROR: there shouldn't be more than one parameter!\n");
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
        *e = make_msg_error("ERROR: failed to open network socket\n");
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
        *e = make_msg_error("ERROR: couldn't connect to server.\n");
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

void start_gfx(int argc, char **argv, struct error *e) {
    (void)argc; (void)argv; (void)e;
    pthread_create(&g_gfx_pid,
                   NULL,
                   &gfx_thread,
                   NULL);
    
    g_gfx_running = true;
}
