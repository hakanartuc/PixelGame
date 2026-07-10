#include <SDL3/SDL.h>   // inventory.c'ye ekle (şu an sadece inventory.h var)
#include "inventory.h"
#include "font.h"
#include "item_textures.h"
#include "ui_theme.h"

void inventory_render(const Inventory *inv, R2D *renderer,
                      R2DTexture *itemIconTex, const ItemTextures *itemTex,
                      TTF_Font *font, float panelX, float panelY)
{
    int cols = INV_COLS;
    int rows = (INVENTORY_SIZE + cols - 1) / cols;   // 35/7 = 5 satır
    float cellSize = 56.0f;
    float pad = 16.0f;

    float panelW = cols * cellSize + pad * 2;
    float panelH = rows * cellSize + pad * 2 + 30.0f;   // +30 başlık için

    // Panel arka planı (tema: ahşap çerçeve)
    ui_panel(renderer, panelX, panelY, panelW, panelH);

    // Başlık
    if (font != NULL) {
        SDL_Color white = { 255, 255, 255, 255 };
        draw_text(renderer, font, "Envanter", panelX + pad, panelY + 8.0f, white);
    }

    // Slotlar (hepsi — dolu + boş)
    float gridY = panelY + pad + 30.0f;   // başlığın altından başla
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        int col = i % cols, row = i / cols;
        float cx = panelX + pad + col * cellSize;
        float cy = gridY + row * cellSize;

        // Hücre arka planı (her slot — boş da olsa; dolu slot vurgulu ten)
        if (inv->slots[i].type != NULL)
            ui_cell(renderer, cx, cy, cellSize - 6, cellSize - 6);
        else
            ui_cell_gray(renderer, cx, cy, cellSize - 6, cellSize - 6);

        // Dolu slotsa ikon ciz
        if (inv->slots[i].type != NULL) {
            const ItemType *it = inv->slots[i].type;
            SDL_FRect dst = { cx + 8, cy + 8, 32, 32 };

            R2DTexture *icon = item_icon_tex(itemTex, it);
            if (icon != NULL) {
                // item kendi sheet'inden (LPC izole parca = temiz ikon)
                r2d_draw_texture(renderer, icon, &it->iconSrc, &dst);
            } else {
                // fallback: tool_icons + iconIndex
                SDL_FRect src = { it->iconIndex * 16.0f, 0, 16, 16 };
                r2d_draw_texture(renderer, itemIconTex, &src, &dst);
            }

            // Miktar (stack > 1 ise) — AYNEN kalir
            if (inv->slots[i].count > 1 && font != NULL) {
                char buf[16];
                snprintf(buf, sizeof(buf), "%d", inv->slots[i].count);
                SDL_Color yellow = { 255, 230, 80, 255 };
                draw_text(renderer, font, buf, cx + 8, cy + cellSize - 24, yellow);
            }

            // Plus seviyesi (+1 ve üstü) — sağ üst köşe, camgöbeği
            if (inv->slots[i].plusLevel > 0 && font != NULL) {
                char buf[8];
                snprintf(buf, sizeof(buf), "+%d", inv->slots[i].plusLevel);
                SDL_Color cyan = { 90, 220, 255, 255 };
                draw_text(renderer, font, buf, cx + cellSize - 32, cy + 2, cyan);
            }
        }
    }
}

bool inventory_add(Inventory *inv, const ItemType *type, int count, int plusLevel)
{
    if (type == NULL) return false;
    ItemStack s = item_make(type, count, plusLevel);
    return inventory_add_stack(inv, &s);
}

bool inventory_add_stack(Inventory *inv, const ItemStack *s)
{
    if (s == NULL || s->type == NULL) return false;

    // 1) Stack'lenebilen item zaten varsa, ona ekle. Yığın şartı: aynı plus +
    //    aynı fx + her iki taraf da stat'sız (+3, rünlü ya da stat'lı olan
    //    AYRI item'dir, yığına girmez)
    bool sPlain = (s->statType[0] < 0 && s->statType[1] < 0);
    if (s->type->maxStack > 1 && sPlain) {
        for (int i = 0; i < INVENTORY_SIZE; i++) {
            ItemStack *t = &inv->slots[i];
            if (t->type == s->type &&
                t->plusLevel == s->plusLevel &&
                t->fx == s->fx &&
                t->statType[0] < 0 && t->statType[1] < 0 &&
                t->count < s->type->maxStack) {
                t->count += s->count;
                // (Basit: maxStack taşmasını şimdilik kontrol etmiyoruz —
                //  ileride taşan kısmı yeni slota koyarız)
                return true;
            }
        }
    }

    // 2) Boş slot bul — instance OLDUĞU GİBİ taşınır (fx/stat kaybolmaz)
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (inv->slots[i].type == NULL) {
            inv->slots[i] = *s;
            return true;
        }
    }

    return false;   // çanta dolu
}

bool inventory_consume(Inventory *inv, const ItemType *type)
{
    if (type == NULL) return false;
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (inv->slots[i].type != type) continue;
        inv->slots[i].count--;
        if (inv->slots[i].count <= 0) {
            inv->slots[i].type = NULL;
            inv->slots[i].plusLevel = 0;
        }
        return true;
    }
    return false;   // envanterde yok
}

int inventory_count(const Inventory *inv, const ItemType *type)
{
    if (type == NULL) return 0;
    int total = 0;
    for (int i = 0; i < INVENTORY_SIZE; i++)
        if (inv->slots[i].type == type)
            total += inv->slots[i].count;
    return total;
}

bool inventory_consume_n(Inventory *inv, const ItemType *type, int n)
{
    if (inventory_count(inv, type) < n)
        return false;   // yetersiz — hiçbirine dokunma
    while (n-- > 0)
        inventory_consume(inv, type);
    return true;
}

// Ekran koordinatindan hangi envanter slotuna denk geldigini bulur.
// panelX/panelY: inventory_render'a verilen panel sol-ust (AYNI degerler).
// Donen: slot indeksi (0..INVENTORY_SIZE-1) ya da -1 (bos alana tiklandi).
int inventory_slot_at(float x, float y, float panelX, float panelY)
{
    // inventory_render'daki AYNI matematik (tek kaynak — desenkronize olmasin)
    int cols = INV_COLS;
    float cellSize = 56.0f;
    float pad = 16.0f;
    float gridY = panelY + pad + 30.0f;   // baslik altindan basliyor (render ile ayni)

    for (int i = 0; i < INVENTORY_SIZE; i++) {
        int col = i % cols, row = i / cols;
        float cx = panelX + pad + col * cellSize;
        float cy = gridY + row * cellSize;
        // hucrenin tiklanabilir alani: cellSize-6 (render'daki cell ile ayni)
        SDL_FRect cell = { cx, cy, cellSize - 6, cellSize - 6 };
        if (x >= cell.x && x < cell.x + cell.w &&
            y >= cell.y && y < cell.y + cell.h) {
            return i;
        }
    }
    return -1;
}