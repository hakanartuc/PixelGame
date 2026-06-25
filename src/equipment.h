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

#endif