#ifndef MENU_H
#define MENU_H
#include "r2d.h"
#include <SDL3/SDL.h>
#include "font.h"

// Oyunun ust-duzey durumu. Ileride GAME_SETTINGS, GAME_SAVE vs. eklenebilir.
typedef enum {
    GAME_PLAYING,    // normal oynanis
    GAME_PAUSED,     // ESC menusu acik, dunya donmus
} GameState;

// Pause menusunun ic durumu (hangi ekran gosteriliyor).
typedef enum {
    MENU_MAIN,         // ana menu (Devam / Cik)
    MENU_CONFIRM_QUIT, // "Emin misin?" onayi
} MenuScreen;

// Menu butonuna tiklaninca donen aksiyon.
typedef enum {
    MENU_ACTION_NONE,    // bir sey olmadi / tiklama bos alana
    MENU_ACTION_RESUME,  // oyuna don
    MENU_ACTION_SAVE,    // oyunu kaydet (main halleder — menu dosya bilmez)
    MENU_ACTION_LOAD,    // kayittan yukle
    MENU_ACTION_QUIT,    // oyundan cik
} MenuAction;

// Pause menusunu cizer (yari saydam karartma + butonlar).
void menu_render(R2D *renderer, TTF_Font *font,
                 MenuScreen screen, int winW, int winH);

// Tiklama testi: verilen ekrandaki hangi butona basildi → aksiyon.
// screen'i degistirebilir (orn. Cik → CONFIRM_QUIT'e gecis) — o yuzden pointer.
MenuAction menu_handle_click(MenuScreen *screen, float mx, float my, int winW, int winH);

#endif