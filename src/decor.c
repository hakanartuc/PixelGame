#include "decor.h"

// Köy evleri — NPC'lerin gece gittiği kapılar bunların önü (npc.c rutinleri)
#define T(n) ((n) * (float)TILE_SIZE)
const DecorDef DECORS[] = {
    // RİTÜEL TAŞLARI (YANKICI — gizli): Karanlık Orman derinliği, işaretsiz.
    // Uzaktan sıradan kaya kümesi; 3. sömürüden sonra fısıltı buraya çağırır.
    { .texPath = "assets/decor/ritual_stones.png",
      .x = T(108), .y = T(7), .scale = 2.0f, .solidBase = false },
    // KÖR KUYU (Kirli Mal & Aklama): köy dışında, yol kıyısında unutulmuş kuyu.
    // Gece kirli malı bırak → şafakta temiz kese (kim aldığını görmezsin).
    { .texPath = "assets/decor/well.png",
      .x = T(26), .y = T(15), .scale = 2.0f, .solidBase = true },
    { .texPath = "assets/buildings/House_1_Limestone_Base_Red.png",
      .x = T(10), .y = T(4),  .scale = 1.6f, .solidBase = true },   // Baran'ın evi (dükkanın arkası)
    { .texPath = "assets/buildings/House_2_Limestone_Base_Blue.png",
      .x = T(14), .y = T(6),  .scale = 1.6f, .solidBase = true },   // Kemal'in evi
    { .texPath = "assets/buildings/House_3_Limestone_Base_Black.png",
      .x = T(6),  .y = T(10), .scale = 1.6f, .solidBase = true },   // Osman'ın evi
    // HAN (K2 — köy GD köşesi): gerçek Inn binası (Unique_Buildings — ev
    // kopyası değil; 240px geniş sprite, 0.8 ölçek ≈ ev ayak izi)
    { .texPath = "assets/buildings/Inn_Red.png",
      .x = T(17), .y = T(10), .scale = 0.8f, .solidBase = true },

    // KÖY DETAYLARI (harita görsel yenileme): el yerleşimi — rutin yollarının
    // dışında, meydanı çerçeveleyen küçük yaşam izleri
    { .texPath = "assets/decor/barrels.png",
      .x = T(9.2f), .y = T(5.8f), .scale = 1.2f, .solidBase = true },   // Baran'ın dükkan yanı
    { .texPath = "assets/decor/Benches.png",
      .x = T(12.3f), .y = T(8.3f), .scale = 1.2f, .solidBase = false }, // meydan bankı
    { .texPath = "assets/decor/barrels.png",
      .x = T(18.6f), .y = T(11.4f), .scale = 1.1f, .solidBase = true }, // han önü fıçılar
    { .texPath = "assets/decor/Stone_Fence_Big.png",
      .x = T(7.5f), .y = T(8.6f), .scale = 1.2f, .solidBase = true },   // ocak arkası çit
    { .texPath = "assets/decor/Benches.png",
      .x = T(14.6f), .y = T(10.8f), .scale = 1.2f, .solidBase = false },// güney yolu bankı

    // Ağaçlar: köy çevresi + goblin kampına giden yol (siper/LoS oyunu için)
    { .texPath = "assets/decor/oak_big.png",    .x = T(18), .y = T(8),  .scale = 1.6f, .solidBase = true },
    { .texPath = "assets/decor/oak_medium.png", .x = T(19), .y = T(12), .scale = 1.6f, .solidBase = true },
    { .texPath = "assets/decor/oak_big.png",    .x = T(21), .y = T(15), .scale = 1.6f, .solidBase = true },
    { .texPath = "assets/decor/oak_medium.png", .x = T(16), .y = T(17), .scale = 1.6f, .solidBase = true },
    { .texPath = "assets/decor/oak_medium.png", .x = T(15), .y = T(3),  .scale = 1.6f, .solidBase = true },
    { .texPath = "assets/decor/oak_big.png",    .x = T(27), .y = T(19), .scale = 1.6f, .solidBase = true },

    // Çalılar: goblin kampına sneak yaklaşma hattı (içine eğilerek girilir)
    { .texPath = "assets/decor/bush_1.png", .x = T(22), .y = T(23), .scale = 4.0f, .conceals = true },
    { .texPath = "assets/decor/bush_2.png", .x = T(24), .y = T(21), .scale = 4.0f, .conceals = true },
    { .texPath = "assets/decor/bush_1.png", .x = T(21), .y = T(26), .scale = 4.0f, .conceals = true },
    { .texPath = "assets/decor/bush_2.png", .x = T(27), .y = T(23), .scale = 4.0f, .conceals = true },
    { .texPath = "assets/decor/bush_1.png", .x = T(23), .y = T(28), .scale = 4.0f, .conceals = true },

    // Saman balyaları: Osman'ın tarlası (çiftlik gizlenmesi)
    { .texPath = "assets/decor/hay_bale.png", .x = T(6), .y = T(12),   .scale = 3.0f, .conceals = true },
    { .texPath = "assets/decor/hay_bale.png", .x = T(8), .y = T(12.3f), .scale = 3.0f, .conceals = true },
};
const int DECOR_COUNT = (int)(sizeof(DECORS) / sizeof(DECORS[0]));

// Texture boyutları (yüklemede bir kez okunur — çizim/collision tek kaynaktan)
#define DECOR_CAP 32
static float texW[DECOR_CAP], texH[DECOR_CAP];

static bool decorActive = true;
void decor_set_active(bool active) { decorActive = active; }

void decor_set_texture_sizes(R2DTexture **decorTextures)
{
    for (int i = 0; i < DECOR_COUNT && i < DECOR_CAP; i++) {
        texW[i] = texH[i] = 0.0f;
        if (decorTextures[i] != NULL)
            r2d_texture_size(decorTextures[i], &texW[i], &texH[i]);
    }
}

// Dekorun dünya-uzayı çizim kutusu (taban orta noktasından)
static SDL_FRect decor_rect(int i)
{
    float w = texW[i] * DECORS[i].scale;
    float h = texH[i] * DECORS[i].scale;
    SDL_FRect r = { DECORS[i].x - w / 2.0f, DECORS[i].y - h, w, h };
    return r;
}

void decor_render_all(R2D *renderer, R2DTexture **decorTextures,
                      const Camera *camera)
{
    if (!decorActive) return;
    float zoom = camera->zoom;
    for (int i = 0; i < DECOR_COUNT; i++) {
        if (decorTextures[i] == NULL) continue;
        SDL_FRect r = decor_rect(i);
        SDL_FRect dst = {
            (r.x - camera->x) * zoom, (r.y - camera->y) * zoom,
            r.w * zoom, r.h * zoom,
        };
        r2d_draw_texture(renderer, decorTextures[i], NULL, &dst);
    }
}

bool decor_blocks(SDL_FRect box)
{
    if (!decorActive) return false;
    for (int i = 0; i < DECOR_COUNT; i++) {
        if (texW[i] <= 0.0f || !DECORS[i].solidBase) continue;   // çalı/saman engellemez
        SDL_FRect r = decor_rect(i);
        // Katı taban: alt %35 (üst kısım çatı/yaprak — arkasından geçilebilir)
        SDL_FRect base = { r.x, r.y + r.h * 0.65f, r.w, r.h * 0.35f };
        if (box.x < base.x + base.w && box.x + box.w > base.x &&
            box.y < base.y + base.h && box.y + box.h > base.y)
            return true;
    }
    return false;
}

bool decor_conceals(float x, float y)
{
    for (int i = 0; i < DECOR_COUNT; i++) {
        if (texW[i] <= 0.0f || !DECORS[i].conceals) continue;
        SDL_FRect r = decor_rect(i);   // çalı küçük — tüm sprite alanı örtüdür
        if (x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h)
            return true;
    }
    return false;
}
