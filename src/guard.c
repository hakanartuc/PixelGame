#include <SDL3/SDL.h>
#include "guard.h"
#include "npc.h"
#include "gametime.h"

typedef enum {
    GUARD_POST,      // nöbette (rutin nokta)
    GUARD_APPROACH,  // bounty sahibi oyuncuya yürüyor (yakalamak için)
    GUARD_HOSTILE,   // karşı konuldu — saldırıyor
    GUARD_RETURN,    // nöbet noktasına dönüyor (leash ya da iş bitti)
} GuardState;

static int gIdx = -1;            // muhafızın NPC indeksi
static GuardState gState = GUARD_POST;
static float postX, postY;       // nöbet noktası (diriliş de buraya)
static float atkTimer = 0.0f;
static int lastDay = -1;         // gün değişimi tespiti (diriliş)

void guard_init(int npcIndex)
{
    gIdx = npcIndex;
    gState = GUARD_POST;
    lastDay = gametime_day();
    // Nöbet noktası tanımdan (schedule[0]) — diriliş/dönüş hedefi
    postX = NPCS[npcIndex].schedule[0].x;
    postY = NPCS[npcIndex].schedule[0].y;
}

void guard_set_hostile(bool hostile)
{
    if (gIdx < 0) return;
    gState = hostile ? GUARD_HOSTILE : GUARD_RETURN;
}

bool guard_is_hostile(void) { return gState == GUARD_HOSTILE; }
int  guard_index(void)      { return gIdx; }

// Hedefe doğru adım at; kalan mesafeyi döndürür
static float step_towards(float *x, float *y, float tx, float ty, float step)
{
    float dx = tx - *x, dy = ty - *y;
    float dist = SDL_sqrtf(dx * dx + dy * dy);
    if (dist < 0.001f) return 0.0f;
    if (step > dist) step = dist;
    *x += dx / dist * step;
    *y += dy / dist * step;
    return dist - step;
}

GuardTick guard_update(float dt, float px, float py, int bounty, bool uiFrozen)
{
    GuardTick out = { false, 0.0f };
    if (gIdx < 0) return out;

    // Diriliş: öldüyse ertesi sabah nöbet noktasında yenisi belirir
    int day = gametime_day();
    if (day != lastDay) {
        lastDay = day;
        if (!npc_is_alive(gIdx)) {
            npc_set_alive(gIdx, true);
            npc_set_pos(gIdx, postX, postY);
            gState = GUARD_POST;
        }
    }
    if (!npc_is_alive(gIdx) || uiFrozen)
        return out;

    float gx, gy;
    if (!npc_pos(gIdx, &gx, &gy)) return out;

    float pdx = px - gx, pdy = py - gy;
    float playerDist = SDL_sqrtf(pdx * pdx + pdy * pdy);
    float postDx = gx - postX, postDy = gy - postY;
    float postDist = SDL_sqrtf(postDx * postDx + postDy * postDy);

    switch (gState) {
    case GUARD_POST:
        if (bounty > 0 && playerDist <= GUARD_SIGHT)
            gState = GUARD_APPROACH;
        break;

    case GUARD_APPROACH:
        if (bounty <= 0) { gState = GUARD_RETURN; break; }   // ödendi/temiz
        if (postDist > GUARD_LEASH) { gState = GUARD_RETURN; break; }  // kaçtı
        if (playerDist <= GUARD_CATCH) {
            out.wantCatch = true;   // panel main'de açılır; muhafız bekler
        } else {
            step_towards(&gx, &gy, px, py, GUARD_SPEED * dt);
            npc_set_pos(gIdx, gx, gy);
        }
        break;

    case GUARD_HOSTILE:
        if (postDist > GUARD_LEASH) { gState = GUARD_RETURN; break; }  // kaçtı
        if (playerDist > GUARD_ATK_RANGE) {
            step_towards(&gx, &gy, px, py, GUARD_SPEED * dt);
            npc_set_pos(gIdx, gx, gy);
            atkTimer = 0.0f;   // menzile girince ilk vuruş kısa gecikmeli
        } else {
            atkTimer -= dt;
            if (atkTimer <= 0.0f) {
                out.attackDamage = GUARD_ATK_DMG;
                atkTimer = GUARD_ATK_INTERVAL;
            }
        }
        break;

    case GUARD_RETURN:
        if (step_towards(&gx, &gy, postX, postY, GUARD_SPEED * dt) <= 1.0f)
            gState = GUARD_POST;   // vardı — düşmanlık leash'te söner (yeniden yakalamayı dener)
        npc_set_pos(gIdx, gx, gy);
        break;
    }
    return out;
}
