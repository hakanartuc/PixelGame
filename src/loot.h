#ifndef LOOT_H
#define LOOT_H
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "common.h"

#define MAX_LOOT 20   // dünyada aynı anda max kaç kasa

typedef enum {
    LOOT_CLOSED,
    LOOT_OPENING,
    LOOT_OPEN,
} LootState;

typedef struct {
    SDL_FRect rect;
    bool active;
    LootState state;
    int currentFrame;
    float animTimer;
} Loot;

// Boş slota bir loot (kasa) düşür. Başarılıysa true.
bool loot_spawn(Loot *loots, int maxLoot, float x, float y);

// Tüm loot'ları çiz.
void loot_render_all(Loot *loots, int maxLoot, SDL_Renderer *renderer, SDL_Texture *crateTex, const Camera *camera);

// Dünya noktasındaki aktif kasanın slot indeksini döndürür (yoksa -1).
int loot_find_at(Loot *loots, int maxLoot, float worldX, float worldY);

// Kasaları güncelle (açılma animasyonu).
void loot_update_all(Loot *loots, int maxLoot, float dt);

#endif