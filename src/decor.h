#ifndef DECOR_H
#define DECOR_H
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "common.h"

// Dekor: dünyaya konan statik sprite'lar (ev, çit, ağaç...).
// Tabanları katıdır (oyuncu/mob giremez); NPC rutinleri ev kapılarına gider.
// İç mekanlar ileride: kapı tile'ı + ayrı harita.

typedef struct {
    const char *texPath;   // tek parça sprite (Cute_Fantasy building vb.)
    float x, y;            // TABAN ORTA noktası (dünya pikseli) — ayak hizası
    float scale;           // kaynak piksel → dünya çarpanı
} DecorDef;

extern const DecorDef DECORS[];
extern const int DECOR_COUNT;

// Hepsini çiz (tilemap'ten sonra, entity'lerden önce)
void decor_render_all(SDL_Renderer *renderer, SDL_Texture **decorTextures,
                      const Camera *camera);

// Kutu herhangi bir dekorun katı tabanına çarpıyor mu?
// (taban = sprite'ın alt %35'i — çatının arkasından yürünebilir)
bool decor_blocks(SDL_FRect box);

// Texture boyutları render/collision için gerekli — yükleme sonrası bir kez çağır
void decor_set_texture_sizes(SDL_Texture **decorTextures);

#endif
