#include <SDL3/SDL.h>
#include "crime.h"
#include "npc.h"
#include "sneak.h"   // sneak_sight_blocked — duvar arkasindaki NPC tanik olamaz

void rep_add(float *rep, float delta)
{
    *rep += delta;
    if (*rep < REP_MIN) *rep = REP_MIN;
    if (*rep > REP_MAX) *rep = REP_MAX;
}

// -50..+50 itibar → doğrusal çarpan; dışı uçlarda sabitlenir
float rep_buy_mult(float rep)
{
    if (rep >= 50.0f) return 0.9f;
    if (rep <= -50.0f) return 1.5f;
    // rep 50 → 0.9, rep -50 → 1.5 (doğrusal)
    return 1.2f - rep * (0.6f / 100.0f);
}

float rep_sell_mult(float rep)
{
    if (rep >= 50.0f) return 1.1f;
    if (rep <= -50.0f) return 0.5f;
    // rep 50 → 1.1, rep -50 → 0.5
    return 0.8f + rep * (0.6f / 100.0f);
}

bool crime_witnessed(float x, float y, int victimIdx, bool victimAlive, float range)
{
    if (victimAlive)
        return true;   // hayatta kalan kurban kendisi tanıktır

    for (int i = 0; i < NPC_COUNT; i++) {
        if (i == victimIdx) continue;
        float nx, ny;
        if (!npc_visible_pos(i, &nx, &ny)) continue;   // içeride/ölü göremez
        float dx = nx - x, dy = ny - y;
        if (SDL_sqrtf(dx * dx + dy * dy) <= range &&
            !sneak_sight_blocked(x, y, nx, ny))   // ev arasi gorusu keser
            return true;
    }
    return false;
}
