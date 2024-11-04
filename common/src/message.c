#include "command-line.h"
#include "error.h"
#include "scenario.h"
#include "message.h"
#include "sexp/sexp-base.h"
#include "sexp/sexp-utils.h"
#include "vector.h"
#include "nonstdint.h"
#include "sexp.h"


#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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

    struct sexp *root = r.ok;

    for (u32 i = 0; i < vec_len(coords); i++) {
        struct coord *c = vec_ref(coords, i);
        
        sexp_push_integer(root, c->x);
        sexp_push_integer(root, c->x);
    }

    return result_sexp_ok(root);
}

struct result_vec coords_sexp_to_vector(struct sexp *coords) {
    struct vector *vec = make_vector(sizeof(struct coord), 32);

    struct result_sexp r;
    while (!sexp_is_nil(coords)) {
        struct coord coordinate;
        
        r = sexp_car(coords);
        if (r.status == RESULT_ERROR) return result_vec_error(r.error);
        sexp *car = r.ok;

        struct result_s32 int_r;
        
        int_r = sexp_int_val(car);
        if (int_r.status == RESULT_ERROR) return result_vec_error(int_r.error);
        
        coordinate.x = int_r.ok;

        r = sexp_cdr(coords);
        if (r.status == RESULT_ERROR) return result_vec_error(r.error);
        coords = r.ok;

        r = sexp_car(coords);
        if (r.status == RESULT_ERROR) return result_vec_error(r.error);
        car = r.ok;

        int_r = sexp_int_val(car);
        if (int_r.status == RESULT_ERROR) return result_vec_error(int_r.error);
        coordinate.y = int_r.ok;

        r = sexp_cdr(coords);
        if (r.status == RESULT_ERROR) return result_vec_error(r.error);
        coords = r.ok;

        vec_push(vec, &coordinate);
    }

    return vec;
}


void message_send(int fd, const struct sexp_dyn *msg) {
    (void)fd;
    (void)msg;

    
        
    return;
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
        struct sexp_dyn *sexp = sexp_dyn_read(vec_dat(buf));
        vec_resize(buf, 0);

        return sexp;
    }
    
    return NULL;
}

/** returns either the enum value or a string for the enum.
*/
#define USE_STRING_HEADER
#ifdef USE_STRING_HEADER
#define MAKE_HEADER(type)                                                      \
    (make_string(#type))
#else
#define MAKE_HEADER(type)                                                      \
    (make_integer(type))

#endif


/*************************** Text Message Functions ***************************/
struct result_sexp make_text_message(const char *message) {
    return list((struct sexp_dyn*[]){
            MAKE_HEADER(MSG_REQUEST_DEBUG),
            make_string(message)}
        );
}

const char *unwrap_text_message(const struct sexp_dyn *msg) {
    if (nth(msg, 1)->type != SEXP_STRING)
        return NULL;

    return nth(msg, 1)->text;
}


/************************** Status Message Functions **************************/
struct result_sexp make_status_message(enum message_status status) {
    return list((struct sexp_dyn*[]){
                MAKE_HEADER(MSG_RESPONSE_STATUS),
                make_integer(status)}
        );
}

enum message_status unwrap_status_message(const struct sexp_dyn *msg) {
    if (nth(msg, 1)->type != SEXP_INTEGER) {
        // TODO what to do if this is wrong!?
    }
        
    return nth(msg, 1)->integer;
}

/********************* User Credentials Message Functions *********************/
struct result_sexp
make_user_credentials_message(const struct user_credentials *creds) {
    return list((struct sexp_dyn*[]){
            MAKE_HEADER(MSG_REQUEST_AUTHENTICATE),
            make_string(vec_dat(creds->username)),
            make_string(vec_dat(creds->password)),
        });
}

struct user_credentials
unwrap_user_credentials_message(const struct sexp_dyn *msg) {
    struct user_credentials creds;

    struct sexp_dyn *username = nth(msg, 1);
    struct sexp_dyn *password = nth(msg, 2);

    if (username->type != SEXP_STRING || password->type != SEXP_STRING) {
        // TODO  what to do if this wrong!?
    }
    
    creds.username = make_vector(sizeof(char), username->text_len);
    vec_pushn(creds.username, username->text, username->text_len);
    
    creds.password = make_vector(sizeof(char), password->text_len);
    vec_pushn(creds.password, password->text, password->text_len);

    return creds;
}

/********************** Player Update Message Functions ***********************/
struct result_sexp
make_player_update_message(const struct player_update *player_update) {

    return list((struct sexp_dyn*[]){
            MAKE_HEADER(MSG_REQUEST_PLAYER_UPDATE),
            coords_vec_to_sexp(player_update->tank_target_coords),
            coords_vec_to_sexp(player_update->tank_instructions),
        });
}

struct player_update unwrap_player_update_message(const struct sexp_dyn *msg) {
    struct player_update update;

    struct sexp_dyn *targets = nth(msg, 1);
    struct sexp_dyn *commands = nth(msg, 2);

    update.tank_target_coords = coords_sexp_to_vector(targets);

    update.tank_instructions = make_vector(sizeof(enum tank_command),
                                           vec_len(update.tank_target_coords));

    for (u32 c = 0;
         (c < vec_len(update.tank_target_coords)) || is_nil(commands);
         c++) {
        vec_push(update.tank_instructions, &car(commands)->integer);
        commands = cdr(commands);
    }

    return update;
}

/********************** Scenario Tick Message Functions ***********************/
/*
  (SCENARIO-TICK (USERNAME1 (X Y X Y X Y ...))
                 (USERNAME2 (X Y X Y X Y ...))
                 (USERNAME3 (X Y X Y X Y ...))
                 ...)
 */
 
struct result_sexp make_scenario_tick_message(const struct scenario_tick *tick) {
    struct sexp *msg_root = make_cons(NULL, NULL);
    struct sexp *msg = msg_root;

    for (u32 p = 0; p < vec_len(tick->players_public_data); p++ ) {
        struct player_public_data *data = vec_ref(tick->players_public_data, p);

        append(msg, list((struct sexp_dyn *[])
                         { make_string(vec_dat(data->username)),
                           coords_vec_to_sexp(data->tank_positions),
                         }));

        msg = cdr(msg);
    }

    return msg_root;
}

struct scenario_tick unwrap_scenario_tick_message(const struct sexp_dyn *msg) {
    struct scenario_tick tick = {
        .players_public_data = make_vector(sizeof(struct player_public_data), 10)
    };

    s32 num_players = length(msg) - 1;

    struct sexp_dyn *player = cdr(msg);

    for (s32 i = 0; i < num_players; i++) {
        struct player_public_data data;

        struct sexp_dyn *username = nth(player, 0);
        struct sexp_dyn *tank_coords = nth(player, 1);

        data.username = make_vector(sizeof(char), username->text_len);
        vec_pushn(data.username, username->text, username->text_len);
        
        data.tank_positions = coords_sexp_to_vector(tank_coords);

        vec_push(tick.players_public_data, &data);

        player = cdr(player);
    }

    return tick;
}
