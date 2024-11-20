#include "command-line.h"
#include "error.h"
#include "scenario.h"
#include "message.h"
#include "sexp/sexp-utils.h"
#include "vector.h"
#include "nonstdint.h"
#include "enum_reflect.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

REFLECT_ENUM(message_type, MESSAGE_TYPE_ENUM_VALUES)

IMPL_RESULT_TYPE_CUSTOM(enum message_type, message_type) 
IMPL_RESULT_TYPE_CUSTOM(enum message_status, message_status)
IMPL_RESULT_TYPE_CUSTOM(struct user_credentials, user_credentials)
IMPL_RESULT_TYPE_CUSTOM(struct player_update, player_update)   
IMPL_RESULT_TYPE_CUSTOM(struct scenario_tick, scenario_tick)

void print_hex(const void *data, size_t len) {
    for (char *c = (char *)data, i = 1; c < (char *)data + len; c++, i++) {
        if (i % 16 == 0)
            printf("\n");

        printf("%02x ", *c);
    }
    printf("\n");
}

struct result_sexp coords_vec_to_sexp(struct vector *coords) {
    struct result_sexp r;
    r = make_sexp(SEXP_CONS, SEXP_MEMORY_TREE, NULL);
    if (r.status == RESULT_ERROR)
        return r;

    sexp *root = r.ok;

    for (u32 i = 0; i < vec_len(coords); i++) {
        struct coord *c = vec_ref(coords, i);
        
        sexp_push_integer(root, c->x);
        sexp_push_integer(root, c->x);
    }

    return result_sexp_ok(root);
}

struct result_vec coords_sexp_to_vector(sexp *coords) {
    struct vector *vec = make_vector(sizeof(struct coord), 32);

    struct result_sexp r;
    while (!sexp_is_nil(coords)) {
        struct coord coordinate;
        
        r = sexp_car(coords);
        if (r.status == RESULT_ERROR) return result_vec_error(r.error);
        sexp *car = r.ok;

        // (X Y X Y X Y)
        //  ^
        RESULT_UNWRAP(vec, coordinate.x, sexp_int_val(car));

        RESULT_UNWRAP(vec, coords, sexp_cdr(coords));

        RESULT_UNWRAP(vec, car, sexp_car(coords));

        // (X Y X Y X Y)
        //    ^
        RESULT_UNWRAP(vec, coordinate.y, sexp_int_val(car));

        RESULT_UNWRAP(vec, coords, sexp_cdr(coords));

        vec_push(vec, &coordinate);
    }

    return result_vec_ok(vec);
}

struct result_s32 message_send(int fd, const sexp *msg) {
    char *msg_str;
    RESULT_UNWRAP(s32, msg_str, sexp_serialize(msg));

    int bytes_sent = send(fd, msg_str, strlen(msg_str), 0);

    free(msg_str);
    return result_s32_ok(bytes_sent);
}

struct result_sexp message_recv(int fd, struct vector* buf) {
    size_t space_available = vec_cap(buf) - vec_len(buf);
    
    if (space_available < 50) {
        vec_reserve(buf, vec_len(buf) * 2);
        space_available = vec_cap(buf) - vec_len(buf);
    }

    int bytes_read = read(fd, (char *)vec_end(buf) + 1, space_available);
    vec_resize(buf, vec_len(buf) + bytes_read);

    // increases with open paren, decreases with close paren
    s32 paren_count = 0;
    for (char *c = vec_dat(buf); c < (char *)vec_end(buf); c++) {
        if (*c == '(')
            paren_count++;
        else if (*c == ')')
            paren_count--;
    }

    if (paren_count < 0) {
        // TODO this is an error state (extra close paren)
    }

    if (paren_count == 0 && vec_len(buf) > 0) {
        // closed parenthesis, and a non-empty buffer. complete message.

        // FIXME assumes no errors can happen.
        struct result_sexp r = sexp_read(vec_dat(buf), SEXP_MEMORY_TREE);
        vec_resize(buf, 0);

        return r; 
    }
    
    return result_sexp_ok(NULL);
}

/** returns either the enum value or a string for the enum.
*/

#define USE_STRING_HEADER
struct result_sexp message_make_header(enum message_type type) {
#ifdef USE_STRING_HEADER
    return make_symbol_sexp(g_reflected_message_type[type]);
#else
    return make_integer_sexp(type);
#endif

}

enum message_type message_get_type(const sexp *msg) {
    struct error e;

    struct result_sexp type_sym = sexp_nth(msg, 0);
    if (type_sym.status == RESULT_ERROR) {
        e = type_sym.error;
        goto error_condition;
    }

    if (type_sym.ok->sexp_type == SEXP_SYMBOL) {
        struct result_str msg_type_str = sexp_str_val(type_sym.ok);
        if (msg_type_str.status == RESULT_ERROR) {
            e = msg_type_str.error;
            goto error_condition;
        }

        for (int i = 0; i < MSG_NULL; i++) {
            if (strcmp(msg_type_str.ok, g_reflected_message_type[i]) == 0)
                return i;
        }
    } else if (type_sym.ok->sexp_type == SEXP_INTEGER) {
        struct result_s32 msg_type = sexp_int_val(type_sym.ok);
        if (msg_type.status == RESULT_ERROR) {
            e = msg_type.error;
            goto error_condition;
        }

        return msg_type.ok;
    }

    return MSG_NULL;

 error_condition:
    free_error(e);
    return MSG_NULL;
}


/*************************** Text Message Functions ***************************/
struct result_sexp make_text_message(const char *message) {
    return sexp_list(message_make_header(MSG_REQUEST_DEBUG),
                     make_string_sexp(message),
                     sexp_nil());
}

struct result_str unwrap_text_message(const sexp *msg) {
    const sexp *first;
    RESULT_UNWRAP(str, first, sexp_nth(msg, 1));
    
    // if first is not an integer, then it will return an error.
    // TODO: create a more descriptive error that includes the text message context
    return sexp_str_val(first);
}

/************************** Status Message Functions **************************/
struct result_sexp make_status_message(enum message_status status) {
    return sexp_list(message_make_header(MSG_RESPONSE_STATUS),
                     make_integer_sexp(status),
                     sexp_nil());
}

struct result_message_status unwrap_status_message(const sexp *msg) {
    const sexp *first;
    RESULT_UNWRAP(message_status, first, sexp_nth(msg, 1));

    struct result_s32 r = sexp_int_val(first);

    if (r.status == RESULT_OK)
        return result_message_status_ok(r.ok);
    else 
        return result_message_status_error(r.error);
}

struct result_s32 message_status_send(int fd, enum message_status status, char *brief) {
    sexp *msg;
    RESULT_UNWRAP(s32, msg, make_status_message(status));

    // add an optional brief description
    if (msg != NULL)
        RESULT_CALL(s32, sexp_push_string(msg, brief));

    struct result_s32 r = message_send(fd, msg);

    free_sexp(msg);

    return r;
}

/********************* User Credentials Message Functions *********************/
struct result_sexp
make_user_credentials_message(const struct user_credentials *creds) {
    return sexp_list(message_make_header(MSG_REQUEST_AUTHENTICATE),
                     make_string_sexp(vec_dat(creds->username)),
                     make_string_sexp(vec_dat(creds->password)),
                     sexp_nil());
}

struct result_sexp
make_user_credentials_message_str(const char *username, const char *password) {
    return sexp_list(message_make_header(MSG_REQUEST_AUTHENTICATE),
                     make_string_sexp(username),
                     make_string_sexp(password),
                     sexp_nil());
}

struct result_user_credentials
unwrap_user_credentials_message(const sexp *msg) {
    struct user_credentials creds;

    sexp *username;
    sexp *password;
    RESULT_UNWRAP(user_credentials, username, sexp_nth(msg, 1));
    RESULT_UNWRAP(user_credentials, password, sexp_nth(msg, 2));

    char *username_str;
    char *password_str;
    RESULT_UNWRAP(user_credentials, username_str, sexp_str_val(username));
    RESULT_UNWRAP(user_credentials, password_str, sexp_str_val(password));

    // data length will be the length of the encoded string
    creds.username = make_vector(sizeof(char), username->data_length);
    vec_pushn(creds.username, username_str, username->data_length);
    
    creds.password = make_vector(sizeof(char), password->data_length);
    vec_pushn(creds.password, password_str, password->data_length);

    return result_user_credentials_ok(creds);
}


/********************** Player Update Message Functions ***********************/
struct result_sexp
make_player_update_message(const struct player_update *player_update) {
    return sexp_list(message_make_header(MSG_REQUEST_PLAYER_UPDATE),
                     coords_vec_to_sexp(player_update->tank_target_coords),
                     coords_vec_to_sexp(player_update->tank_instructions),
                     sexp_nil());
}

struct result_player_update unwrap_player_update_message(const sexp *msg) {
    struct player_update update;

    sexp *targets;
    sexp *commands;
    RESULT_UNWRAP(player_update, targets, sexp_nth(msg, 1));
    RESULT_UNWRAP(player_update, commands, sexp_nth(msg, 2));

    // convert target coordinates, returning any error
    RESULT_UNWRAP(player_update, update.tank_target_coords,
                  coords_sexp_to_vector(targets));

    update.tank_instructions = make_vector(sizeof(enum tank_command),
                                           vec_len(update.tank_target_coords));

    for (u32 c = 0;
         (c < vec_len(update.tank_target_coords)) || sexp_is_nil(commands);
         c++) {

        sexp *car;
        RESULT_UNWRAP(player_update, car, sexp_car(commands));
        
        enum tank_command cmd;
        RESULT_UNWRAP(player_update, cmd, sexp_int_val(car));
        vec_push(update.tank_instructions, &cmd);

        // go to next element in commands, returning any errors that occur
        RESULT_UNWRAP(player_update, commands, sexp_cdr(commands));
    }

    return result_player_update_ok(update);
}

/********************** Scenario Tick Message Functions ***********************/
/*
  (SCENARIO-TICK (USERNAME1 (X Y X Y X Y ...))
                 (USERNAME2 (X Y X Y X Y ...))
                 (USERNAME3 (X Y X Y X Y ...))
                 ...)
 */

void free_scenario_tick(struct scenario_tick tick) {
    free_vector(tick.players_public_data);
}
 
struct result_sexp make_scenario_tick_message(const struct scenario_tick *tick) {
    sexp *msg_root;
    RESULT_UNWRAP(sexp, msg_root, make_cons_sexp());

    struct result_sexp msg = result_sexp_ok(msg_root);

    for (u32 p = 0; p < vec_len(tick->players_public_data); p++ ) {
        struct player_public_data *data = vec_ref(tick->players_public_data, p);

        struct result_sexp encoded_player_data = 
            sexp_list(make_string_sexp(vec_dat(data->username)),
                      coords_vec_to_sexp(data->tank_positions),
                      sexp_nil());

        msg = sexp_rpush(msg, encoded_player_data);
    }

    if (msg.status == RESULT_ERROR)
        return msg;
    else
        return result_sexp_ok(msg_root);
}

struct result_scenario_tick unwrap_scenario_tick_message(const sexp *msg) {
    struct scenario_tick tick = {
        .players_public_data = make_vector(sizeof(struct player_public_data), 10)
    };

    s32 num_players;
    RESULT_UNWRAP(scenario_tick, num_players, sexp_length(msg));
    num_players--; // remove the "SCENARIO-TICK" symbol

    sexp *player;
    RESULT_UNWRAP(scenario_tick, player, sexp_cdr(msg))

    for (s32 i = 0; i < num_players; i++) {
        struct player_public_data data;
        sexp *username;
        RESULT_UNWRAP(scenario_tick, username, sexp_nth(player, 0));

        sexp *tank_coords;
        RESULT_UNWRAP(scenario_tick, tank_coords, sexp_nth(player, 1));

        char *username_str;
        RESULT_UNWRAP(scenario_tick, username_str, sexp_str_val(username));

        data.username = make_vector(sizeof(char), username->data_length);
        vec_pushn(data.username, username_str, username->data_length);

        RESULT_UNWRAP(scenario_tick, data.tank_positions,
                      coords_sexp_to_vector(tank_coords));

        vec_push(tick.players_public_data, &data);

        RESULT_UNWRAP(scenario_tick, player, sexp_cdr(player));
    }

    return result_scenario_tick_ok(tick);
}

struct result_sexp make_join_scenario_message(const char *scenario_name) {
    return sexp_list(message_make_header(MSG_REQUEST_JOIN_SCENARIO),
                     make_string_sexp(scenario_name),
                     sexp_nil());  
}

struct result_sexp make_return_to_lobby_message() {
    return message_make_header(MSG_REQUEST_RETURN_TO_LOBBY);
}
struct result_sexp make_list_scenarios_message() {
    return message_make_header(MSG_REQUEST_LIST_SCENARIOS);
}
