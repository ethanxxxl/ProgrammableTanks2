#include <message.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <tank.h>
#include <stdbool.h>
#include <netinet/in.h>

int send_message(int fd, const struct message* msg) {
    return send(fd, msg, sizeof(struct message), 0);
}

int recv_message(int fd, struct message* msg) {
    char buff[sizeof(struct message)];
    int ret = recv(fd, buff, sizeof(struct message), 0);
    memcpy(msg, buff, sizeof(struct message));

    return ret;
}
