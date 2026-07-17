#ifndef SCENE_H
#define SCENE_H
#include "r2d.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

// Vinyet/sahne motoru (Pillars tarzı "scripted interaction").
// NASIL ilkeleri gömülü: fade ile süzülme, müzik duck'ı, typewriter (hızlanır
// ama ATLANMAZ), seçenekler metin bitince belirir, panel girişi SFX direktifi.
//
// Sahne dosyası (assets/scenes/*.txt — diyalog formatının kardeşi):
//   [panel_id]
//   image: assets/scenes/x.png     (opsiyonel — üst bölgede çerçeveli diorama)
//   duck: 0.2                      (opsiyonel — müzik kısma hedefi, vrs 0.25)
//   sfx: whisper|heartbeat         (opsiyonel — panel girişinde çalar)
//   text: paragraf                 (birden çok text: = paragraflar)
//   opt: Dinle -> panel_id|end
//   fx: witness|consume|essence|gold N   (son opt'a bağlanır, seçilince uygulanır)
//
// Etkiler MAIN'de uygulanır (envanter/sayaç erişimi) — scene_choose döndürür.

#define SCENE_MAX_PANELS 16
#define SCENE_MAX_OPTS 4
#define SCENE_TEXT_LEN 700
#define SCENE_CPS 40.0f        // typewriter: karakter/sn (basılı tut = ×4)

// Bir seçeneğin etkileri (main işler)
typedef struct {
    int witness;    // Dinle/Defnet sayacı +N
    int consume;    // Çek/Sömür sayacı +N
    int essence;    // Soluk Öz item +N (sömürü ödülü)
    int gold;
    int s3;         // S3 kayıt odası kararı: 2 devral / 3 kapat (D1c)
    int ritual;     // YANKICI ritüeli: 1 = kabul (sınıf + silah açılır)
} SceneFx;

// Görsel yükleme için renderer (main init'te bir kez çağırır)
void scene_set_renderer(R2D *r);

bool scene_start(const char *path);   // yükle + fade-in (false = dosya/format hatası)
void scene_set_speech(int level);     // vinyet zarı için Speech seviyesi (D1a)
bool scene_active(void);

// accelHeld: Space/E basılı — typewriter ×4 (metin ASLA atlanmaz)
void scene_update(float dt, bool accelHeld);

// Metin bitti mi (seçenekler görünür/seçilebilir mi)
bool scene_options_ready(void);
int  scene_option_count(void);

// Seçim (0-tabanlı). Etkiler out'a yazılır; hedef panelse geçilir, end'se
// fade-out başlar. false = geçersiz seçim / metin bitmedi.
bool scene_choose(int idx, SceneFx *out);

void scene_render(R2D *renderer, TTF_Font *font, int winW, int winH);

#endif
