#include <SDL3/SDL.h>
#include "groundfx.h"
#include "tilemap.h"

// stb_image r2d.c'de zaten implementation ile derleniyor; burada yalnız
// prototipini alıyoruz. Karo PNG'sini CPU'da okumak ŞART: parçanın alfasını
// maskelemek için piksellere erişmek gerekiyor (GPU dokusundan okunamıyor).
extern unsigned char *stbi_load(const char *f, int *x, int *y, int *comp, int req);
extern void stbi_image_free(void *p);

// Davranış sınıfı (bkz. groundfx.h: karo ALANLARINDAN türetilir, id'den değil)
typedef enum { GK_PLANT = 0, GK_ROCK, GK_GRAIN } GroundClass;

typedef struct {
    bool active;
    float x, y, vx, vy;
    float ang, spin;
    float life, maxLife;
    float size;                 // dünya px (kare)
    R2DTexture *tex;            // KARO dokusu — parça ondan kesiliyor
    float sx, sy, sw;           // doku içindeki pencere (doku pikseli)
    GroundClass cls;
} Chunk;

static Chunk s_ch[GROUNDFX_MAX];
static R2DTexture **s_tiles = NULL;      // main'in karo doku dizisi (salt okunur)

// ── MASKELENMİŞ PARÇA ATLASI.
// HATA GEÇMİŞİ: parça, karo dokusundan doğrudan kesilen bir kare olarak
// çiziliyordu. Doku doğruydu ama alfa her yerde 255 olduğu için kenar zorunlu
// olarak KESKİN KARE kalıyordu — Hakan iki tur bildirdi: "hala parçacıklar kare
// kare gözüküyor... daha yumuşak parçalar halinde yapsan".
// Kareyi yumuşatmak için alfayı MASKELEMEK gerekiyor ve bu GPU dokusundan
// yapılamaz (r2d'de çarpımsal alfa blend'i yok, çok-dokulu geçiş de yok).
// ÇÖZÜM: açılışta karo PNG'leri CPU'da okunuyor (stb_image), her karo için DÖRT
// yumuşak parça varyantı üretilip TEK atlasa basılıyor. Maske düzensiz-yuvarlak
// (yarıçap açıya göre modüle — decal.c ve is pulunda kanıtlı numara), kenarı
// 6 kademeli (biçim alfası 2-3 kademe olunca "kağıt gibi" duruyordu).
// YAN KAZANÇ: bütün parçalar artık TEK dokudan çiziliyor, yani batch geri geldi.
// Eskiden karo başına doku değişiyordu ve her parça batch'i kırıyordu.
#define GF_FRAG 8            // parça hücresi (doku pikseli) → 32 dünya px tavanı
#define GF_VAR  4            // karo başına parça varyantı
static R2DTexture *s_fragTex = NULL;
static bool s_fragHas[MAX_TILE_TYPES];

static inline float gf_soft(float c)
{
    if (c <= 0.0f) return 0.0f;
    float q = SDL_floorf(c * 6.0f + 0.5f) / 6.0f;
    return (q > 1.0f) ? 1.0f : q;
}

// Düzensiz-yuvarlak maske. v = varyant (siluet ondan türüyor).
static float gf_mask(int v, int i, int j)
{
    float dx = ((float)i + 0.5f) / GF_FRAG * 2.0f - 1.0f;
    float dy = ((float)j + 0.5f) / GF_FRAG * 2.0f - 1.0f;
    float fv = (float)v;
    float dyf = dy * (1.0f + 0.18f * SDL_sinf(fv * 2.3f));
    float r = SDL_sqrtf(dx * dx + dyf * dyf);
    float ang = SDL_atan2f(dyf, dx);
    float R = 0.86f + 0.15f * SDL_sinf(ang * 3.0f + fv * 1.7f)
                    + 0.09f * SDL_sinf(ang * 5.0f + fv * 2.9f);
    if (r >= R) return 0.0f;
    return gf_soft(1.0f - SDL_powf(r / R, 2.4f) * 0.92f);
}

// Açılışta bir kez: karo PNG'lerinden yumuşak parça atlası bas.
static void gf_bake_fragments(R2D *renderer)
{
    const int AW = GF_VAR * GF_FRAG, AH = MAX_TILE_TYPES * GF_FRAG;
    static Uint8 atlas[GF_VAR * GF_FRAG * MAX_TILE_TYPES * GF_FRAG * 4];
    SDL_memset(atlas, 0, sizeof atlas);
    for (int id = 0; id < MAX_TILE_TYPES; id++) {
        s_fragHas[id] = false;
        const char *path = TILE_TYPES[id].texturePath;
        if (path == NULL) continue;
        int w = 0, h = 0, comp = 0;
        unsigned char *px = stbi_load(path, &w, &h, &comp, 4);
        if (px == NULL || w < GF_FRAG || h < GF_FRAG) {
            if (px) stbi_image_free(px);
            continue;
        }
        for (int v = 0; v < GF_VAR; v++) {
            // Kaynak pencere: 16'ya hizalı hücre içinden, varyanta göre kaydırma.
            // Hizalamayı bozmak iki karo hücresinin arasından kesip yapay dikiş
            // çıkarıyor (dual-grid sayfaları).
            int cellsX = (w / 16 > 0) ? w / 16 : 1;
            int cellsY = (h / 16 > 0) ? h / 16 : 1;
            int cx = (v * 7 + id * 3) % cellsX;
            int cy = (v * 5 + id * 2) % cellsY;
            int base = (w >= 16 && h >= 16) ? 16 : GF_FRAG;
            int maxOff = base - GF_FRAG; if (maxOff < 0) maxOff = 0;
            int ox = cx * base + ((v * 3 + id) % (maxOff + 1));
            int oy = cy * base + ((v * 2 + id) % (maxOff + 1));
            if (ox + GF_FRAG > w) ox = w - GF_FRAG;
            if (oy + GF_FRAG > h) oy = h - GF_FRAG;
            for (int j = 0; j < GF_FRAG; j++)
            for (int i = 0; i < GF_FRAG; i++) {
                const unsigned char *sp = &px[(((oy + j) * w) + (ox + i)) * 4];
                float m = gf_mask(v, i, j);
                Uint8 *o = &atlas[(((id * GF_FRAG + j) * AW)
                                   + v * GF_FRAG + i) * 4];
                o[0] = sp[0]; o[1] = sp[1]; o[2] = sp[2];
                o[3] = (Uint8)((float)sp[3] * m);
            }
        }
        stbi_image_free(px);
        s_fragHas[id] = true;
    }
    s_fragTex = r2d_texture_from_pixels(renderer, atlas, AW, AH);
    // TEŞHİS: sessiz başarısızlık en kötüsü. Kaç karo parça verdi, atlas kuruldu
    // mu — açılışta bir satır. Sıfır görürsen PNG yolları çalışma dizinine göre
    // bulunamıyor demektir.
    int nOk = 0;
    for (int i = 0; i < MAX_TILE_TYPES; i++) if (s_fragHas[i]) nOk++;
    SDL_Log("groundfx: parca atlasi %s (%d karo, %dx%d)",
            s_fragTex ? "kuruldu" : "KURULAMADI", nOk, AW, AH);
}
static int s_next = 0;
static unsigned s_rng = 0x9E3779B9u;

static inline float gf_rnd(void)
{
    s_rng = s_rng * 1664525u + 1013904223u;
    return (float)((s_rng >> 8) & 0xFFFFu) / 65535.0f;
}
static inline float gf_sym(void) { return gf_rnd() * 2.0f - 1.0f; }

void groundfx_bind_tiles(R2DTexture **tileTextures) { s_tiles = tileTextures; }

void groundfx_bake(R2D *renderer)
{
    if (s_fragTex == NULL) gf_bake_fragments(renderer);
}

void groundfx_init(void)
{
    for (int i = 0; i < GROUNDFX_MAX; i++) s_ch[i].active = false;
    s_next = 0;
}

int groundfx_active(void)
{
    int n = 0;
    for (int i = 0; i < GROUNDFX_MAX; i++) if (s_ch[i].active) n++;
    return n;
}

void groundfx_burst(float wx, float wy, int count, float power, float upBias)
{
    if (s_tiles == NULL) return;
    int id = tile_at_point(wx, wy);
    if (id < 0 || id >= MAX_TILE_TYPES) return;

    // SU karosunda parça YOK: su kırılmaz, sıçrar. Sıçrama su sisteminin işi
    // (tilemap yağmur halkaları / swash) — burada sessizce çıkıyoruz ki
    // gölette taş kırıntısı uçmasın.
    int col = (int)(wx / TILE_SIZE), row = (int)(wy / TILE_SIZE);
    if (tilemap_is_water(col, row)) return;

    // Parça ATLASINDAN çiziliyor (yumuşak kenar için maskelenmiş). Atlas
    // basılmamışsa ya da bu karonun dokusu yoksa parça yok.
    if (s_fragTex == NULL || !s_fragHas[id]) return;

    // Davranış sınıfı karo ALANLARINDAN (harita değişse de geçerli)
    const TileType *T = &TILE_TYPES[id];
    GroundClass cls = T->conceals ? GK_PLANT : (T->solid ? GK_ROCK : GK_GRAIN);

    // Kaynak HÜCRE: dual-grid sayfalarında doku 16×16'dan büyük olabiliyor.
    // 16'ya hizalı rastgele bir hücre seçiliyor ki kenar parçası yerine gövde
    // parçası gelsin; hizalamayı bozmak iki hücrenin arasından kesip yapay bir
    // dikiş çıkarıyor.

    for (int k = 0; k < count; k++) {
        // Havuz: en eskiyi ezmek yerine sıradan ilerle (halka). Dolu havuzda
        // parça DÜŞER — bütçe kuralı (bkz. HANDOVER havuz tavanları).
        int slot = -1;
        for (int p = 0; p < 24; p++) {           // sınırlı tarama (fx.c dersi)
            int i = (s_next + p) % GROUNDFX_MAX;
            if (!s_ch[i].active) { slot = i; break; }
        }
        if (slot < 0) { s_next = (s_next + 24) % GROUNDFX_MAX; break; }
        s_next = (slot + 1) % GROUNDFX_MAX;

        // ── ÖLÇÜ. Karo 64 dünya px = 16 doku px = 16 SANAT pikseli.
        // İki tur bu ölçü ısırdı:
        //  1) pencere 3-5 doku px (12-20 dünya px, kayada 30) → "zemin bloklara
        //     ayrılıyor" ve gövdeleri gömüyor.
        //  2) küçültüldü ama kenar hâlâ keskin kareydi → "kare kare gözüküyor".
        // Şimdi parça MASKELİ olduğu için biraz BÜYÜK olabilir: yumuşak kenarlı
        // 4-6 sanat pikseli (16-24 dünya px) "toprak kesekli" okunuyor. Küçük
        // parça maskeyi barındıramıyor (2 pikselde yuvarlaklık olmaz), büyük ama
        // maskesiz parça kutu oluyor — ikisi arasındaki nokta burası.
        int var = (int)(gf_rnd() * (float)GF_VAR) % GF_VAR;
        float ang = gf_rnd() * 6.28318f;
        // upBias: 1'e yakınken hız YUKARI ağırlıklı (yerden fışkırma)
        float dirX = SDL_cosf(ang) * (1.0f - upBias * 0.55f);
        float dirY = SDL_sinf(ang) * (1.0f - upBias * 0.80f) - upBias;

        float sp, life, sz, spin;
        switch (cls) {
        case GK_PLANT:   // çim/saman: hafif, yavaş, çok savrulur
            sp = 90.0f + gf_rnd() * 130.0f;
            life = 0.75f + gf_rnd() * 0.65f;
            sz = 16.0f + gf_rnd() * 7.0f;
            spin = gf_sym() * 5.0f;
            break;
        case GK_ROCK:    // taş: ağır, hızlı, az savrulur, büyük
            sp = 150.0f + gf_rnd() * 210.0f;
            life = 0.60f + gf_rnd() * 0.45f;
            sz = 18.0f + gf_rnd() * 8.0f;
            spin = gf_sym() * 9.0f;
            break;
        default:         // toprak/kum: orta, bol
            sp = 120.0f + gf_rnd() * 170.0f;
            life = 0.55f + gf_rnd() * 0.50f;
            sz = 16.0f + gf_rnd() * 7.0f;
            spin = gf_sym() * 7.0f;
            break;
        }
        sp *= power; sz *= (0.90f + 0.18f * power);
        if (sz > 27.0f) sz = 27.0f;

        s_ch[slot] = (Chunk){
            .active = true,
            .x = wx + gf_sym() * 6.0f, .y = wy + gf_sym() * 4.0f,
            .vx = dirX * sp, .vy = dirY * sp,
            .ang = gf_rnd() * 6.28318f, .spin = spin,
            .life = life, .maxLife = life,
            .size = sz, .tex = s_fragTex,
            .sx = (float)(var * GF_FRAG), .sy = (float)(id * GF_FRAG),
            .sw = (float)GF_FRAG, .cls = cls,
        };
    }
}

void groundfx_update(float dt)
{
    for (int i = 0; i < GROUNDFX_MAX; i++) {
        Chunk *c = &s_ch[i];
        if (!c->active) continue;
        c->life -= dt;
        if (c->life <= 0.0f) { c->active = false; continue; }

        // Yerçekimi ve sürüklenme sınıfa göre: bitki havada kalır, taş çakılır.
        float grav, drag;
        switch (c->cls) {
        case GK_PLANT: grav = 190.0f; drag = 2.6f; break;
        case GK_ROCK:  grav = 660.0f; drag = 0.5f; break;
        default:       grav = 430.0f; drag = 1.2f; break;
        }
        c->vx *= (1.0f - drag * dt);
        c->vy = c->vy * (1.0f - drag * 0.35f * dt) + grav * dt;
        c->x += c->vx * dt;
        c->y += c->vy * dt;
        // Dönme yavaşlıyor: havada takla atan parça yere yaklaşırken durulur
        c->ang += c->spin * dt;
        c->spin *= (1.0f - 1.1f * dt);
    }
}

void groundfx_render_band(R2D *renderer, const Camera *camera,
                          float yMin, float yMax)
{
    float z = camera->zoom;
    r2d_set_blend(renderer, R2D_BLEND_ALPHA);
    for (int i = 0; i < GROUNDFX_MAX; i++) {
        const Chunk *c = &s_ch[i];
        if (!c->active || c->tex == NULL) continue;
        if (c->y < yMin || c->y >= yMax) continue;      // derinlik bandı

        float t = c->life / c->maxLife;
        // Solma yalnız SONDA: parça ömrünün çoğunda tam opak durur (madde,
        // ışık değil). Baştan solan parça "hayalet" gibi okunuyor.
        Uint8 a = (Uint8)((t > 0.35f ? 1.0f : t / 0.35f) * 255.0f);
        float half = c->size * 0.5f;
        float sx = (c->x - camera->x) * z, sy = (c->y - camera->y) * z;
        float ux = SDL_cosf(c->ang), uy = SDL_sinf(c->ang);
        float px = -uy, py = ux;
        float h = half * z;
        SDL_FPoint q[4] = {
            { sx - ux * h + px * h, sy - uy * h + py * h },   // TL
            { sx + ux * h + px * h, sy + uy * h + py * h },   // TR
            { sx - ux * h - px * h, sy - uy * h - py * h },   // BL
            { sx + ux * h - px * h, sy + uy * h - py * h },   // BR
        };
        SDL_FRect src = { c->sx, c->sy, c->sw, c->sw };
        // HAFİF KOYULTMA: parça zeminin AYNI dokusundan geldiği için zemin
        // üstünde neredeyse görünmez kalıyor. %18 koyultma onu "havaya kalkmış,
        // kendi gölgesini taşıyan parça" yapıyor — renk kimliği bozulmuyor.
        r2d_draw_texture_quad(renderer, c->tex, &src, q,
                              (SDL_Color){ 210, 210, 210, a });
    }
    r2d_set_color(renderer, 255, 255, 255, 255);
}
