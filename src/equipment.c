#include "equipment.h"
#include "inventory.h"
#include "player.h"

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

    // Envantere eklemeyi dene
    if (!inventory_add(inv, eq->slots[slot].type, eq->slots[slot].count))
        return false;   // çanta dolu → çıkarılamadı

    eq->slots[slot].type = NULL;   // slotu boşalt
    eq->slots[slot].count = 0;
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
    if (targetSlot == EQUIP_WEAPON || targetSlot == EQUIP_SHIELD) {
        WeaponType wt = item->weaponType;

        if (wt == WEAPON_TWO_HAND) {
            // Çift el: hem WEAPON hem SHIELD slotu boşalmalı (ikisini de çıkar)
            if (!unequip_to_inventory(eq, inv, EQUIP_WEAPON)) return false;
            if (!unequip_to_inventory(eq, inv, EQUIP_SHIELD)) return false;
            // İki-el silahı WEAPON'a koy (SHIELD boş kalır = kilitli)
            targetSlot = EQUIP_WEAPON;
        }
        else if (wt == WEAPON_SHIELD) {
            // Kalkan: WEAPON two-hand ise kuşanılamaz (iki el dolu)
            if (eq->slots[EQUIP_WEAPON].type != NULL &&
                eq->slots[EQUIP_WEAPON].type->weaponType == WEAPON_TWO_HAND) {
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

    // --- Hedef slottaki eskiyi envantere geri koy (takas) ---
    // Ama önce: kuşanılacak item'i envanterden çıkaracağız, o yüzden
    // takas sırası önemli. Şöyle: eskiyi geri koy, sonra yeniyi koy.

    // Önce kuşanılacak item'i sakla (envanterden çıkmadan)
    const ItemType *toEquip = item;
    int toEquipCount = invStack->count;

    // Envanter slotunu boşalt (item'i "elimize aldık")
    invStack->type = NULL;
    invStack->count = 0;

    // Hedef slottaki eskiyi envantere geri koy
    if (eq->slots[targetSlot].type != NULL) {
        // Eskisini envantere ekle (yer var, çünkü az önce bir slot boşalttık)
        inventory_add(inv, eq->slots[targetSlot].type, eq->slots[targetSlot].count);
    }

    // Yeniyi kuşan
    eq->slots[targetSlot].type = toEquip;
    eq->slots[targetSlot].count = toEquipCount;

    return true;
}

// Bir ekipman slotunu çıkar (envantere geri koy).
bool unequip_slot(Equipment *eq, Inventory *inv, EquipSlot slot)
{
    return unequip_to_inventory(eq, inv, slot);
}

int slot_zorder(EquipSlot slot)
{
    switch (slot) {
        case EQUIP_LEGS:    return 20;   // pantolon (body 10 ustu)
        case EQUIP_BOOTS:   return 25;   // ayakkabi
        case EQUIP_CHEST:   return 60;   // govde zirhi
        case EQUIP_GLOVES:  return 70;   // eldiven
        case EQUIP_HELMET:  return 130;  // kask (en ust)
        case EQUIP_WEAPON:  return 140;  // silah onu (en ust, LPC z140)
        default:            return 0;    // gorseli olmayan slot (yuzuk/kolye vs)
    }
}

// Karakter panelini cizer: arkaplan + paper-doll onizleme + slot kutulari.
// Ekran-uzayi (kamera yok), envanterin USTUNDE. panelX/panelY = panel sol-ust (ekran).
void equipment_render(const Equipment *eq, SDL_Renderer *renderer,
                      SDL_Texture *itemIcons, const ItemTextures *itemTex,
                      const PlayerLayer *built, int builtCount,
                      int panelX, int panelY)
{
    const int PANEL_W = 400;
    const int PANEL_H = 340;

    // 1) Panel arkaplani (koyu yari-saydam)
    SDL_FRect bg = { (float)panelX, (float)panelY, (float)PANEL_W, (float)PANEL_H };
    SDL_SetRenderDrawColor(renderer, 30, 30, 40, 230);
    SDL_RenderFillRect(renderer, &bg);
    SDL_SetRenderDrawColor(renderer, 200, 200, 210, 255);
    SDL_RenderRect(renderer, &bg);

    // 2) Paper-doll onizleme: idle-down, tum giyili katmanlar. built[] zaten z-sirali.
    
        const int DOLL_X = panelX + 136;   // 128px doll'u ~ortala (sol15+48 ile sag337 arasi)
        const int DOLL_Y = panelY + 40;
        const int DOLL_SCALE = 2;          // 64 → 128, panelde belirgin

        // idle satiri 22 + DIR_DOWN(2) = 24, ilk kare (x=0)
        SDL_FRect srcDoll = { 0, 24 * FRAME_H, FRAME_W, FRAME_H };
        SDL_FRect dstDoll = { (float)DOLL_X, (float)DOLL_Y,
                              (float)(FRAME_W * DOLL_SCALE), (float)(FRAME_H * DOLL_SCALE) };
        for (int i = 0; i < builtCount; i++)
            SDL_RenderTexture(renderer, built[i].tex, &srcDoll, &dstDoll);
    
    
    // 3) Slot kutulari (EQUIP_SLOT_POS tablosu). Dolu slot → item ikonu.
    for (int s = 0; s < EQUIP_SLOT_COUNT; s++) {
        SlotPos sp = EQUIP_SLOT_POS[s];
        SDL_FRect cell = { (float)(panelX + sp.x), (float)(panelY + sp.y),
                           EQUIP_SLOT_SIZE, EQUIP_SLOT_SIZE };

        // bos hucre cercevesi
        SDL_SetRenderDrawColor(renderer, 70, 70, 85, 255);
        SDL_RenderFillRect(renderer, &cell);
        SDL_SetRenderDrawColor(renderer, 140, 140, 155, 255);
        SDL_RenderRect(renderer, &cell);

        // dolu ise ikon bas (envanter ikonu, 16x16 → hucreye olcekli)
        const ItemStack *st = &eq->slots[s];
        if (st->type != NULL) {
            SDL_Texture *icon = item_icon_tex(itemTex, st->type);
            if (icon != NULL) {
                SDL_RenderTexture(renderer, icon, &st->type->iconSrc, &cell);
            } else {
                int ic = st->type->iconIndex;
                SDL_FRect srcIcon = { (float)(ic * 16), 0, 16, 16 };
                SDL_RenderTexture(renderer, itemIcons, &srcIcon, &cell);
            }
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