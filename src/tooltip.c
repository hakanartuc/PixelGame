#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "tooltip.h"
#include "font.h"
#include "ui_theme.h"

// Satır havuzu: metin + renk (kutu boyu satırlardan hesaplanır)
#define TT_MAX_LINES 14
#define TT_LINE_LEN 96

typedef struct {
    char text[TT_LINE_LEN];
    SDL_Color color;
} TtLine;

static const SDL_Color TT_WHITE = { 255, 255, 255, 255 };
static const SDL_Color TT_DIM   = { 165, 160, 175, 255 };
static const SDL_Color TT_GOLD  = { 255, 210, 60, 255 };
static const SDL_Color TT_CYAN  = { 90, 220, 255, 255 };
static const SDL_Color TT_PINK  = { 235, 140, 220, 255 };
static const SDL_Color TT_GREEN = { 120, 255, 120, 255 };
static const SDL_Color TT_RED   = { 255, 110, 100, 255 };

static void tt_add(TtLine *lines, int *n, SDL_Color c, const char *fmt, ...)
{
    if (*n >= TT_MAX_LINES) return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(lines[*n].text, TT_LINE_LEN, fmt, ap);
    va_end(ap);
    lines[*n].color = c;
    (*n)++;
}

// Kuşanılıyla fark satırı: pozitif yeşil, negatif kırmızı (0 yazılmaz)
static void tt_delta(TtLine *lines, int *n, const char *label, float delta)
{
    if (delta > 0.05f)
        tt_add(lines, n, TT_GREEN, "  ^ %s +%.0f (kuşanılıya göre)", label, delta);
    else if (delta < -0.05f)
        tt_add(lines, n, TT_RED, "  v %s %.0f (kuşanılıya göre)", label, delta);
}

// Tür etiketi: oyuncunun dilinden tek kelime
static const char *tt_kind(const ItemType *t)
{
    if (t->weaponType != WEAPON_NONE) return "Silah";
    if (t->armorClass == ARMOR_LIGHT)  return "Hafif Zırh";
    if (t->armorClass == ARMOR_MEDIUM) return "Orta Zırh";
    if (t->armorClass == ARMOR_HEAVY)  return "Ağır Zırh";
    if (t->equipable && t->defense > 0.0f) return "Koruma";
    if (t->scrollXp > 0.0f)   return "Parşömen";
    if (t->foodRegen > 0.0f)  return "Yemek";
    if (t->consumable)        return "Tüketilebilir";
    if (t->trapItem)          return "Tuzak";
    if (t->equipable)         return "Araç";
    return "Malzeme";
}

void tooltip_render(R2D *renderer, TTF_Font *font,
                    const ItemStack *s, const ItemStack *equipped,
                    float mx, float my, int winW, int winH)
{
    if (font == NULL || s == NULL || s->type == NULL) return;
    const ItemType *t = s->type;

    TtLine lines[TT_MAX_LINES];
    int n = 0;

    // Başlık: ad (+plus) — plus'lı isim altın
    if (s->plusLevel > 0)
        tt_add(lines, &n, TT_GOLD, "%s +%d", t->name, s->plusLevel);
    else
        tt_add(lines, &n, TT_WHITE, "%s", t->name);
    tt_add(lines, &n, TT_DIM, "%s", tt_kind(t));

    // Silah/zırh değerleri (instance: plus dahil) + kuşanılıyla fark
    if (t->weaponType != WEAPON_NONE) {
        tt_add(lines, &n, TT_WHITE, "Hasar: %.0f", item_damage(s));
        if (equipped != NULL && equipped->type != NULL)
            tt_delta(lines, &n, "hasar", item_damage(s) - item_damage(equipped));
        if (t->attackInterval > 0.0f)
            tt_add(lines, &n, TT_DIM, "Vuruş aralığı: %.1f sn", t->attackInterval);
        if (t->lifesteal > 0.0f)
            tt_add(lines, &n, TT_PINK, "Can çalma: %%%.0f", t->lifesteal * 100.0f);
        if (t->spellPower > 0.0f)
            tt_add(lines, &n, TT_CYAN, "Büyü gücü: +%%%.0f", t->spellPower * 100.0f);
    }
    else if (t->defense > 0.0f) {
        tt_add(lines, &n, TT_WHITE, "Defans: %.0f", item_defense(s));
        if (equipped != NULL && equipped->type != NULL)
            tt_delta(lines, &n, "defans", item_defense(s) - item_defense(equipped));
    }

    // Rün + stat slotları (instance)
    {
        static const char *fxName[] = { NULL, "Pembe Rün (+%8 hasar)",
                                        "Alev Rünü (yakar)", "Buz Rünü (yavaşlatır)",
                                        "Zehir Rünü (zehirler)" };
        if (s->fx > 0 && s->fx < 5 && fxName[s->fx] != NULL)
            tt_add(lines, &n, TT_PINK, "%s", fxName[s->fx]);
        for (int k = 0; k < 2; k++)
            if (s->statType[k] >= 0 && s->statType[k] < STAT_COUNT)
                tt_add(lines, &n, TT_CYAN, "+%d %s (kademe %d)",
                       STAT_TIER_VALUE[s->statType[k]] * s->statTier[k],
                       STAT_NAMES[s->statType[k]], s->statTier[k]);
    }

    // Tüketim etkileri
    if (t->healAmount > 0.0f)  tt_add(lines, &n, TT_GREEN, "İçince: +%.0f can", t->healAmount);
    if (t->manaAmount > 0.0f)  tt_add(lines, &n, TT_CYAN, "İçince: +%.0f mana", t->manaAmount);
    if (t->buffSpeedMult > 1.0f)
        tt_add(lines, &n, TT_CYAN, "%.0f sn +%%%.0f hız", t->buffDuration,
               (t->buffSpeedMult - 1.0f) * 100.0f);
    if (t->coatDuration > 0.0f)
        tt_add(lines, &n, TT_PINK, "Silaha sürülür (%.0f sn kaplama)", t->coatDuration);
    if (t->foodRegen > 0.0f)
        tt_add(lines, &n, TT_GREEN, "Yenir: %.0f sn boyunca +%.0f can/sn",
               t->foodDuration, t->foodRegen);
    if (t->scrollXp > 0.0f) tt_add(lines, &n, TT_CYAN, "Okunur: +%.0f XP", t->scrollXp);
    if (t->useCooldown > 0.0f)
        tt_add(lines, &n, TT_DIM, "Bekleme: %.0f sn", t->useCooldown);

    // Değer + adet
    tt_add(lines, &n, TT_GOLD, "Değer: %dg (satış %dg)",
           item_value(s), item_value(s) / 2);
    if (s->count > 1) tt_add(lines, &n, TT_DIM, "Adet: %d", s->count);

    // --- Kutu yerleşimi: imlecin sağ-altı; taşarsa çevir ---
    float pad = 10.0f, lineH = 20.0f;
    float w = 0.0f;
    for (int i = 0; i < n; i++) {
        int tw = 0, th = 0;
        TTF_GetStringSize(font, lines[i].text, 0, &tw, &th);
        if ((float)tw > w) w = (float)tw;
    }
    w += pad * 2;
    float h = n * lineH + pad * 2;
    float x = mx + 18.0f, y = my + 18.0f;
    if (x + w > winW) x = mx - w - 6.0f;
    if (y + h > winH) y = (float)winH - h - 4.0f;
    if (x < 0.0f) x = 0.0f;
    if (y < 0.0f) y = 0.0f;

    r2d_set_blend(renderer, R2D_BLEND_ALPHA);
    r2d_set_color(renderer, 16, 14, 22, 240);
    r2d_fill_rect(renderer, &(SDL_FRect){ x, y, w, h });
    r2d_set_color(renderer, 120, 110, 140, 255);
    r2d_rect(renderer, &(SDL_FRect){ x, y, w, h });

    for (int i = 0; i < n; i++)
        draw_text(renderer, font, lines[i].text,
                  x + pad, y + pad + i * lineH, lines[i].color);
}
