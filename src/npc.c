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
    { .name = "Tüccar Baran",
      .sheetPath = "assets/characters/npcs/Bartender_Bruno.png",
      .dialogFile = "assets/dialogs/merchant.txt",
      .isVendor = true, .questId = "simya_masa",
      .schedule = {
          { .hour =  8.0f, .x = T(10),   .y = T(5),   .indoors = false },  // dükkan başı
          { .hour = 20.0f, .x = T(10),   .y = T(4.2f), .indoors = true },  // eve girer
      }, .stopCount = 2 },
    { .name = "Yaşlı Kemal",
      .sheetPath = "assets/characters/npcs/Farmer_Bob.png",
      .dialogFile = "assets/dialogs/elder.txt",
      .isVendor = false, .questId = "goblin_derdi",
      .schedule = {
          { .hour =  7.0f, .x = T(13),    .y = T(7),     .indoors = false }, // meydan
          { .hour = 12.0f, .x = T(17.5f), .y = T(11.5f), .indoors = false }, // han önü (Nazife ziyareti)
          { .hour = 16.0f, .x = T(7),     .y = T(12),    .indoors = false }, // tarla kıyısı
          { .hour = 21.0f, .x = T(14),    .y = T(6.2f),  .indoors = true },
      }, .stopCount = 4 },
    { .name = "Köylü Osman",
      .sheetPath = "assets/characters/npcs/Farmer_Buba.png",
      .dialogFile = "assets/dialogs/villager.txt",
      .isVendor = false, .questId = NULL,
      .schedule = {
          { .hour =  9.0f, .x = T(7),    .y = T(11),   .indoors = false }, // tarla
          { .hour = 13.0f, .x = T(11),   .y = T(8),    .indoors = false }, // meydanda mola
          { .hour = 15.0f, .x = T(7),    .y = T(11),   .indoors = false }, // tarlaya dönüş
          { .hour = 19.0f, .x = T(6),    .y = T(10.2f), .indoors = true },
      }, .stopCount = 4 },
    { .name = "Demirci Mahir",
      .sheetPath = "assets/characters/npcs/Miner_Mike.png",
      .dialogFile = "assets/dialogs/demirci.txt",
      .isVendor = false, .questId = "demirci_ocak",
      .schedule = {
          { .hour =  7.0f, .x = T(8), .y = T(7.6f), .indoors = false },  // ocak başı
          { .hour = 21.0f, .x = T(8), .y = T(6.4f), .indoors = true },   // gece kapanır
      }, .stopCount = 2 },
    { .name = "Muhafız",
      .sheetPath = "assets/characters/npcs/Guard_Swordman.png",
      .dialogFile = "assets/dialogs/guard.txt",
      .isVendor = false, .questId = NULL,
      .schedule = {
          { .hour = 0.0f, .x = T(11), .y = T(9), .indoors = false },   // nöbet noktası (meydan)
      }, .stopCount = 1,
      .frameSize = 48, .drawScale = 2.0f, .maxHealth = 120.0f, .isGuard = true },

    // K9 Kandil (D1a): oğlunu Eski Geçit'te kaybeden ana — gündüz köyde,
    // akşam geçit kapısına yürür (kandilin başında bekler)
    { .name = "Fatma",
      .sheetPath = "assets/characters/npcs/Bartender_Katy.png",
      .dialogFile = "assets/dialogs/fatma.txt",
      .isVendor = false, .questId = NULL,
      .schedule = {
          { .hour = 8.0f,  .x = T(13), .y = T(11), .indoors = false },  // gündüz meydan kıyısı
          { .hour = 17.0f, .x = T(142.5f), .y = T(9), .indoors = false }, // akşam geçit kapısı
      }, .stopCount = 2 },

    // K2 Son Müşteri (D1b): hancı — kervancının karısı; gece sofra kurar
    { .name = "Nazife",
      .sheetPath = "assets/characters/npcs/Chef_Chloe.png",
      .dialogFile = "assets/dialogs/nazife.txt",
      .isVendor = false, .questId = NULL,
      .schedule = {
          { .hour = 7.0f,  .x = T(18),   .y = T(12.6f), .indoors = false }, // han kapısı
          { .hour = 19.0f, .x = T(19.2f), .y = T(13),    .indoors = false }, // sofra başı
      }, .stopCount = 2 },

    // S3 Sayan Adam (D1c): Nuh — gündüz sunak yolunda sayarak volta, gece köy kenarı
    { .name = "Nuh",
      .sheetPath = "assets/characters/npcs/Fisherman_Fin.png",
      .dialogFile = "assets/dialogs/nuh.txt",
      .isVendor = false, .questId = NULL,
      .schedule = {
          { .hour = 7.0f,  .x = T(60), .y = T(18),   .indoors = false },  // yol üstü
          { .hour = 12.0f, .x = T(64), .y = T(19),   .indoors = false },  // volta (sayarak)
          { .hour = 16.0f, .x = T(60), .y = T(18),   .indoors = false },
          { .hour = 20.0f, .x = T(21), .y = T(12),   .indoors = false },  // gece köy kenarı
      }, .stopCount = 4 },
    // Zehra — Nuh'un torunu; köy meydanında dedesini dert eder
    { .name = "Zehra",
      .sheetPath = "assets/characters/npcs/Desert_Person_2.png",
      .dialogFile = "assets/dialogs/zehra.txt",
      .isVendor = false, .questId = NULL,
      .schedule = {
          { .hour = 8.0f,  .x = T(12), .y = T(7.5f), .indoors = false },  // meydan
          { .hour = 20.0f, .x = T(14), .y = T(6.2f), .indoors = true },   // gece içeri
      }, .stopCount = 2,
      .frameSize = 32, .drawScale = 2.8f },

    // KÖR KUYU tanıtıcısı: gece kuyu civarında BİR KEZ belirir, anlatır, kaybolur.
    // Kim olduğu söylenmez — 'sayanlar'dan biri (yeraltı örgütü tohumu).
    { .name = "Yabancı",
      .sheetPath = "assets/characters/npcs/Desert_Person_3.png",
      .dialogFile = "assets/dialogs/yabanci.txt",
      .isVendor = false, .questId = NULL,
      .schedule = {
          { .hour = 0.0f, .x = T(27), .y = T(16), .indoors = false },  // kuyu yanı
      }, .stopCount = 1,
      .frameSize = 32, .drawScale = 2.8f },

    // Dünya Olayı: Gezgin Kervan tüccarı — yalnız kervan günlerinde görünür
    { .name = "Kervancı Selim",
      .sheetPath = "assets/characters/npcs/Desert_Trader_1.png",
      .dialogFile = "assets/dialogs/kervanci.txt",
      .isVendor = true, .questId = NULL,
      .schedule = {
          { .hour = 0.0f, .x = T(9), .y = T(11), .indoors = false },  // meydan güneyi
      }, .stopCount = 1,
      .frameSize = 32, .drawScale = 2.8f },

    // --- SESSİZ EL LOCASI (Suikastçi P1 — map 5 = MAP_LOCA) ---
    // Usta: az konuşur, görevleri verir. Eski saray celladı — ESO derinliği
    // diyalogda (neriman.txt).
    { .name = "'Dilsiz' Neriman",
      .sheetPath = "assets/characters/npcs/Pharaoh.png",
      .dialogFile = "assets/dialogs/neriman.txt",
      .isVendor = false, .questId = NULL,
      .schedule = {
          { .hour = 0.0f, .x = T(28), .y = T(14), .indoors = false },  // salon başköşe
      }, .stopCount = 1,
      .map = 5, .frameSize = 32, .drawScale = 2.8f },
    { .name = "Gözcü Çelme",
      .sheetPath = "assets/characters/npcs/Desert_Person_1.png",
      .dialogFile = "assets/dialogs/celme.txt",
      .isVendor = false, .questId = NULL,
      .schedule = {
          { .hour = 0.0f, .x = T(21), .y = T(12), .indoors = false },  // tünel ağzı
      }, .stopCount = 1,
      .map = 5, .frameSize = 32, .drawScale = 2.8f },
    { .name = "Zehirci İğne",
      .sheetPath = "assets/characters/npcs/Desert_Person_4.png",
      .dialogFile = "assets/dialogs/igne.txt",
      .isVendor = false, .questId = NULL,
      .schedule = {
          { .hour = 0.0f, .x = T(27), .y = T(22), .indoors = false },  // zehir hücresi
      }, .stopCount = 1,
      .map = 5, .frameSize = 32, .drawScale = 2.8f },

    // ŞİFACI (Faz REVIR): revirin sahibi — yaralı köylüler ona yatar,
    // masraf ödemesi + oyuncuya paralı tedavi. Revir köy kuzeybatısında.
    { .name = "Şifacı Ela",
      .sheetPath = "assets/characters/npcs/Healer_Witch.png",
      .dialogFile = "assets/dialogs/sifaci.txt",
      .isVendor = false, .questId = NULL,
      .schedule = {
          { .hour = 0.0f, .x = T(4.5f), .y = T(6.5f), .indoors = false },  // revir başı
      }, .stopCount = 1,
      .frameSize = 32, .drawScale = 2.8f },
};
const int NPC_COUNT = (int)(sizeof(NPCS) / sizeof(NPCS[0]));

// Def'teki 0 alanlar → varsayılanlar (eski NPC girdileri dokunulmadan kaldı)
static int def_frame(int i)     { return NPCS[i].frameSize > 0 ? NPCS[i].frameSize : NPC_FRAME; }
static float def_scale(int i)   { return NPCS[i].drawScale > 0.0f ? NPCS[i].drawScale : NPC_DRAW_SCALE; }
float npc_max_health(int i)
{
    if (i < 0 || i >= NPC_COUNT) return NPC_MAX_HEALTH;
    return NPCS[i].maxHealth > 0.0f ? NPCS[i].maxHealth : NPC_MAX_HEALTH;
}

// Çalışma durumu: konum + görünürlük (tanımlar sabit, hareket burada yaşar)
typedef struct {
    float x, y;
    bool hidden;      // içeride (çizilmez, konuşulamaz)
    bool walking;     // isim etiketi/anim için (şimdilik idle anim kayarak yürür)
    bool initialized;
    float health;     // suç sistemi: NPC'ler ölebilir
    bool alive;       // ölüm KALICI (save'e yazılır); ölü = hidden gibi davranır
    float hitFlash;   // vuruş kızarması (sn, geri sayar)
    char bark[96];    // baş üstü balon metni (D1a — boş = yok)
    float barkTimer;  // kalan balon süresi
    bool scriptWalk;  // sahne yürütmesi aktif (rutini ezer)
    float scriptX, scriptY;
    bool forceHidden; // olay NPC'si (kervan): main görünürlüğü yönetir
    // REVİR (Faz REVIR): sivil 0'a düşmez — 1 canda bayılır, revire yatar
    bool infirm;      // revirde yatıyor (hidden gibi davranır)
    int  infirmDay;   // yatış günü (masraf bileşik artar)
    int  beatCount;   // kaçıncı yatış (masraf çarpanı — seri dayak caydırma)
    bool grudge;      // köy topladı ama AFFETMEDİ (soğuk diyalog + kalıcı zam)
} NpcState;
#define NPC_STATE_CAP 16   // NPC_COUNT tavanı (12'de doluydu — Şifacı ile büyüdü)
static NpcState npcState[NPC_STATE_CAP];

void npc_reset(void)
{
    for (int i = 0; i < NPC_STATE_CAP; i++) {
        npcState[i].alive = true;
        npcState[i].health = (i < NPC_COUNT) ? npc_max_health(i) : NPC_MAX_HEALTH;
        npcState[i].hitFlash = 0.0f;
        npcState[i].infirm = false;
        npcState[i].infirmDay = 0;
        npcState[i].beatCount = 0;
        npcState[i].grudge = false;
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

static bool npcActive = true;   // dünyada mıyız (zindanda NPC işlemez)
static int npcMap = 0;            // aktif harita — NPCS[i].map eşleşmeli
void npc_set_map(int mapId) { npcMap = mapId; }
#define NPC_ON_MAP(i) (NPCS[i].map == npcMap)
static int talkingIdx = -1;       // diyalogdaki NPC — hareketi durur
static bool shelter = false;      // yağmur sığınması (main basar)
static int shelterExempt = -1;    // sığınmayan NPC (kervancı — gölgelik)

void npc_set_talking(int i) { talkingIdx = i; }

void npc_set_shelter(bool s, int exemptIdx)
{
    shelter = s;
    shelterExempt = exemptIdx;
}

void npc_force_hidden(int i, bool hidden)
{
    if (i < 0 || i >= NPC_COUNT || i >= NPC_STATE_CAP) return;
    npcState[i].forceHidden = hidden;
    if (hidden) npcState[i].hidden = true;
}

void npc_set_active(bool active) { npcActive = active; }

void npc_update(float dt, float hours)
{
    if (!npcActive) return;
    for (int i = 0; i < NPC_COUNT && i < NPC_STATE_CAP; i++) {
        NpcState *st = &npcState[i];
        if (!NPC_ON_MAP(i)) continue;   // başka haritanın NPC'si (loca vs dünya)
        if (!st->alive) continue;   // ölü NPC rutin yürümez
        if (st->infirm) { st->hidden = true; st->walking = false; continue; }
        if (st->hitFlash > 0.0f) st->hitFlash -= dt;
        if (!st->initialized) {
            const NpcStop *s0 = current_stop(&NPCS[i], hours);
            st->x = s0->x;
            st->y = s0->y;
            st->hidden = s0->indoors || st->forceHidden;
            st->initialized = true;
        }

        if (st->barkTimer > 0.0f) {
            st->barkTimer -= dt;
            if (st->barkTimer <= 0.0f) st->bark[0] = '\0';
        }

        // Oyuncuyla konuşuyor: olduğu yerde durur (rutin/sahne yürüyüşü bekler)
        if (i == talkingIdx) {
            st->walking = false;
            continue;
        }

        // Sahne yürütmesi (D1a): rutini ezer — hedefe varana kadar yürü, dur
        if (st->scriptWalk) {
            float sdx = st->scriptX - st->x;
            float sdy = st->scriptY - st->y;
            float sdist = SDL_sqrtf(sdx * sdx + sdy * sdy);
            st->hidden = false;
            if (sdist > 4.0f) {
                st->walking = true;
                float step = NPC_WALK_SPEED * dt;
                if (step > sdist) step = sdist;
                st->x += sdx / sdist * step;
                st->y += sdy / sdist * step;
            } else {
                st->walking = false;   // vardı — npc_walk_done true döner, bekler
            }
            continue;
        }

        if (NPCS[i].isGuard) continue;   // muhafızı guard.c beyni yürütür

        // Yağmur sığınması: köylü evine çekilir (yalnız DÜNYA sakinleri —
        // loca yeraltıdır, yağmur işlemez)
        if (shelter && NPCS[i].map == 0 && i != shelterExempt) {
            st->hidden = true;
            st->walking = false;
            continue;
        }

        const NpcStop *target = current_stop(&NPCS[i], hours);
        float dx = target->x - st->x;
        float dy = target->y - st->y;
        float dist = SDL_sqrtf(dx * dx + dy * dy);

        if (dist > 4.0f) {
            // Hedefe yürü — yürüyene kadar dışarıda görünür
            st->hidden = st->forceHidden;
            st->walking = true;
            float step = NPC_WALK_SPEED * dt;
            if (step > dist) step = dist;
            st->x += dx / dist * step;
            st->y += dy / dist * step;
        } else {
            st->walking = false;
            st->hidden = target->indoors || st->forceHidden;
        }
    }
}

bool npc_bark_active(int i)
{
    if (i < 0 || i >= NPC_COUNT || i >= NPC_STATE_CAP) return false;
    return npcState[i].barkTimer > 0.0f;
}

void npc_bark(int i, const char *text, float seconds)
{
    if (i < 0 || i >= NPC_COUNT || i >= NPC_STATE_CAP) return;
    SDL_strlcpy(npcState[i].bark, text, sizeof(npcState[i].bark));
    npcState[i].barkTimer = seconds;
}

void npc_walk_to(int i, float x, float y)
{
    if (i < 0 || i >= NPC_COUNT || i >= NPC_STATE_CAP) return;
    npcState[i].scriptWalk = true;
    npcState[i].scriptX = x;
    npcState[i].scriptY = y;
}

bool npc_walk_done(int i)
{
    if (i < 0 || i >= NPC_COUNT || i >= NPC_STATE_CAP) return true;
    if (!npcState[i].scriptWalk) return true;
    float dx = npcState[i].scriptX - npcState[i].x;
    float dy = npcState[i].scriptY - npcState[i].y;
    return dx * dx + dy * dy < 6.0f * 6.0f;
}

void npc_walk_cancel(int i)
{
    if (i < 0 || i >= NPC_COUNT || i >= NPC_STATE_CAP) return;
    npcState[i].scriptWalk = false;
}

bool npc_visible_pos(int i, float *x, float *y)
{
    if (!npcActive) return false;
    if (i < 0 || i >= NPC_COUNT || i >= NPC_STATE_CAP ||
        !NPC_ON_MAP(i) || npcState[i].hidden || !npcState[i].alive)
        return false;
    *x = npcState[i].x;
    *y = npcState[i].y;
    return true;
}

int npc_at_point(float wx, float wy)
{
    for (int i = 0; i < NPC_COUNT && i < NPC_STATE_CAP; i++) {
        float nx, ny;
        if (!npc_visible_pos(i, &nx, &ny)) continue;
        // Çizim kutusu (render ile aynı hiza: x,y = ayak noktası)
        float drawSize = def_frame(i) * def_scale(i);
        if (wx >= nx - drawSize / 2.0f && wx <= nx + drawSize / 2.0f &&
            wy >= ny - drawSize + 8.0f && wy <= ny + 8.0f)
            return i;
    }
    return -1;
}

bool npc_damage(int i, float dmg)
{
    if (i < 0 || i >= NPC_COUNT || i >= NPC_STATE_CAP || !npcState[i].alive)
        return false;
    npcState[i].health -= dmg;
    npcState[i].hitFlash = 0.15f;
    // REVİR: sivilin canı 0'A İNMEZ — 1'de kilitlenir (0 = ölüm, o yalnız
    // muhafızda/suikastta). Bayıltma kararını main verir (npc_send_infirmary).
    if (!NPCS[i].isGuard && npcState[i].health < 1.0f) {
        npcState[i].health = 1.0f;
        return false;
    }
    if (npcState[i].health <= 0.0f) {
        npcState[i].health = 0.0f;
        npcState[i].alive = false;   // kalıcı — npc_reset/save dışında geri gelmez
        return true;
    }
    return false;
}

bool npc_is_alive(int i)
{
    return i >= 0 && i < NPC_COUNT && i < NPC_STATE_CAP && npcState[i].alive;
}

float npc_health(int i)
{
    if (i < 0 || i >= NPC_COUNT || i >= NPC_STATE_CAP) return 0.0f;
    return npcState[i].health;
}

void npc_set_alive(int i, bool alive)
{
    if (i < 0 || i >= NPC_COUNT || i >= NPC_STATE_CAP) return;
    npcState[i].alive = alive;
    npcState[i].health = alive ? npc_max_health(i) : 0.0f;
}

bool npc_pos(int i, float *x, float *y)
{
    if (i < 0 || i >= NPC_COUNT || i >= NPC_STATE_CAP || !npcState[i].initialized)
        return false;
    *x = npcState[i].x;
    *y = npcState[i].y;
    return true;
}

void npc_set_pos(int i, float x, float y)
{
    if (i < 0 || i >= NPC_COUNT || i >= NPC_STATE_CAP) return;
    npcState[i].x = x;
    npcState[i].y = y;
}

int npc_nearest_in_range(float px, float py)
{
    if (!npcActive) return -1;
    int best = -1;
    float bestDist = NPC_INTERACT_RANGE;
    for (int i = 0; i < NPC_COUNT && i < NPC_STATE_CAP; i++) {
        if (!NPC_ON_MAP(i)) continue;
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
                    const Camera *camera, TTF_Font *font, int nearestIdx,
                    bool sneaking)
{
    if (!npcActive) return;
    float zoom = camera->zoom;
    // Tüm NPC'ler aynı ritimde soluk alır — NPC başına sayaç tutmaya değmez
    int frame = (int)(SDL_GetTicks() / NPC_IDLE_SPEED_MS) % NPC_IDLE_FRAMES;

    for (int i = 0; i < NPC_COUNT && i < NPC_STATE_CAP; i++) {
        if (!NPC_ON_MAP(i)) continue;
        if (npcTextures[i] == NULL || npcState[i].hidden || !npcState[i].alive)
            continue;

        int fs = def_frame(i);
        SDL_FRect src = {
            .x = (float)(frame * fs),
            .y = (float)(NPC_IDLE_ROW * fs),
            .w = (float)fs, .h = (float)fs,
        };
        // x,y = ayak noktası → çizim kutusu ayağa hizalanır
        float drawSize = fs * def_scale(i);
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
                draw_text(renderer, font, "[E] Konuş",
                          dst.x + dst.w / 2.0f - 40.0f, dst.y - 40.0f, hint);
                if (sneaking) {   // yankesicilik daveti (eğilme = niyet)
                    SDL_Color vio = { 200, 160, 255, 255 };
                    draw_text(renderer, font, "[P] Yankesicilik",
                              dst.x + dst.w / 2.0f - 60.0f, dst.y - 60.0f, vio);
                }
            }

            // Bark balonu (D1a): ismin üstünde, son yarım saniyede solar
            if (npcState[i].bark[0] != '\0' && npcState[i].barkTimer > 0.0f) {
                int bw = 0, bh = 0;
                TTF_GetStringSize(font, npcState[i].bark, 0, &bw, &bh);
                float bx = dst.x + dst.w / 2.0f - bw / 2.0f;
                float by = dst.y - 46.0f - (i == nearestIdx ? 20.0f : 0.0f);
                float a = npcState[i].barkTimer < 0.5f
                        ? npcState[i].barkTimer / 0.5f : 1.0f;
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                r2d_set_color(renderer, 16, 14, 22, (Uint8)(220 * a));
                r2d_fill_rect(renderer, &(SDL_FRect){ bx - 8.0f, by - 5.0f,
                                                      bw + 16.0f, bh + 10.0f });
                SDL_Color bc = { 235, 230, 245, (Uint8)(255 * a) };
                draw_text(renderer, font, npcState[i].bark, bx, by, bc);
            }
        }
    }
}

// --- REVİR (Faz REVIR): bayılma + yatış + taburcu + kırgınlık ---

bool npc_infirm(int i)
{
    return i >= 0 && i < NPC_COUNT && i < NPC_STATE_CAP && npcState[i].infirm;
}

int npc_infirm_day(int i)
{
    if (i < 0 || i >= NPC_COUNT || i >= NPC_STATE_CAP) return 0;
    return npcState[i].infirmDay;
}

int npc_beat_count(int i)
{
    if (i < 0 || i >= NPC_COUNT || i >= NPC_STATE_CAP) return 0;
    return npcState[i].beatCount;
}

bool npc_grudge(int i)
{
    return i >= 0 && i < NPC_COUNT && i < NPC_STATE_CAP && npcState[i].grudge;
}

void npc_send_infirmary(int i, int day)
{
    if (i < 0 || i >= NPC_COUNT || i >= NPC_STATE_CAP) return;
    npcState[i].infirm = true;
    npcState[i].infirmDay = day;
    npcState[i].beatCount++;
    npcState[i].hidden = true;   // dünyadan çekilir (revirde yatar)
}

void npc_infirmary_discharge(int i, bool grudge)
{
    if (i < 0 || i >= NPC_COUNT || i >= NPC_STATE_CAP) return;
    npcState[i].infirm = false;
    npcState[i].hidden = false;      // rutin bir sonraki update'te konumlar
    npcState[i].health = npc_max_health(i);
    if (grudge) npcState[i].grudge = true;
}

// Save köprüsü: yatış durumu toleranslı kuyruktan gelir/gider
void npc_infirm_state_get(int i, int *infirm, int *day, int *beats, int *grudge)
{
    if (i < 0 || i >= NPC_COUNT || i >= NPC_STATE_CAP) {
        *infirm = 0; *day = 0; *beats = 0; *grudge = 0; return;
    }
    *infirm = npcState[i].infirm ? 1 : 0;
    *day = npcState[i].infirmDay;
    *beats = npcState[i].beatCount;
    *grudge = npcState[i].grudge ? 1 : 0;
}

void npc_infirm_state_set(int i, int infirm, int day, int beats, int grudge)
{
    if (i < 0 || i >= NPC_COUNT || i >= NPC_STATE_CAP) return;
    npcState[i].infirm = infirm != 0;
    npcState[i].infirmDay = day;
    npcState[i].beatCount = beats;
    npcState[i].grudge = grudge != 0;
    if (npcState[i].infirm) npcState[i].hidden = true;
}

// Revirde yatanları çiz: yatak pozisyonlarında soluk siluet + isim
// (bedX/bedY: ilk yatağın dünya konumu; yataklar yatay dizilir)
void npc_render_infirmary(R2D *renderer, R2DTexture **npcTextures,
                          const Camera *camera, TTF_Font *font,
                          float bedX, float bedY)
{
    if (!npcActive) return;
    int bed = 0;
    for (int i = 0; i < NPC_COUNT && i < NPC_STATE_CAP; i++) {
        if (!npcState[i].infirm || npcTextures[i] == NULL) continue;
        float bx = bedX + (float)bed * 56.0f;
        int fr = def_frame(i);
        float sc = def_scale(i) * 0.9f;
        SDL_FRect src = { 0, 0, (float)fr, (float)fr };   // idle ilk kare
        SDL_FRect dst = {
            (bx - fr * sc / 2.0f - camera->x) * camera->zoom,
            (bedY - fr * sc + 10.0f - camera->y) * camera->zoom,
            fr * sc * camera->zoom, fr * sc * camera->zoom,
        };
        r2d_texture_set_alpha_mod(npcTextures[i], 150);   // baygın soluk
        r2d_draw_texture(renderer, npcTextures[i], &src, &dst);
        r2d_texture_set_alpha_mod(npcTextures[i], 255);
        if (font != NULL) {
            char lbl[48];
            SDL_snprintf(lbl, sizeof(lbl), "%s (yatıyor)", NPCS[i].name);
            SDL_Color dim = { 200, 190, 200, 255 };
            draw_text(renderer, font, lbl,
                      (bx - 30.0f - camera->x) * camera->zoom,
                      (bedY + 14.0f - camera->y) * camera->zoom, dim);
        }
        bed++;
        if (bed >= 4) break;   // 4 yatak — fazlası kuyrukta (görünmez ama yatar)
    }
}

// Takip görevi (Suikastçi P1): NPC'nin şu anki durak indeksi (rapor bitmask'i)
int npc_current_stop_index(int i, float hours)
{
    if (i < 0 || i >= NPC_COUNT) return -1;
    int best = NPCS[i].stopCount - 1;
    for (int s2 = 0; s2 < NPCS[i].stopCount; s2++)
        if (hours >= NPCS[i].schedule[s2].hour) best = s2;
    return best;
}
