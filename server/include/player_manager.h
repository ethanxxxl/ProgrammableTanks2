#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H

#include <message.h>
#include <stdbool.h>
#include <sys/socket.h>

struct ringbuffer {
    void* data;
    size_t elem_size;
    int capacity;
    int read_head;
    int write_head;
};

int make_ringbuffer(int len, size_t elem_size);
int free_ringbuffer(struct ringbuffer *rb);

int ringbuffer_push(struct ringbuffer *rb, const void *item);
int ringbuffer_pop(struct ringbuffer *rb, void *item);

enum player_state {
    STATE_DISCONNECTED,
    STATE_IDLE,
    STATE_LOBBY,
    STATE_SCENARIO
};

struct player_manager {
    int socket;
    socklen_t size;
    struct sockaddr address;

    enum player_state state;
    char username[50];

    struct ringbuffer to_scenario;
};

int make_player_manager(struct player_manager *p);
void print_player(struct player_manager *p);

// recieves player messages from the network, sends them to the
// scenario
int player_handle_messages(struct player_manager *p);

int player_idle_handler(struct player_manager *p, struct message msg);
int player_lobby_handler(struct player_manager *p, struct message msg);
int player_scenario_handler(struct player_manager *p, struct message msg);

#endif
