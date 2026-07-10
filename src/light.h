#ifndef LIGHT_H
#define LIGHT_H
#include "r2d.h"
#include "common.h"
#include "fx.h"   // WeaponFxType — silah efekti gece ışık verir

// Dünya-uzayı ışık kaynakları (gece ışık pass'ine girer).
// Gündüz ambient 1.0 olduğundan katkıları görünmez — açık/kapalı mantığı gerekmez.

typedef struct {
    float x, y;        // dünya px
    float radius;      // dünya px — bu uzaklıkta katkı sıfır
    float r, g, b;     // ışık rengi (0..1)
    float intensity;   // katkı gücü (0..1 tipik)
    float flicker;     // titreşim genliği (0 = sabit, 0.1-0.15 = meşale)
} LightDef;

extern const LightDef LIGHTS[];
extern const int LIGHT_COUNT;

// Statik ışıklar + oyuncu ışıklarını (meşale + silah efekti) ekran-uzayına toplar.
// sw/sh: ekran boyutu (ekran dışı ışıklar atlanır). Dönen: out'a yazılan adet.
int light_collect(R2DLight *out, int cap, const Camera *cam, int sw, int sh,
                  float torchX, float torchY, bool torchOn, WeaponFxType weaponFx);

// Kamp ateşi ışığı (Faz 6): konumu dinamik (oyuncu kurar) — çağıran dünya
// konumu verir, ekran dışıysa eklenmez. Dönen: yeni ışık adedi (n ya da n+1).
int light_append_campfire(R2DLight *out, int n, int cap, const Camera *cam,
                          int sw, int sh, float x, float y);

#endif
