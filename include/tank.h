#ifndef TANK_H
#define TANK_H

/// this file simply provides tank definitions for use across both the
/// server and client.

enum tank_command {
    TANK_MOVE,
    TANK_FIRE,
    TANK_HEAL,
};

#define TANK_FIRE_DISTANCE 10
#define TANK_HEAL_RATE 15
#define TANK_MAX_SPEED 5
#define TANK_SHELL_DAMAGE 75
struct tank { // TODO change out these x/y fields with coordinate fields.
    int x,y;
    int health;
    enum tank_command cmd;

    int aim_at_x, aim_at_y;
    int move_to_x, move_to_y;
};

#endif
