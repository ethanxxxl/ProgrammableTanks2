#include <fcntl.h>
#include <vector.h>
#include <message.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <netinet/in.h>


// TODO: there too many places where you need to update things. too many switch
// case statements. you need to simplify this stuff.

int message_len(const struct message msg) {
    switch (msg.type) {
    case MSG_RESPONSE_SUCCESS:
    case MSG_RESPONSE_FAIL:
    case MSG_RESPONSE_INVALID_REQUEST:
	return msg.text.len;

    case MSG_REQUEST_AUTHENTICATE:
	return strlen(msg.user_credentials.username);
    case MSG_REQUEST_DEBUG:
	return strlen(msg.debug_msg);
    default:
	return 0;
    }
}

// TODO: there will need to be helper functions for some message types that have
// union data.

/// sends message
int send_message(int fd, const struct message msg) {
    int header_size = 0;

    int header[] = {msg.type, message_len(msg)};
    header_size = send(fd, header, sizeof(header), 0); // just send header
    
    // return error in this case.
    if (header_size < 0) {
	return -1;
    }

    // send the body of the message, keeping track of its size.
    int body_size = 0;
    switch (msg.type) {
    case MSG_RESPONSE_SUCCESS:
    case MSG_RESPONSE_FAIL:
    case MSG_RESPONSE_INVALID_REQUEST:
	if (msg.text.len > 0) {
	    body_size = send(fd, msg.text.data, msg.text.len, 0);
	}
	break;

    case MSG_REQUEST_AUTHENTICATE:
	body_size = send(fd, msg.user_credentials.username,
			 strlen(msg.user_credentials.username), 0);
	break;
    case MSG_REQUEST_DEBUG:
	body_size = send(fd, msg.debug_msg, strlen(msg.debug_msg), 0);
    default:
	break;
    }

    if (body_size < 0) {
	return -1;
    }

    return header_size + body_size;
}

/// sends a SUCCESS, FAIL, or INVALID response. This function doesn't require a
/// vector to be initialized.
int send_conf_message(int fd, enum message_type type, const char* text) {
    if (type != MSG_RESPONSE_SUCCESS
	&& type != MSG_RESPONSE_FAIL
	&& type != MSG_RESPONSE_INVALID_REQUEST)
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

void print_hex(void *data, size_t len) {
    for (char *c = (char *)data, i = 1; c < (char *)data + len;
         c++, i++) {
	if (i % 16 == 0)
	    printf("\n");
	
	printf("%02x ", *c);
    }
    printf("\n");
}

/// creates new message and recieves data into it. Some messages require malloc,
/// so be sure to call free_message() before msg goes out of scope.
///
/// this function is non-blocking, and if the message length hasn't been read
/// into buffer yet, it will continue reading into that buffer until the proper
/// length is achieved.
#define HEADER_SIZE (sizeof(enum message_type) + sizeof(int))
int recv_message(int fd, struct message *msg, struct vector *buf) {
    /* DETERMINE AMOUNT OF DATA TO READ */
    int body_size = 0;
    if (buf->len >= HEADER_SIZE) {
	body_size = *(int*)(buf->data + sizeof(enum message_type));
    }

    int read_amnt = buf->len - HEADER_SIZE + body_size;

    /* READ AS MUCH DATA AS POSSIBLE */
    vec_reserve(buf, buf->len + read_amnt);
    int bytes_read = recv(fd, vec_ref(buf, buf->len), read_amnt, 0);
    
    if (bytes_read <= 0)
	return -1; // recv can potentially return -1.

    buf->len += bytes_read; // update with number of bytes read.
    
    // update body_size to match header data, if able.
    if (buf->len >= HEADER_SIZE) {
	body_size = *(int*)(buf->data + sizeof(enum message_type));
    }

    if (buf->len < HEADER_SIZE ||
	buf->len < HEADER_SIZE + body_size) {
	return -1;
    }

    /* CREATE MESSAGE, CLEAR BUFFER */
    *msg = (struct message){0}; // clear out garbage from msg struct.
    msg->type = *(enum message_type*)buf->data;
    
    // fill out msg appropriately
    switch (msg->type) {
    case MSG_RESPONSE_SUCCESS:
    case MSG_RESPONSE_FAIL:
    case MSG_RESPONSE_INVALID_REQUEST:
	make_vector(&msg->text, sizeof(char), body_size);
	vec_pushn(&msg->text, buf->data + HEADER_SIZE, body_size);
	vec_push(&msg->text, "\0"); // ensure null termination
	break;

    case MSG_REQUEST_AUTHENTICATE:
	vec_resize(buf, 50 + HEADER_SIZE);
	
        memmove(msg->user_credentials.username, buf->data + HEADER_SIZE, 50);
	break;
    default:
	break;
    }

    vec_resize(buf, 0);
    return 0;
}

static const char* message_type_labels[] = {
    [MSG_REQUEST_AUTHENTICATE]     = "MSG_REQUEST_AUTHENTICATE",
    [MSG_REQUEST_LIST_SCENARIOS]   = "MSG_REQUEST_LIST_SCENARIOS",
    [MSG_REQUEST_CREATE_SCENARIO]  = "MSG_REQUEST_CREATE_SCENARIO",
    [MSG_REQUEST_JOIN_SCENARIO]    = "MSG_REQUEST_JOIN_SCENARIO",
    [MSG_REQUEST_WORLD]            = "MSG_REQUEST_WORLD",
    [MSG_REQUEST_PROPOSE_UPDATE]   = "MSG_REQUEST_PROPOSE_UPDATE",
    [MSG_REQUEST_DEBUG]            = "MSG_REQUEST_DEBUG",
    [MSG_REQUEST_RETURN_TO_LOBBY]  = "MSG_REQUEST_RETURN_TO_LOBBY",
    [MSG_RESPONSE_WORLD_DATA]      = "MSG_RESPONSE_WORLD_DATA",
    [MSG_RESPONSE_SUCCESS]         = "MSG_RESPONSE_SUCCESS",
    [MSG_RESPONSE_FAIL]            = "MSG_RESPONSE_FAIL",
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
	    printf(" [txt] %s\n", (char*)msg.text.data);
	break;

    case MSG_REQUEST_AUTHENTICATE:
	printf(" [username] %s\n", msg.user_credentials.username);
	
    default:
	break;
    }
}

int free_message (struct message msg) {
    switch (msg.type) {
    case MSG_RESPONSE_SUCCESS:
    case MSG_RESPONSE_FAIL:
    case MSG_RESPONSE_INVALID_REQUEST:
	free_vector(&msg.text);
        break;
    default:
        break;
    }

    // FIXME: should return an actual value.
    return 0;
}
