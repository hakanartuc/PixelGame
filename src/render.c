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
        SDL_Log("Yüklenemedi: %s", stbi_failure_reason());
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
    // Karakterin GÖRSEL merkezi (çizim kutusunun ortası)
    // kamera hedefi parametre olarak gelir, ekranda ortalanır
    camera->x = targetX - winW / 2.0f;
    camera->y = targetY - winH / 2.0f;

    // --- KISIT: kamera görüş alanını harita sınırları içine çek ---
    float worldW = MAP_W * TILE_SIZE;
    float worldH = MAP_H * TILE_SIZE;
    if (camera->x < 0.0f)         camera->x = 0.0f;          // sol kenar
    if (camera->y < 0.0f)         camera->y = 0.0f;          // üst kenar
    if (camera->x > worldW - winW) camera->x = worldW - winW;  // sağ kenar
    if (camera->y > worldH - winH) camera->y = worldH - winH;  // alt kenar
}