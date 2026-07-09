#include <SDL3/SDL.h>
#include <stdlib.h>
#include "common.h"
#include "font.h"
#include "tilemap.h"
#include "mob.h"

#define RESPAWN_DELAY 3.0f   // mob ölünce kaç saniye sonra yenisi doğar
#define SEPARATION_RADIUS 70.0f   // bu mesafeden yakın mob'lar iter
#define SEPARATION_FORCE 250.0f   // itme gücü

// Mob türleri tablosu — her tür bir kez tanımlı. (TILE_TYPES gibi)
const MobType MOB_TYPES[] = {
    // [0] Goblin Maceman — uniform 32×32, klasik düzen
    {
        .name = "Goblin Maceman",
        .texturePath = "assets/characters/mobs/Goblin_Maceman.png",
        .level = 1,
        .maxHealth = 100.0f,
        .speed = 200.0f,
        .attackRange = 60.0f,
        .detectRange = 300.0f,
        .damage = 5.0f,
        .defense = 0.0f,
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
            // [3] dying — death animasyonu (üç yöne de aynı, tek yönlü ölüm)
            {
                { .x=0, .y=288, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=23 },  // aşağı
                { .x=0, .y=288, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=23 },  // yukarı
                { .x=0, .y=288, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=23 },  // yan
            },
        },
    },
    // [1] Orc Grunt — KARIŞIK boyut: idle/run 32×32, attack 64×64
    {
        .name = "Orc Grunt",
        .texturePath = "assets/characters/mobs/Orc_Grunt.png",
        .level = 3,
        .maxHealth = 200.0f,
        .speed = 150.0f,
        .attackRange = 70.0f,
        .detectRange = 320.0f,
        .damage = 10.0f,
        .defense = 10.0f,
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
            // [3] dying — death animasyonu (üç yöne de aynı, tek yönlü ölüm)
            {
                { .x=0, .y=384, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },  // aşağı
                { .x=0, .y=384, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },  // yukarı
                { .x=0, .y=384, .frameW=32, .frameH=32, .frameCount=4, .frameDuration=0.15f, .feetY=30 },  // yan
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

// Bölge içinde rastgele bir konum üretir.
static void zone_random_pos(const SpawnZone *z, float *outX, float *outY)
{
    float angle = (float)(rand() % 360) * 3.14159265f / 180.0f;
    float dist = (float)(rand() % (int)z->radius);
    *outX = z->x + SDL_cosf(angle) * dist;
    *outY = z->y + SDL_sinf(angle) * dist;
}

void mob_render(const Mob *m, SDL_Renderer *renderer, SDL_Texture **mob_textures, const Camera *camera, int targetMobId)
{
    if (m->type == NULL) return;   // boş slot → çizme

    if (!m->e.alive && !m->dying) return;   // ne canlı ne ölüyorsa çizme

    // Aktif animasyon clip'i: durum + yön
    const AnimClip *clip = &m->type->anims[m->state][m->e.facing];

    // Kaynak: clip'in başlangıcından currentFrame kadar kay (clip'in kendi kare boyutuyla)
    SDL_FRect src = {
        .x = clip->x + m->currentFrame * clip->frameW,
        .y = clip->y,
        .w = clip->frameW,
        .h = clip->frameH
    };

    float zoom = camera->zoom;
    float scale = 2.0f;
    float drawW = clip->frameW * scale;
    float drawH = clip->frameH * scale;
    float feetOffset = clip->feetY * scale;   // ayak, ölçekli

    // Once dunya-uzayinda kose, sonra zoom ile ekrana
    float mobWorldX = (m->e.rect.x + m->e.rect.w / 2.0f) - drawW / 2.0f;
    float mobWorldY = (m->e.rect.y + m->e.rect.h) - feetOffset;
    SDL_FRect screen = {
        .x = (mobWorldX - camera->x) * zoom,
        .y = (mobWorldY - camera->y) * zoom,
        .w = drawW * zoom,
        .h = drawH * zoom
    };

    // Seçili hedefse, ayağında halka çiz (sprite'tan ÖNCE)
    if (m->id == targetMobId) {
        float ringCx = ((m->e.rect.x + m->e.rect.w / 2.0f) - camera->x) * zoom;
        float ringCy = ((m->e.rect.y + m->e.rect.h) - camera->y) * zoom;
        SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
        draw_target_ring(renderer, ringCx, ringCy,
                         m->e.rect.w * 0.7f * zoom, m->e.rect.w * 0.35f * zoom);
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

void mob_update(Mob *m, Mob *mobs, int mobCount, float targetX, float targetY, float dt)
{
    if (m->type == NULL) return;   // boş slot (hiç mob doğmamış) → atla
    
    m->attackTriggered = false;

    // Ölüm animasyonu oynuyorsa: sadece onu ilerlet, AI çalışmaz
    if (m->dying) {
        const AnimClip *clip = &m->type->anims[MOB_DYING][m->e.facing];
        m->animTimer += dt;
        if (m->animTimer >= clip->frameDuration) {
            m->animTimer -= clip->frameDuration;
            m->currentFrame++;
            if (m->currentFrame >= clip->frameCount) {
                // Death animasyonu bitti → mob tamamen kayboldu
                m->dying = false;
                m->currentFrame = clip->frameCount - 1;  // son karede kal (ya da kaybol)
            }
        }
        return;   // dying'de AI yok
    }

    if (!m->e.alive) return;   // ölü (ve death anim bitmiş) → işlem yok

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
            m->state = MOB_CHASE;
            if (dist > 0.0f) {
                float nx = dx / dist, ny = dy / dist;

                // Separation: yakın mob'lardan uzaklaş
                float sepX = 0.0f, sepY = 0.0f;
                for (int j = 0; j < mobCount; j++) {
                    if (&mobs[j] == m) continue;
                    if (!mobs[j].e.alive) continue;
                    float ox = m->e.rect.x - mobs[j].e.rect.x;
                    float oy = m->e.rect.y - mobs[j].e.rect.y;
                    float d = SDL_sqrtf(ox*ox + oy*oy);
                    if (d > 0.0f && d < SEPARATION_RADIUS) {
                        float strength = (SEPARATION_RADIUS - d) / SEPARATION_RADIUS;
                        sepX += (ox / d) * strength;
                        sepY += (oy / d) * strength;
                    }
                }

                // Oyuncuya doğru + ayrışma
                m->e.rect.x += (nx * m->e.speed + sepX * SEPARATION_FORCE) * dt;
                m->e.rect.y += (ny * m->e.speed + sepY * SEPARATION_FORCE) * dt;
            }
        }
        else {
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
void target_hp_bar_render(SDL_Renderer *renderer, const Mob *target, int winW, TTF_Font *font)
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


    // Mob ismi — bar'ın ÜSTÜNE, ortalı
    if (font != NULL && target->type != NULL) {
        SDL_Color white = { 255, 255, 255, 255 };

        // Yazının piksel boyutunu ölç (ortalamak için genişlik lazım)
        int textW = 0, textH = 0;
        TTF_GetStringSize(font, target->type->name, 0, &textW, &textH);

        // Bar'a göre yatay ortala
        float textX = barX + (barW - textW) / 2.0f;
        float textY = barY - 20.0f;   // bar'ın üstünde

        draw_text(renderer, font, target->type->name, textX, textY, white);
    }
}

// Boş slota mob doğurur. Doğan mob'un ID'sini döndürür (0 = başarısız).
int mob_spawn(Mob *mobs, int maxMobs, int *nextMobId, const MobType *type, float x, float y, int zoneId)
{
    int slot = -1;
    for (int i = 0; i < maxMobs; i++) {
        if (!mobs[i].e.alive && !mobs[i].dying) { slot = i; break; }
    }
    if (slot == -1) return 0;   // boş slot yok

    int id = (*nextMobId)++;
    mobs[slot] = (Mob){
        .id = id,
        .type = type,
        .e = { .rect = { .x=x, .y=y, .w=48.0f, .h=64.0f },
               .health = type->maxHealth, .maxHealth = type->maxHealth,
               .speed = type->speed, .alive = true },
        .state = MOB_IDLE, .attackCooldown = 0.0f,
        .lastState = -1, .lastFacing = -1,
        .zoneId = zoneId
    };
    return id;   // doğan mob'un ID'si
}

void spawn_zones_update(SpawnZone *zones, int zoneCount, Mob *mobs, int maxMobs, int *nextMobId, float dt)
{
    for (int i = 0; i < zoneCount; i++) {
        SpawnZone *z = &zones[i];

        // Bu bölgeye ait canlı (ya da ölüm animasyonu oynayan) mob say
        int aliveCount = 0;
        for (int j = 0; j < maxMobs; j++) {
            if (mobs[j].zoneId == i && (mobs[j].e.alive || mobs[j].dying))
                aliveCount++;
        }

        // Bölge dolu mu?
        if (aliveCount >= z->maxCount) {
            z->respawnTimer = RESPAWN_DELAY;   // dolu → sayacı tam tut
            continue;
        }

        // Eksik mob var → respawn sayacını azalt
        z->respawnTimer -= dt;
        if (z->respawnTimer <= 0.0f) {
            // Bölge içinde rastgele konum (merkez ± yarıçap)
           float sx, sy;
            zone_random_pos(z, &sx, &sy);
            mob_spawn(mobs, maxMobs, nextMobId, z->type, sx, sy, i);
            z->respawnTimer = RESPAWN_DELAY;
        }
    }
}

// Oyun başında: tüm bölgeleri maxCount kadar mob'la doldur (respawn beklemeden).
void spawn_zones_fill(SpawnZone *zones, int zoneCount, Mob *mobs, int maxMobs, int *nextMobId)
{
    for (int i = 0; i < zoneCount; i++) {
        SpawnZone *z = &zones[i];
        for (int n = 0; n < z->maxCount; n++) {
            // Bölge içinde rastgele konum
            float sx, sy;
            zone_random_pos(z, &sx, &sy);
            mob_spawn(mobs, maxMobs, nextMobId, z->type, sx, sy, i);
        }
    }
}