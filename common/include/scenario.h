#ifndef PLAYER_H
#define PLAYER_H

#include "vector.h"
#include "nonstdint.h"

#include <stdint.h>


struct coord {
    s32 x, y;
};

enum tank_command {
    TANK_MOVE,
    TANK_FIRE,
    TANK_HEAL,
};

struct scenario_params {
    u32 tank_fire_distance;
    u32 tank_heal_rate;
    u32 tank_max_speed;
    u32 tank_shell_damage;
};

#define TANK_FIRE_DISTANCE 10
#define TANK_HEAL_RATE 15
#define TANK_MAX_SPEED 5
#define TANK_SHELL_DAMAGE 75

struct tank { // TODO change out these x/y fields with coordinate fields.
    u32 health;
    enum tank_command cmd;

    struct coord pos;
    struct coord aim_at;
    struct coord move_to;
};


// XXX idea: maybe you could have message history information here?

struct player_data {
    struct vector* username;
    struct vector* tanks;
};

struct player_data make_player_data();
void free_player_data(struct player_data* pd);

struct player_public_data {
    struct vector* username;
    struct vector* tank_positions;
};

struct player_public_data make_player_public_data();
struct player_public_data player_public_data_get(const struct player_data*);
void free_player_public_data(struct player_public_data *);

/** takes a vector of `struct player_data`, and returns a new vector of `struct
    player_pubic_data` */
struct vector *player_public_data_get_all(const struct vector *player_data);

/** frees all the `struct player_public_data` elements in the vector. */
void free_all_player_public_data(struct vector *player_public_data);
#endif
