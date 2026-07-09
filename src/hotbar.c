#include <SDL3/SDL.h>
#include <stdio.h>
#include "hotbar.h"
#include "font.h"
#include "ui_theme.h"

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

void hotbar_place_unlocks(Hotbar *hb, const Skill *skills)
{
    for (int a = 0; a < ABILITY_COUNT; a++) {
        if (skills[ABILITIES[a].skill].level < ABILITIES[a].unlockLevel)
            continue;   // henüz açılmadı
        if (ability_on_bar(hb, (AbilityId)a))
            continue;   // zaten barda
        int slot = first_empty(hb);
        if (slot < 0)
            return;     // bar dolu — oyuncu yer açınca tekrar denenir
        hb->slots[slot].kind = HOTBAR_SKILL;
        hb->slots[slot].abilityId = (AbilityId)a;
    }
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

void hotbar_render(const Hotbar *hb, SDL_Renderer *renderer,
                   SDL_Texture *itemIconTex, const ItemTextures *itemTex,
                   TTF_Font *font, const float *abilityCd, const float *itemCd,
                   const Inventory *inv, int winW, int winH)
{
    float barW = HOTBAR_SLOTS * CELL + (HOTBAR_SLOTS - 1) * GAP;
    float x0 = (winW - barW) / 2.0f;
    float y0 = winH - CELL - 12.0f;

    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        float cx = x0 + i * (CELL + GAP);
        const HotbarSlot *s = &hb->slots[i];

        // Hücre zemini (tema: dolu ten, boş gri)
        SDL_FRect cell = { cx, y0, CELL, CELL };
        if (s->kind != HOTBAR_EMPTY)
            ui_cell(renderer, cell.x, cell.y, cell.w, cell.h);
        else
            ui_cell_gray(renderer, cell.x, cell.y, cell.w, cell.h);

        if (s->kind == HOTBAR_SKILL) {
            const AbilityDef *a = &ABILITIES[s->abilityId];

            // Kısaltma (ikon gelene kadar) — ten zeminde koyu metin
            if (font != NULL) {
                draw_text(renderer, font, a->abbrev, cx + 10.0f, y0 + 16.0f, UI_TEXT_DARK);
            }

            // Cooldown perdesi: kalan süreye oranlı, alttan dolu
            float cd = abilityCd[s->abilityId];
            if (cd > 0.0f) {
                float ratio = cd / a->cooldown;   // 1 → 0
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 170);
                SDL_FRect shade = { cx, y0 + CELL * (1.0f - ratio), CELL, CELL * ratio };
                SDL_RenderFillRect(renderer, &shade);

                if (font != NULL) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "%d", (int)cd + 1);   // 2.3 → "3"
                    SDL_Color gray = { 200, 200, 210, 255 };
                    draw_text(renderer, font, buf, cx + 18.0f, y0 + 30.0f, gray);
                }
            }
        }
        else if (s->kind == HOTBAR_ITEM) {
            const ItemType *it = s->itemType;
            SDL_FRect dst = { cx + 8.0f, y0 + 8.0f, 32.0f, 32.0f };

            SDL_Texture *icon = item_icon_tex(itemTex, it);
            if (icon != NULL) {
                SDL_RenderTexture(renderer, icon, &it->iconSrc, &dst);
            } else {
                SDL_FRect src = { it->iconIndex * 16.0f, 0, 16, 16 };
                SDL_RenderTexture(renderer, itemIconTex, &src, &dst);
            }

            // Tür cooldown perdesi (skill'lerle aynı dil: alttan dolu + kalan sn)
            float cd = itemCd[it - ITEM_TYPES];
            if (cd > 0.0f && it->useCooldown > 0.0f) {
                float ratio = cd / it->useCooldown;   // 1 → 0
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 170);
                SDL_FRect shade = { cx, y0 + CELL * (1.0f - ratio), CELL, CELL * ratio };
                SDL_RenderFillRect(renderer, &shade);

                if (font != NULL) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "%d", (int)cd + 1);
                    SDL_Color gray = { 200, 200, 210, 255 };
                    draw_text(renderer, font, buf, cx + 18.0f, y0 + 30.0f, gray);
                }
            }

            // Envanterdeki adet — 0 ise hücre soluk (tür barda kalır, KO gibi)
            int count = inv_total_count(inv, it);
            if (count == 0) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 170);
                SDL_RenderFillRect(renderer, &cell);
            }
            if (font != NULL) {
                char buf[8];
                snprintf(buf, sizeof(buf), "%d", count);
                SDL_Color yellow = { 255, 230, 80, 255 };
                draw_text(renderer, font, buf, cx + 4.0f, y0 + CELL - 18.0f, yellow);
            }
        }

        // Tuş numarası (çerçeve tema hücresinden geliyor)
        if (font != NULL) {
            char key[4];
            snprintf(key, sizeof(key), "%d", i + 1);
            draw_text(renderer, font, key, cx + 3.0f, y0 + 2.0f, UI_TEXT_DARK);
        }
    }
}
