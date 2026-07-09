#include "decor.h"

// Köy evleri — NPC'lerin gece gittiği kapılar bunların önü (npc.c rutinleri)
#define T(n) ((n) * (float)TILE_SIZE)
const DecorDef DECORS[] = {
    { .texPath = "assets/buildings/House_1_Limestone_Base_Red.png",
      .x = T(10), .y = T(4),  .scale = 1.6f },   // Baran'ın evi (dükkanın arkası)
    { .texPath = "assets/buildings/House_2_Limestone_Base_Blue.png",
      .x = T(14), .y = T(6),  .scale = 1.6f },   // Kemal'in evi
    { .texPath = "assets/buildings/House_3_Limestone_Base_Black.png",
      .x = T(6),  .y = T(10), .scale = 1.6f },   // Osman'ın evi
};
const int DECOR_COUNT = (int)(sizeof(DECORS) / sizeof(DECORS[0]));

// Texture boyutları (yüklemede bir kez okunur — çizim/collision tek kaynaktan)
static float texW[16], texH[16];

void decor_set_texture_sizes(SDL_Texture **decorTextures)
{
    for (int i = 0; i < DECOR_COUNT && i < 16; i++) {
        texW[i] = texH[i] = 0.0f;
        if (decorTextures[i] != NULL)
            SDL_GetTextureSize(decorTextures[i], &texW[i], &texH[i]);
    }
}

// Dekorun dünya-uzayı çizim kutusu (taban orta noktasından)
static SDL_FRect decor_rect(int i)
{
    float w = texW[i] * DECORS[i].scale;
    float h = texH[i] * DECORS[i].scale;
    SDL_FRect r = { DECORS[i].x - w / 2.0f, DECORS[i].y - h, w, h };
    return r;
}

void decor_render_all(SDL_Renderer *renderer, SDL_Texture **decorTextures,
                      const Camera *camera)
{
    float zoom = camera->zoom;
    for (int i = 0; i < DECOR_COUNT; i++) {
        if (decorTextures[i] == NULL) continue;
        SDL_FRect r = decor_rect(i);
        SDL_FRect dst = {
            (r.x - camera->x) * zoom, (r.y - camera->y) * zoom,
            r.w * zoom, r.h * zoom,
        };
        SDL_RenderTexture(renderer, decorTextures[i], NULL, &dst);
    }
}

bool decor_blocks(SDL_FRect box)
{
    for (int i = 0; i < DECOR_COUNT; i++) {
        if (texW[i] <= 0.0f) continue;
        SDL_FRect r = decor_rect(i);
        // Katı taban: alt %35 (üst kısım çatı — arkasından geçilebilir)
        SDL_FRect base = { r.x, r.y + r.h * 0.65f, r.w, r.h * 0.35f };
        if (box.x < base.x + base.w && box.x + box.w > base.x &&
            box.y < base.y + base.h && box.y + box.h > base.y)
            return true;
    }
    return false;
}
