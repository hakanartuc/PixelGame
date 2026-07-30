#ifndef LAMP_H
#define LAMP_H
#include "r2d.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "common.h"

// KANDİL DİREĞİ — LORE.md §1.1, §1.4, §1.5.
//
// LIGHTS[] statik tablosundan farkı: DURUMU var.
//   · Yakıtı (kor) biterse söner.
//   · Yanan direğin halkası içinde ölü KALKMAZ (§1.4) — bastırma yarıçapı
//     ışık yarıçapıyla AYNI: oyuncu gördüğü şeyle korunduğu şeyi ayırt etmesin.
//   · Bir ada yazılır (§5.1 bilezik): o direk bundan sonra o adı taşır.
//
// DİKKAT — §1.5: ışık korur VE ifşa eder. Bu modül yalnız "korur" tarafını
// verir; ifşa tarafı (gizlilik/tanık yarıçapı) light_level_at üstünden okunur.

#define LAMP_MAX             16      // save tamponu buna bağlı — büyütünce save.c!
#define LAMP_INTERACT_RANGE  76.0f
#define LAMP_RADIUS          122.0f  // ışık = bastırma yarıçapı (bilinçli aynı)
// 210 idi (3.3 karo): komşu halkalar birleşip tek aydınlık alan oluyordu ve
// gece 'karanlık' hissi kayboluyordu. 122 ≈ 1.9 karo — halkalar ayrı okunuyor.
#define LAMP_NIGHTS_PER_KOR  3       // bir kor kaç gece yakar

// Direği kim besliyor — bileziğe vurulan damga (§5.1).
// Save'de Uint8 olarak durur; sıra DEĞİŞMEZ, sona eklenir.
typedef enum {
    LAMP_OWNER_NONE = 0,   // kimsenin: sönük ya da sahipsiz yanıyor
    LAMP_OWNER_PLAYER,     // oyuncunun adına
    LAMP_OWNER_VILLAGE,    // köyün ortak kesesinden
    LAMP_OWNER_LORD,       // vadi lordu
    LAMP_OWNER_VEKIL,      // yol vekili (Coldhearth)
    LAMP_OWNER_PYRE,       // ölüyakanlar
    LAMP_OWNER_COUNT,
} LampOwner;

const char *lamp_owner_name(LampOwner o);   // UI metni

typedef struct {
    float x, y;
    const char *place;      // "Meydan" — UI ve tören metni
    int   startNights;      // yeni oyunda kalan gece (0 = sönük başlar)
    bool  graves;           // ALTINDA GÖMÜ VAR MI?
    // §1.4: ölü gömülü olduğu yerde kalkar. Direk yalnız BASTIRIR — kendisi
    // ölü üretmez. Meydanda gömü yoktur: sönük olsa da orada iskelet çıkmaz,
    // sadece karanlık ve fısıltı olur. Ölüler mezarlıktan gelir ve karanlık
    // sokaklarda yürür.
} LampDef;

extern const LampDef LAMPS[];
extern const int LAMP_COUNT;

// --- Durum (save) ---
void  lamp_reset(void);                       // yeni oyun / load öncesi
int   lamp_nights(int i);                     // kalan gece (0 = sönük)
void  lamp_set_nights(int i, int n);
LampOwner lamp_owner(int i);
void  lamp_set_owner(int i, LampOwner o);
bool  lamp_lit(int i);
void  lamp_add_kor(int i, int korCount);      // kor koy: yakıt +3/kor, yanmaya başlar

// Gün dönümünde çağrılır: yanan her direğin yakıtı 1 azalır.
// Sönen direk olduysa true döner (main uyarı basar).
bool  lamp_night_passed(void);

// --- Sorgular ---
// §1.5 — bu dünya noktasında kandil katkısı (0..1). Halka içinde 1'e yakın,
// kenarda 0. Ölünün kalkıp kalkmayacağı ve ifşa hesabı bunu okur.
float lamp_level_at(float x, float y);

// Yakın direk (E menzilinde, -1 yok)
int   lamp_nearest(float px, float py);

// --- Render ---
// Direği çizer (yanık/sönük ayrı görünür) + nearestIdx'e [E] ipucu.
void  lamp_render(R2D *renderer, const Camera *camera, TTF_Font *font,
                  int nearestIdx, float timeSec);

// Yanan direklerin ışığını R2DLight listesine ekler (light_append_point sarmalı).
int   lamp_append_lights(R2DLight *out, int n, int cap, const Camera *cam,
                         int sw, int sh);

#endif
