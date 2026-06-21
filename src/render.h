#ifndef RENDER_H
#define RENDER_H

#include <SDL3/SDL.h>
#include "common.h"     // Camera

SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path);
void camera_update(Camera *camera, float targetX, float targetY, int winW, int winH);

#endif // RENDER_H