#include <stdio.h>
#include "gametime.h"

// Faz sınırları (saat)
#define DAWN_START  5.0f
#define DAY_START   7.0f
#define DUSK_START 19.0f
#define NIGHT_START 21.0f

#define DARKNESS_MAX 170   // zifiri gecede overlay alpha

static int   curDay = 1;
static float curHours = GAMETIME_START_HOUR;

void gametime_init(void)
{
    curDay = 1;
    curHours = GAMETIME_START_HOUR;
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

DayPhase gametime_phase(void)
{
    if (curHours >= NIGHT_START || curHours < DAWN_START) return PHASE_NIGHT;
    if (curHours >= DUSK_START) return PHASE_DUSK;
    if (curHours >= DAY_START)  return PHASE_DAY;
    return PHASE_DAWN;
}

bool gametime_is_night(void)
{
    return gametime_phase() == PHASE_NIGHT;
}

Uint8 gametime_darkness(void)
{
    // Gündüz 0, gecede DARKNESS_MAX; dusk/dawn'da doğrusal geçiş
    switch (gametime_phase()) {
        case PHASE_DAY:
            return 0;
        case PHASE_NIGHT:
            return DARKNESS_MAX;
        case PHASE_DUSK: {
            float t = (curHours - DUSK_START) / (NIGHT_START - DUSK_START);   // 0→1
            return (Uint8)(DARKNESS_MAX * t);
        }
        case PHASE_DAWN:
        default: {
            float t = (curHours - DAWN_START) / (DAY_START - DAWN_START);     // 0→1
            return (Uint8)(DARKNESS_MAX * (1.0f - t));
        }
    }
}

void gametime_format(char *buf, int bufSize)
{
    int h = (int)curHours;
    int m = (int)((curHours - h) * 60.0f);
    snprintf(buf, bufSize, "Gun %d  %02d:%02d", curDay, h, m);
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
