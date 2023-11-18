#include <tank.h>

struct Tank** g_tank_list;
size_t g_tank_list_size;  
size_t g_tank_list_len;

void init_tank_list(int initial_size) {
    g_tank_list = (struct Tank**)malloc(initial_size * sizeof(struct Tank*));
    g_tank_list_size = initial_size;
    g_tank_list_len = 0;
}

void delete_tank_list() {
    free(g_tank_list);
    g_tank_list_size = 0;
    g_tank_list_len = 0;
}

// pushes a new tank onto the tank list.
void push_tank(struct Tank* tank) {
    if (g_tank_list_len >= g_tank_list_size) {
        g_tank_list = (struct Tank**)realloc(g_tank_list, 2*g_tank_list_size);
        g_tank_list_size *= 2;
    }

    g_tank_list[g_tank_list_len++] = tank;
}

int get_num_tanks() {
    return g_tank_list_len;
}


int make_tank_id(void) { return get_num_tanks() + 1; }
