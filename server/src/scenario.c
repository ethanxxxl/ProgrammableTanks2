#include <arpa/inet.h>
#include <message.h>
#include <netinet/in.h>
#include <stddef.h>
#include <string.h>
#include <vector.h>
#include <math.h>
#include <scenario.h>
#include <stdio.h>
#include <time.h>

int make_scenario(struct scenario *scene) {
    
    scene->actors = make_vector(sizeof(struct actor), 10);
    if (scene->actors == NULL) {
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

    vec_push(scene->actors, &a);
    
    return 0;
}

int scenario_rem_player(struct scenario *scene, struct player_manager *player) {
    size_t actor_id;

    // find the actor corresponding to player.
    for (actor_id = 0; actor_id < vec_len(scene->actors); actor_id++) {
        struct actor* a = vec_ref(scene->actors, actor_id);
        if (a->player != player)
            continue; // this isn't the player, keep looking.

        // found the player!
        vec_rem(scene->actors, actor_id);
        return 0;
    }

    // the player wasn't in this scene...
    return -1;
}

struct actor *scenario_find_actor(struct scenario *scene,
                                  struct player_manager *player) {
    // find the actor corresponding to player.
    for (size_t actor_id = 0; actor_id < vec_len(scene->actors); actor_id++) {
        struct actor* a = vec_ref(scene->actors, actor_id);
        if (a->player != player)
            continue; // this isn't the player, keep looking.

        return vec_ref(scene->actors, actor_id);
    }

    return NULL;
}

/// returns a reference to the tank in the scenario. Actor and Tank IDs are
/// simply their index in their respective arrays.
struct tank* scenario_get_tank(struct scenario *scene,
                               size_t actor_id,
                               size_t tank_id) {
    if (tank_id >= TANKS_IN_SCENARIO || actor_id >= vec_len(scene->actors))
        return NULL;

    struct actor* actor = vec_ref(scene->actors, actor_id);
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
    for (size_t p = 0; p < vec_len(scene->actors); p++) {
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

    printf("tank moving too far (%f), doing a partial move.\n", move_distance);

    // otherwise, we can only move the max distance. move the tank
    // TANK_MAX_SPEED units in the direction of xx and yy.
    tank->x += roundf(((xx - x) / move_distance) * TANK_MAX_SPEED);
    tank->y += roundf(((yy - y) / move_distance) * TANK_MAX_SPEED);
    printf("move_to x/y: %d, %d\nnew x/y: %d, %d\n",
           xx, yy, tank->x, tank->y);
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
        for (size_t a = 0; a < vec_len(scene->actors); a++) {
            scenario_heal_tank(scenario_get_tank(scene, a, t));
        }

        for (size_t a = 0; a < vec_len(scene->actors); a++) {
            scenario_fire_tank(scene,
                               scenario_get_tank(scene, a, t));
        }

        for (size_t a = 0; a < vec_len(scene->actors); a++) {
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

    //printf(": %s\n", msg_text);

    // send updates to all the players
    // create message to send to players
    struct message msg;
    make_message(&msg, MSG_RESPONSE_SCENARIO_TICK);

    for (size_t u = 0; u < vec_len(scene->actors); u++) {
        const struct actor* actor = vec_ref(scene->actors, u);

        // FIXME: the username may not always be limited to 50 chars.
        struct vector* username =  make_vector(sizeof(char), 50);
        size_t name_len = strnlen(actor->player->username, 50);
        vec_pushn(username, actor->player->username, name_len + 1); // include null terminator

        vec_push(msg.scenario_tick.username_vecs, &username);

        // add tanks
        struct vector* tank_vec =
            make_vector(sizeof(struct coordinate), TANKS_IN_SCENARIO);
        
        for (size_t t = 0; t < TANKS_IN_SCENARIO; t++) {
            struct tank this_tank = actor->tanks[t];
            struct coordinate pos_coord = { .x = this_tank.x, .y = this_tank.y };
            
            vec_push(tank_vec, &pos_coord);
        }
        vec_push(msg.scenario_tick.tank_positions, &tank_vec);
    }

    // send the newly created message.        
    for (size_t a = 0; a < vec_len(scene->actors); a++) {
        struct actor actor;
        vec_at(scene->actors, a, &actor);

        #ifdef DEBUG
        struct sockaddr_in player_a = *(struct sockaddr_in *)(&actor.player->address);
        struct in_addr a = player_a.sin_addr;
        printf("socket: %d, addr: %s\n", actor.player->socket,
               inet_ntoa(a));
        #endif

        message_send(actor.player->socket, msg);
    }

    free_message(msg);
    
    return 0;
}
