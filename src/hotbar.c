#include <SDL3/SDL.h>
#include <stdio.h>
#include "hotbar.h"
#include "font.h"
#include "ui_theme.h"
#include "uilayout.h"

#define CELL 48.0f
#define GAP 6.0f

static bool ability_on_bar(const Hotbar *hb, AbilityId id)
{
    for (int i = 0; i < HOTBAR_SLOTS; i++)
        if (hb->slots[i].kind == HOTBAR_SKILL && hb->slots[i].abilityId == id)
            return true;
    return false;
}

static int first_empty(const Hotbar *hb)
{
    for (int i = 0; i < HOTBAR_SLOTS; i++)
        if (hb->slots[i].kind == HOTBAR_EMPTY)
            return i;
    return -1;
}

void hotbar_place_unlocks(Hotbar *hb, const bool abilityOpen[ABILITY_COUNT])
{
    // Faz 21: açılış seviyeden değil AĞAÇTAN gelir (AKTİF düğüm satın alınır)
    for (int a = 0; a < ABILITY_COUNT; a++) {
        if (!abilityOpen[a])
            continue;   // düğümü henüz satın alınmadı
        if (ability_on_bar(hb, (AbilityId)a))
            continue;   // zaten barda
        int slot = first_empty(hb);
        if (slot < 0)
            return;     // bar dolu — oyuncu yer açınca tekrar denenir
        hb->slots[slot].kind = HOTBAR_SKILL;
        hb->slots[slot].abilityId = (AbilityId)a;
    }
}

// Ağaç sıfırlanınca (respec): artık açık olmayan aktifleri bardan kaldır
void hotbar_purge_closed(Hotbar *hb, const bool abilityOpen[ABILITY_COUNT])
{
    for (int i = 0; i < HOTBAR_SLOTS; i++)
        if (hb->slots[i].kind == HOTBAR_SKILL &&
            !abilityOpen[hb->slots[i].abilityId])
            hb->slots[i].kind = HOTBAR_EMPTY;
}

bool hotbar_assign_item(Hotbar *hb, const ItemType *type)
{
    if (type == NULL || !type->consumable)
        return false;
    for (int i = 0; i < HOTBAR_SLOTS; i++)
        if (hb->slots[i].kind == HOTBAR_ITEM && hb->slots[i].itemType == type)
            return true;   // tür zaten barda
    int slot = first_empty(hb);
    if (slot < 0)
        return false;
    hb->slots[slot].kind = HOTBAR_ITEM;
    hb->slots[slot].itemType = type;
    return true;
}

// Envanterdeki toplam adet (bar hücresindeki sayı için)
static int inv_total_count(const Inventory *inv, const ItemType *type)
{
    int total = 0;
    for (int i = 0; i < INVENTORY_SIZE; i++)
        if (inv->slots[i].type == type)
            total += inv->slots[i].count;
    return total;
}

#define BAR_W (HOTBAR_COLS * CELL + (HOTBAR_COLS - 1) * GAP)

// Barın TABAN NOKTASI: sol kenar + alt kenar. Yerleşim dosyasından gelir
// ("hotbar", alt-orta çapa, orta hiza). Anahtar yoksa varsayılan eski
// davranışın birebir aynısı: (winW-barW)/2 ve winH-12.
//
// Neden ayrı fonksiyon: bu nokta slot_rect'i, kaldırma hedefini VE main.c'deki
// ✕ çizimini besliyor. Formül kopyalanırsa bar taşındığında tıklama alanı
// görselden ayrılır — hata da sessiz olur (bar görünür, tıklama tutmaz).
static void bar_origin(int winW, int winH, float *x0, float *yBase)
{
    // Dikey hizası "ust" (varsayılan): dönen y BARIN TABANI, yükseklik
    // çıkarması slot_rect'te yapılıyor (bar 4 sıra + dikey kanatlar, tek bir
    // "yükseklik" sayısı onu temsil etmiyor)
    uilayout_get_aligned("hotbar", ((float)winW - BAR_W) * 0.5f,
                         (float)winH - 12.0f, BAR_W, 0.0f, x0, yBase);
}

// YERLEŞİM (tek kaynak): slot i → ekran dikdörtgeni.
// 0-7 alt bar, 8-15 üst bar (ortada alt alta); 16-23 sağ dikey 2×4;
// 24-31 sol dikey 2×4 (dikeyler ana barların bitişiğinde, alt hizalı)
static SDL_FRect slot_rect(int i, int winW, int winH)
{
    float barW = BAR_W;
    float x0, yBase;
    bar_origin(winW, winH, &x0, &yBase);
    float yBot = yBase - CELL;
    if (i < 8)           // alt ana bar
        return (SDL_FRect){ x0 + i * (CELL + GAP), yBot, CELL, CELL };
    if (i < 16)          // üst ana bar
        return (SDL_FRect){ x0 + (i - 8) * (CELL + GAP),
                            yBot - (CELL + GAP), CELL, CELL };
    if (i < 24) {        // sağ dikey 2×4
        int k = i - 16;
        float rx = x0 + barW + GAP * 3 + (float)(k % 2) * (CELL + GAP);
        float ry = yBot - (float)(3 - k / 2) * (CELL + GAP);
        return (SDL_FRect){ rx, ry, CELL, CELL };
    }
    {                    // sol dikey 2×4
        int k = i - 24;
        float lx = x0 - GAP * 3 - CELL - (float)(k % 2) * (CELL + GAP);
        float ly = yBot - (float)(3 - k / 2) * (CELL + GAP);
        return (SDL_FRect){ lx, ly, CELL, CELL };
    }
}

int hotbar_slot_at(float x, float y, int winW, int winH)
{
    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        SDL_FRect r = slot_rect(i, winW, winH);
        if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h)
            return i;
    }
    return -1;
}

// ✕ kaldırma hedefi: üst ana barın ÜSTÜNDE ortada (yalnız sürüklerken).
// Dikdörtgeni döndürüyoruz çünkü main.c bunu ÇİZİYOR da — formülü orada
// tekrarlamak bar taşınınca çizim ile tıklamayı ayırırdı.
SDL_FRect hotbar_remove_zone_rect(int winW, int winH)
{
    float x0, yBase;
    bar_origin(winW, winH, &x0, &yBase);
    return (SDL_FRect){ x0 + BAR_W / 2.0f - CELL / 2.0f,
                        yBase - 3.0f * (CELL + GAP), CELL, CELL };
}

bool hotbar_remove_zone_at(float x, float y, int winW, int winH)
{
    SDL_FRect r = hotbar_remove_zone_rect(winW, winH);
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

void hotbar_render(const Hotbar *hb, R2D *renderer,
                   R2DTexture *itemIconTex, const ItemTextures *itemTex,
                   R2DTexture *abilityIconTex,
                   TTF_Font *font, const float *abilityCd, const float *itemCd,
                   const Inventory *inv, const bool *abilityUsable,
                   int winW, int winH)
{
    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        SDL_FRect cell = slot_rect(i, winW, winH);
        float cx = cell.x, y0 = cell.y;
        const HotbarSlot *s = &hb->slots[i];

        if (s->kind != HOTBAR_EMPTY)
            ui_cell(renderer, cell.x, cell.y, cell.w, cell.h);
        else
            ui_cell_gray(renderer, cell.x, cell.y, cell.w, cell.h);

        if (s->kind == HOTBAR_SKILL) {
            const AbilityDef *a = &ABILITIES[s->abilityId];
            bool usable = (abilityUsable == NULL) || abilityUsable[s->abilityId];

            if (abilityIconTex != NULL) {
                SDL_FRect isrc = { (float)(s->abilityId * 32), 0, 32, 32 };
                SDL_FRect idst = { cx + 4.0f, y0 + 4.0f, CELL - 8.0f, CELL - 8.0f };
                if (!usable) r2d_texture_set_alpha_mod(abilityIconTex, 90);
                r2d_draw_texture(renderer, abilityIconTex, &isrc, &idst);
                r2d_texture_set_alpha_mod(abilityIconTex, 255);
            }
            else if (font != NULL)
                draw_text(renderer, font, a->abbrev, cx + 10.0f, y0 + 16.0f, UI_TEXT_DARK);

            // Silah uymuyor: soluk perde (yay becerisi kilicla vs.)
            if (!usable) {
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                r2d_set_color(renderer, 20, 20, 26, 130);
                r2d_fill_rect(renderer, &cell);
            }

            float cd = abilityCd[s->abilityId];
            if (cd > 0.0f) {
                float ratio = cd / a->cooldown;
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                r2d_set_color(renderer, 0, 0, 0, 170);
                SDL_FRect shade = { cx, y0 + CELL * (1.0f - ratio), CELL, CELL * ratio };
                r2d_fill_rect(renderer, &shade);
                if (font != NULL) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "%d", (int)cd + 1);
                    SDL_Color gray = { 200, 200, 210, 255 };
                    draw_text(renderer, font, buf, cx + 18.0f, y0 + 30.0f, gray);
                }
            }
        }
        else if (s->kind == HOTBAR_ITEM) {
            const ItemType *it = s->itemType;
            SDL_FRect dst = { cx + 8.0f, y0 + 8.0f, 32.0f, 32.0f };
            R2DTexture *icon = item_icon_tex(itemTex, it);
            if (icon != NULL)
                r2d_draw_texture(renderer, icon, &it->iconSrc, &dst);
            else {
                SDL_FRect src = { it->iconIndex * 16.0f, 0, 16, 16 };
                r2d_draw_texture(renderer, itemIconTex, &src, &dst);
            }
            float cd = itemCd[it - ITEM_TYPES];
            if (cd > 0.0f && it->useCooldown > 0.0f) {
                float ratio = cd / it->useCooldown;
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                r2d_set_color(renderer, 0, 0, 0, 170);
                SDL_FRect shade = { cx, y0 + CELL * (1.0f - ratio), CELL, CELL * ratio };
                r2d_fill_rect(renderer, &shade);
            }
            int count = inv_total_count(inv, it);
            if (count == 0) {
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                r2d_set_color(renderer, 0, 0, 0, 170);
                r2d_fill_rect(renderer, &cell);
            }
            if (font != NULL) {
                char buf[8];
                snprintf(buf, sizeof(buf), "%d", count);
                SDL_Color yellow = { 255, 230, 80, 255 };
                draw_text(renderer, font, buf, cx + 4.0f, y0 + CELL - 18.0f, yellow);
            }
        }

        // Tus etiketi: alt bar 1-8, ust bar S1-S8, sag dikey A1-A8, sol yok
        if (font != NULL && i < 24) {
            char key[6];
            if (i < 8)        snprintf(key, sizeof(key), "%d", i + 1);
            else if (i < 16)  snprintf(key, sizeof(key), "S%d", i - 7);
            else              snprintf(key, sizeof(key), "A%d", i - 15);
            draw_text(renderer, font, key, cx + 3.0f, y0 + 2.0f, UI_TEXT_DARK);
        }
    }
}
