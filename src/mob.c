#include <SDL3/SDL.h>
#include "common.h"
#include "tilemap.h"
#include "mob.h"

// Mob türleri tablosu — her tür bir kez tanımlı. (TILE_TYPES gibi)
const MobType MOB_TYPES[] = {
    // [0] Goblin Maceman — uniform 32×32, klasik düzen
    {
        .name = "Goblin Maceman",
        .texturePath = "assets/Mobs/Goblins/Goblin_Maceman.png",
        .maxHealth = 100.0f,
        .speed = 200.0f,
        .attackRange = 60.0f,
        .detectRange = 300.0f,
        .damage = 10.0f,
        .attackType = ATTACK_MELEE,
        .attackHitFrame = 3,
        .anims = {
            // [0] idle — aşağı, yukarı, yan (4 kare, 32×32)
            {
                { .x=0, .y=0,   .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.18f, .feetY=30 },
                { .x=0, .y=64,  .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.18f, .feetY=30 },
                { .x=0, .y=32,  .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.18f, .feetY=30 },
            },
            // [1] run — aşağı, yukarı, yan (6 kare, 32×32)
            {
                { .x=0, .y=96,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=160, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.15f, .feetY=30 },
                { .x=0, .y=128, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.15f, .feetY=30 },
            },
            // [2] attack — aşağı, yukarı, yan (6 kare, 32×32)
            {
                { .x=0, .y=192, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.08f, .feetY=30 },
                { .x=0, .y=256, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.08f, .feetY=30 },
                { .x=0, .y=224, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.08f, .feetY=30 },
            },
        },
    },
    // [1] Orc Grunt — KARIŞIK boyut: idle/run 32×32, attack 64×64
    {
        .name = "Orc Grunt",
        .texturePath = "assets/Mobs/Orcs/Orc_Grunt.png",
        .maxHealth = 200.0f,
        .speed = 150.0f,
        .attackRange = 70.0f,
        .detectRange = 320.0f,
        .damage = 20.0f,
        .attackType = ATTACK_MELEE,
        .attackHitFrame = 4,
        .anims = {
            // [0] idle — aşağı, yukarı, yan (6 kare, 32×32)
            {
                { .x=0, .y=0,   .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.13, .feetY=30 },
                { .x=0, .y=64,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.13, .feetY=30 },
                { .x=0, .y=32,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.13, .feetY=30 },
            },
            // [1] run — aşağı, yukarı, yan (6 kare, 32×32)
            {
                { .x=0, .y=96,  .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.12f, .feetY=30 },
                { .x=0, .y=160, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.12f, .feetY=30 },
                { .x=0, .y=128, .frameW=32, .frameH=32, .frameCount=6, .frameDuration=0.12f, .feetY=30 },
            },
            // [2] attack — aşağı, yukarı, yan (8 kare, 64×64 — feetY FARKLI: 46)
            {
                { .x=0, .y=192, .frameW=64, .frameH=64, .frameCount=8, .frameDuration=0.07f, .feetY=46 },
                { .x=0, .y=320, .frameW=64, .frameH=64, .frameCount=8, .frameDuration=0.07f, .feetY=46 },
                { .x=0, .y=256, .frameW=64, .frameH=64, .frameCount=8, .frameDuration=0.07f, .feetY=46 },
            },
        },
    },
};

const int MOB_TYPE_COUNT = (int)(sizeof(MOB_TYPES) / sizeof(MOB_TYPES[0]));

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

void mob_render(const Mob *m, SDL_Renderer *renderer, SDL_Texture **mob_textures, const Camera *camera, int targetMobId)
{
    if (!m->e.alive) return;

    // Aktif animasyon clip'i: durum + yön
    const AnimClip *clip = &m->type->anims[m->state][m->e.facing];

    // Kaynak: clip'in başlangıcından currentFrame kadar kay (clip'in kendi kare boyutuyla)
    SDL_FRect src = {
        .x = clip->x + m->currentFrame * clip->frameW,
        .y = clip->y,
        .w = clip->frameW,
        .h = clip->frameH
    };

    float scale = 2.0f;
    float drawW = clip->frameW * scale;
    float drawH = clip->frameH * scale;
    float feetOffset = clip->feetY * scale;   // ayak, ölçekli

    SDL_FRect screen = {
        .x = (m->e.rect.x + m->e.rect.w / 2.0f) - drawW / 2.0f - camera->x,
        .y = (m->e.rect.y + m->e.rect.h) - feetOffset - camera->y,  // ayak hizası (altı değil)
        .w = drawW,
        .h = drawH
    };

    // Seçili hedefse, ayağında halka çiz (sprite'tan ÖNCE)
    if (m->id == targetMobId) {
        // Halka rect'in altına (sabit boyut — clip'e bağlı değil, mob'un ayak izi)
        float ringCx = (m->e.rect.x + m->e.rect.w / 2.0f) - camera->x;
        float ringCy = (m->e.rect.y + m->e.rect.h) - camera->y;
        SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
        draw_target_ring(renderer, ringCx, ringCy, m->e.rect.w * 0.7f, m->e.rect.w * 0.35f);
    }

    SDL_FlipMode flip = m->e.facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

    int typeIndex = (int)(m->type - MOB_TYPES);
    SDL_Texture *texture = mob_textures[typeIndex];

    if (m->e.hitFlash > 0.0f)
        SDL_SetTextureColorMod(texture, 255, 150, 150);
    else
        SDL_SetTextureColorMod(texture, 255, 255, 255);

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

    if (m->attackCooldown > 0.0f) m->attackCooldown -= dt;

    float dx = targetX - (m->e.rect.x + m->e.rect.w/2.0f);
    float dy = targetY - (m->e.rect.y + m->e.rect.h/2.0f);
    float dist = SDL_sqrtf(dx*dx + dy*dy);

    float detectRange = m->type->detectRange;
    float attackRange = m->type->attackRange;

    m->attackTriggered = false;

    if (m->attacking) {
        // --- SALDIRI SÜRÜYOR: tek-seferlik animasyon ---
        m->state = MOB_ATTACK;   // attack clip'i (durum 2)
        const AnimClip *clip = &m->type->anims[MOB_ATTACK][m->e.facing];

        m->animTimer += dt;
        if (m->animTimer >= clip->frameDuration) {
            m->animTimer -= clip->frameDuration;
            m->currentFrame++;

            // Vuruş karesinde hasar tetikle
            if (m->currentFrame == m->type->attackHitFrame) {
                if (dist < attackRange + 20.0f)
                    m->attackTriggered = true;
            }

            // Son kareye ulaşınca saldırı bitti
            if (m->currentFrame >= clip->frameCount) {
                m->attacking = false;
                m->currentFrame = 0;
                m->animTimer = 0.0f;
                m->attackCooldown = 0.6f;
                m->state = MOB_IDLE;   // idle'a dön
            }
        }
    }
    else {
        // --- SALDIRI YOK: idle / chase ---

        // Yön: oyuncuya doğru (menzildeyse)
        if (dist > 0.0f && dist < detectRange) {
            if (SDL_fabsf(dx) > SDL_fabsf(dy)) {
                m->e.facing = 2;
                m->e.facingLeft = (dx < 0);
            } else {
                m->e.facing = (dy < 0) ? 1 : 0;
            }
        }

        // Durum: chase mi idle mi
        if (dist < detectRange && dist >= attackRange) {
            m->state = MOB_CHASE;   // run clip'i (durum 1)
            if (dist > 0.0f) {
                float nx = dx / dist, ny = dy / dist;
                m->e.rect.x += nx * m->e.speed * dt;
                m->e.rect.y += ny * m->e.speed * dt;
            }
        } else {
            m->state = MOB_IDLE;   // idle clip'i (durum 0)
        }

        // Saldırı menzilinde + cooldown bittiyse → saldırı başlat
        if (dist < attackRange && m->attackCooldown <= 0.0f) {
            m->attacking = true;
            m->currentFrame = 0;
            m->animTimer = 0.0f;
        }

        // Döngüsel animasyon (aktif clip: state + facing)
        const AnimClip *clip = &m->type->anims[m->state][m->e.facing];

        // Geçiş: durum ya da yön değiştiyse kareyi sıfırla
        if (m->state != m->lastState || m->e.facing != m->lastFacing) {
            m->currentFrame = 0;
            m->animTimer = 0.0f;
            m->lastState = m->state;
            m->lastFacing = m->e.facing;
        }

        m->animTimer += dt;
        if (m->animTimer >= clip->frameDuration) {
            m->animTimer -= clip->frameDuration;
            m->currentFrame = (m->currentFrame + 1) % clip->frameCount;
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
