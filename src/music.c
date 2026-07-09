#include <SDL3/SDL.h>
#include "music.h"

#define MUSIC_VOLUME 0.35f    // müzik SFX'i bastırmasın
#define FADE_SECONDS 1.5f     // crossfade süresi

static const char *TRACK_FILES[MOOD_COUNT] = {
    [MOOD_TOWN]   = "town",
    [MOOD_WILD]   = "wild",
    [MOOD_DANGER] = "danger",
    [MOOD_COMBAT] = "combat",
};

static Uint8 *trkBuf[MOOD_COUNT];
static Uint32 trkLen[MOOD_COUNT];
static SDL_AudioStream *trkStream[MOOD_COUNT];
static float trkGain[MOOD_COUNT];   // 0..1 (MUSIC_VOLUME ile çarpılır)
static MusicMood target = MOOD_TOWN;
static bool musicReady = false;

bool music_init(void)
{
    // SDL_INIT_AUDIO audio_init'te açıldı; yine de garanti et (ref sayılır)
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_Log("Muzik: ses alt sistemi yok: %s", SDL_GetError());
        return false;
    }

    for (int i = 0; i < MOOD_COUNT; i++) {
        char path[128];
        SDL_snprintf(path, sizeof(path), "assets/audio/music_%s.wav", TRACK_FILES[i]);

        SDL_AudioSpec spec;
        if (!SDL_LoadWAV(path, &spec, &trkBuf[i], &trkLen[i])) {
            SDL_Log("Muzik yuklenemedi (%s): %s", path, SDL_GetError());
            continue;
        }
        trkStream[i] = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                                 &spec, NULL, NULL);
        if (trkStream[i] == NULL) {
            SDL_Log("Muzik stream acilamadi (%s): %s", path, SDL_GetError());
            continue;
        }
        SDL_SetAudioStreamGain(trkStream[i], 0.0f);
        SDL_ResumeAudioStreamDevice(trkStream[i]);
    }

    musicReady = true;
    trkGain[target] = 0.0f;   // ilk parça fade-in ile girer
    return true;
}

void music_set_mood(MusicMood m)
{
    if (!musicReady || m < 0 || m >= MOOD_COUNT || m == target) return;
    target = m;

    // Parça sıfırdan başlasın (yarıdan girmesin — savaş girişi vurgulu olur)
    if (trkStream[m] != NULL && trkGain[m] <= 0.001f)
        SDL_ClearAudioStream(trkStream[m]);
}

void music_update(float dt)
{
    if (!musicReady) return;

    float step = dt / FADE_SECONDS;
    for (int i = 0; i < MOOD_COUNT; i++) {
        if (trkStream[i] == NULL || trkBuf[i] == NULL) continue;

        // Gain'i hedefe doğru yürüt (hedef parça 1, digerleri 0)
        float want = (i == (int)target) ? 1.0f : 0.0f;
        if (trkGain[i] < want) { trkGain[i] += step; if (trkGain[i] > want) trkGain[i] = want; }
        if (trkGain[i] > want) { trkGain[i] -= step; if (trkGain[i] < want) trkGain[i] = want; }
        SDL_SetAudioStreamGain(trkStream[i], trkGain[i] * MUSIC_VOLUME);

        // Duyulur parçanın kuyruğu azaldıysa baştan ekle (kesintisiz loop)
        if (trkGain[i] > 0.001f &&
            SDL_GetAudioStreamQueued(trkStream[i]) < (int)trkLen[i] / 2) {
            SDL_PutAudioStreamData(trkStream[i], trkBuf[i], (int)trkLen[i]);
        }
    }
}

void music_quit(void)
{
    for (int i = 0; i < MOOD_COUNT; i++) {
        if (trkStream[i]) SDL_DestroyAudioStream(trkStream[i]);
        if (trkBuf[i]) SDL_free(trkBuf[i]);
    }
    musicReady = false;
}
