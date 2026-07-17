#include <stdio.h>
#include <stdlib.h>
#include "tilemap.h"

#define FEET_H 20.0f   // ayak kutusunun yüksekliği
#define FEET_INSET 8.0f // yanlardan içeri ne kadar daraltılsın

// --- Tile tablosu: gerçek tanım burada ---
const TileType TILE_TYPES[] = {
    [0] = { .solid = false, .r = 50,  .g = 200, .b = 50,  .texturePath = "assets/tiles/Grass/Grass_1_Middle.png", .speedMultiplier = 1.0f, .damage = 0.0f, .conceals = true }, // çimen — eğilince içine girilir
    [1] = { .solid = false, .r = 150, .g = 100, .b = 50,  .texturePath = "assets/tiles/Grass/Path_Middle.png", .speedMultiplier = 1.0f, .damage = 0.0f }, // yol
    [2] = { .solid = true,  .r = 50,  .g = 50,  .b = 200, .texturePath = "assets/tiles/Water/Water_Middle.png", .speedMultiplier = 1.0f, .damage = 0.0f }, // su
    [3] = { .solid = true,  .r = 110, .g = 110, .b = 120, .texturePath = "assets/tiles/terrain/stone.png", .speedMultiplier = 1.0f, .damage = 0.0f },  // taş
    [4] = { .solid = false, .r = 90, .g = 110, .b = 70, .texturePath = "assets/tiles/terrain/swamp.png", .speedMultiplier = 0.4f, .damage = 0.0f },  // bataklık: yavaşlatır
    [5] = { .solid = false, .r = 200, .g = 60, .b = 40, .texturePath = "assets/tiles/terrain/thorns.png", .speedMultiplier = 1.0f, .damage = 20.0f },  // diken: saniyede 20 hasar

    // --- Zindan tile'lari (Cute_Fantasy Dungeons, 16px kirpimlar) ---
    [6] = { .solid = false, .r = 60,  .g = 62,  .b = 88,  .texturePath = "assets/tiles/dungeon/d1_floor.png", .speedMultiplier = 1.0f, .damage = 0.0f }, // K1 doseme
    [7] = { .solid = true,  .r = 95,  .g = 100, .b = 130, .texturePath = "assets/tiles/dungeon/d1_wall.png",  .speedMultiplier = 1.0f, .damage = 0.0f }, // K1 duvar
    [8] = { .solid = false, .r = 70,  .g = 72,  .b = 100, .texturePath = "assets/tiles/dungeon/d2_floor.png", .speedMultiplier = 1.0f, .damage = 0.0f }, // K2 doseme
    [9] = { .solid = true,  .r = 105, .g = 110, .b = 140, .texturePath = "assets/tiles/dungeon/d2_wall.png",  .speedMultiplier = 1.0f, .damage = 0.0f }, // K2 duvar
    [10] = { .solid = true, .r = 7,   .g = 7,   .b = 13,  .texturePath = NULL, .speedMultiplier = 1.0f, .damage = 0.0f },  // void: oda disi karanlik (duz renk)

    // --- Lanetli Mezarlık (Faz HG-B — undead paleti, açık hava zindanı) ---
    [12] = { .solid = false, .r = 111, .g = 115, .b = 106, .texturePath = "assets/tiles/undead/mz_ground.png", .speedMultiplier = 1.0f, .damage = 0.0f },  // ölü toprak
    [13] = { .solid = true,  .r = 38,  .g = 42,  .b = 54,  .texturePath = "assets/tiles/undead/mz_rock.png",   .speedMultiplier = 1.0f, .damage = 0.0f },  // dikenli kaya
    [14] = { .solid = true,  .r = 26,  .g = 32,  .b = 46,  .texturePath = "assets/tiles/undead/mz_water.png",  .speedMultiplier = 1.0f, .damage = 0.0f },  // ölü su
};
const int TILE_TYPE_COUNT = (int)(sizeof(TILE_TYPES) / sizeof(TILE_TYPES[0]));

// --- Harita verisi: modüle özel (dışarıdan görünmez) ---
// Statik tavan boyutunda; gerçek boyut mapW/mapH (CSV'den ya da generate_map'ten)
static int tilemap[MAP_H_MAX][MAP_W_MAX];
static int mapW = 40, mapH = 30;   // generate_map varsayılanı

int map_width(void)  { return mapW; }

void tilemap_set_tile(int col, int row, int id)
{
    if (col < 0 || col >= mapW || row < 0 || row >= mapH) return;
    if (id < 0 || id >= TILE_TYPE_COUNT) return;
    tilemap[row][col] = id;
}
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

// --- Harita görsel yenileme: çimen varyantları + kenar kaplamaları ---
// Çimen "taşar": çimen olmayan komşu tile'ın kenarına dalgalı çimen dili
// çizilir (grass_edges.png atlası — 12 hücre: N,S,W,E, 4 dış köşe, 4 iç köşe).
// Yalnız DÜNYA tile'larına taşar (yol/su/bataklık/diken) — zindan dokunulmaz.
static R2DTexture *grassVar[4] = {0};   // [0] ana doku (tileTextures'tan gelmez — kendi kopyası)
static R2DTexture *edgeTex = NULL;
static R2DTexture *flowerTex = NULL;    // flowers.png (160×160 — 10×10 hücre 16px)
static R2DTexture *pathDecTex = NULL;   // Path_Decoration.png (3 hücre 16px)
static R2DTexture *waterDecTex = NULL;  // Water_Decoration.png (3 hücre 16px)
static R2DTexture *mzGround2 = NULL;    // mezarlık zemin 2. varyantı

void tilemap_load_details(R2D *renderer)
{
    extern R2DTexture *load_texture(R2D *r, const char *path);   // main yardımcısı
    grassVar[0] = load_texture(renderer, "assets/tiles/Grass/Grass_1_Middle.png");
    grassVar[1] = load_texture(renderer, "assets/tiles/Grass/Grass_2_Middle.png");
    grassVar[2] = load_texture(renderer, "assets/tiles/Grass/Grass_3_Middle.png");
    grassVar[3] = load_texture(renderer, "assets/tiles/Grass/Grass_4_Middle.png");
    edgeTex = load_texture(renderer, "assets/tiles/Grass/grass_edges.png");
    flowerTex = load_texture(renderer, "assets/decor/flowers.png");
    pathDecTex = load_texture(renderer, "assets/tiles/Grass/Path_Decoration.png");
    waterDecTex = load_texture(renderer, "assets/tiles/Water/Water_Decoration.png");
    mzGround2 = load_texture(renderer, "assets/tiles/undead/mz_ground2.png");
}

void tilemap_free_details(void)
{
    for (int i = 0; i < 4; i++)
        if (grassVar[i]) { r2d_texture_destroy(NULL, grassVar[i]); grassVar[i] = NULL; }
    if (edgeTex) { r2d_texture_destroy(NULL, edgeTex); edgeTex = NULL; }
    if (flowerTex) { r2d_texture_destroy(NULL, flowerTex); flowerTex = NULL; }
    if (pathDecTex) { r2d_texture_destroy(NULL, pathDecTex); pathDecTex = NULL; }
    if (waterDecTex) { r2d_texture_destroy(NULL, waterDecTex); waterDecTex = NULL; }
    if (mzGround2) { r2d_texture_destroy(NULL, mzGround2); mzGround2 = NULL; }
}

// Deterministik varyant: hash(col,row) — save'e girmez, hep aynı görünür
static inline int grass_variant(int col, int row)
{
    unsigned v = (unsigned)(col * 73856093u ^ row * 19349663u);
    v = (v >> 13) ^ v;
    return (int)(v % 4u);
}

static inline bool is_grass(int col, int row)
{
    if (col < 0 || col >= mapW || row < 0 || row >= mapH) return false;
    return tilemap[row][col] == 0;
}

// Çimenin taşabileceği taban mı? (dünya dış mekan tile'ları — zindan HARİÇ)
static inline bool grass_bleeds_onto(int id)
{
    return id == 1 || id == 2 || id == 4 || id == 5;
}

static void edge_cell_draw(R2D *renderer, int cell, const SDL_FRect *dst)
{
    SDL_FRect src = { (float)(cell * 16), 0, 16, 16 };
    r2d_draw_texture(renderer, edgeTex, &src, dst);
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

            // Mezarlık zemini: 2 varyant (aynı hash deseni)
            if (tileID == 12 && mzGround2 != NULL && grass_variant(col, row) >= 2)
            {
                r2d_draw_texture(renderer, mzGround2, NULL, &dst);
            }
            // Çimen: 4 varyanttan hash ile seç (tekdüzelik kırılır)
            else if (tileID == 0 && grassVar[0] != NULL)
            {
                R2DTexture *gv = grassVar[grass_variant(col, row)];
                if (gv != NULL) { r2d_draw_texture(renderer, gv, NULL, &dst); }
                else            tile_draw(renderer, tileTextures, tileID, dst);
            }
            else
                tile_draw(renderer, tileTextures, tileID, dst);

            // KENAR KAPLAMALARI: çimen komşu tabana taşar (yumuşak geçiş)
            if (edgeTex != NULL && grass_bleeds_onto(tileID))
            {
                bool n = is_grass(col, row - 1), s2 = is_grass(col, row + 1);
                bool w = is_grass(col - 1, row), e = is_grass(col + 1, row);
                if (n) edge_cell_draw(renderer, 0, &dst);
                if (s2) edge_cell_draw(renderer, 1, &dst);
                if (w) edge_cell_draw(renderer, 2, &dst);
                if (e) edge_cell_draw(renderer, 3, &dst);
                // Dış köşeler: yalnız çaprazda çimen (kenarlar boşken)
                if (!n && !w && is_grass(col - 1, row - 1)) edge_cell_draw(renderer, 4, &dst);
                if (!n && !e && is_grass(col + 1, row - 1)) edge_cell_draw(renderer, 5, &dst);
                if (!s2 && !w && is_grass(col - 1, row + 1)) edge_cell_draw(renderer, 6, &dst);
                if (!s2 && !e && is_grass(col + 1, row + 1)) edge_cell_draw(renderer, 7, &dst);
                // İç köşeler: iki kenar birleşiyor — köşe boşluğu dolsun
                if (n && w) edge_cell_draw(renderer, 8, &dst);
                if (n && e) edge_cell_draw(renderer, 9, &dst);
                if (s2 && w) edge_cell_draw(renderer, 10, &dst);
                if (s2 && e) edge_cell_draw(renderer, 11, &dst);
            }
        }
    }
}

// --- SÜS SERPİŞTİRMESİ: dünya boş hissettirmesin (çarpışmasız, deterministik) ---
// Kural: hash(col,row) — çimende %6 çiçek/ot, yol KENARINDAKİ çimende %10 taş,
// su kıyısındaki suda saz. Save'e girmez: aynı koordinat hep aynı süsü taşır.
void tilemap_render_scatter(R2D *renderer, const Camera *camera)
{
    if (flowerTex == NULL) return;
    float zoom = camera->zoom;
    int outW = 0, outH = 0;
    r2d_output_size(renderer, &outW, &outH);
    int colL = (int)(camera->x / TILE_SIZE) - 1;
    int rowT = (int)(camera->y / TILE_SIZE) - 1;
    int colR = (int)((camera->x + outW / zoom) / TILE_SIZE) + 2;
    int rowB = (int)((camera->y + outH / zoom) / TILE_SIZE) + 2;
    if (colL < 0) colL = 0;
    if (rowT < 0) rowT = 0;
    if (colR > mapW) colR = mapW;
    if (rowB > mapH) rowB = mapH;

    for (int row = rowT; row < rowB; row++) {
        for (int col = colL; col < colR; col++) {
            int id = tilemap[row][col];
            unsigned h = (unsigned)(col * 2654435761u ^ row * 40503u);
            h = (h >> 11) ^ h;
            float dx = (float)((h >> 4) % 40) / 40.0f;   // tile içi kayma
            float dy = (float)((h >> 9) % 40) / 40.0f;
            SDL_FRect dst = {
                (col * TILE_SIZE + dx * (TILE_SIZE - 20) - camera->x) * zoom,
                (row * TILE_SIZE + dy * (TILE_SIZE - 20) - camera->y) * zoom,
                20.0f * zoom, 20.0f * zoom };

            if (id == 0) {   // çimen: %6 çiçek/ot tömreği
                bool nearPath = (col > 0 && tilemap[row][col-1] == 1) ||
                                (col < mapW-1 && tilemap[row][col+1] == 1) ||
                                (row > 0 && tilemap[row-1][col] == 1) ||
                                (row < mapH-1 && tilemap[row+1][col] == 1);
                if (nearPath && (h % 100) < 10 && pathDecTex != NULL) {
                    SDL_FRect src = { (float)((h >> 6) % 3) * 16.0f, 0, 16, 16 };
                    r2d_draw_texture(renderer, pathDecTex, &src, &dst);
                }
                else if ((h % 100) < 6) {
                    // flowers.png: 10×10 hücre — üst 3 satırdan seç (çiçek/ot)
                    int fc = (int)((h >> 6) % 10), fr = (int)((h >> 13) % 3);
                    SDL_FRect src = { fc * 16.0f, fr * 16.0f, 16, 16 };
                    r2d_draw_texture(renderer, flowerTex, &src, &dst);
                }
            }
            else if (id == 2 && waterDecTex != NULL) {   // su: kıyıda %8 saz
                bool nearLand = (col > 0 && tilemap[row][col-1] != 2) ||
                                (col < mapW-1 && tilemap[row][col+1] != 2) ||
                                (row > 0 && tilemap[row-1][col] != 2) ||
                                (row < mapH-1 && tilemap[row+1][col] != 2);
                if (nearLand && (h % 100) < 8) {
                    SDL_FRect src = { (float)((h >> 6) % 3) * 16.0f, 0, 16, 16 };
                    r2d_draw_texture(renderer, waterDecTex, &src, &dst);
                }
            }
        }
    }
}
