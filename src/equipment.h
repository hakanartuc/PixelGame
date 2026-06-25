#ifndef EQUIPMENT_H
#define EQUIPMENT_H
#include "item.h"
#include "inventory.h"

typedef struct {
    ItemStack slots[EQUIP_SLOT_COUNT];
} Equipment;

// Envanterdeki item'i kuşan (silah tipi kurallarıyla). Başarılıysa true.
bool equip_from_inventory(Equipment *eq, Inventory *inv, int invSlotIndex);

// Ekipman slotunu çıkar (envantere geri). Başarılıysa true.
bool unequip_slot(Equipment *eq, Inventory *inv, EquipSlot slot);

// Bir ekipman slotunun paper-doll cizim z'si (LPC dosya oneki).
// Kucuk = arka, buyuk = on. Base katmanlar: body 10, head 100, face 101.
int slot_zorder(EquipSlot slot);

#endif