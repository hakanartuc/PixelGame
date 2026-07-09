#ifndef DIALOG_H
#define DIALOG_H
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

// Diyalog sistemi. Veri assets/dialogs/*.txt dosyalarından yüklenir:
//
//   [dugum_adi]
//   text: NPC'nin soyledigi metin (tek satir, UI'da sarilir)
//   opt: Secenek metni -> hedef
//
// Hedef ya başka bir düğüm adıdır ya da ÖZEL EYLEM (main işler):
//   end            → diyalogu kapat
//   shop           → dükkanı aç
//   accept:ID      → görevi başlat (quest.h id'si)
//   complete:ID    → görevi teslim et (ödül main'de)
// Düğüm adı eylemlerle çakışmamalı. Seçim 1-9 tuşlarıyla.

#define DIALOG_MAX_NODES 32
#define DIALOG_MAX_OPTS 4
#define DIALOG_ID_LEN 32
#define DIALOG_TEXT_LEN 220
#define DIALOG_OPT_LEN 80

typedef struct {
    char text[DIALOG_OPT_LEN];
    char target[DIALOG_ID_LEN];   // düğüm adı ya da eylem
} DialogOpt;

typedef struct {
    char id[DIALOG_ID_LEN];
    char text[DIALOG_TEXT_LEN];
    DialogOpt opts[DIALOG_MAX_OPTS];
    int optCount;
} DialogNode;

typedef struct {
    DialogNode nodes[DIALOG_MAX_NODES];
    int nodeCount;
} Dialog;

// Dosyayı yükle. Format hatasında false (satır SDL_Log'a yazılır).
bool dialog_load(Dialog *d, const char *path);

// Düğüm adı → indeks (-1 = yok)
int dialog_find(const Dialog *d, const char *id);

// Alt panel: NPC adı + metin (sarılı) + numaralı seçenekler
void dialog_render(const Dialog *d, int nodeIdx, const char *npcName,
                   SDL_Renderer *renderer, TTF_Font *font, int winW, int winH);

#endif
