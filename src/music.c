#include <SDL3/SDL.h>
#include <stdlib.h>
#include "music.h"

// stb_vorbis (src/stb_vorbis.c, ayrı derleme birimi) — tüm dosyayı 16-bit
// PCM'e açar; buffer malloc'lu, free() ile bırakılır
extern int stb_vorbis_decode_filename(const char *filename, int *channels,
                                      int *sample_rate, short **output);

#define MUSIC_VOLUME 0.35f    // müzik SFX'i bastırmasın
#define FADE_SECONDS 1.5f     // crossfade süresi

static const char *TRACK_FILES[MOOD_COUNT] = {
    [MOOD_TOWN]   = "town",
    [MOOD_WILD]   = "wild",
    [MOOD_DANGER] = "danger",
    [MOOD_COMBAT] = "combat",
    [MOOD_DUNGEON] = "dungeon",
    [MOOD_NIGHT]   = "night",
    [MOOD_MENU]    = "menu",
    [MOOD_BOSS]    = "boss",
};

// LOOPSTART (sn): xDeviruchi dokümanındaki tablo — intro loop DIŞI kalır;
// ilk çalış intro dahil, sonraki beslemeler bu noktadan (kesintisiz döngü)
static const float LOOP_START_SEC[MOOD_COUNT] = {
    [MOOD_TOWN]    = 23.106f,   // Definitely Our Town
    [MOOD_WILD]    = 0.639f,    // Where The Winds Roam
    [MOOD_DANGER]  = 2.438f,    // The Calm Before The Storm
    [MOOD_COMBAT]  = 10.760f,   // Battle 1
    [MOOD_DUNGEON] = 0.0f,      // Lost Shrine (introsuz)
    [MOOD_NIGHT]   = 0.0f,      // Silent Forest (introsuz)
    [MOOD_MENU]    = 0.0f,      // Title Theme (loop işaretsiz — baştan döner)
    [MOOD_BOSS]    = 0.167f,    // Decisive Battle 1
};

static Uint8 *trkBuf[MOOD_COUNT];
static Uint32 trkLen[MOOD_COUNT];
static Uint32 trkLoopOff[MOOD_COUNT]; // LOOPSTART'ın byte karşılığı
static bool trkIntroFed[MOOD_COUNT];  // ilk besleme intro DAHİL, sonrası loop
static bool trkFromOgg[MOOD_COUNT];   // buffer kaynağı: free() mi SDL_free() mi
static SDL_AudioStream *trkStream[MOOD_COUNT];
static float trkGain[MOOD_COUNT];   // 0..1 (MUSIC_VOLUME ile çarpılır)
static MusicMood target = MOOD_TOWN;
static bool musicReady = false;
static float duckTarget = 1.0f;   // anlatı kısması hedefi
static float duckNow = 1.0f;      // yumuşak yaklaşan mevcut değer
static float userVol = 1.0f;      // kullanıcı müzik seviyesi (ayarlar)

bool music_init(void)
{
    // DEV: PIXE_NOAUDIO — bkz. audio.c audio_init (duman testi kancası)
    if (SDL_getenv("PIXE_NOAUDIO")) return false;
    // SDL_INIT_AUDIO audio_init'te açıldı; yine de garanti et (ref sayılır)
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_Log("Muzik: ses alt sistemi yok: %s", SDL_GetError());
        return false;
    }

    for (int i = 0; i < MOOD_COUNT; i++) {
        char path[128];
        SDL_AudioSpec spec;

        // Önce gerçek müzik seti (xDeviruchi OGG loop'ları)
        SDL_snprintf(path, sizeof(path), "assets/audio/music/%s.ogg", TRACK_FILES[i]);
        int oggCh = 0, oggRate = 0;
        short *oggPcm = NULL;
        int oggSamples = stb_vorbis_decode_filename(path, &oggCh, &oggRate, &oggPcm);
        if (oggSamples > 0 && oggPcm != NULL) {
            trkBuf[i] = (Uint8 *)oggPcm;
            trkLen[i] = (Uint32)oggSamples * (Uint32)oggCh * sizeof(short);
            trkFromOgg[i] = true;
            spec = (SDL_AudioSpec){ .format = SDL_AUDIO_S16,
                                    .channels = oggCh, .freq = oggRate };
        }
        else {
            // OGG yok/açılamadı: eski prosedürel WAV'a düş
            SDL_snprintf(path, sizeof(path), "assets/audio/music_%s.wav",
                         TRACK_FILES[i]);
            if (!SDL_LoadWAV(path, &spec, &trkBuf[i], &trkLen[i])) {
                SDL_Log("Muzik yuklenemedi (%s): %s", path, SDL_GetError());
                continue;
            }
        }
        // Loop başlangıcı byte cinsinden (örnek hizası: sample × kanal × 2)
        trkLoopOff[i] = (Uint32)(LOOP_START_SEC[i] * (float)spec.freq)
                      * (Uint32)spec.channels * sizeof(short);
        if (trkLoopOff[i] >= trkLen[i]) trkLoopOff[i] = 0;

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

void music_set_duck(float duck01)
{
    if (duck01 < 0.0f) duck01 = 0.0f;
    if (duck01 > 1.0f) duck01 = 1.0f;
    duckTarget = duck01;
}

void music_set_mood(MusicMood m)
{
    if (!musicReady || m < 0 || m >= MOOD_COUNT || m == target) return;
    target = m;

    // Parça sıfırdan başlasın (yarıdan girmesin — savaş girişi vurgulu olur);
    // intro yeniden çalınır (sonraki döngüler LOOPSTART'tan)
    if (trkStream[m] != NULL && trkGain[m] <= 0.001f) {
        SDL_ClearAudioStream(trkStream[m]);
        trkIntroFed[m] = false;
    }
}

void music_update(float dt)
{
    if (!musicReady) return;

    // Duck yumuşak yaklaşır (vinyet girişi ~0.7 sn'de çöker)
    float dstep = dt / 0.7f;
    if (duckNow < duckTarget) { duckNow += dstep; if (duckNow > duckTarget) duckNow = duckTarget; }
    if (duckNow > duckTarget) { duckNow -= dstep; if (duckNow < duckTarget) duckNow = duckTarget; }

    float step = dt / FADE_SECONDS;
    for (int i = 0; i < MOOD_COUNT; i++) {
        if (trkStream[i] == NULL || trkBuf[i] == NULL) continue;

        // Gain'i hedefe doğru yürüt (hedef parça 1, digerleri 0)
        float want = (i == (int)target) ? 1.0f : 0.0f;
        if (trkGain[i] < want) { trkGain[i] += step; if (trkGain[i] > want) trkGain[i] = want; }
        if (trkGain[i] > want) { trkGain[i] -= step; if (trkGain[i] < want) trkGain[i] = want; }
        SDL_SetAudioStreamGain(trkStream[i], trkGain[i] * MUSIC_VOLUME * duckNow * userVol);

        // Duyulur parçanın kuyruğu azaldıysa besle: ilk kez intro DAHİL,
        // sonrası LOOPSTART'tan (xDeviruchi loop tablosu; kesintisiz döngü)
        if (trkGain[i] > 0.001f &&
            SDL_GetAudioStreamQueued(trkStream[i]) < (int)trkLen[i] / 2) {
            if (!trkIntroFed[i]) {
                SDL_PutAudioStreamData(trkStream[i], trkBuf[i], (int)trkLen[i]);
                trkIntroFed[i] = true;
            }
            else
                SDL_PutAudioStreamData(trkStream[i], trkBuf[i] + trkLoopOff[i],
                                       (int)(trkLen[i] - trkLoopOff[i]));
        }
    }
}

void music_set_volume(float v01)
{
    if (v01 < 0.0f) v01 = 0.0f;
    if (v01 > 1.0f) v01 = 1.0f;
    userVol = v01;
}

void music_quit(void)
{
    // Önce SUSTUR: aygıt thread'i karıştırırken stream yok etmek nadir bir
    // kapanış çökmesi üretiyordu (SDL_QuitAudio/DestroyLogicalAudioDevice
    // SIGSEGV) — duraklatma yok etmeyle karıştırma yarışını kapatır
    for (int i = 0; i < MOOD_COUNT; i++)
        if (trkStream[i]) SDL_PauseAudioStreamDevice(trkStream[i]);
    for (int i = 0; i < MOOD_COUNT; i++) {
        if (trkStream[i]) SDL_DestroyAudioStream(trkStream[i]);
        if (trkBuf[i]) {
            if (trkFromOgg[i]) free(trkBuf[i]);   // stb_vorbis malloc'u
            else SDL_free(trkBuf[i]);
        }
    }
    musicReady = false;
}
