#ifndef UI_THEME_H
#define UI_THEME_H
#include <SDL3/SDL.h>
#include <stdbool.h>

// UI teması — Cute_Fantasy_UI sheet'inden 9-grid çizim yardımcıları.
// Tüm paneller/hücreler/butonlar TEK yerden stillenir: sprite değişince
// oyun genelinde görünüm değişir, modüller renk kodu taşımaz.
//
// 9-grid: kaynak karenin köşeleri sabit kalır, kenar/orta esner —
// 42px'lik ahşap çerçeve her boyutta panel olur (SDL_RenderTexture9Grid).

bool ui_theme_init(SDL_Renderer *renderer);   // assets/ui/UI_Frames.png yükler
void ui_theme_quit(void);

// Büyük panel: ahşap çerçeve, koyu iç (beyaz metin okunur)
void ui_panel(SDL_Renderer *renderer, float x, float y, float w, float h);

// Buton: perçinli açık-ten kare (metin KOYU olmalı — bkz. UI_TEXT_DARK)
void ui_button(SDL_Renderer *renderer, float x, float y, float w, float h);

// Item/skill hücresi: ten rengi slot; _gray pasif, _hi vurgulu (turuncu)
void ui_cell(SDL_Renderer *renderer, float x, float y, float w, float h);
void ui_cell_gray(SDL_Renderer *renderer, float x, float y, float w, float h);
void ui_cell_hi(SDL_Renderer *renderer, float x, float y, float w, float h);

// Buton üstü metin rengi (açık zeminde koyu kahve)
#define UI_TEXT_DARK (SDL_Color){ 62, 39, 25, 255 }

#endif
