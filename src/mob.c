#include <SDL3/SDL.h>
#include <stdlib.h>
#include "common.h"
#include "font.h"
#include "tilemap.h"
#include "decor.h"
#include "mob.h"
#include "gametime.h"
#include "weather.h"   // E2: gece/gündüz ritmi

// Önerilen kaydırmayı tile/dekor çarpışmasına göre uygula.
// Eksen ayrı denenir → duvar boyunca kayma (oyuncu hareketiyle aynı desen).
static void mob_move(Mob *m, float dx, float dy)
{
    SDL_FRect fx = feet_box(m->e.rect.x + dx, m->e.rect.y, m->e.rect.w, m->e.rect.h);
    if (!box_hits_solid(fx) && !decor_blocks(fx))
        m->e.rect.x += dx;

    SDL_FRect fy = feet_box(m->e.rect.x, m->e.rect.y + dy, m->e.rect.w, m->e.rect.h);
    if (!box_hits_solid(fy) && !decor_blocks(fy))
        m->e.rect.y += dy;
}

#define SEPARATION_RADIUS 70.0f   // bu mesafeden yakın mob'lar iter
#define SEPARATION_FORCE 250.0f   // itme gücü

// Loot tabloları — her mob türü ne düşürür (şans + plus tavanı).
// Girdiler bağımsız: aynı ölümde birden fazlası düşebilir.
static const LootEntry GOBLIN_LOOT[] = {
    { &ITEM_TYPES[ITEM_POTION_HP],  0.40f, 0 },
    { &ITEM_TYPES[ITEM_LONG_SWORD], 0.08f, 3 },
    // Faz 3 — kesmenin anlamı: blueprint + öğretici parşömenler
    { &ITEM_TYPES[ITEM_BP_HELMET_IRON],   0.03f, 0 },
    { &ITEM_TYPES[ITEM_SCROLL_MINING],    0.03f, 0 },
    { &ITEM_TYPES[ITEM_SCROLL_HERBALISM], 0.03f, 0 },
    // Faz 4 — sunak ekonomisi: öz (rün hammaddesi) + çevik/dayanıklı taşlar
    { &ITEM_TYPES[ITEM_ESSENCE_PALE], 0.20f, 0 },
    { &ITEM_TYPES[ITEM_STONE_DEX],    0.03f, 0 },
    { &ITEM_TYPES[ITEM_STONE_HP],     0.03f, 0 },
    { &ITEM_TYPES[ITEM_STONE_STA],    0.02f, 0 },
};
static const LootEntry ORC_LOOT[] = {
    { &ITEM_TYPES[ITEM_POTION_HP],   0.50f, 0 },
    { &ITEM_TYPES[ITEM_HELMET_IRON], 0.15f, 3 },
    { &ITEM_TYPES[ITEM_CHEST_IRON],  0.12f, 3 },
    { &ITEM_TYPES[ITEM_LEGS_IRON],   0.15f, 3 },
    { &ITEM_TYPES[ITEM_LONG_SWORD],  0.12f, 5 },
    // Faz 3 — ork: gelişmiş silah/simya bilgisi
    { &ITEM_TYPES[ITEM_BP_SWORD_IRON],   0.05f, 0 },
    { &ITEM_TYPES[ITEM_BP_POISON_OIL],   0.04f, 0 },
    { &ITEM_TYPES[ITEM_SCROLL_SMITHING], 0.03f, 0 },
    { &ITEM_TYPES[ITEM_SCROLL_ONEHAND],  0.03f, 0 },
    // Faz 4 — ork: zehirli öz + savaşçı taşları
    { &ITEM_TYPES[ITEM_ESSENCE_TOXIC], 0.20f, 0 },
    { &ITEM_TYPES[ITEM_STONE_STR],     0.03f, 0 },
    { &ITEM_TYPES[ITEM_STONE_DEF],     0.03f, 0 },
    { &ITEM_TYPES[ITEM_STONE_MP],      0.02f, 0 },
};
// Faz 7 — slime: loot YALNIZ Küçük'ten (zincir sonu ödülü; kasa spam'i olmaz)
static const LootEntry SLIME_G_LOOT[] = {
    { &ITEM_TYPES[ITEM_SLIME_GEL], 0.80f, 0 },
};
static const LootEntry SLIME_B_LOOT[] = {
    { &ITEM_TYPES[ITEM_SLIME_GEL],     0.80f, 0 },
    { &ITEM_TYPES[ITEM_ESSENCE_TOXIC], 0.10f, 0 },   // sunağa ikinci kaynak
};

// Faz 8 — iskelet: kemik + kadim parşömenler (Bowman = Archery'nin TEK kaynağı)
static const LootEntry SKEL_W_LOOT[] = {
    { &ITEM_TYPES[ITEM_BONE], 0.70f, 0 },
    { &ITEM_TYPES[ITEM_BONE], 0.35f, 0 },
    { &ITEM_TYPES[ITEM_SCROLL_ONEHAND], 0.03f, 0 },
    { &ITEM_TYPES[ITEM_POTION_HP],      0.25f, 0 },
};
static const LootEntry SKEL_B_LOOT[] = {
    { &ITEM_TYPES[ITEM_BONE], 0.60f, 0 },
    { &ITEM_TYPES[ITEM_ARROW],          0.50f, 0 },   // sadağından kalanlar
    { &ITEM_TYPES[ITEM_SCROLL_ARCHERY], 0.04f, 0 },
};
static const LootEntry SKEL_M_LOOT[] = {
    { &ITEM_TYPES[ITEM_BONE], 0.50f, 0 },
    { &ITEM_TYPES[ITEM_ESSENCE_PALE],   0.15f, 0 },   // büyücü — sunak kaynağı
    { &ITEM_TYPES[ITEM_SCROLL_ENCHANT], 0.04f, 0 },
    { &ITEM_TYPES[ITEM_STONE_INT],      0.03f, 0 },   // Bilgelik Taşı (Faz 22)
};

// Faz 6 — av: deri/et (girdiler bağımsız zar → 1-2 adet hissi)
static const LootEntry SHEEP_LOOT[] = {
    { &ITEM_TYPES[ITEM_LEATHER], 0.70f, 0 },
    { &ITEM_TYPES[ITEM_LEATHER], 0.35f, 0 },
    { &ITEM_TYPES[ITEM_MEAT],    0.90f, 0 },
    { &ITEM_TYPES[ITEM_MEAT],    0.45f, 0 },
};
static const LootEntry CHICKEN_LOOT[] = {
    { &ITEM_TYPES[ITEM_MEAT], 0.80f, 0 },
};
static const LootEntry BOAR_LOOT[] = {
    { &ITEM_TYPES[ITEM_LEATHER], 1.00f, 0 },
    { &ITEM_TYPES[ITEM_LEATHER], 0.60f, 0 },
    { &ITEM_TYPES[ITEM_LEATHER], 0.30f, 0 },
    { &ITEM_TYPES[ITEM_MEAT],    1.00f, 0 },
    { &ITEM_TYPES[ITEM_MEAT],    0.50f, 0 },
};
#define LOOT_COUNT(t) ((int)(sizeof(t) / sizeof((t)[0])))

static const LootEntry SKEL_LORD_LOOT[] = {
    { &ITEM_TYPES[ITEM_BONE],           1.00f, 0 },
    { &ITEM_TYPES[ITEM_BONE],           1.00f, 0 },
    { &ITEM_TYPES[ITEM_STONE_HP],       1.00f, 0 },   // her ölümde stat taşı
    { &ITEM_TYPES[ITEM_SCROLL_ARCHERY], 0.50f, 0 },   // ikisinden biri düşer gibi
    { &ITEM_TYPES[ITEM_SCROLL_ENCHANT], 0.50f, 0 },
};

// Mob türleri tablosu — her tür bir kez tanımlı. (TILE_TYPES gibi)
const MobType MOB_TYPES[] = {
    // [0] Goblin Maceman — uniform 32×32, klasik düzen
    {
        .name = "Goblin Maceman",
        .texturePath = "assets/characters/mobs/Goblin_Maceman.png",
        .nocturnal = true,   // E2: gece aktif, gündüz kampta uyuklar
        .level = 1,
        .maxHealth = 100.0f,
        .speed = 200.0f,
        .attackRange = 60.0f,
        .detectRange = 300.0f,
        .damage = 5.0f,
        .defense = 0.0f,
        .attackType = ATTACK_MELEE,
        .lootTable = GOBLIN_LOOT, .lootTableCount = LOOT_COUNT(GOBLIN_LOOT),
        .goldMin = 5, .goldMax = 15,
        .attackHitFrame = 3,
        .anims = {
            // [0] idle — aşağı, yukarı, yan (4 kare, 32×32)
            {
                { .x=0, .y=0,   .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.18f, .feetY=30 },
                { .x=0, .y=64,  .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.18f, .feetY=30 },
                { .x=0, .y=32,  .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.18f, .feetY=30 },
            },
            // [1] run — aşağı, yukarı, yan (6 kare, 32×32)
            {
                { .x=0, .y=96,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=160, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=128, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.15f, .feetY=30 },
            },
            // [2] attack — aşağı, yukarı, yan (6 kare, 32×32)
            {
                { .x=0, .y=192, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.08f, .feetY=30 },
                { .x=0, .y=256, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.08f, .feetY=30 },
                { .x=0, .y=224, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.08f, .feetY=30 },
            },
            // [3] dying — death animasyonu (üç yöne de aynı, tek yönlü ölüm)
            {
                { .x=0, .y=288, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=23 },  // aşağı
                { .x=0, .y=288, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=23 },  // yukarı
                { .x=0, .y=288, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=23 },  // yan
            },
        },
    },
    // [1] Orc Grunt — KARIŞIK boyut: idle/run 32×32, attack 64×64
    {
        .name = "Orc Grunt",
        .texturePath = "assets/characters/mobs/Orc_Grunt.png",
        .level = 3,
        .maxHealth = 200.0f,
        .speed = 150.0f,
        .attackRange = 70.0f,
        .detectRange = 320.0f,
        .damage = 10.0f,
        .defense = 10.0f,
        .attackType = ATTACK_MELEE,
        .lootTable = ORC_LOOT, .lootTableCount = LOOT_COUNT(ORC_LOOT),
        .goldMin = 20, .goldMax = 50,
        .attackHitFrame = 4,
        .anims = {
            // [0] idle — aşağı, yukarı, yan (6 kare, 32×32)
            {
                { .x=0, .y=0,   .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.13, .feetY=30 },
                { .x=0, .y=64,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.13, .feetY=30 },
                { .x=0, .y=32,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.13, .feetY=30 },
            },
            // [1] run — aşağı, yukarı, yan (6 kare, 32×32)
            {
                { .x=0, .y=96,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.12f, .feetY=30 },
                { .x=0, .y=160, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.12f, .feetY=30 },
                { .x=0, .y=128, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.12f, .feetY=30 },
            },
            // [2] attack — aşağı, yukarı, yan (8 kare, 64×64 — feetY FARKLI: 46)
            {
                { .x=0, .y=192, .frameW=64, .frameH=64, .frameCount=8, .frameDuration=0.07f, .feetY=46 },
                { .x=0, .y=320, .frameW=64, .frameH=64, .frameCount=8, .frameDuration=0.07f, .feetY=46 },
                { .x=0, .y=256, .frameW=64, .frameH=64, .frameCount=8, .frameDuration=0.07f, .feetY=46 },
            },
            // [3] dying — death animasyonu (üç yöne de aynı, tek yönlü ölüm)
            {
                { .x=0, .y=384, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },  // aşağı
                { .x=0, .y=384, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },  // yukarı
                { .x=0, .y=384, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },  // yan
            },
        },
    },

    // --- Faz 6: av hayvanları (Cute_Fantasy Animals) ---
    // Sheet düzeni (koyun/domuz): idle satır 0-2 (2 kare, aşağı/yukarı/yan),
    // yürüme satır 3-5 = YAN(96)/YUKARI(128)/AŞAĞI(160), yatma satır 8 (ölüm).
    // Pasif: attack clip'i hiç oynamaz (idle kopyası); kaçış CHASE clip'iyle koşar.

    // [2] Koyun — çayır avı: kırılgan, vurulunca hızla kaçar (oyuncudan hızlı!)
    {
        .name = "Koyun",
        .texturePath = "assets/characters/mobs/Sheep.png",
        .level = 1,
        .maxHealth = 30.0f,
        .speed = 550.0f,          // kaçış: oyuncu ×1.1 — melee köşeye sıkıştırmalı
        .attackRange = 0.0f,
        .detectRange = 0.0f,      // pasif: yakınlık şüphesi birikmez
        .damage = 0.0f,
        .defense = 0.0f,
        .attackType = ATTACK_MELEE,
        .lootTable = SHEEP_LOOT, .lootTableCount = LOOT_COUNT(SHEEP_LOOT),
        .goldMin = 0, .goldMax = 2,
        .attackHitFrame = 99,     // asla tetiklenmez
        .passive = true, .dailyRespawn = true,
        .anims = {
            { // idle (2 kare, otlama duruşu)
                { .x=0, .y=0,   .frameW=32, .frameH=32, .frameCount=2, .frameDuration=0.5f, .feetY=30 },
                { .x=0, .y=32,  .frameW=32, .frameH=32, .frameCount=2, .frameDuration=0.5f, .feetY=30 },
                { .x=0, .y=64,  .frameW=32, .frameH=32, .frameCount=2, .frameDuration=0.5f, .feetY=30 },
            },
            { // run/kaçış (8 kare zıplama)
                { .x=0, .y=160, .frameW=32, .frameH=32, .frameCount=8, .frameDuration=0.09f, .feetY=30 },
                { .x=0, .y=128, .frameW=32, .frameH=32, .frameCount=8, .frameDuration=0.09f, .feetY=30 },
                { .x=0, .y=96,  .frameW=32, .frameH=32, .frameCount=8, .frameDuration=0.09f, .feetY=30 },
            },
            { // attack — pasif, oynamaz (idle kopyası)
                { .x=0, .y=0,   .frameW=32, .frameH=32, .frameCount=2, .frameDuration=0.5f, .feetY=30 },
                { .x=0, .y=32,  .frameW=32, .frameH=32, .frameCount=2, .frameDuration=0.5f, .feetY=30 },
                { .x=0, .y=64,  .frameW=32, .frameH=32, .frameCount=2, .frameDuration=0.5f, .feetY=30 },
            },
            { // dying — yere çöküş (yatma dizisi)
                { .x=0, .y=256, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=256, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=256, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },
            },
        },
    },

    // [3] Domuz — çayır avı (koyunla aynı davranış, sheet düzeni aynı)
    {
        .name = "Domuz",
        .texturePath = "assets/characters/mobs/Pig.png",
        .level = 1,
        .maxHealth = 30.0f,
        .speed = 550.0f,
        .attackRange = 0.0f,
        .detectRange = 0.0f,
        .damage = 0.0f,
        .defense = 0.0f,
        .attackType = ATTACK_MELEE,
        .lootTable = SHEEP_LOOT, .lootTableCount = LOOT_COUNT(SHEEP_LOOT),
        .goldMin = 0, .goldMax = 2,
        .attackHitFrame = 99,
        .passive = true, .dailyRespawn = true,
        .anims = {
            {
                { .x=0, .y=0,   .frameW=32, .frameH=32, .frameCount=2, .frameDuration=0.5f, .feetY=30 },
                { .x=0, .y=32,  .frameW=32, .frameH=32, .frameCount=2, .frameDuration=0.5f, .feetY=30 },
                { .x=0, .y=64,  .frameW=32, .frameH=32, .frameCount=2, .frameDuration=0.5f, .feetY=30 },
            },
            {
                { .x=0, .y=160, .frameW=32, .frameH=32, .frameCount=8, .frameDuration=0.09f, .feetY=30 },
                { .x=0, .y=128, .frameW=32, .frameH=32, .frameCount=8, .frameDuration=0.09f, .feetY=30 },
                { .x=0, .y=96,  .frameW=32, .frameH=32, .frameCount=8, .frameDuration=0.09f, .feetY=30 },
            },
            {
                { .x=0, .y=0,   .frameW=32, .frameH=32, .frameCount=2, .frameDuration=0.5f, .feetY=30 },
                { .x=0, .y=32,  .frameW=32, .frameH=32, .frameCount=2, .frameDuration=0.5f, .feetY=30 },
                { .x=0, .y=64,  .frameW=32, .frameH=32, .frameCount=2, .frameDuration=0.5f, .feetY=30 },
            },
            {
                { .x=0, .y=256, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=256, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=256, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },
            },
        },
    },

    // [4] Tavuk — küçük av: tek et, deri yok. Sheet düzeni farklı:
    // satır 0 idle (2), satır 1 yürüme-aşağı (6), satır 2 yürüme-yan (8), satır 7 oturma (ölüm)
    {
        .name = "Tavuk",
        .texturePath = "assets/characters/mobs/Chicken.png",
        .level = 1,
        .maxHealth = 12.0f,
        .speed = 500.0f,
        .attackRange = 0.0f,
        .detectRange = 0.0f,
        .damage = 0.0f,
        .defense = 0.0f,
        .attackType = ATTACK_MELEE,
        .lootTable = CHICKEN_LOOT, .lootTableCount = LOOT_COUNT(CHICKEN_LOOT),
        .goldMin = 0, .goldMax = 1,
        .attackHitFrame = 99,
        .passive = true, .dailyRespawn = true,
        .anims = {
            {
                { .x=0, .y=0,   .frameW=32, .frameH=32, .frameCount=2, .frameDuration=0.5f, .feetY=28 },
                { .x=0, .y=0,   .frameW=32, .frameH=32, .frameCount=2, .frameDuration=0.5f, .feetY=28 },
                { .x=0, .y=0,   .frameW=32, .frameH=32, .frameCount=2, .frameDuration=0.5f, .feetY=28 },
            },
            {
                { .x=0, .y=32,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.09f, .feetY=28 },
                { .x=0, .y=32,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.09f, .feetY=28 },
                { .x=0, .y=64,  .frameW=32, .frameH=32, .frameCount=8, .frameDuration=0.08f, .feetY=28 },
            },
            {
                { .x=0, .y=0,   .frameW=32, .frameH=32, .frameCount=2, .frameDuration=0.5f, .feetY=28 },
                { .x=0, .y=0,   .frameW=32, .frameH=32, .frameCount=2, .frameDuration=0.5f, .feetY=28 },
                { .x=0, .y=0,   .frameW=32, .frameH=32, .frameCount=2, .frameDuration=0.5f, .feetY=28 },
            },
            {
                { .x=0, .y=224, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=28 },
                { .x=0, .y=224, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=28 },
                { .x=0, .y=224, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=28 },
            },
        },
    },

    // [5] Yaban Domuzu — riskli av: saldırgan (goblin AI), deri ana kaynağı.
    // Saldırı animasyonu yok → hücum = hızlandırılmış koşu clip'i (attackHitFrame 4)
    {
        .name = "Yaban Domuzu",
        .texturePath = "assets/characters/mobs/Boar.png",
        .level = 3,
        .maxHealth = 90.0f,
        .speed = 170.0f,
        .attackRange = 55.0f,
        .detectRange = 260.0f,
        .damage = 12.0f,
        .defense = 5.0f,
        .attackType = ATTACK_MELEE,
        .lootTable = BOAR_LOOT, .lootTableCount = LOOT_COUNT(BOAR_LOOT),
        .goldMin = 0, .goldMax = 5,
        .attackHitFrame = 4,
        .dailyRespawn = true,
        .anims = {
            {
                { .x=0, .y=0,   .frameW=32, .frameH=32, .frameCount=2, .frameDuration=0.4f, .feetY=30 },
                { .x=0, .y=32,  .frameW=32, .frameH=32, .frameCount=2, .frameDuration=0.4f, .feetY=30 },
                { .x=0, .y=64,  .frameW=32, .frameH=32, .frameCount=2, .frameDuration=0.4f, .feetY=30 },
            },
            {
                { .x=0, .y=160, .frameW=32, .frameH=32, .frameCount=8, .frameDuration=0.10f, .feetY=30 },
                { .x=0, .y=128, .frameW=32, .frameH=32, .frameCount=8, .frameDuration=0.10f, .feetY=30 },
                { .x=0, .y=96,  .frameW=32, .frameH=32, .frameCount=8, .frameDuration=0.10f, .feetY=30 },
            },
            { // attack: koşu clip'i hızlı (kafa atma hücumu)
                { .x=0, .y=160, .frameW=32, .frameH=32, .frameCount=8, .frameDuration=0.06f, .feetY=30 },
                { .x=0, .y=128, .frameW=32, .frameH=32, .frameCount=8, .frameDuration=0.06f, .feetY=30 },
                { .x=0, .y=96,  .frameW=32, .frameH=32, .frameCount=8, .frameDuration=0.06f, .feetY=30 },
            },
            {
                { .x=0, .y=256, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=256, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=256, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },
            },
        },
    },
    // --- Faz 7: Slime ailesi (Cute_Fantasy Enemies/Slime) ---
    // Sheet düzeni (yönsüz blob, tüm yönler aynı satır): idle y=0 (4 kare),
    // zıplama y=1·px (8), parçalanma-ölüm y=2·px (8). Saldırı = hızlı zıplama.
    // Boylar: Büyük 64px / Orta 32px / Küçük 16px grid.
    #define SLIME_CLIP(y0, px, n, dur, fy) \
        { .x = 0, .y = (y0), .frameW = (px), .frameH = (px), \
          .frameCount = (n), .frameDuration = (dur), .feetY = (fy) }
    #define SLIME_ANIMS(px, fy) { \
        { SLIME_CLIP(0, px, 4, 0.25f, fy), SLIME_CLIP(0, px, 4, 0.25f, fy), \
          SLIME_CLIP(0, px, 4, 0.25f, fy) },                                  \
        { SLIME_CLIP(px, px, 8, 0.12f, fy), SLIME_CLIP(px, px, 8, 0.12f, fy), \
          SLIME_CLIP(px, px, 8, 0.12f, fy) },                                 \
        { SLIME_CLIP(px, px, 8, 0.07f, fy), SLIME_CLIP(px, px, 8, 0.07f, fy), \
          SLIME_CLIP(px, px, 8, 0.07f, fy) },                                 \
        { SLIME_CLIP(2*(px), px, 8, 0.09f, fy), SLIME_CLIP(2*(px), px, 8, 0.09f, fy), \
          SLIME_CLIP(2*(px), px, 8, 0.09f, fy) },                             \
    }

    // [6] Yeşil Slime (Büyük) — ova kenarı: yavaş, bölünen bela
    {
        .name = "Yeşil Slime", .texturePath = "assets/characters/mobs/Slime_Big_Green.png",
        .level = 2, .maxHealth = 120.0f, .speed = 130.0f,
        .attackRange = 70.0f, .detectRange = 260.0f, .damage = 8.0f, .defense = 0.0f,
        .attackType = ATTACK_MELEE, .attackHitFrame = 4,
        .goldMin = 0, .goldMax = 0,   // loot/altın YOK — zincir sonu Küçük'te
        .splitCount = 2, .splitChild = 7,
        .bodyW = 72.0f, .bodyH = 60.0f,
        .anims = SLIME_ANIMS(64, 56),
    },
    // [7] Yeşil Slime (Orta)
    {
        .name = "Yeşil Slime", .texturePath = "assets/characters/mobs/Slime_Medium_Green.png",
        .level = 1, .maxHealth = 50.0f, .speed = 150.0f,
        .attackRange = 45.0f, .detectRange = 240.0f, .damage = 5.0f, .defense = 0.0f,
        .attackType = ATTACK_MELEE, .attackHitFrame = 4,
        .goldMin = 0, .goldMax = 0,
        .splitCount = 2, .splitChild = 8,
        .bodyW = 40.0f, .bodyH = 34.0f,
        .anims = SLIME_ANIMS(32, 28),
    },
    // [8] Yeşil Slime (Küçük) — zincir sonu: jel + altın burada
    {
        .name = "Yeşil Slime", .texturePath = "assets/characters/mobs/Slime_Small_Green.png",
        .level = 1, .maxHealth = 20.0f, .speed = 170.0f,
        .attackRange = 30.0f, .detectRange = 220.0f, .damage = 3.0f, .defense = 0.0f,
        .attackType = ATTACK_MELEE, .attackHitFrame = 4,
        .lootTable = SLIME_G_LOOT, .lootTableCount = LOOT_COUNT(SLIME_G_LOOT),
        .goldMin = 2, .goldMax = 6,
        .bodyW = 22.0f, .bodyH = 18.0f,
        .anims = SLIME_ANIMS(16, 14),
    },
    // [9] Mavi Slime (Büyük) — bataklık: dayanıklı + vuruşu zehirler
    {
        .name = "Mavi Slime", .texturePath = "assets/characters/mobs/Slime_Big_Blue.png",
        .level = 4, .maxHealth = 160.0f, .speed = 130.0f,
        .attackRange = 70.0f, .detectRange = 280.0f, .damage = 12.0f, .defense = 5.0f,
        .attackType = ATTACK_MELEE, .attackHitFrame = 4,
        .goldMin = 0, .goldMax = 0,
        .splitCount = 2, .splitChild = 10,
        .bodyW = 72.0f, .bodyH = 60.0f,
        .hitPoisonDps = 2.0f, .hitPoisonTime = 3.0f,
        .anims = SLIME_ANIMS(64, 56),
    },
    // [10] Mavi Slime (Orta)
    {
        .name = "Mavi Slime", .texturePath = "assets/characters/mobs/Slime_Medium_Blue.png",
        .level = 2, .maxHealth = 70.0f, .speed = 150.0f,
        .attackRange = 45.0f, .detectRange = 240.0f, .damage = 8.0f, .defense = 3.0f,
        .attackType = ATTACK_MELEE, .attackHitFrame = 4,
        .goldMin = 0, .goldMax = 0,
        .splitCount = 2, .splitChild = 11,
        .bodyW = 40.0f, .bodyH = 34.0f,
        .hitPoisonDps = 2.0f, .hitPoisonTime = 3.0f,
        .anims = SLIME_ANIMS(32, 28),
    },
    // [11] Mavi Slime (Küçük) — jel + Zehirli Öz şansı
    {
        .name = "Mavi Slime", .texturePath = "assets/characters/mobs/Slime_Small_Blue.png",
        .level = 1, .maxHealth = 25.0f, .speed = 170.0f,
        .attackRange = 30.0f, .detectRange = 220.0f, .damage = 4.0f, .defense = 0.0f,
        .attackType = ATTACK_MELEE, .attackHitFrame = 4,
        .lootTable = SLIME_B_LOOT, .lootTableCount = LOOT_COUNT(SLIME_B_LOOT),
        .goldMin = 3, .goldMax = 8,
        .bodyW = 22.0f, .bodyH = 18.0f,
        .hitPoisonDps = 2.0f, .hitPoisonTime = 3.0f,
        .anims = SLIME_ANIMS(16, 14),
    },

    // --- Faz 8: İskelet ailesi (Kayalık Geçit — gece kuralıyla undead kimliği) ---
    // Sheet düzeni (goblin deseni): idle y0(aşağı)/y32(yan)/y64(yukarı) 6 kare,
    // yürüme y96/128/160 6 kare; ölüm = kemik yığınına çöküş. Diriliş satırları kullanılmıyor.

    // [12] İskelet Savaşçı — melee: hızlı, orta dayanıklı; saldırı 64×64 kılıç savurması
    {
        .name = "İskelet Savaşçı",
        .texturePath = "assets/characters/mobs/Skeleton_Swordman.png",
        .level = 4, .maxHealth = 140.0f, .speed = 180.0f,
        .attackRange = 65.0f, .detectRange = 300.0f, .damage = 12.0f, .defense = 8.0f,
        .attackType = ATTACK_MELEE, .attackHitFrame = 2, .attackRecover = 0.8f,
        .lootTable = SKEL_W_LOOT, .lootTableCount = LOOT_COUNT(SKEL_W_LOOT),
        .goldMin = 15, .goldMax = 30,
        .anims = {
            { // idle (6 kare)
                { .x=0, .y=0,   .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=64,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=32,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.15f, .feetY=30 },
            },
            { // yürüme (6 kare)
                { .x=0, .y=96,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.12f, .feetY=30 },
                { .x=0, .y=160, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.12f, .feetY=30 },
                { .x=0, .y=128, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.12f, .feetY=30 },
            },
            { // saldırı — 64×64 kılıç savurması (ork deseni, feetY 46)
                { .x=0, .y=224, .frameW=64, .frameH=64, .frameCount=4, .frameDuration=0.10f, .feetY=46 },
                { .x=0, .y=352, .frameW=64, .frameH=64, .frameCount=4, .frameDuration=0.10f, .feetY=46 },
                { .x=0, .y=288, .frameW=64, .frameH=64, .frameCount=4, .frameDuration=0.10f, .feetY=46 },
            },
            { // ölüm: kemik yığınına çöküş (4 kare)
                { .x=0, .y=192, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=192, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=192, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },
            },
        },
    },
    // [13] İskelet Okçu — İLK menzilli düşman: ok düz hatta uçar, KAÇILABİLİR.
    // Menzili (280) oyuncu yayından (340) kısa — açılışta oyuncu hep önde
    {
        .name = "İskelet Okçu",
        .texturePath = "assets/characters/mobs/Skeleton_Bowman.png",
        .level = 4, .maxHealth = 90.0f, .speed = 150.0f,
        .attackRange = 280.0f, .detectRange = 320.0f, .damage = 14.0f, .defense = 3.0f,
        .attackType = ATTACK_RANGED, .attackHitFrame = 4, .attackRecover = 1.5f,
        .projSpeed = 300.0f,
        .lootTable = SKEL_B_LOOT, .lootTableCount = LOOT_COUNT(SKEL_B_LOOT),
        .goldMin = 15, .goldMax = 30,
        .anims = {
            {
                { .x=0, .y=0,   .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=64,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=32,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.15f, .feetY=30 },
            },
            {
                { .x=0, .y=96,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.12f, .feetY=30 },
                { .x=0, .y=160, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.12f, .feetY=30 },
                { .x=0, .y=128, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.12f, .feetY=30 },
            },
            { // yay çekme — ok kare 4'te çıkar (main mermi doğurur)
                { .x=0, .y=192, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.12f, .feetY=30 },
                { .x=0, .y=256, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.12f, .feetY=30 },
                { .x=0, .y=224, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.12f, .feetY=30 },
            },
            {
                { .x=0, .y=288, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=288, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=288, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },
            },
        },
    },
    // [14] İskelet Büyücü — cam top: az can, sert ve yavaş büyü mermisi
    {
        .name = "İskelet Büyücü",
        .texturePath = "assets/characters/mobs/Skeleton_Mage.png",
        .level = 5, .maxHealth = 70.0f, .speed = 140.0f,
        .attackRange = 240.0f, .detectRange = 300.0f, .damage = 22.0f, .defense = 0.0f,
        .attackType = ATTACK_MAGIC, .attackHitFrame = 5, .attackRecover = 2.0f,
        .projSpeed = 240.0f,
        .lootTable = SKEL_M_LOOT, .lootTableCount = LOOT_COUNT(SKEL_M_LOOT),
        .goldMin = 20, .goldMax = 40,
        .anims = {
            {
                { .x=0, .y=0,   .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=64,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=32,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.15f, .feetY=30 },
            },
            {
                { .x=0, .y=96,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.12f, .feetY=30 },
                { .x=0, .y=160, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.12f, .feetY=30 },
                { .x=0, .y=128, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.12f, .feetY=30 },
            },
            { // büyü kanalize — top kare 5'te fırlar (8 kare)
                { .x=0, .y=192, .frameW=32, .frameH=32, .frameCount=8, .frameDuration=0.12f, .feetY=30 },
                { .x=0, .y=256, .frameW=32, .frameH=32, .frameCount=8, .frameDuration=0.12f, .feetY=30 },
                { .x=0, .y=224, .frameW=32, .frameH=32, .frameCount=8, .frameDuration=0.12f, .feetY=30 },
            },
            {
                { .x=0, .y=288, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=288, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=288, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },
            },
        },
    },
    // [15] İSKELET LORDU — ilk boss (Eski Geçit K2). Savaşçının büyütülmüş,
    // koyu-mor tonlu hali; faz mantığı main'de (yağmur + çağrı), leash yok.
    {
        .name = "İskelet Lordu",
        .texturePath = "assets/characters/mobs/Skeleton_Swordman.png",
        .level = 12, .maxHealth = 900.0f, .speed = 150.0f,
        .attackRange = 85.0f, .detectRange = 420.0f, .damage = 28.0f, .defense = 20.0f,
        .attackType = ATTACK_MELEE, .attackHitFrame = 2, .attackRecover = 1.1f,
        .lootTable = SKEL_LORD_LOOT, .lootTableCount = LOOT_COUNT(SKEL_LORD_LOOT),
        .goldMin = 200, .goldMax = 300,
        .bodyW = 80.0f, .bodyH = 64.0f,
        .drawScale = 3.4f,
        .tintR = 200, .tintG = 170, .tintB = 255,   // soluk mor — hükümdar havası
        .isBoss = true,
        .anims = {
            { // idle (6 kare)
                { .x=0, .y=0,   .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=64,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=32,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.15f, .feetY=30 },
            },
            { // yürüme (6 kare)
                { .x=0, .y=96,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.12f, .feetY=30 },
                { .x=0, .y=160, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.12f, .feetY=30 },
                { .x=0, .y=128, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.12f, .feetY=30 },
            },
            { // saldırı — 64×64 kılıç savurması
                { .x=0, .y=224, .frameW=64, .frameH=64, .frameCount=4, .frameDuration=0.12f, .feetY=46 },
                { .x=0, .y=352, .frameW=64, .frameH=64, .frameCount=4, .frameDuration=0.12f, .feetY=46 },
                { .x=0, .y=288, .frameW=64, .frameH=64, .frameCount=4, .frameDuration=0.12f, .feetY=46 },
            },
            { // ölüm: çöküş (4 kare)
                { .x=0, .y=192, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.20f, .feetY=30 },
                { .x=0, .y=192, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.20f, .feetY=30 },
                { .x=0, .y=192, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.20f, .feetY=30 },
            },
        },
    },

    // --- Mob Ekolojisi E1 (SONA eklendi — indeksler save/tablolarda sabit) ---
    // [16] Goblin Reisi — yuvanın kalbi: ölünce kamp ÇÖKER, kalanlar kaçışır.
    // Goblin sheet'i, iri ve kızıl tonlu; erlerden belirgin güçlü ama boss değil.
    {
        .name = "Goblin Reisi",
        .texturePath = "assets/characters/mobs/Goblin_Maceman.png",
        .nocturnal = true,   // kampıyla aynı ritim
        .level = 4,
        .maxHealth = 260.0f,
        .speed = 190.0f,
        .attackRange = 65.0f,
        .detectRange = 320.0f,
        .damage = 12.0f,
        .defense = 4.0f,
        .attackType = ATTACK_MELEE,
        .lootTable = GOBLIN_LOOT, .lootTableCount = LOOT_COUNT(GOBLIN_LOOT),
        .goldMin = 40, .goldMax = 80,
        .attackHitFrame = 3,
        .drawScale = 2.8f,
        .tintR = 255, .tintG = 170, .tintB = 150,   // kızıl savaş boyası havası
        .bodyW = 52.0f, .bodyH = 52.0f,
        .anims = {
            {
                { .x=0, .y=0,   .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.18f, .feetY=30 },
                { .x=0, .y=64,  .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.18f, .feetY=30 },
                { .x=0, .y=32,  .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.18f, .feetY=30 },
            },
            {
                { .x=0, .y=96,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=160, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=128, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.15f, .feetY=30 },
            },
            {
                { .x=0, .y=192, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.08f, .feetY=30 },
                { .x=0, .y=256, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.08f, .feetY=30 },
                { .x=0, .y=224, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.08f, .feetY=30 },
            },
            {
                { .x=0, .y=288, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=23 },
                { .x=0, .y=288, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=23 },
                { .x=0, .y=288, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=23 },
            },
        },
    },
    // [17] Ork Savaş Totemi — YAPI: kımıldamaz, saldırmaz; yıkılınca kamp çöker.
    // Görsel: assets/decor/nests.png hücre 0 (tek kare her durumda).
    #define TOTEM_CLIP { .x=0, .y=0, .frameW=48, .frameH=48, .frameCount=1, \
                         .frameDuration=1.0f, .feetY=46 }
    {
        .name = "Savaş Totemi",
        .texturePath = "assets/decor/nests.png",
        .level = 5,
        .maxHealth = 300.0f,
        .speed = 0.0f,
        .attackRange = 0.0f,
        .detectRange = 0.0f,
        .damage = 0.0f,
        .defense = 8.0f,
        .attackType = ATTACK_MELEE,
        .goldMin = 0, .goldMax = 0,
        .attackHitFrame = 0,
        .isStructure = true,
        .bodyW = 56.0f, .bodyH = 56.0f,
        .anims = {
            { TOTEM_CLIP, TOTEM_CLIP, TOTEM_CLIP },
            { TOTEM_CLIP, TOTEM_CLIP, TOTEM_CLIP },
            { TOTEM_CLIP, TOTEM_CLIP, TOTEM_CLIP },
            { TOTEM_CLIP, TOTEM_CLIP, TOTEM_CLIP },
        },
    },
};

const int MOB_TYPE_COUNT = (int)(sizeof(MOB_TYPES) / sizeof(MOB_TYPES[0]));

// Yere yatık bir oval (hedef halkası) çizer. cx,cy = merkez (ekran), 
// rx,ry = yatay/dikey yarıçap.
static void draw_target_ring(R2D *renderer, float cx, float cy, float rx, float ry)
{
    int segments = 24;   // ovali kaç parçaya bölelim (çok = pürüzsüz)
    float prevX = cx + rx;   // ilk nokta (açı 0)
    float prevY = cy;
    for (int i = 1; i <= segments; i++) {
        float angle = (float)i / segments * 2.0f * 3.14159265f;  // 0 → 2π
        float x = cx + rx * SDL_cosf(angle);
        float y = cy + ry * SDL_sinf(angle);
        r2d_line(renderer, prevX, prevY, x, y);   // önceki noktadan bu noktaya çizgi
        prevX = x;
        prevY = y;
    }
}

// Bölge içinde rastgele bir konum üretir.
static void zone_random_pos(const SpawnZone *z, float *outX, float *outY)
{
    float angle = (float)(rand() % 360) * 3.14159265f / 180.0f;
    float dist = (float)(rand() % (int)z->radius);
    *outX = z->x + SDL_cosf(angle) * dist;
    *outY = z->y + SDL_sinf(angle) * dist;
}

void mob_render(const Mob *m, R2D *renderer, R2DTexture **mob_textures, const Camera *camera, int targetMobId)
{
    if (m->type == NULL) return;   // boş slot → çizme

    if (!m->e.alive && !m->dying) return;   // ne canlı ne ölüyorsa çizme

    // Aktif animasyon clip'i: durum + yön (RETURN'un kendi satırı yok → koşma clip'i)
    MobState animState = (m->state == MOB_RETURN) ? MOB_CHASE
                       : (m->state == MOB_SEARCH) ? MOB_IDLE : m->state;
    const AnimClip *clip = &m->type->anims[animState][m->e.facing];

    // Kaynak: clip'in başlangıcından currentFrame kadar kay (clip'in kendi kare boyutuyla)
    SDL_FRect src = {
        .x = clip->x + m->currentFrame * clip->frameW,
        .y = clip->y,
        .w = clip->frameW,
        .h = clip->frameH
    };

    float zoom = camera->zoom;
    float scale = (m->type->drawScale > 0.0f) ? m->type->drawScale : 2.0f;
    float drawW = clip->frameW * scale;
    float drawH = clip->frameH * scale;
    float feetOffset = clip->feetY * scale;   // ayak, ölçekli

    // Once dunya-uzayinda kose, sonra zoom ile ekrana
    float mobWorldX = (m->e.rect.x + m->e.rect.w / 2.0f) - drawW / 2.0f;
    float mobWorldY = (m->e.rect.y + m->e.rect.h) - feetOffset;
    SDL_FRect screen = {
        .x = (mobWorldX - camera->x) * zoom,
        .y = (mobWorldY - camera->y) * zoom,
        .w = drawW * zoom,
        .h = drawH * zoom
    };

    // Seçili hedefse, ayağında halka çiz (sprite'tan ÖNCE)
    if (m->id == targetMobId) {
        float ringCx = ((m->e.rect.x + m->e.rect.w / 2.0f) - camera->x) * zoom;
        float ringCy = ((m->e.rect.y + m->e.rect.h) - camera->y) * zoom;
        r2d_set_color(renderer, 255, 50, 50, 255);
        draw_target_ring(renderer, ringCx, ringCy,
                         m->e.rect.w * 0.7f * zoom, m->e.rect.w * 0.35f * zoom);
    }

    SDL_FlipMode flip = m->e.facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

    int typeIndex = (int)(m->type - MOB_TYPES);
    R2DTexture *texture = mob_textures[typeIndex];

    if (m->e.hitFlash > 0.0f)
        r2d_texture_set_color_mod(texture, 255, 150, 150);
    else if (m->type->tintR != 0 || m->type->tintG != 0 || m->type->tintB != 0)
        r2d_texture_set_color_mod(texture, m->type->tintR, m->type->tintG, m->type->tintB);
    else
        r2d_texture_set_color_mod(texture, 255, 255, 255);

    r2d_draw_texture_flip(renderer, texture, &src, &screen, flip == SDL_FLIP_HORIZONTAL);

    // Şüphe göstergesi: fark etme süreci dolarken baş üstünde sarı çubuk;
    // alarm anında kısa süre kırmızı "!" (dikdörtgenlerle — font gerekmez)
    if (m->e.alive) {
        float headX = ((m->e.rect.x + m->e.rect.w / 2.0f) - camera->x) * zoom;
        float headY = screen.y - 8.0f * zoom;
        if (m->alertFlash > 0.0f) {
            r2d_set_color(renderer, 255, 60, 50, 255);
            r2d_fill_rect(renderer, &(SDL_FRect){ headX - 2*zoom, headY - 14*zoom, 4*zoom, 9*zoom });
            r2d_fill_rect(renderer, &(SDL_FRect){ headX - 2*zoom, headY - 3*zoom,  4*zoom, 4*zoom });
        } else if (!m->aggro && m->suspicion > 0.05f) {
            SDL_FRect bg = { headX - 12*zoom, headY - 6*zoom, 24*zoom, 4*zoom };
            SDL_FRect fill = bg;
            fill.w *= m->suspicion;
            r2d_set_color(renderer, 25, 25, 30, 200);
            r2d_fill_rect(renderer, &bg);
            r2d_set_color(renderer, 255, 210, 60, 255);
            r2d_fill_rect(renderer, &fill);
        } else if (m->sleeping) {
            // Uyku "z"si (E2): baş üstünde yavaşça soluyan küçük Z (font gerekmez)
            float t = (float)((SDL_GetTicks() / 100) % 20) / 20.0f;
            Uint8 a = (Uint8)(120 + 100 * (t < 0.5f ? t * 2.0f : (1.0f - t) * 2.0f));
            r2d_set_color(renderer, 200, 220, 255, a);
            float zx = headX + 6 * zoom, zy = headY - 12 * zoom;
            r2d_fill_rect(renderer, &(SDL_FRect){ zx, zy,            5*zoom, 1.4f*zoom });
            r2d_fill_rect(renderer, &(SDL_FRect){ zx, zy + 4.6f*zoom, 5*zoom, 1.4f*zoom });
            r2d_line(renderer, zx + 4.4f*zoom, zy + 1.4f*zoom, zx + 0.6f*zoom, zy + 4.6f*zoom);
        }
    }

    #if DEBUG_DRAW
    SDL_FRect rectScreen = {
        m->e.rect.x - camera->x,
        m->e.rect.y - camera->y,
        m->e.rect.w,
        m->e.rect.h
    };
    r2d_set_color(renderer, 255, 0, 0, 255);
    r2d_rect(renderer, &rectScreen);
    #endif
}

void mob_update(Mob *m, Mob *mobs, int mobCount, float targetX, float targetY, float dt,
                float detectMul)
{
    if (m->type == NULL) return;   // boş slot (hiç mob doğmamış) → atla

    m->attackTriggered = false;

    // Buz Oku yavaşlatması: süre boyunca hareket hızı çarpanı düşük
    float slowK = (m->slowTimer > 0.0f) ? m->slowMult : 1.0f;
    if (m->slowTimer > 0.0f) m->slowTimer -= dt;
    if (m->tauntTimer > 0.0f) m->tauntTimer -= dt;   // M4: kabuk taunt'u söner

    // Ölüm animasyonu oynuyorsa: sadece onu ilerlet, AI çalışmaz
    if (m->dying) {
        const AnimClip *clip = &m->type->anims[MOB_DYING][m->e.facing];
        m->animTimer += dt;
        if (m->animTimer >= clip->frameDuration) {
            m->animTimer -= clip->frameDuration;
            m->currentFrame++;
            if (m->currentFrame >= clip->frameCount) {
                // Death animasyonu bitti → mob tamamen kayboldu
                m->dying = false;
                m->currentFrame = clip->frameCount - 1;  // son karede kal (ya da kaybol)
            }
        }
        return;   // dying'de AI yok
    }

    if (!m->e.alive) return;   // ölü (ve death anim bitmiş) → işlem yok

    // Flaş ve knockback (itilme de duvara çarpar)
    if (m->e.hitFlash > 0.0f) m->e.hitFlash -= dt;
    mob_move(m, m->e.knockbackX * dt, m->e.knockbackY * dt);
    float damping = 12.0f;
    m->e.knockbackX -= m->e.knockbackX * damping * dt;
    m->e.knockbackY -= m->e.knockbackY * damping * dt;

    if (m->attackCooldown > 0.0f) m->attackCooldown -= dt;

    float dx = targetX - (m->e.rect.x + m->e.rect.w/2.0f);
    float dy = targetY - (m->e.rect.y + m->e.rect.h/2.0f);
    float dist = SDL_sqrtf(dx*dx + dy*dy);

    // Gece/gündüz ritmi (E2): gececi gündüz uyuklar — algı çöker, devriye durur.
    // (Uyuyana vuruş sinsi sayılır — main. Aggro olan uyanıktır.)
    m->sleeping = m->type->nocturnal && !gametime_is_night() && !m->aggro;
    float detectRange = m->type->detectRange * detectMul;   // gece daha uzağı görür
    if (m->sleeping) detectRange *= 0.35f;
    float attackRange = m->type->attackRange;

    // --- PASİF AV (Faz 6): asla saldırmaz; vuruş/kamp alarmı = ürküp KAÇAR ---
    // Leash'ten ÖNCE: kaçan hayvan RETURN dokunulmazlığına (iyileşme) girmesin.
    // Yuva çöktü (ekoloji): kalanlar kaçışır, süre bitince sessizce gider
    if (m->scatter)
    {
        m->fleeTimer -= dt;
        if (m->fleeTimer <= 0.0f) { m->e.alive = false; return; }
        m->state = MOB_CHASE;
        if (dist > 0.0f) {
            float nx = -dx / dist, ny = -dy / dist;
            if (SDL_fabsf(nx) > SDL_fabsf(ny)) {
                m->e.facing = 2;
                m->e.facingLeft = (nx < 0);
            } else {
                m->e.facing = (ny < 0) ? 1 : 0;
            }
            mob_move(m, nx * m->e.speed * 1.3f * dt, ny * m->e.speed * 1.3f * dt);
        }
        const AnimClip *sclip = &m->type->anims[m->state][m->e.facing];
        if (m->state != m->lastState || m->e.facing != m->lastFacing) {
            m->currentFrame = 0; m->animTimer = 0.0f;
            m->lastState = m->state; m->lastFacing = m->e.facing;
        }
        m->animTimer += dt;
        if (m->animTimer >= sclip->frameDuration) {
            m->animTimer -= sclip->frameDuration;
            m->currentFrame = (m->currentFrame + 1) % sclip->frameCount;
        }
        return;
    }

    // Yapı-mob (totem): hiçbir AI yok — durur, vurulur, yıkılır
    if (m->type->isStructure)
        return;

    if (m->type->passive)
    {
        if (m->aggro) {              // main her vuruşta aggro basar → kaçışa çevrilir
            m->fleeTimer = MOB_FLEE_TIME;
            m->aggro = false;
            m->alertFlash = 0.6f;    // baş üstü "!" — ürktü
            m->suspicion = 0.0f;
        }
        m->sleeping = gametime_is_night() && m->fleeTimer <= 0.0f;   // hayvan gece uyur
        float hrs = gametime_hours();
        if (m->fleeTimer > 0.0f && dist > 0.0f) {
            m->fleeTimer -= dt;
            m->state = MOB_CHASE;    // koşu clip'i (kaçış — düz hat, zigzag yok)
            float nx = -dx / dist, ny = -dy / dist;
            if (SDL_fabsf(nx) > SDL_fabsf(ny)) {
                m->e.facing = 2;
                m->e.facingLeft = (nx < 0);
            } else {
                m->e.facing = (ny < 0) ? 1 : 0;
            }
            mob_move(m, nx * m->e.speed * slowK * dt, ny * m->e.speed * slowK * dt);
        } else if (!m->sleeping && m->waterX != 0.0f && hrs >= 10.0f && hrs < 14.0f &&
                   !weather_is_wet()) {
            // Su ziyareti (E2): öğlen su kenarına iner — avcıya pusu fırsatı
            // (yağmurda gerek yok: su ayağına geldi)
            float wdx = m->waterX - (m->e.rect.x + m->e.rect.w / 2.0f);
            float wdy = m->waterY - (m->e.rect.y + m->e.rect.h / 2.0f);
            float wd = SDL_sqrtf(wdx * wdx + wdy * wdy);
            if (wd > 14.0f) {
                m->state = MOB_CHASE;   // yürüyüş (yarı hız — telaşsız)
                if (SDL_fabsf(wdx) > SDL_fabsf(wdy)) {
                    m->e.facing = 2;
                    m->e.facingLeft = (wdx < 0);
                } else {
                    m->e.facing = (wdy < 0) ? 1 : 0;
                }
                mob_move(m, wdx / wd * m->e.speed * 0.45f * slowK * dt,
                            wdy / wd * m->e.speed * 0.45f * slowK * dt);
            } else {
                m->state = MOB_IDLE;    // su başında dinlenir
            }
        } else {
            m->state = MOB_IDLE;     // otlamaya döner (gece: uyku — kımıldamaz)
        }
        if (m->alertFlash > 0.0f) m->alertFlash -= dt;

        // Animasyon ilerletme (alttaki ortak blokla aynı desen)
        const AnimClip *pclip = &m->type->anims[m->state][m->e.facing];
        if (m->state != m->lastState || m->e.facing != m->lastFacing) {
            m->currentFrame = 0;
            m->animTimer = 0.0f;
            m->lastState = m->state;
            m->lastFacing = m->e.facing;
        }
        m->animTimer += dt;
        if (m->animTimer >= pclip->frameDuration) {
            m->animTimer -= pclip->frameDuration;
            m->currentFrame = (m->currentFrame + 1) % pclip->frameCount;
        }
        return;
    }

    // --- LEASH: kamptan çok uzaklaştıysa peşi bırak, eve dön ---
    float hdx = m->homeX - (m->e.rect.x + m->e.rect.w/2.0f);
    float hdy = m->homeY - (m->e.rect.y + m->e.rect.h/2.0f);
    float homeDist = SDL_sqrtf(hdx*hdx + hdy*hdy);

    if (m->state != MOB_RETURN && m->state != MOB_SEARCH &&
        homeDist > MOB_LEASH_RANGE && !m->type->isBoss) {
        // ARAMA (E2+): kovalayan mob izi hemen bırakmaz — olduğu yerde
        // bakınır (sis + sinsilikle saklambaç). Pasif av aramaz, direkt döner.
        bool wasHunting = m->aggro && !m->type->passive && !m->scatter;
        m->attacking = false;
        m->aggro = false;      // alarm biter — dönünce normal algıya döner
        m->suspicion = 0.0f;   // şüphe de sıfırlanır (temiz sayfa)
        if (wasHunting) {
            m->state = MOB_SEARCH;
            m->searchTimer = 8.0f;
            m->patrolTimer = 0.0f;   // bakınma yönü sayacı (yeniden kullanım)
        } else {
            m->state = MOB_RETURN;
        }
    }

    if (m->state == MOB_SEARCH) {
        m->searchTimer -= dt;

        // Dikilip bakınır: 1.4-2.4 sn'de bir yön değiştirir (IDLE clip'i oynar)
        m->patrolTimer -= dt;
        if (m->patrolTimer <= 0.0f) {
            m->patrolTimer = 1.4f + randf();
            m->e.facing = (int)(randf() * 2.99f);
            m->e.facingLeft = randf() < 0.5f;
        }
        const AnimClip *sclip = &m->type->anims[MOB_IDLE][m->e.facing];
        m->animTimer += dt;
        if (m->animTimer >= sclip->frameDuration) {
            m->animTimer -= sclip->frameDuration;
            m->currentFrame = (m->currentFrame + 1) % sclip->frameCount;
        }
        m->lastState = MOB_SEARCH;

        // Oyuncu yakına dönerse: yeniden alarm (arama boşa değil)
        float sdx2 = targetX - (m->e.rect.x + m->e.rect.w / 2.0f);
        float sdy2 = targetY - (m->e.rect.y + m->e.rect.h / 2.0f);
        float sd2 = SDL_sqrtf(sdx2 * sdx2 + sdy2 * sdy2);
        if (sd2 < m->type->detectRange * 0.6f) {
            m->aggro = true;
            m->suspicion = 1.0f;
            m->alertFlash = 0.6f;
            m->state = MOB_CHASE;
        }
        else if (m->searchTimer <= 0.0f)
            m->state = MOB_RETURN;   // pes etti — kampa döner
        return;   // arama sırasında normal algı/saldırı akışı yok
    }

    if (m->state == MOB_RETURN) {
        // Dönüşte hızla iyileşir (KO tarzı: kite/tren istismarı boşa düşer)
        m->e.health += m->e.maxHealth * MOB_RETURN_HEAL_RATE * dt;
        if (m->e.health > m->e.maxHealth) m->e.health = m->e.maxHealth;

        if (homeDist > 8.0f) {
            // Yön eve doğru
            if (SDL_fabsf(hdx) > SDL_fabsf(hdy)) {
                m->e.facing = 2;
                m->e.facingLeft = (hdx < 0);
            } else {
                m->e.facing = (hdy < 0) ? 1 : 0;
            }
            mob_move(m, hdx/homeDist * m->e.speed * slowK * dt,
                        hdy/homeDist * m->e.speed * slowK * dt);

            // Koşma animasyonu (RETURN'un kendi satırı yok)
            const AnimClip *clip = &m->type->anims[MOB_CHASE][m->e.facing];
            m->animTimer += dt;
            if (m->animTimer >= clip->frameDuration) {
                m->animTimer -= clip->frameDuration;
                m->currentFrame = (m->currentFrame + 1) % clip->frameCount;
            }
            m->lastState = MOB_RETURN;
        } else {
            m->state = MOB_IDLE;   // vardı — normal AI'a dön
        }
        return;   // dönüş sırasında algı/saldırı yok
    }

    if (m->attacking) {
        // --- SALDIRI SÜRÜYOR: tek-seferlik animasyon ---
        m->state = MOB_ATTACK;   // attack clip'i (durum 2)
        const AnimClip *clip = &m->type->anims[MOB_ATTACK][m->e.facing];

        m->animTimer += dt;
        if (m->animTimer >= clip->frameDuration) {
            m->animTimer -= clip->frameDuration;
            m->currentFrame++;

            // Vuruş karesinde hasar tetikle
            if (m->currentFrame == m->type->attackHitFrame) {
                if (dist < attackRange + 20.0f)
                    m->attackTriggered = true;
            }

            // Son kareye ulaşınca saldırı bitti (menzilli türler kendi temposunda)
            if (m->currentFrame >= clip->frameCount) {
                m->attacking = false;
                m->currentFrame = 0;
                m->animTimer = 0.0f;
                m->attackCooldown = (m->type->attackRecover > 0.0f)
                                  ? m->type->attackRecover : 0.6f;
                m->state = MOB_IDLE;   // idle'a dön
            }
        }
    }
    else {
        // --- SALDIRI YOK: şüphe / idle / chase ---

        // ŞÜPHE: algı ikili değil — menzilde birikir (yakınsa hızlı), dışında söner.
        // 1'e dolunca tam alarm (aggro): artık eğilmek/uzaklaşmak kurtarmaz (leash'e kadar).
        if (!m->aggro) {
            if (dist < detectRange) {
                float rate = 0.5f + 3.0f * (1.0f - dist / detectRange);   // dipte ~0.3sn, sınırda ~2sn
                m->suspicion += rate * dt;
                if (m->suspicion >= 1.0f) {
                    m->suspicion = 1.0f;
                    m->aggro = true;
                    m->alertFlash = 0.8f;   // baş üstü "!"
                }
            } else {
                m->suspicion -= 0.6f * dt;   // gözden kaybol → yavaşça unutur
                if (m->suspicion < 0.0f) m->suspicion = 0.0f;
            }
        }
        if (m->alertFlash > 0.0f) m->alertFlash -= dt;

        // Yön: şüphelenen ya da alarmdaki mob oyuncuya döner (kolaçan hissi)
        if (dist > 0.0f && (m->suspicion > 0.25f || m->aggro)) {
            if (SDL_fabsf(dx) > SDL_fabsf(dy)) {
                m->e.facing = 2;
                m->e.facingLeft = (dx < 0);
            } else {
                m->e.facing = (dy < 0) ? 1 : 0;
            }
        }

        // Durum: yalnız TAM alarm kovalar (şüpheli mob yerinde bakınır)
        if (m->aggro && dist >= attackRange) {
            m->state = MOB_CHASE;
            if (dist > 0.0f) {
                float nx = dx / dist, ny = dy / dist;

                // Separation: yakın mob'lardan uzaklaş
                float sepX = 0.0f, sepY = 0.0f;
                for (int j = 0; j < mobCount; j++) {
                    if (&mobs[j] == m) continue;
                    if (!mobs[j].e.alive) continue;
                    float ox = m->e.rect.x - mobs[j].e.rect.x;
                    float oy = m->e.rect.y - mobs[j].e.rect.y;
                    float d = SDL_sqrtf(ox*ox + oy*oy);
                    if (d > 0.0f && d < SEPARATION_RADIUS) {
                        float strength = (SEPARATION_RADIUS - d) / SEPARATION_RADIUS;
                        sepX += (ox / d) * strength;
                        sepY += (oy / d) * strength;
                    }
                }

                // Oyuncuya doğru + ayrışma (tile/dekor çarpışmalı)
                mob_move(m, (nx * m->e.speed * slowK + sepX * SEPARATION_FORCE) * dt,
                            (ny * m->e.speed * slowK + sepY * SEPARATION_FORCE) * dt);
            }
        }
        else if (!m->sleeping && m->suspicion < 0.25f && m->zoneId >= 0 &&
                 !(weather_is_wet() && m->type->nocturnal) &&   // yağmur: goblin sığınır
                 (m->type->nocturnal ||          // gececi: aktif saatinde hep devriyede
                  !gametime_is_night() ||        // gündüzcü olmayan (ork): gündüz herkes
                  (m->id % 2 == 0)))             // gece yalnız nöbetçi yarısı (E2)
        {
            // Devriye (E2): yuva çevresinde nokta nokta tur — "slotta bekleme" ölür
            m->patrolTimer -= dt;
            if (m->patrolTimer <= 0.0f) {
                m->patrolAngle += 1.8f + randf() * 1.6f;     // sıradaki nokta
                m->patrolTimer = 3.5f + randf() * 4.0f;
            }
            float ptx = m->homeX + SDL_cosf(m->patrolAngle) * 70.0f;
            float pty = m->homeY + SDL_sinf(m->patrolAngle) * 70.0f;
            float pdx = ptx - (m->e.rect.x + m->e.rect.w / 2.0f);
            float pdy = pty - (m->e.rect.y + m->e.rect.h / 2.0f);
            float pd = SDL_sqrtf(pdx * pdx + pdy * pdy);
            if (pd > 12.0f) {
                m->state = MOB_CHASE;   // yürüyüş clip'i (yarı hız — devriye telaşsız)
                if (SDL_fabsf(pdx) > SDL_fabsf(pdy)) {
                    m->e.facing = 2;
                    m->e.facingLeft = (pdx < 0);
                } else {
                    m->e.facing = (pdy < 0) ? 1 : 0;
                }
                mob_move(m, pdx / pd * m->e.speed * 0.4f * slowK * dt,
                            pdy / pd * m->e.speed * 0.4f * slowK * dt);
            } else {
                m->state = MOB_IDLE;    // noktada soluklan
            }
        }
        else {
            m->state = MOB_IDLE;   // idle clip'i (durum 0)
        }

        // Saldırı menzilinde + ALARMDA + cooldown bittiyse → saldırı başlat
        // (fark etmemiş mobun dibinde durmak saldırı yemez — ama şüphe dipte
        //  ~0.3 sn'de dolar, uzun süre dibinde duramazsın)
        if (m->aggro && dist < attackRange && m->attackCooldown <= 0.0f) {
            m->attacking = true;
            m->currentFrame = 0;
            m->animTimer = 0.0f;
        }

        // Döngüsel animasyon (aktif clip: state + facing)
        const AnimClip *clip = &m->type->anims[m->state][m->e.facing];

        // Geçiş: durum ya da yön değiştiyse kareyi sıfırla
        if (m->state != m->lastState || m->e.facing != m->lastFacing) {
            m->currentFrame = 0;
            m->animTimer = 0.0f;
            m->lastState = m->state;
            m->lastFacing = m->e.facing;
        }

        m->animTimer += dt;
        if (m->animTimer >= clip->frameDuration) {
            m->animTimer -= clip->frameDuration;
            m->currentFrame = (m->currentFrame + 1) % clip->frameCount;
        }
    }
}

// Verilen ID'ye sahip canlı mob'u bul. Yoksa NULL.
Mob *mob_find_by_id(Mob *mobs, int mobCount, int id)
{
    if (id == 0) return NULL;

    for (int i = 0; i < mobCount; i++) {
        if (mobs[i].e.alive && mobs[i].id == id)
            return &mobs[i];
    }
    return NULL;
}

// Z'ye basınca: en yakın canlı mob'u hedefle
int mob_find_nearest(Mob *mobs, int mobCount, float px, float py)
{
    int bestId = 0;
    float bestDist = 999999.0f;
    for (int i = 0; i < mobCount; i++) {
        if (!mobs[i].e.alive) continue;
        float dx = (mobs[i].e.rect.x + mobs[i].e.rect.w/2.0f) - px;
        float dy = (mobs[i].e.rect.y + mobs[i].e.rect.h/2.0f) - py;
        float d = dx*dx + dy*dy;   // karekök gereksiz (sadece karşılaştırma)
        if (d < bestDist) {
            bestDist = d;
            bestId = mobs[i].id;
        }
    }
    return bestId;   // 0 = mob yok
}

// Hedef mob'un HP barını ekranın üstünde çizer (KO tarzı sabit panel).
void target_hp_bar_render(R2D *renderer, const Mob *target, int winW, TTF_Font *font)
{
    if (target == NULL) return;   // hedef yoksa çizme

    // Bar ölçüleri — ekranın üst-ortasında
    float barW = 240.0f, barH = 22.0f;
    float barX = (winW - barW) / 2.0f;   // yatay ortala
    float barY = 20.0f;                   // üstten boşluk

    // 1) Arka plan (boş bar)
    SDL_FRect bg = { barX, barY, barW, barH };
    r2d_set_color(renderer, 40, 20, 20, 255);
    r2d_fill_rect(renderer, &bg);

    // 2) Dolu kısım (cana oranlı)
    float ratio = target->e.health / target->e.maxHealth;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    SDL_FRect fill = { barX, barY, barW * ratio, barH };
    r2d_set_color(renderer, 200, 40, 40, 255);   // kırmızı
    r2d_fill_rect(renderer, &fill);

    // 3) Çerçeve
    r2d_set_color(renderer, 255, 255, 255, 255);
    r2d_rect(renderer, &bg);


    // Mob ismi — bar'ın ÜSTÜNE, ortalı
    if (font != NULL && target->type != NULL) {
        SDL_Color white = { 255, 255, 255, 255 };

        // Yazının piksel boyutunu ölç (ortalamak için genişlik lazım)
        int textW = 0, textH = 0;
        TTF_GetStringSize(font, target->type->name, 0, &textW, &textH);

        // Bar'a göre yatay ortala
        float textX = barX + (barW - textW) / 2.0f;
        float textY = barY - 20.0f;   // bar'ın üstünde

        draw_text(renderer, font, target->type->name, textX, textY, white);
    }
}

// Boş slota mob doğurur. Doğan mob'un ID'sini döndürür (0 = başarısız).
int mob_spawn(Mob *mobs, int maxMobs, int *nextMobId, const MobType *type, float x, float y, int zoneId)
{
    int slot = -1;
    for (int i = 0; i < maxMobs; i++) {
        if (!mobs[i].e.alive && !mobs[i].dying) { slot = i; break; }
    }
    if (slot == -1) return 0;   // boş slot yok

    // Katı zeminde (su/kaya/ev) doğma: yakında boş nokta ara.
    // 12 denemede bulunamazsa yine de doğar (kamp tamamen kayalıksa takılmasın).
    for (int attempt = 0; attempt < 12; attempt++) {
        SDL_FRect fb = feet_box(x, y, 48.0f, 64.0f);
        if (!box_hits_solid(fb) && !decor_blocks(fb))
            break;
        x += (randf() - 0.5f) * 250.0f;
        y += (randf() - 0.5f) * 250.0f;
    }

    int id = (*nextMobId)++;
    float bw = (type->bodyW > 0.0f) ? type->bodyW : 48.0f;   // tür boyu (slime'lar farklı)
    float bh = (type->bodyH > 0.0f) ? type->bodyH : 64.0f;
    mobs[slot] = (Mob){
        .id = id,
        .type = type,
        .e = { .rect = { .x=x, .y=y, .w=bw, .h=bh },
               .health = type->maxHealth, .maxHealth = type->maxHealth,
               .speed = type->speed, .alive = true },
        .state = MOB_IDLE, .attackCooldown = 0.0f,
        .lastState = -1, .lastFacing = -1,
        .zoneId = zoneId,
        .homeX = x + bw / 2.0f, .homeY = y + bh / 2.0f,   // leash referansı (rect merkezi)
    };
    return id;   // doğan mob'un ID'si
}

void mob_alert_camp(Mob *mobs, int mobCount, const Mob *victim)
{
    if (victim->zoneId < 0) return;   // kampsız mob alarm veremez

    float vx = victim->e.rect.x + victim->e.rect.w / 2.0f;
    float vy = victim->e.rect.y + victim->e.rect.h / 2.0f;

    for (int j = 0; j < mobCount; j++) {
        Mob *o = &mobs[j];
        if (o == victim || !o->e.alive || o->dying) continue;
        if (o->zoneId != victim->zoneId) continue;        // sadece kamp arkadaşları
        if (o->state == MOB_RETURN) continue;             // dönen mob alarmı duymaz

        float dx = (o->e.rect.x + o->e.rect.w / 2.0f) - vx;
        float dy = (o->e.rect.y + o->e.rect.h / 2.0f) - vy;
        if (SDL_sqrtf(dx * dx + dy * dy) <= MOB_AGGRO_SHARE_RANGE) {
            o->aggro = true;   // algı menzili yok sayılır → saldırıya geçer
            o->suspicion = 1.0f;
            o->alertFlash = 0.8f;   // kamp alarmı görünür olsun
        }
    }
}


// Oyun başında: tüm bölgeleri maxCount kadar mob'la doldur (respawn beklemeden).
void spawn_zones_fill(SpawnZone *zones, int zoneCount, Mob *mobs, int maxMobs, int *nextMobId)
{
    for (int i = 0; i < zoneCount; i++) {
        SpawnZone *z = &zones[i];

        // Mevcutları say — EKSİĞİ doldur (günlük av yenilemesi taşırmasın)
        int aliveCount = 0;
        for (int j = 0; j < maxMobs; j++) {
            if (mobs[j].zoneId == i && (mobs[j].e.alive || mobs[j].dying))
                aliveCount++;
        }

        for (int n = aliveCount; n < z->maxCount; n++) {
            // Bölge içinde rastgele konum
            float sx, sy;
            zone_random_pos(z, &sx, &sy);
            mob_spawn(mobs, maxMobs, nextMobId, z->type, sx, sy, i);
        }
    }
}