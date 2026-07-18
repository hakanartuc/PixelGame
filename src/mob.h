#ifndef MOB_H
#define MOB_H

#include "r2d.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>   // TTF_Font (target_hp_bar_render)
#include <stdbool.h>
#include "common.h"
#include "item.h"   // LootEntry (tür başına loot tablosu)


#define MAX_MOBS 64        // dünya geneli aynı anda max mob (yuva tavanları + reis/totem + pay)
#define MAX_MOB_TYPES 24   // tür tablosu kapasitesi (E1: 18 tür — reis+totem eklendi)
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
                 // (pasif av kaçışı da ayrı durum DEĞİL: CHASE clip'iyle koşar)
    MOB_SEARCH,  // izini kaybetti: olduğu yerde 8 sn bakınır (IDLE clip'i),
                 // oyuncu yakına dönerse yeniden alarm; süre dolunca RETURN
} MobState;

#define MOB_FLEE_TIME 4.0f   // pasif hayvan vuruştan sonra bu kadar sn kaçar

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

    // Av paketi (Faz 6)
    bool passive;            // pasif av: asla saldırmaz, vurulunca kaçar
    bool nocturnal;          // gececi (goblin): gündüz kampta uyuklar (E2)
    bool dailyRespawn;       // spawn zone anında değil GÜN dönünce doldurur (av kaynağı)

    // Slime paketi (Faz 7)
    int splitCount;          // >0: ölünce bu kadar yavruya bölünür (main işler)
    int splitChild;          // yavrunun MOB_TYPES indeksi (splitCount > 0 ise geçerli)
    float bodyW, bodyH;      // hitbox boyutu (0 = varsayılan 48×64 — slime boyları farklı)
    float drawScale;         // çizim ölçeği (0 = varsayılan 2.0 — boss büyük çizilir)
    Uint8 tintR, tintG, tintB; // tür rengi (0,0,0 = boyama yok — boss koyu/mor)
    bool isBoss;             // boss: leash yok, faz mantığı main'de, tepe HP barı
    bool isStructure;        // yapı-mob (ork totemi): kımıldamaz, saldırmaz, kaçmaz
    float hitPoisonDps;      // vuruşu oyuncuyu zehirler: sn başına hasar (0 = yok)
    float hitPoisonTime;     // zehir süresi (sn)

    // İskelet paketi (Faz 8): menzilli düşmanlar
    float attackRecover;     // saldırı sonrası bekleme (0 = varsayılan 0.6 sn);
                             // menzilli türlerde atış temposunu belirler
    float projSpeed;         // mermi hızı px/sn (ATTACK_RANGED/MAGIC; düz hat, KAÇILABİLİR)

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
    float suspicion;      // 0..1 şüphe: menzilde birikir (yakınsa hızlı), dışında söner;
                          // 1'e ulaşınca aggro — algı ikili değil, "fark edilme süreci"
    float alertFlash;     // aggro anında baş üstü "!" süresi (sn)
    float bleedDps, bleedTime;   // Kanatma DoT'u (main işler — ölüm/loot orada)
    float poisonDps, poisonTime; // Zehir DoT'u (rün/kaplama) — kanamayla AYRI kanal,
                                 // ÜST ÜSTE biner (tam DoT build'i meşru)
    float burnDps, burnTime;     // Yanma DoT'u (Ateş Topu) — 3. bağımsız kanal (Faz 22)
    int arrowsHit;        // saplanan ok sayısı — ölünce ~%30'u lootta geri (Faz 5)
    Sint8 bossPhase;      // boss: 1 kılıç / 2 ok yağmuru+çağrı (faz mantığı main'de)
    float bossTimer;      // boss: yağmur sayacı
    bool bossSummoned;    // faz 2 çağrısı bir kez
    float slowMult, slowTimer;   // Buz Oku: süre boyunca hız çarpanı (0 = etkisiz)
    float fleeTimer;      // pasif av: kalan kaçış süresi (vuruş tazeler)
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
    bool scatter;         // yuva çöktü: kaçış + süre sonunda sessiz despawn (ekoloji)
    bool sleeping;        // şu an uyuyor (E2: ritimden türetilir — sinsi vuruş sayılır)
    float tauntTimer;     // Öz Kabuğu taunt'u (M4): >0 iken hedefi kabuk (main geçirir)
    float patrolAngle;    // devriye: yuva çevresi hedef açısı
    float patrolTimer;    // sıradaki devriye noktasına geçiş sayacı
    float waterX, waterY; // hayvan su noktası (0 = yok; öğlen iner)
    bool raider;          // gece baskını mobu (WE_RAID): öldüren itibar kazanır,
                          // şafakta sağ kalanlar sessizce çekilir (despawn)
    bool risen;           // huzursuz mezarlık mobu (WE_GRAVE): şafakta 2+ sağsa
                          // mezar yeniden yuva olur (koloni), sonra geri çekilir
    float searchTimer;    // MOB_SEARCH kalan süre (bakınma)
} Mob;



typedef struct {
    float x, y;          // bölge merkezi
    float radius;        // yarıçap (mob'lar bu alanda rastgele doğar)
    const MobType *type; // hangi tür
    int maxCount;        // bölgede kaç mob olmalı
    float respawnTimer;  // eksik varsa geri sayım
} SpawnZone;

// Ekoloji özel türleri (MOB_TYPES sonuna eklendi — save indeks güvenliği)
#define MOB_GOBLIN_CHIEF 16   // Goblin Reisi: ölünce kamp çöker
#define MOB_ORC_TOTEM    17   // Ork Savaş Totemi: yapı — yıkılınca kamp çöker
#define MOB_GHOUL_WIZARD  18  // Hortlak Büyücü (mezarlık elit caster — Faz MOB-2)
#define MOB_BLOOD_MONSTER 19  // Kan Canavarı (mezarlık ölü su kıyısı)
#define MOB_BRINGER       20  // ÖLÜM GETİREN (mezarlık boss'u — haftalık diriliş)

extern const MobType MOB_TYPES[];
extern const int MOB_TYPE_COUNT;


void mob_render(const Mob *m, R2D *renderer, R2DTexture **mob_textures, const Camera *camera, int targetMobId);

// detectMul: algı menzili çarpanı (gece 1.5 — gametime'dan main geçirir)
void mob_update(Mob *m, Mob *mobs, int mobCount, float targetX, float targetY, float dt,
                float detectMul);

// Grup aggro: vurulan mobun kamp arkadaşlarını (aynı zoneId, yakın) saldırıya geçir
void mob_alert_camp(Mob *mobs, int mobCount, const Mob *victim);
int mob_find_nearest(Mob *mobs, int mobCount, float px, float py);
Mob *mob_find_by_id(Mob *mobs, int mobCount, int id);
void target_hp_bar_render(R2D *renderer, const Mob *target, int winW, TTF_Font *font);
int mob_spawn(Mob *mobs, int maxMobs, int *nextMobId, const MobType *type, float x, float y, int zoneId);
void spawn_zones_fill(SpawnZone *zones, int zoneCount, Mob *mobs, int maxMobs, int *nextMobId);

#endif