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
    float x, y;   // kameranın dünya uzayındaki sol üst köşesi
} Camera;

#endif // COMMON_H