#ifndef COMMON_H
#define COMMON_H

#include <SDL3/SDL.h>   // SDL_FRect, Uint8 gibi tipler için


#define DEBUG_DRAW 0

// --- Harita boyut sabitleri (birçok modül kullanır) ---
#define TILE_SIZE 64    // bir tile kaç piksel
#define MAP_W     40    // harita genişliği (tile)
#define MAP_H     30    // harita yüksekliği (tile)

// --- Kamera: dünya uzayında bir görüş penceresi ---
typedef struct {
    float x;
    float y;
    float zoom;   // 1.0 = normal, 1.5 = %50 yakin
} Camera;


// 0..1 arası rastgele float — partikül saçılımı, hasar varyansı vb.
// (rand() tabanlı; oyun için yeterli, kripto için değil)
#include <stdlib.h>
static inline float randf(void)
{
    return (float)rand() / ((float)RAND_MAX + 1.0f);   // [0,1) — 1.0 dahil değil
}

// Bir animasyonun sheet üzerindeki tanımı (konum + boyut + zamanlama)
typedef struct {
    int x, y;             // sheet'te ilk karenin sol-üst köşesi (piksel)
    int frameW, frameH;   // bu animasyonun kare boyutu
    int frameCount;       // kaç kare
    float frameDuration;  // kare başına süre
    int feetY;            // karakterin ayağının hücre içindeki y'si (piksel)
} AnimClip;

#endif // COMMON_H