#include <stdio.h>
#include "lockpick.h"
#include "font.h"
#include "ui_theme.h"
#include "common.h"   // randf

// Kademe tabanları: bölge genişliği (0..1 bar payı) ve imleç hızı (gidiş/sn)
static const float TIER_ZONE[LOCK_TIERS]  = { 0.22f, 0.14f, 0.08f };
static const float TIER_SPEED[LOCK_TIERS] = { 0.90f, 1.30f, 1.80f };
static const char *TIER_NAME[LOCK_TIERS]  = { "Basit Kilit", "Saglam Kilit", "Usta Kilit" };

static bool active = false;
static int  lootIdx = -1;
static int  tier = 0;          // efektif kademe (keystone düşürmüş olabilir)
static float cursor = 0.0f;    // 0..1 ping-pong
static int   dirSign = 1;
static float zoneCenter = 0.5f;

void lockpick_start(int tierIn, int lootIn, const PerkEffects *fx)
{
    tier = tierIn;
    if (fx->lockMasterEase && tier > 0)
        tier--;   // KİLİT FISILDAYAN: bir kademe kolay

    active = true;
    lootIdx = lootIn;
    cursor = 0.0f;
    dirSign = 1;
    // Bölge merkezi her denemede farklı — ezber olmasın (kenarlardan uzak)
    zoneCenter = 0.25f + randf() * 0.5f;
}

bool lockpick_active(void) { return active; }
int  lockpick_loot(void)   { return lootIdx; }
int  lockpick_tier(void)   { return tier; }

// Efektif bölge genişliği: taban × ağaç çarpanı × skill katkısı (+%0.4/sv)
static float effective_zone(const PerkEffects *fx, int skillLevel)
{
    float z = TIER_ZONE[tier] * fx->lockZoneMult
            * (1.0f + (float)skillLevel * 0.004f);
    if (z > 0.5f) z = 0.5f;   // bar yarısından geniş olmasın
    return z;
}

void lockpick_update(float dt, const PerkEffects *fx)
{
    if (!active) return;
    cursor += TIER_SPEED[tier] * fx->lockCursorMult * dt * (float)dirSign;
    if (cursor > 1.0f) { cursor = 1.0f; dirSign = -1; }
    if (cursor < 0.0f) { cursor = 0.0f; dirSign = 1; }
}

void lockpick_cancel(void)
{
    active = false;
    lootIdx = -1;
}

int lockpick_press(const PerkEffects *fx, int skillLevel)
{
    if (!active) return 0;

    float z = effective_zone(fx, skillLevel);
    bool hit = SDL_fabsf(cursor - zoneCenter) <= z / 2.0f;

    active = false;   // tek basış — sonuç ne olursa minigame biter
    return hit ? 1 : -1;
}

void lockpick_render(R2D *renderer, TTF_Font *font, int winW, int winH,
                     const PerkEffects *fx, int skillLevel)
{
    if (!active) return;

    float barW = 420.0f, barH = 26.0f;
    float x = (winW - barW) / 2.0f;
    float y = winH * 0.62f;

    ui_panel(renderer, x - 16.0f, y - 46.0f, barW + 32.0f, 110.0f);

    if (font != NULL) {
        SDL_Color white = { 255, 255, 255, 255 };
        draw_text(renderer, font, TIER_NAME[tier], x, y - 30.0f, white);
        SDL_Color dim = { 200, 200, 210, 255 };
        draw_text(renderer, font, "Space = ac  ·  ESC = vazgec", x, y + barH + 10.0f, dim);
    }

    // Bar zemini
    r2d_set_color(renderer, 40, 40, 55, 255);
    r2d_fill_rect(renderer, &(SDL_FRect){ x, y, barW, barH });

    // Yeşil bölge — press ile AYNI hesap (görsel yalan söylemesin)
    float z = effective_zone(fx, skillLevel);
    r2d_set_color(renderer, 90, 200, 90, 255);
    r2d_fill_rect(renderer, &(SDL_FRect){ x + (zoneCenter - z / 2.0f) * barW, y,
                                          z * barW, barH });

    // İmleç
    r2d_set_color(renderer, 255, 230, 80, 255);
    r2d_fill_rect(renderer, &(SDL_FRect){ x + cursor * barW - 2.0f, y - 4.0f,
                                          4.0f, barH + 8.0f });

    // Çerçeve
    r2d_set_color(renderer, 200, 200, 210, 255);
    r2d_rect(renderer, &(SDL_FRect){ x, y, barW, barH });
}
