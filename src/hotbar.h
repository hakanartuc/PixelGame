#ifndef HOTBAR_H
#define HOTBAR_H
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "ability.h"
#include "item.h"
#include "item_textures.h"
#include "inventory.h"

// KO tarzı hotbar: 1-8 tuşları. Skill'ler açıldıkça ilk boş slota otomatik
// oturur; tüketilebilir item envanterde sağ tıkla bara atanır (item'ın
// KENDİSİ değil TÜRÜ atanır — kullanınca envanterden düşer, sayısı barda görünür).

#define HOTBAR_SLOTS 8

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

// Skill seviyelerine göre açılmış ama barda olmayan ability'leri ilk boş
// slotlara yerleştir. Başlangıçta ve her level-up sonrası çağrılır.
void hotbar_place_unlocks(Hotbar *hb, const Skill *skills);

// Tüketilebilir türü ilk boş slota ata (zaten bardaysa dokunma). Başarılıysa true.
bool hotbar_assign_item(Hotbar *hb, const ItemType *type);

// Bar çizimi: alt-orta, tuş numarası + kısaltma/ikon + cooldown perdesi + adet.
// abilityCd: ability başına, itemCd: item TÜRÜ başına kalan bekleme (Player'dan).
void hotbar_render(const Hotbar *hb, SDL_Renderer *renderer,
                   SDL_Texture *itemIconTex, const ItemTextures *itemTex,
                   TTF_Font *font, const float *abilityCd, const float *itemCd,
                   const Inventory *inv, int winW, int winH);

#endif
