#ifndef INVENTORY_H
#define INVENTORY_H

#include "r2d.h"
#include <SDL3/SDL.h>           // R2D, R2DTexture için
#include <SDL3_ttf/SDL_ttf.h>   // TTF_Font için
#include <stdbool.h>
#include "item.h"
#include "item_textures.h"

#define INVENTORY_SIZE 20   // çanta slot sayısı

// Oyuncunun çantası — item slotları
typedef struct {
    ItemStack slots[INVENTORY_SIZE];   // boş slot: type = NULL
} Inventory;

void inventory_render(const Inventory *inv, R2D *renderer,
                      R2DTexture *itemIconTex, const ItemTextures *itemTex,
                      TTF_Font *font, float panelX, float panelY);

// Envantere item ekle (stack'lenebilirse mevcut yığına, yoksa boş slota).
// plusLevel instance'a aittir: yalnız AYNI plus'lı yığınlar birleşir.
// Başarılıysa true (yer varsa), çanta doluysa false.
// (item_make ile doğru başlatılmış taze instance üretir — fx türden kopyalanır)
bool inventory_add(Inventory *inv, const ItemType *type, int count, int plusLevel);

// Mevcut instance'ı OLDUĞU GİBİ ekle (fx/stat korunur — unequip/loot taşıma yolu)
bool inventory_add_stack(Inventory *inv, const ItemStack *s);

int inventory_slot_at(float x, float y, float panelX, float panelY);

// Bu türden 1 adet düş (hotbar'dan kullanım). Yoksa false.
bool inventory_consume(Inventory *inv, const ItemType *type);

// Bu türden toplam kaç adet var (tüm yığınlar; plus farkı gözetmez)
int inventory_count(const Inventory *inv, const ItemType *type);

// Bu türden N adet düş (craft/görev teslimi). Yetersizse HİÇBİRİNİ düşmez, false.
bool inventory_consume_n(Inventory *inv, const ItemType *type, int n);

#endif