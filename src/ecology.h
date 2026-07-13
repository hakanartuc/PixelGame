#ifndef ECOLOGY_H
#define ECOLOGY_H
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "mob.h"

// MOB EKOLOJİSİ (E1) — KO slot spawn'ının yerini alır (yalnız DÜNYA haritası;
// zindan "mühür istisnası": eski giriş-başına tazelenme orada sürer).
//
// Model: her kampın bir YUVASI var — popülasyon oradan beslenir.
//   · Öldürülen mob GERİ GELMEZ; her şafak yuva +1 üretir (tavana kadar).
//   · Yuva yok edilirse bölge KALICI temizlenir; 5-15 gün sonra aynı ailenin
//     yeni sürüsü BOŞ bir alternatif sahaya göçer (kolonizasyon).
//   · Göz önünde spawn yok: doğum yuva noktasında olur.
// Yıkım biçimi aileye göre (kullanıcı kararı — tekdüzelik olmasın):
//   GOBLIN reis (öldür → kamp çöker, kalanlar kaçışır) · ORK savaş totemi
//   (HP'li yapı-mob) · SLIME gölcük kurutma (E, tüm slime'lar ölüyken) ·
//   İSKELET mezar mühürleme (E) · HAYVAN sürü (yuva yok; biterse göçer).

typedef enum {
    NEST_CHIEF = 0,   // reis mobu ölünce çöker (goblin)
    NEST_TOTEM,       // yapı-mob yıkılınca çöker (ork)
    NEST_POOL,        // ritüel: tüm moblar ölü + E (slime gölcüğü)
    NEST_GRAVE,       // ritüel: tüm moblar ölü + E (lanetli mezar)
    NEST_HERD,        // yuva yok: popülasyon biterse sürü göçer
} NestKind;

#define NEST_MAX_ENTRIES 3   // karışık kamplar (iskelet: savaşçı+okçu+büyücü)

typedef struct {
    NestKind kind;
    float x, y;              // yuva noktası (doğum + görsel + ritüel menzili)
    float radius;            // mobların gezinme/leash yarıçapı (zone'a gider)
    int family;              // göç eşleşmesi için aile etiketi (FAM_*)
    bool startColonized;     // taban saha mı (true) alternatif saha mı (false)
    struct { const MobType *type; int max; } entries[NEST_MAX_ENTRIES];
} NestSiteDef;

// Aile etiketleri (göç: yıkılan ailenin YENİ sahası bu etiketle aranır)
enum { FAM_GOBLIN = 0, FAM_ORC, FAM_SLIME_G, FAM_SLIME_B, FAM_SKELETON,
       FAM_SHEEP, FAM_PIG, FAM_CHICKEN, FAM_BOAR };

typedef struct {
    bool colonized;      // sahada yuva var
    bool destroyed;      // yıkıldı/mühürlendi/kurudu (kalıcı)
    bool chiefAlive;     // NEST_CHIEF/TOTEM: reis/totem hayatta mı
    Sint16 pop[NEST_MAX_ENTRIES];   // giriş başına canlı popülasyon
    Sint16 destroyedDay; // göç sayacı için
    Sint16 migrateDay;   // planlanan koloni günü (0 = plan yok)
} NestState;

extern const NestSiteDef NEST_SITES[];
extern const int NEST_SITE_COUNT;

void ecology_reset(void);   // yeni oyun: taban sahalar dolu, alternatifler boş

// Dünya haritası doldurma: zone tablosunu kurar + MEVCUT popülasyonu spawn eder
// (reis/totem dahil). mob dizisi temiz gelmeli. Dönen: zone sayısı.
int ecology_populate(SpawnZone *zones, int maxZones,
                     Mob *mobs, int maxMobs, int *nextMobId);

// Şafak: her canlı yuva +1 üretir (tavana kadar; mobs'a doğrudan spawn eder —
// yalnız dünya haritasındayken çağır) + planlı göçleri işler.
void ecology_dawn(int day, SpawnZone *zones, int zoneCount,
                  Mob *mobs, int maxMobs, int *nextMobId, bool onWorldMap);

// Mob ölümü: popülasyon düşer. isChief=reis/totem öldü → yuva çöker; dönen
// true = yuva bu ölümle ÇÖKTÜ (main kalanları kaçırtır + bildirim).
bool ecology_on_mob_death(int zoneId, bool isChief, int day);

// Ritüel yuvaları (POOL/GRAVE): menzilde, tüm mobları ölü, yıkılmamış saha
// indeksi (-1 yok). ecology_do_ritual onu kalıcı kapatır.
int  ecology_ritual_near(float px, float py, const Mob *mobs, int maxMobs);
void ecology_do_ritual(int site, int day);

// zoneId → saha indeksi (-1 = zindan/bilinmeyen zone)
int ecology_site_of_zone(int zoneId);
const NestSiteDef *ecology_site(int i);
NestState *ecology_state(int i);   // save/yükleme + render erişimi

#endif
