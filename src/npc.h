#ifndef NPC_H
#define NPC_H
#include "r2d.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "common.h"

// NPC'ler: dünyada sabit duran, E ile konuşulan karakterler.
// Görsel: tek sheet (LPC düzeni varsayılır, idle satırı oynar).
// Şimdilik yer tutucu olarak oyuncunun base sheet'i — gerçek NPC sprite'ı
// gelince NPCS[] tablosunda sheetPath değişir, kod değişmez.

#define NPC_INTERACT_RANGE 90.0f   // E prompt'unun çıktığı mesafe
#define NPC_MAX_STOPS 4            // rutindeki max durak
#define NPC_MAX_HEALTH 60.0f       // şimdilik tüm NPC'ler aynı (ileride NpcDef alanı)

// Rutin durağı: bu saatten itibaren NPC bu noktaya yürür.
// indoors = varınca içeri girer (görünmez + konuşulamaz) — gece evi.
typedef struct {
    float hour;      // 0..24
    float x, y;
    bool indoors;
} NpcStop;

typedef struct {
    const char *name;         // baş üstü + diyalog panelinde
    const char *sheetPath;    // sprite sheet (Cute_Fantasy NPC düzeni)
    const char *dialogFile;   // assets/dialogs/*.txt
    bool isVendor;            // dükkan açabilir (shop hedefi çalışır)
    const char *questId;      // verdiği görev (NULL = yok) — giriş düğümünü seçer
    NpcStop schedule[NPC_MAX_STOPS];
    int stopCount;            // en az 1 (ilk durak = başlangıç konumu)

    // Sheet/görünüm farkları (0 = varsayılan: 64px hücre, 1.5x, NPC_MAX_HEALTH)
    int frameSize;            // hücre boyutu (Knights sheet'leri 48)
    float drawScale;
    float maxHealth;
    bool isGuard;             // muhafız: rutin yerine guard.c beyni yönetir
} NpcDef;

extern const NpcDef NPCS[];
extern const int NPC_COUNT;

// Harita geçişi: dünyada değilken NPC'ler tamamen devre dışı
// (update/render/nearest/visible hepsi boş döner)
void npc_set_active(bool active);

// Rutin: saate göre hedef durak seç, oraya yürü, indoors'ta gizlen.
void npc_update(float dt, float hours);

// Tüm NPC'leri çiz (idle animasyon + isim; en yakındakine E prompt'u).
// nearestIdx: npc_nearest_in_range sonucu (-1 = prompt yok).
void npc_render_all(R2D *renderer, R2DTexture **npcTextures,
                    const Camera *camera, TTF_Font *font, int nearestIdx);

// Oyuncuya NPC_INTERACT_RANGE içindeki en yakın KONUŞULABILIR NPC (-1 = yok).
// İçerideki (indoors) NPC seçilmez — gece dükkan da böyle kapanır.
int npc_nearest_in_range(float px, float py);

// Olay NPC'si (kervan): dünyada görünürlüğü main açar/kapar (rutinden bağımsız)
void npc_force_hidden(int i, bool hidden);

// Hava sığınması: yağmur/fırtınada köylüler evlerine çekilir (muhafız nöbette,
// exemptIdx — kervancı gölgeliği — dışarıda kalır). Main her kare basar.
void npc_set_shelter(bool s, int exemptIdx);

// Diyalogdaki NPC (her kare main basar; -1 = yok): konuşan NPC rutinini
// DURDURUR — oyuncuyla konuşurken yürüyüp gitmesin (doğallık düzeltmesi)
void npc_set_talking(int i);

// --- Sahne araçları (D1a) ---
// Bark: NPC başının üstünde kısa konuşma balonu (süreli; yenisi eskisini ezer)
void npc_bark(int i, const char *text, float seconds);
// Yürütme scripti: rutini geçersiz kıl, noktaya yürü (varınca bitir).
// Dönen (npc_walk_done): hedefe vardı mı — sahne akışı bekleyebilsin.
void npc_walk_to(int i, float x, float y);
bool npc_walk_done(int i);
void npc_walk_cancel(int i);   // rutine geri dön

// NPC'nin anlık konumu (minimap vb. için). İçerideyse/ölüyse false (görünmez).
bool npc_visible_pos(int i, float *x, float *y);

// --- Savaş köprüsü (suç sistemi) ---
// Tüm NPC'leri diriltir + canları doldurur. Oyun başında ve load öncesi çağrılır.
void npc_reset(void);

// Dünya noktasındaki görünür NPC (çizim kutusu hit-testi). -1 = yok.
int npc_at_point(float wx, float wy);

// Hasar uygula. Dönen: bu vuruşla öldü mü. Ölüm kalıcıdır (save'e yazılır).
bool npc_damage(int i, float dmg);

bool  npc_is_alive(int i);
float npc_health(int i);
float npc_max_health(int i);       // def'ten (0 ise NPC_MAX_HEALTH)
void  npc_set_alive(int i, bool alive);   // save/load köprüsü

// Muhafız beyni (guard.c) için konum erişimi — rutin bu NPC'yi yönetmez
bool npc_pos(int i, float *x, float *y);         // ölü de olsa son konum
void npc_set_pos(int i, float x, float y);

#endif
