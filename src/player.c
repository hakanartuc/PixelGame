#include <SDL3/SDL.h>
#include "common.h"
#include "tilemap.h"
#include "player.h"

// LPC animasyon tanımı: her durum için başlangıç satırı + kare sayısı
// Yön, başlangıç satırına eklenir (UP=0, LEFT=1, DOWN=2, RIGHT=3)
typedef struct {
    int startRow;     // animasyonun ilk yönünün (UP) satırı
    int frameCount;   // kaç kare
    float frameDur;   // kare süresi
    bool loop;        // döngüsel mi (idle/walk) yoksa tek-seferlik mi (slash)
} LpcAnim;

// PlayerState sırasıyla: IDLE, WALK, SLASH
static const LpcAnim PLAYER_ANIMS[] = {
    [PLAYER_IDLE]  = { .startRow = 22, .frameCount = 2, .frameDur = 0.4f,  .loop = true  },  // satır 22-25
    [PLAYER_WALK]  = { .startRow = 8,  .frameCount = 9, .frameDur = 0.08f, .loop = true  },  // satır 8-11
    [PLAYER_SLASH] = { .startRow = 12, .frameCount = 6, .frameDur = 0.06f, .loop = false },  // satır 12-15
};



// Oyuncuyu çizer - sadece OKUR, değiştirmez.
void player_render(const Player *p, SDL_Renderer *renderer, const PlayerLayer *layers, int layerCount, const Camera *camera)
{
    const LpcAnim *anim = &PLAYER_ANIMS[p->state];
    int row = anim->startRow + (int)p->facing;   // durum satırı + yön

    SDL_FRect src = {
        .x = p->currentFrame * FRAME_W,
        .y = row * FRAME_H,
        .w = FRAME_W,
        .h = FRAME_H
    };

    // Ayak hizalama (LPC: ayak hücrede SPRITE_FEET_Y'de)
    float scale = p->drawH / (float)FRAME_H;
    float footPadding = (FRAME_H - SPRITE_FEET_Y) * scale;
    SDL_FRect screen = {
        .x = (p->rect.x + p->rect.w / 2.0f) - p->drawW / 2.0f - camera->x,
        .y = (p->rect.y + p->rect.h) - p->drawH + footPadding - camera->y,
        .w = p->drawW,
        .h = p->drawH
    };

    // LPC'de flip YOK — sol/sağ ayrı satır. Tüm katmanları düz çiz.
    for (int i = 0; i < layerCount; i++) {
        SDL_RenderTexture(renderer, layers[i].tex, &src, &screen);
    }

    #if DEBUG_DRAW
    SDL_FRect rectScreen = { p->rect.x - camera->x, p->rect.y - camera->y, p->rect.w, p->rect.h };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderRect(renderer, &rectScreen);
    SDL_FRect fb = feet_box(p->rect.x, p->rect.y, p->rect.w, p->rect.h);
    SDL_FRect feetScreen = { fb.x - camera->x, fb.y - camera->y, fb.w, fb.h };
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_RenderRect(renderer, &feetScreen);
    #endif
}

// Oyuncuyu günceller
void player_update(Player *p, const bool *keys, bool attackPressed, float dt)
{
    // --- TILE TEPKİSİ (aynı) ---
    SDL_FRect fb = feet_box(p->rect.x, p->rect.y, p->rect.w, p->rect.h);
    int groundTile = tile_at_point(fb.x + fb.w / 2.0f, fb.y + fb.h / 2.0f);
    if (groundTile >= 0 && groundTile < TILE_TYPE_COUNT) {
        p->speed = PLAYER_SPEED * TILE_TYPES[groundTile].speedMultiplier;
        if (TILE_TYPES[groundTile].damage > 0.0f)
            p->health -= TILE_TYPES[groundTile].damage * dt;
    } else {
        p->speed = PLAYER_SPEED;
    }

    if (p->health <= 0.0f) {
        p->rect.x = SPAWN_X;
        p->rect.y = SPAWN_Y;
        p->health = p->maxHealth;
        p->attacking = false;
    }

    // --- YÖN: WASD'den hareket vektörü ---
    float dx = 0.0f, dy = 0.0f;
    if (keys[SDL_SCANCODE_W]) dy -= 1;
    if (keys[SDL_SCANCODE_S]) dy += 1;
    if (keys[SDL_SCANCODE_A]) dx -= 1;
    if (keys[SDL_SCANCODE_D]) dx += 1;

    bool moving = (dx != 0.0f || dy != 0.0f);

    // Normalize + hareket
    if (moving) {
        float len = SDL_sqrtf(dx*dx + dy*dy);
        dx /= len; dy /= len;

        // X ekseni
        float newX = p->rect.x + dx * p->speed * dt;
        SDL_FRect fx = feet_box(newX, p->rect.y, p->rect.w, p->rect.h);
        if (!box_hits_solid(fx)) p->rect.x = newX;

        // Y ekseni
        float newY = p->rect.y + dy * p->speed * dt;
        SDL_FRect fy = feet_box(p->rect.x, newY, p->rect.w, p->rect.h);
        if (!box_hits_solid(fy)) p->rect.y = newY;

        // YÖN belirleme (4 yön — saldırı sırasında değiştirme)
        if (!p->attacking) {
            // Baskın eksene göre yön (LPC: 4 ayrı yön)
            if (SDL_fabsf(dx) > SDL_fabsf(dy)) {
                p->facing = (dx < 0) ? DIR_LEFT : DIR_RIGHT;
            } else {
                p->facing = (dy < 0) ? DIR_UP : DIR_DOWN;
            }
        }
    }

    // --- DURUM (state) belirleme ---
    if (p->attacking) {
        p->state = PLAYER_SLASH;
    } else if (moving) {
        p->state = PLAYER_WALK;
    } else {
        p->state = PLAYER_IDLE;
    }

    // --- Yeni saldırı başlat ---
    if (attackPressed && !p->attacking) {
        p->attacking = true;
        p->state = PLAYER_SLASH;
        p->currentFrame = 0;
        p->animTimer = 0.0f;
        p->attackHit = false;
    }

    // --- ANİMASYON ilerlet ---
    const LpcAnim *anim = &PLAYER_ANIMS[p->state];

    // Geçiş: durum ya da yön değişti mi → kareyi sıfırla
    if (p->state != p->lastState || p->facing != p->lastFacing) {
        p->currentFrame = 0;
        p->animTimer = 0.0f;
        p->lastState = p->state;
        p->lastFacing = p->facing;
    }

    p->animTimer += dt;
    if (p->animTimer >= anim->frameDur) {
        p->animTimer -= anim->frameDur;
        p->currentFrame++;

        if (p->currentFrame >= anim->frameCount) {
            if (anim->loop) {
                p->currentFrame = 0;   // döngüsel (idle/walk)
            } else {
                // tek-seferlik (slash) bitti
                p->currentFrame = anim->frameCount - 1;
                if (p->state == PLAYER_SLASH) {
                    p->attacking = false;   // saldırı bitti
                }
            }
        }
    }
}

// Can barını ekranın köşesine çizer. Ekran-uzayına bağlıdır, kamera hareketinden etkilenmez.
void health_bar_render(const Player *p, SDL_Renderer *renderer)
{
    // Bar ölçüleri (ekran pikseli - doğrudan, kamera yok)
    float barX = 20.0f, barY = 20.0f;  // sol üst köşeden offset
    float barW = 200.0f, barH = 24.0f; // tam barın boyutu

    // 1) Arka plan (boş bar) - koy kırmızı/gri
    SDL_FRect bg = {barX, barY, barW, barH};
    SDL_SetRenderDrawColor(renderer, 60, 20, 20, 255);
    SDL_RenderFillRect(renderer, &bg);

    // 2) Dolu kısım- cana oranlı genişlik
    float ratio = p->health / p->maxHealth; // 0..1
    // ratio'yu 0..1 arasında sıkıştır (can negatif ya da max'tan büyük olamaz)
    if(ratio < 0.0f) ratio = 0.0f;
    if(ratio > 1.0f) ratio = 1.0f;

    SDL_FRect fill = {barX, barY, barW * ratio, barH};  // genişlik oransal
    SDL_SetRenderDrawColor(renderer, 220, 50, 50, 255); // parlak kırmızı
    SDL_RenderFillRect(renderer, &fill);

    // 3) Çerçeve - barın kenarı
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &bg);
}