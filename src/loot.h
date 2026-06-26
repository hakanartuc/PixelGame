#ifndef LOOT_H
#define LOOT_H
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "common.h"
#include "item.h"
#include "item_textures.h"

#define MAX_LOOT 20   // dünyada aynı anda max kaç kasa
#define MAX_LOOT_ITEMS 9   // dünyada aynı anda max kaç kasa

typedef enum {
    LOOT_CLOSED,
    LOOT_OPENING,
    LOOT_OPEN,
    LOOT_CLOSING,   // YENİ — kapanıyor (animasyon ters)
} LootState;

typedef struct {
    SDL_FRect rect;
    bool active;
    LootState state;
    int currentFrame;
    float animTimer;
    ItemStack items[MAX_LOOT_ITEMS];   // ← kasanın içindekiler
    int itemCount;
} Loot;

// Boş slota bir loot (kasa) düşür. Başarılıysa true.
bool loot_spawn(Loot *loots, int maxLoot, float x, float y);

// Tüm loot'ları çiz.
void loot_render_all(Loot *loots, int maxLoot, SDL_Renderer *renderer, SDL_Texture *crateTex, const Camera *camera);

// Dünya noktasındaki aktif kasanın slot indeksini döndürür (yoksa -1).
int loot_find_at(Loot *loots, int maxLoot, float worldX, float worldY);

// Kasaları güncelle (açılma animasyonu).
void loot_update_all(Loot *loots, int maxLoot, float dt);

void loot_popup_render(SDL_Renderer *renderer, const Loot *loot,
                       SDL_Texture *itemIconTex, const ItemTextures *itemTex,
                       TTF_Font *font, const Camera *camera);

int loot_popup_item_at(const Loot *loot, float worldX, float worldY);

bool loot_is_empty(const Loot *loot);

#endif