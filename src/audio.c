#include <SDL3/SDL.h>
#include "audio.h"

// SfxId sırasıyla dosya adları (assets/audio/<ad>.wav)
static const char *SFX_FILES[SFX_COUNT] = {
    [SFX_HIT]     = "hit",
    [SFX_CRIT]    = "crit",
    [SFX_LOOT]    = "loot",
    [SFX_POTION]  = "potion",
    [SFX_LEVELUP] = "levelup",
    [SFX_UI]        = "ui",
    [SFX_DEATH]     = "death",
    [SFX_PERK]      = "perk",
    [SFX_DODGE]     = "dodge",
    [SFX_REFLECT]   = "reflect",
    [SFX_DEATHSAVE] = "deathsave",
    [SFX_WHISPER]   = "whisper",
    [SFX_HEARTBEAT] = "heartbeat",
    [SFX_TYPETICK]  = "typetick",
};

// Efekt başına: ham WAV verisi + cihaza bağlı stream
static Uint8 *sfxBuf[SFX_COUNT];
static Uint32 sfxLen[SFX_COUNT];
static SDL_AudioStream *sfxStream[SFX_COUNT];
static bool audioReady = false;

bool audio_init(void)
{
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_Log("Ses alt sistemi acilamadi: %s", SDL_GetError());
        return false;
    }

    for (int i = 0; i < SFX_COUNT; i++) {
        char path[128];
        SDL_snprintf(path, sizeof(path), "assets/audio/%s.wav", SFX_FILES[i]);

        SDL_AudioSpec spec;
        if (!SDL_LoadWAV(path, &spec, &sfxBuf[i], &sfxLen[i])) {
            SDL_Log("SFX yuklenemedi (%s): %s", path, SDL_GetError());
            continue;   // eksik efekt sessiz kalır, digerleri calisir
        }

        // Stream cihaza bağlanır; SDL formatı cihaza kendisi çevirir + miksler
        sfxStream[i] = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                                 &spec, NULL, NULL);
        if (sfxStream[i] == NULL) {
            SDL_Log("SFX stream acilamadi (%s): %s", path, SDL_GetError());
            continue;
        }
        SDL_ResumeAudioStreamDevice(sfxStream[i]);   // stream'ler duraklı başlar
    }

    audioReady = true;
    return true;
}

void audio_play(SfxId id)
{
    if (!audioReady || id < 0 || id >= SFX_COUNT) return;
    if (sfxStream[id] == NULL || sfxBuf[id] == NULL) return;

    // Retrigger: çalmakta olanı kes, baştan başlat
    SDL_ClearAudioStream(sfxStream[id]);
    SDL_PutAudioStreamData(sfxStream[id], sfxBuf[id], (int)sfxLen[id]);
}

void audio_quit(void)
{
    for (int i = 0; i < SFX_COUNT; i++) {
        if (sfxStream[i]) SDL_DestroyAudioStream(sfxStream[i]);
        if (sfxBuf[i]) SDL_free(sfxBuf[i]);
    }
    audioReady = false;
}
