#include <SDL3/SDL.h>
#include <stdio.h>
#include "floattext.h"
#include "font.h"

#define FLOAT_SPEED 40.0f   // yukarı kayma hızı (px/sn)
#define FLOAT_LIFE 1.0f     // ömür (saniye)

void floattext_spawn(FloatText *texts, int maxTexts, float x, float y, int amount)
{
    // Boş slot bul
    for (int i = 0; i < maxTexts; i++) {
        if (!texts[i].active) {
            texts[i] = (FloatText){
                .x = x, .y = y,
                .amount = amount,
                .life = FLOAT_LIFE,
                .maxLife = FLOAT_LIFE,
                .active = true
            };
            return;
        }
    }
    // Boş slot yoksa: en eskiyi ez (ya da hiç ekleme) — şimdilik atla
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

void floattext_render_all(FloatText *texts, int maxTexts, SDL_Renderer *renderer,
                          TTF_Font *font, const Camera *camera)
{
    if (font == NULL) return;
    for (int i = 0; i < maxTexts; i++) {
        if (!texts[i].active) continue;

        // Sayıyı metne çevir
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", texts[i].amount);

        // Yazıyı ortala (genişliğinin yarısı kadar sola kaydır)
        int textW = 0, textH = 0;
        TTF_GetStringSize(font, buf, 0, &textW, &textH);

        // Dünya → ekran
        float sx = texts[i].x - camera->x - textW / 2.0f;   // ortalı
        float sy = texts[i].y - camera->y;

        // Solma: ömür azaldıkça alpha düşür
        float ratio = texts[i].life / texts[i].maxLife;   // 1 → 0
        Uint8 alpha = (Uint8)(255 * ratio);
        SDL_Color color = { 255, 230, 80, alpha };   // sarımsı (hasar)

        draw_text(renderer, font, buf, sx, sy, color);
    }
}