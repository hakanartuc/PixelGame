#include "render.h"
#include "common.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Bir PNG'yi yükleyip GPU texture'ına düşüştürür; hata olursa NULL döner.
// DİKKAT (sahiplik): bu fonksiyon yeni bir texture "yaratır"; onu yok etmek
// ARTIK ÇAĞIRANIN SORUMLULUĞUNDADIR.
SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path)
{
    int w, h, channels;
    unsigned char *pixels = stbi_load(path, &w, &h, &channels, 4); // 4 = RGBA'ya zorla
    if(pixels == NULL){
        SDL_Log("Yuklenemedi (%s): %s", path, SDL_GetError());
        return NULL;
    }

    SDL_Surface *surface = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, pixels, w * 4);
    if(surface == NULL){
        SDL_Log("Surface olusturulamadi: %s %s", path, SDL_GetError());
        stbi_image_free(pixels);
        return NULL;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    if(texture == NULL){
        SDL_Log("Texture olusturulamadi: %s %s", path, SDL_GetError());
        SDL_DestroySurface(surface);
        stbi_image_free(pixels);
        return NULL;
    }

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST); // pikselleri keskin tut, bulanıklaştırma yapma
    return texture;

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

    // Harita sinirlari (zoom'lu gorus alanina gore)
    float worldW = MAP_W * TILE_SIZE;
    float worldH = MAP_H * TILE_SIZE;
    if (camera->x < 0.0f) camera->x = 0.0f;
    if (camera->y < 0.0f) camera->y = 0.0f;
    if (camera->x > worldW - viewW) camera->x = worldW - viewW;
    if (camera->y > worldH - viewH) camera->y = worldH - viewH;
}

// Yumusak radial glow texture'i uretir (merkez parlak, kenar saydam).
// Bir kez cagrilir, sonra additive blend ile cizilir.
SDL_Texture *create_glow_texture(SDL_Renderer *renderer, int size)
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

    SDL_Surface *surface = SDL_CreateSurfaceFrom(size, size, SDL_PIXELFORMAT_RGBA32, pixels, size * 4);
    if (!surface) { SDL_free(pixels); return NULL; }

    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    SDL_free(pixels);

    if (tex) SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);  // glow yumusak olsun (NEAREST degil)
    return tex;
}