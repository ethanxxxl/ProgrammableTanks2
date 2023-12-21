#ifndef SCENARIO_H
#define SCENARIO_H

#include <player_manager.h>
#include <vector.h>

// the global scenario
extern struct scenario g_scenario;

struct new_tank {
    int x,y;
    int player_id;
    int last_command;
};

enum SCENARIO_OBJECTIVES {
    DEFEND_POSITION,
    ATTACK_POSITION,
    KILL_PLAYERS,
    OBSERVER,
};

#define TANKS_IN_SCENARIO 36
struct actor {
    struct player *player;
    enum SCENARIO_OBJECTIVES objective;
    struct new_tank tanks[TANKS_IN_SCENARIO];
};

/* Scenario manager structure for now, objectives will be fixed and
   maps will be plain, (ie nonexistant)
 */
struct scenario {
    struct vector actors;
    struct vector tanks;
};

int make_scenario(struct scenario *scene);
int free_scenario(const struct scenario *scene);

/* Adds a new player to the scenario. The player will must choose
 an objective before it may begin the scenario.
*/
int scenario_add_player(struct scenario *scene, struct player *player);

int scenario_tick(struct scenario *scene);

#endif
