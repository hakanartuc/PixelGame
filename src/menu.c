#include <stdio.h>
#include "menu.h"
#include "ui_theme.h"
#include "worldmap.h"   // slot kartinda konum adi (MAPS[].name)

// --- Buton yerlesimi: ekran ortasinda dikey istif ---
// Tek kaynak: hem cizim hem tiklama bu olculeri kullanir (desenkron olmaz).
#define BTN_W   240.0f
#define BTN_H    56.0f
#define BTN_GAP  20.0f

// --- Slot izgarasi: 4 sutun x 5 satir kart + ustte otomatik kayit (yalniz yukle) ---
#define SLOT_COLS   4
#define SLOT_CARD_W 232.0f
#define SLOT_CARD_H 64.0f
#define SLOT_GAP    10.0f

// Dolu slota YENI OYUN onayi icin bekleyen slot (ekran gecisiyle tasinir)
static int pendingSlot = -1;
static float shownMusicVol = 1.0f, shownSfxVol = 1.0f;   // ayarlar ekrani degerleri

void menu_set_volumes(float musicVol, float sfxVol)
{
    shownMusicVol = musicVol;
    shownSfxVol = sfxVol;
}

// Kaydirici yerlesimi (cizim + tiklama ayni kaynak)
static SDL_FRect slider_rect(int i, int winW, int winH)
{
    SDL_FRect r = { (winW - 360.0f) / 2.0f,
                    winH / 2.0f - 60.0f + i * 90.0f, 360.0f, 14.0f };
    return r;
}

// i. butonun dikdortgenini hesaplar (ekran ortasi, dikey istif).
// count: o ekrandaki toplam buton sayisi (dikey ortalamak icin).
static SDL_FRect menu_button_rect(int i, int count, int winW, int winH)
{
    float totalH = count * BTN_H + (count - 1) * BTN_GAP;
    float startY = (winH - totalH) / 2.0f;
    SDL_FRect r = {
        (winW - BTN_W) / 2.0f,
        startY + i * (BTN_H + BTN_GAP),
        BTN_W, BTN_H
    };
    return r;
}

// Tek buton ciz (kutu + ortali metin).
static void draw_button(R2D *renderer, TTF_Font *font,
                        SDL_FRect r, const char *label)
{
    ui_button(renderer, r.x, r.y, r.w, r.h);   // perçinli ten buton (tema)

    if (font != NULL) {
        int tw = 0, th = 0;
        TTF_GetStringSize(font, label, 0, &tw, &th);
        draw_text(renderer, font, label,
                  r.x + (r.w - tw) / 2.0f,
                  r.y + (r.h - th) / 2.0f, UI_TEXT_DARK);   // açık zeminde koyu
    }
}

// Ekran ustune ortali baslik satiri
static void draw_screen_title(R2D *renderer, TTF_Font *font,
                              const char *s, float y, int winW)
{
    if (font == NULL) return;
    int tw = 0, th = 0;
    TTF_GetStringSize(font, s, 0, &tw, &th);
    SDL_Color white = { 255, 255, 255, 255 };
    draw_text(renderer, font, s, (winW - tw) / 2.0f, y, white);
}

// --- Slot izgarasi yerlesimi (cizim + tiklama ayni kaynak) ---
// idx 0..SAVE_SLOT_COUNT-1: manuel kartlar. withAuto ise izgaranin ustunde
// genis otomatik-kayit karti var (idx SAVE_SLOT_AUTO).
static SDL_FRect slot_card_rect(int idx, bool withAuto, int winW, int winH)
{
    float gridW = SLOT_COLS * SLOT_CARD_W + (SLOT_COLS - 1) * SLOT_GAP;
    int rows = SAVE_SLOT_COUNT / SLOT_COLS;
    float gridH = rows * SLOT_CARD_H + (rows - 1) * SLOT_GAP;
    float autoH = withAuto ? SLOT_CARD_H + SLOT_GAP : 0.0f;
    float startX = (winW - gridW) / 2.0f;
    float startY = (winH - gridH - autoH) / 2.0f + 14.0f;   // basliga yer birak

    if (idx == SAVE_SLOT_AUTO) {
        SDL_FRect r = { startX, startY, gridW, SLOT_CARD_H };
        return r;
    }
    SDL_FRect r = {
        startX + (idx % SLOT_COLS) * (SLOT_CARD_W + SLOT_GAP),
        startY + autoH + (idx / SLOT_COLS) * (SLOT_CARD_H + SLOT_GAP),
        SLOT_CARD_W, SLOT_CARD_H
    };
    return r;
}

// "Geri" butonu: izgaranin altinda ortali
static SDL_FRect slot_back_rect(int winW, int winH)
{
    SDL_FRect r = { (winW - BTN_W) / 2.0f, winH - BTN_H - 24.0f, BTN_W, BTN_H };
    return r;
}

// Tek slot karti: baslik + ozet (dolu) ya da "— Bos —"
static void draw_slot_card(R2D *renderer, TTF_Font *font, SDL_FRect r,
                           const char *title, const SaveSummary *s)
{
    // kart zemini: dolu = ten (tema butonu), bos = koyu
    r2d_set_blend(renderer, R2D_BLEND_ALPHA);
    if (s != NULL && s->used)
        ui_button(renderer, r.x, r.y, r.w, r.h);
    else {
        r2d_set_color(renderer, 30, 28, 34, 230);
        r2d_fill_rect(renderer, &r);
        r2d_set_color(renderer, 70, 66, 78, 255);
        SDL_FRect line = { r.x, r.y, r.w, 2.0f };
        r2d_fill_rect(renderer, &line);
    }
    if (font == NULL) return;

    SDL_Color dim = { 150, 145, 160, 255 };
    if (s != NULL && s->used) {
        char info[96];
        const char *loc = (s->mapId >= 0 && s->mapId < MAP_COUNT)
                        ? MAPS[s->mapId].name : "?";
        draw_text(renderer, font, title, r.x + 8.0f, r.y + 6.0f, UI_TEXT_DARK);
        SDL_snprintf(info, sizeof(info), "Sv %d G%d %s", s->totalLevel, s->day, loc);
        draw_text(renderer, font, info, r.x + 8.0f, r.y + 34.0f, UI_TEXT_DARK);
    } else {
        draw_text(renderer, font, title, r.x + 8.0f, r.y + 6.0f, dim);
        draw_text(renderer, font, "- Boş -", r.x + 8.0f, r.y + 34.0f, dim);
    }
}

// Slot ekranini ciz (baslik + kartlar + geri)
static void render_slots(R2D *renderer, TTF_Font *font, MenuScreen screen,
                         int winW, int winH, const SaveSummary *sums)
{
    const char *title = (screen == MENU_SLOTS_NEW)  ? "Yeni oyun — slot seç"
                      : (screen == MENU_SLOTS_SAVE) ? "Kaydet — slot seç"
                                                    : "Yükle — slot seç";
    draw_screen_title(renderer, font, title, 26.0f, winW);

    bool withAuto = (screen == MENU_SLOTS_LOAD);
    if (withAuto) {
        SDL_FRect ar = slot_card_rect(SAVE_SLOT_AUTO, true, winW, winH);
        draw_slot_card(renderer, font, ar, "Otomatik Kayıt",
                       sums != NULL ? &sums[SAVE_SLOT_AUTO] : NULL);
    }
    for (int i = 0; i < SAVE_SLOT_COUNT; i++) {
        char t[24];
        SDL_snprintf(t, sizeof(t), "Slot %d", i + 1);
        SDL_FRect r = slot_card_rect(i, withAuto, winW, winH);
        draw_slot_card(renderer, font, r, t,
                       sums != NULL ? &sums[i] : NULL);
    }
    draw_button(renderer, font, slot_back_rect(winW, winH), "Geri");
}

void menu_render(R2D *renderer, TTF_Font *font,
                 MenuScreen screen, int winW, int winH,
                 const SaveSummary *sums)
{
    // 1) Tum ekrani karart — ESC kokenli ekranlarda yari saydam (oyun arkada
    //    donuk durur), ana menu ekranlarinda tam koyu (arkada dunya yok)
    bool inGameOverlay = (screen == MENU_MAIN || screen == MENU_CONFIRM_QUIT ||
                          screen == MENU_CONFIRM_MAINMENU ||
                          screen == MENU_SLOTS_SAVE);
    r2d_set_blend(renderer, R2D_BLEND_ALPHA);
    r2d_set_color(renderer, 8, 6, 12, inGameOverlay ? 180 : 255);
    SDL_FRect dim = { 0, 0, (float)winW, (float)winH };
    r2d_fill_rect(renderer, &dim);

    // 2) Ekrana gore icerik
    if (screen == MENU_MAIN) {
        draw_button(renderer, font, menu_button_rect(0, 6, winW, winH), "Devam Et");
        draw_button(renderer, font, menu_button_rect(1, 6, winW, winH), "Kaydet");
        draw_button(renderer, font, menu_button_rect(2, 6, winW, winH), "Yükle");
        draw_button(renderer, font, menu_button_rect(3, 6, winW, winH), "Ayarlar");
        draw_button(renderer, font, menu_button_rect(4, 6, winW, winH), "Ana Menüye Dön");
        draw_button(renderer, font, menu_button_rect(5, 6, winW, winH), "Oyundan Çık");
    }
    else if (screen == MENU_ROOT) {
        draw_button(renderer, font, menu_button_rect(0, 4, winW, winH), "Yeni Oyun");
        draw_button(renderer, font, menu_button_rect(1, 4, winW, winH), "Yükle");
        draw_button(renderer, font, menu_button_rect(2, 4, winW, winH), "Ayarlar");
        draw_button(renderer, font, menu_button_rect(3, 4, winW, winH), "Çıkış");
    }
    else if (screen == MENU_SETTINGS) {
        draw_screen_title(renderer, font, "Ayarlar", winH / 2.0f - 130.0f, winW);
        const char *labels[2] = { "Müzik", "Ses Efektleri" };
        float vals[2] = { shownMusicVol, shownSfxVol };
        for (int i = 0; i < 2; i++) {
            SDL_FRect r = slider_rect(i, winW, winH);
            SDL_Color white = { 255, 255, 255, 255 };
            char lb[48];
            SDL_snprintf(lb, sizeof(lb), "%s: %%%d", labels[i],
                         (int)(vals[i] * 100.0f + 0.5f));
            draw_text(renderer, font, lb, r.x, r.y - 26.0f, white);
            r2d_set_color(renderer, 40, 38, 48, 255);
            r2d_fill_rect(renderer, &r);
            r2d_set_color(renderer, 210, 170, 90, 255);
            SDL_FRect fill = { r.x, r.y, r.w * vals[i], r.h };
            r2d_fill_rect(renderer, &fill);
            r2d_set_color(renderer, 235, 225, 205, 255);
            SDL_FRect knob = { r.x + r.w * vals[i] - 4.0f, r.y - 4.0f, 8.0f, r.h + 8.0f };
            r2d_fill_rect(renderer, &knob);
        }
        draw_button(renderer, font, slot_back_rect(winW, winH), "Geri");
    }
    else if (screen == MENU_CONFIRM_QUIT || screen == MENU_CONFIRM_MAINMENU ||
             screen == MENU_CONFIRM_OVERWRITE) {
        const char *q = (screen == MENU_CONFIRM_QUIT)
                          ? "Emin misin?"
                      : (screen == MENU_CONFIRM_MAINMENU)
                          ? "Kaydedilmemiş ilerleme kaybolur. Emin misin?"
                          : "Bu slotta kayıt var. Üzerine başlansın mı?";
        float totalH = 2 * BTN_H + BTN_GAP;
        float startY = (winH - totalH) / 2.0f;
        int tw = 0, th = 0;
        if (font != NULL) TTF_GetStringSize(font, q, 0, &tw, &th);
        SDL_Color white = { 255, 255, 255, 255 };
        draw_text(renderer, font, q, (winW - tw) / 2.0f, startY - 50.0f, white);
        draw_button(renderer, font, menu_button_rect(0, 2, winW, winH), "Evet");
        draw_button(renderer, font, menu_button_rect(1, 2, winW, winH), "Hayır");
    }
    else {   // slot ekranlari
        render_slots(renderer, font, screen, winW, winH, sums);
    }
}

static bool hit(SDL_FRect r, float mx, float my)
{
    return mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h;
}

MenuAction menu_handle_click(MenuScreen *screen, float mx, float my,
                             int winW, int winH,
                             const SaveSummary *sums, int *outSlot,
                             float *musicVol, float *sfxVol)
{
    if (*screen == MENU_MAIN) {
        if (hit(menu_button_rect(0, 6, winW, winH), mx, my))
            return MENU_ACTION_RESUME;
        if (hit(menu_button_rect(1, 6, winW, winH), mx, my)) {
            *screen = MENU_SLOTS_SAVE;
            return MENU_ACTION_NONE;
        }
        if (hit(menu_button_rect(2, 6, winW, winH), mx, my)) {
            *screen = MENU_SLOTS_LOAD;
            return MENU_ACTION_NONE;
        }
        if (hit(menu_button_rect(3, 6, winW, winH), mx, my)) {
            *screen = MENU_SETTINGS;
            return MENU_ACTION_NONE;
        }
        if (hit(menu_button_rect(4, 6, winW, winH), mx, my)) {
            *screen = MENU_CONFIRM_MAINMENU;
            return MENU_ACTION_NONE;
        }
        if (hit(menu_button_rect(5, 6, winW, winH), mx, my)) {
            *screen = MENU_CONFIRM_QUIT;
            return MENU_ACTION_NONE;
        }
    }
    else if (*screen == MENU_ROOT) {
        if (hit(menu_button_rect(0, 4, winW, winH), mx, my)) {
            *screen = MENU_SLOTS_NEW;
            return MENU_ACTION_NONE;
        }
        if (hit(menu_button_rect(1, 4, winW, winH), mx, my)) {
            *screen = MENU_SLOTS_LOAD;
            return MENU_ACTION_NONE;
        }
        if (hit(menu_button_rect(2, 4, winW, winH), mx, my)) {
            *screen = MENU_SETTINGS;
            return MENU_ACTION_NONE;
        }
        if (hit(menu_button_rect(3, 4, winW, winH), mx, my))
            return MENU_ACTION_QUIT;
    }
    else if (*screen == MENU_SETTINGS) {
        // Kaydırıcılar: bar üstüne (dikeyde biraz toleranslı) tıkla = değer
        for (int i = 0; i < 2; i++) {
            SDL_FRect r = slider_rect(i, winW, winH);
            SDL_FRect wide = { r.x - 6, r.y - 12, r.w + 12, r.h + 24 };
            if (!hit(wide, mx, my)) continue;
            float v = (mx - r.x) / r.w;
            if (v < 0.0f) v = 0.0f;
            if (v > 1.0f) v = 1.0f;
            if (i == 0) { *musicVol = v; shownMusicVol = v; }
            else        { *sfxVol = v;  shownSfxVol = v; }
            return MENU_ACTION_VOLUME;
        }
        if (hit(slot_back_rect(winW, winH), mx, my)) {
            *screen = MENU_ROOT;   // ESC kökenliyse main MAIN'e çevirir
            return MENU_ACTION_NONE;
        }
    }
    else if (*screen == MENU_CONFIRM_QUIT) {
        if (hit(menu_button_rect(0, 2, winW, winH), mx, my))
            return MENU_ACTION_QUIT;
        if (hit(menu_button_rect(1, 2, winW, winH), mx, my)) {
            *screen = MENU_MAIN;
            return MENU_ACTION_NONE;
        }
    }
    else if (*screen == MENU_CONFIRM_MAINMENU) {
        if (hit(menu_button_rect(0, 2, winW, winH), mx, my))
            return MENU_ACTION_TO_MAINMENU;
        if (hit(menu_button_rect(1, 2, winW, winH), mx, my)) {
            *screen = MENU_MAIN;
            return MENU_ACTION_NONE;
        }
    }
    else if (*screen == MENU_CONFIRM_OVERWRITE) {
        if (hit(menu_button_rect(0, 2, winW, winH), mx, my)) {
            *outSlot = pendingSlot;
            pendingSlot = -1;
            return MENU_ACTION_NEW_GAME;
        }
        if (hit(menu_button_rect(1, 2, winW, winH), mx, my)) {
            pendingSlot = -1;
            *screen = MENU_SLOTS_NEW;
            return MENU_ACTION_NONE;
        }
    }
    else {   // slot ekranlari
        MenuScreen scr = *screen;
        bool withAuto = (scr == MENU_SLOTS_LOAD);

        if (hit(slot_back_rect(winW, winH), mx, my)) {
            // Geri: SAVE ekrani yalniz ESC'den acilir → MAIN'e; NEW yalniz ana
            // menuden → ROOT'a. LOAD iki yerden de acilabilir: dogru donus
            // ekranini main bilir (gameState) — ROOT'a doneriz, ESC kokenli
            // LOAD'da main bunu MAIN'e cevirir (bkz. main ESC handling).
            *screen = (scr == MENU_SLOTS_SAVE) ? MENU_MAIN : MENU_ROOT;
            return MENU_ACTION_NONE;
        }

        if (withAuto && sums != NULL && sums[SAVE_SLOT_AUTO].used &&
            hit(slot_card_rect(SAVE_SLOT_AUTO, true, winW, winH), mx, my)) {
            *outSlot = SAVE_SLOT_AUTO;
            return MENU_ACTION_LOAD_SLOT;
        }

        for (int i = 0; i < SAVE_SLOT_COUNT; i++) {
            if (!hit(slot_card_rect(i, withAuto, winW, winH), mx, my))
                continue;
            bool used = sums != NULL && sums[i].used;
            if (scr == MENU_SLOTS_SAVE) {
                *outSlot = i;
                return MENU_ACTION_SAVE_SLOT;   // dolu slota yazmak normal akis
            }
            if (scr == MENU_SLOTS_LOAD) {
                if (!used) return MENU_ACTION_NONE;   // bos slot yuklenemez
                *outSlot = i;
                return MENU_ACTION_LOAD_SLOT;
            }
            // MENU_SLOTS_NEW: dolu slota onay sor, bosa direkt basla
            if (used) {
                pendingSlot = i;
                *screen = MENU_CONFIRM_OVERWRITE;
                return MENU_ACTION_NONE;
            }
            *outSlot = i;
            return MENU_ACTION_NEW_GAME;
        }
    }
    return MENU_ACTION_NONE;
}
