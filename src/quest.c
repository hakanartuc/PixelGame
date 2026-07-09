#include <stdio.h>
#include <string.h>
#include "quest.h"
#include "font.h"
#include "ui_theme.h"

// Görev tanımları — içerik büyüdükçe buraya satır eklenir (ITEM_TYPES deseni)
const QuestDef QUESTS[] = {
    { .id = "goblin_derdi", .name = "Goblin Derdi",
      .targetMob = &MOB_TYPES[0], .targetCount = 5, .rewardGold = 100 },
};
const int QUEST_COUNT = (int)(sizeof(QUESTS) / sizeof(QUESTS[0]));

int quest_find(const char *id)
{
    for (int i = 0; i < QUEST_COUNT; i++)
        if (strcmp(QUESTS[i].id, id) == 0)
            return i;
    return -1;
}

void quest_log_render(const QuestStatus *status, R2D *renderer,
                      TTF_Font *font, float panelX, float panelY)
{
    // Panel dili skill/envanterle aynı (renkler oradan)
    float pad = 16.0f;
    float rowH = 58.0f;
    float panelW = 340.0f;
    float panelH = QUEST_COUNT * rowH + pad * 2 + 30.0f;

    ui_panel(renderer, panelX, panelY, panelW, panelH);

    if (font == NULL) return;

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color gray  = { 150, 150, 160, 255 };
    SDL_Color cyan  = { 90, 220, 255, 255 };
    SDL_Color green = { 120, 255, 120, 255 };
    SDL_Color gold  = { 255, 210, 60, 255 };

    draw_text(renderer, font, "Gorevler", panelX + pad, panelY + 8.0f, white);

    float rowY = panelY + pad + 30.0f;
    for (int i = 0; i < QUEST_COUNT; i++) {
        const QuestDef *q = &QUESTS[i];
        const QuestStatus *st = &status[i];

        // Ad + durum etiketi
        SDL_Color nameCol = (st->state == QUEST_NOT_STARTED) ? gray : white;
        draw_text(renderer, font, q->name, panelX + pad, rowY, nameCol);

        const char *stateTxt =
            (st->state == QUEST_ACTIVE)   ? "Aktif" :
            (st->state == QUEST_DONE)     ? "Teslim et!" :
            (st->state == QUEST_REWARDED) ? "Tamam" : "Alinmadi";
        SDL_Color stateCol =
            (st->state == QUEST_DONE)     ? green :
            (st->state == QUEST_ACTIVE)   ? cyan : gray;
        int tw = 0, th = 0;
        TTF_GetStringSize(font, stateTxt, 0, &tw, &th);
        draw_text(renderer, font, stateTxt, panelX + panelW - pad - tw, rowY, stateCol);

        // İlerleme çubuğu + sayı + ödül
        float barW = panelW - pad * 2 - 90.0f, barH = 8.0f;
        float barY = rowY + 24.0f;
        float ratio = (st->state == QUEST_NOT_STARTED) ? 0.0f
                    : (float)st->progress / (float)q->targetCount;
        if (st->state == QUEST_REWARDED || st->state == QUEST_DONE) ratio = 1.0f;

        r2d_set_color(renderer, 50, 50, 65, 255);
        SDL_FRect barBg = { panelX + pad, barY, barW, barH };
        r2d_fill_rect(renderer, &barBg);
        r2d_set_color(renderer, 120, 200, 120, 255);
        SDL_FRect barFg = { panelX + pad, barY, barW * ratio, barH };
        r2d_fill_rect(renderer, &barFg);

        char info[48];
        int shown = (st->progress > q->targetCount) ? q->targetCount : st->progress;
        snprintf(info, sizeof(info), "%d/%d  %dg", shown, q->targetCount, q->rewardGold);
        draw_text(renderer, font, info, panelX + pad + barW + 8.0f, barY - 4.0f, gold);

        rowY += rowH;
    }
}

int quest_on_kill(QuestStatus *status, const MobType *killed)
{
    int completed = -1;
    for (int i = 0; i < QUEST_COUNT; i++) {
        if (status[i].state != QUEST_ACTIVE) continue;
        if (QUESTS[i].targetMob != killed) continue;

        status[i].progress++;
        if (status[i].progress >= QUESTS[i].targetCount) {
            status[i].state = QUEST_DONE;
            completed = i;
        }
    }
    return completed;
}
