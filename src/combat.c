#include <SDL3/SDL.h>
#include "combat.h"
#include "player.h"
#include "equipment.h"

// Ayar sabitleri — denge tek yerden oynanır
#define STR_DAMAGE_PER_POINT   0.02f   // taban 10 üstü her STR puanı: +%2 hasar
#define SKILL_DAMAGE_PER_LEVEL 0.01f   // her skill seviyesi (1 üstü): +%1 hasar
#define CRIT_BASE_CHANCE       0.05f   // taban kritik şansı
#define CRIT_PER_DEX           0.005f  // taban 10 üstü her DEX puanı: +%0.5 şans
#define CRIT_MULTIPLIER        1.5f    // kritik çarpanı
#define STAT_BASE              10      // STR/DEX referans noktası
#define XP_PER_DAMAGE          0.5f    // 1 hasar = 0.5 XP
#define XP_LEVEL_GRACE         3       // mob'un bu kadar seviye üstüne kadar tam XP

SkillType combat_weapon_skill(const ItemType *weapon)
{
    if (weapon == NULL)
        return SKILL_NONE;   // silahsız — şimdilik skill geliştirmez

    switch (weapon->weaponType) {
        case WEAPON_ONE_HAND:
        case WEAPON_DUAL:      return SKILL_ONE_HANDED;
        case WEAPON_TWO_HAND:  return SKILL_TWO_HANDED;
        case WEAPON_SHIELD:    return SKILL_BLOCK;
        default:               return SKILL_NONE;
    }
}

// Varyans: 0.9–1.1 — aynı vuruş hep aynı sayıyı basmasın
static float damage_variance(void)
{
    return 0.9f + randf() * 0.2f;
}

// Defans azalan getirili: 100 def = yarı hasar, hiçbir zaman tam bağışıklık yok
static float apply_defense(float raw, float defense)
{
    if (defense < 0.0f)
        defense = 0.0f;
    return raw * 100.0f / (100.0f + defense);
}

float combat_attack_interval(const ItemStack *weapon)
{
    if (weapon != NULL && weapon->type != NULL && weapon->type->attackInterval > 0.0f)
        return weapon->type->attackInterval;
    return 1.0f;   // silahsız/tanımsız varsayılanı
}

HitResult combat_player_hit(const Player *p, const ItemStack *weapon,
                            float targetDefense, float abilityScale)
{
    HitResult hit = {0};

    // Taban: instance hasarı (tür × plus ölçeği); silahsızsa sabit
    bool armed = (weapon != NULL && weapon->type != NULL);
    float base = armed ? item_damage(weapon) : UNARMED_DAMAGE;

    // STR ölçeği (taban 10 = çarpansız)
    float strScale = 1.0f + (float)(p->strength - STAT_BASE) * STR_DAMAGE_PER_POINT;

    // Skill bonusu — silahın geliştirdiği skill'in seviyesinden
    float skillScale = 1.0f;
    SkillType sk = combat_weapon_skill(armed ? weapon->type : NULL);
    if (sk != SKILL_NONE)
        skillScale += (float)(p->skills[sk].level - 1) * SKILL_DAMAGE_PER_LEVEL;

    float raw = base * strScale * skillScale * abilityScale * damage_variance();

    // Kritik: DEX şansı, sabit çarpan
    float critChance = CRIT_BASE_CHANCE
                     + (float)(p->dexterity - STAT_BASE) * CRIT_PER_DEX;
    if (randf() < critChance) {
        raw *= CRIT_MULTIPLIER;
        hit.crit = true;
    }

    hit.damage = apply_defense(raw, targetDefense);
    return hit;
}

float equipment_total_defense(const Equipment *eq)
{
    float total = 0.0f;
    for (int s = 0; s < EQUIP_SLOT_COUNT; s++)
        total += item_defense(&eq->slots[s]);   // boş slot 0 döner
    return total;
}

float combat_mob_damage(float mobDamage, const Equipment *eq)
{
    float raw = mobDamage * damage_variance();
    return apply_defense(raw, equipment_total_defense(eq));
}

float combat_xp_for_hit(float damage, int mobLevel, int skillLevel)
{
    float xp = damage * XP_PER_DAMAGE;

    // Azalan getiri: skill, mob'un XP_LEVEL_GRACE üstüne çıktıysa her fark
    // seviyesi XP'yi hızla kısar — düşük bölgede sonsuz grind'i engeller.
    int diff = skillLevel - mobLevel - XP_LEVEL_GRACE;
    if (diff > 0)
        xp /= 1.0f + 0.5f * (float)diff;
    return xp;
}
