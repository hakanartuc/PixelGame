#ifndef RENDER_H
#define RENDER_H

#include <SDL3/SDL.h>
#include "common.h"     // Camera
#include "r2d.h"        // R2D / R2DTexture (SDL_Renderer'ın yerini aldı)

R2DTexture *load_texture(R2D *renderer, const char *path);
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

R2DTexture *create_glow_texture(R2D *renderer, int size);

#endif // RENDER_H