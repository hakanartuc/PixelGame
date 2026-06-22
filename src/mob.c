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

// Yere yatık bir oval (hedef halkası) çizer. cx,cy = merkez (ekran), 
// rx,ry = yatay/dikey yarıçap.
static void draw_target_ring(SDL_Renderer *renderer, float cx, float cy, float rx, float ry)
{
    int segments = 24;   // ovali kaç parçaya bölelim (çok = pürüzsüz)
    float prevX = cx + rx;   // ilk nokta (açı 0)
    float prevY = cy;
    for (int i = 1; i <= segments; i++) {
        float angle = (float)i / segments * 2.0f * 3.14159265f;  // 0 → 2π
        float x = cx + rx * SDL_cosf(angle);
        float y = cy + ry * SDL_sinf(angle);
        SDL_RenderLine(renderer, prevX, prevY, x, y);   // önceki noktadan bu noktaya çizgi
        prevX = x;
        prevY = y;
    }
}

void mob_render(const Mob *m, SDL_Renderer *renderer, SDL_Texture *texture, const Camera *camera, int targetMobId)
{
    if (!m->e.alive) return;

    // Kaynak: sheet'ten doğru kare
    SDL_FRect src = {
        .x = m->currentFrame * MOB_FRAME_W,
        .y = m->animRow * MOB_FRAME_H,
        .w = MOB_FRAME_W,
        .h = MOB_FRAME_H
    };

    // Hedef: ekranda nereye, ne boyutta (drawW/drawH bir kez — hem sprite hem halka kullanır)
    float drawW = 64.0f, drawH = 64.0f;   // 32x32 kareyi 64x64'e ölçekle
    SDL_FRect screen = {
        .x = (m->e.rect.x + m->e.rect.w / 2.0f) - drawW / 2.0f - camera->x,
        .y = (m->e.rect.y + m->e.rect.h) - drawH - camera->y,
        .w = drawW,
        .h = drawH
    };

    // Seçili hedefse, ayağında halka çiz (sprite'tan ÖNCE — altında kalsın)
    if (m->id == targetMobId) {
        float ringCx = screen.x + drawW / 2.0f;
        float ringCy = screen.y + drawH - 20.0f;   // sprite ayağı kutu altından ~20px yukarıda (gözle ayarlandı)

        SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
        draw_target_ring(renderer, ringCx, ringCy, drawW * 0.35f, drawW * 0.18f);
    }

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

// Verilen ID'ye sahip canlı mob'u bul. Yoksa NULL.
Mob *mob_find_by_id(Mob *mobs, int mobCount, int id)
{
    if (id == 0) return NULL;

    for (int i = 0; i < mobCount; i++) {
        if (mobs[i].e.alive && mobs[i].id == id)
            return &mobs[i];
    }
    return NULL;
}

// Z'ye basınca: en yakın canlı mob'u hedefle
int mob_find_nearest(Mob *mobs, int mobCount, float px, float py)
{
    int bestId = 0;
    float bestDist = 999999.0f;
    for (int i = 0; i < mobCount; i++) {
        if (!mobs[i].e.alive) continue;
        float dx = (mobs[i].e.rect.x + mobs[i].e.rect.w/2.0f) - px;
        float dy = (mobs[i].e.rect.y + mobs[i].e.rect.h/2.0f) - py;
        float d = dx*dx + dy*dy;   // karekök gereksiz (sadece karşılaştırma)
        if (d < bestDist) {
            bestDist = d;
            bestId = mobs[i].id;
        }
    }
    return bestId;   // 0 = mob yok
}

// Hedef mob'un HP barını ekranın üstünde çizer (KO tarzı sabit panel).
void target_hp_bar_render(SDL_Renderer *renderer, const Mob *target, int winW)
{
    if (target == NULL) return;   // hedef yoksa çizme

    // Bar ölçüleri — ekranın üst-ortasında
    float barW = 240.0f, barH = 22.0f;
    float barX = (winW - barW) / 2.0f;   // yatay ortala
    float barY = 20.0f;                   // üstten boşluk

    // 1) Arka plan (boş bar)
    SDL_FRect bg = { barX, barY, barW, barH };
    SDL_SetRenderDrawColor(renderer, 40, 20, 20, 255);
    SDL_RenderFillRect(renderer, &bg);

    // 2) Dolu kısım (cana oranlı)
    float ratio = target->e.health / target->e.maxHealth;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    SDL_FRect fill = { barX, barY, barW * ratio, barH };
    SDL_SetRenderDrawColor(renderer, 200, 40, 40, 255);   // kırmızı
    SDL_RenderFillRect(renderer, &fill);

    // 3) Çerçeve
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &bg);
}
