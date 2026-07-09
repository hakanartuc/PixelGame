#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include "save.h"
#include "item.h"
#include "skill.h"

#define SAVE_MAGIC "PIXE"
#define SAVE_VERSION 1u

// --- Yazma/okuma yardımcıları: her alan tek tek, hata tek noktadan ---
// fwrite/fread sayı döndürür; 1 değilse dosya kesik/bozuk demektir.

static bool w32(FILE *f, Sint32 v)  { return fwrite(&v, sizeof(v), 1, f) == 1; }
static bool wf(FILE *f, float v)    { return fwrite(&v, sizeof(v), 1, f) == 1; }
static bool r32(FILE *f, Sint32 *v) { return fread(v, sizeof(*v), 1, f) == 1; }
static bool rf(FILE *f, float *v)   { return fread(v, sizeof(*v), 1, f) == 1; }

// ItemStack diske: tür İNDEKS olarak (pointer diske yazılmaz), -1 = boş slot
static bool write_stack(FILE *f, const ItemStack *s)
{
    Sint32 idx = (s->type != NULL) ? (Sint32)(s->type - ITEM_TYPES) : -1;
    return w32(f, idx) && w32(f, s->count) && w32(f, s->plusLevel);
}

static bool read_stack(FILE *f, ItemStack *s)
{
    Sint32 idx, count, plus;
    if (!r32(f, &idx) || !r32(f, &count) || !r32(f, &plus))
        return false;
    if (idx < -1 || idx >= ITEM_TYPE_COUNT)
        return false;   // bozuk/eski dosya — indeks tablo dışı
    s->type = (idx >= 0) ? &ITEM_TYPES[idx] : NULL;
    s->count = count;
    s->plusLevel = plus;
    return true;
}

bool save_game(const char *path, const Player *p,
               const Inventory *inv, const Equipment *eq)
{
    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        SDL_Log("Save acilamadi: %s", path);
        return false;
    }

    bool ok = fwrite(SAVE_MAGIC, 4, 1, f) == 1 && w32(f, (Sint32)SAVE_VERSION);

    // Oyuncu
    ok = ok && wf(f, p->rect.x) && wf(f, p->rect.y)
            && wf(f, p->health) && wf(f, p->maxHealth)
            && w32(f, p->strength) && w32(f, p->dexterity) && w32(f, p->gold);

    // Skill'ler (sayı da yazılır: ileride skill eklenirse eski save okunabilir kalır)
    ok = ok && w32(f, SKILL_COUNT);
    for (int i = 0; ok && i < SKILL_COUNT; i++)
        ok = w32(f, p->skills[i].level) && wf(f, p->skills[i].xp)
          && wf(f, p->skills[i].xpToNext);

    // Envanter + ekipman (slot sayıları da yazılır, aynı sebeple)
    ok = ok && w32(f, INVENTORY_SIZE);
    for (int i = 0; ok && i < INVENTORY_SIZE; i++)
        ok = write_stack(f, &inv->slots[i]);

    ok = ok && w32(f, EQUIP_SLOT_COUNT);
    for (int i = 0; ok && i < EQUIP_SLOT_COUNT; i++)
        ok = write_stack(f, &eq->slots[i]);

    fclose(f);
    if (!ok)
        SDL_Log("Save yazilamadi (disk dolu?): %s", path);
    return ok;
}

bool load_game(const char *path, Player *p,
               Inventory *inv, Equipment *eq)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return false;   // save yok — sessizce geç (ilk oyun)

    char magic[4];
    Sint32 version;
    if (fread(magic, 4, 1, f) != 1 || memcmp(magic, SAVE_MAGIC, 4) != 0 ||
        !r32(f, &version) || version != (Sint32)SAVE_VERSION) {
        SDL_Log("Save dosyasi taninmadi/eski surum: %s", path);
        fclose(f);
        return false;
    }

    // Önce GEÇİCİ kopyalara oku: dosya yarıda bozuksa mevcut oyun durumu bozulmasın
    Player tmpP = *p;
    Inventory tmpInv = {0};
    Equipment tmpEq = {0};

    bool ok = rf(f, &tmpP.rect.x) && rf(f, &tmpP.rect.y)
           && rf(f, &tmpP.health) && rf(f, &tmpP.maxHealth)
           && r32(f, &tmpP.strength) && r32(f, &tmpP.dexterity) && r32(f, &tmpP.gold);

    Sint32 n;
    ok = ok && r32(f, &n);
    for (int i = 0; ok && i < n; i++) {
        Skill s;
        ok = r32(f, &s.level) && rf(f, &s.xp) && rf(f, &s.xpToNext);
        if (ok && i < SKILL_COUNT)   // fazlası (ileriki sürüm) atlanır
            tmpP.skills[i] = s;
    }

    ok = ok && r32(f, &n);
    for (int i = 0; ok && i < n; i++) {
        ItemStack s;
        ok = read_stack(f, &s);
        if (ok && i < INVENTORY_SIZE)
            tmpInv.slots[i] = s;
    }

    ok = ok && r32(f, &n);
    for (int i = 0; ok && i < n; i++) {
        ItemStack s;
        ok = read_stack(f, &s);
        if (ok && i < EQUIP_SLOT_COUNT)
            tmpEq.slots[i] = s;
    }

    fclose(f);
    if (!ok) {
        SDL_Log("Save okunamadi (bozuk dosya): %s", path);
        return false;
    }

    // Her şey okundu — şimdi güvenle uygula
    *p = tmpP;
    *inv = tmpInv;
    *eq = tmpEq;
    return true;
}
