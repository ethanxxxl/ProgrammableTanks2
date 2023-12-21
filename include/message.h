#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <vector.h>

struct request_player_update {
    uint32_t update_items;
    struct xy_pos* positions;
};

struct user_credentials_msg {
    char username[50];
};

/// All possible message types.
///
/// Response messages are generally SUCCESS, FAIL, or INVALID.
/// These messages include display/status text in the message body.
enum message_type {
    // IDLE STATE REQUESTS
    MSG_REQUEST_AUTHENTICATE,
    
    // LOBBY STATE REQUESTS
    MSG_REQUEST_LIST_SCENARIOS,
    MSG_REQUEST_CREATE_SCENARIO,
    MSG_REQUEST_JOIN_SCENARIO,
    
    // SCENARIO STATE REQUESTS
    MSG_REQUEST_WORLD,
    MSG_REQUEST_PROPOSE_UPDATE,
    MSG_REQUEST_DEBUG,
    MSG_REQUEST_RETURN_TO_LOBBY,

    // RESPONSES
    MSG_RESPONSE_WORLD_DATA,

    MSG_RESPONSE_SUCCESS,
    MSG_RESPONSE_FAIL,
    MSG_RESPONSE_INVALID_REQUEST,

    MSG_NULL,
};

#define MESSAGE_HEADER_SIZE (sizeof(enum message_type) + sizeof(int))
struct message {
    // message header
    enum message_type type;

    // message body
    union {
	struct vector text; /// char vector
	struct request_player_update player_update;
	struct user_credentials_msg user_credentials;	
	char debug_msg[50];
    };
};

int message_len(const struct message msg);

int send_message(int fd, const struct message msg);
int send_conf_message(int fd, enum message_type type, const char *text);

/// recieves a message from the socket. This function is designed to be
/// non-blocking. requires an initialized vector be supplied to store incomplete
/// messages between calls.
///
/// returns  0 when it fills out `msg`.
/// returns -1 otherwise
int recv_message(int fd, struct message *msg, struct vector *buf);

void print_message(struct message msg);

int free_message(struct message msg);

#endif
