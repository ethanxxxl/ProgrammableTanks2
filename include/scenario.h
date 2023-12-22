#ifndef SCENARIO_H
#define SCENARIO_H

#include <player_manager.h>
#include <vector.h>

// the global scenario
extern struct scenario g_scenario;

enum tank_command {
    TANK_MOVE,
    TANK_FIRE,
    TANK_HEAL,
};

#define TANK_FIRE_DISTANCE 10
#define TANK_HEAL_RATE 15
#define TANK_MAX_SPEED 5
#define TANK_SHELL_DAMAGE 75
struct tank {
    int x,y;
    int health;
    enum tank_command cmd;

    int aim_at_x, aim_at_y;
    int move_to_x, move_to_y;
};

enum SCENARIO_OBJECTIVES {
    DEFEND_POSITION,
    ATTACK_POSITION,
    KILL_PLAYERS,
    OBSERVER,
};

struct scenario_map {
    int size_x, size_y;
};

#define TANKS_IN_SCENARIO 36
struct actor {
    struct player_manager *player;
    enum SCENARIO_OBJECTIVES objective;
    struct tank tanks[TANKS_IN_SCENARIO];
};

/* Scenario manager structure for now, objectives will be fixed and
   maps will be plain, (ie nonexistant)
 */
struct scenario {
    struct scenario_map map;
    struct vector actors;
    float tick_rate;
    int tick_number;
};

int make_scenario(struct scenario *scene);
int free_scenario(const struct scenario *scene);

/* Adds a new player to the scenario. The player will must choose
 an objective before it may begin the scenario.
*/
int scenario_add_player(struct scenario *scene, struct player_manager *player);
int scenario_rem_player(struct scenario *scene, struct player_manager *player);

/// Runs updates on everything in the scenario:
///  tank health
///  tank shooting
///  tank movement
int scenario_tick(struct scenario *scene);

/// responsible for the timing and communication with players.
/// returns 1 if nothing is done (not time for a scene update).
/// returns 0 if a scene update is done.
/// returns -1 in the case of an error.
int scenario_handler(struct scenario *scene);

#endif
