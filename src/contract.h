#ifndef CONTRACT_H
#define CONTRACT_H
#include "r2d.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "common.h"
#include "mob.h"
#include "inventory.h"

// İlan panosu (Faz 6 — Witcher tarzı): köy meydanında pano, günlük 3 kontrat.
// Havuz 3 kademeli (Temel / Güvenilir / Saygın) — kademe itibarla açılır,
// tamamlamak altın + bölge itibarı verir (kendini besleyen döngü).
// Gün dönünce seçim değişir, durumlar sıfırlanır (save v12 köprüsü).

#define CONTRACT_TIERS 3
#define CONTRACT_PER_TIER 3          // havuz: kademe başına 3 ilan (satır ekle → büyür)
#define CONTRACT_ACTIVE 3            // panoda aynı anda: her kademeden 1

// Kademe itibar eşikleri (indeks = kademe)
extern const float CONTRACT_TIER_REP[CONTRACT_TIERS];
extern const char *const CONTRACT_TIER_NAMES[CONTRACT_TIERS];

typedef struct {
    const char *name;      // ilan başlığı ("Goblin sorunu")
    int tier;              // 0 Temel, 1 Güvenilir, 2 Saygın
    const MobType *targetMob;  // KES: hangi tür (NULL = getir kontratı)
    int fetchItem;         // GETİR: ITEM_TYPES indeksi (-1 = kes kontratı)
    int count;             // hedef adet
    int gold;              // ödül altın
    float rep;             // ödül itibar (panonun bölgesine)
} ContractDef;

extern const ContractDef CONTRACTS[];
extern const int CONTRACT_COUNT;

typedef enum {
    CONTRACT_AVAILABLE,    // panoda, alınmamış
    CONTRACT_ACCEPTED,     // alındı, sürüyor
    CONTRACT_DONE,         // teslim edildi (bugünlük kapandı)
} ContractState;

// --- Günün panosu + durum (modül durumu; save v12 buradan okur/yazar) ---
void contract_new_day(int day);            // gün dönüşü: yeni seçim + durum sıfırla
int  contract_today(int slot);             // slot 0-2 → CONTRACTS indeksi (-1 yok)
ContractState contract_state(int slot);
int  contract_progress(int slot);          // kes kontratı kill sayacı
void contract_accept(int slot);
void contract_turn_in(int slot);           // durumu DONE yapar (ödül main'de)
void contract_on_kill(const MobType *type);// kabul edilmiş kes kontratlarını say

// Save köprüsü
int  contract_save_day(void);
void contract_save_get(int slot, int *state, int *progress);
void contract_save_set(int day, const int *states, const int *progress);

// --- Pano paneli (craft panel deseniyle: çizim + tıklama aynı yerleşim) ---
// rep: panonun bölgesindeki itibar (kilit görünümü için)
void contract_panel_render(R2D *renderer, TTF_Font *font, const Inventory *inv,
                           float rep, float panelX, float panelY);
// Tıklanan pano satırı: 0-2 (slot) ya da -1
int contract_panel_slot_at(float x, float y, float panelX, float panelY);

#endif
