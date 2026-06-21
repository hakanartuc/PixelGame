#include <SDL3/SDL.h>
#include "common.h"
#include "tilemap.h"
#include "enemy.h"

#define DEBUG_DRAW 1

#define ENEMY_FRAME_W 32      // sprite kare genişliği
#define ENEMY_FRAME_H 32      // sprite kare yüksekliği
#define ENEMY_ATTACK_HIT_FRAME 3   // saldırı animasyonunun bu karesinde hasar uygulanır

// enemy.c'nin üstünde, static
// [durum][yön] → sheet satırı.  durum: 0=idle, 1=run, 2=attack | yön: 0=aşağı, 1=yukarı, 2=yan
static const int ENEMY_ANIM_ROWS[3][3] = {
    // aşağı  yukarı  yan
    {   0,      2,     1  },   // [0] idle
    {   3,      5,     4  },   // [1] run
    {   6,      8,     7  },   // [2] attack
};

// durum başına kare sayısı: idle 4, run 6, attack 6
static const int ENEMY_FRAME_COUNTS[3] = { 4, 6, 6 };

// durum başına kare süresi: idle, run, attack
static const float ENEMY_FRAME_DURATIONS[3] = { 0.18f, 0.15f, 0.08f };  // attack en hızlı

void enemy_render(const Enemy *d, SDL_Renderer *renderer, SDL_Texture *texture, const Camera *camera)
{
    if (!d->e.alive) return;

    // Kaynak: sheet'ten doğru kare
    SDL_FRect src = {
        .x = d->currentFrame * ENEMY_FRAME_W,
        .y = d->animRow * ENEMY_FRAME_H,
        .w = ENEMY_FRAME_W,
        .h = ENEMY_FRAME_H
    };

    // Hedef: ekranda nereye, ne boyutta
    float drawW = 64.0f, drawH = 64.0f;   // 32x32 kareyi 64x64'e ölçekle
    // Sprite'ı rect'in ortasına/altına hizala (oyuncudaki mantık gibi, basitleştirilmiş)
    SDL_FRect screen = {
        .x = (d->e.rect.x + d->e.rect.w / 2.0f) - drawW / 2.0f - camera->x,
        .y = (d->e.rect.y + d->e.rect.h) - drawH - camera->y,
        .w = drawW,
        .h = drawH
    };

    SDL_FlipMode flip = d->e.facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

    // hitFlash varken beyazımsı çizmek için renk modülasyonu (basit yaklaşım)
    if (d->e.hitFlash > 0.0f)
        SDL_SetTextureColorMod(texture, 255, 150, 150);   // hafif kırmızımsı flaş
    else
        SDL_SetTextureColorMod(texture, 255, 255, 255);   // normal

    SDL_RenderTextureRotated(renderer, texture, &src, &screen, 0.0, NULL, flip);

    #if DEBUG_DRAW
    SDL_FRect rectScreen = {
        d->e.rect.x - camera->x,
        d->e.rect.y - camera->y,
        d->e.rect.w,
        d->e.rect.h
    };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderRect(renderer, &rectScreen);
    #endif
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

    // Cooldown her kare azalır
    if (d->attackCooldown > 0.0f) d->attackCooldown -= dt;

    // Hedefe mesafe ve yön
    float dx = targetX - (d->e.rect.x + d->e.rect.w/2.0f);
    float dy = targetY - (d->e.rect.y + d->e.rect.h/2.0f);
    float dist = SDL_sqrtf(dx*dx + dy*dy);

    float detectRange = 300.0f;
    float attackRange = 60.0f;

    d->attackTriggered = false;   // her kare sıfırla

    if (d->attacking) {
        // --- SALDIRI SÜRÜYOR: tek-seferlik animasyon ---
        int frameCount = ENEMY_FRAME_COUNTS[2];
        bool justFinished = false;

        d->animTimer += dt;
        if (d->animTimer >= ENEMY_FRAME_DURATIONS[2]) {
            d->animTimer -= ENEMY_FRAME_DURATIONS[2];
            d->currentFrame++;

            // Vuruş karesinde hasar tetikle (bir kez)
            if (d->currentFrame == ENEMY_ATTACK_HIT_FRAME) {
                if (dist < attackRange + 20.0f)
                    d->attackTriggered = true;
            }

            // Son kareye ulaşınca saldırı bitti
            if (d->currentFrame >= frameCount) {
                d->attacking = false;
                d->currentFrame = 0; 
                d->animTimer = 0.0f;
                d->attackCooldown = 0.6f;
                justFinished = true;
            }
        }

        // Saldırı HÂLÂ sürüyorsa attack satırını çiz.
        // Bittiyse dokunma → sonraki update else koluna girip doğru satırı seçecek.
        if (!justFinished) {
            // Saldırı sürüyor → attack satırı
            d->animRow = ENEMY_ANIM_ROWS[2][d->e.facing];
            d->lastRow = d->animRow;
        } else {
            // Saldırı yeni bitti → hemen idle satırına geç (boş kareyi önle)
            d->animRow = ENEMY_ANIM_ROWS[0][d->e.facing];   // idle, aynı yön
            d->lastRow = d->animRow;
            // currentFrame zaten 0 (yukarıda sıfırlandı)
        }
    }
    else {
        // --- SALDIRI YOK: idle / chase, ve saldırı tetikleme ---

        // Yön: oyuncuya doğru (menzildeyse)
        if (dist > 0.0f && dist < detectRange) {
            if (SDL_fabsf(dx) > SDL_fabsf(dy)) {
                d->e.facing = 2;
                d->e.facingLeft = (dx < 0);
            } else {
                d->e.facing = (dy < 0) ? 1 : 0;
            }
        }

        int animState;   // 0=idle, 1=run
        if (dist < detectRange && dist >= attackRange) {
            d->state = ENEMY_CHASE;
            animState = 1;
            if (dist > 0.0f) {
                float nx = dx / dist, ny = dy / dist;
                d->e.rect.x += nx * d->e.speed * dt;
                d->e.rect.y += ny * d->e.speed * dt;
            }
        } else {
            d->state = ENEMY_IDLE;
            animState = 0;
        }

        // Saldırı menzilinde + cooldown bittiyse → YENİ SALDIRI BAŞLAT
        if (dist < attackRange && d->attackCooldown <= 0.0f) {
            d->attacking = true;
            d->currentFrame = 0;
            d->animTimer = 0.0f;
        }

        // Döngüsel animasyon (idle/run)
        int frameCount = ENEMY_FRAME_COUNTS[animState];
        d->animRow = ENEMY_ANIM_ROWS[animState][d->e.facing];

        // Satır değiştiyse kareyi sıfırla (olmayan kareye düşmeyi önler)
        if (d->animRow != d->lastRow) {
            d->currentFrame = 0;
            d->animTimer = 0.0f;
            d->lastRow = d->animRow;
        }

        d->animTimer += dt;
        if (d->animTimer >= ENEMY_FRAME_DURATIONS[animState]) {
            d->animTimer -= ENEMY_FRAME_DURATIONS[animState];
            d->currentFrame = (d->currentFrame + 1) % frameCount;
        }
    }
}