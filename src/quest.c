#include <string.h>
#include "quest.h"

// Görev tanımları — içerik büyüdükçe buraya satır eklenir (ITEM_TYPES deseni)
const QuestDef QUESTS[] = {
    { .id = "goblin_derdi", .name = "Goblin Derdi",
      .targetMob = &MOB_TYPES[0], .targetCount = 5, .rewardGold = 100 },
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
