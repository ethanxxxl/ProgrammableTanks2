#include <SDL2/SDL_shape.h>
#include <player_manager.h>
#include <message.h>
#include <stdio.h>
#include <string.h>

int make_player(struct player_manager *p) {

    return 0;
}

int player_idle_handler(struct player_manager *p, struct message msg) {
    switch (msg.type) {
    case MSG_REQUEST_AUTHENTICATE:
	strcpy(p->username, msg.user_credentials.username);
	p->state = STATE_LOBBY; // FIXME: no authentication done here!
	printf("%s: authenticated\n", p->username);
	printf("%s: authenticated (msg data)\n", msg.user_credentials.username);

	{
	    char buf[50] = {0};
	    snprintf(buf, 50, "authenticated %s.", p->username);
	    send_conf_message(p->socket, MSG_RESPONSE_SUCCESS,
			      buf);
	}

        break;
		 
    default: 
	send_conf_message(p->socket, MSG_RESPONSE_FAIL,
			  "you must be authenticated first");
	break;
    }

    return 0;
}

int player_lobby_handler(struct player_manager *p, struct message msg) {
    switch (msg.type) {
    case MSG_REQUEST_LIST_SCENARIOS:
	send_conf_message(p->socket, MSG_RESPONSE_SUCCESS,
			  "There is only one scenario (0)");
	break;
    case MSG_REQUEST_CREATE_SCENARIO:
	break;
    case MSG_REQUEST_JOIN_SCENARIO:
	p->state = STATE_SCENARIO;
	// TODO: add player to global scenario.
	
	send_conf_message(p->socket, MSG_RESPONSE_SUCCESS,
			  "entering scenario...");
	break;
    default:
	send_conf_message(p->socket, MSG_RESPONSE_INVALID_REQUEST,
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

	// TODO: remove player from global scenario.
	send_conf_message(p->socket, MSG_RESPONSE_SUCCESS,
			  "returning to lobby...");

	break;
	
    case MSG_REQUEST_WORLD:
    case MSG_REQUEST_PROPOSE_UPDATE:
    case MSG_REQUEST_DEBUG:
	send_conf_message(p->socket, MSG_RESPONSE_FAIL,
			  "not implemented");
	break;
	
	
    default:
	send_conf_message(p->socket, MSG_RESPONSE_INVALID_REQUEST,
			  "command not supported in scenario.");
	break;

    }
			 
    return 0;
}

void print_player(struct player_manager *p) {
    printf("[player %p]\n  state: %d\n  username: %s\n\n", p, p->state, p->username);
}
