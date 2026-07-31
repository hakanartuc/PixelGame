#include "uilayout.h"
#include "jsonx.h"
#include <SDL3/SDL.h>

#define UI_MAX_ITEMS 96
#define UI_NAME_LEN  40

typedef struct {
    char name[UI_NAME_LEN];
    UiAnchor anchor;
    UiAlign  align;
    UiVAlign valign;
    float ox, oy;
    float w, h;      // 0 = BELİRTİLMEMİŞ → kodun kendi ölçüsü geçerli
} UiItem;

static UiItem s_items[UI_MAX_ITEMS];
static int    s_count = 0;
static int    s_winW = 1280, s_winH = 720;
static int    s_version = 0;

// Dosyadaki çapa adları. Türkçe ve ilk duyuşta anlaşılır (dil standardı kuralı);
// dosyayı elle düzenleyen kişi "top_right" yerine "sag-ust" görsün.
static const char *ANCHOR_NAMES[UI_ANCHOR_COUNT] = {
    "sol-ust", "ust-orta", "sag-ust",
    "sol-orta", "orta",    "sag-orta",
    "sol-alt", "alt-orta", "sag-alt",
};

static const char *ALIGN_NAMES[UI_ALIGN_COUNT]   = { "sol", "orta", "sag" };
static const char *VALIGN_NAMES[UI_VALIGN_COUNT] = { "ust", "orta", "alt" };

const char *uilayout_anchor_name(UiAnchor a)
{
    if (a < 0 || a >= UI_ANCHOR_COUNT) return ANCHOR_NAMES[0];
    return ANCHOR_NAMES[a];
}

const char *uilayout_align_name(UiAlign a)
{
    if (a < 0 || a >= UI_ALIGN_COUNT) return ALIGN_NAMES[0];
    return ALIGN_NAMES[a];
}

static UiAlign align_from_name(const char *s)
{
    if (s == NULL) return UI_ALIGN_LEFT;
    for (int i = 0; i < UI_ALIGN_COUNT; i++)
        if (SDL_strcasecmp(s, ALIGN_NAMES[i]) == 0) return (UiAlign)i;
    SDL_Log("uilayout: bilinmeyen hiza '%s' — sol varsayildi", s);
    return UI_ALIGN_LEFT;
}

const char *uilayout_valign_name(UiVAlign a)
{
    if (a < 0 || a >= UI_VALIGN_COUNT) return VALIGN_NAMES[0];
    return VALIGN_NAMES[a];
}

static UiVAlign valign_from_name(const char *s)
{
    if (s == NULL) return UI_VALIGN_TOP;
    for (int i = 0; i < UI_VALIGN_COUNT; i++)
        if (SDL_strcasecmp(s, VALIGN_NAMES[i]) == 0) return (UiVAlign)i;
    SDL_Log("uilayout: bilinmeyen dikey hiza '%s' — ust varsayildi", s);
    return UI_VALIGN_TOP;
}

static UiAnchor anchor_from_name(const char *s)
{
    if (s == NULL) return UI_ANCHOR_TOP_LEFT;
    for (int i = 0; i < UI_ANCHOR_COUNT; i++)
        if (SDL_strcasecmp(s, ANCHOR_NAMES[i]) == 0) return (UiAnchor)i;
    SDL_Log("uilayout: bilinmeyen capa '%s' — sol-ust varsayildi", s);
    return UI_ANCHOR_TOP_LEFT;
}

void uilayout_set_window(int w, int h)
{
    if (w > 0) s_winW = w;
    if (h > 0) s_winH = h;
}

bool uilayout_load(const char *path)
{
    s_count = 0;
    cJSON *root = jsonx_load(path);
    if (root == NULL) {
        SDL_Log("uilayout: yerlesim yuklenemedi, kod varsayilanlari kullanilacak");
        return false;
    }
    s_version = jsonx_int(root, "version", 0);
    const cJSON *items = jsonx_obj(root, "ogeler");
    if (items == NULL) {
        SDL_Log("uilayout: 'ogeler' nesnesi yok");
        cJSON_Delete(root);
        return false;
    }
    const cJSON *it = NULL;
    cJSON_ArrayForEach(it, items) {
        if (it->string == NULL) continue;
        if (s_count >= UI_MAX_ITEMS) {
            SDL_Log("uilayout: UI_MAX_ITEMS (%d) doldu — '%s' ve sonrasi atlandi",
                    UI_MAX_ITEMS, it->string);
            break;
        }
        UiItem *d = &s_items[s_count];
        SDL_memset(d, 0, sizeof(*d));
        SDL_strlcpy(d->name, it->string, UI_NAME_LEN);
        d->anchor = anchor_from_name(jsonx_str(it, "capa", "sol-ust"));
        d->align  = align_from_name(jsonx_str(it, "hiza", "sol"));
        d->valign = valign_from_name(jsonx_str(it, "dikey", "ust"));
        d->ox = jsonx_flt(it, "x", 0.0f);
        d->oy = jsonx_flt(it, "y", 0.0f);
        // w/h İSTEĞE BAĞLI. Yoksa 0 kalır ve kodun hesabı geçerli olur — can
        // barının genişliği max stat'la büyüyor, onu veriye çivilemek yanlış
        // olurdu. Yalnız panellerde (envanter, dükkan) boyut anlamlı.
        d->w = jsonx_flt(it, "w", 0.0f);
        d->h = jsonx_flt(it, "h", 0.0f);
        if (d->w < 0.0f) d->w = 0.0f;
        if (d->h < 0.0f) d->h = 0.0f;
        s_count++;
    }
    cJSON_Delete(root);
    SDL_Log("uilayout: %s yuklendi (v%d, %d oge)", path, s_version, s_count);
    return true;
}

void uilayout_free(void) { s_count = 0; }

// Çapa noktasını pencere boyutundan çöz. Öğenin kendi genişliğini BİLMİYORUZ —
// sağ/alt çapalarda hizalama çağıranın işi (metin genişliği TTF_GetStringSize
// ile ölçülüyor, burada font yok). Bu bilinçli: yerleşim sistemi çapa noktasını
// verir, hizalamayı çizen kod yapar.
static void anchor_origin(UiAnchor a, float *ax, float *ay)
{
    float w = (float)s_winW, h = (float)s_winH;
    switch (a) {
    case UI_ANCHOR_TOP_LEFT:      *ax = 0;      *ay = 0;      break;
    case UI_ANCHOR_TOP_CENTER:    *ax = w*0.5f; *ay = 0;      break;
    case UI_ANCHOR_TOP_RIGHT:     *ax = w;      *ay = 0;      break;
    case UI_ANCHOR_MID_LEFT:      *ax = 0;      *ay = h*0.5f; break;
    case UI_ANCHOR_CENTER:        *ax = w*0.5f; *ay = h*0.5f; break;
    case UI_ANCHOR_MID_RIGHT:     *ax = w;      *ay = h*0.5f; break;
    case UI_ANCHOR_BOTTOM_LEFT:   *ax = 0;      *ay = h;      break;
    case UI_ANCHOR_BOTTOM_CENTER: *ax = w*0.5f; *ay = h;      break;
    case UI_ANCHOR_BOTTOM_RIGHT:  *ax = w;      *ay = h;      break;
    default:                      *ax = 0;      *ay = 0;      break;
    }
}

void uilayout_anchor_point(UiAnchor a, float *outX, float *outY)
{
    float ax = 0.0f, ay = 0.0f;
    anchor_origin(a, &ax, &ay);
    if (outX) *outX = ax;
    if (outY) *outY = ay;
}

SDL_FRect uilayout_panel(const char *name, float defX, float defY, float defW)
{
    float w = defW;
    uilayout_get_size(name, defW, 0.0f, &w, NULL);
    float x, y;
    uilayout_get_aligned(name, defX, defY, w, 0.0f, &x, &y);
    return (SDL_FRect){ x, y, w, 0.0f };
}

UiAnchor uilayout_best_anchor(float px, float py)
{
    // Ekranı üçe-üç böl, nokta hangi hücreye düşüyorsa o çapa. Sezgisel olan
    // bu: sağ üstteki bir öğe sağ üst köşeye bağlanmalı, çünkü pencere
    // büyüdüğünde onun sabit kalmasını beklediğin kenar orası.
    // Enum sırası satır-öncelikli (ust-sol, ust-orta, ust-sag, orta-sol, ...)
    // olduğu için satır*3 + kolon doğrudan çapayı veriyor.
    float w = (float)s_winW, h = (float)s_winH;
    int col = (px < w / 3.0f) ? 0 : (px < w * 2.0f / 3.0f) ? 1 : 2;
    int row = (py < h / 3.0f) ? 0 : (py < h * 2.0f / 3.0f) ? 1 : 2;
    return (UiAnchor)(row * 3 + col);
}

bool uilayout_offset_suspect(int i)
{
    if (i < 0 || i >= s_count) return false;
    // Ofset pencerenin yarısından büyükse çapa büyük olasılıkla yanlış: öğe
    // yakın bir kenara bağlı olsa bu kadar büyük bir sayıya ihtiyaç olmazdı.
    // "sol-orta çapası + x=1580" tam bu deseni veriyor ve yeniden
    // boyutlandırmada öğeyi ekran dışına atıyor.
    float lim = (float)((s_winW > s_winH ? s_winW : s_winH)) * 0.5f;
    float ax = SDL_fabsf(s_items[i].ox), ay = SDL_fabsf(s_items[i].oy);
    return (ax > lim || ay > lim);
}

void uilayout_get(const char *name, float defX, float defY, float *outX, float *outY)
{
    if (outX == NULL || outY == NULL) return;
    for (int i = 0; i < s_count; i++) {
        if (SDL_strcmp(s_items[i].name, name) != 0) continue;
        float ax, ay;
        anchor_origin(s_items[i].anchor, &ax, &ay);
        *outX = ax + s_items[i].ox;
        *outY = ay + s_items[i].oy;
        return;
    }
    // Bulunamadı: koddaki varsayılana düş. Uyarıyı bir kez bas — sessiz kalmak
    // hatayı gizler, her karede basmak logu boğar.
    static char warned[UI_MAX_ITEMS][UI_NAME_LEN];
    static int warnedCount = 0;
    bool seen = false;
    for (int i = 0; i < warnedCount; i++)
        if (SDL_strcmp(warned[i], name) == 0) { seen = true; break; }
    if (!seen && warnedCount < UI_MAX_ITEMS) {
        SDL_strlcpy(warned[warnedCount++], name, UI_NAME_LEN);
        SDL_Log("uilayout: '%s' yerlesimde yok — kod varsayilani (%.0f,%.0f)",
                name, defX, defY);
    }
    *outX = defX;
    *outY = defY;
}

void uilayout_get_aligned(const char *name, float defX, float defY,
                          float contentW, float contentH,
                          float *outX, float *outY)
{
    if (outX == NULL || outY == NULL) return;
    uilayout_get(name, defX, defY, outX, outY);
    // Hizayı ÖĞE BULUNDUYSA uygula. Bulunamadıysa dokunma: defX/defY zaten
    // çağıranın "eski davranış" konumu, üstüne hiza uygulamak onu bozar.
    for (int i = 0; i < s_count; i++) {
        if (SDL_strcmp(s_items[i].name, name) != 0) continue;
        if (s_items[i].align == UI_ALIGN_CENTER)      *outX -= contentW * 0.5f;
        else if (s_items[i].align == UI_ALIGN_RIGHT)  *outX -= contentW;
        if (s_items[i].valign == UI_VALIGN_MIDDLE)    *outY -= contentH * 0.5f;
        else if (s_items[i].valign == UI_VALIGN_BOTTOM) *outY -= contentH;
        return;
    }
}

void uilayout_get_size(const char *name, float defW, float defH,
                       float *outW, float *outH)
{
    if (outW) *outW = defW;
    if (outH) *outH = defH;
    for (int i = 0; i < s_count; i++) {
        if (SDL_strcmp(s_items[i].name, name) != 0) continue;
        if (outW && s_items[i].w > 0.0f) *outW = s_items[i].w;
        if (outH && s_items[i].h > 0.0f) *outH = s_items[i].h;
        return;
    }
}

int uilayout_count(void) { return s_count; }

const char *uilayout_name_at(int i)
{ return (i >= 0 && i < s_count) ? s_items[i].name : NULL; }

bool uilayout_raw_at(int i, UiAnchor *anchor, UiAlign *align, UiVAlign *valign,
                     float *ox, float *oy)
{
    if (i < 0 || i >= s_count) return false;
    if (anchor) *anchor = s_items[i].anchor;
    if (align)  *align  = s_items[i].align;
    if (valign) *valign = s_items[i].valign;
    if (ox) *ox = s_items[i].ox;
    if (oy) *oy = s_items[i].oy;
    return true;
}

void uilayout_set_raw(int i, UiAnchor anchor, UiAlign align, UiVAlign valign,
                      float ox, float oy)
{
    if (i < 0 || i >= s_count) return;
    if (anchor >= 0 && anchor < UI_ANCHOR_COUNT)  s_items[i].anchor = anchor;
    if (align  >= 0 && align  < UI_ALIGN_COUNT)   s_items[i].align  = align;
    if (valign >= 0 && valign < UI_VALIGN_COUNT)  s_items[i].valign = valign;
    s_items[i].ox = ox;
    s_items[i].oy = oy;
}

bool uilayout_size_at(int i, float *w, float *h)
{
    if (i < 0 || i >= s_count) return false;
    if (w) *w = s_items[i].w;
    if (h) *h = s_items[i].h;
    // GENİŞLİK yeterli. Panellerin çoğunda yükseklik satır sayısından türüyor
    // (dükkan: stockCount × ROW_H) ve veriye alınamaz; genişlik ise gerçek bir
    // tasarım kararı. h == 0 dönen öğede düzenleyici yalnız genişliği düzenler.
    return (s_items[i].w > 0.0f);
}

void uilayout_set_size(int i, float w, float h)
{
    if (i < 0 || i >= s_count) return;
    // Boyutu olan bir öğeyi sıfıra indirmeye izin YOK: 0 "belirtilmemiş"
    // anlamına geliyor, kaza sonucu 0'a çekilen panel kodun varsayılanına
    // düşerdi ve düzenleyicide sebebi anlaşılmazdı.
    if (w >= 8.0f) s_items[i].w = w;
    if (h >= 8.0f) s_items[i].h = h;
}

bool uilayout_point_at(int i, float *outX, float *outY)
{
    if (i < 0 || i >= s_count) return false;
    float ax, ay;
    anchor_origin(s_items[i].anchor, &ax, &ay);
    if (outX) *outX = ax + s_items[i].ox;
    if (outY) *outY = ay + s_items[i].oy;
    return true;
}

bool uilayout_save(const char *path)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) return false;
    cJSON_AddNumberToObject(root, "version", s_version > 0 ? s_version : 1);
    cJSON *items = cJSON_AddObjectToObject(root, "ogeler");
    if (items == NULL) { cJSON_Delete(root); return false; }
    // Yazma sırası bellekteki sıra = dosyadaki okuma sırası. Böylece kaydetmek
    // git diff'inde satırları karıştırmıyor (yalnız değişen öğe görünüyor).
    for (int i = 0; i < s_count; i++) {
        cJSON *o = cJSON_AddObjectToObject(items, s_items[i].name);
        if (o == NULL) continue;
        cJSON_AddStringToObject(o, "capa", uilayout_anchor_name(s_items[i].anchor));
        // "hiza" yalnız varsayılandan farklıysa yazılır — dosya sade kalsın,
        // her satıra "hiza": "sol" eklemek gürültü olur
        if (s_items[i].align != UI_ALIGN_LEFT)
            cJSON_AddStringToObject(o, "hiza", uilayout_align_name(s_items[i].align));
        if (s_items[i].valign != UI_VALIGN_TOP)
            cJSON_AddStringToObject(o, "dikey",
                                    uilayout_valign_name(s_items[i].valign));
        cJSON_AddNumberToObject(o, "x", (double)s_items[i].ox);
        cJSON_AddNumberToObject(o, "y", (double)s_items[i].oy);
        // w/h yalnız belirtilmişse yazılır — 0'ları dosyaya serpmek "boyut
        // veriden geliyor" yanılgısı yaratırdı
        if (s_items[i].w > 0.0f) cJSON_AddNumberToObject(o, "w", (double)s_items[i].w);
        if (s_items[i].h > 0.0f) cJSON_AddNumberToObject(o, "h", (double)s_items[i].h);
    }
    bool ok = jsonx_save(path, root, true);
    cJSON_Delete(root);
    if (ok) SDL_Log("uilayout: %s kaydedildi (%d oge) — DIKKAT: dosyadaki el "
                    "yazisi yorumlar KORUNMADI", path, s_count);
    return ok;
}
