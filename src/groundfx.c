#include <SDL3/SDL.h>
#include "groundfx.h"
#include "tilemap.h"

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
static int s_next = 0;
static unsigned s_rng = 0x9E3779B9u;

static inline float gf_rnd(void)
{
    s_rng = s_rng * 1664525u + 1013904223u;
    return (float)((s_rng >> 8) & 0xFFFFu) / 65535.0f;
}
static inline float gf_sym(void) { return gf_rnd() * 2.0f - 1.0f; }

void groundfx_bind_tiles(R2DTexture **tileTextures) { s_tiles = tileTextures; }

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

    R2DTexture *tex = s_tiles[id];
    if (tex == NULL) return;                 // dokusu olmayan karo (düz renk)

    float tw = 0.0f, th = 0.0f;
    r2d_texture_size(tex, &tw, &th);
    if (tw < 16.0f || th < 16.0f) return;

    // Davranış sınıfı karo ALANLARINDAN (harita değişse de geçerli)
    const TileType *T = &TILE_TYPES[id];
    GroundClass cls = T->conceals ? GK_PLANT : (T->solid ? GK_ROCK : GK_GRAIN);

    // Kaynak HÜCRE: dual-grid sayfalarında doku 16×16'dan büyük olabiliyor.
    // 16'ya hizalı rastgele bir hücre seçiliyor ki kenar parçası yerine gövde
    // parçası gelsin; hizalamayı bozmak iki hücrenin arasından kesip yapay bir
    // dikiş çıkarıyor.
    int cellsX = (int)(tw / 16.0f), cellsY = (int)(th / 16.0f);
    if (cellsX < 1) cellsX = 1;
    if (cellsY < 1) cellsY = 1;

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

        // Pencere boyu doku pikseli cinsinden: 3-5 px → 12-20 dünya px
        float win = 3.0f + gf_rnd() * 2.0f;
        int cx = (int)(gf_rnd() * (float)cellsX);
        int cy = (int)(gf_rnd() * (float)cellsY);
        float maxOff = 16.0f - win;
        float ox = gf_rnd() * maxOff, oy = gf_rnd() * maxOff;

        float ang = gf_rnd() * 6.28318f;
        // upBias: 1'e yakınken hız YUKARI ağırlıklı (yerden fışkırma)
        float dirX = SDL_cosf(ang) * (1.0f - upBias * 0.55f);
        float dirY = SDL_sinf(ang) * (1.0f - upBias * 0.80f) - upBias;

        float sp, life, sz, spin;
        switch (cls) {
        case GK_PLANT:   // çim/saman: hafif, yavaş, çok savrulur
            sp = 90.0f + gf_rnd() * 130.0f;
            life = 0.75f + gf_rnd() * 0.65f;
            sz = win * 4.0f * (0.70f + gf_rnd() * 0.40f);
            spin = gf_sym() * 5.0f;
            break;
        case GK_ROCK:    // taş: ağır, hızlı, az savrulur, büyük
            sp = 150.0f + gf_rnd() * 210.0f;
            life = 0.60f + gf_rnd() * 0.45f;
            sz = win * 4.0f * (0.95f + gf_rnd() * 0.55f);
            spin = gf_sym() * 9.0f;
            break;
        default:         // toprak/kum: orta, bol
            sp = 120.0f + gf_rnd() * 170.0f;
            life = 0.55f + gf_rnd() * 0.50f;
            sz = win * 4.0f * (0.75f + gf_rnd() * 0.45f);
            spin = gf_sym() * 7.0f;
            break;
        }
        sp *= power; sz *= (0.85f + 0.30f * power);

        s_ch[slot] = (Chunk){
            .active = true,
            .x = wx + gf_sym() * 6.0f, .y = wy + gf_sym() * 4.0f,
            .vx = dirX * sp, .vy = dirY * sp,
            .ang = gf_rnd() * 6.28318f, .spin = spin,
            .life = life, .maxLife = life,
            .size = sz, .tex = tex,
            .sx = (float)cx * 16.0f + ox, .sy = (float)cy * 16.0f + oy,
            .sw = win, .cls = cls,
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
        r2d_draw_texture_quad(renderer, c->tex, &src, q,
                              (SDL_Color){ 255, 255, 255, a });
    }
    r2d_set_color(renderer, 255, 255, 255, 255);
}
