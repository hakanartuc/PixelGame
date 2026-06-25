#ifndef INVENTORY_H
#define INVENTORY_H

#include <SDL3/SDL.h>           // SDL_Renderer, SDL_Texture için
#include <SDL3_ttf/SDL_ttf.h>   // TTF_Font için
#include <stdbool.h>
#include "item.h"

#define INVENTORY_SIZE 20   // çanta slot sayısı

// Oyuncunun çantası — item slotları
typedef struct {
    ItemStack slots[INVENTORY_SIZE];   // boş slot: type = NULL
} Inventory;

void inventory_render(const Inventory *inv, SDL_Renderer *renderer, SDL_Texture *itemIconTex, TTF_Font *font, int winW, int winH);

// Envantere item ekle (stack'lenebilirse mevcut yığına, yoksa boş slota).
// Başarılıysa true (yer varsa), çanta doluysa false.
bool inventory_add(Inventory *inv, const ItemType *type, int count);

#endif