#ifndef ITEM_H
#define ITEM_H
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "common.h"
#include "fx.h"   // WeaponFxType

#define MAX_ITEM_TYPES 64   // ITEM_TYPES kapasitesi (paralel diziler icin sabit boyut)

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

// Zırh sınıfı — hangi zırh skill'ini geliştirir/perk'lerden etkilenir
typedef enum {
    ARMOR_NONE = 0,    // zırh değil
    ARMOR_LIGHT,       // hafif (deri vb. — item'ları ileride)
    ARMOR_MEDIUM,      // orta
    ARMOR_HEAVY,       // ağır (mevcut demir set)
} ArmorClass;

// Silah tipi — hangi el slotlarını nasıl kullandığı
typedef enum {
    WEAPON_NONE,       // silah değil
    WEAPON_ONE_HAND,   // tek el (WEAPON slotu)
    WEAPON_TWO_HAND,   // çift el (WEAPON dolar, SHIELD kilitlenir)
    WEAPON_SHIELD,     // kalkan (SHIELD slotu)
    WEAPON_DUAL,       // çift tek-el (WEAPON + SHIELD'e tek-el)
    WEAPON_TOOL,       // toplama aracı (kazma/balta): WEAPON slotuna takılır,
                       // SALDIRAMAZ ("Silah kuşan!") — toplama savurması için
} WeaponType;

// Item TÜRÜ — bir kez tanımlanır, tüm o item'ler paylaşır
typedef struct {
    const char *name;
    int iconIndex;               // envanter ikonu (Tool_Icons'ta sıra)
    int maxStack;
    int value;                   // vendor alış fiyatı (satış = yarısı)

    // --- Ekipman bilgileri ---
    bool equipable;              // kuşanılabilir mi
    EquipSlot equipSlot;         // hangi slota gider (EQUIP_NONE: kuşanılamaz)
    WeaponType weaponType;       // silahsa tipi (değilse WEAPON_NONE)
    float damage;                // silah taban hasarı (combat formülünün girdisi)
    float attackInterval;        // silahsa: iki otomatik vuruş arası süre (0 = varsayılan)
    float defense;               // zırh/kalkan defansı (azalan getirili: 100/(100+def))
    ArmorClass armorClass;       // zırhsa sınıfı (skill XP + perk çarpanları)
    const char *equipSpritePath; // paper-doll katmanı (kuşanınca görünen sprite, NULL: yok)
    const char *equipSpritePathBack;   // 2. katman (silah arka — yoksa NULL)
    int equipZ;                  // ön katman z (LPC dosya öneki; 0 = slot_zorder varsayılanı)
    int equipZBack;              // arka katman z (equipSpritePathBack varsa zorunlu)
    int equipIdleRow;            // idle'da çizilecek satır (0 = yok, sheet'in kendi idle'ı oynar)
    bool slashOversize;          // slash'ı oversize bloktan çiz (64px slash satırı boş silahlar)
    WeaponFxType weaponFx;       // silahın partikül efekti (0 = WEAPON_FX_NONE)

    // --- Tüketilebilir (iksir vb.) ---
    bool consumable;             // sağ tık hotbar'a atar, hotbar tuşuyla kullanılır
    float healAmount;            // kullanınca iyileşen can
    float useCooldown;           // kullanım sonrası bu TÜRÜN kilit süresi (sn)

    const char *iconSpritePath;   // ikon dosyasi (NULL → tool_icons fallback)
    SDL_FRect   iconSrc;          // o dosyadaki kare {x, y, w, h}
} ItemType;

// Bir somut item (kasada/envanterde) = tür + miktar + instance roll'ları.
// plusLevel TÜRDE değil BURADA: aynı türden +0 ve +3 kılıç ayrı item'dir.
typedef struct {
    const ItemType *type;   // NULL = boş slot
    int count;
    int plusLevel;          // +0..+8 (drop anında roll'lanır; stack'lenenler hep +0)
} ItemStack;

#define ITEM_MAX_PLUS 8
#define ITEM_PLUS_SCALE 0.10f   // her plus: +%10 hasar/defans

// Instance değerleri: tür tabanı × plus ölçeği (combat bunları kullanır)
float item_damage(const ItemStack *s);    // silah değilse 0
float item_defense(const ItemStack *s);   // zırh/kalkan değilse 0
int   item_value(const ItemStack *s);     // instance fiyatı (plus dahil)

// Loot tablosu girdisi: mob ölünce her girdi için ayrı zar atılır
typedef struct {
    const ItemType *type;
    float chance;     // 0..1 düşme şansı (girdiler bağımsız)
    int maxPlus;      // plus roll üst sınırı (0 = hep +0)
} LootEntry;

// ITEM_TYPES dizin sabitleri — loot tabloları sihirli sayı kullanmasın.
// item.c'deki dizi sırasıyla AYNI tutulmalı.
typedef enum {
    ITEM_BOW = 0,
    ITEM_PICKAXE,
    ITEM_TORCH,
    ITEM_LONG_SWORD,
    ITEM_HELMET_IRON,
    ITEM_CHEST_IRON,
    ITEM_LEGS_IRON,
    ITEM_POTION_HP,
    // Meslek paketi (Faz 1): araç + toplama malzemeleri
    ITEM_AXE,           // Balta (oduculuk aracı)
    ITEM_ORE_COPPER,    // Bakır Cevheri
    ITEM_ORE_IRON,      // Demir Cevheri
    ITEM_HERB_RED,      // Kızılot (ova bitkisi)
    ITEM_HERB_BLUE,     // Ayotu (bataklık bitkisi)
    ITEM_WOOD,          // Odun
    // Meslek paketi (Faz 2): craft araç/ara ürün/çıktılar
    ITEM_HAMMER,        // Çekiç (demircilik aracı — Demirci görev ödülü)
    ITEM_INGOT_COPPER,  // Bakır Külçe
    ITEM_INGOT_IRON,    // Demir Külçe
    ITEM_SWORD_COPPER,  // Bakır Kılıç (ilk craft silahı)
    ITEM_POTION_BIG,    // Büyük Can İksiri
} ItemId;

extern const ItemType ITEM_TYPES[];
extern const int ITEM_TYPE_COUNT;

#endif