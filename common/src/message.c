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

    struct vector* body = make_vector(sizeof(char), 10);

    if (g_message_funcs[msg.type] != NULL) {
        g_message_funcs[msg.type]->message_ser(&msg, body);
        header.body_len = vec_len(body);
    }

    send(fd, &header, sizeof(header), 0);
    send(fd, vec_dat(body), vec_len(body), 0);

    free_vector(body);
    return 0;
}

int message_recv(int fd, struct message* msg, struct vector* buf) {
    /* DETERMINE AMOUNT OF DATA TO READ */


    // FIXME: this should probably be a fixed width integer
    int body_size = 0; // amount to read based off of length in header
    if (vec_len(buf) >= MESSAGE_HEADER_SIZE) {
        body_size = *(int*)vec_byte_ref(buf, sizeof(enum message_type));
    }
     
    int read_amnt = vec_len(buf) - MESSAGE_HEADER_SIZE + body_size;

    if (read_amnt < 0)
        read_amnt = MESSAGE_HEADER_SIZE; // when vec_len(buf) = 0, then read_ammnt < 0.

    /* READ AS MUCH DATA AS POSSIBLE */
    uint8_t tmp[read_amnt];
    int bytes_read = recv(fd, tmp, read_amnt, 0);

    if (bytes_read <= 0)
        return -1; // recv can potentially return -1.

    vec_pushn(buf, tmp, bytes_read);

    // update body_size to match header data, if able.
    if (vec_len(buf) >= MESSAGE_HEADER_SIZE) {
        body_size = *(int*)vec_byte_ref(buf, sizeof(enum message_type));
    }

    if (vec_len(buf) < MESSAGE_HEADER_SIZE ||
        vec_len(buf) < MESSAGE_HEADER_SIZE + body_size) {
        return -1;
    }

    /* CREATE MESSAGE, CLEAR BUFFER */
    *msg = (struct message){0}; // clear out garbage from msg struct.
    msg->type = *(enum message_type *)vec_dat(buf);

    if (msg->type >= MSG_NULL) {
        // the message type is not recognized.  Through out the message.
        vec_resize(buf, 0);
        printf("received an invalid message!\n");
        return -1; // message is invalid
    }

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
    vec_concat(dat, msg->text);
}

void text_des(struct message* msg, const struct vector* dat) {
    vec_concat(msg->text, dat);
    vec_push(msg->text, "\0"); // ensure null termination
}
void text_init(struct message* msg) {
    msg->text = make_vector(sizeof(char), 10);
}
void text_free(struct message* msg) { free_vector(msg->text); }

/* USER_CREDENTIALS_FNS
 * */
void user_credentials_ser(const struct message* msg, struct vector* dat) {
    vec_concat(dat, msg->user_credentials.username);
}
void user_credentials_des(struct message* msg, const struct vector* dat) {
    vec_concat(msg->user_credentials.username, dat);
    vec_push(msg->user_credentials.username, "\0");
}
void user_credentials_init(struct message* msg) {
    msg->user_credentials.username = make_vector(sizeof(char), 10);
}
void user_credentials_free(struct message* msg) {
    free_vector(msg->user_credentials.username);
}

/* PLAYER_UPDATE_FNS
 * message is encoded with the following pattern:
 * { tank_instructions[0..n], tank_positions[0..n], tank_targets[0..n]}
 * */
void player_update_ser(const struct message *msg, struct vector *dat) {
    // do a sanity check first
    struct vector* instructions  = msg->player_update.tank_instructions;
    struct vector* target_coords = msg->player_update.tank_target_coords;

    if (vec_len(instructions) != vec_len(target_coords))
        // TODO: this should return -1.
        return;

    // add each element to the data stream. 
    for (size_t i = 0; i < vec_len(instructions); i++) {
        // convert a tank command enum to a uint8_t, without losing any data due
        // to endianness
        uint8_t tank_command =
            (uint8_t)(*(enum tank_command*)vec_ref(instructions, i));

        vec_push(dat, &tank_command);
    }

    // convert each element in the data stream to network order
    for (size_t i = 0; i < vec_len(target_coords); i++) {
        struct coordinate* target = vec_ref(instructions, i);
        struct coordinate net_order_target = {
            .x = htonl(target->x),
            .y = htonl(target->y)
        };

        vec_pushn(dat, &net_order_target, sizeof(struct coordinate));
    }
    
    return;
}
void player_update_des(struct message *msg, const struct vector *dat) {
    // tank data is disjoint and total number of tanks isn't encoded directly in
    // the data stream. the number of tanks is found by calculating the amount
    // of space all the data for a single tank consumes, then dividing the byte
    // stream length by that amount.
    const int element_total_data =
        sizeof(enum tank_command) + sizeof(struct coordinate);

    const size_t num_tanks = vec_len(dat) / element_total_data;

    struct player_update* msg_data = &msg->player_update;
    
    // copy tank commands into message.
    // since tank_commands are only one byte, no endianness conversion is
    // necessary.
    vec_pushn(msg_data->tank_instructions, vec_ref(dat, 0), num_tanks);

    // copy data from network to a message object.  Converts from network order
    // to host order.
    for (size_t i = 0; i < num_tanks; i++) {
        const size_t targets_offset = num_tanks * sizeof(uint8_t);
        
        struct coordinate* net_order_target = vec_ref(dat, i + targets_offset);
        struct coordinate host_order_target = {
            .x = ntohl(net_order_target->x),
            .y = ntohl(net_order_target->y)
        };

        vec_push(msg_data->tank_target_coords, &host_order_target);
    }
        
    // copy tank targets into message
    vec_pushn(msg_data->tank_target_coords,
              vec_ref(dat, sizeof(enum tank_command)*num_tanks),
              num_tanks);

    return;
}
void player_update_init(struct message *msg) {
    // allocate members of the struct
    msg->player_update = (struct player_update) {
        .tank_instructions =    make_vector(sizeof(enum tank_command), 30),
        .tank_target_coords =   make_vector(sizeof(struct coordinate), 30)
    };
    return;    
}
void player_update_free(struct message *msg) {
    struct player_update* player_update = &msg->player_update;
    free_vector(player_update->tank_instructions);
    free_vector(player_update->tank_target_coords);

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
    struct vector* usernames = msg->scenario_tick.username_vecs;
    struct vector* tanks_pos = msg->scenario_tick.tank_positions;

    if (vec_len(usernames) != vec_len(tanks_pos))
        return; // FIXME this should be a return -1;

    // NUM_PLAYERS
    const uint8_t num_players = vec_len(usernames);

    vec_push(dat, &num_players);
    
    // NUM TANKS
    struct vector* first_tank_vec;
    vec_at(tanks_pos, 0, &first_tank_vec);
    const uint8_t num_tanks = vec_len(first_tank_vec) / vec_len(usernames);
    vec_push(dat, &num_tanks);

    // USERNAMES
    for (int n = 0; n < num_players; n++) {
        struct vector *username = vec_ref(usernames, n);
        vec_concat(dat, username);
    }

    // TANKS
    for (int n = 0; n < num_players; n++) {
        struct vector *tank_vec = vec_ref(tanks_pos, n);
        vec_concat(dat, tank_vec);
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

    const uint8_t* const dat_data = (const uint8_t *)vec_dat((struct vector*)dat);
    const uint8_t* data_start = dat_data + sizeof(uint8_t) * 2;
    const uint8_t* const data_end = dat_data + (vec_len(dat) * vec_element_len(dat));

    // USERNAMES
    for (int n = 0; n < num_players; n++) {
        // get length of string including null terminator
        size_t name_len = strnlen((char *)data_start, data_end - data_start) + 1;
        
        struct vector* username = make_vector(sizeof(char), name_len);
        vec_pushn(username, data_start, name_len);

        data_start += name_len;
        
        vec_push(msg->scenario_tick.username_vecs, &username);
    }

    // TANKS
    for (int t = 0; t < num_players; t++) {
        struct vector* tank_positions = make_vector(sizeof(struct coordinate), 30);

        vec_pushn(tank_positions, data_start, num_tanks);
        data_start += sizeof(struct coordinate) * num_tanks;

        vec_push(msg->scenario_tick.tank_positions, &tank_positions);
    }
}
void scenario_tick_init(struct message *msg) {
    msg->scenario_tick.username_vecs = make_vector(sizeof(struct vector*), 5);
    msg->scenario_tick.tank_positions = make_vector(sizeof(struct vector*), 5);
    return;
}
void scenario_tick_free(struct message *msg) {
    // gotta free all them strings...
    for (size_t n = 0; n < vec_len(msg->scenario_tick.username_vecs); n++) {
        struct vector *name_str = vec_ref(msg->scenario_tick.username_vecs, n);
        free_vector(name_str);
    }

    for (size_t n = 0; n < vec_len(msg->scenario_tick.tank_positions); n++) {
        struct vector *tank_vec = vec_ref(msg->scenario_tick.tank_positions, n);
        free_vector(tank_vec);
    }

    free_vector(msg->scenario_tick.username_vecs);
    free_vector(msg->scenario_tick.tank_positions);

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
        if (vec_len(msg.text) > 0)
            printf(" [txt] %s\n", (char *)vec_dat(msg.text));
        break;

    case MSG_REQUEST_AUTHENTICATE:
        printf(" [username] %s\n", (char*)vec_dat(msg.user_credentials.username));
        break;

    case MSG_RESPONSE_SCENARIO_TICK:
        printf(" [users] [%zu]\n", vec_len(msg.scenario_tick.username_vecs));
        for (size_t i = 0; i < vec_len(msg.scenario_tick.username_vecs); i++) {
            struct vector* username =
                vec_ref(msg.scenario_tick.username_vecs, i);
            printf("   %s\n\n", (char*)vec_dat(username));
        }

        printf(" [tanks] %zu\n", vec_len(msg.scenario_tick.tank_positions));
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
