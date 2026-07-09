#ifndef MOB_H
#define MOB_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>   // TTF_Font (target_hp_bar_render)
#include <stdbool.h>
#include "common.h"
#include "item.h"   // LootEntry (tür başına loot tablosu)


#define MAX_MOBS 48        // dünya geneli aynı anda max mob (bölge kampları toplamı + pay)
#define MAX_MOB_TYPES 16   // tür tablosu kapasitesi (tile'lardaki MAX_TILE_TYPES gibi)
#define MAX_SPAWN_ZONES 32 // bölge kamplarından kurulabilecek max spawn bölgesi

// Dünyada var olan, hareket eden, hasar alabilen her şeyin ortak çekirdeği.
typedef struct {
    SDL_FRect rect;            // konum + boyut (collision/hurtbox)
    float health, maxHealth;   // can
    bool alive;                // yaşıyor mu
    int facing;                // 0=aşağı, 1=yukarı, 2=yan
    bool facingLeft;
    float speed;               // hareket hızı
    float hitFlash;            // vuruş flaşı (saniye, geri sayar)
    float knockbackX, knockbackY;  // geri itilme hızı
} Entity;

// Düşmanın davranış durumu
typedef enum {
    MOB_IDLE,    // bekliyor
    MOB_CHASE,   // oyuncuyu kovalıyor
    MOB_ATTACK,  // saldırı menzilinde
    MOB_DYING,   // ölüm animasyonu oynuyor
    MOB_RETURN,  // leash: kampına dönüyor (oyuncuyu yok sayar, hızla iyileşir)
                 // DİKKAT: anims[4][3]'te satırı YOK — animasyonu CHASE clip'idir
} MobState;

#define MOB_LEASH_RANGE 550.0f       // kamptan bu kadar uzaklaşınca peşi bırakır
#define MOB_RETURN_HEAL_RATE 0.35f   // dönüşte saniyede max canın yüzdesi
#define MOB_AGGRO_SHARE_RANGE 300.0f // vurulan mobun bu çaptaki kamp arkadaşları saldırır

// Saldırı türü — mob nasıl saldırır
typedef enum {
    ATTACK_MELEE,    // yakın dövüş: menzilde anında hasar
    ATTACK_RANGED,   // okçu: mermi fırlatır (İLERİDE — mermi sistemi gerekir)
    ATTACK_MAGIC,    // büyücü: büyü fırlatır (İLERİDE)
} AttackType;


// Mob TÜRÜ — bir kez tanımlanır, tüm o türdeki mob'lar paylaşır.
// (TileType gibi: sabit, türe-özel veriler burada)
typedef struct {
    const char *name;          // "Goblin Maceman" (HP bar'da gösterilir)
    const char *texturePath;   // sprite dosyası

    // savaş/hareket istatistikleri
    int level;                 // mob seviyesi (XP azalan getirisi + ileride UI)
    float maxHealth;
    float speed;
    float attackRange;         // saldırı menzili (türe göre — okçu uzun, melee kısa)
    float detectRange;         // oyuncuyu fark etme menzili
    float damage;              // saldırı başına hasar
    float defense;             // defans (oyuncu hasarı 100/(100+def) ile kısılır)
    AttackType attackType;     // nasıl saldırır

    // Loot: her girdi için ayrı zar atılır (bkz. loot_spawn)
    const LootEntry *lootTable;
    int lootTableCount;
    int goldMin, goldMax;      // düşen altın aralığı

     // Animasyon düzeni (türe özel)
    int attackHitFrame;      // saldırının hangi karesinde hasar

    AnimClip anims[4][3];
} MobType;

// Düşman = ortak çekirdek + AI'a özel alanlar
typedef struct {
    int id;
    Entity e;                 // ortak çekirdek (konum, can, hareket, vuruş tepkisi)
    const MobType *type;   // mob hangi türden (goblin? ork?)
    int zoneId;           // hangi spawn bölgesinden doğdu (-1 = hiçbiri/manuel)
    float homeX, homeY;   // doğduğu nokta (leash bunu referans alır)
    bool aggro;           // vuruldu/kamp alarmı: algı menzili yok sayılır (leash sıfırlar)
    MobState state;         // AI durumu
    float attackCooldown;     // bir sonraki saldırıya kalan süre
    bool attackTriggered;
    // animasyon
    int currentFrame;     // hangi karedeyiz
    float animTimer;      // kare ilerleme sayacı
    bool attacking;       // şu an bir saldırı animasyonu oynuyor mu?
    MobState lastState;        // geçiş tespiti: önceki durum
    int lastFacing;       // geçiş tespiti: önceki yön
    bool dying;           // ölüm animasyonu sürüyor mu (çiziliyor ama AI/hedef dışı)
} Mob;



typedef struct {
    float x, y;          // bölge merkezi
    float radius;        // yarıçap (mob'lar bu alanda rastgele doğar)
    const MobType *type; // hangi tür
    int maxCount;        // bölgede kaç mob olmalı
    float respawnTimer;  // eksik varsa geri sayım
} SpawnZone;

extern const MobType MOB_TYPES[];
extern const int MOB_TYPE_COUNT;


void mob_render(const Mob *m, SDL_Renderer *renderer, SDL_Texture **mob_textures, const Camera *camera, int targetMobId);

// detectMul: algı menzili çarpanı (gece 1.5 — gametime'dan main geçirir)
void mob_update(Mob *m, Mob *mobs, int mobCount, float targetX, float targetY, float dt,
                float detectMul);

// Grup aggro: vurulan mobun kamp arkadaşlarını (aynı zoneId, yakın) saldırıya geçir
void mob_alert_camp(Mob *mobs, int mobCount, const Mob *victim);
int mob_find_nearest(Mob *mobs, int mobCount, float px, float py);
Mob *mob_find_by_id(Mob *mobs, int mobCount, int id);
void target_hp_bar_render(SDL_Renderer *renderer, const Mob *target, int winW, TTF_Font *font);
int mob_spawn(Mob *mobs, int maxMobs, int *nextMobId, const MobType *type, float x, float y, int zoneId);
void spawn_zones_update(SpawnZone *zones, int zoneCount, Mob *mobs, int maxMobs, int *nextMobId, float dt);
void spawn_zones_fill(SpawnZone *zones, int zoneCount, Mob *mobs, int maxMobs, int *nextMobId);

#endif