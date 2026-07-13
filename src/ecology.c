#include <string.h>
#include "ecology.h"
#include "common.h"   // randf

#define T(n) ((n) * (float)TILE_SIZE)

// --- Saha tablosu -------------------------------------------------------------
// Taban sahalar = eski region.c kampları (koordinatlar oradan — harita doğrulanmış).
// Alternatif sahalar (startColonized=false) göç hedefleri: açıklıkları benzer
// bölge dokusundan seçildi; spawn zaten çevredeki en yakın açık noktaya düşer.
// GOBLIN kamplarında reis İLAVE doğar (entries yalnız erleri sayar);
// ORK kamplarında totem ilave doğar.
const NestSiteDef NEST_SITES[] = {
    // --- GOBLIN (reis modeli) ---
    { NEST_CHIEF, T(25),  T(25), T(4), FAM_GOBLIN, true,
      { { &MOB_TYPES[0], 3 } } },
    { NEST_CHIEF, T(75),  T(45), T(4), FAM_GOBLIN, true,
      { { &MOB_TYPES[0], 3 } } },
    { NEST_CHIEF, T(130), T(25), T(4), FAM_GOBLIN, true,
      { { &MOB_TYPES[0], 4 } } },
    { NEST_CHIEF, T(55),  T(60), T(4), FAM_GOBLIN, false,   // alternatif: ova güneyi
      { { &MOB_TYPES[0], 3 } } },
    { NEST_CHIEF, T(155), T(15), T(4), FAM_GOBLIN, false,   // alternatif: orman kuzeyi
      { { &MOB_TYPES[0], 4 } } },

    // --- ORK (savaş totemi) ---
    { NEST_TOTEM, T(170), T(30),  T(4), FAM_ORC, true,
      { { &MOB_TYPES[1], 2 } } },
    { NEST_TOTEM, T(30),  T(100), T(5), FAM_ORC, true,
      { { &MOB_TYPES[1], 3 } } },
    { NEST_TOTEM, T(70),  T(130), T(4), FAM_ORC, true,
      { { &MOB_TYPES[1], 2 } } },
    { NEST_TOTEM, T(130), T(110), T(4), FAM_ORC, true,
      { { &MOB_TYPES[1], 4 } } },
    { NEST_TOTEM, T(175), T(130), T(4), FAM_ORC, true,
      { { &MOB_TYPES[1], 4 } } },
    { NEST_TOTEM, T(120), T(135), T(4), FAM_ORC, false,     // alternatif: geçit güneyi
      { { &MOB_TYPES[1], 3 } } },
    { NEST_TOTEM, T(55),  T(110), T(4), FAM_ORC, false,     // alternatif: bataklık ortası
      { { &MOB_TYPES[1], 3 } } },

    // --- SLIME (gölcük kurutma ritüeli) ---
    { NEST_POOL, T(44), T(20), T(4), FAM_SLIME_G, true,
      { { &MOB_TYPES[6], 2 } } },
    { NEST_POOL, T(60), T(28), T(4), FAM_SLIME_G, false,    // alternatif
      { { &MOB_TYPES[6], 2 } } },
    { NEST_POOL, T(40), T(85), T(4), FAM_SLIME_B, true,
      { { &MOB_TYPES[9], 2 } } },
    { NEST_POOL, T(25), T(92), T(4), FAM_SLIME_B, false,    // alternatif
      { { &MOB_TYPES[9], 2 } } },

    // --- İSKELET (mezar mühürleme — göç YOK: mühür kalıcı) ---
    { NEST_GRAVE, T(120), T(81), T(4), FAM_SKELETON, true,
      { { &MOB_TYPES[12], 2 }, { &MOB_TYPES[13], 1 } } },
    { NEST_GRAVE, T(150), T(95), T(4), FAM_SKELETON, true,
      { { &MOB_TYPES[12], 1 }, { &MOB_TYPES[13], 1 }, { &MOB_TYPES[14], 1 } } },

    // --- HAYVAN SÜRÜLERİ (yuva görseli yok; biterse göçer) ---
    { NEST_HERD, T(28),  T(10), T(4), FAM_SHEEP, true,
      { { &MOB_TYPES[2], 3 } } },
    { NEST_HERD, T(60),  T(35), T(4), FAM_SHEEP, false,     // alternatif çayır
      { { &MOB_TYPES[2], 3 } } },
    { NEST_HERD, T(35),  T(20), T(4), FAM_PIG, true,
      { { &MOB_TYPES[3], 2 } } },
    { NEST_HERD, T(50),  T(50), T(4), FAM_PIG, false,
      { { &MOB_TYPES[3], 2 } } },
    { NEST_HERD, T(22),  T(17), T(3), FAM_CHICKEN, true,
      { { &MOB_TYPES[4], 3 } } },
    { NEST_HERD, T(30),  T(28), T(3), FAM_CHICKEN, false,
      { { &MOB_TYPES[4], 3 } } },
    { NEST_HERD, T(108), T(22), T(4), FAM_BOAR, true,
      { { &MOB_TYPES[5], 2 } } },
    { NEST_HERD, T(90),  T(40), T(4), FAM_BOAR, false,
      { { &MOB_TYPES[5], 2 } } },
};
const int NEST_SITE_COUNT = (int)(sizeof(NEST_SITES) / sizeof(NEST_SITES[0]));

static NestState states[64];        // NEST_SITE_COUNT <= 64
static int zoneSite[MAX_SPAWN_ZONES];   // zoneId → saha (populate kurar)
static int zoneEntry[MAX_SPAWN_ZONES];  // zoneId → giriş

void ecology_reset(void)
{
    memset(states, 0, sizeof(states));
    for (int i = 0; i < NEST_SITE_COUNT; i++) {
        if (!NEST_SITES[i].startColonized) continue;
        states[i].colonized = true;
        states[i].chiefAlive = true;
        for (int e = 0; e < NEST_MAX_ENTRIES; e++)
            states[i].pop[e] = (Sint16)NEST_SITES[i].entries[e].max;
    }
}

const NestSiteDef *ecology_site(int i) { return &NEST_SITES[i]; }
NestState *ecology_state(int i)        { return &states[i]; }

int ecology_site_of_zone(int zoneId)
{
    return (zoneId >= 0 && zoneId < MAX_SPAWN_ZONES) ? zoneSite[zoneId] : -1;
}

// Yuva noktası çevresine doğum (yuvadan "çıkar" — göz önünde belirme yok gibi
// hissettiren dar yarıçap)
static void nest_spawn(const NestSiteDef *s, const MobType *t,
                       Mob *mobs, int maxMobs, int *nextMobId, int zoneId)
{
    float ang = randf() * 6.2832f;
    float r = 20.0f + randf() * 40.0f;
    mob_spawn(mobs, maxMobs, nextMobId, t,
              s->x + SDL_cosf(ang) * r, s->y + SDL_sinf(ang) * r, zoneId);
}

int ecology_populate(SpawnZone *zones, int maxZones,
                     Mob *mobs, int maxMobs, int *nextMobId)
{
    int n = 0;
    for (int z = 0; z < MAX_SPAWN_ZONES; z++) { zoneSite[z] = -1; zoneEntry[z] = -1; }

    for (int i = 0; i < NEST_SITE_COUNT; i++) {
        const NestSiteDef *s = &NEST_SITES[i];
        NestState *st = &states[i];
        if (!st->colonized || st->destroyed) continue;

        for (int e = 0; e < NEST_MAX_ENTRIES; e++) {
            if (s->entries[e].type == NULL || n >= maxZones) continue;
            zones[n] = (SpawnZone){ .x = s->x, .y = s->y, .radius = s->radius,
                                    .type = s->entries[e].type,
                                    .maxCount = s->entries[e].max };
            zoneSite[n] = i;
            zoneEntry[n] = e;
            for (int k = 0; k < st->pop[e]; k++)
                nest_spawn(s, s->entries[e].type, mobs, maxMobs, nextMobId, n);
            n++;
        }
        // Reis/totem İLAVE doğar (ilk zone'a bağlı — pop sayımına girmez)
        if (st->chiefAlive && n > 0 &&
            (s->kind == NEST_CHIEF || s->kind == NEST_TOTEM)) {
            const MobType *ct = &MOB_TYPES[(s->kind == NEST_CHIEF)
                                           ? MOB_GOBLIN_CHIEF : MOB_ORC_TOTEM];
            mob_spawn(mobs, maxMobs, nextMobId, ct,
                      s->x, s->y - (s->kind == NEST_TOTEM ? 0.0f : 10.0f), n - 1);
        }
    }
    return n;
}

void ecology_dawn(int day, SpawnZone *zones, int zoneCount,
                  Mob *mobs, int maxMobs, int *nextMobId, bool onWorldMap)
{
    (void)zones;   // zone eşlemesi zoneSite/zoneEntry tablolarından okunur
    for (int i = 0; i < NEST_SITE_COUNT; i++) {
        const NestSiteDef *s = &NEST_SITES[i];
        NestState *st = &states[i];

        // Planlı göç günü geldi: boş sahaya küçük koloni yerleşir
        if (st->migrateDay > 0 && day >= st->migrateDay &&
            !st->colonized && !st->destroyed) {
            st->colonized = true;
            st->chiefAlive = true;
            st->migrateDay = 0;
            for (int e = 0; e < NEST_MAX_ENTRIES; e++)
                st->pop[e] = (Sint16)((s->entries[e].type != NULL) ? 2 : 0);
            // Zone'lar bir sonraki harita kuruluşunda oluşur; dünyadaysak
            // görünmesi de gerekir — main şafakta populate SONRASI değil,
            // mevcut zone düzeninde olmadığı için haritayı yeniden kurmaz;
            // koloni ilk harita yenilenişinde canlanır (dünya olayı hissi:
            // "yeni yerleşmişler" — oyuncu keşfeder).
            continue;
        }

        if (!st->colonized || st->destroyed) continue;

        // Şafak üremesi: yuva başına +1 (eksik olan İLK girişe)
        for (int e = 0; e < NEST_MAX_ENTRIES; e++) {
            if (s->entries[e].type == NULL) continue;
            if (st->pop[e] >= s->entries[e].max) continue;
            st->pop[e]++;
            if (onWorldMap) {
                // Bu sahanın zone'unu bul → moba dünyada da can ver
                for (int z = 0; z < zoneCount; z++)
                    if (zoneSite[z] == i && zoneEntry[z] == e) {
                        nest_spawn(s, s->entries[e].type,
                                   mobs, maxMobs, nextMobId, z);
                        break;
                    }
            }
            break;   // yuva başına GÜNDE 1
        }
    }
}

// Aile için boş alternatif saha ara ve göçü planla (5-15 gün sonra)
static void plan_migration(int family, int day)
{
    for (int i = 0; i < NEST_SITE_COUNT; i++) {
        if (NEST_SITES[i].family != family) continue;
        if (states[i].colonized || states[i].destroyed) continue;
        if (states[i].migrateDay > 0) return;   // zaten planlı
        states[i].migrateDay = (Sint16)(day + 5 + (int)(randf() * 10.0f));
        return;
    }
}

bool ecology_on_mob_death(int zoneId, bool isChief, int day)
{
    int i = ecology_site_of_zone(zoneId);
    if (i < 0) return false;   // zindan/mühür alanı — ekoloji dışı
    const NestSiteDef *s = &NEST_SITES[i];
    NestState *st = &states[i];

    if (isChief) {
        // Reis/totem düştü: yuva ÇÖKER (kalıcı) + göç planlanır
        st->chiefAlive = false;
        st->destroyed = true;
        st->destroyedDay = (Sint16)day;
        plan_migration(s->family, day);
        return true;
    }

    int e = (zoneId >= 0 && zoneId < MAX_SPAWN_ZONES) ? zoneEntry[zoneId] : -1;
    if (e >= 0 && st->pop[e] > 0) st->pop[e]--;

    // Sürü bitti: kalan yok — saha terk edilir, sürü "göçer"
    if (s->kind == NEST_HERD) {
        int total = 0;
        for (int k = 0; k < NEST_MAX_ENTRIES; k++) total += st->pop[k];
        if (total <= 0) {
            st->destroyed = true;   // bu çayır boşaldı (kalıcı)
            st->destroyedDay = (Sint16)day;
            plan_migration(s->family, day);
        }
    }
    return false;
}

int ecology_ritual_near(float px, float py, const Mob *mobs, int maxMobs)
{
    for (int i = 0; i < NEST_SITE_COUNT; i++) {
        const NestSiteDef *s = &NEST_SITES[i];
        NestState *st = &states[i];
        if (s->kind != NEST_POOL && s->kind != NEST_GRAVE) continue;
        if (!st->colonized || st->destroyed) continue;
        float dx = s->x - px, dy = s->y - py;
        if (dx * dx + dy * dy > 80.0f * 80.0f) continue;

        // Sahanın tüm mobları ölü mü? (canlı VEYA ölmekte olan varsa hayır)
        bool anyAlive = false;
        for (int m = 0; m < maxMobs; m++) {
            if (!mobs[m].e.alive && !mobs[m].dying) continue;
            int ms = ecology_site_of_zone(mobs[m].zoneId);
            if (ms == i) { anyAlive = true; break; }
        }
        if (!anyAlive) return i;
    }
    return -1;
}

void ecology_do_ritual(int site, int day)
{
    if (site < 0 || site >= NEST_SITE_COUNT) return;
    states[site].destroyed = true;
    states[site].destroyedDay = (Sint16)day;
    // Mühürlenen mezar göç ÜRETMEZ (mühür kalıcı); gölcük ailesi göçer
    if (NEST_SITES[site].kind == NEST_POOL)
        plan_migration(NEST_SITES[site].family, day);
}
