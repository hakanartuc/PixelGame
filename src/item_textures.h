#ifndef ITEM_TEXTURES_H
#define ITEM_TEXTURES_H
#include "r2d.h"
#include <SDL3/SDL.h>
#include "item.h"   // ItemType, MAX_ITEM_TYPES, ITEM_TYPE_COUNT

// Item gorsellerinin TEK sahibi: paper-doll katmani + envanter ikonu.
// ITEM_TYPES (veri) ile paralel; idx = type - ITEM_TYPES koprusuyle erisilir.
typedef struct {
    R2DTexture *equipTex[MAX_ITEM_TYPES];   // paper-doll katmani (equipSpritePath)
    R2DTexture *equipTexBack[MAX_ITEM_TYPES];  // paper-doll arka katman (silah)
    R2DTexture *iconTex[MAX_ITEM_TYPES];    // envanter ikonu (iconSpritePath)
} ItemTextures;

void item_textures_load(ItemTextures *t, R2D *r);
void item_textures_free(ItemTextures *t);

// Kopru: item turu → texture. type NULL / yuklenmemis ise NULL.
R2DTexture *item_equip_tex(const ItemTextures *t, const ItemType *type);
R2DTexture *item_icon_tex(const ItemTextures *t, const ItemType *type);
R2DTexture *item_equip_tex_back(const ItemTextures *t, const ItemType *type);

#endif