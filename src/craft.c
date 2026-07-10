#include <stdio.h>
#include "craft.h"
#include "item.h"
#include "font.h"
#include "ui_theme.h"
#include "item_textures.h"

#define T(n) ((n) * (float)TILE_SIZE)

// İstasyonlar: köy meydanı çevresi — ocak Demirci'nin tezgahı, kazan Baran'ın arkası
const StationDef STATIONS[] = {
    { .type = STATION_SMITH, .name = "Demirci Ocağı",
      .texPath = "assets/decor/furnace.png", .src = { 0, 0, 16, 32 },
      .x = T(8), .y = T(7), .scale = 3.0f,
      .permQuest = "demirci_ocak", .ownerName = "Demirci Mahir" },
    { .type = STATION_ALCH, .name = "Simya Kazanı",
      .texPath = "assets/decor/cauldron.png", .src = { 0, 0, 32, 32 },
      .x = T(11), .y = T(5), .scale = 2.5f,
      .permQuest = "simya_masa", .ownerName = "Tüccar Baran" },
};
const int STATION_COUNT = (int)(sizeof(STATIONS) / sizeof(STATIONS[0]));

// Temel tarifler (hibrit model: bunlar baştan açık; gelişmişler Faz 3'te blueprint'le)
const RecipeDef RECIPES[] = {
    // Demirci ocağı — Çekiç savurması (work = vuruş)
    { .station = STATION_SMITH, .name = "Bakır Külçe",
      .in1Item = ITEM_ORE_COPPER, .in1Count = 2, .in2Item = -1,
      .outItem = ITEM_INGOT_COPPER, .outCount = 1, .work = 4.0f, .xp = 6.0f },
    { .station = STATION_SMITH, .name = "Demir Külçe",
      .in1Item = ITEM_ORE_IRON, .in1Count = 2, .in2Item = -1,
      .outItem = ITEM_INGOT_IRON, .outCount = 1, .work = 6.0f, .xp = 9.0f,
      .minSkill = 10 },
    { .station = STATION_SMITH, .name = "Bakır Kılıç",
      .in1Item = ITEM_INGOT_COPPER, .in1Count = 3, .in2Item = ITEM_WOOD, .in2Count = 1,
      .outItem = ITEM_SWORD_COPPER, .outCount = 1, .work = 10.0f, .xp = 18.0f,
      .minSkill = 5 },

    // Simya kazanı — kaynatma (work = saniye)
    { .station = STATION_ALCH, .name = "Can İksiri",
      .in1Item = ITEM_HERB_RED, .in1Count = 2, .in2Item = -1,
      .outItem = ITEM_POTION_HP, .outCount = 1, .work = 4.0f, .xp = 8.0f },
    { .station = STATION_ALCH, .name = "Büyük Can İksiri",
      .in1Item = ITEM_HERB_BLUE, .in1Count = 2, .in2Item = ITEM_HERB_RED, .in2Count = 1,
      .outItem = ITEM_POTION_BIG, .outCount = 1, .work = 6.0f, .xp = 14.0f,
      .minSkill = 10 },
};
const int RECIPE_COUNT = (int)(sizeof(RECIPES) / sizeof(RECIPES[0]));

SkillType station_skill(StationType t)
{
    return (t == STATION_SMITH) ? SKILL_SMITHING : SKILL_ALCHEMY;
}

int station_nearest(float px, float py)
{
    int best = -1;
    float bestDist = STATION_INTERACT_RANGE;
    for (int i = 0; i < STATION_COUNT; i++) {
        float dx = STATIONS[i].x - px;
        float dy = STATIONS[i].y - py;
        float dist = SDL_sqrtf(dx * dx + dy * dy);
        if (dist < bestDist) {
            bestDist = dist;
            best = i;
        }
    }
    return best;
}

void station_render_all(R2D *renderer, R2DTexture **textures,
                        const Camera *camera, TTF_Font *font, int nearestIdx)
{
    float zoom = camera->zoom;
    for (int i = 0; i < STATION_COUNT; i++) {
        if (textures[i] == NULL) continue;
        const StationDef *s = &STATIONS[i];
        float w = s->src.w * s->scale;
        float h = s->src.h * s->scale;
        SDL_FRect dst = {
            (s->x - w / 2.0f - camera->x) * zoom,
            (s->y - h - camera->y) * zoom,
            w * zoom, h * zoom,
        };
        r2d_draw_texture(renderer, textures[i], &s->src, &dst);

        if (i == nearestIdx && font != NULL) {
            SDL_Color c = { 255, 230, 80, 255 };
            draw_text(renderer, font, "[E] Kullan",
                      dst.x + dst.w / 2.0f - 40.0f, dst.y - 22.0f, c);
        }
    }
}

// --- Panel yerleşimi (render ve tıklama AYNI ölçüler) ---
#define CRAFT_ROW_H 44.0f
#define CRAFT_PAD 16.0f
#define CRAFT_W 380.0f

// Bu istasyonun kaç tarifi var + n'inci tarifin RECIPES indeksi
static int station_recipe(int stationIdx, int n)
{
    int seen = 0;
    for (int i = 0; i < RECIPE_COUNT; i++) {
        if (RECIPES[i].station != STATIONS[stationIdx].type) continue;
        if (seen == n) return i;
        seen++;
    }
    return -1;
}

static int station_recipe_count(int stationIdx)
{
    int n = 0;
    for (int i = 0; i < RECIPE_COUNT; i++)
        if (RECIPES[i].station == STATIONS[stationIdx].type) n++;
    return n;
}

void craft_panel_render(int stationIdx, const Inventory *inv, const Skill *skills,
                        int selRecipe, float plusChancePct, int plusCost,
                        const ItemStack *weapon,
                        R2D *renderer, TTF_Font *font, float panelX, float panelY)
{
    const StationDef *st = &STATIONS[stationIdx];
    int rows = station_recipe_count(stationIdx);
    bool smith = (st->type == STATION_SMITH);
    float panelH = (rows + (smith ? 1 : 0)) * CRAFT_ROW_H + CRAFT_PAD * 2 + 56.0f;

    ui_panel(renderer, panelX, panelY, CRAFT_W, panelH);
    if (font == NULL) return;

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color gold  = { 255, 210, 60, 255 };
    SDL_Color dim   = { 140, 140, 150, 255 };
    SDL_Color green = { 150, 255, 150, 255 };

    draw_text(renderer, font, st->name, panelX + CRAFT_PAD, panelY + 8.0f, white);

    SkillType sk = station_skill(st->type);
    float rowY = panelY + CRAFT_PAD + 30.0f;
    for (int n = 0; n < rows; n++) {
        int ri = station_recipe(stationIdx, n);
        const RecipeDef *r = &RECIPES[ri];

        SDL_FRect row = { panelX + 8.0f, rowY, CRAFT_W - 16.0f, CRAFT_ROW_H - 6.0f };
        if (ri == selRecipe) ui_cell_hi(renderer, row.x, row.y, row.w, row.h);
        else                 ui_cell_gray(renderer, row.x, row.y, row.w, row.h);

        bool lvlOk = skills[sk].level >= r->minSkill;
        bool haveIn1 = inventory_count(inv, &ITEM_TYPES[r->in1Item]) >= r->in1Count;
        bool haveIn2 = r->in2Item < 0 ||
                       inventory_count(inv, &ITEM_TYPES[r->in2Item]) >= r->in2Count;
        SDL_Color nameCol = (lvlOk && haveIn1 && haveIn2) ? white : dim;

        char line[96];
        if (!lvlOk)
            snprintf(line, sizeof(line), "%s  [%s %d]", r->name,
                     skill_name(sk), r->minSkill);
        else
            snprintf(line, sizeof(line), "%s", r->name);
        draw_text(renderer, font, line, row.x + 10.0f, rowY + 4.0f, nameCol);

        // Malzeme listesi (var/yok renkli)
        char mats[96];
        if (r->in2Item >= 0)
            snprintf(mats, sizeof(mats), "%dx %s + %dx %s",
                     r->in1Count, ITEM_TYPES[r->in1Item].name,
                     r->in2Count, ITEM_TYPES[r->in2Item].name);
        else
            snprintf(mats, sizeof(mats), "%dx %s",
                     r->in1Count, ITEM_TYPES[r->in1Item].name);
        draw_text(renderer, font, mats, row.x + 10.0f, rowY + 20.0f,
                  (haveIn1 && haveIn2) ? green : dim);

        rowY += CRAFT_ROW_H;
    }

    // '+basma' satırı (yalnız ocak): kuşanılı GERÇEK silah gerekir
    if (smith) {
        SDL_FRect row = { panelX + 8.0f, rowY, CRAFT_W - 16.0f, CRAFT_ROW_H - 6.0f };
        ui_cell_gray(renderer, row.x, row.y, row.w, row.h);
        char line[96];
        if (weapon->type == NULL || weapon->type->weaponType == WEAPON_TOOL)
            snprintf(line, sizeof(line), "Güçlendir: silah kuşanılı değil");
        else if (weapon->plusLevel >= ITEM_MAX_PLUS)
            snprintf(line, sizeof(line), "%s +%d — tavanda",
                     weapon->type->name, weapon->plusLevel);
        else
            snprintf(line, sizeof(line), "Güçlendir: %s +%d>+%d (%dx Demir Külçe, %%%d)",
                     weapon->type->name, weapon->plusLevel, weapon->plusLevel + 1,
                     plusCost, (int)plusChancePct);
        draw_text(renderer, font, line, row.x + 10.0f, rowY + 4.0f, gold);
        draw_text(renderer, font, "Başarısızlık: +1 geriler, külçeler yanar",
                  row.x + 10.0f, rowY + 20.0f, dim);
        rowY += CRAFT_ROW_H;
    }

    SDL_Color hint = { 170, 200, 255, 255 };
    draw_text(renderer, font, smith ? "Tıkla: üret (Çekiç savurması)   ESC: kapat"
                                    : "Tıkla: kaynat   ESC: kapat",
              panelX + CRAFT_PAD, rowY + 6.0f, hint);
}

int craft_panel_recipe_at(int stationIdx, float x, float y, float panelX, float panelY)
{
    int rows = station_recipe_count(stationIdx);
    bool smith = (STATIONS[stationIdx].type == STATION_SMITH);
    float rowY = panelY + CRAFT_PAD + 30.0f;
    for (int n = 0; n < rows + (smith ? 1 : 0); n++) {
        SDL_FRect row = { panelX + 8.0f, rowY, CRAFT_W - 16.0f, CRAFT_ROW_H - 6.0f };
        if (x >= row.x && x < row.x + row.w && y >= row.y && y < row.y + row.h)
            return (n < rows) ? station_recipe(stationIdx, n) : CRAFT_ROW_PLUS;
        rowY += CRAFT_ROW_H;
    }
    return -1;
}
