#include "assassin.h"
#include "uilayout.h"
#include "mob.h"
#include "font.h"
#include "ui_theme.h"
#include <stdio.h>

// K1 kademesi kontratları (Suikastçi P3): hedefler mevcut dünya mobları —
// yeni tür GEREKMEZ (MOB_TYPES tavanı 23/24 dolu). Kaçak Tahsildar (21)
// kontrat aktifken gece kervan yolunda belirir (main spawn kancası).
const AssassinContract ASSASSIN_CONTRACTS[] = {
    { "Dikenli Mesele",
      "Goblinler kervan yolunu kesiyor. Loca yol açık kalsın ister — "
      "üçünü sustur. Kim, neden diye sorma; yol para demek.",
      0, 3, 120, 1, 0,
      "Goblin kampı — Yeşil Ova batısı (köyün GD yolu)" },
    { "Reisin Sonu",
      "Bir goblin kampı fazla büyüdü. Reisi düşerse gerisi dağılır — "
      "başı kes, gövde çöksün.",
      MOB_GOBLIN_CHIEF, 1, 220, 2, 0,
      "Goblin Reisi — kampın kalbinde (gece uykuda yakala)" },
    { "Totem Devrilir",
      "Ork totemi bir hizbin gözü kulağı. Kör et onları — totemi yık, "
      "sesleri kesilsin.",
      MOB_ORC_TOTEM, 1, 260, 2, 1,
      "Ork Savaş Totemi — ork kampı ortası (yapı, kaçmaz)" },
    { "Kaçak Yine Kaçak",
      "Tahsildar öldü sanıldı, bir başkası adını aldı. Aynı hikaye, "
      "aynı son. Gece kervan yolunda bulursun.",
      21 /*MOB_TAX_RUNNER*/, 1, 420, 3, 2,
      "Kaçak Tahsildar — GECE kervan yolunda (doğu); 2 koruması var" },
    { "Mezar Sessizliği",
      "Bir mezarlık fazla gürültülü. Üç iskeleti tekrar yatır — "
      "loca sessizliği sever.",
      12 /*İskelet Savaşçı*/, 3, 380, 3, 3,
      "İskelet Savaşçı — Lanetli Mezarlık (kervan yolu geçidinden)" },
};
const int ASSASSIN_CONTRACT_COUNT =
    (int)(sizeof(ASSASSIN_CONTRACTS) / sizeof(ASSASSIN_CONTRACTS[0]));

// Rütbe eşikleri (kümülatif puan)
static const int RANK_THRESH[5] = { 0, 3, 7, 13, 20 };
static const char *RANK_NAME[5] =
    { "Çırak", "El", "Bıçak", "Gölge", "Usta El" };

int assassin_rank(int points)
{
    int r = 0;
    for (int i = 4; i >= 0; i--)
        if (points >= RANK_THRESH[i]) { r = i; break; }
    return r;
}
const char *assassin_rank_name(int rank)
{
    if (rank < 0) rank = 0;
    if (rank > 4) rank = 4;
    return RANK_NAME[rank];
}
int assassin_next_threshold(int rank)
{
    if (rank >= 4) return -1;
    return RANK_THRESH[rank + 1];
}

#define BOARD_W 480.0f
#define ROW_H 62.0f
#define ROW_STEP (ROW_H + 14.0f)

// Panonun konumu + genişliği (yerleşim: "suikast_panosu"). Çizim ve
// assassin_board_row_at aynı çağrıdan geçiyor — pano taşındığında kontrat
// satırı tıklaması beraber taşınsın. px/py artık VARSAYILAN değer.
static SDL_FRect ab_rect(float px, float py)
{
    return uilayout_panel("suikast_panosu", px, py, BOARD_W);
}

void assassin_board_render(int playerPoints, int active, int progress,
                           bool ready, R2D *renderer, TTF_Font *font,
                           float px, float py)
{
    int rank = assassin_rank(playerPoints);
    int shown = 0;
    for (int i = 0; i < ASSASSIN_CONTRACT_COUNT; i++)
        if (ASSASSIN_CONTRACTS[i].minRank <= rank) shown++;

    float panelH = 96.0f + (float)shown * (ROW_H + 14.0f) + 40.0f;
    SDL_FRect pr = ab_rect(px, py);
    px = pr.x; py = pr.y;   // gerisi bu ikisinden türüyor
    const float boardW = pr.w;
    ui_panel(renderer, px, py, boardW, panelH);
    if (font == NULL) return;

    SDL_Color gold = { 255, 210, 60, 255 };
    SDL_Color white = { 232, 230, 222, 255 };
    SDL_Color dim = { 140, 140, 155, 255 };
    SDL_Color grn = { 150, 230, 150, 255 };

    draw_text(renderer, font, "SESSİZ EL — Kontrat Tahtası", px + 16.0f,
              py + 12.0f, gold);
    char rbuf[64];
    int nxt = assassin_next_threshold(rank);
    if (nxt >= 0)
        snprintf(rbuf, sizeof(rbuf), "Rütbe: %s   (%d / %d puan)",
                 assassin_rank_name(rank), playerPoints, nxt);
    else
        snprintf(rbuf, sizeof(rbuf), "Rütbe: %s   (en yüksek)",
                 assassin_rank_name(rank));
    draw_text(renderer, font, rbuf, px + 16.0f, py + 36.0f, white);

    float rowY = py + 70.0f;
    for (int i = 0; i < ASSASSIN_CONTRACT_COUNT; i++) {
        const AssassinContract *c = &ASSASSIN_CONTRACTS[i];
        if (c->minRank > rank) continue;
        ui_cell_gray(renderer, px + 10.0f, rowY, boardW - 20.0f, ROW_STEP - 8.0f);

        bool isActive = (active == i);
        SDL_Color tc = isActive ? gold : white;
        draw_text(renderer, font, c->name, px + 20.0f, rowY + 6.0f, tc);

        char meta[80];
        if (isActive && ready)
            snprintf(meta, sizeof(meta), "TAMAM — [Ödülü al]  +%d altın",
                     c->rewardGold);
        else if (isActive)
            snprintf(meta, sizeof(meta), "Etkin: %d / %d  ·  +%d altın",
                     progress, c->count, c->rewardGold);
        else
            snprintf(meta, sizeof(meta), "+%d altın · %d puan",
                     c->rewardGold, c->rankPoints);
        draw_text(renderer, font, meta, px + 20.0f, rowY + 24.0f,
                  (isActive && ready) ? grn : dim);
        // Konum ipucu (küçük, gri) — ne yapılacağı okunur olsun
        draw_text(renderer, font, c->where, px + 20.0f, rowY + 42.0f,
                  (SDL_Color){ 120, 120, 135, 255 });
        rowY += ROW_STEP;
    }

    draw_text(renderer, font,
              active >= 0 ? "Etkin kontratı tamamla ya da tıkla=vazgeç"
                          : "Bir kontrata tıkla = üstlen",
              px + 16.0f, rowY + 4.0f, dim);
}

int assassin_board_row_at(int playerPoints, int active, bool ready,
                          float x, float y, float px, float py)
{
    (void)active; (void)ready;
    int rank = assassin_rank(playerPoints);
    SDL_FRect pr = ab_rect(px, py);
    px = pr.x; py = pr.y;
    if (x < px + 10.0f || x > px + pr.w - 10.0f) return -1;
    float rowY = py + 70.0f;
    for (int i = 0; i < ASSASSIN_CONTRACT_COUNT; i++) {
        if (ASSASSIN_CONTRACTS[i].minRank > rank) continue;
        if (y >= rowY && y < rowY + ROW_STEP - 8.0f) return i;
        rowY += ROW_STEP;
    }
    return -1;
}
