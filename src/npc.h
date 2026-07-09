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

typedef struct {
    const char *name;         // baş üstü + diyalog panelinde
    float x, y;               // dünya konumu (ayak noktası)
    const char *sheetPath;    // LPC sheet (idle satırı çizilir)
    const char *dialogFile;   // assets/dialogs/*.txt
    bool isVendor;            // dükkan açabilir (shop hedefi çalışır)
    const char *questId;      // verdiği görev (NULL = yok) — giriş düğümünü seçer
} NpcDef;

extern const NpcDef NPCS[];
extern const int NPC_COUNT;

// Tüm NPC'leri çiz (idle animasyon + isim; en yakındakine E prompt'u).
// nearestIdx: npc_nearest_in_range sonucu (-1 = prompt yok).
void npc_render_all(SDL_Renderer *renderer, SDL_Texture **npcTextures,
                    const Camera *camera, TTF_Font *font, int nearestIdx);

// Oyuncuya NPC_INTERACT_RANGE içindeki en yakın NPC (-1 = yok)
int npc_nearest_in_range(float px, float py);

#endif
