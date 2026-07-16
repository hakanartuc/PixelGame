#ifndef AMBIENCE_H
#define AMBIENCE_H
#include <stdbool.h>

// Ambiyans katmanı (Cila): müziğin ALTINDA hafif ortam sesi — crossfade'li.
// music.c'nin küçük kardeşi: tür başına stream, kesintisiz loop besleme.
//   AMB_DAY     → açık arazi gündüz (kuş + rüzgar)
//   AMB_NIGHT   → açık arazi gecesi (cırcır)
//   AMB_DUNGEON → zindan (uğultu + damla)
//   AMB_NONE    → menü/başlık (sessiz)

typedef enum {
    AMB_DAY = 0,
    AMB_NIGHT,
    AMB_DUNGEON,
    AMB_KIND_COUNT,
    AMB_NONE = AMB_KIND_COUNT,   // hepsi fade-out
} AmbKind;

bool ambience_init(void);
void ambience_set(AmbKind k);          // aynıysa no-op; geçiş crossfade
void ambience_set_volume(float v01);   // kullanıcı müzik seviyesiyle ölçeklenir

// Yağmur katmanı: day/night'ın ÜSTÜNE biner (crossfade havuzundan bağımsız).
// g01 = hedef şiddet (weather_intensity) — geçişi ambience_update yumuşatır.
void ambience_set_rain(float g01);
void ambience_update(float dt);
void ambience_quit(void);

#endif
