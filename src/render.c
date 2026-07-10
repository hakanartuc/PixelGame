#include "render.h"
#include "common.h"
#include "tilemap.h"   // map_width/map_height (kamera clamp)

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Bir PNG'yi yükleyip GPU texture'ına dönüştürür; hata olursa NULL döner.
// DİKKAT (sahiplik): bu fonksiyon yeni bir texture "yaratır"; onu yok etmek
// ÇAĞIRANIN SORUMLULUĞUNDADIR. (r2d varsayılanı NEAREST — pixel-art keskin)
R2DTexture *load_texture(R2D *renderer, const char *path)
{
    return r2d_texture_load(renderer, path);
}

// Kamera karakteri ekranda ortalayacak şekilde ayarlanır. Clamp ile harita dışına çıkmaz.
void camera_update(Camera *camera, float targetX, float targetY, int winW, int winH)
{
    float zoom = camera->zoom;

    // Ekranda gorunen dunya alani zoom ile kuculur
    float viewW = winW / zoom;
    float viewH = winH / zoom;

    // Kamerayi hedefe ortala (dunya koordinati)
    camera->x = targetX - viewW / 2.0f;
    camera->y = targetY - viewH / 2.0f;

    // Harita sinirlari (zoom'lu gorus alanina gore; gercek boyut tilemap'ten)
    float worldW = map_width() * TILE_SIZE;
    float worldH = map_height() * TILE_SIZE;
    if (camera->x < 0.0f) camera->x = 0.0f;
    if (camera->y < 0.0f) camera->y = 0.0f;
    if (camera->x > worldW - viewW) camera->x = worldW - viewW;
    if (camera->y > worldH - viewH) camera->y = worldH - viewH;
}

// Yumusak radial glow texture'i uretir (merkez parlak, kenar saydam).
// Bir kez cagrilir, sonra additive blend ile cizilir.
R2DTexture *create_glow_texture(R2D *renderer, int size)
{
    // RGBA pixel buffer
    Uint8 *pixels = (Uint8 *)SDL_malloc(size * size * 4);
    if (!pixels) return NULL;

    float center = size / 2.0f;
    float maxDist = center;

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float dx = x - center;
            float dy = y - center;
            float dist = SDL_sqrtf(dx*dx + dy*dy);

            // Merkez=1, kenar=0 (yumusak dususe icin kare al)
            float intensity = 1.0f - (dist / maxDist);
            if (intensity < 0.0f) intensity = 0.0f;
            intensity = intensity * intensity;   // daha yumusak kenar (gaussian benzeri)

            int idx = (y * size + x) * 4;
            pixels[idx + 0] = 255;   // R (beyaz taban — tint ile renklenecek)
            pixels[idx + 1] = 255;   // G
            pixels[idx + 2] = 255;   // B
            pixels[idx + 3] = (Uint8)(intensity * 255);  // A (radial)
        }
    }

    R2DTexture *tex = r2d_texture_from_pixels(renderer, pixels, size, size);
    SDL_free(pixels);

    if (tex) r2d_texture_set_linear(tex, true);   // glow yumusak olsun (NEAREST degil)
    return tex;
}

// Vinyet: merkez seffaf, kenarlara dogru opaklasir (hasar flash'i icin —
// tam ekrana gerilir, color mod ile renklenir). Glow'un tersi.
R2DTexture *create_vignette_texture(R2D *renderer, int size)
{
    Uint8 *pixels = (Uint8 *)SDL_malloc(size * size * 4);
    if (!pixels) return NULL;

    float center = size / 2.0f;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            // Merkeze gore normalize mesafe: 0 merkez, ~1.41 kose
            float nx = (x - center) / center;
            float ny = (y - center) / center;
            float d = SDL_sqrtf(nx * nx + ny * ny);

            // Ic %55 tamamen seffaf, sonra yumusak yukselis
            float v = (d - 0.55f) / 0.65f;
            if (v < 0.0f) v = 0.0f;
            if (v > 1.0f) v = 1.0f;
            v = v * v;   // kenara dogru ivmelenen opaklik

            int idx = (y * size + x) * 4;
            pixels[idx + 0] = 255;   // beyaz taban — tint ile renklenir
            pixels[idx + 1] = 255;
            pixels[idx + 2] = 255;
            pixels[idx + 3] = (Uint8)(v * 255);
        }
    }

    R2DTexture *tex = r2d_texture_from_pixels(renderer, pixels, size, size);
    SDL_free(pixels);

    if (tex) r2d_texture_set_linear(tex, true);   // gerilince bantlanmasin
    return tex;
}