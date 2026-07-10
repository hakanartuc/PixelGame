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
#define NPC_WALK_SPEED 90.0f   // rutin yürüyüş hızı (px/sn)

// Yeşil Ova köyü. Rutinler: gündüz iş yerinde, akşam ev kapısına yürü + içeri.
// Ev kapıları decor.c'deki evlerin önü.
#define T(n) ((n) * (float)TILE_SIZE)
const NpcDef NPCS[] = {
    { .name = "Tuccar Baran",
      .sheetPath = "assets/characters/npcs/Bartender_Bruno.png",
      .dialogFile = "assets/dialogs/merchant.txt",
      .isVendor = true, .questId = NULL,
      .schedule = {
          { .hour =  8.0f, .x = T(10),   .y = T(5),   .indoors = false },  // dükkan başı
          { .hour = 20.0f, .x = T(10),   .y = T(4.2f), .indoors = true },  // eve girer
      }, .stopCount = 2 },
    { .name = "Yasli Kemal",
      .sheetPath = "assets/characters/npcs/Farmer_Bob.png",
      .dialogFile = "assets/dialogs/elder.txt",
      .isVendor = false, .questId = "goblin_derdi",
      .schedule = {
          { .hour =  7.0f, .x = T(13),   .y = T(7),    .indoors = false }, // meydan
          { .hour = 21.0f, .x = T(14),   .y = T(6.2f), .indoors = true },
      }, .stopCount = 2 },
    { .name = "Koylu Osman",
      .sheetPath = "assets/characters/npcs/Farmer_Buba.png",
      .dialogFile = "assets/dialogs/villager.txt",
      .isVendor = false, .questId = NULL,
      .schedule = {
          { .hour =  9.0f, .x = T(7),    .y = T(11),   .indoors = false }, // tarla
          { .hour = 13.0f, .x = T(11),   .y = T(8),    .indoors = false }, // meydanda mola
          { .hour = 15.0f, .x = T(7),    .y = T(11),   .indoors = false }, // tarlaya dönüş
          { .hour = 19.0f, .x = T(6),    .y = T(10.2f), .indoors = true },
      }, .stopCount = 4 },
};
const int NPC_COUNT = (int)(sizeof(NPCS) / sizeof(NPCS[0]));

// Çalışma durumu: konum + görünürlük (tanımlar sabit, hareket burada yaşar)
typedef struct {
    float x, y;
    bool hidden;      // içeride (çizilmez, konuşulamaz)
    bool walking;     // isim etiketi/anim için (şimdilik idle anim kayarak yürür)
    bool initialized;
    float health;     // suç sistemi: NPC'ler ölebilir
    bool alive;       // ölüm KALICI (save'e yazılır); ölü = hidden gibi davranır
    float hitFlash;   // vuruş kızarması (sn, geri sayar)
} NpcState;
static NpcState npcState[8];   // NPC_COUNT tavanı (dizi statik kalsın diye bolca)

void npc_reset(void)
{
    for (int i = 0; i < 8; i++) {
        npcState[i].alive = true;
        npcState[i].health = NPC_MAX_HEALTH;
        npcState[i].hitFlash = 0.0f;
        npcState[i].initialized = false;   // konumlar rutinden yeniden kurulur
    }
}

// Saate göre geçerli durak: saati geçmiş SON durak; gün sarmalında sonuncusu
static const NpcStop *current_stop(const NpcDef *def, float hours)
{
    const NpcStop *best = &def->schedule[def->stopCount - 1];   // gece yarısı öncesi sarkma
    for (int s = 0; s < def->stopCount; s++) {
        if (hours >= def->schedule[s].hour)
            best = &def->schedule[s];
    }
    return best;
}

void npc_update(float dt, float hours)
{
    for (int i = 0; i < NPC_COUNT && i < 8; i++) {
        NpcState *st = &npcState[i];
        if (!st->alive) continue;   // ölü NPC rutin yürümez
        if (st->hitFlash > 0.0f) st->hitFlash -= dt;
        if (!st->initialized) {
            const NpcStop *s0 = current_stop(&NPCS[i], hours);
            st->x = s0->x;
            st->y = s0->y;
            st->hidden = s0->indoors;
            st->initialized = true;
        }

        const NpcStop *target = current_stop(&NPCS[i], hours);
        float dx = target->x - st->x;
        float dy = target->y - st->y;
        float dist = SDL_sqrtf(dx * dx + dy * dy);

        if (dist > 4.0f) {
            // Hedefe yürü — yürüyene kadar dışarıda görünür
            st->hidden = false;
            st->walking = true;
            float step = NPC_WALK_SPEED * dt;
            if (step > dist) step = dist;
            st->x += dx / dist * step;
            st->y += dy / dist * step;
        } else {
            st->walking = false;
            st->hidden = target->indoors;   // kapıya vardıysa içeri girer
        }
    }
}

bool npc_visible_pos(int i, float *x, float *y)
{
    if (i < 0 || i >= NPC_COUNT || i >= 8 ||
        npcState[i].hidden || !npcState[i].alive)
        return false;
    *x = npcState[i].x;
    *y = npcState[i].y;
    return true;
}

int npc_at_point(float wx, float wy)
{
    float drawSize = NPC_FRAME * NPC_DRAW_SCALE;
    for (int i = 0; i < NPC_COUNT && i < 8; i++) {
        float nx, ny;
        if (!npc_visible_pos(i, &nx, &ny)) continue;
        // Çizim kutusu (render ile aynı hiza: x,y = ayak noktası)
        if (wx >= nx - drawSize / 2.0f && wx <= nx + drawSize / 2.0f &&
            wy >= ny - drawSize + 8.0f && wy <= ny + 8.0f)
            return i;
    }
    return -1;
}

bool npc_damage(int i, float dmg)
{
    if (i < 0 || i >= NPC_COUNT || i >= 8 || !npcState[i].alive)
        return false;
    npcState[i].health -= dmg;
    npcState[i].hitFlash = 0.15f;
    if (npcState[i].health <= 0.0f) {
        npcState[i].health = 0.0f;
        npcState[i].alive = false;   // kalıcı — npc_reset/save dışında geri gelmez
        return true;
    }
    return false;
}

bool npc_is_alive(int i)
{
    return i >= 0 && i < NPC_COUNT && i < 8 && npcState[i].alive;
}

float npc_health(int i)
{
    if (i < 0 || i >= NPC_COUNT || i >= 8) return 0.0f;
    return npcState[i].health;
}

void npc_set_alive(int i, bool alive)
{
    if (i < 0 || i >= NPC_COUNT || i >= 8) return;
    npcState[i].alive = alive;
    npcState[i].health = alive ? NPC_MAX_HEALTH : 0.0f;
}

int npc_nearest_in_range(float px, float py)
{
    int best = -1;
    float bestDist = NPC_INTERACT_RANGE;
    for (int i = 0; i < NPC_COUNT && i < 8; i++) {
        if (npcState[i].hidden || !npcState[i].alive) continue;   // içeride/ölü konuşulamaz
        float dx = npcState[i].x - px;
        float dy = npcState[i].y - py;
        float dist = SDL_sqrtf(dx * dx + dy * dy);
        if (dist < bestDist) {
            bestDist = dist;
            best = i;
        }
    }
    return best;
}

void npc_render_all(R2D *renderer, R2DTexture **npcTextures,
                    const Camera *camera, TTF_Font *font, int nearestIdx)
{
    float zoom = camera->zoom;
    // Tüm NPC'ler aynı ritimde soluk alır — NPC başına sayaç tutmaya değmez
    int frame = (int)(SDL_GetTicks() / NPC_IDLE_SPEED_MS) % NPC_IDLE_FRAMES;

    for (int i = 0; i < NPC_COUNT && i < 8; i++) {
        if (npcTextures[i] == NULL || npcState[i].hidden || !npcState[i].alive)
            continue;

        SDL_FRect src = {
            .x = (float)(frame * NPC_FRAME),
            .y = (float)(NPC_IDLE_ROW * NPC_FRAME),
            .w = NPC_FRAME, .h = NPC_FRAME,
        };
        // x,y = ayak noktası → çizim kutusu ayağa hizalanır
        float drawSize = NPC_FRAME * NPC_DRAW_SCALE;
        SDL_FRect dst = {
            .x = (npcState[i].x - drawSize / 2.0f - camera->x) * zoom,
            .y = (npcState[i].y - drawSize + 8.0f - camera->y) * zoom,
            .w = drawSize * zoom, .h = drawSize * zoom,
        };
        if (npcState[i].hitFlash > 0.0f)
            r2d_texture_set_color_mod(npcTextures[i], 255, 90, 90);   // vuruş kızarması
        r2d_draw_texture(renderer, npcTextures[i], &src, &dst);
        if (npcState[i].hitFlash > 0.0f)
            r2d_texture_set_color_mod(npcTextures[i], 255, 255, 255);

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
