#ifndef NPC_H
#define NPC_H
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "common.h"

// NPC'ler: dünyada sabit duran, E ile konuşulan karakterler.
// Görsel: tek sheet (LPC düzeni varsayılır, idle satırı oynar).
// Şimdilik yer tutucu olarak oyuncunun base sheet'i — gerçek NPC sprite'ı
// gelince NPCS[] tablosunda sheetPath değişir, kod değişmez.

#define NPC_INTERACT_RANGE 90.0f   // E prompt'unun çıktığı mesafe
#define NPC_MAX_STOPS 4            // rutindeki max durak

// Rutin durağı: bu saatten itibaren NPC bu noktaya yürür.
// indoors = varınca içeri girer (görünmez + konuşulamaz) — gece evi.
typedef struct {
    float hour;      // 0..24
    float x, y;
    bool indoors;
} NpcStop;

typedef struct {
    const char *name;         // baş üstü + diyalog panelinde
    const char *sheetPath;    // sprite sheet (Cute_Fantasy NPC düzeni)
    const char *dialogFile;   // assets/dialogs/*.txt
    bool isVendor;            // dükkan açabilir (shop hedefi çalışır)
    const char *questId;      // verdiği görev (NULL = yok) — giriş düğümünü seçer
    NpcStop schedule[NPC_MAX_STOPS];
    int stopCount;            // en az 1 (ilk durak = başlangıç konumu)
} NpcDef;

extern const NpcDef NPCS[];
extern const int NPC_COUNT;

// Rutin: saate göre hedef durak seç, oraya yürü, indoors'ta gizlen.
void npc_update(float dt, float hours);

// Tüm NPC'leri çiz (idle animasyon + isim; en yakındakine E prompt'u).
// nearestIdx: npc_nearest_in_range sonucu (-1 = prompt yok).
void npc_render_all(SDL_Renderer *renderer, SDL_Texture **npcTextures,
                    const Camera *camera, TTF_Font *font, int nearestIdx);

// Oyuncuya NPC_INTERACT_RANGE içindeki en yakın KONUŞULABILIR NPC (-1 = yok).
// İçerideki (indoors) NPC seçilmez — gece dükkan da böyle kapanır.
int npc_nearest_in_range(float px, float py);

// NPC'nin anlık konumu (minimap vb. için). İçerideyse false (görünmez).
bool npc_visible_pos(int i, float *x, float *y);

#endif
