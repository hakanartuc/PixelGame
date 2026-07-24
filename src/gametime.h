#ifndef GAMETIME_H
#define GAMETIME_H
#include <SDL3/SDL.h>

// Oyun-içi zaman: bir tam gün = GAMETIME_DAY_SECONDS gerçek saniye.
// Fazlar karanlık overlay'ini, gece tehlikesini ve NPC rutinlerini sürer.
// (adı time.h ile çakışmasın diye "gametime")

#define GAMETIME_DAY_SECONDS 300.0f   // 5 dk = 24 saat
#define GAMETIME_START_HOUR 8.0f      // oyun sabah 08:00'de başlar

typedef enum {
    PHASE_DAWN,    // karanlık çözülür (sınırlar mevsime + haritaya göre kayar)
    PHASE_DAY,     // aydınlık
    PHASE_DUSK,    // kararır
    PHASE_NIGHT,   // karanlık (tehlike saatleri)
} DayPhase;

// Mevsimler (Faz DOGU-P2): 10 günde bir döner — yıl 40 gün.
// Gündüz uzunluğu mevsime bağlı: yaz 05-21, kış 08-17.
#define SEASON_DAYS 10
typedef enum {
    SEASON_SPRING = 0,
    SEASON_SUMMER,
    SEASON_AUTUMN,
    SEASON_WINTER,
    SEASON_COUNT,
} Season;

Season gametime_season(void);            // günden türetilir (save'e girmez)
const char *gametime_season_name(void);  // "İlkbahar" / "Yaz" / ...

// Harita gündoğumu kayması (saat): doğu haritaları NEGATİF (erken doğar,
// erken batar). Harita geçişinde main çağırır (MapDef.sunriseOffset).
void gametime_set_sun_offset(float hours);

void gametime_init(void);
void gametime_update(float dt);

int   gametime_day(void);     // 1'den başlar
float gametime_hours(void);   // 0..24 (kesirli: 13.5 = 13:30)
DayPhase gametime_phase(void);
bool  gametime_is_night(void);   // NIGHT fazı mı (mob algısı/spawn için)

// Karanlık overlay alpha'sı (0 = gündüz, 170 = zifiri) — fazlar arası yumuşak
Uint8 gametime_darkness(void);

// Aynı eğri 0..1 normalize (ışık pass'i ambient'i için)
float gametime_darkness01(void);

// HUD saati: "Gun 3  08:30" (buf en az 24 byte)
void gametime_format(char *buf, int bufSize);

// Save/load köprüsü
void gametime_get(int *day, float *hours);
void gametime_set(int day, float hours);

#endif
