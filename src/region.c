#include "region.h"

// Koordinatlar world.csv'deki 200×150 tile haritaya göre (TILE_SIZE=64):
// dünya 12800×9600 px. Bölge sınırları dörde bölünmüş kabaca:
// sol-üst Yeşil Ova (kolay) → sağ-üst Karanlık Orman → sol-alt Bataklık →
// sağ-alt Kayalık Geçit (zor). Zorluk = kamp türü/sayısı, scaling yok.

#define T(n) ((n) * (float)TILE_SIZE)   // tile → dünya pikseli

static const RegionCamp OVA_CAMPS[] = {
    { .x = T(25), .y = T(25), .radius = T(4), .type = &MOB_TYPES[0], .maxCount = 3 },  // goblin
    { .x = T(75), .y = T(45), .radius = T(4), .type = &MOB_TYPES[0], .maxCount = 3 },
};
static const RegionCamp ORMAN_CAMPS[] = {
    { .x = T(130), .y = T(25), .radius = T(4), .type = &MOB_TYPES[0], .maxCount = 4 },
    { .x = T(170), .y = T(30), .radius = T(4), .type = &MOB_TYPES[1], .maxCount = 2 },  // ork
};
static const RegionCamp BATAKLIK_CAMPS[] = {
    { .x = T(30), .y = T(100), .radius = T(5), .type = &MOB_TYPES[1], .maxCount = 3 },
    { .x = T(70), .y = T(130), .radius = T(4), .type = &MOB_TYPES[1], .maxCount = 2 },
};
static const RegionCamp GECIT_CAMPS[] = {
    { .x = T(130), .y = T(110), .radius = T(4), .type = &MOB_TYPES[1], .maxCount = 4 },
    { .x = T(175), .y = T(130), .radius = T(4), .type = &MOB_TYPES[1], .maxCount = 4 },
};

#define CAMPS(c) .camps = (c), .campCount = (int)(sizeof(c) / sizeof((c)[0]))

// Sıra önemli: region_at İLK eşleşeni döndürür → Köy, Ova'nın içinde ama önce.
// Mood: köy huzurlu, ova keşif, diğerleri tekinsiz (gece kuralı + savaş main'de).
const Region REGIONS[] = {
    { .name = "Koy",             .rect = { T(0),   T(0),  T(20),  T(15) }, .mood = MOOD_TOWN,
      .camps = NULL, .campCount = 0 },
    { .name = "Yesil Ova",       .rect = { T(0),   T(0),  T(100), T(75) }, .mood = MOOD_WILD,   CAMPS(OVA_CAMPS) },
    { .name = "Karanlik Orman",  .rect = { T(100), T(0),  T(100), T(75) }, .mood = MOOD_DANGER, CAMPS(ORMAN_CAMPS) },
    { .name = "Bataklik",        .rect = { T(0),   T(75), T(100), T(75) }, .mood = MOOD_DANGER, CAMPS(BATAKLIK_CAMPS) },
    { .name = "Kayalik Gecit",   .rect = { T(100), T(75), T(100), T(75) }, .mood = MOOD_DANGER, CAMPS(GECIT_CAMPS) },
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
