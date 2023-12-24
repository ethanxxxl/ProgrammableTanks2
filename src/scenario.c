#include <message.h>
#include <vector.h>
#include <math.h>
#include <scenario.h>
#include <stdio.h>
#include <time.h>


int make_scenario(struct scenario *scene) {
    int status;
    
    status = make_vector(&scene->actors, sizeof(struct actor), 10);
    if (status != 0) {
        return -1;
    }

    scene->tick_rate = 0.75;
    scene->tick_number = 0;
    
    return 0;
}

int scenario_add_player(struct scenario *scene, struct player_manager *player) {
    struct actor a = {
        .tanks = {{0}}, // double braces b/c of a gcc bug apparently (suppresses superfluous warning)
        .player = player,
        .objective = OBSERVER,
    };

    vec_push(&scene->actors, &a);
    
    return 0;
}

int scenario_rem_player(struct scenario *scene, struct player_manager *player) {
    int actor_id;

    // find the actor corresponding to player.
    for (actor_id = 0; actor_id < scene->actors.len; actor_id++) {
        struct actor* a = vec_ref(&scene->actors, actor_id);
        if (a->player != player)
            continue; // this isn't the player, keep looking.

        // found the player!
        vec_rem(&scene->actors, actor_id);
        return 0;
    }

    // the player wasn't in this scene...
    return -1;
}

/// returns a reference to the tank in the scenario. Actor and Tank IDs are
/// simply their index in their respective arrays.
struct tank* scenario_get_tank(struct scenario *scene,
                               int actor_id,
                               int tank_id) {
    if (tank_id >= TANKS_IN_SCENARIO || actor_id >= scene->actors.len)
        return NULL;

    struct actor* actor = vec_ref(&scene->actors, actor_id);
    return &actor->tanks[tank_id];
}

void scenario_heal_tank(struct tank *tank) {
    if (tank->cmd != TANK_HEAL)
        return;

    tank->health += TANK_HEAL_RATE;
    
    if (tank->health > 100)
        tank->health = 100;

    return;
}

/// Friendly fire is on. tanks can also shoot themselves.
void scenario_fire_tank(struct scenario *scene, struct tank* tank) {
    if (tank->cmd != TANK_FIRE)
        return;

    struct tank* target = NULL;
    for (int p = 0; p < scene->actors.len; p++) {
        for (int t = 0; t < TANKS_IN_SCENARIO; t++) {
            struct tank* other = scenario_get_tank(scene, p, t);

            if (other->x == tank->aim_at_x &&
                other->y == tank->aim_at_y) {
                target = other;
                goto end_search; // I just want to break out of both
                // loops lol
            }
        }
    }
 end_search:
    if (target == NULL)
        return;

    target->health -= TANK_SHELL_DAMAGE;
    return;
}

void scenario_move_tank(struct tank *tank) {
    if (tank->cmd != TANK_MOVE)
        return;

    int x = tank->x,
        y = tank->y,
        xx = tank->move_to_x,
        yy = tank->move_to_y;
    
    float move_distance =
        sqrtf((powf(xx - x, 2) + powf(yy - y, 2)));

    if (move_distance <= TANK_MAX_SPEED) {
        tank->x = xx;
        tank->y = yy;
        return;
    }

    // otherwise, we can only move the max distance. move the tank
    // TANK_MAX_SPEED units in the direction of xx and yy.
    tank->x += roundf(((xx - x) / move_distance) * TANK_MAX_SPEED);
    tank->y += roundf(((yy - y) / move_distance) * TANK_MAX_SPEED);
    return;
}

int scenario_tick(struct scenario *scene) {
    /*
     0. get proposed updates
     1. validate proposed updates (first pass)
     2. update tank positions
     3. check for interferance (are there tanks on top of each other?)
     4. run weapons simulation
     5. return
     */

    for (int t = 0; t < TANKS_IN_SCENARIO; t++) {
        for (int a = 0; a < scene->actors.len; a++) {
            scenario_heal_tank(scenario_get_tank(scene, a, t));
        }

        for (int a = 0; a < scene->actors.len; a++) {
            scenario_fire_tank(scene,
                               scenario_get_tank(scene, a, t));
        }

        for (int a = 0; a < scene->actors.len; a++) {
            scenario_move_tank(scenario_get_tank(scene, a, t));
        }
    }
    
    return 0;
}

int scenario_handler(struct scenario *scene) {
    float current_time = (float)clock() / CLOCKS_PER_SEC;
    float next_tick_time = (1/scene->tick_rate) * scene->tick_number;

    if (current_time < next_tick_time)
        return 1; // exit early since it is not time for the next update.

    scenario_tick(scene);
    scene->tick_number++;

    char msg_text[30];
    snprintf(msg_text, 30, "game is on tick %d", scene->tick_number);

    printf(": %s\n", msg_text);

    // send updates to all the players
    for (int a = 0; a < scene->actors.len; a++) {
        struct actor actor;
        vec_at(&scene->actors, a, &actor);

        message_send_conf(actor.player->socket, MSG_RESPONSE_SUCCESS, msg_text);
    }
    
    return 0;
}
