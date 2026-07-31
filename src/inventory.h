#ifndef INVENTORY_H
#define INVENTORY_H

#include "r2d.h"
#include <SDL3/SDL.h>           // R2D, R2DTexture için
#include <SDL3_ttf/SDL_ttf.h>   // TTF_Font için
#include <stdbool.h>
#include "item.h"
#include "item_textures.h"

#define INVENTORY_SIZE 60   // slot DİZİ tavanı (aktif slot sayısı = cap, craft'la büyür)
#define INV_COLS 10         // panel sütun sayısı (render + slot_at aynı kaynak)
#define INV_PANEL_W (INV_COLS * 56 + 32)   // panel genişliği (main çapaları bununla ortalar)
#define INV_EQUIP_H 340                    // ekipman bölümünün yüksekliği; çanta
                                           // ızgarası ve sekme şeridi bunun altında

// Envanter panelinin sol-üst köşesi — TEK KAYNAK.
//
// Neden fonksiyon: bu köşe çizimi, yuva tıklamasını, sürükle-bırak hedeflerini,
// sekme şeridini, çöp hücresini ve tooltip'i besliyor — formül main.c'de BEŞ
// yere kopyalanmıştı. Paneli veriden taşınabilir yapmak o kopyaları sessiz
// sapma noktasına çevirirdi: panel taşınır, yuvalar eski yerinde tıklanmaya
// devam ederdi ve hiçbir uyarı çıkmazdı.
//
// BOYUT YOK, bilinçli: panel genişliği INV_COLS × 56'dan, yüksekliği çanta
// KAPASİTESİNDEN çıkıyor (çanta yükseltmesiyle satır ekleniyor). Boyut burada
// bir tasarım kararı değil, içeriğin sonucu — veriye taşımak ya hiçbir şey
// yapmaz ya yuva ızgarasını panelden koparır.
void inventory_panel_origin(int winW, int winH, float *outX, float *outY);

// Çanta kademe boyutları (ÇANTA SEKMELERİ fazı): iki çanta ayrı büyür.
// Eşya:   30 → +10 (Deri Çanta) → +10 (Gezgin Çantası) = max 50
// Malzeme: 25 → +15 (Malzeme Kesesi) → +15 (Usta Kesesi) = max 55
#define INV_MAIN_BASE 30
#define INV_MAIN_STEP 10
#define INV_CRAFT_BASE 25
#define INV_CRAFT_STEP 15

// Oyuncunun çantası — item slotları (Eşya ve Malzeme sekmesi ayrı instance)
typedef struct {
    ItemStack slots[INVENTORY_SIZE];   // boş slot: type = NULL
    int cap;                           // aktif slot sayısı (çanta kademesi belirler)
} Inventory;

// title: sekme başlığı ("Eşya"/"Malzeme") — cap'e göre satırlar çizilir
void inventory_render(const Inventory *inv, R2D *renderer,
                      R2DTexture *itemIconTex, const ItemTextures *itemTex,
                      TTF_Font *font, float panelX, float panelY,
                      const char *title);

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