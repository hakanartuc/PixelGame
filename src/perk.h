#ifndef PERK_H
#define PERK_H
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "skill.h"

// Perk sistemi: skill 25/50/75/100 eşiklerinde 3 seçenekten 1'i KALICI seçilir.
// İçerik verisi (isim/açıklama) PERK_TABLE'da; sayısal ETKİLER tek yerde,
// perk_compute_effects()'te toplanır — combat/main oradan okur, perk mantığı
// oyun koduna dağılmaz. v1 içerik: One-Handed + 3 zırh skill'i (48 perk).

#define PERK_TIERS 4          // eşik sayısı (25/50/75/100)
#define PERK_OPTIONS 3        // eşik başına seçenek
#define PERK_UNCHOSEN (-1)    // henüz seçilmedi

extern const int PERK_THRESHOLDS[PERK_TIERS];   // {25,50,75,100}

typedef struct {
    const char *name;
    const char *desc;   // panelde gösterilen kısa etki açıklaması
} PerkOption;

// Bir skill'in perk içeriği var mı? (v1: OneHanded + zırhlar)
bool perk_skill_has_content(SkillType s);

// (skill, tier, option) → tanım; içerik yoksa NULL
const PerkOption *perk_option(SkillType s, int tier, int opt);

// Oyuncunun seçimlerinden birleşik etkiler. Tüm çarpanlar nötr=1, bonuslar=0.
typedef struct {
    // One-Handed
    float oneHandDmgMult;     // Güçlü Tutuş / Kılıç Ustası
    float attackIntervalMult; // Hızlı/Usta Bilek
    float critChanceBonus;    // Keskin Göz
    float critMult;           // Derin Kesik (1.5 taban → 1.8)
    float psScaleOverride;    // Ezici Darbe (0 = yok)
    float psCdMult, qsCdMult; // Fırtına / Seri Vuruş
    float bleedFrac;          // Kanatma: vuruş hasarının bu oranı 3 sn'ye yayılır
    float bleedPower;         // Kasap: süre+hasar çarpanı
    float executeBonus;       // İnfaz: hedef HP<%20 iken ek çarpan payı

    // Zırh (savunma)
    float defMult[4];         // ArmorClass indeksli parça-defans çarpanı
    float dmgTakenMult;       // Koruyucu Katman/Kale/Granit/Juggernaut
    float defenseK;           // apply_defense sabiti (100; Kalkan Duvarı 120)
    float dodgeChance;        // Kaçınma/Refleks
    float reflectFrac;        // Karşılık
    float reflectFlat;        // Diken Zırh
    float kbTakenMult;        // Sağlam Duruş / Sarsılmaz (0 = bağışık)
    bool  kuleIdleDef;        // Kule: hareketsizken alınan hasar ×0.8

    // Genel
    float speedMult;          // Çevik/Rüzgar/Uyumlu
    float nightSpeedMult;     // Gölge Adımı
    float maxHpBonus;         // Sağlam/Dayanıklılık/Metanet...
    float potionEffMult;      // İlk Yardım
    float potionCdMult;       // Akıcı
    float detectOnPlayerMult; // Hayalet: moblar seni geç farkeder
    float kbDealtMult;        // Ezici Ağırlık
    float dmgWhenHealthy;     // Zinde: HP>%50 iken ek hasar payı
    float gaziDmgBonus;       // Gazi: hasar yiyince 5 sn bu kadar ek hasar
    float deathSaveCooldown;  // Yıkılmaz 60 / Ölümsüz 90 (0 = yok)
} PerkEffects;

// choice[skill][tier] → birleşik etkiler (her seçim değişiminde çağır)
void perk_compute_effects(const Sint8 choice[SKILL_COUNT][PERK_TIERS],
                          PerkEffects *out);

// Skill'in seçilebilir (ulaşılmış ama seçilmemiş) en düşük tier'ı (-1 = yok)
int perk_pending_tier(SkillType s, int level, const Sint8 choice[SKILL_COUNT][PERK_TIERS]);

// Skill paneli satırlarına perk pip'leri: seçilmiş=dolu, SEÇİLEBİLİR=sarı "!",
// ulaşılmamış=soluk. skill_panel_render'dan SONRA, aynı panelX/panelY ile çağır.
void perk_pips_render(const Skill *skills,
                      const Sint8 choice[SKILL_COUNT][PERK_TIERS],
                      SDL_Renderer *renderer, TTF_Font *font,
                      float panelX, float panelY);

// Seçim overlay'i: 3 seçenek kartı (K panelinin yanında)
void perk_choice_render(SkillType s, int tier, SDL_Renderer *renderer,
                        TTF_Font *font, float x, float y);

// Overlay'de tıklanan seçenek (0..2, -1 = dışarı). Render ile aynı yerleşim.
int perk_choice_option_at(float mx, float my, float x, float y);

#endif
