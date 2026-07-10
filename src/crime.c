#include <SDL3/SDL.h>
#include "crime.h"
#include "npc.h"

bool crime_witnessed(float x, float y, int victimIdx, bool victimAlive)
{
    if (victimAlive)
        return true;   // hayatta kalan kurban kendisi tanıktır

    for (int i = 0; i < NPC_COUNT; i++) {
        if (i == victimIdx) continue;
        float nx, ny;
        if (!npc_visible_pos(i, &nx, &ny)) continue;   // içeride/ölü göremez
        float dx = nx - x, dy = ny - y;
        if (SDL_sqrtf(dx * dx + dy * dy) <= CRIME_WITNESS_RANGE)
            return true;
    }
    return false;
}
