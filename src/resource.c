#include <SDL3/SDL.h>
#include <stdio.h>   // snprintf (E ipucu)
#include "resource.h"
#include "item.h"
#include "font.h"

// Yerleşim: bakır köye yakın kayalıkta (kolay başlangıç), demir kamp yolunda
// (riskli), kızılot ova çayırında, ayotu bataklık kenarında (tehlikeli),
// kesilebilir ağaçlar orman sınırında. "Değerli malzeme riskli yerde" ilkesi.
#define T(n) ((n) * (float)TILE_SIZE)

// Ores.png satırları: 0=demir(gri), 1=bakır; sütun 0 = kaya-içinde-cevher (düğüm)
#define ORE_NODE(row) { 0.0f, (row) * 16.0f, 16.0f, 16.0f }

const ResourceDef RESOURCES[] = {
    // Bakır düğümleri (Madencilik + Kazma)
    { .texPath = "assets/decor/ores.png", .src = ORE_NODE(1),
      .x = T(17), .y = T(14), .scale = 3.0f, .itemIdx = ITEM_ORE_COPPER,
      .minCount = 1, .maxCount = 2, .skill = SKILL_MINING,
      .toolItem = ITEM_PICKAXE, .work = 5.0f, .verb = "Kaz" },
    { .texPath = "assets/decor/ores.png", .src = ORE_NODE(1),
      .x = T(19), .y = T(18), .scale = 3.0f, .itemIdx = ITEM_ORE_COPPER,
      .minCount = 1, .maxCount = 2, .skill = SKILL_MINING,
      .toolItem = ITEM_PICKAXE, .work = 5.0f, .verb = "Kaz" },
    { .texPath = "assets/decor/ores.png", .src = ORE_NODE(1),
      .x = T(30), .y = T(16), .scale = 3.0f, .itemIdx = ITEM_ORE_COPPER,
      .minCount = 1, .maxCount = 2, .skill = SKILL_MINING,
      .toolItem = ITEM_PICKAXE, .work = 5.0f, .verb = "Kaz" },

    // Demir düğümleri (goblin kampı civarı — risk/ödül)
    { .texPath = "assets/decor/ores.png", .src = ORE_NODE(0),
      .x = T(27), .y = T(26), .scale = 3.0f, .itemIdx = ITEM_ORE_IRON,
      .minCount = 1, .maxCount = 2, .skill = SKILL_MINING,
      .toolItem = ITEM_PICKAXE, .work = 10.0f, .minSkill = 15, .verb = "Kaz" },
    { .texPath = "assets/decor/ores.png", .src = ORE_NODE(0),
      .x = T(32), .y = T(28), .scale = 3.0f, .itemIdx = ITEM_ORE_IRON,
      .minCount = 1, .maxCount = 2, .skill = SKILL_MINING,
      .toolItem = ITEM_PICKAXE, .work = 10.0f, .minSkill = 15, .verb = "Kaz" },

    // Kızılot (ova çayırı — araçsız)
    { .texPath = "assets/decor/flowers.png", .src = { 0, 0, 16, 16 },
      .x = T(13), .y = T(13), .scale = 3.0f, .itemIdx = ITEM_HERB_RED,
      .minCount = 1, .maxCount = 2, .skill = SKILL_HERBALISM,
      .toolItem = -1, .work = 2.5f, .verb = "Topla" },
    { .texPath = "assets/decor/flowers.png", .src = { 0, 0, 16, 16 },
      .x = T(20), .y = T(9), .scale = 3.0f, .itemIdx = ITEM_HERB_RED,
      .minCount = 1, .maxCount = 2, .skill = SKILL_HERBALISM,
      .toolItem = -1, .work = 2.5f, .verb = "Topla" },
    { .texPath = "assets/decor/flowers.png", .src = { 0, 0, 16, 16 },
      .x = T(24), .y = T(15), .scale = 3.0f, .itemIdx = ITEM_HERB_RED,
      .minCount = 1, .maxCount = 2, .skill = SKILL_HERBALISM,
      .toolItem = -1, .work = 2.5f, .verb = "Topla" },

    // Ayotu (bataklık kenarı — ork bölgesine yakın)
    { .texPath = "assets/decor/flowers.png", .src = { 64, 80, 16, 16 },
      .x = T(25), .y = T(70), .scale = 3.0f, .itemIdx = ITEM_HERB_BLUE,
      .minCount = 1, .maxCount = 2, .skill = SKILL_HERBALISM,
      .toolItem = -1, .work = 4.0f, .verb = "Topla" },
    { .texPath = "assets/decor/flowers.png", .src = { 64, 80, 16, 16 },
      .x = T(32), .y = T(78), .scale = 3.0f, .itemIdx = ITEM_HERB_BLUE,
      .minCount = 1, .maxCount = 2, .skill = SKILL_HERBALISM,
      .toolItem = -1, .work = 4.0f, .verb = "Topla" },

    // Kesilebilir ağaçlar (orman sınırı — Oduculuk + Balta)
    { .texPath = "assets/decor/oak_medium.png", .src = { 0, 0, 32, 48 },
      .x = T(34), .y = T(8), .scale = 1.6f, .itemIdx = ITEM_WOOD,
      .minCount = 2, .maxCount = 3, .skill = SKILL_WOODCUTTING,
      .toolItem = ITEM_AXE, .work = 7.0f, .verb = "Kes" },
    { .texPath = "assets/decor/oak_medium.png", .src = { 0, 0, 32, 48 },
      .x = T(37), .y = T(12), .scale = 1.6f, .itemIdx = ITEM_WOOD,
      .minCount = 2, .maxCount = 3, .skill = SKILL_WOODCUTTING,
      .toolItem = ITEM_AXE, .work = 7.0f, .verb = "Kes" },
    { .texPath = "assets/decor/oak_medium.png", .src = { 0, 0, 32, 48 },
      .x = T(40), .y = T(9), .scale = 1.6f, .itemIdx = ITEM_WOOD,
      .minCount = 2, .maxCount = 3, .skill = SKILL_WOODCUTTING,
      .toolItem = ITEM_AXE, .work = 7.0f, .verb = "Kes" },
};
const int RESOURCE_COUNT = (int)(sizeof(RESOURCES) / sizeof(RESOURCES[0]));

static bool harvested[MAX_RESOURCES];
static float progress[MAX_RESOURCES];   // yarım iş düğümde durur (gün sonuna dek)

void resource_reset(void)
{
    for (int i = 0; i < MAX_RESOURCES; i++) {
        harvested[i] = false;
        progress[i] = 0.0f;
    }
}

void resource_daily_respawn(void)
{
    resource_reset();   // yeni gün: doğa tazelenir
}

bool resource_available(int i)
{
    return i >= 0 && i < RESOURCE_COUNT && i < MAX_RESOURCES && !harvested[i];
}

void resource_harvest(int i)
{
    if (i >= 0 && i < MAX_RESOURCES)
        harvested[i] = true;
}

float resource_progress(int i)
{
    if (i < 0 || i >= MAX_RESOURCES) return 0.0f;
    return progress[i];
}

void resource_add_progress(int i, float amount)
{
    if (i < 0 || i >= MAX_RESOURCES) return;
    progress[i] += amount;
}

int resource_nearest_available(float px, float py)
{
    int best = -1;
    float bestDist = RESOURCE_INTERACT_RANGE;
    for (int i = 0; i < RESOURCE_COUNT && i < MAX_RESOURCES; i++) {
        if (harvested[i]) continue;
        float dx = RESOURCES[i].x - px;
        float dy = RESOURCES[i].y - py;
        float dist = SDL_sqrtf(dx * dx + dy * dy);
        if (dist < bestDist) {
            bestDist = dist;
            best = i;
        }
    }
    return best;
}

void resource_render_all(R2D *renderer, R2DTexture **textures,
                         const Camera *camera, TTF_Font *font, int nearestIdx)
{
    float zoom = camera->zoom;
    for (int i = 0; i < RESOURCE_COUNT && i < MAX_RESOURCES; i++) {
        if (harvested[i] || textures[i] == NULL) continue;
        const ResourceDef *r = &RESOURCES[i];

        // x,y = taban orta → çizim kutusu ayağa hizalanır (dekor ile aynı dil)
        float w = r->src.w * r->scale;
        float h = r->src.h * r->scale;
        SDL_FRect dst = {
            (r->x - w / 2.0f - camera->x) * zoom,
            (r->y - h - camera->y) * zoom,
            w * zoom, h * zoom,
        };
        r2d_draw_texture(renderer, textures[i], &r->src, &dst);

        // Süreç barı: yarım kalmış iş görünür (0 < ilerleme < iş)
        if (progress[i] > 0.0f && progress[i] < r->work) {
            SDL_FRect bg = { dst.x, dst.y - 8.0f * zoom, dst.w, 4.0f * zoom };
            SDL_FRect fill = bg;
            fill.w *= progress[i] / r->work;
            r2d_set_blend(renderer, R2D_BLEND_ALPHA);
            r2d_set_color(renderer, 25, 25, 30, 220);
            r2d_fill_rect(renderer, &bg);
            r2d_set_color(renderer, 255, 210, 60, 255);
            r2d_fill_rect(renderer, &fill);
        }

        if (i == nearestIdx && font != NULL) {
            char hint[32];
            snprintf(hint, sizeof(hint), "[E] %s", r->verb);
            SDL_Color c = { 255, 230, 80, 255 };
            draw_text(renderer, font, hint,
                      dst.x + dst.w / 2.0f - 28.0f, dst.y - 22.0f, c);
        }
    }
}
