#include <stdio.h>
#include "perk.h"
#include "item.h"     // ArmorClass indeksleri (defMult)
#include "font.h"
#include "ui_theme.h"

const int PERK_THRESHOLDS[PERK_TIERS] = { 25, 50, 75, 100 };

// İçerik tablosu — SADECE isim/açıklama (etkiler perk_compute_effects'te).
// Sıra: [tier][option]. Onaylı taslak (2026-07-09).
static const PerkOption ONE_HANDED_PERKS[PERK_TIERS][PERK_OPTIONS] = {
    { {"Kanatma",      "Vuruslar 3 sn'de hasarin %30'u kadar kanatir"},
      {"Hizli Bilek",  "Saldiri araligi -%10"},
      {"Guclu Tutus",  "Tek el hasari +%10"} },
    { {"Derin Kesik",  "Kritik carpani 1.5 -> 1.8"},
      {"Seri Vurus",   "Quick Slash bekleme -%25"},
      {"Ezici Darbe",  "Power Strike x2.0 -> x2.4"} },
    { {"Kasap",        "Kanama suresi ve hasari 2x"},
      {"Usta Bilek",   "Saldiri araligi -%15 daha"},
      {"Keskin Goz",   "Kritik sansi +%8"} },
    { {"Kilic Ustasi", "Tek el hasari +%25"},
      {"Firtina",      "Power Strike bekleme 6 -> 3 sn"},
      {"Infaz",        "Cani %20 altindaki hedefe +%50 hasar"} },
};
static const PerkOption LIGHT_PERKS[PERK_TIERS][PERK_OPTIONS] = {
    { {"Cevik",        "+%5 hareket hizi"},
      {"Esnek",        "Hafif zirh defansi +%10"},
      {"Akici",        "Iksir beklemesi -%15"} },
    { {"Golge Adimi",  "Gece +%5 ek hiz"},
      {"Kacinma",      "%5 sansla hasari tamamen savustur"},
      {"Dayaniklilik", "+15 max HP"} },
    { {"Ruzgar",       "+%5 hiz daha"},
      {"Ikinci Deri",  "Hafif zirh defansi +%20"},
      {"Refleks",      "Kacinma sansi %8'e cikar"} },
    { {"Hayalet",      "Moblar seni %15 gec farkeder"},
      {"Sarsilmaz",    "Itilme yemezsin"},
      {"Zinde",        "Can %50 ustundeyken +%10 hasar"} },
};
static const PerkOption MEDIUM_PERKS[PERK_TIERS][PERK_OPTIONS] = {
    { {"Dengeli",      "Orta zirh defansi +%7"},
      {"Uyumlu",       "+%3 hiz ve +5 HP"},
      {"Saglam",       "+10 max HP"} },
    { {"Koruyucu Katman", "Alinan hasar -%5"},
      {"Karsilik",     "Vurana hasarin %10'u yansir"},
      {"Ilk Yardim",   "Iksirler +%25 etkili"} },
    { {"Zirh Ustasi",  "Orta zirh defansi +%25"},
      {"Yarali Ayi",   "Can %30 altinda alinan hasar -%15"},
      {"Metanet",      "+20 max HP"} },
    { {"Kale",         "Alinan hasar -%10 daha"},
      {"Gazi",         "Hasar yiyince 5 sn +%10 hasar"},
      {"Yikilmaz",     "Olumcul darbe %20 sansla 1 HP birakir (60sn)"} },
};
static const PerkOption HEAVY_PERKS[PERK_TIERS][PERK_OPTIONS] = {
    { {"Demir Ten",    "Agir zirh defansi +%10"},
      {"Yuk Aliskanligi", "+10 max HP"},
      {"Saglam Durus", "Itilme -%50"} },
    { {"Kalkan Duvari","Tum defans daha verimli (K: 100->120)"},
      {"Diken Zirh",   "Yakin saldirgana 3 hasar yansir"},
      {"Dayanikli",    "+20 max HP"} },
    { {"Agir Ustalik", "Agir zirh defansi +%25"},
      {"Ezici Agirlik","Itme +%10, hasar +%5"},
      {"Granit",       "Alinan hasar -%8"} },
    { {"Juggernaut",   "Alinan hasar -%12 daha"},
      {"Kule",         "Hareketsizken alinan hasar -%20"},
      {"Olumsuz",      "Olumcul darbe 1 HP birakir (90sn)"} },
};

static const PerkOption (*perk_table(SkillType s))[PERK_OPTIONS]
{
    switch (s) {
        case SKILL_ONE_HANDED:   return ONE_HANDED_PERKS;
        case SKILL_LIGHT_ARMOR:  return LIGHT_PERKS;
        case SKILL_MEDIUM_ARMOR: return MEDIUM_PERKS;
        case SKILL_HEAVY_ARMOR:  return HEAVY_PERKS;
        default:                 return NULL;   // v1'de içerik yok
    }
}

bool perk_skill_has_content(SkillType s)
{
    return perk_table(s) != NULL;
}

const PerkOption *perk_option(SkillType s, int tier, int opt)
{
    const PerkOption (*t)[PERK_OPTIONS] = perk_table(s);
    if (t == NULL || tier < 0 || tier >= PERK_TIERS || opt < 0 || opt >= PERK_OPTIONS)
        return NULL;
    return &t[tier][opt];
}

int perk_pending_tier(SkillType s, int level, const Sint8 choice[SKILL_COUNT][PERK_TIERS])
{
    if (!perk_skill_has_content(s)) return -1;
    for (int t = 0; t < PERK_TIERS; t++)
        if (level >= PERK_THRESHOLDS[t] && choice[s][t] == PERK_UNCHOSEN)
            return t;
    return -1;
}

// Bir seçim aktif mi? (okunabilirlik makrosu)
#define HAS(sk, t, o) (choice[sk][t] == (o))

void perk_compute_effects(const Sint8 choice[SKILL_COUNT][PERK_TIERS],
                          PerkEffects *fx)
{
    // Nötr başlangıç
    *fx = (PerkEffects){
        .oneHandDmgMult = 1.0f, .attackIntervalMult = 1.0f,
        .critMult = 1.5f, .psCdMult = 1.0f, .qsCdMult = 1.0f,
        .bleedPower = 1.0f,
        .defMult = { 1.0f, 1.0f, 1.0f, 1.0f },
        .dmgTakenMult = 1.0f, .defenseK = 100.0f, .kbTakenMult = 1.0f,
        .speedMult = 1.0f, .nightSpeedMult = 1.0f,
        .potionEffMult = 1.0f, .potionCdMult = 1.0f,
        .detectOnPlayerMult = 1.0f, .kbDealtMult = 1.0f,
    };

    // --- One-Handed ---
    if (HAS(SKILL_ONE_HANDED, 0, 0)) fx->bleedFrac = 0.30f;
    if (HAS(SKILL_ONE_HANDED, 0, 1)) fx->attackIntervalMult *= 0.90f;
    if (HAS(SKILL_ONE_HANDED, 0, 2)) fx->oneHandDmgMult *= 1.10f;
    if (HAS(SKILL_ONE_HANDED, 1, 0)) fx->critMult = 1.8f;
    if (HAS(SKILL_ONE_HANDED, 1, 1)) fx->qsCdMult = 0.75f;
    if (HAS(SKILL_ONE_HANDED, 1, 2)) fx->psScaleOverride = 2.4f;
    if (HAS(SKILL_ONE_HANDED, 2, 0)) fx->bleedPower = 2.0f;
    if (HAS(SKILL_ONE_HANDED, 2, 1)) fx->attackIntervalMult *= 0.85f;
    if (HAS(SKILL_ONE_HANDED, 2, 2)) fx->critChanceBonus += 0.08f;
    if (HAS(SKILL_ONE_HANDED, 3, 0)) fx->oneHandDmgMult *= 1.25f;
    if (HAS(SKILL_ONE_HANDED, 3, 1)) fx->psCdMult = 0.5f;
    if (HAS(SKILL_ONE_HANDED, 3, 2)) fx->executeBonus = 0.50f;

    // --- Hafif Zırh ---
    if (HAS(SKILL_LIGHT_ARMOR, 0, 0)) fx->speedMult *= 1.05f;
    if (HAS(SKILL_LIGHT_ARMOR, 0, 1)) fx->defMult[ARMOR_LIGHT] *= 1.10f;
    if (HAS(SKILL_LIGHT_ARMOR, 0, 2)) fx->potionCdMult *= 0.85f;
    if (HAS(SKILL_LIGHT_ARMOR, 1, 0)) fx->nightSpeedMult *= 1.05f;
    if (HAS(SKILL_LIGHT_ARMOR, 1, 1)) fx->dodgeChance = 0.05f;
    if (HAS(SKILL_LIGHT_ARMOR, 1, 2)) fx->maxHpBonus += 15.0f;
    if (HAS(SKILL_LIGHT_ARMOR, 2, 0)) fx->speedMult *= 1.05f;
    if (HAS(SKILL_LIGHT_ARMOR, 2, 1)) fx->defMult[ARMOR_LIGHT] *= 1.20f;
    if (HAS(SKILL_LIGHT_ARMOR, 2, 2)) fx->dodgeChance = 0.08f;
    if (HAS(SKILL_LIGHT_ARMOR, 3, 0)) fx->detectOnPlayerMult *= 0.85f;
    if (HAS(SKILL_LIGHT_ARMOR, 3, 1)) fx->kbTakenMult = 0.0f;
    if (HAS(SKILL_LIGHT_ARMOR, 3, 2)) fx->dmgWhenHealthy = 0.10f;

    // --- Orta Zırh ---
    if (HAS(SKILL_MEDIUM_ARMOR, 0, 0)) fx->defMult[ARMOR_MEDIUM] *= 1.07f;
    if (HAS(SKILL_MEDIUM_ARMOR, 0, 1)) { fx->speedMult *= 1.03f; fx->maxHpBonus += 5.0f; }
    if (HAS(SKILL_MEDIUM_ARMOR, 0, 2)) fx->maxHpBonus += 10.0f;
    if (HAS(SKILL_MEDIUM_ARMOR, 1, 0)) fx->dmgTakenMult *= 0.95f;
    if (HAS(SKILL_MEDIUM_ARMOR, 1, 1)) fx->reflectFrac += 0.10f;
    if (HAS(SKILL_MEDIUM_ARMOR, 1, 2)) fx->potionEffMult *= 1.25f;
    if (HAS(SKILL_MEDIUM_ARMOR, 2, 0)) fx->defMult[ARMOR_MEDIUM] *= 1.25f;
    // Yarali Ayi: can esigine bagli — main dmgTaken hesabında ayrıca uygular
    if (HAS(SKILL_MEDIUM_ARMOR, 2, 2)) fx->maxHpBonus += 20.0f;
    if (HAS(SKILL_MEDIUM_ARMOR, 3, 0)) fx->dmgTakenMult *= 0.90f;
    if (HAS(SKILL_MEDIUM_ARMOR, 3, 1)) fx->gaziDmgBonus = 0.10f;
    if (HAS(SKILL_MEDIUM_ARMOR, 3, 2)) fx->deathSaveCooldown = 60.0f;

    // --- Ağır Zırh ---
    if (HAS(SKILL_HEAVY_ARMOR, 0, 0)) fx->defMult[ARMOR_HEAVY] *= 1.10f;
    if (HAS(SKILL_HEAVY_ARMOR, 0, 1)) fx->maxHpBonus += 10.0f;
    if (HAS(SKILL_HEAVY_ARMOR, 0, 2)) fx->kbTakenMult *= 0.5f;
    if (HAS(SKILL_HEAVY_ARMOR, 1, 0)) fx->defenseK = 120.0f;
    if (HAS(SKILL_HEAVY_ARMOR, 1, 1)) fx->reflectFlat += 3.0f;
    if (HAS(SKILL_HEAVY_ARMOR, 1, 2)) fx->maxHpBonus += 20.0f;
    if (HAS(SKILL_HEAVY_ARMOR, 2, 0)) fx->defMult[ARMOR_HEAVY] *= 1.25f;
    if (HAS(SKILL_HEAVY_ARMOR, 2, 1)) { fx->kbDealtMult *= 1.10f; fx->oneHandDmgMult *= 1.05f; }
    if (HAS(SKILL_HEAVY_ARMOR, 2, 2)) fx->dmgTakenMult *= 0.92f;
    if (HAS(SKILL_HEAVY_ARMOR, 3, 0)) fx->dmgTakenMult *= 0.88f;
    if (HAS(SKILL_HEAVY_ARMOR, 3, 1)) fx->kuleIdleDef = true;
    if (HAS(SKILL_HEAVY_ARMOR, 3, 2)) fx->deathSaveCooldown = 90.0f;
}

// "Yarali Ayi" özel durumu perk.c'de saklanmasın diye main'e sorgu:
// (choice erişimi main'de var; basit yardımcı)
#undef HAS

void perk_pips_render(const Skill *skills,
                      const Sint8 choice[SKILL_COUNT][PERK_TIERS],
                      R2D *renderer, TTF_Font *font,
                      float panelX, float panelY)
{
    (void)font;
    float rowY = panelY + SKILL_PANEL_PAD + SKILL_PANEL_HEADER;

    for (int s = 0; s < SKILL_COUNT; s++, rowY += SKILL_PANEL_ROW_H) {
        if (!perk_skill_has_content((SkillType)s)) continue;

        // 4 pip: XP çubuğunun (60px) solunda, seviye sayısıyla çakışmadan
        float px = panelX + SKILL_PANEL_W - SKILL_PANEL_PAD - 60.0f - 6.0f
                 - PERK_TIERS * 10.0f;
        for (int t = 0; t < PERK_TIERS; t++) {
            SDL_FRect pip = { px + t * 10.0f, rowY + 5.0f, 8.0f, 8.0f };
            if (choice[s][t] != PERK_UNCHOSEN)
                r2d_set_color(renderer, 120, 220, 120, 255);   // seçildi
            else if (skills[s].level >= PERK_THRESHOLDS[t])
                r2d_set_color(renderer, 255, 210, 60, 255);    // SEÇ! (tıkla)
            else
                r2d_set_color(renderer, 70, 70, 85, 255);      // kilitli
            r2d_fill_rect(renderer, &pip);
        }
    }
}

// --- Seçim overlay'i ---
#define CARD_W 260.0f
#define CARD_H 64.0f
#define CARD_GAP 10.0f

void perk_choice_render(SkillType s, int tier, R2D *renderer,
                        TTF_Font *font, float x, float y)
{
    float panelW = CARD_W + 32.0f;
    float panelH = PERK_OPTIONS * (CARD_H + CARD_GAP) + 56.0f;
    ui_panel(renderer, x, y, panelW, panelH);

    if (font == NULL) return;
    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color gray  = { 210, 210, 220, 255 };

    char title[64];
    snprintf(title, sizeof(title), "%s %d — perk sec:",
             skill_name(s), PERK_THRESHOLDS[tier]);
    draw_text(renderer, font, title, x + 16.0f, y + 12.0f, white);

    float cy = y + 40.0f;
    for (int o = 0; o < PERK_OPTIONS; o++) {
        const PerkOption *p = perk_option(s, tier, o);
        if (p == NULL) continue;
        ui_cell(renderer, x + 16.0f, cy, CARD_W, CARD_H);
        draw_text(renderer, font, p->name, x + 26.0f, cy + 8.0f, UI_TEXT_DARK);
        // Açıklama sarılı (kart genişliğine)
        draw_text_wrapped(renderer, font, p->desc, x + 26.0f, cy + 30.0f,
                          CARD_W - 20.0f, UI_TEXT_DARK);
        (void)gray;
        cy += CARD_H + CARD_GAP;
    }
}

int perk_choice_option_at(float mx, float my, float x, float y)
{
    float cy = y + 40.0f;
    for (int o = 0; o < PERK_OPTIONS; o++) {
        if (mx >= x + 16.0f && mx < x + 16.0f + CARD_W &&
            my >= cy && my < cy + CARD_H)
            return o;
        cy += CARD_H + CARD_GAP;
    }
    return -1;
}
