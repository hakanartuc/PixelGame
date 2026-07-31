#include "jsonx.h"
#include <SDL3/SDL.h>

// Yorumları boşlukla değiştirir (silmez!) — böylece karakter ofsetleri korunur
// ve hata mesajındaki satır/kolon hesabı ORİJİNAL dosyayla eşleşir. Silseydik
// hata "3. satır" derken kullanıcı 5. satıra bakardı.
// Dize içindeki // korunur: "http://..." bozulmasın.
static void strip_comments(char *s)
{
    bool inStr = false;
    for (size_t i = 0; s[i] != '\0'; i++) {
        if (inStr) {
            if (s[i] == '\\' && s[i + 1] != '\0') { i++; continue; }  // kaçış
            if (s[i] == '"') inStr = false;
            continue;
        }
        if (s[i] == '"') { inStr = true; continue; }
        if (s[i] == '/' && s[i + 1] == '/') {
            while (s[i] != '\0' && s[i] != '\n') s[i++] = ' ';
            if (s[i] == '\0') break;
            continue;   // '\n' korunur (satır sayımı bozulmasın)
        }
        if (s[i] == '/' && s[i + 1] == '*') {
            s[i] = s[i + 1] = ' ';
            i += 2;
            while (s[i] != '\0' && !(s[i] == '*' && s[i + 1] == '/')) {
                if (s[i] != '\n') s[i] = ' ';   // satır sonları korunur
                i++;
            }
            if (s[i] != '\0') { s[i] = ' '; s[i + 1] = ' '; i++; }
        }
    }
}

// cJSON'un hata işaretçisini satır/kolona çevirip bağlamla birlikte basar.
static void report_error(const char *text, const char *errPtr, const char *name)
{
    int line = 1, col = 1;
    const char *p = text;
    if (errPtr != NULL && errPtr >= text) {
        for (; p < errPtr && *p != '\0'; p++) {
            if (*p == '\n') { line++; col = 1; } else { col++; }
        }
    }
    // Hatalı satırı kes ve göster — "sondaki fazla virgül" gözle görülsün
    const char *ls = errPtr;
    while (ls > text && *(ls - 1) != '\n') ls--;
    const char *le = ls;
    while (*le != '\0' && *le != '\n') le++;
    int len = (int)(le - ls);
    if (len > 96) len = 96;
    SDL_Log("JSON hatasi: %s satir %d kolon %d", name ? name : "(metin)", line, col);
    SDL_Log("  > %.*s", len, ls);
    SDL_Log("  (ipucu: JSON'da sondaki fazla virgul ve tek tirnak GECERSIZ)");
}

cJSON *jsonx_parse(const char *text, const char *name)
{
    if (text == NULL) return NULL;
    size_t n = SDL_strlen(text);
    char *copy = (char *)SDL_malloc(n + 1);
    if (copy == NULL) return NULL;
    SDL_memcpy(copy, text, n + 1);
    strip_comments(copy);

    cJSON *root = cJSON_Parse(copy);
    if (root == NULL) {
        const char *err = cJSON_GetErrorPtr();
        // Hata ofsetini ORİJİNAL metne taşı (uzunluklar aynı, ofset korunuyor)
        const char *mapped = (err != NULL && err >= copy && err <= copy + n)
                             ? text + (err - copy) : text;
        report_error(text, mapped, name);
    }
    SDL_free(copy);
    return root;
}

cJSON *jsonx_load(const char *path)
{
    size_t size = 0;
    char *data = (char *)SDL_LoadFile(path, &size);
    if (data == NULL) {
        SDL_Log("JSON okunamadi: %s (%s)", path, SDL_GetError());
        return NULL;
    }
    cJSON *root = jsonx_parse(data, path);
    SDL_free(data);
    return root;
}

// --- Getter'lar: obj NULL, alan yok ya da tip yanlışsa def döner ---
double jsonx_num(const cJSON *obj, const char *key, double def)
{
    const cJSON *it = cJSON_GetObjectItemCaseSensitive(obj, key);
    return cJSON_IsNumber(it) ? it->valuedouble : def;
}
int   jsonx_int(const cJSON *obj, const char *key, int def)
{ return (int)jsonx_num(obj, key, (double)def); }

float jsonx_flt(const cJSON *obj, const char *key, float def)
{ return (float)jsonx_num(obj, key, (double)def); }

bool jsonx_bool(const cJSON *obj, const char *key, bool def)
{
    const cJSON *it = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsBool(it)) return cJSON_IsTrue(it);
    return def;
}

const char *jsonx_str(const cJSON *obj, const char *key, const char *def)
{
    const cJSON *it = cJSON_GetObjectItemCaseSensitive(obj, key);
    return (cJSON_IsString(it) && it->valuestring != NULL) ? it->valuestring : def;
}

const cJSON *jsonx_arr(const cJSON *obj, const char *key)
{
    const cJSON *it = cJSON_GetObjectItemCaseSensitive(obj, key);
    return cJSON_IsArray(it) ? it : NULL;
}

const cJSON *jsonx_obj(const cJSON *obj, const char *key)
{
    const cJSON *it = cJSON_GetObjectItemCaseSensitive(obj, key);
    return cJSON_IsObject(it) ? it : NULL;
}

bool jsonx_save(const char *path, const cJSON *root, bool pretty)
{
    if (root == NULL || path == NULL) return false;
    char *text = pretty ? cJSON_Print((cJSON *)root)
                        : cJSON_PrintUnformatted((cJSON *)root);
    if (text == NULL) { SDL_Log("JSON yazilamadi (bellek): %s", path); return false; }
    SDL_IOStream *io = SDL_IOFromFile(path, "wb");
    bool ok = false;
    if (io != NULL) {
        size_t n = SDL_strlen(text);
        ok = (SDL_WriteIO(io, text, n) == n);
        // Dosya sonu newline'ı: git "\ No newline at end of file" göstermesin
        if (ok) ok = (SDL_WriteIO(io, "\n", 1) == 1);
        SDL_CloseIO(io);
    }
    if (!ok) SDL_Log("JSON yazilamadi: %s (%s)", path, SDL_GetError());
    cJSON_free(text);
    return ok;
}
