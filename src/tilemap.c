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

    // SUALTI TABAN (Faz SU): suyun ALTINDA kalan yürünebilir kaya. Dokusu taş —
    // yani bu karo su asseti KULLANMIYOR. Sualtı görünümünün tamamı su hacmi
    // geçişinden geliyor (shaders/water.frag.hlsl): kaya bükülüyor, derinlik
    // gradyanı üstüne biniyor, kostik ağı üstünde geziyor. Katı DEĞİL: sığlık,
    // içine girilir ama su ayağı ağırlaştırır.
    [20] = { .solid = false, .r = 96,  .g = 104, .b = 112, .texturePath = "assets/Tiles/terrain/stone.png", .speedMultiplier = 0.72f, .damage = 0.0f },  // sualtı taban (sığlık)
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
static R2DTexture *uwBushTex[2] = { NULL, NULL };  // bush_1/2 — sualtı bitkisi+taşı (renk moduyla)
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

// --- VFX MOD ANAHTARI (dev tuşu Ctrl+V) — ÜÇ DURUMLU KARŞILAŞTIRMA ---
// Sorulan soru şuydu: kıyıda görülen hareket ASSET'in animasyon karelerinden mi
// geliyor, bizim kodumuzdan mı? İki kaynağı BAĞIMSIZ kısabilen tek bir anahtar
// cevabı kesinleştiriyor:
//   0 = ikisi de açık · 1 = yalnız ŞERİT hilesi (asset akışı dondu)
//   2 = yalnız ASSET akışı (şerit demosu kapalı) · 3 = ikisi de kapalı
static int vfxMode = 0;
static inline bool vfx_asset_flow(void) { return vfxMode == 0 || vfxMode == 2; }
static inline bool vfx_strip_demo(void) { return vfxMode == 0 || vfxMode == 1; }

void tilemap_vfx_cycle(void) { vfxMode = (vfxMode + 1) % 4; }

const char *tilemap_vfx_label(void)
{
    switch (vfxMode) {
        case 1:  return "VFX: yalniz SERIT hilesi (asset akisi DONDU)";
        case 2:  return "VFX: yalniz ASSET akisi (serit demosu KAPALI)";
        case 3:  return "VFX: IKISI DE KAPALI";
        default: return "VFX: hepsi acik (asset akisi + serit hilesi)";
    }
}

// ---------------------------------------------------------------------------
// SU DERİNLİK MASKESİ (Faz SU). Karo başına WMASK_SUB×WMASK_SUB teksel; R kanalı
// derinlik (0 = kara, 255 = en derin). Shader bunu LINEAR örnekler — kıyının
// köşesiz silueti ve derinlik gradyanı bu iki karardan doğuyor, shader'da ayrıca
// yumuşatma yok.
//
// ÇÖZÜNÜRLÜK NEDEN 2 TEKSEL/KARO: karo başına tek teksel, 8 karoluk bir gölette
// yalnız 8 örnek demek — LINEAR arada düz bir rampa çizse de kademe kaba kalıyor
// ve en önemlisi KIYI RAMPASI yarım karo (8 sanat px) genişliyordu, yani su
// tinti çimenin 8 pikselini maviye boyuyordu. 2 teksel/karo ile kıyı rampası
// 4 sanat px'e iniyor ve su çizgisi TAM karo sınırına oturuyor (aşağıda türetme).
//
// Derinlik = karo türünün TABAN ağırlığı × karadan uzaklık rampası.
//
// HATA GEÇMİŞİ — RAMPA HİÇ ÇALIŞMIYORDU: uzaklık sabit 5 karoya bölünüyordu
// (WMASK_FALLOFF+1). 8×6 karoluk gölette merkez ancak d=3 alıyor, yani derinlik
// base*3/5 = menzilin %60'ı, sonra base 0.30 ile çarpılınca 0.18 — tüm gölet
// 0.06..0.18 arasında, gözle DÜZ. Çözüm: uzaklık alanını GÖVDE BAŞINA normalize
// etmek. Her bağlı su gövdesi kendi en büyük uzaklığına bölünüyor, böylece
// küçük bir gölet de merkezinde tam derinliğe erişiyor.
//
// Gövde başına normalizasyona bir TAVAN kondu (WMASK_RAMP_MAX): saf gövde
// normalizasyonu koca bir denizde rampayı 100 karoya yayardı ve kıyıdaki sığ
// raf kaybolurdu. Küçük gövde kendi merkezine kadar rampalanır, büyük gövde
// eski davranışta olduğu gibi 5 karoda doyar.
//
// ÖLÇÜM (world.csv, 200×150, dört su gövdesi bulundu):
//   gövde 1 nehir/kenar suyu 5536 teksel, dmax= 2 → norm  2 (kendi merkezine)
//   gövde 2 KÖY GÖLETİ         128 teksel, dmax= 4 → norm  4 (kendi merkezine)
//   gövde 3 göl               1148 teksel, dmax=12 → norm 10 (tavan)
//   gövde 4 göl                644 teksel, dmax=10 → norm 10 (tavan)
// YAN ETKİ, sahibinin bilmesi gereken: iki karo genişliğindeki NEHİRLER de artık
// merkezlerinde tam derinliğe (id 2 base = 1.00 → örtme 0.93) erişiyor; eskiden
// 0.53'te kalıyorlardı. id 2 "su" katı/geçilmez ve taban ağırlığı 1.00 seçilmiş,
// yani tür kendini "en derin" ilan ediyor — davranış tutarlı ama nehirler
// eskisinden belirgin koyu. Fazla geldiyse çare bu dosyada: id 2'nin tabanını
// düşürmek (rampayı değil).
#define WMASK_SUB       2        // karo başına teksel (her eksende)
#define WMASK_RAMP_MAX 10        // rampanın en çok kaç YARIM karoda doyduğu (=5 karo)
#define WMASK_SHORE_HT  6        // G kanalının doyduğu yarım karo sayısı (=3 karo)
#define WMASK_BANK_REACH 3       // kıyı gölgesinin eriştiği yarım karo (=24 sanat px)
#define WMASK_BODY_MAX 255       // ayrı su gövdesi tavanı (0 = "kara" etiketi)

// Taban ağırlığı: sıvının ne kadar KAPATTIĞI. Shader'da
// cover = lerp(shallowCover, deepCover, R) — yani bu sayı gövdenin MERKEZİNDEKİ
// örtmeyi seçiyor, her yerdeki örtmeyi değil.
//
// id 20 NEDEN 0.30 → 0.62: 0.30 gövde normalizasyonundan ÖNCE yazılmıştı ve o
// zaman zaten hiçbir yerde ulaşılmıyordu (en fazla 0.18). Artık gerçekten
// merkezde ulaşılıyor, dolayısıyla "merkez" değeri olarak seçilmeli. 0.62 →
// merkezde cover ≈ 0.26+0.67*0.62 = 0.67 (taban %33 okunur: koyu bir kütle
// olarak görünen ama kaybolmayan dip), kıyı çizgisinde R ≈ 0.62/12 = 0.05 →
// cover ≈ 0.29, yani kenar neredeyse berrak. Gölet ortasının koyu, kenarının
// berrak olması "bir göleti gölet yapan" şey; tek sayı ikisini birden veremezdi,
// menzil şart.
static float wmask_base(int id)
{
    switch (id) {
        case 2:  return 1.00f;   // su
        case 17: return 0.55f;   // vaha suyu — kanon "dingin ve sığ" istiyor
        case 20: return 0.92f;   // gölet ortası OPAK lacivert (referans: taban
                                  // yalnız kıyı RAFINDA okunur, ortada değil.
                                  // 0.62'de orta band SINIRINA oturuyordu ve
                                  // dither koca bir dama dikdörtgeni çiziyordu)
        default: return 0.0f;    // kara
    }
}

// ÇIRPINTI: 0 = ayna gibi durgun · 255 = açık su. Köpük, dalga boyu ve dalga
// hızı buna bağlı; kostik ise TERS yönde (durgun suda ışık dibe iner).
static Uint8 wmask_agitation(int id)
{
    switch (id) {
        case 2:  return 255;   // göl/nehir/deniz — açık su
        case 17: return 30;    // vaha suyu: kanon "dingin" istiyor
        case 20: return 26;    // gölet/sığlık: deniz değil, kıpırtı kadar
        default: return 0;
    }
}

#define WMASK_W_MAX (MAP_W_MAX * WMASK_SUB)
#define WMASK_H_MAX (MAP_H_MAX * WMASK_SUB)

// Statik tampon bütçesi (toplam ≈ 2.75 MB, harita başına bir kez doldurulur):
//   wmaskPix  512*512*4 = 1024 KB   maske dokusu
//   wmaskDist 512*512*2 =  512 KB   kıyı uzaklığı (YARIM KARO cinsinden; 256
//                                   karoluk haritada 256'yı geçebilir → Uint16)
//   wmaskComp 512*512   =  256 KB   bağlı su gövdesi etiketi (0 = kara)
//   wmaskQ    512*512*4 = 1024 KB   BFS/taşma-dolgu kuyruğu (iki geçiş paylaşır)
bool tilemap_is_water(int col, int row)
{
    if (col < 0 || col >= mapW || row < 0 || row >= mapH) return false;
    return wmask_base(tilemap[row][col]) > 0.0f;
}

static Uint8  wmaskPix[WMASK_W_MAX * WMASK_H_MAX * 4];
static Uint16 wmaskDist[WMASK_H_MAX][WMASK_W_MAX];
static Uint8  wmaskComp[WMASK_H_MAX][WMASK_W_MAX];
static Uint32 wmaskQ[WMASK_W_MAX * WMASK_H_MAX];
static Uint16 wmaskBodyMax[WMASK_BODY_MAX + 1];
// BALIK ÇAPALARI — gövde başına GARANTİLİ (zar değil). Raf halkasından
// (dist==3 teksel = kıyıdan 1.5 karo) eşit aralıklı seçilir: balık suyun
// ŞEFFAF olduğu yerde yüzer, opak ortada görünmez gölge çizmeyiz.
#define WMASK_FISH_MAX 48
static float wmaskFishXY[WMASK_FISH_MAX][2];
static int   wmaskFishN = 0;
static Uint16 wmaskCand[WMASK_BODY_MAX + 1];   // gövde başına aday sayısı
static Uint16 wmaskPick[WMASK_BODY_MAX + 1];   // seçim adımı sayacı

// 8-KOMŞULUK. 4-komşuluk yetmiyor: yalnız KÖŞESİYLE karaya değen bir su tekseli
// 4-komşulukta uzaklık 2 alıyor (karaya ancak bir su tekselinden dolanarak
// ulaşıyor) ve kıyı köşelerinde derinlik bir kademe fazla çıkıyor — çapraz
// kıyılarda kıyı çizgisi içeri kaçıyordu.
static const int WMASK_NX[8] = { -1,  0,  1, -1,  1, -1,  0,  1 };
static const int WMASK_NY[8] = { -1, -1, -1,  0,  0,  1,  1,  1 };

static bool wmask_wet(int tx, int ty)
{
    return wmask_base(tilemap[ty][tx]) > 0.0f;
}

bool wld_water_mask_fill(int *outW, int *outH, const Uint8 **outPix)
{
    const int W = mapW * WMASK_SUB, H = mapH * WMASK_SUB;

    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            wmaskDist[y][x] = 0;
            wmaskComp[y][x] = 0;
        }

    // 1) BAĞLI SU GÖVDELERİNİ ETİKETLE (8-komşuluk taşma dolgusu). Gövde
    //    kimliği gerekiyor çünkü rampa gövde BAŞINA normalize edilecek: köyün
    //    göleti ile kuzeydeki deniz aynı ölçekle rampalanamaz.
    bool any = false;
    int bodies = 0;
    bool capped = false;
    for (int sy = 0; sy < H; sy++)
        for (int sx = 0; sx < W; sx++) {
            if (wmaskComp[sy][sx] != 0) continue;
            if (!wmask_wet(sx / WMASK_SUB, sy / WMASK_SUB)) continue;
            any = true;
            int id;
            if (bodies < WMASK_BODY_MAX) {
                id = ++bodies;
            } else {
                // Tavan aşıldı: yeni gövdeyi SON gövdeye kat. Görsel sonuç
                // "rampa ölçeği yanlış gövdeden geldi" — kabul edilebilir,
                // maskenin tamamen boş çıkmasından iyi.
                id = bodies;
                if (!capped) {
                    capped = true;
                    SDL_Log("wmask: su govdesi tavani (%d) asildi — fazlasi son govdeye katildi",
                            WMASK_BODY_MAX);
                }
            }
            int head = 0, tail = 0;
            wmaskComp[sy][sx] = (Uint8)id;
            wmaskQ[tail++] = (Uint32)(sy * W + sx);
            while (head < tail) {
                Uint32 c = wmaskQ[head++];
                int cx = (int)(c % (Uint32)W), cy = (int)(c / (Uint32)W);
                for (int k = 0; k < 8; k++) {
                    int nx = cx + WMASK_NX[k], ny = cy + WMASK_NY[k];
                    if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
                    if (wmaskComp[ny][nx] != 0) continue;
                    if (!wmask_wet(nx / WMASK_SUB, ny / WMASK_SUB)) continue;
                    wmaskComp[ny][nx] = (Uint8)id;
                    wmaskQ[tail++] = (Uint32)(ny * W + nx);
                }
            }
        }
    if (!any) return false;

    // 2) KIYI UZAKLIĞI: karadan çok-kaynaklı BFS (8-komşuluk). Karaya değen su
    //    tekseli d=1, ondan sonra dalga dalga içeri. Eskiden dört genişleme
    //    adımıyla kesiliyordu; gövde maksimumunu bilmek için TAM alan gerekli.
    //    Harita DIŞI kara sayılıyor: bu dünya sınırlı, kenarda sonsuz deniz yok
    //    (eski kod kenar suyunu "çok uzak" sayıp en derin yapıyordu).
    {
        int head = 0, tail = 0;
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++) {
                if (wmaskComp[y][x] == 0) continue;
                bool atShore = false;
                for (int k = 0; k < 8 && !atShore; k++) {
                    int nx = x + WMASK_NX[k], ny = y + WMASK_NY[k];
                    if (nx < 0 || ny < 0 || nx >= W || ny >= H) atShore = true;
                    else if (wmaskComp[ny][nx] == 0) atShore = true;
                }
                if (atShore) {
                    wmaskDist[y][x] = 1;
                    wmaskQ[tail++] = (Uint32)(y * W + x);
                }
            }
        while (head < tail) {
            Uint32 c = wmaskQ[head++];
            int cx = (int)(c % (Uint32)W), cy = (int)(c / (Uint32)W);
            Uint16 nd = (Uint16)(wmaskDist[cy][cx] + 1);
            for (int k = 0; k < 8; k++) {
                int nx = cx + WMASK_NX[k], ny = cy + WMASK_NY[k];
                if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
                if (wmaskComp[ny][nx] == 0 || wmaskDist[ny][nx] != 0) continue;
                wmaskDist[ny][nx] = nd;
                wmaskQ[tail++] = (Uint32)(ny * W + nx);
            }
        }
    }

    // 3) Gövde başına en büyük uzaklık — rampanın böleni
    for (int i = 0; i <= WMASK_BODY_MAX; i++) wmaskBodyMax[i] = 0;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            Uint8 c = wmaskComp[y][x];
            if (c != 0 && wmaskDist[y][x] > wmaskBodyMax[c])
                wmaskBodyMax[c] = wmaskDist[y][x];
        }

    // 3b) BALIK ÇAPALARI: gövde başına aday say, eşit aralıkla seç.
    //     Aday = raf halkası (dist==3 teksel). Gövde başına 1 + aday/40 balık
    //     (gölet ~12 aday → 1 balık GARANTİ; göl ~yüzlerce → 5-8 balık).
    wmaskFishN = 0;
    for (int i = 0; i <= WMASK_BODY_MAX; i++) { wmaskCand[i] = 0; wmaskPick[i] = 0; }
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            if (wmaskComp[y][x] != 0 && wmaskDist[y][x] == 3)
                wmaskCand[wmaskComp[y][x]]++;
    for (int y = 0; y < H && wmaskFishN < WMASK_FISH_MAX; y++)
        for (int x = 0; x < W && wmaskFishN < WMASK_FISH_MAX; x++) {
            Uint8 cB = wmaskComp[y][x];
            if (cB == 0 || wmaskDist[y][x] != 3 || wmaskCand[cB] == 0) continue;
            int want = 1 + wmaskCand[cB] / 24;   // gölde daha kalabalık
            int stride = wmaskCand[cB] / want;
            if (stride < 1) stride = 1;
            if ((wmaskPick[cB]++ % stride) != (Uint16)(stride / 2)) continue;
            if (wmaskPick[cB] / stride > want) continue;   // gövde kotası doldu
            float ts = (float)TILE_SIZE / (float)WMASK_SUB;
            wmaskFishXY[wmaskFishN][0] = (x + 0.5f) * ts;
            wmaskFishXY[wmaskFishN][1] = (y + 0.5f) * ts;
            wmaskFishN++;
        }

    // 4) Teksel yazımı
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            int id = tilemap[y / WMASK_SUB][x / WMASK_SUB];
            float base = wmask_base(id);
            Uint8 *p = &wmaskPix[(y * W + x) * 4];
            if (base <= 0.0f) {   // kara: DÖRT kanal da sıfır. A=0 şart: A artık
                                  // kıyı gölgesi taşıyor ve karada 255 kalsaydı
                                  // LINEAR örnekleme her kıyıya (güneye de!)
                                  // yarım teksel sahte gölge sızdırırdı.
                p[0] = 0; p[1] = 0; p[2] = 0; p[3] = 0;
                continue;
            }
            float norm = (float)wmaskBodyMax[wmaskComp[y][x]];
            if (norm > (float)WMASK_RAMP_MAX) norm = (float)WMASK_RAMP_MAX;
            if (norm < 1.0f) norm = 1.0f;
            float f = (float)wmaskDist[y][x] / norm;
            if (f > 1.0f) f = 1.0f;
            int v = (int)(base * f * 255.0f + 0.5f);
            p[0] = (Uint8)(v < 0 ? 0 : v > 255 ? 255 : v);
            // G = KIYI UZAKLIĞI, R'den AYRI. Köpük derinliğe değil kıyıya ait:
            // R'ye bağlandığında sığ bir havuzun TAMAMI köpük sayılıyordu (en
            // derin yeri 0.18, köpük eşiği 0.22 — hepsi eşiğin altında kaldı).
            // Bu kanal "kıyıdan kaç YARIM KARO içerdeyim" diyor, su ne kadar
            // derin olursa olsun; gövdeden BAĞIMSIZ, yani shader'daki su çizgisi
            // sabitleri her gövdede aynı anlama geliyor. 0 = kara, 1 = üç karo.
            float sf = (float)wmaskDist[y][x] / (float)WMASK_SHORE_HT;
            if (sf > 1.0f) sf = 1.0f;
            int sv = (int)(sf * 255.0f + 0.5f);
            p[1] = (Uint8)(sv < 0 ? 0 : sv > 255 ? 255 : sv);
            // B = ÇIRPINTI: bu su gövdesinin mizacı. Gölet ile deniz aynı
            // davranamaz — gölette köpük yoktur, dalga küçüktür, yüzey durgun
            // olduğu için ışık dibe iner (kostik güçlenir). Karo türünden
            // geliyor, yani her gövde mizacını kendi taşıyor; shader'a tek tek
            // ayar geçirmeye gerek kalmıyor.
            p[2] = wmask_agitation(id);
            // A = KUZEY KIYI YAKINLIĞI (kıyı gölgesi için). Kamera 3/4 eğik
            // (r2d.h ışık bölümü: ışık lekeleri bu yüzden basık elips) — eğik
            // bakışta gölet zemine açılmış bir ÇUKURDUR: kuzey kıyısının iç
            // duvarı görünür ve suya gölge düşürür. Bu kanal "hemen kuzeyimde
            // kara var mı, kaç teksel ötede" diyor; shader bunu yönlü gölgeye
            // çeviriyor. Çaprazlar yarım ağırlıkta: köşeler de payını alsın
            // ("köşelerin açıya göre gölgesi"), yoksa gölge dik kenarda bıçakla
            // kesilmiş gibi bitiyor.
            {
                float bank = 0.0f;
                for (int dy = 1; dy <= WMASK_BANK_REACH; dy++) {
                    int ny = y - dy;
                    float wgt = (float)(WMASK_BANK_REACH + 1 - dy)
                              / (float)WMASK_BANK_REACH;
                    bool landUp = (ny < 0) || (wmaskComp[ny][x] == 0);
                    bool landUL = (ny < 0) || (x - dy < 0)  || (wmaskComp[ny][x - dy] == 0);
                    bool landUR = (ny < 0) || (x + dy >= W) || (wmaskComp[ny][x + dy] == 0);
                    if (landUp && wgt > bank)          bank = wgt;
                    if (landUL && wgt * 0.6f > bank)   bank = wgt * 0.6f;
                    if (landUR && wgt * 0.6f > bank)   bank = wgt * 0.6f;
                    if (bank >= 1.0f) break;
                }
                p[3] = (Uint8)(bank * 255.0f + 0.5f);
            }
        }
    *outW = W;
    *outH = H;
    *outPix = wmaskPix;
    return true;
}

// SU ÜSTÜ ÖRTÜSÜ (Faz SU 2): nilüferler — su geçişinden SONRA çizilir, yani
// kırılmadan ve süzgeçten ETKİLENMEZLER: yüzeyin ÜSTÜNDE dururlar (referans
// görüntünün imzası). Önce 3 px güneydoğuya koyu temas gölgesi, sonra pad —
// gölge, pad'i yüzeye "oturtan" şey. Deterministik hash; save'e girmez.
void tilemap_render_water_overlay(R2D *renderer, const Camera *camera)
{
    if (uwBushTex[1] == NULL) return;
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

    for (int row = rowT; row < rowB; row++)
        for (int col = colL; col < colR; col++) {
            if (tilemap[row][col] != 20) continue;   // nilüfer gölete özgü
            unsigned h = (unsigned)(col * 2654435761u ^ row * 40503u);
            h = (h >> 11) ^ h;
            // Serpiştirmeden FARKLI zar (h>>17): pad batık bitkinin üstüne binmesin
            if (((h >> 17) % 100) >= 7) continue;
            // Kıyı çizgisine binmesin: dört komşu da ıslak olmalı
            if (!tilemap_is_water(col-1, row) || !tilemap_is_water(col+1, row) ||
                !tilemap_is_water(col, row-1) || !tilemap_is_water(col, row+1))
                continue;
            float px = col * TILE_SIZE + (float)((h >> 3) % 40);
            float py = row * TILE_SIZE + (float)((h >> 8) % 40);
            R2DTexture *bt = uwBushTex[(h >> 5) & 1];
            SDL_FRect sh = { (px + 3.0f - camera->x) * zoom,
                             (py + 4.0f - camera->y) * zoom,
                             20.0f * zoom, 20.0f * zoom };
            r2d_texture_set_color_mod(bt, 14, 30, 48);
            r2d_texture_set_alpha_mod(bt, 120);
            r2d_draw_texture(renderer, bt, NULL, &sh);
            r2d_texture_set_alpha_mod(bt, 255);
            SDL_FRect pd = { (px - camera->x) * zoom, (py - camera->y) * zoom,
                             20.0f * zoom, 20.0f * zoom };
            r2d_texture_set_color_mod(bt, 112, 188, 108);   // taze yaprak yeşili
            r2d_draw_texture(renderer, bt, NULL, &pd);
            r2d_texture_set_color_mod(bt, 255, 255, 255);
        }
}

static int wld_water_frame(void)
{
    // Anahtar 1/3'te kare 0'a çivilenir: asset animasyonu DONAR, kıyı yine çizilir
    if (wldWaterFrames <= 1 || !vfx_asset_flow()) return 0;
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
    uwBushTex[0] = load_texture(renderer, "assets/decor/bush_1.png");
    uwBushTex[1] = load_texture(renderer, "assets/decor/bush_2.png");
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
    for (int i = 0; i < 2; i++)
        if (uwBushTex[i]) { r2d_texture_destroy(NULL, uwBushTex[i]); uwBushTex[i] = NULL; }
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
                    // YALNIZ id 2: kıyı atlası DENENDİ ve gölete parlak mavi
                    // köpük halkası getirdi ('köşelerdeki köpükleri
                    // kaldıracaktık' — Hakan). Atlas köpüklü/dalgalı kıyı
                    // sanatı; deniz/göl kenarında doğru, durgun gölette yanlış.
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

// ===================================================================
//  "SU OLMADAN SU" — ŞERİT KAYDIRMA DEMOSU (brief C.1 Katman 2 / B.1)
// ===================================================================
// NİYE VAR: kıyıda görülen hareketin kaynağı belirsizdi (asset animasyonu mu,
// bizim kodumuz mu). Bu alan ASSET'İ DENKLEMDEN ÇIKARIYOR — çizilen tek doku
// TAŞ (id 3), assets/Tiles/Water/* hiç kullanılmıyor. Yani burada görülen her
// hareket kesinlikle bu fonksiyondan geliyor.
//
// TEKNİK: 8/16-bit donanımının satır kaydırma (line scroll / HDMA) hilesi.
// Karo tek quad olarak değil YATAY ŞERİTLER halinde çizilir; her şeridin
// yatay konumu, şeridin DÜNYA y'sine ve zamana bağlı iki sinüsün toplamıyla
// kaydırılır. Altındaki taş "suyun altında bükülüyor" gibi okunur.
//
// NEDEN hedef (dst) x kaydırılıyor da kaynak (src) x DEĞİL: sampler'ımızın iki
// modu da CLAMP_TO_EDGE (r2d.c) ve karo dokusu 16 px. src'yi karo içinde
// kaydırmak kenar sütunlarını kırpar → her karo kendi kenarını uzatır ve
// IZGARA GÖRÜNÜR. Satırın tamamını kaydırıp havuz kenarında kırpmak donanım
// hilesinin birebir kendisi ve bu kusuru taşımıyor. Taban pass'i (tilemap_render)
// kaydırılmamış taşı zaten çizdiği için kenardaki boşluk yine taş gösterir.
// RAFA KALDIRILDI (Faz SU) — silinmedi. Bu şerit-kaydırma demosu "su asseti
// olmadan su" sorusunu cevaplamak için yazıldı ve cevabı verdi: shader'sız
// katmanlarla suyun ALTINI bükemiyorsun, yalnız örtebiliyorsun. Yerine gerçek
// su hacmi geçişi geldi (shaders/water.frag.hlsl + r2d_water_flush).
// Kod çalışmıyor çünkü VFXD_TILE hâlâ taş (3), havuz ise artık sualtı taban
// (20). Kasıtlı: teknik hâlâ geçerli ve gerekirse geri döner — ısı bükülmesi
// (demircinin ocağı) tam olarak bu şerit hilesinin dikey ve maskeli hâli.
#define VFXD_COL0 3     // havuzun karo kutusu (dünya haritası, köy güneyi)
#define VFXD_COL1 10
#define VFXD_ROW0 19
#define VFXD_ROW1 24
#define VFXD_TILE 3     // TAŞ. Su asset'i yok — demonun bütün anlamı bu.

// Karo başına şerit sayısı. 8 → şerit 16×2 doku pikseli, 8 dünya px yüksek.
// Artırmak (16) dalgayı yumuşatır ama maliyeti iki katına çıkarır; azaltmak (4)
// dalgayı basamaklandırır ve su yerine "kayan bloklar" görünür.
#define VFXD_STRIPS 8
// Bir sanat pikseli kaç dünya pikseli: 16 px karo sanatı TILE_SIZE 64'e basılıyor.
#define VFXD_ART    ((float)TILE_SIZE / 16.0f)
// Tepe kayma, SANAT PİKSELİ cinsinden. 2 → ±8 dünya px (karonun 1/8'i).
// Artırınca taş "kopuk" görünür ve kenardaki kırpma boşluğu büyür; 1'e indirince
// snap yüzünden ofset yalnız -1/0/1 değerini alır ve hareket neredeyse kaybolur.
#define VFXD_AMP_ART 2.0f
// İki dalga sayısı (dünya px başına radyan) → dalga boyu 2pi/k.
// K1 ≈ 90 px (1.4 karo), K2 ≈ 39 px (0.6 karo). İKİ FREKANS ŞART: tek frekans
// "akordeon" gibi durur (brief B.1). Oran 2.33 harmonik değil — desen gözle
// tekrar etmiyor. Büyütmek dalgayı sıkıştırır (titrek), küçültmek düzleştirir.
#define VFXD_K1 0.070f
#define VFXD_K2 0.163f
// Dalga hızları (rad/sn), İKİNCİSİ TERS YÖNDE: karşı karşıya iki dalga durgun
// suyun girişim desenini verir (tek yön akıntı gibi görünür — göl için yanlış).
// Artırınca su "kaynıyor", azaltınca donuk kalır.
#define VFXD_S1 2.1f
#define VFXD_S2 3.7f
// Ağırlıklar toplamı 1.0 → genlik tavanı tam olarak VFXD_AMP_ART kalır.
#define VFXD_W1 0.62f
#define VFXD_W2 0.38f
// DERİNLİK KATMANI: yarı saydam mavi-camgöbeği tül. Alpha'yı artırınca taş
// dokusu kaybolur (kırılma da görünmez olur — demo anlamını yitirir), azaltınca
// alan "ıslak taş" gibi durur, "su" gibi durmaz.
#define VFXD_TINT_R 34
#define VFXD_TINT_G 92
#define VFXD_TINT_B 120
#define VFXD_TINT_A 108
// YÜZEY YANSIMASI: gezen parlaklık bandı (ADD). İnce (1 sanat px), yavaş, alpha
// düşük. İki bant: tek bant havuzu geçtikten sonra uzun bir "ölü" süre bırakıyor.
// Alpha'yı artırınca bant floresan bir çizgiye döner; hız artınca "tarama
// çizgisi" gibi okunur (su değil, CRT).
#define VFXD_SHEEN_BANDS 2
#define VFXD_SHEEN_SPD   26.0f   // dünya px/sn — havuzu ~15 sn'de geçer
#define VFXD_SHEEN_A     34
#define VFXD_SHEEN_WOB   1.0f    // bandın dikey kırılması, sanat px (0 = cetvel gibi düz)
#define VFXD_SHEEN_K     0.100f  // kırılmanın dalga sayısı (dünya px başına radyan)
#define VFXD_SHEEN_S     0.9f    // kırılmanın hızı (rad/sn)

// Havuz hücresi mi? Kutunun İÇİ **ve** karo gerçekten taş olmalı — böylece
// sabitler CSV'den kayarsa efekt sessizce hiçbir şey yapmaz, çimenin üstüne
// su boyamaz.
static inline bool vfxd_pool(int col, int row)
{
    if (col < VFXD_COL0 || col > VFXD_COL1) return false;
    if (row < VFXD_ROW0 || row > VFXD_ROW1) return false;
    if (col < 0 || col >= mapW || row < 0 || row >= mapH) return false;
    return tilemap[row][col] == VFXD_TILE;
}

void tilemap_vfxdemo_render(R2D *renderer, R2DTexture **tileTextures,
                            const Camera *camera)
{
    if (!vfx_strip_demo()) return;
    R2DTexture *stone = (tileTextures != NULL) ? tileTextures[VFXD_TILE] : NULL;
    if (stone == NULL) return;   // doku yoksa demo yok (taban pass'i düz renk basar)

    const float zoom = camera->zoom;
    int outW = 0, outH = 0;
    r2d_output_size(renderer, &outW, &outH);

    // KABA GÖRÜNÜRLÜK: havuz 8×6 karo. Karo karo culling'e gerek yok (hepsi bir
    // ekrana sığıyor) ama ekranda hiç yoksa tek çizim bile yapmamak gerek.
    const float poolL = VFXD_COL0 * (float)TILE_SIZE;
    const float poolT = VFXD_ROW0 * (float)TILE_SIZE;
    const float poolR = (VFXD_COL1 + 1) * (float)TILE_SIZE;
    const float poolB = (VFXD_ROW1 + 1) * (float)TILE_SIZE;
    if (poolR < camera->x || poolL > camera->x + outW / zoom ||
        poolB < camera->y || poolT > camera->y + outH / zoom)
        return;

    // DUVAR SAATİ — wld_water_frame ile aynı gerekçe: su geceleyin yavaşlamaz,
    // hit-stop onu durdurmaz, mevsim dondurmaz.
    const float t  = (float)SDL_GetTicks() / 1000.0f;
    const float sh = (float)TILE_SIZE / (float)VFXD_STRIPS;   // şerit yüksekliği (dünya px)
    const float srcSh = 16.0f / (float)VFXD_STRIPS;           // ... doku px karşılığı

    r2d_set_blend(renderer, R2D_BLEND_ALPHA);

    // === KATMAN 1: KIRILMA — kaydırılmış taş şeritleri ===
    for (int row = VFXD_ROW0; row <= VFXD_ROW1; row++) {
        // Satırın havuz koşusu: kaydırma bu aralığın DIŞINA taşmayacak
        int c0 = -1, c1 = -1;
        for (int c = VFXD_COL0; c <= VFXD_COL1; c++)
            if (vfxd_pool(c, row)) { if (c0 < 0) c0 = c; c1 = c; }
        if (c0 < 0) continue;
        const float runL = c0 * (float)TILE_SIZE;
        const float runR = (c1 + 1) * (float)TILE_SIZE;

        for (int s = 0; s < VFXD_STRIPS; s++) {
            const float wy = row * (float)TILE_SIZE + (s + 0.5f) * sh;   // şeridin dünya y'si
            const float d  = SDL_sinf(wy * VFXD_K1 + t * VFXD_S1) * VFXD_W1
                           + SDL_sinf(wy * VFXD_K2 - t * VFXD_S2) * VFXD_W2;
            // SNAP — pixel-art'ın şartı (brief B.1/B.10). Ofset TAM sanat
            // pikseline yuvarlanıyor; kesirli ofset NEAREST örneklemede 1→2→2
            // diye ZIPLAR (su dalgalanmıyor, TİTRİYOR görünür). Ölçek DÜNYA
            // biriminde: zoom sonra çarpılıyor, yani yakınlaşınca ofset sanat
            // ızgarasında kalır, ekran ızgarasına kaymaz.
            const float offW = SDL_roundf(d * VFXD_AMP_ART) * VFXD_ART;

            for (int c = c0; c <= c1; c++) {
                if (!vfxd_pool(c, row)) continue;
                float dx = c * (float)TILE_SIZE + offW, dw = (float)TILE_SIZE;
                float sx = 0.0f, sw = 16.0f;
                // Havuz kenarına kırp; src DÜNYA→DOKU oranıyla (VFXD_ART) birlikte
                // daralır — ofset tam sanat pikseli olduğu için bu kırpma da tam.
                if (dx < runL)       { float k = runL - dx;      dx += k; dw -= k; sx += k / VFXD_ART; sw -= k / VFXD_ART; }
                if (dx + dw > runR)  { float k = dx + dw - runR;          dw -= k;                     sw -= k / VFXD_ART; }
                if (dw <= 0.5f) continue;

                SDL_FRect src = { sx, s * srcSh, sw, srcSh };
                SDL_FRect dst = { (dx - camera->x) * zoom,
                                  (row * (float)TILE_SIZE + s * sh - camera->y) * zoom,
                                  dw * zoom, sh * zoom };
                r2d_draw_texture(renderer, stone, &src, &dst);
            }
        }
    }

    // === KATMAN 2: DERİNLİK — karo başına tek mavi tül ===
    // Karo başına (şerit başına değil): tül suyun GÖVDESİ, dalga onun ALTINDA
    // yaşıyor. Şerit başına çizmek quad sayısını 8 katına çıkarır ve tülün
    // kendisi dalgayla birlikte kayar — bu istenmiyor: kayması gereken şey
    // suyun altındaki taş, suyun rengi değil.
    r2d_set_color(renderer, VFXD_TINT_R, VFXD_TINT_G, VFXD_TINT_B, VFXD_TINT_A);
    for (int row = VFXD_ROW0; row <= VFXD_ROW1; row++)
        for (int col = VFXD_COL0; col <= VFXD_COL1; col++) {
            if (!vfxd_pool(col, row)) continue;
            SDL_FRect dst = { (col * (float)TILE_SIZE - camera->x) * zoom,
                              (row * (float)TILE_SIZE - camera->y) * zoom,
                              TILE_SIZE * zoom, TILE_SIZE * zoom };
            r2d_fill_rect(renderer, &dst);
        }

    // === KATMAN 3: YÜZEY YANSIMASI — gezen parlaklık bandı ===
    // Bandı dalga ofseti BÜKMÜYOR, bu bilinçli: yansıma yüzeyde olur, kırılma
    // suyun altında. Bandın kendi kırılması sütun başına ayrı (yoksa cetvelle
    // çizilmiş bir çizgi gibi durur).
    r2d_set_blend(renderer, R2D_BLEND_ADD);
    r2d_set_color(renderer, 210, 240, 255, VFXD_SHEEN_A);
    const float span = poolB - poolT;
    for (int b = 0; b < VFXD_SHEEN_BANDS; b++) {
        const float by = poolT + SDL_fmodf(t * VFXD_SHEEN_SPD
                                           + b * span / (float)VFXD_SHEEN_BANDS, span);
        for (int col = VFXD_COL0; col <= VFXD_COL1; col++) {
            const float cx  = (col + 0.5f) * (float)TILE_SIZE;
            const float wob = SDL_roundf(SDL_sinf(cx * VFXD_SHEEN_K + t * VFXD_SHEEN_S)
                                         * VFXD_SHEEN_WOB) * VFXD_ART;
            // Sanat ızgarasına çivile (aynı snap kuralı)
            const float y   = SDL_floorf((by + wob) / VFXD_ART) * VFXD_ART;
            if (!vfxd_pool(col, (int)(y / (float)TILE_SIZE))) continue;
            SDL_FRect dst = { (col * (float)TILE_SIZE - camera->x) * zoom,
                              (y - camera->y) * zoom,
                              TILE_SIZE * zoom, VFXD_ART * zoom };
            r2d_fill_rect(renderer, &dst);
        }
    }

    // Durumu varsayılana geri bırak — sonraki çizimler bu ayarları miras alıyor
    r2d_set_blend(renderer, R2D_BLEND_ALPHA);
    r2d_set_color(renderer, 255, 255, 255, 255);
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

    // BALIK GÖLGELERİ — maske kurulurken gövde başına GARANTİLİ seçilen
    // çapalar (bkz. wld_water_mask_fill 3b). Çapa etrafında yavaş yörünge;
    // su geçişinden ÖNCE çizildiği için kırılma+süzgeç üstüne biner.
    // İlk sürüm karo başına zardı ve gölette beklenen balık sayısı ~0 çıktı.
    if (uwBushTex[0] != NULL) {
        float ftm = (float)SDL_GetTicks() / 1000.0f;
        for (int fi = 0; fi < wmaskFishN; fi++) {
            unsigned fh = (unsigned)(fi * 2654435761u); fh = (fh >> 11) ^ fh;
            float dir = (fh & 4) ? 1.0f : -1.0f;
            float ang = ftm * 0.40f * dir + (float)(fh & 1023);
            float fxp = wmaskFishXY[fi][0] + SDL_cosf(ang) * 30.0f;
            float fyp = wmaskFishXY[fi][1] + SDL_sinf(ang) * 20.0f;
            if (fxp < (colL - 1) * TILE_SIZE || fxp > (colR + 1) * TILE_SIZE ||
                fyp < (rowT - 1) * TILE_SIZE || fyp > (rowB + 1) * TILE_SIZE)
                continue;   // ekran dışı
            // Gövde + geride küçük kuyruk lekesi: tek yuvarlak leke bitkiyle
            // karışıyordu ("balık göremiyorum"); kuyruk, yüzme yönünü okutur.
            float hx = -SDL_sinf(ang) * dir, hy = SDL_cosf(ang) * dir * 0.66f;
            R2DTexture *fb = uwBushTex[0];
            r2d_texture_set_color_mod(fb, 20, 32, 56);
            r2d_texture_set_alpha_mod(fb, 190);
            SDL_FRect fd = { (fxp - 11.0f - camera->x) * zoom,
                             (fyp - 6.0f - camera->y) * zoom,
                             22.0f * zoom, 12.0f * zoom };
            r2d_draw_texture(renderer, fb, NULL, &fd);
            SDL_FRect ft = { (fxp - 5.0f - hx * 14.0f - camera->x) * zoom,
                             (fyp - 3.5f - hy * 14.0f - camera->y) * zoom,
                             10.0f * zoom, 7.0f * zoom };
            r2d_draw_texture(renderer, fb, NULL, &ft);
            r2d_texture_set_alpha_mod(fb, 255);
            r2d_texture_set_color_mod(fb, 255, 255, 255);
        }
    }

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
            // SUALTI TABANI (Faz SU): şeffaflık hissi altta TANINACAK bir şey
            // ister — çıplak taş tabanda su ne kadar berrak olsa da "altı
            // görünüyor" okuması çıkmaz (Hakan'ın "içine bitki yerleştir"
            // önerisi). Serpiştirme su geçişinden ÖNCE çizildiği için kırılma
            // ve derinlik bantları üstlerine kendiliğinden biner: yosun suyla
            // bükülür, derindeki taş kararır — şeffaflığı KANITLAYAN nesneler.
            // Kıyıya yakın yoğun, ortada seyrek: gerçek gölette bitki sığlıkta.
            else if ((id == 20 || id == 2)
                     && uwBushTex[0] != NULL && uwBushTex[1] != NULL) {
                bool nearLand = (col > 0 && tilemap[row][col-1] != id) ||
                                (col < mapW-1 && tilemap[row][col+1] != id) ||
                                (row > 0 && tilemap[row-1][col] != id) ||
                                (row < mapH-1 && tilemap[row+1][col] != id);
                // Göl/deniz sazı korunur: raftaki taş/bitkiden ÖNCE zar atar
                if (id == 2 && waterDecTex != NULL && nearLand && (h % 100) >= 90) {
                    SDL_FRect src = { (float)((h >> 6) % 3) * 16.0f, 0, 16, 16 };
                    r2d_draw_texture(renderer, waterDecTex, &src, &dst);
                }
                else {
                // Batık taş rafı + bitkiler. Gölet ortası da payını alıyor
                // ('içine de taş ve bitkiler ekleyip doğallaştıralım' —
                // Hakan): örtme 0.80'e indiği için orta artık siluet gösterir.
                // BALIK GÖLGESİ: 3×3'ü tamamen ıslak seyrek çapalarda, çapa
                // çevresinde yavaşça dolanan koyu leke. Su geçişinden ÖNCE
                // çizildiği için kırılma+süzgeç üstüne biner: "suyun içinde
                // bir şey yüzüyor" okuması bedavaya çıkar (Stardew usulü).
                // DUVAR saati: balık gece durmaz.
                R2DTexture *bt = uwBushTex[(h >> 5) & 1];
                unsigned roll = h % 100;
                unsigned pPlant = nearLand ? 14 : (id == 20 ? 12 : 3);
                unsigned pStone = nearLand ? 42 : (id == 20 ?  9 : 3);
                if (roll < pStone) {
                    r2d_texture_set_color_mod(bt, 104, 110, 126);
                    r2d_draw_texture(renderer, bt, NULL, &dst);
                    r2d_texture_set_color_mod(bt, 255, 255, 255);
                } else if (roll < pStone + pPlant) {
                    r2d_texture_set_color_mod(bt, 96, 148, 132);
                    r2d_draw_texture(renderer, bt, NULL, &dst);
                    r2d_texture_set_color_mod(bt, 255, 255, 255);
                }
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
