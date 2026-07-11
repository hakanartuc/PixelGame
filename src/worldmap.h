#ifndef WORLDMAP_H
#define WORLDMAP_H
#include "r2d.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "common.h"
#include "music.h"
#include "region.h"   // RegionCamp (kamp tanımı bölge sistemiyle ortak)

// Çoklu harita sistemi: dünya + zindan katları. Her harita kendi CSV'si,
// mood'u, ambient ışığı, kampları ve GEÇİTLERİYLE (portal) tanımlanır.
// Geçiş akışının kendisi main'dedir (mob/loot/minimap sıfırlama oradaki
// dizilere dokunur) — bu modül VERİYİ ve aktif harita durumunu tutar.

typedef enum {
    MAP_WORLD = 0,
    MAP_DUNGEON_F1,   // Eski Geçit — Kat 1 (Dungeon_1 tileset)
    MAP_DUNGEON_F2,   // Kat 2 (Dungeon_2 tileset + boss)
    MAP_COUNT,
} MapId;

typedef struct {
    const char *name;      // girişte splash
    const char *csvPath;
    MusicMood mood;        // sabit harita mood'u (dünyada bölge sistemi kazanır)
    float ambient;         // 0 = gametime'a bağlı (dünya); >0 = sabit karanlık çarpanı
} MapDef;

// Geçit: E ile kullanılır. Tek yönlü — çift yön = iki satır.
typedef struct {
    MapId from;
    float x, y;            // geçit konumu (from haritasında, dünya px)
    float radius;          // etkileşim yarıçapı
    MapId to;
    float tx, ty;          // varış noktası (to haritasında)
    const char *label;     // "[E] Zindana in"
    const char *decorTex;  // görsel (NULL = görünmez)
    float decorScale;
} MapPortal;

extern const MapDef MAPS[MAP_COUNT];
extern const MapPortal PORTALS[];
extern const int PORTAL_COUNT;

// Geçit görselleri (texture yükleme)
bool worldmap_init(R2D *renderer);
void worldmap_quit(void);

MapId map_current(void);
void  map_set_current(MapId id);   // durumu günceller (yükleme akışı main'de)
bool  map_is_dungeon(MapId id);

// Aktif haritada, noktaya menzilli geçit indeksi (-1 = yok)
int map_portal_near(float px, float py);

// Aktif haritanın geçit görselleri + en yakındakine [E] etiketi
void map_portals_render(R2D *renderer, const Camera *cam, TTF_Font *font, int nearIdx);

// Aktif haritanın mob kampları (dünya: NULL — REGIONS kullanılır)
const RegionCamp *map_camps(MapId id, int *count);

// Zindan duvar meşaleleri: ışık listesine ekler (dünya: dokunmaz).
// Dönen: yeni ışık sayısı.
int map_append_lights(MapId id, R2DLight *out, int n, int cap,
                      const Camera *cam, int sw, int sh);

// --- Zindan sandıkları (kilit kademeli, GÜNLÜK yeniden kilitlenir) ---
typedef struct {
    MapId map;
    float x, y;        // dünya px (sandık tabanı)
    Sint8 lockTier;    // -1 kilitsiz, 0 Basit, 1 Sağlam, 2 Usta
} ChestDef;
extern const ChestDef CHESTS[];
extern const int CHEST_COUNT;

// Sandık günlük kilit durumu (save v15 köprüsü — durum burada yaşar)
int  chest_opened_day(int idx);              // -999 = hiç açılmadı
void chest_set_opened_day(int idx, int day);

// --- Tuzaklar ---
typedef struct { MapId map; float x, y; float phaseOffset; } SpikeDef;   // periyodik diken
typedef struct {
    MapId map;
    float plateX, plateY;   // basınç plakası
    float bowX, bowY;       // duvardaki arbalet
    float dirX, dirY;       // atış yönü (normalize)
} BoltTrapDef;
extern const SpikeDef SPIKES[];
extern const int SPIKE_COUNT;
extern const BoltTrapDef BOLT_TRAPS[];
extern const int BOLT_TRAP_COUNT;

#endif
