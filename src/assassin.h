#ifndef ASSASSIN_H
#define ASSASSIN_H
#include "r2d.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

// SESSİZ EL: lonca kontrat sistemi + rütbe (Suikastçi P3).
// Kontratlar KURGULU (rastgele değil) — K1 kademesi: dünyaya olay olarak
// doğan hedefler (haydut reisleri, Kaçak Tahsildar). Rütbe kontrat puanıyla
// yükselir; loca dükkanı + kuyu aklama oranı rütbeye göre açılır.

typedef struct {
    const char *name;       // kontrat başlığı
    const char *desc;       // ESO tarzı brifing (loca ağzından)
    int targetMob;          // MOB_TYPES indeksi (öldürülecek)
    int count;              // kaç adet
    int rewardGold;         // tamamlama altını
    int rankPoints;         // rütbe puanı
    int minRank;            // 0=Çırak · bu rütbenin altında görünmez
    const char *where;      // "nerede/nasıl" ipucu (J sekmesi + tahta)
} AssassinContract;

extern const AssassinContract ASSASSIN_CONTRACTS[];
extern const int ASSASSIN_CONTRACT_COUNT;

// Rütbe: 0 Çırak · 1 El · 2 Bıçak · 3 Gölge · 4 Usta El
int  assassin_rank(int points);
const char *assassin_rank_name(int rank);
int  assassin_next_threshold(int rank);   // sonraki rütbe puanı (-1 = son)

// Kontrat tahtası paneli (loca). active/progress: aktif kontrat + ilerleme
// (-1 = yok). ready: aktif kontrat tamamlandı, ödül bekliyor.
void assassin_board_render(int playerPoints, int active, int progress,
                           bool ready, R2D *renderer, TTF_Font *font,
                           float px, float py);
// Tıklanan satır: 0..N-1 kontrat indeksi · -2 "ödülü al" · -1 yok
#define ASSASSIN_ROW_CLAIM (-2)
int assassin_board_row_at(int playerPoints, int active, bool ready,
                          float x, float y, float px, float py);

#endif
