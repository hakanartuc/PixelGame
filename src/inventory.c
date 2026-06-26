#include <SDL3/SDL.h>   // inventory.c'ye ekle (şu an sadece inventory.h var)
#include "inventory.h"
#include "font.h"
#include "item_textures.h"

void inventory_render(const Inventory *inv, SDL_Renderer *renderer,
                      SDL_Texture *itemIconTex, const ItemTextures *itemTex,
                      TTF_Font *font, float panelX, float panelY)
{
    int cols = 5;
    int rows = (INVENTORY_SIZE + cols - 1) / cols;   // 20/5 = 4 satır
    float cellSize = 56.0f;
    float pad = 16.0f;

    float panelW = cols * cellSize + pad * 2;
    float panelH = rows * cellSize + pad * 2 + 30.0f;   // +30 başlık için

    // Panel arka planı
    SDL_SetRenderDrawColor(renderer, 25, 25, 35, 240);
    SDL_FRect panel = { panelX, panelY, panelW, panelH };
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 200, 200, 210, 255);
    SDL_RenderRect(renderer, &panel);

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

        // Hücre arka planı (her slot — boş da olsa)
        SDL_SetRenderDrawColor(renderer, 50, 50, 65, 255);
        SDL_FRect cell = { cx, cy, cellSize - 6, cellSize - 6 };
        SDL_RenderFillRect(renderer, &cell);
        SDL_SetRenderDrawColor(renderer, 90, 90, 105, 255);
        SDL_RenderRect(renderer, &cell);

        // Dolu slotsa ikon ciz
        if (inv->slots[i].type != NULL) {
            const ItemType *it = inv->slots[i].type;
            SDL_FRect dst = { cx + 8, cy + 8, 32, 32 };

            SDL_Texture *icon = item_icon_tex(itemTex, it);
            if (icon != NULL) {
                // item kendi sheet'inden (LPC izole parca = temiz ikon)
                SDL_RenderTexture(renderer, icon, &it->iconSrc, &dst);
            } else {
                // fallback: tool_icons + iconIndex
                SDL_FRect src = { it->iconIndex * 16.0f, 0, 16, 16 };
                SDL_RenderTexture(renderer, itemIconTex, &src, &dst);
            }

            // Miktar (stack > 1 ise) — AYNEN kalir
            if (inv->slots[i].count > 1 && font != NULL) {
                char buf[16];
                snprintf(buf, sizeof(buf), "%d", inv->slots[i].count);
                SDL_Color yellow = { 255, 230, 80, 255 };
                draw_text(renderer, font, buf, cx + 8, cy + cellSize - 24, yellow);
            }
        }
    }
}

bool inventory_add(Inventory *inv, const ItemType *type, int count)
{
    if (type == NULL) return false;

    // 1) Stack'lenebilen item zaten varsa, ona ekle
    if (type->maxStack > 1) {
        for (int i = 0; i < INVENTORY_SIZE; i++) {
            if (inv->slots[i].type == type && inv->slots[i].count < type->maxStack) {
                inv->slots[i].count += count;
                // (Basit: maxStack taşmasını şimdilik kontrol etmiyoruz —
                //  ileride taşan kısmı yeni slota koyarız)
                return true;
            }
        }
    }

    // 2) Boş slot bul
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (inv->slots[i].type == NULL) {
            inv->slots[i].type = type;
            inv->slots[i].count = count;
            return true;
        }
    }

    return false;   // çanta dolu
}

// Ekran koordinatindan hangi envanter slotuna denk geldigini bulur.
// panelX/panelY: inventory_render'a verilen panel sol-ust (AYNI degerler).
// Donen: slot indeksi (0..INVENTORY_SIZE-1) ya da -1 (bos alana tiklandi).
int inventory_slot_at(float x, float y, float panelX, float panelY)
{
    // inventory_render'daki AYNI matematik (tek kaynak — desenkronize olmasin)
    int cols = 5;
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