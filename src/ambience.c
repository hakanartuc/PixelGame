#include <SDL3/SDL.h>
#include "ambience.h"

#define AMB_VOLUME 0.16f     // müziğin altında kalır
#define AMB_FADE 2.0f        // ortam sesi yavaş girer/çıkar (fark edilmeden)

static const char *AMB_FILES[AMB_KIND_COUNT] = {
    [AMB_DAY]     = "day",
    [AMB_NIGHT]   = "night",
    [AMB_DUNGEON] = "dungeon",
};

static Uint8 *ambBuf[AMB_KIND_COUNT];
static Uint32 ambLen[AMB_KIND_COUNT];
static SDL_AudioStream *ambStream[AMB_KIND_COUNT];
static float ambGain[AMB_KIND_COUNT];
static AmbKind target = AMB_NONE;
static bool ambReady = false;
static float userVol = 1.0f;

// Yağmur katmanı (hava sistemi): bağımsız stream, şiddeti weather verir
static Uint8 *rainBuf = NULL;
static Uint32 rainLen = 0;
static SDL_AudioStream *rainStream = NULL;
static float rainGain = 0.0f, rainWant = 0.0f;

bool ambience_init(void)
{
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) return false;

    for (int i = 0; i < AMB_KIND_COUNT; i++) {
        char path[128];
        SDL_snprintf(path, sizeof(path), "assets/audio/amb_%s.wav", AMB_FILES[i]);

        SDL_AudioSpec spec;
        if (!SDL_LoadWAV(path, &spec, &ambBuf[i], &ambLen[i])) {
            SDL_Log("Ambiyans yuklenemedi (%s): %s", path, SDL_GetError());
            continue;
        }
        ambStream[i] = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                                 &spec, NULL, NULL);
        if (ambStream[i] == NULL) continue;
        SDL_SetAudioStreamGain(ambStream[i], 0.0f);
        SDL_ResumeAudioStreamDevice(ambStream[i]);
    }
    // Yağmur loop'u (amb_rain.wav) — crossfade havuzu dışında ayrı katman
    {
        SDL_AudioSpec spec;
        if (SDL_LoadWAV("assets/audio/amb_rain.wav", &spec, &rainBuf, &rainLen)) {
            rainStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                                   &spec, NULL, NULL);
            if (rainStream != NULL) {
                SDL_SetAudioStreamGain(rainStream, 0.0f);
                SDL_ResumeAudioStreamDevice(rainStream);
            }
        } else {
            SDL_Log("Yagmur ambiyansi yuklenemedi: %s", SDL_GetError());
        }
    }
    ambReady = true;
    return true;
}

void ambience_set_rain(float g01)
{
    if (g01 < 0.0f) g01 = 0.0f;
    if (g01 > 1.0f) g01 = 1.0f;
    rainWant = g01;
}

void ambience_set(AmbKind k)
{
    if (!ambReady || k == target) return;
    target = k;
}

void ambience_set_volume(float v01)
{
    if (v01 < 0.0f) v01 = 0.0f;
    if (v01 > 1.0f) v01 = 1.0f;
    userVol = v01;
}

void ambience_update(float dt)
{
    if (!ambReady) return;
    float step = dt / AMB_FADE;
    for (int i = 0; i < AMB_KIND_COUNT; i++) {
        if (ambStream[i] == NULL || ambBuf[i] == NULL) continue;
        float want = (i == (int)target) ? 1.0f : 0.0f;
        if (ambGain[i] < want) { ambGain[i] += step; if (ambGain[i] > want) ambGain[i] = want; }
        if (ambGain[i] > want) { ambGain[i] -= step; if (ambGain[i] < want) ambGain[i] = want; }
        SDL_SetAudioStreamGain(ambStream[i], ambGain[i] * AMB_VOLUME * userVol);

        if (ambGain[i] > 0.001f &&
            SDL_GetAudioStreamQueued(ambStream[i]) < (int)ambLen[i] / 2)
            SDL_PutAudioStreamData(ambStream[i], ambBuf[i], (int)ambLen[i]);
    }

    // Yağmur katmanı: hedefe yumuşak yaklaş (yağmur sesle "bastırır")
    if (rainStream != NULL && rainBuf != NULL) {
        if (rainGain < rainWant) { rainGain += step; if (rainGain > rainWant) rainGain = rainWant; }
        if (rainGain > rainWant) { rainGain -= step; if (rainGain < rainWant) rainGain = rainWant; }
        SDL_SetAudioStreamGain(rainStream, rainGain * AMB_VOLUME * 1.6f * userVol);
        if (rainGain > 0.001f &&
            SDL_GetAudioStreamQueued(rainStream) < (int)rainLen / 2)
            SDL_PutAudioStreamData(rainStream, rainBuf, (int)rainLen);
    }
}

void ambience_quit(void)
{
    if (rainStream) SDL_DestroyAudioStream(rainStream);
    if (rainBuf) SDL_free(rainBuf);
    for (int i = 0; i < AMB_KIND_COUNT; i++) {
        if (ambStream[i]) SDL_DestroyAudioStream(ambStream[i]);
        if (ambBuf[i]) SDL_free(ambBuf[i]);
    }
    ambReady = false;
}
