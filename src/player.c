#include <SDL3/SDL.h>
#include "common.h"
#include "tilemap.h"
#include "player.h"


#define DEBUG_DRAW 0

// Animasyon satır tablosu: [hareket durumu][yön] → sheet satırı
// hareket: 0 = idle, 1 = yürüyüş
// yön:     0 = aşağı, 1 = yukarı, 2 = yan
static const int ANIM_ROWS[2][3] = {
    // aşağı  yukarı  yan
    {   0,      2,     1  },   // [0] idle
    {   3,      5,     4  },   // [1] yürüyüş
};

// Oyuncu saldırı satırları [yön] (aşağı/yukarı/yan) → sheet satırı
// (mevcut ANIM_ROWS idle/walk içindi; bu attack için ayrı)
static const int PLAYER_ATTACK_ROWS[3] = { 32, 34, 33 };  // aşağı=32, yukarı=34, yan=33
#define PLAYER_ATTACK_FRAMES 6

// Oyuncunun saldırı hitbox'ını (baktığı yönde) hesaplar.
SDL_FRect attack_hitbox(const Player *p)
{
    float w = 40.0f, h = 40.0f;   // hitbox boyutu
    // Oyuncunun merkezinden başla
    float cx = p->rect.x + p->rect.w / 2.0f;
    float cy = p->rect.y + p->rect.h / 2.0f;

    float hx = cx, hy = cy;
    // Yöne göre hitbox'ı oyuncunun ÖNÜNE kaydır:
    if (p->facing == 2) {            // yan
        hx = p->facingLeft ? (p->rect.x - w) : (p->rect.x + p->rect.w);
        hy = cy - h / 2.0f;
    } else if (p->facing == 1) {     // yukarı
        hx = cx - w / 2.0f;
        hy = p->rect.y - h;
    } else {                          // aşağı (facing == 0)
        hx = cx - w / 2.0f;
        hy = p->rect.y + p->rect.h;
    }
    SDL_FRect box = { hx, hy, w, h };
    return box;
}

// Oyuncuyu çizer - sadece OKUR, değiştirmez.
void player_render(const Player *p, SDL_Renderer *renderer, SDL_Texture **layers, int layerCount, const Camera *camera)
{
    SDL_FRect src = {
        .x = p->currentFrame * FRAME_W,
        .y = p->animRow * FRAME_H,
        .w = FRAME_W,
        .h = FRAME_H
    };
    
    // Sprite'ın görsel ayağı (hücrede y=40), rect'in ALTINA hizalanmalı.
    // Çizim kutusunun altındaki boşluk: (FRAME_H - 40) * ölçek = (64-40) * (drawH/FRAME_H)
    float scale = p->drawH / (float)FRAME_H;          // 128/64 = 2
    float footPadding = (FRAME_H - SPRITE_FEET_Y) * scale;       // (64-40)*2 = 48px boşluk
    SDL_FRect screen = {
        .x = (p->rect.x + p->rect.w / 2.0f) - p->drawW / 2.0f - camera->x,
        .y = (p->rect.y + p->rect.h) - p->drawH + footPadding - camera->y,  // +footPadding: çizimi aşağı kaydır
        .w = p->drawW,
        .h = p->drawH
    };

    SDL_FlipMode flip = p->facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    for (int i = 0; i < layerCount; i++)
    {
        SDL_RenderTextureRotated(renderer, layers[i], &src, &screen, 0.0, NULL, flip);
    }

    #if DEBUG_DRAW
    // --- DEBUG: collision kutularını çiz (dünya → ekran: kamerayı çıkar) ---
    // 1) rect (tüm ayak izi) — kırmızı çerçeve
    SDL_FRect rectScreen = {
        p->rect.x - camera->x,
        p->rect.y - camera->y,
        p->rect.w,
        p->rect.h
    };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);   // kırmızı
    SDL_RenderRect(renderer, &rectScreen);

    // 2) ayak kutusu (gerçek collision) — sarı çerçeve
    //    feetY = rect.y + rect.h - FEET_H, feetX = rect.x + FEET_INSET, feetW = rect.w - FEET_INSET*2
    SDL_FRect fb = feet_box(p->rect.x, p->rect.y, p->rect.w, p->rect.h);
    SDL_FRect feetScreen = { fb.x - camera->x, fb.y - camera->y, fb.w, fb.h };

    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // sarı
    SDL_RenderRect(renderer, &feetScreen);

    if (p->attacking) {
        SDL_FRect hb = attack_hitbox(p);
        SDL_FRect hbScreen = { hb.x - camera->x, hb.y - camera->y, hb.w, hb.h };
        SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);  // mavi
        SDL_RenderRect(renderer, &hbScreen);
    }

    #endif
}

// Oyuncuyu günceller
void player_update(Player *p, const bool *keys, bool attackPressed, float dt)
{
    // --- TILE TEPKİSİ: bastığın tile'ın özelliklerine göre hız/can ayarla ---
    SDL_FRect fb = feet_box(p->rect.x, p->rect.y, p->rect.w, p->rect.h);
    int groundTile = tile_at_point(fb.x + fb.w / 2.0f, fb.y + fb.h / 2.0f);

    if (groundTile >= 0 && groundTile < TILE_TYPE_COUNT) {
        // hız: temel hız × tile'ın çarpanı (her kare yeniden hesaplanır)
        p->speed = PLAYER_SPEED * TILE_TYPES[groundTile].speedMultiplier;

        // hasar: tile saniyede 'damage' veriyorsa, dt kadarını uygula
        if (TILE_TYPES[groundTile].damage > 0.0f)
            p->health -= TILE_TYPES[groundTile].damage * dt;

    } else {
        p->speed = PLAYER_SPEED;   // geçersiz tile → normal hız
    }

    if (p->health <= 0.0f) {
        p->rect.x = SPAWN_X;
        p->rect.y = SPAWN_Y;
        p->health = p->maxHealth;
        p->attacking = false;      // yarım kalan saldırıyı iptal et
    }

    // --- YÖN: basılı tuşlardan bir yön vektörü (dx, dy) kur ---
    float dx = 0.0f, dy = 0.0f;
    if (keys[SDL_SCANCODE_W]) dy -= 1;  // yukarı
    if (keys[SDL_SCANCODE_S]) dy += 1;  // aşağı
    if (keys[SDL_SCANCODE_A]) dx -= 1;  // sol
    if (keys[SDL_SCANCODE_D]) dx += 1;  // sağ

    bool moving = false;

    // --- NORMALİZE: yön vektörünü 1 birim uzunluğa indir ---
    float length = SDL_sqrtf(dx * dx + dy * dy);
    if (length > 0) {                    // sıfır kontrolü ŞART: 0/0 = NaN, kutuyu sessizce yok eder
        dx = dx / length;
        dy = dy / length;
        moving = true;

        // X EKSENİ
        float newX = p->rect.x + dx * p->speed * dt;
        SDL_FRect fx = feet_box(newX, p->rect.y, p->rect.w, p->rect.h);
        if (!box_hits_solid(fx))
            p->rect.x = newX;

        // Y EKSENİ
        float newY = p->rect.y + dy * p->speed * dt;
        SDL_FRect fy = feet_box(p->rect.x, newY, p->rect.w, p->rect.h);
        if (!box_hits_solid(fy))
            p->rect.y = newY;
        

        if (SDL_fabsf(dx) > SDL_fabsf(dy)) {
            if (!p->attacking) { p->facing = 2; p->facingLeft = (dx < 0); }
        } else {
            if (!p->attacking) p->facing = (dy < 0) ? 1 : 0;
        }
    }

    if (p->attacking) {
        // --- SALDIRI: tek-seferlik animasyon ---
        p->animRow = PLAYER_ATTACK_ROWS[p->facing];
        p->lastRow = p->animRow;   // saldırıdan çıkınca geçiş tetiklenmesin

        p->animTimer += dt;
        if (p->animTimer >= FRAME_DURATION) {
            p->animTimer -= FRAME_DURATION;
            p->currentFrame++;

            // Vuruş karesine ulaştık → hasar main loop'ta uygulanacak
            // (burada sadece animasyon ilerliyor; hasar tetikleme aşağıda açıklanıyor)

            // Son kareye ulaşınca saldırı bitti
            if (p->currentFrame >= PLAYER_ATTACK_FRAMES) {
                p->attacking = false;
                p->currentFrame = 0;
                // idle/walk satırına dönüşü bir sonraki kare normal kol halleder
            }
        }
    }
    else {
        // --- NORMAL: idle/walk ---
        p->animRow = ANIM_ROWS[moving][p->facing];
        int frameCount = 6;
        if (p->animRow != p->lastRow) {
            p->currentFrame = 0;
            p->animTimer = 0.0f;
            p->lastRow = p->animRow;
        }
        p->animTimer += dt;
        if (p->animTimer >= FRAME_DURATION) {
            p->animTimer -= FRAME_DURATION;
            p->currentFrame = (p->currentFrame + 1) % frameCount;
        }
    }

    // --- SALDIRI DURUMU ---
    // Yeni saldırı başlat (saldırı sürmüyorsa)
    if (attackPressed && !p->attacking) {
        p->attacking = true;
        p->currentFrame = 0;
        p->animTimer = 0.0f;
        p->attackHit = false;     // bu saldırıda henüz vurmadık
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