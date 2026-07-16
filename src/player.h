#ifndef PLAYER_H
#define PLAYER_H
#include "r2d.h"
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "common.h"
#include "item_textures.h"
#include "fx.h"
#include "skill.h"
#include "ability.h"
#include "perk.h"

typedef struct Equipment Equipment;   // ← forward decl (pointer için yeter)

// LPC sprite sheet sabitleri
#define FRAME_W 64           // LPC kare 64×64
#define FRAME_H 64
#define LPC_SHEET_COLS 13    // sheet 13 sütun

// ULPC tam sheet'te oversize bölge: 54 standart satırdan sonra, 192×192 hücreler.
// Slash bloğu bölgenin ilk 4 satır (yön sırası up/left/down/right, 6 kare).
#define OVERSIZE_Y 3456      // 54 * 64
#define OVERSIZE_FRAME 192

// LPC Hand Tools "smash" sheet'leri: kendi başına 128px grid — satır = yön
// (up/left/down/right), 6 kare. Araç YALNIZ savurma sırasında çizilir.
#define SMASH_FRAME 128

// LPC'de karakterin ayağı hücrenin neresinde (collision/feet hizası için)
// LPC karakterleri 64px hücrede, ayak ~y=58 civarı (ölçeceğiz, şimdilik tahmin)
#define SPRITE_FEET_Y 61

#define PLAYER_SPEED 500.0f
#define SPAWN_X 300.0f
#define SPAWN_Y 300.0f
#define FRAME_DURATION 0.1f
#define PLAYER_ATTACK_RANGE 80.0f
#define PLAYER_ATTACK_HIT_FRAME 3   // slash'ın vuruş karesi (6 karelik slash'ta ~3)

// Yay (Faz 5): köklü atış — çekme boyunca durulur, WASD iptal eder
#define PLAYER_BOW_RANGE 340.0f          // atış menzili (melee 80'in ~4x'ü, algı sınırı üstü)
#define PLAYER_SHOOT_RELEASE_FRAME 9     // 13 karelik shoot'ta ipin bırakıldığı kare


// LPC yön sırası (sheet satır sırası): 0=yukarı, 1=sol, 2=aşağı, 3=sağ
typedef enum {
    DIR_UP = 0,
    DIR_LEFT = 1,
    DIR_DOWN = 2,
    DIR_RIGHT = 3,
} Direction;

// Oyuncunun animasyon durumu (hangi LPC animasyonu)
typedef enum {
    PLAYER_IDLE,    // dururken (satır 22-25, 2 kare)
    PLAYER_WALK,    // yürüme (satır 8-11, 9 kare)
    PLAYER_SLASH,   // kılıç saldırısı (satır 12-15, 6 kare)
    PLAYER_CAST,    // spellcast (satır 0-3, 7 kare) — simya kaynatma/büyü
    PLAYER_SHOOT,   // yay çekme (satır 16-19, 13 kare) — KÖKLÜ: hareket iptal eder
    PLAYER_THRUST,  // asa dürtmesi (satır 4-7, 8 kare) — asa melee'si (Faz 22)
    // ileride: HURT, RUN, DEATH...
} PlayerState;

#define PLAYER_THRUST_HIT_FRAME 5   // dürtmenin hasar karesi

// player.h — AnimClip gibi
typedef struct PlayerLayer {
    R2DTexture *tex;
    int z;
    int idleRowOverride;  // -1 = yok (base satirini kullan), >=0 = idle'da bu satiri ciz
    bool slashOversize;   // slash'ta 64px satir yerine oversize blogu ciz (silah katmanlari)
    bool smash128;        // arac katmani (LPC Hand Tools): YALNIZ slash'ta, kendi 128 grid'i
} PlayerLayer;

// Karakterin TÜM kalıcı durumu
// (struct etiketi combat.h'ın forward declaration'ı için gerekli)
typedef struct Player {
    SDL_FRect rect;      // dünya konumu + mantıksal boyut (ayak izi)
    float drawW, drawH;  // ekranda çizim boyutu

    Direction facing;    // 4 yön (LPC: UP/LEFT/DOWN/RIGHT)
    PlayerState state;   // mevcut animasyon durumu

    int currentFrame;    // animasyon karesi
    float animTimer;     // kare ilerleme sayacı
    PlayerState lastState; // geçiş tespiti: önceki durum
    Direction lastFacing;  // geçiş tespiti: önceki yön

    float speed;
    float speedMult; // dış hız çarpanı (perk × gece × eğilme) — main her kare yazar,
                     // player_update tile hızıyla ÇARPAR (önceden ezilip kayboluyordu)
    float health;
    float maxHealth;

    // Mana + Stamina havuzları: tüketen sistemler (büyü/dodge) gelecekte —
    // barlar şimdiden HUD'da (stat basma da bu havuzları büyütecek, Faz 4)
    float mana, maxMana;
    float manaReservedFrac;   // M4: dost rezervi (0..1) — bar kilitli bölme çizer
    float stamina, maxStamina;

    // Süreli tüketilebilir etkileri (Faz 3)
    float speedBuffMult, speedBuffTimer;   // Hız İksiri
    WeaponFxType coatFx;                   // silah kaplaması (Zehir Yağı)
    float coatTimer;
    float foodRegenRate, foodTimer;        // yemek: sn başına can, kalan süre (Faz 6)
    int   foodBuffType;                    // M2 "Tok" buffu (FoodBuff; tek aktif)
    float foodBuffPower, foodBuffTimer;
    const char *foodBuffName;              // HUD satırı için (ItemType.name)
    float poisonDps, poisonTimer;          // mavi slime zehri: sn hasarı + süre (Faz 7)
    float flameArmorTimer;                 // Alev Zırhı: yakın saldırgana hasar (Faz 22)
    float manaShieldTimer;                 // Büyü Kalkanı: hasar manadan emilir (×1.2)
    float shieldBreakT;                    // kalkan kırılma animasyonu sayacı (görsel)

    // Statlar: dağıtım yok — sabit taban (10), ileride ekipman/perk bonusu ekler.
    // Skill'ler stat değil doğrudan hasar çarpanı verir (use-based felsefe).
    int strength;    // hasar ölçeği (+ zırh ağırlığı telafisi, Faz 22)
    int dexterity;   // kritik şansı
    int intelligence; // büyü hasar ölçeği (Faz 22 — taban 10 + INT taşları)
    int gold;        // para (loot'tan gelir, vendor'da harcanacak)

    // Suç & itibar — BÖLGE BAŞINA (indeks: region_at; -1 dışarısı → 0'a düşer)
    int bounty[MAX_REGIONS];        // yasal borç: tanıklı suçla birikir, ödenince silinir
    float reputation[MAX_REGIONS];  // toplumsal hafıza (-100..+100): ödeme DÜZELTMEZ
    int lastCrimeDay;               // son suç günü (suçsuz gün → itibar 0'a doğru +1)

    Skill skills[SKILL_COUNT];   // use-based skill'ler (kullandıkça gelişir)

    // Skill ağacı seçimleri (Faz 21): skill başına 32-bit alınmış-düğüm maskesi.
    // Etkiler her değişimde tree_compute_effects ile toplanır (main tutar). Save v13.
    Uint32 treeTaken[SKILL_COUNT];
    Uint32 fateTaken;   // Kader Ağacı maskesi (puan TOPLAM seviyeden)
    float gaziTimer;       // "Gazi": hasar yiyince 5 sn hasar buff'ı
    float deathSaveCd;     // "Yikilmaz/Olumsuz" ölüm kurtarışı beklemesi

    // Mob vuruşları oyuncuyu iter (perk'lerle azalır/sıfırlanır)
    float knockbackX, knockbackY;

    bool sneaking;   // C toggle — eğilme (algı/tanık azalır, hız düşer; kesin gizlilik değil)
    bool casting;    // simya kaynatma vb. — durgunken spellcast animasyonu (main yazar)

    // Saldırı durumu
    bool attacking;
    bool attackHit;
    int targetMobId;

    // Yay durumu (Faz 5): main her kare bowEquipped yazar; attackPressed
    // yay kuşanılıyken slash yerine SHOOT başlatır. Hareket atışı iptal eder.
    bool bowEquipped;
    bool staffEquipped;  // asa melee'si SLASH değil THRUST oynar (main yazar; Faz 22)
    bool shooting;       // çekme animasyonu sürüyor
    bool shotReleased;   // bu çekmede ok çıktı mı (release karesinde main atar)
    float drawSpeedMult; // çekme süresi çarpanı (ağaç düğümleri — main yazar, <1 hızlı)

    // KO modeli: hedef seçiliyken normal vuruş otomatik döngüde
    bool autoAttack;                  // Space ile açılır/kapanır
    float attackTimer;                // sonraki otomatik vuruşa kalan süre
    int pendingAbility;               // bu vuruş hangi ability (-1 = normal vuruş)
    float abilityCd[ABILITY_COUNT];   // ability başına kalan cooldown
    float itemCd[MAX_ITEM_TYPES];     // item TÜRÜ başına kalan cooldown (pot kilidi)
    WeaponFxType weaponFx;   // aktif silah efekti (her frame kusanili silahtan senkronlanir)

    // Mühür ekseni (anlatı, Faz A) — GİZLİ sayaçlar: UI'da GÖSTERİLMEZ.
    // Fısıltının tonu ve dünyanın tepkisi bunlardan türer (save v17).
    int echoesWitnessed;   // Dinle/Defnet seçimleri
    int echoesConsumed;    // Çek/Sömür seçimleri

    // Kişilik izi (anlatı, Faz B1) — GİZLİ: diyalog seçimlerinden birikir,
    // ileride NPC'ler "seni tanır" (kimlik davranıştan doğar; save v18)
    int persKaba;    // kaba/sert cevaplar
    int persKibar;   // kibar/yardımsever cevaplar
    int persKetum;   // ketum/mesafeli cevaplar
    bool guardGreeted;   // köye varış karşılaması yaşandı (bir kez)

    // Hikaye kartı bayrakları (D1a, save v22) — kart başına küçük durum kodu.
    // 0 = dokunulmadı; anlamlar kart başına (SF_* indeksleri aşağıda).
    Uint8 storyFlags[16];

    // Davranış ünvanları (M3, save v23): gizli sayaçlar + taşınan ünvan.
    // Sayaçlar UI'da yalnız ilerleme çubuğu olarak görünür (ahlak sayaçları gibi
    // çıplak sayı sergilenmez — ilerleme his verir, sayı büyü bozar... ama
    // eşik ilerlemesi ünvan panelinde N/M gösterilir: hedefli birikim).
    Sint32 titleStats[12];
    Sint8 carriedTitle;   // -1 = yok; seçili ünvanın pasifi ×2 + isimde görünür
} Player;

// storyFlags indeksleri (kartlar geldikçe eklenir — sıra SABİT, save'e girer)
enum {
    SF_K9_KANDIL = 0,   // 0 yok · 1 gerçek (tapınak) · 2 yalan (söner) · 3 sus
    SF_K2_CART   = 1,   // devrik araba hattı: 0 yaşanmadı · 1 Tanık · 2 Sömüren
    SF_K2_NAZIFE = 2,   // 0 yok · 1 yumuşak söyledi · 2 düz söyledi · 4 anlatamadı
    SF_S3_TRUST  = 3,   // Nuh güveni: 0-3 (üç doğru yorum → konak yolu açılır)
    SF_S3_STATE  = 4,   // 0 yok · 1 konak yolu · 2 nöbet DEVRALINDI · 3 kayıt kapatıldı
    SF_S3_HINTS  = 5,   // Zehra ipuçları alındı (1) — Nuh seçeneklerinde işaret
};



void player_render(const Player *p, R2D *renderer, const PlayerLayer *layers, int layerCount, const Camera *camera);
void player_update(Player *p, const bool *keys, bool attackPressed, float dt, ParticleSystem *fx);
void health_bar_render(const Player *p, R2D *renderer);


// Oyuncunun cizilecek katmanlarini olusturur: base (body/head/face) + giyili ekipman.
// out: en az (3 + EQUIP_SLOT_COUNT) kapasiteli olmali. z'ye gore sirali doner.
// Donen deger: katman sayisi.
int build_player_layers(const PlayerLayer *base, int baseCount,
                        const Equipment *eq, const ItemTextures *itemTex,
                        PlayerLayer *out, int outCap);


#endif