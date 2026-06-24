#ifndef FLOATTEXT_H
#define FLOATTEXT_H
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "common.h"

#define MAX_FLOAT_TEXTS 32

typedef struct {
    float x, y;        // dünya konumu
    int amount;        // gösterilecek sayı
    float life;        // kalan ömür (saniye)
    float maxLife;     // toplam ömür (solma hesabı için)
    bool active;
} FloatText;

// Yeni bir uçan yazı oluştur (dünya konumunda).
void floattext_spawn(FloatText *texts, int maxTexts, float x, float y, int amount);

// Tüm yazıları güncelle (yukarı kaydır, ömür azalt).
void floattext_update_all(FloatText *texts, int maxTexts, float dt);

// Tüm aktif yazıları çiz.
void floattext_render_all(FloatText *texts, int maxTexts, SDL_Renderer *renderer,
                          TTF_Font *font, const Camera *camera);

#endif