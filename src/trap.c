#include "trap.h"
#include "render.h"

// Diken döngüsü: gizli → UYARI (uçlar belirir) → dışarıda → geri çekilir
#define SPIKE_HIDDEN 1.2f
#define SPIKE_WARN   0.45f
#define SPIKE_OUT    0.8f
#define SPIKE_CYCLE  (SPIKE_HIDDEN + SPIKE_WARN + SPIKE_OUT)
#define SPIKE_HIT_CD 0.6f      // aynı dikenin art arda vuruş beklemesi
#define SPIKE_SIZE   56.0f     // dünya px (32px kare ~1.75x)

#define MAX_TRAP_STATE 16      // SPIKES/BOLT_TRAPS tavanı (tablolar küçük)

static R2DTexture *spikeTex = NULL;     // 4 kare 32x32 (gizli→tam dışarı)
static R2DTexture *plateTex = NULL;     // animasyon sheet'i — kare 0 (salık) / son (basılı)
static R2DTexture *bowTex = NULL;       // 5 kare 16x16 kurulu arbalet

static float spikeHitCd[MAX_TRAP_STATE];   // diken başına vuruş beklemesi
static bool plateDown[MAX_TRAP_STATE];     // plaka basılı mı (kalkınca yeniden kurulur)
static float trapClock = 0.0f;             // diken fazları için ortak saat

bool trap_init(R2D *renderer)
{
    spikeTex = load_texture(renderer, "assets/decor/erw/floor_spikes.png");
    plateTex = load_texture(renderer, "assets/decor/Dungeon_1_Pressure_Plate_Anim.png");
    bowTex   = load_texture(renderer, "assets/decor/Mounted_Crossbow_anim.png");
    return spikeTex != NULL;
}

void trap_quit(void)
{
    if (spikeTex) { r2d_texture_destroy(NULL, spikeTex); spikeTex = NULL; }
    if (plateTex) { r2d_texture_destroy(NULL, plateTex); plateTex = NULL; }
    if (bowTex)   { r2d_texture_destroy(NULL, bowTex);   bowTex = NULL; }
}

void trap_reset(void)
{
    for (int i = 0; i < MAX_TRAP_STATE; i++) {
        spikeHitCd[i] = 0.0f;
        plateDown[i] = false;
    }
}

// Dikenin döngü içindeki evresi: 0 gizli, 1 uyarı, 2 dışarıda
static int spike_phase(const SpikeDef *s)
{
    float t = SDL_fmodf(trapClock + s->phaseOffset * SPIKE_CYCLE, SPIKE_CYCLE);
    if (t < SPIKE_HIDDEN) return 0;
    if (t < SPIKE_HIDDEN + SPIKE_WARN) return 1;
    return 2;
}

float trap_update(float dt, MapId map, SDL_FRect playerFeet,
                  TrapShot *shots, int maxShots, int *outShotCount)
{
    trapClock += dt;
    *outShotCount = 0;
    float damage = 0.0f;

    // Dikenler: dışarıdayken ayak kutusuyla kesişim = hasar (tuzak başına CD)
    for (int i = 0; i < SPIKE_COUNT && i < MAX_TRAP_STATE; i++) {
        if (spikeHitCd[i] > 0.0f) spikeHitCd[i] -= dt;
        if (SPIKES[i].map != map) continue;
        if (spike_phase(&SPIKES[i]) != 2) continue;

        SDL_FRect box = { SPIKES[i].x - SPIKE_SIZE / 2.0f,
                          SPIKES[i].y - SPIKE_SIZE / 2.0f,
                          SPIKE_SIZE, SPIKE_SIZE };
        bool hit = playerFeet.x < box.x + box.w && playerFeet.x + playerFeet.w > box.x &&
                   playerFeet.y < box.y + box.h && playerFeet.y + playerFeet.h > box.y;
        if (hit && spikeHitCd[i] <= 0.0f) {
            damage += SPIKE_DAMAGE;
            spikeHitCd[i] = SPIKE_HIT_CD;
        }
    }

    // Plakalar: basınca arbalet atar; plakadan inince yeniden kurulur
    for (int i = 0; i < BOLT_TRAP_COUNT && i < MAX_TRAP_STATE; i++) {
        if (BOLT_TRAPS[i].map != map) continue;

        SDL_FRect box = { BOLT_TRAPS[i].plateX - 24.0f, BOLT_TRAPS[i].plateY - 24.0f,
                          48.0f, 48.0f };
        bool on = playerFeet.x < box.x + box.w && playerFeet.x + playerFeet.w > box.x &&
                  playerFeet.y < box.y + box.h && playerFeet.y + playerFeet.h > box.y;

        if (on && !plateDown[i]) {
            plateDown[i] = true;
            if (*outShotCount < maxShots) {
                shots[*outShotCount] = (TrapShot){
                    .x = BOLT_TRAPS[i].bowX, .y = BOLT_TRAPS[i].bowY,
                    .vx = BOLT_TRAPS[i].dirX * BOLT_SPEED,
                    .vy = BOLT_TRAPS[i].dirY * BOLT_SPEED,
                    .dmg = BOLT_DAMAGE,
                };
                (*outShotCount)++;
            }
        }
        else if (!on && plateDown[i]) {
            plateDown[i] = false;   // yeniden kuruldu
        }
    }

    return damage;
}

void trap_render(R2D *renderer, MapId map, const Camera *cam)
{
    float zoom = cam->zoom;

    for (int i = 0; i < SPIKE_COUNT; i++) {
        if (SPIKES[i].map != map || spikeTex == NULL) continue;
        // Kare: 0 gizli (delikler), 1 uyarı (uçlar), 3 tam dışarı
        int phase = spike_phase(&SPIKES[i]);
        int frame = (phase == 0) ? 0 : (phase == 1) ? 1 : 3;
        SDL_FRect src = { frame * 32.0f, 0, 32, 32 };
        SDL_FRect dst = {
            (SPIKES[i].x - SPIKE_SIZE / 2.0f - cam->x) * zoom,
            (SPIKES[i].y - SPIKE_SIZE / 2.0f - cam->y) * zoom,
            SPIKE_SIZE * zoom, SPIKE_SIZE * zoom,
        };
        r2d_draw_texture(renderer, spikeTex, &src, &dst);
    }

    for (int i = 0; i < BOLT_TRAP_COUNT; i++) {
        if (BOLT_TRAPS[i].map != map) continue;

        if (plateTex != NULL) {
            // Sheet 16px kareler: 0 = salık, basılıysa ileri bir kare
            float fx = (i < MAX_TRAP_STATE && plateDown[i]) ? 16.0f : 0.0f;
            SDL_FRect src = { fx, 0, 16, 16 };
            SDL_FRect dst = {
                (BOLT_TRAPS[i].plateX - 24.0f - cam->x) * zoom,
                (BOLT_TRAPS[i].plateY - 24.0f - cam->y) * zoom,
                48.0f * zoom, 48.0f * zoom,
            };
            r2d_draw_texture(renderer, plateTex, &src, &dst);
        }
        if (bowTex != NULL) {
            SDL_FRect src = { 0, 0, 16, 16 };   // kurulu kare
            SDL_FRect dst = {
                (BOLT_TRAPS[i].bowX - 24.0f - cam->x) * zoom,
                (BOLT_TRAPS[i].bowY - 24.0f - cam->y) * zoom,
                48.0f * zoom, 48.0f * zoom,
            };
            r2d_draw_texture(renderer, bowTex, &src, &dst);
        }
    }
}
