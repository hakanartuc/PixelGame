#ifndef SNEAK_H
#define SNEAK_H
#include <stdbool.h>
#include "mob.h"

// Gizlilik modeli: mob algısı dört faktörle çarpılır —
//   1) eğilme (C): taban 0.45, skill ile 0.30'a iner (yalnız sneak'teyken)
//   2) görüş açısı: mobun ARKASINDAYSAN ×0.5 (her zaman — duyma payı kalır)
//   3) siper: aradaki dekor (ev tabanı) görüşü kesiyorsa ×0.4 (her zaman)
//   4) örtü: gizleyen tile'da (çim/saman — TileType.conceals) EĞİLİRKEN ×0.6
//      (ayakta örtünün üstünden görünürsün; eğilince içine girersin)
// Eğilmek kesin gizlilik DEĞİLDİR; aggro olan mob leash'e kadar peşini bırakmaz.

#define SNEAK_WITNESS_RANGE 160.0f   // eğilirken suç tanık yarıçapı (normal 350)
#define SNEAK_XP_TICK   4.0f         // tehlike yanında süzülme XP'si (sn başına)
#define SNEAK_XP_CRIME  25.0f        // fark edilmeden tanıksız suç
#define SNEAK_XP_BACKSTAB 15.0f      // sinsi vuruş

float sneak_speed_mult(int level);   // eğilme hız cezası: 0.5 → skill 100'de 0.7

// Mob algı çarpanı (gece/perk çarpanlarının ÜSTÜNE): eğilme × açı × siper × örtü
// inConceal: oyuncunun bastığı tile gizleyici mi (main tile_at_point ile verir)
float sneak_detect_mul(bool sneaking, int sneakLevel, bool inConceal,
                       float px, float py, const Mob *m);

// İki nokta arasında dekor tabanı (ev) görüşü kesiyor mu?
bool sneak_sight_blocked(float x1, float y1, float x2, float y2);

#endif
