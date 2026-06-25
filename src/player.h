#ifndef PLAYER_H
#define PLAYER_H
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "common.h"

// LPC sprite sheet sabitleri
#define FRAME_W 64           // LPC kare 64×64
#define FRAME_H 64
#define LPC_SHEET_COLS 13    // sheet 13 sütun

// LPC'de karakterin ayağı hücrenin neresinde (collision/feet hizası için)
// LPC karakterleri 64px hücrede, ayak ~y=58 civarı (ölçeceğiz, şimdilik tahmin)
#define SPRITE_FEET_Y 58

#define PLAYER_SPEED 500.0f
#define SPAWN_X 300.0f
#define SPAWN_Y 300.0f
#define FRAME_DURATION 0.1f
#define PLAYER_ATTACK_RANGE 80.0f
#define PLAYER_ATTACK_HIT_FRAME 3   // slash'ın vuruş karesi (6 karelik slash'ta ~3)

// LPC yön sırası (sheet satır sırası): 0=yukarı, 1=sol, 2=aşağı, 3=sağ
typedef enum {
    DIR_UP = 0,
    DIR_LEFT = 1,
    DIR_DOWN = 2,
    DIR_RIGHT = 3,
} Direction;

// Oyuncunun animasyon durumu (hangi LPC animasyonu)
typedef enum {
    PLAYER_IDLE,    // dururken (satır 22-25, 2 kare)
    PLAYER_WALK,    // yürüme (satır 8-11, 9 kare)
    PLAYER_SLASH,   // kılıç saldırısı (satır 12-15, 6 kare)
    // ileride: THRUST, SHOOT, SPELLCAST, HURT, RUN, DEATH...
} PlayerState;

// Karakterin TÜM kalıcı durumu
typedef struct {
    SDL_FRect rect;      // dünya konumu + mantıksal boyut (ayak izi)
    float drawW, drawH;  // ekranda çizim boyutu

    Direction facing;    // 4 yön (LPC: UP/LEFT/DOWN/RIGHT)
    PlayerState state;   // mevcut animasyon durumu

    int currentFrame;    // animasyon karesi
    float animTimer;     // kare ilerleme sayacı
    PlayerState lastState; // geçiş tespiti: önceki durum
    Direction lastFacing;  // geçiş tespiti: önceki yön

    float speed;
    float health;
    float maxHealth;

    // Saldırı durumu
    bool attacking;
    bool attackHit;
    int targetMobId;
} Player;

void player_render(const Player *p, SDL_Renderer *renderer, SDL_Texture **layers, int layerCount, const Camera *camera);
void player_update(Player *p, const bool *keys, bool attackPressed, float dt);
void health_bar_render(const Player *p, SDL_Renderer *renderer);

#endif