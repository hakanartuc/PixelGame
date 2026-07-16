#ifndef TITLE_H
#define TITLE_H
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "mob.h"

// DAVRANIŞ ÜNVANLARI (M3): "kimlik davranıştan doğar"ın vitrini.
// Gizli sayaçlar oyun stilinden birikir; eşik geçilince ünvan KENDİLİĞİNDEN
// açılır (bildirim + günlük). Tüm açık ünvanların pasifi kalıcı işler;
// ayrıca TEK "taşınan" ünvan seçilir: pasifi ×2 + isim yanında görünür.

typedef enum {
    TS_KILL_SLIME = 0,
    TS_KILL_GOBLIN,
    TS_KILL_ORC,
    TS_SNEAK_HITS,
    TS_NIGHT_KILLS,
    TS_NESTS,        // yuva çöküşü/ritüeli (oyuncu eliyle)
    TS_KILL_ANIMAL,
    TS_ARROWS,       // atılan ok
    TS_POTIONS,      // içilen iksir
    TS_COOKED,       // pişirilen yemek
    TS_RECORD,       // S3: nöbet devralındı (0/1 — yalnız hikaye seçimiyle)
    TS_COUNT,
} TitleStat;

typedef struct {
    const char *name;
    const char *effect;   // panelde gösterilen pasif açıklaması
    TitleStat stat;
    int threshold;
} TitleDef;

#define TITLE_COUNT 11
extern const TitleDef TITLES[TITLE_COUNT];

struct Player;   // forward

bool title_unlocked(const struct Player *p, int title);

// Sayaç artır; BU artışla yeni açılan ünvanın indeksi döner (-1 = yok).
// (Aynı anda tek eşik geçer — sayaçlar 1'er artar.)
int title_stat_add(struct Player *p, TitleStat stat, int n);

// --- Pasif etkiler (taşınan ×2 dahil) ---
// Aile/gece/yuva hasar çarpanı: mob türüne ve saate göre (vuruş anında çağır)
float title_damage_mult(const struct Player *p, const MobType *t, bool night);
float title_backstab_bonus(const struct Player *p);   // sinsi çarpana eklenir
float title_bow_mult(const struct Player *p);         // yay hasarı çarpanı
float title_potion_mult(const struct Player *p);      // iksir etki çarpanı
float title_food_dur_mult(const struct Player *p);    // Tok süre çarpanı
float title_double_loot_chance(const struct Player *p); // av çift ürün şansı (0..1)
float title_max_mana_bonus(const struct Player *p);      // Kayıt Tutan: +max mana

#endif
