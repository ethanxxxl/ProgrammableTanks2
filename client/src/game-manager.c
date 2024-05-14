#include "game-manager.h"
#include "message.h"
#include "tank.h"

#include <string.h>


extern struct vector* g_players;

void players_update_player(char *username, struct vector *tank_positions) {
    // see if the player exists in the structure.
    struct player *player = NULL;
    for (size_t p = 0; p < vec_len(g_players); p++) {
        player = vec_ref(g_players, p);
        int status = strcmp(username, player->username);

        if (status == 0)
            goto update_player;
    }
    // emulated for-else syntax from python
    // this is the else case, we didn't find the player.
    struct player new_player;
    strncpy(new_player.username, username, 50);
    new_player.tanks = make_vector(sizeof(struct tank), 30);

    vec_push(g_players, &new_player);
    player = vec_ref(g_players, vec_len(g_players) - 1);

update_player: ; // can't have a declaration after a label in cstd < c2x
    vec_resize(player->tanks, vec_len(tank_positions));

    for (size_t t = 0; t < vec_len(tank_positions); t++) {
        struct coordinate coord;
        vec_at(tank_positions, t, &coord);

        struct tank *tank = vec_ref(player->tanks, t);
        tank->x = coord.x;
        tank->y = coord.y;
    }

    return;
}
