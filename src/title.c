#include <string.h>
#include "title.h"
#include "player.h"

const TitleDef TITLES[TITLE_COUNT] = {
    { "Slime Kasabı",  "Slime'lara +%5 hasar",        TS_KILL_SLIME,  100 },
    { "Goblin Belası", "Goblinlere +%5 hasar",        TS_KILL_GOBLIN, 100 },
    { "Ork Kıran",     "Orklara +%5 hasar",           TS_KILL_ORC,     75 },
    { "Gölge",         "Sinsi çarpanı +0.1",          TS_SNEAK_HITS,   50 },
    { "Gece Kuşu",     "Gece +%3 hasar",              TS_NIGHT_KILLS, 100 },
    { "Yuva Yıkıcı",   "Reis/toteme +%10 hasar",      TS_NESTS,         5 },
    { "Usta Avcı",     "Av çift ürün +%10 şans",      TS_KILL_ANIMAL, 100 },
    { "Nişancı",       "Yay hasarı +%3",              TS_ARROWS,      300 },
    { "Kadeh",         "İksir etkisi +%5",            TS_POTIONS,      50 },
    { "Aşçı",          "Tok süresi +%20",             TS_COOKED,       25 },
};

bool title_unlocked(const struct Player *p, int title)
{
    if (title < 0 || title >= TITLE_COUNT) return false;
    return p->titleStats[TITLES[title].stat] >= TITLES[title].threshold;
}

int title_stat_add(struct Player *p, TitleStat stat, int n)
{
    if (stat < 0 || stat >= TS_COUNT) return -1;
    int before = p->titleStats[stat];
    p->titleStats[stat] = before + n;
    for (int i = 0; i < TITLE_COUNT; i++)
        if (TITLES[i].stat == stat &&
            before < TITLES[i].threshold &&
            p->titleStats[stat] >= TITLES[i].threshold)
            return i;
    return -1;
}

// Taşınan bu ünvansa pasif ×2 (kimlik seçiminin ödülü)
static float carry_mul(const struct Player *p, int title)
{
    return (p->carriedTitle == title) ? 2.0f : 1.0f;
}

// Açıksa bonusu (taşıma çarpanıyla), değilse 0
static float bonus(const struct Player *p, int title, float base)
{
    return title_unlocked(p, title) ? base * carry_mul(p, title) : 0.0f;
}

float title_damage_mult(const struct Player *p, const MobType *t, bool night)
{
    float m = 1.0f;
    if (t != NULL) {
        int idx = (int)(t - MOB_TYPES);
        if (idx >= 6 && idx <= 11)                       // slime ailesi
            m += bonus(p, 0, 0.05f);
        if (idx == 0 || idx == MOB_GOBLIN_CHIEF)         // goblin + reis
            m += bonus(p, 1, 0.05f);
        if (idx == 1 || idx == MOB_ORC_TOTEM)            // ork + totem
            m += bonus(p, 2, 0.05f);
        if (idx == MOB_GOBLIN_CHIEF || idx == MOB_ORC_TOTEM)
            m += bonus(p, 5, 0.10f);                     // Yuva Yıkıcı
    }
    if (night) m += bonus(p, 4, 0.03f);                  // Gece Kuşu
    return m;
}

float title_backstab_bonus(const struct Player *p) { return bonus(p, 3, 0.10f); }
float title_bow_mult(const struct Player *p)       { return 1.0f + bonus(p, 7, 0.03f); }
float title_potion_mult(const struct Player *p)    { return 1.0f + bonus(p, 8, 0.05f); }
float title_food_dur_mult(const struct Player *p)  { return 1.0f + bonus(p, 9, 0.20f); }
float title_double_loot_chance(const struct Player *p) { return bonus(p, 6, 0.10f); }
