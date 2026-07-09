#ifndef EQUIPMENT_H
#define EQUIPMENT_H
#include "r2d.h"
#include "item.h"
#include "inventory.h"

typedef struct Equipment {
    ItemStack slots[EQUIP_SLOT_COUNT];
} Equipment;

// Bir ekipman slotunun panel-ici (yerel) konumu. Cizim: panelX + x, panelY + y.
typedef struct {
    int x;
    int y;
} SlotPos;

#define EQUIP_SLOT_SIZE 48   // slot kutusu kenar (envanter hucresiyle ayni)

extern const SlotPos EQUIP_SLOT_POS[EQUIP_SLOT_COUNT];

typedef struct PlayerLayer PlayerLayer;   // forward decl — pointer icin yeterli, dongusel include'i kirar

// Envanterdeki item'i kuşan (silah tipi kurallarıyla). Başarılıysa true.
bool equip_from_inventory(Equipment *eq, Inventory *inv, int invSlotIndex);

// Ekipman slotunu çıkar (envantere geri). Başarılıysa true.
bool unequip_slot(Equipment *eq, Inventory *inv, EquipSlot slot);

// Bir ekipman slotunun paper-doll cizim z'si (LPC dosya oneki).
// Kucuk = arka, buyuk = on. Base katmanlar: body 10, head 100, face 101.
int slot_zorder(EquipSlot slot);

void equipment_render(const Equipment *eq, R2D *renderer,
                      R2DTexture *itemIcons, const ItemTextures *itemTex,
                      const PlayerLayer *built, int builtCount,
                      int panelX, int panelY);

EquipSlot equipment_slot_at(float x, float y, int panelX, int panelY);

#endif