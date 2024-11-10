#include "player_manager.h"
#include "error.h"
#include "server-scenario.h"

#include "scenario.h"
#include "message.h"
#include "sexp/sexp-base.h"

#include <player_manager.h>
#include <stdio.h>
#include <string.h>


extern struct scenario g_scenario;

int make_player(struct player_manager *p) {
    (void)p; // we don't need to allocate anything currently.
    return 0;
}

struct result_void player_idle_handler(struct player_manager *p, sexp *msg) {
    switch (message_get_type(msg)) {
    case MSG_REQUEST_AUTHENTICATE: {
        struct user_credentials user_credentials =
            unwrap_user_credentials_message(msg);
        
        strcpy(p->username, vec_dat(user_credentials.username));
        p->state = STATE_LOBBY; // FIXME: no authentication done here!
        printf("%s: authenticated\n", p->username);
        printf("%s: authenticated (msg data)\n",
               (char*)vec_dat(user_credentials.username));
        
        {
            char buf[50] = {0};
            int ret =
                snprintf(buf, 50, "authenticated %s.", p->username);
            if (ret < 0)
                return RESULT_MSG_ERROR(void, "Name was too large for the buffer");

            struct result_s32 r =
                message_status_send(p->socket, MESSAGE_STATUS_SUCCESS, NULL);
            if (r.status == RESULT_ERROR) return result_void_error(r.error);
        }

        break;
    }
    default: {
        struct result_s32 r = message_status_send(p->socket, MESSAGE_STATUS_FAIL,
                            "you must be authenticated first");
        if (r.status == RESULT_ERROR) return result_void_error(r.error);
        break;
    }
    }

    return result_void_ok(0);
}

struct result_void player_lobby_handler(struct player_manager *p, sexp *msg) {
    struct result_s32 r;
    
    switch (message_get_type(msg)) {
    case MSG_REQUEST_LIST_SCENARIOS:
        r = message_status_send(p->socket, MESSAGE_STATUS_SUCCESS,
                                "There is only one scenario (0)");
        break;
    case MSG_REQUEST_CREATE_SCENARIO:
        break;
        
    case MSG_REQUEST_JOIN_SCENARIO:
        p->state = STATE_SCENARIO;

        scenario_add_player(&g_scenario, p);

        r = message_status_send(p->socket, MESSAGE_STATUS_SUCCESS,
                                "entering scenario...");
        break;
    default:
        r = message_status_send(p->socket, MESSAGE_STATUS_INVALID_MESSAGE,
                          "not supported in lobby.");
        break;
    }

    if (r.status == RESULT_ERROR)
        return result_void_error(r.error);
    else
        return result_void_ok(0);
}

struct result_void player_scenario_handler(struct player_manager *p, sexp *msg) {
    struct result_s32 r;
    switch (message_get_type(msg)) {
    case MSG_REQUEST_RETURN_TO_LOBBY:
        // FIXME: there should be some limitations on when a player can exit a
        // scenario. don't want a player to be able to leave in the heat of
        // battle. At the very least, some logging should be done to track bad
        // behavior.
        
        p->state = STATE_LOBBY;
        scenario_rem_player(&g_scenario, p);

        // TODO: remove player from global scenario.
        
        r = message_status_send(p->socket, MESSAGE_STATUS_SUCCESS,
                                "returning to lobby...");
        break;
        
    case MSG_REQUEST_PLAYER_UPDATE: {
        // FIXME: probably should handle validation code in the scenario module,
        // but this will have to do for now.

        // ensure the client doesn't try to update more tanks than actually
        // exist.
        const struct player_update body = unwrap_player_update_message(msg);
        
        int num_tanks = vec_len(body.tank_instructions);
        if (TANKS_IN_SCENARIO < num_tanks)
            num_tanks = TANKS_IN_SCENARIO;

        for (int t = 0; t < num_tanks; t++) {
            struct player_data* player = scenario_find_player(&g_scenario, p);

            struct coord target;
            enum tank_command command;
            
            vec_at(body.tank_target_coords, t, &target);
            vec_at(body.tank_instructions, t, &command);           

            struct tank* tank = vec_ref(player->tanks, t);

            if (command == TANK_MOVE)
                tank->move_to = target;
            else if (command == TANK_FIRE)
                tank->aim_at = target;

            tank->cmd = command;
        }
        
        
        r = message_status_send(p->socket, MESSAGE_STATUS_SUCCESS,
                                "updated successfully");
        break;
    }
    case MSG_REQUEST_DEBUG:
        r = message_status_send(p->socket, MESSAGE_STATUS_FAIL,
                                "not implemented");
        break;

        
    default:
        r = message_status_send(p->socket, MESSAGE_STATUS_INVALID_MESSAGE,
                                "command not supported in scenario.");
        break;

    }
                  
    if (r.status == RESULT_ERROR)
        return result_void_error(r.error);
    else
        return result_void_ok(0);
}

void print_player(struct player_manager *p) {
    printf("[player %p]\n  state: %d\n  username: %s\n\n",
           (void *)p, p->state, p->username);
}
