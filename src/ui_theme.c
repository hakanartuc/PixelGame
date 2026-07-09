#include "ui_theme.h"
#include "render.h"   // load_texture

// UI_Frames.png üzerindeki kaynak kareler (piksel ölçümle çıkarıldı)
static const SDL_FRect SRC_PANEL    = { 915, 4,  42, 41 };   // ahşap çerçeve
static const SDL_FRect SRC_BUTTON   = { 1112, 8, 32, 32 };   // perçinli ten panel
static const SDL_FRect SRC_CELL     = { 10, 10,  28, 31 };   // ten slot
static const SDL_FRect SRC_CELL_GRY = { 10, 58,  28, 31 };   // gri slot (pasif)
static const SDL_FRect SRC_CELL_HI  = { 10, 202, 28, 31 };   // turuncu slot (vurgu)

static R2DTexture *uiTex = NULL;

bool ui_theme_init(R2D *renderer)
{
    uiTex = load_texture(renderer, "assets/ui/UI_Frames.png");
    if (uiTex == NULL) {
        SDL_Log("UI temasi yuklenemedi — duz renk cizime dusulur");
        return false;
    }
    // (r2d varsayılanı NEAREST — ayrıca ayar gerekmez)
    return true;
}

void ui_theme_quit(void)
{
    if (uiTex) r2d_texture_destroy(NULL, uiTex);
    uiTex = NULL;
}

// Ortak 9-grid çizimi; tema yoksa eski düz-renk görünüme düşer (oyun ölmez).
// scale: köşenin ekrandaki büyütmesi (panel 1x = ince kenar, hücre 2x = tombul)
static void draw9(R2D *renderer, const SDL_FRect *src, float corner,
                  float scale, float x, float y, float w, float h)
{
    SDL_FRect dst = { x, y, w, h };
    if (uiTex == NULL) {
        r2d_set_blend(renderer, R2D_BLEND_ALPHA);
        r2d_set_color(renderer, 25, 25, 35, 240);
        r2d_fill_rect(renderer, &dst);
        r2d_set_color(renderer, 200, 200, 210, 255);
        r2d_rect(renderer, &dst);
        return;
    }
    r2d_draw_9grid(renderer, uiTex, src, corner, scale, &dst);
}

void ui_panel(R2D *renderer, float x, float y, float w, float h)
{
    // Köşe 14: ahşap barlar kaynakta ~14px — daha küçük köşe barları
    // "esneyen orta banda" düşürüp paneli yayıyordu (diyalog altı taşması).
    // Ölçek 1x: kenar 14px kalır, panel pad'leri (16) içeriği bardan kurtarır.
    draw9(renderer, &SRC_PANEL, 14.0f, 1.0f, x, y, w, h);
}

void ui_button(R2D *renderer, float x, float y, float w, float h)
{
    draw9(renderer, &SRC_BUTTON, 10.0f, 2.0f, x, y, w, h);
}

void ui_cell(R2D *renderer, float x, float y, float w, float h)
{
    draw9(renderer, &SRC_CELL, 8.0f, 2.0f, x, y, w, h);
}

void ui_cell_gray(R2D *renderer, float x, float y, float w, float h)
{
    draw9(renderer, &SRC_CELL_GRY, 8.0f, 2.0f, x, y, w, h);
}

void ui_cell_hi(R2D *renderer, float x, float y, float w, float h)
{
    draw9(renderer, &SRC_CELL_HI, 8.0f, 2.0f, x, y, w, h);
}
