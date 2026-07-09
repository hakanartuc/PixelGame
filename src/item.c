#include "item.h"

// idle-down karesi: LPC satir 24 (idle 22 + DIR_DOWN 2), kare yuksekligi 64 → y=1536.
// LPC ekipman sheet'leri izole parca icerdiginden bu kare temiz ikon verir.
const ItemType ITEM_TYPES[] = {
    // --- Kusanilamaz (tool_icons fallback: iconSpritePath = NULL) ---
    { .name = "Yay",    .iconIndex = 0, .maxStack = 1,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .equipSpritePath = NULL,
      .iconSpritePath = NULL, .iconSrc = {0,0,0,0} },
    { .name = "Kazma",  .iconIndex = 2, .maxStack = 1,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .equipSpritePath = NULL,
      .iconSpritePath = NULL, .iconSrc = {0,0,0,0} },
    { .name = "Mesale", .iconIndex = 9, .maxStack = 99,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .equipSpritePath = NULL,
      .iconSpritePath = NULL, .iconSrc = {0,0,0,0} },

    // --- Silah: farkli grid (Adim 5), simdilik tool_icons fallback ---
    {.name = "Long Sword", .iconIndex = 4, .maxStack = 1,
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
    { .name = "Demir Miger", .iconIndex = 1, .maxStack = 1,
      .equipable = true, .equipSlot = EQUIP_HELMET, .weaponType = WEAPON_NONE,
      .defense = 8.0f,
      .equipSpritePath = "assets/equipment/head/armet_ceramic.png",
      .iconSpritePath = "assets/equipment/head/armet_ceramic.png",
      .iconSrc = { 0, 1536, 64, 64 } },
    { .name = "Demir Zirh", .iconIndex = 8, .maxStack = 1,
      .equipable = true, .equipSlot = EQUIP_CHEST, .weaponType = WEAPON_NONE,
      .defense = 15.0f,
      .equipSpritePath = "assets/equipment/torso/plate_ceramic.png",
      .iconSpritePath = "assets/equipment/torso/plate_ceramic.png",
      .iconSrc = { 0, 1536, 64, 64 } },
    { .name = "Demir Pantolon", .iconIndex = 3, .maxStack = 1,
      .equipable = true, .equipSlot = EQUIP_LEGS, .weaponType = WEAPON_NONE,
      .defense = 10.0f,
      .equipSpritePath = "assets/equipment/legs/hose_brown.png",
      .iconSpritePath = "assets/equipment/legs/hose_brown.png",
      .iconSrc = { 0, 1536, 64, 64 } },

    // --- Tuketilebilir ---
    { .name = "Can Iksiri", .iconIndex = 5, .maxStack = 10,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .consumable = true, .healAmount = 50.0f,
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