#ifndef CLIENT_GFX_H
#define CLIENT_GFX_H

#include <SDL2/SDL.h>

void gfx_render_rect(SDL_Renderer *renderer, SDL_Rect *cam, SDL_Rect *rect);

void gfx_render_grid(SDL_Renderer *renderer, SDL_Rect *cam,
                     const SDL_Rect *tile, int spacing, int rows, int cols,
                     uint8_t fg[3], uint8_t bg[3]);

void gfx_draw_tank(SDL_Renderer *renderer, SDL_Rect *cam, const SDL_Rect *tile,
                   int spacing, int x, int y);

void *gfx_thread(void *arg);
#endif
