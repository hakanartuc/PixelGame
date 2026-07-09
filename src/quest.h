#ifndef QUEST_H
#define QUEST_H
#include <stdbool.h>
#include "mob.h"

// Görev çekirdeği. Şimdilik tek tür: KILL (N tane X öldür).
// Tanımlar (QuestDef) sabittir; oyuncunun ilerlemesi QuestStatus'ta yaşar
// (main tutar, save v3 kaydeder). Diyalog dosyaları görevlere id ile bağlanır
// (accept:goblin_derdi gibi) — bkz. dialog.h eylem hedefleri.

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
    const MobType *targetMob;  // hangi tür öldürülecek
    int targetCount;
    int rewardGold;
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

#endif
