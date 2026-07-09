#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include "floattext.h"
#include "font.h"

#define FLOAT_SPEED 40.0f   // yukarı kayma hızı (px/sn)
#define FLOAT_LIFE 1.0f     // ömür (saniye)

void floattext_spawn_text(FloatText *texts, int maxTexts, float x, float y,
                          const char *text, SDL_Color color)
{
    // Boş slot bul
    for (int i = 0; i < maxTexts; i++) {
        if (!texts[i].active) {
            texts[i] = (FloatText){
                .x = x, .y = y,
                .color = color,
                .life = FLOAT_LIFE,
                .maxLife = FLOAT_LIFE,
                .active = true
            };
            snprintf(texts[i].text, FLOAT_TEXT_LEN, "%s", text);
            return;
        }
    }
    // Boş slot yoksa: en eskiyi ez (ya da hiç ekleme) — şimdilik atla
}

void floattext_spawn(FloatText *texts, int maxTexts, float x, float y,
                     int amount, bool crit)
{
    char buf[FLOAT_TEXT_LEN];
    snprintf(buf, sizeof(buf), crit ? "%d!" : "%d", amount);

    SDL_Color yellow = { 255, 230, 80, 255 };   // normal hasar
    SDL_Color orange = { 255, 120, 30, 255 };   // kritik — sıcak, dikkat çeker
    floattext_spawn_text(texts, maxTexts, x, y, buf, crit ? orange : yellow);
}

void floattext_update_all(FloatText *texts, int maxTexts, float dt)
{
    for (int i = 0; i < maxTexts; i++) {
        if (!texts[i].active) continue;
        texts[i].y -= FLOAT_SPEED * dt;   // yukarı kay
        texts[i].life -= dt;               // ömür azalt
        if (texts[i].life <= 0.0f)
            texts[i].active = false;       // öldü
    }
}

void floattext_render_all(FloatText *texts, int maxTexts, R2D *renderer,
                          TTF_Font *font, const Camera *camera)
{
    if (font == NULL) return;
    for (int i = 0; i < maxTexts; i++) {
        if (!texts[i].active) continue;

        // Yazıyı ortala (genişliğinin yarısı kadar sola kaydır)
        int textW = 0, textH = 0;
        TTF_GetStringSize(font, texts[i].text, 0, &textW, &textH);

        // Dünya → ekran (zoom dahil)
        float zoom = camera->zoom;
        float sx = (texts[i].x - camera->x) * zoom - textW / 2.0f;   // ortalı (yazi boyutu sabit)
        float sy = (texts[i].y - camera->y) * zoom;

        // Solma: ömür azaldıkça alpha düşür
        float ratio = texts[i].life / texts[i].maxLife;   // 1 → 0
        SDL_Color color = texts[i].color;
        color.a = (Uint8)(255 * ratio);

        draw_text(renderer, font, texts[i].text, sx, sy, color);
    }
}
