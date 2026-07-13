#ifndef MENU_H
#define MENU_H
#include "r2d.h"
#include <SDL3/SDL.h>
#include "font.h"
#include "save.h"   // SaveSummary + SAVE_SLOT_COUNT

// Oyunun ust-duzey durumu.
typedef enum {
    GAME_TITLE,      // atmosferik baslik ekrani ("bir tusa bas") — Faz B1
    GAME_MENU,       // ana menu (Yeni Oyun / Yukle / Cikis)
    GAME_PLAYING,    // normal oynanis
    GAME_PAUSED,     // ESC menusu acik, dunya donmus
} GameState;

// Menu ekranlari (hem ana menu hem ESC menusu bu makineden gecer).
typedef enum {
    MENU_MAIN,              // ESC menusu (Devam/Kaydet/Yukle/Ana Menu/Cik)
    MENU_CONFIRM_QUIT,      // "Emin misin?" (oyundan cikis)
    MENU_CONFIRM_MAINMENU,  // "Kaydedilmemis ilerleme kaybolur" (ana menuye don)
    MENU_ROOT,              // ana menu (Yeni Oyun / Yukle / Cikis)
    MENU_SLOTS_NEW,         // yeni oyun icin slot secimi
    MENU_SLOTS_SAVE,        // kaydet icin slot secimi
    MENU_SLOTS_LOAD,        // yukle icin slot secimi (+ otomatik kayit)
    MENU_CONFIRM_OVERWRITE, // dolu slota YENI OYUN yazma onayi
} MenuScreen;

// Menu tiklamasinin sonucu. Slotlu aksiyonlarda slot outSlot'a yazilir.
typedef enum {
    MENU_ACTION_NONE,       // bir sey olmadi / ekran ici gecis
    MENU_ACTION_RESUME,     // oyuna don (ESC menusu)
    MENU_ACTION_QUIT,       // programdan cik
    MENU_ACTION_NEW_GAME,   // secilen slotta yeni oyun baslat
    MENU_ACTION_SAVE_SLOT,  // secilen slota kaydet
    MENU_ACTION_LOAD_SLOT,  // secilen slottan yukle
    MENU_ACTION_TO_MAINMENU,// ana menuye don (onaylandi)
} MenuAction;

// Menu cizimi. sums: SAVE_SLOT_COUNT+1 ozet (son eleman otomatik kayit) —
// yalniz slot ekranlarinda okunur, digerlerinde NULL olabilir.
void menu_render(R2D *renderer, TTF_Font *font,
                 MenuScreen screen, int winW, int winH,
                 const SaveSummary *sums);

// Tiklama testi. screen'i degistirebilir (ekran gecisleri menu icinde cozulur).
// Slotlu aksiyon donerse secilen slot *outSlot'a yazilir.
MenuAction menu_handle_click(MenuScreen *screen, float mx, float my,
                             int winW, int winH,
                             const SaveSummary *sums, int *outSlot);

#endif
