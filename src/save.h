#ifndef SAVE_H
#define SAVE_H
#include <stdbool.h>
#include "player.h"
#include "inventory.h"
#include "equipment.h"

// Basit binary save/load — v1 kapsamı: oyuncu (konum/can/stat/skill/gold),
// envanter ve ekipman. Dünya durumu (mob/loot) KAYDEDİLMEZ; yükleyince
// dünya mevcut haliyle kalır (bölge sistemi gelince dünya da eklenecek).
//
// Format: "PIXE" + u32 versiyon + alan alan yazım (struct dump DEĞİL —
// struct'lara alan eklemek eski save'i bozmasın diye her alan tek tek).
// ItemType diske pointer olarak değil ITEM_TYPES indeksi olarak yazılır.

#define SAVE_PATH "save.dat"

// Başarılıysa true. Hata SDL_Log'a yazılır, oyun çökmez.
bool save_game(const char *path, const Player *p,
               const Inventory *inv, const Equipment *eq);

// Başarılıysa true (dosya yoksa/bozuksa false — mevcut durum korunur).
bool load_game(const char *path, Player *p,
               Inventory *inv, Equipment *eq);

#endif
