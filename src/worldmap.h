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
    MAP_KONAK,        // Eşikçi Konağı (S3 — loş ama zindan DEĞİL: dalga/mob yok)
    MAP_MEZARLIK,     // Lanetli Mezarlık (açık hava zindanı — undead tileset)
    MAP_LOCA,         // Sessiz El Locası (kuyu dibi — Suikastçi P1; güvenli, dalga yok)
    MAP_DOGU,         // Taşdurak — Doğu sınır kasabası (Kıta planı ilk adım; çöl)
    MAP_COUNT,
} MapId;

typedef struct {
    const char *name;      // girişte splash
    const char *csvPath;
    MusicMood mood;        // sabit harita mood'u (dünyada bölge sistemi kazanır)
    float ambient;         // 0 = gametime'a bağlı (dünya); >0 = sabit karanlık çarpanı
    float sunriseOffset;   // gündoğumu kayması (saat): doğu haritaları NEGATİF
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

// Boss durumu (save v16): haftalık diriliş + benzersiz ilk-ödül
int  boss_last_kill_day(void);               // -999 = hiç ölmedi
void boss_set_last_kill_day(int day);
// İkinci boss (Ölüm Getiren — mezarlık): save'e TOLERANSLI KUYRUK olarak
// yazılır (eski save'lerde alan yok → varsayılanla açılır, sürüm bump'sız)
int  boss2_last_kill_day(void);
void boss2_set_last_kill_day(int day);
bool boss2_first_kill_done(void);
void boss2_set_first_kill_done(bool done);
bool boss_first_kill_done(void);
void boss_set_first_kill_done(bool done);

// --- Anlatı noktaları (Faz A) ---
// İki tür, tek makine (vinyet sahnesi — scene.c çalar):
//   ECHO_ANI:   ölünün son anları. Mor soluk + [E] Dokun. BİR KEZ yaşanır
//               (seçim yapılınca echo_done, save v17) — anı iki kez okunmaz.
//   ECHO_YAZIT: taş yazıt/kitap — çevresel anlatı. Görseli decorTex çizer,
//               [E] Oku. TEKRAR okunabilir (fx'siz sahne done işaretlenmez).
typedef enum { ECHO_ANI = 0, ECHO_YAZIT } EchoKind;

typedef struct {
    EchoKind kind;
    MapId map;
    float x, y;             // dünya px (soluk merkezi / taşın TABANI)
    const char *scenePath;  // assets/scenes/*.txt
    const char *decorTex;   // dünyada duran görsel (NULL = yok; yazıt taşı vb.)
    float decorScale;
} EchoDef;
extern const EchoDef ECHOES[];
extern const int ECHO_COUNT;

// Yazıt/anı decor görselleri + en yakına [E] etiketi (mor soluğu main çizer —
// nabız efekti glow dokusuyla orada yaşıyor)
void map_echo_decor_render(R2D *renderer, const Camera *cam);

bool echo_done(int idx);
void echo_set_done(int idx, bool done);

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
