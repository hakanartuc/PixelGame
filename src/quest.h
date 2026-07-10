#ifndef QUEST_H
#define QUEST_H
#include "r2d.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "mob.h"

// Görev çekirdeği. İki tür:
//   KILL  — targetMob != NULL: N tane X öldür (quest_on_kill sayar)
//   FETCH — fetchItem >= 0: N adet malzeme getir (teslimde envanterden düşülür;
//           "yeterli var mı" kontrolü E-konuşma girişinde — main)
// Tanımlar (QuestDef) sabittir; oyuncunun ilerlemesi QuestStatus'ta yaşar
// (main tutar, save v3 kaydeder). Diyalog dosyaları görevlere id ile bağlanır
// (accept:goblin_derdi gibi) — bkz. dialog.h eylem hedefleri.
// rewardItem: altına ek item ödülü (-1 = yok) — örn. Demirci görevi → Çekiç.
// İstasyon İZNİ = ilgili görevin REWARDED olması (ayrı kayıt gerekmez).

#define MAX_QUESTS 16   // durum dizileri için derleme-zamanı tavan (QUESTS <= bu)

typedef enum {
    QUEST_NOT_STARTED = 0,
    QUEST_ACTIVE,       // alındı, sayaç işliyor
    QUEST_DONE,         // hedef tamam — teslim bekliyor
    QUEST_REWARDED,     // ödül verildi, kapandı
} QuestState;

typedef struct {
    const char *id;            // diyalogdan referans ("goblin_derdi")
    const char *name;          // UI'da gösterilen ad
    const MobType *targetMob;  // KILL: hangi tür öldürülecek (FETCH'te NULL)
    int targetCount;           // öldürme ya da getirme adedi
    int rewardGold;
    int fetchItem;             // FETCH: getirilecek ItemId (KILL'de 0 kalır — dizilimde
                               //        targetMob NULL + fetchCount > 0 belirleyicidir)
    int fetchCount;            // FETCH adedi (0 = KILL görevi)
    int rewardItem;            // ek item ödülü (ItemId; 0/negatif = yok — ITEM_BOW=0
                               //        ödül olamaz, sorun değil)
} QuestDef;

// Oyuncunun bir görevdeki durumu (QUESTS ile paralel dizi)
typedef struct {
    QuestState state;
    int progress;
} QuestStatus;

extern const QuestDef QUESTS[];
extern const int QUEST_COUNT;

// id → QUESTS indeksi (-1 = yok)
int quest_find(const char *id);

// Öldürme kancası: aktif görevlerin sayaçlarını ilerletir.
// Bu ölümle DONE'a geçen görevin indeksi döner (-1 = yok; UI bildirimi için).
int quest_on_kill(QuestStatus *status, const MobType *killed);

// Quest log paneli (J): tüm görevler — durum, ilerleme çubuğu, ödül.
void quest_log_render(const QuestStatus *status, R2D *renderer,
                      TTF_Font *font, float panelX, float panelY);

#endif
