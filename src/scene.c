#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "scene.h"
#include "font.h"
#include "audio.h"
#include "music.h"

typedef struct {
    char text[64];
    char target[32];   // panel id ya da "end"
    SceneFx fx;
    int rollSkill;     // 0 = zar yok; 1 = speech (D1a — vinyette zar)
    int rollThr;       // eşik (dialog zarıyla aynı formül: geçtiyse %100)
    char rollFail[32]; // başarısızlıkta gidilecek panel
} SceneOpt;

typedef struct {
    char id[32];
    char imagePath[96];
    float duck;                  // 0 = varsayılan 0.25
    int sfx;                     // -1 yok, yoksa SfxId
    char text[SCENE_TEXT_LEN];   // paragraflar \n\n ile birleşik
    SceneOpt opts[SCENE_MAX_OPTS];
    int optCount;
} ScenePanel;

static ScenePanel panels[SCENE_MAX_PANELS];
static int panelCount = 0;
static int cur = -1;

typedef enum { SC_OFF, SC_FADEIN, SC_SHOW, SC_FADEOUT } SceneState;
static SceneState state = SC_OFF;
static float fade = 0.0f;        // 0..1 sahne zaman çizgisi (karartı + içerik türetilir)
static float typedChars = 0.0f;  // typewriter ilerleme (kod noktası sayısı)
static int   textCodepoints = 0;
static float tickAccum = 0.0f;   // tık sesi sayacı
static R2DTexture *img = NULL;      // aktif panelin görseli
static R2DTexture *prevImg = NULL;  // önceki görsel (crossfade için)
static float crossT = 1.0f;         // 0..1: yeni görsel eskisinin üstüne biner

// Sinematik tempolar: acele yok — dünya zaten durdu
#define FADE_TIME 0.9f    // dünyadan sahneye süzülme
#define CROSS_TIME 0.8f   // panel görselleri arası çapraz geçiş

// ---------------------------------------------------------------------------
// UTF-8 yardımcıları — typewriter Türkçe karakteri ORTADAN kesmesin
// ---------------------------------------------------------------------------
static int utf8_count(const char *s)
{
    int n = 0;
    for (; *s; s++)
        if (((unsigned char)*s & 0xC0) != 0x80) n++;
    return n;
}

// İlk n kod noktasının byte uzunluğu
static int utf8_bytes_for(const char *s, int n)
{
    int i = 0;
    while (s[i] && n > 0) {
        i++;
        while (((unsigned char)s[i] & 0xC0) == 0x80) i++;
        n--;
    }
    return i;
}

// ---------------------------------------------------------------------------
// Yükleme (dialog parser deseni)
// ---------------------------------------------------------------------------
static void chomp(char *s)
{
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) s[--n] = '\0';
}
static const char *skip_ws(const char *s)
{
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

static int find_panel(const char *id)
{
    for (int i = 0; i < panelCount; i++)
        if (strcmp(panels[i].id, id) == 0) return i;
    return -1;
}

static bool scene_load(const char *path)
{
    panelCount = 0;
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        SDL_Log("Sahne dosyasi acilamadi: %s", path);
        return false;
    }

    char line[512];
    ScenePanel *p = NULL;
    int lineNo = 0;
    while (fgets(line, sizeof(line), f) != NULL) {
        lineNo++;
        chomp(line);
        const char *s = skip_ws(line);
        if (*s == '\0' || *s == '#') continue;

        if (*s == '[') {
            const char *close = strchr(s, ']');
            if (close == NULL || panelCount >= SCENE_MAX_PANELS) goto fail;
            p = &panels[panelCount++];
            memset(p, 0, sizeof(*p));
            p->sfx = -1;
            size_t len = (size_t)(close - s - 1);
            if (len >= sizeof(p->id)) len = sizeof(p->id) - 1;
            memcpy(p->id, s + 1, len);
        }
        else if (p == NULL) goto fail;
        else if (strncmp(s, "image:", 6) == 0)
            snprintf(p->imagePath, sizeof(p->imagePath), "%s", skip_ws(s + 6));
        else if (strncmp(s, "duck:", 5) == 0)
            p->duck = (float)atof(skip_ws(s + 5));
        else if (strncmp(s, "sfx:", 4) == 0) {
            const char *v = skip_ws(s + 4);
            if (strcmp(v, "whisper") == 0)   p->sfx = SFX_WHISPER;
            if (strcmp(v, "heartbeat") == 0) p->sfx = SFX_HEARTBEAT;
        }
        else if (strncmp(s, "text:", 5) == 0) {
            // Birden çok text: satırı paragraflara birleşir
            if (p->text[0] != '\0')
                strncat(p->text, "\n\n", sizeof(p->text) - strlen(p->text) - 1);
            strncat(p->text, skip_ws(s + 5), sizeof(p->text) - strlen(p->text) - 1);
        }
        else if (strncmp(s, "opt:", 4) == 0) {
            if (p->optCount >= SCENE_MAX_OPTS) goto fail;
            const char *body = skip_ws(s + 4);
            const char *arrow = strstr(body, "->");
            if (arrow == NULL) goto fail;
            SceneOpt *o = &p->opts[p->optCount++];
            memset(o, 0, sizeof(*o));
            size_t tlen = (size_t)(arrow - body);
            while (tlen > 0 && body[tlen-1] == ' ') tlen--;
            if (tlen >= sizeof(o->text)) tlen = sizeof(o->text) - 1;
            memcpy(o->text, body, tlen);
            snprintf(o->target, sizeof(o->target), "%s", skip_ws(arrow + 2));
        }
        else if (strncmp(s, "roll:", 5) == 0) {
            // "roll: speech 20 -> fail_id" — son opt'a Speech zarı bağlar (D1a)
            ScenePanel *pp = &panels[panelCount - 1];
            if (pp->optCount > 0) {
                SceneOpt *o = &pp->opts[pp->optCount - 1];
                int thr = 0; char fid[32] = "end";
                if (SDL_sscanf(s + 5, " speech %d -> %31s", &thr, fid) == 2) {
                    o->rollSkill = 1;
                    o->rollThr = thr;
                    SDL_strlcpy(o->rollFail, fid, sizeof(o->rollFail));
                }
            }
        }
        else if (strncmp(s, "fx:", 3) == 0) {
            if (p->optCount == 0) goto fail;
            SceneOpt *o = &p->opts[p->optCount - 1];
            const char *v = skip_ws(s + 3);
            if (strcmp(v, "witness") == 0) o->fx.witness++;
            else if (strcmp(v, "consume") == 0) o->fx.consume++;
            else if (strcmp(v, "essence") == 0) o->fx.essence++;
            else if (strncmp(v, "gold ", 5) == 0) o->fx.gold += atoi(v + 5);
            else if (strncmp(v, "s3 ", 3) == 0)   o->fx.s3 = atoi(v + 3);
            else if (strncmp(v, "ritual ", 7) == 0) o->fx.ritual = atoi(v + 7);
        }
        else goto fail;
    }
    fclose(f);
    return panelCount > 0;
fail:
    SDL_Log("Sahne format hatasi (%s:%d)", path, lineNo);
    fclose(f);
    panelCount = 0;
    return false;
}

// ---------------------------------------------------------------------------
// Yaşam döngüsü
// ---------------------------------------------------------------------------
static R2D *sceneRenderer = NULL;   // görsel yükleme için (main set eder — init'te)

void scene_set_renderer(R2D *r) { sceneRenderer = r; }

static void enter_panel(int idx)
{
    cur = idx;
    typedChars = 0.0f;
    tickAccum = 0.0f;
    textCodepoints = utf8_count(panels[cur].text);

    // Görsel: panel yeni bir tane belirtirse ÇAPRAZ GEÇİŞLE biner; belirtmezse
    // önceki fonda kalır (diorama sahne boyunca yaşar)
    if (panels[cur].imagePath[0] != '\0') {
        if (prevImg != NULL) { r2d_texture_destroy(sceneRenderer, prevImg); prevImg = NULL; }
        R2DTexture *nw = r2d_texture_load(sceneRenderer, panels[cur].imagePath);
        if (nw == NULL) {
            SDL_Log("Sahne gorseli yuklenemedi: %s", panels[cur].imagePath);
        } else {
            prevImg = img;                       // eski görsel altta kalır, yenisi üstüne doğar
            img = nw;
            crossT = (prevImg != NULL) ? 0.0f : 1.0f;
        }
    }

    float d = (panels[cur].duck > 0.0f) ? panels[cur].duck : 0.25f;
    music_set_duck(d);
    if (panels[cur].sfx >= 0)
        audio_play((SfxId)panels[cur].sfx);
}

bool scene_start(const char *path)
{
    if (!scene_load(path)) return false;
    state = SC_FADEIN;
    fade = 0.0f;
    enter_panel(0);
    return true;
}

bool scene_active(void) { return state != SC_OFF; }

void scene_update(float dt, bool accelHeld)
{
    if (state == SC_OFF) return;

    // Görsel çapraz geçişi (panel değişiminde eskisi yenisine erir)
    if (crossT < 1.0f) {
        crossT += dt / CROSS_TIME;
        if (crossT >= 1.0f) {
            crossT = 1.0f;
            if (prevImg != NULL) { r2d_texture_destroy(sceneRenderer, prevImg); prevImg = NULL; }
        }
    }

    if (state == SC_FADEIN) {
        fade += dt / FADE_TIME;
        if (fade >= 1.0f) { fade = 1.0f; state = SC_SHOW; }
    }
    else if (state == SC_FADEOUT) {
        fade -= dt / FADE_TIME;
        if (fade <= 0.0f) {
            fade = 0.0f;
            state = SC_OFF;
            cur = -1;
            if (img != NULL)     { r2d_texture_destroy(sceneRenderer, img); img = NULL; }
            if (prevImg != NULL) { r2d_texture_destroy(sceneRenderer, prevImg); prevImg = NULL; }
            music_set_duck(1.0f);   // müzik geri dolar (süzülme)
        }
        return;
    }

    // Typewriter: hızlanır ama atlamaz (NASIL ilkesi 4)
    if (state == SC_SHOW && typedChars < (float)textCodepoints) {
        float speed = SCENE_CPS * (accelHeld ? 4.0f : 1.0f);
        float before = typedChars;
        typedChars += speed * dt;
        if (typedChars > (float)textCodepoints) typedChars = (float)textCodepoints;

        // Yumuşak tık: her ~3 karakterde bir (hızlanınca da oranlı)
        tickAccum += typedChars - before;
        if (tickAccum >= 3.0f) {
            tickAccum = 0.0f;
            audio_play(SFX_TYPETICK);
        }
    }
}

bool scene_options_ready(void)
{
    return state == SC_SHOW && cur >= 0 && typedChars >= (float)textCodepoints;
}

int scene_option_count(void)
{
    return (cur >= 0) ? panels[cur].optCount : 0;
}

static int scnSpeech = 0;   // oyuncunun Speech seviyesi (main her sahne öncesi basar)
void scene_set_speech(int level) { scnSpeech = level; }

bool scene_choose(int idx, SceneFx *out)
{
    if (!scene_options_ready()) return false;
    if (idx < 0 || idx >= panels[cur].optCount) return false;

    *out = panels[cur].opts[idx].fx;
    const char *target = panels[cur].opts[idx].target;
    audio_play(SFX_UI);

    // Vinyet zarı (D1a): diyalog zarıyla aynı eğri — eşik altı %70'e kadar şans
    const SceneOpt *co = &panels[cur].opts[idx];
    if (co->rollSkill == 1) {
        int pct = (scnSpeech >= co->rollThr) ? 100
                : (int)((float)scnSpeech / (float)co->rollThr * 70.0f);
        if ((int)(SDL_rand(100)) >= pct) {
            *out = (SceneFx){0};           // başarısız seçim etki taşımaz
            target = co->rollFail;         // zar düştü — fail dalı
        }
    }

    if (strcmp(target, "end") == 0) {
        state = SC_FADEOUT;
        return true;
    }
    int nx = find_panel(target);
    if (nx >= 0) enter_panel(nx);
    else state = SC_FADEOUT;   // bozuk hedef — sahneyi kapat (oyun kilitlenmesin)
    return true;
}

// ---------------------------------------------------------------------------
// Çizim: dünya koyu karartıyla altta durur; görsel üst 2/3, metin alt 1/3
// ---------------------------------------------------------------------------
void scene_render(R2D *renderer, TTF_Font *font, int winW, int winH)
{
    if (state == SC_OFF || cur < 0) return;

    // Sahneleme iki katmanlı zamanla akar: önce dünya TAM karanlığa iner,
    // içerik (görsel+metin) o karanlığın içinden doğar. Tek 'fade' saatinden:
    float ov = fade * 1.6f;                    // karartı önden gider
    if (ov > 1.0f) ov = 1.0f;
    float ca = (fade - 0.35f) / 0.65f;         // içerik arkadan gelir
    if (ca < 0.0f) ca = 0.0f;
    if (ca > 1.0f) ca = 1.0f;
    Uint8 a = (Uint8)(ca * 255.0f);

    r2d_set_blend(renderer, R2D_BLEND_ALPHA);
    r2d_set_color(renderer, 0, 0, 0, (Uint8)(ov * 255.0f));
    r2d_fill_rect(renderer, &(SDL_FRect){ 0, 0, (float)winW, (float)winH });

    ScenePanel *p = &panels[cur];

    // Görsel: TAM EKRAN — ekrana sığan en büyük ölçek (kalan siyah = letterbox).
    // Panel değişiminde eski görsel altta, yenisi üstüne çapraz geçişle biner.
    if (img != NULL) {
        float iw, ih;
        r2d_texture_size(img, &iw, &ih);
        float s = SDL_min((float)winW / iw, (float)winH / ih);
        float dw = iw * s, dh = ih * s;
        SDL_FRect dst = { (winW - dw) / 2.0f, (winH - dh) / 2.0f, dw, dh };

        if (prevImg != NULL && crossT < 1.0f) {
            r2d_texture_set_alpha_mod(prevImg, a);
            r2d_draw_texture(renderer, prevImg, NULL, &dst);
            r2d_texture_set_alpha_mod(prevImg, 255);
        }
        Uint8 ia = (prevImg != NULL && crossT < 1.0f)
                 ? (Uint8)(ca * crossT * 255.0f) : a;
        r2d_texture_set_alpha_mod(img, ia);
        r2d_draw_texture(renderer, img, NULL, &dst);
        r2d_texture_set_alpha_mod(img, 255);
    }

    // Alt bant: görselin üstüne yumuşak siyah gradyan (sinematik anlatı şeridi)
    float bandTop = winH * 0.58f;
    int steps = 14;
    for (int i = 0; i < steps; i++) {
        float t = (float)i / (float)(steps - 1);
        float y0 = bandTop + (winH - bandTop) * (float)i / (float)steps;
        float y1 = bandTop + (winH - bandTop) * (float)(i + 1) / (float)steps;
        r2d_set_color(renderer, 0, 0, 0, (Uint8)(ca * (30.0f + 195.0f * t)));
        r2d_fill_rect(renderer, &(SDL_FRect){ 0, y0, (float)winW, y1 - y0 + 1.0f });
    }

    if (font == NULL) return;

    // Typewriter: ilk N kod noktası (UTF-8 güvenli kesim)
    char shown[SCENE_TEXT_LEN];
    int nbytes = utf8_bytes_for(p->text, (int)typedChars);
    memcpy(shown, p->text, (size_t)nbytes);
    shown[nbytes] = '\0';
    SDL_Color ink = { 238, 234, 222, a };
    float px = winW * 0.16f, pw = winW * 0.68f;
    float ty = winH * 0.68f;
    ty += draw_text_wrapped(renderer, font, shown, px, ty, pw, ink) + 14.0f;

    // Seçenekler: metin bitince belirir
    if (scene_options_ready()) {
        SDL_Color optCol = { 175, 205, 255, a };
        for (int i = 0; i < p->optCount; i++) {
            char buf[96];
            if (p->opts[i].rollSkill == 1)
                snprintf(buf, sizeof(buf), "%d) %s [Speech %d]",
                         i + 1, p->opts[i].text, p->opts[i].rollThr);
            else
                snprintf(buf, sizeof(buf), "%d) %s", i + 1, p->opts[i].text);
            draw_text(renderer, font, buf, px, ty, optCol);
            ty += 24.0f;
        }
    }
    else if (state == SC_SHOW) {
        SDL_Color dim = { 150, 150, 160, (Uint8)(ca * 150.0f) };
        draw_text(renderer, font, "...", winW - px - 30.0f, winH - 40.0f, dim);
    }
}
