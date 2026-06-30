#ifndef FX_H
#define FX_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#include "common.h"

#define MAX_PARTICLES 512

// Silah efekt tipi — silaha/ozellige gore secilir
typedef enum {
    WEAPON_FX_NONE = 0,    // efekt yok (normal silah)
    WEAPON_FX_PINK,        // pembe enerji (+8 tarzi)
    WEAPON_FX_FLAME,       // alev
} WeaponFxType;

// Tek bir partikul
typedef struct {
    float x, y;          // dunya konumu
    float vx, vy;        // hiz (px/sn)
    float life;          // kalan omur (sn)
    float maxLife;       // baslangic omru
    float size;          // baslangic boyutu (px, dunya olcegi)
    Uint8 r, g, b;       // renk
    bool active;
    WeaponFxType fxType;   // renk gecisi davranisi icin
} Particle;

// Partikul havuzu
typedef struct {
    Particle items[MAX_PARTICLES];
    int nextFree;        // dairesel arama ipucu
} ParticleSystem;



// Havuzu sifirla
void fx_init(ParticleSystem *fx);

// Tek partikul dogur (rastgele varyasyon CAGIRAN tarafindan verilir)
void fx_spawn(ParticleSystem *fx, float x, float y, float vx, float vy,
              float life, float size, Uint8 r, Uint8 g, Uint8 b, WeaponFxType fxType);

// Tum partikulleri ilerlet (hareket + omur + sonme)
void fx_update(ParticleSystem *fx, float dt);

// Tum partikulleri ciz (glow texture ile, kamera + zoom)
void fx_render(ParticleSystem *fx, SDL_Renderer *renderer,
               SDL_Texture *glowTex, const Camera *camera);

#endif