#ifndef MINIMAP_H
#define MINIMAP_H
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "common.h"
#include "r2d.h"
#include "mob.h"

// Minimap + fog of war: gidilen yerler açılır (tile rengiyle), gerisi karanlık.
// Harita bir kez texture'a işlenir; her kare yalnız YENİ açılan tile'lar eklenir.
// Üstüne canlı noktalar: oyuncu (beyaz), mob (kırmızı), NPC (yeşil).
// M ile açılır/kapanır (main). Keşif durumu save v5'te saklanır.

void minimap_init(R2D *renderer);   // harita boyutunda target texture
void minimap_quit(void);

// Harita geçişinde çağır: texture yeni harita boyutunda kurulur.
// worldPersistent=true → kalıcı dünya keşfi; false → geçici (zindan, sıfırlanır)
void minimap_set_map(R2D *renderer, bool worldPersistent);

// Oyuncunun çevresini keşfet (her kare çağrılır; yeni tile'ları texture'a işler)
void minimap_reveal(R2D *renderer, float worldX, float worldY);

// Sağ-alt köşeye çiz (noktalarla birlikte)
void minimap_render(R2D *renderer, float playerX, float playerY,
                    const Mob *mobs, int mobCount, int winW, int winH);

// --- Save köprüsü (bit-paketli keşif matrisi) ---
// pack: out'a yazar, yazılan byte sayısını döner (cap yetmezse 0).
int  minimap_pack_explored(Uint8 *out, int cap);
// unpack: yükleme sonrası çağrılır; texture bir sonraki render'da yeniden işlenir.
void minimap_unpack_explored(const Uint8 *in, int bytes);
// yeni oyun: tüm keşif silinir (dünya + geçici)
void minimap_reset_explored(void);

#endif
