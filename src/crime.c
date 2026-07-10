#include <SDL3/SDL.h>
#include "crime.h"
#include "npc.h"
#include "sneak.h"   // sneak_sight_blocked — duvar arkasindaki NPC tanik olamaz

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
