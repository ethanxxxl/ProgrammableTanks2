#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <tank.h>

struct propose_update_msg {
    uint32_t update_items;
    tank_id_t* ids;
    struct xy_pos* positions;
};

struct request_world_msg {
    
};

struct user_credentials_msg {
    char username[50];
};

enum client_message_type {
    CLIENT_USER_CREDENTIALS,
    CLIENT_REQUEST_WORLD,
    CLIENT_PROPOSE_UPDATE,
    CLIENT_DEBUG_MESSAGE,
};

enum server_message_type {
    SERVER_UPDATE_VALIDATION,
    SERVER_WORLD_DATA,
};

struct server_message {
    enum server_message_type msg_type;
    union {
	int a;
	int b;
	int c;
    };
};

struct client_message {
    enum client_message_type msg_type;
    
    union {
	struct propose_update_msg propose_update;
	struct request_world_msg request_world;
	char debug_msg[50];
	struct user_credentials_msg user_credentials;
    };
};

int send_client_message(int fd, const struct client_message *msg);
int recv_client_message(int fd, struct client_message *msg);
int send_server_message(int fd, const struct server_message *msg);
int recv_server_message(int fd, struct server_message *msg);

#endif
