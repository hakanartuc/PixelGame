#include <stdio.h>
#include "skilltree.h"
#include "item.h"     // ArmorClass indeksleri (defMult)
#include "font.h"
#include "ui_theme.h"

// ---------------------------------------------------------------------------
// SAVAŞ AĞACI TOPOLOJİSİ (ortak iskelet):
//   0=kök · 1-5 SOL dal · 6-10 ORTA dal · 11-15 SAĞ dal (b0→b1→B2*→b3→B4*)
//   16/17 = KEYSTONE (dal uçlarından) · 18+ = AKTİF düğümler (beceri açar)
// Maliyet: savaş ağacı ~28-38p, kazanılabilir 25p → tamamı ASLA alınamaz.
// MESLEK MİNİ AĞACI: 0=kök · 1-3 A dalı · 4-6 B dalı · 7=keystone (12p — dolabilir)
// KADER: standart iskelet, puan TOPLAM seviyeden (10'da 1) — savaşmayan da kazanır.
// ---------------------------------------------------------------------------

#define N_ROOT(nm, ds)            { nm, ds, NODE_MINOR,    -1, -1, 3, 0, -1 }
#define N_MIN(nm, ds, pre, x, y)  { nm, ds, NODE_MINOR,   pre, -1, x, y, -1 }
#define N_NOT(nm, ds, pre, x, y)  { nm, ds, NODE_NOTABLE, pre, -1, x, y, -1 }
#define N_NOT2(nm, ds, pa, pb, x, y) { nm, ds, NODE_NOTABLE, pa, pb, x, y, -1 }
#define N_KEY(nm, ds, pa, pb, x)  { nm, ds, NODE_KEYSTONE, pa, pb, x, 6, -1 }
#define N_ACT(nm, ds, pre, x, y, ab) { nm, ds, NODE_ACTIVE, pre, -1, x, y, ab }

// --- ONE-HANDED: "Kılıç Yolu" — Kan / Tempo / Kritik + PS/QS aktifleri ---
static const TreeNode T_ONEHAND[] = {
    N_ROOT("Silah Eğitimi", "Tek el hasarı +%3"),
    N_MIN("Keskin Kenar",  "Tek el hasarı +%3",                0, 1, 1),
    N_MIN("Tetikte",       "Kritik şansı +%2",                 1, 1, 2),
    N_NOT("Kanatma",       "Vuruşlar 3 sn'de hasarın %30'u kadar kanatır", 2, 1, 3),
    N_MIN("Derin Yara",    "Kanama gücü +%25",                 3, 1, 4),
    N_NOT("Kasap",         "Kanama süresi ve hasarı 2x",       4, 1, 5),

    N_MIN("Hafif Bilek",   "Saldırı aralığı -%4",              0, 3, 1),
    N_MIN("Akış",          "Saldırı aralığı -%4",              6, 3, 2),
    N_NOT("Hızlı Bilek",   "Saldırı aralığı -%10 daha",        7, 3, 3),
    N_MIN("Soluk",         "Quick Slash bekleme -%15",         8, 3, 4),
    N_NOT("Fırtına",       "Power Strike bekleme 6 -> 3 sn",   9, 3, 5),

    N_MIN("Nişan",         "Kritik şansı +%2",                 0, 5, 1),
    N_MIN("Zayıf Nokta",   "Kritik şansı +%3",                11, 5, 2),
    N_NOT("Keskin Göz",    "Kritik şansı +%8",                12, 5, 3),
    N_MIN("Acımasız",      "Tek el hasarı +%4",               13, 5, 4),
    N_NOT("Derin Kesik",   "Kritik çarpanı 1.5 -> 1.8",       14, 5, 5),

    N_KEY("BERSERKER",     "Tek el hasarı +%25 — ALINAN hasar +%20",      5, 10, 2),
    N_KEY("İNFAZCI",       "Canı %20 altındaki hedefe +%60 — max HP -10", 10, 15, 4),

    // Aktifler: satın alınınca hotbar'a düşer
    N_ACT("Power Strike",  "AKTİF: x2.0 hasarlı vuruş (6 sn bekleme)",    0, 2, 1, ABILITY_POWER_STRIKE),
    N_ACT("Quick Slash",   "AKTİF: x0.7 hızlı kesik (2 sn bekleme)",      7, 2, 3, ABILITY_QUICK_SLASH),
};

// --- ARCHERY: "Yay Yolu" — Nişancı / Tempo / Beceri + 5 yay aktifi ---
static const TreeNode T_ARCHERY[] = {
    N_ROOT("Okçu Eğitimi", "Yay hasarı +%3"),
    N_MIN("Sabit El",      "Yay hasarı +%3",                   0, 1, 1),
    N_MIN("Uzun Nefes",    "Yay menzili +20",                  1, 1, 2),
    N_NOT("Şahin Gözü",    "Menzil +40, kritik +%3",           2, 1, 3),
    N_MIN("Ölümcül Uç",    "Kritik şansı +%3",                 3, 1, 4),
    N_NOT("Tam İsabet",    "Yay hasarı +%12",                  4, 1, 5),

    N_MIN("Çevik Parmak",  "Yay çekme -%5 hızlı",              0, 3, 1),
    N_MIN("Ritim",         "Yay çekme -%5 hızlı",              6, 3, 2),
    N_NOT("Usta Çekiş",    "Yay çekme -%15 hızlı",             7, 3, 3),
    N_MIN("Soğukkanlı",    "Yay becerileri -%15 stamina",      8, 3, 4),
    N_NOT("Cephaneci",     "%20 şansla ok tükenmez",           9, 3, 5),

    N_MIN("Hazırlık",      "Yay becerileri -%10 stamina",      0, 5, 1),
    N_MIN("Esnek Kiriş",   "Yay hasarı +%3",                  11, 5, 2),
    N_NOT2("Çifte Yaylım", "Yaylım Ateşi +1 ok",              12, 21, 5, 3),
    N_MIN("İtiş",          "Geri Sıçrama +40px",              13, 5, 4),
    N_NOT("Parçalayan",    "Alan Oku yarıçapı +30px",         14, 5, 5),

    N_KEY("KESKİN NİŞANCI","Yay hasarı +%30 — çekme +%20 yavaş",           5, 10, 2),
    N_KEY("AKINCI",        "Çekme -%25 hızlı — yay hasarı -%10",          10, 15, 4),

    // Aktifler (5 yay becerisi ağaçtan açılır)
    N_ACT("Güçlü Ok",      "AKTİF: x2.2 hasarlı ok (6sn, 20 stamina)",     0, 2, 1, ABILITY_POWER_SHOT),
    N_ACT("Buz Oku",       "AKTİF: %40 yavaşlatan ok (10sn, 15 stamina)",  1, 0, 2, ABILITY_ICE_ARROW),
    N_ACT("Geri Sıçrama",  "AKTİF: 140px anında kaçış (8sn, 25 stamina)", 11, 6, 2, ABILITY_DISENGAGE),
    N_ACT("Yaylım Ateşi",  "AKTİF: tek hedefe 3 ok x0.6 (12sn, 30 st.)",   7, 2, 3, ABILITY_VOLLEY),
    N_ACT("Alan Oku",      "AKTİF: varışta 90px patlama (15sn, 35 st.)",  12, 6, 4, ABILITY_AOE_ARROW),
};

// --- LIGHT ARMOR: "Rüzgar Yolu" ---
static const TreeNode T_LIGHT[] = {
    N_ROOT("Hafif Adım",   "+%2 hareket hızı"),
    N_MIN("Esnek",         "Hafif zırh defansı +%10",          0, 1, 1),
    N_MIN("Yumuşak İniş",  "+5 max HP",                        1, 1, 2),
    N_NOT("İkinci Deri",   "Hafif zırh defansı +%20",          2, 1, 3),
    N_MIN("Dinç",          "+10 max HP",                       3, 1, 4),
    N_NOT("Zinde",         "Can %50 üstündeyken +%10 hasar",   4, 1, 5),

    N_MIN("Hafif Yük",     "+%2 hız",                          0, 3, 1),
    N_MIN("Salınım",       "+%2 hız",                          6, 3, 2),
    N_NOT("Rüzgar",        "+%5 hız",                          7, 3, 3),
    N_MIN("Karanlığa Alışkın", "Gece +%3 hız",                 8, 3, 4),
    N_NOT("Gölge Adımı",   "Gece +%5 hız daha",                9, 3, 5),

    N_MIN("Sezgi",         "%2 şansla hasarı savuştur",        0, 5, 1),
    N_MIN("Yılan Beli",    "%2 kaçınma daha",                 11, 5, 2),
    N_NOT("Kaçınma",       "%5 kaçınma daha",                 12, 5, 3),
    N_MIN("Toparlanma",    "İksir beklemesi -%15",            13, 5, 4),
    N_NOT("Refleks",       "%4 kaçınma daha",                 14, 5, 5),

    N_KEY("HAYALET",       "Moblar seni %25 geç fark eder, itilmezsin — max HP -15", 5, 10, 2),
    N_KEY("RÜZGARIN OĞLU", "+%10 hız — alınan hasar +%10",    10, 15, 4),
};

// --- MEDIUM ARMOR: "Denge Yolu" ---
static const TreeNode T_MEDIUM[] = {
    N_ROOT("Uyum",         "+5 max HP"),
    N_MIN("Dengeli",       "Orta zırh defansı +%7",            0, 1, 1),
    N_MIN("Sağlam",        "+10 max HP",                       1, 1, 2),
    N_NOT("Zırh Ustası",   "Orta zırh defansı +%25",           2, 1, 3),
    N_MIN("Metanet",       "+10 max HP",                       3, 1, 4),
    N_NOT("Yaralı Ayı",    "Can %30 altında alınan hasar -%15", 4, 1, 5),

    N_MIN("Koruma",        "Alınan hasar -%2",                 0, 3, 1),
    N_MIN("Sabır",         "+5 max HP",                        6, 3, 2),
    N_NOT("Koruyucu Katman", "Alınan hasar -%5",               7, 3, 3),
    N_MIN("Pekişme",       "Alınan hasar -%3",                 8, 3, 4),
    N_NOT("Kale",          "Alınan hasar -%10",                9, 3, 5),

    N_MIN("Sargı Bilgisi", "İksirler +%10 etkili",             0, 5, 1),
    N_MIN("Şifacı Eli",    "İksirler +%15 etkili",            11, 5, 2),
    N_NOT("İlk Yardım",    "İksirler +%25 etkili",            12, 5, 3),
    N_MIN("Öfke",          "Gazi etkisi +%5",                 13, 5, 4),
    N_NOT("Gazi",          "Hasar yiyince 5 sn +%10 hasar",   14, 5, 5),

    N_KEY("YAŞAYAN KALE",  "Alınan hasar -%15 — hız -%8",      5, 10, 2),
    N_KEY("FEDAİ",         "Vurana %20 yansır + ölümcül darbe 1 HP bırakır (60sn) — iksirler -%20 etkili", 10, 15, 4),
};

// --- HEAVY ARMOR: "Demir Yolu" ---
static const TreeNode T_HEAVY[] = {
    N_ROOT("Ağır Eğitim",  "Ağır zırh defansı +%5"),
    N_MIN("Demir Ten",     "Ağır zırh defansı +%10",           0, 1, 1),
    N_MIN("Yük Alışkanlığı", "+10 max HP",                     1, 1, 2),
    N_NOT("Ağır Ustalık",  "Ağır zırh defansı +%25",           2, 1, 3),
    N_MIN("Kaya",          "+10 max HP",                       3, 1, 4),
    N_NOT("Kalkan Duvarı", "Tüm defans daha verimli (K: 100->120)", 4, 1, 5),

    N_MIN("Duruş",         "İtilme -%25",                      0, 3, 1),
    N_MIN("Kök Salma",     "İtilme -%25 daha",                 6, 3, 2),
    N_NOT("Sağlam Duruş",  "İtilme yemezsin",                  7, 3, 3),
    N_MIN("Katman",        "Alınan hasar -%3",                 8, 3, 4),
    N_NOT("Granit",        "Alınan hasar -%8",                 9, 3, 5),

    N_MIN("Karşı Ağırlık", "Verdiğin itme +%5",                0, 5, 1),
    N_MIN("Omuz Darbesi",  "Verdiğin itme +%5",               11, 5, 2),
    N_NOT("Ezici Ağırlık", "İtme +%10, tek el hasarı +%5",    12, 5, 3),
    N_MIN("Diken",         "Yakın saldırgana 2 hasar yansır", 13, 5, 4),
    N_NOT("Diken Zırh",    "Yakın saldırgana 4 hasar yansır", 14, 5, 5),

    N_KEY("TAŞ BEDEN",     "Alınan hasar -%25 — hız -%10",     5, 10, 2),
    N_KEY("ÖLÜMSÜZ",       "Ölümcül darbe 1 HP bırakır (90sn), alınan -%5 — hız -%5", 10, 15, 4),
};

// --- SNEAK: "Gölge Yolu" ---
static const TreeNode T_SNEAK[] = {
    N_ROOT("Sessiz Adım",  "Eğilme hız cezası %10 azalır"),
    N_MIN("Alçak Duruş",   "Eğilirken algı -%5",               0, 1, 1),
    N_MIN("Nefes Kontrolü","Eğilirken algı -%5",               1, 1, 2),
    N_NOT("Sis",           "Eğilirken algı -%12",              2, 1, 3),
    N_MIN("Örtü Alışkanlığı", "Eğilirken algı -%6",            3, 1, 4),
    N_NOT("Kayıp Silüet",  "Eğilirken algı -%15",              4, 1, 5),

    N_MIN("Çevik Eğilme",  "Eğilme cezası %10 azalır",         0, 3, 1),
    N_MIN("Akıcı Gölge",   "Eğilme cezası %10 azalır",         6, 3, 2),
    N_NOT("Gölge Yürüyüşü","Eğilme cezası %25 azalır",         7, 3, 3),
    N_MIN("Hazır Bacak",   "+%2 hız (her zaman)",              8, 3, 4),
    N_NOT("Panter",        "Eğilme hız cezası tamamen kalkar", 9, 3, 5),

    N_MIN("Hedef Gözleme", "Sinsi vuruş +%15",                 0, 5, 1),
    N_MIN("Soğuk Kan",     "Sinsi vuruş +%15",                11, 5, 2),
    N_NOT("Suikastçı",     "Sinsi vuruş +%50",                12, 5, 3),
    N_MIN("Kirli İş",      "Sinsi vuruş +%20",                13, 5, 4),
    N_NOT("Boğaz Kesen",   "Sinsi vuruş +%50 daha",           14, 5, 5),

    N_KEY("GÖLGE",         "Sinsi vuruş +%100, algı -%10 — max HP -15",  5, 10, 2),
    N_KEY("HORTLAK",       "Gece algı -%30 — gündüz algı +%10",         10, 15, 4),
};

// --- SPEECH: "Dil Yolu" ---
static const TreeNode T_SPEECH[] = {
    N_ROOT("Tatlı Dil",    "Speech zarı +%3"),
    N_MIN("Kıyaslama",     "Alış fiyatları -%3",               0, 1, 1),
    N_MIN("Israr",         "Alış fiyatları -%4",               1, 1, 2),
    N_NOT("Pazarlıkçı",    "Alış fiyatları -%8",               2, 1, 3),
    N_MIN("Sunum",         "Satış fiyatları +%5",              3, 1, 4),
    N_NOT("Tüccar Damarı", "Satış fiyatları +%12",             4, 1, 5),

    N_MIN("Göz Teması",    "Speech zarı +%4",                  0, 3, 1),
    N_MIN("Beden Dili",    "Speech zarı +%5",                  6, 3, 2),
    N_NOT("İkna Ustası",   "Speech zarı +%10",                 7, 3, 3),
    N_MIN("Sabırlı Öğrenci", "Ders ücretleri -%15",            8, 3, 4),
    N_NOT("Hatip",         "Speech zarı +%12",                 9, 3, 5),

    N_MIN("Nezaket",       "Kontrat altını +%5",               0, 5, 1),
    N_MIN("Güvenilir Yüz", "Kontrat altını +%5",              11, 5, 2),
    N_NOT("Sözünün Eri",   "Kontrat altını +%15",             12, 5, 3),
    N_MIN("Bahşiş Kültürü","Rüşvet -%15",                     13, 5, 4),
    N_NOT("Arabulucu",     "Rüşvet yarı fiyat",               14, 5, 5),

    N_KEY("GÜMÜŞ DİL",     "Speech zarı +%20 — başarısız zar itibar cezası 2x", 5, 10, 2),
    N_KEY("ALTIN AĞIZ",    "Alış -%12, satış +%15 — kontrat itibarı yarıya",   10, 15, 4),
};

// --- DESTRUCTION: "Yıkım Yolu" — Ateş / Verim / Kontrol + 4 büyü aktifi.
// Büyüler SİLAHTAN BAĞIMSIZ (mana tüketir) — kılıçlı/yaylı/zırhlı büyücü serbest.
static const TreeNode T_DESTRUCTION[] = {
    N_ROOT("İlk Kıvılcım",  "Büyü hasarı +%3"),
    N_MIN("Kor",            "Büyü hasarı +%3",                  0, 1, 1),
    N_MIN("Közleme",        "Yanma DoT'u +%25",                 1, 1, 2),
    N_NOT("Yangın",         "Yanma DoT'u +%50 daha",            2, 1, 3),
    N_MIN("Isı",            "Büyü hasarı +%4",                  3, 1, 4),
    N_NOT("Cehennem",       "Büyü hasarı +%12",                 4, 1, 5),

    N_MIN("Tutumlu Kanal",  "Büyü mana maliyeti -%5",           0, 3, 1),
    N_MIN("Akış",           "Büyü mana maliyeti -%5",           6, 3, 2),
    N_NOT("Verimli Büyü",   "Büyü mana maliyeti -%15",          7, 3, 3),
    N_MIN("Hızlı Söz",      "Cast süresi -%10",                 8, 3, 4),
    N_NOT("Anında Kanal",   "Cast süresi -%20",                 9, 3, 5),

    N_MIN("Soğuk Dokunuş",  "Büyü hasarı +%3",                  0, 5, 1),
    N_MIN("Geniş Dalga",    "Buz Novası yarıçapı +20px",       11, 5, 2),
    N_NOT("Buz Fırtınası",  "Buz Novası yarıçapı +40px daha",  12, 5, 3),
    N_MIN("Statik",         "Kritik şansı +%3",                13, 5, 4),
    N_NOT("Yüksek Gerilim", "Büyü hasarı +%10",                14, 5, 5),

    N_KEY("ARŞİBÜYÜCÜ",     "Büyü hasarı +%30 — max HP -15",               5, 10, 2),
    N_KEY("SAVAŞ RAHİBİ",   "Melee vuruş +4 mana yeniler — büyü -%10",    10, 15, 4),

    // Aktif büyüler (satın al → hotbar; mana tüketir, cast köklü).
    // Ateş Topu AĞAÇTA DEĞİL: temel kıvılcım herkeste doğuştan açık —
    // yoksa Yıkım XP'si kazanılamaz (bootstrap). Melee'deki 'normal vuruş' dengi.
    N_ACT("Buz Novası",     "AKTİF: çevrene patlama + %50 yavaş (20 mana)",11, 6, 2, ABILITY_ICE_NOVA),
    N_ACT("Yıldırım",       "AKTİF: hedefe anında 45 hasar (30 mana)",      7, 2, 3, ABILITY_LIGHTNING),
    N_ACT("Alev Zırhı",     "AKTİF: 15sn yakın saldırgana 8 hasar (25 mana)", 1, 0, 2, ABILITY_FLAME_ARMOR),
    N_ACT("Büyü Kalkanı",   "AKTİF: 12sn hasar HP yerine MANADAN emilir (25 mana)", 6, 4, 2, ABILITY_MAGIC_SHIELD),
    N_ACT("Toprak Dikenleri","AKTİF: çevrende diken patlaması + yavaş (22 mana)", 12, 6, 4, ABILITY_SPIKES),
};

// --- STAFF: "Asa Yolu" — tam büyücünün silah ağacı (asa kuşanılıyken parlar)
static const TreeNode T_STAFFTREE[] = {
    N_ROOT("Asa Tutuşu",    "Asa melee hasarı +%5"),
    N_MIN("Odak Kristali",  "Asa ile büyü +%3",                 0, 1, 1),
    N_MIN("Kanal Oyması",   "Asa ile büyü +%4",                 1, 1, 2),
    N_NOT("Güç Kanalı",     "Asa ile büyü +%8",                 2, 1, 3),
    N_MIN("Uyumlanma",      "Asa ile büyü +%5",                 3, 1, 4),
    N_NOT("Kadim Asa",      "Asa ile büyü +%10",                4, 1, 5),

    N_MIN("Mana Damarı",    "Mana yenilenmesi +%10",            0, 3, 1),
    N_MIN("Derin Kuyu",     "+10 max mana",                     6, 3, 2),
    N_NOT("Taşan Kaynak",   "Mana yenilenmesi +%20",            7, 3, 3),
    N_MIN("Geniş Havuz",    "+10 max mana",                     8, 3, 4),
    N_NOT("Öz Cimriliği",   "Büyü mana maliyeti -%10",          9, 3, 5),

    N_MIN("Savurma",        "Asa melee +%5",                    0, 5, 1),
    N_MIN("Denge Ucu",      "Asa melee +%5",                   11, 5, 2),
    N_NOT("Asa Dövüşü",     "Asa melee +%15",                  12, 5, 3),
    N_MIN("Sağlam Tutuş",   "Verdiğin itme +%10",              13, 5, 4),
    N_NOT("Kırbaç Ucu",     "Asa melee +%20",                  14, 5, 5),

    N_KEY("BAŞ BÜYÜCÜ",     "Asa ile büyü +%25 — asa melee -%30",          5, 10, 2),
    N_KEY("SIR TAŞIYICI",   "+25 mana, yenilenme +%25 — max stamina -20", 10, 15, 4),
};

// --- KADER: karakter-geneli ağaç (puan TOPLAM seviyeden — herkes kazanır) ---
static const TreeNode T_FATE[] = {
    N_ROOT("Yolcu",        "+5 max HP"),
    // Beden dalı
    N_MIN("Dinç Beden",    "+10 max HP",                       0, 1, 1),
    N_MIN("Geniş Ciğer",   "+15 max stamina",                  1, 1, 2),
    N_NOT("Demir Ciğer",   "Stamina yenilenmesi +%25",         2, 1, 3),
    N_MIN("Sağlam Bünye",  "+15 max HP",                       3, 1, 4),
    N_NOT("Yaşam Gücü",    "Saniyede 0.5 can yenilenir",       4, 1, 5),
    // Zihin dalı
    N_MIN("Odak",          "+10 max mana",                     0, 3, 1),
    N_MIN("Berrak Zihin",  "Mana yenilenmesi +%25",            6, 3, 2),
    N_NOT("Hızlı Öğrenen", "TÜM skill XP'si +%5",              7, 3, 3),
    N_MIN("Derin Odak",    "+15 max mana",                     8, 3, 4),
    N_NOT("Bilge",         "TÜM skill XP'si +%8",              9, 3, 5),
    // Yaşam dalı
    N_MIN("Pazarcı",       "Satış fiyatları +%3",              0, 5, 1),
    N_MIN("Talihli",       "Kasa altını +%8",                 11, 5, 2),
    N_NOT("Hazineci",      "Kasa altını +%15",                12, 5, 3),
    N_MIN("Cin Fikirli",   "İksir beklemesi -%10",            13, 5, 4),
    N_NOT("Dört Ayak",     "%3 şansla hasarı savuştur",       14, 5, 5),

    N_KEY("AZİM",          "TÜM XP +%15 — kasa altını -%25",              5, 10, 2),
    N_KEY("ŞANSLI YILDIZ", "Kasa altını +%30, kritik +%3 — TÜM XP -%10", 10, 15, 4),
};

// --- MESLEK MİNİ AĞAÇLARI: kök + A dalı (hız) + B dalı (verim) + keystone ---
#define M_ROOT(nm, ds)            { nm, ds, NODE_MINOR,    -1, -1, 3, 0, -1 }
#define M_KEY(nm, ds)             { nm, ds, NODE_KEYSTONE,  3,  6, 3, 4, -1 }

static const TreeNode T_MINING[] = {
    M_ROOT("Kazma Alışkanlığı", "Kazma işi +%5 hızlı"),
    N_MIN("Sağlam Vuruş",  "Kazma işi +%8 hızlı",              0, 1, 1),
    N_MIN("Ritmik Kazma",  "Kazma işi +%10 hızlı",             1, 1, 2),
    N_NOT("Damar Ustası",  "Kazma işi +%15 hızlı",             2, 1, 3),
    N_MIN("Keskin Bakış",  "%5 şansla çift cevher",            0, 5, 1),
    N_MIN("Eli Açık Dağ",  "%8 şansla çift cevher",            4, 5, 2),
    N_NOT("Zengin Damar",  "%15 şansla çift cevher",           5, 5, 3),
    M_KEY("DERİN DAMAR",   "Çift cevher +%25 — kazma %20 yavaş"),
};
static const TreeNode T_HERBALISM[] = {
    M_ROOT("Bitki Bilgisi", "Toplama +%5 hızlı"),
    N_MIN("Hızlı Parmak",  "Toplama +%8 hızlı",                0, 1, 1),
    N_MIN("Alışkın El",    "Toplama +%10 hızlı",               1, 1, 2),
    N_NOT("Kök Sökücü",    "Toplama +%15 hızlı",               2, 1, 3),
    N_MIN("Dikkatli Göz",  "%5 şansla çift bitki",             0, 5, 1),
    N_MIN("Bereket",       "%8 şansla çift bitki",             4, 5, 2),
    N_NOT("Doğanın Payı",  "%15 şansla çift bitki",            5, 5, 3),
    M_KEY("YEŞİL EL",      "Çift bitki +%25 — toplama %20 yavaş"),
};
static const TreeNode T_WOODCUT[] = {
    M_ROOT("Balta Bilgisi", "Kesim +%5 hızlı"),
    N_MIN("Güçlü Savuruş", "Kesim +%8 hızlı",                  0, 1, 1),
    N_MIN("Çentik Ustası", "Kesim +%10 hızlı",                 1, 1, 2),
    N_NOT("Tek Darbe",     "Kesim +%15 hızlı",                 2, 1, 3),
    N_MIN("Budak Gözü",    "%5 şansla çift odun",              0, 5, 1),
    N_MIN("Verimli Kesim", "%8 şansla çift odun",              4, 5, 2),
    N_NOT("Orman Hasadı",  "%15 şansla çift odun",             5, 5, 3),
    M_KEY("ORMANIN DOSTU", "Çift odun +%25 — kesim %20 yavaş"),
};
static const TreeNode T_SMITHING[] = {
    M_ROOT("Ocak Alışkanlığı", "Dövme işi +%5 hızlı"),
    N_MIN("İsabetli Çekiç","Dövme işi +%8 hızlı",              0, 1, 1),
    N_MIN("Çelik Bilek",   "Dövme işi +%10 hızlı",             1, 1, 2),
    N_NOT("Usta Demirci",  "Dövme işi +%15 hızlı",             2, 1, 3),
    N_MIN("Dikkatli Dövme","'+basma' şansı +%3",               0, 5, 1),
    N_MIN("Soğutma Sırrı", "'+basma' şansı +%4",               4, 5, 2),
    N_NOT("Kusursuz Dövme","'+basma' şansı +%8",               5, 5, 3),
    M_KEY("USTA ÇEKİÇ",    "'+basma' şansı +%15 — dövme %20 yavaş"),
};
static const TreeNode T_ALCHEMY[] = {
    M_ROOT("Kazan Bilgisi", "Kaynatma +%5 hızlı"),
    N_MIN("Kıvam Ustası",  "Kaynatma +%8 hızlı",               0, 1, 1),
    N_MIN("Ateş Kontrolü", "Kaynatma +%10 hızlı",              1, 1, 2),
    N_NOT("Seri Kaynatma", "Kaynatma +%15 hızlı",              2, 1, 3),
    N_MIN("Saf Özüt",      "İksirler +%5 etkili",              0, 5, 1),
    N_MIN("Damıtma",       "İksirler +%5 etkili",              4, 5, 2),
    N_NOT("Güçlü Karışım", "İksirler +%10 etkili",             5, 5, 3),
    M_KEY("İKSİR USTASI",  "İksirler +%20 etkili — iksir beklemesi +%20"),
};
static const TreeNode T_ENCHANTING[] = {
    M_ROOT("Sunak Bilgisi", "Büyüleme törenleri +%5 hızlı"),
    N_MIN("Akıcı Tören",   "Törenler +%8 hızlı",               0, 1, 1),
    N_MIN("Öz Hakimiyeti", "Törenler +%10 hızlı",              1, 1, 2),
    N_NOT("Kadim Ustalık", "Törenler +%15 hızlı",              2, 1, 3),
    N_MIN("Dengeli Öz",    "Rün üretim şansı +%3",             0, 5, 1),
    N_MIN("Saf Kanal",     "Rün üretim şansı +%4",             4, 5, 2),
    N_NOT("Rün Bilgisi",   "Rün üretim şansı +%8",             5, 5, 3),
    M_KEY("RÜN HAKİMİ",    "Rün şansı +%15 — törenler %30 yavaş"),
};

#define TREE_DEF(id, tbl) { id, tbl, (int)(sizeof(tbl) / sizeof((tbl)[0])) }
static const SkillTreeDef TREES[] = {
    TREE_DEF(SKILL_ONE_HANDED,   T_ONEHAND),
    TREE_DEF(SKILL_ARCHERY,      T_ARCHERY),
    TREE_DEF(SKILL_LIGHT_ARMOR,  T_LIGHT),
    TREE_DEF(SKILL_MEDIUM_ARMOR, T_MEDIUM),
    TREE_DEF(SKILL_HEAVY_ARMOR,  T_HEAVY),
    TREE_DEF(SKILL_SNEAK,        T_SNEAK),
    TREE_DEF(SKILL_SPEECH,       T_SPEECH),
    TREE_DEF(SKILL_MINING,       T_MINING),
    TREE_DEF(SKILL_HERBALISM,    T_HERBALISM),
    TREE_DEF(SKILL_WOODCUTTING,  T_WOODCUT),
    TREE_DEF(SKILL_SMITHING,     T_SMITHING),
    TREE_DEF(SKILL_ALCHEMY,      T_ALCHEMY),
    TREE_DEF(SKILL_ENCHANTING,   T_ENCHANTING),
    TREE_DEF(SKILL_DESTRUCTION,  T_DESTRUCTION),
    TREE_DEF(SKILL_STAFF,        T_STAFFTREE),
    TREE_DEF(TREE_FATE,          T_FATE),
};
static const int TREE_COUNT = (int)(sizeof(TREES) / sizeof(TREES[0]));

const SkillTreeDef *tree_get(int treeId)
{
    for (int i = 0; i < TREE_COUNT; i++)
        if (TREES[i].treeId == treeId) return &TREES[i];
    return NULL;
}

bool tree_has_content(int treeId) { return tree_get(treeId) != NULL; }

int tree_node_cost(NodeClass c)
{
    return (c == NODE_KEYSTONE) ? 3
         : (c == NODE_NOTABLE || c == NODE_ACTIVE) ? 2 : 1;
}

// DEBUG (F10): her ağaca eklenen serbest puan — kaydedilmez, restart'ta sıfır
static int g_debugPoints = 0;

void tree_set_debug_points(int bonus)
{
    g_debugPoints = bonus;
}

int tree_points_earned(int treeId, int level)
{
    return level / (treeId == TREE_FATE ? FATE_LEVELS_PER_POINT
                                        : TREE_LEVELS_PER_POINT)
         + g_debugPoints;
}

int tree_points_spent(int treeId, Uint32 taken)
{
    const SkillTreeDef *t = tree_get(treeId);
    if (t == NULL) return 0;
    int spent = 0;
    for (int i = 0; i < t->nodeCount; i++)
        if (taken & (1u << i))
            spent += tree_node_cost(t->nodes[i].cls);
    return spent;
}

bool tree_node_available(int treeId, int node, Uint32 taken, int freePoints)
{
    const SkillTreeDef *t = tree_get(treeId);
    if (t == NULL || node < 0 || node >= t->nodeCount) return false;
    if (taken & (1u << node)) return false;                    // zaten alınmış
    const TreeNode *n = &t->nodes[node];
    if (tree_node_cost(n->cls) > freePoints) return false;     // puan yetmez
    if (n->prereqA < 0 && n->prereqB < 0) return true;         // kök
    bool pathA = n->prereqA >= 0 && (taken & (1u << n->prereqA));
    bool pathB = n->prereqB >= 0 && (taken & (1u << n->prereqB));
    return pathA || pathB;                                     // BİRİ yeter
}

// ---------------------------------------------------------------------------
// ETKİLER: alınmış düğümlerden birleşik PerkEffects (tek nokta — eski desen)
// ---------------------------------------------------------------------------
#define ON(tree, i) (taken[tree] & (1u << (i)))
#define ONF(i)      (fateTaken & (1u << (i)))

void tree_compute_effects(const Uint32 taken[SKILL_COUNT], Uint32 fateTaken,
                          PerkEffects *fx)
{
    // Nötr başlangıç
    *fx = (PerkEffects){
        .oneHandDmgMult = 1.0f, .attackIntervalMult = 1.0f,
        .critMult = 1.5f, .psCdMult = 1.0f, .qsCdMult = 1.0f,
        .bleedPower = 1.0f,
        .defMult = { 1.0f, 1.0f, 1.0f, 1.0f },
        .dmgTakenMult = 1.0f, .defenseK = 100.0f, .kbTakenMult = 1.0f,
        .speedMult = 1.0f, .nightSpeedMult = 1.0f,
        .potionEffMult = 1.0f, .potionCdMult = 1.0f,
        .detectOnPlayerMult = 1.0f, .kbDealtMult = 1.0f,
        .bowDmgMult = 1.0f, .drawSpeedMult = 1.0f, .bowStaminaMult = 1.0f,
        .sneakDetectMult = 1.0f, .backstabMult = 2.0f, .nightDetectMult = 1.0f,
        .shopBuyMult = 1.0f, .shopSellMult = 1.0f, .dicePenaltyMult = 1.0f,
        .contractGoldMult = 1.0f, .contractRepMult = 1.0f,
        .trainCostMult = 1.0f, .bribeCostMult = 1.0f,
        .mineSpeedMult = 1.0f, .herbSpeedMult = 1.0f, .woodSpeedMult = 1.0f,
        .smithSpeedMult = 1.0f, .alchSpeedMult = 1.0f, .enchSpeedMult = 1.0f,
        .stamRegenMult = 1.0f, .manaRegenMult = 1.0f,
        .xpGainMult = 1.0f, .goldFindMult = 1.0f,
        .spellDmgMult = 1.0f, .manaCostMult = 1.0f, .castSpeedMult = 1.0f,
        .staffDmgMult = 1.0f,
    };

    // Temel kıvılcım: Ateş Topu HERKESTE açık (büyünün 'normal vuruşu' —
    // Yıkım XP'sinin başlangıç kaynağı; gerçek büyüler ağaçtan satın alınır)
    fx->abilityOpen[ABILITY_FIREBOLT] = true;

    // Aktif düğümler: hangi beceriler açık (tüm ağaçlarda grantAbility taraması)
    for (int t = 0; t < TREE_COUNT; t++)
    {
        if (TREES[t].treeId == TREE_FATE) continue;   // kaderde aktif yok
        Uint32 m = taken[TREES[t].treeId];
        for (int i = 0; i < TREES[t].nodeCount; i++)
            if ((m & (1u << i)) && TREES[t].nodes[i].grantAbility >= 0)
                fx->abilityOpen[TREES[t].nodes[i].grantAbility] = true;
    }

    // --- One-Handed (Kılıç Yolu) ---
    {
        const int T = SKILL_ONE_HANDED;
        if (ON(T, 0))  fx->oneHandDmgMult *= 1.03f;
        if (ON(T, 1))  fx->oneHandDmgMult *= 1.03f;
        if (ON(T, 2))  fx->critChanceBonus += 0.02f;
        if (ON(T, 3))  fx->bleedFrac = 0.30f;
        if (ON(T, 4))  fx->bleedPower += 0.25f;
        if (ON(T, 5))  fx->bleedPower *= 2.0f;
        if (ON(T, 6))  fx->attackIntervalMult *= 0.96f;
        if (ON(T, 7))  fx->attackIntervalMult *= 0.96f;
        if (ON(T, 8))  fx->attackIntervalMult *= 0.90f;
        if (ON(T, 9))  fx->qsCdMult *= 0.85f;
        if (ON(T, 10)) fx->psCdMult *= 0.5f;
        if (ON(T, 11)) fx->critChanceBonus += 0.02f;
        if (ON(T, 12)) fx->critChanceBonus += 0.03f;
        if (ON(T, 13)) fx->critChanceBonus += 0.08f;
        if (ON(T, 14)) fx->oneHandDmgMult *= 1.04f;
        if (ON(T, 15)) fx->critMult = 1.8f;
        if (ON(T, 16)) { fx->oneHandDmgMult *= 1.25f; fx->dmgTakenMult *= 1.20f; }   // BERSERKER
        if (ON(T, 17)) { fx->executeBonus = 0.60f; fx->maxHpBonus -= 10.0f; }        // İNFAZCI
    }

    // --- Archery (Yay Yolu) ---
    {
        const int T = SKILL_ARCHERY;
        if (ON(T, 0))  fx->bowDmgMult *= 1.03f;
        if (ON(T, 1))  fx->bowDmgMult *= 1.03f;
        if (ON(T, 2))  fx->bowRangeBonus += 20.0f;
        if (ON(T, 3))  { fx->bowRangeBonus += 40.0f; fx->critChanceBonus += 0.03f; }
        if (ON(T, 4))  fx->critChanceBonus += 0.03f;
        if (ON(T, 5))  fx->bowDmgMult *= 1.12f;
        if (ON(T, 6))  fx->drawSpeedMult *= 0.95f;
        if (ON(T, 7))  fx->drawSpeedMult *= 0.95f;
        if (ON(T, 8))  fx->drawSpeedMult *= 0.85f;
        if (ON(T, 9))  fx->bowStaminaMult *= 0.85f;
        if (ON(T, 10)) fx->ammoSaveChance += 0.20f;
        if (ON(T, 11)) fx->bowStaminaMult *= 0.90f;
        if (ON(T, 12)) fx->bowDmgMult *= 1.03f;
        if (ON(T, 13)) fx->volleyExtra += 1;
        if (ON(T, 14)) fx->disengageBonus += 40.0f;
        if (ON(T, 15)) fx->aoeRadiusBonus += 30.0f;
        if (ON(T, 16)) { fx->bowDmgMult *= 1.30f; fx->drawSpeedMult *= 1.20f; }      // KESKİN NİŞANCI
        if (ON(T, 17)) { fx->drawSpeedMult *= 0.75f; fx->bowDmgMult *= 0.90f; }      // AKINCI
    }

    // --- Light Armor (Rüzgar Yolu) ---
    {
        const int T = SKILL_LIGHT_ARMOR;
        if (ON(T, 0))  fx->speedMult *= 1.02f;
        if (ON(T, 1))  fx->defMult[ARMOR_LIGHT] *= 1.10f;
        if (ON(T, 2))  fx->maxHpBonus += 5.0f;
        if (ON(T, 3))  fx->defMult[ARMOR_LIGHT] *= 1.20f;
        if (ON(T, 4))  fx->maxHpBonus += 10.0f;
        if (ON(T, 5))  fx->dmgWhenHealthy = 0.10f;
        if (ON(T, 6))  fx->speedMult *= 1.02f;
        if (ON(T, 7))  fx->speedMult *= 1.02f;
        if (ON(T, 8))  fx->speedMult *= 1.05f;
        if (ON(T, 9))  fx->nightSpeedMult *= 1.03f;
        if (ON(T, 10)) fx->nightSpeedMult *= 1.05f;
        if (ON(T, 11)) fx->dodgeChance += 0.02f;
        if (ON(T, 12)) fx->dodgeChance += 0.02f;
        if (ON(T, 13)) fx->dodgeChance += 0.05f;
        if (ON(T, 14)) fx->potionCdMult *= 0.85f;
        if (ON(T, 15)) fx->dodgeChance += 0.04f;
        if (ON(T, 16)) { fx->detectOnPlayerMult *= 0.75f; fx->kbTakenMult = 0.0f;
                         fx->maxHpBonus -= 15.0f; }                                  // HAYALET
        if (ON(T, 17)) { fx->speedMult *= 1.10f; fx->dmgTakenMult *= 1.10f; }        // RÜZGARIN OĞLU
    }

    // --- Medium Armor (Denge Yolu) ---
    {
        const int T = SKILL_MEDIUM_ARMOR;
        if (ON(T, 0))  fx->maxHpBonus += 5.0f;
        if (ON(T, 1))  fx->defMult[ARMOR_MEDIUM] *= 1.07f;
        if (ON(T, 2))  fx->maxHpBonus += 10.0f;
        if (ON(T, 3))  fx->defMult[ARMOR_MEDIUM] *= 1.25f;
        if (ON(T, 4))  fx->maxHpBonus += 10.0f;
        if (ON(T, 5))  fx->yaraliAyi = true;
        if (ON(T, 6))  fx->dmgTakenMult *= 0.98f;
        if (ON(T, 7))  fx->maxHpBonus += 5.0f;
        if (ON(T, 8))  fx->dmgTakenMult *= 0.95f;
        if (ON(T, 9))  fx->dmgTakenMult *= 0.97f;
        if (ON(T, 10)) fx->dmgTakenMult *= 0.90f;
        if (ON(T, 11)) fx->potionEffMult *= 1.10f;
        if (ON(T, 12)) fx->potionEffMult *= 1.15f;
        if (ON(T, 13)) fx->potionEffMult *= 1.25f;
        if (ON(T, 14)) fx->gaziDmgBonus += 0.05f;
        if (ON(T, 15)) fx->gaziDmgBonus += 0.10f;
        if (ON(T, 16)) { fx->dmgTakenMult *= 0.85f; fx->speedMult *= 0.92f; }        // YAŞAYAN KALE
        if (ON(T, 17)) { fx->reflectFrac += 0.20f;
                         if (fx->deathSaveCooldown <= 0.0f) fx->deathSaveCooldown = 60.0f;
                         fx->potionEffMult *= 0.80f; }                               // FEDAİ
    }

    // --- Heavy Armor (Demir Yolu) ---
    {
        const int T = SKILL_HEAVY_ARMOR;
        if (ON(T, 0))  fx->defMult[ARMOR_HEAVY] *= 1.05f;
        if (ON(T, 1))  fx->defMult[ARMOR_HEAVY] *= 1.10f;
        if (ON(T, 2))  fx->maxHpBonus += 10.0f;
        if (ON(T, 3))  fx->defMult[ARMOR_HEAVY] *= 1.25f;
        if (ON(T, 4))  fx->maxHpBonus += 10.0f;
        if (ON(T, 5))  fx->defenseK = 120.0f;
        if (ON(T, 6))  fx->kbTakenMult *= 0.75f;
        if (ON(T, 7))  fx->kbTakenMult *= 0.75f;
        if (ON(T, 8))  fx->kbTakenMult = 0.0f;
        if (ON(T, 9))  fx->dmgTakenMult *= 0.97f;
        if (ON(T, 10)) fx->dmgTakenMult *= 0.92f;
        if (ON(T, 11)) fx->kbDealtMult *= 1.05f;
        if (ON(T, 12)) fx->kbDealtMult *= 1.05f;
        if (ON(T, 13)) { fx->kbDealtMult *= 1.10f; fx->oneHandDmgMult *= 1.05f; }
        if (ON(T, 14)) fx->reflectFlat += 2.0f;
        if (ON(T, 15)) fx->reflectFlat += 4.0f;
        if (ON(T, 16)) { fx->dmgTakenMult *= 0.75f; fx->speedMult *= 0.90f; }        // TAŞ BEDEN
        if (ON(T, 17)) { fx->deathSaveCooldown = 90.0f; fx->dmgTakenMult *= 0.95f;
                         fx->speedMult *= 0.95f; }                                   // ÖLÜMSÜZ
    }

    // --- Sneak (Gölge Yolu) ---
    {
        const int T = SKILL_SNEAK;
        if (ON(T, 0))  fx->sneakSpeedRelief += 0.10f;
        if (ON(T, 1))  fx->sneakDetectMult *= 0.95f;
        if (ON(T, 2))  fx->sneakDetectMult *= 0.95f;
        if (ON(T, 3))  fx->sneakDetectMult *= 0.88f;
        if (ON(T, 4))  fx->sneakDetectMult *= 0.94f;
        if (ON(T, 5))  fx->sneakDetectMult *= 0.85f;
        if (ON(T, 6))  fx->sneakSpeedRelief += 0.10f;
        if (ON(T, 7))  fx->sneakSpeedRelief += 0.10f;
        if (ON(T, 8))  fx->sneakSpeedRelief += 0.25f;
        if (ON(T, 9))  fx->speedMult *= 1.02f;
        if (ON(T, 10)) fx->sneakSpeedRelief = 1.0f;   // Panter: ceza tamamen kalkar
        if (ON(T, 11)) fx->backstabMult += 0.15f;
        if (ON(T, 12)) fx->backstabMult += 0.15f;
        if (ON(T, 13)) fx->backstabMult += 0.50f;
        if (ON(T, 14)) fx->backstabMult += 0.20f;
        if (ON(T, 15)) fx->backstabMult += 0.50f;
        if (ON(T, 16)) { fx->backstabMult += 1.0f; fx->sneakDetectMult *= 0.90f;
                         fx->maxHpBonus -= 15.0f; }                                  // GÖLGE
        if (ON(T, 17)) { fx->nightDetectMult *= 0.70f;
                         fx->detectOnPlayerMult *= 1.10f; }                          // HORTLAK
    }

    // --- Speech (Dil Yolu) ---
    {
        const int T = SKILL_SPEECH;
        if (ON(T, 0))  fx->speechDiceBonus += 3.0f;
        if (ON(T, 1))  fx->shopBuyMult *= 0.97f;
        if (ON(T, 2))  fx->shopBuyMult *= 0.96f;
        if (ON(T, 3))  fx->shopBuyMult *= 0.92f;
        if (ON(T, 4))  fx->shopSellMult *= 1.05f;
        if (ON(T, 5))  fx->shopSellMult *= 1.12f;
        if (ON(T, 6))  fx->speechDiceBonus += 4.0f;
        if (ON(T, 7))  fx->speechDiceBonus += 5.0f;
        if (ON(T, 8))  fx->speechDiceBonus += 10.0f;
        if (ON(T, 9))  fx->trainCostMult *= 0.85f;
        if (ON(T, 10)) fx->speechDiceBonus += 12.0f;
        if (ON(T, 11)) fx->contractGoldMult *= 1.05f;
        if (ON(T, 12)) fx->contractGoldMult *= 1.05f;
        if (ON(T, 13)) fx->contractGoldMult *= 1.15f;
        if (ON(T, 14)) fx->bribeCostMult *= 0.85f;
        if (ON(T, 15)) fx->bribeCostMult *= 0.50f;
        if (ON(T, 16)) { fx->speechDiceBonus += 20.0f; fx->dicePenaltyMult = 2.0f; } // GÜMÜŞ DİL
        if (ON(T, 17)) { fx->shopBuyMult *= 0.88f; fx->shopSellMult *= 1.15f;
                         fx->contractRepMult *= 0.5f; }                              // ALTIN AĞIZ
    }

    // --- Meslek mini ağaçları ---
    {
        const int T = SKILL_MINING;
        if (ON(T, 0)) fx->mineSpeedMult *= 1.05f;
        if (ON(T, 1)) fx->mineSpeedMult *= 1.08f;
        if (ON(T, 2)) fx->mineSpeedMult *= 1.10f;
        if (ON(T, 3)) fx->mineSpeedMult *= 1.15f;
        if (ON(T, 4)) fx->mineDouble += 0.05f;
        if (ON(T, 5)) fx->mineDouble += 0.08f;
        if (ON(T, 6)) fx->mineDouble += 0.15f;
        if (ON(T, 7)) { fx->mineDouble += 0.25f; fx->mineSpeedMult *= 0.80f; }       // DERİN DAMAR
    }
    {
        const int T = SKILL_HERBALISM;
        if (ON(T, 0)) fx->herbSpeedMult *= 1.05f;
        if (ON(T, 1)) fx->herbSpeedMult *= 1.08f;
        if (ON(T, 2)) fx->herbSpeedMult *= 1.10f;
        if (ON(T, 3)) fx->herbSpeedMult *= 1.15f;
        if (ON(T, 4)) fx->herbDouble += 0.05f;
        if (ON(T, 5)) fx->herbDouble += 0.08f;
        if (ON(T, 6)) fx->herbDouble += 0.15f;
        if (ON(T, 7)) { fx->herbDouble += 0.25f; fx->herbSpeedMult *= 0.80f; }       // YEŞİL EL
    }
    {
        const int T = SKILL_WOODCUTTING;
        if (ON(T, 0)) fx->woodSpeedMult *= 1.05f;
        if (ON(T, 1)) fx->woodSpeedMult *= 1.08f;
        if (ON(T, 2)) fx->woodSpeedMult *= 1.10f;
        if (ON(T, 3)) fx->woodSpeedMult *= 1.15f;
        if (ON(T, 4)) fx->woodDouble += 0.05f;
        if (ON(T, 5)) fx->woodDouble += 0.08f;
        if (ON(T, 6)) fx->woodDouble += 0.15f;
        if (ON(T, 7)) { fx->woodDouble += 0.25f; fx->woodSpeedMult *= 0.80f; }       // ORMANIN DOSTU
    }
    {
        const int T = SKILL_SMITHING;
        if (ON(T, 0)) fx->smithSpeedMult *= 1.05f;
        if (ON(T, 1)) fx->smithSpeedMult *= 1.08f;
        if (ON(T, 2)) fx->smithSpeedMult *= 1.10f;
        if (ON(T, 3)) fx->smithSpeedMult *= 1.15f;
        if (ON(T, 4)) fx->plusChanceBonus += 3.0f;
        if (ON(T, 5)) fx->plusChanceBonus += 4.0f;
        if (ON(T, 6)) fx->plusChanceBonus += 8.0f;
        if (ON(T, 7)) { fx->plusChanceBonus += 15.0f; fx->smithSpeedMult *= 0.80f; } // USTA ÇEKİÇ
    }
    {
        const int T = SKILL_ALCHEMY;
        if (ON(T, 0)) fx->alchSpeedMult *= 1.05f;
        if (ON(T, 1)) fx->alchSpeedMult *= 1.08f;
        if (ON(T, 2)) fx->alchSpeedMult *= 1.10f;
        if (ON(T, 3)) fx->alchSpeedMult *= 1.15f;
        if (ON(T, 4)) fx->potionEffMult *= 1.05f;
        if (ON(T, 5)) fx->potionEffMult *= 1.05f;
        if (ON(T, 6)) fx->potionEffMult *= 1.10f;
        if (ON(T, 7)) { fx->potionEffMult *= 1.20f; fx->potionCdMult *= 1.20f; }     // İKSİR USTASI
    }
    {
        const int T = SKILL_ENCHANTING;
        if (ON(T, 0)) fx->enchSpeedMult *= 1.05f;
        if (ON(T, 1)) fx->enchSpeedMult *= 1.08f;
        if (ON(T, 2)) fx->enchSpeedMult *= 1.10f;
        if (ON(T, 3)) fx->enchSpeedMult *= 1.15f;
        if (ON(T, 4)) fx->runeChanceBonus += 3.0f;
        if (ON(T, 5)) fx->runeChanceBonus += 4.0f;
        if (ON(T, 6)) fx->runeChanceBonus += 8.0f;
        if (ON(T, 7)) { fx->runeChanceBonus += 15.0f; fx->enchSpeedMult *= 0.70f; }  // RÜN HAKİMİ
    }

    // --- Destruction (Yıkım Yolu) ---
    {
        const int T = SKILL_DESTRUCTION;
        if (ON(T, 0))  fx->spellDmgMult *= 1.03f;
        if (ON(T, 1))  fx->spellDmgMult *= 1.03f;
        if (ON(T, 2))  fx->burnBonus += 0.25f;
        if (ON(T, 3))  fx->burnBonus += 0.50f;
        if (ON(T, 4))  fx->spellDmgMult *= 1.04f;
        if (ON(T, 5))  fx->spellDmgMult *= 1.12f;
        if (ON(T, 6))  fx->manaCostMult *= 0.95f;
        if (ON(T, 7))  fx->manaCostMult *= 0.95f;
        if (ON(T, 8))  fx->manaCostMult *= 0.85f;
        if (ON(T, 9))  fx->castSpeedMult *= 0.90f;
        if (ON(T, 10)) fx->castSpeedMult *= 0.80f;
        if (ON(T, 11)) fx->spellDmgMult *= 1.03f;
        if (ON(T, 12)) fx->novaRadiusBonus += 20.0f;
        if (ON(T, 13)) fx->novaRadiusBonus += 40.0f;
        if (ON(T, 14)) fx->critChanceBonus += 0.03f;
        if (ON(T, 15)) fx->spellDmgMult *= 1.10f;
        if (ON(T, 16)) { fx->spellDmgMult *= 1.30f; fx->maxHpBonus -= 15.0f; }      // ARŞİBÜYÜCÜ
        if (ON(T, 17)) { fx->meleeManaRegen += 4.0f; fx->spellDmgMult *= 0.90f; }   // SAVAŞ RAHİBİ
    }

    // --- Staff (Asa Yolu) ---
    {
        const int T = SKILL_STAFF;
        if (ON(T, 0))  fx->staffDmgMult *= 1.05f;
        if (ON(T, 1))  fx->staffSpellBonus += 0.03f;
        if (ON(T, 2))  fx->staffSpellBonus += 0.04f;
        if (ON(T, 3))  fx->staffSpellBonus += 0.08f;
        if (ON(T, 4))  fx->staffSpellBonus += 0.05f;
        if (ON(T, 5))  fx->staffSpellBonus += 0.10f;
        if (ON(T, 6))  fx->manaRegenMult *= 1.10f;
        if (ON(T, 7))  fx->maxManaBonus += 10.0f;
        if (ON(T, 8))  fx->manaRegenMult *= 1.20f;
        if (ON(T, 9))  fx->maxManaBonus += 10.0f;
        if (ON(T, 10)) fx->manaCostMult *= 0.90f;
        if (ON(T, 11)) fx->staffDmgMult *= 1.05f;
        if (ON(T, 12)) fx->staffDmgMult *= 1.05f;
        if (ON(T, 13)) fx->staffDmgMult *= 1.15f;
        if (ON(T, 14)) fx->kbDealtMult *= 1.10f;
        if (ON(T, 15)) fx->staffDmgMult *= 1.20f;
        if (ON(T, 16)) { fx->staffSpellBonus += 0.25f; fx->staffDmgMult *= 0.70f; } // BAŞ BÜYÜCÜ
        if (ON(T, 17)) { fx->maxManaBonus += 25.0f; fx->manaRegenMult *= 1.25f;
                         fx->maxStaminaBonus -= 20.0f; }                            // SIR TAŞIYICI
    }

    // --- Kader Ağacı (karakter-geneli) ---
    if (ONF(0))  fx->maxHpBonus += 5.0f;
    if (ONF(1))  fx->maxHpBonus += 10.0f;
    if (ONF(2))  fx->maxStaminaBonus += 15.0f;
    if (ONF(3))  fx->stamRegenMult *= 1.25f;
    if (ONF(4))  fx->maxHpBonus += 15.0f;
    if (ONF(5))  fx->hpRegenPerSec += 0.5f;
    if (ONF(6))  fx->maxManaBonus += 10.0f;
    if (ONF(7))  fx->manaRegenMult *= 1.25f;
    if (ONF(8))  fx->xpGainMult *= 1.05f;
    if (ONF(9))  fx->maxManaBonus += 15.0f;
    if (ONF(10)) fx->xpGainMult *= 1.08f;
    if (ONF(11)) fx->shopSellMult *= 1.03f;
    if (ONF(12)) fx->goldFindMult *= 1.08f;
    if (ONF(13)) fx->goldFindMult *= 1.15f;
    if (ONF(14)) fx->potionCdMult *= 0.90f;
    if (ONF(15)) fx->dodgeChance += 0.03f;
    if (ONF(16)) { fx->xpGainMult *= 1.15f; fx->goldFindMult *= 0.75f; }             // AZİM
    if (ONF(17)) { fx->goldFindMult *= 1.30f; fx->critChanceBonus += 0.03f;
                   fx->xpGainMult *= 0.90f; }                                        // ŞANSLI YILDIZ
}

// ---------------------------------------------------------------------------
// PANEL: düğüm-kenar haritası. Çizim ve tıklama AYNI yerleşimi kullanır.
// ---------------------------------------------------------------------------
#define TP_CELL 56.0f
#define TP_PAD 16.0f
#define TP_W (7 * TP_CELL + TP_PAD * 2)          // gx 0..6
#define TP_GRID_Y 34.0f                          // başlık altı
#define TP_DETAIL_H 64.0f                        // alt detay kutusu

static float node_size(NodeClass c)
{
    return (c == NODE_KEYSTONE) ? 40.0f
         : (c == NODE_NOTABLE || c == NODE_ACTIVE) ? 32.0f : 24.0f;
}

static void node_rect(const TreeNode *n, float px, float py, SDL_FRect *out)
{
    float s = node_size(n->cls);
    float cx = px + TP_PAD + n->gx * TP_CELL + TP_CELL / 2.0f;
    float cy = py + TP_GRID_Y + n->gy * TP_CELL + TP_CELL / 2.0f;
    *out = (SDL_FRect){ cx - s / 2.0f, cy - s / 2.0f, s, s };
}

static const char *tree_title(int treeId)
{
    return (treeId == TREE_FATE) ? "Kader" : skill_name((SkillType)treeId);
}

void tree_panel_render(int treeId, Uint32 taken, int level,
                       int selNode, int respecCost,
                       R2D *renderer, TTF_Font *font, float px, float py)
{
    const SkillTreeDef *t = tree_get(treeId);
    if (t == NULL) return;

    float panelH = TP_GRID_Y + 7 * TP_CELL + TP_DETAIL_H + TP_PAD * 2;
    ui_panel(renderer, px, py, TP_W, panelH);
    if (font == NULL) return;

    SDL_Color white  = { 255, 255, 255, 255 };
    SDL_Color gold   = { 255, 210, 60, 255 };
    SDL_Color dim    = { 140, 140, 150, 255 };
    SDL_Color green  = { 150, 255, 150, 255 };
    SDL_Color purple = { 200, 150, 255, 255 };
    SDL_Color blue   = { 120, 190, 255, 255 };

    int earned = tree_points_earned(treeId, level);
    int spent  = tree_points_spent(treeId, taken);
    int freeP  = earned - spent;

    char title[96];
    snprintf(title, sizeof(title), "%s Ağacı — Puan: %d (kazanılan %d / harcanan %d)",
             tree_title(treeId), freeP, earned, spent);
    draw_text(renderer, font, title, px + TP_PAD, py + 8.0f, freeP > 0 ? gold : white);

    // Kenarlar (düğümlerin ALTINA çizilir)
    r2d_set_blend(renderer, R2D_BLEND_ALPHA);
    for (int i = 0; i < t->nodeCount; i++)
    {
        const TreeNode *n = &t->nodes[i];
        SDL_FRect a;
        node_rect(n, px, py, &a);
        float ax = a.x + a.w / 2.0f, ay = a.y + a.h / 2.0f;
        for (int p = 0; p < 2; p++)
        {
            int pre = (p == 0) ? n->prereqA : n->prereqB;
            if (pre < 0) continue;
            SDL_FRect b;
            node_rect(&t->nodes[pre], px, py, &b);
            bool lit = (taken & (1u << i)) && (taken & (1u << pre));
            bool half = (taken & (1u << pre)) != 0;
            if (lit)       r2d_set_color(renderer, 255, 210, 60, 255);
            else if (half) r2d_set_color(renderer, 190, 180, 140, 200);
            else           r2d_set_color(renderer, 90, 90, 100, 160);
            r2d_line(renderer, b.x + b.w / 2.0f, b.y + b.h / 2.0f, ax, ay);
        }
    }

    // Düğümler
    for (int i = 0; i < t->nodeCount; i++)
    {
        const TreeNode *n = &t->nodes[i];
        SDL_FRect r;
        node_rect(n, px, py, &r);
        bool isTaken = (taken & (1u << i)) != 0;
        bool avail   = tree_node_available(treeId, i, taken, freeP);

        if (isTaken)      ui_cell_hi(renderer, r.x, r.y, r.w, r.h);
        else              ui_cell_gray(renderer, r.x, r.y, r.w, r.h);

        // Sınıf çerçevesi: keystone mor, AKTİF mavi, dikkat çekici altın
        if (n->cls == NODE_KEYSTONE)     r2d_set_color(renderer, 200, 150, 255, 255);
        else if (n->cls == NODE_ACTIVE)  r2d_set_color(renderer, 120, 190, 255, 255);
        else if (n->cls == NODE_NOTABLE) r2d_set_color(renderer, 255, 210, 60, 220);
        else                             r2d_set_color(renderer, 150, 150, 160, 180);
        r2d_rect(renderer, &r);
        if (avail && !isTaken)
        {
            r2d_set_color(renderer, 150, 255, 150, 255);
            SDL_FRect r2 = { r.x - 2, r.y - 2, r.w + 4, r.h + 4 };
            r2d_rect(renderer, &r2);
        }
        if (i == selNode)
        {
            r2d_set_color(renderer, 255, 255, 255, 255);
            SDL_FRect r3 = { r.x - 4, r.y - 4, r.w + 8, r.h + 8 };
            r2d_rect(renderer, &r3);
        }
    }

    // Alt detay kutusu: seçili düğüm ya da genel ipucu
    float dy = py + TP_GRID_Y + 7 * TP_CELL + 8.0f;
    if (selNode >= 0 && selNode < t->nodeCount)
    {
        const TreeNode *n = &t->nodes[selNode];
        bool isTaken = (taken & (1u << selNode)) != 0;
        bool avail   = tree_node_available(treeId, selNode, taken, freeP);
        char line[128];
        snprintf(line, sizeof(line), "%s  [%s, %d puan]",
                 n->name,
                 n->cls == NODE_KEYSTONE ? "KEYSTONE"
                 : n->cls == NODE_ACTIVE ? "AKTİF BECERİ"
                 : n->cls == NODE_NOTABLE ? "Dikkat çekici" : "Küçük",
                 tree_node_cost(n->cls));
        draw_text(renderer, font, line, px + TP_PAD, dy,
                  n->cls == NODE_KEYSTONE ? purple
                  : n->cls == NODE_ACTIVE ? blue : white);
        draw_text(renderer, font, n->desc, px + TP_PAD, dy + 18.0f, dim);
        draw_text(renderer, font,
                  isTaken ? "Alınmış"
                  : avail ? "Tekrar tıkla: AL"
                  : "Kilitli (yol ya da puan eksik)",
                  px + TP_PAD, dy + 36.0f,
                  isTaken ? gold : avail ? green : dim);
    }
    else
    {
        draw_text(renderer, font, "Düğüme tıkla: incele — tekrar tıkla: al",
                  px + TP_PAD, dy, dim);
        draw_text(renderer, font,
                  treeId == TREE_FATE
                      ? "Kader: TOPLAM seviyenin her 10'u = 1 puan"
                      : "Her 4 seviye = 1 puan",
                  px + TP_PAD, dy + 18.0f, dim);
    }

    // Respec satırı (yalnız sunak yakınında — main respecCost >= 0 geçirir)
    if (respecCost >= 0)
    {
        char rline[64];
        snprintf(rline, sizeof(rline), "[Sunak] Ağacı sıfırla: %dg", respecCost);
        draw_text(renderer, font, rline, px + TP_PAD, dy + 36.0f + 4.0f,
                  (SDL_Color){ 255, 170, 120, 255 });
    }
}

int tree_panel_node_at(int treeId, float x, float y, float px, float py)
{
    const SkillTreeDef *t = tree_get(treeId);
    if (t == NULL) return -1;
    for (int i = 0; i < t->nodeCount; i++)
    {
        SDL_FRect r;
        node_rect(&t->nodes[i], px, py, &r);
        // tıklama alanı biraz cömert (küçük düğümler için)
        if (x >= r.x - 6 && x < r.x + r.w + 6 && y >= r.y - 6 && y < r.y + r.h + 6)
            return i;
    }
    // Respec satırı: detay kutusunun alt bandı
    float dy = py + TP_GRID_Y + 7 * TP_CELL + 8.0f + 40.0f;
    if (x >= px && x < px + TP_W && y >= dy && y < dy + 20.0f)
        return TREE_CLICK_RESPEC;
    return -1;
}
