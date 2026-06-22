#include <SDL3/SDL.h>
#include "common.h"
#include "tilemap.h"
#include "mob.h"


#define MOB_FRAME_W 32      // sprite kare genişliği
#define MOB_FRAME_H 32      // sprite kare yüksekliği
#define MOB_ATTACK_HIT_FRAME 3   // saldırı animasyonunun bu karesinde hasar uygulanır

// mob.c'nin üstünde, static
// [durum][yön] → sheet satırı.  durum: 0=idle, 1=run, 2=attack | yön: 0=aşağı, 1=yukarı, 2=yan
static const int MOB_ANIM_ROWS[3][3] = {
    // aşağı  yukarı  yan
    {   0,      2,     1  },   // [0] idle
    {   3,      5,     4  },   // [1] run
    {   6,      8,     7  },   // [2] attack
};

// durum başına kare sayısı: idle 4, run 6, attack 6
static const int MOB_FRAME_COUNTS[3] = { 4, 6, 6 };

// durum başına kare süresi: idle, run, attack
static const float MOB_FRAME_DURATIONS[3] = { 0.18f, 0.15f, 0.08f };  // attack en hızlı

void mob_render(const Mob *m, SDL_Renderer *renderer, SDL_Texture *texture, const Camera *camera)
{
    if (!m->e.alive) return;

    // Kaynak: sheet'ten doğru kare
    SDL_FRect src = {
        .x = m->currentFrame * MOB_FRAME_W,
        .y = m->animRow * MOB_FRAME_H,
        .w = MOB_FRAME_W,
        .h = MOB_FRAME_H
    };

    // Hedef: ekranda nereye, ne boyutta
    float drawW = 64.0f, drawH = 64.0f;   // 32x32 kareyi 64x64'e ölçekle
    // Sprite'ı rect'in ortasına/altına hizala (oyuncudaki mantık gibi, basitleştirilmiş)
    SDL_FRect screen = {
        .x = (m->e.rect.x + m->e.rect.w / 2.0f) - drawW / 2.0f - camera->x,
        .y = (m->e.rect.y + m->e.rect.h) - drawH - camera->y,
        .w = drawW,
        .h = drawH
    };

    SDL_FlipMode flip = m->e.facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

    // hitFlash varken beyazımsı çizmek için renk modülasyonu (basit yaklaşım)
    if (m->e.hitFlash > 0.0f)
        SDL_SetTextureColorMod(texture, 255, 150, 150);   // hafif kırmızımsı flaş
    else
        SDL_SetTextureColorMod(texture, 255, 255, 255);   // normal

    SDL_RenderTextureRotated(renderer, texture, &src, &screen, 0.0, NULL, flip);

    #if DEBUG_DRAW
    SDL_FRect rectScreen = {
        m->e.rect.x - camera->x,
        m->e.rect.y - camera->y,
        m->e.rect.w,
        m->e.rect.h
    };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderRect(renderer, &rectScreen);
    #endif
}

void mob_update(Mob *m, float targetX, float targetY, float dt)
{
    if (!m->e.alive) return;

    // Flaş ve knockback
    if (m->e.hitFlash > 0.0f) m->e.hitFlash -= dt;
    m->e.rect.x += m->e.knockbackX * dt;
    m->e.rect.y += m->e.knockbackY * dt;
    float damping = 12.0f;
    m->e.knockbackX -= m->e.knockbackX * damping * dt;
    m->e.knockbackY -= m->e.knockbackY * damping * dt;

    // Cooldown her kare azalır
    if (m->attackCooldown > 0.0f) m->attackCooldown -= dt;

    // Hedefe mesafe ve yön
    float dx = targetX - (m->e.rect.x + m->e.rect.w/2.0f);
    float dy = targetY - (m->e.rect.y + m->e.rect.h/2.0f);
    float dist = SDL_sqrtf(dx*dx + dy*dy);

    float detectRange = 300.0f;
    float attackRange = 60.0f;

    m->attackTriggered = false;   // her kare sıfırla

    if (m->attacking) {
        // --- SALDIRI SÜRÜYOR: tek-seferlik animasyon ---
        int frameCount = MOB_FRAME_COUNTS[2];
        bool justFinished = false;

        m->animTimer += dt;
        if (m->animTimer >= MOB_FRAME_DURATIONS[2]) {
            m->animTimer -= MOB_FRAME_DURATIONS[2];
            m->currentFrame++;

            // Vuruş karesinde hasar tetikle (bir kez)
            if (m->currentFrame == MOB_ATTACK_HIT_FRAME) {
                if (dist < attackRange + 20.0f)
                    m->attackTriggered = true;
            }

            // Son kareye ulaşınca saldırı bitti
            if (m->currentFrame >= frameCount) {
                m->attacking = false;
                m->currentFrame = 0; 
                m->animTimer = 0.0f;
                m->attackCooldown = 0.6f;
                justFinished = true;
            }
        }

        // Saldırı HÂLÂ sürüyorsa attack satırını çiz.
        // Bittiyse dokunma → sonraki update else koluna girip doğru satırı seçecek.
        if (!justFinished) {
            // Saldırı sürüyor → attack satırı
            m->animRow = MOB_ANIM_ROWS[2][m->e.facing];
            m->lastRow = m->animRow;
        } else {
            // Saldırı yeni bitti → hemen idle satırına geç (boş kareyi önle)
            m->animRow = MOB_ANIM_ROWS[0][m->e.facing];   // idle, aynı yön
            m->lastRow = m->animRow;
            // currentFrame zaten 0 (yukarıda sıfırlandı)
        }
    }
    else {
        // --- SALDIRI YOK: idle / chase, ve saldırı tetikleme ---

        // Yön: oyuncuya doğru (menzildeyse)
        if (dist > 0.0f && dist < detectRange) {
            if (SDL_fabsf(dx) > SDL_fabsf(dy)) {
                m->e.facing = 2;
                m->e.facingLeft = (dx < 0);
            } else {
                m->e.facing = (dy < 0) ? 1 : 0;
            }
        }

        int animState;   // 0=idle, 1=run
        if (dist < detectRange && dist >= attackRange) {
            m->state = MOB_CHASE;
            animState = 1;
            if (dist > 0.0f) {
                float nx = dx / dist, ny = dy / dist;
                m->e.rect.x += nx * m->e.speed * dt;
                m->e.rect.y += ny * m->e.speed * dt;
            }
        } else {
            m->state = MOB_IDLE;
            animState = 0;
        }

        // Saldırı menzilinde + cooldown bittiyse → YENİ SALDIRI BAŞLAT
        if (dist < attackRange && m->attackCooldown <= 0.0f) {
            m->attacking = true;
            m->currentFrame = 0;
            m->animTimer = 0.0f;
        }

        // Döngüsel animasyon (idle/run)
        int frameCount = MOB_FRAME_COUNTS[animState];
        m->animRow = MOB_ANIM_ROWS[animState][m->e.facing];

        // Satır değiştiyse kareyi sıfırla (olmayan kareye düşmeyi önler)
        if (m->animRow != m->lastRow) {
            m->currentFrame = 0;
            m->animTimer = 0.0f;
            m->lastRow = m->animRow;
        }

        m->animTimer += dt;
        if (m->animTimer >= MOB_FRAME_DURATIONS[animState]) {
            m->animTimer -= MOB_FRAME_DURATIONS[animState];
            m->currentFrame = (m->currentFrame + 1) % frameCount;
        }
    }
}