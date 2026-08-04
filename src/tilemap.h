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

// SU DERİNLİK MASKESİ (Faz SU) — karo başına 2×2 RGBA teksel, R = derinlik
// (0 kara → 255 en derin). outW/outH TEKSEL cinsindendir (karo değil):
// mapW*2 × mapH*2. Su hacmi geçişine verilir (r2d.h R2DWater.mask) ve shader'da
// LINEAR örneklenir; kıyının köşesiz silueti ile derinlik gradyanı oradan gelir.
// false = bu haritada hiç sıvı yok (su geçişi hiç çalışmaz).
// Dönen tampon statiktir; çağıran sahiplenmez, harita değişince yeniden sorar.
bool wld_water_mask_fill(int *outW, int *outH, const Uint8 **outPix);

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

// Harita görsel yenileme: çimen varyantları + kenar kaplamaları (main init'te
// bir kez — modül kendi detay dokularını yükler/yıkar)
void tilemap_load_details(R2D *renderer);
void tilemap_free_details(void);

// Süs serpiştirmesi: çimen çiçekleri + yol kenarı taşları + su sazları —
// deterministik hash (save'e girmez), çarpışmasız, YALNIZ dünya haritasında
// (main harita kontrolüyle çağırır — tilemap_render'dan hemen sonra)
void tilemap_render_scatter(R2D *renderer, const Camera *camera);

// --- "SU OLMADAN SU" ŞERİT KAYDIRMA DEMOSU (VFX testi) ---
// Köy güneyindeki taş döşeli oval çukur (karo 3..10 × 19..24) su asset'i
// KULLANILMADAN "altında su varmış gibi" çizilir: taş karo yatay şeritlere
// bölünüp her şerit sinüsle kaydırılır (line scroll hilesi) + mavi tül +
// gezen parlaklık bandı. YALNIZ dünya haritasında çağrılır.
void tilemap_vfxdemo_render(R2D *renderer, R2DTexture **tileTextures,
                            const Camera *camera);

// Dev anahtarı (Ctrl+V) — dört durum: asset akışı ve şerit hilesi bağımsız
// kısılır. "Gördüğüm hareket asset'ten mi bizden mi" sorusunun cevabı bu.
void tilemap_vfx_cycle(void);
const char *tilemap_vfx_label(void);

int  tile_at_point(float worldX, float worldY);
bool box_hits_solid(SDL_FRect box);
SDL_FRect feet_box(float rectX, float rectY, float rectW, float rectH);

#endif // TILEMAP_H