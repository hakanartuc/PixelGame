#include "inventory.h"

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