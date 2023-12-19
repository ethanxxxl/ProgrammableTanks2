#ifndef GAME_ERRORS_H
#define GAME_ERRORS_H

// master enumeration of all possible errors in the game.
enum GAME_ERROR_ENUM {
    GE_SUCCESS = 0,
    GE_UNKNOWN,
    GE_SCENARIO_PLAYER_EXISTS
};

void print_game_error(enum GAME_ERROR_ENUM);

#endif
