#include "decal.h"
#include <SDL3/SDL.h>

static Decal s_decals[MAX_DECALS];
static R2DTexture *s_tex = NULL;

// Düzensiz kenarlı radyal leke. Kusursuz daire yapay duruyor (ışık halkalarında
// öğrenilen ders), o yüzden yarıçap açıya göre üç sinüsle modüle ediliyor —
// üç farklı frekans, tekrar deseni gözle yakalanmasın.
void decal_init(R2D *renderer)
{
    decal_reset();
    if (s_tex != NULL) return;
    const int W = 64, H = 64;
    static unsigned char px[64 * 64 * 4];
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float nx = ((float)x + 0.5f) / (float)W * 2.0f - 1.0f;
            float ny = ((float)y + 0.5f) / (float)H * 2.0f - 1.0f;
            float d = SDL_sqrtf(nx * nx + ny * ny);
            float ang = SDL_atan2f(ny, nx);
            // Kenar dalgası: ±%18 — tanınabilir bir leke, dairesel değil
            float wob = 1.0f + 0.10f * SDL_sinf(ang * 3.0f)
                             + 0.05f * SDL_sinf(ang * 7.0f + 1.3f)
                             + 0.03f * SDL_sinf(ang * 13.0f + 2.7f);
            float a = 1.0f - d / wob;
            if (a < 0.0f) a = 0.0f;
            if (a > 1.0f) a = 1.0f;
            a = a * a * (3.0f - 2.0f * a);   // yumuşak kenar
            int i = (y * W + x) * 4;
            px[i + 0] = 255; px[i + 1] = 255; px[i + 2] = 255;
            px[i + 3] = (unsigned char)(a * 255.0f);
        }
    }
    s_tex = r2d_texture_from_pixels(renderer, px, W, H);
    if (s_tex != NULL) r2d_texture_set_linear(s_tex, true);
}

void decal_reset(void)
{
    for (int i = 0; i < MAX_DECALS; i++) s_decals[i].active = false;
}

void decal_spawn(float x, float y, float radius,
                 float r, float g, float b, float alpha, float life)
{
    if (radius <= 0.0f || life <= 0.0f) return;
    int slot = -1;
    for (int i = 0; i < MAX_DECALS; i++)
        if (!s_decals[i].active) { slot = i; break; }
    if (slot < 0) {
        // Havuz dolu: EN ESKİYİ (en az ömrü kalanı) ez. Sessizce atlamak yerine
        // ezmek doğru — oyuncunun az önce yaptığı şeyin izi görünmeli.
        float least = 1e9f;
        for (int i = 0; i < MAX_DECALS; i++)
            if (s_decals[i].life < least) { least = s_decals[i].life; slot = i; }
    }
    if (slot < 0) return;
    s_decals[slot] = (Decal){ .x = x, .y = y, .radius = radius,
                              .r = r, .g = g, .b = b, .alpha = alpha,
                              .life = life, .maxLife = life,
                              .rot = (float)(SDL_rand(4)), .active = true };
}

void decal_update(float dt)
{
    for (int i = 0; i < MAX_DECALS; i++) {
        if (!s_decals[i].active) continue;
        s_decals[i].life -= dt;
        if (s_decals[i].life <= 0.0f) s_decals[i].active = false;
    }
}

void decal_render(R2D *renderer, const Camera *camera)
{
    // BLEND AÇIKÇA KURULUYOR (VFX borcu #1): bu fonksiyon eskiden blend
    // modunu HİÇ set etmiyordu, yani kendinden önce ne çizildiyse onun
    // modunu miras alıyordu. Ateş efektleri ADD kullanmaya başlayınca
    // yanık izleri sessizce EKLEMELİ çizilecekti (yer izi koyulaştırır,
    // parlatmaz). Sessiz hata sınıfı: hiçbir uyarı vermez.
    r2d_set_blend(renderer, R2D_BLEND_ALPHA);
    if (s_tex == NULL || camera == NULL) return;
    float z = camera->zoom;
    for (int i = 0; i < MAX_DECALS; i++) {
        const Decal *d = &s_decals[i];
        if (!d->active) continue;
        // Solma: ömrün son %35'inde iner. Öncesinde tam görünür — iz "hemen
        // solmaya başlıyor" gibi durmasın, bir süre orada DURSUN.
        float u = d->life / d->maxLife;
        float k = (u > 0.35f) ? 1.0f : (u / 0.35f);
        float sx = (d->x - camera->x) * z;
        float sy = (d->y - camera->y) * z;
        // Zeminde yatık: dikey kısaltma (3/4 kamera — gölge lekesiyle aynı oran)
        float rw = d->radius * z;
        float rh = rw / 1.37f;
        SDL_FRect dst = { sx - rw, sy - rh, rw * 2.0f, rh * 2.0f };
        r2d_texture_set_color_mod(s_tex, (Uint8)(d->r * 255.0f),
                                         (Uint8)(d->g * 255.0f),
                                         (Uint8)(d->b * 255.0f));
        r2d_texture_set_alpha_mod(s_tex, (Uint8)(d->alpha * k * 255.0f));
        // rot: dokuyu 4 yönden birinde çevir — aynı leke deseni tekrar etmesin
        r2d_draw_texture_flip(renderer, s_tex, NULL, &dst, (int)d->rot & 1);
    }
    r2d_texture_set_color_mod(s_tex, 255, 255, 255);
    r2d_texture_set_alpha_mod(s_tex, 255);
}

// --- Hazır tarifler ---
// Ömürler bilinçli farklı: yanık en uzun (taş kararır), kırağı erir, zehir
// buharlaşır, kan kurur. Aynı süre verilseydi hepsi "aynı sistem" gibi okunurdu.
void decal_scorch(float x, float y, float radius)
{ decal_spawn(x, y, radius, 0.12f, 0.09f, 0.08f, 0.72f, 45.0f); }

void decal_frost(float x, float y, float radius)
{ decal_spawn(x, y, radius, 0.78f, 0.90f, 1.00f, 0.55f, 18.0f); }

void decal_poison(float x, float y, float radius)
{ decal_spawn(x, y, radius, 0.35f, 0.72f, 0.22f, 0.50f, 12.0f); }

void decal_blood(float x, float y, float radius)
{ decal_spawn(x, y, radius, 0.42f, 0.06f, 0.07f, 0.62f, 30.0f); }
