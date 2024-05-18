#include "scenario.h"
#include "server-scenario.h"
#include "message.h"
#include "vector.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

int make_scenario(struct scenario *scene) {
    scene->players = make_vector(sizeof(struct player_data), 10);
    
    if (scene->players == NULL) {
        return -1;
    }

    scene->player_managers = make_vector(sizeof(struct player_manager), 10);
    if  (scene->player_managers == NULL) {
        return -1;
    }

    scene->tick_rate = 0.75;
    scene->tick_number = 0;
    
    return 0;
}

int scenario_add_player(struct scenario* scene, struct player_manager* player) {
    struct player_data player_data = make_player_data();
    vec_pushn(player_data.username, player->username, strlen(player->username));

    struct tank default_tank = {0};
    
    for (int i = 0; i < TANKS_IN_SCENARIO; i++) {
        vec_push(player_data.tanks, &default_tank);
        default_tank.pos.x += 2;
    }
 
    vec_push(scene->players, &player_data);
    vec_push(scene->player_managers, &player);
    
    return 0;
}

int scenario_rem_player(struct scenario *scene, struct player_manager *player) {
    size_t player_idx;

    // find the actor corresponding to player.
    for (player_idx = 0; player_idx < vec_len(scene->players); player_idx++) {
        struct player_data* pd = vec_ref(scene->players, player_idx);
        
        if (strcmp(player->username, vec_dat(pd->username)) != 0)
            continue; // this isn't the player, keep looking.

        // found the player!
        vec_rem(scene->players, player_idx);
        vec_rem(scene->player_managers, player_idx);
        return 0;
    }

    // the player wasn't in this scene...
    return -1;
}

struct player_data* scenario_find_player(struct scenario *scene,
                                   struct player_manager *player) {
    // find the actor corresponding to player.
    for (size_t player_idx = 0; player_idx < vec_len(scene->players); player_idx++) {
        struct player_data* pd = vec_ref(scene->players, player_idx);

        if (strcmp(player->username, vec_dat(pd->username)) != 0)
            continue; // this isn't the player, keep looking.

        return vec_ref(scene->players, player_idx);
    }

    return NULL;
}

/// returns a reference to the tank in the scenario. Actor and Tank IDs are
/// simply their index in their respective arrays.
struct tank* scenario_get_tank(struct scenario *scene,
                               size_t player_idx,
                               size_t tank_id) {
    if (tank_id >= TANKS_IN_SCENARIO || player_idx >= vec_len(scene->players))
        return NULL;

    struct player_data* pd = vec_ref(scene->players, player_idx);
    return vec_ref(pd->tanks, tank_id);
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
    for (size_t p = 0; p < vec_len(scene->players); p++) {
        for (int t = 0; t < TANKS_IN_SCENARIO; t++) {
            struct tank* other = scenario_get_tank(scene, p, t);

            if (other->pos.x == tank->aim_at.x &&
                other->pos.y == tank->aim_at.y) {
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

    int x = tank->pos.x,
        y = tank->pos.y,
        xx = tank->move_to.x,
        yy = tank->move_to.y;
    
    float move_distance =
        sqrtf((powf(xx - x, 2) + powf(yy - y, 2)));

    if (move_distance <= TANK_MAX_SPEED) {
        tank->pos.x = xx;
        tank->pos.y = yy;
        return;
    }

    printf("tank moving too far (%f), doing a partial move.\n", move_distance);

    // otherwise, we can only move the max distance. move the tank
    // TANK_MAX_SPEED units in the direction of xx and yy.
    tank->pos.x += roundf(((xx - x) / move_distance) * TANK_MAX_SPEED);
    tank->pos.y += roundf(((yy - y) / move_distance) * TANK_MAX_SPEED);
    printf("move_to x/y: %d, %d\nnew x/y: %d, %d\n",
           xx, yy, tank->pos.x, tank->pos.y);
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
        for (size_t a = 0; a < vec_len(scene->players); a++) {
            scenario_heal_tank(scenario_get_tank(scene, a, t));
        }

        for (size_t a = 0; a < vec_len(scene->players); a++) {
            scenario_fire_tank(scene,
                               scenario_get_tank(scene, a, t));
        }

        for (size_t a = 0; a < vec_len(scene->players); a++) {
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

    // add public data to the message
    for (size_t p = 0; p < vec_len(scene->players); p++) {
        struct player_data* player = vec_ref(scene->players, p);
        struct vector* players_public = msg.scenario_tick.players_public_data;
        
        struct player_public_data pub_dat = player_public_data_get(player);
        vec_push(players_public, &pub_dat);
    }

    // send the newly created message.        
    for (size_t a = 0; a < vec_len(scene->players); a++) {
        struct player_manager* pm = vec_ref(scene->player_managers, a);
        

        #ifdef DEBUG
        struct sockaddr_in player_a = *(struct sockaddr_in *)(&actor.player->address);
        struct in_addr a = player_a.sin_addr;
        printf("socket: %d, addr: %s\n", actor.player->socket,
               inet_ntoa(a));
        #endif

        message_send(pm->socket, msg);
    }

    free_message(msg);
    
    return 0;
}
