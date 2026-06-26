#ifndef ITEM_TEXTURES_H
#define ITEM_TEXTURES_H
#include <SDL3/SDL.h>
#include "item.h"   // ItemType, MAX_ITEM_TYPES, ITEM_TYPE_COUNT

// Item gorsellerinin TEK sahibi: paper-doll katmani + envanter ikonu.
// ITEM_TYPES (veri) ile paralel; idx = type - ITEM_TYPES koprusuyle erisilir.
typedef struct {
    SDL_Texture *equipTex[MAX_ITEM_TYPES];   // paper-doll katmani (equipSpritePath)
    SDL_Texture *iconTex[MAX_ITEM_TYPES];    // envanter ikonu (iconSpritePath)
} ItemTextures;

void item_textures_load(ItemTextures *t, SDL_Renderer *r);
void item_textures_free(ItemTextures *t);

// Kopru: item turu → texture. type NULL / yuklenmemis ise NULL.
SDL_Texture *item_equip_tex(const ItemTextures *t, const ItemType *type);
SDL_Texture *item_icon_tex(const ItemTextures *t, const ItemType *type);

#endif