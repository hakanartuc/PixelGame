#include <stdio.h>
#include <string.h>
#include "equipment.h"
#include "inventory.h"
#include "player.h"
#include "ui_theme.h"
#include "font.h"

// Slotlarin panel-ici konumlari (sol-ust kose orijin). KO yerlesimi.
// Orta sutun ~ paper-doll; sag sutun zirh; sol sutun aksesuar; alt silah/kalkan.
const SlotPos EQUIP_SLOT_POS[EQUIP_SLOT_COUNT] = {
    // SAG kolon — zirh parcalari (paper-doll'un sagi)
    [EQUIP_HELMET]   = { 337,  30 },
    [EQUIP_CHEST]    = { 337,  90 },
    [EQUIP_GLOVES]   = { 337, 150 },
    [EQUIP_LEGS]     = { 337, 210 },
    [EQUIP_BOOTS]    = { 337, 270 },
    // SOL kolon — aksesuar
    [EQUIP_NECKLACE] = {  15,  30 },
    [EQUIP_EARRING_1]= {  15,  90 },
    [EQUIP_EARRING_2]= {  15, 150 },
    [EQUIP_RING_1]   = {  15, 210 },
    [EQUIP_RING_2]   = {  15, 270 },
    // ALT orta — silah / kalkan / kemer
    [EQUIP_WEAPON]   = { 120, 285 },
    [EQUIP_SHIELD]   = { 180, 285 },
    [EQUIP_BELT]     = { 240, 285 },
};

// Bir slottaki item'i envantere geri koy + slotu boşalt.
// (Yer yoksa false — kuşanma iptal edilmeli)
static bool unequip_to_inventory(Equipment *eq, Inventory *inv, EquipSlot slot)
{
    if (eq->slots[slot].type == NULL) return true;   // zaten boş, sorun yok

    // Envantere eklemeyi dene (plus/rün/stat instance'la birlikte taşınır)
    if (!inventory_add_stack(inv, &eq->slots[slot]))
        return false;   // çanta dolu → çıkarılamadı

    eq->slots[slot] = item_make(NULL, 0, 0);   // slotu boşalt (tüm instance alanları)
    return true;
}

// Envanterdeki bir item'i kuşan (envanter slot indeksi verilir).
// Silah tipi kurallarını uygular. Başarılıysa true.
bool equip_from_inventory(Equipment *eq, Inventory *inv, int invSlotIndex)
{
    ItemStack *invStack = &inv->slots[invSlotIndex];
    if (invStack->type == NULL) return false;            // boş slot
    if (!invStack->type->equipable) return false;         // kuşanılamaz

    const ItemType *item = invStack->type;
    EquipSlot targetSlot = item->equipSlot;

    // --- Silah tipi özel kuralları ---
    bool clearShield = false;   // iki-el: kalkan da çıkmalı (takas planına girer)
    if (targetSlot == EQUIP_WEAPON || targetSlot == EQUIP_SHIELD) {
        WeaponType wt = item->weaponType;

        if (wt == WEAPON_TWO_HAND || wt == WEAPON_BOW || wt == WEAPON_STAFF) {
            // Çift el (yay dahil): WEAPON'a takılır, SHIELD boşalmalı.
            // Çıkarma SIRASI aşağıda takasla çözülür — çanta dolu olsa bile
            // eski silah yeni itemin slotuna YERİNDE takas edilir.
            clearShield = true;
            targetSlot = EQUIP_WEAPON;
        }
        else if (wt == WEAPON_SHIELD) {
            // Kalkan: WEAPON iki elli ise kuşanılamaz (iki el dolu)
            if (eq->slots[EQUIP_WEAPON].type != NULL &&
                (eq->slots[EQUIP_WEAPON].type->weaponType == WEAPON_TWO_HAND ||
                 eq->slots[EQUIP_WEAPON].type->weaponType == WEAPON_BOW ||
                 eq->slots[EQUIP_WEAPON].type->weaponType == WEAPON_STAFF)) {
                return false;   // iki-el silah varken kalkan takılamaz
            }
            targetSlot = EQUIP_SHIELD;
        }
        else if (wt == WEAPON_ONE_HAND) {
            // Tek el: WEAPON'a. Mevcut two-hand varsa o çıkar (SHIELD kilidi açılır)
            targetSlot = EQUIP_WEAPON;
        }
        // (WEAPON_DUAL — şimdilik ONE_HAND gibi davransın, ikinci eli sonra ele alırız)
    }

    // --- YERİNDE TAKAS: eski ekipman, yeni itemin geldiği slota yazılır ---
    // (çanta DOLU olsa bile çalışır — yer aramıyoruz, slotu takas ediyoruz;
    //  instance'lar olduğu gibi taşınır: plus/rün/stat kaybolmaz)
    ItemStack toEquip = *invStack;
    ItemStack old = eq->slots[targetSlot];   // eski (type NULL olabilir)

    // İki el + takılı kalkan: kalkana da yer lazım.
    if (clearShield && eq->slots[EQUIP_SHIELD].type != NULL) {
        if (old.type != NULL) {
            // Eski silah takas slotunu kullanacak → kalkana AYRI yer gerek.
            // Çanta doluysa dürüst ret (hiçbir şey değişmez).
            if (!unequip_to_inventory(eq, inv, EQUIP_SHIELD)) return false;
        } else {
            // Silah eli boş: kalkan doğrudan yeni itemin slotuna takas olur
            *invStack = eq->slots[EQUIP_SHIELD];
            eq->slots[EQUIP_SHIELD] = item_make(NULL, 0, 0);
            eq->slots[targetSlot] = toEquip;
            return true;
        }
    }

    *invStack = old;                    // eski ekipman yeni itemin slotuna
    if (old.type == NULL)               // eski yoksa slot temiz boşalır
        *invStack = item_make(NULL, 0, 0);
    eq->slots[targetSlot] = toEquip;

    return true;
}

// Bir ekipman slotunu çıkar (envantere geri koy).
bool unequip_slot(Equipment *eq, Inventory *inv, EquipSlot slot)
{
    return unequip_to_inventory(eq, inv, slot);
}

// Slot icin VARSAYILAN z — item kendi equipZ'sini verirse o kazanir (build_player_layers).
int slot_zorder(EquipSlot slot)
{
    switch (slot) {
        case EQUIP_LEGS:    return 20;   // pantolon (body 10 ustu)
        case EQUIP_BOOTS:   return 25;   // ayakkabi
        case EQUIP_CHEST:   return 60;   // govde zirhi
        case EQUIP_GLOVES:  return 70;   // eldiven
        case EQUIP_HELMET:  return 130;  // kask
        case EQUIP_SHIELD:  return 135;  // kalkan (kaskin ustu, silahin alti)
        case EQUIP_WEAPON:  return 140;  // silah onu (en ust, LPC z140)
        default:            return 0;    // gorseli olmayan slot (yuzuk/kolye vs)
    }
}

// Karakter panelini cizer: arkaplan + paper-doll onizleme + slot kutulari.
// Ekran-uzayi (kamera yok), envanterin USTUNDE. panelX/panelY = panel sol-ust (ekran).
void equipment_render(const Equipment *eq, R2D *renderer,
                      R2DTexture *itemIcons, const ItemTextures *itemTex,
                      const PlayerLayer *built, int builtCount,
                      int panelX, int panelY, TTF_Font *font)
{
    const int PANEL_W = 400;
    const int PANEL_H = 340;

    // 1) Panel arkaplani (tema: ahşap çerçeve)
    ui_panel(renderer, (float)panelX, (float)panelY, (float)PANEL_W, (float)PANEL_H);

    // 2) Paper-doll onizleme: idle-down, tum giyili katmanlar. built[] zaten z-sirali.
    
        const int DOLL_X = panelX + 136;   // 128px doll'u ~ortala (sol15+48 ile sag337 arasi)
        const int DOLL_Y = panelY + 40;
        const int DOLL_SCALE = 2;          // 64 → 128, panelde belirgin

        // idle satiri 22 + DIR_DOWN(2) = 24, ilk kare (x=0)
        SDL_FRect srcDoll = { 0, 24 * FRAME_H, FRAME_W, FRAME_H };
        SDL_FRect dstDoll = { (float)DOLL_X, (float)DOLL_Y,
                              (float)(FRAME_W * DOLL_SCALE), (float)(FRAME_H * DOLL_SCALE) };
        for (int i = 0; i < builtCount; i++) {
            if (built[i].smash128) continue;   // arac sheet'inde idle yok — cop basar
            r2d_draw_texture(renderer, built[i].tex, &srcDoll, &dstDoll);
        }
    
    
    // 3) Slot kutulari (EQUIP_SLOT_POS tablosu). Dolu slot → item ikonu.
    for (int s = 0; s < EQUIP_SLOT_COUNT; s++) {
        SlotPos sp = EQUIP_SLOT_POS[s];
        SDL_FRect cell = { (float)(panelX + sp.x), (float)(panelY + sp.y),
                           EQUIP_SLOT_SIZE, EQUIP_SLOT_SIZE };

        // hucre: dolu ten, bos gri (tema)
        if (eq->slots[s].type != NULL)
            ui_cell(renderer, cell.x, cell.y, cell.w, cell.h);
        else
            ui_cell_gray(renderer, cell.x, cell.y, cell.w, cell.h);

        // dolu ise ikon bas (envanter ikonu, 16x16 → hucreye olcekli)
        const ItemStack *st = &eq->slots[s];
        if (st->type != NULL) {
            R2DTexture *icon = item_icon_tex(itemTex, st->type);
            if (icon != NULL) {
                r2d_draw_texture(renderer, icon, &st->type->iconSrc, &cell);
            } else {
                int ic = st->type->iconIndex;
                SDL_FRect srcIcon = { (float)(ic * 16), 0, 16, 16 };
                r2d_draw_texture(renderer, itemIcons, &srcIcon, &cell);
            }

            // Sunak stat basımı: slot köşesinde küçük mor rozet (kaç slot dolu)
            int nStat = (st->statType[0] >= 0) + (st->statType[1] >= 0);
            if (nStat > 0) {
                SDL_FRect dot = { cell.x + cell.w - 8.0f, cell.y + 2.0f, 6.0f, 6.0f };
                r2d_set_color(renderer, 190, 120, 255, 255);
                r2d_fill_rect(renderer, &dot);
                if (nStat > 1) {
                    dot.x -= 8.0f;
                    r2d_fill_rect(renderer, &dot);
                }
            }
        }
    }

    // 4) Sunak stat toplamları — doll'un altında tek satır (0 olanlar gizli)
    if (font != NULL) {
        int gear[STAT_COUNT] = {0};
        for (int s = 0; s < EQUIP_SLOT_COUNT; s++) {
            const ItemStack *st = &eq->slots[s];
            if (st->type == NULL) continue;
            for (int k = 0; k < 2; k++)
                if (st->statType[k] >= 0)
                    gear[st->statType[k]] +=
                        STAT_TIER_VALUE[st->statType[k]] * st->statTier[k];
        }
        char line[96] = "";
        for (int t = 0; t < STAT_COUNT; t++) {
            if (gear[t] <= 0) continue;
            char part[24];
            snprintf(part, sizeof(part), "%s%s +%d",
                     line[0] ? "  " : "", STAT_NAMES[t], gear[t]);
            strncat(line, part, sizeof(line) - strlen(line) - 1);
        }
        if (line[0]) {
            SDL_Color purple = { 200, 150, 255, 255 };
            draw_text(renderer, font, line,
                      (float)(panelX + 100), (float)(panelY + PANEL_H - 26), purple);
        }
    }
}

// Ekran koordinatindan hangi ekipman slotuna denk geldigini bulur.
// panelX/panelY: equipment_render'a verilen panel sol-ust (AYNI degerler).
// Donen: EquipSlot (0..EQUIP_SLOT_COUNT-1) ya da EQUIP_NONE(-1).
EquipSlot equipment_slot_at(float x, float y, int panelX, int panelY)
{
    for (int s = 0; s < EQUIP_SLOT_COUNT; s++) {
        SlotPos sp = EQUIP_SLOT_POS[s];
        SDL_FRect cell = { (float)(panelX + sp.x), (float)(panelY + sp.y),
                           EQUIP_SLOT_SIZE, EQUIP_SLOT_SIZE };
        if (x >= cell.x && x < cell.x + cell.w &&
            y >= cell.y && y < cell.y + cell.h) {
            return (EquipSlot)s;
        }
    }
    return EQUIP_NONE;
}