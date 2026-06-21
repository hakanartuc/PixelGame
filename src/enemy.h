#ifndef ENEMY_H
#define ENEMY_H

#include <SDL3/SDL.h>
#include <stdbool.h> 
#include "common.h"


#define MAX_ENEMIES 10

// Dünyada var olan, hareket eden, hasar alabilen her şeyin ortak çekirdeği.
typedef struct {
    SDL_FRect rect;            // konum + boyut (collision/hurtbox)
    float health, maxHealth;   // can
    bool alive;                // yaşıyor mu
    int facing;                // 0=aşağı, 1=yukarı, 2=yan
    bool facingLeft;
    float speed;               // hareket hızı
    float hitFlash;            // vuruş flaşı (saniye, geri sayar)
    float knockbackX, knockbackY;  // geri itilme hızı
} Entity;

// Düşmanın davranış durumu
typedef enum {
    ENEMY_IDLE,    // bekliyor
    ENEMY_CHASE,   // oyuncuyu kovalıyor
    ENEMY_ATTACK,  // saldırı menzilinde
} EnemyState;

// Düşman = ortak çekirdek + AI'a özel alanlar
typedef struct {
    Entity e;                 // ortak çekirdek (konum, can, hareket, vuruş tepkisi)
    EnemyState state;         // AI durumu
    float attackCooldown;     // bir sonraki saldırıya kalan süre
    bool attackTriggered;
    // animasyon
    int animRow;          // o an çizilen sheet satırı
    int currentFrame;     // hangi karedeyiz
    float animTimer;      // kare ilerleme sayacı
    bool attacking;       // şu an bir saldırı animasyonu oynuyor mu?
    int lastRow;          // YENİ — animasyon değişince currentFrame sıfırlamak için
} Enemy;

void enemy_render(const Enemy *d, SDL_Renderer *renderer, SDL_Texture *texture, const Camera *camera);
void enemy_update(Enemy *d, float targetX, float targetY, float dt);

#endif