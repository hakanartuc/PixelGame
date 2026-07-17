#ifndef JOURNAL_H
#define JOURNAL_H
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "r2d.h"
#include "quest.h"

// "Yolculuk" günlüğü (Faz B2): J paneli artık kitap — iki sekme.
// GÖREVLER: aktif işler birinci ağız. GÜNLÜK: kronolojik yaşanmışlıklar.
// Girişler KOD+PARAM olarak saklanır (save v19) — metin render'da türetilir
// (dil değişse de eski save'ler doğru okunur).

typedef enum {
    JE_WAKE = 0,        // açılış: yol kenarında uyanış
    JE_QUEST_ACCEPT,    // param = quest indeksi
    JE_QUEST_DONE,      // param = quest indeksi
    JE_ECHO_WITNESS,    // param = yankı indeksi (Tanık yolu)
    JE_ECHO_CONSUME,    // param = yankı indeksi (Sömüren yolu)
    JE_ECHO_YAZIT,      // param = yankı indeksi (ilk okuma)
    JE_BOSS_KILL,       // param = 0 (İskelet Lordu)
    JE_DEATH,           // param = 0
    JE_DUNGEON_FIRST,   // param = MapId (ilk giriş)
    JE_DAY_NOTE,        // param = gün (atmosfer satırı — journal_day_tick)
    JE_NEST_CLEANSE,    // param = 0 reis / 1 totem / 2 gölcük / 3 mezar (ekoloji E1)
    JE_K9_CHOICE,       // K9 Kandil: param = 1 gerçek / 2 yalan / 3 sus (D1a)
    JE_K2_CHOICE,       // K2 Son Müşteri: 1 yumuşak / 2 düz / 4 anlatamadı (D1b)
    JE_TITLE,           // ünvan kazanıldı: param = TITLES indeksi (M3)
    JE_S3_STEP,         // S3: 0 Zehra ipuçları · 1 Nuh güveni/konak yolu (D1c)
    JE_S3_CHOICE,       // S3 kapanış: 2 nöbet devralındı · 3 kayıt kapatıldı
    JE_GRAVE_RISE,      // huzursuz mezarlık: mezar yeniden yuva oldu (WE_GRAVE)
    JE_WELL_INTRO,      // Kör Kuyu: Yabancı anlattı (kim olduğunu sormadım — soramadım)
    JE_ECHO_CALL,       // YANKICI: fısıltı taşlara çağırdı (3. sömürü)
    JE_RITUAL,          // YANKICI: ritüel tamamlandı — geri dönüşü yok
} JournalEvent;

typedef struct {
    Sint16 type;    // JournalEvent
    Sint16 param;
    Sint16 day;     // yaşandığı oyun günü
} JournalEntry;

#define JOURNAL_MAX 96   // havuz dolunca yeni giriş sessizce düşer (spam siperi)

void journal_reset(void);
bool journal_add(int type, int param);          // gün otomatik damgalanır
bool journal_add_unique(int type, int param);   // aynı tip+param varsa eklemez
void journal_day_tick(int day);                 // gün dönümü satırları (tablodan)
int  journal_count(void);
const JournalEntry *journal_get(int i);
void journal_load_entry(int type, int param, int day);   // save yükleme köprüsü

// Kitap panelini çiz. tab: 0=Görevler 1=Günlük 2=Ünvanlar; page: yaprak indeksi.
// Dönen: o sekmenin toplam yaprak sayısı (main page'i bununla klample).
// player: Ünvanlar sekmesi (sayaç ilerlemeleri + taşınan) için.
struct Player;
int journal_render(R2D *renderer, TTF_Font *font, R2DTexture *bookTex,
                   const QuestStatus *quests, const struct Player *player,
                   int tab, int page, int winW, int winH);

// Ünvanlar sekmesinde tıklanan ünvan satırı (-1 = yok) — taşınan seçimi için
int journal_title_at(float x, float y, int winW, int winH);

#endif
