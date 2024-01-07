#include <fcntl.h>
#include <message.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <vector.h>
#include <tank.h>

void print_hex(const void *data, size_t len) {
    for (char *c = (char *)data, i = 1; c < (char *)data + len; c++, i++) {
        if (i % 16 == 0)
            printf("\n");

        printf("%02x ", *c);
    }
    printf("\n");
}

const struct message_fns G_TEXT_FNS = {
    &text_ser,
    &text_des,
    &text_init,
    &text_free,
    NULL
};
const struct message_fns G_USER_CREDENTIALS_FNS = {
    &user_credentials_ser,
    &user_credentials_des,
    &user_credentials_init,
    &user_credentials_free,
    NULL
};
const struct message_fns G_PLAYER_UPDATE_FNS = {
    &player_update_ser,
    &player_update_des,
    &player_update_init,
    &player_update_free,
    NULL
};
const struct message_fns G_SCENARIO_TICK_FNS = {
    &scenario_tick_ser,
    &scenario_tick_des,
    &scenario_tick_init,
    &scenario_tick_free,
    NULL
};

const struct message_fns* g_message_funcs[] =  {
    // IDLE STATE REQUESTS
    [MSG_REQUEST_AUTHENTICATE] = &G_USER_CREDENTIALS_FNS,

    // LOBBY STATE REQUESTS
    [MSG_REQUEST_LIST_SCENARIOS] = NULL,
    [MSG_REQUEST_CREATE_SCENARIO] = NULL,
    [MSG_REQUEST_JOIN_SCENARIO] = NULL,

    // SCENARIO STATE REQUESTS
    [MSG_REQUEST_PLAYER_UPDATE] = &G_PLAYER_UPDATE_FNS,
    [MSG_REQUEST_DEBUG] = &G_TEXT_FNS,
    [MSG_REQUEST_RETURN_TO_LOBBY] = NULL,

    // RESPONSES
    [MSG_RESPONSE_SCENARIO_TICK] = &G_SCENARIO_TICK_FNS,

    [MSG_RESPONSE_SUCCESS] = &G_TEXT_FNS,
    [MSG_RESPONSE_FAIL] = &G_TEXT_FNS,
    [MSG_RESPONSE_INVALID_REQUEST] = &G_TEXT_FNS,

    [MSG_NULL] = NULL,
};

int message_send(int fd, const struct message msg) {
    struct {
        enum message_type t;
        int body_len;
    } header = {0};

    header.t = msg.type;
    header.body_len = 0;

    struct vector body = {0};
    if (g_message_funcs[msg.type] != NULL) {
        make_vector(&body, sizeof(char), 10);

        g_message_funcs[msg.type]->message_ser(&msg, &body);
        header.body_len = body.len;
    }

    send(fd, &header, sizeof(header), 0);
    send(fd, body.data, body.len, 0);

    return 0;
}

int message_recv(int fd, struct message* msg, struct vector* buf) {
    /* DETERMINE AMOUNT OF DATA TO READ */


    // FIXME: this should probably be a fixed width integer
    int body_size = 0; // amount to read based off of length in header
    if (buf->len >= MESSAGE_HEADER_SIZE) {
        body_size = *(int *)((uint8_t *)buf->data + sizeof(enum message_type));
    }
     
    int read_amnt = buf->len - MESSAGE_HEADER_SIZE + body_size;

    if (read_amnt < 0)
        read_amnt = MESSAGE_HEADER_SIZE; // when buf->len = 0, then read_ammnt < 0.

    /* READ AS MUCH DATA AS POSSIBLE */
    vec_reserve(buf, buf->len + read_amnt);
    const uint8_t *buf_data = (uint8_t *)buf->data;
    int bytes_read = recv(fd, vec_ref(buf, buf->len), read_amnt, 0);

    if (bytes_read <= 0)
        return -1; // recv can potentially return -1.

    buf->len += bytes_read; // update with number of bytes read.

    // update body_size to match header data, if able.
    if (buf->len >= MESSAGE_HEADER_SIZE) {
        body_size = *(int *)(buf_data + sizeof(enum message_type));
    }

    if (buf->len < MESSAGE_HEADER_SIZE ||
        buf->len < MESSAGE_HEADER_SIZE + body_size) {
        return -1;
    }

    /* CREATE MESSAGE, CLEAR BUFFER */
    *msg = (struct message){0}; // clear out garbage from msg struct.
    msg->type = *(enum message_type *)buf->data;

    // remove the header from the vector, so it only contains the body.
    for (unsigned int i = 0; i < MESSAGE_HEADER_SIZE; i++)
        vec_rem(buf, 0);

    if (g_message_funcs[msg->type] != NULL) {
        g_message_funcs[msg->type]->message_init(msg);
        g_message_funcs[msg->type]->message_des(msg, buf);
    }

    vec_resize(buf, 0);
    return 0;
}

/// sends a SUCCESS, FAIL, or INVALID response. This function doesn't require a
/// vector to be initialized.
int message_send_conf(int fd, enum message_type type, const char *text) {
    if (type != MSG_RESPONSE_SUCCESS && type != MSG_RESPONSE_FAIL &&
        type != MSG_RESPONSE_INVALID_REQUEST)
        return -1;

    int header[] = {type, strlen(text)};
    int header_len = send(fd, header, sizeof(header), 0);
    if (header_len < 0)
        return -1;

    // don't send body if there is no text.
    if (text == NULL)
        return header_len;

    int body_len = send(fd, text, strlen(text), 0);
    if (body_len < 0)
        return -1;

    return header_len + body_len;
}

/* TEXT_FNS
 * */
void text_ser(const struct message* msg, struct vector* dat) {
    vec_pushn(dat, msg->text.data, msg->text.len);
}

void text_des(struct message* msg, const struct vector* dat) {
    vec_pushn(&msg->text, dat->data, dat->len);
    vec_push(&msg->text, "\0"); // ensure null termination
}
void text_init(struct message* msg) {
    make_vector(&msg->text, sizeof(char), 10);
}
void text_free(struct message* msg) { free_vector(&msg->text); }

/* USER_CREDENTIALS_FNS
 * */
void user_credentials_ser(const struct message* msg, struct vector* dat) {
    vec_pushn(dat, msg->user_credentials.username.data,
              msg->user_credentials.username.len);
}
void user_credentials_des(struct message* msg, const struct vector* dat) {
    print_hex(dat->data, dat->len);
    vec_pushn(&msg->user_credentials.username, dat->data, dat->len);
    vec_push(&msg->user_credentials.username, "\0");
}
void user_credentials_init(struct message* msg) {
    make_vector(&msg->user_credentials.username, sizeof(char), 10);
}
void user_credentials_free(struct message* msg) {
    free_vector(&msg->user_credentials.username);
}

/* PLAYER_UPDATE_FNS
 * message is encoded with the following pattern:
 * { tank_instructions[0..n], tank_positions[0..n], tank_targets[0..n]}
 * */
void player_update_ser(const struct message *msg, struct vector *dat) {
    // do a sanity check first
    struct vector instructions = msg->player_update.tank_instructions;
    struct vector target_coords = msg->player_update.tank_target_coords;
    struct vector pos_coords = msg->player_update.tank_position_coords;

    // lisp syntax is more succinct here:
    // (not (= instructions.len target_coords.len pos_coords.len))
    if ((instructions.len != target_coords.len) ||
        (instructions.len != pos_coords.len))
        // TODO: this should return -1.
        return;

    // add each element to the data stream.
    vec_pushn(dat, instructions.data,
              instructions.len * instructions.element_len);
    vec_pushn(dat, target_coords.data,
              target_coords.len * target_coords.element_len);
    vec_pushn(dat, pos_coords.data,
              pos_coords.len * pos_coords.element_len);

    return;
}
void player_update_des(struct message *msg, const struct vector *dat) {
    // tank data is disjoint and total number of tanks isn't encoded directly in
    // the data stream. the number of tanks is found by calculating the amount
    // of space all the data for a single tank consumes, then dividing the byte
    // stream length by that amount.
    const int element_total_data =
        sizeof(enum tank_command) + 2*sizeof(struct coordinate);

    const int num_tanks = dat->len / element_total_data;

    struct player_update* msg_data = &msg->player_update;

    // this will point to the array that needs to be copied into each vector
    uint8_t* data_array = dat->data;
    
    vec_pushn(&msg_data->tank_instructions, data_array, num_tanks);
    data_array += msg_data->tank_instructions.element_len * num_tanks;
    
    vec_pushn(&msg_data->tank_position_coords, data_array, num_tanks);
    data_array += msg_data->tank_position_coords.element_len * num_tanks;
    
    vec_pushn(&msg_data->tank_target_coords, data_array, num_tanks);

    return;
}
void player_update_init(struct message *msg) {
    // allocate members of the struct
    struct player_update player_update;
    make_vector(&player_update.tank_instructions,
                sizeof(enum tank_command), 30);
    make_vector(&player_update.tank_position_coords,
                sizeof(struct coordinate), 30);
    make_vector(&player_update.tank_target_coords,
                sizeof(struct coordinate), 30);

    // copy struct and return
    msg->player_update = player_update;
    return;    
}
void player_update_free(struct message *msg) {
    struct player_update* player_update = &msg->player_update;
    free_vector(&player_update->tank_instructions);
    free_vector(&player_update->tank_position_coords);
    free_vector(&player_update->tank_target_coords);

    memset(player_update, 0, sizeof(struct player_update));
    return;
}

/* SCENARIO_TICK
 *
 * the server sends this to the player at regular intervals.
 *
 * this message is encoded in the following format:
 * { num_players, num-tanks, usernames, tank_pos }
 * where num players are null terminated c-strings. there are num_players of
 * them.
 */
void scenario_tick_ser(const struct message *msg, struct vector *dat) {
    // do a sanity check first
    struct vector usernames = msg->scenario_tick.username_vecs;
    struct vector tanks_pos = msg->scenario_tick.tank_positions;

    if (usernames.len != tanks_pos.len)
        return; // FIXME this should be a return -1;

    // NUM_PLAYERS
    const uint8_t num_players = usernames.len;

    vec_push(dat, &num_players);
    
    // NUM TANKS
    struct vector first_tank_vec;
    vec_at(&tanks_pos, 0, &first_tank_vec);
    const uint8_t num_tanks = first_tank_vec.len / usernames.len;
    vec_push(dat, &num_tanks);

    // USERNAMES
    for (int n = 0; n < num_players; n++) {
        struct vector *username = vec_ref(&usernames, n);
        vec_pushn(dat, username->data, username->element_len*username->len);
    }

    // TANKS
    for (int n = 0; n < num_players; n++) {
        struct vector *tank_vec = vec_ref(&tanks_pos, n);
        vec_pushn(dat, tank_vec->data, tank_vec->element_len * tank_vec->len);
    }

    return;
}
void scenario_tick_des(struct message *msg, const struct vector *dat) {
    // NUM PLAYERS
    uint8_t num_players;
    vec_at(dat, 0, &num_players);

    // NUM TANKS
    uint8_t num_tanks;
    vec_at(dat, 1, &num_tanks);

    uint8_t const* dat_data = (uint8_t *)dat->data;
    const uint8_t* data_start = dat_data + sizeof(uint8_t) * 2;
    uint8_t const* data_end = dat_data + (dat->len * dat->element_len);

    // USERNAMES
    for (int n = 0; n < num_players; n++) {
        // get length of string including null terminator
        size_t name_len = strnlen((char *)data_start, data_end - data_start) + 1;
        
        struct vector username;
        make_vector(&username, sizeof(char), name_len);
        vec_pushn(&username, data_start, name_len);

        data_start += name_len;
        
        vec_push(&msg->scenario_tick.username_vecs, &username);
    }

    // TANKS
    for (int t = 0; t < num_players; t++) {
        struct vector tank_positions;
        make_vector(&tank_positions, sizeof(struct coordinate), 30);

        vec_pushn(&tank_positions, data_start, num_tanks);
        data_start += sizeof(struct coordinate) * num_tanks;

        vec_push(&msg->scenario_tick.tank_positions, &tank_positions);
    }
}
void scenario_tick_init(struct message *msg) {
    make_vector(&msg->scenario_tick.username_vecs, sizeof(struct vector), 5);

    make_vector(&msg->scenario_tick.tank_positions,
                sizeof(struct vector),5);
    return;
}
void scenario_tick_free(struct message *msg) {
    // gotta free all them strings...
    for (size_t n = 0; n < msg->scenario_tick.username_vecs.len; n++) {
        struct vector *name_str = vec_ref(&msg->scenario_tick.username_vecs, n);
        free_vector(name_str);
    }

    for (size_t n = 0; n < msg->scenario_tick.tank_positions.len; n++) {
        struct vector *tank_vec = vec_ref(&msg->scenario_tick.tank_positions, n);
        free_vector(tank_vec);
    }

    struct vector *usernames = &msg->scenario_tick.username_vecs;
    struct vector *tanks = &msg->scenario_tick.tank_positions;
    free_vector(usernames);
    free_vector(tanks);
    return;
}

static const char *message_type_labels[] = {
    [MSG_REQUEST_AUTHENTICATE] = "MSG_REQUEST_AUTHENTICATE",
    [MSG_REQUEST_LIST_SCENARIOS] = "MSG_REQUEST_LIST_SCENARIOS",
    [MSG_REQUEST_CREATE_SCENARIO] = "MSG_REQUEST_CREATE_SCENARIO",
    [MSG_REQUEST_JOIN_SCENARIO] = "MSG_REQUEST_JOIN_SCENARIO",
    [MSG_REQUEST_PLAYER_UPDATE] = "MSG_REQUEST_PROPOSE_UPDATE",
    [MSG_REQUEST_DEBUG] = "MSG_REQUEST_DEBUG",
    [MSG_REQUEST_RETURN_TO_LOBBY] = "MSG_REQUEST_RETURN_TO_LOBBY",
    [MSG_RESPONSE_SCENARIO_TICK] = "MSG_RESPONSE_SCENARIO_TICK",
    [MSG_RESPONSE_SUCCESS] = "MSG_RESPONSE_SUCCESS",
    [MSG_RESPONSE_FAIL] = "MSG_RESPONSE_FAIL",
    [MSG_RESPONSE_INVALID_REQUEST] = "MSG_RESPONSE_INVALID_REQUEST",
};

void print_message(struct message msg) {
    if (msg.type < sizeof(message_type_labels)) {
        printf("message: %s\n", message_type_labels[msg.type]);
    } else {
        printf("message: %d (unknown)\n", msg.type);
    }

    switch (msg.type) {
    case MSG_RESPONSE_SUCCESS:
    case MSG_RESPONSE_FAIL:
    case MSG_RESPONSE_INVALID_REQUEST:
        if (msg.text.len > 0)
            printf(" [txt] %s\n", (char *)msg.text.data);
        break;

    case MSG_REQUEST_AUTHENTICATE:
        printf(" [username] %s\n", (char*)msg.user_credentials.username.data);
        break;

    case MSG_RESPONSE_SCENARIO_TICK:
        printf(" [users] [%zu]\n", msg.scenario_tick.username_vecs.len);
        for (size_t i = 0; i < msg.scenario_tick.username_vecs.len; i++) {
            struct vector *username =
                vec_ref(&msg.scenario_tick.username_vecs, i);
            printf("   %s\n\n", (char*)username->data);
        }

        printf(" [tanks] %zu\n", msg.scenario_tick.tank_positions.len);
        break;
    default:
        break;
    }
}

int make_message(struct message* msg, enum message_type type) {
    msg->type = type;
    if (g_message_funcs[type] == NULL) {
        return 0;
    }

    g_message_funcs[type]->message_init(msg);
    return 0;
}

int free_message(struct message msg) {
    if (g_message_funcs[msg.type] == NULL)
        return 0;

    g_message_funcs[msg.type]->message_free(&msg);
    return 0;
}
