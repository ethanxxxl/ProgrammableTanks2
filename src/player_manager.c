#include "scenario.h"
#include <SDL2/SDL_shape.h>
#include <player_manager.h>
#include <message.h>
#include <stdio.h>
#include <string.h>


extern struct scenario g_scenario;

int make_player(struct player_manager *p) {
    (void)p; // we don't need to allocate anything currently.
    return 0;
}

int player_idle_handler(struct player_manager *p, struct message msg) {
    switch (msg.type) {
    case MSG_REQUEST_AUTHENTICATE:
        strcpy(p->username, msg.user_credentials.username.data);
        p->state = STATE_LOBBY; // FIXME: no authentication done here!
        printf("%s: authenticated\n", p->username);
        printf("%s: authenticated (msg data)\n", (char*)msg.user_credentials.username.data);

        {
            char buf[50] = {0};
            int ret =
                snprintf(buf, 50, "authenticated %s.", p->username);
            if (ret < 0)
                return -1; // name was too large for the buffer.
            
            message_send_conf(p->socket, MSG_RESPONSE_SUCCESS,
                              buf);
        }

        break;
                 
    default: 
        message_send_conf(p->socket, MSG_RESPONSE_FAIL,
                          "you must be authenticated first");
        break;
    }

    return 0;
}

int player_lobby_handler(struct player_manager *p, struct message msg) {
    switch (msg.type) {
    case MSG_REQUEST_LIST_SCENARIOS:
        message_send_conf(p->socket, MSG_RESPONSE_SUCCESS,
                   "There is only one scenario (0)");
        break;
    case MSG_REQUEST_CREATE_SCENARIO:
        break;
        
    case MSG_REQUEST_JOIN_SCENARIO:
        p->state = STATE_SCENARIO;

        scenario_add_player(&g_scenario, p);
        
        message_send_conf(p->socket, MSG_RESPONSE_SUCCESS,
                   "entering scenario...");
        break;
    default:
        message_send_conf(p->socket, MSG_RESPONSE_INVALID_REQUEST,
                          "not supported in lobby.");
        break;
    }
    
    return 0;
}

int player_scenario_handler(struct player_manager *p, struct message msg) {
    switch (msg.type) {
    case MSG_REQUEST_RETURN_TO_LOBBY:
        // FIXME: there should be some limitations on when a player can exit a
        // scenario. don't want a player to be able to leave in the heat of
        // battle. At the very least, some logging should be done to track bad
        // behavior.
        
        p->state = STATE_LOBBY;
        scenario_rem_player(&g_scenario, p);

        // TODO: remove player from global scenario.
        message_send_conf(p->socket, MSG_RESPONSE_SUCCESS,
                          "returning to lobby...");

        break;
        
    case MSG_REQUEST_PLAYER_UPDATE: {
        // FIXME: probably should handle validation code in the scenario module,
        // but this will have to do for now.

        // ensure the client doesn't try to update more tanks than actually
        // exist.
        const struct player_update body = msg.player_update;
        
        int num_tanks = body.tank_instructions.len;
        if (TANKS_IN_SCENARIO < num_tanks)
            num_tanks = TANKS_IN_SCENARIO;

        for (int t = 0; t < num_tanks; t++) {
            struct actor *actor = scenario_find_actor(&g_scenario, p);

            struct coordinate move_to;
            struct coordinate aim_at;
            enum tank_command command;
            
            vec_at(&body.tank_position_coords, t, &move_to);
            vec_at(&body.tank_target_coords, t, &aim_at);
            vec_at(&body.tank_instructions, t, &command);           

            struct tank *tank = &actor->tanks[t];

            tank->move_to_x = move_to.x;
            tank->move_to_y = move_to.y;

            tank->aim_at_x = aim_at.x;
            tank->aim_at_y = aim_at.y;

            tank->cmd = command;
        }
        
        
        message_send_conf(p->socket, MSG_RESPONSE_FAIL,
                   "updated successfully");
    }
        break;
    case MSG_REQUEST_DEBUG:
        message_send_conf(p->socket, MSG_RESPONSE_FAIL,
                   "not implemented");
        break;

        
    default:
        message_send_conf(p->socket, MSG_RESPONSE_INVALID_REQUEST,
                   "command not supported in scenario.");
        break;

    }
                  
    return 0;
}

void print_player(struct player_manager *p) {
    printf("[player %p]\n  state: %d\n  username: %s\n\n",
           (void *)p, p->state, p->username);
}
