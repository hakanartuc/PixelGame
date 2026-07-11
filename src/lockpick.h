#ifndef LOCKPICK_H
#define LOCKPICK_H
#include "r2d.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "perk.h"

// Kilit açma minigame'i: bar üzerinde salınan imleç, yeşil bölgede Space.
// Skill + ağaç bölgeyi büyütür, imleci yavaşlatır. Başarısızlık maymuncuk
// kırar + GÜRÜLTÜ çıkarır (yakın mobların şüphesi dolar — main işler).
// Kademeler: 0 Basit / 1 Sağlam / 2 Usta (K1 sandık / K2 sandık / boss+nadir).

#define LOCK_TIERS 3

// Minigame'i başlat. lootIdx: açılmayı bekleyen sandığın loots[] indeksi.
// KİLİT FISILDAYAN usta kilidi bir kademe düşürür (fx bakılır).
void lockpick_start(int tier, int lootIdx, const PerkEffects *fx);
bool lockpick_active(void);
int  lockpick_loot(void);        // hedef sandık indeksi
int  lockpick_tier(void);        // efektif kademe (keystone düşürmüş olabilir)

void lockpick_update(float dt, const PerkEffects *fx);
void lockpick_cancel(void);

// Space basıldı: 1 = açıldı, -1 = kırıldı, 0 = aktif değil.
// skillLevel bölgeye küçük ek verir (+%0.4/seviye).
int lockpick_press(const PerkEffects *fx, int skillLevel);

// Bar + bölge + imleç + kademe etiketi (ekran ortası altı).
// fx+skill: yeşil bölge press ile aynı hesapla çizilir (görsel dürüstlük)
void lockpick_render(R2D *renderer, TTF_Font *font, int winW, int winH,
                     const PerkEffects *fx, int skillLevel);

#endif
