#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include "save.h"
#include "item.h"
#include "skill.h"
#include "gametime.h"
#include "minimap.h"
#include "npc.h"
#include "craft.h"   // öğrenilen tarifler (v10)

#define SAVE_MAGIC "PIXE"
#define SAVE_VERSION 10u  // ...; v8: bölgesel bounty/itibar; v9: NPC listesi değişti;
                          // v10: öğrenilen blueprint tarifleri

// Bit-paketli keşif matrisi tamponu (256×256 tavan → 8KB)
#define EXPLORED_BUF_CAP ((MAP_W_MAX * MAP_H_MAX + 7) / 8)

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
               const Inventory *inv, const Equipment *eq, const Hotbar *hb,
               const QuestStatus *quests)
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

    // Hotbar: tür kind'a göre — skill ability indeksi, item ITEM_TYPES indeksi
    ok = ok && w32(f, HOTBAR_SLOTS);
    for (int i = 0; ok && i < HOTBAR_SLOTS; i++) {
        const HotbarSlot *s = &hb->slots[i];
        Sint32 ref = (s->kind == HOTBAR_SKILL) ? (Sint32)s->abilityId
                   : (s->kind == HOTBAR_ITEM)  ? (Sint32)(s->itemType - ITEM_TYPES)
                   : -1;
        ok = w32(f, (Sint32)s->kind) && w32(f, ref);
    }

    // Görev durumları (v3)
    ok = ok && w32(f, QUEST_COUNT);
    for (int i = 0; ok && i < QUEST_COUNT; i++)
        ok = w32(f, (Sint32)quests[i].state) && w32(f, quests[i].progress);

    // Dünya saati (v4) — gametime doğrudan modülden okunur (parametre şişmesin)
    int day; float hours;
    gametime_get(&day, &hours);
    ok = ok && w32(f, day) && wf(f, hours);

    // Keşif haritası (v5) — bit-paketli
    static Uint8 exploredBuf[EXPLORED_BUF_CAP];
    int exploredBytes = minimap_pack_explored(exploredBuf, EXPLORED_BUF_CAP);
    ok = ok && w32(f, exploredBytes);
    if (ok && exploredBytes > 0)
        ok = fwrite(exploredBuf, (size_t)exploredBytes, 1, f) == 1;

    // Perk seçimleri (v6): skill × tier matris
    ok = ok && w32(f, SKILL_COUNT) && w32(f, PERK_TIERS);
    for (int s = 0; ok && s < SKILL_COUNT; s++)
        for (int t = 0; ok && t < PERK_TIERS; t++)
            ok = w32(f, p->perkChoice[s][t]);

    // Suç (v7/v8): bölgesel bounty + itibar + son suç günü + NPC yaşam bayrakları
    ok = ok && w32(f, MAX_REGIONS);
    for (int r = 0; ok && r < MAX_REGIONS; r++)
        ok = w32(f, p->bounty[r]) && wf(f, p->reputation[r]);
    ok = ok && w32(f, p->lastCrimeDay);
    ok = ok && w32(f, NPC_COUNT);
    for (int i = 0; ok && i < NPC_COUNT; i++)
        ok = w32(f, npc_is_alive(i) ? 1 : 0);

    // Öğrenilen blueprint tarifleri (v10) — craft modülünden okunur
    ok = ok && w32(f, RECIPE_COUNT);
    for (int i = 0; ok && i < RECIPE_COUNT; i++)
        ok = w32(f, craft_recipe_learned(i) &&
                    RECIPES[i].blueprintItem > 0 ? 1 : 0);

    fclose(f);
    if (!ok)
        SDL_Log("Save yazilamadi (disk dolu?): %s", path);
    return ok;
}

bool load_game(const char *path, Player *p,
               Inventory *inv, Equipment *eq, Hotbar *hb,
               QuestStatus *quests)
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
    Hotbar tmpHb = {0};
    QuestStatus tmpQ[MAX_QUESTS];
    memset(tmpQ, 0, sizeof(tmpQ));

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

    // Hotbar (v2): kind + referans (ability ya da item indeksi)
    ok = ok && r32(f, &n);
    for (int i = 0; ok && i < n; i++) {
        Sint32 kind, ref;
        ok = r32(f, &kind) && r32(f, &ref);
        if (!ok || i >= HOTBAR_SLOTS) continue;
        if (kind == HOTBAR_SKILL && ref >= 0 && ref < ABILITY_COUNT) {
            tmpHb.slots[i].kind = HOTBAR_SKILL;
            tmpHb.slots[i].abilityId = (AbilityId)ref;
        } else if (kind == HOTBAR_ITEM && ref >= 0 && ref < ITEM_TYPE_COUNT) {
            tmpHb.slots[i].kind = HOTBAR_ITEM;
            tmpHb.slots[i].itemType = &ITEM_TYPES[ref];
        }
        // tanınmayan kind/ref → slot boş kalır (bozuk kayda tolerans)
    }

    // Görev durumları (v3)
    ok = ok && r32(f, &n);
    for (int i = 0; ok && i < n; i++) {
        Sint32 state, progress;
        ok = r32(f, &state) && r32(f, &progress);
        if (ok && i < MAX_QUESTS &&
            state >= QUEST_NOT_STARTED && state <= QUEST_REWARDED) {
            tmpQ[i].state = (QuestState)state;
            tmpQ[i].progress = progress;
        }
    }

    // Dünya saati (v4)
    Sint32 tmpDay;
    float tmpHours;
    ok = ok && r32(f, &tmpDay) && rf(f, &tmpHours);

    // Perk seçimleri geçici matrisi (v6)
    for (int s = 0; s < SKILL_COUNT; s++)
        for (int t = 0; t < PERK_TIERS; t++)
            tmpP.perkChoice[s][t] = PERK_UNCHOSEN;

    // Keşif haritası (v5)
    static Uint8 exploredBuf[EXPLORED_BUF_CAP];
    Sint32 exploredBytes = 0;
    ok = ok && r32(f, &exploredBytes);
    if (ok && (exploredBytes < 0 || exploredBytes > (Sint32)EXPLORED_BUF_CAP))
        ok = false;   // bozuk boyut
    if (ok && exploredBytes > 0)
        ok = fread(exploredBuf, (size_t)exploredBytes, 1, f) == 1;

    // Perk seçimleri (v6): boyutlar da okunur (ileride skill/tier artarsa tolerans)
    Sint32 nSkill, nTier;
    ok = ok && r32(f, &nSkill) && r32(f, &nTier);
    for (int s = 0; ok && s < nSkill; s++)
        for (int t = 0; ok && t < nTier; t++) {
            Sint32 c;
            ok = r32(f, &c);
            if (ok && s < SKILL_COUNT && t < PERK_TIERS &&
                c >= PERK_UNCHOSEN && c < PERK_OPTIONS)
                tmpP.perkChoice[s][t] = (Sint8)c;
        }

    // Suç (v7/v8): bölgesel bounty/itibar + son suç günü + NPC yaşam bayrakları
    Sint32 nRegion = 0;
    ok = ok && r32(f, &nRegion);
    if (ok && (nRegion < 0 || nRegion > MAX_REGIONS))
        ok = false;
    for (int r = 0; ok && r < nRegion; r++)
        ok = r32(f, &tmpP.bounty[r]) && rf(f, &tmpP.reputation[r]);
    ok = ok && r32(f, &tmpP.lastCrimeDay);

    Sint32 nNpc = 0;
    Sint32 npcAliveFlag[16];
    ok = ok && r32(f, &nNpc);
    if (ok && (nNpc < 0 || nNpc > 16))
        ok = false;   // bozuk sayı
    for (int i = 0; ok && i < nNpc; i++)
        ok = r32(f, &npcAliveFlag[i]);

    // Öğrenilen tarifler (v10)
    Sint32 nRecipe = 0;
    Sint32 recipeFlag[MAX_RECIPES];
    ok = ok && r32(f, &nRecipe);
    if (ok && (nRecipe < 0 || nRecipe > MAX_RECIPES))
        ok = false;
    for (int i = 0; ok && i < nRecipe; i++)
        ok = r32(f, &recipeFlag[i]);

    fclose(f);
    if (!ok) {
        SDL_Log("Save okunamadi (bozuk dosya): %s", path);
        return false;
    }

    // Her şey okundu — şimdi güvenle uygula
    *p = tmpP;
    *inv = tmpInv;
    *eq = tmpEq;
    *hb = tmpHb;
    memcpy(quests, tmpQ, sizeof(tmpQ));
    gametime_set(tmpDay, tmpHours);
    if (exploredBytes > 0)
        minimap_unpack_explored(exploredBuf, exploredBytes);
    npc_reset();   // önce herkes canlı, sonra save'deki ölümler uygulanır
    for (int i = 0; i < nNpc && i < NPC_COUNT; i++)
        npc_set_alive(i, npcAliveFlag[i] != 0);
    craft_reset_learned();
    for (int i = 0; i < nRecipe && i < RECIPE_COUNT; i++)
        if (recipeFlag[i])
            craft_learn_recipe(i);
    return true;
}
