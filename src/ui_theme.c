#include "ui_theme.h"
#include "render.h"   // load_texture

// UI_Frames.png üzerindeki kaynak kareler (piksel ölçümle çıkarıldı)
static const SDL_FRect SRC_PANEL    = { 915, 4,  42, 41 };   // ahşap çerçeve
static const SDL_FRect SRC_BUTTON   = { 1112, 8, 32, 32 };   // perçinli ten panel
static const SDL_FRect SRC_CELL     = { 10, 10,  28, 31 };   // ten slot
static const SDL_FRect SRC_CELL_GRY = { 10, 58,  28, 31 };   // gri slot (pasif)
static const SDL_FRect SRC_CELL_HI  = { 10, 202, 28, 31 };   // turuncu slot (vurgu)

static SDL_Texture *uiTex = NULL;

bool ui_theme_init(SDL_Renderer *renderer)
{
    uiTex = load_texture(renderer, "assets/ui/UI_Frames.png");
    if (uiTex == NULL) {
        SDL_Log("UI temasi yuklenemedi — duz renk cizime dusulur");
        return false;
    }
    SDL_SetTextureScaleMode(uiTex, SDL_SCALEMODE_NEAREST);   // pixel-art keskin
    return true;
}

void ui_theme_quit(void)
{
    if (uiTex) SDL_DestroyTexture(uiTex);
    uiTex = NULL;
}

// Ortak 9-grid çizimi; tema yoksa eski düz-renk görünüme düşer (oyun ölmez).
// scale: köşenin ekrandaki büyütmesi (panel 1x = ince kenar, hücre 2x = tombul)
static void draw9(SDL_Renderer *renderer, const SDL_FRect *src, float corner,
                  float scale, float x, float y, float w, float h)
{
    SDL_FRect dst = { x, y, w, h };
    if (uiTex == NULL) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 25, 25, 35, 240);
        SDL_RenderFillRect(renderer, &dst);
        SDL_SetRenderDrawColor(renderer, 200, 200, 210, 255);
        SDL_RenderRect(renderer, &dst);
        return;
    }
    SDL_RenderTexture9Grid(renderer, uiTex, src, corner, corner, corner, corner,
                           scale, &dst);
}

void ui_panel(SDL_Renderer *renderer, float x, float y, float w, float h)
{
    // Köşe 14: ahşap barlar kaynakta ~14px — daha küçük köşe barları
    // "esneyen orta banda" düşürüp paneli yayıyordu (diyalog altı taşması).
    // Ölçek 1x: kenar 14px kalır, panel pad'leri (16) içeriği bardan kurtarır.
    draw9(renderer, &SRC_PANEL, 14.0f, 1.0f, x, y, w, h);
}

void ui_button(SDL_Renderer *renderer, float x, float y, float w, float h)
{
    draw9(renderer, &SRC_BUTTON, 10.0f, 2.0f, x, y, w, h);
}

void ui_cell(SDL_Renderer *renderer, float x, float y, float w, float h)
{
    draw9(renderer, &SRC_CELL, 8.0f, 2.0f, x, y, w, h);
}

void ui_cell_gray(SDL_Renderer *renderer, float x, float y, float w, float h)
{
    draw9(renderer, &SRC_CELL_GRY, 8.0f, 2.0f, x, y, w, h);
}

void ui_cell_hi(SDL_Renderer *renderer, float x, float y, float w, float h)
{
    draw9(renderer, &SRC_CELL_HI, 8.0f, 2.0f, x, y, w, h);
}
