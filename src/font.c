#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "font.h"

// TTF yüzeyini r2d texture'ına çevirir (RGBA32'ye normalize edip yükler).
// Her karede oluşturup yıkmak basit ama yeterli — r2d yıkımı kare sonuna erteler.
static R2DTexture *texture_from_surface(R2D *renderer, SDL_Surface *surface)
{
    SDL_Surface *rgba = surface;
    if (surface->format != SDL_PIXELFORMAT_RGBA32) {
        rgba = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        if (rgba == NULL) return NULL;
    }

    R2DTexture *tex;
    if (rgba->pitch == rgba->w * 4) {
        tex = r2d_texture_from_pixels(renderer, rgba->pixels, rgba->w, rgba->h);
    } else {
        // pitch hizalıysa satır satır sıkıştır (GPU upload bitişik bekler)
        Uint8 *packed = SDL_malloc((size_t)rgba->w * rgba->h * 4);
        for (int row = 0; row < rgba->h; row++)
            memcpy(packed + (size_t)row * rgba->w * 4,
                   (Uint8 *)rgba->pixels + (size_t)row * rgba->pitch,
                   (size_t)rgba->w * 4);
        tex = r2d_texture_from_pixels(renderer, packed, rgba->w, rgba->h);
        SDL_free(packed);
    }

    if (rgba != surface) SDL_DestroySurface(rgba);
    return tex;
}

bool font_init(void)
{
    if (!TTF_Init()) {
        SDL_Log("TTF_Init basarisiz: %s", SDL_GetError());
        return false;
    }
    return true;
}

TTF_Font *font_load(const char *path, float size)
{
    TTF_Font *font = TTF_OpenFont(path, size);
    if (font == NULL) {
        SDL_Log("Font yuklenemedi (%s): %s", path, SDL_GetError());
        return NULL;
    }
    return font;
}

void draw_text(R2D *renderer, TTF_Font *font, const char *text,
               float x, float y, SDL_Color color)
{
    if (font == NULL || text == NULL || text[0] == '\0') return;

    // 1) Metni bir yüzeye (surface) render et
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, 0, color);
    if (surface == NULL) {
        SDL_Log("Metin render basarisiz: %s", SDL_GetError());
        return;
    }

    // 2) Yüzeyi texture'a çevir (GPU'da çizilebilsin)
    R2DTexture *texture = texture_from_surface(renderer, surface);
    if (texture == NULL) {
        SDL_DestroySurface(surface);
        return;
    }

    // 3) Texture'ı ekrana çiz
    SDL_FRect dst = { x, y, (float)surface->w, (float)surface->h };
    r2d_draw_texture(renderer, texture, NULL, &dst);

    // 4) Temizlik (her karede oluşturulup yıkılıyor — basit ama yeterli)
    r2d_texture_destroy(NULL, texture);
    SDL_DestroySurface(surface);
}

float draw_text_wrapped(R2D *renderer, TTF_Font *font, const char *text,
                        float x, float y, float wrapW, SDL_Color color)
{
    if (font == NULL || text == NULL || text[0] == '\0') return 0.0f;

    // draw_text ile aynı akış; tek fark _Wrapped render (satır sonlarını TTF sarar)
    SDL_Surface *surface = TTF_RenderText_Blended_Wrapped(font, text, 0, color, (int)wrapW);
    if (surface == NULL) {
        SDL_Log("Metin render basarisiz: %s", SDL_GetError());
        return 0.0f;
    }

    R2DTexture *texture = texture_from_surface(renderer, surface);
    if (texture == NULL) {
        SDL_DestroySurface(surface);
        return 0.0f;
    }

    SDL_FRect dst = { x, y, (float)surface->w, (float)surface->h };
    r2d_draw_texture(renderer, texture, NULL, &dst);

    float h = (float)surface->h;
    r2d_texture_destroy(NULL, texture);
    SDL_DestroySurface(surface);
    return h;
}

void font_close(TTF_Font *font)
{
    if (font) TTF_CloseFont(font);
}

void font_shutdown(void)
{
    TTF_Quit();
}