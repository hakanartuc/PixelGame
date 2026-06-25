#include "equipment.h"
#include "inventory.h"

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
        default:            return 0;    // gorseli olmayan slot (yuzuk/kolye vs)
    }
}