#ifndef ITEM_H
#define ITEM_H
#include <SDL3/SDL.h>
#include "common.h"

#define MAX_ITEM_TYPES 32

// Item TÜRÜ — bir kez tanımlanır, tüm o item'ler paylaşır (MobType/TileType gibi)
typedef struct {
    const char *name;        // "Kılıç", "Balta", "Altın"
    int iconIndex;           // Tool_Icons sheet'inde kaçıncı ikon (0-9)
    int maxStack;            // kaç tane üst üste yığılır (silah 1, altın 99)
} ItemType;

// Bir somut item (kasada/envanterde) = tür + miktar
typedef struct {
    const ItemType *type;    // hangi item (NULL = boş)
    int count;               // kaç tane
} ItemStack;

extern const ItemType ITEM_TYPES[];
extern const int ITEM_TYPE_COUNT;

#endif