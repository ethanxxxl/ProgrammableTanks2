#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <tank.h>

struct request_player_update {
    uint32_t update_items;
    tank_id_t* ids;
    struct xy_pos* positions;
};

struct user_credentials_msg {
    char username[50];
};

enum message_type {
    MSG_REQUEST_AUTHENTICATE,
    MSG_REQUEST_WORLD,
    MSG_REQUEST_PROPOSE_UPDATE,
    MSG_REQUEST_DEBUG,
    MSG_RESPONSE_UPDATE_VALIDATION,
    MSG_RESPONSE_WORLD_DATA,
};

struct message {
    enum message_type type;
    
    union {
	struct request_player_update player_update;
	struct user_credentials_msg user_credentials;	
	char debug_msg[50];
    };
};


int send_message(int fd, const struct message *msg);
int recv_message(int fd, struct message *msg);

#endif
