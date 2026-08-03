#include <stdio.h>
#include <stdlib.h>
#include "tilemap.h"

#define FEET_H 20.0f   // ayak kutusunun yüksekliği
#define FEET_INSET 8.0f // yanlardan içeri ne kadar daraltılsın

// --- Tile tablosu: gerçek tanım burada ---
const TileType TILE_TYPES[] = {
    [0] = { .solid = false, .r = 50,  .g = 200, .b = 50,  .texturePath = "assets/Tiles/Grass/Grass_1_Middle.png", .speedMultiplier = 1.0f, .damage = 0.0f, .conceals = true }, // çimen — eğilince içine girilir
    [1] = { .solid = false, .r = 150, .g = 100, .b = 50,  .texturePath = "assets/Tiles/Grass/Path_Middle.png", .speedMultiplier = 1.0f, .damage = 0.0f }, // yol
    [2] = { .solid = true,  .r = 50,  .g = 50,  .b = 200, .texturePath = "assets/Tiles/Water/Water_Middle.png", .speedMultiplier = 1.0f, .damage = 0.0f }, // su
    [3] = { .solid = true,  .r = 110, .g = 110, .b = 120, .texturePath = "assets/Tiles/terrain/stone.png", .speedMultiplier = 1.0f, .damage = 0.0f },  // taş
    [4] = { .solid = false, .r = 90, .g = 110, .b = 70, .texturePath = "assets/Tiles/terrain/swamp.png", .speedMultiplier = 0.4f, .damage = 0.0f },  // bataklık: yavaşlatır
    [5] = { .solid = false, .r = 200, .g = 60, .b = 40, .texturePath = "assets/Tiles/terrain/thorns.png", .speedMultiplier = 1.0f, .damage = 20.0f },  // diken: saniyede 20 hasar

    // --- Zindan tile'lari (Cute_Fantasy Dungeons, 16px kirpimlar) ---
    [6] = { .solid = false, .r = 60,  .g = 62,  .b = 88,  .texturePath = "assets/Tiles/dungeon/d1_floor.png", .speedMultiplier = 1.0f, .damage = 0.0f }, // K1 doseme
    [7] = { .solid = true,  .r = 95,  .g = 100, .b = 130, .texturePath = "assets/Tiles/dungeon/d1_wall.png",  .speedMultiplier = 1.0f, .damage = 0.0f }, // K1 duvar
    [8] = { .solid = false, .r = 70,  .g = 72,  .b = 100, .texturePath = "assets/Tiles/dungeon/d2_floor.png", .speedMultiplier = 1.0f, .damage = 0.0f }, // K2 doseme
    [9] = { .solid = true,  .r = 105, .g = 110, .b = 140, .texturePath = "assets/Tiles/dungeon/d2_wall.png",  .speedMultiplier = 1.0f, .damage = 0.0f }, // K2 duvar
    [10] = { .solid = true, .r = 7,   .g = 7,   .b = 13,  .texturePath = NULL, .speedMultiplier = 1.0f, .damage = 0.0f },  // void: oda disi karanlik (duz renk)

    // --- Lanetli Mezarlık (Faz HG-B — undead paleti, açık hava zindanı) ---
    [12] = { .solid = false, .r = 111, .g = 115, .b = 106, .texturePath = "assets/Tiles/undead/mz_ground.png", .speedMultiplier = 1.0f, .damage = 0.0f },  // ölü toprak
    [13] = { .solid = true,  .r = 57,  .g = 31,  .b = 33,  .texturePath = NULL, .speedMultiplier = 1.0f, .damage = 0.0f },  // mezarlık kayası (üstü koyu — doku dual-grid'den)
    [14] = { .solid = true,  .r = 26,  .g = 32,  .b = 46,  .texturePath = "assets/Tiles/undead/mz_water.png",  .speedMultiplier = 1.0f, .damage = 0.0f },  // ölü su
    // Doğu çölü (Taşdurak — Faz DOGU)
    [15] = { .solid = false, .r = 222, .g = 180, .b = 120, .texturePath = "assets/Tiles/desert/sand.png",  .speedMultiplier = 1.0f, .damage = 0.0f },  // kum
    [16] = { .solid = true,  .r = 160, .g = 110, .b = 70,  .texturePath = "assets/Tiles/desert/rock.png",  .speedMultiplier = 1.0f, .damage = 0.0f },  // çöl kayası
    [17] = { .solid = true,  .r = 70,  .g = 190, .b = 200, .texturePath = "assets/Tiles/desert/water.png", .speedMultiplier = 1.0f, .damage = 0.0f },  // vaha suyu

    // Mağara (Loca v2 — Faz MAPGEN-2): düz renk tabanlar, doku dual-grid
    // ring/yüz parçalarından gelir (Cave_Walls atlası)
    [18] = { .solid = false, .r = 138, .g = 72,  .b = 54,  .texturePath = NULL, .speedMultiplier = 1.0f, .damage = 0.0f },  // mağara zemini
    [19] = { .solid = true,  .r = 225, .g = 163, .b = 122, .texturePath = NULL, .speedMultiplier = 1.0f, .damage = 0.0f },  // mağara duvarı (üstü ten)
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
static R2DTexture *dsrWaterTex = NULL;  // çöl vaha atlası (dual-grid 16 parça)
static R2DTexture *dsrCliffTex = NULL;  // çöl kaya atlası (dual-grid 16 parça)
static R2DTexture *caveWallTex = NULL;  // mağara duvar atlası (Loca — dual-grid)
static R2DTexture *wldWaterTex = NULL;  // dünya suyu kıyı atlası (dual-grid)

// SU AKIŞI. Kıyı atlası animasyonlu sürümde 8 kare taşıyor; kare seçimi DUVAR
// SAATİNDEN geliyor, oyun saatinden değil — su geceleyin yavaşlamaz, mevsim onu
// dondurmaz. (Buz tutması ayrı bir iş: o gün gelirse kare yerine ATLAS değişir.)
// 8 kare / 1.1 sn: asset'in kendi ritmi; daha hızlısı pixel-art'ta titriyor,
// daha yavaşı su durgun görünüyor.
#define WLD_WATER_FRAMES 8
#define WLD_WATER_FPS    7.3f
static int wldWaterFrames = 1;   // yükleme başarısına göre 1 ya da 8

static int wld_water_frame(void)
{
    if (wldWaterFrames <= 1) return 0;
    float t = (float)SDL_GetTicks() / 1000.0f;
    int f = (int)(t * WLD_WATER_FPS) % wldWaterFrames;
    return f < 0 ? 0 : f;
}

void tilemap_load_details(R2D *renderer)
{
    extern R2DTexture *load_texture(R2D *r, const char *path);   // main yardımcısı
    grassVar[0] = load_texture(renderer, "assets/Tiles/Grass/Grass_1_Middle.png");
    grassVar[1] = load_texture(renderer, "assets/Tiles/Grass/Grass_2_Middle.png");
    grassVar[2] = load_texture(renderer, "assets/Tiles/Grass/Grass_3_Middle.png");
    grassVar[3] = load_texture(renderer, "assets/Tiles/Grass/Grass_4_Middle.png");
    edgeTex = load_texture(renderer, "assets/Tiles/Grass/grass_edges.png");
    flowerTex = load_texture(renderer, "assets/decor/flowers.png");
    pathDecTex = load_texture(renderer, "assets/Tiles/Grass/Path_Decoration.png");
    waterDecTex = load_texture(renderer, "assets/Tiles/Water/Water_Decoration.png");
    mzGround2 = load_texture(renderer, "assets/Tiles/undead/mz_ground2.png");
    dsrWaterTex = load_texture(renderer, "assets/Tiles/desert/water_atlas.png");
    dsrCliffTex = load_texture(renderer, "assets/Tiles/desert/cliff_atlas.png");
    // Keyed varyant: koyu blokların düz arka planı şeffaf (bake_cave_ring.py) —
    // ışık bloklar dokunulmamış; loca + mezarlık AYNI atlası kullanır
    caveWallTex = load_texture(renderer, "assets/Tiles/Cave/Cave_Walls_Keyed.png");
    // SU ANİMASYONU: atlasın animasyonlu sürümü aynı 3x5 düzeni 8 KARE yan yana
    // taşıyor (384x80 = 24x5 hücre). Yani LUT değişmiyor; kıyı hücresinin cx'ine
    // "kare * 3" ekleyince su akmaya başlıyor. Statik sürüm yedek olarak duruyor —
    // animasyonlu dosya bulunamazsa akış durur, kıyı yine çizilir.
    wldWaterTex = load_texture(renderer, "assets/Tiles/Water/Water_Tile_1_Anim.png");
    if (wldWaterTex != NULL) wldWaterFrames = WLD_WATER_FRAMES;
    else {
        wldWaterTex = load_texture(renderer, "assets/Tiles/Water/Water_Tile_1.png");
        wldWaterFrames = 1;
    }
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
    if (dsrWaterTex) { r2d_texture_destroy(NULL, dsrWaterTex); dsrWaterTex = NULL; }
    if (dsrCliffTex) { r2d_texture_destroy(NULL, dsrCliffTex); dsrCliffTex = NULL; }
    if (caveWallTex) { r2d_texture_destroy(NULL, caveWallTex); caveWallTex = NULL; }
    if (wldWaterTex) { r2d_texture_destroy(NULL, wldWaterTex); wldWaterTex = NULL; }
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

// --- ÇÖL DUAL-GRID AUTOTILE (Faz MAPGEN) ---
// Kaya (16) ve vaha suyu (17) tek karo tekrarıyla değil, köşe örneklemeli
// dual-grid ile çizilir: her "dual hücre" 4 harita köşesine bakar, 16 parçalık
// atlastan seçilen parça YARIM KARO kaymış basılır — kenar, dış köşe ve iç
// köşe tüm kombinasyonlar çözülür (Cute Fantasy Desert atlas düzeni).
// Taban olarak kum çizilir; sınır görseli tamamen overlay parçalarından gelir.
#define DSR_ROCK  16
#define DSR_WATER 17

// mask bitleri (dual hücrenin köşeleri): 1=NW 2=NE 4=SW 8=SE hedef tür.
// LUT[mask] = atlas hücre {cx,cy} (16px grid). mask 0 çizilmez.
typedef struct { Uint8 cx, cy; } DsrCell;
static const DsrCell DSR_WATER_LUT[16] = {
    [1]={5,2}, [2]={3,2}, [4]={5,0}, [8]={3,0},      // tek köşe su
    [3]={4,2}, [12]={4,0}, [5]={5,1}, [10]={3,1},    // yarımlar (S/N/E/W kenarı)
    [7]={0,0}, [11]={2,0}, [13]={0,2}, [14]={2,2},   // iç köşe (ada bloğu)
    [6]={3,2}, [9]={3,0},                            // çapraz (sette yok → köşe)
    [15]={4,1},                                      // tam su
};
static const DsrCell DSR_ROCK_LUT[16] = {
    [1]={5,4}, [2]={3,4}, [4]={5,2}, [8]={3,2},
    [3]={4,4}, [12]={4,2}, [5]={5,3}, [10]={3,3},
    [7]={3,7}, [11]={5,7}, [13]={3,9}, [14]={5,9},
    [6]={3,4}, [9]={3,2},
    [15]={255,255},   // tam kaya = plato üstü: taban rock.png çizer, parça yok
};

static inline bool dsr_is(int col, int row, int id)
{
    if (col < 0) col = 0; if (col >= mapW) col = mapW - 1;   // OOB = kenar hücresi
    if (row < 0) row = 0; if (row >= mapH) row = mapH - 1;
    return tilemap[row][col] == id;
}

// Güneyi kum olan kaya = KLİF YÜZÜ hücresi (çizgili duvar basılır).
// Dual pass yüz hücrelerini kayadan SAYMAZ → plato dudağı yüzün üstüne oturur.
static inline bool dsr_is_face(int col, int row)
{
    return dsr_is(col, row, DSR_ROCK) && !dsr_is(col, row + 1, DSR_ROCK);
}

static inline bool dsr_rockish(int col, int row)
{
    return dsr_is(col, row, DSR_ROCK) && !dsr_is_face(col, row);
}

static void dsr_dual_pass(R2D *renderer, R2DTexture *atlas, int id,
                          const Camera *camera, int colL, int rowT,
                          int colR, int rowB)
{
    const DsrCell *lut = (id == DSR_WATER) ? DSR_WATER_LUT : DSR_ROCK_LUT;
    float zoom = camera->zoom;
    for (int row = rowT - 1; row < rowB; row++) {
        for (int col = colL - 1; col < colR; col++) {
            int m;
            if (id == DSR_ROCK)
                m = (dsr_rockish(col, row) ? 1 : 0)
                  | (dsr_rockish(col + 1, row) ? 2 : 0)
                  | (dsr_rockish(col, row + 1) ? 4 : 0)
                  | (dsr_rockish(col + 1, row + 1) ? 8 : 0);
            else
                m = (dsr_is(col, row, id) ? 1 : 0)
                  | (dsr_is(col + 1, row, id) ? 2 : 0)
                  | (dsr_is(col, row + 1, id) ? 4 : 0)
                  | (dsr_is(col + 1, row + 1, id) ? 8 : 0);
            if (m == 0 || lut[m].cx == 255) continue;
            SDL_FRect src = { lut[m].cx * 16.0f, lut[m].cy * 16.0f, 16, 16 };
            SDL_FRect dst = {
                ((col + 0.5f) * TILE_SIZE - camera->x) * zoom,
                ((row + 0.5f) * TILE_SIZE - camera->y) * zoom,
                TILE_SIZE * zoom, TILE_SIZE * zoom };
            r2d_draw_texture(renderer, atlas, &src, &dst);
        }
    }
}

// --- MAĞARA DUAL-GRID (Loca v2 — Faz MAPGEN-2) ---
// Duvar(19) ile zemin sınırı Cave_Walls ring parçalarıyla çizilir; duvarın
// void'e bakan tarafı ÇİZGİSİZ kalır (karanlığa karışır). Güneye bakan duvar
// sırası kaya yığını YÜZÜ alır; yüz hücreleri ring pass'inde duvardan sayılmaz.
#define CAVE_FLOOR 18
#define CAVE_WALL  19
#define MZ_ROCK    13

// Işık bloklar (loca): ring (0-2,0-2) + delikli iç köşe (0-2,3-4)
static const DsrCell CAVE_LUT[16] = {
    [1]={2,2}, [2]={0,2}, [4]={2,0}, [8]={0,0},
    [3]={1,2}, [12]={1,0}, [5]={2,1}, [10]={0,1},
    [7]={0,3}, [11]={2,3}, [13]={0,4}, [14]={2,4},   // iç köşe (delikli blok)
    [6]={255,255}, [9]={255,255},   // çapraz: parçasız (yuvarlak kötü durur)
    [15]={255,255},                 // duvar içi: düz taban kalır
};
// Koyu bloklar (mezarlık): ring (4-6,0-2) + delikli (4-6,3-4) — keyed atlas
static const DsrCell MZ_LUT[16] = {
    [1]={6,2}, [2]={4,2}, [4]={6,0}, [8]={4,0},
    [3]={5,2}, [12]={5,0}, [5]={6,1}, [10]={4,1},
    [7]={4,3}, [11]={6,3}, [13]={4,4}, [14]={6,4},
    [6]={255,255}, [9]={255,255}, [15]={255,255},
};

// Duvar id'sine göre "zemin" sayılan komşu (ring yalnız zemine bakar)
static inline bool cave_floor_at(int wallId, int col, int row)
{
    if (col < 0) col = 0; if (col >= mapW) col = mapW - 1;
    if (row < 0) row = 0; if (row >= mapH) row = mapH - 1;
    int t = tilemap[row][col];
    if (wallId == MZ_ROCK) return t == 12;                  // ölü toprak
    return t == CAVE_FLOOR || t == 6;                       // mağara + taş salon
}

static inline bool cave_is_face(int wallId, int col, int row)
{
    return dsr_is(col, row, wallId) && cave_floor_at(wallId, col, row + 1);
}

static inline bool cave_wallish(int wallId, int col, int row)
{
    return dsr_is(col, row, wallId) && !cave_is_face(wallId, col, row);
}

static void cave_dual_pass(R2D *renderer, const DsrCell *lut, int wallId,
                           const Camera *camera,
                           int colL, int rowT, int colR, int rowB)
{
    float zoom = camera->zoom;
    for (int row = rowT - 1; row < rowB; row++) {
        for (int col = colL - 1; col < colR; col++) {
            int m = (cave_wallish(wallId, col, row) ? 1 : 0)
                  | (cave_wallish(wallId, col + 1, row) ? 2 : 0)
                  | (cave_wallish(wallId, col, row + 1) ? 4 : 0)
                  | (cave_wallish(wallId, col + 1, row + 1) ? 8 : 0);
            if (m == 0 || lut[m].cx == 255) continue;
            bool touchesFloor = cave_floor_at(wallId, col, row) ||
                                cave_floor_at(wallId, col + 1, row) ||
                                cave_floor_at(wallId, col, row + 1) ||
                                cave_floor_at(wallId, col + 1, row + 1);
            if (!touchesFloor) continue;   // duvar|void tarafı çizgisiz
            SDL_FRect src = { lut[m].cx * 16.0f, lut[m].cy * 16.0f, 16, 16 };
            SDL_FRect dst = {
                ((col + 0.5f) * TILE_SIZE - camera->x) * zoom,
                ((row + 0.5f) * TILE_SIZE - camera->y) * zoom,
                TILE_SIZE * zoom, TILE_SIZE * zoom };
            r2d_draw_texture(renderer, caveWallTex, &src, &dst);
        }
    }
}

// --- DÜNYA SUYU KIYI GEÇİŞLERİ (Faz MAPGEN-2) ---
// Su(2) kıyıları Water_Tile_1 dual-grid setiyle yumuşar: çimen tutamlı
// toprak dudak. Parça yalnız kıyının TAMAMI ÇİMEN(0) olduğunda basılır —
// yol/bataklık/taş kıyısı bugünkü sert haliyle kalır (parça bg'si çimen).
static const DsrCell WLD_WATER_LUT[16] = {
    [1]={2,2}, [2]={0,2}, [4]={2,0}, [8]={0,0},
    [3]={1,2}, [12]={1,0}, [5]={2,1}, [10]={0,1},
    [7]={0,3}, [11]={1,3}, [13]={0,4}, [14]={1,4},   // iç köşe (2x2 ada bloğu)
    [6]={0,2}, [9]={0,0},
    [15]={255,255},   // tam su: taban Water_Middle zaten çizili
};

static void wld_water_pass(R2D *renderer, const Camera *camera,
                           int colL, int rowT, int colR, int rowB)
{
    float zoom = camera->zoom;
    // Kare bir kez hesaplanır: aynı karede bütün kıyı hücreleri AYNI kareyi
    // kullanmalı, yoksa su hücre hücre farklı fazda akar ve ızgara görünür.
    const float frameOffX = (float)wld_water_frame() * 3.0f * 16.0f;
    for (int row = rowT - 1; row < rowB; row++) {
        for (int col = colL - 1; col < colR; col++) {
            int m = 0;
            bool shoreGrass = true;
            for (int dy = 0; dy < 2; dy++)
                for (int dx = 0; dx < 2; dx++) {
                    int cc = col + dx, rr = row + dy;
                    if (cc < 0) cc = 0; if (cc >= mapW) cc = mapW - 1;
                    if (rr < 0) rr = 0; if (rr >= mapH) rr = mapH - 1;
                    int t = tilemap[rr][cc];
                    if (t == 2) m |= 1 << (dy * 2 + dx);
                    else if (t != 0) shoreGrass = false;
                }
            if (m == 0 || !shoreGrass || WLD_WATER_LUT[m].cx == 255) continue;
            SDL_FRect src = { WLD_WATER_LUT[m].cx * 16.0f + frameOffX,
                              WLD_WATER_LUT[m].cy * 16.0f, 16, 16 };
            SDL_FRect dst = {
                ((col + 0.5f) * TILE_SIZE - camera->x) * zoom,
                ((row + 0.5f) * TILE_SIZE - camera->y) * zoom,
                TILE_SIZE * zoom, TILE_SIZE * zoom };
            r2d_draw_texture(renderer, wldWaterTex, &src, &dst);
        }
    }
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

            // Çöl suyu: dual-grid overlay çizer — hücreye KUM tabanı bas
            if (tileID == DSR_WATER && dsrWaterTex != NULL)
            {
                tile_draw(renderer, tileTextures, 15, dst);   // 15 = kum
            }
            // Çöl kayası = MESA: üstü de KUM tabanı (paket stili — plato
            // yüzeyi zeminle aynı, sınırı dudak çizer); güneye bakan sıra
            // KLİF YÜZÜ (çizgili duvar, L/M/R uçlu)
            else if (tileID == DSR_ROCK && dsrCliffTex != NULL)
            {
                tile_draw(renderer, tileTextures, 15, dst);
                if (dsr_is_face(col, row))
                {
                    bool fl = dsr_is_face(col - 1, row), fr = dsr_is_face(col + 1, row);
                    int fc = (fl && fr) ? 10 : (fr ? 9 : (fl ? 11 : 10));
                    SDL_FRect fsrc = { fc * 16.0f, 2 * 16.0f, 16, 16 };
                    r2d_draw_texture(renderer, dsrCliffTex, &fsrc, &dst);
                }
            }
            // Mezarlık zemini: 2 varyant (aynı hash deseni)
            else if (tileID == 12 && mzGround2 != NULL && grass_variant(col, row) >= 2)
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

            // MAĞARA/MEZARLIK YÜZÜ: güneyi zemin olan duvar hücresi kaya
            // yığını duvarı gösterir (atlas satır 6; loca L/M/R, mezarlık
            // 2 koyu varyant hash ile)
            if (tileID == CAVE_WALL && caveWallTex != NULL &&
                cave_is_face(CAVE_WALL, col, row))
            {
                bool fl = cave_is_face(CAVE_WALL, col - 1, row);
                bool fr = cave_is_face(CAVE_WALL, col + 1, row);
                int fc = (fl && fr) ? 1 : (fr ? 0 : (fl ? 2 : 1));
                SDL_FRect fsrc = { fc * 16.0f, 6 * 16.0f, 16, 16 };
                r2d_draw_texture(renderer, caveWallTex, &fsrc, &dst);
            }
            else if (tileID == MZ_ROCK && caveWallTex != NULL &&
                     cave_is_face(MZ_ROCK, col, row))
            {
                SDL_FRect fsrc = { (4 + grass_variant(col, row) % 2) * 16.0f,
                                   6 * 16.0f, 16, 16 };
                r2d_draw_texture(renderer, caveWallTex, &fsrc, &dst);
            }

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

    // ÇÖL AUTOTILE: taban pass'i bitti — su ve kaya sınır overlay'leri
    // (yalnız bu id'leri içeren haritada iş yapar; diğerlerinde mask hep 0)
    if (dsrWaterTex != NULL)
        dsr_dual_pass(renderer, dsrWaterTex, DSR_WATER, camera, colL, rowT, colR, rowB);
    if (dsrCliffTex != NULL)
        dsr_dual_pass(renderer, dsrCliffTex, DSR_ROCK, camera, colL, rowT, colR, rowB);
    if (caveWallTex != NULL)
    {
        cave_dual_pass(renderer, CAVE_LUT, CAVE_WALL, camera, colL, rowT, colR, rowB);
        cave_dual_pass(renderer, MZ_LUT, MZ_ROCK, camera, colL, rowT, colR, rowB);
    }
    if (wldWaterTex != NULL)
        wld_water_pass(renderer, camera, colL, rowT, colR, rowB);
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
            // ÇÖL BENEKLERİ (paket örneklerindeki organik zemin yamaları):
            // grid'e HİZALI çizilir (parçalar komşu karolarla kompoze) —
            // büyük 4×3 koyu yama / küçük 2×2 açık yama / tekli çatlak izi
            else if ((id == 15 || id == DSR_ROCK) && dsrCliffTex != NULL) {
                SDL_FRect adst = {
                    (col * TILE_SIZE - camera->x) * zoom,
                    (row * TILE_SIZE - camera->y) * zoom,
                    TILE_SIZE * zoom, TILE_SIZE * zoom };
                bool sand43 = true, sand22 = true;
                for (int dy = 0; dy < 3 && sand43; dy++)
                    for (int dx = 0; dx < 4; dx++)
                        if (col+dx >= mapW || row+dy >= mapH ||
                            tilemap[row+dy][col+dx] != 15) { sand43 = false; break; }
                for (int dy = 0; dy < 2 && sand22; dy++)
                    for (int dx = 0; dx < 2; dx++)
                        if (col+dx >= mapW || row+dy >= mapH ||
                            tilemap[row+dy][col+dx] != 15) { sand22 = false; break; }
                if (id == 15 && (h % 1000) < 6 && sand43) {
                    SDL_FRect src = { 7 * 16.0f, 7 * 16.0f, 64, 48 };
                    SDL_FRect pd = { adst.x, adst.y,
                                     4 * TILE_SIZE * zoom, 3 * TILE_SIZE * zoom };
                    r2d_draw_texture(renderer, dsrCliffTex, &src, &pd);
                }
                else if (id == 15 && (h % 1000) < 22 && sand22) {
                    SDL_FRect src = { 9 * 16.0f, 5 * 16.0f, 32, 32 };
                    SDL_FRect pd = { adst.x, adst.y,
                                     2 * TILE_SIZE * zoom, 2 * TILE_SIZE * zoom };
                    r2d_draw_texture(renderer, dsrCliffTex, &src, &pd);
                }
                else if ((h % 100) < 7 && !(id == DSR_ROCK && dsr_is_face(col, row))) {
                    // tekli çatlak izleri: atlas (11-12, 5-7) — 6 varyant
                    int v = (int)((h >> 6) % 6);
                    SDL_FRect src = { (11 + v % 2) * 16.0f, (5 + v / 2) * 16.0f, 16, 16 };
                    r2d_draw_texture(renderer, dsrCliffTex, &src, &adst);
                }
            }
        }
    }
}
