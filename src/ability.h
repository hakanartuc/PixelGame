#ifndef ABILITY_H
#define ABILITY_H
#include "skill.h"

// Aktif skill'ler (hotbar'dan atılan vuruşlar) — pasif skill KATEGORİLERİNDEN
// (SkillType) ayrıdır: kategori seviye kazanır, ability o seviyeyle açılır.
// Kaynak yok, sadece cooldown (mana/stamina ileride büyüyle gelebilir).

typedef enum {
    ABILITY_POWER_STRIKE,   // güçlü vuruş: yüksek hasar, uzun bekleme
    ABILITY_QUICK_SLASH,    // hızlı kesik: düşük hasar, kısa bekleme
    ABILITY_COUNT,
} AbilityId;

typedef struct {
    const char *name;     // panel/tooltip için
    const char *abbrev;   // hotbar hücresinde gösterilen kısaltma (ikon gelene dek)
    SkillType skill;      // hangi skill kategorisine bağlı
    int unlockLevel;      // o kategori kaç seviye olunca açılır
    float damageScale;    // normal vuruş hasarının çarpanı
    float cooldown;       // saniye
} AbilityDef;

extern const AbilityDef ABILITIES[ABILITY_COUNT];

#endif
