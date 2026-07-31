#ifndef GUARD_H
#define GUARD_H
#include <stdbool.h>

// Muhafız beyni: bounty varsa yaklaşır → yakalama paneli (öde / karşı koy).
// Karşı koyan oyuncuyu kovalar; nöbet noktasından GUARD_LEASH'ten uzağa gitmez
// (köyü terk ederek kaçmak mümkün — Gothic tarzı). Ölürse ertesi sabah dirilir.

#define GUARD_SIGHT   500.0f   // bounty sahibi oyuncuyu bu mesafede fark eder
#define GUARD_LEASH   800.0f   // nöbet noktasından max uzaklık (kaçış sınırı)
#define GUARD_CATCH   70.0f    // yakalama paneli mesafesi
#define GUARD_ATK_RANGE 55.0f
#define GUARD_ATK_DMG  14.0f   // ham hasar (oyuncu defansı combat_mob_damage'da düşer)
#define GUARD_ATK_INTERVAL 1.3f
#define GUARD_SPEED   150.0f

// Bir güncelleme adımının main'e söyledikleri
typedef struct {
    bool wantCatch;       // yakalama menzilinde — panel açılmalı
    float attackDamage;   // >0: bu karede oyuncuya vuruş (ham, defanssız)
} GuardTick;

void guard_init(int npcIndex);   // hangi NPC muhafız (main yükler)

// uiFrozen: yakalama paneli açıkken muhafız yürümez/vurmaz.
GuardTick guard_update(float dt, float px, float py, int bounty, bool uiFrozen);

void guard_set_hostile(bool hostile);   // karşı koyma / oyuncunun saldırısı
bool guard_is_hostile(void);
// TEK MUHAFIZ — bilinçli olarak öyle bırakıldı (2026-07-31).
// DEMİRBOYUN kanonu altı şehir getiriyor ve bu sistem çoğullanacak, ama YAPIYI
// kanon kadrosu belirleyecek: şehir başına kaç muhafız, kime bağlı (Ergün'e mi
// valiye mi), Ak Nöbet ayrı bir örgüt mü. Şimdi guards[N] dizisine çevirmek
// hiyerarşiyi TAHMİNLE kurmak olurdu ve kadro gelince yeniden yazılırdı.
// Sıra: lore → GDD → yapı → hikaye → görev → kod. Bkz. GDD §8.1 komşusu.
int  guard_index(void);                 // muhafızın NPC indeksi (-1 = yok)

#endif
