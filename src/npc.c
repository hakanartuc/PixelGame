#include <SDL3/SDL.h>
#include "npc.h"
#include "font.h"

// Cute_Fantasy NPC sheet düzeni: 64px hücreler, 6 sütun; satır 0 = idle-down.
// Chibi karakter LPC oyuncudan ufak → 1.5x çizilir.
#define NPC_FRAME 64
#define NPC_IDLE_ROW 0
#define NPC_IDLE_FRAMES 6
#define NPC_IDLE_SPEED_MS 180  // kare başına ms (6 karelik akıcı idle)
#define NPC_DRAW_SCALE 1.5f

// Yeşil Ova köyü — spawn'ın (300,300) hemen çevresi.
#define T(n) ((n) * (float)TILE_SIZE)
const NpcDef NPCS[] = {
    { .name = "Tuccar Baran", .x = T(10), .y = T(5),
      .sheetPath = "assets/characters/npcs/Bartender_Bruno.png",
      .dialogFile = "assets/dialogs/merchant.txt",
      .isVendor = true, .questId = NULL },
    { .name = "Yasli Kemal", .x = T(13), .y = T(7),
      .sheetPath = "assets/characters/npcs/Farmer_Bob.png",
      .dialogFile = "assets/dialogs/elder.txt",
      .isVendor = false, .questId = "goblin_derdi" },
    { .name = "Koylu Osman", .x = T(7), .y = T(11),
      .sheetPath = "assets/characters/npcs/Farmer_Buba.png",
      .dialogFile = "assets/dialogs/villager.txt",
      .isVendor = false, .questId = NULL },
};
const int NPC_COUNT = (int)(sizeof(NPCS) / sizeof(NPCS[0]));

int npc_nearest_in_range(float px, float py)
{
    int best = -1;
    float bestDist = NPC_INTERACT_RANGE;
    for (int i = 0; i < NPC_COUNT; i++) {
        float dx = NPCS[i].x - px;
        float dy = NPCS[i].y - py;
        float dist = SDL_sqrtf(dx * dx + dy * dy);
        if (dist < bestDist) {
            bestDist = dist;
            best = i;
        }
    }
    return best;
}

void npc_render_all(SDL_Renderer *renderer, SDL_Texture **npcTextures,
                    const Camera *camera, TTF_Font *font, int nearestIdx)
{
    float zoom = camera->zoom;
    // Tüm NPC'ler aynı ritimde soluk alır — NPC başına sayaç tutmaya değmez
    int frame = (int)(SDL_GetTicks() / NPC_IDLE_SPEED_MS) % NPC_IDLE_FRAMES;

    for (int i = 0; i < NPC_COUNT; i++) {
        if (npcTextures[i] == NULL) continue;

        SDL_FRect src = {
            .x = (float)(frame * NPC_FRAME),
            .y = (float)(NPC_IDLE_ROW * NPC_FRAME),
            .w = NPC_FRAME, .h = NPC_FRAME,
        };
        // x,y = ayak noktası → çizim kutusu ayağa hizalanır
        float drawSize = NPC_FRAME * NPC_DRAW_SCALE;
        SDL_FRect dst = {
            .x = (NPCS[i].x - drawSize / 2.0f - camera->x) * zoom,
            .y = (NPCS[i].y - drawSize + 8.0f - camera->y) * zoom,
            .w = drawSize * zoom, .h = drawSize * zoom,
        };
        SDL_RenderTexture(renderer, npcTextures[i], &src, &dst);

        if (font != NULL) {
            // İsim baş üstünde; menzildeyse E ipucu
            SDL_Color name = { 180, 255, 180, 255 };
            int tw = 0, th = 0;
            TTF_GetStringSize(font, NPCS[i].name, 0, &tw, &th);
            draw_text(renderer, font, NPCS[i].name,
                      dst.x + dst.w / 2.0f - tw / 2.0f, dst.y - 20.0f, name);

            if (i == nearestIdx) {
                SDL_Color hint = { 255, 230, 80, 255 };
                draw_text(renderer, font, "[E] Konus",
                          dst.x + dst.w / 2.0f - 40.0f, dst.y - 40.0f, hint);
            }
        }
    }
}
