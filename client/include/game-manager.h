#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include "vector.h"

// this translation unit will provide functions and structures that control the
// player and game objects of the player.  This will be a sort of library for
// controlling the game.

struct player {
    char username[50];
    struct vector* tanks;
};

void players_update_player(char *username, struct vector *tank_positions);

#endif
