#ifndef AUDIO_H
#define AUDIO_H
#include <stdbool.h>

// Ses sistemi — saf SDL3 (SDL_mixer YOK, "no hidden machinery"):
// her efekt kendi audio stream'ine bağlı; SDL cihazda kendisi miksler.
// Aynı efekt üst üste tetiklenirse baştan çalar (retrigger — retro oyun hissi),
// FARKLI efektler sorunsuz üst üste biner.
// WAV'lar prosedürel üretildi: assets/audio/ (script ile yeniden üretilebilir).

typedef enum {
    SFX_HIT,       // normal vuruş isabeti
    SFX_CRIT,      // kritik vuruş
    SFX_LOOT,      // item/altın alma
    SFX_POTION,    // iksir içme
    SFX_LEVELUP,   // skill seviye atlama
    SFX_UI,        // menü/panel tık
    SFX_DEATH,     // oyuncu ölümü
    SFX_PERK,      // perk seçimi (törensel arpej)
    SFX_DODGE,     // Kaçınma perki tetiklendi (whoosh)
    SFX_REFLECT,   // Karşılık/Diken yansıması (metalik ting)
    SFX_DEATHSAVE, // Yıkılmaz/Ölümsüz kurtarışı (gong + yükseliş)
    SFX_WHISPER,   // fısıltı — yankı imzası ("...henüz değil" hissi)
    SFX_HEARTBEAT, // tek kalp atımı (lub-dub) — Sessiz Gün ritmi
    SFX_TYPETICK,  // typewriter yumuşak tık (vinyet metni)
    SFX_THUNDER,   // gök gürültüsü (fırtına şimşeği)
    SFX_COUNT,
} SfxId;

// Cihazı aç + tüm WAV'ları yükle. Başarısızlık oyunu durdurmaz (sessiz devam).
bool audio_init(void);

// Efekti çal (init başarısızsa sessizce hiçbir şey yapmaz).
void audio_play(SfxId id);
void audio_set_volume(float v01);   // kullanıcı SFX seviyesi (0..1, ayarlar)

void audio_quit(void);

#endif
