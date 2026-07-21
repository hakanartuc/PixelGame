#include <stdio.h>
#include <stdlib.h>   // atof (req/fx sayıları)
#include <string.h>
#include "dialog.h"
#include "font.h"
#include "ui_theme.h"
#include "audio.h"    // typewriter tık sesi

// Satır sonundaki \n ve \r'ı kırp
static void chomp(char *s)
{
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r'))
        s[--n] = '\0';
}

// Baştaki boşlukları atla
static const char *skip_ws(const char *s)
{
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

// "speech" / "gold" / "rep" / "bounty" / kişilik → DialogVar. Tanınmazsa false.
static bool parse_var(const char *s, size_t len, DialogVar *out)
{
    if (len == 6 && strncmp(s, "speech", 6) == 0) { *out = DVAR_SPEECH; return true; }
    if (len == 4 && strncmp(s, "gold", 4) == 0)   { *out = DVAR_GOLD;   return true; }
    if (len == 3 && strncmp(s, "rep", 3) == 0)    { *out = DVAR_REP;    return true; }
    if (len == 6 && strncmp(s, "bounty", 6) == 0) { *out = DVAR_BOUNTY; return true; }
    if (len == 4 && strncmp(s, "kaba", 4) == 0)   { *out = DVAR_KABA;   return true; }
    if (len == 5 && strncmp(s, "kibar", 5) == 0)  { *out = DVAR_KIBAR;  return true; }
    if (len == 5 && strncmp(s, "ketum", 5) == 0)  { *out = DVAR_KETUM;  return true; }
    if (len == 4 && strncmp(s, "hint", 4) == 0)   { *out = DVAR_HINT;   return true; }
    if (len == 5 && strncmp(s, "event", 5) == 0)  { *out = DVAR_EVENT;  return true; }
    if (len == 4 && strncmp(s, "hava", 4) == 0)   { *out = DVAR_WEATHER; return true; }
    if (len == 7 && strncmp(s, "weather", 7) == 0) { *out = DVAR_WEATHER; return true; }
    return false;
}

// "degisken OP sayi" ayrıştır (req: >=|<= — fx: +=|-=|=). Hatalıysa false.
static bool parse_cond_line(const char *body, DialogVar *var,
                            char opBuf[3], float *value)
{
    const char *p = skip_ws(body);
    const char *ve = p;
    while (*ve && *ve != ' ' && *ve != '\t' && *ve != '<' && *ve != '>' &&
           *ve != '+' && *ve != '-' && *ve != '=') ve++;
    if (!parse_var(p, (size_t)(ve - p), var))
        return false;

    p = skip_ws(ve);
    if ((p[0] == '>' || p[0] == '<' || p[0] == '+' || p[0] == '-') && p[1] == '=') {
        opBuf[0] = p[0]; opBuf[1] = '='; opBuf[2] = '\0';
        p += 2;
    } else if (p[0] == '=') {
        opBuf[0] = '='; opBuf[1] = '\0';
        p += 1;
    } else {
        return false;
    }
    *value = (float)atof(skip_ws(p));
    return true;
}

bool dialog_load(Dialog *d, const char *path)
{
    d->nodeCount = 0;

    FILE *f = fopen(path, "r");
    if (f == NULL) {
        SDL_Log("Diyalog dosyasi acilamadi: %s", path);
        return false;
    }

    char line[512];
    DialogNode *cur = NULL;
    DialogOpt *lastOpt = NULL;   // req:/fx: satırları bir üstteki opt'a bağlanır
    int lineNo = 0;

    while (fgets(line, sizeof(line), f) != NULL) {
        lineNo++;
        chomp(line);
        const char *p = skip_ws(line);

        if (*p == '\0' || *p == '#')
            continue;   // boş satır / yorum

        if (*p == '[') {
            // Yeni düğüm: [dugum_adi]
            const char *close = strchr(p, ']');
            if (close == NULL || d->nodeCount >= DIALOG_MAX_NODES) {
                SDL_Log("Diyalog hatasi (%s:%d): bozuk dugum basligi", path, lineNo);
                fclose(f);
                return false;
            }
            cur = &d->nodes[d->nodeCount++];
            memset(cur, 0, sizeof(*cur));
            lastOpt = NULL;
            size_t len = (size_t)(close - p - 1);
            if (len >= DIALOG_ID_LEN) len = DIALOG_ID_LEN - 1;
            memcpy(cur->id, p + 1, len);
        }
        else if (strncmp(p, "text:", 5) == 0) {
            if (cur == NULL) {
                SDL_Log("Diyalog hatasi (%s:%d): dugum disinda text", path, lineNo);
                fclose(f);
                return false;
            }
            snprintf(cur->text, DIALOG_TEXT_LEN, "%s", skip_ws(p + 5));
        }
        else if (strncmp(p, "opt:", 4) == 0) {
            if (cur == NULL || cur->optCount >= DIALOG_MAX_OPTS) {
                SDL_Log("Diyalog hatasi (%s:%d): fazla/yersiz opt", path, lineNo);
                fclose(f);
                return false;
            }
            // "opt: Metin -> hedef"
            const char *body = skip_ws(p + 4);
            const char *arrow = strstr(body, "->");
            if (arrow == NULL) {
                SDL_Log("Diyalog hatasi (%s:%d): opt'ta '->' yok", path, lineNo);
                fclose(f);
                return false;
            }
            DialogOpt *o = &cur->opts[cur->optCount++];
            size_t tlen = (size_t)(arrow - body);
            while (tlen > 0 && body[tlen-1] == ' ') tlen--;   // ok öncesi boşluk
            if (tlen >= DIALOG_OPT_LEN) tlen = DIALOG_OPT_LEN - 1;
            memcpy(o->text, body, tlen);
            o->text[tlen] = '\0';
            snprintf(o->target, DIALOG_ID_LEN, "%s", skip_ws(arrow + 2));
            lastOpt = o;
        }
        else if (strncmp(p, "req:", 4) == 0) {
            DialogVar var; char op[3]; float val;
            if (lastOpt == NULL || lastOpt->reqCount >= DIALOG_MAX_REQS ||
                !parse_cond_line(p + 4, &var, op, &val) ||
                (op[0] != '>' && op[0] != '<')) {
                SDL_Log("Diyalog hatasi (%s:%d): bozuk/yersiz req", path, lineNo);
                fclose(f);
                return false;
            }
            DialogReq *r = &lastOpt->reqs[lastOpt->reqCount++];
            r->var = var;
            r->lessEq = (op[0] == '<');
            r->value = val;
        }
        else if (strncmp(p, "fx:", 3) == 0) {
            DialogVar var; char op[3]; float val;
            if (lastOpt == NULL || lastOpt->fxCount >= DIALOG_MAX_FX ||
                !parse_cond_line(p + 3, &var, op, &val) ||
                (op[0] != '+' && op[0] != '-' && op[0] != '=') ||
                var == DVAR_SPEECH ||    // skill'e diyalog efekti yazılamaz
                var == DVAR_EVENT || var == DVAR_WEATHER) {   // dünya salt okunur
                SDL_Log("Diyalog hatasi (%s:%d): bozuk/yersiz fx", path, lineNo);
                fclose(f);
                return false;
            }
            DialogFx *x = &lastOpt->fx[lastOpt->fxCount++];
            x->var = var;
            x->op = op[0];
            x->value = val;
        }
        else {
            SDL_Log("Diyalog hatasi (%s:%d): taninmayan satir: %s", path, lineNo, p);
            fclose(f);
            return false;
        }
    }
    fclose(f);

    if (d->nodeCount == 0) {
        SDL_Log("Diyalog bos: %s", path);
        return false;
    }
    return true;
}

int dialog_find(const Dialog *d, const char *id)
{
    for (int i = 0; i < d->nodeCount; i++)
        if (strcmp(d->nodes[i].id, id) == 0)
            return i;
    return -1;
}

bool dialog_opt_speech_req(const DialogOpt *o, float *outThreshold)
{
    for (int i = 0; i < o->reqCount; i++)
        if (o->reqs[i].var == DVAR_SPEECH && !o->reqs[i].lessEq) {
            *outThreshold = o->reqs[i].value;
            return true;
        }
    return false;
}

// ---------------------------------------------------------------------------
// Typewriter — kod noktası bazlı (Türkçe karakter ortadan kesilmez)
// ---------------------------------------------------------------------------
static int utf8_count(const char *s)
{
    int n = 0;
    for (; *s; s++)
        if (((unsigned char)*s & 0xC0) != 0x80) n++;
    return n;
}


static float dlgTyped = 0.0f;    // yazılmış kod noktası (float — dt birikir)
static float dlgTick = 0.0f;     // tık sesi sayacı
static const Dialog *typeDlg = NULL;   // düğüm değişimini yakalamak için
static int typeNode = -1;

void dialog_type_reset(void)
{
    dlgTyped = 0.0f;
    dlgTick = 0.0f;
    typeDlg = NULL;
    typeNode = -1;
}

void dialog_type_reveal(void) { dlgTyped = 1e9f; }

void dialog_type_update(const Dialog *d, int nodeIdx, float dt)
{
    if (d == NULL || nodeIdx < 0 || nodeIdx >= d->nodeCount) return;
    if (d != typeDlg || nodeIdx != typeNode) {   // düğüm geçişi → baştan yaz
        typeDlg = d;
        typeNode = nodeIdx;
        dlgTyped = 0.0f;
        dlgTick = 0.0f;
    }
    int len = utf8_count(d->nodes[nodeIdx].text);
    if (dlgTyped >= (float)len) { dlgTyped = (float)len; return; }

    float before = dlgTyped;
    dlgTyped += DIALOG_CPS * dt;
    if (dlgTyped > (float)len) dlgTyped = (float)len;

    dlgTick += dlgTyped - before;   // her ~3 karakterde yumuşak tık
    if (dlgTick >= 3.0f) {
        dlgTick = 0.0f;
        audio_play(SFX_TYPETICK);
    }
}

bool dialog_type_done(const Dialog *d, int nodeIdx)
{
    if (d == NULL || nodeIdx < 0 || nodeIdx >= d->nodeCount) return true;
    if (d != typeDlg || nodeIdx != typeNode) return false;   // henüz başlamadı
    return dlgTyped >= (float)utf8_count(d->nodes[nodeIdx].text);
}

// --- ESO SUNUMU (v2): vurgulu metin çizimi + satır hit-test durumu ---
// Diyalog metninde *yıldızlar* arası ALTIN vurgulanır (anahtar kelimeler).
// Satır konumları hover/tıklama için saklanır (dialog_opt_row_at).

static float optRowYs[DIALOG_MAX_OPTS];
static float optPanelPX, optPanelPW;
static int optRowN = 0;

int dialog_opt_row_at(float mx, float my)
{
    if (mx < optPanelPX || mx > optPanelPX + optPanelPW) return -1;
    for (int i = 0; i < optRowN; i++)
        if (my >= optRowYs[i] && my < optRowYs[i] + 22.0f)
            return i;
    return -1;
}

// Kelime-sarmalı + *vurgu* renk geçişli + typewriter bütçeli metin.
// KRİTİK: satır kırma HEP TAM kelime genişliğiyle hesaplanır — kısmi
// gösterimde yerleşim OYNAMAZ (akış takılması bunun eksikliğiydi).
// renderer NULL = yalnız ölç (panel yüksekliği için). Dönen: yükseklik.
static float draw_markup_wrapped(R2D *renderer, TTF_Font *font,
                                 const char *text, float x, float y,
                                 float maxW, SDL_Color base, SDL_Color hi,
                                 int revealChars)
{
    float cx = x, cy = y;
    bool lit = false;
    bool limited = (revealChars >= 0);   // false = tamamını çiz (ölçüm/tamam)
    int remaining = revealChars;         // görünür karakter bütçesi
    char word[96];
    const char *p = text;
    while (*p != '\0') {
        if (*p == ' ') { p++; cx += 7.0f; continue; }
        // kelimeyi topla ('*' işaretleri renk değiştirir, çizilmez)
        int wn = 0;
        bool litAtStart = lit;
        while (*p != '\0' && *p != ' ') {
            if (*p == '*') { lit = !lit; p++; continue; }
            if (wn < (int)sizeof(word) - 5) {
                word[wn++] = *p;
                // utf8 devam baytlarını kopyala
                while ((p[1] & 0xC0) == 0x80 && wn < (int)sizeof(word) - 1)
                    word[wn++] = *++p;
            }
            p++;
        }
        word[wn] = '\0';
        if (wn == 0) continue;

        // Bütçe bitti: SONRAKİ kelimeler HİÇ çizilmez (eski kod negatif
        // bütçede guard'ı atlayıp hepsini tam çiziyordu — "full metin" bug'ı)
        if (limited && remaining <= 0) break;

        // Yerleşim TAM kelimeyle: önce tam genişliği ölç, satırı ona göre kır
        // (kısmi gösterimde yerleşim OYNAMAZ — akış titremez)
        int fullW = 0, th2 = 0;
        TTF_GetStringSize(font, word, 0, &fullW, &th2);
        if (cx + (float)fullW > x + maxW && cx > x) { cx = x; cy += 20.0f; }

        int chars = 0;
        for (int b = 0; b < wn; b++)
            if ((word[b] & 0xC0) != 0x80) chars++;

        // Bütçe kelimenin ortasında biterse kelimeyi KISMEN göster
        if (limited && chars > remaining) {
            int bi = 0, seen = 0;
            while (word[bi] != '\0') {
                if ((word[bi] & 0xC0) != 0x80) {
                    if (seen == remaining) break;
                    seen++;
                }
                bi++;
            }
            word[bi] = '\0';
        }

        if (renderer != NULL)
            draw_text(renderer, font, word, cx, cy, litAtStart ? hi : base);
        cx += (float)fullW + 7.0f;
        if (limited) remaining -= chars;
    }
    return cy - y + 20.0f;
}

void dialog_render(const Dialog *d, int nodeIdx, const char *npcName,
                   const DialogOptState *states,
                   R2D *renderer, TTF_Font *font, int winW, int winH,
                   R2DTexture *sheet, float frameSize,
                   float mouseX, float mouseY)
{
    if (nodeIdx < 0 || nodeIdx >= d->nodeCount) return;
    const DialogNode *n = &d->nodes[nodeIdx];
    optRowN = 0;

    // Duyarlı yerleşim: metin alanı pencereye göre; panel yüksekliği METNE
    // ve seçenek sayısına göre ÖLÇÜLÜR (uzun metin taşıp seçenekleri
    // ekran dışına itiyordu — Hakan raporu)
    float pad = 24.0f;
    float px = winW * 0.5f - 380.0f;
    if (px < pad + 176.0f) px = pad + 176.0f;
    float textW = (float)winW - px - 260.0f;
    if (textW > 860.0f) textW = 860.0f;
    if (textW < 320.0f) textW = 320.0f;

    float textH = (font != NULL)
                ? draw_markup_wrapped(NULL, font, n->text, 0.0f, 0.0f,
                                      textW, (SDL_Color){0,0,0,0},
                                      (SDL_Color){0,0,0,0}, -1)
                : 60.0f;
    int visOpts = 0;
    for (int i = 0; i < n->optCount; i++)
        if (states == NULL || !states[i].hidden) visOpts++;
    float panelH = 60.0f + textH + 12.0f + (float)visOpts * 22.0f + 20.0f;
    if (panelH < 220.0f) panelH = 220.0f;
    if (panelH > winH * 0.6f) panelH = winH * 0.6f;
    float py = (float)winH - panelH;
    r2d_set_blend(renderer, R2D_BLEND_ALPHA);
    r2d_set_color(renderer, 10, 10, 16, 236);
    SDL_FRect sheetRect = { 0.0f, py, (float)winW, panelH };
    r2d_fill_rect(renderer, &sheetRect);
    r2d_set_color(renderer, 255, 210, 60, 110);
    SDL_FRect topLine = { 0.0f, py, (float)winW, 2.0f };
    r2d_fill_rect(renderer, &topLine);

    // Portre: sol blok, büyük (ESO'nun NPC yakın çekimi hissi)
    if (sheet != NULL && frameSize > 0.0f) {
        float box = 150.0f;
        float bx = px - box - 26.0f, by = py + 40.0f;
        r2d_set_color(renderer, 20, 20, 30, 255);
        SDL_FRect pb = { bx - 4.0f, by - 4.0f, box + 8.0f, box + 8.0f };
        r2d_fill_rect(renderer, &pb);
        r2d_set_color(renderer, 255, 210, 60, 90);
        SDL_FRect pfrm = { bx - 4.0f, by - 4.0f, box + 8.0f, 2.0f };
        r2d_fill_rect(renderer, &pfrm);
        SDL_FRect src = { frameSize * 0.25f, frameSize * 0.02f,
                          frameSize * 0.5f, frameSize * 0.5f };
        float inw = box - 10.0f;
        int k = (int)(inw / src.w);
        if (k < 1) k = 1;
        float dw = src.w * (float)k, dh = src.h * (float)k;
        SDL_FRect dst = { bx + (box - dw) / 2.0f, by + (box - dh) / 2.0f, dw, dh };
        r2d_draw_texture(renderer, sheet, &src, &dst);
    }

    if (font == NULL) return;

    SDL_Color gold  = { 255, 210, 60, 255 };
    SDL_Color white = { 232, 230, 222, 255 };
    SDL_Color dim   = { 120, 120, 130, 255 };
    SDL_Color hover = { 255, 235, 160, 255 };

    // İsim başlığı + altın alt çizgi (ESO isim plakası)
    float y = py + 16.0f;
    draw_text(renderer, font, npcName, px, y, gold);
    int nw = 0, nh = 0;
    TTF_GetStringSize(font, npcName, 0, &nw, &nh);
    r2d_set_color(renderer, 255, 210, 60, 140);
    SDL_FRect nameLine = { px, y + 20.0f, (float)nw + 30.0f, 1.0f };
    r2d_fill_rect(renderer, &nameLine);
    y += 32.0f;

    // Metin: typewriter + *vurgu* (görünür karakter bütçesi — işaretler sayılmaz).
    // Düğüm henüz type_update görmediyse 0 (boş) — ESKİDEN -1 (tam metin)
    // dönüyordu: ilk kare tam metni basıp sonraki karede sıfırdan yazıyordu
    // ("üstünden geçen efekt" bug'ı — dialog_type_done ile de çelişiyordu)
    int typedN = (d == typeDlg && nodeIdx == typeNode)
               ? (int)dlgTyped : 0;
    y += draw_markup_wrapped(renderer, font, n->text, px, y, textW,
                             white, gold, typedN) + 8.0f;

    if (!dialog_type_done(d, nodeIdx)) {
        SDL_Color hint = { 150, 150, 160, 160 };
        draw_text(renderer, font, "[E / tık]",
                  (float)winW - 110.0f, py + panelH - 26.0f, hint);
        return;   // seçenekler metin bitmeden görünmez
    }

    optPanelPX = px - 8.0f;
    optPanelPW = textW + 16.0f;
    for (int i = 0; i < n->optCount; i++) {
        const DialogOpt *o = &n->opts[i];
        const DialogOptState *st = states ? &states[i] : NULL;

        char label[32] = "";
        float th;
        if (dialog_opt_speech_req(o, &th)) {
            if (st != NULL && st->chancePct >= 0 && st->chancePct < 100)
                snprintf(label, sizeof(label), "[Speech %d - %%%d] ",
                         (int)th, st->chancePct);
            else
                snprintf(label, sizeof(label), "[Speech %d] ", (int)th);
        }

        if (st != NULL && st->hidden) continue;
        bool inactive = st != NULL && (st->locked || st->burned);
        bool hov = !inactive && mouseX >= optPanelPX &&
                   mouseX <= optPanelPX + optPanelPW &&
                   mouseY >= y - 2.0f && mouseY < y + 20.0f;
        if (hov) {   // ESO hover şeridi
            r2d_set_blend(renderer, R2D_BLEND_ALPHA);
            r2d_set_color(renderer, 255, 210, 60, 26);
            SDL_FRect hrow = { optPanelPX, y - 2.0f, optPanelPW, 22.0f };
            r2d_fill_rect(renderer, &hrow);
        }
        char buf[DIALOG_OPT_LEN + 48];
        snprintf(buf, sizeof(buf), "%d. %s%s%s", i + 1, label, o->text,
                 (st != NULL && st->burned) ? " (bugün olmaz)" : "");
        draw_text(renderer, font, buf, px, y,
                  inactive ? dim : (hov ? hover : (SDL_Color){ 190, 210, 245, 255 }));
        if (optRowN < DIALOG_MAX_OPTS) optRowYs[optRowN++] = y - 2.0f;
        y += 22.0f;
    }
}
