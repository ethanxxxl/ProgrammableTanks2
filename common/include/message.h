#ifndef MESSAGE_H
#define MESSAGE_H

#include "scenario.h"

#include <stdint.h>
#include <vector.h>
#include <csexp.h>

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

/* TEXT
 *
 * Many messages are simply status messages with the option to include ascii
 * text for debug/logging purposes. This generic message is associated with the
 * text field in the message union. */
struct sexp *make_text_message();

/* USER_CREDENTIALS
 *
 * This is how users are admitted into the server and authenticated.
 * */
struct user_credentials {
    struct vector* username;
};

struct sexp *make_user_credentials_message();

/* PLAYER_UPDATE
 *
 * A player sends this mesage when they want to update their tanks.
 *
 */
struct player_update {
    struct vector* tank_target_coords;
    struct vector* tank_instructions;
};
struct sexp *make_player_update_message();

/* SCENARIO_TICK
 *
 * main message body structure.
 *  
 */
struct scenario_tick {
    struct vector* players_public_data;
};
struct sexp *make_player_update_message();

s32 message_send(int fd, const struct sexp *sexp);
struct sexp *message_recv;

#endif
