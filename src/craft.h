#ifndef CRAFT_H
#define CRAFT_H
#include "r2d.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "common.h"
#include "skill.h"
#include "inventory.h"

// Craft istasyonları + tarifler.
// İstasyon kullanımı ÜÇ kapılı (main denetler):
//   1) tanışma görevi REWARDED (permQuest) — ilk açılış
//   2) bölgede bounty yok — "Önce cezanı öde!"
//   3) itibar > REP_NO_QUESTS — "Ustanın sana güveni kalmadı" (toparlanınca geri gelir)
// Üretim SÜREÇLİDİR: demircilik çekiç savurmasıyla (vuruş = iş), simya süreyle kaynar.

#define STATION_INTERACT_RANGE 80.0f

typedef enum {
    STATION_SMITH,   // demirci ocağı (Çekiç kuşanılı şart)
    STATION_ALCH,    // simya kazanı (süre bazlı)
    STATION_ENCHANT, // harabe sunağı (süre bazlı; rün craft ŞANSLI — Enchanting telafi eder)
} StationType;

// Sunak işlemleri: rün basma garantili, stat basma garantili-pahalı;
// rün ÜRETİMİ şanslı (60 + Enchanting×0.4, tavan 95 — tutmayan özler yanar)
#define ENCH_RUNE_CAST_TIME 3.0f    // rün/stat basma süresi (sn)
#define ENCH_CRAFT_BASE_CHANCE 60.0f

typedef struct {
    StationType type;
    const char *name;       // panel başlığı ("Demirci Ocağı")
    const char *texPath;
    SDL_FRect src;          // İLK kare (sheet yatay kareli)
    int frames;             // animasyon kare sayısı (kullanım sırasında oynar)
    float x, y;             // dünya taban-orta
    float scale;
    const char *permQuest;  // izin görevi id'si (REWARDED = izin)
    const char *ownerName;  // yönlendirme mesajı için ("Demirci Mahir")
} StationDef;

extern const StationDef STATIONS[];
extern const int STATION_COUNT;

// Tarifler: en çok 2 girdi → 1 çıktı. work: demircilikte vuruş, simyada saniye.
typedef struct {
    StationType station;
    const char *name;
    int in1Item, in1Count;
    int in2Item, in2Count;   // in2Item = -1 → tek girdi
    int outItem, outCount;
    float work;
    float xp;                // tamamlanınca istasyon skill'ine
    int minSkill;            // bu seviyenin altında tarif kilitli (0 = yok)
    int blueprintItem;       // <=0 baştan açık; yoksa bu blueprint'le öğrenilir
                             // (öğrenilenler main'de + save v10)
} RecipeDef;

#define MAX_RECIPES 32   // öğrenilen tarif dizisi tavanı (save v10)

extern const RecipeDef RECIPES[];
extern const int RECIPE_COUNT;

// İstasyonun skill'i (SMITH → Smithing, ALCH → Alchemy)
SkillType station_skill(StationType t);

// E menzilindeki en yakın istasyon (-1 = yok)
int station_nearest(float px, float py);

// İstasyonları çiz (+ nearestIdx'e [E] ipucu).
// activeIdx: aktif üretimdeki istasyon — animasyon kareleri oynar (-1 = hepsi durgun)
void station_render_all(R2D *renderer, R2DTexture **textures,
                        const Camera *camera, TTF_Font *font,
                        int nearestIdx, int activeIdx);

// Öğrenilen blueprint tarifleri — modül durumu (save v10 buradan okur/yazar)
bool craft_recipe_learned(int ri);   // blueprintItem <= 0 tarifler HEP açık
void craft_learn_recipe(int ri);
void craft_reset_learned(void);      // yeni oyun / load öncesi

// Craft paneli: istasyonun tarifleri (malzeme durumu + kilitler) + '+basma' satırı.
// selRecipe: aktif süreçteki tarif (vurgulanır, -1 yok).
// Öğrenilmemiş blueprint tarifi "???" görünür. craft_panel_recipe_at ile aynı yerleşim.
void craft_panel_render(int stationIdx, const Inventory *inv, const Skill *skills,
                        int selRecipe, float plusChancePct, int plusCost,
                        const ItemStack *weapon,
                        R2D *renderer, TTF_Font *font, float panelX, float panelY);

// Panelde tıklanan satır: 0..N-1 = RECIPES indeksi (bu istasyonunkiler),
// negatifler özel satırlar, -1 = boş
#define CRAFT_ROW_PLUS   (-2)    // '+basma' (ocak)
#define CRAFT_ROW_TARGET (-3)    // hedef ekipmanı değiştir (sunak)
#define CRAFT_ROW_RUNE0  (-4)    // Pembe Rün bas (sunak)
#define CRAFT_ROW_RUNE1  (-5)    // Zehir Rünü bas (sunak)
#define CRAFT_ROW_STAT_BASE (-10)  // -10-STAT_x → stat basma satırı (-10..-15)
int craft_panel_recipe_at(int stationIdx, float x, float y, float panelX, float panelY);

#endif
