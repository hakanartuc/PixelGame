#ifndef WORLDEVENT_H
#define WORLDEVENT_H
#include <stdbool.h>

// DÜNYA OLAYLARI çekirdeği: her şafak %35 şansla GÜNÜN OLAYI (aynısı üst üste
// gelmez). Etkiler main'de işlenir; bu modül yalnız zar + durum + süre tutar.
//   RAID    — gece köy kenarına goblin baskını (öldüren itibar kazanır)
//   CARAVAN — 2 gün meydanda kervan tüccarı (özel stok, %15 pahalı)
//   BOUNTY  — bereket: av sürüleri +2 doğum
//   SEAL    — mühür sancısı: gece yankılar parlar, fısıltı duyulur
//   GRAVE   — huzursuz mezarlık: gece iskeletler yükselir; sabaha
//             temizlenmezse mühürlü/boş mezar YENİDEN yuva olur

typedef enum {
    WE_NONE = 0,
    WE_RAID,
    WE_CARAVAN,
    WE_BOUNTY,
    WE_SEAL,
    WE_GRAVE,
    WE_COUNT,
} WorldEvent;

// Şafak zarı: yeni günü işle; yeni başlayan olayı döner (WE_NONE = olay yok).
// Kervan 2 gün sürer — 2. gün WE_NONE döner ama current CARAVAN kalır.
WorldEvent worldevent_dawn(int day);

WorldEvent worldevent_current(void);   // bugün aktif olay (süre dahil)
const char *worldevent_name(WorldEvent e);
const char *worldevent_rumor(WorldEvent e);   // pano söylentisi satırı

// Save köprüsü (v25)
void worldevent_save_get(int *cur, int *startDay, int *last);
void worldevent_save_set(int cur, int startDay, int last);
void worldevent_reset(void);

#endif
