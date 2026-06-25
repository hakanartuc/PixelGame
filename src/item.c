#include "item.h"

const ItemType ITEM_TYPES[] = {
    // --- Kuşanılamaz (eski item'ler, ekipman alanları sıfır/none) ---
    { .name = "Yay",    .iconIndex = 0, .maxStack = 1,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .equipSpritePath = NULL, .plusLevel = 0 },
    { .name = "Kazma",  .iconIndex = 2, .maxStack = 1,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .equipSpritePath = NULL, .plusLevel = 0 },
    { .name = "Mesale", .iconIndex = 9, .maxStack = 99,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .equipSpritePath = NULL, .plusLevel = 0 },

    // --- Ekipman item'leri ---
    { .name = "Demir Kilic", .iconIndex = 4, .maxStack = 1,
      .equipable = true, .equipSlot = EQUIP_WEAPON, .weaponType = WEAPON_ONE_HAND,
      .equipSpritePath = "assets/player/Tools/Iron/Iron_Sword.png", .plusLevel = 0 },

    { .name = "Demir Miger", .iconIndex = 1, .maxStack = 1,   // kask
      .equipable = true, .equipSlot = EQUIP_HELMET, .equipSpritePath = "assets/equipment/head/armet_ceramic.png",},

    { .name = "Demir Zirh", .iconIndex = 8, .maxStack = 1,    // göğüs
     .equipable = true, .equipSlot = EQUIP_CHEST,  .equipSpritePath = "assets/equipment/torso/plate_ceramic.png", },

    { .name = "Demir Pantolon", .iconIndex = 3, .maxStack = 1, // bacak
      .equipable = true, .equipSlot = EQUIP_LEGS,   .equipSpritePath = "assets/equipment/legs/hose_brown.png", },
};

const int ITEM_TYPE_COUNT = (int)(sizeof(ITEM_TYPES) / sizeof(ITEM_TYPES[0]));