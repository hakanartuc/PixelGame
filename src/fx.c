#include <SDL3/SDL.h>
#include "fx.h"

void fx_init(ParticleSystem *fx)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
        fx->items[i].active = false;
    fx->nextFree = 0;
}

void fx_spawn(ParticleSystem *fx, float x, float y, float vx, float vy,
              float life, float size, Uint8 r, Uint8 g, Uint8 b, WeaponFxType fxType)
{
    // Bos slot bul (nextFree'den itibaren dairesel)
    for (int n = 0; n < MAX_PARTICLES; n++) {
        int i = (fx->nextFree + n) % MAX_PARTICLES;
        if (!fx->items[i].active) {
            fx->items[i] = (Particle){
                .x = x, .y = y,
                .vx = vx, .vy = vy,
                .life = life, .maxLife = life,
                .size = size,
                .r = r, .g = g, .b = b,
                .fxType = fxType,     
                .active = true
            };
            fx->nextFree = (i + 1) % MAX_PARTICLES;
            return;
        }
    }
    // Havuz dolu → bu partikul atlanir (sorun degil)
}

void fx_update(ParticleSystem *fx, float dt)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &fx->items[i];
        if (!p->active) continue;

        // Hareket
        p->x += p->vx * dt;
        p->y += p->vy * dt;

        // Hafif yavaslama (surtunme) — partikuller yumusak dursun
        p->vx *= (1.0f - 2.0f * dt);
        p->vy *= (1.0f - 2.0f * dt);

        // Omur
        p->life -= dt;
        if (p->life <= 0.0f)
            p->active = false;
    }
}

void fx_render(ParticleSystem *fx, SDL_Renderer *renderer,
               SDL_Texture *glowTex, const Camera *camera)
{
    if (glowTex == NULL) return;
    float zoom = camera->zoom;

    SDL_SetTextureBlendMode(glowTex, SDL_BLENDMODE_BLEND);

    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &fx->items[i];
        if (!p->active) continue;

        // Omur orani (1=yeni, 0=oluyor)
        float t = p->life / p->maxLife;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        // Solma + kuculme: omur azaldikca alpha ve boyut duser
        Uint8 alpha = (Uint8)(t * 200);
        float size = p->size * (0.3f + t * 0.7f) * zoom;  // olurken kucul

        // Dunya → ekran
        float sx = (p->x - camera->x) * zoom;
        float sy = (p->y - camera->y) * zoom;

        // Renk: efekt tipine gore
        Uint8 cr, cg, cb;
        if (p->fxType == WEAPON_FX_FLAME) {
            // Alev: yeni=sari, yaslandikca kizillasir (yesil+mavi duser)
            cr = 255;
            cg = (Uint8)(p->g * t);
            cb = (Uint8)(p->b * t * t);
        } else {
            // Pembe (ve digerleri): sabit renk, sadece soner
            cr = p->r;
            cg = p->g;
            cb = p->b;
        }

        SDL_SetTextureColorMod(glowTex, cr, cg, cb);
        SDL_SetTextureAlphaMod(glowTex, alpha);
        SDL_FRect dst = {
            .x = sx - size * 0.5f,
            .y = sy - size * 0.5f,
            .w = size, .h = size
        };
        SDL_RenderTexture(renderer, glowTex, NULL, &dst);
    }

    // Normale dondur
    SDL_SetTextureColorMod(glowTex, 255, 255, 255);
    SDL_SetTextureAlphaMod(glowTex, 255);
}