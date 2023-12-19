#include "game_errors.h"
#include "vector.h"
#include <scenario.h>


int make_scenario(struct scenario *scene) {
    int status;
    
    status = make_vector(&scene->actors, sizeof(struct actor), 10);
    if (status != 0) {
	return -1;
    }
    status = make_vector(&scene->tanks, sizeof(struct new_tank), 10);
    if (status != 0) {
	return -1;
    }

    return GE_SUCCESS;
}

int scenario_add_player(struct scenario *scene, struct player *player) {

    struct actor a = {
      .tanks = {0},
      .player = player,
      .objective = OBSERVER,
    };

    vec_push(&scene->actors, &a);
    
    return GE_SUCCESS;
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
    
    for (int i = 0; i < scene->actors.len; i++) {
	
    }
    
    return GE_SUCCESS;
}

int scenario_handle_player(struct scenario* scene, int player_num);
