#ifndef DECOR_H
#define DECOR_H
#include "r2d.h"
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "common.h"

// Dekor: dünyaya konan statik sprite'lar (ev, çit, ağaç...).
// Tabanları katıdır (oyuncu/mob giremez); NPC rutinleri ev kapılarına gider.
// İç mekanlar ileride: kapı tile'ı + ayrı harita.

typedef struct {
    const char *texPath;   // tek parça sprite (Cute_Fantasy building vb.)
    float x, y;            // TABAN ORTA noktası (dünya pikseli) — ayak hizası
    float scale;           // kaynak piksel → dünya çarpanı
    bool solidBase;        // tabanı katı mı (ev/ağaç girer, çalı girmez)
    bool conceals;         // içine girilebilir gizlenme (çalı/saman — sneak örtüsü)
    int map;               // hangi haritada (0 = MAP_WORLD; Lanetli Mezarlık = MAP_MEZARLIK)
} DecorDef;

#define DECOR_TEX_CAP 64   // texture dizileri tavanı (main + decor.c AYNI sabit)
extern const DecorDef DECORS[];
extern const int DECOR_COUNT;

// Harita geçişi: aktif haritanın dekorları işler (map alanıyla filtre —
// dekorsuz haritalarda (zindan katları) hepsi kapalı)
void decor_set_active(bool active);
void decor_set_map(int mapId);   // aktif harita (filtre; main geçişte basar)

// Hepsini çiz (tilemap'ten sonra, entity'lerden önce)
void decor_render_all(R2D *renderer, R2DTexture **decorTextures,
                      const Camera *camera);

// Güneş gölgeleri: zeminin üstüne, dekorun altına (shadow.h'den parametreler).
// strength 0 ise (gece) hiç çizmez.
void decor_render_shadows(R2D *renderer, R2DTexture **decorTextures,
                          const Camera *camera,
                          float dx, float dy, float lenMul, float strength);

// IŞIĞI KESEN GÖVDELER — ekran uzayı dikdörtgenleri, en yakın `cap` tanesi.
// Nesnenin AYAK İZİ + gövdesi: ağacın tacı ışığı kesmez (yapraklar arasından
// sızar), gövdesi keser. Ev tüm yüksekliğiyle keser.
// Dönen: yazılan adet. cx/cy = öncelik merkezi (oyuncu konumu).
int decor_light_occluders(SDL_FRect *out, int cap, const Camera *camera,
                          float cx, float cy);

// Kutu herhangi bir dekorun katı tabanına çarpıyor mu?
// (taban = sprite'ın alt %35'i — çatının arkasından yürünebilir)
bool decor_blocks(SDL_FRect box);

// Nokta gizleyici bir dekorun (çalı/saman) içinde mi? (sneak örtüsü)
bool decor_conceals(float x, float y);

// Texture boyutları render/collision için gerekli — yükleme sonrası bir kez çağır
void decor_set_texture_sizes(R2DTexture **decorTextures);

#endif
