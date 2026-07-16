#include "worldevent.h"
#include "common.h"   // randf

static WorldEvent current = WE_NONE;
static int startDay = -1;
static WorldEvent lastEvent = WE_NONE;   // aynı olay üst üste gelmesin

void worldevent_reset(void)
{
    current = WE_NONE;
    startDay = -1;
    lastEvent = WE_NONE;
}

WorldEvent worldevent_dawn(int day)
{
    // Süren olay: kervan 2 gün kalır, gerisi tek gün
    if (current == WE_CARAVAN && day - startDay < 2)
        return WE_NONE;   // devam ediyor, yeni olay yok
    current = WE_NONE;

    if (randf() >= 0.35f) return WE_NONE;

    // Son olayla aynı gelmesin: havuzdan çek
    WorldEvent pick;
    int guard = 8;
    do {
        pick = (WorldEvent)(1 + (int)(randf() * (WE_COUNT - 1)));
        if (pick >= WE_COUNT) pick = WE_SEAL;
    } while (pick == lastEvent && guard-- > 0);

    current = pick;
    startDay = day;
    lastEvent = pick;
    return pick;
}

WorldEvent worldevent_current(void) { return current; }

const char *worldevent_name(WorldEvent e)
{
    switch (e) {
        case WE_RAID:    return "Goblin Baskını";
        case WE_CARAVAN: return "Gezgin Kervan";
        case WE_BOUNTY:  return "Bereket";
        case WE_SEAL:    return "Mühür Sancısı";
        case WE_GRAVE:   return "Huzursuz Mezarlık";
        default:         return "";
    }
}

const char *worldevent_rumor(WorldEvent e)
{
    switch (e) {
        case WE_RAID:    return "Goblinler gece köye sarkacakmış — kapıları sıkı tutun.";
        case WE_CARAVAN: return "Meydana kervan indi! Nadir mal varmış, pahalıymış ama.";
        case WE_BOUNTY:  return "Sürüler bereketli bu mevsim — otlaklar dolu.";
        case WE_SEAL:    return "Bu gece taşların dibinden ses geliyor diyorlar...";
        case WE_GRAVE:   return "Mezarlıkta kıpırtı görülmüş — ölüler rahat durmuyor.";
        default:         return "";
    }
}

void worldevent_save_get(int *cur, int *sd, int *last)
{
    *cur = (int)current;
    *sd = startDay;
    *last = (int)lastEvent;
}

void worldevent_save_set(int cur, int sd, int last)
{
    current = (cur >= 0 && cur < WE_COUNT) ? (WorldEvent)cur : WE_NONE;
    startDay = sd;
    lastEvent = (last >= 0 && last < WE_COUNT) ? (WorldEvent)last : WE_NONE;
}
