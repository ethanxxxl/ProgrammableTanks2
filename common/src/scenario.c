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

struct vector *player_public_data_get_all(const struct vector *player_data) {
    if (player_data == NULL)
        return NULL;

    struct vector *all_pub_data = make_vector(sizeof(struct player_public_data),
                                          vec_len(player_data));
    if (all_pub_data == NULL)
        return NULL;

    for (size_t i = 0; i < vec_len(player_data); i++) {
        struct player_public_data pub_data =
            player_public_data_get(vec_ref(player_data, i));

        vec_push(all_pub_data, &pub_data);
        
        if (pub_data.tank_positions == NULL || pub_data.username == NULL)
            goto error_occured;
    }

    return all_pub_data;

 error_occured:
    free_all_player_public_data(all_pub_data);
    return NULL;
}

void free_all_player_public_data(struct vector *player_public_data) {
    if (player_public_data == NULL)
        return;
    
    for (size_t i = 0; i < vec_len(player_public_data); i++)
        free_player_public_data(vec_ref(player_public_data, i));

    free_vector(player_public_data);
    return;
}
