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

// Tek bir fontu kapat (yüklenen her font için bir kez).
void font_close(TTF_Font *font);

// Font sistemini kapat (TTF_Quit) — TÜM fontlar kapandıktan sonra, BİR kez.
// (Eski font_quit hem fontu hem sistemi kapatıyordu; iki font olunca
//  ikinci çağrı ölü FreeType'a dokunup çöküyordu.)
void font_shutdown(void);

#endif