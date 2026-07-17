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
    // Büyüler (Faz 22) — silahtan bağımsız, MANA tüketir, köklü cast (~0.45sn)
    ABILITY_FIREBOLT,       // Ateş Topu: güdümlü mermi + 3sn yanma DoT'u
    ABILITY_ICE_NOVA,       // Buz Novası: ÇEVRENE patlama + alan yavaşlatma
    ABILITY_LIGHTNING,      // Yıldırım: hedefe anında sert vuruş (pahalı, uzun CD)
    ABILITY_FLAME_ARMOR,    // Alev Zırhı: 15sn yakın saldırgana hasar yansıtma
    ABILITY_MAGIC_SHIELD,   // Büyü Kalkanı: 12sn hasar HP yerine MANADAN emilir (×1.2)
    ABILITY_SPIKES,         // Toprak Dikenleri: çevrende diken patlaması + yavaşlatma
    ABILITY_SUMMON_SNAKE,   // Öz Yılanı çağır (M4 — bootstrap: herkese açık)
    ABILITY_SUMMON_TURTLE,  // Öz Kabuğu çağır (M4 — ağaçtan)
    // Büyü dalgası 2 (tomar ekonomisi — dünyada bulunan kayıp büyüler)
    ABILITY_LIGHTNING_CLAW, // Yıldırım Pençesi: yakın koni — büyücünün panik butonu
    ABILITY_ICETACLE,       // Buz Dikenleri: hedef alanda yerden buz + güçlü yavaş
    ABILITY_TORNADO,        // Hortum: ileri süzülen alan — tick hasarı + yavaşlatma
    ABILITY_SNAKEBITE,      // Yılan Isırığı: hedefe zehir DoT — Zehir Patlaması hattı
    ABILITY_FIRELION,       // Ateş Aslanı: hat dalgası — çarptığını yakar
    ABILITY_COUNT,
} AbilityId;

typedef struct {
    const char *name;     // panel/tooltip için
    const char *abbrev;   // hotbar hücresinde gösterilen kısaltma (ikon gelene dek)
    SkillType skill;      // hangi skill kategorisine bağlı
    int unlockLevel;      // (Faz 21: KULLANILMIYOR — aktifler ağaçtaki AKTİF
                          //  düğümlerle açılır; alan tarihsel referans için durur)
    float damageScale;    // normal vuruş hasarının çarpanı
    float cooldown;       // saniye
    float stamina;        // atım maliyeti (0 = kaynaksız); yetmezse atılmaz, CD yanmaz
    float mana;           // büyü maliyeti (Destruction becerileri; 0 = manasız)
} AbilityDef;

extern const AbilityDef ABILITIES[ABILITY_COUNT];

#endif
