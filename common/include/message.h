#ifndef MESSAGE_H
#define MESSAGE_H

#include "scenario.h"
#include "sexp/sexp-base.h"
#include "enum_reflect.h"

#include <stdint.h>
#include <vector.h>
#include <sexp.h>


         

// BUG: TODO: add return value to the message callback functions. if
// there is an error in the functions, then they should return -1, so
// that further errors are prevented.

/// All possible message types.
///
/// Response messages are generally SUCCESS, FAIL, or INVALID.
/// These messages include display/status text in the message body.
REFLECT_ENUM(message_type,
             // IDLE STATE REQUESTS
             MSG_REQUEST_AUTHENTICATE,

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
             MSG_RESPONSE_SCENARIO_TICK,
             MSG_RESPONSE_STATUS,
             
             MSG_RESPONSE_NULL, // not an actual message type
             
             MSG_NULL,
             )

struct result_sexp make_message(enum message_type type);
void message_send(int fd, const struct sexp *message);
struct result_sexp message_recv(int fd, struct vector *buf);

/** Return the type of the message. */
enum message_type message_get_type(const sexp *msg);

/* TEXT
 *
 * Many messages are simply status messages with the option to include ascii
 * text for debug/logging purposes. This generic message is associated with the
 * text field in the message union. */
struct result_sexp make_text_message(const char *message);
const char *unwrap_text_message(const sexp *msg);
/* STATUS

   A message that indicates whether the previous message sent by the client was:
   - SUCCESS
   - FAIL
   - INVALID_MESSAGE
*/

enum message_status {
  MESSAGE_STATUS_SUCCESS,
  MESSAGE_STATUS_FAIL,
  MESSAGE_STATUS_INVALID_MESSAGE,
};

struct result_sexp make_status_message(enum message_status status);
enum message_status unwrap_status_message(const sexp *msg);
struct result_s32 message_status_send(int fd, enum message_status status, const char *brief);

/* USER_CREDENTIALS
 *
 * This is how users are admitted into the server and authenticated.
 * */
struct user_credentials {
    struct vector* username;
    struct vector* password;
};

struct result_sexp
make_user_credentials_message(const struct user_credentials *creds);

struct user_credentials
unwrap_user_credentials_message(const sexp *msg);

/* PLAYER_UPDATE
 *
 * A player sends this mesage when they want to update their tanks.
 *
 */
struct player_update {
    struct vector* tank_target_coords;
    struct vector* tank_instructions;
};

struct result_sexp
make_player_update_message(const struct player_update *player_update);

struct player_update unwrap_player_update_message(const struct sexp *msg);


/* SCENARIO_TICK
 *
 * main message body structure.
 *  
 */
struct scenario_tick {
    struct vector* players_public_data;
};

struct result_sexp make_scenario_tick_message(const struct scenario_tick *tick);
struct scenario_tick unwrap_scenario_tick_message(const sexp *msg);
void message_scenario_tick_add_player(sexp **msg, const struct player_data* pd);

#endif
