#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "font.h"

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

void draw_text(SDL_Renderer *renderer, TTF_Font *font, const char *text,
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
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == NULL) {
        SDL_DestroySurface(surface);
        return;
    }

    // 3) Texture'ı ekrana çiz
    SDL_FRect dst = { x, y, (float)surface->w, (float)surface->h };
    SDL_RenderTexture(renderer, texture, NULL, &dst);

    // 4) Temizlik (her karede oluşturulup yıkılıyor — basit ama yeterli)
    SDL_DestroyTexture(texture);
    SDL_DestroySurface(surface);
}

void font_quit(TTF_Font *font)
{
    if (font) TTF_CloseFont(font);
    TTF_Quit();
}