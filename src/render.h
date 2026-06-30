#ifndef RENDER_H
#define RENDER_H

#include <SDL3/SDL.h>
#include "common.h"     // Camera

SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path);
void camera_update(Camera *camera, float targetX, float targetY, int winW, int winH);

// Dunya koordinatini ekran koordinatina cevirir (zoom dahil)
static inline SDL_FRect world_to_screen(float worldX, float worldY, float worldW, float worldH, const Camera *cam) {
    return (SDL_FRect){
        .x = (worldX - cam->x) * cam->zoom,
        .y = (worldY - cam->y) * cam->zoom,
        .w = worldW * cam->zoom,
        .h = worldH * cam->zoom
    };
}

SDL_Texture *create_glow_texture(SDL_Renderer *renderer, int size);

#endif // RENDER_H