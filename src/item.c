#include "item.h"

// idle-down karesi: LPC satir 24 (idle 22 + DIR_DOWN 2), kare yuksekligi 64 → y=1536.
// LPC ekipman sheet'leri izole parca icerdiginden bu kare temiz ikon verir.
const ItemType ITEM_TYPES[] = {
    // --- Kusanilamaz (tool_icons fallback: iconSpritePath = NULL) ---
    { .name = "Yay",    .iconIndex = 0, .maxStack = 1, .value = 15,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .equipSpritePath = NULL,
      .iconSpritePath = NULL, .iconSrc = {0,0,0,0} },
    { .name = "Kazma",  .iconIndex = 2, .maxStack = 1, .value = 10,
      .equipable = true, .equipSlot = EQUIP_WEAPON, .weaponType = WEAPON_TOOL,
      .equipSpritePath     = "assets/equipment/weapon/pickaxe_fg.png",   // smash128 on
      .equipSpritePathBack = "assets/equipment/weapon/pickaxe_bg.png",   // smash128 arka
      .equipZ = 140, .equipZBack = 9,
      .iconSpritePath = NULL, .iconSrc = {0,0,0,0} },
    { .name = "Meşale", .iconIndex = 9, .maxStack = 99, .value = 5,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .equipSpritePath = NULL,
      .iconSpritePath = NULL, .iconSrc = {0,0,0,0} },

    // --- Silah: farkli grid (Adim 5), simdilik tool_icons fallback ---
    {.name = "Long Sword", .iconIndex = 4, .maxStack = 1, .value = 200,
  .equipable = true, .equipSlot = EQUIP_WEAPON, .weaponType = WEAPON_ONE_HAND,
  .damage = 50.0f,                  // taban hasar (eski sabit 50 buraya taşındı)
  .attackInterval = 1.2f,           // otomatik vuruş aralığı (sn)
  .equipSpritePath     = "assets/equipment/weapon/long_sword_140.png",   // on
  .equipSpritePathBack = "assets/equipment/weapon/long_sword_009.png",   // arka
  .equipZ = 140, .equipZBack = 9,   // dosya oneklerinden (LPC z)
  .equipIdleRow = 8,                // silahin idle'i yok → walk row 8 ayakta durus
  .slashOversize = true,            // 64px slash satiri bos → oversize savurma blogu
  .weaponFx = WEAPON_FX_PINK,       // taban efekt (run sistemi gelince instance'a tasinacak)

  .iconSpritePath = NULL, .iconSrc = {0,0,0,0} },

    // --- Zirh: kendi LPC sheet'inin idle-down karesi (izole parca = temiz ikon) ---
    { .name = "Demir Miğfer", .iconIndex = 1, .maxStack = 1, .value = 120,
      .equipable = true, .equipSlot = EQUIP_HELMET, .weaponType = WEAPON_NONE,
      .defense = 8.0f, .armorClass = ARMOR_HEAVY,
      .equipSpritePath = "assets/equipment/head/armet_ceramic.png",
      .iconSpritePath = "assets/equipment/head/armet_ceramic.png",
      .iconSrc = { 0, 1536, 64, 64 } },
    { .name = "Demir Zırh", .iconIndex = 8, .maxStack = 1, .value = 200,
      .equipable = true, .equipSlot = EQUIP_CHEST, .weaponType = WEAPON_NONE,
      .defense = 15.0f, .armorClass = ARMOR_HEAVY,
      .equipSpritePath = "assets/equipment/torso/plate_ceramic.png",
      .iconSpritePath = "assets/equipment/torso/plate_ceramic.png",
      .iconSrc = { 0, 1536, 64, 64 } },
    { .name = "Demir Pantolon", .iconIndex = 3, .maxStack = 1, .value = 150,
      .equipable = true, .equipSlot = EQUIP_LEGS, .weaponType = WEAPON_NONE,
      .defense = 10.0f, .armorClass = ARMOR_HEAVY,
      .equipSpritePath = "assets/equipment/legs/hose_brown.png",
      .iconSpritePath = "assets/equipment/legs/hose_brown.png",
      .iconSrc = { 0, 1536, 64, 64 } },

    // --- Tuketilebilir ---
    { .name = "Can İksiri", .iconIndex = 5, .maxStack = 10, .value = 25,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .consumable = true, .healAmount = 50.0f, .useCooldown = 10.0f,
      .iconSpritePath = NULL, .iconSrc = {0,0,0,0} },

    // --- Meslek paketi: arac + toplama malzemeleri (ikonlar sheet'lerden iconSrc ile) ---
    { .name = "Balta", .iconIndex = 3, .maxStack = 1, .value = 15,
      .equipable = true, .equipSlot = EQUIP_WEAPON, .weaponType = WEAPON_TOOL,
      .equipSpritePath     = "assets/equipment/weapon/axe_fg.png",
      .equipSpritePathBack = "assets/equipment/weapon/axe_bg.png",
      .equipZ = 140, .equipZBack = 9,
      .iconSpritePath = NULL, .iconSrc = {0,0,0,0} },   // tool_icons balta
    { .name = "Bakır Cevheri", .iconIndex = 0, .maxStack = 99, .value = 8,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/resources.png", .iconSrc = { 16, 16, 16, 16 } },
    { .name = "Demir Cevheri", .iconIndex = 0, .maxStack = 99, .value = 12,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/resources.png", .iconSrc = { 16, 0, 16, 16 } },
    { .name = "Kızılot", .iconIndex = 0, .maxStack = 99, .value = 6,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/other_icons.png", .iconSrc = { 0, 0, 16, 16 } },
    { .name = "Ayotu", .iconIndex = 0, .maxStack = 99, .value = 6,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/other_icons.png", .iconSrc = { 0, 16, 16, 16 } },
    { .name = "Odun", .iconIndex = 0, .maxStack = 99, .value = 4,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/resources.png", .iconSrc = { 0, 64, 16, 16 } },

    // --- Meslek paketi Faz 2: craft araci + ara urun + ciktilar ---
    { .name = "Çekiç", .iconIndex = 2, .maxStack = 1, .value = 25,
      .equipable = true, .equipSlot = EQUIP_WEAPON, .weaponType = WEAPON_TOOL,
      .equipSpritePath     = "assets/equipment/weapon/hammer_fg.png",
      .equipSpritePathBack = "assets/equipment/weapon/hammer_bg.png",
      .equipZ = 140, .equipZBack = 9,
      .iconSpritePath = "assets/icons/items/hammer_icon.png",
      .iconSrc = { 0, 0, 25, 44 } },   // fg+bg kompozit, sıkı kırpım
    { .name = "Bakır Külçe", .iconIndex = 0, .maxStack = 99, .value = 22,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/resources.png", .iconSrc = { 32, 16, 16, 16 } },
    { .name = "Demir Külçe", .iconIndex = 0, .maxStack = 99, .value = 32,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/resources.png", .iconSrc = { 32, 0, 16, 16 } },
    { .name = "Bakır Kılıç", .iconIndex = 4, .maxStack = 1, .value = 120,
      .equipable = true, .equipSlot = EQUIP_WEAPON, .weaponType = WEAPON_ONE_HAND,
      .damage = 35.0f, .attackInterval = 1.1f,
      .equipSpritePath     = "assets/equipment/weapon/long_sword_140.png",  // görsel: longsword paylaşır
      .equipSpritePathBack = "assets/equipment/weapon/long_sword_009.png",
      .equipZ = 140, .equipZBack = 9, .equipIdleRow = 8, .slashOversize = true,
      .iconSpritePath = NULL, .iconSrc = {0,0,0,0} },
    { .name = "Büyük Can İksiri", .iconIndex = 5, .maxStack = 10, .value = 60,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .consumable = true, .healAmount = 100.0f, .useCooldown = 10.0f,
      .iconSpritePath = NULL, .iconSrc = {0,0,0,0} },
};
const int ITEM_TYPE_COUNT = (int)(sizeof(ITEM_TYPES) / sizeof(ITEM_TYPES[0]));

// Plus ölçeği: her plus taban değerin %10'unu ekler (+8 = ×1.8)
static float plus_scale(int plusLevel)
{
    return 1.0f + (float)plusLevel * ITEM_PLUS_SCALE;
}

float item_damage(const ItemStack *s)
{
    if (s == NULL || s->type == NULL) return 0.0f;
    return s->type->damage * plus_scale(s->plusLevel);
}

float item_defense(const ItemStack *s)
{
    if (s == NULL || s->type == NULL) return 0.0f;
    return s->type->defense * plus_scale(s->plusLevel);
}

int item_value(const ItemStack *s)
{
    if (s == NULL || s->type == NULL) return 0;
    return (int)(s->type->value * plus_scale(s->plusLevel));
}