#ifndef DIALOG_H
#define DIALOG_H
#include "r2d.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

// Diyalog sistemi. Veri assets/dialogs/*.txt dosyalarından yüklenir:
//
//   [dugum_adi]
//   text: NPC'nin soyledigi metin (tek satir, UI'da sarilir)
//   opt: Secenek metni -> hedef
//   req: speech >= 25        (bir ustteki opt'a kosul — speech/gold/rep/bounty, >= veya <=)
//   fx: gold -= 50           (secilince efekt — gold/rep/bounty; +=, -=, =)
//
// Hedef ya başka bir düğüm adıdır ya da ÖZEL EYLEM (main işler):
//   end            → diyalogu kapat
//   shop           → dükkanı aç
//   accept:ID      → görevi başlat (quest.h id'si)
//   complete:ID    → görevi teslim et (ödül main'de)
// Düğüm adı eylemlerle çakışmamalı. Seçim 1-9 tuşlarıyla.
//
// Koşul kuralları (değerlendirme main'de — modül yalnız veri taşır):
//   speech: ZAR — eşik üstü %100, altı (speech/eşik)×%70; başarısızlık oturumda kilitler
//   gold/rep/bounty: SERT kilit — sağlanmazsa seçenek soluk, seçilemez

#define DIALOG_MAX_NODES 32
#define DIALOG_MAX_OPTS 5   // S3: 3 yorum + Speech + ipucu
#define DIALOG_ID_LEN 32
#define DIALOG_TEXT_LEN 220
#define DIALOG_OPT_LEN 80
#define DIALOG_MAX_REQS 3   // seçenek başına koşul/efekt tavanı
#define DIALOG_MAX_FX 3

typedef enum {
    DVAR_SPEECH,   // Speech skill seviyesi (zar)
    DVAR_GOLD,     // oyuncu altını
    DVAR_REP,      // NPC bölgesindeki itibar
    DVAR_BOUNTY,   // NPC bölgesindeki bounty
    DVAR_KABA,     // kişilik izi: kaba/sert (gizli sayaç, Faz B1)
    DVAR_KIBAR,    // kişilik izi: kibar/yardımsever
    DVAR_KETUM,    // kişilik izi: ketum/mesafeli
    DVAR_HINT,     // S3: Zehra ipuçları alındı mı (storyFlags[SF_S3_HINTS])
    DVAR_EVENT,    // aktif dünya olayı (WorldEvent: 0 yok..5 mezarlık) — salt okunur
    DVAR_WEATHER,  // aktif hava (Weather: 0 açık 1 yağmur 2 sis 3 fırtına) — salt okunur
} DialogVar;

typedef struct {
    DialogVar var;
    bool lessEq;    // true: <=, false: >=
    float value;
} DialogReq;

typedef struct {
    DialogVar var;  // DVAR_SPEECH efekt olamaz (parser reddeder)
    char op;        // '+', '-', '='
    float value;
} DialogFx;

typedef struct {
    char text[DIALOG_OPT_LEN];
    char target[DIALOG_ID_LEN];   // düğüm adı ya da eylem
    DialogReq reqs[DIALOG_MAX_REQS];
    int reqCount;
    DialogFx fx[DIALOG_MAX_FX];
    int fxCount;
} DialogOpt;

// Seçeneğin ANLIK durumu — main hesaplar, render gösterir
typedef struct {
    bool locked;      // sert koşul sağlanmadı → soluk, seçilemez
    bool hidden;      // dünya-durumu koşulu tutmadı (event/hava) → hiç çizilmez
                      // (numara tuşları ham indeksle eşleşir — satır atlanır)
    bool burned;      // bu konuşmada zar başarısız oldu → kilit
    int chancePct;    // speech zarı yüzdesi (100 = garanti, -1 = speech koşulu yok)
} DialogOptState;

typedef struct {
    char id[DIALOG_ID_LEN];
    char text[DIALOG_TEXT_LEN];
    DialogOpt opts[DIALOG_MAX_OPTS];
    int optCount;
} DialogNode;

typedef struct {
    DialogNode nodes[DIALOG_MAX_NODES];
    int nodeCount;
} Dialog;

// Dosyayı yükle. Format hatasında false (satır SDL_Log'a yazılır).
bool dialog_load(Dialog *d, const char *path);

// Düğüm adı → indeks (-1 = yok)
int dialog_find(const Dialog *d, const char *id);

// Alt panel: portre + NPC adı + metin (typewriter) + numaralı seçenekler.
// states: seçenek başına durum (NULL = hepsi açık); kilitliler soluk +
// speech eşiği/şansı etiketli çizilir.
// sheet: NPC sprite sheet'i (NULL = portresiz) — idle-down ilk karenin baş
// bölgesi kırpılıp panel soluna büyütülür. frameSize: sheet hücre boyutu.
// ESO sunumu: mouseX/Y hover vurgusu için; tıklama dialog_opt_row_at ile
int dialog_opt_row_at(float mx, float my);
void dialog_render(const Dialog *d, int nodeIdx, const char *npcName,
                   const DialogOptState *states,
                   R2D *renderer, TTF_Font *font, int winW, int winH,
                   R2DTexture *sheet, float frameSize,
                   float mouseX, float mouseY);

// --- Typewriter (anlatı Faz A): metin akarak yazılır, ATLANMAZ ama E ile
// tamamlanır. Seçenekler metin bitmeden görünmez/seçilemez. ---
#define DIALOG_CPS 55.0f   // karakter/sn (diyalog vinyetten hızlı — okuma temposu)

void dialog_type_reset(void);    // diyalog AÇILIRKEN çağrılır (düğüm geçişi otomatik)
void dialog_type_update(const Dialog *d, int nodeIdx, float dt);
void dialog_type_reveal(void);   // E: metni anında tamamla
bool dialog_type_done(const Dialog *d, int nodeIdx);

// Seçeneğin speech eşiği (varsa). Dönen: var mı; outThreshold'a eşik yazılır.
bool dialog_opt_speech_req(const DialogOpt *o, float *outThreshold);

#endif
