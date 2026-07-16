#include "weather.h"
#include "common.h"   // randf
#include "gametime.h" // GAMETIME_DAY_SECONDS

static Weather current = WEATHER_CLEAR;
static float hoursLeft = 0.0f;      // yağmur/fırtına kalan süresi (oyun saati)
static float intensity = 0.0f;      // 0..1 görsel/etki şiddeti (yumuşak geçiş)
static bool rainedToday = false;    // doğa beslenir bayrağı (şafakta okunur)
static bool middayRolled = false;   // öğlen zarı günde bir kez
static bool debugForce = false;     // DEV: F8 döngüsü — sis saat şartını aşar

void weather_reset(void)
{
    current = WEATHER_CLEAR;
    hoursLeft = 0.0f;
    intensity = 0.0f;
    rainedToday = false;
    middayRolled = false;
}

// Havuzdan çek: Açık %55 / Yağmur %25 / Sis %15 / Fırtına %5
static Weather roll_pool(void)
{
    float r = randf();
    if (r < 0.55f) return WEATHER_CLEAR;
    if (r < 0.80f) return WEATHER_RAIN;
    if (r < 0.95f) return WEATHER_FOG;
    return WEATHER_STORM;
}

static void start_weather(Weather w)
{
    current = w;
    if (w == WEATHER_RAIN || w == WEATHER_STORM) {
        hoursLeft = 2.0f + randf() * 4.0f;   // 2-6 saat yağar
        rainedToday = true;
    } else {
        hoursLeft = 0.0f;   // sis saate bakar (05-10), açık süresiz
    }
}

void weather_dawn(int day)
{
    (void)day;
    rainedToday = false;    // dünün bayrağı şafakta okunur — SONRA çağrılmalı
    middayRolled = false;
    debugForce = false;     // debug hava yeni günle biter
    start_weather(roll_pool());
}

void weather_debug_cycle(void)
{
    debugForce = true;
    start_weather((Weather)((current + 1) % WEATHER_COUNT));
    if (current == WEATHER_RAIN || current == WEATHER_STORM)
        hoursLeft = 6.0f;   // testte erken dinmesin
}

void weather_update(float dt, float hours)
{
    // Öğlen zarı (12:00'dan sonra bir kez): %25 hava döner
    if (!middayRolled && hours >= 12.0f) {
        middayRolled = true;
        if (randf() < 0.25f) {
            Weather nw = roll_pool();
            if (nw != current) start_weather(nw);
        }
    }

    // Yağmur/fırtına süresi dolunca açar
    if ((current == WEATHER_RAIN || current == WEATHER_STORM) && hoursLeft > 0.0f) {
        hoursLeft -= dt * (24.0f / GAMETIME_DAY_SECONDS);
        if (hoursLeft <= 0.0f) { current = WEATHER_CLEAR; hoursLeft = 0.0f; }
    }

    // Sis yalnız sabah yoğun (05-10) — öğlen kendiliğinden dağılır
    bool active = (current == WEATHER_RAIN || current == WEATHER_STORM) ||
                  (current == WEATHER_FOG &&
                   (debugForce || (hours >= 5.0f && hours < 10.0f)));

    // Yumuşak geçiş: yağmur ~6 sn'de bastırır, ~10 sn'de diner
    float target = active ? 1.0f : 0.0f;
    float rate = (target > intensity) ? (1.0f / 6.0f) : (1.0f / 10.0f);
    intensity += (target - intensity) * rate * dt * 6.0f;
    if (intensity < 0.001f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;
}

Weather weather_current(void) { return current; }
float weather_intensity(void) { return intensity; }
bool weather_rained_today(void) { return rainedToday; }

const char *weather_name(Weather w)
{
    switch (w) {
        case WEATHER_RAIN:  return "Yağmur";
        case WEATHER_FOG:   return "Sis";
        case WEATHER_STORM: return "Fırtına";
        default:            return "Açık";
    }
}

// --- Etki sorguları: intensity ile ölçekli (yağmur bastırırken etki de büyür) ---

bool weather_is_wet(void)
{
    return (current == WEATHER_RAIN || current == WEATHER_STORM) && intensity > 0.3f;
}

float weather_burn_time_mult(void)
{
    if (current != WEATHER_RAIN && current != WEATHER_STORM) return 1.0f;
    return 1.0f - 0.5f * intensity;    // tam yağmurda ×0.5
}

float weather_fire_mult(void)
{
    if (current != WEATHER_RAIN && current != WEATHER_STORM) return 1.0f;
    return 1.0f - 0.15f * intensity;   // tam yağmurda -%15
}

float weather_bolt_mult(void)
{
    if (current != WEATHER_RAIN && current != WEATHER_STORM) return 1.0f;
    return 1.0f + 0.20f * intensity;   // ıslak iletken: +%20
}

float weather_percept_mult(void)
{
    if (current != WEATHER_FOG) return 1.0f;
    return 1.0f - 0.4f * intensity;    // yoğun siste ×0.6
}

float weather_darkness01(void)
{
    if (current == WEATHER_STORM) return 0.45f * intensity;   // fırtına gündüzü loşlaştırır
    if (current == WEATHER_RAIN)  return 0.20f * intensity;
    return 0.0f;
}

void weather_save_get(int *cur, float *hl, int *rained)
{
    *cur = (int)current;
    *hl = hoursLeft;
    *rained = rainedToday ? 1 : 0;
}

void weather_save_set(int cur, float hl, int rained)
{
    current = (cur >= 0 && cur < WEATHER_COUNT) ? (Weather)cur : WEATHER_CLEAR;
    hoursLeft = hl;
    rainedToday = rained != 0;
    middayRolled = true;    // yükleme günü öğlen zarı tekrarlanmasın (güvenli taraf)
    intensity = 0.0f;       // görsel yumuşak bastırır
}
