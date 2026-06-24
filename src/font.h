#ifndef FONT_H
#define FONT_H
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

// Font sistemini başlat (TTF_Init). Başarılıysa true.
bool font_init(void);

// Bir font dosyası yükle (belli boyutta). Başarısızsa NULL.
TTF_Font *font_load(const char *path, float size);

// Metni ekrana çiz (sol-üst köşe x,y konumunda, verilen renkte).
void draw_text(SDL_Renderer *renderer, TTF_Font *font, const char *text,
               float x, float y, SDL_Color color);

// Temizlik
void font_quit(TTF_Font *font);

#endif