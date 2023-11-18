#include <message.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <tank.h>
#include <stdbool.h>
#include <netinet/in.h>

int send_client_message(int fd, const struct client_message* msg) {
    return send(fd, msg, sizeof(struct client_message), 0);
}

int recv_client_message(int fd, struct client_message* msg) {
    char buff[sizeof(struct client_message)];
    int ret = recv(fd, buff, sizeof(struct client_message), 0);
    memcpy(msg, buff, sizeof(struct client_message));

    return ret;
}

int send_server_message(int fd, const struct server_message* msg) {
    return send(fd, msg, sizeof(struct server_message), 0);
}

int recv_server_message(int fd, struct server_message* msg) {
    char buff[sizeof(struct server_message)];
    int ret = recv(fd, buff, sizeof(struct server_message), 0);
    memcpy(msg, buff, sizeof(struct server_message));

    return ret;
}
