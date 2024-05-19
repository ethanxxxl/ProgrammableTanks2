#include "message.h"
#include "scenario.h"
#include "vector.h"
#include "nonstdint.h"


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

    write(fd, &header, sizeof(header));
    write(fd, vec_dat(body), vec_len(body));

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
    int bytes_read = read(fd, tmp, read_amnt);

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
        struct coord* target = vec_ref(target_coords, i);
        struct coord net_order_target = {
            .x = htonl(target->x),
            .y = htonl(target->y)
        };

        vec_pushn(dat, &net_order_target, sizeof(struct coord));
    }
    
    return;
}
void player_update_des(struct message *msg, const struct vector *dat) {
    // tank data is disjoint and total number of tanks isn't encoded directly in
    // the data stream. the number of tanks is found by calculating the amount
    // of space all the data for a single tank consumes, then dividing the byte
    // stream length by that amount.
    const int element_total_data =
        sizeof(u8) + sizeof(struct coord); // tank commands are sent as a u8

    const size_t num_tanks = vec_len(dat) / element_total_data;

    struct player_update* msg_data = &msg->player_update;

    for (size_t cmd = 0; cmd < num_tanks; cmd++) {
        enum tank_command tc = (enum tank_command)*(u8*)vec_ref(dat, cmd);
        vec_push(msg_data->tank_instructions, &tc);
    }

    // copy data from network to a message object.  Converts from network order
    // to host order.
    for (size_t i = 0; i < num_tanks; i++) {
        const size_t targets_offset = num_tanks * sizeof(uint8_t);
        
        struct coord* net_order_target = vec_ref(dat, targets_offset + i*sizeof(struct coord));
        struct coord host_order_target = {
            .x = ntohl(net_order_target->x),
            .y = ntohl(net_order_target->y)
        };

        vec_push(msg_data->tank_target_coords, &host_order_target);
    }
        
    return;
}
void player_update_init(struct message *msg) {
    // allocate members of the struct
    msg->player_update = (struct player_update) {
        .tank_instructions =    make_vector(sizeof(enum tank_command), 30),
        .tank_target_coords =   make_vector(sizeof(struct coord), 30)
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
 */
void scenario_tick_ser(const struct message *msg, struct vector *dat) {
    struct vector* players_data = msg->scenario_tick.players_public_data;

    // USERNAMES section
    for (size_t i = 0; i < vec_len(players_data); i++) {
        struct player_public_data* player = vec_ref(players_data, i);
        vec_pushn(dat, vec_dat(player->username), vec_len(player->username));
        vec_push(dat, ",");
    }

    // change last ',' to a '\0'
    vec_pop(dat, NULL);
    vec_push(dat, "\0");

    // NUM_TANKS section
    for (size_t i = 0; i < vec_len(players_data); i++) {
        struct player_public_data* player = vec_ref(players_data, i);
        u32 num_tanks_host = vec_len(player->tank_positions);
        u32 num_tanks_net = htonl(num_tanks_host);

        vec_pushn(dat, &num_tanks_net, sizeof(u32));
    }

    // TANK_POSITIONS section
    for (size_t i = 0; i < vec_len(players_data); i++) {
        struct player_public_data* player = vec_ref(players_data, i);

        for (size_t t = 0; t < vec_len(player->tank_positions); t++) {
            struct coord* pos_host = vec_ref(player->tank_positions, t);
            struct coord pos_net = {
                .x = htonl(pos_host->x),
                .y = htonl(pos_host->y)
            };
            
            vec_pushn(dat, &pos_net, sizeof(struct coord));
        }
    }

    return;
}

void scenario_tick_des(struct message *msg, const struct vector *dat) {
    struct vector* players_data = msg->scenario_tick.players_public_data;
    // USERNAMES section
    size_t num_tanks_offset = 0;
    for (const char* c = vec_ref(dat, 0); *c != '\0'; c++) {
        const char* c_end = c;
        while (*c_end != ',' && *c_end != '\0') c_end++;

        // initialize new player
        struct player_public_data p = make_player_public_data();

        vec_pushn(p.username, c, c_end-c); // copy username
        vec_push(players_data, &p);        // push player data onto vector
        
        num_tanks_offset += c_end - c + 1; // add one to account for the ','
        c = c_end;
    }

    size_t num_players = vec_len(players_data);
    size_t tank_positions_offset = num_tanks_offset + sizeof(u32)*num_players;
    
    // TANK POSITIONS section
    for (size_t i = 0; i < num_players; i++) {
        struct player_public_data* p = vec_ref(players_data, i);

        // find the number of tanks from the NUM TANKS section
        u32 player_num_tanks = *(u32*)vec_ref(dat, num_tanks_offset);
        player_num_tanks = ntohl(player_num_tanks);

        // convert endianness and copy positions into array
        for (size_t t = 0; t < player_num_tanks; t++) {
            struct coord* net_order_pos =
                vec_ref(dat, tank_positions_offset + t*sizeof(struct coord));

            struct coord host_order_pos = {
                .x = ntohl(net_order_pos->x),
                .y = ntohl(net_order_pos->y)
            };

            vec_push(p->tank_positions, &host_order_pos);
        }

        // make the position offset start at the next players tanks
        tank_positions_offset += sizeof(struct coord) * player_num_tanks;
    }
}
void scenario_tick_init(struct message *msg) {
    msg->scenario_tick.players_public_data =
        make_vector(sizeof(struct player_public_data), 5);

    return;
}
void scenario_tick_free(struct message *msg) {
    struct vector* players_data = msg->scenario_tick.players_public_data;

    // free every object in the vector
    for (size_t i = 0; i < vec_len(players_data); i++) {
        free_player_public_data(vec_ref(players_data, i));
    }

    // free the vector storing players data.
    free_vector(players_data);
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
        
        printf(" [users] [%zu]\n", vec_len(msg.scenario_tick.players_public_data));
        for (size_t i = 0; i < vec_len(msg.scenario_tick.players_public_data); i++) {
            struct player_public_data* player =
                vec_ref(msg.scenario_tick.players_public_data, i);
            
            printf("   %s\n\n", (char*)vec_dat(player->username));
        }

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
