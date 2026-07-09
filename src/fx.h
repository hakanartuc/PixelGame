#ifndef FX_H
#define FX_H

#include "r2d.h"
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "common.h"

#define MAX_PARTICLES 512

// Silah efekt tipi — silahin item verisinden gelir; ileride run basma bu degeri degistirecek
typedef enum {
    WEAPON_FX_NONE = 0,    // efekt yok (normal silah)
    WEAPON_FX_PINK,        // pembe enerji (+8 tarzi)
    WEAPON_FX_FLAME,       // alev
    WEAPON_FX_ICE,         // buz (mavi-beyaz, agir dusen kristaller)
    WEAPON_FX_POISON,      // zehir (yesil, yukselen kabarciklar)
    WEAPON_FX_COUNT,       // sayac (onizleme dongusu icin)
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

// Yone gore kilic bolgesinin karakter merkezine gore kabaca ofseti (dunya px)
// LPC'de kilic sag elde; yone gore govdenin su tarafinda.
// facing: UP=0, LEFT=1, DOWN=2, RIGHT=3
static const struct { float dx, dy; } WEAPON_FX_OFFSET[4] = {
    {  -15.0f, -2.0f },  // UP    — kilic ust-sagda (arkada)
    { -14.0f,  -2.0f },  // LEFT  — kilic solda
    { -4.0f,  15.0f },  // DOWN  — kilic sol-onde
    {  14.0f,  -2.0f },  // RIGHT — kilic sagda
};

// Slash sirasinda kilicin kare-kare konumu (yon x kare, sprite px, karakter merkezine gore).
// Longsword oversize bloklarinin alpha-agirlikli merkezlerinden uretildi — savurma yayini izler.
static const struct { float dx, dy; } WEAPON_SLASH_FX_OFFSET[4][6] = {
    { {  -2.5f,   8.0f}, { -10.3f,  12.0f}, { -14.8f,   7.3f}, { -22.7f,   0.8f}, {   9.7f, -24.2f}, {  44.4f,  -6.5f} }, // UP
    { { -22.6f,  20.0f}, { -11.1f,  16.1f}, {   1.5f,  10.6f}, {   2.6f,  10.0f}, { -53.4f,  -0.2f}, { -37.0f,  -5.9f} }, // LEFT
    { {  -2.5f,  21.0f}, { -10.3f,  14.0f}, { -14.8f,   4.7f}, { -19.7f,  11.2f}, {   9.7f,  36.2f}, {  44.3f,  18.5f} }, // DOWN
    { {  21.6f,  20.0f}, {  10.1f,  16.1f}, {  -2.5f,  10.6f}, {  -3.7f,  10.0f}, {  52.4f,  -0.2f}, {  36.0f,  -5.9f} }, // RIGHT
};



// Havuzu sifirla
void fx_init(ParticleSystem *fx);

// Tek partikul dogur (rastgele varyasyon CAGIRAN tarafindan verilir)
void fx_spawn(ParticleSystem *fx, float x, float y, float vx, float vy,
              float life, float size, Uint8 r, Uint8 g, Uint8 b, WeaponFxType fxType);

// Tum partikulleri ilerlet (hareket + omur + sonme)
void fx_update(ParticleSystem *fx, float dt);

// Tum partikulleri ciz (glow texture ile, kamera + zoom)
void fx_render(ParticleSystem *fx, R2D *renderer,
               R2DTexture *glowTex, const Camera *camera);

#endif