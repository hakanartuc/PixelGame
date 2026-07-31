#ifndef HOTBAR_H
#define HOTBAR_H
#include "r2d.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "ability.h"
#include "item.h"
#include "item_textures.h"
#include "inventory.h"

// WoW düzeni hotbar (Hakan tasarımı): 2 ANA BAR alt alta (8 kolon) +
// iki YANDA dikey ek bar (2 kolon × 4 satır) — hepsi AYNI ANDA görünür.
//   slot  0-7  alt bar    → 1-8 (ya da tıklama)
//   slot  8-15 üst bar    → Shift+1-8
//   slot 16-23 sağ dikey  → Alt/Option+1-8
//   slot 24-31 sol dikey  → tuşsuz (tıklamayla kullanılır)
// Tıklama = becerine BAS (click-to-cast); taşıma/takas yalnız SHIFT basılı
// sürüklemeyle; kaldırma yalnız ✕ hedefine bırakınca (dışarı = İPTAL).

#define HOTBAR_COLS 8
#define HOTBAR_SLOTS 32

typedef enum {
    HOTBAR_EMPTY = 0,
    HOTBAR_SKILL,    // aktif skill (abilityId geçerli)
    HOTBAR_ITEM,     // tüketilebilir tür (itemType geçerli)
} HotbarKind;

typedef struct {
    HotbarKind kind;
    AbilityId abilityId;
    const ItemType *itemType;
} HotbarSlot;

typedef struct {
    HotbarSlot slots[HOTBAR_SLOTS];
} Hotbar;

// AĞAÇTAN açılmış (AKTİF düğüm satın alınmış) ama barda olmayan ability'leri
// ilk boş slotlara yerleştir. Başlangıçta, her düğüm alımı ve level-up sonrası çağrılır.
void hotbar_place_unlocks(Hotbar *hb, const bool abilityOpen[ABILITY_COUNT]);

// Respec sonrası: artık açık olmayan aktifleri bardan kaldır
void hotbar_purge_closed(Hotbar *hb, const bool abilityOpen[ABILITY_COUNT]);

// Tüketilebilir türü ilk boş slota ata (zaten bardaysa dokunma). Başarılıysa true.
bool hotbar_assign_item(Hotbar *hb, const ItemType *type);

// Bar çizimi: alt-orta, tuş numarası + kısaltma/ikon + cooldown perdesi + adet.
// abilityCd: ability başına, itemCd: item TÜRÜ başına kalan bekleme (Player'dan).
// Ekran noktasındaki hotbar hücresi (-1 = yok) — AKTİF SIRANIN slot indeksi
// (row*10+col döner). removeZone: sürükleme sırasında ✕ hedefi (-2 döner)
int hotbar_slot_at(float x, float y, int winW, int winH);
bool hotbar_remove_zone_at(float x, float y, int winW, int winH);   // ✕ hedefi
// ✕ hedefinin dikdörtgeni. Çizim ve tıklama AYNI kaynaktan beslenmek zorunda:
// bar yerleşim dosyasından taşınabiliyor, formül kopyası sessizce kayar.
SDL_FRect hotbar_remove_zone_rect(int winW, int winH);



// abilityIconTex: beceri ikon atlası (AbilityId × 32px; NULL = kısaltma metni)
// abilityUsable: beceri mevcut silahla kullanılabilir mi (NULL = hepsi) —
// uymayanlar soluk çizilir (yay becerisi kılıçla vb.)
void hotbar_render(const Hotbar *hb, R2D *renderer,
                   R2DTexture *itemIconTex, const ItemTextures *itemTex,
                   R2DTexture *abilityIconTex,
                   TTF_Font *font, const float *abilityCd, const float *itemCd,
                   const Inventory *inv, const bool *abilityUsable,
                   int winW, int winH);

#endif
