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
	break;
    default:
	break;
    }

    return 0;
}

int player_lobby_handler(struct player_manager *p, struct message msg) {

    return 0;
}

int player_scenario_handler(struct player_manager *p, struct message msg) {

    return 0;
}
