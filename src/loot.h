#ifndef LOOT_H
#define LOOT_H
#include "r2d.h"
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
    int gold;                          // kasadaki altın (açılınca otomatik alınır)
} Loot;

// Boş slota bir loot (kasa) düşür; içerik verilen tablodan roll'lanır.
// Her tablo girdisi için ayrı zar atılır, silah/zırh plus'ı da burada roll'lanır.
// Başarılıysa true.
bool loot_spawn(Loot *loots, int maxLoot, float x, float y,
                const LootEntry *table, int tableCount, int goldMin, int goldMax);

// Tüm loot'ları çiz.
void loot_render_all(Loot *loots, int maxLoot, R2D *renderer, R2DTexture *crateTex, const Camera *camera);

// Dünya noktasındaki aktif kasanın slot indeksini döndürür (yoksa -1).
int loot_find_at(Loot *loots, int maxLoot, float worldX, float worldY);

// Kasaları güncelle (açılma animasyonu).
void loot_update_all(Loot *loots, int maxLoot, float dt);

void loot_popup_render(R2D *renderer, const Loot *loot,
                       R2DTexture *itemIconTex, const ItemTextures *itemTex,
                       TTF_Font *font, const Camera *camera);

// loot_popup_item_at'in özel dönüşü: altın hücresine tıklandı
#define LOOT_CELL_GOLD (-2)

// Popup'ta tıklanan hücre: item indeksi, LOOT_CELL_GOLD ya da -1 (dışarı).
int loot_popup_item_at(const Loot *loot, float worldX, float worldY);

// Kasa tamamen boş mu? (item'lar alınmış VE altın alınmış)
bool loot_is_empty(const Loot *loot);

#endif