#include <SDL3/SDL.h>
#include "common.h"
#include "tilemap.h"
#include "enemy.h"

void enemy_render(const Enemy *d, SDL_Renderer *renderer, const Camera *camera)
{
    if (!d->e.alive) return;

    SDL_FRect screen = {
        d->e.rect.x - camera->x,
        d->e.rect.y - camera->y,
        d->e.rect.w,
        d->e.rect.h
    };

    if (d->e.hitFlash > 0.0f)
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);  // flaş
    else
        SDL_SetRenderDrawColor(renderer, 180, 40, 160, 255);   // normal mor

    SDL_RenderFillRect(renderer, &screen);
}

void enemy_update(Enemy *d, float targetX, float targetY, float dt)
{
    if (!d->e.alive) return;

    // Flaş ve knockback
    if (d->e.hitFlash > 0.0f) d->e.hitFlash -= dt;
    d->e.rect.x += d->e.knockbackX * dt;
    d->e.rect.y += d->e.knockbackY * dt;
    float damping = 12.0f;
    d->e.knockbackX -= d->e.knockbackX * damping * dt;
    d->e.knockbackY -= d->e.knockbackY * damping * dt;

    // AI: hedefe mesafe (hedef merkezi parametre olarak geldi)
    float dx = targetX - (d->e.rect.x + d->e.rect.w/2.0f);
    float dy = targetY - (d->e.rect.y + d->e.rect.h/2.0f);
    float dist = SDL_sqrtf(dx*dx + dy*dy);

    // Durum kararı
    float detectRange = 300.0f;
    float attackRange = 60.0f;
    if (dist < attackRange)       d->state = ENEMY_ATTACK;
    else if (dist < detectRange)  d->state = ENEMY_CHASE;
    else                          d->state = ENEMY_IDLE;

    // Cooldown azalt
    if (d->attackCooldown > 0.0f) d->attackCooldown -= dt;

    // Davranış
    if (d->state == ENEMY_CHASE) {
        if (dist > 0.0f) {                       // sıfır bölme koruması
            float nx = dx / dist, ny = dy / dist;
            d->e.rect.x += nx * d->e.speed * dt;
            d->e.rect.y += ny * d->e.speed * dt;
        }
    }

    d->attackTriggered = false;   // her kare sıfırla
    if (d->state == ENEMY_ATTACK) {
        if (d->attackCooldown <= 0.0f) {
            d->attackTriggered = true;   // "saldırdım" — hasarı dışarısı versin
            d->attackCooldown = 1.0f;
        }
    }
}
