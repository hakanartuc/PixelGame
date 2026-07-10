#ifndef RESOURCE_H
#define RESOURCE_H
#include "r2d.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "common.h"
#include "skill.h"

// Kaynak düğümleri: dünyada toplama noktaları (cevher kayası, bitki, ağaç).
// Toplama bir SÜREÇTİR: E ile başlar, karakter savurma animasyonuyla vurur,
// her vuruş 'work' ilerletir (skill hızlandırır). Hareket/hasar süreci keser;
// yarım ilerleme düğümde saklanır. Biten düğüm ERTESİ SABAH yenilenir.
// Araç şartı: KUŞANILI olmalı (silah slotu — WEAPON_TOOL).

#define RESOURCE_INTERACT_RANGE 80.0f
#define MAX_RESOURCES 64

typedef struct {
    const char *texPath;   // sprite sheet (assets/decor/*)
    SDL_FRect src;         // sheet'teki hücre
    float x, y;            // dünya taban-orta noktası
    float scale;           // kaynak px → dünya çarpanı
    int itemIdx;           // düşen malzeme (ItemId)
    int minCount, maxCount;
    SkillType skill;       // hangi toplama skill'i gelişir
    int toolItem;          // -1 = araç gerekmez; yoksa KUŞANILI şart (ItemId)
    const char *verb;      // E ipucu fiili ("Kaz" / "Topla" / "Kes")
    float work;            // gereken iş (vuruş/saniye başına gather_mult ilerler)
    int minSkill;          // bu seviyenin altında toplanamaz (0 = kapı yok)
} ResourceDef;

extern const ResourceDef RESOURCES[];
extern const int RESOURCE_COUNT;

void resource_reset(void);           // hepsi toplanabilir (oyun başı / load)
void resource_daily_respawn(void);   // gün değişince: toplananlar geri gelir

bool resource_available(int i);
void resource_harvest(int i);        // toplandı işaretle (süreç bitti)

// Süreç ilerlemesi (0..work): vuruşlar ekler, iptal SİLMEZ (gün sonuna dek durur)
float resource_progress(int i);
void  resource_add_progress(int i, float amount);

// E menzilindeki en yakın TOPLANABİLİR düğüm (-1 = yok)
int resource_nearest_available(float px, float py);

// Toplanabilir düğümleri çiz; nearestIdx'e [E] ipucu (verb) yazılır
void resource_render_all(R2D *renderer, R2DTexture **textures,
                         const Camera *camera, TTF_Font *font, int nearestIdx);

#endif
