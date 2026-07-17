#include <SDL3/SDL.h>
#include <stdio.h>
#include "skill.h"
#include "font.h"
#include "ui_theme.h"

// Seviye eşiği: 100 × level^1.5 — erken seviyeler hızlı, sonrası yavaşlar.
// Eğri ayarı tek yerden: burayı değiştirmek tüm sistemi ayarlar.
static float xp_threshold(int level)
{
    return 100.0f * SDL_powf((float)level, 1.5f);
}

// Kader Ağacı XP çarpanı — main her ağaç değişiminde yazar (tek nokta köprü)
static float g_xpMult = 1.0f;
void skill_set_xp_mult(float m) { g_xpMult = (m > 0.0f) ? m : 1.0f; }

void skills_init(Skill *skills)
{
    for (int i = 0; i < SKILL_COUNT; i++) {
        skills[i].level = 1;
        skills[i].xp = 0.0f;
        skills[i].xpToNext = xp_threshold(1);
    }
}

bool skill_add_xp(Skill *skills, SkillType t, float xp)
{
    if (t <= SKILL_NONE || t >= SKILL_COUNT || xp <= 0.0f)
        return false;

    Skill *s = &skills[t];
    if (s->level >= SKILL_MAX_LEVEL)
        return false;   // tavan — XP birikmez

    s->xp += xp * g_xpMult;   // Kader: Hızlı Öğrenen / Bilge / AZİM

    // Tek vuruşta çok XP gelirse birden fazla seviye atlanabilir; taşan devreder.
    bool leveled = false;
    while (s->xp >= s->xpToNext && s->level < SKILL_MAX_LEVEL) {
        s->xp -= s->xpToNext;
        s->level++;
        s->xpToNext = xp_threshold(s->level);
        leveled = true;
    }
    if (s->level >= SKILL_MAX_LEVEL)
        s->xp = 0.0f;   // tavanda birikim anlamsız
    return leveled;
}

// YANKICI gizliliği: SKILL_ECHO enum sonunda — görünür satır sayısı
// bayrağa göre 1 eksik (main her kare basar; ritüel öncesi sınıf YOK sayılır)
static bool echoVisible = false;
void skill_set_echo_visible(bool v) { echoVisible = v; }
static int skill_rows(void) { return SKILL_COUNT - (echoVisible ? 0 : 1); }

const char *skill_name(SkillType t)
{
    switch (t) {
        case SKILL_ONE_HANDED:   return "One-Handed";
        case SKILL_TWO_HANDED:   return "Two-Handed";
        case SKILL_ARCHERY:      return "Archery";
        case SKILL_BLOCK:        return "Block";
        case SKILL_LIGHT_ARMOR:  return "Light Armor";
        case SKILL_MEDIUM_ARMOR: return "Medium Armor";
        case SKILL_HEAVY_ARMOR:  return "Heavy Armor";
        case SKILL_SNEAK:        return "Sneak";
        case SKILL_LOCKPICKING:  return "Lockpicking";
        case SKILL_PICKPOCKET:   return "Pickpocket";
        case SKILL_SPEECH:       return "Speech";
        case SKILL_ALCHEMY:      return "Alchemy";
        case SKILL_SMITHING:     return "Smithing";
        case SKILL_ENCHANTING:   return "Enchanting";
        case SKILL_MINING:       return "Mining";
        case SKILL_HERBALISM:    return "Herbalism";
        case SKILL_WOODCUTTING:  return "Woodcutting";
        case SKILL_DESTRUCTION:  return "Destruction";
        case SKILL_STAFF:        return "Staff";
        case SKILL_CONJURATION:  return "Conjuration";
        case SKILL_ECHO:         return "Yankı";
        default:                 return "?";
    }
}

void skill_panel_render(const Skill *skills, R2D *renderer,
                        TTF_Font *font, float panelX, float panelY)
{
    // Envanter paneliyle aynı görsel dil (yerleşim sabitleri skill.h'ta —
    // perk pip'leri ve tıklama testi de aynı ölçüleri kullanır)
    float pad = SKILL_PANEL_PAD;
    float rowH = SKILL_PANEL_ROW_H;
    float panelW = SKILL_PANEL_W;
    float panelH = (skill_rows() + 1) * rowH + pad * 2 + SKILL_PANEL_HEADER + 20.0f;   // +Kader satırı + alt ipucu

    ui_panel(renderer, panelX, panelY, panelW, panelH);

    if (font != NULL) {
        SDL_Color white = { 255, 255, 255, 255 };
        draw_text(renderer, font, "Skills", panelX + pad, panelY + 8.0f, white);
    }

    float rowY = panelY + pad + 30.0f;
    for (int i = 0; i < skill_rows(); i++) {
        const Skill *s = &skills[i];

        if (font != NULL) {
            char buf[48];
            snprintf(buf, sizeof(buf), "%-13s %3d", skill_name((SkillType)i), s->level);
            SDL_Color gray = { 210, 210, 220, 255 };
            draw_text(renderer, font, buf, panelX + pad, rowY, gray);
        }

        // XP çubuğu (satırın sağında): bu seviyedeki ilerleme
        // (60px — solunda perk pip'lerine yer kalır, bkz. perk_pips_render)
        float barW = 60.0f, barH = 8.0f;
        float barX = panelX + panelW - pad - barW;
        float barY = rowY + 5.0f;
        float ratio = (s->xpToNext > 0.0f) ? s->xp / s->xpToNext : 0.0f;

        r2d_set_color(renderer, 50, 50, 65, 255);
        SDL_FRect barBg = { barX, barY, barW, barH };
        r2d_fill_rect(renderer, &barBg);
        r2d_set_color(renderer, 120, 200, 120, 255);
        SDL_FRect barFg = { barX, barY, barW * ratio, barH };
        r2d_fill_rect(renderer, &barFg);

        rowY += rowH;
    }

    // Kader Ağacı satırı (özel — toplam seviyeden puan; row_at SKILL_COUNT döner)
    if (font != NULL) {
        int total = 0;
        for (int i = 0; i < SKILL_COUNT; i++) total += skills[i].level;
        char fbuf[48];
        snprintf(fbuf, sizeof(fbuf), "%-13s %3d", "* Kader", total);
        SDL_Color fate = { 200, 150, 255, 255 };
        draw_text(renderer, font, fbuf, panelX + pad, rowY, fate);
    }
    rowY += rowH;

    // Alt ipucu: skill ağacı nasıl açılır (Faz 21)
    if (font != NULL) {
        SDL_Color hint = { 255, 210, 60, 255 };
        draw_text(renderer, font, "Satıra tıkla: ağacı aç (4 sv = 1p; Kader: 10 toplam sv = 1p)",
                  panelX + pad, rowY + 4.0f, hint);
    }
}

int skill_panel_row_at(float x, float y, float panelX, float panelY)
{
    float gridY = panelY + SKILL_PANEL_PAD + SKILL_PANEL_HEADER;
    if (x < panelX || x > panelX + SKILL_PANEL_W) return -1;
    int row = (int)((y - gridY) / SKILL_PANEL_ROW_H);
    if (y < gridY || row < 0 || row > skill_rows()) return -1;
    if (row == skill_rows()) return SKILL_COUNT;   // Kader satırı (özel — kaysa da anlamı sabit)
    return row;
}
