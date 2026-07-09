#include <stdio.h>
#include <stdlib.h>
#include "tilemap.h"

#define FEET_H 20.0f   // ayak kutusunun yüksekliği
#define FEET_INSET 8.0f // yanlardan içeri ne kadar daraltılsın

// --- Tile tablosu: gerçek tanım burada ---
const TileType TILE_TYPES[] = {
    [0] = { .solid = false, .r = 50,  .g = 200, .b = 50,  .texturePath = "assets/tiles/Grass/Grass_1_Middle.png", .speedMultiplier = 1.0f, .damage = 0.0f }, // çimen
    [1] = { .solid = false, .r = 150, .g = 100, .b = 50,  .texturePath = "assets/tiles/Grass/Path_Middle.png", .speedMultiplier = 1.0f, .damage = 0.0f }, // yol
    [2] = { .solid = true,  .r = 50,  .g = 50,  .b = 200, .texturePath = "assets/tiles/Water/Water_Middle.png", .speedMultiplier = 1.0f, .damage = 0.0f }, // su
    [3] = { .solid = true,  .r = 110, .g = 110, .b = 120, .texturePath = NULL, .speedMultiplier = 1.0f, .damage = 0.0f },  // taş: şimdilik renk
    [4] = { .solid = false, .r = 90, .g = 110, .b = 70, .texturePath = NULL, .speedMultiplier = 0.4f, .damage = 0.0f },  // bataklık: yavaşlatır
    [5] = { .solid = false, .r = 200, .g = 60, .b = 40, .texturePath = NULL, .speedMultiplier = 1.0f, .damage = 20.0f },  // diken: saniyede 20 hasar
};
const int TILE_TYPE_COUNT = (int)(sizeof(TILE_TYPES) / sizeof(TILE_TYPES[0]));

// --- Harita verisi: modüle özel (dışarıdan görünmez) ---
// Statik tavan boyutunda; gerçek boyut mapW/mapH (CSV'den ya da generate_map'ten)
static int tilemap[MAP_H_MAX][MAP_W_MAX];
static int mapW = 40, mapH = 30;   // generate_map varsayılanı

int map_width(void)  { return mapW; }
int map_height(void) { return mapH; }

void generate_map(void)
{
    mapW = 40;
    mapH = 30;
    for (int row = 0; row < mapH; row++) {
        for (int col = 0; col < mapW; col++) {
            if (row == 0 || col == 0 || row == mapH-1 || col == mapW-1)
                tilemap[row][col] = 2;        // kenar çerçevesi: su
            else
                tilemap[row][col] = 0;        // iç: çimen
        }
    }

    // Yatay bir taş duvar: row 10, col 5..15 arası
    for (int col = 5; col <= 15; col++)
        tilemap[10][col] = 3;   // taş

        // Su gölü: row 15..18, col 20..25 arası dikdörtgen
    for (int row = 15; row <= 18; row++)
        for (int col = 20; col <= 25; col++)
            tilemap[row][col] = 2;   // su

    // Test: bataklık şeridi
    for (int col = 6; col <= 10; col++) tilemap[6][col] = 4;
    // Test: diken alanı
    for (int col = 6; col <= 8; col++) tilemap[8][col] = 5;
}

// Tiled CSV: her satır virgülle ayrılmış GID'ler. GID 0 = boş (çimen sayarız),
// 1+ = tileset'teki sıra → bizim ID = GID - 1. Satır uzunlukları eşit olmalı.
bool tilemap_load_csv(const char *path)
{
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        SDL_Log("Harita bulunamadi: %s (generate_map kullanilacak)", path);
        return false;
    }

    char line[MAP_W_MAX * 8];   // satır tamponu: tile başına "9999," bolca sığar
    int row = 0, cols = -1;

    while (row < MAP_H_MAX && fgets(line, sizeof(line), f) != NULL) {
        // Boş satırı atla (dosya sonu newline'ı vb.)
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0')
            continue;

        int col = 0;
        char *cursor = line;
        while (col < MAP_W_MAX) {
            char *end;
            long gid = strtol(cursor, &end, 10);
            if (end == cursor)
                break;   // sayı yok → satır bitti

            int id = (int)gid - 1;                       // GID → tile ID
            if (id < 0 || id >= TILE_TYPE_COUNT) id = 0; // bilinmeyen → çimen
            tilemap[row][col++] = id;

            cursor = end;
            if (*cursor == ',') cursor++;
        }

        if (cols == -1) cols = col;
        if (col != cols) {
            SDL_Log("CSV satir uzunlugu tutarsiz (satir %d: %d, beklenen %d): %s",
                    row, col, cols, path);
            fclose(f);
            return false;
        }
        row++;
    }
    fclose(f);

    if (row == 0 || cols <= 0) {
        SDL_Log("CSV bos/okunamadi: %s", path);
        return false;
    }

    mapW = cols;
    mapH = row;
    SDL_Log("Harita yuklendi: %s (%dx%d)", path, mapW, mapH);
    return true;
}

// --- İçsel yardımcılar (static = sadece bu dosyada görünür) ---
static bool tile_is_solid(int tileID) 
{ 
    // SINIR KONTROLÜ: geçersiz ID'ler katı sayılır (oyuncu dünyadan çıkamasın)
    if (tileID < 0 || tileID >= TILE_TYPE_COUNT)
        return true;

    return TILE_TYPES[tileID].solid;
}
// Bir tile ID'sinin rengini tablodan alıp renderer'a set eder.
static void tile_set_color(R2D *renderer, int tileID)
{
    // SINIR KONTROLÜ: geçersiz ID'ler siyah sayılır
    Uint8 r = 0, g = 0, b = 0;
    if (tileID >= 0 && tileID < TILE_TYPE_COUNT) {
        r = TILE_TYPES[tileID].r;
        g = TILE_TYPES[tileID].g;
        b = TILE_TYPES[tileID].b;
    }
    r2d_set_color(renderer, r, g, b, 255);
    
}

// Bir tile'ı verilen ekran-diktörgenine çizer. Eğer tile'ın texture'ı yoksa renk basılır.
static void tile_draw(R2D *renderer, R2DTexture **textures,int tileID, SDL_FRect dst)
{
    if (tileID >= 0 && tileID < TILE_TYPE_COUNT) {
        R2DTexture *texture = textures[tileID];
        if (texture != NULL) {
            r2d_draw_texture(renderer, texture, NULL, &dst);
            return;
        }
    }

    // Geçersiz ID veya texture yoksa renk bas:
    tile_set_color(renderer, tileID);
    r2d_fill_rect(renderer, &dst);
}

// --- Genel fonksiyonlar (header'da ilan edilmiş, static DEĞİL) ---
// Dünya koordinatındaki bir noktanın hangi tile ID'sinde olduğunu döndürür. Harita dışı ise -1 döner.
int tile_at_point(float worldX, float worldY)
{
    int col = (int)(worldX / TILE_SIZE);
    int row = (int)(worldY / TILE_SIZE);

    if (col < 0 || col >= mapW || row < 0 || row >= mapH)
        return -1; // harita dışı

    return tilemap[row][col];
}

// Verilen DÜNYA kutusu (x, y, w, h) katı bir tile ile çarpışıyor mu?
bool box_hits_solid(SDL_FRect box)
{
    // Kutunun kapladığı tile aralığı (sütun ve satır olarak)
    int colL = (int)(box.x / TILE_SIZE); // sol kenar -> ilk sütun
    int colR = (int)((box.x + box.w -1) / TILE_SIZE); // sağ kenar -> son sütun (-1: kutunun sağ kenarı tam olarak bir tile'ın sol kenarına denk gelirse, o tile'ı da kapsasın)
    int rowT = (int)(box.y / TILE_SIZE); // üst kenar -> ilk satır
    int rowB = (int)((box.y + box.h -1) / TILE_SIZE); // alt kenar -> son satır

    // Kutunun kapsadığı tüm tile'ları kontrol et
    for (int row = rowT; row <= rowB; row++) {
        for (int col = colL; col <= colR; col++) {
            // SINIR KONTROLÜ: harita dışı = katı say (oyuncu dünyadan çıkamasın)
            if (col < 0 || col >= mapW || row < 0 || row >= mapH)
                return true; // harita dışı

            if(tile_is_solid(tilemap[row][col])) {
                return true; // çarpışma bulundu
            }
        }
    }

    return false;
}

// Verilen rect konumu için ayak kutusunu (collision alanı) hesaplar.
SDL_FRect feet_box(float rectX, float rectY, float rectW, float rectH)
{
    SDL_FRect f = {
        rectX + FEET_INSET,
        rectY + rectH - FEET_H,
        rectW - FEET_INSET * 2.0f,
        FEET_H
    };
    return f;
}

void tilemap_render(R2D *renderer, R2DTexture **tileTextures, const Camera *camera)
{
    float zoom = camera->zoom;

    // CULLING: sadece ekranda görünen tile aralığını çiz.
    // Büyük haritada (200×150 = 30k tile) hepsini çizmek boşa draw call'dur.
    int outW = 0, outH = 0;
    r2d_output_size(renderer, &outW, &outH);

    int colL = (int)(camera->x / TILE_SIZE);
    int rowT = (int)(camera->y / TILE_SIZE);
    int colR = (int)((camera->x + outW / zoom) / TILE_SIZE) + 1;
    int rowB = (int)((camera->y + outH / zoom) / TILE_SIZE) + 1;
    if (colL < 0) colL = 0;
    if (rowT < 0) rowT = 0;
    if (colR > mapW) colR = mapW;
    if (rowB > mapH) rowB = mapH;

    for (int row = rowT; row < rowB; row++) {
        for (int col = colL; col < colR; col++) {
            int tileID = tilemap[row][col];
            SDL_FRect dst = {
                (col * TILE_SIZE - camera->x) * zoom,
                (row * TILE_SIZE - camera->y) * zoom,
                TILE_SIZE * zoom,
                TILE_SIZE * zoom
            };
            tile_draw(renderer, tileTextures, tileID, dst);
        }
    }
}