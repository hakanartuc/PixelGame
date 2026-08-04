#include "decor.h"
#include "tilemap.h"   // tilemap_is_water (yansıma kırpması)
#include "shadow.h"   // temas lekesi (ayak izi)

// Köy evleri — NPC'lerin gece gittiği kapılar bunların önü (npc.c rutinleri)
#define T(n) ((n) * (float)TILE_SIZE)
#define DM_MEZARLIK 4   // MAP_MEZARLIK (worldmap.h enum sırası — include döngüsünden kaçınmak için literal)
#define DM_LOCA 5       // MAP_LOCA (Sessiz El kontrat tahtası dekoru)
#define DM_DOGU 6       // MAP_DOGU (Taşdurak — Faz DOGU)

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
    // REVİR (Faz REVIR): köy KB köşesi — 3 yatak, Şifacı Ela başında
    { .texPath = "assets/decor/bed_single.png",
      .x = T(3.6f), .y = T(6.2f), .scale = 1.6f, .solidBase = false },
    { .texPath = "assets/decor/bed_single.png",
      .x = T(4.5f), .y = T(6.2f), .scale = 1.6f, .solidBase = false },
    { .texPath = "assets/decor/bed_single.png",
      .x = T(5.4f), .y = T(6.2f), .scale = 1.6f, .solidBase = false },
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
    // GÖL KUZEY KIYISI AĞAÇLARI (Faz SU 2 — Hakan: "göl kıyısında ağaç yok,
    // ekler misin"): yansıma ayaktan GÜNEYE uzandığı için ağaç suyun KUZEY
    // kıyısında durmalı ki aynası suya düşsün. Ayaklar kıyı çimeninin alt
    // kenarına yakın — yansıma su çizgisinin hemen ardından başlar.
    { .texPath = "assets/decor/oak_big.png",    .x = T(54.5f), .y = T(22.9f), .scale = 1.6f, .solidBase = true },
    { .texPath = "assets/decor/oak_medium.png", .x = T(59.3f), .y = T(22.85f), .scale = 1.6f, .solidBase = true },
    { .texPath = "assets/decor/oak_big.png",    .x = T(63.8f), .y = T(22.9f), .scale = 1.6f, .solidBase = true },
    { .texPath = "assets/decor/bush_2.png",     .x = T(66.4f), .y = T(23.8f), .scale = 4.0f, .conceals = true },
    // Gölet kuzeyine tek ağaç: yansıması sığlık rafına + koyu ortaya düşer
    { .texPath = "assets/decor/oak_medium.png", .x = T(7.5f),  .y = T(18.9f), .scale = 1.6f, .solidBase = true },
    { .texPath = "assets/decor/bush_1.png", .x = T(22), .y = T(23), .scale = 4.0f, .conceals = true },
    { .texPath = "assets/decor/bush_2.png", .x = T(24), .y = T(21), .scale = 4.0f, .conceals = true },
    { .texPath = "assets/decor/bush_1.png", .x = T(21), .y = T(26), .scale = 4.0f, .conceals = true },
    { .texPath = "assets/decor/bush_2.png", .x = T(27), .y = T(23), .scale = 4.0f, .conceals = true },
    { .texPath = "assets/decor/bush_1.png", .x = T(23), .y = T(28), .scale = 4.0f, .conceals = true },

    // Saman balyaları: Osman'ın tarlası (çiftlik gizlenmesi)
    { .texPath = "assets/decor/hay_bale.png", .x = T(6), .y = T(12),   .scale = 3.0f, .conceals = true },
    { .texPath = "assets/decor/hay_bale.png", .x = T(8), .y = T(12.3f), .scale = 3.0f, .conceals = true },
    // --- LANETLİ MEZARLIK dekorları (map = MEZARLIK) ---
    { .texPath = "assets/decor/undead/lich.png",          // DEV İSKELET KALINTISI
      .x = T(30), .y = T(15), .scale = 2.2f, .solidBase = true, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/undead/skulls.png",
      .x = T(27), .y = T(17), .scale = 1.4f, .solidBase = false, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/undead/dead_tree1.png",
      .x = T(10), .y = T(16), .scale = 1.6f, .solidBase = true, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/undead/dead_tree2.png",
      .x = T(50), .y = T(18), .scale = 1.6f, .solidBase = true, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/undead/dead_tree1.png",
      .x = T(38), .y = T(36), .scale = 1.5f, .solidBase = true, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/undead/ruin1.png",
      .x = T(12), .y = T(9), .scale = 1.8f, .solidBase = true, .map = DM_MEZARLIK },   // Sağlam sandık harabesi
    { .texPath = "assets/decor/undead/grave1.png",
      .x = T(17), .y = T(29), .scale = 1.6f, .solidBase = true, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/undead/grave2.png",
      .x = T(20), .y = T(31), .scale = 1.6f, .solidBase = true, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/undead/grave1.png",
      .x = T(43), .y = T(29), .scale = 1.6f, .solidBase = true, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/undead/grave2.png",
      .x = T(41), .y = T(32), .scale = 1.6f, .solidBase = true, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/undead/bones1.png",
      .x = T(24), .y = T(24), .scale = 1.6f, .solidBase = false, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/undead/bones2.png",
      .x = T(36), .y = T(22), .scale = 1.6f, .solidBase = false, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/undead/crystal.png",
      .x = T(52), .y = T(9), .scale = 1.5f, .solidBase = true, .map = DM_MEZARLIK },

    // SESSİZ EL kontrat tahtası (loca T32,19 — salon sağ-alt, NPC'lerden uzak, [E] açar)
    { .texPath = "assets/decor/notice_board.png",
      .x = T(32), .y = T(19), .scale = 1.8f, .solidBase = true, .map = DM_LOCA },

    // --- TAŞDURAK dekorları (Faz DOGU — map 6) ---
    { .texPath = "assets/decor/desert/house1.png",       // Tüccar Ferah'ın evi
      .x = T(13), .y = T(27), .scale = 1.8f, .solidBase = true, .map = DM_DOGU },
    { .texPath = "assets/decor/desert/house2.png",       // Han (Duman)
      .x = T(20), .y = T(28), .scale = 1.8f, .solidBase = true, .map = DM_DOGU },
    { .texPath = "assets/decor/desert/house3.png",       // Kâtip konağı
      .x = T(27), .y = T(27), .scale = 1.8f, .solidBase = true, .map = DM_DOGU },
    { .texPath = "assets/decor/desert/pergola.png",      // meydan gölgeliği
      .x = T(17), .y = T(34), .scale = 1.6f, .solidBase = false, .map = DM_DOGU },
    { .texPath = "assets/decor/desert/market.png",       // pazar sahnesi (deve+halı)
      .x = T(24), .y = T(35), .scale = 1.6f, .solidBase = true, .map = DM_DOGU },
    { .texPath = "assets/decor/desert/acacia.png",       // vaha akasyası
      .x = T(19), .y = T(41), .scale = 1.6f, .solidBase = true, .map = DM_DOGU },
    { .texPath = "assets/decor/desert/cactus.png",
      .x = T(38), .y = T(30), .scale = 2.0f, .solidBase = true, .map = DM_DOGU },
    { .texPath = "assets/decor/desert/cactus.png",
      .x = T(52), .y = T(44), .scale = 2.0f, .solidBase = true, .map = DM_DOGU },
    { .texPath = "assets/decor/desert/dead_bush.png",
      .x = T(45), .y = T(22), .scale = 1.8f, .solidBase = false, .map = DM_DOGU },
    { .texPath = "assets/decor/desert/dead_bush.png",
      .x = T(66), .y = T(40), .scale = 1.8f, .solidBase = false, .map = DM_DOGU },
    { .texPath = "assets/decor/desert/temple.png",       // TAPINAK (mumya vadisi)
      .x = T(80), .y = T(14), .scale = 2.0f, .solidBase = true, .map = DM_DOGU },
    { .texPath = "assets/decor/desert/obelisk.png",
      .x = T(75), .y = T(16), .scale = 1.8f, .solidBase = true, .map = DM_DOGU },
    { .texPath = "assets/decor/desert/obelisk.png",
      .x = T(85), .y = T(16), .scale = 1.8f, .solidBase = true, .map = DM_DOGU },
};
const int DECOR_COUNT = (int)(sizeof(DECORS) / sizeof(DECORS[0]));

// Texture boyutları (yüklemede bir kez okunur — çizim/collision tek kaynaktan)
#define DECOR_CAP DECOR_TEX_CAP   // ortak tavan (decor.h) — 32 taşmıştı, CRASH
static float texW[DECOR_CAP], texH[DECOR_CAP];

static bool decorActive = true;
static int decorMap = 0;   // aktif harita (DecorDef.map ile eşleşen çizilir)
void decor_set_map(int mapId) { decorMap = mapId; }

void decor_set_active(bool active) { decorActive = active; }

// Occluder toplarken dokulara erişmek için: set_texture_sizes çağrısında
// diziyi saklıyoruz (aynı dizi kare boyunca geçerli).
R2DTexture **decorTextures_cached = NULL;

void decor_set_texture_sizes(R2DTexture **decorTextures)
{
    decorTextures_cached = decorTextures;
    if (DECOR_COUNT > DECOR_CAP)
        SDL_Log("UYARI: DECOR_COUNT (%d) > DECOR_CAP (%d) — tavani buyut!",
                DECOR_COUNT, DECOR_CAP);
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
    for (int i = 0; i < DECOR_COUNT && i < DECOR_CAP; i++) {
        if (decorTextures[i] == NULL) continue;
        if (DECORS[i].map != decorMap) continue;   // başka haritanın dekoru
        SDL_FRect r = decor_rect(i);
        SDL_FRect dst = {
            (r.x - camera->x) * zoom, (r.y - camera->y) * zoom,
            r.w * zoom, r.h * zoom,
        };
        r2d_draw_texture(renderer, decorTextures[i], NULL, &dst);
    }
}

// SU YANSIMALARI — gölgenin kardeşi: ayak çizgisinde çakılı ama güneşe göre
// değil AŞAĞI (aynada), soluk, ve YALNIZ ıslak karolara kırpılmış. Su
// geçişinden ÖNCE çizilir: kırılma yansımayı büker, derinlik süzgeci
// koyulaştırır, benekler üstünden geçer — yansıma "yapıştırılmış kopya"
// değil "suyun içindeki görüntü" olur, ekstra shader gerekmez.
// Kırpma karo karo: sprite dikdörtgeni ıslak olmayan karoya taşarsa o parça
// hiç çizilmez (yoksa çimende baş aşağı ağaç hayaleti belirir).
void decor_render_reflections(R2D *renderer, R2DTexture **decorTextures,
                              const Camera *camera)
{
    if (!decorActive) return;
    float zoom = camera->zoom;
    for (int i = 0; i < DECOR_COUNT && i < DECOR_CAP; i++) {
        if (decorTextures[i] == NULL) continue;
        if (DECORS[i].map != decorMap) continue;
        SDL_FRect r = decor_rect(i);

        float opX, opY, opW, opH;
        r2d_texture_opaque_bounds(decorTextures[i], &opX, &opY, &opW, &opH);
        float texW2 = 0.0f, texH2 = 0.0f;
        r2d_texture_size(decorTextures[i], &texW2, &texH2);
        if (texH2 <= 0.0f || opH <= 1.0f) continue;

        // Opak bölge DÜNYA koordinatlarında (gölgedeki hesabın aynısı, ekransız)
        float sxScale = r.w / texW2, syScale = r.h / texH2;
        float wx = r.x + opX * sxScale;          // yansımanın sol kenarı
        float ww = opW * sxScale;
        float footY = r.y + (opY + opH) * syScale;   // ayak çizgisi (dünya)
        // Yansıma hafif KISA (0.82): eğik bakışta su yüzeyi sıkışık görünür
        // (slynyrd: yansımalar hafifçe kısaltılır)
        float wh = opH * syScale * 0.82f;

        // Hızlı red: yansıma kutusu hiç ıslak karoya değmiyorsa hiç uğraşma
        int c0 = (int)(wx / TILE_SIZE), c1 = (int)((wx + ww) / TILE_SIZE);
        int r0 = (int)(footY / TILE_SIZE), r1 = (int)((footY + wh) / TILE_SIZE);
        bool any = false;
        for (int rr = r0; rr <= r1 && !any; rr++)
            for (int cc = c0; cc <= c1 && !any; cc++)
                if (tilemap_is_water(cc, rr)) any = true;
        if (!any) continue;

        // Karo karo kırp: her ıslak karoyla kesişen parça ayrı çizilir.
        // Parça eşlemesi: yansımanın üstü sprite'ın AYAĞI — dst kesiti
        // [f0,f1] ise src kesiti [1-f1, 1-f0] (reflect çizimi parçayı
        // kendi içinde aynaladığı için toplam eşleme doğru çıkıyor).
        for (int rr = r0; rr <= r1; rr++)
            for (int cc = c0; cc <= c1; cc++) {
                if (!tilemap_is_water(cc, rr)) continue;
                float tx0 = cc * (float)TILE_SIZE, ty0 = rr * (float)TILE_SIZE;
                float ix0 = wx > tx0 ? wx : tx0;
                float iy0 = footY > ty0 ? footY : ty0;
                float ix1 = (wx + ww) < (tx0 + TILE_SIZE) ? (wx + ww) : (tx0 + TILE_SIZE);
                float iy1 = (footY + wh) < (ty0 + TILE_SIZE) ? (footY + wh) : (ty0 + TILE_SIZE);
                if (ix1 <= ix0 || iy1 <= iy0) continue;

                float fx0 = (ix0 - wx) / ww, fx1 = (ix1 - wx) / ww;
                float fy0 = (iy0 - footY) / wh, fy1 = (iy1 - footY) / wh;
                SDL_FRect ps = {
                    opX + fx0 * opW,
                    opY + (1.0f - fy1) * opH,
                    (fx1 - fx0) * opW,
                    (fy1 - fy0) * opH };
                SDL_FRect pd = {
                    (ix0 - camera->x) * zoom, (iy0 - camera->y) * zoom,
                    (ix1 - ix0) * zoom, (iy1 - iy0) * zoom };
                r2d_draw_texture_reflect(renderer, decorTextures[i], &ps, &pd, 96);
            }
    }
}

// Güneş gölgeleri — zeminin ÜSTÜNE, dekorun ALTINA çizilir (main sırası).
// Sprite'ın yatık kopyası: taban ayak hizasında çakılı, tepe güneşten uzağa.
// Renk vertex'e siyaha yakın verilir; sprite dokusuyla çarpıldığı için silüet
// kendiliğinden çıkar (ayrı gölge sprite'ı gerekmez).
void decor_render_shadows(R2D *renderer, R2DTexture **decorTextures,
                          const Camera *camera,
                          float dx, float dy, float lenMul, float strength)
{
    if (!decorActive || strength <= 0.02f) return;
    float zoom = camera->zoom;
    SDL_Color col = { 22, 24, 34, (Uint8)(78.0f * strength) };

    for (int i = 0; i < DECOR_COUNT && i < DECOR_CAP; i++) {
        if (decorTextures[i] == NULL) continue;
        if (DECORS[i].map != decorMap) continue;
        SDL_FRect r = decor_rect(i);
        SDL_FRect dst = {
            (r.x - camera->x) * zoom, (r.y - camera->y) * zoom,
            r.w * zoom, r.h * zoom,
        };

        // ══ AYAK NOKTASI — eski hatanın kaynağı ══
        // Gölge sprite DİKDÖRTGENİNİN alt kenarından başlıyordu. Ağaç PNG'sinde
        // altta şeffaf boşluk var, yani o kenar gövdenin dibi DEĞİL — gölge
        // nesneden kopuk görünüyordu. Stardew de sprite dikdörtgenini
        // kullanmıyor (Character.cs: ayak noktası çarpışma kutusundan + elle
        // düzeltme). Biz opak sınırları yüklemede tarıyoruz (Unity "Trim"
        // muadili) ve ayak çizgisini oradan alıyoruz.
        float opX, opY, opW, opH;
        r2d_texture_opaque_bounds(decorTextures[i], &opX, &opY, &opW, &opH);
        float texH2 = 0.0f, texW2 = 0.0f;
        r2d_texture_size(decorTextures[i], &texW2, &texH2);
        if (texH2 <= 0.0f) continue;

        // Opak sınırları sprite'ın ekran dikdörtgenine ölçekle
        float sxScale = dst.w / texW2, syScale = dst.h / texH2;
        float oL = dst.x + opX * sxScale;              // opak sol
        float oW = opW * sxScale;                      // opak genişlik
        float oT = dst.y + opY * syScale;              // opak üst
        float footY = dst.y + (opY + opH) * syScale;   // AYAK ÇİZGİSİ
        float objH = footY - oT;                       // gerçek nesne yüksekliği
        if (objH <= 1.0f) continue;

        // Blinn shear'i: taban ayak çizgisinde çakılı, tepe güneşten uzağa.
        // Uzunluk gerçek nesne yüksekliğinin cot(α) katı.
        float offX = SDL_roundf(dx * lenMul * objH);
        float offY = SDL_roundf(dy * lenMul * objH);

        SDL_FPoint c4[4] = {
            { oL + offX,       footY + offY },   // TL = sprite tepesi, uzak uç
            { oL + oW + offX,  footY + offY },   // TR
            { oL,              footY        },   // BL = ayak çizgisi, yerinde
            { oL + oW,         footY        },   // BR
        };
        // UV: yalnız OPAK bölge — şeffaf kenarları gölgeye sokmak gölgeyi
        // yanlış boyutlu ve kopuk gösteriyordu.
        SDL_FRect srcOpaque = { opX, opY, opW, opH };
        r2d_draw_texture_quad(renderer, decorTextures[i], &srcOpaque, c4, col);

        // Temas lekesi: shear öğlen sıfıra yaklaşır ve nesne zeminden kopar;
        // bu leke o boşluğu doldurur ve gölgeyi ayak izine çiviler.
        // DÜNYA-uzayı ayak çizgisi — quad'la AYNI yer. Eskiden DECORS[i].y
        // (sprite dikdörtgeninin altı) geçiliyordu ve leke gölgenin altında
        // yüzüyordu: lich.png'de 158 px, dead_tree'de 32 px kayma.
        float footWorld = DECORS[i].y - (texH2 - opY - opH) * DECORS[i].scale;
        shadow_ground_blob(renderer, camera,
                           DECORS[i].x, footWorld, oW / zoom, strength);
    }
}

int decor_light_occluders(SDL_FRect *out, int cap, const Camera *camera,
                          float cx, float cy)
{
    if (!decorActive || out == NULL || cap <= 0) return 0;
    float zoom = camera->zoom;

    // En yakın nesneleri seç: occluder bütçesi küçük (16), uzaktakiler zaten
    // ışık menzilinde değil. Basit seçim sıralaması — n küçük.
    int    idx[64];
    float  dist[64];
    int    m = 0;
    for (int i = 0; i < DECOR_COUNT && i < DECOR_CAP && m < 64; i++) {
        if (decorTextures_cached[i] == NULL) continue;
        if (DECORS[i].map != decorMap) continue;
        if (!DECORS[i].solidBase) continue;      // çalı/saman ışığı kesmez
        float dx = DECORS[i].x - cx, dy = DECORS[i].y - cy;
        float d2 = dx * dx + dy * dy;
        if (d2 > 900.0f * 900.0f) continue;      // çok uzak
        idx[m] = i; dist[m] = d2; m++;
    }
    int n = 0;
    for (; n < cap && n < m; n++) {
        int best = n;
        for (int j = n + 1; j < m; j++) if (dist[j] < dist[best]) best = j;
        int ti = idx[best]; float td = dist[best];
        idx[best] = idx[n]; dist[best] = dist[n];
        idx[n] = ti; dist[n] = td;

        int i = idx[n];
        SDL_FRect r = decor_rect(i);
        // GÖVDE: alt %45 ve genişliğin ortadaki %60'ı. Ağaç için bu gövdedir
        // (taç ışığı kesmez); ev için duvar hattı.
        float bw = r.w * 0.60f, bh = r.h * 0.45f;
        out[n].x = (r.x + (r.w - bw) * 0.5f - camera->x) * zoom;
        out[n].y = (r.y + r.h - bh - camera->y) * zoom;
        out[n].w = bw * zoom;
        out[n].h = bh * zoom;
    }
    return n;
}

bool decor_blocks(SDL_FRect box)
{
    if (!decorActive) return false;
    for (int i = 0; i < DECOR_COUNT && i < DECOR_CAP; i++) {
        if (texW[i] <= 0.0f || !DECORS[i].solidBase) continue;   // çalı/saman engellemez
        if (DECORS[i].map != decorMap) continue;
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
    for (int i = 0; i < DECOR_COUNT && i < DECOR_CAP; i++) {
        if (texW[i] <= 0.0f || !DECORS[i].conceals) continue;
        if (DECORS[i].map != decorMap) continue;
        SDL_FRect r = decor_rect(i);   // çalı küçük — tüm sprite alanı örtüdür
        if (x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h)
            return true;
    }
    return false;
}
