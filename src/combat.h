#ifndef COMBAT_H
#define COMBAT_H
#include <stdbool.h>
#include "item.h"
#include "skill.h"
#include "perk.h"

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

// Oyuncunun bir vuruşta vereceği hasar (Combat A formülü + perk etkileri).
// weapon instance olarak gelir (plus ölçeği dahil); type NULL ise silahsız.
// abilityScale: aktif skill çarpanı (normal vuruş = 1.0).
// targetHpRatio: hedefin can oranı (İnfaz perki için).
HitResult combat_player_hit(const Player *p, const ItemStack *weapon,
                            float targetDefense, float abilityScale,
                            float targetHpRatio, const PerkEffects *fx);

// Silahın otomatik vuruş aralığı (perk çarpanı dahil).
float combat_attack_interval(const ItemStack *weapon, const PerkEffects *fx);

// Mob'un oyuncuya vereceği hasar: mob hasarı × varyans → oyuncu defansı
// (perk çarpanları dahil). outDodged: Kaçınma perki tetiklendiyse true (hasar 0).
// playerIdle: Kule perki (hareketsizken ek koruma). healthRatio: Yaralı Ayı.
// outBlock: 0 yok / 1 pasif blok / 2 duruş / 3 PERFECT (hasar 0 + sersemletme)
// blockLevel: Block skill seviyesi; blocking/blockHold: SHIFT duruşu durumu
float combat_mob_damage(float mobDamage, const Equipment *eq,
                        const PerkEffects *fx, bool playerIdle,
                        float healthRatio, bool yaraliAyi,
                        int blockLevel, bool blocking, float blockHold,
                        bool *outDodged, int *outBlock);

// Kuşanılı ekipmanın toplam defansı (sınıf perk çarpanlarıyla).
float equipment_total_defense(const Equipment *eq, const PerkEffects *fx);

// Vuruş XP'si: verilen hasarla orantılı; oyuncunun skill seviyesi mob
// seviyesinin çok üstündeyse azalan getiri (düşük mob'da grind engeli).
float combat_xp_for_hit(float damage, int mobLevel, int skillLevel);

// Büyü vuruşu (Faz 22): INT × Destruction seviyesi × silah büyü gücü (asa/rün)
// × ağaç çarpanı; DEX kritiği uygulanır, hedef defansı kısar.
HitResult combat_spell_hit(const struct Player *p, const ItemStack *weapon,
                           float baseDamage, float targetDefense,
                           const PerkEffects *fx);

#endif
