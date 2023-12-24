#include <fcntl.h>
#include <message.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <vector.h>

const struct message_fns G_TEXT_FNS = {
    &text_ser,
    &text_des,
    &text_init,
    &text_free,
    NULL
};
const struct message_fns G_PLAYER_UPDATE_FNS = {
    &player_update_ser,
    &player_update_des,
    &player_update_init,
    &player_update_free,
    NULL
};
const struct message_fns G_USER_CREDENTIALS_FNS = {
    &user_credentials_ser,
    &user_credentials_des,
    &user_credentials_init,
    &user_credentials_free,
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
    [MSG_REQUEST_WORLD] = NULL,
    [MSG_REQUEST_PROPOSE_UPDATE] = &G_PLAYER_UPDATE_FNS,

    [MSG_REQUEST_DEBUG] = &G_TEXT_FNS,
    [MSG_REQUEST_RETURN_TO_LOBBY] = NULL,

    // RESPONSES
    [MSG_RESPONSE_WORLD_DATA] = NULL,

    [MSG_RESPONSE_SUCCESS] = &G_TEXT_FNS,
    [MSG_RESPONSE_FAIL] = &G_TEXT_FNS,
    [MSG_RESPONSE_INVALID_REQUEST] = &G_TEXT_FNS,

    [MSG_NULL] = NULL,
};

int message_send(int fd, const struct message msg) {
    struct {
        enum message_type t;
        int body_len;
    } header = {msg.type};

    struct vector body = {0};
    if (g_message_funcs[msg.type] != NULL) {
        make_vector(&body, sizeof(char), 10);

        g_message_funcs[msg.type]->message_ser(&msg, &body);
    }

    header.body_len = body.len;
    send(fd, &header, sizeof(header), 0);
    send(fd, &body.data, body.len, 0);

    return 0;
}

int message_recv(int fd, struct message* msg, struct vector* buf) {
    /* DETERMINE AMOUNT OF DATA TO READ */
    int body_size = 0;
    if (buf->len >= MESSAGE_HEADER_SIZE) {
        body_size = *(int *)(buf->data + sizeof(enum message_type));
    }

    int read_amnt = buf->len - MESSAGE_HEADER_SIZE + body_size;

    /* READ AS MUCH DATA AS POSSIBLE */
    vec_reserve(buf, buf->len + read_amnt);
    int bytes_read = recv(fd, vec_ref(buf, buf->len), read_amnt, 0);

    if (bytes_read <= 0)
        return -1; // recv can potentially return -1.

    buf->len += bytes_read; // update with number of bytes read.

    // update body_size to match header data, if able.
    if (buf->len >= MESSAGE_HEADER_SIZE) {
        body_size = *(int *)(buf->data + sizeof(enum message_type));
    }

    if (buf->len < MESSAGE_HEADER_SIZE ||
        buf->len < MESSAGE_HEADER_SIZE + body_size) {
        return -1;
    }

    /* CREATE MESSAGE, CLEAR BUFFER */
    *msg = (struct message){0}; // clear out garbage from msg struct.
    msg->type = *(enum message_type *)buf->data;

    // remove the header from the vector, so it only contains the body.
    for (int i = 0; i < MESSAGE_HEADER_SIZE; i++)
        vec_rem(buf, 0);

    if (g_message_funcs[msg->type] != NULL) {
        g_message_funcs[msg->type]->message_init(msg);
        g_message_funcs[msg->type]->message_des(msg, buf);
    }

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
    vec_pushn(&msg->text, dat->data, msg->text.len);
    vec_push(&msg->text, "\0"); // ensure null termination
}
void text_init(struct message* msg) {
    make_vector(&msg->text, sizeof(char), 10);
}
void text_free(struct message* msg) { free_vector(&msg->text); }

/* PLAYER_UPDATE_FNS
 * */
void player_update_ser(const struct message* msg, struct vector* dat) {}
void player_update_des(struct message* msg, const struct vector* dat) {}
void player_update_init(struct message* msg) {}
void player_update_free(struct message* msg) {}

/* USER_CREDENTIALS_FNS
 * */
void user_credentials_ser(const struct message* msg, struct vector* dat) {
    vec_pushn(dat, msg->user_credentials.username.data,
              msg->user_credentials.username.len);
}
void user_credentials_des(struct message* msg, const struct vector* dat) {
    vec_pushn(&msg->user_credentials.username, dat->data, dat->len);
    vec_push(&msg->user_credentials.username, "\0");
}
void user_credentials_init(struct message* msg) {
    make_vector(&msg->user_credentials.username, sizeof(char), 10);
}
void user_credentials_free(struct message* msg) {
    free_vector(&msg->user_credentials.username);
}

void print_hex(void *data, size_t len) {
    for (char *c = (char *)data, i = 1; c < (char *)data + len; c++, i++) {
        if (i % 16 == 0)
            printf("\n");

        printf("%02x ", *c);
    }
    printf("\n");
}

static const char *message_type_labels[] = {
    [MSG_REQUEST_AUTHENTICATE] = "MSG_REQUEST_AUTHENTICATE",
    [MSG_REQUEST_LIST_SCENARIOS] = "MSG_REQUEST_LIST_SCENARIOS",
    [MSG_REQUEST_CREATE_SCENARIO] = "MSG_REQUEST_CREATE_SCENARIO",
    [MSG_REQUEST_JOIN_SCENARIO] = "MSG_REQUEST_JOIN_SCENARIO",
    [MSG_REQUEST_WORLD] = "MSG_REQUEST_WORLD",
    [MSG_REQUEST_PROPOSE_UPDATE] = "MSG_REQUEST_PROPOSE_UPDATE",
    [MSG_REQUEST_DEBUG] = "MSG_REQUEST_DEBUG",
    [MSG_REQUEST_RETURN_TO_LOBBY] = "MSG_REQUEST_RETURN_TO_LOBBY",
    [MSG_RESPONSE_WORLD_DATA] = "MSG_RESPONSE_WORLD_DATA",
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

    default:
        break;
    }
}

int free_message(struct message msg) {
    if (g_message_funcs[msg.type] == NULL)
        return 0;

    g_message_funcs[msg.type]->message_free(&msg);
    return 0;
}
