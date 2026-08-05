#include <SDL3/SDL.h>
#include "fx.h"

// KATMAN MASKESİ (dev tuşu Ctrl+F). 0 gelirse hepsi açık sayılır: çağıran
// alanı hiç yazmazsa efekt sessizce kaybolmasın.
static int s_fxLayers = FX_L_ALL;
void fx_set_layers(int mask) { s_fxLayers = (mask == 0) ? FX_L_ALL : mask; }

static inline bool fx_layer_on(WeaponFxType t)
{
    switch (t) {
        case WEAPON_FX_SMOKE: return (s_fxLayers & FX_L_SMOKE) != 0;
        case WEAPON_FX_SPARK: return (s_fxLayers & FX_L_SPARK) != 0;
        case WEAPON_FX_STEAM: return (s_fxLayers & FX_L_STEAM) != 0;
        case WEAPON_FX_EMBER: return (s_fxLayers & FX_L_EMBER) != 0;
        case WEAPON_FX_LICK:  return (s_fxLayers & FX_L_LICK) != 0;
        case WEAPON_FX_SOOT:  return (s_fxLayers & FX_L_SOOT) != 0;
        case WEAPON_FX_FLAME_DEEP: return (s_fxLayers & FX_L_DEEP) != 0;
        case WEAPON_FX_FLAME_DARK: return (s_fxLayers & FX_L_DARK) != 0;
        case WEAPON_FX_FLAME: return (s_fxLayers & FX_L_FLAME) != 0;
        default: return true;   // silah efektleri maskeden bağımsız
    }
}

// Deterministik olmayan ama TAŞINABİLİR rastgele: SDL_rand kullanmıyoruz
// (sürüm farkı), kendi 32-bit LCG'si — üç arka uçta da aynı davranır.
// KUVVET ALANI ve ÖLÜM KANCASI — motor durumu, kare başına çağırandan gelir
static float s_windX = 0.0f, s_windY = 0.0f;
void fx_set_wind(float vx, float vy) { s_windX = vx; s_windY = vy; }
static FxDeathHook s_deathHook = NULL;

// ── ZAMANLI BÜKÜLME HAVUZU (bkz. fx.h). Motor durumu: skill ateşler ve unutur.
typedef struct {
    float x, y, radius, strength, life, maxLife;
    int style;
    bool active;
} FxBurst;
static FxBurst s_bursts[FX_MAX_BURST];

void fx_distort_burst(float wx, float wy, int style, float radius,
                      float strength, float durationSec)
{
    for (int i = 0; i < FX_MAX_BURST; i++) {
        if (s_bursts[i].active) continue;
        s_bursts[i] = (FxBurst){ .x = wx, .y = wy, .radius = radius,
                                 .strength = strength, .life = durationSec,
                                 .maxLife = durationSec, .style = style,
                                 .active = true };
        return;
    }
    // Havuz dolu → en eskisini EZ (yeni olay daha alakalı: patlama olurken
    // bir önceki halkanın kuyruğunu korumanın değeri yok)
    int old = 0;
    for (int i = 1; i < FX_MAX_BURST; i++)
        if (s_bursts[i].life < s_bursts[old].life) old = i;
    s_bursts[old] = (FxBurst){ .x = wx, .y = wy, .radius = radius,
                               .strength = strength, .life = durationSec,
                               .maxLife = durationSec, .style = style,
                               .active = true };
}

void fx_distort_update(float dt)
{
    for (int i = 0; i < FX_MAX_BURST; i++) {
        if (!s_bursts[i].active) continue;
        s_bursts[i].life -= dt;
        if (s_bursts[i].life <= 0.0f) s_bursts[i].active = false;
    }
}

int fx_distort_collect(R2DDistort *out, int cap,
                       float camX, float camY, float zoom)
{
    int n = 0;
    for (int i = 0; i < FX_MAX_BURST && n < cap; i++) {
        if (!s_bursts[i].active) continue;
        float u = 1.0f - s_bursts[i].life / s_bursts[i].maxLife;   // 0→1 ilerleme
        float amp = s_bursts[i].strength;
        // ŞOK'ta genlik sabit kalır (fazın kendisi sönümü taşıyor — shader
        // halkanın enerjisini (1-phase) ile düşürüyor); diğerlerinde ömürle söner
        if (s_bursts[i].style != R2D_DIST_SHOCK)
            amp *= (1.0f - u) * (1.0f - u);
        out[n].x = (s_bursts[i].x - camX) * zoom;
        out[n].y = (s_bursts[i].y - camY) * zoom;
        out[n].radius = s_bursts[i].radius * zoom;
        out[n].strength = amp * zoom;
        out[n].style = (float)s_bursts[i].style;
        out[n].phase = u;            // SHOCK: halkanın yarıçap oranı
        out[n].freq = 1.0f;
        out[n].aniso = 1.0f;
        n++;
    }
    return n;
}
void fx_set_death_hook(FxDeathHook fn) { s_deathHook = fn; }

// ── PALET VARYANTI (skill başına ton kimliği). Aynı tip (alev, buz, taş) farklı
// büyülerde farklı TON taşımalı: ateş topu altın-turuncu, ateş aslanı kızıl,
// arcane ateş mavi-beyaz. fx_spawn imzasına renk eklemek 100+ çağrı yerini
// dokunmak demekti; onun yerine "şu andaki varyant" global'i: skill spawn'dan
// önce kurar, parçacık doğduğu anda varyantı içine alır.
static int s_palVar = 0, s_palAlt = 0;
static float s_palMix = 0.0f;

void fx_set_pal_variant(int v)
{
    s_palVar = s_palAlt = (v < 0 || v > 2) ? 0 : v;
    s_palMix = 0.0f;
}

// TON KARIŞIMI. Tek varyant tek rampa demek; bir alevin İÇİNDE ton çeşitliliği
// olmuyor. Karışım parçacık BAŞINA seçildiği için aynı patlamada altın ve kızıl
// diller yan yana duruyor (Hakan: "arasına biraz daha fazla kırmızılık").
// Zaman içinde değişen bir gradyan değil, madde içinde dağılmış bir DOKU.
void fx_set_pal_mix(int base, int alt, float altChance)
{
    s_palVar = (base < 0 || base > 2) ? 0 : base;
    s_palAlt = (alt  < 0 || alt  > 2) ? 0 : alt;
    s_palMix = (altChance < 0.0f) ? 0.0f : (altChance > 1.0f ? 1.0f : altChance);
}

static unsigned s_fxRng = 0x1234567u;
static inline float fx_rnd(void)
{
    s_fxRng = s_fxRng * 1664525u + 1013904223u;
    return (float)((s_fxRng >> 8) & 0xFFFFu) / 65535.0f;
}
static inline float fx_sym(void) { return fx_rnd() * 2.0f - 1.0f; }

// Kaç partikül doğacak? rate*dt kesirli çıkar; kesir OLASILIK olarak
// harcanıyor, yoksa düşük dt'de hiç doğmaz (60 fps'de rate<60 ölür).
static int fx_count(float ratePerSec, float dt)
{
    float n = ratePerSec * dt;
    int k = (int)n;
    if (fx_rnd() < (n - (float)k)) k++;
    return k;
}

float fx_breath(FireKind kind, unsigned seed, float timeSec)
{
    // Kaynak karakterine göre nefes: kor/kandil SAKİN (kanon LORE §1.9 —
    // "seni koruyan şey durur"), odun ateşi KAOTİK. Üç sinüsün toplamı;
    // frekanslar ORANSIZ (2.3/5.7/11.3) ki desen tekrar etmesin.
    float sp = (kind == FIRE_CANDLE) ? 0.55f
             : (kind == FIRE_HEARTH) ? 1.35f
             : (kind == FIRE_FORGE)  ? 0.85f : 0.7f;
    float amp = (kind == FIRE_CANDLE) ? 0.16f
              : (kind == FIRE_HEARTH) ? 0.38f
              : (kind == FIRE_FORGE)  ? 0.22f : 0.25f;
    float ph = (float)(seed & 1023u) * 0.0061f;
    float t = timeSec * sp + ph;
    float w = 0.55f * SDL_sinf(t * 2.3f)
            + 0.30f * SDL_sinf(t * 5.7f + 1.7f)
            + 0.15f * SDL_sinf(t * 11.3f + 0.4f);
    float v = 1.0f - amp + amp * w;
    return (v < 0.05f) ? 0.05f : (v > 1.6f ? 1.6f : v);
}

void fx_emit_fire(ParticleSystem *fx, float x, float y, FireKind kind,
                  float strength, float dt)
{
    // ATEŞ TÜRÜNÜN TONU: ocak sıkı ve beyaza yakın olduğu için kanonik altın
    // aile; kazan alev değil buhar üretiyor ve tonu soğuk aileden alıyor.
    // Böylece aynı yayıcı kodu farklı ateşlerde farklı RENK okuyor.
    // Ateş türleri de iki tonlu: ocak sıkı ve beyaza yakın olduğu için kızıl
    // payı düşük, kamp ateşi kaotik olduğu için yüksek. Kazanç motor katmanında
    // olduğu için büyüde ne kazandıysak kamp ateşinde de kazanıyoruz.
    if (kind == FIRE_CAULDRON)      fx_set_pal_mix(1, 0, 0.20f);
    else if (kind == FIRE_FORGE)    fx_set_pal_mix(0, 2, 0.18f);
    else if (kind == FIRE_CANDLE)   fx_set_pal_mix(0, 2, 0.26f);
    else                            fx_set_pal_mix(0, 2, 0.40f);   // kamp ateşi
    if (fx == NULL || strength <= 0.01f || dt <= 0.0f) return;
    // Kaynak KARAKTERİ tek tabloda: su fazının "mizaç" dersinin aynısı —
    // ocak sakin yanar, kamp ateşi oynar, kandil kıpırdar. Her satır bir
    // kaynağın kimliği; ayarlar dağılmıyor.
    struct FireProfile {
        float flameRate, smokeRate, sparkRate, steamRate, emberRate;
        float flameLife, flameSize, rise, spread;
        Uint8 fr, fg, fb;          // alev doğum rengi
    };
    static const struct FireProfile PROF[FIRE_KIND_COUNT] = {
        // kamp ateşi: geniş, kaotik, bol duman, ara sıra kıvılcım
        { 46.0f, 12.0f, 5.0f, 0.0f, 5.0f,  0.62f, 15.0f, -54.0f, 15.0f, 255, 190,  70 },
        // demirci ocağı: sıkı ve beyaza yakın, ÇOK kıvılcım, az duman
        { 40.0f, 3.0f, 16.0f, 0.0f, 9.0f,  0.42f, 11.0f, -40.0f,  8.0f, 255, 225, 165 },
        // kandil: küçük, sakin, dumanı yok denecek kadar
        { 15.0f, 1.2f,  0.6f, 0.0f, 1.0f,  0.40f,  7.0f, -26.0f,  4.0f, 255, 200, 110 },
        // kazan: alev YOK, buhar
        {  0.0f, 0.0f,  0.0f,14.0f, 0.0f,  0.0f,   0.0f,   0.0f,  9.0f, 255, 255, 255 },
    };
    const struct FireProfile *P = &PROF[(int)kind];

    // ALEV — ADD, yukarı, ömürle kızıllaşıp küçülür (fx_render'daki mevcut
    // FLAME renk geçişi kullanılıyor: sarı doğar, kızıl ölür)
    if (fx_layer_on(WEAPON_FX_FLAME)) {
        int n = fx_count(P->flameRate * strength, dt);
        for (int i = 0; i < n; i++)
            fx_spawn(fx, x + fx_sym() * P->spread, y + fx_sym() * 3.0f,
                     fx_sym() * P->spread * 0.7f, P->rise * (0.7f + 0.5f * fx_rnd()),
                     P->flameLife * (0.7f + 0.6f * fx_rnd()),
                     P->flameSize * (0.7f + 0.6f * fx_rnd()),
                     P->fr, P->fg, P->fb, WEAPON_FX_FLAME);
    }
    // KIZIL ZARF — ALPHA, alevin DIŞ GÖVDESİ. Kamp ateşi de aynı kazancı alıyor:
    // toplamalı alev tek başına sarıya doyuyor, zarf kızılı görünür kılıyor.
    // Kazan alev üretmediği için (flameRate 0) onda kendiliğinden kapalı.
    if (P->flameRate > 0.0f && fx_layer_on(WEAPON_FX_FLAME_DEEP)) {
        int n = fx_count(P->flameRate * strength * 0.55f, dt);
        for (int i = 0; i < n; i++)
            fx_spawn(fx, x + fx_sym() * P->spread * 1.15f, y + fx_sym() * 3.0f,
                     fx_sym() * P->spread * 0.4f, P->rise * 0.72f,
                     P->flameLife * (0.55f + 0.45f * fx_rnd()),
                     P->flameSize * (1.7f + 0.9f * fx_rnd()),
                     226, 84, 48, WEAPON_FX_FLAME_DEEP);
    }
    // DUMAN — ALPHA, alevin ÜSTÜNDEN doğar (alevin içinde duman olmaz),
    // yavaş yükselir, yana savrulur, büyüyerek söner
    if (fx_layer_on(WEAPON_FX_SMOKE)) {
        int n = fx_count(P->smokeRate * strength, dt);
        for (int i = 0; i < n; i++)
            fx_spawn(fx, x + fx_sym() * P->spread * 0.8f, y - 16.0f - fx_rnd() * 8.0f,
                     fx_sym() * 9.0f + 5.0f, -16.0f - fx_rnd() * 10.0f,
                     2.2f + fx_rnd() * 1.5f, 12.0f + fx_rnd() * 8.0f,
                     62, 58, 55, WEAPON_FX_SMOKE);
    }
    // KIVILCIM — ADD, küçük ve hızlı; yukarı fırlar, sürtünme yavaşlatır
    if (fx_layer_on(WEAPON_FX_SPARK)) {
        int n = fx_count(P->sparkRate * strength, dt);
        for (int i = 0; i < n; i++)
            fx_spawn(fx, x + fx_sym() * P->spread * 0.5f, y - 4.0f,
                     fx_sym() * 34.0f, -70.0f - fx_rnd() * 60.0f,
                     0.55f + fx_rnd() * 0.5f, 3.0f + fx_rnd() * 2.0f,
                     255, 236, 190, WEAPON_FX_SPARK);
    }
    // KOR — ADD, ateşin DİBİNDE, yerinde yanar ve yavaş söner. Uzun ömür
    // (2-4 sn) ki sürekli bir kor yatağı görünsün; alevin altında kalıyor
    // çünkü alev yukarı gidiyor, kor gitmiyor.
    if (fx_layer_on(WEAPON_FX_EMBER)) {
        int n = fx_count(P->emberRate * strength, dt);
        for (int i = 0; i < n; i++)
            fx_spawn(fx, x + fx_sym() * P->spread * 1.1f, y + 2.0f + fx_sym() * 3.0f,
                     fx_sym() * 4.0f, fx_sym() * 3.0f,
                     2.0f + fx_rnd() * 2.0f, 5.0f + fx_rnd() * 4.0f,
                     255, 150, 60, WEAPON_FX_EMBER);
    }
    // BUHAR — ALPHA, beyaza yakın, hızlı büyür
    if (fx_layer_on(WEAPON_FX_STEAM)) {
        int n = fx_count(P->steamRate * strength, dt);
        for (int i = 0; i < n; i++)
            fx_spawn(fx, x + fx_sym() * P->spread, y - 6.0f,
                     fx_sym() * 7.0f, -22.0f - fx_rnd() * 12.0f,
                     1.2f + fx_rnd() * 0.9f, 8.0f + fx_rnd() * 6.0f,
                     226, 230, 234, WEAPON_FX_STEAM);
    }
}

// DEV TEŞHİS: "efekt görünmüyor" derken partikül HİÇ doğmuyor mu, yoksa
// doğuyor da görünmüyor mu? İkisi bambaşka arama yeri. Su fazında bu ayrımı
// yapamadığımız için turlar harcandı.
int fx_count_active(const ParticleSystem *fx, WeaponFxType t)
{
    int n = 0;
    for (int i = 0; i < MAX_PARTICLES; i++)
        if (fx->items[i].active && fx->items[i].fxType == t) n++;
    return n;
}

// ── PARÇACIK BİÇİM ATLASI (kodda basılır, dış asset YOK).
// NEDEN: her parçacık aynı yumuşak yuvarlak lekeden çiziliyordu; 300 tane de
// olsa göz "nokta bulutu" görüyor çünkü BİÇİM arıyor. Buz köşeli kırık olmalı,
// kıvılcım ince çizik, duman yumuşak öbek, enerji yıldız. Beş hücre × 32 px,
// tek 160×32 doku. Kodda basılması bilinçli: dil tutarlı kalıyor ve yeni biçim
// eklemek dosya indirmek değil formül yazmak oluyor.
#define FXS_CELL 32
#define FXS_COUNT 12
#define FXS_PUFF   0   // yumuşak öbek — duman/buhar
#define FXS_SHARD  1   // köşeli eşkenar dörtgen — buz/kaya
#define FXS_STREAK 2   // ince yatay çizik — kıvılcım (hızla uzatılır)
#define FXS_STAR   3   // dört kollu yıldız — enerji/yıldırım
#define FXS_DOT    4   // sert küçük nokta + hafif hale — kor
#define FXS_TEAR   5   // damla: dibi geniş, ucu ince — ALEV DİLİ
#define FXS_FLAKE  6   // düzensiz pul — is/kül
// ALEV DİLİ ÜÇ VARYANT. Tek kalıp kullanıldığında 120 dil aynı silueti
// tekrarlıyor ve göz "damga basılmış üçgen" diyor — Hakan dört tur bunu
// bildirdi ("üçgenlik olayını doğallaştır"). Doğallık ÇEŞİTLİLİKTEN geliyor:
// her varyantın omurgası başka yöne yatık, kenar düzensizliği başka frekansta.
#define FXS_TEAR_B 7   // sağa yatık dil
#define FXS_TEAR_C 8   // sola yatık, daha düzensiz dil
// BUZ KRİSTALİ ÜÇ VARYANT. Eskiden buz KIYMIK hücresini kullanıyordu; kıymık
// |dx|+|dy| ölçüsüyle çizilmiş bir EŞKENAR DÖRTGEN ve büyük boyda, dönerken,
// üst üste binince "kare kare, sahte" okunuyor (Hakan). Gerçek buz kristali
// elmas değil İNCE PRİZMA: iki ucu sivri, ortası en geniş, parlak bir omurgası
// var. Kıymık kaya/moloz için KALIYOR — taş gerçekten köşelidir.
#define FXS_CRY_A  9
#define FXS_CRY_B 10
#define FXS_CRY_C 11

static R2DTexture *s_shapeTex = NULL;

// ── YUMUŞAK KADEME. Kapsama değerini 6 kademeye kuantalar.
// HATA GEÇMİŞİ: biçimler 2-3 sert kademeyle yazılmıştı ve her parçacık DÜZ
// RENKLİ bir leke oluyordu; sonuç Hakan'ın tarifiyle "kağıt gibi duruyor" —
// hacim yok, kenarlar kesik kağıt. Pixel-art disiplini "bant" ister, "iki bant"
// istemez: 6 kademe hem bant hem HACİM veriyor. Kapsama fonksiyonları da
// doğrusal değil KARESEL düşüyor (merkez dolgun, kenar hızlı incelir) — hacim
// hissi buradan geliyor.
static inline float fx_soft(float cov)
{
    if (cov <= 0.0f) return 0.0f;
    float q = SDL_floorf(cov * 6.0f + 0.5f) / 6.0f;
    return (q > 1.0f) ? 1.0f : q;
}

// ── ALEV DİLİ (parametrik). HATA GEÇMİŞİ (dört tur):
//  1) genişlik y ile DOĞRUSAL daralıyordu → keskin ÜÇGEN HUNİ.
//  2) profil yaprağa çevrildi ama TEK kalıp kaldı → 120 kopya aynı siluet,
//     hâlâ "üçgen damga" okunuyordu.
// ÇÖZÜM: (a) yaprak profili — pow(t,0.55) uca sivrilik, (1-0.34·t³) dibi
// toplar; (b) OMURGA eğik — dil dümdüz yukarı çıkmaz, yana yatar; (c) kenar
// düzensizliği DİKEY frekanstan — yatay hash kullanılsa komşu satırlar kopar
// (su fazının 2. kuralı); (d) üç varyant, parçacık başına biri seçilir.
static float fx_tongue(float dx, float dy, float lean, float wfq, float wamp,
                       float seed)
{
    float t = (dy + 1.0f) * 0.5f;                  // 0 uç → 1 dip
    if (t <= 0.0f || t >= 1.0f) return 0.0f;
    float spine = lean * (1.0f - t) * (1.0f - t);   // uca doğru yatar
    float w = SDL_powf(t, 0.55f) * (1.0f - 0.34f * t * t * t);
    w *= (1.0f + wamp * SDL_sinf(t * wfq + seed));
    if (w < 0.02f) return 0.0f;
    float d = SDL_fabsf(dx - spine) / (w * 0.95f);
    if (d >= 1.0f) return 0.0f;
    // Karesel düşüş: merkez dolgun, kenar yumuşak → kağıt değil HACİM
    float core = (1.0f - d * d) * (0.34f + t * 0.66f);
    return fx_soft(core);
}

// ── BUZ KRİSTALİ: iki ucu sivri ince prizma + parlak omurga. Üç varyant
// (uzun-ince, kısa-geniş, kırık uçlu) ve hafif yatıklık; tek kalıp üst üste
// binince kafes gibi duruyor.
static float fx_crystal(float dx, float dy, int var)
{
    static const float ASP[3]  = { 0.30f,  0.40f, 0.24f };
    static const float LEAN[3] = { 0.00f, -0.18f, 0.22f };
    static const float PW[3]   = { 0.55f,  0.72f, 0.48f };
    float t = (dy + 1.0f) * 0.5f;
    if (t <= 0.0f || t >= 1.0f) return 0.0f;
    if (var == 2 && t < 0.18f) return 0.0f;          // kırık (düz kesik) uç
    float hw = ASP[var] * SDL_powf(SDL_sinf(3.14159f * t), PW[var]);
    if (hw < 0.02f) return 0.0f;
    float cx = LEAN[var] * (t - 0.5f) * 2.0f * 0.35f;
    float d = SDL_fabsf(dx - cx);
    if (d > hw) return 0.0f;
    // Omurga parlak, kenara doğru KARESEL düşüş (üç sert kademe "cam kırığı
    // kağıdı" gibi duruyordu)
    float k = d / hw;
    return fx_soft(1.0f - k * k * 0.92f);
}

static void fx_bake_shapes(R2D *renderer)
{
    static Uint8 px[FXS_CELL * FXS_COUNT * FXS_CELL * 4];
    const int W = FXS_CELL * FXS_COUNT, H = FXS_CELL;
    const float C = (float)FXS_CELL * 0.5f - 0.5f;
    for (int cell = 0; cell < FXS_COUNT; cell++) {
        for (int y = 0; y < H; y++) {
            for (int x = 0; x < FXS_CELL; x++) {
                float dx = ((float)x - C) / C, dy = ((float)y - C) / C;
                float r = SDL_sqrtf(dx * dx + dy * dy);
                float a = 0.0f;
                switch (cell) {
                    case FXS_PUFF:
                        // Yumuşak radyal — kenarı belirsiz (duman kütlesi).
                        // (1-r)² merkeze çok toplanıyordu; 1-r^1.7 daha dolgun
                        // bir gövde ve daha yumuşak omuz veriyor.
                        a = (r < 1.0f)
                          ? fx_soft(1.0f - SDL_powf(r, 1.7f)) : 0.0f;
                        break;
                    case FXS_SHARD: {
                        // SÜPERELİPS: |dx|^1.55 + |dy|^1.55. Üs 1.0 eşkenar
                        // dörtgen (dik kenar, keskin köşe — Hakan: "köşe hatları
                        // çizgisel, daha yuvarlak olmalı"), 2.0 tam elips.
                        // 1.55 kırık taşın köşesini yuvarlatıyor ama hâlâ
                        // köşeli okunuyor.
                        float d = SDL_powf(SDL_fabsf(dx), 1.55f)
                                + SDL_powf(SDL_fabsf(dy) * 0.78f, 1.55f);
                        float R = SDL_powf(0.94f, 1.55f);
                        a = (d < R) ? fx_soft(1.0f - (d / R) * (d / R) * 0.85f)
                                    : 0.0f;
                        break;
                    }
                    case FXS_STREAK: {
                        // İnce yatay çizik: x boyunca uzun, y'de 3-4 px.
                        float w2 = SDL_fabsf(dy) * 4.2f;
                        a = (w2 < 1.0f && SDL_fabsf(dx) < 0.96f)
                          ? (1.0f - w2) * (1.0f - SDL_fabsf(dx) * 0.55f) : 0.0f;
                        if (a > 0.0f) a = fx_soft(a * 1.35f);
                        break;
                    }
                    case FXS_STAR: {
                        // Dört kollu yıldız: eksenlere yakınlık + parlak çekirdek
                        // min() keskin bir çapraz köşe üretiyordu; yumuşak
                        // minimum (kuvvet ortalaması) kolları yuvarlatıyor.
                        float ax = SDL_fabsf(dx), ay = SDL_fabsf(dy);
                        float arm = SDL_powf(SDL_powf(ax, -3.0f)
                                           + SDL_powf(ay + 0.001f, -3.0f),
                                             -1.0f / 3.0f) * 6.0f;
                        float core = (r < 0.24f) ? 1.0f : 0.0f;
                        float ray = (arm < 1.0f && r < 1.0f)
                                  ? (1.0f - arm * arm) * (1.0f - r) * 1.6f : 0.0f;
                        a = fx_soft(SDL_max(core, SDL_min(ray, 1.0f)));
                        break;
                    }
                    case FXS_DOT:
                        // Kor: dolgun çekirdek + soluk hale (iki sert kademe
                        // "delikli kağıt" gibi duruyordu)
                        a = (r < 0.90f)
                          ? fx_soft(1.0f - SDL_powf(r / 0.90f, 2.6f) * 0.80f)
                          : 0.0f;
                        break;
                    case FXS_TEAR:   a = fx_tongue(dx, dy,  0.00f,  9.0f, 0.10f, 0.0f); break;
                    case FXS_TEAR_B: a = fx_tongue(dx, dy,  0.26f,  7.0f, 0.16f, 1.9f); break;
                    case FXS_TEAR_C: a = fx_tongue(dx, dy, -0.34f, 11.0f, 0.20f, 3.4f); break;
                    case FXS_CRY_A:  a = fx_crystal(dx, dy, 0); break;
                    case FXS_CRY_B:  a = fx_crystal(dx, dy, 1); break;
                    case FXS_CRY_C:  a = fx_crystal(dx, dy, 2); break;
                    default: {
                        // PUL (is/kül).
                        // HATA GEÇMİŞİ: |dx|+|dy| ölçüsüyle yazılmıştı. O ölçü
                        // matematiksel olarak EŞKENAR DÖRTGEN çiziyor — sert
                        // kenarlı, eksene hizalı bir elmas. Her ateş patlaması
                        // 34 tanesini fırlattığı için ekranda "üçgen parçacıklar"
                        // okunuyordu (Hakan iki tur üst üste bildirdi; ilk turda
                        // yanlış yere, alev diline baktım).
                        // DOĞRUSU: yarıçapı AÇIYA göre modüle et. Düz kenar ve
                        // köşe hiç oluşmaz, düzensizlik kalır. Aynı numara
                        // decal.c'de yer izlerinin kenarında zaten kanıtlıydı —
                        // kusursuz daire yapay durur, L1 ölçüsü ise geometrik.
                        float dyf = dy * 1.22f;              // pul yassı
                        float rr2 = SDL_sqrtf(dx * dx + dyf * dyf);
                        float ang = SDL_atan2f(dyf, dx);
                        float R = 0.80f + 0.13f * SDL_sinf(ang * 3.0f + 0.6f)
                                        + 0.08f * SDL_sinf(ang * 5.0f + 2.0f);
                        a = (rr2 < R)
                          ? fx_soft(1.0f - (rr2 / R) * (rr2 / R) * 0.88f)
                          : 0.0f;
                        break;
                    }
                }
                Uint8 *o = &px[((y * W) + cell * FXS_CELL + x) * 4];
                o[0] = 255; o[1] = 255; o[2] = 255;
                o[3] = (Uint8)(a * 255.0f + 0.5f);
            }
        }
    }
    s_shapeTex = r2d_texture_from_pixels(renderer, px, W, H);
}

// Hangi tip hangi biçimi kullanır
static int fx_type_shape(WeaponFxType t)
{
    switch (t) {
        case WEAPON_FX_SMOKE:
        case WEAPON_FX_STEAM: return FXS_PUFF;
        case WEAPON_FX_ICE:
        case WEAPON_FX_ROCK:  return FXS_SHARD;   // buz kırığı / kaya parçası
        case WEAPON_FX_SPARK: return FXS_STREAK;
        case WEAPON_FX_EMBER: return FXS_DOT;
        case WEAPON_FX_PINK:  return FXS_STAR;
        case WEAPON_FX_LICK:  return FXS_TEAR;
        case WEAPON_FX_SOOT:  return FXS_FLAKE;
        case WEAPON_FX_FLAME_DEEP: return FXS_PUFF;   // zarf: yumuşak kütle
        case WEAPON_FX_FLAME_DARK: return FXS_PUFF;   // siluet: yumuşak kütle
        case WEAPON_FX_FLAME: return FXS_PUFF;    // alev yumuşak kütle
        default: return FXS_PUFF;
    }
}

// Bu tip HIZ YÖNÜNDE uzar mı? Hızlı giden yuvarlak nokta YAVAŞ okunur;
// hareket yönünde uzayınca enerji doğar (kıvılcım, buz kırığı, kaya).
static bool fx_type_stretch(WeaponFxType t)
{
    return t == WEAPON_FX_SPARK || t == WEAPON_FX_ICE || t == WEAPON_FX_ROCK;
}

// ── KUANTALANMIŞ PALETLER (4 kademe). Sürekli renk geçişi pixel-art'ta
// yumuşuyor ve "airbrush" gibi duruyor; 4 sert kademe hem daha canlı hem
// daha çizilmiş okunuyor. Ton kaydırma kuralı: sıcaklar sarıdan kızıla
// DÜŞERKEN doygunluk artar, soğuklar beyazdan maviye iner.
static void fx_palette(WeaponFxType t, int palVar, float life01, int seed,
                       Uint8 *r, Uint8 *g, Uint8 *b)
{
    // [satır][varyant][kademe][rgb]. Kademe sırası: doğum → ölüm.
    // VARYANT ANLAMI HER SATIRDA AYNI OLMAK ZORUNDA: 0 = maddenin kanonik tonu,
    // 1 = SOĞUK aile (mavi/gri/mor), 2 = SICAK aile (kızıl/kahve/kum).
    // İlk yazımda satırlar karışıktı (FLAME'de 1=kızıl ama SPARK'ta 1=mavi) ve
    // sonuç ters çıkıyordu: YILDIRIM sıcak kahve duman, TOPRAK soğuk gri duman
    // alıyordu. Olay başına tek indeks kullanmanın bedeli bu tutarlılık.
    static const Uint8 PAL[9][3][4][3] = {
        // ALTIN rampa. İKİ TUR ISINDI:
        //  1) kırmızı yalnız SON (ölüm) kademesindeydi, orada alfa düşük olduğu
        //     için hiç görünmüyordu → alev "biraz sarı" duruyordu.
        //  2) DOĞUM kademesi neredeyse BEYAZDI (255,246,206). Kısa ömürlü
        //     parçacık vaktinin çoğunu o kademede geçiriyor ve toplamalı (ADD)
        //     üst üste binme de beyaza doyuruyor → "çok beyaz gibi gözüküyor"
        //     (Hakan). Alev sıcak bir yapı: doğum kademesi de SICAK olmalı,
        //     parlaklık beyazlıktan değil ALFA'dan gelmeli.
        // Şimdi dört kademe boyunca hep sıcak: altın → turuncu → kor → koyu kor.
        // ── ATEŞ RAMPALARI KARA CİSİM IŞIMASI EĞRİSİNDEN (2026-08-05).
        // Hakan: "renkler hala daha sıcak olması gerekiyor özellikle sarı/beyaz
        // olan kısımlar. İç alev rengi de biraz garip duruyor. Sen internetten
        // alev renklerini araştır, deneme yanılma yapmayalım."
        // ARAŞTIRMA SONUCU (Tanner Helland kara cisim yaklaşımı; vendian.org ve
        // firgelliauto çapalarıyla doğrulandı — 2000K ve 3000K'da birebir):
        //     1100K (255, 77,  0)      2100K (255,142, 27)
        //     1500K (255,108,  0)      2500K (255,159, 70)
        //     1900K (255,132,  0)      2900K (255,174,103)
        // KÖK SEBEP BULUNDU: eğride MAVİ kanal 2000K'nın ALTINDA SIFIR ve
        // 3000K'da bile ancak ~110. Benim elle uydurduğum "sıcak" renklerde
        // B=48..132 vardı, yani eğrinin DIŞINDAYDIM — bu yüzden hem sarı beyaza
        // kaçıyordu hem iç alev rengi mat/pembemsi duruyordu.
        // YAN KAZANÇ: B≈0 olduğu için toplamalı (ADD) üst üste binme artık
        // beyaza değil SARI-TURUNCUYA doyuyor. Yoğun alevin beyazlaması
        // matematiksel olarak ortadan kalktı.
        // Kademeler = sıcaklık düşüşü (doğum sıcak → ölüm soğuk) + sönüm çarpanı.
        // ELLE DEĞER GİRMEK YASAK: yeni ateş tonu gerekirse sıcaklık seçilip
        // eğriden hesaplanır (formül bu yorumda; script gerekmez).
        // En sıcak kademe 2900K → 2700K. GEREKÇE ölçüldü: 2900K'nın luması 0.746
        // ve bloom eşiği 0.74 — alfa çarpanıyla tek parçacık eşiği zaten
        // geçmiyor, yani 2900K'nın tek kazancı iki parçacık üst üste binince
        // maviyi 206'ya çıkarıp çekirdeği BEYAZLATMAKTI. 2700K'da aynı üst üste
        // binme (255,255,174) veriyor: parlak SARI çekirdek, beyaz değil.
        // KURAL: bloom artık YOĞUNLUKTAN gelir, tek parçacıktan gelmez —
        // fiziksel olarak da doğrusu bu (tek kor hale yapmaz, alev kütlesi yapar).
        /*FLAME*/ {{{255,167, 87},{255,155, 61},{255,132,  0},{189, 72,  0}},   // 2700→1350K altın
                   {{240,250,255},{182,214,250},{124,158,238},{ 62, 78,158}},   // mavi-beyaz (arcane: eğri DIŞI, bilinçli)
                   {{255,155, 61},{255,132,  0},{255,108,  0},{173, 56,  0}}},  // 2400→1150K kızıl
        /*ICE  */ {{{255,255,255},{200,240,255},{120,190,255},{ 60,110,200}},   // camgöbeği
                   {{230,240,255},{160,190,250},{ 92,120,220},{ 40, 56,150}},   // derin mavi
                   {{245,255,250},{190,240,225},{130,205,190},{ 60,130,120}}},  // nane
        /*SPARK*/ {{{255,255,240},{255,230,170},{255,150, 60},{180, 60, 20}},   // çelik altını
                   {{255,255,255},{225,240,255},{170,205,255},{ 90,130,210}},   // yıldırım mavisi
                   {{255,244,206},{255,196, 96},{240,120, 34},{140, 40, 14}}},  // ateş turuncusu
        /*EMBER*/ {{{255,115,  0},{255,101,  0},{209, 71,  0},{128, 34,  0}},   // 1600→1000K kor
                   {{210,225,255},{150,180,240},{ 90,120,200},{ 50, 66,130}},
                   {{255,180,160},{240,100, 90},{170, 40, 40},{ 90, 20, 20}}},
        /*SMOKE*/ {{{120,116,112},{ 96, 92, 90},{ 72, 70, 70},{ 52, 52, 54}},
                   {{116,124,132},{ 92,100,110},{ 70, 78, 88},{ 50, 56, 66}},
                   {{132,112,100},{104, 86, 76},{ 78, 64, 58},{ 54, 44, 40}}},
        /*STEAM*/ {{{255,255,255},{232,240,248},{206,222,238},{180,200,220}},
                   {{250,246,255},{230,226,246},{204,198,230},{176,168,210}},
                   {{255,255,250},{238,246,232},{212,230,206},{184,208,178}}},
        /*ROCK */ {{{158,142,116},{126,110, 88},{ 98, 84, 66},{ 68, 58, 46}},   // toprak
                   {{170,170,172},{136,136,140},{104,104,110},{ 72, 72, 80}},   // gri taş
                   {{176,132,104},{144, 98, 76},{112, 72, 56},{ 78, 48, 38}}},  // kızıl kil
        // KIZIL ZARF (ALPHA). Değerler bilerek DÜŞÜK: alpha katmanı toplamıyor,
        // parlak yazılırsa "kırmızı boya" gibi duruyor. Kademe sırası ateşin
        // dışının soğuma yönü: turuncu-kızıl → kızıl → koyu kızıl → is kızılı.
        /*DEEP */ {{{235,111,  0},{217, 89,  0},{173, 61,  0},{107, 31,  0}},   // 1700→1050K zarf
                   {{176,120,168},{140, 62,128},{ 96, 34, 92},{ 54, 16, 52}},   // soğuk (mor ateş)
                   {{244, 96, 48},{214, 40, 26},{158, 18, 14},{ 88,  8,  8}}},  // sıcak (kor kızılı)
        // KOYU SİLUET: sarının ÜSTÜNE çiziliyor, o yüzden değerler DAHA da
        // düşük — parlak yazılırsa siluet değil "kırmızı boya" olur. Kademe
        // sırası koyulaşma yönünde: alevin içindeki karanlık çekirdek.
        // Siluet de TURUNCUYA çalıyor (Hakan): saf kırmızı, sıcak sarının
        // içinde "boya lekesi" gibi duruyordu; turuncuya kayınca aynı ateşin
        // koyu çekirdeği okunuyor.
        // Siluet aynı eğride ama SÖNDÜRÜLMÜŞ: sarının üstüne çizildiği için
        // parlak olursa siluet değil boya olur. Sönüm çarpanları 0.72→0.30.
        /*DARK */ {{{184, 78,  0},{153, 59,  0},{117, 40,  0},{ 76, 22,  0}},   // 1500→1050K kanonik
                   {{132, 58,132},{104, 42,108},{ 76, 30, 82},{ 48, 18, 52}},   // soğuk (mor ateş: eğri DIŞI, bilinçli)
                   {{189, 87,  0},{158, 65,  0},{120, 42,  0},{ 76, 22,  0}}},  // 1650→1050K sıcak
    };
    int row;
    switch (t) {
        case WEAPON_FX_FLAME:
        case WEAPON_FX_LICK:  row = 0; break;
        case WEAPON_FX_SOOT:  row = 4; break;   // duman paleti (koyu)
        case WEAPON_FX_FLAME_DEEP: row = 7; break;   // KIZIL zarf paleti
        case WEAPON_FX_FLAME_DARK: row = 8; break;   // KOYU siluet paleti
        case WEAPON_FX_ICE:   row = 1; break;
        case WEAPON_FX_SPARK: row = 2; break;
        case WEAPON_FX_EMBER: row = 3; break;
        case WEAPON_FX_SMOKE: row = 4; break;
        case WEAPON_FX_STEAM: row = 5; break;
        // PINK ve VENOM BİLEREK yok: ikisi de SİLAH efekti, renkleri
        // ründen/eşyadan geliyor. Palete alınınca pembe parlama beyaza
        // döndü (Hakan yakaladı) — kimlik taşıyan rengin üstüne yazmak
        // her zaman hata. Onlar kendi tip-özel matematiğinde kalıyor.
        case WEAPON_FX_ROCK:  row = 6; break;
        default: row = -1; break;
    }
    if (row < 0) { *r = *g = *b = 255; return; }
    int k = (int)((1.0f - life01) * 4.0f);
    if (k < 0) k = 0;
    if (k > 3) k = 3;
    // RENK KIRILMASI: kademeyi parçacık başına ±1 oynat. Hepsi aynı
    // kademede olunca 300 parçacık TEK RENK bloğu gibi duruyor; ±1
    // kayma paleti bozmadan doku veriyor (dither mantığı).
    // HATA: bu kayma fx_rnd() ile ÇİZİM ANINDA hesaplanıyordu, yani her
    // parçacığın rengi KARE KARE zıplıyordu (duman ve buzda gürültü, alevde
    // sahte kaynama). Kırılma parçacığa BAĞLI olmak zorunda: tohum slot
    // indeksinden geliyor, ömür boyunca sabit kalıyor.
    int jit = (int)(((unsigned)seed * 2654435761u >> 13) % 3u) - 1;
    k += jit;
    if (k < 0) k = 0;
    if (k > 3) k = 3;
    int v = (palVar < 0 || palVar > 2) ? 0 : palVar;
    *r = PAL[row][v][k][0]; *g = PAL[row][v][k][1]; *b = PAL[row][v][k][2];
}

void fx_init(ParticleSystem *fx)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
        fx->items[i].active = false;
    fx->nextFree = 0;
}

void fx_spawn(ParticleSystem *fx, float x, float y, float vx, float vy,
              float life, float size, Uint8 r, Uint8 g, Uint8 b, WeaponFxType fxType)
{
    // Bos slot bul (nextFree'den itibaren dairesel)
    // ── SINIRLI TARAMA. Havuz doluyken eskiden TÜM havuz (5120 slot) taranıp
    // boş dönülüyordu. Ateş Aslanı kare başına ~380 partikül istiyor; birkaç mob
    // iz taşıyıp havuz doyduğunda bu kare başına ~2 MİLYON iterasyon oluyor ve
    // oyun gerçekten takılıyor (Hakan: "moba çarpıp ilerlerken donmalar oluyor";
    // dummy'de olmamasının sebebi tek mobun havuzu doyurmaması).
    // Havuz bir halka ve partiküller sürekli ölüyor: 128 slot probe etmek boş
    // slot bulmak için fazlasıyla yeterli, bulunamazsa partikül DÜŞER (zaten
    // doymuş bir bulutta bir partikül fark etmez). Başarısızlıkta nextFree
    // ilerliyor ki sıradaki deneme başka yeri yoklasın — yoksa aynı dolu bölge
    // tekrar tekrar taranıyor.
    #define FX_PROBE 128
    for (int n = 0; n < FX_PROBE; n++) {
        int i = (fx->nextFree + n) % MAX_PARTICLES;
        if (!fx->items[i].active) {
            fx->items[i] = (Particle){
                .x = x, .y = y,
                .vx = vx, .vy = vy,
                .life = life, .maxLife = life,
                .size = size,
                .r = r, .g = g, .b = b,
                .fxType = fxType,
                .additive = fx_type_additive(fxType),
                // Takla yalnız köşeli tiplerde; işaret rastgele (iki yöne döner)
                .palVar = (Uint8)((s_palMix > 0.0f && fx_rnd() < s_palMix)
                                  ? s_palAlt : s_palVar),
                .spin = (fxType == WEAPON_FX_ICE || fxType == WEAPON_FX_ROCK ||
                         fxType == WEAPON_FX_SOOT)
                        ? ((fx_rnd() < 0.5f) ? -1.0f : 1.0f)
                          * (3.0f + fx_rnd() * 7.0f) : 0.0f,
                .active = true
            };
            fx->nextFree = (i + 1) % MAX_PARTICLES;
            return;
        }
    }
    // Havuz dolu → bu partikul atlanir (sorun degil)
    fx->nextFree = (fx->nextFree + FX_PROBE) % MAX_PARTICLES;
    #undef FX_PROBE
}

void fx_update(ParticleSystem *fx, float dt)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &fx->items[i];
        if (!p->active) continue;

        // Hareket
        p->x += p->vx * dt;
        p->y += p->vy * dt;

        // SÜRTÜNME tipe göre. Duman/buhar havada asılı kalır (yavaş sönüm,
        // yükselmeye devam eder); kıvılcım hızlı frenlenir ve YERÇEKİMİ ile
        // düşer (yukarı fırlayıp geri inen kavis — kıvılcımı kıvılcım yapan o).
        if (fx_type_grows(p->fxType)) {
            p->vx *= (1.0f - 0.6f * dt);
            p->vy *= (1.0f - 0.3f * dt);
        } else if (p->fxType == WEAPON_FX_EMBER) {
            p->vx *= (1.0f - 6.0f * dt);   // hemen durur: kor yerinde yanar
            p->vy *= (1.0f - 6.0f * dt);
        } else if (p->fxType == WEAPON_FX_LICK) {
            // Alev dili YUKARI kıvrılır: yatay hız hızla söner, dikey
            // hız yukarı çekilir (sıcak hava kalkıyor)
            p->vx *= (1.0f - 3.4f * dt);
            p->vy = p->vy * (1.0f - 1.2f * dt) - 110.0f * dt;
        } else if (p->fxType == WEAPON_FX_FLAME_DEEP ||
                   p->fxType == WEAPON_FX_FLAME_DARK) {
            // Zarf ÇEKİRDEKLE BİRLİKTE hareket eder. Önce çok ağır yazılmıştı
            // (yatay sönüm 2.2, yükseliş -34) ve patlamanın gerisinde kalıyordu;
            // mekânsal gecikme de "sonradan geliyor" okumasına katkı yapıyordu.
            // Şimdi alev dilinin dinamiğine yakın, yalnız bir tık ağır.
            p->vx *= (1.0f - 2.8f * dt);
            p->vy = p->vy * (1.0f - 1.1f * dt) - 78.0f * dt;
        } else if (p->fxType == WEAPON_FX_SOOT) {
            // İs pulu: havada salınarak düşer (yerçekimi zayıf + yalpa)
            p->vx += SDL_sinf(p->life * 7.0f) * 26.0f * dt;
            p->vy = p->vy * (1.0f - 0.9f * dt) + 34.0f * dt;
        } else if (p->fxType == WEAPON_FX_SPARK) {
            p->vx *= (1.0f - 1.4f * dt);
            p->vy = p->vy * (1.0f - 1.4f * dt) + 210.0f * dt;   // yerçekimi
        } else if (p->fxType == WEAPON_FX_ROCK) {
            // TAŞ AĞIRDIR: yukarı fırlar, yavaşlar, DÜŞER. Yerçekimi olmadan
            // kaya parçaları havada asılı kalıyor ve efekt "toz" gibi okunuyor
            // (Hakan: toprak büyüsü "sadece toprak toz efekti gibi").
            p->vx *= (1.0f - 0.7f * dt);
            p->vy = p->vy * (1.0f - 0.5f * dt) + 620.0f * dt;
        } else {
            p->vx *= (1.0f - 2.0f * dt);
            p->vy *= (1.0f - 2.0f * dt);
        }

        // KUVVET ALANI: rüzgâr partikülü sürükler, duyarlılık tipe göre.
        // İvme olarak uygulanıyor (hız değil): partikül rüzgâra ZAMANLA
        // uyum sağlıyor, bir anda savrulmuyor.
        float wk = fx_type_wind(p->fxType);
        if (wk > 0.001f && (s_windX != 0.0f || s_windY != 0.0f)) {
            p->vx += (s_windX - p->vx) * wk * 1.4f * dt;
            p->vy += (s_windY * 0.35f - p->vy) * wk * 0.5f * dt;
        }

        // Omur
        p->life -= dt;
        if (p->life <= 0.0f) {
            p->active = false;
            // ÖLÜM KANCASI: motor "öldü" der, kalıntıyı oyun katmanı seçer
            if (s_deathHook != NULL)
                s_deathHook(p->x, p->y, p->fxType, p->size);
        }
    }
}

// Tüm aralık: eski çağrı yerleri bozulmasın (bant sarmalayıcısı aşağıda)
void fx_render(ParticleSystem *fx, R2D *renderer,
               R2DTexture *glowTex, const Camera *camera)
{
    fx_render_band(fx, renderer, glowTex, camera, -1.0e9f, 1.0e9f);
}

void fx_render_band(ParticleSystem *fx, R2D *renderer,
                    R2DTexture *glowTex, const Camera *camera,
                    float yMin, float yMax)
{
    if (glowTex == NULL) return;
    if (s_shapeTex == NULL) fx_bake_shapes(renderer);   // ilk karede bir kez
    R2DTexture *tex = (s_shapeTex != NULL) ? s_shapeTex : glowTex;
    float zoom = camera->zoom;

    // İKİ GEÇİŞ: önce ALPHA (madde: zehir/buz/duman), sonra ADD (ışık: alev).
    // Tipe göre gruplamak batch'in blend değişiminde bölünmesini de azaltıyor
    // (blend başına tek draw). VFX borcu #2: eskiden TEK geçiş ve HEP ALPHA'ydı.
    // ÜÇ GEÇİŞ: 0 ALPHA (madde: duman, taş, kızıl zarf) → 1 ADD (ışık: alev,
    // kıvılcım, kor) → 2 ALPHA-ÜSTÜ (koyu kırmızı siluetler).
    // 3. geçiş neden var: toplamalı katman altındaki her koyu rengi yutuyor
    // (R doyunca hue kayboluyor). "Sarı içinde kırmızı" ancak sarının ÜSTÜNE
    // çizilerek olur. Ayrı bir geometri katmanı yerine partikül olması şart,
    // yoksa kırmızı sarıdan bağımsız hareket ediyor (Hakan: "iç içe olmalı").
    for (int pass = 0; pass < 3; pass++) {
    bool wantAdd = (pass == 1);
    r2d_set_blend(renderer, wantAdd ? R2D_BLEND_ADD : R2D_BLEND_ALPHA);

    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &fx->items[i];
        if (!p->active) continue;
        // Geçiş seçimi: 2. geçiş yalnız "üstüne çizilen" tipleri alır, ilk iki
        // geçiş onları ATLAR (yoksa iki kez çizilirler).
        // DERİNLİK BANDI: aralık dışındaki partikül bu çağrıda çizilmez
        if (p->y < yMin || p->y >= yMax) continue;
        bool over = fx_type_over(p->fxType);
        if (pass == 2) { if (!over) continue; }
        else if (over || p->additive != wantAdd) continue;

        // Omur orani (1=yeni, 0=oluyor)
        float t = p->life / p->maxLife;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        // Solma + kuculme: omur azaldikca alpha ve boyut duser
        // EKLEMELİDE daha yüksek: ADD'de alfa "ne kadar ışık kattığı" demek.
        // 200'de tek partikul eşiğin altında kalıyordu; 235'te iki-üç
        // partikulun üst üste binmesi bloom eşiğini (0.74) aşıyor.
        Uint8 alpha;
        if (fx_type_grows(p->fxType)) {
            // Duman/buhar DOĞUMDA da şeffaf: bir anda belirmesin (su fazının
            // "köpük enerjiyle doğsun" dersinin aynısı). Tepe ömrün %25'inde.
            float u = 1.0f - t;                      // 0=yeni, 1=ölüyor
            float e = (u < 0.25f) ? (u / 0.25f) : (1.0f - (u - 0.25f) / 0.75f);
            // Duman 105 → 155: gündüz açık zeminde silik kalıyordu (Hakan)
            alpha = (Uint8)(e * (p->fxType == WEAPON_FX_STEAM ? 145.0f : 155.0f));
        } else if (p->fxType == WEAPON_FX_FLAME_DEEP ||
                   p->fxType == WEAPON_FX_FLAME_DARK) {
            // ZARF DOĞDUĞU ANDA VAR: alfa tepesi ömrün BAŞINDA, son %55'te
            // sönüyor. Rampalı doğuş burada yanlış — dış gövde çekirdekle
            // birlikte belirmek zorunda.
            alpha = (Uint8)((t > 0.55f ? 1.0f : t / 0.55f) * 190.0f);
        } else if (p->fxType == WEAPON_FX_EMBER) {
            // KOR SABİT PARLAR: ömrün son %30'una kadar alfa neredeyse
            // düşmüyor, sonra hızla sönüyor. Titremeyen ışık kanon gereği
            // (LORE §1.9) ve mekaniğe de oturuyor: kor bastırır.
            alpha = (Uint8)((t > 0.30f ? 1.0f : t / 0.30f) * 225.0f);
        } else {
            alpha = (Uint8)(t * (p->additive ? 235.0f : 200.0f));
        }
        // BOYUT DAVRANIŞI tipe göre: duman/buhar soğuyup YAYILIR (büyür),
        // alev/kıvılcım yanıp TÜKENİR (küçülür). Eskiden tek davranış vardı
        // ve duman "küçülen gri top" gibi duruyordu.
        float size;
        if (p->fxType == WEAPON_FX_EMBER)
            size = p->size * (0.55f + t * 0.45f) * zoom;   // az küçülür: kor korunur
        else if (fx_type_grows(p->fxType))
            size = p->size * (0.55f + (1.0f - t) * 1.85f) * zoom;   // 0.55 → 2.4×
        else if (p->fxType == WEAPON_FX_FLAME_DEEP ||
                 p->fxType == WEAPON_FX_FLAME_DARK)
            // Zarf TAM boyda doğar, hafifçe genişleyip söner (küçülmüyor:
            // küçülen zarf çekirdeğin içine çekilip kayboluyor)
            size = p->size * (1.0f + (1.0f - t) * 0.35f) * zoom;
        else
            size = p->size * (0.3f + t * 0.7f) * zoom;              // ölürken küçül

        // Dunya → ekran
        float sx = (p->x - camera->x) * zoom;
        float sy = (p->y - camera->y) * zoom;

        // Renk: efekt tipine gore
        Uint8 cr, cg, cb;
        if (p->fxType == WEAPON_FX_FLAME) {
            // Alev: yeni=sari, yaslandikca kizillasir (yesil+mavi duser)
            cr = 255;
            cg = (Uint8)(p->g * t);
            cb = (Uint8)(p->b * t * t);
        } else if (p->fxType == WEAPON_FX_VENOM) {
            // İğne zehri: mor doğar, ölürken yeşile çalar (rünün tersi yönde akar)
            cr = (Uint8)(p->r * t);
            cg = (Uint8)(p->g + (1.0f - t) * 60.0f);
            cb = (Uint8)(p->b * t);
        } else if (p->fxType == WEAPON_FX_ICE) {
            // Buz: yeni=beyaz-mavi, olurken maviye doner (kirmizi duser)
            cr = (Uint8)(p->r * t);
            cg = p->g;
            cb = p->b;
        } else if (p->fxType == WEAPON_FX_SMOKE) {
            // Duman yükseldikçe SOĞUR: soluklaşır ve maviye kayar (gökyüzü
            // ışığı alır). Düz gri "is" gibi durmasın.
            cr = (Uint8)(p->r + (1.0f - t) * 42.0f);
            cg = (Uint8)(p->g + (1.0f - t) * 46.0f);
            cb = (Uint8)(p->b + (1.0f - t) * 58.0f);
        } else if (p->fxType == WEAPON_FX_EMBER) {
            // Kor: turuncu-kızıl, ömürle koyulaşır (akkor → kızıl kor)
            cr = 255;
            cg = (Uint8)(70.0f + p->g * t * 0.42f);
            cb = (Uint8)(18.0f + p->b * t * 0.25f);
        } else if (p->fxType == WEAPON_FX_EMBER) {
            // Kor: turuncu-kızıl, ömürle koyulaşır (akkor → kızıl kor)
            cr = 255;
            cg = (Uint8)(70.0f + (float)p->g * t * 0.42f);
            cb = (Uint8)(18.0f + (float)p->b * t * 0.25f);
        } else if (p->fxType == WEAPON_FX_SPARK) {
            // Kıvılcım: beyaz-sarı doğar, ölürken kızarır (soğuyan metal)
            cr = 255;
            cg = (Uint8)(140.0f + p->g * t * 0.45f);
            cb = (Uint8)(p->b * t * t);
        } else {
            // Pembe (ve digerleri): sabit renk, sadece soner
            cr = p->r;
            cg = p->g;
            cb = p->b;
        }

        // PALET: tanımlı tiplerde 4 kademeli sert palet, tanımsızda
        // yukarıdaki tip-özel matematik (silah efektleri) korunuyor.
        {
            Uint8 pr, pg, pb;
            fx_palette(p->fxType, (int)p->palVar, t, i, &pr, &pg, &pb);
            if (!(pr == 255 && pg == 255 && pb == 255)) { cr = pr; cg = pg; cb = pb; }
        }
        r2d_texture_set_color_mod(tex, cr, cg, cb);
        r2d_texture_set_alpha_mod(tex, alpha);
        // BİÇİM: atlastan tipe göre hücre
        int shape = (s_shapeTex != NULL) ? fx_type_shape(p->fxType) : 0;
        // ALEV DİLİ: üç varyanttan biri. Seçim parçacık SLOTUNDAN türüyor —
        // sabit (aynı parçacık kare kare şekil değiştirmez) ama havuz boyunca
        // dağınık. Rastgele sayı harcamıyor, alan da açmıyor.
        if (p->fxType == WEAPON_FX_LICK) {
            static const int TV[3] = { FXS_TEAR, FXS_TEAR_B, FXS_TEAR_C };
            shape = TV[i % 3];
        } else if (p->fxType == WEAPON_FX_ICE) {
            static const int CV[3] = { FXS_CRY_A, FXS_CRY_B, FXS_CRY_C };
            shape = CV[i % 3];
        }
        SDL_FRect src = { (float)(shape * FXS_CELL), 0.0f,
                          (float)FXS_CELL, (float)FXS_CELL };
        const SDL_FRect *srcp = (s_shapeTex != NULL) ? &src : NULL;

        if (s_shapeTex != NULL && p->spin != 0.0f) {
            // TAKLA ATAN KIRIK: yönü SPIN açısından, uzunluğu hızdan.
            // Hız yönüne kilitlemek yerine döndürmek "havada savrulan enkaz"
            // okumasını veriyor — buz/kaya için doğru olan bu.
            float age = p->maxLife - p->life;
            float ang = p->spin * age;
            float sp2 = SDL_sqrtf(p->vx * p->vx + p->vy * p->vy);
            float st = 1.0f + SDL_min(sp2 / 190.0f, 1.6f);
            float ux = SDL_cosf(ang), uy = SDL_sinf(ang);
            float hl = size * 0.5f * st, hw = size * 0.5f;
            float px2 = -uy, py2 = ux;
            SDL_FPoint c4[4] = {
                { sx - ux * hl + px2 * hw, sy - uy * hl + py2 * hw },
                { sx + ux * hl + px2 * hw, sy + uy * hl + py2 * hw },
                { sx - ux * hl - px2 * hw, sy - uy * hl - py2 * hw },
                { sx + ux * hl - px2 * hw, sy + uy * hl - py2 * hw },
            };
            r2d_draw_texture_quad(renderer, tex, srcp, c4,
                                  (SDL_Color){ cr, cg, cb, alpha });
        } else if (s_shapeTex != NULL && fx_type_stretch(p->fxType)) {
            // HIZ YÖNÜNDE UZAMA. Yuvarlak nokta hızlı giderken YAVAŞ okunur;
            // hareket yönünde uzatınca hız görünür hale geliyor. Uzama hıza
            // bağlı ve TAVANLI (4×) — sınırsız bırakılırsa parçacık çizgiye
            // dönüşüp biçimini kaybediyor.
            float sp2 = SDL_sqrtf(p->vx * p->vx + p->vy * p->vy);
            float st = 1.0f + SDL_min(sp2 / 130.0f, 3.0f);
            float ux = (sp2 > 1.0f) ? p->vx / sp2 : 1.0f;
            float uy = (sp2 > 1.0f) ? p->vy / sp2 : 0.0f;
            float hl = size * 0.5f * st, hw = size * 0.5f;
            // Köşe sırası TL, TR, BL, BR (r2d sözleşmesi): uzun eksen HIZ yönü
            float px2 = -uy, py2 = ux;   // dik vektör
            SDL_FPoint c4[4] = {
                { sx - ux * hl + px2 * hw, sy - uy * hl + py2 * hw },
                { sx + ux * hl + px2 * hw, sy + uy * hl + py2 * hw },
                { sx - ux * hl - px2 * hw, sy - uy * hl - py2 * hw },
                { sx + ux * hl - px2 * hw, sy + uy * hl - py2 * hw },
            };
            r2d_draw_texture_quad(renderer, tex, srcp, c4,
                                  (SDL_Color){ cr, cg, cb, alpha });
        } else {
            SDL_FRect dst = {
                .x = sx - size * 0.5f,
                .y = sy - size * 0.5f,
                .w = size, .h = size
            };
            r2d_draw_texture(renderer, tex, srcp, &dst);
        }
    }
    }   // pass

    // Normale dondur (blend dahil — sonraki çizim ADD miras almasın)
    r2d_set_blend(renderer, R2D_BLEND_ALPHA);
    r2d_texture_set_color_mod(tex, 255, 255, 255);
    r2d_texture_set_alpha_mod(tex, 255);
}
// --- SHEET-ANİM HAVUZU (VFX his cilası) ---

void animfx_init(AnimFx *pool)
{
    for (int i = 0; i < MAX_ANIM_FX; i++)
        pool[i].active = false;
}

void animfx_spawn_lit(AnimFx *pool, R2DTexture *tex, int frames, int cols,
                      float fw, float fh, float x, float y, float fps, float scale,
                      float r, float g, float b, float radius, float intensity)
{
    if (tex == NULL) return;
    int slot = -1;
    for (int i = 0; i < MAX_ANIM_FX; i++)
        if (!pool[i].active) { slot = i; break; }
    if (slot < 0) return;   // havuz dolu — efekt atlanir (kritik degil)
    pool[slot] = (AnimFx){ .tex = tex, .frames = frames, .cols = cols,
                           .fw = fw, .fh = fh, .x = x, .y = y,
                           .t = 0.0f, .fps = fps, .scale = scale,
                           .active = true,
                           .lightR = r, .lightG = g, .lightB = b,
                           .lightRadius = radius, .lightIntensity = intensity };
}

void animfx_spawn(AnimFx *pool, R2DTexture *tex, int frames, int cols,
                  float fw, float fh, float x, float y, float fps, float scale)
{
    // Işıksız sürüm: renk sıfır → animfx_append_lights bu slotu atlar
    animfx_spawn_lit(pool, tex, frames, cols, fw, fh, x, y, fps, scale,
                     0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
}

// Aktif efektlerin ışıkları. Eğri: ilk %20 hızlı yükseliş, sonra sönüş.
// Patlama önce parlar, sonra kor gibi çekilir — düz sönüş "kapandı" gibi duruyor.
int animfx_append_lights(const AnimFx *pool, R2DLight *out, int n, int cap,
                         const Camera *cam, int sw, int sh)
{
    if (pool == NULL || out == NULL || cam == NULL) return n;
    float total, z = cam->zoom;
    for (int i = 0; i < MAX_ANIM_FX && n < cap; i++) {
        const AnimFx *a = &pool[i];
        if (!a->active) continue;
        if (a->lightR <= 0.0f && a->lightG <= 0.0f && a->lightB <= 0.0f) continue;
        if (a->fps <= 0.0f || a->frames <= 0) continue;
        total = (float)a->frames / a->fps;          // efektin toplam süresi
        if (total <= 0.0f) continue;
        float u = a->t / total;                      // 0..1 ilerleme
        if (u < 0.0f) u = 0.0f;
        if (u > 1.0f) continue;
        float k = (u < 0.2f) ? (u / 0.2f)             // yükseliş
                             : (1.0f - (u - 0.2f) / 0.8f);   // sönüş
        if (k <= 0.01f) continue;
        k = k * k;   // kare: sönüş göze doğrusaldan daha doğal gelir
        float sx = (a->x - cam->x) * z;
        float sy = (a->y - cam->y) * z;
        float sr = a->lightRadius * z * (0.6f + 0.4f * k);   // yarıçap da nefes alır
        // Ekran dışı atla (yarıçap payıyla)
        if (sx + sr < 0.0f || sx - sr > (float)sw ||
            sy + sr < 0.0f || sy - sr > (float)sh) continue;
        out[n++] = (R2DLight){ sx, sy, sr, a->lightR, a->lightG, a->lightB,
                               a->lightIntensity * k, 0.0f, 0.0f, 0.0f,
                               0.85f, -6.0f };   // hafif basık, biraz yukarı
    }
    return n;
}

void animfx_update(AnimFx *pool, float dt)
{
    for (int i = 0; i < MAX_ANIM_FX; i++) {
        if (!pool[i].active) continue;
        pool[i].t += dt;
        if (pool[i].t * pool[i].fps >= (float)pool[i].frames)
            pool[i].active = false;
    }
}

void animfx_render(AnimFx *pool, R2D *renderer, const Camera *camera)
{
    float zoom = camera->zoom;
    for (int i = 0; i < MAX_ANIM_FX; i++) {
        if (!pool[i].active) continue;
        int fr = (int)(pool[i].t * pool[i].fps);
        if (fr >= pool[i].frames) fr = pool[i].frames - 1;
        SDL_FRect src = { (float)(fr % pool[i].cols) * pool[i].fw,
                          (float)(fr / pool[i].cols) * pool[i].fh,
                          pool[i].fw, pool[i].fh };
        float w = pool[i].fw * pool[i].scale;
        float h = pool[i].fh * pool[i].scale;
        SDL_FRect dst = { (pool[i].x - w / 2.0f - camera->x) * zoom,
                          (pool[i].y - h / 2.0f - camera->y) * zoom,
                          w * zoom, h * zoom };
        r2d_draw_texture(renderer, pool[i].tex, &src, &dst);
    }
}
