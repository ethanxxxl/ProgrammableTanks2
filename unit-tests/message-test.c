#include "scenario.h"
#include "server-scenario.h"
#include "unit-test.h"
#include "message.h"
#include "vector.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>


// HACK these are externs referenced in scenario.c which are defined in the
// server translation unit. In order for the tests to compile, they need to
// provide a defnition for this global variable.
extern struct scenario g_scenario;
struct scenario g_scenario;

// data-carrying message types.
struct result_void tst_text_msg_serde(void) {
    return fail_msg(TEST_NOT_IMPLEMENTED_ERROR);
}
struct result_void tst_user_credentials_serde(void) {
    return fail_msg(TEST_NOT_IMPLEMENTED_ERROR);
}
struct result_void tst_player_update_serde(void) {
    char* error_msg = NULL;

    vector* commands = make_vector(sizeof(enum tank_command), 30);
    vector* targets = make_vector(sizeof(struct coord), 30);

    for (size_t i = 0; i < 30; i++) {
        enum tank_command tc = TANK_FIRE;
        struct coord pos = {i, 5};

        vec_push(commands, &tc);
        vec_push(targets, &pos);
    }

    struct sexp out_msg;
    make_message(&out_msg, MSG_REQUEST_PLAYER_UPDATE);

    out_msg.player_update.tank_instructions = commands;
    out_msg.player_update.tank_target_coords = targets;

    int fd[2];
    if (pipe(fd) != 0) {
        error_msg = "couldn't create pipe to sim net traffic";
        goto free_out_msg;
    }

    message_send(fd[1], out_msg);
    close(fd[1]);

    struct message in_msg;
    struct vector* tmp_buf = make_vector(sizeof(u8), 10);
    while (message_recv(fd[0], &in_msg, tmp_buf) != 0);
    free_vector(tmp_buf);

    if (in_msg.type != MSG_REQUEST_PLAYER_UPDATE) {
        error_msg = "message is the incorrect type";
        goto free_in_msg;
    }

    vector* in_commands = in_msg.player_update.tank_instructions;
    vector* in_targets = in_msg.player_update.tank_target_coords;
    
    if (vec_len(in_commands) < vec_len(commands)) {
        error_msg = "received less commands than sent";
        goto free_in_msg;
    } else if (vec_len(in_commands) > vec_len(commands)) {
        error_msg = "received more commands than sent";
        goto free_in_msg;        
    }

    if (memcmp(vec_dat(in_commands),
               vec_dat(commands),
               vec_len(commands)) != 0) {
        error_msg = "commands are not the same";
        goto free_in_msg;
    }

    if (vec_len(in_targets) < vec_len(targets)) {
        error_msg = "received less targets than sent";
        goto free_in_msg;
    } else if (vec_len(in_targets) > vec_len(targets)) {
        error_msg = "received more targets than sent";
        goto free_in_msg;        
    }

    if (memcmp(vec_dat(in_targets),
               vec_dat(targets),
               vec_len(targets)) != 0) {
        error_msg = "targets are not the same";
        goto free_in_msg;
    }

 free_in_msg:
    free_message(in_msg);

 free_out_msg:
    free_message(out_msg);
    
    return error_msg;
 }
 
struct result_void tst_scenario_tick_serde(void) {
    char* error_msg = NULL;
    
    struct vector* player_data = make_vector(sizeof(struct player_data), 10);
    // create 5 players...
    char* usernames[5] = {
        "WhimsyW4rLord88",
        "MindM4ge202",
        "StratS0rc3rer5",
        "R1ddleRebel13",
        "Cr4ftyConjurer11"
    };
    
    for (int p = 0; p < 5; p++) {
        struct player_data pd = make_player_data();

        vec_pushn(pd.username, usernames[p], strlen(usernames[p]));
        
        for (int t = 0; t < 35; t++) {
            struct tank tank = {
                .cmd = TANK_MOVE,
                .move_to = {t,p},
                .health = 100,
                .pos = {t, p},
                .aim_at = {0,0}
            };

            vec_push(pd.tanks, &tank);
        }

        vec_push(player_data, &pd);
    }

    // get public data
    vector* public_data = make_vector(sizeof(struct player_public_data), 10);

    for (struct player_data* pd = vec_dat(player_data);
         pd <= (struct player_data*)vec_last
             (player_data);
         pd++) {
        struct player_public_data pub_dat = player_public_data_get(pd);
        vec_push(public_data, &pub_dat);
    }

    struct scenario_tick tick = {
        .players_public_data = public_data,
    };

    struct sexp_dyn *out_msg = make_scenario_tick_message(&tick);

    int fd[2];
    if (pipe(fd) == -1) {
        error_msg = "couldn't create pipe.";
        goto free_out_msg;
    }
    fcntl(fd[0], F_SETFL, O_NONBLOCK);

    message_send(fd[1], out_msg);
    close(fd[1]);
    
    struct message in_msg;
    struct vector* tmp_buf = make_vector(sizeof(u8), 10);
    
    while (message_recv(fd[0], &in_msg, tmp_buf) != 0);
    free_vector(tmp_buf);
    close(fd[0]);

    if (in_msg.type != MSG_RESPONSE_SCENARIO_TICK) {
        error_msg = "message type is not MSG_RESPONSE_SCENARIO_TICK";
        goto free_in_msg;
    }

    vector* in_public_data = in_msg.scenario_tick.players_public_data;
    size_t in_bytes = vec_len(in_public_data) * vec_element_len(in_public_data);
    size_t out_bytes = vec_len(public_data) * vec_element_len(public_data);

    if (in_bytes > out_bytes) {
        error_msg = "received more data than was sent.";
        goto free_in_msg;
    } else if (in_bytes < out_bytes) {
        error_msg = "received less data than was sent.";
        goto free_in_msg;
    }

    for (size_t p = 0; p < vec_len(public_data); p++) {
        struct player_public_data* sent = vec_ref(public_data, p);
        struct player_public_data* recvd = vec_ref(in_public_data, p);

        if (vec_len(sent->username) != vec_len(recvd->username)) {
            error_msg = "usernames have different lengths";
            goto free_in_msg;
        }

        if (memcmp(vec_dat(sent->username),
                   vec_dat(recvd->username),
                   vec_len(sent->username)) != 0) {
            error_msg = "usernames are not the same";
            goto free_in_msg;
        }

        if (vec_len(sent->tank_positions) != vec_len(recvd->tank_positions)) {
            error_msg = "tanks positoins are different lengths";
            goto free_in_msg;
        }

        if (memcmp(vec_dat(sent->tank_positions),
                   vec_dat(recvd->tank_positions),
                   vec_len(sent->tank_positions)) != 0) {
            error_msg = "tank positions are not the same";
            goto free_in_msg;
        }
            
    }

    //
    // free malloced memory
    //
 free_in_msg:
    free_message(in_msg);
    
 free_out_msg:
    free_message(out_msg);
    
    for (struct player_data* pd = vec_dat(player_data);
         pd <= (struct player_data*)vec_end(player_data);
         pd++)
        free_player_data(pd);
    free_vector(player_data);

    return error_msg;
}

// TODO make tests for the rest of the message types.

struct test g_all_tests[] = {
    {"serialization: text", &tst_text_msg_serde},
    {"serialization: user credentials", &tst_user_credentials_serde},
    {"serialization: player update", &tst_player_update_serde},
    {"serialization: scenario tick", &tst_scenario_tick_serde},
};

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    size_t num_tests = sizeof(g_all_tests)/sizeof(struct test);
    const char header[] = "message test";
    run_test_suite(g_all_tests, num_tests, header);
    
    return 0;
}
