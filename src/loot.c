#include <SDL3/SDL.h>
#include "common.h"
#include "loot.h"

#define CRATE_SCALE  1.2f

// Kasa açılma animasyonu clip'i (Crate_Anim.png: 6 kare, 48×48, 48px aralık)
// Şimdilik tek clip — kapalı=kare 0, açılma=kare 0→5.
static const AnimClip CRATE_CLIP = {
    .x = 0, .y = 0,
    .frameW = 48, .frameH = 48,
    .frameCount = 6,
    .frameDuration = 0.08f,
    .feetY = 33,   // kasa alt hizası (ölçtüğümüz son kare ~33)
};

// Kasanın dünyadaki görsel dikdörtgenini hesaplar (çizim + tıklama aynı kaynaktan).
static SDL_FRect loot_visual_rect(const Loot *l)
{
    const AnimClip *clip = &CRATE_CLIP;
    float scale = CRATE_SCALE;   // ← sabit yap (aşağıda)
    float drawW = clip->frameW * scale;
    float drawH = clip->frameH * scale;
    float feetOffset = clip->feetY * scale;

    SDL_FRect r = {
        .x = (l->rect.x + l->rect.w/2.0f) - drawW/2.0f,
        .y = (l->rect.y + l->rect.h) - feetOffset,
        .w = drawW,
        .h = drawH
    };
    return r;
}

bool loot_spawn(Loot *loots, int maxLoot, float x, float y)
{
    // Boş slot bul
    int slot = -1;
    for (int i = 0; i < maxLoot; i++) {
        if (!loots[i].active) { slot = i; break; }
    }
    if (slot == -1) return false;   // doluysa düşürme

    loots[slot] = (Loot){
        .rect = { .x = x, .y = y, .w = 48.0f, .h = 48.0f },
        .active = true,
        .state = LOOT_CLOSED,
        .currentFrame = 0,
        .animTimer = 0.0f,
    };
    return true;
}

void loot_render_all(Loot *loots, int maxLoot, SDL_Renderer *renderer, SDL_Texture *crateTex, const Camera *camera)
{
    for (int i = 0; i < maxLoot; i++) {
        Loot *l = &loots[i];
        if (!l->active) continue;

        const AnimClip *clip = &CRATE_CLIP;

        // Kaynak: currentFrame'inci kare
        SDL_FRect src = {
            .x = clip->x + l->currentFrame * clip->frameW,
            .y = clip->y,
            .w = clip->frameW,
            .h = clip->frameH
        };

        SDL_FRect vr = loot_visual_rect(l);
        SDL_FRect screen = 
        {
            .x = vr.x - camera->x,
            .y = vr.y - camera->y,
            .w = vr.w,
            .h = vr.h
        };
        SDL_RenderTexture(renderer, crateTex, &src, &screen);
    }
}

int loot_find_at(Loot *loots, int maxLoot, float worldX, float worldY)
{
    for (int i = 0; i < maxLoot; i++) {
        Loot *l = &loots[i];
        if (!l->active) continue;
        // Nokta kasanın rect'i içinde mi?
        SDL_FRect vr = loot_visual_rect(l);
        if (worldX >= vr.x && worldX <= vr.x + vr.w && worldY >= vr.y && worldY <= vr.y + vr.h) 
        {
            return i;
        }
    }
    return -1;
}

void loot_update_all(Loot *loots, int maxLoot, float dt)
{
    for (int i = 0; i < maxLoot; i++) {
        Loot *l = &loots[i];
        if (!l->active) continue;
        if (l->state != LOOT_OPENING) continue;   // sadece açılan kasalar

        // Açılma animasyonu (tek-seferlik)
        l->animTimer += dt;
        if (l->animTimer >= CRATE_CLIP.frameDuration) {
            l->animTimer -= CRATE_CLIP.frameDuration;
            l->currentFrame++;
            if (l->currentFrame >= CRATE_CLIP.frameCount) {
                l->currentFrame = CRATE_CLIP.frameCount - 1;  // son karede kal
                l->state = LOOT_OPEN;                          // açık
            }
        }
    }
}


