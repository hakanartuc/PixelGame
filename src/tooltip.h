#ifndef TOOLTIP_H
#define TOOLTIP_H
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "r2d.h"
#include "item.h"

// Item tooltip'i (Cila Paketi): imlecin üstündeki hücrenin tam detayı —
// ad(+plus), tür, hasar/defans, rün/stat'lar, tüketim etkileri, değer.
// equipped != NULL ise AYNI SLOTTAKİ kuşanılıyla fark gösterilir
// (yeşil ▲ / kırmızı ▼ — alım/kuşanım kararı tek bakışta).
// İmleç konumuna göre ekran içinde kalacak şekilde yerleşir.
void tooltip_render(R2D *renderer, TTF_Font *font,
                    const ItemStack *s, const ItemStack *equipped,
                    float mx, float my, int winW, int winH);

#endif
