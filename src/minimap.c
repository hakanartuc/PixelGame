#include <string.h>
#include "minimap.h"
#include "tilemap.h"
#include "npc.h"

#define REVEAL_RADIUS 12     // oyuncu çevresinde kaç tile açılır
#define MINIMAP_SIZE 170.0f  // ekrandaki kenar uzunluğu (uzun kenara göre ölçek)
#define MINIMAP_PAD 16.0f

static Uint8 explored[MAP_H_MAX][MAP_W_MAX];   // 0 = sis, 1 = keşfedildi
static SDL_Texture *mapTex = NULL;             // keşfedilen tile'ların rengi
static bool rebuildAll = false;                // load sonrası komple yeniden işle

// Tek tile'ı texture'a boya (çağıran render target'ı ayarlamış olmalı)
static void paint_tile(SDL_Renderer *renderer, int col, int row)
{
    int id = tile_at_point(col * TILE_SIZE + 1.0f, row * TILE_SIZE + 1.0f);
    Uint8 r = 15, g = 15, b = 20;   // bilinmeyen id → koyu
    if (id >= 0 && id < TILE_TYPE_COUNT) {
        // Tile tablosundaki fallback rengi minimap rengi olarak kullan
        r = TILE_TYPES[id].r;
        g = TILE_TYPES[id].g;
        b = TILE_TYPES[id].b;
    }
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderPoint(renderer, (float)col, (float)row);
}

void minimap_init(SDL_Renderer *renderer)
{
    memset(explored, 0, sizeof(explored));

    mapTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                               SDL_TEXTUREACCESS_TARGET, map_width(), map_height());
    if (mapTex == NULL) {
        SDL_Log("Minimap texture olusturulamadi: %s", SDL_GetError());
        return;
    }
    SDL_SetTextureScaleMode(mapTex, SDL_SCALEMODE_NEAREST);   // pixel-art keskin

    // Başlangıç: her yer sis (koyu)
    SDL_SetRenderTarget(renderer, mapTex);
    SDL_SetRenderDrawColor(renderer, 8, 8, 12, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderTarget(renderer, NULL);
}

void minimap_quit(void)
{
    if (mapTex) SDL_DestroyTexture(mapTex);
    mapTex = NULL;
}

void minimap_reveal(SDL_Renderer *renderer, float worldX, float worldY)
{
    if (mapTex == NULL) return;

    int pc = (int)(worldX / TILE_SIZE);
    int pr = (int)(worldY / TILE_SIZE);
    bool targetSet = false;

    // Load sonrası: tüm keşfedilmişleri bir kez yeniden işle
    if (rebuildAll) {
        SDL_SetRenderTarget(renderer, mapTex);
        SDL_SetRenderDrawColor(renderer, 8, 8, 12, 255);
        SDL_RenderClear(renderer);
        for (int row = 0; row < map_height(); row++)
            for (int col = 0; col < map_width(); col++)
                if (explored[row][col])
                    paint_tile(renderer, col, row);
        SDL_SetRenderTarget(renderer, NULL);
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
                SDL_SetRenderTarget(renderer, mapTex);   // yalnız gerekince değiştir
                targetSet = true;
            }
            paint_tile(renderer, col, row);
        }
    }
    if (targetSet)
        SDL_SetRenderTarget(renderer, NULL);
}

void minimap_render(SDL_Renderer *renderer, float playerX, float playerY,
                    const Mob *mobs, int mobCount, int winW, int winH)
{
    if (mapTex == NULL) return;

    int mw = map_width(), mh = map_height();
    float scale = MINIMAP_SIZE / (float)((mw > mh) ? mw : mh);
    float w = mw * scale, h = mh * scale;
    float x0 = winW - w - MINIMAP_PAD;
    float y0 = winH - h - MINIMAP_PAD;

    SDL_FRect dst = { x0, y0, w, h };
    SDL_RenderTexture(renderer, mapTex, NULL, &dst);
    SDL_SetRenderDrawColor(renderer, 200, 200, 210, 255);
    SDL_RenderRect(renderer, &dst);

    // Noktalar — yalnız keşfedilmiş tile üzerindekiler (sisin altı sızmasın)
    for (int i = 0; i < mobCount; i++) {
        if (!mobs[i].e.alive) continue;
        int col = (int)(mobs[i].e.rect.x / TILE_SIZE);
        int row = (int)(mobs[i].e.rect.y / TILE_SIZE);
        if (col < 0 || col >= mw || row < 0 || row >= mh || !explored[row][col])
            continue;
        SDL_SetRenderDrawColor(renderer, 230, 60, 60, 255);
        SDL_FRect dot = { x0 + col * scale - 1, y0 + row * scale - 1, 3, 3 };
        SDL_RenderFillRect(renderer, &dot);
    }

    for (int i = 0; i < NPC_COUNT; i++) {
        float nx, ny;
        if (!npc_visible_pos(i, &nx, &ny)) continue;   // içerideki görünmez
        int col = (int)(nx / TILE_SIZE), row = (int)(ny / TILE_SIZE);
        if (col < 0 || col >= mw || row < 0 || row >= mh || !explored[row][col])
            continue;
        SDL_SetRenderDrawColor(renderer, 90, 230, 90, 255);
        SDL_FRect dot = { x0 + col * scale - 1, y0 + row * scale - 1, 3, 3 };
        SDL_RenderFillRect(renderer, &dot);
    }

    // Oyuncu en üstte (beyaz, biraz büyük)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_FRect pdot = { x0 + (playerX / TILE_SIZE) * scale - 2,
                       y0 + (playerY / TILE_SIZE) * scale - 2, 4, 4 };
    SDL_RenderFillRect(renderer, &pdot);
}

int minimap_pack_explored(Uint8 *out, int cap)
{
    int mw = map_width(), mh = map_height();
    int bytes = (mw * mh + 7) / 8;
    if (bytes > cap) return 0;

    memset(out, 0, bytes);
    for (int row = 0; row < mh; row++)
        for (int col = 0; col < mw; col++)
            if (explored[row][col]) {
                int bit = row * mw + col;
                out[bit / 8] |= (Uint8)(1u << (bit % 8));
            }
    return bytes;
}

void minimap_unpack_explored(const Uint8 *in, int bytes)
{
    int mw = map_width(), mh = map_height();
    memset(explored, 0, sizeof(explored));
    for (int row = 0; row < mh; row++)
        for (int col = 0; col < mw; col++) {
            int bit = row * mw + col;
            if (bit / 8 >= bytes) return;
            if (in[bit / 8] & (1u << (bit % 8)))
                explored[row][col] = 1;
        }
    rebuildAll = true;   // texture bir sonraki reveal'da yeniden işlenir
}
