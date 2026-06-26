#include "menu.h"

// --- Buton yerlesimi: ekran ortasinda dikey istif ---
// Tek kaynak: hem cizim hem tiklama bu olculeri kullanir (desenkron olmaz).
#define BTN_W   240.0f
#define BTN_H    56.0f
#define BTN_GAP  20.0f

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
static void draw_button(SDL_Renderer *renderer, TTF_Font *font,
                        SDL_FRect r, const char *label)
{
    SDL_SetRenderDrawColor(renderer, 45, 45, 60, 255);
    SDL_RenderFillRect(renderer, &r);
    SDL_SetRenderDrawColor(renderer, 200, 200, 210, 255);
    SDL_RenderRect(renderer, &r);

    if (font != NULL) {
        int tw = 0, th = 0;
        TTF_GetStringSize(font, label, 0, &tw, &th);
        SDL_Color white = { 255, 255, 255, 255 };
        draw_text(renderer, font, label,
                  r.x + (r.w - tw) / 2.0f,
                  r.y + (r.h - th) / 2.0f, white);
    }
}

void menu_render(SDL_Renderer *renderer, TTF_Font *font,
                 MenuScreen screen, int winW, int winH)
{
    // 1) Tum ekrani karart (yari saydam siyah) — oyun arkada donuk durur
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect dim = { 0, 0, (float)winW, (float)winH };
    SDL_RenderFillRect(renderer, &dim);

    // 2) Ekrana gore butonlar
    if (screen == MENU_MAIN) {
        draw_button(renderer, font, menu_button_rect(0, 2, winW, winH), "Devam Et");
        draw_button(renderer, font, menu_button_rect(1, 2, winW, winH), "Oyundan Cik");
    }
    else if (screen == MENU_CONFIRM_QUIT) {
        // Baslik
        if (font != NULL) {
            const char *q = "Emin misin?";
            int tw = 0, th = 0;
            TTF_GetStringSize(font, q, 0, &tw, &th);
            SDL_Color white = { 255, 255, 255, 255 };
            float totalH = 2 * BTN_H + BTN_GAP;
            float startY = (winH - totalH) / 2.0f;
            draw_text(renderer, font, q,
                      (winW - tw) / 2.0f, startY - 50.0f, white);
        }
        draw_button(renderer, font, menu_button_rect(0, 2, winW, winH), "Evet, Cik");
        draw_button(renderer, font, menu_button_rect(1, 2, winW, winH), "Hayir, Geri");
    }
}

MenuAction menu_handle_click(MenuScreen *screen, float mx, float my, int winW, int winH)
{
    if (*screen == MENU_MAIN) {
        SDL_FRect resume = menu_button_rect(0, 2, winW, winH);
        SDL_FRect quit   = menu_button_rect(1, 2, winW, winH);
        if (mx >= resume.x && mx < resume.x + resume.w &&
            my >= resume.y && my < resume.y + resume.h)
            return MENU_ACTION_RESUME;
        if (mx >= quit.x && mx < quit.x + quit.w &&
            my >= quit.y && my < quit.y + quit.h) {
            *screen = MENU_CONFIRM_QUIT;   // onay ekranina gec
            return MENU_ACTION_NONE;
        }
    }
    else if (*screen == MENU_CONFIRM_QUIT) {
        SDL_FRect yes = menu_button_rect(0, 2, winW, winH);
        SDL_FRect no  = menu_button_rect(1, 2, winW, winH);
        if (mx >= yes.x && mx < yes.x + yes.w &&
            my >= yes.y && my < yes.y + yes.h)
            return MENU_ACTION_QUIT;
        if (mx >= no.x && mx < no.x + no.w &&
            my >= no.y && my < no.y + no.h) {
            *screen = MENU_MAIN;   // ana menuye geri don
            return MENU_ACTION_NONE;
        }
    }
    return MENU_ACTION_NONE;
}