#include "ability.h"

// Denge notu: Quick Slash DPS'i Power Strike'tan hafif yüksek ama tuş baskısı
// ister; Power Strike tek tuşla patlama hasarı. İkisi de One-Handed'a bağlı.
const AbilityDef ABILITIES[ABILITY_COUNT] = {
    [ABILITY_POWER_STRIKE] = {
        .name = "Power Strike", .abbrev = "PS",
        .skill = SKILL_ONE_HANDED, .unlockLevel = 1,
        .damageScale = 2.0f, .cooldown = 6.0f,
    },
    [ABILITY_QUICK_SLASH] = {
        .name = "Quick Slash", .abbrev = "QS",
        .skill = SKILL_ONE_HANDED, .unlockLevel = 5,
        .damageScale = 0.7f, .cooldown = 2.0f,
    },

    // Yay merdiveni (Faz 5): Archery kastıkça açılır; hepsi stamina tüketir.
    // Denge: köklü atışın bedeli beceri çeşitliliğiyle dönüyor — GS kaçış,
    // BO kontrol, YA/AO patlama; stamina (100, +15/sn) arka arkaya 2-3 beceriye izin verir.
    [ABILITY_POWER_SHOT] = {
        .name = "Güçlü Ok", .abbrev = "GO",
        .skill = SKILL_ARCHERY, .unlockLevel = 1,
        .damageScale = 2.2f, .cooldown = 6.0f, .stamina = 20.0f,
    },
    [ABILITY_ICE_ARROW] = {
        .name = "Buz Oku", .abbrev = "BO",
        .skill = SKILL_ARCHERY, .unlockLevel = 10,
        .damageScale = 1.0f, .cooldown = 10.0f, .stamina = 15.0f,
    },
    [ABILITY_DISENGAGE] = {
        .name = "Geri Sıçrama", .abbrev = "GS",
        .skill = SKILL_ARCHERY, .unlockLevel = 20,
        .damageScale = 0.0f, .cooldown = 8.0f, .stamina = 25.0f,
    },
    [ABILITY_VOLLEY] = {
        .name = "Yaylım Ateşi", .abbrev = "YA",
        .skill = SKILL_ARCHERY, .unlockLevel = 30,
        .damageScale = 0.6f, .cooldown = 12.0f, .stamina = 30.0f,
    },
    [ABILITY_AOE_ARROW] = {
        .name = "Alan Oku", .abbrev = "AO",
        .skill = SKILL_ARCHERY, .unlockLevel = 45,
        .damageScale = 1.0f, .cooldown = 15.0f, .stamina = 35.0f,
    },

    // Büyüler (Faz 22): silahtan bağımsız — MANA tüketir; damageScale = büyü
    // taban hasarının çarpanı değil, TABAN hasar combat_spell_hit'e ayrıca gider.
    [ABILITY_FIREBOLT] = {
        .name = "Ateş Topu", .abbrev = "AT",
        .skill = SKILL_DESTRUCTION, .unlockLevel = 1,
        .damageScale = 25.0f,   // büyü TABAN hasarı (spell_hit ölçekler)
        .cooldown = 2.0f, .mana = 15.0f,
    },
    [ABILITY_ICE_NOVA] = {
        .name = "Buz Novası", .abbrev = "BN",
        .skill = SKILL_DESTRUCTION, .unlockLevel = 1,
        .damageScale = 18.0f,
        .cooldown = 6.0f, .mana = 20.0f,
    },
    [ABILITY_LIGHTNING] = {
        .name = "Yıldırım", .abbrev = "YI",
        .skill = SKILL_DESTRUCTION, .unlockLevel = 1,
        .damageScale = 45.0f,
        .cooldown = 8.0f, .mana = 30.0f,
    },
    [ABILITY_FLAME_ARMOR] = {
        .name = "Alev Zırhı", .abbrev = "AZ",
        .skill = SKILL_DESTRUCTION, .unlockLevel = 1,
        .damageScale = 0.0f,
        .cooldown = 20.0f, .mana = 25.0f,
    },
    [ABILITY_MAGIC_SHIELD] = {
        .name = "Büyü Kalkanı", .abbrev = "BK",
        .skill = SKILL_DESTRUCTION, .unlockLevel = 1,
        .damageScale = 0.0f,
        .cooldown = 25.0f, .mana = 25.0f,
    },
    [ABILITY_SPIKES] = {
        .name = "Toprak Dikenleri", .abbrev = "TD",
        .skill = SKILL_DESTRUCTION, .unlockLevel = 1,
        .damageScale = 20.0f,   // büyü taban hasarı
        .cooldown = 10.0f, .mana = 22.0f,
    },
};
