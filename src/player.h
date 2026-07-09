#ifndef PLAYER_H
#define PLAYER_H
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "common.h"
#include "item_textures.h"
#include "fx.h"
#include "skill.h"

typedef struct Equipment Equipment;   // ← forward decl (pointer için yeter)

// LPC sprite sheet sabitleri
#define FRAME_W 64           // LPC kare 64×64
#define FRAME_H 64
#define LPC_SHEET_COLS 13    // sheet 13 sütun

// ULPC tam sheet'te oversize bölge: 54 standart satırdan sonra, 192×192 hücreler.
// Slash bloğu bölgenin ilk 4 satırı (yön sırası up/left/down/right, 6 kare).
#define OVERSIZE_Y 3456      // 54 * 64
#define OVERSIZE_FRAME 192

// LPC'de karakterin ayağı hücrenin neresinde (collision/feet hizası için)
// LPC karakterleri 64px hücrede, ayak ~y=58 civarı (ölçeceğiz, şimdilik tahmin)
#define SPRITE_FEET_Y 61

#define PLAYER_SPEED 500.0f
#define SPAWN_X 300.0f
#define SPAWN_Y 300.0f
#define FRAME_DURATION 0.1f
#define PLAYER_ATTACK_RANGE 80.0f
#define PLAYER_ATTACK_HIT_FRAME 3   // slash'ın vuruş karesi (6 karelik slash'ta ~3)


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
    // ileride: THRUST, SHOOT, SPELLCAST, HURT, RUN, DEATH...
} PlayerState;

// player.h — AnimClip gibi
typedef struct PlayerLayer {
    SDL_Texture *tex;
    int z;
    int idleRowOverride;  // -1 = yok (base satirini kullan), >=0 = idle'da bu satiri ciz
    bool slashOversize;   // slash'ta 64px satir yerine oversize blogu ciz (silah katmanlari)
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
    float health;
    float maxHealth;

    // Statlar: dağıtım yok — sabit taban (10), ileride ekipman/perk bonusu ekler.
    // Skill'ler stat değil doğrudan hasar çarpanı verir (use-based felsefe).
    int strength;    // hasar ölçeği
    int dexterity;   // kritik şansı
    int gold;        // para (loot'tan gelir, vendor'da harcanacak)

    Skill skills[SKILL_COUNT];   // use-based skill'ler (kullandıkça gelişir)

    // Saldırı durumu
    bool attacking;
    bool attackHit;
    int targetMobId;
    WeaponFxType weaponFx;   // aktif silah efekti (her frame kusanili silahtan senkronlanir)
} Player;



void player_render(const Player *p, SDL_Renderer *renderer, const PlayerLayer *layers, int layerCount, const Camera *camera);
void player_update(Player *p, const bool *keys, bool attackPressed, float dt, ParticleSystem *fx);
void health_bar_render(const Player *p, SDL_Renderer *renderer);


// Oyuncunun cizilecek katmanlarini olusturur: base (body/head/face) + giyili ekipman.
// out: en az (3 + EQUIP_SLOT_COUNT) kapasiteli olmali. z'ye gore sirali doner.
// Donen deger: katman sayisi.
int build_player_layers(const PlayerLayer *base, int baseCount,
                        const Equipment *eq, const ItemTextures *itemTex,
                        PlayerLayer *out, int outCap);

void weapon_render_test(Player *p, SDL_Renderer *renderer,
                        SDL_Texture *weaponTex, SDL_Texture *glowTex,
                        ParticleSystem *fx, const Camera *camera, bool backLayer);

#endif