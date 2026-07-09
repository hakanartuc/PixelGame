#ifndef ITEM_H
#define ITEM_H
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "common.h"
#include "fx.h"   // WeaponFxType

#define MAX_ITEM_TYPES 32   // ITEM_TYPES kapasitesi (paralel diziler icin sabit boyut)

// Ekipman slotları (KO düzeni)
typedef enum {
    EQUIP_HELMET,      // kask
    EQUIP_CHEST,       // göğüs zırhı
    EQUIP_LEGS,        // pantolon
    EQUIP_GLOVES,      // eldiven
    EQUIP_BOOTS,       // ayakkabı
    EQUIP_WEAPON,      // sağ el (silah)
    EQUIP_SHIELD,      // sol el (kalkan / ikinci silah)
    EQUIP_NECKLACE,    // kolye
    EQUIP_RING_1,      // yüzük 1
    EQUIP_RING_2,      // yüzük 2
    EQUIP_EARRING_1,   // küpe 1
    EQUIP_EARRING_2,   // küpe 2
    EQUIP_BELT,        // kemer
    EQUIP_SLOT_COUNT,  // = 13 (slot sayısı)
    EQUIP_NONE = -1,   // kuşanılamaz item (slot yok)
} EquipSlot;

// Silah tipi — hangi el slotlarını nasıl kullandığı
typedef enum {
    WEAPON_NONE,       // silah değil
    WEAPON_ONE_HAND,   // tek el (WEAPON slotu)
    WEAPON_TWO_HAND,   // çift el (WEAPON dolar, SHIELD kilitlenir)
    WEAPON_SHIELD,     // kalkan (SHIELD slotu)
    WEAPON_DUAL,       // çift tek-el (WEAPON + SHIELD'e tek-el)
} WeaponType;

// Item TÜRÜ — bir kez tanımlanır, tüm o item'ler paylaşır
typedef struct {
    const char *name;
    int iconIndex;               // envanter ikonu (Tool_Icons'ta sıra)
    int maxStack;

    // --- Ekipman bilgileri ---
    bool equipable;              // kuşanılabilir mi
    EquipSlot equipSlot;         // hangi slota gider (EQUIP_NONE: kuşanılamaz)
    WeaponType weaponType;       // silahsa tipi (değilse WEAPON_NONE)
    float damage;                // silah taban hasarı (combat formülünün girdisi)
    float defense;               // zırh/kalkan defansı (azalan getirili: 100/(100+def))
    const char *equipSpritePath; // paper-doll katmanı (kuşanınca görünen sprite, NULL: yok)
    const char *equipSpritePathBack;   // 2. katman (silah arka — yoksa NULL)
    int equipZ;                  // ön katman z (LPC dosya öneki; 0 = slot_zorder varsayılanı)
    int equipZBack;              // arka katman z (equipSpritePathBack varsa zorunlu)
    int equipIdleRow;            // idle'da çizilecek satır (0 = yok, sheet'in kendi idle'ı oynar)
    bool slashOversize;          // slash'ı oversize bloktan çiz (64px slash satırı boş silahlar)
    WeaponFxType weaponFx;       // silahın partikül efekti (0 = WEAPON_FX_NONE)
    int plusLevel;               // +0..+8 (parlama efekti için)

    const char *iconSpritePath;   // ikon dosyasi (NULL → tool_icons fallback)
    SDL_FRect   iconSrc;          // o dosyadaki kare {x, y, w, h}
} ItemType;

// Bir somut item (kasada/envanterde) = tür + miktar
typedef struct {
    const ItemType *type;
    int count;
} ItemStack;

extern const ItemType ITEM_TYPES[];
extern const int ITEM_TYPE_COUNT;

#endif