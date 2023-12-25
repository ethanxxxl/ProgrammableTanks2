#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <vector.h>

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

/* MESSAGE_FNS
 *
 * Every field of the union in the message structure has data that needs to be
 * sent over the network. */
struct message;
struct message_fns {
    void (*message_ser)(const struct message*, struct vector*);
    void (*message_des)(struct message*, const struct vector*);
    void (*message_init)(struct message*);
    void (*message_free)(struct message*);
    void (*message_print)(struct message*);
};

/* TEXT
 *
 * Many messages are simply status messages with the option to include ascii
 * text for debug/logging purposes. This generic message is associated with the
 * text field in the message union. */
void text_ser(const struct message* msg, struct vector* dat);
void text_des(struct message* msg, const struct vector* dat);
void text_init(struct message*);
void text_free(struct message*);

/* PLAYER_UPDATE
 *
 * whenever the player wants to send updates to the tanks in a scenario, this is
 * the data that will be sent. */
struct player_update {
    uint32_t update_items;
    struct xy_pos* positions;
};
void player_update_ser(const struct message* msg, struct vector* dat);
void player_update_des(struct message* msg, const struct vector* dat);
void player_update_init(struct message*);
void player_update_free(struct message*);

/* USER_CREDENTIALS
 *
 * This is how users are admitted into the server and authenticated.
 * */
struct user_credentials {
    struct vector username;
};
void user_credentials_ser(const struct message* msg, struct vector* dat);
void user_credentials_des(struct message* msg, const struct vector* dat);
void user_credentials_init(struct message*);
void user_credentials_free(struct message*);

#define MESSAGE_HEADER_SIZE (sizeof(enum message_type) + sizeof(int))
struct message {
    // message header
    enum message_type type;

    // message body. every one of these elements will have an encoding/decoding
    // function associated with them.
    union {
        struct vector text; /// char vector
        struct player_update player_update;
        struct user_credentials user_credentials;
    };
};

int message_send(int fd, const struct message msg);
int message_send_conf(int fd, enum message_type type, const char *text);

/// recieves a message from the socket. This function is designed to be
/// non-blocking. requires an initialized vector be supplied to store incomplete
/// messages between calls.
///
/// CREATES A NEW MESSAGE, POTENTIALLY USING MALOC
///
/// returns  0 when it fills out `msg`.
/// returns -1 otherwise
int message_recv(int fd, struct message *msg, struct vector *buf);

void print_message(struct message msg);

int make_message(struct message* msg, enum message_type type);
int free_message(struct message msg);

#endif
