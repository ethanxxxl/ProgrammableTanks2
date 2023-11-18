#ifndef TANK_H
#define TANK_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

typedef uint32_t tank_id_t;
struct xy_pos {
    int32_t x, y;
};

struct Tank {
    tank_id_t id;
    struct xy_pos pos;
    char name[50];   // consider changing these fields to smart strings in the future.
    char player[50];
    // TODO: fill with more fields.
};


extern struct Tank** g_tank_list;
extern size_t g_tank_list_size;
extern size_t g_tank_list_len;

int make_tank_id(void);
int get_num_tanks();
void push_tank(struct Tank *tank);
void delete_tank_list();
void init_tank_list(int initial_size);

#endif
