#include <stdio.h>
#include <string.h>   // strncat (hedef satırı stat etiketi)
#include "craft.h"
#include "item.h"
#include "font.h"
#include "ui_theme.h"
#include "item_textures.h"

#define T(n) ((n) * (float)TILE_SIZE)

// İstasyonlar: köy meydanı çevresi — ocak Demirci'nin tezgahı, kazan Baran'ın arkası.
// const DEĞİL: oyuncu kamp ateşi (STATION_PLAYER_FIRE_IDX) konumunu main yazar.
StationDef STATIONS[] = {
    { .type = STATION_SMITH, .name = "Demirci Ocağı",
      .texPath = "assets/decor/furnace.png", .src = { 0, 0, 16, 32 }, .frames = 5,
      .x = T(8), .y = T(7), .scale = 3.0f,
      .permQuest = "demirci_ocak", .ownerName = "Demirci Mahir" },
    { .type = STATION_ALCH, .name = "Simya Kazanı",
      .texPath = "assets/decor/cauldron.png", .src = { 0, 0, 32, 32 }, .frames = 6,
      .x = T(11), .y = T(5), .scale = 2.5f,
      .permQuest = "simya_masa", .ownerName = "Tüccar Baran" },
    { .type = STATION_ENCHANT, .name = "Kadim Sunak",
      .texPath = "assets/decor/obelisk.png", .src = { 0, 0, 32, 80 }, .frames = 1,
      .x = T(102), .y = T(14), .scale = 2.0f,   // Karanlık Orman sınırı — keşifle bulunur
      .permQuest = NULL, .ownerName = NULL },   // sahipsiz harabe: kapı yok
    // Faz 6: kamp ateşleri — KAPISIZ (ateş kimsenin değil), yemek pişirme
    { .type = STATION_CAMPFIRE, .name = "Kamp Ateşi",
      .texPath = "assets/decor/campfire.png", .src = { 0, 0, 16, 32 }, .frames = 8,
      .x = T(15), .y = T(7) + 20.0f, .scale = 3.0f,   // köy: pano yanı (sabit)
      .permQuest = NULL, .ownerName = NULL },
    { .type = STATION_CAMPFIRE, .name = "Kamp Ateşi",   // STATION_PLAYER_FIRE_IDX (4)
      .texPath = "assets/decor/campfire.png", .src = { 0, 0, 16, 32 }, .frames = 8,
      .x = CAMPFIRE_PARKED, .y = CAMPFIRE_PARKED,      // sönük: park (main kurar/söndürür)
      .scale = 3.0f,
      .permQuest = NULL, .ownerName = NULL },
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

    // Ocak — blueprint'li gelişmiş tarifler (Faz 3)
    { .station = STATION_SMITH, .name = "Demir Kılıç",
      .in1Item = ITEM_INGOT_IRON, .in1Count = 3, .in2Item = ITEM_WOOD, .in2Count = 1,
      .outItem = ITEM_SWORD_IRON, .outCount = 1, .work = 12.0f, .xp = 22.0f,
      .minSkill = 15, .blueprintItem = ITEM_BP_SWORD_IRON },
    { .station = STATION_SMITH, .name = "Demir Zırh",
      .in1Item = ITEM_INGOT_IRON, .in1Count = 4, .in2Item = -1,
      .outItem = ITEM_CHEST_IRON, .outCount = 1, .work = 12.0f, .xp = 20.0f,
      .minSkill = 15, .blueprintItem = ITEM_BP_CHEST_IRON },
    { .station = STATION_SMITH, .name = "Demir Miğfer",
      .in1Item = ITEM_INGOT_IRON, .in1Count = 3, .in2Item = -1,
      .outItem = ITEM_HELMET_IRON, .outCount = 1, .work = 10.0f, .xp = 16.0f,
      .minSkill = 12, .blueprintItem = ITEM_BP_HELMET_IRON },

    // Simya kazanı — kaynatma (work = saniye)
    { .station = STATION_ALCH, .name = "Can İksiri",
      .in1Item = ITEM_HERB_RED, .in1Count = 2, .in2Item = -1,
      .outItem = ITEM_POTION_HP, .outCount = 1, .work = 4.0f, .xp = 8.0f },
    { .station = STATION_ALCH, .name = "Büyük Can İksiri",
      .in1Item = ITEM_HERB_BLUE, .in1Count = 2, .in2Item = ITEM_HERB_RED, .in2Count = 1,
      .outItem = ITEM_POTION_BIG, .outCount = 1, .work = 6.0f, .xp = 14.0f,
      .minSkill = 10 },

    // Kazan — blueprint'li (Faz 3)
    { .station = STATION_ALCH, .name = "Hız İksiri",
      .in1Item = ITEM_HERB_RED, .in1Count = 2, .in2Item = ITEM_HERB_BLUE, .in2Count = 1,
      .outItem = ITEM_POTION_SPEED, .outCount = 1, .work = 5.0f, .xp = 12.0f,
      .minSkill = 5, .blueprintItem = ITEM_BP_POTION_SPEED },
    { .station = STATION_ALCH, .name = "Zehir Yağı",
      .in1Item = ITEM_HERB_BLUE, .in1Count = 3, .in2Item = -1,
      .outItem = ITEM_POISON_OIL, .outCount = 1, .work = 6.0f, .xp = 15.0f,
      .minSkill = 15, .blueprintItem = ITEM_BP_POISON_OIL },

    // Sunak — rün üretimi (ŞANSLI: tutmayan özler yanar, Enchanting telafi eder)
    { .station = STATION_ENCHANT, .name = "Pembe Rün",
      .in1Item = ITEM_ESSENCE_PALE, .in1Count = 3, .in2Item = -1,
      .outItem = ITEM_RUNE_PINK, .outCount = 1, .work = 5.0f, .xp = 15.0f },
    { .station = STATION_ENCHANT, .name = "Zehir Rünü",
      .in1Item = ITEM_ESSENCE_TOXIC, .in1Count = 3, .in2Item = -1,
      .outItem = ITEM_RUNE_POISON, .outCount = 1, .work = 5.0f, .xp = 18.0f,
      .minSkill = 10 },

    // Ocak — oduculuk ürünleri (Faz 5): ok ve yay (odunun craft anlamı).
    // SONA eklendi: save v10 öğrenilen-tarif bayrakları İNDEKSLE saklanır,
    // ortaya ekleme eski kayıtları kaydırırdı.
    // Ok kademeleri (Faz 8): standart en ucuz (külçesiz), kemik/demir pahalı-sert
    { .station = STATION_SMITH, .name = "Ok x10",
      .in1Item = ITEM_WOOD, .in1Count = 1, .in2Item = -1,
      .outItem = ITEM_ARROW, .outCount = 10, .work = 3.0f, .xp = 4.0f },
    { .station = STATION_SMITH, .name = "Yay",
      .in1Item = ITEM_WOOD, .in1Count = 2, .in2Item = ITEM_INGOT_IRON, .in2Count = 1,
      .outItem = ITEM_BOW, .outCount = 1, .work = 8.0f, .xp = 14.0f,
      .minSkill = 5 },

    // Ocak — deri işleme (Faz 6): hafif zırh seti (ARMOR_LIGHT nihayet item'lı)
    { .station = STATION_SMITH, .name = "Deri Başlık",
      .in1Item = ITEM_LEATHER, .in1Count = 2, .in2Item = -1,
      .outItem = ITEM_HELMET_LEATHER, .outCount = 1, .work = 4.0f, .xp = 8.0f },
    { .station = STATION_SMITH, .name = "Deri Zırh",
      .in1Item = ITEM_LEATHER, .in1Count = 4, .in2Item = -1,
      .outItem = ITEM_CHEST_LEATHER, .outCount = 1, .work = 6.0f, .xp = 12.0f },
    { .station = STATION_SMITH, .name = "Deri Bot",
      .in1Item = ITEM_LEATHER, .in1Count = 2, .in2Item = -1,
      .outItem = ITEM_BOOTS_LEATHER, .outCount = 1, .work = 4.0f, .xp = 8.0f },

    // Kazan — yemek (Faz 6): yavaş ama toplamı yüksek yenilenme (iksir alternatifi)
    { .station = STATION_ALCH, .name = "Pişmiş Et",
      .in1Item = ITEM_MEAT, .in1Count = 1, .in2Item = -1,
      .outItem = ITEM_MEAT_COOKED, .outCount = 1, .work = 3.0f, .xp = 4.0f },

    // Kamp ateşi — aynı yemek (avcı döngüsü: avla → oracıkta pişir → ye)
    { .station = STATION_CAMPFIRE, .name = "Pişmiş Et",
      .in1Item = ITEM_MEAT, .in1Count = 1, .in2Item = -1,
      .outItem = ITEM_MEAT_COOKED, .outCount = 1, .work = 3.0f, .xp = 4.0f },

    // Kazan — slime jeli (Faz 7): alan yavaşlatma tuzağı (Buz Oku'nun alan kardeşi)
    { .station = STATION_ALCH, .name = "Yapışkan Tuzağı",
      .in1Item = ITEM_SLIME_GEL, .in1Count = 2, .in2Item = ITEM_HERB_BLUE, .in2Count = 1,
      .outItem = ITEM_STICKY_TRAP, .outCount = 1, .work = 4.0f, .xp = 8.0f },

    // Ocak — premium oklar (Faz 8): keskin kemik uç / demir uç
    { .station = STATION_SMITH, .name = "Kemik Oku x10",
      .in1Item = ITEM_BONE, .in1Count = 1, .in2Item = ITEM_WOOD, .in2Count = 1,
      .outItem = ITEM_ARROW_BONE, .outCount = 10, .work = 4.0f, .xp = 6.0f },
    { .station = STATION_ALCH, .name = "Mana İksiri",
      .in1Item = ITEM_HERB_BLUE, .in1Count = 2, .in2Item = -1,
      .outItem = ITEM_POTION_MANA, .outCount = 1, .work = 4.0f, .xp = 8.0f },
    { .station = STATION_SMITH, .name = "Asa",
      .in1Item = ITEM_WOOD, .in1Count = 2, .in2Item = ITEM_ESSENCE_PALE, .in2Count = 1,
      .outItem = ITEM_STAFF, .outCount = 1, .work = 8.0f, .xp = 14.0f,
      .minSkill = 5 },
    { .station = STATION_SMITH, .name = "Demir Ok x10",
      .in1Item = ITEM_INGOT_IRON, .in1Count = 1, .in2Item = ITEM_WOOD, .in2Count = 1,
      .outItem = ITEM_ARROW_IRON, .outCount = 10, .work = 5.0f, .xp = 8.0f,
      .minSkill = 10 },
    { .station = STATION_SMITH, .name = "Maymuncuk x3",
      .in1Item = ITEM_INGOT_IRON, .in1Count = 1, .in2Item = -1,
      .outItem = ITEM_LOCKPICK, .outCount = 3, .work = 4.0f, .xp = 5.0f,
      .minSkill = 5 },
    // --- M2 Buff yemekleri: kamp ateşinde ve kazanda (Pişmiş Et deseni) ---
    { .station = STATION_CAMPFIRE, .name = "Avcı Yahnisi",
      .in1Item = ITEM_MEAT, .in1Count = 2, .in2Item = ITEM_HERB_RED, .in2Count = 1,
      .outItem = ITEM_STEW, .outCount = 1, .work = 4.0f, .xp = 6.0f },
    { .station = STATION_ALCH, .name = "Avcı Yahnisi",
      .in1Item = ITEM_MEAT, .in1Count = 2, .in2Item = ITEM_HERB_RED, .in2Count = 1,
      .outItem = ITEM_STEW, .outCount = 1, .work = 4.0f, .xp = 6.0f },
    { .station = STATION_CAMPFIRE, .name = "Ot Çorbası",
      .in1Item = ITEM_HERB_BLUE, .in1Count = 2, .in2Item = ITEM_MEAT, .in2Count = 1,
      .outItem = ITEM_SOUP, .outCount = 1, .work = 4.0f, .xp = 6.0f },
    { .station = STATION_ALCH, .name = "Ot Çorbası",
      .in1Item = ITEM_HERB_BLUE, .in1Count = 2, .in2Item = ITEM_MEAT, .in2Count = 1,
      .outItem = ITEM_SOUP, .outCount = 1, .work = 4.0f, .xp = 6.0f },
    // ÇANTA SEKMELERİ: İp (ara malzeme) + 4 çanta eşyası (üretilince OTOMATİK
    // takılır — craft_finish özel yolu; gümüş oyunda yok, toka = Demir Külçe)
    { .station = STATION_CAMPFIRE, .name = "İp",
      .in1Item = ITEM_HERB_BLUE, .in1Count = 2, .in2Item = -1,
      .outItem = ITEM_ROPE, .outCount = 1, .work = 3.0f, .xp = 2.0f },
    { .station = STATION_SMITH, .name = "Malzeme Kesesi",
      .in1Item = ITEM_LEATHER, .in1Count = 4, .in2Item = ITEM_ROPE, .in2Count = 2,
      .outItem = ITEM_POUCH_MATERIAL, .outCount = 1, .work = 8.0f, .xp = 15.0f,
      .minSkill = 10 },
    { .station = STATION_SMITH, .name = "Deri Çanta",
      .in1Item = ITEM_LEATHER, .in1Count = 6, .in2Item = ITEM_ROPE, .in2Count = 2,
      .outItem = ITEM_BAG_LEATHER, .outCount = 1, .work = 10.0f, .xp = 20.0f,
      .minSkill = 15 },
    { .station = STATION_SMITH, .name = "Usta Kesesi",
      .in1Item = ITEM_LEATHER, .in1Count = 8, .in2Item = ITEM_INGOT_IRON, .in2Count = 1,
      .outItem = ITEM_POUCH_MASTER, .outCount = 1, .work = 12.0f, .xp = 25.0f,
      .minSkill = 35 },
    { .station = STATION_SMITH, .name = "Gezgin Çantası",
      .in1Item = ITEM_LEATHER, .in1Count = 10, .in2Item = ITEM_INGOT_IRON, .in2Count = 1,
      .outItem = ITEM_BAG_TRAVELER, .outCount = 1, .work = 14.0f, .xp = 30.0f,
      .minSkill = 40 },

    { .station = STATION_CAMPFIRE, .name = "Gezgin Kavurması",
      .in1Item = ITEM_MEAT, .in1Count = 2, .in2Item = ITEM_HERB_BLUE, .in2Count = 1,
      .outItem = ITEM_KAVURMA, .outCount = 1, .work = 5.0f, .xp = 8.0f },
    { .station = STATION_ALCH, .name = "Gezgin Kavurması",
      .in1Item = ITEM_MEAT, .in1Count = 2, .in2Item = ITEM_HERB_BLUE, .in2Count = 1,
      .outItem = ITEM_KAVURMA, .outCount = 1, .work = 5.0f, .xp = 8.0f },
};
const int RECIPE_COUNT = (int)(sizeof(RECIPES) / sizeof(RECIPES[0]));

SkillType station_skill(StationType t)
{
    if (t == STATION_SMITH) return SKILL_SMITHING;
    if (t == STATION_ENCHANT) return SKILL_ENCHANTING;
    return SKILL_ALCHEMY;
}

// --- Öğrenilen tarifler (blueprint'li olanlar; save v10 köprüsü) ---
static bool learned[MAX_RECIPES];

bool craft_recipe_learned(int ri)
{
    if (ri < 0 || ri >= RECIPE_COUNT) return false;
    if (RECIPES[ri].blueprintItem <= 0) return true;   // temel tarif hep açık
    return ri < MAX_RECIPES && learned[ri];
}

void craft_learn_recipe(int ri)
{
    if (ri >= 0 && ri < MAX_RECIPES)
        learned[ri] = true;
}

void craft_reset_learned(void)
{
    for (int i = 0; i < MAX_RECIPES; i++)
        learned[i] = false;
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
                        const Camera *camera, TTF_Font *font,
                        int nearestIdx, int activeIdx)
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

        // Aktif üretimde animasyon oynar (kaynayan kazan / alazlanan ocak);
        // kamp ateşi YANAN bir şeydir — hep oynar
        SDL_FRect src = s->src;
        if ((i == activeIdx || s->type == STATION_CAMPFIRE) && s->frames > 1)
            src.x += s->src.w * (float)((SDL_GetTicks() / 130) % (Uint64)s->frames);
        r2d_draw_texture(renderer, textures[i], &src, &dst);

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
#define CRAFT_MAX_VIS 16   // panelde aynı anda görünen satır (900px penceresi)

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

int craft_panel_max_scroll(int stationIdx)
{
    const StationDef *st = &STATIONS[stationIdx];
    bool smith = (st->type == STATION_SMITH);
    bool ench  = (st->type == STATION_ENCHANT);
    int total = station_recipe_count(stationIdx)
              + (smith ? 1 : (ench ? (1 + 2 + STAT_COUNT) : 0));
    return (total > CRAFT_MAX_VIS) ? total - CRAFT_MAX_VIS : 0;
}

void craft_panel_render(int stationIdx, const Inventory *inv,
                        const Inventory *inv2, const Skill *skills, int scroll,
                        int selRecipe, float plusChancePct, int plusCost,
                        const ItemStack *weapon,
                        R2D *renderer, TTF_Font *font, float panelX, float panelY)
{
    const StationDef *st = &STATIONS[stationIdx];
    int rows = station_recipe_count(stationIdx);
    bool smith = (st->type == STATION_SMITH);
    bool ench  = (st->type == STATION_ENCHANT);
    int extraRows = smith ? 1 : (ench ? (1 + 2 + STAT_COUNT) : 0);
    int visRows = rows + extraRows;
    if (visRows > CRAFT_MAX_VIS) visRows = CRAFT_MAX_VIS;
    float panelH = visRows * CRAFT_ROW_H + CRAFT_PAD * 2 + 56.0f;
    float clipTop = panelY + CRAFT_PAD + 30.0f;             // satır alanı
    float clipBot = clipTop + visRows * CRAFT_ROW_H;

    ui_panel(renderer, panelX, panelY, CRAFT_W, panelH);
    if (font == NULL) return;

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color gold  = { 255, 210, 60, 255 };
    SDL_Color dim   = { 140, 140, 150, 255 };
    SDL_Color green = { 150, 255, 150, 255 };

    draw_text(renderer, font, st->name, panelX + CRAFT_PAD, panelY + 8.0f, white);

    SkillType sk = station_skill(st->type);
    // Kaydırma: satırlar yukarı kayar; klip dışına düşen HİÇ çizilmez
    #define CROW_ON (rowY >= clipTop - 0.5f && rowY <= clipBot - CRAFT_ROW_H + 0.5f)
    float rowY = panelY + CRAFT_PAD + 30.0f - (float)scroll * CRAFT_ROW_H;
    for (int n = 0; n < rows; n++) {
        int ri = station_recipe(stationIdx, n);
        const RecipeDef *r = &RECIPES[ri];

        if (!CROW_ON) { rowY += CRAFT_ROW_H; continue; }

        SDL_FRect row = { panelX + 8.0f, rowY, CRAFT_W - 16.0f, CRAFT_ROW_H - 6.0f };
        if (ri == selRecipe) ui_cell_hi(renderer, row.x, row.y, row.w, row.h);
        else                 ui_cell_gray(renderer, row.x, row.y, row.w, row.h);

        // Blueprint kilidi: öğrenilmemiş tarif gizemli görünür (varlığı öğretir)
        if (!craft_recipe_learned(ri)) {
            draw_text(renderer, font, "???", row.x + 10.0f, rowY + 4.0f, dim);
            draw_text(renderer, font, "Blueprint gerekli", row.x + 10.0f, rowY + 20.0f, dim);
            rowY += CRAFT_ROW_H;
            continue;
        }

        bool lvlOk = skills[sk].level >= r->minSkill;
        bool haveIn1 = inventory_count(inv, &ITEM_TYPES[r->in1Item]) +
                       inventory_count(inv2, &ITEM_TYPES[r->in1Item]) >= r->in1Count;
        bool haveIn2 = r->in2Item < 0 ||
                       inventory_count(inv, &ITEM_TYPES[r->in2Item]) +
                       inventory_count(inv2, &ITEM_TYPES[r->in2Item]) >= r->in2Count;
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
    if (smith && CROW_ON) {
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
    else if (smith) rowY += CRAFT_ROW_H;   // klip dışı — sayaç yine ilerler

    // --- Sunak özel satırları: hedef + rün basma + stat basma ---
    if (ench) {
        // Hedef satırı (tıkla: kuşanılı ekipmanlar arasında döner)
        if (!CROW_ON) goto ench_target_done;
        {
        SDL_FRect row = { panelX + 8.0f, rowY, CRAFT_W - 16.0f, CRAFT_ROW_H - 6.0f };
        ui_cell_hi(renderer, row.x, row.y, row.w, row.h);
        char line[112];
        if (weapon->type == NULL)
            snprintf(line, sizeof(line), "Hedef: (boş slot — tıkla: değiştir)");
        else {
            char stats[64] = "";
            // Basılı rün görünür olsun (uygulama başarısı panelden doğrulanır)
            static const char *fxName[] = { NULL, "Pembe", "Alev", "Buz", "Zehir" };
            if (weapon->fx > 0 && weapon->fx < (Sint8)(sizeof(fxName)/sizeof(fxName[0]))) {
                char one[24];
                snprintf(one, sizeof(one), " [Rün: %s]", fxName[weapon->fx]);
                strncat(stats, one, sizeof(stats) - strlen(stats) - 1);
            }
            for (int k = 0; k < 2; k++)
                if (weapon->statType[k] >= 0) {
                    char one[20];
                    snprintf(one, sizeof(one), " [%s %d]",
                             STAT_NAMES[weapon->statType[k]], weapon->statTier[k]);
                    strncat(stats, one, sizeof(stats) - strlen(stats) - 1);
                }
            snprintf(line, sizeof(line), "Hedef: %s +%d%s (tıkla: değiştir)",
                     weapon->type->name, weapon->plusLevel, stats);
        }
        draw_text(renderer, font, line, row.x + 10.0f, rowY + 12.0f, UI_TEXT_DARK);
        }
ench_target_done:
        rowY += CRAFT_ROW_H;

        // Rün basma satırları (yalnız SİLAHA; eski efekt yanar)
        const int runeItem[2] = { ITEM_RUNE_PINK, ITEM_RUNE_POISON };
        const char *runeTxt[2] = { "Rün Bas: Pembe (+%8 hasar)",
                                   "Rün Bas: Zehir (vuruşta DoT)" };
        for (int rn = 0; rn < 2; rn++) {
            if (!CROW_ON) { rowY += CRAFT_ROW_H; continue; }
            SDL_FRect rrow = { panelX + 8.0f, rowY, CRAFT_W - 16.0f, CRAFT_ROW_H - 6.0f };
            ui_cell_gray(renderer, rrow.x, rrow.y, rrow.w, rrow.h);
            bool haveRune = inventory_count(inv, &ITEM_TYPES[runeItem[rn]]) +
                            inventory_count(inv2, &ITEM_TYPES[runeItem[rn]]) >= 1;
            draw_text(renderer, font, runeTxt[rn], rrow.x + 10.0f, rowY + 4.0f,
                      haveRune ? white : dim);
            draw_text(renderer, font, "1 rün — kuşanılı silaha (eski efekt yanar)",
                      rrow.x + 10.0f, rowY + 20.0f, haveRune ? green : dim);
            rowY += CRAFT_ROW_H;
        }

        // Stat basma satırları: hedefteki kademe + maliyet (garantili ama pahalı)
        const int stoneItem[STAT_COUNT] = { ITEM_STONE_STR, ITEM_STONE_DEX, ITEM_STONE_HP,
                                            ITEM_STONE_DEF, ITEM_STONE_MP, ITEM_STONE_STA,
                                            ITEM_STONE_INT };
        for (int s = 0; s < STAT_COUNT; s++) {
            if (!CROW_ON) { rowY += CRAFT_ROW_H; continue; }
            SDL_FRect srow = { panelX + 8.0f, rowY, CRAFT_W - 16.0f, CRAFT_ROW_H - 6.0f };
            ui_cell_gray(renderer, srow.x, srow.y, srow.w, srow.h);

            // Hedefteki mevcut kademe + boş slot durumu
            int cur = 0; bool slotFree = false;
            if (weapon->type != NULL) {
                int used = 0;
                for (int k = 0; k < 2; k++) {
                    if (weapon->statType[k] == s) cur = weapon->statTier[k];
                    if (weapon->statType[k] >= 0) used++;
                }
                slotFree = (cur > 0) || (used < 2);
            }
            int next = cur + 1;
            char line2[112];
            if (cur >= STAT_MAX_TIER)
                snprintf(line2, sizeof(line2), "%s III — tavanda", STAT_NAMES[s]);
            else if (!slotFree)
                snprintf(line2, sizeof(line2), "%s — slotlar dolu (2/2)", STAT_NAMES[s]);
            else
                snprintf(line2, sizeof(line2), "%s %d>%d (+%d) — %dx %s + %dx Külçe",
                         STAT_NAMES[s], cur, next, STAT_TIER_VALUE[s] * next,
                         next, ITEM_TYPES[stoneItem[s]].name, next * 2);
            bool can = weapon->type != NULL && cur < STAT_MAX_TIER && slotFree &&
                       inventory_count(inv, &ITEM_TYPES[stoneItem[s]]) +
                       inventory_count(inv2, &ITEM_TYPES[stoneItem[s]]) >= next &&
                       inventory_count(inv, &ITEM_TYPES[ITEM_INGOT_IRON]) +
                       inventory_count(inv2, &ITEM_TYPES[ITEM_INGOT_IRON]) >= next * 2;
            draw_text(renderer, font, line2, srow.x + 10.0f, rowY + 12.0f,
                      can ? white : dim);
            rowY += CRAFT_ROW_H;
        }
    }

    SDL_Color hint = { 170, 200, 255, 255 };
    draw_text(renderer, font, smith ? "Tıkla: üret (Çekiç savurması)   ESC: kapat"
              : ench ? "Tıkla: bas/üret (şanslı rün üretimi)   ESC: kapat"
              : (st->type == STATION_CAMPFIRE) ? "Tıkla: pişir   ESC: kapat"
                     : "Tıkla: kaynat   ESC: kapat",
              panelX + CRAFT_PAD, clipBot + 6.0f, hint);

    // Kaydırma göstergeleri: liste taşıyorsa yukarı/aşağı ipucu
    int maxScroll = craft_panel_max_scroll(stationIdx);
    if (scroll > 0)
        draw_text(renderer, font, "▲", panelX + CRAFT_W - 26.0f, clipTop - 22.0f, hint);
    if (scroll < maxScroll)
        draw_text(renderer, font, "▼ tekerlek", panelX + CRAFT_W - 110.0f,
                  clipBot + 6.0f, hint);
    #undef CROW_ON
}

int craft_panel_recipe_at(int stationIdx, float x, float y, float panelX, float panelY, int scroll)
{
    int rows = station_recipe_count(stationIdx);
    bool smith = (STATIONS[stationIdx].type == STATION_SMITH);
    bool ench  = (STATIONS[stationIdx].type == STATION_ENCHANT);
    int extraRows = smith ? 1 : (ench ? (1 + 2 + STAT_COUNT) : 0);
    int visRows = rows + extraRows;
    if (visRows > CRAFT_MAX_VIS) visRows = CRAFT_MAX_VIS;
    float clipTop = panelY + CRAFT_PAD + 30.0f;
    float clipBot = clipTop + visRows * CRAFT_ROW_H;

    float rowY = clipTop - (float)scroll * CRAFT_ROW_H;
    for (int n = 0; n < rows + extraRows; n++) {
        if (rowY < clipTop - 0.5f || rowY > clipBot - CRAFT_ROW_H + 0.5f) {
            rowY += CRAFT_ROW_H;
            continue;   // klip dışı satır tıklanamaz (render ile aynı)
        }
        SDL_FRect row = { panelX + 8.0f, rowY, CRAFT_W - 16.0f, CRAFT_ROW_H - 6.0f };
        if (x >= row.x && x < row.x + row.w && y >= row.y && y < row.y + row.h) {
            if (n < rows) return station_recipe(stationIdx, n);
            int e = n - rows;   // özel satır sırası
            if (smith) return CRAFT_ROW_PLUS;
            if (e == 0) return CRAFT_ROW_TARGET;
            if (e == 1) return CRAFT_ROW_RUNE0;
            if (e == 2) return CRAFT_ROW_RUNE1;
            return CRAFT_ROW_STAT_BASE - (e - 3);   // -10..-15
        }
        rowY += CRAFT_ROW_H;
    }
    return -1;
}
