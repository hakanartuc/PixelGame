#ifndef FLOATTEXT_H
#define FLOATTEXT_H
#include "r2d.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "common.h"

#define MAX_FLOAT_TEXTS 32
#define FLOAT_TEXT_LEN 32   // serbest metin kapasitesi ("One-Handed 5!" sığar)

typedef struct {
    float x, y;        // dünya konumu
    char text[FLOAT_TEXT_LEN];   // gösterilecek metin (sayılar da metne çevrilir)
    SDL_Color color;   // metin rengi (alpha solmayla ezilir)
    float life;        // kalan ömür (saniye)
    float maxLife;     // toplam ömür (solma hesabı için)
    bool active;
} FloatText;

// Hasar sayısı için uçan yazı (sarı). crit: farklı renk + "!" eki.
void floattext_spawn(FloatText *texts, int maxTexts, float x, float y,
                     int amount, bool crit);

// Serbest metinli uçan yazı (level-up bildirimi vb.), renk çağırandan.
void floattext_spawn_text(FloatText *texts, int maxTexts, float x, float y,
                          const char *text, SDL_Color color);

// Tüm yazıları güncelle (yukarı kaydır, ömür azalt).
void floattext_update_all(FloatText *texts, int maxTexts, float dt);

// Tüm aktif yazıları çiz.
void floattext_render_all(FloatText *texts, int maxTexts, R2D *renderer,
                          TTF_Font *font, const Camera *camera);

#endif
