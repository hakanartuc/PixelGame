#include "ally.h"
#include "player.h"

// Denge tabanları (Conjuration seviyesi %1/sv büyütür — use-based ölçek)
#define SNAKE_HP 60.0f
#define SNAKE_DMG 6.0f
#define SNAKE_SPEED 240.0f
#define TURTLE_HP 220.0f
#define TURTLE_DMG 4.0f
#define TURTLE_SPEED 140.0f
#define ALLY_ATTACK_RANGE 52.0f
#define ALLY_ATTACK_INTERVAL 1.2f
#define ALLY_SEEK_RANGE 260.0f     // yalnız AGGRO hedef arar (sadık kural)
#define ALLY_FOLLOW_DIST 58.0f
#define ALLY_TELEPORT_DIST 640.0f  // çok koptuysa yanına süzülür (ışınlanma)
#define TAUNT_PULSE 2.0f
#define TAUNT_RANGE 150.0f

void ally_reset(Ally *allies)
{
    for (int i = 0; i < ALLY_MAX; i++) allies[i] = (Ally){0};
}

static float kind_hp(AllyKind k, int lvl, float hpMult)
{
    float base = (k == ALLY_SNAKE) ? SNAKE_HP : TURTLE_HP;
    return base * (1.0f + 0.01f * (float)(lvl - 1)) * hpMult;
}

void ally_summon(Ally *allies, AllyKind kind, float px, float py,
                 int conjLevel, float hpMult)
{
    Ally *a = &allies[kind];
    a->active = true;
    a->kind = kind;
    a->x = px + (kind == ALLY_SNAKE ? -40.0f : 40.0f);
    a->y = py + 10.0f;
    a->maxHp = kind_hp(kind, conjLevel, hpMult);
    a->hp = a->maxHp;
    a->attackTimer = 0.0f;
    a->animT = 0.0f;          // belirme animasyonu baştan
    a->attacking = false;
    a->tauntTimer = TAUNT_PULSE;
    a->bobT = 0.0f;
}

void ally_dismiss(Ally *allies, AllyKind kind) { allies[kind].active = false; }
bool ally_active(const Ally *allies, AllyKind kind) { return allies[kind].active; }

int ally_count(const Ally *allies)
{
    int n = 0;
    for (int i = 0; i < ALLY_MAX; i++)
        if (allies[i].active) n++;
    return n;
}

void ally_update(Ally *allies, float dt, float px, float py,
                 Mob *mobs, int mobCount, int conjLevel, float dmgMult,
                 AllyHit *outHits)
{
    for (int i = 0; i < ALLY_MAX; i++) {
        outHits[i] = (AllyHit){0};
        Ally *a = &allies[i];
        if (!a->active) continue;

        a->animT += dt;
        a->bobT += dt;
        if (a->attackTimer > 0.0f) a->attackTimer -= dt;

        // Hamle animasyonu bitince idle'a dön
        if (a->attacking && a->animT > 0.45f) a->attacking = false;

        // Hedef: SENİN kavgan — yalnız aggro'lu, menzildeki en yakın mob
        Mob *best = NULL;
        float bestD = ALLY_SEEK_RANGE;
        for (int m = 0; m < mobCount; m++) {
            if (!mobs[m].e.alive || !mobs[m].aggro) continue;
            if (mobs[m].type != NULL && mobs[m].type->isStructure) continue;
            float dx = (mobs[m].e.rect.x + mobs[m].e.rect.w / 2.0f) - a->x;
            float dy = (mobs[m].e.rect.y + mobs[m].e.rect.h / 2.0f) - a->y;
            float d = SDL_sqrtf(dx * dx + dy * dy);
            if (d < bestD) { bestD = d; best = &mobs[m]; }
        }

        float speed = (a->kind == ALLY_SNAKE) ? SNAKE_SPEED : TURTLE_SPEED;
        float tx, ty;   // süzülme hedefi
        if (best != NULL) {
            tx = best->e.rect.x + best->e.rect.w / 2.0f;
            ty = best->e.rect.y + best->e.rect.h / 2.0f;
        } else {
            // Oyuncunun yanı: tür başına ayrı omuz (üst üste binmesinler)
            tx = px + (a->kind == ALLY_SNAKE ? -44.0f : 44.0f);
            ty = py + 16.0f;
        }

        float dx = tx - a->x, dy = ty - a->y;
        float dist = SDL_sqrtf(dx * dx + dy * dy);
        float stopAt = (best != NULL) ? ALLY_ATTACK_RANGE * 0.8f : 8.0f;
        if (dist > stopAt) {
            a->x += dx / dist * speed * dt;
            a->y += dy / dist * speed * dt;
            if (SDL_fabsf(dx) > 2.0f) a->facingLeft = dx < 0;
        }

        // Çok koptuysa (leash/ışınlanma): oyuncunun yanına süzül
        float pdx = px - a->x, pdy = py - a->y;
        if (pdx * pdx + pdy * pdy > ALLY_TELEPORT_DIST * ALLY_TELEPORT_DIST) {
            a->x = px;
            a->y = py + 20.0f;
        }

        // Vuruş: menzilde + CD bitti (hasar main'de uygulanır — tek nokta)
        if (best != NULL && bestD <= ALLY_ATTACK_RANGE &&
            a->attackTimer <= 0.0f && a->animT > 0.8f) {
            a->attackTimer = ALLY_ATTACK_INTERVAL;
            a->attacking = true;
            a->animT = 0.0f;   // hamle animasyonu; idle bekleme 0.8s korunur
            float base = (a->kind == ALLY_SNAKE) ? SNAKE_DMG : TURTLE_DMG;
            outHits[i].targetMobId = best->id;
            outHits[i].damage = base * (1.0f + 0.01f * (float)(conjLevel - 1))
                              * dmgMult;
            outHits[i].poison = (a->kind == ALLY_SNAKE);
        }

        // Kaplumbağa taunt nabzı: yakın mobların dikkati kabuğa (main okur)
        if (a->kind == ALLY_TURTLE) {
            a->tauntTimer -= dt;
            if (a->tauntTimer <= 0.0f) {
                a->tauntTimer = TAUNT_PULSE;
                for (int m = 0; m < mobCount; m++) {
                    if (!mobs[m].e.alive || !mobs[m].aggro) continue;
                    float mdx = (mobs[m].e.rect.x + mobs[m].e.rect.w / 2.0f) - a->x;
                    float mdy = (mobs[m].e.rect.y + mobs[m].e.rect.h / 2.0f) - a->y;
                    if (mdx * mdx + mdy * mdy <= TAUNT_RANGE * TAUNT_RANGE)
                        mobs[m].tauntTimer = TAUNT_PULSE + 0.5f;
                }
            }
        }

        if (a->hp <= 0.0f) a->active = false;   // sessiz söner (main CD basar)
    }
}

// 128px hücreli efekt sheet'i: 0-11 belirme, 8-11 idle döngü, 12-14 hamle
static void draw_ally(const Ally *a, R2D *r, R2DTexture *tex, const Camera *cam)
{
    if (tex == NULL) return;
    int frame;
    if (a->animT < 0.8f && !a->attacking)
        frame = (int)(a->animT / 0.0667f);            // belirme (12 kare, ~0.8sn)
    else if (a->attacking)
        frame = 12 + (int)(a->animT / 0.15f);         // hamle
    else
        frame = 8 + ((int)(a->animT / 0.16f)) % 4;    // idle salınım
    if (frame > 15) frame = 15;

    SDL_FRect src = { (float)((frame % 4) * 128), (float)((frame / 4) * 128),
                      128, 128 };
    float bob = SDL_sinf(a->bobT * 3.2f) * 3.0f;      // süzülme
    float size = (a->kind == ALLY_SNAKE) ? 64.0f : 72.0f;
    SDL_FRect dst = { (a->x - size / 2.0f - cam->x) * cam->zoom,
                      (a->y - size / 2.0f + bob - cam->y) * cam->zoom,
                      size * cam->zoom, size * cam->zoom };
    r2d_set_blend(r, R2D_BLEND_ALPHA);
    r2d_draw_texture_flip(r, tex, &src, &dst, a->facingLeft);

    // Can şeridi (yalnız hasarlıyken)
    if (a->hp < a->maxHp) {
        SDL_FRect bg = { dst.x, dst.y - 6.0f, dst.w, 4.0f };
        SDL_FRect fg = bg;
        fg.w *= a->hp / a->maxHp;
        r2d_set_color(r, 20, 25, 20, 220);
        r2d_fill_rect(r, &bg);
        r2d_set_color(r, 110, 235, 120, 255);
        r2d_fill_rect(r, &fg);
    }
}

void ally_render(const Ally *allies, R2D *renderer,
                 R2DTexture *snakeTex, R2DTexture *turtleTex,
                 const Camera *camera)
{
    for (int i = 0; i < ALLY_MAX; i++) {
        if (!allies[i].active) continue;
        draw_ally(&allies[i], renderer,
                  allies[i].kind == ALLY_SNAKE ? snakeTex : turtleTex, camera);
    }
}
