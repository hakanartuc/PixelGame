#ifndef INVENTORY_H
#define INVENTORY_H
#include <stdbool.h>
#include "item.h"

#define INVENTORY_SIZE 20   // çanta slot sayısı

// Oyuncunun çantası — item slotları
typedef struct {
    ItemStack slots[INVENTORY_SIZE];   // boş slot: type = NULL
} Inventory;

// Envantere item ekle (stack'lenebilirse mevcut yığına, yoksa boş slota).
// Başarılıysa true (yer varsa), çanta doluysa false.
bool inventory_add(Inventory *inv, const ItemType *type, int count);

#endif