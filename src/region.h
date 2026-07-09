#ifndef REGION_H
#define REGION_H
#include <SDL3/SDL.h>
#include "mob.h"
#include "music.h"   // MusicMood — bölgenin müzik ruh hali

// Bölge sistemi: tek büyük harita üstünde MANTIKSAL dikdörtgen alanlar.
// Zorluk coğrafyaya sabittir (Gothic/Souls) — level scaling yok:
// bölgenin mob kampları neyse o, oyuncu seviyesine bakılmaz.
// Bölgeye ilk girişte adı ekranda belirir (Skyrim tarzı splash — main çizer).

// Bir bölgedeki mob kampı (KO "spawn kampı")
typedef struct {
    float x, y;          // kamp merkezi (dünya pikseli)
    float radius;        // doğma yarıçapı
    const MobType *type;
    int maxCount;        // kampta aynı anda kaç mob
} RegionCamp;

typedef struct {
    const char *name;    // splash'ta gösterilen ad
    SDL_FRect rect;      // bölge alanı (dünya pikseli)
    MusicMood mood;      // bölge müziği (savaş bunu geçici ezer — main)
    const RegionCamp *camps;
    int campCount;
} Region;

extern const Region REGIONS[];
extern const int REGION_COUNT;

// Noktanın bölge indeksi (-1 = hiçbir bölgede). Çakışmada ilk kazanır.
int region_at(float worldX, float worldY);

#endif
