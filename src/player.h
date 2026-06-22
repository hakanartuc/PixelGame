#ifndef PLAYER_H
#define PLAYER_H

#include <SDL3/SDL.h>
#include <stdbool.h> 
#include "common.h"



// Ayak kutusu: rect'in ALT kısmında, dar bir collision alanı.
// (top-down'da karakter ayağıyla çarpışır, tüm gövdesiyle değil)
#define SPRITE_FEET_Y 40 // sprite hücresinde karakterin ayağının olduğu satır, ölçülen değer
#define PLAYER_SPEED 500.0f

#define SPAWN_X 300.0f
#define SPAWN_Y 300.0f

#define FRAME_W        64
#define FRAME_H        64
#define FRAME_DURATION 0.1f

#define PLAYER_ATTACK_HIT_FRAME 1

// Karakterin TÜM kalıcı durumu tek bir yapıda.
typedef struct{
    SDL_FRect rect;     // DÜNYA konumu + MANTIKSAL boyut (hareket, clamp, ileride çarpışma)
    float drawW, drawH; // EKRANDA çizim boyutu (sadece render kullanır)
    int facing;         // 0=aşağı, 1=yukarı, 2=yan
    bool facingLeft;    // true = sola bakıyor, false = sağa bakıyor
    int animRow;        // o an çizilecek sheet satırı (update yazar, render okur)
    int currentFrame;   // animasyonun hangi karesinde olduğumuz.
    float animTimer;    // kare ilerleme sayacı
    int lastRow;        // önceki karenin animRow'u; animasyon değiştiğinde sıfırlamak için

    // Durum sistemleri
    float speed;        // anlık hareket hızı
    float health;       // anlık can
    float maxHealth;    // maksimum can

    // Saldırı durumu
    bool attacking;       // şu an saldırı animasyonu/penceresi aktif mi?
    bool attackHit;       // bu saldırıda dummy'ye değdi mi (tekrar vurmasın)

} Player;

SDL_FRect attack_hitbox(const Player *p);
void player_render(const Player *p, SDL_Renderer *renderer, SDL_Texture **layers, int layerCount, const Camera *camera);
void player_update(Player *p, const bool *keys, bool attackPressed, float dt);
void health_bar_render(const Player *p, SDL_Renderer *renderer);

#endif // PLAYER_H