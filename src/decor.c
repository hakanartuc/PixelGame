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
    { .texPath = "assets/decor/erw/ritual_stones.png",
      .x = T(108), .y = T(7), .scale = 2.0f, .solidBase = false },
    // KÖR KUYU (Kirli Mal & Aklama): köy dışında, yol kıyısında unutulmuş kuyu.
    // Gece kirli malı bırak → şafakta temiz kese (kim aldığını görmezsin).
    { .texPath = "assets/decor/erw/well.png",
      .x = T(26), .y = T(15), .scale = 2.0f, .solidBase = true,
      .baseWFrac = 0.85f, .baseHFrac = 0.30f },
    // KÖY-YENİDEN (Faz EPIC-3): ERW evleri — orta boy üçlü meydan çevresine,
    // BÜYÜK han yolun GÜNEYİNE (K2 kanonu: kervan yolu üstü, köy GD; feet
    // T24,30 → row 17-18 ana yolu ÇATI ÖRTMEZ). Kapı önleri npc.c duraklarıyla
    // eş — taşıyan ikisini birlikte taşır.
    { .texPath = "assets/buildings/erw/house_baran.png",
      .x = T(5.2f), .y = T(10.5f), .scale = 2.0f, .solidBase = true,
      .baseWFrac = 0.92f, .baseHFrac = 0.28f },   // Baran'ın evi (dükkanın batısı)
    { .texPath = "assets/buildings/erw/house_kemal.png",
      .x = T(19), .y = T(7.5f), .scale = 2.0f, .solidBase = true,
      .baseWFrac = 0.92f, .baseHFrac = 0.28f },   // Kemal'in evi (meydan doğusu)
    { .texPath = "assets/buildings/erw/house_osman.png",
      .x = T(4.5f), .y = T(16.5f), .scale = 2.0f, .solidBase = true,
      .baseWFrac = 0.92f, .baseHFrac = 0.28f },   // Osman'ın kulübesi (tarla batısı)
    // HAN (K2): ERW büyük taş yapı — yol kıyısı, güney cephe
    { .texPath = "assets/buildings/erw/inn.png",
      .x = T(24), .y = T(30), .scale = 2.0f, .solidBase = true,
      .baseWFrac = 0.92f, .baseHFrac = 0.26f },

    // KÖY DETAYLARI (harita görsel yenileme): el yerleşimi — rutin yollarının
    // dışında, meydanı çerçeveleyen küçük yaşam izleri
    { .texPath = "assets/decor/erw/barrel_1.png",
      .x = T(9.2f), .y = T(5.8f), .scale = 2.0f, .solidBase = true },   // Baran'ın dükkan yanı
    { .texPath = "assets/decor/erw/bench.png",
      .x = T(12.3f), .y = T(8.3f), .scale = 2.0f, .solidBase = false }, // meydan bankı
    { .texPath = "assets/decor/erw/barrel_2.png",
      .x = T(20.5f), .y = T(30.3f), .scale = 2.0f, .solidBase = true }, // han önü fıçılar
    { .texPath = "assets/decor/erw/fence.png",
      .x = T(12), .y = T(6.5f), .scale = 2.0f, .solidBase = true },   // meydan kuzeyi çiti
    // REVİR (Faz REVIR): köy KB köşesi — 3 yatak, Şifacı Ela başında
    { .texPath = "assets/decor/erw/bed.png",
      .x = T(21), .y = T(10.5f), .scale = 1.2f, .solidBase = false },
    { .texPath = "assets/decor/erw/bed.png",
      .x = T(21.9f), .y = T(10.5f), .scale = 1.2f, .solidBase = false },
    { .texPath = "assets/decor/erw/bed.png",
      .x = T(22.8f), .y = T(10.5f), .scale = 1.2f, .solidBase = false },
    { .texPath = "assets/decor/erw/bench.png",
      .x = T(14.6f), .y = T(10.8f), .scale = 2.0f, .solidBase = false },// güney yolu bankı

    // Ağaçlar: köy çevresi + goblin kampına giden yol (siper/LoS oyunu için)
    { .texPath = "assets/decor/erw/tree1_anim.png", .frames = 8, .x = T(24.5f), .y = T(6),  .scale = 2.0f, .solidBase = true, .baseWFrac = 0.40f, .baseHFrac = 0.14f },   // doğu koruluğu (Kemal evinin yolunu açtı)
    { .texPath = "assets/decor/erw/tree2_anim.png", .frames = 8, .x = T(19), .y = T(12), .scale = 2.0f, .solidBase = true, .baseWFrac = 0.40f, .baseHFrac = 0.14f },
    { .texPath = "assets/decor/erw/tree1_anim.png", .frames = 8, .x = T(21), .y = T(15), .scale = 2.0f, .solidBase = true, .baseWFrac = 0.40f, .baseHFrac = 0.14f },
    { .texPath = "assets/decor/erw/tree2_anim.png", .frames = 8, .x = T(16), .y = T(17), .scale = 2.0f, .solidBase = true, .baseWFrac = 0.40f, .baseHFrac = 0.14f },
    { .texPath = "assets/decor/erw/tree2_anim.png", .frames = 8, .x = T(15), .y = T(3),  .scale = 2.0f, .solidBase = true, .baseWFrac = 0.40f, .baseHFrac = 0.14f },
    { .texPath = "assets/decor/erw/tree1_anim.png", .frames = 8, .x = T(27), .y = T(19), .scale = 2.0f, .solidBase = true, .baseWFrac = 0.40f, .baseHFrac = 0.14f },

    // Çalılar: goblin kampına sneak yaklaşma hattı (içine eğilerek girilir)
    // GÖL KUZEY KIYISI AĞAÇLARI (Faz SU 2 — Hakan: "göl kıyısında ağaç yok,
    // ekler misin"): yansıma ayaktan GÜNEYE uzandığı için ağaç suyun KUZEY
    // kıyısında durmalı ki aynası suya düşsün. Ayaklar kıyı çimeninin alt
    // kenarına yakın — yansıma su çizgisinin hemen ardından başlar.
    { .texPath = "assets/decor/erw/tree1_anim.png", .frames = 8, .x = T(54.5f), .y = T(22.9f), .scale = 2.0f, .solidBase = true, .baseWFrac = 0.40f, .baseHFrac = 0.14f },
    { .texPath = "assets/decor/erw/tree2_anim.png", .frames = 8, .x = T(59.3f), .y = T(22.85f), .scale = 2.0f, .solidBase = true, .baseWFrac = 0.40f, .baseHFrac = 0.14f },
    { .texPath = "assets/decor/erw/tree1_anim.png", .frames = 8, .x = T(63.8f), .y = T(22.9f), .scale = 2.0f, .solidBase = true, .baseWFrac = 0.40f, .baseHFrac = 0.14f },
    { .texPath = "assets/decor/erw/bush_2.png",     .x = T(66.4f), .y = T(23.8f), .scale = 2.0f, .conceals = true },
    // Gölet kuzeyine tek ağaç: yansıması sığlık rafına + koyu ortaya düşer
    { .texPath = "assets/decor/erw/tree2_anim.png", .frames = 8, .x = T(7.5f),  .y = T(18.9f), .scale = 2.0f, .solidBase = true, .baseWFrac = 0.40f, .baseHFrac = 0.14f },
    // Su kenarı fenerleri — light.c LIGHTS[] ile AYNI koordinat (ışık orada)
    { .texPath = "assets/decor/erw/lamp_post.png", .x = T(9.6f),  .y = T(19.0f), .scale = 2.0f, .solidBase = true, .baseWFrac = 0.45f, .baseHFrac = 0.18f },
    { .texPath = "assets/decor/erw/lamp_post.png", .x = T(56.9f), .y = T(22.6f), .scale = 2.0f, .solidBase = true, .baseWFrac = 0.45f, .baseHFrac = 0.18f },
    { .texPath = "assets/decor/erw/bush_1.png", .x = T(22), .y = T(23), .scale = 2.0f, .conceals = true },
    { .texPath = "assets/decor/erw/bush_2.png", .x = T(24), .y = T(21), .scale = 2.0f, .conceals = true },
    { .texPath = "assets/decor/erw/bush_1.png", .x = T(21), .y = T(26), .scale = 2.0f, .conceals = true },
    { .texPath = "assets/decor/erw/bush_2.png", .x = T(27), .y = T(23), .scale = 2.0f, .conceals = true },
    { .texPath = "assets/decor/erw/bush_1.png", .x = T(23), .y = T(28), .scale = 2.0f, .conceals = true },

    // Saman balyaları: Osman'ın tarlası (çiftlik gizlenmesi)
    { .texPath = "assets/decor/erw/hay.png", .x = T(9.5f), .y = T(13),   .scale = 3.0f, .conceals = true },
    { .texPath = "assets/decor/erw/hay.png", .x = T(11), .y = T(13.5f), .scale = 3.0f, .conceals = true },
    // --- LANETLİ MEZARLIK dekorları (map = MEZARLIK) ---
    { .texPath = "assets/decor/erw/mz_lich.png",          // DEV İSKELET KALINTISI
      .x = T(30), .y = T(15), .scale = 2.2f, .solidBase = true, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/erw/mz_skulls.png",
      .x = T(27), .y = T(17), .scale = 1.4f, .solidBase = false, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/erw/mz_dead_tree1.png",
      .x = T(10), .y = T(16), .scale = 1.6f, .solidBase = true, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/erw/mz_dead_tree2.png",
      .x = T(50), .y = T(18), .scale = 1.6f, .solidBase = true, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/erw/mz_dead_tree1.png",
      .x = T(38), .y = T(36), .scale = 1.5f, .solidBase = true, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/erw/mz_ruin.png",
      .x = T(12), .y = T(9), .scale = 1.8f, .solidBase = true, .map = DM_MEZARLIK },   // Sağlam sandık harabesi
    { .texPath = "assets/decor/erw/mz_grave1.png",
      .x = T(17), .y = T(29), .scale = 1.6f, .solidBase = true, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/erw/mz_grave2.png",
      .x = T(20), .y = T(31), .scale = 1.6f, .solidBase = true, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/erw/mz_grave1.png",
      .x = T(43), .y = T(29), .scale = 1.6f, .solidBase = true, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/erw/mz_grave2.png",
      .x = T(41), .y = T(32), .scale = 1.6f, .solidBase = true, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/erw/mz_bones1.png",
      .x = T(24), .y = T(24), .scale = 1.6f, .solidBase = false, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/erw/mz_bones2.png",
      .x = T(36), .y = T(22), .scale = 1.6f, .solidBase = false, .map = DM_MEZARLIK },
    { .texPath = "assets/decor/erw/mz_crystal.png",
      .x = T(52), .y = T(9), .scale = 1.5f, .solidBase = true, .map = DM_MEZARLIK },

    // SESSİZ EL kontrat tahtası (loca T32,19 — salon sağ-alt, NPC'lerden uzak, [E] açar)
    { .texPath = "assets/decor/erw/notice_board.png",
      .x = T(32), .y = T(19), .scale = 1.8f, .solidBase = true, .map = DM_LOCA },

    // --- TAŞDURAK dekorları (Faz DOGU — TAŞDURAK-YENİDEN, 2× yerleşim) ---
    // Cadde y=35 hattı: batı portal T(4,35) → dükkan/han/konak kuzey sırada,
    // pazar güneyde (vahanın doğusu), ŞEHİR SURU doğu kapısı (tapınak yolu).
    // Kapı önü = NPC durağı kuralı (npc.c ile birlikte taşınır).
    { .texPath = "assets/decor/erw/ds_house1.png",       // Tüccar Ferah'ın evi
      .x = T(16), .y = T(30), .scale = 2.0f, .solidBase = true,
      .baseWFrac = 0.92f, .baseHFrac = 0.28f, .map = DM_DOGU },
    { .texPath = "assets/decor/erw/ds_house2.png",       // Han (Duman)
      .x = T(34), .y = T(30), .scale = 2.0f, .solidBase = true,
      .baseWFrac = 0.92f, .baseHFrac = 0.28f, .map = DM_DOGU },
    { .texPath = "assets/decor/erw/ds_house3.png",       // Kâtip konağı
      .x = T(52), .y = T(30), .scale = 2.0f, .solidBase = true,
      .baseWFrac = 0.92f, .baseHFrac = 0.28f, .map = DM_DOGU },
    { .texPath = "assets/decor/erw/ds_house4.png",       // ŞEHİR SURU — doğu kapısı
      .x = T(70), .y = T(36), .scale = 2.0f, .solidBase = true,
      .baseWFrac = 0.96f, .baseHFrac = 0.45f, .map = DM_DOGU },
    { .texPath = "assets/decor/erw/ds_house6.png",       // pazar (kemerli çarşı)
      .x = T(33), .y = T(41), .scale = 2.0f, .solidBase = true,
      .baseWFrac = 0.92f, .baseHFrac = 0.30f, .map = DM_DOGU },
    { .texPath = "assets/decor/erw/ds_palm.png",         // vaha palmiyesi
      .x = T(24), .y = T(39.5f), .scale = 2.0f, .solidBase = true,
      .baseWFrac = 0.35f, .baseHFrac = 0.12f, .map = DM_DOGU },
    { .texPath = "assets/decor/erw/ds_cactus.png",
      .x = T(44), .y = T(34), .scale = 2.0f, .solidBase = true, .map = DM_DOGU },
    { .texPath = "assets/decor/erw/ds_cactus.png",
      .x = T(52), .y = T(44), .scale = 2.0f, .solidBase = true, .map = DM_DOGU },
    { .texPath = "assets/decor/erw/ds_dead_bush.png",
      .x = T(45), .y = T(22), .scale = 2.0f, .solidBase = false, .map = DM_DOGU },
    { .texPath = "assets/decor/erw/ds_dead_bush.png",
      .x = T(66), .y = T(40), .scale = 2.0f, .solidBase = false, .map = DM_DOGU },
    { .texPath = "assets/decor/erw/ds_temple.png",       // TAPINAK (mumya vadisi)
      .x = T(80), .y = T(14), .scale = 2.0f, .solidBase = true,
      .baseWFrac = 0.92f, .baseHFrac = 0.30f, .map = DM_DOGU },
    { .texPath = "assets/decor/erw/ds_obelisk.png",
      .x = T(75), .y = T(16), .scale = 2.0f, .solidBase = true,
      .baseWFrac = 0.6f, .baseHFrac = 0.15f, .map = DM_DOGU },
    { .texPath = "assets/decor/erw/ds_obelisk.png",
      .x = T(85), .y = T(16), .scale = 2.0f, .solidBase = true,
      .baseWFrac = 0.6f, .baseHFrac = 0.15f, .map = DM_DOGU },
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
        if (decorTextures[i] != NULL) {
            r2d_texture_size(decorTextures[i], &texW[i], &texH[i]);
            // Şeritli dekor: texW KARE genişliği olarak saklanır — decor_rect,
            // blocks, conceals, occluders hiç değişmeden doğru kalır.
            if (DECORS[i].frames > 1) texW[i] /= (float)DECORS[i].frames;
        }
    }
}

// Şeridin o anki karesi — çizim ve gölge AYNI kareyi kullanır (determinist).
// i*3 faz kayması: iki ağaç yan yana senkron sallanmasın.
static int decor_frame(int i)
{
    int f = DECORS[i].frames;
    if (f <= 1) return 0;
    return (int)((SDL_GetTicks() / 150 + (Uint64)i * 3) % (Uint64)f);
}

// Opak sınırlar — statikte r2d taramasından, şeritte KARE-YEREL kutu (bake
// union-trim garantisi). Dönen değerler kare uzayında; şerit içi kare kayması
// src kurulurken frameOff ile eklenir.
static void decor_opaque(int i, R2DTexture *tex,
                         float *opX, float *opY, float *opW, float *opH)
{
    if (DECORS[i].frames > 1) {
        *opX = 0.0f; *opY = 0.0f; *opW = texW[i]; *opH = texH[i];
    } else {
        r2d_texture_opaque_bounds(tex, opX, opY, opW, opH);
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
        if (DECORS[i].frames > 1) {
            SDL_FRect src = { (float)decor_frame(i) * texW[i], 0.0f,
                              texW[i], texH[i] };
            r2d_draw_texture(renderer, decorTextures[i], &src, &dst);
        } else {
            r2d_draw_texture(renderer, decorTextures[i], NULL, &dst);
        }
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
        decor_opaque(i, decorTextures[i], &opX, &opY, &opW, &opH);
        float texW2 = texW[i], texH2 = texH[i];   // şeritte KARE boyutu
        float frameOff = (float)decor_frame(i) * texW[i];   // şerit içi kayma
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
                    frameOff + opX + fx0 * opW,
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
        decor_opaque(i, decorTextures[i], &opX, &opY, &opW, &opH);
        float texW2 = texW[i], texH2 = texH[i];   // şeritte KARE boyutu
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
        // yanlış boyutlu ve kopuk gösteriyordu. Şeritte o anki kare seçilir:
        // gölge de ağaçla birlikte sallanır (çizimle AYNI kare — decor_frame).
        SDL_FRect srcOpaque = { (float)decor_frame(i) * texW[i] + opX, opY,
                                opW, opH };
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
        // (taç ışığı kesmez); ev için duvar hattı. Taban oranı verilmişse
        // (ERW ağaçları) genişlik oradan gelir — 164px taçlı ağaçta %60 bile
        // gövdenin dört katıydı, ışık dev blok gölge atıyordu.
        float bw = r.w * (DECORS[i].baseWFrac > 0.0f ? DECORS[i].baseWFrac : 0.60f);
        float bh = r.h * (DECORS[i].baseHFrac > 0.0f ? DECORS[i].baseHFrac * 2.0f : 0.45f);
        out[n].x = (r.x + (r.w - bw) * 0.5f - camera->x) * zoom;
        out[n].y = (r.y + r.h - bh - camera->y) * zoom;
        out[n].w = bw * zoom;
        out[n].h = bh * zoom;
    }
    return n;
}

// Katı taban dikdörtgeni — TEK KAYNAK (çarpışma + occluder buradan okur;
// yerleşim tuzağı dersi: aynı formül iki yerde yaşamaz).
// Varsayılan: alt %35 × tam genişlik. DecorDef oranları verirse onlar kazanır
// (ağaçta gövde dibi — taç katı değil, arkasından yürünür).
static SDL_FRect decor_base_rect(int i)
{
    SDL_FRect r = decor_rect(i);
    float wf = DECORS[i].baseWFrac > 0.0f ? DECORS[i].baseWFrac : 1.0f;
    float hf = DECORS[i].baseHFrac > 0.0f ? DECORS[i].baseHFrac : 0.35f;
    SDL_FRect base = {
        r.x + r.w * (1.0f - wf) * 0.5f,   // yatay ortalı (taban orta çapa)
        r.y + r.h * (1.0f - hf),
        r.w * wf, r.h * hf,
    };
    return base;
}

bool decor_blocks(SDL_FRect box)
{
    if (!decorActive) return false;
    for (int i = 0; i < DECOR_COUNT && i < DECOR_CAP; i++) {
        if (texW[i] <= 0.0f || !DECORS[i].solidBase) continue;   // çalı/saman engellemez
        if (DECORS[i].map != decorMap) continue;
        SDL_FRect base = decor_base_rect(i);
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
