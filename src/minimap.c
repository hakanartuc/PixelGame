#include <string.h>
#include "minimap.h"
#include "tilemap.h"
#include "npc.h"

#define REVEAL_RADIUS 12     // oyuncu çevresinde kaç tile açılır
#define MINIMAP_SIZE 170.0f  // ekrandaki kenar uzunluğu (uzun kenara göre ölçek)
#define MINIMAP_PAD 16.0f

// İki keşif tamponu: dünya KALICI (save'e girer), zindan GEÇİCİ (girişte sıfırlanır)
static Uint8 exploredWorld[MAP_H_MAX][MAP_W_MAX];
static Uint8 exploredTemp[MAP_H_MAX][MAP_W_MAX];
static Uint8 (*explored)[MAP_W_MAX] = exploredWorld;   // aktif tampon
static R2DTexture *mapTex = NULL;             // keşfedilen tile'ların rengi
static bool rebuildAll = false;                // load sonrası komple yeniden işle

// Tek tile'ı texture'a boya (çağıran render target'ı ayarlamış olmalı)
static void paint_tile(R2D *renderer, int col, int row)
{
    int id = tile_at_point(col * TILE_SIZE + 1.0f, row * TILE_SIZE + 1.0f);
    Uint8 r = 15, g = 15, b = 20;   // bilinmeyen id → koyu
    if (id >= 0 && id < TILE_TYPE_COUNT) {
        // Tile tablosundaki fallback rengi minimap rengi olarak kullan
        r = TILE_TYPES[id].r;
        g = TILE_TYPES[id].g;
        b = TILE_TYPES[id].b;
    }
    r2d_set_color(renderer, r, g, b, 255);
    r2d_point(renderer, (float)col, (float)row);
}

static int worldW = 0, worldH = 0;   // dünya boyutu (pack/unpack HEP bununla)

void minimap_init(R2D *renderer)
{
    memset(exploredWorld, 0, sizeof(exploredWorld));
    worldW = map_width();
    worldH = map_height();

    mapTex = r2d_target_create(renderer, map_width(), map_height());
    if (mapTex == NULL) {
        SDL_Log("Minimap texture olusturulamadi");
        return;
    }

    // Başlangıç: her yer sis (koyu) — boş pass sadece clear yapar
    r2d_target_begin(renderer, mapTex, true, 8, 8, 12);
    r2d_target_end(renderer);
}

// Harita değişimi: texture'ı yeni boyutta yeniden kur; dünya tamponu KORUNUR,
// zindan tamponu her girişte sıfırlanır (zindan keşfi kalıcı değil — bilinçli)
void minimap_set_map(R2D *renderer, bool worldPersistent)
{
    if (mapTex) { r2d_texture_destroy(renderer, mapTex); mapTex = NULL; }

    if (worldPersistent) {
        explored = exploredWorld;
    } else {
        memset(exploredTemp, 0, sizeof(exploredTemp));
        explored = exploredTemp;
    }

    mapTex = r2d_target_create(renderer, map_width(), map_height());
    if (mapTex == NULL) return;
    r2d_target_begin(renderer, mapTex, true, 8, 8, 12);
    r2d_target_end(renderer);
    rebuildAll = true;   // mevcut keşif (dünyaya dönüşte) yeniden işlensin
}

void minimap_quit(void)
{
    if (mapTex) r2d_texture_destroy(NULL, mapTex);
    mapTex = NULL;
}

void minimap_reveal(R2D *renderer, float worldX, float worldY)
{
    if (mapTex == NULL) return;

    int pc = (int)(worldX / TILE_SIZE);
    int pr = (int)(worldY / TILE_SIZE);
    bool targetSet = false;

    // Load sonrası: tüm keşfedilmişleri bir kez yeniden işle
    if (rebuildAll) {
        r2d_target_begin(renderer, mapTex, true, 8, 8, 12);
        for (int row = 0; row < map_height(); row++)
            for (int col = 0; col < map_width(); col++)
                if (explored[row][col])
                    paint_tile(renderer, col, row);
        r2d_target_end(renderer);
        rebuildAll = false;
    }

    for (int row = pr - REVEAL_RADIUS; row <= pr + REVEAL_RADIUS; row++) {
        for (int col = pc - REVEAL_RADIUS; col <= pc + REVEAL_RADIUS; col++) {
            if (col < 0 || col >= map_width() || row < 0 || row >= map_height())
                continue;
            if (explored[row][col])
                continue;   // zaten açık — texture'a tekrar dokunma

            // Dairesel keşif (köşeleri kırp — fener hissi)
            int dc = col - pc, dr = row - pr;
            if (dc * dc + dr * dr > REVEAL_RADIUS * REVEAL_RADIUS)
                continue;

            explored[row][col] = 1;
            if (!targetSet) {
                r2d_target_begin(renderer, mapTex, false, 0, 0, 0);   // clear YOK (artımlı)
                targetSet = true;
            }
            paint_tile(renderer, col, row);
        }
    }
    if (targetSet)
        r2d_target_end(renderer);
}

static float questMarkX = 0.0f, questMarkY = 0.0f;
static bool questMarkOn = false;
void minimap_set_quest(float wx, float wy, bool on)
{ questMarkX = wx; questMarkY = wy; questMarkOn = on; }

void minimap_render(R2D *renderer, float playerX, float playerY,
                    const Mob *mobs, int mobCount, int winW, int winH)
{
    if (mapTex == NULL) return;

    int mw = map_width(), mh = map_height();
    float scale = MINIMAP_SIZE / (float)((mw > mh) ? mw : mh);
    float w = mw * scale, h = mh * scale;
    float x0 = winW - w - MINIMAP_PAD;
    float y0 = winH - h - MINIMAP_PAD;

    SDL_FRect dst = { x0, y0, w, h };
    r2d_draw_texture(renderer, mapTex, NULL, &dst);
    r2d_set_color(renderer, 200, 200, 210, 255);
    r2d_rect(renderer, &dst);

    // GÖREV İŞARETÇİSİ (ESO sunumu): altın yanıp sönen kare — sis şartı yok
    // (hedef bilgisi görevden gelir, keşiften değil)
    if (questMarkOn) {
        int qc = (int)(questMarkX / TILE_SIZE), qr = (int)(questMarkY / TILE_SIZE);
        if (qc >= 0 && qc < mw && qr >= 0 && qr < mh) {
            Uint8 qa = (Uint8)(160 + 95 * ((SDL_GetTicks() / 400) % 2));
            r2d_set_color(renderer, 255, 210, 60, qa);
            SDL_FRect qdot = { x0 + qc * scale - 2, y0 + qr * scale - 2, 5, 5 };
            r2d_fill_rect(renderer, &qdot);
        }
    }

    // Noktalar — yalnız keşfedilmiş tile üzerindekiler (sisin altı sızmasın)
    for (int i = 0; i < mobCount; i++) {
        if (!mobs[i].e.alive) continue;
        int col = (int)(mobs[i].e.rect.x / TILE_SIZE);
        int row = (int)(mobs[i].e.rect.y / TILE_SIZE);
        if (col < 0 || col >= mw || row < 0 || row >= mh || !explored[row][col])
            continue;
        r2d_set_color(renderer, 230, 60, 60, 255);
        SDL_FRect dot = { x0 + col * scale - 1, y0 + row * scale - 1, 3, 3 };
        r2d_fill_rect(renderer, &dot);
    }

    for (int i = 0; i < NPC_COUNT; i++) {
        float nx, ny;
        if (!npc_visible_pos(i, &nx, &ny)) continue;   // içerideki görünmez
        int col = (int)(nx / TILE_SIZE), row = (int)(ny / TILE_SIZE);
        if (col < 0 || col >= mw || row < 0 || row >= mh || !explored[row][col])
            continue;
        r2d_set_color(renderer, 90, 230, 90, 255);
        SDL_FRect dot = { x0 + col * scale - 1, y0 + row * scale - 1, 3, 3 };
        r2d_fill_rect(renderer, &dot);
    }

    // Oyuncu en üstte (beyaz, biraz büyük)
    r2d_set_color(renderer, 255, 255, 255, 255);
    SDL_FRect pdot = { x0 + (playerX / TILE_SIZE) * scale - 2,
                       y0 + (playerY / TILE_SIZE) * scale - 2, 4, 4 };
    r2d_fill_rect(renderer, &pdot);
}

int minimap_pack_explored(Uint8 *out, int cap)
{
    int mw = worldW, mh = worldH;   // aktif harita zindan olsa bile dünya keşfi yazılır
    int bytes = (mw * mh + 7) / 8;
    if (bytes > cap) return 0;

    memset(out, 0, bytes);
    for (int row = 0; row < mh; row++)
        for (int col = 0; col < mw; col++)
            if (exploredWorld[row][col]) {
                int bit = row * mw + col;
                out[bit / 8] |= (Uint8)(1u << (bit % 8));
            }
    return bytes;
}

void minimap_reset_explored(void)
{
    memset(exploredWorld, 0, sizeof(exploredWorld));
    memset(exploredTemp, 0, sizeof(exploredTemp));
    rebuildAll = true;   // texture bir sonraki reveal'da yeniden işlenir
}

void minimap_unpack_explored(const Uint8 *in, int bytes)
{
    int mw = worldW, mh = worldH;
    memset(exploredWorld, 0, sizeof(exploredWorld));
    for (int row = 0; row < mh; row++)
        for (int col = 0; col < mw; col++) {
            int bit = row * mw + col;
            if (bit / 8 >= bytes) return;
            if (in[bit / 8] & (1u << (bit % 8)))
                exploredWorld[row][col] = 1;
        }
    rebuildAll = true;   // texture bir sonraki reveal'da yeniden işlenir
}
