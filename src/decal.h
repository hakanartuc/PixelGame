#ifndef DECAL_H
#define DECAL_H
#include "r2d.h"
#include "common.h"

// YER İZLERİ — zemine kalan geçici lekeler: yanık, kırağı, zehir birikintisi, kan.
//
// AnimFx (fx.h) bir kez oynayıp söner: patlamanın KENDİSİ. Decal ise patlamanın
// BIRAKTIĞI şey — saniyeler boyunca durur, sonra solar. İkisi birlikte "bir şey
// oldu ve burada oldu" hissini kuruyor; efekt tek başına iz bırakmadan biterse
// dünya hiçbir şey hatırlamıyor gibi duruyor.
//
// Çizim sırası: tilemap'ten SONRA, varlıklardan ÖNCE (zeminin üstünde, ayak altında).
//
// Doku kodda üretilir (asset beklemez, gölge lekesindeki yöntem). Kenar bilinçli
// olarak DÜZENSİZ: kusursuz daire yapay duruyor — ışık halkalarında öğrendiğimiz
// ders burada da geçerli.

#define MAX_DECALS 64

typedef struct {
    float x, y;        // dünya MERKEZ noktası
    float radius;      // dünya px
    float r, g, b;     // renk (0..1)
    float alpha;       // tepe saydamsızlık (0..1)
    float life, maxLife;   // kalan / toplam ömür (sn)
    float rot;         // 0/90/180/270 seçimi (0..3) — aynı doku tekrar etmesin
    bool active;
} Decal;

// Dokuyu üretir. Bir kez, r2d hazır olduktan sonra çağrılır.
void decal_init(R2D *renderer);

void decal_reset(void);   // hepsini temizle (harita geçişi / yükleme)

// Havuz doluysa EN ESKİYİ ezer — yeni iz her zaman görünür (sessizce kaybolmaz).
void decal_spawn(float x, float y, float radius,
                 float r, float g, float b, float alpha, float life);

void decal_update(float dt);
void decal_render(R2D *renderer, const Camera *camera);

// Hazır tarifler — çağıranın renk/ömür ezberlemesi gerekmesin.
void decal_scorch(float x, float y, float radius);   // yanık: koyu, uzun ömürlü
void decal_frost(float x, float y, float radius);    // kırağı: soluk mavi-beyaz
void decal_poison(float x, float y, float radius);   // zehir: hasta yeşil
void decal_blood(float x, float y, float radius);    // kan: koyu kırmızı

#endif
