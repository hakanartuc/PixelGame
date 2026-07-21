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

void fx_render(ParticleSystem *fx, R2D *renderer,
               R2DTexture *glowTex, const Camera *camera)
{
    if (glowTex == NULL) return;
    float zoom = camera->zoom;

    r2d_set_blend(renderer, R2D_BLEND_ALPHA);   // glow dokusu yumusak, normal alfa

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
        } else if (p->fxType == WEAPON_FX_VENOM) {
            // İğne zehri: mor doğar, ölürken yeşile çalar (rünün tersi yönde akar)
            cr = (Uint8)(p->r * t);
            cg = (Uint8)(p->g + (1.0f - t) * 60.0f);
            cb = (Uint8)(p->b * t);
        } else if (p->fxType == WEAPON_FX_ICE) {
            // Buz: yeni=beyaz-mavi, olurken maviye doner (kirmizi duser)
            cr = (Uint8)(p->r * t);
            cg = p->g;
            cb = p->b;
        } else {
            // Pembe (ve digerleri): sabit renk, sadece soner
            cr = p->r;
            cg = p->g;
            cb = p->b;
        }

        r2d_texture_set_color_mod(glowTex, cr, cg, cb);
        r2d_texture_set_alpha_mod(glowTex, alpha);
        SDL_FRect dst = {
            .x = sx - size * 0.5f,
            .y = sy - size * 0.5f,
            .w = size, .h = size
        };
        r2d_draw_texture(renderer, glowTex, NULL, &dst);
    }

    // Normale dondur
    r2d_texture_set_color_mod(glowTex, 255, 255, 255);
    r2d_texture_set_alpha_mod(glowTex, 255);
}
// --- SHEET-ANİM HAVUZU (VFX his cilası) ---

void animfx_init(AnimFx *pool)
{
    for (int i = 0; i < MAX_ANIM_FX; i++)
        pool[i].active = false;
}

void animfx_spawn(AnimFx *pool, R2DTexture *tex, int frames, int cols,
                  float fw, float fh, float x, float y, float fps, float scale)
{
    if (tex == NULL) return;
    int slot = -1;
    for (int i = 0; i < MAX_ANIM_FX; i++)
        if (!pool[i].active) { slot = i; break; }
    if (slot < 0) return;   // havuz dolu — efekt atlanir (kritik degil)
    pool[slot] = (AnimFx){ .tex = tex, .frames = frames, .cols = cols,
                           .fw = fw, .fh = fh, .x = x, .y = y,
                           .t = 0.0f, .fps = fps, .scale = scale,
                           .active = true };
}

void animfx_update(AnimFx *pool, float dt)
{
    for (int i = 0; i < MAX_ANIM_FX; i++) {
        if (!pool[i].active) continue;
        pool[i].t += dt;
        if (pool[i].t * pool[i].fps >= (float)pool[i].frames)
            pool[i].active = false;
    }
}

void animfx_render(AnimFx *pool, R2D *renderer, const Camera *camera)
{
    float zoom = camera->zoom;
    for (int i = 0; i < MAX_ANIM_FX; i++) {
        if (!pool[i].active) continue;
        int fr = (int)(pool[i].t * pool[i].fps);
        if (fr >= pool[i].frames) fr = pool[i].frames - 1;
        SDL_FRect src = { (float)(fr % pool[i].cols) * pool[i].fw,
                          (float)(fr / pool[i].cols) * pool[i].fh,
                          pool[i].fw, pool[i].fh };
        float w = pool[i].fw * pool[i].scale;
        float h = pool[i].fh * pool[i].scale;
        SDL_FRect dst = { (pool[i].x - w / 2.0f - camera->x) * zoom,
                          (pool[i].y - h / 2.0f - camera->y) * zoom,
                          w * zoom, h * zoom };
        r2d_draw_texture(renderer, pool[i].tex, &src, &dst);
    }
}
