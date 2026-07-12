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

// "speech" / "gold" / "rep" / "bounty" → DialogVar. Tanınmazsa false.
static bool parse_var(const char *s, size_t len, DialogVar *out)
{
    if (len == 6 && strncmp(s, "speech", 6) == 0) { *out = DVAR_SPEECH; return true; }
    if (len == 4 && strncmp(s, "gold", 4) == 0)   { *out = DVAR_GOLD;   return true; }
    if (len == 3 && strncmp(s, "rep", 3) == 0)    { *out = DVAR_REP;    return true; }
    if (len == 6 && strncmp(s, "bounty", 6) == 0) { *out = DVAR_BOUNTY; return true; }
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
                var == DVAR_SPEECH) {   // skill'e diyalog efekti yazılamaz
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

void dialog_render(const Dialog *d, int nodeIdx, const char *npcName,
                   const DialogOptState *states,
                   R2D *renderer, TTF_Font *font, int winW, int winH,
                   R2DTexture *sheet, float frameSize)
{
    if (nodeIdx < 0 || nodeIdx >= d->nodeCount) return;
    const DialogNode *n = &d->nodes[nodeIdx];

    // Panel: alt bölge, hotbar'ın üstünde (portre varsa sola taşar)
    float pad = 16.0f;
    float panelW = winW * 0.6f;
    float panelH = 150.0f;
    float px = (winW - panelW) / 2.0f;
    float py = winH - panelH - 80.0f;   // hotbar (~60px) + boşluk

    ui_panel(renderer, px, py, panelW, panelH);

    // Portre: idle-down ilk karenin baş bölgesi, panel soluna tam-piksel katla
    if (sheet != NULL && frameSize > 0.0f) {
        float box = 96.0f;
        float bx = px - box - 10.0f, by = py + panelH - box;
        ui_panel(renderer, bx, by, box, box);
        SDL_FRect src = { frameSize * 0.25f, frameSize * 0.02f,
                          frameSize * 0.5f, frameSize * 0.5f };
        float inw = box - 12.0f;
        int k = (int)(inw / src.w);
        if (k < 1) k = 1;
        float dw = src.w * (float)k, dh = src.h * (float)k;
        SDL_FRect dst = { bx + (box - dw) / 2.0f, by + (box - dh) / 2.0f, dw, dh };
        r2d_draw_texture(renderer, sheet, &src, &dst);
    }

    if (font == NULL) return;

    // NPC adı + metin (typewriter, sarılı) + seçenekler (metin bitince)
    SDL_Color gold  = { 255, 210, 60, 255 };
    SDL_Color white = { 235, 235, 245, 255 };
    SDL_Color gray  = { 170, 200, 255, 255 };

    float y = py + pad;
    draw_text(renderer, font, npcName, px + pad, y, gold);
    y += 24.0f;

    char shown[DIALOG_TEXT_LEN];
    int typedN = (d == typeDlg && nodeIdx == typeNode)
               ? (int)dlgTyped : utf8_count(n->text);
    int nbytes = utf8_bytes_for(n->text, typedN);
    memcpy(shown, n->text, (size_t)nbytes);
    shown[nbytes] = '\0';
    y += draw_text_wrapped(renderer, font, shown,
                           px + pad, y, panelW - pad * 2, white) + 10.0f;

    if (!dialog_type_done(d, nodeIdx)) {
        SDL_Color hint = { 150, 150, 160, 160 };
        draw_text(renderer, font, "[E]", px + panelW - 40.0f, py + panelH - 26.0f, hint);
        return;   // seçenekler metin bitmeden görünmez
    }

    SDL_Color dim = { 120, 120, 130, 255 };
    for (int i = 0; i < n->optCount; i++) {
        const DialogOpt *o = &n->opts[i];
        const DialogOptState *st = states ? &states[i] : NULL;

        // Etiket: speech eşiği (+ eşik altındaysa şans yüzdesi)
        char label[32] = "";
        float th;
        if (dialog_opt_speech_req(o, &th)) {
            if (st != NULL && st->chancePct >= 0 && st->chancePct < 100)
                snprintf(label, sizeof(label), "[Speech %d - %%%d] ",
                         (int)th, st->chancePct);
            else
                snprintf(label, sizeof(label), "[Speech %d] ", (int)th);
        }

        bool inactive = st != NULL && (st->locked || st->burned);
        char buf[DIALOG_OPT_LEN + 48];
        snprintf(buf, sizeof(buf), "%d) %s%s%s", i + 1, label, o->text,
                 (st != NULL && st->burned) ? " (bugün olmaz)" : "");
        draw_text(renderer, font, buf, px + pad, y, inactive ? dim : gray);
        y += 20.0f;
    }
}
