#ifndef GAMETIME_H
#define GAMETIME_H
#include <SDL3/SDL.h>

// Oyun-içi zaman: bir tam gün = GAMETIME_DAY_SECONDS gerçek saniye.
// Fazlar karanlık overlay'ini, gece tehlikesini ve NPC rutinlerini sürer.
// (adı time.h ile çakışmasın diye "gametime")

#define GAMETIME_DAY_SECONDS 600.0f   // 10 dk = 24 saat
#define GAMETIME_START_HOUR 8.0f      // oyun sabah 08:00'de başlar

typedef enum {
    PHASE_DAWN,    // 05-07: karanlık çözülür
    PHASE_DAY,     // 07-19: aydınlık
    PHASE_DUSK,    // 19-21: kararır
    PHASE_NIGHT,   // 21-05: karanlık (tehlike saatleri)
} DayPhase;

void gametime_init(void);
void gametime_update(float dt);

int   gametime_day(void);     // 1'den başlar
float gametime_hours(void);   // 0..24 (kesirli: 13.5 = 13:30)
DayPhase gametime_phase(void);
bool  gametime_is_night(void);   // NIGHT fazı mı (mob algısı/spawn için)

// Karanlık overlay alpha'sı (0 = gündüz, 170 = zifiri) — fazlar arası yumuşak
Uint8 gametime_darkness(void);

// HUD saati: "Gun 3  08:30" (buf en az 24 byte)
void gametime_format(char *buf, int bufSize);

// Save/load köprüsü
void gametime_get(int *day, float *hours);
void gametime_set(int day, float hours);

#endif
