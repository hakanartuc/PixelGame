#ifndef COMBAT_H
#define COMBAT_H
#include <stdbool.h>
#include "item.h"
#include "skill.h"

// Combat A modeli: ıska yok, her vuruş isabet eder — derinlik hasar hesabında.
// raw = silahHasarı × STR ölçeği × skill bonusu × varyans(0.9-1.1) [× crit]
// final = raw × 100 / (100 + hedefDefansı)  → defans azalan getirili, asla %100 olmaz.

typedef struct Player Player;         // forward decl (player.h ↔ combat.h döngüsü olmasın)
typedef struct Equipment Equipment;

#define UNARMED_DAMAGE 10.0f   // silahsız taban hasar (skill bonusu almaz)

// Bir vuruşun sonucu — hasar + UI'ın ihtiyaç duyduğu bilgiler
typedef struct {
    float damage;   // uygulanacak nihai hasar
    bool  crit;     // kritik vuruş muydu (UI: farklı gösterim)
} HitResult;

// Silah → geliştirdiği skill. NULL (silahsız) veya eşleşme yoksa SKILL_NONE.
SkillType combat_weapon_skill(const ItemType *weapon);

// Oyuncunun bir vuruşta vereceği hasar (Combat A formülü).
// weapon instance olarak gelir (plus ölçeği dahil); type NULL ise silahsız.
HitResult combat_player_hit(const Player *p, const ItemStack *weapon,
                            float targetDefense);

// Mob'un oyuncuya vereceği hasar: mob hasarı × varyans, oyuncu zırh
// defansından (kuşanılı item'ların defense toplamı) geçirilir.
float combat_mob_damage(float mobDamage, const Equipment *eq);

// Kuşanılı ekipmanın toplam defansı (zırh + kalkan).
float equipment_total_defense(const Equipment *eq);

// Vuruş XP'si: verilen hasarla orantılı; oyuncunun skill seviyesi mob
// seviyesinin çok üstündeyse azalan getiri (düşük mob'da grind engeli).
float combat_xp_for_hit(float damage, int mobLevel, int skillLevel);

#endif
