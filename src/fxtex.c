#include "fxtex.h"

// ---------------------------------------------------------------------------
// Tamsayı hash — frac(sin()) YASAK dersinin CPU karşılığı: determinist,
// platformdan bağımsız, dağılımı düzgün (PCG karması). Kafes düğümü başına
// bir değer; aynı (x, y, seed) her makinede aynı sonucu verir.
// ---------------------------------------------------------------------------
static unsigned int hash2(int x, int y, unsigned int seed)
{
    unsigned int h = (unsigned int)x * 374761393u
                   + (unsigned int)y * 668265263u
                   + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

static float hash01(int x, int y, unsigned int seed)
{
    return (float)(hash2(x, y, seed) & 0xFFFFFF) / (float)0xFFFFFF;
}

// smoothstep — value-noise'un hücre içi geçişi. Doğrusal geçiş kafesi belli
// eder (elmas desenli artefakt), smoothstep türevi kenarda sıfırlanır.
static float sstep(float t) { return t * t * (3.0f - 2.0f * t); }

// Döşenebilir value-noise: kafes koordinatı periyoda MOD alınır — son hücre
// ilk hücreye interpolasyon yapar, kenar dikişsiz kapanır.
static float vnoise(float px, float py, int period, unsigned int seed)
{
    int x0 = (int)px, y0 = (int)py;
    float fx = px - (float)x0, fy = py - (float)y0;
    int x1 = (x0 + 1) % period, y1 = (y0 + 1) % period;
    x0 %= period; y0 %= period;
    float a = hash01(x0, y0, seed), b = hash01(x1, y0, seed);
    float c = hash01(x0, y1, seed), d = hash01(x1, y1, seed);
    float sx = sstep(fx), sy = sstep(fy);
    float top = a + (b - a) * sx;
    float bot = c + (d - c) * sx;
    return top + (bot - top) * sy;
}

R2DTexture *fxtex_noise(R2D *r2d, int size, unsigned int seed)
{
    if (size < 8) size = 8;
    Uint8 *px = SDL_malloc((size_t)size * (size_t)size * 4);
    if (px == NULL) return NULL;

    // Kanal başına bir oktav; frekanslar size'ı bölmek zorunda (döşeme şartı).
    static const int FREQ[4] = { 4, 8, 16, 32 };
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            Uint8 *p = &px[(y * size + x) * 4];
            for (int ch = 0; ch < 4; ch++) {
                int period = FREQ[ch];
                float sx = (float)x / (float)size * (float)period;
                float sy = (float)y / (float)size * (float)period;
                // Oktavlar ayrı seed alanında — aynı desenin büyütülmüş hali
                // değil, bağımsız katman (üst üste kaydırınca desen tekrarı
                // göze batmasın)
                float v = vnoise(sx, sy, period, seed + (unsigned int)ch * 747796405u);
                p[ch] = (Uint8)(v * 255.0f + 0.5f);
            }
        }
    }
    R2DTexture *t = r2d_texture_from_pixels(r2d, px, size, size);
    SDL_free(px);
    if (t != NULL) r2d_texture_set_repeat(t, true);   // varlık sebebi bu
    return t;
}

// RGB (0..1) → ton kaydırma. Tam HSV gidiş-dönüşü yerine luma-korumalı
// YIQ rotasyonu: tek matris, gri eksen sabit kalır (rampanın koyu/açık
// dengesi bozulmaz).
static void hue_rotate(float *r, float *g, float *b, float degrees)
{
    float rad = degrees * 0.017453293f;
    float cosA = SDL_cosf(rad), sinA = SDL_sinf(rad);
    // Rodrigues formülünün gri eksen (1,1,1)/√3 çevresi için kapalı hali
    float m00 = cosA + (1.0f - cosA) / 3.0f;
    float m01 = (1.0f - cosA) / 3.0f - 0.5774f * sinA;
    float m02 = (1.0f - cosA) / 3.0f + 0.5774f * sinA;
    float rr = *r * m00 + *g * m01 + *b * m02;
    float gg = *r * m02 + *g * m00 + *b * m01;
    float bb = *r * m01 + *g * m02 + *b * m00;
    *r = rr < 0.0f ? 0.0f : (rr > 1.0f ? 1.0f : rr);
    *g = gg < 0.0f ? 0.0f : (gg > 1.0f ? 1.0f : gg);
    *b = bb < 0.0f ? 0.0f : (bb > 1.0f ? 1.0f : bb);
}

R2DTexture *fxtex_ramp(R2D *r2d, const float (*rgb)[3], const float *stop,
                       int n, int width, float hueShift)
{
    if (rgb == NULL || stop == NULL || n < 2 || width < 2) return NULL;
    Uint8 *px = SDL_malloc((size_t)width * 4);
    if (px == NULL) return NULL;

    for (int x = 0; x < width; x++) {
        float t = (float)x / (float)(width - 1);
        // t'nin düştüğü durak aralığını bul (n küçük — doğrusal arama yeter)
        int i = 0;
        while (i < n - 2 && t > stop[i + 1]) i++;
        float span = stop[i + 1] - stop[i];
        float u = (span > 0.0001f) ? (t - stop[i]) / span : 0.0f;
        if (u < 0.0f) u = 0.0f;
        if (u > 1.0f) u = 1.0f;
        float r = rgb[i][0] + (rgb[i + 1][0] - rgb[i][0]) * u;
        float g = rgb[i][1] + (rgb[i + 1][1] - rgb[i][1]) * u;
        float b = rgb[i][2] + (rgb[i + 1][2] - rgb[i][2]) * u;
        if (hueShift != 0.0f) hue_rotate(&r, &g, &b, hueShift * t);
        Uint8 *p = &px[x * 4];
        p[0] = (Uint8)(r * 255.0f + 0.5f);
        p[1] = (Uint8)(g * 255.0f + 0.5f);
        p[2] = (Uint8)(b * 255.0f + 0.5f);
        p[3] = 255;
    }
    R2DTexture *t = r2d_texture_from_pixels(r2d, px, width, 1);
    SDL_free(px);
    return t;
}
