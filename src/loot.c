#include <SDL3/SDL.h>
#include "common.h"
#include "loot.h"

// Kasa açılma animasyonu clip'i (Crate_Anim.png: 6 kare, 48×48, 48px aralık)
// Şimdilik tek clip — kapalı=kare 0, açılma=kare 0→5.
static const AnimClip CRATE_CLIP = {
    .x = 0, .y = 0,
    .frameW = 48, .frameH = 48,
    .frameCount = 6,
    .frameDuration = 0.08f,
    .feetY = 33,   // kasa alt hizası (ölçtüğümüz son kare ~33)
};

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

        // Hedef: ekranda, ölçekli (mob gibi 2×), ayak hizalı
        float scale = 2.0f;
        float drawW = clip->frameW * scale;
        float drawH = clip->frameH * scale;
        float feetOffset = clip->feetY * scale;

        SDL_FRect screen = {
            .x = (l->rect.x + l->rect.w / 2.0f) - drawW / 2.0f - camera->x,
            .y = (l->rect.y + l->rect.h) - feetOffset - camera->y,
            .w = drawW,
            .h = drawH
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
        if (worldX >= l->rect.x && worldX <= l->rect.x + l->rect.w &&
            worldY >= l->rect.y && worldY <= l->rect.y + l->rect.h) {
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