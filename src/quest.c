#include <stdio.h>
#include <string.h>
#include "quest.h"

// Görev tanımları — içerik büyüdükçe buraya satır eklenir (ITEM_TYPES deseni)
const QuestDef QUESTS[] = {
    { .id = "goblin_derdi", .name = "Goblin Derdi",
      .targetMob = &MOB_TYPES[0], .targetCount = 5, .rewardGold = 100,
      .journal = "Kemal'e söz verdim: ovadaki goblinlerden beşini temizleyeceğim. "
                 "Yaşlı adam sürüleri için korkuyor — gözlerinden belli." },

    // Meslek tanışma görevleri (FETCH) — teslim = istasyon İZNİ (REWARDED kontrolü)
    { .id = "demirci_ocak", .name = "Demircinin Sınavı",
      .targetMob = NULL, .targetCount = 5, .rewardGold = 0,
      .fetchItem = ITEM_ORE_COPPER, .fetchCount = 5, .rewardItem = ITEM_HAMMER,
      .journal = "Mahir beni sınıyor: beş bakır cevheri. Ocağını yabancıya "
                 "emanet etmeden önce ellerime bakmak istiyor." },
    { .id = "simya_masa", .name = "Baran'ın Kazanı",
      .targetMob = NULL, .targetCount = 3, .rewardGold = 15,
      .fetchItem = ITEM_HERB_RED, .fetchCount = 3, .rewardItem = -1,
      .journal = "Baran kazanı için üç kızılot istedi. Tüccar dediğin önce "
                 "seni tartar — otlar bahane sanki." },
};
const int QUEST_COUNT = (int)(sizeof(QUESTS) / sizeof(QUESTS[0]));

int quest_find(const char *id)
{
    for (int i = 0; i < QUEST_COUNT; i++)
        if (strcmp(QUESTS[i].id, id) == 0)
            return i;
    return -1;
}

int quest_on_kill(QuestStatus *status, const MobType *killed)
{
    int completed = -1;
    for (int i = 0; i < QUEST_COUNT; i++) {
        if (status[i].state != QUEST_ACTIVE) continue;
        if (QUESTS[i].targetMob != killed) continue;

        status[i].progress++;
        if (status[i].progress >= QUESTS[i].targetCount) {
            status[i].state = QUEST_DONE;
            completed = i;
        }
    }
    return completed;
}
