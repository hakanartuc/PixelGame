#ifndef ABILITY_H
#define ABILITY_H
#include "skill.h"

// Aktif skill'ler (hotbar'dan atılan vuruşlar) — pasif skill KATEGORİLERİNDEN
// (SkillType) ayrıdır: kategori seviye kazanır, ability o seviyeyle açılır.
// Yay becerileri stamina tüketir (havuzun ilk gerçek tüketicisi);
// PS/QS şimdilik yalnız cooldown (melee kaynağı ileride).

typedef enum {
    ABILITY_POWER_STRIKE,   // güçlü vuruş: yüksek hasar, uzun bekleme
    ABILITY_QUICK_SLASH,    // hızlı kesik: düşük hasar, kısa bekleme
    // Yay becerileri (Faz 5) — yalnız yay kuşanılıyken; skill = Archery
    ABILITY_POWER_SHOT,     // Güçlü Ok: ×2.2 tek ok
    ABILITY_ICE_ARROW,      // Buz Oku: ×1.0 + hedef %40 yavaş 3sn
    ABILITY_DISENGAGE,      // Geri Sıçrama: hedeften ters yöne 140px kaçış (ok atmaz)
    ABILITY_VOLLEY,         // Yaylım: tek hedefe 0.15sn arayla 3 ok (×0.6/ok, 3 ok tüketir)
    ABILITY_AOE_ARROW,      // Alan Oku: varışta patlar — hedef ×1.0 + 90px çevre ×0.6
    ABILITY_COUNT,
} AbilityId;

typedef struct {
    const char *name;     // panel/tooltip için
    const char *abbrev;   // hotbar hücresinde gösterilen kısaltma (ikon gelene dek)
    SkillType skill;      // hangi skill kategorisine bağlı
    int unlockLevel;      // o kategori kaç seviye olunca açılır
    float damageScale;    // normal vuruş hasarının çarpanı
    float cooldown;       // saniye
    float stamina;        // atım maliyeti (0 = kaynaksız); yetmezse atılmaz, CD yanmaz
} AbilityDef;

extern const AbilityDef ABILITIES[ABILITY_COUNT];

#endif
