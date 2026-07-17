#ifndef ITEM_H
#define ITEM_H
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "common.h"
#include "fx.h"   // WeaponFxType

#define MAX_ITEM_TYPES 96   // ITEM_TYPES kapasitesi (paralel diziler icin sabit boyut)
                            // DİKKAT: tablo 65'e çıkınca 64 taşmıştı (tomar crash'i) —
                            // yeni item eklerken bu tavana bakılmaz, bol tutuldu + load klampli

// Ekipman slotları (KO düzeni)
typedef enum {
    EQUIP_HELMET,      // kask
    EQUIP_CHEST,       // göğüs zırhı
    EQUIP_LEGS,        // pantolon
    EQUIP_GLOVES,      // eldiven
    EQUIP_BOOTS,       // ayakkabı
    EQUIP_WEAPON,      // sağ el (silah)
    EQUIP_SHIELD,      // sol el (kalkan / ikinci silah)
    EQUIP_NECKLACE,    // kolye
    EQUIP_RING_1,      // yüzük 1
    EQUIP_RING_2,      // yüzük 2
    EQUIP_EARRING_1,   // küpe 1
    EQUIP_EARRING_2,   // küpe 2
    EQUIP_BELT,        // kemer
    EQUIP_SLOT_COUNT,  // = 13 (slot sayısı)
    EQUIP_NONE = -1,   // kuşanılamaz item (slot yok)
} EquipSlot;

// Zırh sınıfı — hangi zırh skill'ini geliştirir/perk'lerden etkilenir
typedef enum {
    ARMOR_NONE = 0,    // zırh değil
    ARMOR_LIGHT,       // hafif (deri vb. — item'ları ileride)
    ARMOR_MEDIUM,      // orta
    ARMOR_HEAVY,       // ağır (mevcut demir set)
} ArmorClass;

// Silah tipi — hangi el slotlarını nasıl kullandığı
typedef enum {
    WEAPON_NONE,       // silah değil
    WEAPON_ONE_HAND,   // tek el (WEAPON slotu)
    WEAPON_TWO_HAND,   // çift el (WEAPON dolar, SHIELD kilitlenir)
    WEAPON_SHIELD,     // kalkan (SHIELD slotu)
    WEAPON_DUAL,       // çift tek-el (WEAPON + SHIELD'e tek-el)
    WEAPON_TOOL,       // toplama aracı (kazma/balta): WEAPON slotuna takılır,
                       // SALDIRAMAZ ("Silah kuşan!") — toplama savurması için
    WEAPON_BOW,        // yay: iki el (SHIELD kilitli), köklü atış + ok tüketir,
                       // hasar DEX ile ölçeklenir, skill = Archery
    WEAPON_STAFF,      // asa: iki el, zayıf melee + büyü gücü (skill = Staff; Faz 22)
} WeaponType;

// Item TÜRÜ — bir kez tanımlanır, tüm o item'ler paylaşır
typedef struct {
    const char *name;
    int iconIndex;               // envanter ikonu (Tool_Icons'ta sıra)
    int maxStack;
    int value;                   // vendor alış fiyatı (satış = yarısı)

    // --- Ekipman bilgileri ---
    bool equipable;              // kuşanılabilir mi
    EquipSlot equipSlot;         // hangi slota gider (EQUIP_NONE: kuşanılamaz)
    WeaponType weaponType;       // silahsa tipi (değilse WEAPON_NONE)
    float damage;                // silah taban hasarı (combat formülünün girdisi)
    float attackInterval;        // silahsa: iki otomatik vuruş arası süre (0 = varsayılan)
    float lifesteal;             // vuruş hasarının bu oranı cana döner (boss benzersizi)
    float defense;               // zırh/kalkan defansı (azalan getirili: 100/(100+def))
    ArmorClass armorClass;       // zırhsa sınıfı (skill XP + perk çarpanları)
    const char *equipSpritePath; // paper-doll katmanı (kuşanınca görünen sprite, NULL: yok)
    const char *equipSpritePathBack;   // 2. katman (silah arka — yoksa NULL)
    int equipZ;                  // ön katman z (LPC dosya öneki; 0 = slot_zorder varsayılanı)
    int equipZBack;              // arka katman z (equipSpritePathBack varsa zorunlu)
    int equipIdleRow;            // idle'da çizilecek satır (0 = yok, sheet'in kendi idle'ı oynar)
    bool slashOversize;          // slash'ı oversize bloktan çiz (64px slash satırı boş silahlar)
    bool thrustOversize;         // mızrak: katman SALT thrust-oversize sheet'i (1536×768,
                                 // 8 kare × 4 yön 192px) — thrust dışında çizilmez
    float armorPierce;           // hedef defansının bu oranını yok sayar (mızrak 0.25)
    WeaponFxType weaponFx;       // silahın partikül efekti (0 = WEAPON_FX_NONE)

    // --- Tüketilebilir (iksir vb.) ---
    bool consumable;             // sağ tık hotbar'a atar, hotbar tuşuyla kullanılır
    float healAmount;            // kullanınca iyileşen can
    float manaAmount;            // kullanınca dolan mana (Mana İksiri; Faz 22)
    float useCooldown;           // kullanım sonrası bu TÜRÜN kilit süresi (sn)
    float buffSpeedMult;         // >1: süreli hız buff'ı çarpanı (Hız İksiri 1.2)
    float buffDuration;          // hız buff süresi (sn)
    WeaponFxType coatFx;         // silah kaplaması efekti (Zehir Yağı → POISON)
    float coatDuration;          // kaplama süresi (sn)

    // --- Kullanılabilir parşömenler (envanterde sağ tık) ---
    int scrollSkill;             // öğretici parşömen: hangi skill (scrollXp > 0 ise geçerli)
    float scrollXp;              // kullanınca verilen XP (0 = parşömen değil)

    // --- Büyü tomarları (Büyü dalgası 2): sağ tık = oku → büyü KALICI açılır ---
    int tomeAbility;             // öğretilen AbilityId (0 = tomar değil)
    int tomeReqLevel;            // okumak için gereken Destruction seviyesi

    // --- Çanta sekmeleri: hammadde MALZEME sekmesine düşer (otomatik ayıklama) ---
    bool material;               // cevher/ot/odun/jel/kemik/öz/külçe/deri/çiğ et
    int bagUpgrade;              // >0: çanta eşyası — üretilince OTOMATİK takılır
                                 // (1 Deri Çanta / 2 Gezgin / 3 Malzeme Kesesi / 4 Usta)

    // --- Yemek (Faz 6 — av): iksirden farklı, YAVAŞ ama toplamı yüksek yenilenme ---
    float foodRegen;             // saniyede yenilenen can (>0 = yemek)
    float foodDuration;          // yenilenme süresi (sn)

    // --- Buff yemekleri (M2): tek "Tok" buffu — yenisi eskisini ezer ---
    int   foodBuffType;          // FoodBuff (0 = yok)
    float foodBuffPower;         // türe göre anlam (çarpan ya da oran)
    float foodBuffDur;           // buff süresi (sn)
    float rawRisk;               // çiğ yeme zehir riski 0..1 (çiğ et 0.25)

    bool trapItem;               // Yapışkan Tuzağı: hotbar kullanımı yere tuzak kurar (Faz 7)

    float ammoDmgMult;           // ok kademesi hasar çarpanı (>0 = mühimmat; Faz 8)
    float spellPower;            // silahın büyü gücü katkısı (asa 0.20 = +%20; Faz 22)

    const char *iconSpritePath;   // ikon dosyasi (NULL → tool_icons fallback)
    SDL_FRect   iconSrc;          // o dosyadaki kare {x, y, w, h}
} ItemType;

// Stat basma (Faz 4 — sunak): silah/zırh instance'ına en çok 2 stat, kademe I-III
typedef enum {
    STAT_STR = 0,   // hasar ölçeği
    STAT_DEX,       // kritik şansı
    STAT_HP,        // max can
    STAT_DEF,       // defans
    STAT_MP,        // max mana
    STAT_STA,       // max stamina
    STAT_INT,       // büyü hasar ölçeği (Faz 22 — büyücünün STR'ı)
    STAT_COUNT,
} StatType;

// Kademe başına kazanç (StatType ile paralel): STR/DEX +2, HP +10, DEF +3, MP +8, STA +10
extern const int STAT_TIER_VALUE[STAT_COUNT];
extern const char *const STAT_NAMES[STAT_COUNT];
#define STAT_MAX_TIER 3

// Bir somut item (kasada/envanterde) = tür + miktar + instance roll'ları.
// plusLevel/fx/stat'lar TÜRDE değil BURADA: aynı türden iki kılıç farklı olabilir.
typedef struct {
    const ItemType *type;   // NULL = boş slot
    int count;
    int plusLevel;          // +0..+8 (drop anında roll'lanır; stack'lenenler hep +0)
    Sint8 fx;               // rün efekti (WeaponFxType; türün varsayılanıyla doğar)
    Sint8 statType[2];      // stat slotları (-1 = boş; StatType)
    Sint8 statTier[2];      // kademe 1-3
} ItemStack;

// Doğru başlatılmış instance üret (fx = türün efekti, stat slotları boş).
// (ItemStack){t,c,p} YAZMA — fx sıfırlanır, rün/efekt kaybolur.
ItemStack item_make(const ItemType *type, int count, int plusLevel);

// Buff yemekleri (M2) — foodBuffType değerleri
typedef enum {
    FB_NONE = 0,
    FB_DMG,        // tüm hasar +power (0.08 = +%8) — Avcı Yahnisi
    FB_MANA_REGEN, // mana rejeni ×power — Ot Çorbası
    FB_STA_SPEED,  // stamina rejeni ×power + hız +%5 — Gezgin Kavurması
} FoodBuff;

#define ITEM_MAX_PLUS 8
#define ITEM_PLUS_SCALE 0.10f   // her plus: +%10 hasar/defans

// Instance değerleri: tür tabanı × plus ölçeği (combat bunları kullanır)
float item_damage(const ItemStack *s);    // silah değilse 0
float item_defense(const ItemStack *s);   // zırh/kalkan değilse 0
int   item_value(const ItemStack *s);     // instance fiyatı (plus dahil)

// Loot tablosu girdisi: mob ölünce her girdi için ayrı zar atılır
typedef struct {
    const ItemType *type;
    float chance;     // 0..1 düşme şansı (girdiler bağımsız)
    int maxPlus;      // plus roll üst sınırı (0 = hep +0)
} LootEntry;

// ITEM_TYPES dizin sabitleri — loot tabloları sihirli sayı kullanmasın.
// item.c'deki dizi sırasıyla AYNI tutulmalı.
typedef enum {
    ITEM_BOW = 0,
    ITEM_PICKAXE,
    ITEM_TORCH,
    ITEM_LONG_SWORD,
    ITEM_HELMET_IRON,
    ITEM_CHEST_IRON,
    ITEM_LEGS_IRON,
    ITEM_POTION_HP,
    // Meslek paketi (Faz 1): araç + toplama malzemeleri
    ITEM_AXE,           // Balta (oduculuk aracı)
    ITEM_ORE_COPPER,    // Bakır Cevheri
    ITEM_ORE_IRON,      // Demir Cevheri
    ITEM_HERB_RED,      // Kızılot (ova bitkisi)
    ITEM_HERB_BLUE,     // Ayotu (bataklık bitkisi)
    ITEM_WOOD,          // Odun
    // Meslek paketi (Faz 2): craft araç/ara ürün/çıktılar
    ITEM_HAMMER,        // Çekiç (demircilik aracı — Demirci görev ödülü)
    ITEM_INGOT_COPPER,  // Bakır Külçe
    ITEM_INGOT_IRON,    // Demir Külçe
    ITEM_SWORD_COPPER,  // Bakır Kılıç (ilk craft silahı)
    ITEM_POTION_BIG,    // Büyük Can İksiri
    // Meslek paketi (Faz 3): blueprint ekonomisi + tüketilebilir buff'lar
    ITEM_SWORD_IRON,    // Demir Kılıç (craft > vendor — döngü ödülü)
    ITEM_POTION_SPEED,  // Hız İksiri (30 sn +%20 hız)
    ITEM_POISON_OIL,    // Zehir Yağı (60 sn silaha zehir efekti — rün öncüsü)
    ITEM_BP_SWORD_IRON,   // Blueprint: Demir Kılıç (ork dropu)
    ITEM_BP_CHEST_IRON,   // Blueprint: Demir Zırh (Demirci'den satın alınır)
    ITEM_BP_HELMET_IRON,  // Blueprint: Demir Miğfer (goblin dropu)
    ITEM_BP_POTION_SPEED, // Blueprint: Hız İksiri (Baran'da satılık)
    ITEM_BP_POISON_OIL,   // Blueprint: Zehir Yağı (ork dropu)
    ITEM_SCROLL_MINING,   // Öğretici parşömen: +250 Mining XP (goblin)
    ITEM_SCROLL_HERBALISM,// +250 Herbalism XP (goblin)
    ITEM_SCROLL_SMITHING, // +250 Smithing XP (ork)
    ITEM_SCROLL_ONEHAND,  // +250 One-Handed XP (ork)
    // Meslek paketi (Faz 4): sunak — özler, rünler, stat taşları
    ITEM_ESSENCE_PALE,    // Soluk Öz (goblin) → Pembe Rün hammaddesi
    ITEM_ESSENCE_TOXIC,   // Zehirli Öz (ork) → Zehir Rünü hammaddesi
    ITEM_RUNE_PINK,       // Pembe Rün: silaha basılır, +%8 hasar + enerji efekti
    ITEM_RUNE_POISON,     // Zehir Rünü: vuruşta 4sn DoT + zehir efekti
    ITEM_STONE_STR,       // Stat taşları (sunakta basılır; scroll+külçe maliyeti)
    ITEM_STONE_DEX,
    ITEM_STONE_HP,
    ITEM_STONE_DEF,
    ITEM_STONE_MP,
    ITEM_STONE_STA,
    // Meslek paketi (Faz 5): oduculuk ürünleri + okçuluk
    ITEM_ARROW,           // Ok (yay mühimmatı — atış başına 1, leşten ~%30 geri)
    // Av paketi (Faz 6): hayvan lootu + deri seti + yemek
    ITEM_LEATHER,         // Deri (hayvan dropu → hafif zırh hammaddesi)
    ITEM_MEAT,            // Et (çiğ — kazanda pişirilir ya da satılır)
    ITEM_MEAT_COOKED,     // Pişmiş Et (yemek: 12 sn +3 can/sn)
    ITEM_HELMET_LEATHER,  // Deri Başlık (ARMOR_LIGHT — perk ağacı nihayet işler)
    ITEM_CHEST_LEATHER,   // Deri Zırh
    ITEM_BOOTS_LEATHER,   // Deri Bot
    // Slime paketi (Faz 7): jel ekonomisi + alan yavaşlatma tuzağı
    ITEM_SLIME_GEL,       // Slime Jeli (Küçük slime dropu → simya hammaddesi)
    ITEM_STICKY_TRAP,     // Yapışkan Tuzağı (yere kurulur, tetiklenince alan yavaşı)
    // İskelet paketi (Faz 8): kemik ekonomisi + kadim parşömenler
    ITEM_BONE,            // Kemik (→ Kemik Oku tarifi)
    ITEM_SCROLL_ARCHERY,  // +250 Archery XP (Bowman — türün tek kaynağı)
    ITEM_SCROLL_ENCHANT,  // +250 Enchanting XP (Mage)
    // Ok kademeleri (Faz 8): standart en ucuz, kemik/demir pahalı ve sert
    ITEM_ARROW_BONE,      // Kemik Oku (×1.15 hasar)
    ITEM_ARROW_IRON,      // Demir Ok (×1.3 hasar, Smithing 10)
    // Büyü paketi (Faz 22)
    ITEM_STONE_INT,       // Bilgelik Taşı (sunak: INT basımı — büyü hasarı)
    ITEM_STAFF,           // Asa (iki el; zayıf melee + büyü gücü +%20)
    ITEM_POTION_MANA,     // Mana İksiri (+40 mana)
    ITEM_LOCKPICK,        // Maymuncuk (kilit minigame'i — kırılınca gider)
    ITEM_SWORD_LORD,      // Lord'un Kılıcı (boss benzersizi: can çalma)
    ITEM_STEW,            // Avcı Yahnisi (M2: 180sn hasar +%8)
    ITEM_SOUP,            // Ot Çorbası (M2: 180sn mana rejeni ×1.5)
    ITEM_KAVURMA,         // Gezgin Kavurması (M2: 180sn stamina ×1.6 + hız +%5)
    // Büyü dalgası 2: kayıp büyü tomarları (kervan/zindan — okuyunca öğrenilir)
    ITEM_TOME_CLAW,       // Tomar: Yıldırım Pençesi (Yıkım 15)
    ITEM_TOME_ICETACLE,   // Tomar: Buz Dikenleri (Yıkım 30)
    ITEM_TOME_TORNADO,    // Tomar: Hortum (Yıkım 40 — yalnız zindan USTA sandığı)
    // Çanta sekmeleri: İp (ara malzeme) + 4 çanta eşyası (kalıcı kademe)
    ITEM_ROPE,            // İp (2 Ot → 1 İp, kamp ateşi)
    ITEM_BAG_LEATHER,     // Deri Çanta: Eşya +10 (Smithing 15)
    ITEM_BAG_TRAVELER,    // Gezgin Çantası: Eşya +10 daha (Smithing 40)
    ITEM_POUCH_MATERIAL,  // Malzeme Kesesi: Malzeme +15 (Smithing 10)
    ITEM_POUCH_MASTER,    // Usta Kesesi: Malzeme +15 daha (Smithing 35)
    // Büyü dalgası 2, parça 2 — SONA eklendi (save item indeksleri kaymasın)
    ITEM_TOME_SNAKEBITE,  // Tomar: Yılan Isırığı (Yıkım 20 — kervan havuzu)
    ITEM_TOME_FIRELION,   // Tomar: Ateş Aslanı (Yıkım 50 — yalnız boss leşi)
    // Two-Handed hattı (Parça A)
    ITEM_SPEAR,           // Mızrak: uzun menzil + yavaş + delici + hat vuruşu
} ItemId;

extern const ItemType ITEM_TYPES[];
extern const int ITEM_TYPE_COUNT;

#endif