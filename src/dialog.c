#include <stdio.h>
#include <string.h>
#include "dialog.h"
#include "font.h"

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

void dialog_render(const Dialog *d, int nodeIdx, const char *npcName,
                   SDL_Renderer *renderer, TTF_Font *font, int winW, int winH)
{
    if (nodeIdx < 0 || nodeIdx >= d->nodeCount) return;
    const DialogNode *n = &d->nodes[nodeIdx];

    // Panel: alt bölge, hotbar'ın üstünde
    float pad = 16.0f;
    float panelW = winW * 0.6f;
    float panelH = 150.0f;
    float px = (winW - panelW) / 2.0f;
    float py = winH - panelH - 80.0f;   // hotbar (~60px) + boşluk

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 235);
    SDL_FRect panel = { px, py, panelW, panelH };
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 200, 200, 210, 255);
    SDL_RenderRect(renderer, &panel);

    if (font == NULL) return;

    // NPC adı + metin (sarılı) + seçenekler
    SDL_Color gold  = { 255, 210, 60, 255 };
    SDL_Color white = { 235, 235, 245, 255 };
    SDL_Color gray  = { 170, 200, 255, 255 };

    float y = py + pad;
    draw_text(renderer, font, npcName, px + pad, y, gold);
    y += 24.0f;
    y += draw_text_wrapped(renderer, font, n->text,
                           px + pad, y, panelW - pad * 2, white) + 10.0f;

    for (int i = 0; i < n->optCount; i++) {
        char buf[DIALOG_OPT_LEN + 8];
        snprintf(buf, sizeof(buf), "%d) %s", i + 1, n->opts[i].text);
        draw_text(renderer, font, buf, px + pad, y, gray);
        y += 20.0f;
    }
}
