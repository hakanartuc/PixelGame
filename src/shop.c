#include <stdio.h>
#include "shop.h"
#include "font.h"
#include "ui_theme.h"

// Satır yerleşimi — çizim ve tıklama aynı ölçüleri kullanır (desenkron olmaz)
#define SHOP_ROW_H 44.0f
#define SHOP_PAD 16.0f
#define SHOP_W 300.0f

const ShopEntry SHOP_STOCK[] = {
    { &ITEM_TYPES[ITEM_POTION_HP] },
    { &ITEM_TYPES[ITEM_POTION_MANA] },
    { &ITEM_TYPES[ITEM_TORCH] },       // gece hazirligi: envanterdeyken isik verir
    { &ITEM_TYPES[ITEM_LOCKPICK] },    // zindan hazirligi: kilitli sandiklar
    { &ITEM_TYPES[ITEM_PICKAXE] },     // madencilik araci
    { &ITEM_TYPES[ITEM_AXE] },         // oduculuk araci
    { &ITEM_TYPES[ITEM_BP_POTION_SPEED] },  // Hiz Iksiri tarifi (altin gideri)
    { &ITEM_TYPES[ITEM_STONE_HP] },    // stat tasi: vendor'da nadir/pahali giris kademesi
    { &ITEM_TYPES[ITEM_BOW] },         // yay: craft (~90g maliyet) ya da hazir 180g
    { &ITEM_TYPES[ITEM_STAFF] },       // asa: tam-buyucu silahi (buyu +%20)
    { &ITEM_TYPES[ITEM_ARROW] },       // ok: tik basina 1 (craft 10'lu, ~yari fiyat)
    { &ITEM_TYPES[ITEM_LONG_SWORD] },
    { &ITEM_TYPES[ITEM_HELMET_IRON] },
    { &ITEM_TYPES[ITEM_CHEST_IRON] },
    { &ITEM_TYPES[ITEM_LEGS_IRON] },
};
const int SHOP_STOCK_COUNT = (int)(sizeof(SHOP_STOCK) / sizeof(SHOP_STOCK[0]));

// Kervan özel stoğu (Dünya Olayı — WE_CARAVAN): köyde bulunmayan nadir mallar.
// Fiyat çarpanı (%15 pahalı) çağrı yerinde buyMult'a eklenir — kervancı pazarlıksız.
const ShopEntry CARAVAN_STOCK[] = {
    { &ITEM_TYPES[ITEM_STONE_INT] },      // Bilgelik Taşı — büyücüler için tek alternatif kaynak
    { &ITEM_TYPES[ITEM_STONE_STR] },
    { &ITEM_TYPES[ITEM_ESSENCE_PALE] },   // rün hammaddesi — avlanmadan alma yolu
    { &ITEM_TYPES[ITEM_ESSENCE_TOXIC] },
    { &ITEM_TYPES[ITEM_ARROW] },
    { &ITEM_TYPES[ITEM_POTION_MANA] },
    // Tomarlar SABİT DEĞİL: her kervanda havuzdan 2 rastgele tomar bulunur —
    // aktif stok main'de kurulur (CARAVAN_STOCK + 2 tomar; startDay tohumlu)
};
const int CARAVAN_STOCK_COUNT = (int)(sizeof(CARAVAN_STOCK) / sizeof(CARAVAN_STOCK[0]));

void shop_render(R2D *renderer, TTF_Font *font,
                 R2DTexture *itemIconTex, const ItemTextures *itemTex,
                 int playerGold, float buyMult, float panelX, float panelY,
                 const ShopEntry *stock, int stockCount)
{
    if (stock == NULL) { stock = SHOP_STOCK; stockCount = SHOP_STOCK_COUNT; }
    float panelH = stockCount * SHOP_ROW_H + SHOP_PAD * 2 + 56.0f;

    ui_panel(renderer, panelX, panelY, SHOP_W, panelH);

    if (font == NULL) return;

    SDL_Color white  = { 255, 255, 255, 255 };
    SDL_Color gold   = { 255, 210, 60, 255 };
    SDL_Color dim    = { 150, 150, 160, 255 };

    draw_text(renderer, font, "Dükkan", panelX + SHOP_PAD, panelY + 8.0f, white);

    float rowY = panelY + SHOP_PAD + 30.0f;
    for (int i = 0; i < stockCount; i++) {
        const ItemType *it = stock[i].type;

        // Satır zemini (tema hücresi)
        SDL_FRect row = { panelX + 8.0f, rowY, SHOP_W - 16.0f, SHOP_ROW_H - 6.0f };
        ui_cell_gray(renderer, row.x, row.y, row.w, row.h);

        // İkon (envanterle aynı fallback zinciri)
        SDL_FRect dst = { row.x + 4.0f, rowY + 3.0f, 32.0f, 32.0f };
        R2DTexture *icon = item_icon_tex(itemTex, it);
        if (icon != NULL) {
            r2d_draw_texture(renderer, icon, &it->iconSrc, &dst);
        } else {
            SDL_FRect src = { it->iconIndex * 16.0f, 0, 16, 16 };
            r2d_draw_texture(renderer, itemIconTex, &src, &dst);
        }

        // Ad + fiyat (itibar çarpanlı; altın yetmiyorsa soluk)
        draw_text(renderer, font, it->name, row.x + 44.0f, rowY + 4.0f, white);
        int shown = (int)(it->value * buyMult);
        char price[16];
        snprintf(price, sizeof(price), "%d g", shown);
        draw_text(renderer, font, price, row.x + 44.0f, rowY + 20.0f,
                  (playerGold >= shown) ? gold : dim);

        rowY += SHOP_ROW_H;
    }

    // Alt bilgi: satış ipucu
    SDL_Color hint = { 170, 200, 255, 255 };
    draw_text(renderer, font, "Sat: envanterde sağ tık", panelX + SHOP_PAD, rowY + 4.0f, hint);
}

int shop_entry_at(float x, float y, float panelX, float panelY, int stockCount)
{
    if (stockCount <= 0) stockCount = SHOP_STOCK_COUNT;
    float rowY = panelY + SHOP_PAD + 30.0f;
    for (int i = 0; i < stockCount; i++) {
        SDL_FRect row = { panelX + 8.0f, rowY, SHOP_W - 16.0f, SHOP_ROW_H - 6.0f };
        if (x >= row.x && x < row.x + row.w && y >= row.y && y < row.y + row.h)
            return i;
        rowY += SHOP_ROW_H;
    }
    return -1;
}
