#ifndef TILEMAP_H
#define TILEMAP_H

#include "r2d.h"
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "common.h"     // TILE_SIZE, MAP_W/H, Camera


// Bir tile türünün özellikleri
typedef struct 
{
    bool solid;
    Uint8 r, g, b;          // fallback renk (texture yüklenemezse)
    const char *texturePath; // bu tile'ın 16x16 doku dosyası (NULL = sadece renk)
    float speedMultiplier;   // 1.0 = normal hız; <1 yavaşlatır (bataklık), >1 hızlandırır
    float damage;            // saniyede verdiği hasar (0 = zararsız)
    bool conceals;           // içine girilebilir gizlenme (çim, saman...) — EĞİLİRKEN algıyı düşürür
} TileType;

#define MAX_TILE_TYPES 64   // tile türü tavanı (dizi boyutları için derleme-zamanı sabiti)

// Tile ID -> özellik tablosu (tanımı tilemap.c'de)
extern const TileType TILE_TYPES[];
extern const int TILE_TYPE_COUNT;

// --- Genel arayüz (başka modüller bunları kullanır) ---
void generate_map(void);

// Tiled CSV export'unu yükle (değerler GID: 0=boş, 1=ilk tile → id = GID-1).
// Boyut dosyadan okunur (satır/sütun sayısı), tavan MAP_W_MAX×MAP_H_MAX.
// Başarısızsa false — çağıran generate_map fallback'ine düşmeli.
bool tilemap_load_csv(const char *path);

// Gerçek harita boyutu (tile) — CSV'den ya da generate_map'ten
int map_width(void);
int map_height(void);

// Tek tile değiştir (boss arena kapısı: zemin↔duvar) — sınır dışı sessiz
void tilemap_set_tile(int col, int row, int id);

void tilemap_render(R2D *renderer, R2DTexture **tileTextures, const Camera *camera);

int  tile_at_point(float worldX, float worldY);
bool box_hits_solid(SDL_FRect box);
SDL_FRect feet_box(float rectX, float rectY, float rectW, float rectH);

#endif // TILEMAP_H