#include "client-gfx.h"
#include "game-manager.h"

#include "vector.h"
#include "message.h"
#include "tank.h"

#include <stdbool.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

extern char g_bg_color;
extern bool g_gfx_running;

extern struct vector *g_players;
extern char g_username[50];

int g_map_height;
int g_map_width;
/// Draws the grid with dimensions on the surface, but only the portion that cam
/// is viewing.
void gfx_render_rect(SDL_Renderer* renderer, SDL_Rect* cam, SDL_Rect* rect) {
    int l_edge = rect->x - cam->x;
    int t_edge = rect->y - cam->y;
    int r_edge = (rect->x + rect->w) - cam->x;
    int b_edge = (rect->y + rect->h) - cam->y;

    if (l_edge < 0) l_edge = 0;            // | bounds checking
    if (t_edge < 0) t_edge = 0;            // |
    if (r_edge > cam->w) r_edge = cam->w;  // |
    if (b_edge > cam->h) b_edge = cam->h;  // |

    SDL_Rect the_thing = {
        .x = l_edge,
        .y = t_edge,
        .w = r_edge - l_edge,
        .h = b_edge - t_edge
    };

    SDL_RenderFillRect(renderer, &the_thing);
}

void gfx_render_grid(SDL_Renderer* renderer, SDL_Rect* cam, const SDL_Rect* tile,
                     int spacing, int rows, int cols, uint8_t fg[3], uint8_t bg[3]) {
    SDL_Rect drawn_tile = *tile;
    SDL_Rect canvas = {
        .x = drawn_tile.x - 5,
        .y = drawn_tile.y - 5,
        .w = cols * (drawn_tile.w + spacing) + 10,
        .h = rows * (drawn_tile.h + spacing) + 10
    };

    // clear the ground first
    SDL_SetRenderDrawColor(renderer, bg[0], bg[1], bg[2], SDL_ALPHA_OPAQUE);
    gfx_render_rect(renderer, cam, &canvas);

    // now draw the grid
    SDL_SetRenderDrawColor(renderer, fg[0], fg[1], fg[2], SDL_ALPHA_OPAQUE);
    for (int x = 0; x < cols; x++) {
        for (int y = 0; y < rows; y++) {
            gfx_render_rect(renderer, cam, &drawn_tile);

            drawn_tile.y += drawn_tile.h + spacing;
        }

        drawn_tile.y = canvas.y + 5;
        drawn_tile.x += drawn_tile.w + spacing;
    }
}

void gfx_draw_tank(SDL_Renderer *renderer, SDL_Rect *cam, const SDL_Rect *tile,
                   int spacing, int x, int y) {
    SDL_Rect tank_tile = {
        .x = x * (tile->w + spacing),
        .y = y * (tile->h + spacing),
        .w = tile->w,
        .h = tile->h
    };

    // TODO: Render different color for each player
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    gfx_render_rect(renderer, cam, &tank_tile);    
}

void *gfx_thread(void *arg) {
    (void)arg;
    
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    g_bg_color = 0xff;
    g_map_height = 50;
    g_map_width = 50;

    /*
    ** INITIALIZE SDL
     */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize!\n");
        printf("SDL_Error: %s\n", SDL_GetError());
        return NULL;
    };

    window = SDL_CreateWindow("SDL Tutorial",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              SCREEN_WIDTH,
                              SCREEN_HEIGHT,
                              SDL_WINDOW_SHOWN);

    if (window == NULL) {
        printf("SDL could not create a window!\n");
        printf("SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return NULL;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("SDL could not create a renderer!\n");
        printf("SLD_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return NULL;
    }

    /*
    ** LOAD MEDIA
     */
    uint8_t bg_colors[3] = {47, 196, 97};
    uint8_t fg_colors[3] = {88, 119, 140};

    /*
    ** RUN GFX LOOP
     */
    SDL_Rect camera = { .x = 0, .y = 0, .h = SCREEN_HEIGHT, .w = SCREEN_WIDTH};
    SDL_Rect tile = {.x = 0, .y=0, .h = 5, .w = 5};

    SDL_Event e;
    g_gfx_running = true;
    while (g_gfx_running) {
        SDL_SetRenderDrawColor(renderer, g_bg_color, g_bg_color, g_bg_color,
                               SDL_ALPHA_OPAQUE);

        SDL_RenderClear(renderer);
        const int grid_spacing = 1;
        gfx_render_grid(renderer, &camera, &tile, grid_spacing,
                        g_map_height, g_map_width, bg_colors, fg_colors);

        for (size_t p = 0; p < vec_len(g_players); p++) {
            struct player *player = vec_ref(g_players, p);

            for (size_t t = 0; t < vec_len(player->tanks); t++) {
                struct tank tank;
                vec_at(player->tanks, t, &tank);
                gfx_draw_tank(renderer, &camera, &tile, grid_spacing, tank.x, tank.y);
            }
        }
        
        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_MOUSEMOTION:
                // see if the user is panning
                if ((e.motion.state & SDL_BUTTON_LMASK) != SDL_BUTTON_LMASK)
                    break;

                camera.x -= e.motion.xrel;
                camera.y -= e.motion.yrel;
                break;
            case SDL_MOUSEWHEEL:
                tile.h += e.wheel.y;
                tile.w += e.wheel.y;
                break;
            case SDL_QUIT:
                g_gfx_running = false;
                break;
            default:
                break;
            }
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return NULL;
}
