#ifndef ITEM_H
#define ITEM_H
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "common.h"

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
    const char *equipSpritePath; // paper-doll katmanı (kuşanınca görünen sprite, NULL: yok)
    int plusLevel;               // +0..+8 (parlama efekti için)
} ItemType;

// Bir somut item (kasada/envanterde) = tür + miktar
typedef struct {
    const ItemType *type;
    int count;
} ItemStack;

extern const ItemType ITEM_TYPES[];
extern const int ITEM_TYPE_COUNT;

#endif