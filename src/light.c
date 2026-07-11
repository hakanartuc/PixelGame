#include "light.h"

// Köy ışıkları: ev pencereleri (decor.c'deki evlerin gövdesine hizalı — ev
// tabanı (x,y)'de, pencere ışığı biraz yukarıda) + meydan meşale direkleri.
// Direklerin sprite'ı yok — şimdilik yalnız ışık, görseli sonra.
#define T(n) ((n) * (float)TILE_SIZE)

// Yoğunluklar bilerek <0.75: ambient+katkı 1.0'a genişçe doymasın —
// doyma düz "plato" yapar, komşu ışıkla birleşince silindir gibi okunur.
// Mavi/yeşil bileşenler düşük: merkez beyazlaşmasın, sıcaklık korunsun.
const LightDef LIGHTS[] = {
    // Ev pencereleri (Baran / Kemal / Osman) — sıcak turuncu, mum gibi hafif titrer
    { .x = T(10), .y = T(4)  - 50.0f, .radius = 150.0f, .r = 1.00f, .g = 0.55f, .b = 0.22f, .intensity = 0.60f, .flicker = 0.05f },
    { .x = T(14), .y = T(6)  - 50.0f, .radius = 150.0f, .r = 1.00f, .g = 0.55f, .b = 0.22f, .intensity = 0.60f, .flicker = 0.05f },
    { .x = T(6),  .y = T(10) - 50.0f, .radius = 150.0f, .r = 1.00f, .g = 0.55f, .b = 0.22f, .intensity = 0.60f, .flicker = 0.05f },

    // Köy meydanı meşale direkleri — belirgin titreşim
    { .x = T(10), .y = T(8),  .radius = 200.0f, .r = 1.00f, .g = 0.62f, .b = 0.28f, .intensity = 0.65f, .flicker = 0.12f },
    { .x = T(13), .y = T(10), .radius = 200.0f, .r = 1.00f, .g = 0.62f, .b = 0.28f, .intensity = 0.65f, .flicker = 0.12f },
};
const int LIGHT_COUNT = (int)(sizeof(LIGHTS) / sizeof(LIGHTS[0]));

// Alev titreşimi: iki farklı frekanslı sinüsün karışımı — periyodik görünmez.
// i: ışık indeksi (fazları ayrışsın, hepsi birlikte yanıp sönmesin)
static float flicker_mul(float amount, float t, int i)
{
    if (amount <= 0.0f) return 1.0f;
    float w = SDL_sinf(t * 13.0f + (float)i * 7.0f) * 0.6f
            + SDL_sinf(t * 7.3f + (float)i * 3.1f) * 0.4f;
    return 1.0f + amount * w;
}

// Oyuncu meşalesi (envanterde Meşale varsa)
#define TORCH_RADIUS 220.0f

int light_collect(R2DLight *out, int cap, const Camera *cam, int sw, int sh, bool staticLights,
                  float torchX, float torchY, bool torchOn, WeaponFxType weaponFx)
{
    float zoom = cam->zoom;
    float t = (float)SDL_GetTicks() / 1000.0f;   // titreşim saati (sn)
    int n = 0;

    for (int i = 0; staticLights && i < LIGHT_COUNT && n < cap; i++) {
        float sx = (LIGHTS[i].x - cam->x) * zoom;
        float sy = (LIGHTS[i].y - cam->y) * zoom;
        float sr = LIGHTS[i].radius * zoom;
        if (sx + sr < 0 || sx - sr > (float)sw ||
            sy + sr < 0 || sy - sr > (float)sh)
            continue;   // ekran dışı — shader döngüsüne sokma
        out[n++] = (R2DLight){ sx, sy, sr,
                               LIGHTS[i].r, LIGHTS[i].g, LIGHTS[i].b,
                               LIGHTS[i].intensity * flicker_mul(LIGHTS[i].flicker, t, i) };
    }

    float psx = (torchX - cam->x) * zoom;
    float psy = (torchY - cam->y) * zoom;

    if (torchOn && n < cap) {
        out[n++] = (R2DLight){ psx, psy, TORCH_RADIUS * zoom,
                               1.00f, 0.65f, 0.30f,
                               0.70f * flicker_mul(0.12f, t, LIGHT_COUNT) };
    }

    // Silah efekti geceye kendi renginde ışık verir: alev güçlü, diğerleri sönük parıltı
    if (weaponFx != WEAPON_FX_NONE && n < cap) {
        R2DLight l = { psx, psy, 120.0f * zoom, 1.0f, 1.0f, 1.0f, 0.40f };
        float flicker = 0.06f;   // enerji efektleri hafif nefes alır
        switch (weaponFx) {
        case WEAPON_FX_FLAME:
            l.radius = 180.0f * zoom;
            l.r = 1.00f; l.g = 0.58f; l.b = 0.22f; l.intensity = 0.75f;
            flicker = 0.15f;   // alev sert titrer
            break;
        case WEAPON_FX_ICE:
            l.r = 0.50f; l.g = 0.72f; l.b = 1.00f;
            break;
        case WEAPON_FX_POISON:
            l.r = 0.38f; l.g = 0.95f; l.b = 0.32f;
            break;
        default: // WEAPON_FX_PINK
            l.r = 1.00f; l.g = 0.38f; l.b = 0.80f;
            break;
        }
        l.intensity *= flicker_mul(flicker, t, LIGHT_COUNT + 1);
        out[n++] = l;
    }
    return n;
}

// Kamp ateşi: meşale direğinden büyük, sert titreşimli sıcak ışık
int light_append_campfire(R2DLight *out, int n, int cap, const Camera *cam,
                          int sw, int sh, float x, float y)
{
    if (n >= cap) return n;
    float zoom = cam->zoom;
    float sx = (x - cam->x) * zoom;
    float sy = (y - cam->y) * zoom;
    float sr = 230.0f * zoom;
    if (sx + sr < 0 || sx - sr > (float)sw ||
        sy + sr < 0 || sy - sr > (float)sh)
        return n;   // ekran dışı
    float t = (float)SDL_GetTicks() / 1000.0f;
    int phase = n + LIGHT_COUNT + 2;   // her ateşin fazı ayrışsın
    out[n++] = (R2DLight){ sx, sy, sr,
                           1.00f, 0.58f, 0.24f,
                           0.70f * flicker_mul(0.14f, t, phase) };
    return n;
}
