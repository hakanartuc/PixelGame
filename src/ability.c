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
    // Büyü dalgası 2: tomarla öğrenilir (ağaç değil — kayıp bilgi dünyada).
    // Hava sinerjileri main'de: pençe ıslakta +%20, diken karda +%20 & yavaş
    // ×1.5, hortum fırtınada tick +%20 & +1 sn.
    [ABILITY_LIGHTNING_CLAW] = {
        .name = "Yıldırım Pençesi", .abbrev = "YP",
        .skill = SKILL_DESTRUCTION, .unlockLevel = 1,
        .damageScale = 35.0f,   // yakın koni (90px) — riskli mesafenin ödülü
        .cooldown = 5.0f, .mana = 18.0f,
    },
    [ABILITY_ICETACLE] = {
        .name = "Buz Dikenleri", .abbrev = "BD",
        .skill = SKILL_DESTRUCTION, .unlockLevel = 1,
        .damageScale = 26.0f,   // hedef alan (100px) + %50 yavaş 3sn
        .cooldown = 8.0f, .mana = 24.0f,
    },
    [ABILITY_TORNADO] = {
        .name = "Hortum", .abbrev = "HO",
        .skill = SKILL_DESTRUCTION, .unlockLevel = 1,
        .damageScale = 8.0f,    // TICK başına (0.5sn × 4sn ömür = ~64 toplam)
        .cooldown = 14.0f, .mana = 35.0f,
    },

    [ABILITY_SNAKEBITE] = {
        .name = "Yılan Isırığı", .abbrev = "YS",
        .skill = SKILL_DESTRUCTION, .unlockLevel = 1,
        .damageScale = 12.0f,   // vuruş küçük — asıl yük zehir DoT'unda (%60/4sn)
        .cooldown = 7.0f, .mana = 20.0f,
    },
    [ABILITY_FIRELION] = {
        .name = "Ateş Aslanı", .abbrev = "AA",
        .skill = SKILL_DESTRUCTION, .unlockLevel = 1,
        .damageScale = 30.0f,   // hat dalgası: 300px ileri, çarptığını yakar
        .cooldown = 9.0f, .mana = 28.0f,
    },

    // Two-Handed (Parça A): mızrak aktifleri (yalnız mızrak kuşanılıyken)
    [ABILITY_STAB] = {
        .name = "Saplama", .abbrev = "SP",
        .skill = SKILL_TWO_HANDED, .unlockLevel = 1,
        .damageScale = 2.2f,    // melee çarpanı (PS deseni) + geri itme ×2
        .cooldown = 8.0f, .stamina = 25.0f,
    },
    [ABILITY_SWEEP] = {
        .name = "Süpürme", .abbrev = "SÜ",
        .skill = SKILL_TWO_HANDED, .unlockLevel = 1,
        .damageScale = 1.0f,    // ön yay AoE + %30 yavaş 2sn (main işler)
        .cooldown = 12.0f, .stamina = 35.0f,
    },
    [ABILITY_GATE_STANCE] = {
        .name = "Kapı Tutucu", .abbrev = "KT",
        .skill = SKILL_TWO_HANDED, .unlockLevel = 1,
        .damageScale = 1.0f,    // duruş: menzile giren otomatik saplanır (0.8sn iç CD)
        .cooldown = 1.0f, .stamina = 0.0f,
    },

    [ABILITY_SHIELD_BASH] = {
        .name = "Kalkan Vuruşu", .abbrev = "KV",
        .skill = SKILL_BLOCK, .unlockLevel = 1,
        .damageScale = 0.6f,    // hasar aracı değil CC aracı: 2sn sersemletme
        .cooldown = 10.0f, .stamina = 20.0f,
    },

    // YANKICI (gizli sınıf): bedel MANA DEĞİL CAN — ritüelle açılır
    [ABILITY_WHISPER_BURST] = {
        .name = "Fısıltı Patlaması", .abbrev = "FP",
        .skill = SKILL_ECHO, .unlockLevel = 1,
        .damageScale = 30.0f,   // çevresel gölge hasarı (Yankı seviyesi büyütür)
        .cooldown = 8.0f, .hpFrac = 0.10f,   // bedel: max HP'nin %10'u
    },
    [ABILITY_SHADOW_FORM] = {
        .name = "Gölge Formu", .abbrev = "GF",
        .skill = SKILL_ECHO, .unlockLevel = 1,
        .damageScale = 0.0f,    // 5sn algılanmaz + hızlı; 4 HP/sn yakar
        .cooldown = 20.0f,
    },

    // KUTSAL okulu (Faz HOLY): şifa + undead avcılığı — XP şifa/kutsal hasardan
    [ABILITY_HEAL_LIGHT] = {
        .name = "Şifa Nuru", .abbrev = "ŞN",
        .skill = SKILL_LIGHT, .unlockLevel = 1,
        .damageScale = 35.0f,   // taban ŞİFA miktarı (INT + Kutsal ölçekler)
        .cooldown = 8.0f, .mana = 30.0f,
    },
    [ABILITY_HOLY_BEAM] = {
        .name = "Kutsal Hüzme", .abbrev = "KH",
        .skill = SKILL_LIGHT, .unlockLevel = 1,
        .damageScale = 26.0f,   // taban hasar — undead'e ×1.6 (main işler)
        .cooldown = 5.0f, .mana = 25.0f,
    },
    [ABILITY_HOLY_ZONE] = {
        .name = "Kutsal Alan", .abbrev = "KA",
        .skill = SKILL_LIGHT, .unlockLevel = 1,
        .damageScale = 7.0f,    // saniye başına tick hasarı (yalnız undead)
        .cooldown = 20.0f, .mana = 40.0f,
    },

    // M4 Conjuration: çağırma büyüleri (köklü cast; rezerve main'de işler)
    [ABILITY_SUMMON_SNAKE] = {
        .name = "Öz Yılanı", .abbrev = "YIL",
        .skill = SKILL_CONJURATION, .unlockLevel = 1,
        .damageScale = 0.0f, .cooldown = 10.0f, .mana = 30.0f,
    },
    [ABILITY_SUMMON_TURTLE] = {
        .name = "Öz Kabuğu", .abbrev = "KAB",
        .skill = SKILL_CONJURATION, .unlockLevel = 1,
        .damageScale = 0.0f, .cooldown = 10.0f, .mana = 30.0f,
    },
};
