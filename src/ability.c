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
};
