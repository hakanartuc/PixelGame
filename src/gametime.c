#include <stdio.h>
#include "gametime.h"

// Faz sınırları mevsime göre değişir (Faz DOGU-P2 — Hakan onaylı):
// yazın gün uzun (05-21), kışın kısa (08-17). Şafak/alacakaranlık 2 saat.
// Sıra: İlkbahar, Yaz, Sonbahar, Kış (SEASON_* enum sırası).
static const struct { float sunrise, sunset; } SUN[SEASON_COUNT] = {
    [SEASON_SPRING] = { 6.0f, 19.0f },
    [SEASON_SUMMER] = { 5.0f, 21.0f },
    [SEASON_AUTUMN] = { 7.0f, 19.0f },
    [SEASON_WINTER] = { 8.0f, 17.0f },
};
#define TWILIGHT 2.0f     // şafak/alacakaranlık geçiş süresi (saat)

#define DARKNESS_MAX 170   // zifiri gecede overlay alpha

static int   curDay = 1;
static float curHours = GAMETIME_START_HOUR;
static float sunOffset = 0.0f;   // harita gündoğumu kayması (saat; doğu < 0)

void gametime_init(void)
{
    curDay = 1;
    curHours = GAMETIME_START_HOUR;
    sunOffset = 0.0f;
}

void gametime_update(float dt)
{
    curHours += dt * (24.0f / GAMETIME_DAY_SECONDS);
    while (curHours >= 24.0f) {
        curHours -= 24.0f;
        curDay++;
    }
}

int   gametime_day(void)   { return curDay; }
float gametime_hours(void) { return curHours; }

Season gametime_season(void)
{
    return (Season)(((curDay - 1) / SEASON_DAYS) % SEASON_COUNT);
}

const char *gametime_season_name(void)
{
    static const char *NAMES[SEASON_COUNT] =
        { "İlkbahar", "Yaz", "Sonbahar", "Kış" };
    return NAMES[gametime_season()];
}

void gametime_set_sun_offset(float hours)
{
    sunOffset = hours;
}

// Mevsim + harita offset'iyle yerel güneş sınırları.
// Doğu haritasında offset negatif: güneş ERKEN doğar ve ERKEN batar.
static void sun_bounds(float *dawnStart, float *dayStart,
                       float *duskStart, float *nightStart)
{
    Season s = gametime_season();
    *dayStart   = SUN[s].sunrise + sunOffset;
    *dawnStart  = *dayStart - TWILIGHT;
    *duskStart  = SUN[s].sunset + sunOffset;
    *nightStart = *duskStart + TWILIGHT;
}

// Güneşin gökteki ilerlemesi: şafak başı 0 → gece başı 1, gece boyunca -1.
// Gölge yönü ve uzunluğu bundan türetilir (shadow.c). Mevsim ve harita
// gündoğumu kayması sun_bounds içinde zaten hesaplı, tekrar etmiyoruz.
float gametime_sun_t(void)
{
    float dawnS, dayS, duskS, nightS;
    sun_bounds(&dawnS, &dayS, &duskS, &nightS);
    if (curHours < dawnS || curHours >= nightS) return -1.0f;   // güneş yok
    return (curHours - dawnS) / (nightS - dawnS);
}

DayPhase gametime_phase(void)
{
    float dawnS, dayS, duskS, nightS;
    sun_bounds(&dawnS, &dayS, &duskS, &nightS);
    if (curHours >= nightS || curHours < dawnS) return PHASE_NIGHT;
    if (curHours >= duskS) return PHASE_DUSK;
    if (curHours >= dayS)  return PHASE_DAY;
    return PHASE_DAWN;
}

bool gametime_is_night(void)
{
    return gametime_phase() == PHASE_NIGHT;
}

Uint8 gametime_darkness(void)
{
    float dawnS, dayS, duskS, nightS;
    sun_bounds(&dawnS, &dayS, &duskS, &nightS);
    // Gündüz 0, gecede DARKNESS_MAX; dusk/dawn'da doğrusal geçiş
    switch (gametime_phase()) {
        case PHASE_DAY:
            return 0;
        case PHASE_NIGHT:
            return DARKNESS_MAX;
        case PHASE_DUSK: {
            float t = (curHours - duskS) / (nightS - duskS);   // 0→1
            return (Uint8)(DARKNESS_MAX * t);
        }
        case PHASE_DAWN:
        default: {
            float t = (curHours - dawnS) / (dayS - dawnS);     // 0→1
            return (Uint8)(DARKNESS_MAX * (1.0f - t));
        }
    }
}

float gametime_darkness01(void)
{
    return (float)gametime_darkness() / (float)DARKNESS_MAX;
}

void gametime_format(char *buf, int bufSize)
{
    int h = (int)curHours;
    int m = (int)((curHours - h) * 60.0f);
    snprintf(buf, bufSize, "Gün %d · %s  %02d:%02d",
             curDay, gametime_season_name(), h, m);
}

void gametime_get(int *day, float *hours)
{
    *day = curDay;
    *hours = curHours;
}

void gametime_set(int day, float hours)
{
    curDay = (day >= 1) ? day : 1;
    curHours = (hours >= 0.0f && hours < 24.0f) ? hours : GAMETIME_START_HOUR;
}
