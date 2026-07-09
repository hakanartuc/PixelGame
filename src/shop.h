#ifndef SHOP_H
#define SHOP_H
#include "r2d.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "item.h"
#include "item_textures.h"

// Vendor dükkanı. Stok sabittir (her vendor aynı — çeşitlenince NpcDef'e
// stok referansı taşınır). Alış = value; satış = value/2 (main işler:
// dükkan açıkken envanterde sağ tık = sat).

typedef struct {
    const ItemType *type;
} ShopEntry;

extern const ShopEntry SHOP_STOCK[];
extern const int SHOP_STOCK_COUNT;

// Dükkan paneli: stok listesi (ikon + ad + fiyat) + oyuncu altını
void shop_render(R2D *renderer, TTF_Font *font,
                 R2DTexture *itemIconTex, const ItemTextures *itemTex,
                 int playerGold, float panelX, float panelY);

// Tıklanan stok satırı (-1 = yok). shop_render ile aynı yerleşim.
int shop_entry_at(float x, float y, float panelX, float panelY);

#endif
