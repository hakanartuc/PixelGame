#include "item.h"

const ItemType ITEM_TYPES[] = {
    { .name = "Yay",     .iconIndex = 0, .maxStack = 1 },
    { .name = "Çekiç",   .iconIndex = 1, .maxStack = 1 },
    { .name = "Kazma",   .iconIndex = 2, .maxStack = 1 },
    { .name = "Balta",   .iconIndex = 3, .maxStack = 1 },
    { .name = "Kılıc",   .iconIndex = 4, .maxStack = 1 },
    { .name = "Asa",     .iconIndex = 5, .maxStack = 1 },
    { .name = "Meşale",  .iconIndex = 9, .maxStack = 99 },
};

const int ITEM_TYPE_COUNT = (int)(sizeof(ITEM_TYPES) / sizeof(ITEM_TYPES[0]));