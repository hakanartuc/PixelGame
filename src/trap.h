#ifndef TRAP_H
#define TRAP_H
#include "r2d.h"
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "common.h"
#include "worldmap.h"

// Zindan tuzakları — iki tür, iki farklı beceri testi:
//   Dikenler: periyodik açılır/kapanır (uyarı evresi var) — RİTMİ izle, aralıkta geç
//   Plaka+arbalet: plakaya basmak duvardaki arbaleti tetikler — DİKKAT et, bolt kaçılabilir
// Tanım tabloları worldmap.c'de (harita verisiyle beraber); durum burada.

#define SPIKE_DAMAGE 15.0f
#define BOLT_DAMAGE  20.0f
#define BOLT_SPEED   340.0f

// Arbalet atış isteği — main mob mermi havuzuna ekler
typedef struct {
    float x, y, vx, vy;
    float dmg;
} TrapShot;

bool trap_init(R2D *renderer);   // sprite'ları yükle
void trap_quit(void);

// Harita değişiminde çağır: plaka/diken durumları sıfırlanır
void trap_reset(void);

// Diken temas hasarı döner (0 = yok; tuzak başına vuruş bekleme içeride).
// Plaka tetiklenirse shots'a istek yazar; dönen ikinci değer shot sayısı.
float trap_update(float dt, MapId map, SDL_FRect playerFeet,
                  TrapShot *shots, int maxShots, int *outShotCount);

// Diken + plaka + arbalet çizimi (zemin katmanı — entity'lerden önce)
void trap_render(R2D *renderer, MapId map, const Camera *cam);

#endif
