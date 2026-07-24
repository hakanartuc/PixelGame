#include "worldmap.h"
#include "render.h"
#include "font.h"

#define T(n) ((n) * (float)TILE_SIZE)

// ---------------------------------------------------------------------------
// Harita tanımları
// ---------------------------------------------------------------------------
const MapDef MAPS[MAP_COUNT] = {
    [MAP_WORLD] = {
        .name = "",   // dünya splash'i bölge sisteminden gelir
        .csvPath = "assets/maps/world.csv",
        .mood = MOOD_WILD,      // kullanılmaz (bölge kazanır) — güvenli varsayılan
        .ambient = 0.0f,        // 0 = gün/gece döngüsüne bağlı
    },
    [MAP_DUNGEON_F1] = {
        .name = "Eski Gecit — Kat 1",
        .csvPath = "assets/maps/dungeon_f1.csv",
        .mood = MOOD_DUNGEON,
        .ambient = 0.35f,       // her saatte loş — meşaleler taşır
    },
    [MAP_DUNGEON_F2] = {
        .name = "Eski Gecit — Derinlik",
        .csvPath = "assets/maps/dungeon_f2.csv",
        .mood = MOOD_DUNGEON,
        .ambient = 0.28f,       // daha karanlık (boss katı)
    },
    [MAP_KONAK] = {
        .name = "Eşikçi Konağı",
        .csvPath = "assets/maps/konak.csv",
        .mood = MOOD_NIGHT,     // sakin-tekinsiz koyu pad (zindan droni DEĞİL)
        .ambient = 0.55f,       // loş ama okunur
    },
    [MAP_MEZARLIK] = {
        .name = "Lanetli Mezarlık",
        .csvPath = "assets/maps/mezarlik.csv",
        .mood = MOOD_DUNGEON,
        .ambient = 0.45f,       // kalıcı alacakaranlık (açık hava ama lanetli)
    },
    [MAP_LOCA] = {
        .name = "Kuyunun Dibi",
        .csvPath = "assets/maps/loca.csv",
        .mood = MOOD_DUNGEON,
        .ambient = 0.30f,   // meşale loşluğu — ama zindan DEĞİL (dalga/mob yok)
    },
    [MAP_DOGU] = {
        .name = "Taşdurak — Sınır Kasabası",
        .csvPath = "assets/maps/dogu.csv",
        .sunriseOffset = -0.75f,   // doğu: güneş 45 dk erken doğar/batar
        .mood = MOOD_WILD,
        .ambient = 0.0f,   // açık hava — gün/gece döngüsü işler
    },
};

// ---------------------------------------------------------------------------
// Geçitler — koordinatlar kat haritalarının SABİT yerleşimiyle eşleşir
// (assets/maps/dungeon_f*.csv scripts/gen_dungeon.py'den; oda düzeni orada)
// ---------------------------------------------------------------------------
const MapPortal PORTALS[] = {
    // DOĞU (Taşdurak): kervan yolunun doğu ucu — çöl geçidi
    { .from = MAP_WORLD, .x = T(197), .y = T(19), .radius = 80.0f,
      .to = MAP_DOGU, .tx = T(4), .ty = T(35),
      .label = "[E] Çöl Geçidi — Taşdurak", .decorTex = NULL },
    { .from = MAP_DOGU, .x = T(2.5f), .y = T(35), .radius = 80.0f,
      .to = MAP_WORLD, .tx = T(195.5f), .ty = T(19),
      .label = "[E] Batıya — Kervan Yolu", .decorTex = NULL },

    // SUİKASTÇİ P1: Kör Kuyu'nun dibi — ip yalnız koşul sağlanınca görünür
    // (main: gece + 500g aklama YA DA loca zaten keşfedilmiş)
    { .from = MAP_WORLD, .x = T(26), .y = T(15), .radius = 60.0f,
      .to = MAP_LOCA, .tx = T(5.5f), .ty = T(5.5f),
      .label = "[R] İpten aşağı in", .decorTex = NULL },
    { .from = MAP_LOCA, .x = T(5.5f), .y = T(4.2f), .radius = 60.0f,
      .to = MAP_WORLD, .tx = T(26), .ty = T(16.2f),
      .label = "[E] İpten yukarı çık", .decorTex = NULL },

    // S3: Eşikçi Konağı — kuzeydoğu tepe eteği (harabe kapısı)
    { .from = MAP_WORLD, .x = T(168), .y = T(45), .radius = 80.0f,
      .to = MAP_KONAK, .tx = T(4), .ty = T(10),
      .label = "[E] Konağa gir", .decorTex = "assets/decor/dungeon_arch.png",
      .decorScale = 2.4f },
    { .from = MAP_KONAK, .x = T(3.5f), .y = T(10), .radius = 70.0f,
      .to = MAP_WORLD, .tx = T(168), .ty = T(46),
      .label = "[E] Dışarı çık", .decorTex = "assets/decor/dungeon_stairs_up.png",
      .decorScale = 1.8f },

    // Dünya (Karanlık Orman açıklığı) → Kat 1 giriş odası
    { .from = MAP_WORLD, .x = T(145), .y = T(7), .radius = 80.0f,
      .to = MAP_DUNGEON_F1, .tx = T(8), .ty = T(7),
      .label = "[E] Eski Gecit'e in", .decorTex = "assets/decor/dungeon_arch.png",
      .decorScale = 3.0f },

    // Kat 1 giriş odası → Dünya (kemerin önü)
    { .from = MAP_DUNGEON_F1, .x = T(6), .y = T(6), .radius = 70.0f,
      .to = MAP_WORLD, .tx = T(145), .ty = T(8),
      .label = "[E] Yuzeye cik", .decorTex = "assets/decor/dungeon_stairs_up.png",
      .decorScale = 2.0f },

    // Kat 1 dip oda → Kat 2
    { .from = MAP_DUNGEON_F1, .x = T(36), .y = T(28), .radius = 70.0f,
      .to = MAP_DUNGEON_F2, .tx = T(7), .ty = T(7),
      .label = "[E] Derinlige in", .decorTex = "assets/decor/stairs_down.png",
      .decorScale = 3.5f },

    // Kayalık Geçit mezar sahaları yanı → Lanetli Mezarlık (güney girişi)
    { .from = MAP_WORLD, .x = T(122), .y = T(83), .radius = 80.0f,
      .to = MAP_MEZARLIK, .tx = T(30), .ty = T(41),
      .label = "[E] Mezarlığa gir", .decorTex = "assets/decor/undead/skull_door.png",
      .decorScale = 2.2f },   // görünür mezar sahasının (yuva T120,81) bitişiği
    { .from = MAP_MEZARLIK, .x = T(30), .y = T(43), .radius = 80.0f,
      .to = MAP_WORLD, .tx = T(122), .ty = T(84),
      .label = "[E] Mezarlıktan çık", .decorTex = NULL,
      .decorScale = 1.0f },

    // Kat 2 giriş → Kat 1 (merdiven başı)
    { .from = MAP_DUNGEON_F2, .x = T(5.5f), .y = T(5.5f), .radius = 70.0f,
      .to = MAP_DUNGEON_F1, .tx = T(35), .ty = T(28),
      .label = "[E] Yukari cik", .decorTex = "assets/decor/dungeon_stairs_up.png",
      .decorScale = 2.0f },
};
const int PORTAL_COUNT = (int)(sizeof(PORTALS) / sizeof(PORTALS[0]));

// ---------------------------------------------------------------------------
// Zindan kampları (MOB_TYPES indeksleri: 12 Savaşçı / 13 Okçu / 14 Büyücü)
// Oda düzenine göre — giriş odası (A) BOŞ bırakılır (nefes alanı)
// ---------------------------------------------------------------------------
static const RegionCamp F1_CAMPS[] = {
    { .x = T(24), .y = T(8),  .radius = T(3), .type = &MOB_TYPES[12], .maxCount = 2 }, // B odası: savaşçılar
    { .x = T(11), .y = T(21), .radius = T(3), .type = &MOB_TYPES[13], .maxCount = 2 }, // C odası: okçular
    { .x = T(28), .y = T(24), .radius = T(4), .type = &MOB_TYPES[12], .maxCount = 2 }, // D odası: karışık
    { .x = T(33), .y = T(26), .radius = T(3), .type = &MOB_TYPES[13], .maxCount = 1 },
};
// Lanetli Mezarlık: iskelet kampları (mühür dalgasıyla tazelenir — açık hava)
static const RegionCamp MZ_CAMPS[] = {
    { .x = T(18), .y = T(30), .radius = T(4), .type = &MOB_TYPES[12], .maxCount = 3 }, // güneybatı mezarlar
    { .x = T(42), .y = T(30), .radius = T(4), .type = &MOB_TYPES[13], .maxCount = 2 }, // güneydoğu — okçular
    { .x = T(30), .y = T(18), .radius = T(5), .type = &MOB_TYPES[12], .maxCount = 3 }, // dev iskelet önü
    { .x = T(16), .y = T(12), .radius = T(4), .type = &MOB_TYPES[14], .maxCount = 2 }, // kuzeybatı — büyücüler
    { .x = T(44), .y = T(12), .radius = T(4), .type = &MOB_TYPES[13], .maxCount = 2 }, // kuzeydoğu
    // Faz MOB-2: mezarlık dalgası
    { .x = T(50), .y = T(26), .radius = T(4), .type = &MOB_TYPES[MOB_GHOUL_WIZARD],  .maxCount = 1 }, // doğu harabe
    { .x = T(10), .y = T(22), .radius = T(4), .type = &MOB_TYPES[MOB_GHOUL_WIZARD],  .maxCount = 1 }, // batı sırt
    { .x = T(34), .y = T(40), .radius = T(5), .type = &MOB_TYPES[MOB_BLOOD_MONSTER], .maxCount = 3 }, // güney ölü su
};
// Taşdurak çölü (Faz DOGU): tapınak vadisi mumyaları + akrep yuvaları +
// kervan yolu hayduları — kasaba alanı (x8-32) TEMİZ bırakılır
static const RegionCamp DOGU_CAMPS[] = {
    { .x = T(78), .y = T(17), .radius = T(5), .type = &MOB_TYPES[MOB_MUMMY],  .maxCount = 3 }, // tapınak
    { .x = T(88), .y = T(24), .radius = T(4), .type = &MOB_TYPES[MOB_MUMMY],  .maxCount = 2 },
    { .x = T(62), .y = T(56), .radius = T(5), .type = &MOB_TYPES[MOB_SCARAB], .maxCount = 4 }, // güney yuva
    { .x = T(84), .y = T(52), .radius = T(4), .type = &MOB_TYPES[MOB_SCARAB], .maxCount = 3 },
    { .x = T(52), .y = T(34), .radius = T(4), .type = &MOB_TYPES[MOB_BANDIT], .maxCount = 2 }, // yol kesen
    { .x = T(70), .y = T(38), .radius = T(4), .type = &MOB_TYPES[MOB_BANDIT], .maxCount = 2 },
};

static const RegionCamp F2_CAMPS[] = {
    { .x = T(20), .y = T(9),  .radius = T(4), .type = &MOB_TYPES[12], .maxCount = 2 }, // salon
    { .x = T(23), .y = T(11), .radius = T(3), .type = &MOB_TYPES[14], .maxCount = 1 }, // salon: büyücü
    { .x = T(18), .y = T(23), .radius = T(4), .type = &MOB_TYPES[13], .maxCount = 2 }, // boss ön odası
    { .x = T(27), .y = T(24), .radius = T(3), .type = &MOB_TYPES[14], .maxCount = 1 },
};

const RegionCamp *map_camps(MapId id, int *count)
{
    switch (id) {
        case MAP_DUNGEON_F1:
            *count = (int)(sizeof(F1_CAMPS) / sizeof(F1_CAMPS[0]));
            return F1_CAMPS;
        case MAP_DUNGEON_F2:
            *count = (int)(sizeof(F2_CAMPS) / sizeof(F2_CAMPS[0]));
            return F2_CAMPS;
        case MAP_MEZARLIK:
            *count = (int)(sizeof(MZ_CAMPS) / sizeof(MZ_CAMPS[0]));
            return MZ_CAMPS;
        case MAP_DOGU:
            *count = (int)(sizeof(DOGU_CAMPS) / sizeof(DOGU_CAMPS[0]));
            return DOGU_CAMPS;
        default:
            *count = 0;
            return NULL;   // dünya: REGIONS kampları (main kurar)
    }
}

// ---------------------------------------------------------------------------
// Zindan duvar meşaleleri (oda köşeleri / kavşaklar)
// ---------------------------------------------------------------------------
typedef struct { float x, y; } TorchPos;
static const TorchPos F1_TORCHES[] = {
    { T(8),  T(4.5f) }, { T(24), T(4.5f) }, { T(11), T(16.5f) },
    { T(30), T(18.5f) }, { T(36), T(25.5f) }, { T(16), T(22) },
};
static const TorchPos F2_TORCHES[] = {
    { T(7), T(4.5f) }, { T(20), T(4.5f) }, { T(14), T(18.5f) },
    { T(30), T(18.5f) }, { T(22), T(27.5f) },
};

int map_append_lights(MapId id, R2DLight *out, int n, int cap,
                      const Camera *cam, int sw, int sh)
{
    const TorchPos *torches = NULL;
    int torchCount = 0;
    if (id == MAP_DUNGEON_F1) { torches = F1_TORCHES; torchCount = 6; }
    if (id == MAP_DUNGEON_F2) { torches = F2_TORCHES; torchCount = 5; }
    if (torches == NULL) return n;

    float t = (float)SDL_GetTicks() / 1000.0f;
    float zoom = cam->zoom;
    for (int i = 0; i < torchCount && n < cap; i++) {
        float sx = (torches[i].x - cam->x) * zoom;
        float sy = (torches[i].y - cam->y) * zoom;
        float sr = 210.0f * zoom;
        if (sx + sr < 0 || sx - sr > (float)sw ||
            sy + sr < 0 || sy - sr > (float)sh)
            continue;   // ekran dışı
        // İki frekanslı titreşim (light.c deseniyle aynı his)
        float fl = 1.0f + 0.12f * (SDL_sinf(t * 9.0f + i * 1.7f) * 0.6f
                                 + SDL_sinf(t * 23.0f + i * 3.1f) * 0.4f);
        out[n++] = (R2DLight){ sx, sy, sr, 1.00f, 0.60f, 0.26f, 0.72f * fl };
    }
    return n;
}

// ---------------------------------------------------------------------------
// Sandıklar — oda düzenine göre (gen_dungeon.py yerleşimi). Günlük kilit
// durumunu main tutar (save v15: açılan gün).
// ---------------------------------------------------------------------------
const ChestDef CHESTS[] = {
    { MAP_DUNGEON_F1, T(13), T(24), -1 },   // C odası dibi: kilitsiz (öğretici tat)
    { MAP_DUNGEON_F1, T(27), T(6),   0 },   // B odası: Basit
    { MAP_DUNGEON_F1, T(31), T(28),  0 },   // D odası: Basit
    { MAP_DUNGEON_F2, T(24), T(5),   1 },   // salon: Sağlam
    { MAP_DUNGEON_F2, T(15), T(26),  1 },   // boss odası köşe: Sağlam
    { MAP_DUNGEON_F2, T(30), T(21),  2 },   // boss odası dip: USTA (nadir ödül)
    { MAP_MEZARLIK,   T(12), T(9),   1 },   // mezarlık kuzeybatı harabesi: Sağlam
    { MAP_MEZARLIK,   T(30), T(13),  2 },   // dev iskeletin ardı: USTA (Hortum tomarı şansı)
};
const int CHEST_COUNT = (int)(sizeof(CHESTS) / sizeof(CHESTS[0]));

// Açıldığı OYUN günü (-999 = hiç) — gün dönünce otomatik "eski" kalır → kilitlenir
static int chestOpenedDay[16] = { [0 ... 15] = -999 };

int chest_opened_day(int idx)
{
    return (idx >= 0 && idx < 16) ? chestOpenedDay[idx] : -999;
}

void chest_set_opened_day(int idx, int day)
{
    if (idx >= 0 && idx < 16) chestOpenedDay[idx] = day;
}

// Boss: haftalık diriliş takibi + benzersiz silah bir kez düşer
static int bossKillDay = -999;
static bool bossFirstKill = false;

static int  boss2KillDay = -999;      // Ölüm Getiren (mezarlık)
static bool boss2FirstKill = false;
int  boss2_last_kill_day(void)         { return boss2KillDay; }
void boss2_set_last_kill_day(int day)  { boss2KillDay = day; }
bool boss2_first_kill_done(void)       { return boss2FirstKill; }
void boss2_set_first_kill_done(bool d) { boss2FirstKill = d; }

int  boss_last_kill_day(void)          { return bossKillDay; }
void boss_set_last_kill_day(int day)   { bossKillDay = day; }
bool boss_first_kill_done(void)        { return bossFirstKill; }
void boss_set_first_kill_done(bool d)  { bossFirstKill = d; }

// ---------------------------------------------------------------------------
// Anlatı noktaları — yankılar (bir kez) + yazıtlar (tekrar okunur). Faz A.
// ---------------------------------------------------------------------------
const EchoDef ECHOES[] = {
    // Köy doğusu, yol kenarı: devrik araba (ilk yankı prototipi)
    { ECHO_ANI, MAP_WORLD, T(20), T(8), "assets/scenes/echo_cart.txt", NULL, 0.0f },
    // Köy yolu kıyısı: eski dikilitaş — çevresel anlatı (tekrar okunabilir)
    { ECHO_YAZIT, MAP_WORLD, T(11), T(14), "assets/scenes/yazit_koy.txt",
      "assets/decor/obelisk.png", 1.6f },
    // K9 Kandil (D1a): F2 nöbetçi odası — Kerem'in sonu (bir kez yaşanır)
    { ECHO_ANI, MAP_DUNGEON_F2, T(20), T(6), "assets/scenes/echo_kerem.txt",
      NULL, 0.0f },
    // S3 (D1c): Eşikçi Konağı büyük salonu — Nuh'un kardeşi (bir kez)
    { ECHO_ANI, MAP_KONAK, T(21), T(12), "assets/scenes/echo_kardes.txt",
      NULL, 0.0f },
};
const int ECHO_COUNT = (int)(sizeof(ECHOES) / sizeof(ECHOES[0]));

static bool echoDone[16];   // save v17 köprüsü

bool echo_done(int idx)              { return (idx >= 0 && idx < 16) ? echoDone[idx] : true; }
void echo_set_done(int idx, bool d)  { if (idx >= 0 && idx < 16) echoDone[idx] = d; }

static R2DTexture *echoTex[16];   // ECHOES ile paralel decor görselleri

void map_echo_decor_render(R2D *renderer, const Camera *cam)
{
    float zoom = cam->zoom;
    for (int i = 0; i < ECHO_COUNT && i < 16; i++) {
        if (ECHOES[i].map != map_current() || echoTex[i] == NULL) continue;
        float w, h;
        r2d_texture_size(echoTex[i], &w, &h);
        float dw = w * ECHOES[i].decorScale, dh = h * ECHOES[i].decorScale;
        SDL_FRect dst = {
            (ECHOES[i].x - dw / 2.0f - cam->x) * zoom,
            (ECHOES[i].y - dh + 8.0f - cam->y) * zoom,   // taban hizası
            dw * zoom, dh * zoom,
        };
        r2d_draw_texture(renderer, echoTex[i], NULL, &dst);
    }
}

// ---------------------------------------------------------------------------
// Tuzaklar — koridor/oda girişleri (kaçınılabilir yerleşim: kenardan geçiş var)
// ---------------------------------------------------------------------------
const SpikeDef SPIKES[] = {
    { MAP_DUNGEON_F1, T(25.5f), T(13), 0.0f },   // B→D koridoru
    { MAP_DUNGEON_F1, T(25.5f), T(16), 0.9f },   // aynı koridor, faz kaymalı
    { MAP_DUNGEON_F1, T(18),    T(21.5f), 0.4f },// C→D koridoru
    { MAP_DUNGEON_F2, T(20.5f), T(15), 0.0f },   // F→G koridoru
    { MAP_DUNGEON_F2, T(20.5f), T(17), 1.1f },
};
const int SPIKE_COUNT = (int)(sizeof(SPIKES) / sizeof(SPIKES[0]));

const BoltTrapDef BOLT_TRAPS[] = {
    // K1: D odası girişi — plakaya basınca sağ duvardan sola bolt
    { MAP_DUNGEON_F1, T(21.5f), T(21.5f), T(23.5f), T(21.5f), -1.0f, 0.0f },
    // K2: boss koridoru — plakaya basınca soldan sağa bolt
    { MAP_DUNGEON_F2, T(20.5f), T(18.5f), T(19.2f), T(18.5f),  1.0f, 0.0f },
};
const int BOLT_TRAP_COUNT = (int)(sizeof(BOLT_TRAPS) / sizeof(BOLT_TRAPS[0]));

// ---------------------------------------------------------------------------
// Durum + geçit yardımcıları
// ---------------------------------------------------------------------------
static MapId curMap = MAP_WORLD;
static R2DTexture *portalTex[16];   // PORTALS ile paralel (küçük tavan)

bool worldmap_init(R2D *renderer)
{
    for (int i = 0; i < PORTAL_COUNT && i < 16; i++)
        portalTex[i] = (PORTALS[i].decorTex != NULL)
                     ? load_texture(renderer, PORTALS[i].decorTex) : NULL;
    for (int i = 0; i < ECHO_COUNT && i < 16; i++)
        echoTex[i] = (ECHOES[i].decorTex != NULL)
                   ? load_texture(renderer, ECHOES[i].decorTex) : NULL;
    return true;
}

void worldmap_quit(void)
{
    for (int i = 0; i < PORTAL_COUNT && i < 16; i++)
        if (portalTex[i]) { r2d_texture_destroy(NULL, portalTex[i]); portalTex[i] = NULL; }
    for (int i = 0; i < ECHO_COUNT && i < 16; i++)
        if (echoTex[i]) { r2d_texture_destroy(NULL, echoTex[i]); echoTex[i] = NULL; }
}

MapId map_current(void) { return curMap; }
void  map_set_current(MapId id) { if (id >= 0 && id < MAP_COUNT) curMap = id; }
bool  map_is_dungeon(MapId id)
{
    // MEZARLIK açık hava ama zindan MEKANİĞİ taşır: mühür dalgası respawn'ı,
    // sabit loşluk, NPC/hava yok — "lanet" sistemsel olarak zindan sayılır
    return id == MAP_DUNGEON_F1 || id == MAP_DUNGEON_F2 || id == MAP_MEZARLIK;
}

int map_portal_near(float px, float py)
{
    for (int i = 0; i < PORTAL_COUNT; i++) {
        if (PORTALS[i].from != curMap) continue;
        float dx = PORTALS[i].x - px, dy = PORTALS[i].y - py;
        if (SDL_sqrtf(dx * dx + dy * dy) <= PORTALS[i].radius)
            return i;
    }
    return -1;
}

void map_portals_render(R2D *renderer, const Camera *cam, TTF_Font *font, int nearIdx)
{
    float zoom = cam->zoom;
    for (int i = 0; i < PORTAL_COUNT; i++) {
        if (PORTALS[i].from != curMap) continue;

        if (i < 16 && portalTex[i] != NULL) {   // portalTex tavanıyla aynı (8'de kalmıştı — mezarlık kapısı görünmüyordu)
            float w, h;
            r2d_texture_size(portalTex[i], &w, &h);
            float dw = w * PORTALS[i].decorScale, dh = h * PORTALS[i].decorScale;
            SDL_FRect dst = {
                (PORTALS[i].x - dw / 2.0f - cam->x) * zoom,
                (PORTALS[i].y - dh + 8.0f - cam->y) * zoom,   // taban hizası
                dw * zoom, dh * zoom,
            };
            r2d_draw_texture(renderer, portalTex[i], NULL, &dst);
        }

        if (i == nearIdx && font != NULL) {
            SDL_Color hint = { 255, 230, 80, 255 };
            int tw = 0, th = 0;
            TTF_GetStringSize(font, PORTALS[i].label, 0, &tw, &th);
            draw_text(renderer, font, PORTALS[i].label,
                      (PORTALS[i].x - cam->x) * zoom - tw / 2.0f,
                      (PORTALS[i].y - 90.0f - cam->y) * zoom, hint);
        }
    }
}
