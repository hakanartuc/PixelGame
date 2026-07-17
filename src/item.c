#include "item.h"
#include "ability.h"   // tomeAbility (Büyü dalgası 2)
#include "skill.h"   // parşömen skill sabitleri (SKILL_MINING vb.)

// idle-down karesi: LPC satir 24 (idle 22 + DIR_DOWN 2), kare yuksekligi 64 → y=1536.
// LPC ekipman sheet'leri izole parca icerdiginden bu kare temiz ikon verir.
const ItemType ITEM_TYPES[] = {
    // --- Kusanilamaz (tool_icons fallback: iconSpritePath = NULL) ---
    { .name = "Yay",    .iconIndex = 0, .maxStack = 1, .value = 180,
      .equipable = true, .equipSlot = EQUIP_WEAPON, .weaponType = WEAPON_BOW,
      .damage = 32.0f,                 // taban (DEX ölçekli; melee DPS'in ~yarısı)
      .attackInterval = 1.6f,          // çekme 0.9sn + toparlanma
      // ULPC great bow (çelik): sheet'te YALNIZ shoot satırları çizili (16-19,
      // nişan oku gömülü) — yay araçlar gibi yalnız çekme sırasında görünür.
      // (walk/idle katmanlı yay varyantı bulunursa equipIdleRow ile bağlanır)
      .equipSpritePath = "assets/equipment/weapon/great_bow_140.png",
      .equipZ = 140,
      .iconSpritePath = "assets/icons/items/bow_icon.png",
      .iconSrc = { 0, 0, 33, 34 } },
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
  // weaponFx yok: efektler artik yalniz runlerden/kaplamadan gelir (instance)

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
    { .name = "Bakır Cevheri", .material = true, .iconIndex = 0, .maxStack = 99, .value = 8,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/resources.png", .iconSrc = { 16, 16, 16, 16 } },
    { .name = "Demir Cevheri", .material = true, .iconIndex = 0, .maxStack = 99, .value = 12,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/resources.png", .iconSrc = { 16, 0, 16, 16 } },
    { .name = "Kızılot", .material = true, .iconIndex = 0, .maxStack = 99, .value = 6,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/other_icons.png", .iconSrc = { 0, 0, 16, 16 } },
    { .name = "Ayotu", .material = true, .iconIndex = 0, .maxStack = 99, .value = 6,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/other_icons.png", .iconSrc = { 0, 16, 16, 16 } },
    { .name = "Odun", .material = true, .iconIndex = 0, .maxStack = 99, .value = 4,
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
    { .name = "Bakır Külçe", .material = true, .iconIndex = 0, .maxStack = 99, .value = 22,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/resources.png", .iconSrc = { 32, 16, 16, 16 } },
    { .name = "Demir Külçe", .material = true, .iconIndex = 0, .maxStack = 99, .value = 32,
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

    // --- Meslek paketi Faz 3: blueprint ekonomisi + buff tuketilebilirleri ---
    { .name = "Demir Kılıç", .iconIndex = 4, .maxStack = 1, .value = 320,
      .equipable = true, .equipSlot = EQUIP_WEAPON, .weaponType = WEAPON_ONE_HAND,
      .damage = 60.0f, .attackInterval = 1.2f,
      .equipSpritePath     = "assets/equipment/weapon/long_sword_140.png",
      .equipSpritePathBack = "assets/equipment/weapon/long_sword_009.png",
      .equipZ = 140, .equipZBack = 9, .equipIdleRow = 8, .slashOversize = true,
      .iconSpritePath = NULL, .iconSrc = {0,0,0,0} },
    { .name = "Hız İksiri", .iconIndex = 5, .maxStack = 10, .value = 45,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .consumable = true, .useCooldown = 30.0f,
      .buffSpeedMult = 1.2f, .buffDuration = 30.0f,
      .iconSpritePath = NULL, .iconSrc = {0,0,0,0} },
    { .name = "Zehir Yağı", .iconIndex = 5, .maxStack = 10, .value = 55,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .consumable = true, .useCooldown = 20.0f,
      .coatFx = WEAPON_FX_POISON, .coatDuration = 60.0f,
      .iconSpritePath = NULL, .iconSrc = {0,0,0,0} },

    // Blueprint'ler (sağ tık: tarifi KALICI öğren, parşömen tükenir)
    { .name = "Blueprint: Demir Kılıç", .iconIndex = 0, .maxStack = 5, .value = 90,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/scrolls.png", .iconSrc = { 0, 0, 16, 16 } },
    { .name = "Blueprint: Demir Zırh", .iconIndex = 0, .maxStack = 5, .value = 150,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/scrolls.png", .iconSrc = { 0, 0, 16, 16 } },
    { .name = "Blueprint: Demir Miğfer", .iconIndex = 0, .maxStack = 5, .value = 80,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/scrolls.png", .iconSrc = { 0, 0, 16, 16 } },
    { .name = "Blueprint: Hız İksiri", .iconIndex = 0, .maxStack = 5, .value = 100,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/scrolls.png", .iconSrc = { 0, 0, 16, 16 } },
    { .name = "Blueprint: Zehir Yağı", .iconIndex = 0, .maxStack = 5, .value = 110,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/scrolls.png", .iconSrc = { 0, 0, 16, 16 } },

    // Öğretici parşömenler (sağ tık: +250 XP, tükenir) — tür moba göre düşer
    { .name = "Parşömen: Mining", .iconIndex = 0, .maxStack = 10, .value = 40,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .scrollSkill = SKILL_MINING, .scrollXp = 250.0f,
      .iconSpritePath = "assets/icons/items/scrolls.png", .iconSrc = { 16, 0, 16, 16 } },
    { .name = "Parşömen: Herbalism", .iconIndex = 0, .maxStack = 10, .value = 40,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .scrollSkill = SKILL_HERBALISM, .scrollXp = 250.0f,
      .iconSpritePath = "assets/icons/items/scrolls.png", .iconSrc = { 32, 0, 16, 16 } },
    { .name = "Parşömen: Smithing", .iconIndex = 0, .maxStack = 10, .value = 50,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .scrollSkill = SKILL_SMITHING, .scrollXp = 250.0f,
      .iconSpritePath = "assets/icons/items/scrolls.png", .iconSrc = { 48, 0, 16, 16 } },
    { .name = "Parşömen: One-Handed", .iconIndex = 0, .maxStack = 10, .value = 50,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .scrollSkill = SKILL_ONE_HANDED, .scrollXp = 250.0f,
      .iconSpritePath = "assets/icons/items/scrolls.png", .iconSrc = { 64, 0, 16, 16 } },

    // --- Meslek paketi Faz 4: sunak — ozler, runler, stat taslari ---
    { .name = "Soluk Öz", .material = true, .iconIndex = 0, .maxStack = 99, .value = 10,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/decor/ores.png", .iconSrc = { 80, 112, 16, 16 } },
    { .name = "Zehirli Öz", .material = true, .iconIndex = 0, .maxStack = 99, .value = 12,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/decor/ores.png", .iconSrc = { 80, 48, 16, 16 } },
    { .name = "Pembe Rün", .iconIndex = 0, .maxStack = 10, .value = 70,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/resources.png", .iconSrc = { 64, 48, 16, 16 } },
    { .name = "Zehir Rünü", .iconIndex = 0, .maxStack = 10, .value = 75,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/resources.png", .iconSrc = { 16, 48, 16, 16 } },
    { .name = "Güç Taşı", .iconIndex = 0, .maxStack = 10, .value = 120,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/scrolls.png", .iconSrc = { 80, 0, 16, 16 } },
    { .name = "Çeviklik Taşı", .iconIndex = 0, .maxStack = 10, .value = 120,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/scrolls.png", .iconSrc = { 96, 0, 16, 16 } },
    { .name = "Yaşam Taşı", .iconIndex = 0, .maxStack = 10, .value = 120,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/scrolls.png", .iconSrc = { 112, 0, 16, 16 } },
    { .name = "Koruma Taşı", .iconIndex = 0, .maxStack = 10, .value = 120,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/scrolls.png", .iconSrc = { 128, 0, 16, 16 } },
    { .name = "Akıl Taşı", .iconIndex = 0, .maxStack = 10, .value = 120,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/scrolls.png", .iconSrc = { 144, 0, 16, 16 } },
    { .name = "Dayanıklılık Taşı", .iconIndex = 0, .maxStack = 10, .value = 120,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/scrolls.png", .iconSrc = { 160, 0, 16, 16 } },

    // --- Faz 5: okçuluk ---
    { .name = "Ok", .iconIndex = 0, .maxStack = 99, .value = 2,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .ammoDmgMult = 1.0f,   // standart kademe (en ucuz)
      .iconSpritePath = "assets/icons/items/arrow_icon.png",
      .iconSrc = { 0, 0, 32, 32 } },

    // --- Faz 6: av paketi — hayvan lootu, deri seti (hafif zırh), yemek ---
    { .name = "Deri", .material = true, .iconIndex = 0, .maxStack = 99, .value = 15,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/resources.png",
      .iconSrc = { 16, 80, 16, 16 } },
    { .name = "Et", .material = true, .iconIndex = 0, .maxStack = 99, .value = 8,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .consumable = true, .useCooldown = 4.0f,   // M2: çiğ yenir — riskli
      .healAmount = 6.0f, .rawRisk = 0.25f,      // %25: 8sn hafif zehir
      .iconSpritePath = "assets/icons/items/food.png",
      .iconSrc = { 0, 16, 16, 16 } },
    { .name = "Pişmiş Et", .iconIndex = 0, .maxStack = 20, .value = 20,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .consumable = true, .useCooldown = 4.0f,
      .foodRegen = 3.0f, .foodDuration = 12.0f,   // toplam 36 can — iksirden yavaş, ucuz
      .iconSpritePath = "assets/icons/items/food.png",
      .iconSrc = { 16, 16, 16, 16 } },
    { .name = "Deri Başlık", .iconIndex = 1, .maxStack = 1, .value = 60,
      .equipable = true, .equipSlot = EQUIP_HELMET, .weaponType = WEAPON_NONE,
      .defense = 4.0f, .armorClass = ARMOR_LIGHT,
      .equipSpritePath = "assets/equipment/head/leather_cap.png",
      .iconSpritePath = "assets/equipment/head/leather_cap.png",
      .iconSrc = { 0, 1536, 64, 64 } },
    { .name = "Deri Zırh", .iconIndex = 8, .maxStack = 1, .value = 120,
      .equipable = true, .equipSlot = EQUIP_CHEST, .weaponType = WEAPON_NONE,
      .defense = 8.0f, .armorClass = ARMOR_LIGHT,
      .equipSpritePath = "assets/equipment/torso/leather_torso.png",
      .iconSpritePath = "assets/equipment/torso/leather_torso.png",
      .iconSrc = { 0, 1536, 64, 64 } },
    { .name = "Deri Bot", .iconIndex = 3, .maxStack = 1, .value = 50,
      .equipable = true, .equipSlot = EQUIP_BOOTS, .weaponType = WEAPON_NONE,
      .defense = 3.0f, .armorClass = ARMOR_LIGHT,
      .equipSpritePath = "assets/equipment/feet/leather_boots.png",
      .iconSpritePath = "assets/equipment/feet/leather_boots.png",
      .iconSrc = { 0, 1536, 64, 64 } },

    // --- Faz 7: slime ekonomisi ---
    { .name = "Slime Jeli", .material = true, .iconIndex = 0, .maxStack = 99, .value = 6,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/resources.png",
      .iconSrc = { 32, 48, 16, 16 } },
    { .name = "Yapışkan Tuzağı", .iconIndex = 0, .maxStack = 10, .value = 25,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .consumable = true, .useCooldown = 2.0f,
      .trapItem = true,   // kullanım: ayak önüne tuzak kur (main işler)
      .iconSpritePath = "assets/icons/items/resources.png",
      .iconSrc = { 80, 64, 16, 16 } },

    // --- Faz 8: iskelet ekonomisi — kemik + kadim parşömenler ---
    { .name = "Kemik", .material = true, .iconIndex = 0, .maxStack = 99, .value = 10,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/resources.png",
      .iconSrc = { 16, 64, 16, 16 } },
    { .name = "Parşömen: Archery", .iconIndex = 0, .maxStack = 10, .value = 50,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .scrollSkill = SKILL_ARCHERY, .scrollXp = 250.0f,
      .iconSpritePath = "assets/icons/items/scrolls.png",
      .iconSrc = { 176, 0, 16, 16 } },
    { .name = "Parşömen: Enchanting", .iconIndex = 0, .maxStack = 10, .value = 50,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .scrollSkill = SKILL_ENCHANTING, .scrollXp = 250.0f,
      .iconSpritePath = "assets/icons/items/scrolls.png",
      .iconSrc = { 192, 0, 16, 16 } },

    // --- Ok kademeleri (Faz 8): sağ tık = aktif mühimmat seçimi ---
    { .name = "Kemik Oku", .iconIndex = 0, .maxStack = 99, .value = 4,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .ammoDmgMult = 1.15f,   // keskin kemik uç
      .iconSpritePath = "assets/icons/items/arrow_bone_icon.png",
      .iconSrc = { 0, 0, 32, 32 } },
    { .name = "Demir Ok", .iconIndex = 0, .maxStack = 99, .value = 8,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .ammoDmgMult = 1.30f,   // en sert kademe
      .iconSpritePath = "assets/icons/items/arrow_iron_icon.png",
      .iconSrc = { 0, 0, 32, 32 } },

    // --- Faz 22: büyü paketi ---
    { .name = "Bilgelik Taşı", .iconIndex = 0, .maxStack = 10, .value = 120,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/scrolls.png",
      .iconSrc = { 208, 0, 16, 16 } },
    { .name = "Asa", .iconIndex = 0, .maxStack = 1, .value = 220,
      .equipable = true, .equipSlot = EQUIP_WEAPON, .weaponType = WEAPON_STAFF,
      .damage = 15.0f,                 // zayıf melee (asıl gücü büyüde)
      .attackInterval = 1.3f,
      .spellPower = 0.20f,             // asa: büyü hasarı +%20 (tam-büyücü silahı)
      // ULPC simple_staff: spellcast+thrust+walk satırları dolu (ön+arka katman).
      // Melee'si SLASH değil THRUST oynar (asa dürtmesi — main staffEquipped yazar)
      .equipSpritePath     = "assets/equipment/weapon/staff_140.png",
      .equipSpritePathBack = "assets/equipment/weapon/staff_009.png",
      .equipZ = 140, .equipZBack = 9,
      .equipIdleRow = 8,               // idle: walk kare 0 (asa elde duruş)
      .iconSpritePath = "assets/icons/items/resources.png",
      .iconSrc = { 32, 64, 16, 16 } },
    { .name = "Mana İksiri", .iconIndex = 5, .maxStack = 10, .value = 30,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .consumable = true, .manaAmount = 40.0f, .useCooldown = 10.0f,
      .iconSpritePath = "assets/icons/items/food.png",
      .iconSrc = { 112, 0, 16, 16 } },   // mavi şişe (Food Icons son sütun)
    { .name = "Maymuncuk", .iconIndex = 7, .maxStack = 20, .value = 15,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/tool_icons.png",
      .iconSrc = { 112, 0, 16, 16 } },   // tool_icons 7. hücre (çubuk görünümlü)

    // İskelet Lordu benzersizi: +2 düşer, hasarın %5'i cana döner.
    // Görsel: longsword paper-doll katmanları (kendi sheet'i bulunana dek)
    { .name = "Lord'un Kılıcı", .iconIndex = 4, .maxStack = 1, .value = 800,
      .equipable = true, .equipSlot = EQUIP_WEAPON, .weaponType = WEAPON_ONE_HAND,
      .damage = 62.0f, .attackInterval = 1.1f, .lifesteal = 0.05f,
      .equipSpritePath     = "assets/equipment/weapon/long_sword_140.png",
      .equipSpritePathBack = "assets/equipment/weapon/long_sword_009.png",
      .equipZ = 140, .equipZBack = 9, .equipIdleRow = 8, .slashOversize = true,
      .iconSpritePath = NULL, .iconSrc = {0,0,0,0} },
    // --- M2 Buff yemekleri: kamp ateşi/kazanda pişer; TEK "Tok" buffu ---
    { .name = "Avcı Yahnisi", .iconIndex = 0, .maxStack = 10, .value = 45,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .consumable = true, .useCooldown = 6.0f,
      .foodBuffType = FB_DMG, .foodBuffPower = 0.08f, .foodBuffDur = 180.0f,
      .iconSpritePath = "assets/icons/items/food.png",
      .iconSrc = { 32, 16, 16, 16 } },
    { .name = "Ot Çorbası", .iconIndex = 0, .maxStack = 10, .value = 40,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .consumable = true, .useCooldown = 6.0f,
      .foodBuffType = FB_MANA_REGEN, .foodBuffPower = 1.5f, .foodBuffDur = 180.0f,
      .iconSpritePath = "assets/icons/items/food.png",
      .iconSrc = { 48, 16, 16, 16 } },
    { .name = "Gezgin Kavurması", .iconIndex = 0, .maxStack = 10, .value = 50,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .consumable = true, .useCooldown = 6.0f,
      .foodBuffType = FB_STA_SPEED, .foodBuffPower = 1.6f, .foodBuffDur = 180.0f,
      .iconSpritePath = "assets/icons/items/food.png",
      .iconSrc = { 64, 16, 16, 16 } },

    // Büyü dalgası 2: kayıp büyü tomarları — sağ tık = oku (Yıkım seviyesi
    // yetiyorsa büyü KALICI açılır, tomar tükenir). Kaynak: kervan/zindan.
    { .name = "Tomar: Yıldırım Pençesi", .iconIndex = 0, .maxStack = 1, .value = 350,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .tomeAbility = ABILITY_LIGHTNING_CLAW, .tomeReqLevel = 15,
      .iconSpritePath = "assets/icons/items/scrolls.png", .iconSrc = { 0, 0, 16, 16 } },
    { .name = "Tomar: Buz Dikenleri", .iconIndex = 0, .maxStack = 1, .value = 550,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .tomeAbility = ABILITY_ICETACLE, .tomeReqLevel = 30,
      .iconSpritePath = "assets/icons/items/scrolls.png", .iconSrc = { 0, 0, 16, 16 } },
    { .name = "Tomar: Hortum", .iconIndex = 0, .maxStack = 1, .value = 650,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .tomeAbility = ABILITY_TORNADO, .tomeReqLevel = 40,
      .iconSpritePath = "assets/icons/items/scrolls.png", .iconSrc = { 0, 0, 16, 16 } },

    // Çanta sekmeleri: İp (ara malzeme) + 4 çanta eşyası.
    // Çanta eşyaları üretilince OTOMATİK takılır (craft_finish özel yolu) —
    // envantere hiç girmez, geri alınamaz, satılamaz.
    { .name = "İp", .material = true, .iconIndex = 6, .maxStack = 99, .value = 8,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE },
    { .name = "Deri Çanta", .iconIndex = 6, .maxStack = 1, .value = 0,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .bagUpgrade = 1 },
    { .name = "Gezgin Çantası", .iconIndex = 6, .maxStack = 1, .value = 0,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .bagUpgrade = 2 },
    { .name = "Malzeme Kesesi", .iconIndex = 6, .maxStack = 1, .value = 0,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .bagUpgrade = 3 },
    { .name = "Usta Kesesi", .iconIndex = 6, .maxStack = 1, .value = 0,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .bagUpgrade = 4 },

    // Büyü dalgası 2, parça 2 — SONA eklendi (save item indeksleri kaymasın)
    { .name = "Tomar: Yılan Isırığı", .iconIndex = 0, .maxStack = 1, .value = 400,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .tomeAbility = ABILITY_SNAKEBITE, .tomeReqLevel = 20,
      .iconSpritePath = "assets/icons/items/scrolls.png", .iconSrc = { 0, 0, 16, 16 } },
    { .name = "Tomar: Ateş Aslanı", .iconIndex = 0, .maxStack = 1, .value = 750,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .tomeAbility = ABILITY_FIRELION, .tomeReqLevel = 50,
      .iconSpritePath = "assets/icons/items/scrolls.png", .iconSrc = { 0, 0, 16, 16 } },

    // Two-Handed hattı (Parça A): mızrak — mesafe kontrolü silahı.
    // Menzil 130 (kılıç 80) + tempo 1.5sn + defans delme %25 + hat vuruşu
    // (saplama çizgisindeki 2. mob %60 yer — main işler). İki el: kalkan kilitli.
    { .name = "Mızrak", .iconIndex = 4, .maxStack = 1, .value = 260,
      .equipable = true, .equipSlot = EQUIP_WEAPON, .weaponType = WEAPON_TWO_HAND,
      .damage = 34.0f, .attackInterval = 1.5f, .armorPierce = 0.25f,
      .equipSpritePath     = "assets/equipment/weapon/spear_140_full.png",
      .equipSpritePathBack = "assets/equipment/weapon/spear_009_full.png",
      .equipZ = 140, .equipZBack = 9, .equipIdleRow = 8, .thrustOversize = true,
      .iconSpritePath = "assets/icons/items/spear.png", .iconSrc = { 0, 0, 64, 64 } },

    // Block hattı (Parça B): kalkanlar — pasif blok şansı + SHIFT aktif duruş.
    // Görsel: ULPC round_shield (ikisi de paylaşır — long_sword deseni)
    { .name = "Tahta Kalkan", .iconIndex = 0, .maxStack = 1, .value = 120,
      .equipable = true, .equipSlot = EQUIP_SHIELD, .weaponType = WEAPON_SHIELD,
      .defense = 6.0f,
      .equipSpritePath = "assets/equipment/shield/round_shield_110.png",
      .equipZ = 110, .equipIdleRow = 8,
      .iconSpritePath = "assets/icons/items/shields.png", .iconSrc = { 0, 0, 16, 16 } },
    { .name = "Demir Kalkan", .iconIndex = 0, .maxStack = 1, .value = 300,
      .equipable = true, .equipSlot = EQUIP_SHIELD, .weaponType = WEAPON_SHIELD,
      .defense = 12.0f,
      .equipSpritePath = "assets/equipment/shield/round_shield_110.png",
      .equipZ = 110, .equipIdleRow = 8,
      .iconSpritePath = "assets/icons/items/shields.png", .iconSrc = { 16, 0, 16, 16 } },

    // Kirli Mal & Aklama: değerli objeler — yalnız çalıntı; aklayıcı tam değer öder
    { .name = "Yüzük", .iconIndex = 0, .maxStack = 10, .value = 80,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/trinkets.png", .iconSrc = { 0, 0, 16, 16 } },
    { .name = "Broş", .iconIndex = 0, .maxStack = 10, .value = 100,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/trinkets.png", .iconSrc = { 16, 0, 16, 16 } },
    { .name = "Cep Saati", .iconIndex = 0, .maxStack = 10, .value = 150,
      .equipable = false, .equipSlot = EQUIP_NONE, .weaponType = WEAPON_NONE,
      .iconSpritePath = "assets/icons/items/trinkets.png", .iconSrc = { 32, 0, 16, 16 } },
};
const int ITEM_TYPE_COUNT = (int)(sizeof(ITEM_TYPES) / sizeof(ITEM_TYPES[0]));

const int STAT_TIER_VALUE[STAT_COUNT] = { 2, 2, 10, 3, 8, 10, 2 };
const char *const STAT_NAMES[STAT_COUNT] = { "STR", "DEX", "HP", "DEF", "MP", "STA", "INT" };

ItemStack item_make(const ItemType *type, int count, int plusLevel)
{
    ItemStack s = { .type = type, .count = count, .plusLevel = plusLevel };
    s.fx = (type != NULL) ? (Sint8)type->weaponFx : 0;
    s.statType[0] = s.statType[1] = -1;
    s.statTier[0] = s.statTier[1] = 0;
    s.stolen = 0;   // temiz doğar — yalnız yankesicilik damgalar
    return s;
}

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