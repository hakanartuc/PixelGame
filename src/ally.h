#ifndef ALLY_H
#define ALLY_H
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "r2d.h"
#include "common.h"
#include "mob.h"

// CONJURATION DOSTLARI (M4): yeşil özden büyü varlıkları — oyuncunun yanında
// süzülür, AGGRO'lu hedeflere saldırır (kendi başına kavga başlatmaz).
// Bedel: dost yaşarken max mananın %25'i REZERVE (iki dost keystone'la, %60).
// Sprite'lar efekt-stili (128px belirme/hamle) — yürüme animasyonu gerekmez.

typedef enum { ALLY_SNAKE = 0, ALLY_TURTLE, ALLY_KIND_COUNT } AllyKind;

typedef struct {
    bool active;
    AllyKind kind;
    float x, y;          // dünya konumu (süzülür)
    float hp, maxHp;
    float attackTimer;   // sonraki vuruşa kalan
    float animT;         // belirme/idle/hamle zamanlayıcısı
    bool attacking;      // hamle animasyonu oynuyor
    bool facingLeft;
    float tauntTimer;    // kaplumbağa: taunt nabzı sayacı
    float bobT;          // süzülme salınımı
} Ally;

#define ALLY_MAX 2   // keystone ile iki dost

struct Player;

void ally_reset(Ally *allies);
// Çağır: aynı türden varsa yerine tazeler; tek-dost kuralını MAIN uygular.
// conjLevel dost gücünü ölçekler (HP/hasar).
void ally_summon(Ally *allies, AllyKind kind, float px, float py,
                 int conjLevel, float hpMult);
void ally_dismiss(Ally *allies, AllyKind kind);
bool ally_active(const Ally *allies, AllyKind kind);
int  ally_count(const Ally *allies);

// Güncelle: oyuncuyu izle, hedef seç (aggro moblar), vuruş zamanla.
// Vuruş gerçekleşirse hedefe hasar UYGULANMAZ — main'e rapor edilir
// (out parametreleri): dmg + hedef id (XP/zehir/ölüm main'de tek noktadan).
typedef struct {
    int targetMobId;     // 0 = bu karede vuruş yok
    float damage;
    bool poison;         // yılan: zehir DoT bassın
} AllyHit;

void ally_update(Ally *allies, float dt, float px, float py,
                 Mob *mobs, int mobCount, int conjLevel, float dmgMult,
                 AllyHit *outHits /* ALLY_MAX kapasiteli */);

void ally_render(const Ally *allies, R2D *renderer,
                 R2DTexture *snakeTex, R2DTexture *turtleTex,
                 const Camera *camera);

#endif
