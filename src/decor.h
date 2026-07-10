#ifndef DECOR_H
#define DECOR_H
#include "r2d.h"
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
    bool solidBase;        // tabanı katı mı (ev/ağaç girer, çalı girmez)
    bool conceals;         // içine girilebilir gizlenme (çalı/saman — sneak örtüsü)
} DecorDef;

extern const DecorDef DECORS[];
extern const int DECOR_COUNT;

// Hepsini çiz (tilemap'ten sonra, entity'lerden önce)
void decor_render_all(R2D *renderer, R2DTexture **decorTextures,
                      const Camera *camera);

// Kutu herhangi bir dekorun katı tabanına çarpıyor mu?
// (taban = sprite'ın alt %35'i — çatının arkasından yürünebilir)
bool decor_blocks(SDL_FRect box);

// Nokta gizleyici bir dekorun (çalı/saman) içinde mi? (sneak örtüsü)
bool decor_conceals(float x, float y);

// Texture boyutları render/collision için gerekli — yükleme sonrası bir kez çağır
void decor_set_texture_sizes(R2DTexture **decorTextures);

#endif
