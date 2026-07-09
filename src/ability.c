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
};
