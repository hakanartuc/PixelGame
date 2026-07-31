#include <stdio.h>
#include "contract.h"
#include "uilayout.h"
#include "worldevent.h"
#include "ecology.h"   // dinamik ilan: nüfussuz türe kontrat yok (E2)
#include "region.h"    // söylenti: koloninin bölge adı
#include "item.h"
#include "font.h"
#include "ui_theme.h"

// Kademe eşikleri + adları: itibar kastıkça daha büyük işler açılır
const float CONTRACT_TIER_REP[CONTRACT_TIERS] = { -25.0f, 10.0f, 25.0f };
const char *const CONTRACT_TIER_NAMES[CONTRACT_TIERS] = { "Temel", "Güvenilir", "Saygın" };

// Havuz: kademe başına 3 ilan (sıra ÖNEMLİ: tier alanı doğru olmalı).
// Satır eklemek = yeni çeşit; CONTRACT_PER_TIER'i de büyütmeyi unutma.
const ContractDef CONTRACTS[] = {
    // --- Temel (herkese; itibar > -25) ---
    { .name = "Goblin sorunu",   .tier = 0, .targetMob = &MOB_TYPES[0],
      .fetchItem = -1, .count = 5, .gold = 60, .rep = 2.0f },
    { .name = "Deri siparişi",   .tier = 0, .targetMob = NULL,
      .fetchItem = ITEM_LEATHER, .count = 5, .gold = 55, .rep = 2.0f },
    { .name = "Kasabın işi",     .tier = 0, .targetMob = NULL,
      .fetchItem = ITEM_MEAT, .count = 4, .gold = 40, .rep = 2.0f },

    // --- Güvenilir (itibar >= 10) ---
    { .name = "Ork tehdidi",     .tier = 1, .targetMob = &MOB_TYPES[1],
      .fetchItem = -1, .count = 8, .gold = 150, .rep = 3.0f },
    { .name = "Usta derici",     .tier = 1, .targetMob = NULL,
      .fetchItem = ITEM_LEATHER, .count = 10, .gold = 120, .rep = 3.0f },
    { .name = "Yaban avı",       .tier = 1, .targetMob = &MOB_TYPES[5],
      .fetchItem = -1, .count = 2, .gold = 80, .rep = 3.0f },

    // --- Saygın (itibar >= 25) ---
    { .name = "Geçit temizliği", .tier = 2, .targetMob = &MOB_TYPES[1],
      .fetchItem = -1, .count = 12, .gold = 260, .rep = 5.0f },
    { .name = "Elit av",         .tier = 2, .targetMob = &MOB_TYPES[5],
      .fetchItem = -1, .count = 4, .gold = 180, .rep = 4.0f },
    { .name = "Öz toplayıcısı",  .tier = 2, .targetMob = NULL,
      .fetchItem = ITEM_ESSENCE_PALE, .count = 8, .gold = 220, .rep = 5.0f },
};
const int CONTRACT_COUNT = (int)(sizeof(CONTRACTS) / sizeof(CONTRACTS[0]));

// --- Modül durumu: günün seçimi + slot durumları ---
static int curDay = -1;
static ContractState states[CONTRACT_ACTIVE];
static int progress[CONTRACT_ACTIVE];

void contract_new_day(int day)
{
    curDay = day;
    for (int s = 0; s < CONTRACT_ACTIVE; s++) {
        states[s] = CONTRACT_AVAILABLE;
        progress[s] = 0;
    }
}

// Günün seçimi: kademe başına 1 ilan, gün indeksiyle döner (deterministik —
// save'de yalnız gün tutulur, seçim yeniden türetilir)
int contract_today(int slot)
{
    if (slot < 0 || slot >= CONTRACT_ACTIVE || curDay < 0) return -1;
    int within = (curDay + slot) % CONTRACT_PER_TIER;   // slot kaydırması: aynı gün

    // GEÇERLİ ilanlar: kes-kontratının hedef türü dünyada yaşıyor olmalı (E2).
    // within geçerliler arasında sayılır; hiç geçerli yoksa slot boş kalır.
    int valid[16], nValid = 0;   // CONTRACT_COUNT const int — sabit tavan (VLA olmasın)
    for (int i = 0; i < CONTRACT_COUNT; i++) {
        if (CONTRACTS[i].tier != slot) continue;        // slot = kademe
        if (CONTRACTS[i].targetMob != NULL &&
            ecology_type_pop(CONTRACTS[i].targetMob) <= 0) continue;
        if (nValid < 16) valid[nValid++] = i;
    }
    if (nValid == 0) return -1;   // "Bugün ilan yok"
    return valid[within % nValid];
}

ContractState contract_state(int slot)
{
    return (slot >= 0 && slot < CONTRACT_ACTIVE) ? states[slot] : CONTRACT_DONE;
}

int contract_progress(int slot)
{
    return (slot >= 0 && slot < CONTRACT_ACTIVE) ? progress[slot] : 0;
}

void contract_accept(int slot)
{
    if (slot >= 0 && slot < CONTRACT_ACTIVE && states[slot] == CONTRACT_AVAILABLE)
        states[slot] = CONTRACT_ACCEPTED;
}

void contract_turn_in(int slot)
{
    if (slot >= 0 && slot < CONTRACT_ACTIVE && states[slot] == CONTRACT_ACCEPTED)
        states[slot] = CONTRACT_DONE;
}

void contract_on_kill(const MobType *type)
{
    for (int s = 0; s < CONTRACT_ACTIVE; s++) {
        if (states[s] != CONTRACT_ACCEPTED) continue;
        int ci = contract_today(s);
        if (ci < 0 || CONTRACTS[ci].targetMob != type) continue;
        if (progress[s] < CONTRACTS[ci].count)
            progress[s]++;
    }
}

// --- Save köprüsü (v12) ---
int contract_save_day(void) { return curDay; }

void contract_save_get(int slot, int *state, int *prog)
{
    *state = (int)states[slot];
    *prog = progress[slot];
}

void contract_save_set(int day, const int *st, const int *prog)
{
    curDay = day;
    for (int s = 0; s < CONTRACT_ACTIVE; s++) {
        states[s] = (st[s] >= 0 && st[s] <= CONTRACT_DONE)
                  ? (ContractState)st[s] : CONTRACT_AVAILABLE;
        progress[s] = prog[s];
    }
}

// --- Panel: başlık + kademe başına 1 satır (2 satır metin) + ipucu ---
#define CT_ROW_H 58.0f
#define CT_PAD 16.0f
#define CT_W 520.0f

// Panelin konumu + genişliği (yerleşim: "ilan_panosu"). Çizim ve
// contract_panel_slot_at aynı çağrıdan geçiyor: panel taşındığında satır
// tıklaması beraber taşınsın. panelX/panelY artık VARSAYILAN değer.
static SDL_FRect ct_rect(float panelX, float panelY)
{
    return uilayout_panel("ilan_panosu", panelX, panelY, CT_W);
}

void contract_panel_render(R2D *renderer, TTF_Font *font, const Inventory *inv,
                           const Inventory *inv2,
                           float rep, float panelX, float panelY)
{
    float panelH = CONTRACT_ACTIVE * CT_ROW_H + CT_PAD * 2 + 56.0f + 64.0f;   // +söylentiler
    SDL_FRect pr = ct_rect(panelX, panelY);
    panelX = pr.x; panelY = pr.y;   // gerisi bu ikisinden türüyor
    const float ctW = pr.w;
    ui_panel(renderer, panelX, panelY, ctW, panelH);
    if (font == NULL) return;

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color gold  = { 255, 210, 60, 255 };
    SDL_Color dim   = { 140, 140, 150, 255 };
    SDL_Color green = { 150, 255, 150, 255 };
    SDL_Color cyan  = { 90, 220, 255, 255 };

    char title[48];
    snprintf(title, sizeof(title), "İlan Panosu — Gün %d", curDay);
    draw_text(renderer, font, title, panelX + CT_PAD, panelY + 8.0f, white);

    // pr yukarıda çözüldü; panelX/panelY zaten ondan geliyor
    float rowY = panelY + CT_PAD + 30.0f;
    for (int s = 0; s < CONTRACT_ACTIVE; s++) {
        SDL_FRect row = { panelX + 8.0f, rowY, ctW - 16.0f, CT_ROW_H - 6.0f };
        int ci = contract_today(s);
        if (ci < 0) {
            ui_cell_gray(renderer, row.x, row.y, row.w, row.h);
            draw_text(renderer, font, "Bugün ilan yok", row.x + 10.0f, rowY + 12.0f, dim);
            rowY += CT_ROW_H;
            continue;
        }
        const ContractDef *c = &CONTRACTS[ci];
        bool locked = rep < CONTRACT_TIER_REP[c->tier];

        if (states[s] == CONTRACT_ACCEPTED) ui_cell_hi(renderer, row.x, row.y, row.w, row.h);
        else                                ui_cell_gray(renderer, row.x, row.y, row.w, row.h);

        char line[112];
        if (locked) {
            snprintf(line, sizeof(line), "[%s] ???", CONTRACT_TIER_NAMES[c->tier]);
            draw_text(renderer, font, line, row.x + 12.0f, rowY + 6.0f, dim);
            snprintf(line, sizeof(line), "İtibar %d gerekli",
                     (int)CONTRACT_TIER_REP[c->tier]);
            draw_text(renderer, font, line, row.x + 12.0f, rowY + 28.0f, dim);
            rowY += CT_ROW_H;
            continue;
        }

        // Başlık satırı: kademe + ad + ödül
        snprintf(line, sizeof(line), "[%s] %s — %dg +%d itibar",
                 CONTRACT_TIER_NAMES[c->tier], c->name, c->gold, (int)c->rep);
        draw_text(renderer, font, line, row.x + 12.0f, rowY + 6.0f,
                  states[s] == CONTRACT_DONE ? dim : white);

        // Durum satırı: hedef + ilerleme + eylem ipucu
        if (states[s] == CONTRACT_DONE) {
            draw_text(renderer, font, "Teslim edildi — yarın yenisi gelir",
                      row.x + 12.0f, rowY + 28.0f, dim);
        }
        else if (c->targetMob != NULL) {
            int cur = (states[s] == CONTRACT_ACCEPTED) ? progress[s] : 0;
            snprintf(line, sizeof(line), "%s x%d  (%d/%d)%s",
                     c->targetMob->name, c->count, cur, c->count,
                     states[s] == CONTRACT_AVAILABLE ? "   > kabul et"
                     : (cur >= c->count ? "   > TESLİM ET" : ""));
            draw_text(renderer, font, line, row.x + 10.0f, rowY + 28.0f,
                      (states[s] == CONTRACT_ACCEPTED && cur >= c->count) ? green : cyan);
        }
        else {
            int have = inventory_count(inv, &ITEM_TYPES[c->fetchItem]) +
                       inventory_count(inv2, &ITEM_TYPES[c->fetchItem]);
            snprintf(line, sizeof(line), "%dx %s getir  (%d/%d)%s",
                     c->count, ITEM_TYPES[c->fetchItem].name,
                     have > c->count ? c->count : have, c->count,
                     states[s] == CONTRACT_AVAILABLE ? "   > kabul et"
                     : (have >= c->count ? "   > TESLİM ET" : ""));
            draw_text(renderer, font, line, row.x + 12.0f, rowY + 28.0f,
                      (states[s] == CONTRACT_ACCEPTED && have >= c->count) ? green : cyan);
        }
        rowY += CT_ROW_H;
    }

    // SÖYLENTİLER (E2 haber ağı): son 7 günün kolonileri panoya düşer
    {
        SDL_Color rumor = { 230, 200, 150, 255 };
        draw_text(renderer, font, "~ Söylentiler ~", panelX + CT_PAD,
                  rowY + 4.0f, gold);
        // Günün olayı en üste düşer (Dünya Olayı — worldevent)
        int rrow = 0;
        if (worldevent_current() != WE_NONE) {
            draw_text(renderer, font, worldevent_rumor(worldevent_current()),
                      panelX + CT_PAD, rowY + 24.0f, rumor);
            rrow = 1;
        }
        int sites[3];
        int n = ecology_recent_colonies(curDay, sites, 3 - rrow);
        if (n == 0 && rrow == 0)
            draw_text(renderer, font, "Ortalık sakin... şimdilik.",
                      panelX + CT_PAD, rowY + 24.0f, dim);
        for (int r = 0; r < n; r++) {
            const NestSiteDef *ns = ecology_site(sites[r]);
            int reg = region_at(ns->x, ns->y);
            char line[112];
            snprintf(line, sizeof(line), "%s %s taraflarına yerleşmiş.",
                     ecology_family_name(ns->family),
                     reg >= 0 ? REGIONS[reg].name : "bilinmez");
            draw_text(renderer, font, line, panelX + CT_PAD,
                      rowY + 24.0f + (r + rrow) * 20.0f, rumor);
        }
    }
    rowY += 64.0f;

    SDL_Color hint = { 170, 200, 255, 255 };
    draw_text(renderer, font, "Kabul + teslim: satıra tıkla   ESC: kapat",
              panelX + CT_PAD, rowY + 6.0f, hint);
}

int contract_panel_slot_at(float x, float y, float panelX, float panelY)
{
    SDL_FRect pr = ct_rect(panelX, panelY);
    float rowY = pr.y + CT_PAD + 30.0f;
    for (int s = 0; s < CONTRACT_ACTIVE; s++) {
        SDL_FRect row = { pr.x + 8.0f, rowY, pr.w - 16.0f, CT_ROW_H - 6.0f };
        if (x >= row.x && x < row.x + row.w && y >= row.y && y < row.y + row.h)
            return s;
        rowY += CT_ROW_H;
    }
    return -1;
}
