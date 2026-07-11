#ifndef PERK_H
#define PERK_H
#include "r2d.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "skill.h"
#include "ability.h"   // ABILITY_COUNT (aktif düğümlerin açılış tablosu)

// PerkEffects: oyuncunun ağaç seçimlerinden BİRLEŞİK etkiler — combat/main
// buradan okur, düğüm mantığı oyun koduna dağılmaz.
// Eski eşik-anket sistemi Faz 21'de skilltree.c'deki puanlı düğüm ağacına
// dönüştü; bu dosyada yalnız etki yapısı kaldı (tree_compute_effects doldurur).

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
    bool  yaraliAyi;          // can %30 altında alınan hasar -%15 (Orta ağaç düğümü)

    // Yay (Archery ağacı — Faz 21)
    float bowDmgMult;         // yay hasar çarpanı
    float drawSpeedMult;      // çekme süresi çarpanı (<1 hızlı, keystone bedeli >1)
    float bowRangeBonus;      // menzile eklenen px
    int   volleyExtra;        // Yaylım'a ek ok
    float ammoSaveChance;     // atışta ok tükenmeme şansı (0..1)
    float bowStaminaMult;     // yay becerisi stamina maliyeti çarpanı
    float disengageBonus;     // Geri Sıçrama mesafesine eklenen px
    float aoeRadiusBonus;     // Alan Oku yarıçapına eklenen px

    // Gizlilik (Sneak ağacı)
    float sneakDetectMult;    // eğilme algısına ek çarpan (<1 daha görünmez)
    float sneakSpeedRelief;   // eğilme hız cezasını geri kazanma payı (0..1)
    float backstabMult;       // sinsi vuruş çarpanı (taban 2.0 — düğümler EKLER)
    float nightDetectMult;    // gece algı çarpanı (Hortlak: gece avcısı)

    // Konuşma (Speech ağacı)
    float shopBuyMult;        // alış fiyat çarpanı (<1 ucuz)
    float shopSellMult;       // satış fiyat çarpanı (>1 kârlı)
    float speechDiceBonus;    // zar başarı şansına eklenen yüzde puanı
    float dicePenaltyMult;    // başarısız zar itibar cezası çarpanı (Gümüş Dil bedeli)
    float contractGoldMult;   // pano kontrat altını çarpanı
    float contractRepMult;    // pano kontrat itibarı çarpanı (Altın Ağız bedeli)
    float trainCostMult;      // öğretmen ders ücreti çarpanı
    float bribeCostMult;      // muhafız rüşveti çarpanı

    // Aktif beceriler (AKTİF düğümler açar — hotbar bunlara göre dolar)
    bool abilityOpen[ABILITY_COUNT];

    // Meslek mini ağaçları (Faz 21b)
    float mineSpeedMult, herbSpeedMult, woodSpeedMult;   // toplama iş hızı
    float mineDouble, herbDouble, woodDouble;            // çift ürün şansı (0..1)
    float smithSpeedMult, alchSpeedMult, enchSpeedMult;  // istasyon iş/tören hızı
    float plusChanceBonus;    // '+basma' şansına eklenen yüzde puanı
    float runeChanceBonus;    // rün üretim şansına eklenen yüzde puanı

    // Büyü (Yıkım ağacı — Faz 22)
    float spellDmgMult;       // büyü hasar çarpanı
    float manaCostMult;       // büyü mana maliyeti çarpanı (<1 verimli)
    float castSpeedMult;      // cast süresi çarpanı (<1 hızlı)
    float burnBonus;          // Ateş Topu yanma DoT çarpanı payı (0 = taban)
    float novaRadiusBonus;    // Buz Novası yarıçapına eklenen px
    float meleeManaRegen;     // SAVAŞ RAHİBİ: melee vuruş başına mana

    // Asa (Staff ağacı)
    float staffDmgMult;       // asa melee çarpanı
    float staffSpellBonus;    // asa KUŞANILIYKEN ek büyü çarpanı payı

    // Kader Ağacı (karakter-geneli)
    float maxStaminaBonus, maxManaBonus;
    float stamRegenMult, manaRegenMult;
    float xpGainMult;         // TÜM skill XP çarpanı
    float hpRegenPerSec;      // pasif can yenilenmesi
    float goldFindMult;       // kasa altını çarpanı
    // Lockpicking (kilit minigame'i — lockpick.c okur)
    float lockZoneMult;      // yeşil bölge genişliği çarpanı
    float lockCursorMult;    // imleç hızı çarpanı (küçük = yavaş = kolay)
    float pickSaveChance;    // kırılınca maymuncuğu kurtarma şansı (0..1)
    float lockNoiseMult;     // başarısızlık gürültü yarıçapı çarpanı
    bool  lockMasterEase;    // KİLİT FISILDAYAN: usta kilit bir kademe kolay
    bool  lockAlwaysConsume; // bedeli: başarıda da 1 maymuncuk gider
} PerkEffects;

#endif
