#include <stdio.h>
#include <stdlib.h>
#include "tilemap.h"

#define FEET_H 20.0f   // ayak kutusunun yüksekliği
#define FEET_INSET 8.0f // yanlardan içeri ne kadar daraltılsın

// --- Tile tablosu: gerçek tanım burada ---
const TileType TILE_TYPES[] = {
    // EPIC RPG geçişi (Faz EPIC-1, 2026-09-20): çim+yol tabanı ERW 32px karo
    // (scripts/bake_erw_terrain.py türetir; kaynak paket repo DIŞINDA).
    // Cute Fantasy yolları git geçmişinde — eski görünüşe dönüş yol değişikliği.
    [0] = { .solid = false, .r = 50,  .g = 200, .b = 50,  .texturePath = "assets/Tiles/erw/grass_1.png", .speedMultiplier = 1.0f, .damage = 0.0f, .conceals = true }, // çimen — eğilince içine girilir
    [1] = { .solid = false, .r = 150, .g = 100, .b = 50,  .texturePath = "assets/Tiles/erw/path.png", .speedMultiplier = 1.0f, .damage = 0.0f }, // yol
    [2] = { .solid = true,  .r = 50,  .g = 50,  .b = 200, .texturePath = "assets/Tiles/erw/water.png", .speedMultiplier = 1.0f, .damage = 0.0f }, // su
    [3] = { .solid = true,  .r = 110, .g = 110, .b = 120, .texturePath = "assets/Tiles/erw/stone.png", .speedMultiplier = 1.0f, .damage = 0.0f },  // taş
    [4] = { .solid = false, .r = 90, .g = 110, .b = 70, .texturePath = "assets/Tiles/erw/swamp.png", .speedMultiplier = 0.4f, .damage = 0.0f },  // bataklık: yavaşlatır
    [5] = { .solid = false, .r = 200, .g = 60, .b = 40, .texturePath = "assets/Tiles/erw/thorns.png", .speedMultiplier = 1.0f, .damage = 20.0f },  // diken: saniyede 20 hasar

    // --- Zindan tile'lari (Cute_Fantasy Dungeons, 16px kirpimlar) ---
    [6] = { .solid = false, .r = 60,  .g = 62,  .b = 88,  .texturePath = "assets/Tiles/erw/d1_floor.png", .speedMultiplier = 1.0f, .damage = 0.0f }, // K1 doseme
    [7] = { .solid = true,  .r = 95,  .g = 100, .b = 130, .texturePath = "assets/Tiles/erw/d1_wall.png",  .speedMultiplier = 1.0f, .damage = 0.0f }, // K1 duvar
    [8] = { .solid = false, .r = 70,  .g = 72,  .b = 100, .texturePath = "assets/Tiles/erw/d2_floor.png", .speedMultiplier = 1.0f, .damage = 0.0f }, // K2 doseme
    [9] = { .solid = true,  .r = 105, .g = 110, .b = 140, .texturePath = "assets/Tiles/erw/d2_wall.png",  .speedMultiplier = 1.0f, .damage = 0.0f }, // K2 duvar
    [10] = { .solid = true, .r = 7,   .g = 7,   .b = 13,  .texturePath = NULL, .speedMultiplier = 1.0f, .damage = 0.0f },  // void: oda disi karanlik (duz renk)

    // --- Lanetli Mezarlık (Faz HG-B — undead paleti, açık hava zindanı) ---
    [12] = { .solid = false, .r = 111, .g = 115, .b = 106, .texturePath = "assets/Tiles/erw/mz_ground.png", .speedMultiplier = 1.0f, .damage = 0.0f },  // ölü toprak
    [13] = { .solid = true,  .r = 57,  .g = 31,  .b = 33,  .texturePath = NULL, .speedMultiplier = 1.0f, .damage = 0.0f },  // mezarlık kayası (üstü koyu — doku dual-grid'den)
    [14] = { .solid = true,  .r = 26,  .g = 32,  .b = 46,  .texturePath = "assets/Tiles/erw/mz_water.png",  .speedMultiplier = 1.0f, .damage = 0.0f },  // ölü su
    // Doğu çölü (Taşdurak — Faz DOGU)
    [15] = { .solid = false, .r = 222, .g = 180, .b = 120, .texturePath = "assets/Tiles/erw/sand.png",  .speedMultiplier = 1.0f, .damage = 0.0f },  // kum
    [16] = { .solid = true,  .r = 160, .g = 110, .b = 70,  .texturePath = "assets/Tiles/erw/rock.png",  .speedMultiplier = 1.0f, .damage = 0.0f },  // çöl kayası
    [17] = { .solid = true,  .r = 70,  .g = 190, .b = 200, .texturePath = "assets/Tiles/erw/oasis_water.png", .speedMultiplier = 1.0f, .damage = 0.0f },  // vaha suyu

    // Mağara (Loca v2 — Faz MAPGEN-2): düz renk tabanlar, doku dual-grid
    // ring/yüz parçalarından gelir (Cave_Walls atlası)
    [18] = { .solid = false, .r = 138, .g = 72,  .b = 54,  .texturePath = "assets/Tiles/erw/cave_floor.png", .speedMultiplier = 1.0f, .damage = 0.0f },  // mağara zemini
    [19] = { .solid = true,  .r = 225, .g = 163, .b = 122, .texturePath = "assets/Tiles/erw/cave_wall.png", .speedMultiplier = 1.0f, .damage = 0.0f },  // mağara duvarı (üstü ten)

    // SUALTI TABAN (Faz SU): suyun ALTINDA kalan yürünebilir kaya. Dokusu taş —
    // yani bu karo su asseti KULLANMIYOR. Sualtı görünümünün tamamı su hacmi
    // geçişinden geliyor (shaders/water.frag.hlsl): kaya bükülüyor, derinlik
    // gradyanı üstüne biniyor, kostik ağı üstünde geziyor. Katı DEĞİL: sığlık,
    // içine girilir ama su ayağı ağırlaştırır.
    [20] = { .solid = false, .r = 96,  .g = 104, .b = 112, .texturePath = "assets/Tiles/erw/stone.png", .speedMultiplier = 0.72f, .damage = 0.0f },  // sualtı taban (sığlık)
    // SAF ERW SUYU (Hakan karşılaştırma göleti, 2026-09-20): su shader'ına
    // GİRMEZ (wmask_base default 0) — görünüm %100 paket asset'i: animasyonlu
    // taban + vaha kıyı atlası. Karar sonrası ya kaldırılır ya da göletlerin
    // yeni dili olur.
    [21] = { .solid = true,  .r = 60,  .g = 140, .b = 180, .texturePath = NULL, .speedMultiplier = 1.0f, .damage = 0.0f },  // saf ERW su (test göleti)
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
// çizilir. ERW GEÇİŞİ (Faz EPIC-1): eski 12 hücreli grass_edges.png overlay'i
// (tile-merkezli, edgeTex) EMEKLİ — yerine maske-sıralı iki ERW dual-grid
// atlası geldi (erw_path_pass / erw_grass_fringe_pass, dsr_dual_pass modeli).
// Eski sistemin kodu git geçmişinde (bu satırın blame'i).
// Yalnız DÜNYA tile'larına taşar (yol/su/bataklık/diken) — zindan dokunulmaz.
static R2DTexture *grassVar[4] = {0};   // [0] ana doku (tileTextures'tan gelmez — kendi kopyası)
static R2DTexture *waterAnimTex = NULL; // ERW su animasyonu (8 kare 32px — deniz/gölet/vaha tabanı)
static R2DTexture *pathEdgeTex = NULL;  // ERW toprak blob'u — maske-sıralı 4×4, 32px hücre
static R2DTexture *grassEdgeTex = NULL; // ERW çim saçağı blob'u — aynı düzen
static R2DTexture *flowerTex = NULL;    // erw/scatter_atlas.png (10×3 hücre 32px)
static R2DTexture *pathDecTex = NULL;   // erw/pathdec_atlas.png (3 hücre 32px)
static R2DTexture *uwBushTex[2] = { NULL, NULL };  // bush_1/2 — sualtı bitkisi+taşı (renk moduyla)
static R2DTexture *mzGround2 = NULL;    // mezarlık zemin 2. varyantı
static R2DTexture *dsrWaterTex = NULL;  // çöl vaha atlası (dual-grid 16 parça)
static R2DTexture *dsrCliffTex = NULL;  // çöl kaya atlası (dual-grid 16 parça)
static R2DTexture *caveRingTex = NULL;  // ERW loca duvar ringi (maske-sıralı 4×4, 32px)
static R2DTexture *mzRockTex = NULL;    // ERW mezarlık kaya ringi (aynı düzen)
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
// G kanalının doyduğu yarım karo sayısı. 6 (=3 karo) iken deniz sırtları
// 3 karodan uzakta DONUYORDU: sırt konumu bu alandan türüyor, alan doyunca
// desen sabitleniyor ve dalgalar kıyıya yakın bir şeritte hapsoluyordu
// (Hakan: "denizin görünen ucundan gelmeleri gerekiyordu"). 24 = 12 karo,
// yani görünen denizin tamamı. Shader'daki SHORE_PX/SHORE_LINE bu sayıdan
// TÜRETİLİYOR (1 sanat px = 1/(8*HT), su çizgisi = 1/(2*HT)) — ikisi
// birlikte değişmek zorunda, yoksa bütün kıyı bantları kayar.
#define WMASK_SHORE_HT 24        // =12 karo
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
        // ÇOK DÜŞÜK ÇIRPINTI (<0.05 → ham 12) SÖZLEŞME: "bu gövdenin KENDİ
        // kıyı sanatı var, shader kıyı bantları çizmesin". Vaha kendi
        // sanatçı atlasını (dsrWaterTex) taşıyor; shader'ın koyu bandı +
        // menisküsü + kıyı gölgesi onun üstüne binince yapay lacivert
        // şeritler çıkıyordu (Hakan). Kıyı KESİTİ de aynı sebeple kapalı.
        case 17: return 12;    // vaha: dingin + kendi kıyı sanatı
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

float tilemap_water_agitation(int col, int row)
{
    if (col < 0 || col >= mapW || row < 0 || row >= mapH) return 0.0f;
    return wmask_agitation(tilemap[row][col]) / 255.0f;
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

// KUMSAL DALGA VURUŞU (swash) — gerçek deniz/okyanus kıyısının imzası:
// dalga kuma TIRMANIYOR, ıslak bir dil bırakıyor, sonra geri çekiliyor.
// NEDEN SHADER'DA DEĞİL: ıslanan yer KUM, yani KARA. Su shader'ı yalnız
// ıslak karoları boyuyor ve maskenin kara tarafındaki rampası ancak 4 sanat
// px sürüyor — dalganın 1 karo içeri girmesi oradan çıkmaz. Burası kara
// üstü ayrı geçiş: sudan SONRA çizilir, kumun üstüne biner.
//
// HAREKET KIYIYA DİK: reach (tırmanma mesafesi) zamanla salınıyor, yani su
// yukarı-aşağı gidip geliyor. Kıyı boyunca faz kayması var (dünya x'ine
// bağlı) — yoksa bütün sahil aynı anda ıslanır ve cetvel gibi durur.
// Salınım asimetrik (ikinci harmonik): hızlı vuruş, yavaş çekilme.
#define SWASH_REACH_MIN  10.0f   // dünya px — en az bu kadar ıslak dil kalır
#define SWASH_REACH_AMP  46.0f   // tırmanma genliği (≈0.7 karo)
#define SWASH_STEP        4.0f   // 1 sanat px = 4 dünya px (kuantalama)
#define SWASH_COLW       12.0f   // sütun şeridi genişliği (3 sanat px)
#define SWASH_MEM       256      // sütun hafızası (2'nin kuvveti — maskeleme)
#define SWASH_DRY       26.0f    // kumun kuruma hızı (dünya px/sn)
// DALGA ZAMANLAMASI. HATA GEÇMİŞİ: setler döngünün yalnız kendi diliminde
// çiziliyordu (sp < 0.34 kapısı), yani ekranda HER AN TEK dalga vardı —
// "ard arda gelmiyorlar" (Hakan). Doğrusu: her set TÜM yolu kendi
// zamanında kat eder, setler birbirinden ARRIVE kadar gecikmeli başlar;
// böylece N sırt aynı anda, farklı uzaklıkta görünür.
#define SWASH_TRAVEL    19.0f    // bir sırtın açıktan kıyıya yolculuğu (sn)
#define SWASH_SETS         6     // aynı anda yolda olan sırt sayısı
#define SWASH_ARRIVE   (SWASH_TRAVEL / (float)SWASH_SETS)   // varış aralığı
// KUMSAL DÖNGÜSÜ deniz yolculuğundan AYRI: 6.5 sn. Eskiden ARRIVE'a
// (3.2 sn) bağlıydı ve hızlı geliyordu; ayırınca kumsalı deniz sırtlarının
// hızını bozmadan yavaşlatabiliyoruz.
#define SWASH_PULSE      6.5f
#define SWASH_SEG_W    560.0f    // bir dalga YAYININ genişliği (≈8.7 karo)
#define SWASH_APPROACH  760.0f   // sırtın denizde katettiği yol (px) —
                                 // 150 kıyıya çok yakındı, dalga birden
                                 // beliriyordu; artık görünen denizin ucundan gelir
static float swashMem[SWASH_MEM];   // sütun başına en yüksek su izi

void tilemap_render_beach_swash(R2D *renderer, const Camera *camera)
{
    float zoom = camera->zoom;
    int outW = 0, outH = 0;
    r2d_output_size(renderer, &outW, &outH);
    float wx0 = camera->x, wx1 = camera->x + outW / zoom;
    float wy0 = camera->y, wy1 = camera->y + outH / zoom;

    Uint64 nowMs = SDL_GetTicks();
    float t = SDL_floorf((float)nowMs / 125.0f) / 8.0f;   // 8 fps kuantalı
    static Uint64 prevMs = 0;
    float dt = (prevMs == 0) ? 0.0f : (float)(nowMs - prevMs) / 1000.0f;
    if (dt > 0.1f) dt = 0.1f;
    prevMs = nowMs;

    for (float sx = SDL_floorf(wx0 / SWASH_COLW) * SWASH_COLW; sx < wx1; sx += SWASH_COLW) {
        int col = (int)(sx / TILE_SIZE);
        if (col < 0 || col >= mapW) continue;
        int r0 = (int)(wy0 / TILE_SIZE) - 1, r1 = (int)(wy1 / TILE_SIZE) + 1;
        if (r0 < 0) r0 = 0;
        if (r1 >= mapH) r1 = mapH - 1;
        int wet = -1;
        for (int r = r0; r <= r1; r++)
            if (tilemap_is_water(col, r)) { wet = r; break; }
        if (wet <= r0) continue;
        if (tilemap_is_water(col, wet - 1)) continue;
        if (tilemap[wet - 1][col] != 15) continue;   // yalnız KUM kıyısı
        float shoreY = wet * (float)TILE_SIZE;

        // ── ÜÇ EVRELİ DÖNGÜ: dalga DENİZDEN gelir, kuma tırmanır, çekilir.
        // Faz kayması x'e göre SİNÜZOİDAL (monoton DEĞİL): kıyı boyunca
        // varyasyon olur ama net yana yürüme olmaz. HATA GEÇMİŞİ: faz
        // x ile doğrusal artınca dalga sahil boyunca yürüyor gibi görünüyor.
        // İki ayrı saat: (a) KUM tarafı varış darbesi — her SWASH_ARRIVE
        // saniyede bir sırt kıyıya varır ve swash bir kez vurur;
        // (b) DENİZ tarafı yolculuk — her sırt SWASH_TRAVEL saniyede yol alır.
        // xw: kum tarafı varış darbesine küçük mekânsal kayma (kıyı boyunca
        // aynı anda vurmasın). pt'ye UYGULANMIYOR — HATA GEÇMİŞİ: uygulanınca
        // 0.05'lik kayma 760 px'lik yolculuk mesafesiyle çarpılıp sırtı sütun
        // boyunca 40-70 px kaydırıyordu, yani her yay EĞİK bir rampa oluyordu
        // ("yaylar doğal durmuyor"). Sırt mesafesi yay içinde SABİT olmalı.
        float xw = 0.05f * SDL_sinf(sx * 0.0041f);
        float pa = t / SWASH_PULSE + xw;
        float cp = pa - SDL_floorf(pa);                    // 0..1 varış döngüsü
        // (pt = yolculuk saati; deniz sırtları shader'a taşındı, burada kullanılmıyor)
        float env = 0.70f + 0.30f * SDL_sinf(sx * 0.0093f + 1.3f);
        float reachMax = SWASH_REACH_AMP * env;

        float reach = 0.0f;    // >0 ise: KUMDA, kıyıdan bu kadar içeride
        // EVRELER YUMUŞAK EĞRİLİ (smoothstep): her evrenin girişi ve çıkışı
        // yavaş, ortası hızlı — ama evre SINIRLARINDA köşe yok. Eskiden
        // tırmanma DOĞRUSALDI ve %21'lik dar bir dilime sıkışmıştı: ortada
        // ani bir hızlanma oluyordu (Hakan). Tırmanma dilimi genişletildi.
        if (cp < 0.30f) {                       // 1) YAKLAŞMA (yavaş kabarma)
            float k = cp / 0.30f;
            reach = SWASH_REACH_MIN * (k * k * (3.0f - 2.0f * k));
        } else if (cp < 0.64f) {                // 2) TIRMANMA (geniş dilim)
            float k = (cp - 0.30f) / 0.34f;
            k = k * k * (3.0f - 2.0f * k);      // ease-in-out
            reach = SWASH_REACH_MIN + (reachMax - SWASH_REACH_MIN) * k;
        } else {                                // 3) ÇEKİLME (en yavaş)
            float k = (cp - 0.64f) / 0.36f;
            k = k * k * (3.0f - 2.0f * k);
            reach = reachMax * (1.0f - k);
        }
        reach = SDL_roundf(reach / SWASH_STEP) * SWASH_STEP;
        // SWASH ENERJİSİ: köpük ve bulanık su kendi ışığını dalganın
        // KONUMUNDAN alır. HATA GEÇMİŞİ: sabit alfa kullanılıyordu, köpük
        // su çizgisinde tam parlaklıkla BİRDEN beliriyordu (Hakan: "bir anda
        // kaybolup varoluyorlar"). Şimdi kıyı çizgisinde 0, tırmanma
        // tepesinde 1 — dalgayla birlikte doğup sönüyor, süreksizlik yok.
        float swashE = SDL_sinf(1.5708f * SDL_min(1.0f, reach / SDL_max(reachMax, 1.0f)));

        // ── DENİZ TARAFI DALGA SIRTLARI: BU GEÇİŞTEN KALDIRILDI (kod
        // silinmedi, aşağıda #if 0 içinde duruyor — yedi turun ayarları
        // orada). SEBEP MİMARİ: sırtlar su geçişinden SONRA, CPU'da
        // çiziliyordu; yani suyun renk hattından, derinlik bantlarından,
        // kıyı geometrisinden ve kırılmasından HİÇ geçmiyorlardı. Sonuç
        // "suyun üstüne yapıştırılmış opak beyaz çubuk" — ne kadar ayar
        // yapılsa da denizin parçası gibi durmuyor (Hakan). Doğru yer
        // shader'ın ARK katmanı (LB_ARC): o kıyı uzaklığı alanını izliyor,
        // dolayısıyla kıyıya kendiliğinden paralel ve suyun renk hattının
        // İÇİNDE. İlerleyen kabarma oraya taşınacak.
#if 0
        // (eski CPU sırt kodu — arşiv) Tek kabarma "cetvel çizgisi"
        // gibi duruyordu (Hakan); gerçek denizde birbirini kovalayan birkaç
        // sıra vardır. Her sıra döngünün 1/SWASH_SETS kadar GERİSİNDE, yani
        // en öndeki kıyıya varırken arkadaki henüz yolun ortasında.
        // En öndeki en parlak ve en kalın (kırılmaya en yakın).
        for (int sIdx = 0; sIdx < SWASH_SETS; sIdx++) {
            // ── SIRT BİÇİMİ — dört denemenin dersi (hepsi görsel testte düştü):
            //   1) şerit şerit bağımsız hash titremesi → sırt AYRI KARELERE dağıldı
            //   2) kaba hücre hash'i → basamaklar kopuk ve birbiriyle alakasız
            //   3) düzgün mekânsal fonksiyon → kesintisiz AMA çok uzun tek çizgi
            //   4) yay bölümlemesi + sabit mesafe → DOĞRU (bu)
            // Kıyı ≈560 px'lik YAYLARA bölünür; her yay kendi zamanında gelir,
            // kendi gücünde ve yüksekliğindedir. Yay İÇİNDE sırt mesafesi SABİT
            // (yoksa rampa olur), biçimi kıyıya PARALEL hafif kemer, uçları
            // zarfla incelerek söner. Zaman hiçbir mekânsal fonksiyona girmez
            // (girerse dalga sahil boyunca yana yürür).
            int segI = (int)SDL_floorf(sx / SWASH_SEG_W);
            unsigned sh2 = (unsigned)(segI + 4096) * 0x9E3779B9u
                         ^ (unsigned)(sIdx * 0x85EBCA6Bu + 0x2545F491u);
            sh2 ^= sh2 >> 15; sh2 *= 0x2545F491u; sh2 ^= sh2 >> 13;
            if ((sh2 & 7u) == 0u) continue;                 // ~%12 yay hiç gelmez
            float segPh = (((float)((sh2 >> 3) & 31u) / 31.0f) - 0.5f) * 0.30f;
            float segY  = (float)((int)((sh2 >> 9) & 3u) - 1) * SWASH_STEP;
            float segAmp = 0.62f + 0.38f * (float)((sh2 >> 13) & 7u) / 7.0f;

            // Yay içi konum 0..1 — uçlarda yumuşak sönüm (yay ortada güçlü,
            // kenarlarda incelir): kesik kesik değil, YAY gibi biter.
            float lx = (sx - (float)segI * SWASH_SEG_W) / SWASH_SEG_W;
            float taper = SDL_sinf(lx * 3.14159265f);       // 0 → 1 → 0
            if (taper < 0.10f) continue;

            // YAY BİÇİMİ. HATA (Hakan haklı): yay içindeki dalgalanma
            // sinüsün YARIM PERİYODU kadardı, yani yay boyunca tek yönde
            // inen/çıkan bir RAMPA çıkıyordu — eğik merdiven gibi, sırt gibi
            // değil. Gerçek dalga sırtı KIYIYA PARALEL ve hafif KAVİSLİ:
            // ortası kıyıya en yakın (önce kırılır), uçları geride kalır.
            // Kavis taper'dan türüyor (0→1→0), yani biçim yayın kendisiyle
            // tutarlı. Üstüne çok küçük genlikli yüksek frekanslı kırışma
            // biniyor — pürüz için, eğim için değil.
            float bow = -taper * 6.0f;                    // ortası öne (kıyıya) çıkar
            float ripple = SDL_sinf(sx * 0.052f + (float)sIdx * 2.1f) * 1.6f;
            float undul = bow + ripple;                   // dünya px

            float sp = pt - (float)sIdx * (1.0f / (float)SWASH_SETS) + segPh;
            sp -= SDL_floorf(sp);
            float crest = SWASH_APPROACH * (1.0f - sp);
            if (crest < 2.0f) continue;
            float near01 = 1.0f - crest / SWASH_APPROACH;
            float depthFade = 1.0f - 0.24f * (float)sIdx;

            // Sırtın y'si: yolculuk + yumuşak dalgalanma (±2 sanat px)
            float cy = shoreY + segY
                     + SDL_roundf((crest + undul) / SWASH_STEP) * SWASH_STEP;

            // VARLIK: sırt yer yer soluyor ama KOPMUYOR — kesik kesik yerine
            // incelen/kalınlaşan bir hat. presence 0'a inince o bölge görünmez
            // olur, ama geçiş yumuşak olduğu için kenarları tırtıklı değil.
            // KÖPÜK DOKUSU: yay boyunca parlaklık/kalınlık yumuşakça
            // dalgalanır — sırt tek parça düz beyaz bant gibi durmasın.
            // YALNIZ presence'a giriyor, KONUMA girmiyor: konuma girse
            // eğim riski geri gelirdi (dört turun dersi).
            float froth = 0.62f + 0.38f * SDL_sinf(sx * 0.0185f + (float)sIdx * 3.1f)
                                * SDL_sinf(sx * 0.0071f + (float)sIdx * 1.3f);
            float presence = taper * segAmp * froth;
            if (presence > 1.0f) presence = 1.0f;

            // Kalınlık 1-3 sanat px, boyunca yumuşak değişiyor (kabarma)
            float thick = 1.0f + SDL_floorf(presence * 1.9f);
            if (near01 > 0.78f) thick += 1.0f;              // kıyıda kırılıyor
            float chh = thick * SWASH_STEP;

            SDL_FRect cr = { (sx - camera->x) * zoom, (cy - camera->y) * zoom,
                             SWASH_COLW * zoom, chh * zoom };
            // Uzaktaki sırtlar daha sönük: 118 (eskiden 138) + taban düştü
            float alpha = (18.0f + 118.0f * near01) * depthFade * presence;
            r2d_set_color(renderer, 226, 236, 242, (Uint8)alpha);
            r2d_fill_rect(renderer, &cr);

            // Çukur: yalnız kıyıya yakın sırtlarda, soluk, sırtla aynı hat
            if (near01 > 0.55f) {
                SDL_FRect tr = { (sx - camera->x) * zoom,
                                 (cy + chh - camera->y) * zoom,
                                 SWASH_COLW * zoom, SWASH_STEP * zoom };
                r2d_set_color(renderer, 24, 52, 92,
                              (Uint8)(46.0f * near01 * depthFade * presence));
                r2d_fill_rect(renderer, &tr);
            }
        }

#endif

        // ── KUMUN HAFIZASI: ıslak iz vuruşta anında yükselir, sonra kurur
        int ci = col & (SWASH_MEM - 1);
        float mem = swashMem[ci];
        if (reach > mem) mem = reach; else mem -= SWASH_DRY * dt;
        if (mem < 0.0f) mem = 0.0f;
        swashMem[ci] = mem;
        float memQ = SDL_roundf(mem / SWASH_STEP) * SWASH_STEP;

        // (a) ÇEKİLMİŞ ISLAK KUM — su gitti, ıslaklık kaldı (kurudan koyu)
        if (memQ > reach) {
            SDL_FRect w = { (sx - camera->x) * zoom,
                            (shoreY - memQ - camera->y) * zoom,
                            SWASH_COLW * zoom, (memQ - reach) * zoom };
            r2d_set_color(renderer, 96, 78, 52, 84);
            r2d_fill_rect(renderer, &w);
        }

        // (b) AKTİF SWASH — BULANIK kumlu su (berrak değil: dalga kumu askıda
        //     taşır). Üç kademe: kıyıya yakın mavi-gri çalkantı → sütlü askı
        //     kumu → uçta köpüğe geçiş.
        if (reach > 0.5f) {
            float seg = reach / 3.0f;
            static const Uint8 TURB[3][4] = {
                { 138, 150, 152, 128 }, { 186, 176, 156, 140 }, { 214, 210, 198, 150 },
            };
            for (int k = 0; k < 3; k++) {
                SDL_FRect b = { (sx - camera->x) * zoom,
                                (shoreY - seg * (float)(k + 1) - camera->y) * zoom,
                                SWASH_COLW * zoom, seg * zoom };
                r2d_set_color(renderer, TURB[k][0], TURB[k][1], TURB[k][2],
                              (Uint8)((float)TURB[k][3] * (0.35f + 0.65f * swashE)));
                r2d_fill_rect(renderer, &b);
            }
        }

        // (c) KÖPÜK DUDAĞI — suyun o anki ucunda, tırmanışta en parlak
        if (reach > 0.5f) {
            // Dudak da tırtıklı: sütun başına ±1 sanat px + kalınlık oynaması
            unsigned lh = (unsigned)((int)(sx / SWASH_COLW) + 8192) * 0x85EBCA6Bu;
            lh ^= lh >> 15; lh *= 0x2545F491u; lh ^= lh >> 13;
            float ljY = (float)((int)(lh & 3u) - 1) * SWASH_STEP;
            float lhH = 4.0f + 4.0f * (float)((lh >> 5) & 1u);
            SDL_FRect lip = { (sx - camera->x) * zoom,
                              (shoreY - reach + ljY - camera->y) * zoom,
                              SWASH_COLW * zoom, lhH * zoom };
            // Enerjiyle doğup sönen köpük: çizgide 0, tepede en parlak
            r2d_set_color(renderer, 236, 244, 248, (Uint8)(30.0f + 195.0f * swashE));
            r2d_fill_rect(renderer, &lip);
        }

        // (d) KUMDAKİ KÖPÜK ÖBEKLERİ — düz çizgi değil KÜME: sütun başına
        //     üç aday, her biri kendi yüksekliğinde/genişliğinde ve suyun
        //     terk ettiği andan bu yana geçen süreye göre soluyor. Kümeler
        //     ıslak kumun içinde dağınık duruyor, kurudukça kayboluyor.
        if (memQ > reach + SWASH_STEP) {
            unsigned h0 = (unsigned)col * 0x9E3779B9u ^ 0x5EA17u;
            h0 ^= h0 >> 15; h0 *= 0x2545F491u; h0 ^= h0 >> 13;
            float band = memQ - reach;
            for (int k = 0; k < 5; k++) {                  // 3 → 5: daha yoğun
                unsigned hk = h0 >> (k * 5);
                if ((hk & 7u) == 0u) continue;             // ~%12 boş (eski %25)
                float f = 0.12f + 0.76f * (float)((hk >> 2) & 15u) / 15.0f;
                float fy = reach + band * f;
                float hgt = 4.0f + 4.0f * (float)((hk >> 6) & 1u);
                float wid = SWASH_COLW * (0.45f + 0.55f * (float)((hk >> 7) & 1u));
                float xo  = ((hk >> 8) & 1u) ? (SWASH_COLW - wid) : 0.0f;
                // Su ucundan uzaklaştıkça kurumuş → daha soluk
                float fade = 1.0f - f * 0.85f;
                SDL_FRect fr = { (sx + xo - camera->x) * zoom,
                                 (shoreY - fy - camera->y) * zoom,
                                 wid * zoom, hgt * zoom };
                r2d_set_color(renderer, 240, 246, 250, (Uint8)(165.0f * fade));
                r2d_fill_rect(renderer, &fr);
            }
        }
    }
    r2d_set_color(renderer, 255, 255, 255, 255);
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

// ---------------------------------------------------------------------------
// KIYI KESİTİ — PROGRAMATİK KARO SANATI (Faz SU)
// Gölet/göl kıyısı "çukur" okunmuyordu çünkü kıyının KESİTİ çizili değildi.
// ALttP haritasından piksel ölçümüyle çıkan anatomi yüklemede RGBA tampona
// piksel piksel boyanır (r2d_texture_from_pixels) ve dual-grid köşe maskesiyle
// kıyı çevresine basılır. Asset yok; atlasın tamamı koddan doğar.
//
// ANATOMİ (16 px sanat ölçeği — karo dokularıyla aynı):
//   KUZEY kenar (kara üstte, su altta): karanın alt kenarından SU tarafına
//     sarkan FİSTOLU kahverengi yamaç — açık(128,96,56) / orta(96,72,40) /
//     dip(72,56,40) bantları + 1-2 px koyu kontur(40,40,40). Alt kenar düz
//     değil: 4 px'lik yarım daire dişler, sütun başına hash'le 0/1 px oynar.
//   GÜNEY kenar (su üstte, kara altta): karanın üst kenarında 2-3 px siluet
//     dudağı (40,40,40) + suya 1-2 px sarkan çim saçağı.
//   DOĞU/BATI: kuzeyle aynı gramer, dikey (bir tık ince yamaç).
//   KÖŞELER: kontur KIRILMADAN döner — kara/su bölgesi köşe ıslaklıklarının
//     BİLİNEER alanından türediği için çeyrek yay kendiliğinden çıkıyor,
//     köşe parçaları ayrıca çizilmiyor.
//
// GEOMETRİ UZLAŞMASI: dual hücrenin su yarısı 8 sanat px — ALttP'nin 10 px'e
// varan yamacı buraya sığmaz. Yamaç+kontur toplamı 8 px'te tavanlanır; taşan
// parça komşu "tam su" (maske 15) hücresine düşerdi ve o hücre çizilmiyor.
//
// ATLAS DÜZENİ (256×96, hücre 16×16, yüklemede bir kez üretilir):
//   sütun = 16'lık köşe maskesi (bit1=NW 2=NE 4=SW 8=SE ıslak; 0 ve 15 boş)
//   satır 0-2 = varyant 0-2 (çim SAÇAKLI) · satır 3-5 = aynıları SAÇAKSIZ
//   (saçak yalnız kara köşelerinin HEPSİ çim olan hücrede — yol/taş kıyısında
//   yeşil püskül bitmez; dudak+yamaç her kara türünde aynı: kesit topraktır).
// Parça wld_water_pass gibi yarım karo kaymış basılır: kuzey yamacı SU
// karosunun üst şeridine, güney dudağı KARA karosunun alt şeridine düşer.
#define SHORE_CELL    16                      // hücre kenarı (sanat px)
#define SHORE_VARS    3                       // varyant sayısı — hash(col,row) seçer
#define SHORE_REACH   12                      // en yakın kara taraması yarıçapı (px)
#define SHORE_ATLAS_W (16 * SHORE_CELL)
#define SHORE_ATLAS_H (SHORE_VARS * 2 * SHORE_CELL)

static R2DTexture *shoreTex = NULL;   // programatik kıyı kesiti atlası

// Taşınabilir tamsayı hash (scatter deseninin aynısı — frac/sin YOK)
static inline unsigned shore_hash(unsigned a, unsigned b)
{
    unsigned h = a * 2654435761u ^ b * 40503u;
    return (h >> 11) ^ h;
}

// Hücre içi (ve dışı: bilineer EKSTRAPOLASYON) ıslaklık alanı. Köşe değerleri
// maskeden, eşik 0.5. Düz kenarlar hücre orta çizgisine, tek köşeler hiperbol
// yayına oturur; alan hücre kenarında komşu hücreninkiyle aynı doğrusala
// indiği için desen hücreler arası sürekli. Piksel MERKEZİ örneklenir (+0.5).
static bool shore_wet(int mask, int px, int py)
{
    float u = ((float)px + 0.5f) / (float)SHORE_CELL;
    float v = ((float)py + 0.5f) / (float)SHORE_CELL;
    float top = ((mask & 1) ? (1.0f - u) : 0.0f) + ((mask & 2) ? u : 0.0f);
    float bot = ((mask & 4) ? (1.0f - u) : 0.0f) + ((mask & 8) ? u : 0.0f);
    return top * (1.0f - v) + bot * v >= 0.5f;
}

static void shore_bake_atlas(R2D *renderer)
{
    // Palet. Saçak yeşilleri zemin çim dokularından ÖRNEKLENDİ ve sabit
    // yazıldı (dokular düz renk: Grass_1 62,137,72 · Grass_2 51,152,75) —
    // GPU dokusundan geri okuma API'si yok, iki sabit için çalışma anında
    // PNG çözmek gereksiz bağımlılık. Çim dokuları değişirse burası elle
    // güncellenmeli. Kahveler ALttP kesit ölçümünden.
    static const Uint8 SLOPE_HI[3]  = { 128,  96,  56 };   // yamaç açık kahve
    static const Uint8 SLOPE_MID[3] = {  96,  72,  40 };   // yamaç orta
    static const Uint8 SLOPE_LO[3]  = {  72,  56,  40 };   // yamaç dibi
    static const Uint8 CONTOUR[3]   = {  40,  40,  40 };   // kontur + güney dudağı
    static const Uint8 GRASS_A[3]   = {  62, 137,  72 };   // saçak (Grass_1)
    static const Uint8 GRASS_B[3]   = {  51, 152,  75 };   // saçak açık (Grass_2)

    static Uint8 pix[SHORE_ATLAS_W * SHORE_ATLAS_H * 4];
    SDL_memset(pix, 0, sizeof(pix));

    // Maske başına bir kez: her su pikselinin EN YAKIN KARA pikseline uzaklığı
    // (d²) ve yönü. Geometri varyanttan bağımsız — varyantlar yalnız boyamada
    // (diş/saçak zarları) ayrışır. Kaba tarama 16×16×25² ≈ 160k bakış/maske,
    // yüklemede bir kez — ucuz.
    static int   nd2[SHORE_CELL * SHORE_CELL];   // 0 = kara · -1 = kara erimde yok
    static Sint8 nox[SHORE_CELL * SHORE_CELL];
    static Sint8 noy[SHORE_CELL * SHORE_CELL];
    static const int TOOTH[4] = { 0, 1, 1, 0 };  // 4 px'lik yarım daire diş profili

    for (int mask = 1; mask <= 14; mask++) {
        for (int py = 0; py < SHORE_CELL; py++)
            for (int px = 0; px < SHORE_CELL; px++) {
                int i = py * SHORE_CELL + px;
                if (!shore_wet(mask, px, py)) { nd2[i] = 0; continue; }
                int best = SHORE_REACH * SHORE_REACH + 1, bx = 0, by = 0;
                for (int oy = -SHORE_REACH; oy <= SHORE_REACH; oy++)
                    for (int ox = -SHORE_REACH; ox <= SHORE_REACH; ox++) {
                        int dd = ox * ox + oy * oy;
                        if (dd == 0 || dd >= best) continue;
                        if (!shore_wet(mask, px + ox, py + oy)) {
                            best = dd; bx = ox; by = oy;
                        }
                    }
                nd2[i] = (best > SHORE_REACH * SHORE_REACH) ? -1 : best;
                nox[i] = (Sint8)bx;
                noy[i] = (Sint8)by;
            }

        for (int fringe = 0; fringe < 2; fringe++)        // 0 = saçaklı set
            for (int var = 0; var < SHORE_VARS; var++) {
                int cellX = mask * SHORE_CELL;
                int cellY = (fringe * SHORE_VARS + var) * SHORE_CELL;
                for (int py = 0; py < SHORE_CELL; py++)
                    for (int px = 0; px < SHORE_CELL; px++) {
                        const Uint8 *c = NULL;
                        int i = py * SHORE_CELL + px;
                        if (nd2[i] == 0) {
                            // KARA — GÜNEY DUDAĞI: üstte 1-3 px içinde su varsa
                            // siluet. 3. sıra sütun başına 1/3 zar (dudak 2-3 px).
                            int k = 0;
                            for (int s = 1; s <= 3 && k == 0; s++)
                                if (shore_wet(mask, px, py - s)) k = s;
                            if (k > 0 && (k <= 2 ||
                                shore_hash((unsigned)px, 0xD1u + (unsigned)var) % 3u == 0))
                                c = CONTOUR;
                        } else if (nd2[i] > 0) {
                            // SU — yön, bilineer alanın GRADYANINDAN (yüzey
                            // normali). İlk sürüm en yakın kara PİKSELİNİN
                            // yönünü kullanıyordu: ayrık ızgarada o yön komşu
                            // pikseller arasında sıçrar ve güneye bakan
                            // yaylarda incelme (taper) D'yi zıplatıp konturu
                            // "tarak dişi" kopuk şeritlere dağıtıyordu
                            // (Hakan: "köşeler garip"). Gradyan pürüzsüz —
                            // kontur yay boyunca kırılmadan döner.
                            float d = SDL_sqrtf((float)nd2[i]);
                            float gu = ((float)px + 0.5f) / (float)SHORE_CELL;
                            float gv = ((float)py + 0.5f) / (float)SHORE_CELL;
                            float nw = (mask & 1) ? 1.0f : 0.0f;
                            float ne = (mask & 2) ? 1.0f : 0.0f;
                            float sw = (mask & 4) ? 1.0f : 0.0f;
                            float se = (mask & 8) ? 1.0f : 0.0f;
                            float lx = -((ne - nw) * (1.0f - gv) + (se - sw) * gv);
                            float ly = -((sw - nw) * (1.0f - gu) + (se - ne) * gu);
                            float len = SDL_sqrtf(lx * lx + ly * ly);
                            if (len < 0.001f) {   // eyer merkezi (maske 6/9): gradyan söner
                                lx = (float)nox[i]; ly = (float)noy[i]; len = d;
                            }
                            float down = (ly > 0.0f) ? ly / len : 0.0f;
                            float ax = (lx < 0.0f) ? -lx : lx;
                            float ay = (ly < 0.0f) ? -ly : ly;
                            if (down >= 0.90f) {
                                // kara dosdoğru altta = GÜNEY kıyısı: çim saçağı
                                // suya 0-2 px sarkar (sütun başına zar — püsküllü)
                                if (fringe == 0) {
                                    unsigned fh = shore_hash((unsigned)px,
                                                             0x5Cu + (unsigned)var);
                                    int flen = (int)(fh % 8u);   // %25 0 · %50 1 · %25 2
                                    flen = (flen < 2) ? 0 : (flen < 6) ? 1 : 2;
                                    if (d <= (float)flen + 0.01f)
                                        c = (fh & 8u) ? GRASS_B : GRASS_A;
                                }
                            } else {
                                // YAMAÇ: dikey ağırlık = kuzey grameri, yatay = D/B
                                bool vert = (ay >= ax);
                                int t = vert ? px : py;
                                float D = (vert ? 5.0f : 4.0f) + (float)TOOTH[t & 3];
                                // fisto zarı: sütun başına 0/1 px — hücre SINIR
                                // sütunlarında kapalı (varyantlar arası dikiş tutsun)
                                if (t > 0 && t < SHORE_CELL - 1 &&
                                    (shore_hash((unsigned)t, 0x33u + (unsigned)var) & 1u))
                                    D += 1.0f;
                                // güneye kıvrılırken yamaç incelir: kontur, dudağa
                                // doğru kırılmadan süzülür (çeyrek yayın bitişi)
                                if (down > 0.35f) D *= (0.90f - down) / 0.55f;
                                if (D < 1.0f) D = 1.0f;
                                if (d > D + 1.6f)        c = NULL;      // açık su
                                else if (d > D)          c = CONTOUR;   // 1-2 px dip kontur
                                else if (d <= D * 0.40f) c = SLOPE_HI;
                                else if (d <= D * 0.72f) c = SLOPE_MID;
                                else                     c = SLOPE_LO;
                            }
                        }
                        if (c == NULL) continue;
                        Uint8 *p = &pix[((size_t)(cellY + py) * SHORE_ATLAS_W
                                         + (size_t)(cellX + px)) * 4];
                        p[0] = c[0]; p[1] = c[1]; p[2] = c[2]; p[3] = 255;
                    }
            }
    }
    shoreTex = r2d_texture_from_pixels(renderer, pix, SHORE_ATLAS_W, SHORE_ATLAS_H);
}

// Kesit ıslaklığı: YALNIZ göl/deniz (2) ve gölet (20). Vaha suyu (17)
// BİLEREK dışarıda: vaha kendi sanatçı kıyı atlasını taşıyor (dsrWaterTex
// dual-grid), baked kesit onun üstüne binince ÇİFT KESİT çakışıyor — fisto +
// koyu kontur, atlasın kum kıyı diline hiç oturmadı (Hakan'ın vaha ekran
// görüntüsü). Harita DIŞI kara sayılır (wmask BFS'iyle aynı karar — kenar
// suyu da kıyı kesiti alır).
// Kıyı kesiti için ıslaklık. Vaha (17) BİLEREK yok: kendi sanatçı atlasını
// (dsrWaterTex) taşıyor, baked kesit üstüne binince çift kesit çakışıyor.
static inline bool shore_cut_wet(int col, int row)
{
    if (col < 0 || col >= mapW || row < 0 || row >= mapH) return false;
    int t = tilemap[row][col];
    return t == 2 || t == 20;
}

// HARİTA KENARI SU ŞERİDİ — batı (süt 0-1) ve doğu (son 2 süt) "dünyanın
// kenarı" dolgusu, gerçek bir kıyı değil. Kesit oraya basılınca çöl geçidinin
// yanında yapay bir sahil çıkıyordu (Hakan: "doğudaki kıyıları kaldır").
// Güney kenarı HARİÇ: orası artık gerçek deniz.
static inline bool shore_edge_filler(int col)
{
    return col <= 1 || col >= mapW - 2;
}

// Kıyı kesiti pass'i — wld_water_pass ile aynı dual-grid yerleşimi, ama
// kara türü serbest (kesit toprak: çim de yol da aynı yamacı alır).
// Deterministik: hash(col,row) varyantı, aynı karo hep aynı parçayı basar.
static void shore_cut_pass(R2D *renderer, const Camera *camera,
                           int colL, int rowT, int colR, int rowB)
{
    float zoom = camera->zoom;
    for (int row = rowT - 1; row < rowB; row++) {
        for (int col = colL - 1; col < colR; col++) {
            int m = 0;
            bool grassLand = true;   // kara köşelerin HEPSİ çim mi? (saçak koşulu)
            for (int dy = 0; dy < 2; dy++)
                for (int dx = 0; dx < 2; dx++) {
                    int cc = col + dx, rr = row + dy;
                    if (shore_cut_wet(cc, rr))
                        m |= 1 << (dy * 2 + dx);
                    else if (cc < 0 || cc >= mapW || rr < 0 || rr >= mapH ||
                             tilemap[rr][cc] != 0)
                        grassLand = false;
                }
            if (m == 0 || m == 15) continue;   // tam kara / açık su: parça yok
            if (shore_edge_filler(col) || shore_edge_filler(col + 1))
                continue;                       // dünya kenarı dolgusu: kesit yok
            // KUMSALDA KESİT YOK. Kesit YÜKSELTİLMİŞ kıyı için tasarlandı
            // (çim/toprak banket: görünen yamaç kesiti + koyu kontur — ALttP
            // grameri). Kumsalda kum doğrudan suya girer, uçurum kesiti
            // olmaz; düz sahilde kesit bütün kıyı boyunca tek tip koyu bir
            // ŞERİT hâline geliyordu ve yapay duruyordu (Hakan). Kum kıyısını
            // dalga vuruşu (swash) + ıslak kum zaten anlatıyor.
            {
                bool sandy = false;
                for (int dy = 0; dy < 2 && !sandy; dy++)
                    for (int dx = 0; dx < 2 && !sandy; dx++) {
                        int cc = col + dx, rr = row + dy;
                        if (cc >= 0 && cc < mapW && rr >= 0 && rr < mapH &&
                            tilemap[rr][cc] == 15) sandy = true;
                    }
                if (sandy) continue;
            }
            int var = (int)(shore_hash((unsigned)col, (unsigned)row) % SHORE_VARS);
            SDL_FRect src = {
                (float)(m * SHORE_CELL),
                (float)(((grassLand ? 0 : SHORE_VARS) + var) * SHORE_CELL),
                (float)SHORE_CELL, (float)SHORE_CELL };
            SDL_FRect dst = {
                ((col + 0.5f) * TILE_SIZE - camera->x) * zoom,
                ((row + 0.5f) * TILE_SIZE - camera->y) * zoom,
                TILE_SIZE * zoom, TILE_SIZE * zoom };
            r2d_draw_texture(renderer, shoreTex, &src, &dst);
        }
    }
}

void tilemap_load_details(R2D *renderer)
{
    extern R2DTexture *load_texture(R2D *r, const char *path);   // main yardımcısı
    grassVar[0] = load_texture(renderer, "assets/Tiles/erw/grass_1.png");
    grassVar[1] = load_texture(renderer, "assets/Tiles/erw/grass_2.png");
    grassVar[2] = load_texture(renderer, "assets/Tiles/erw/grass_3.png");
    grassVar[3] = load_texture(renderer, "assets/Tiles/erw/grass_4.png");
    waterAnimTex = load_texture(renderer, "assets/Tiles/erw/water_anim.png");
    pathEdgeTex  = load_texture(renderer, "assets/Tiles/erw/path_edge_atlas.png");
    grassEdgeTex = load_texture(renderer, "assets/Tiles/erw/grass_edge_atlas.png");
    // ERW dalga-2: serpiştirme atlası bake_erw_wave2.py'den (hücre 32px).
    flowerTex = load_texture(renderer, "assets/Tiles/erw/scatter_atlas.png");
    pathDecTex = load_texture(renderer, "assets/Tiles/erw/pathdec_atlas.png");
    uwBushTex[0] = load_texture(renderer, "assets/decor/erw/bush_1.png");
    uwBushTex[1] = load_texture(renderer, "assets/decor/erw/bush_2.png");
    mzGround2 = load_texture(renderer, "assets/Tiles/erw/mz_ground2.png");
    dsrWaterTex = load_texture(renderer, "assets/Tiles/erw/oasis_atlas.png");
    dsrCliffTex = load_texture(renderer, "assets/Tiles/erw/rocky_atlas.png");
    // Keyed varyant: koyu blokların düz arka planı şeffaf (bake_cave_ring.py) —
    // ışık bloklar dokunulmamış; loca + mezarlık AYNI atlası kullanır
    caveRingTex = load_texture(renderer, "assets/Tiles/erw/cave_ring_atlas.png");
    mzRockTex = load_texture(renderer, "assets/Tiles/erw/mz_rock_atlas.png");
    // SU ANİMASYONU: atlasın animasyonlu sürümü aynı 3x5 düzeni 8 KARE yan yana
    // taşıyor (384x80 = 24x5 hücre). Yani LUT değişmiyor; kıyı hücresinin cx'ine
    // "kare * 3" ekleyince su akmaya başlıyor. Statik sürüm yedek olarak duruyor —
    // animasyonlu dosya bulunamazsa akış durur, kıyı yine çizilir.
    // ERW dalga-2: Cute kıyı animasyon overlay'i (Water_Tile_1_Anim) EMEKLİ —
    // kıyı işini su shader'ı yapıyor (menisküs/köpük/çırpıntı/ıslak bant).
    // wldWaterTex NULL kalır, wld_water_pass kendiliğinden susar; geri almak
    // = bu yüklemeyi geri koymak. (KALDIRILDI: yükleme satırları)
    wldWaterFrames = 1;   // (EMEKLİ yol — wldWaterTex hep NULL, pass susuyor)
    // KIYI KESİTİ atlası: dosyadan yüklenmez, koddan boyanır (shore_bake_atlas)
    shore_bake_atlas(renderer);
}

void tilemap_free_details(void)
{
    for (int i = 0; i < 4; i++)
        if (grassVar[i]) { r2d_texture_destroy(NULL, grassVar[i]); grassVar[i] = NULL; }
    if (waterAnimTex) { r2d_texture_destroy(NULL, waterAnimTex); waterAnimTex = NULL; }
    if (pathEdgeTex) { r2d_texture_destroy(NULL, pathEdgeTex); pathEdgeTex = NULL; }
    if (grassEdgeTex) { r2d_texture_destroy(NULL, grassEdgeTex); grassEdgeTex = NULL; }
    if (flowerTex) { r2d_texture_destroy(NULL, flowerTex); flowerTex = NULL; }
    if (pathDecTex) { r2d_texture_destroy(NULL, pathDecTex); pathDecTex = NULL; }
    for (int i = 0; i < 2; i++)
        if (uwBushTex[i]) { r2d_texture_destroy(NULL, uwBushTex[i]); uwBushTex[i] = NULL; }
    if (mzGround2) { r2d_texture_destroy(NULL, mzGround2); mzGround2 = NULL; }
    if (dsrWaterTex) { r2d_texture_destroy(NULL, dsrWaterTex); dsrWaterTex = NULL; }
    if (dsrCliffTex) { r2d_texture_destroy(NULL, dsrCliffTex); dsrCliffTex = NULL; }
    if (caveRingTex) { r2d_texture_destroy(NULL, caveRingTex); caveRingTex = NULL; }
    if (mzRockTex) { r2d_texture_destroy(NULL, mzRockTex); mzRockTex = NULL; }
    if (wldWaterTex) { r2d_texture_destroy(NULL, wldWaterTex); wldWaterTex = NULL; }
    if (shoreTex) { r2d_texture_destroy(NULL, shoreTex); shoreTex = NULL; }
}

// Deterministik varyant: hash(col,row) — save'e girmez, hep aynı görünür
static inline int grass_variant(int col, int row)
{
    unsigned v = (unsigned)(col * 73856093u ^ row * 19349663u);
    v = (v >> 13) ^ v;
    return (int)(v % 4u);
}

// (is_grass buradaydı — eski 12 hücreli saçak bloğuyla birlikte emekli oldu;
// ERW pass'leri erw_tile_at kullanıyor. Git geçmişinde.)

// Çimenin taşabileceği taban mı? (dünya dış mekan tile'ları — zindan HARİÇ)
// Çim hangi karonun üstüne TAŞAR (autotile kenar sistemi, grass_edges.png).
// 15 (KUM) eklendi: çimen↔kumsal geçişi sert bir çizgiyle bitiyordu; çim
// saçağı kumun üstüne sarkınca geçiş yumuşuyor. Deniz↔kumsal tarafını ise
// kıyı kesiti (shore_cut_pass, çift-ızgara) + swash ıslaklığı kapatıyor;
// yani üç bölge arasındaki geçişlerin HEPSİ artık autotile ile örülü:
//   çimen→kum : bu fonksiyon (çim kenar atlası)
//   kum→deniz : kıyı kesiti + dalga vuruşu
static inline bool grass_bleeds_onto(int id)
{
    // id 1 (yol) LİSTEDE DEĞİL: çim↔yol sınırını erw_path_pass çizer ve
    // toprak blob'unun çim saçağı GÖMÜLÜ — üstüne ayrıca saçak binmesin.
    return id == 2 || id == 4 || id == 5 || id == 15;
}

// --- ERW MASKE-SIRALI DUAL-GRID (Faz EPIC-1) --------------------------------
// dsr_dual_pass'in ERW hali; fark: LUT YOK. bake_erw_terrain.py atlası köşe
// maskesi SIRASIYLA üretir → hücre = (m%4, m/4), hücre boyu 32px (ERW karo
// sanatı 32, oyun 2× ölçekle TILE_SIZE'a basar). Maske bitleri dsr ile aynı:
// 1=NW 2=NE 4=SW 8=SE. m=0 boş, m=15 iç (taban zaten çizdi) — ikisi atlanır.
static inline int erw_tile_at(int col, int row)
{
    if (col < 0) col = 0; if (col >= mapW) col = mapW - 1;   // OOB = kenar hücresi
    if (row < 0) row = 0; if (row >= mapH) row = mapH - 1;
    return tilemap[row][col];
}

static void erw_mask_cell_draw(R2D *renderer, R2DTexture *atlas, int m,
                               int col, int row, const Camera *camera)
{
    SDL_FRect src = { (float)(m % 4) * 32.0f, (float)(m / 4) * 32.0f, 32, 32 };
    SDL_FRect dst = {
        ((col + 0.5f) * TILE_SIZE - camera->x) * camera->zoom,
        ((row + 0.5f) * TILE_SIZE - camera->y) * camera->zoom,
        TILE_SIZE * camera->zoom, TILE_SIZE * camera->zoom };
    r2d_draw_texture(renderer, atlas, &src, &dst);
}

// ÇİM↔YOL sınırı: ERW toprak blob'u. Kenar karoları toprak + GÖMÜLÜ çim
// saçağı taşır; çim tarafı şeffaf → altta çim tabanı görünür.
static void erw_path_pass(R2D *renderer, const Camera *camera,
                          int colL, int rowT, int colR, int rowB)
{
    for (int row = rowT - 1; row < rowB; row++)
        for (int col = colL - 1; col < colR; col++) {
            int m = ((erw_tile_at(col, row) == 1) ? 1 : 0)
                  | ((erw_tile_at(col + 1, row) == 1) ? 2 : 0)
                  | ((erw_tile_at(col, row + 1) == 1) ? 4 : 0)
                  | ((erw_tile_at(col + 1, row + 1) == 1) ? 8 : 0);
            if (m == 0 || m == 15) continue;
            erw_mask_cell_draw(renderer, pathEdgeTex, m, col, row, camera);
        }
}

// ÇİM SAÇAĞI: su/bataklık/diken/kum sınırında ERW çim blob'u (çim tarafı
// opak, saçak hedefin üstüne taşar). Yol köşesi içeren hücre ATLANIR (onu
// path pass çizdi); hedefsiz hücre de atlanır (çim içi — taban yeter).
static void erw_grass_fringe_pass(R2D *renderer, const Camera *camera,
                                  int colL, int rowT, int colR, int rowB)
{
    for (int row = rowT - 1; row < rowB; row++)
        for (int col = colL - 1; col < colR; col++) {
            int t0 = erw_tile_at(col, row),     t1 = erw_tile_at(col + 1, row);
            int t2 = erw_tile_at(col, row + 1), t3 = erw_tile_at(col + 1, row + 1);
            int m = ((t0 == 0) ? 1 : 0) | ((t1 == 0) ? 2 : 0)
                  | ((t2 == 0) ? 4 : 0) | ((t3 == 0) ? 8 : 0);
            if (m == 0 || m == 15) continue;
            if (t0 == 1 || t1 == 1 || t2 == 1 || t3 == 1) continue;
            if (!grass_bleeds_onto(t0) && !grass_bleeds_onto(t1) &&
                !grass_bleeds_onto(t2) && !grass_bleeds_onto(t3)) continue;
            erw_mask_cell_draw(renderer, grassEdgeTex, m, col, row, camera);
        }
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
// ERW MASKE-SIRALI ATLAS SÖZLEŞMESİ (bake_erw_biome.py): hücre = köşe
// maskesi (m%4, m/4), hücre 32px. Vaha, loca ringi ve mezarlık kayası aynı
// kimlik tablosunu kullanır; m15 atlasta var ama İÇ taban zaten çizili → atla.
static const DsrCell ERW_ID_LUT[16] = {
    [1]={1,0}, [2]={2,0}, [3]={3,0}, [4]={0,1}, [5]={1,1}, [6]={2,1},
    [7]={3,1}, [8]={0,2}, [9]={1,2}, [10]={2,2}, [11]={3,2}, [12]={0,3},
    [13]={1,3}, [14]={2,3}, [15]={255,255},
};
#define DSR_WATER_LUT ERW_ID_LUT
// (ERW dalga-5: DSR_ROCK da kimlik tabloda — Cute klif atlası ve plato/yüz
// mimarisi emekli; ERW rocky-blob kenarları kayalık yamayı kendisi anlatır.)
#define DSR_ROCK_LUT ERW_ID_LUT

static inline bool dsr_is(int col, int row, int id)
{
    if (col < 0) col = 0; if (col >= mapW) col = mapW - 1;   // OOB = kenar hücresi
    if (row < 0) row = 0; if (row >= mapH) row = mapH - 1;
    return tilemap[row][col] == id;
}

// Güneyi kum olan kaya = KLİF YÜZÜ hücresi (çizgili duvar basılır).
// Dual pass yüz hücrelerini kayadan SAYMAZ → plato dudağı yüzün üstüne oturur.
// (dsr_is_face emekli — ERW dalga-5: klif yüz kavramı kalktı.)

static inline bool dsr_rockish(int col, int row)
{
    return dsr_is(col, row, DSR_ROCK);   // (ERW: yüz kavramı emekli)
}

static void dsr_dual_pass(R2D *renderer, R2DTexture *atlas, int id,
                          float cellPx,   // atlas hücresi: ERW 32, Cute klif 16
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
            SDL_FRect src = { lut[m].cx * cellPx, lut[m].cy * cellPx,
                              cellPx, cellPx };
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

// (ERW dalga-3: CAVE_LUT/MZ_LUT emekli — iki ring de maske-sıralı ERW
// atlasından ERW_ID_LUT ile okunur; Cave_Walls_Keyed düzeni git geçmişinde.)

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

static void cave_dual_pass(R2D *renderer, R2DTexture *atlas, int wallId,
                           const Camera *camera,
                           int colL, int rowT, int colR, int rowB)
{
    const DsrCell *lut = ERW_ID_LUT;
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
            SDL_FRect src = { lut[m].cx * 32.0f, lut[m].cy * 32.0f, 32, 32 };
            SDL_FRect dst = {
                ((col + 0.5f) * TILE_SIZE - camera->x) * zoom,
                ((row + 0.5f) * TILE_SIZE - camera->y) * zoom,
                TILE_SIZE * zoom, TILE_SIZE * zoom };
            r2d_draw_texture(renderer, atlas, &src, &dst);
        }
    }
}

// --- DÜNYA SUYU KIYI GEÇİŞLERİ (Faz MAPGEN-2) ---
// (EMEKLİ — ERW dalga-2: kıyıyı su shader'ı süslüyor, wldWaterTex artık
// yüklenmiyor; LUT+pass geri dönüş kolaylığı için duruyor.) Eski not: çimen tutamlı
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

            // VAHA (ERW dalga-5): taban kum + üstüne AKAN su animasyonu;
            // kıyı hücrelerini maske-sıralı vaha atlası (dual pass) örter.
            // (Eski Cute sistemde vaha içi atlas m15 hücresinden geliyordu;
            // ERW kimlik LUT m15'i atlar — iç su tabanı buradan.)
            if (tileID == DSR_WATER && dsrWaterTex != NULL)
            {
                tile_draw(renderer, tileTextures, 15, dst);   // 15 = kum
                if (waterAnimTex != NULL) {
                    float wf = (float)((SDL_GetTicks() / 160) % 8);
                    SDL_FRect wsrc = { wf * 32.0f, 0, 32, 32 };
                    r2d_draw_texture(renderer, waterAnimTex, &wsrc, &dst);
                }
            }
            // (ERW dalga-5: çöl kayası MESA/yüz özel dalı emekli — kaya
            // karosu normal tabanını (erw/rock.png) çizer, kenarları rocky
            // blob dual pass'i anlatır.)
            // ERW SU ANİMASYONU: deniz/gölet(2) tabanı 8 karelik şeritten
            // akar — tüm su AYNI karede (faz farkı ızgara gösterir; wld
            // dersi). Asıl görünüm su shader'ında; taban sığlıkta okunur.
            else if ((tileID == 2 || tileID == 21) && waterAnimTex != NULL)
            {
                float wf = (float)((SDL_GetTicks() / 160) % 8);
                SDL_FRect wsrc = { wf * 32.0f, 0, 32, 32 };
                r2d_draw_texture(renderer, waterAnimTex, &wsrc, &dst);
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
            // (ERW dalga-3: kaya yığını YÜZ hücresi çizimi emekli — ERW
            // duvar taban karosu (slope bloğu) yüz görünümünü kendisi taşıyor;
            // cave_is_face ring pass'inde hâlâ kullanılıyor, o kalır.)

            // KENAR KAPLAMALARI: çimen komşu tabana taşar (yumuşak geçiş)
            // (ERW GEÇİŞİ: eski tile-merkezli 12 hücreli çim saçağı bloğu
            // buradaydı — yerine aşağıdaki iki dual pass geldi.)
        }
    }

    // ERW ÇİM/YOL AUTOTILE (Faz EPIC-1): taban bitti — sınır overlay'leri.
    if (pathEdgeTex != NULL)
        erw_path_pass(renderer, camera, colL, rowT, colR, rowB);
    if (grassEdgeTex != NULL)
        erw_grass_fringe_pass(renderer, camera, colL, rowT, colR, rowB);

    // ÇÖL AUTOTILE: taban pass'i bitti — su ve kaya sınır overlay'leri
    // (yalnız bu id'leri içeren haritada iş yapar; diğerlerinde mask hep 0)
    if (dsrWaterTex != NULL) {
        dsr_dual_pass(renderer, dsrWaterTex, DSR_WATER, 32.0f, camera, colL, rowT, colR, rowB);
        // saf ERW test göleti (21): aynı vaha kıyı atlası — kum halkasıyla sarılır
        dsr_dual_pass(renderer, dsrWaterTex, 21, 32.0f, camera, colL, rowT, colR, rowB);
    }
    if (dsrCliffTex != NULL)
        dsr_dual_pass(renderer, dsrCliffTex, DSR_ROCK, 32.0f, camera, colL, rowT, colR, rowB);
    if (caveRingTex != NULL)
        cave_dual_pass(renderer, caveRingTex, CAVE_WALL, camera, colL, rowT, colR, rowB);
    if (mzRockTex != NULL)
        cave_dual_pass(renderer, mzRockTex, MZ_ROCK, camera, colL, rowT, colR, rowB);
    if (wldWaterTex != NULL)
        wld_water_pass(renderer, camera, colL, rowT, colR, rowB);
    // KIYI KESİTİ: kıyı atlasının ÜSTÜNE, su geçişinden (main.c r2d_water_flush)
    // ÖNCE — su tarafına basılan yamaç süzgeçten geçip hafif maviye kayar
    // (İSTENEN davranış) ve kuzeydeki kıyı gölgesi kesiti koyultur (doğru).
    if (shoreTex != NULL)
        shore_cut_pass(renderer, camera, colL, rowT, colR, rowB);
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
                (col * TILE_SIZE + dx * (TILE_SIZE - 32) - camera->x) * zoom,
                (row * TILE_SIZE + dy * (TILE_SIZE - 32) - camera->y) * zoom,
                32.0f * zoom, 32.0f * zoom };   // ERW hücresi 32px → yarım karo

            if (id == 0) {   // çimen: %6 çiçek/ot tömreği
                bool nearPath = (col > 0 && tilemap[row][col-1] == 1) ||
                                (col < mapW-1 && tilemap[row][col+1] == 1) ||
                                (row > 0 && tilemap[row-1][col] == 1) ||
                                (row < mapH-1 && tilemap[row+1][col] == 1);
                if (nearPath && (h % 100) < 10 && pathDecTex != NULL) {
                    SDL_FRect src = { (float)((h >> 6) % 3) * 32.0f, 0, 32, 32 };
                    r2d_draw_texture(renderer, pathDecTex, &src, &dst);
                }
                else if ((h % 100) < 6) {
                    // scatter_atlas: 10×3 hücre 32px — satır 0-1 bitki, 2 taş
                    int fc = (int)((h >> 6) % 10), fr = (int)((h >> 13) % 3);
                    SDL_FRect src = { fc * 32.0f, fr * 32.0f, 32, 32 };
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
                // (ERW dalga-2: Cute kıyı sazı emekli — kıyıyı shader süslüyor;
                // saz hücreleri de raf/bitki zarına katılır)
                {
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
            // (ERW dalga-5: Cute klif atlası süsleri — 4×3 mesa kalıntısı,
            // 2×2 kaya, çatlak izleri — emekli; atlas hücreleri artık yok.
            // Çöl serpiştirmesi scatter_atlas taş satırından gelir.)
            else if (id == 15 && (h % 100) < 7 && flowerTex != NULL) {
                int fc = (int)((h >> 6) % 10);
                SDL_FRect src = { fc * 32.0f, 2 * 32.0f, 32, 32 };   // taş satırı
                SDL_FRect adst = {
                    (col * TILE_SIZE + dx * (TILE_SIZE - 32) - camera->x) * zoom,
                    (row * TILE_SIZE + dy * (TILE_SIZE - 32) - camera->y) * zoom,
                    32.0f * zoom, 32.0f * zoom };
                r2d_draw_texture(renderer, flowerTex, &src, &adst);
            }
        }
    }
}
