#include "region.h"

// Koordinatlar world.csv'deki 200×150 tile haritaya göre (TILE_SIZE=64):
// dünya 12800×9600 px. Bölge sınırları dörde bölünmüş kabaca:
// sol-üst Yeşil Ova (kolay) → sağ-üst Karanlık Orman → sol-alt Bataklık →
// sağ-alt Kayalık Geçit (zor). Zorluk = kamp türü/sayısı, scaling yok.

#define T(n) ((n) * (float)TILE_SIZE)   // tile → dünya pikseli

static const RegionCamp OVA_CAMPS[] = {
    { .x = T(25), .y = T(25), .radius = T(4), .type = &MOB_TYPES[0], .maxCount = 3 },  // goblin
    { .x = T(75), .y = T(45), .radius = T(4), .type = &MOB_TYPES[0], .maxCount = 3 },
    // Faz 6 — av çayırları: köy çevresi otlaklar (pasif sürüler, günlük yenilenir)
    { .x = T(28), .y = T(10), .radius = T(4), .type = &MOB_TYPES[2], .maxCount = 3 },  // koyun
    { .x = T(35), .y = T(20), .radius = T(4), .type = &MOB_TYPES[3], .maxCount = 2 },  // domuz
    { .x = T(22), .y = T(17), .radius = T(3), .type = &MOB_TYPES[4], .maxCount = 3 },  // tavuk
    // Faz 7 — yeşil slime: ova kenarı (haritadan doğrulandı: T44,20 çevresi tamamen açık;
    // T55,30 GÖLÜN İÇİYDİ — spawn suya düşüyordu)
    { .x = T(44), .y = T(20), .radius = T(4), .type = &MOB_TYPES[6], .maxCount = 2 },
};
static const RegionCamp ORMAN_CAMPS[] = {
    { .x = T(130), .y = T(25), .radius = T(4), .type = &MOB_TYPES[0], .maxCount = 4 },
    { .x = T(170), .y = T(30), .radius = T(4), .type = &MOB_TYPES[1], .maxCount = 2 },  // ork
    // Faz 6 — yaban domuzu: orman sınırı (sunağa giden yol — riskli av)
    { .x = T(108), .y = T(22), .radius = T(4), .type = &MOB_TYPES[5], .maxCount = 2 },
};
static const RegionCamp BATAKLIK_CAMPS[] = {
    { .x = T(30), .y = T(100), .radius = T(5), .type = &MOB_TYPES[1], .maxCount = 3 },
    { .x = T(70), .y = T(130), .radius = T(4), .type = &MOB_TYPES[1], .maxCount = 2 },
    // Faz 7 — mavi slime: bataklık girişi (T40,85 — bölgeye girince karşılar; açıklık doğrulandı)
    { .x = T(40), .y = T(85), .radius = T(4), .type = &MOB_TYPES[9], .maxCount = 2 },
};
static const RegionCamp GECIT_CAMPS[] = {
    { .x = T(130), .y = T(110), .radius = T(4), .type = &MOB_TYPES[1], .maxCount = 4 },
    { .x = T(175), .y = T(130), .radius = T(4), .type = &MOB_TYPES[1], .maxCount = 4 },
    // Faz 8 — iskeletler (açıklık haritadan doğrulandı): giriş T120,80 karışık,
    // derin T150,95 (+Büyücü). Gece kuralıyla (algı ×1.5, spawn ×2) undead saati
    { .x = T(118), .y = T(80), .radius = T(4), .type = &MOB_TYPES[12], .maxCount = 2 },  // savaşçı
    { .x = T(122), .y = T(82), .radius = T(4), .type = &MOB_TYPES[13], .maxCount = 1 },  // okçu
    { .x = T(148), .y = T(95), .radius = T(4), .type = &MOB_TYPES[12], .maxCount = 1 },
    { .x = T(150), .y = T(97), .radius = T(4), .type = &MOB_TYPES[13], .maxCount = 1 },
    { .x = T(152), .y = T(93), .radius = T(4), .type = &MOB_TYPES[14], .maxCount = 1 },  // büyücü
};

#define CAMPS(c) .camps = (c), .campCount = (int)(sizeof(c) / sizeof((c)[0]))

// Sıra önemli: region_at İLK eşleşeni döndürür → Köy, Ova'nın içinde ama önce.
// Mood: köy huzurlu, ova keşif, diğerleri tekinsiz (gece kuralı + savaş main'de).
const Region REGIONS[] = {
    { .name = "Köy",             .rect = { T(0),   T(0),  T(20),  T(15) }, .mood = MOOD_TOWN,
      .camps = NULL, .campCount = 0 },
    { .name = "Yeşil Ova",       .rect = { T(0),   T(0),  T(100), T(75) }, .mood = MOOD_WILD,   CAMPS(OVA_CAMPS) },
    { .name = "Karanlık Orman",  .rect = { T(100), T(0),  T(100), T(75) }, .mood = MOOD_DANGER, CAMPS(ORMAN_CAMPS) },
    { .name = "Bataklık",        .rect = { T(0),   T(75), T(100), T(75) }, .mood = MOOD_DANGER, CAMPS(BATAKLIK_CAMPS) },
    { .name = "Kayalık Geçit",   .rect = { T(100), T(75), T(100), T(75) }, .mood = MOOD_DANGER, CAMPS(GECIT_CAMPS) },
};
const int REGION_COUNT = (int)(sizeof(REGIONS) / sizeof(REGIONS[0]));

int region_at(float worldX, float worldY)
{
    for (int i = 0; i < REGION_COUNT; i++) {
        const SDL_FRect *r = &REGIONS[i].rect;
        if (worldX >= r->x && worldX < r->x + r->w &&
            worldY >= r->y && worldY < r->y + r->h)
            return i;
    }
    return -1;
}
