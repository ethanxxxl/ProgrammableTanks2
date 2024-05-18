#include "scenario.h"
#include "vector.h"
#include <stdlib.h>

struct player_data make_player_data() {
    return (struct player_data) {
        .username = make_vector(sizeof(char), 10),
        .tanks = make_vector(sizeof(struct tank), 10)
    };
}

void free_player_data(struct player_data* pd) {
    free_vector(pd->username);
    free_vector(pd->tanks);

    // remove invalid pointers to avoid use after free errors.
    pd->username = NULL;
    pd->tanks = NULL;

    return;
}

struct player_public_data make_player_public_data() {
    return (struct player_public_data) {
        .username = make_vector(sizeof(char), 10),
        .tank_positions = make_vector(sizeof(struct coord), 10)
    };
}
struct player_public_data player_public_data_get(const struct player_data* pd) {
    struct player_public_data public_data;

    // copy username from player_data
    public_data.username = make_vector(sizeof(char), vec_len(pd->username));
    vec_pushn(public_data.username,
              vec_dat(pd->username),
              vec_len(pd->username));

    // copy tank positions from player data tanks
    public_data.tank_positions =
        make_vector(sizeof(struct coord), vec_len(pd->tanks));
    
    struct tank* last_tank = vec_end(pd->tanks);
    for (struct tank* t = vec_dat(pd->tanks); t <= last_tank; t++) {
        vec_push(public_data.tank_positions, &t->pos);
    } 

    return public_data;
}

void free_player_public_data(struct player_public_data* public_data) {
    free_vector(public_data->username);
    free_vector(public_data->tank_positions);
}
