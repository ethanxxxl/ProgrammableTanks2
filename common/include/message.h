#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <vector.h>

// BUG: TODO: add return value to the message callback functions. if
// there is an error in the functions, then they should return -1, so
// that further errors are prevented.

/// All possible message types.
///
/// Response messages are generally SUCCESS, FAIL, or INVALID.
/// These messages include display/status text in the message body.
enum message_type {
  // IDLE STATE REQUESTS
  MSG_REQUEST_AUTHENTICATE = 0x00,

  // LOBBY STATE REQUESTS
  MSG_REQUEST_LIST_SCENARIOS,
  MSG_REQUEST_CREATE_SCENARIO,
  MSG_REQUEST_JOIN_SCENARIO,

  // SCENARIO STATE REQUESTS
  MSG_REQUEST_PLAYER_UPDATE,
  MSG_REQUEST_RETURN_TO_LOBBY,

  // MISC REQUESTS
  MSG_REQUEST_DEBUG,
  MSG_REQUEST_NULL, // not an actual message type

  // RESPONSES
  MSG_RESPONSE_SCENARIO_TICK = 0x80,

  MSG_RESPONSE_SUCCESS,
  MSG_RESPONSE_FAIL,
  MSG_RESPONSE_INVALID_REQUEST,

  MSG_RESPONSE_NULL, // not an actual message type
  
  MSG_NULL ,
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

/* USER_CREDENTIALS
 *
 * This is how users are admitted into the server and authenticated.
 * */
struct user_credentials {
    struct vector* username;
};
void user_credentials_ser(const struct message* msg, struct vector* dat);
void user_credentials_des(struct message* msg, const struct vector* dat);
void user_credentials_init(struct message*);
void user_credentials_free(struct message *);

/* PLAYER_UPDATE
 *
 * A player sends this mesage when they want to update their tanks.
 *
 * TODO: this message is pretty wasteful, since there may be lots of data which
 *       is unchanged since the last tick. At some point, you may want to
 *       consider optimizing this message for space.
 */
struct coordinate {
    int x, y;
};

struct player_update {
    struct vector* tank_target_coords;
    struct vector* tank_instructions;
};
void player_update_ser(const struct message *msg, struct vector *dat);
void player_update_des(struct message *msg, const struct vector *dat);
void player_update_init(struct message *msg);
void player_update_free(struct message *msg);

/* SCENARIO_TICK
 *
 * the server sends this to the player at regular intervals
 * includes each players username, and the position of all their tanks.
 *
 * INFO: the usernames vector is a vector of vectors (we don't know how long a
 *       username might be)

 * TODO: including all the usernames is a bit wasteful, but it was the easiest
 *       way to get the ball rolling on this thing. Implement some more
 *       state-based requests for the client.
 *
 * FIXME: there is missing tank data in these updates (health)
 *
 * TODO: this message is pretty wasteful, since there may be lots of data which
 *       is unchanged since the last tick. At some point, you may want to
 *       consider optimizing this message for space.
 */
struct scenario_tick {
    struct vector* username_vecs;
    // TODO: struct vector* user_tasks ??
    struct vector* tank_positions;
};
void scenario_tick_ser(const struct message *msg, struct vector *dat);
void scenario_tick_des(struct message *msg, const struct vector *dat);
void scenario_tick_init(struct message *msg);
void scenario_tick_free(struct message *msg);

#define MESSAGE_HEADER_SIZE (sizeof(enum message_type) + sizeof(int))
struct message {
    // message header
    enum message_type type;

    // message body. every one of these elements will have an encoding/decoding
    // function associated with them.
    union {
        struct vector* text; /// char vector
        struct user_credentials user_credentials;
        struct player_update player_update;
        struct scenario_tick scenario_tick;
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
