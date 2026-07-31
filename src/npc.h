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
    // KİMLİK — kod bu alanla arar, ASLA name ile. Biçim: <harita>_<yer>_<rol>
    // (ör. "dunya_koy_demirci", "loca_salon_usta", "tasdurak_han_hanci").
    //
    // Neden ad DEĞİL: 13 yerde `SDL_strcmp(NPCS[i].name, "Fatma")` vardı. Görünen
    // ad hem gösterim hem kimlik olunca, adı değiştirmek görev zincirini SESSİZCE
    // koparıyordu — derleyici susar, oyun çökmez, NPC sadece konuşmaz. DEMİRBOYUN
    // kanonuna geçerken tüm adlar değişeceği için bu bomba mutlaka önce sökülmeliydi.
    //
    // Neden ROL, ad değil: ad değişse bile rol aynı kalır ("demirci" kim olursa
    // olsun demircidir). Neden ÖNEK: altı şehirli kanonda iki demirci olacak;
    // harita+yer öneki tekilliği yapı gereği sağlıyor.
    //
    // Save'e GİRMEZ: kayıtta NPC yok, görevler QUESTS[] sırasıyla tutuluyor —
    // yani id'ler serbestçe yeniden adlandırılabilir (tek commit + grep).
    // İleride çeviri anahtarı da bu olacak: npc.<id>.name
    const char *id;
    const char *name;         // YALNIZ gösterim: baş üstü + diyalog panelinde
    const char *sheetPath;    // sprite sheet (Cute_Fantasy NPC düzeni)
    const char *dialogFile;   // assets/dialogs/*.txt
    bool isVendor;            // dükkan açabilir (shop hedefi çalışır)
    const char *questId;      // verdiği görev (NULL = yok) — giriş düğümünü seçer
    NpcStop schedule[NPC_MAX_STOPS];
    int stopCount;            // en az 1 (ilk durak = başlangıç konumu)

    // Sheet/görünüm farkları (0 = varsayılan: 64px hücre, 1.5x, NPC_MAX_HEALTH)
    int map;                  // yaşadığı harita (0 = dünya; 5 = MAP_LOCA — literal,
                              // include döngüsünden kaçınma: decor.c DM_ deseni)
    int frameSize;            // hücre boyutu (Knights sheet'leri 48)
    float drawScale;
    float maxHealth;
    bool isGuard;             // muhafız: rutin yerine guard.c beyni yönetir
} NpcDef;

extern const NpcDef NPCS[];
extern const int NPC_COUNT;

// id ile NPC indeksi (-1 = yok). Bulunamazsa BİR KEZ uyarı basar: id'i yeniden
// adlandırıp bir çağrı yerini güncellemeyi unutmak sessiz kalmamalı — bu hatanın
// tüm maliyeti tam olarak sessiz olmasıydı.
int npc_index_by_id(const char *id);

// Harita geçişi: dünyada değilken NPC'ler tamamen devre dışı
// (update/render/nearest/visible hepsi boş döner)
void npc_set_active(bool active);
// Aktif harita (decor_set_map deseni): yalnız bu haritanın NPC'leri işler
void npc_set_map(int mapId);

// Rutin: saate göre hedef durak seç, oraya yürü, indoors'ta gizlen.
void npc_update(float dt, float hours);

// Tüm NPC'leri çiz (idle animasyon + isim; en yakındakine E prompt'u).
// nearestIdx: npc_nearest_in_range sonucu (-1 = prompt yok).
// sneaking: eğilmişken menzildeki NPC'de '[P] Yankesicilik' ipucu da çıkar
void npc_render_all(R2D *renderer, R2DTexture **npcTextures,
                    const Camera *camera, TTF_Font *font, int nearestIdx,
                    bool sneaking);

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
bool npc_bark_active(int i);   // baş üstü balonu açık mı (yankesicilik: meşgul +%15)
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

// --- REVİR (Faz REVIR): sivil 0'a inmez, 1 canda bayılır → revire yatar ---
bool npc_infirm(int i);
int  npc_infirm_day(int i);
int  npc_beat_count(int i);
bool npc_grudge(int i);
void npc_send_infirmary(int i, int day);
void npc_infirmary_discharge(int i, bool grudge);
void npc_infirm_state_get(int i, int *infirm, int *day, int *beats, int *grudge);
void npc_infirm_state_set(int i, int infirm, int day, int beats, int grudge);
int npc_current_stop_index(int i, float hours);   // Takip görevi (P1)
void npc_render_infirmary(R2D *renderer, R2DTexture **npcTextures,
                          const Camera *camera, TTF_Font *font,
                          float bedX, float bedY);

bool  npc_is_alive(int i);
float npc_health(int i);
float npc_max_health(int i);       // def'ten (0 ise NPC_MAX_HEALTH)
void  npc_set_alive(int i, bool alive);   // save/load köprüsü

// Muhafız beyni (guard.c) için konum erişimi — rutin bu NPC'yi yönetmez
bool npc_pos(int i, float *x, float *y);         // ölü de olsa son konum
void npc_set_pos(int i, float x, float y);

#endif
