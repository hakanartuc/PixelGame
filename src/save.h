#ifndef SAVE_H
#define SAVE_H
#include <stdbool.h>
#include "player.h"
#include "inventory.h"
#include "equipment.h"
#include "hotbar.h"
#include "quest.h"

// Basit binary save/load — v1 kapsamı: oyuncu (konum/can/stat/skill/gold),
// envanter ve ekipman. Dünya durumu (mob/loot) KAYDEDİLMEZ; yükleyince
// dünya mevcut haliyle kalır (bölge sistemi gelince dünya da eklenecek).
//
// Format: "PIXE" + u32 versiyon + alan alan yazım (struct dump DEĞİL —
// struct'lara alan eklemek eski save'i bozmasın diye her alan tek tek).
// ItemType diske pointer olarak değil ITEM_TYPES indeksi olarak yazılır.

#define SAVE_PATH "save.dat"   // eski tek-dosya yolu (slot sistemi öncesi — artık kullanılmıyor)

// --- Slot sistemi (Faz B1): saves/slot_NN.dat + saves/auto.dat ---
#define SAVE_SLOT_COUNT 20   // manuel slotlar (1 PC'de birden çok oyuncu)
#define SAVE_SLOT_AUTO  SAVE_SLOT_COUNT   // otomatik kayıt slotu (yalnız yükleme)

// slot: 0..SAVE_SLOT_COUNT-1 manuel, SAVE_SLOT_AUTO otomatik.
// Dönen: statik tampon (bir sonraki çağrıda ezilir). saves/ klasörünü de kurar.
const char *save_slot_path(int slot);

// Slot kartı özeti (dosyanın başındaki özet bloğundan — tam yükleme yapmadan)
typedef struct {
    bool used;        // dosya var ve sürüm tutuyor
    int totalLevel;   // tüm skill seviyeleri toplamı
    int day;          // oyun günü
    int gold;
    int mapId;        // MapId (konum adı worldmap'ten)
} SaveSummary;
bool save_read_summary(const char *path, SaveSummary *out);

// Başarılıysa true. Hata SDL_Log'a yazılır, oyun çökmez.
bool save_game(const char *path, const Player *p,
               const Inventory *inv, const Equipment *eq, const Hotbar *hb,
               const QuestStatus *quests);

// Başarılıysa true (dosya yoksa/bozuksa false — mevcut durum korunur).
bool load_game(const char *path, Player *p,
               Inventory *inv, Equipment *eq, Hotbar *hb,
               QuestStatus *quests);

#endif
