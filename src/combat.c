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
        case WEAPON_BOW:       return SKILL_ARCHERY;
        case WEAPON_STAFF:     return SKILL_STAFF;
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

float combat_attack_interval(const ItemStack *weapon, const PerkEffects *fx)
{
    float base = 1.0f;   // silahsız/tanımsız varsayılanı
    if (weapon != NULL && weapon->type != NULL && weapon->type->attackInterval > 0.0f)
        base = weapon->type->attackInterval;
    return base * fx->attackIntervalMult;   // Hızlı/Usta Bilek
}

HitResult combat_player_hit(const Player *p, const ItemStack *weapon,
                            float targetDefense, float abilityScale,
                            float targetHpRatio, const PerkEffects *fx)
{
    HitResult hit = {0};

    // Taban: instance hasarı (tür × plus ölçeği); silahsızsa sabit
    bool armed = (weapon != NULL && weapon->type != NULL);
    float base = armed ? item_damage(weapon) : UNARMED_DAMAGE;

    // STR ölçeği (taban 10 = çarpansız) — yay DEX ile ölçeklenir (nişancı statı;
    // DEX zaten crit verir, yayda hasara da işler — DEX taşlarına ikinci anlam)
    bool isBow = armed && weapon->type->weaponType == WEAPON_BOW;
    int dmgStat = isBow ? p->dexterity : p->strength;
    float strScale = 1.0f + (float)(dmgStat - STAT_BASE) * STR_DAMAGE_PER_POINT;

    // Skill bonusu — silahın geliştirdiği skill'in seviyesinden
    float skillScale = 1.0f;
    SkillType sk = combat_weapon_skill(armed ? weapon->type : NULL);
    if (sk != SKILL_NONE)
        skillScale += (float)(p->skills[sk].level - 1) * SKILL_DAMAGE_PER_LEVEL;

    // Perk: tek el hasar çarpanları (Güçlü Tutuş / Kılıç Ustası / Ezici Ağırlık)
    float perkScale = (sk == SKILL_ONE_HANDED) ? fx->oneHandDmgMult : 1.0f;

    // Perk: Zinde (can %50 üstünde) + Gazi (hasar sonrası 5 sn buff)
    if (fx->dmgWhenHealthy > 0.0f && p->health > p->maxHealth * 0.5f)
        perkScale *= 1.0f + fx->dmgWhenHealthy;
    if (fx->gaziDmgBonus > 0.0f && p->gaziTimer > 0.0f)
        perkScale *= 1.0f + fx->gaziDmgBonus;

    // Perk: İnfaz — can %20 altındaki hedefe ek hasar
    if (fx->executeBonus > 0.0f && targetHpRatio < 0.20f)
        perkScale *= 1.0f + fx->executeBonus;

    // Yay ağacı: yay hasar çarpanı (Berserker'ın yay karşılığı düğümlerde)
    if (isBow)
        perkScale *= fx->bowDmgMult;

    // Asa ağacı: asa melee'si (zayıf tabanı düğümler toparlar)
    if (sk == SKILL_STAFF)
        perkScale *= fx->staffDmgMult;

    float raw = base * strScale * skillScale * perkScale
              * abilityScale * damage_variance();

    // Sunak rünü: Pembe Rün basılı silah +%8 hasar
    if (armed && weapon->fx == WEAPON_FX_PINK)
        raw *= 1.08f;

    // Kritik: DEX şansı + Keskin Göz; çarpan Derin Kesik'le yükselir
    float critChance = CRIT_BASE_CHANCE
                     + (float)(p->dexterity - STAT_BASE) * CRIT_PER_DEX
                     + fx->critChanceBonus;
    if (randf() < critChance) {
        raw *= fx->critMult;
        hit.crit = true;
    }

    hit.damage = apply_defense(raw, targetDefense);
    return hit;
}

// Büyü vuruşu: silah TABANI değil büyünün kendi tabanı ölçeklenir.
// Silahın katkısı spellPower (asa +%20) + Pembe Rün (+%8) — "kılıçlı büyücü"
// rünlü kılıçla da büyüsünü güçlendirebilir (kullanıcı kararı: her şey trade-off).
HitResult combat_spell_hit(const struct Player *p, const ItemStack *weapon,
                           float baseDamage, float targetDefense,
                           const PerkEffects *fx)
{
    HitResult hit = {0};

    // INT ölçeği (taban 10 = çarpansız) — büyücünün STR'ı
    float intScale = 1.0f + (float)(p->intelligence - STAT_BASE) * STR_DAMAGE_PER_POINT;

    // Yıkım seviyesi
    float skillScale = 1.0f +
        (float)(p->skills[SKILL_DESTRUCTION].level - 1) * SKILL_DAMAGE_PER_LEVEL;

    // Silahın büyü katkısı: asa spellPower + ağaç çarpanı + Pembe Rün
    float weaponScale = 1.0f;
    if (weapon != NULL && weapon->type != NULL)
    {
        weaponScale += weapon->type->spellPower;
        if (weapon->type->weaponType == WEAPON_STAFF)
            weaponScale += fx->staffSpellBonus;   // Asa ağacı yalnız asayla işler
        if (weapon->fx == WEAPON_FX_PINK)
            weaponScale *= 1.08f;   // rün büyüye de işler
    }

    float raw = baseDamage * intScale * skillScale * weaponScale
              * fx->spellDmgMult * damage_variance();

    // Kritik: DEX + ağaç bonusları (silah vuruşuyla aynı kural)
    float critChance = CRIT_BASE_CHANCE
                     + (float)(p->dexterity - STAT_BASE) * CRIT_PER_DEX
                     + fx->critChanceBonus;
    if (randf() < critChance) {
        raw *= fx->critMult;
        hit.crit = true;
    }

    hit.damage = apply_defense(raw, targetDefense);
    return hit;
}

float equipment_total_defense(const Equipment *eq, const PerkEffects *fx)
{
    float total = 0.0f;
    for (int s = 0; s < EQUIP_SLOT_COUNT; s++) {
        const ItemStack *st = &eq->slots[s];
        float def = item_defense(st);   // boş slot 0 döner
        if (st->type != NULL)
            def *= fx->defMult[st->type->armorClass];   // sınıf perk çarpanı
        total += def;

        // Sunak stat basımı: DEF kademesi düz defans (perk çarpanı dışı)
        for (int k = 0; k < 2; k++)
            if (st->statType[k] == STAT_DEF)
                total += (float)(STAT_TIER_VALUE[STAT_DEF] * st->statTier[k]);
    }
    return total;
}

float combat_mob_damage(float mobDamage, const Equipment *eq,
                        const PerkEffects *fx, bool playerIdle,
                        float healthRatio, bool yaraliAyi, bool *outDodged)
{
    *outDodged = false;

    // Perk: Kaçınma/Refleks — hasarı tamamen savuştur
    if (fx->dodgeChance > 0.0f && randf() < fx->dodgeChance) {
        *outDodged = true;
        return 0.0f;
    }

    float raw = mobDamage * damage_variance();
    float def = equipment_total_defense(eq, fx);

    // Kalkan Duvarı: K sabiti 120 → aynı defans daha çok kısar
    float dmg = raw * fx->defenseK / (fx->defenseK + ((def > 0.0f) ? def : 0.0f));

    dmg *= fx->dmgTakenMult;                        // Koruyucu Katman/Kale/Granit/Juggernaut
    if (fx->kuleIdleDef && playerIdle)
        dmg *= 0.80f;                               // Kule
    if (yaraliAyi && healthRatio < 0.30f)
        dmg *= 0.85f;                               // Yaralı Ayı

    return dmg;
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
