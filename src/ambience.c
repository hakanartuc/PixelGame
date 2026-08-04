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

// Su katmanı (kıyı çırpıntısı): rain'in ikizi, kazancı main mesafeden sürer.
// İki karakter, iki stream: deniz/göl (dalgalı) + gölet (durgun) — hangisinin
// duyulacağını agitasyon belirler, çapraz geçişli.
static Uint8 *waterBuf = NULL;
static Uint32 waterLen = 0;
static SDL_AudioStream *waterStream = NULL;
static float waterGain = 0.0f, waterWant = 0.0f;
static Uint8 *calmBuf = NULL;
static Uint32 calmLen = 0;
static SDL_AudioStream *calmStream = NULL;
static float calmGain = 0.0f, calmWant = 0.0f;

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
    // Su loop'ları — kıyı çırpıntısı, rain ile aynı kurulum:
    // amb_water.wav = deniz/göl (dalgalı), amb_water_calm.wav = gölet (durgun)
    {
        SDL_AudioSpec spec;
        if (SDL_LoadWAV("assets/audio/amb_water.wav", &spec, &waterBuf, &waterLen)) {
            waterStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                                    &spec, NULL, NULL);
            if (waterStream != NULL) {
                SDL_SetAudioStreamGain(waterStream, 0.0f);
                SDL_ResumeAudioStreamDevice(waterStream);
            }
        } else {
            SDL_Log("Su ambiyansi yuklenemedi: %s", SDL_GetError());
        }
        if (SDL_LoadWAV("assets/audio/amb_water_calm.wav", &spec, &calmBuf, &calmLen)) {
            calmStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                                   &spec, NULL, NULL);
            if (calmStream != NULL) {
                SDL_SetAudioStreamGain(calmStream, 0.0f);
                SDL_ResumeAudioStreamDevice(calmStream);
            }
        } else {
            SDL_Log("Golet ambiyansi yuklenemedi: %s", SDL_GetError());
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

void ambience_set_water(float g01, float agit01)
{
    if (g01 < 0.0f) g01 = 0.0f;
    if (g01 > 1.0f) g01 = 1.0f;
    if (agit01 < 0.0f) agit01 = 0.0f;
    if (agit01 > 1.0f) agit01 = 1.0f;
    waterWant = g01 * agit01;           // dalgalı karakter (deniz/göl)
    calmWant  = g01 * (1.0f - agit01);  // durgun karakter (gölet)
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

    // Su katmanları: iki karakter de hedefe yumuşak yaklaşır (rain deseni) —
    // çapraz geçişi hedefler taşır (set_water g*agit / g*(1-agit) böler)
    if (waterStream != NULL && waterBuf != NULL) {
        if (waterGain < waterWant) { waterGain += step; if (waterGain > waterWant) waterGain = waterWant; }
        if (waterGain > waterWant) { waterGain -= step; if (waterGain < waterWant) waterGain = waterWant; }
        SDL_SetAudioStreamGain(waterStream, waterGain * AMB_VOLUME * userVol);
        if (waterGain > 0.001f &&
            SDL_GetAudioStreamQueued(waterStream) < (int)waterLen / 2)
            SDL_PutAudioStreamData(waterStream, waterBuf, (int)waterLen);
    }
    if (calmStream != NULL && calmBuf != NULL) {
        if (calmGain < calmWant) { calmGain += step; if (calmGain > calmWant) calmGain = calmWant; }
        if (calmGain > calmWant) { calmGain -= step; if (calmGain < calmWant) calmGain = calmWant; }
        SDL_SetAudioStreamGain(calmStream, calmGain * AMB_VOLUME * userVol);
        if (calmGain > 0.001f &&
            SDL_GetAudioStreamQueued(calmStream) < (int)calmLen / 2)
            SDL_PutAudioStreamData(calmStream, calmBuf, (int)calmLen);
    }
}

void ambience_quit(void)
{
    // Önce sustur (kapanış yarışına karşı — bkz. music_quit notu)
    if (calmStream) SDL_PauseAudioStreamDevice(calmStream);
    if (waterStream) SDL_PauseAudioStreamDevice(waterStream);
    if (rainStream) SDL_PauseAudioStreamDevice(rainStream);
    for (int i = 0; i < AMB_KIND_COUNT; i++)
        if (ambStream[i]) SDL_PauseAudioStreamDevice(ambStream[i]);
    if (calmStream) SDL_DestroyAudioStream(calmStream);
    if (calmBuf) SDL_free(calmBuf);
    if (waterStream) SDL_DestroyAudioStream(waterStream);
    if (waterBuf) SDL_free(waterBuf);
    if (rainStream) SDL_DestroyAudioStream(rainStream);
    if (rainBuf) SDL_free(rainBuf);
    for (int i = 0; i < AMB_KIND_COUNT; i++) {
        if (ambStream[i]) SDL_DestroyAudioStream(ambStream[i]);
        if (ambBuf[i]) SDL_free(ambBuf[i]);
    }
    ambReady = false;
}
