#ifndef MUSIC_H
#define MUSIC_H
#include <stdbool.h>

// Arka plan müziği — ruh hali (mood) sistemine göre parça seçer, yumuşak
// crossfade ile geçer. Parçalar prosedürel üretildi (assets/audio/music_*.wav).
// audio.c gibi saf SDL3 stream: her parça kendi stream'inde, gain ile fade.
//
// Mood kaynakları (main karar verir):
//   TOWN   → köy içi (güvenli, huzurlu)
//   WILD   → açık arazi gündüz (keşif)
//   DANGER → tehlikeli bölgeler + geceleyin açık arazi (tekinsiz)
//   COMBAT → yakında seni kovalayan/saldıran mob var (gergin)

typedef enum {
    MOOD_TOWN,
    MOOD_WILD,
    MOOD_DANGER,
    MOOD_COMBAT,
    MOOD_DUNGEON,   // zindan katları — derin dron (Cila)
    MOOD_NIGHT,     // açık arazi gecesi — koyu pad (DANGER'dan ayrıştı)
    MOOD_MENU,      // başlık + ana menü — mühür nabzı
    MOOD_COUNT,
} MusicMood;

// Parçaları yükle. Başarısızlık oyunu durdurmaz (müziksiz devam).
bool music_init(void);

// Hedef mood'u değiştir (aynıysa no-op; geçiş crossfade ile)
void music_set_mood(MusicMood m);

// Anlatı kısması: 1.0 normal, 0.15 vinyet — yumuşak geçişle uygulanır
// ("duyular kelimelerden önce": sahneler müziği çekip alanı fısıltıya bırakır)
void music_set_duck(float duck01);

// Her kare: fade ilerlet + biten kuyrukları besle (loop). Menüde de çağrılır.
void music_update(float dt);
void music_set_volume(float v01);   // kullanıcı müzik seviyesi (0..1, ayarlar)

void music_quit(void);

#endif
