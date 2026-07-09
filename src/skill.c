#include <SDL3/SDL.h>
#include <stdio.h>
#include "skill.h"
#include "font.h"

// Seviye eşiği: 100 × level^1.5 — erken seviyeler hızlı, sonrası yavaşlar.
// Eğri ayarı tek yerden: burayı değiştirmek tüm sistemi ayarlar.
static float xp_threshold(int level)
{
    return 100.0f * SDL_powf((float)level, 1.5f);
}

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

    s->xp += xp;

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
        default:                 return "?";
    }
}

void skill_panel_render(const Skill *skills, SDL_Renderer *renderer,
                        TTF_Font *font, float panelX, float panelY)
{
    // Envanter paneliyle aynı görsel dil (renkler/pad inventory_render'dan)
    float pad = 16.0f;
    float rowH = 26.0f;
    float panelW = 360.0f;
    float panelH = SKILL_COUNT * rowH + pad * 2 + 30.0f;   // +30 başlık

    SDL_SetRenderDrawColor(renderer, 25, 25, 35, 240);
    SDL_FRect panel = { panelX, panelY, panelW, panelH };
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 200, 200, 210, 255);
    SDL_RenderRect(renderer, &panel);

    if (font != NULL) {
        SDL_Color white = { 255, 255, 255, 255 };
        draw_text(renderer, font, "Skills", panelX + pad, panelY + 8.0f, white);
    }

    float rowY = panelY + pad + 30.0f;
    for (int i = 0; i < SKILL_COUNT; i++) {
        const Skill *s = &skills[i];

        if (font != NULL) {
            char buf[48];
            snprintf(buf, sizeof(buf), "%-13s %3d", skill_name((SkillType)i), s->level);
            SDL_Color gray = { 210, 210, 220, 255 };
            draw_text(renderer, font, buf, panelX + pad, rowY, gray);
        }

        // XP çubuğu (satırın sağında): bu seviyedeki ilerleme
        float barW = 80.0f, barH = 8.0f;
        float barX = panelX + panelW - pad - barW;
        float barY = rowY + 5.0f;
        float ratio = (s->xpToNext > 0.0f) ? s->xp / s->xpToNext : 0.0f;

        SDL_SetRenderDrawColor(renderer, 50, 50, 65, 255);
        SDL_FRect barBg = { barX, barY, barW, barH };
        SDL_RenderFillRect(renderer, &barBg);
        SDL_SetRenderDrawColor(renderer, 120, 200, 120, 255);
        SDL_FRect barFg = { barX, barY, barW * ratio, barH };
        SDL_RenderFillRect(renderer, &barFg);

        rowY += rowH;
    }
}
