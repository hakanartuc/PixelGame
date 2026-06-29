#include "item.h"

// idle-down karesi: LPC satir 24 (idle 22 + DIR_DOWN 2), kare yuksekligi 64 → y=1536.
// LPC ekipman sheet'leri izole parca icerdiginden bu kare temiz ikon verir.
const ItemType ITEM_TYPES[] = {
    // --- Kusanilamaz (tool_icons fallback: iconSpritePath = NULL) ---
    { .name = "Yay",    .iconIndex = 0, .maxStack = 1,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .equipSpritePath = NULL, .plusLevel = 0,
      .iconSpritePath = NULL, .iconSrc = {0,0,0,0} },
    { .name = "Kazma",  .iconIndex = 2, .maxStack = 1,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .equipSpritePath = NULL, .plusLevel = 0,
      .iconSpritePath = NULL, .iconSrc = {0,0,0,0} },
    { .name = "Mesale", .iconIndex = 9, .maxStack = 99,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .equipSpritePath = NULL, .plusLevel = 0,
      .iconSpritePath = NULL, .iconSrc = {0,0,0,0} },

    // --- Silah: farkli grid (Adim 5), simdilik tool_icons fallback ---
    {.name = "Demir Kilic", .iconIndex = 4, .maxStack = 1,
  .equipable = true, .equipSlot = EQUIP_WEAPON, .weaponType = WEAPON_ONE_HAND,
  .equipSpritePath     = "assets/equipment/weapon/arming_sword_steel_140.png",   // on
  .equipSpritePathBack = "assets/equipment/weapon/arming_sword_steel_009.png",   // arka
  .plusLevel = 0,
  .iconSpritePath = NULL, .iconSrc = {0,0,0,0} },

    // --- Zirh: kendi LPC sheet'inin idle-down karesi (izole parca = temiz ikon) ---
    { .name = "Demir Miger", .iconIndex = 1, .maxStack = 1,
      .equipable = true, .equipSlot = EQUIP_HELMET, .weaponType = WEAPON_NONE,
      .equipSpritePath = "assets/equipment/head/armet_ceramic.png", .plusLevel = 0,
      .iconSpritePath = "assets/equipment/head/armet_ceramic.png",
      .iconSrc = { 0, 1536, 64, 64 } },
    { .name = "Demir Zirh", .iconIndex = 8, .maxStack = 1,
      .equipable = true, .equipSlot = EQUIP_CHEST, .weaponType = WEAPON_NONE,
      .equipSpritePath = "assets/equipment/torso/plate_ceramic.png", .plusLevel = 0,
      .iconSpritePath = "assets/equipment/torso/plate_ceramic.png",
      .iconSrc = { 0, 1536, 64, 64 } },
    { .name = "Demir Pantolon", .iconIndex = 3, .maxStack = 1,
      .equipable = true, .equipSlot = EQUIP_LEGS, .weaponType = WEAPON_NONE,
      .equipSpritePath = "assets/equipment/legs/hose_brown.png", .plusLevel = 0,
      .iconSpritePath = "assets/equipment/legs/hose_brown.png",
      .iconSrc = { 0, 1536, 64, 64 } },
};
const int ITEM_TYPE_COUNT = (int)(sizeof(ITEM_TYPES) / sizeof(ITEM_TYPES[0]));