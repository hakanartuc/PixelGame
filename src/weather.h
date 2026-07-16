#ifndef WEATHER_H
#define WEATHER_H
#include <stdbool.h>

// HAVA SİSTEMİ (Dünya Olayları 2): her şafak zar — Açık %50 / Yağmur %22 /
// Sis %13 / Kar %10 / Fırtına %5. Öğlen (12:00) ikinci küçük zar: %25 döner.
// Yağmur 2-6 saat sürer (biterse Açık'a döner); sis 05-10 arası yoğundur,
// öğlen kendiliğinden dağılır. Etkiler main'de işlenir; bu modül yalnız
// durum + süre + geçiş yumuşatması (intensity) tutar.
//
// SAVAŞ BAĞI (M1 sinerjileriyle simetri — her hava birine kazandırır):
//   Yağmur/Fırtına: yanma ×0.5, ateş -%15, yıldırım +%20 (ıslak iletken)
//   Sis: mob algısı ×0.6 (sinsi fırsatı), hedef görüşü kısalır
//   Fırtına: rastgele yıldırım düşer (açık alan riski)
//   Kar (soğuk cephe): buz büyüleri +%20 + yavaşlatma süresi ×1.5;
//     herkes %12 yavaş (kuru soğuk — ateş/yıldırım NORMAL işler)

typedef enum {
    WEATHER_CLEAR = 0,
    WEATHER_RAIN,
    WEATHER_FOG,
    WEATHER_STORM,
    WEATHER_SNOW,    // soğuk cephe (v26 uyumlu: eski save'ler 0-3 yazar)
    WEATHER_COUNT,
} Weather;

// Şafak zarı: günün havası (worldevent_dawn gibi day-rollover'da çağrılır)
void weather_dawn(int day);

// Saatlik akış: öğlen zarı + yağmur süresi + sis dağılması + intensity yumuşatma.
// Her kare çağrılır (hours = gametime_hours()).
void weather_update(float dt, float hours);

Weather weather_current(void);
const char *weather_name(Weather w);

// Görsel/etki şiddeti 0..1 (geçişler ani değil — yağmur "bastırır", sis "çöker")
float weather_intensity(void);

// --- Etki sorguları (main/combat kısa yoldan okur; intensity işlenmiş) ---
bool  weather_is_wet(void);          // yağmur/fırtına aktif (intensity > 0.3)
float weather_burn_time_mult(void);  // yanma DoT süresi çarpanı (ıslakta 0.5)
float weather_fire_mult(void);       // ateş hasarı çarpanı (ıslakta 0.85)
float weather_bolt_mult(void);       // yıldırım hasarı çarpanı (ıslakta 1.2)
float weather_percept_mult(void);    // mob algı menzili çarpanı (siste 0.6)
float weather_darkness01(void);      // ek karartma 0..1 (fırtına gündüzü loşlaştırır)

// --- Kar (soğuk cephe) ---
bool  weather_is_snowy(void);        // kar aktif (intensity > 0.3)
float weather_ice_mult(void);        // buz büyüsü hasar çarpanı (karda 1.2)
float weather_slow_dur_mult(void);   // buz yavaşlatma SÜRESİ çarpanı (karda 1.5)
float weather_speed_mult(void);      // herkes yavaş: oyuncu+mob hız çarpanı (karda 0.88)
float weather_snow_cover(void);      // zemin beyazlığı 0..1 (yağarken birikir, sonra erir)

// DEV/test: havayı sıradakine döndürür (Açık→Yağmur→Sis→Fırtına→Açık).
// Sis normalde 05-10 arası yoğundur; debug modunda saat şartı aranmaz
// (bir sonraki şafak zarı debug bayrağını temizler).
void weather_debug_cycle(void);

// Save köprüsü (v26)
void weather_save_get(int *cur, float *hoursLeft, int *rainedToday);
void weather_save_set(int cur, float hoursLeft, int rainedToday);
void weather_reset(void);

// Dün yağmur yağdı mı? (doğa beslenir: bitki respawn ×2 — şafakta okunur,
// weather_dawn çağrısı bayrağı sıfırlar)
bool weather_rained_today(void);

#endif
