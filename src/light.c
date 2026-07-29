#include "light.h"
#include "weather.h"
#include <math.h>

// ── DÜŞÜŞ EĞRİSİ — GPU fs_light ile AYNI matematik ──
static int   s_falloffMode  = 0;
static float s_falloffParam = 41.0f;

void  light_set_falloff(int mode, float param) { s_falloffMode = mode; s_falloffParam = param; }
int   light_falloff_mode(void)  { return s_falloffMode; }
float light_falloff_param(void) { return s_falloffParam; }

float light_atten(float d, float r)
{
    if (r <= 1.0f || d >= r) return 0.0f;
    float a;
    switch (s_falloffMode) {
        case 1: {   // Karis pencereli ters-kare (UE4 2013 dnk.9)
            float u = d / s_falloffParam;
            float w = 1.0f - powf(d / r, 4.0f);
            if (w < 0.0f) w = 0.0f;
            a = (w * w) / (u * u + 1.0f);
            break;
        }
        case 2:     // URP 2D pow(1-t, k)
            a = powf(1.0f - d / r, s_falloffParam);
            break;
        case 3: {   // kenar dizi (knee)
            float b = 1.0f - d / r;
            float sa = b * b * (3.0f - 2.0f * b);
            float w = b / 0.25f; if (w > 1.0f) w = 1.0f;
            a = sa * w * w;
            break;
        }
        default: {  // iç plato + smoothstep
            float t = (1.0f - d / r) / LIGHT_CORE_FRAC;
            if (t > 1.0f) t = 1.0f;
            a = t * t * (3.0f - 2.0f * t);
            break;
        }
    }
    // KESİN KENAR — HER MODA uygulanır. "Eşik = ışığın bittiği yer" kararının
    // şartı bu: kuyruğu asimptotik sıfıra giden bir eğride lamp_level_at()>0
    // testi anlamsızdır (att=9e-5 gözle siyahtır ama sıfırdan büyüktür).
    // Eskiden yalnız mod 0'da vardı → F2'ye basmak bastırma yarıçapını
    // 149'dan 165 px'e çıkarıyordu, ve oyuncu KARANLIK gördüğü yerde korunuyordu.
    return (a < LIGHT_EDGE_CUT) ? 0.0f : a;
}

// ── KELVİN → RGB (Tanner Helland yaklaşımı; Mitchell Charity siyah cisim
// tablosu + CIE 1964, geçerli 1000-40000K). Frostbite Tablo 4 referansları:
// kibrit 1700-1800K · mum 1850-1930K · gündoğumu 2000-3000K · gün ışığı 6500K.
// Not: bizim elle seçtiğimiz kandil rengi bu eğride ≈3130K'ye denk — yani
// fiziksel olarak bir tungsten masa lambası. Gerçek kor/mum 1900K.
void light_kelvin_rgb(float kelvin, float *r, float *g, float *b)
{
    float t = kelvin / 100.0f;
    float rr, gg, bb;
    if (t <= 66.0f) rr = 255.0f;
    else            rr = 329.698727446f * powf(t - 60.0f, -0.1332047592f);
    if (t <= 66.0f) gg = 99.4708025861f * logf(t) - 161.1195681661f;
    else            gg = 288.1221695283f * powf(t - 60.0f, -0.0755148492f);
    if (t >= 66.0f)      bb = 255.0f;
    else if (t <= 19.0f) bb = 0.0f;
    else                 bb = 138.5177312231f * logf(t - 10.0f) - 305.0447927307f;
    *r = (rr < 0 ? 0 : rr > 255 ? 255 : rr) / 255.0f;
    *g = (gg < 0 ? 0 : gg > 255 ? 255 : gg) / 255.0f;
    *b = (bb < 0 ? 0 : bb > 255 ? 255 : bb) / 255.0f;
}

// Köy ışıkları: ev pencereleri (decor.c'deki evlerin gövdesine hizalı — ev
// tabanı (x,y)'de, pencere ışığı biraz yukarıda) + meydan meşale direkleri.
// Direklerin sprite'ı yok — şimdilik yalnız ışık, görseli sonra.
#define T(n) ((n) * (float)TILE_SIZE)

// Yoğunluklar bilerek <0.75: ambient+katkı 1.0'a genişçe doymasın —
// doyma düz "plato" yapar, komşu ışıkla birleşince silindir gibi okunur.
// Mavi/yeşil bileşenler düşük: merkez beyazlaşmasın, sıcaklık korunsun.
const LightDef LIGHTS[] = {
    // Ev pencereleri (Baran / Kemal / Osman) — sıcak turuncu, mum gibi hafif titrer
    { .x = T(10), .y = T(4)  - 50.0f, .radius = 78.0f, .r = 1.00f, .g = 0.58f, .b = 0.28f, .intensity = 0.40f, .kind = LK_CANDLE },
    { .x = T(14), .y = T(6)  - 50.0f, .radius = 78.0f, .r = 1.00f, .g = 0.58f, .b = 0.28f, .intensity = 0.40f, .kind = LK_CANDLE },
    { .x = T(6),  .y = T(10) - 50.0f, .radius = 78.0f, .r = 1.00f, .g = 0.58f, .b = 0.28f, .intensity = 0.40f, .kind = LK_CANDLE },

    // Köy meydanı meşale direkleri — DEVRE DIŞI, silinmedi (LORE §14.8).
    // Bu iki direk artık DURUMU OLAN kandil direği: lamp.c LAMPS[] tablosunda
    // (aynı koordinatlar, T(10),T(8) ve T(13),T(10)). Statik tabloda kalsalardı
    // sönmüş direk bile ışık verirdi ve §1.4 (karanlıkta ölü kalkar) çalışmazdı.
    // Kandil sistemi kaldırılırsa bu iki satırın yorumu açılır, köy eski
    // haline döner.
    // { .x = T(10), .y = T(8),  .radius = 200.0f, .r = 1.00f, .g = 0.62f, .b = 0.28f, .intensity = 0.65f, .flicker = 0.12f },
    // { .x = T(13), .y = T(10), .radius = 200.0f, .r = 1.00f, .g = 0.62f, .b = 0.28f, .intensity = 0.65f, .flicker = 0.12f },
};
const int LIGHT_COUNT = (int)(sizeof(LIGHTS) / sizeof(LIGHTS[0]));

// ═══ TİTREŞİM ÇEKİRDEĞİ ═══
//
// Eski sistem yalnız intensity'yi iki simetrik sinüsle oynatıyordu (1.16 + 2.07 Hz).
// Dört kusuru vardı ve hepsi "nefes alan ampul" hissini üretiyordu:
//   (a) periyodik — göz 5 saniyede kalıbı yakalıyordu
//   (b) frekans yanlış — 2 Hz şenlik ateşi frekansı, mum/meşaleye 5-10 Hz gerek
//   (c) simetrik — alev yavaş şişer, HIZLI çöker (oran ~4:1)
//   (d) tek eksen — daire milimetrik aynı yerde, aynı çapta duruyordu
//
// Ve görünmeyen beşinci kusur: çarpım blend'i 1.0'da kırpıyor. Ambient 0.24 +
// kandil 0.66 = 0.90; +%12 titreşim 1.01 yapıp KIRPILIYORDU, yani titreşimin
// yukarı yarısı halkanın göbeğinde hiç görünmüyordu. Çözüm: sinyali AŞAĞI
// yönlü çevirmek (Doom 1993 deseni: tepe = taban değer).
//
// Ölçüm notu: "mum 10 Hz titrer" folkloru 2 cm çaplı mum GRUBUNA ait; tek alev
// periyodik değil. Alev frekansı f ≈ 1.5/√D (Cetegen & Ahmed 1993).
// Ve yarıçap oynatmak parlaklık oynatmaktan ~5.9× etkili: %5 parlaklık kenarı
// 1.4 px kaydırır, %5 yarıçap 8.2 px. Bütçe yarıçaba yatırılıyor.

typedef struct {
    float iAmp;   // intensity genliği (AŞAĞI yönlü)
    float rAmp;   // yarıçap genliği — asıl iş bunda
    float pAmp;   // merkez sapması (dünya px) — "yere çivili daire"yi kırar
    float freq;   // taban frekans (Hz)
    float wAmp;   // SİLUET genliği: halka kusursuz daire olmasın (GPU'da)
    float aniso;  // ŞEKİL: <1 basık elips · 1 daire · >1 dikey
    float yOff;   // kaynağın YÜKSEKLİĞİ (ekran px, negatif = yukarı)
} FlickerKind;

// FİZİKSEL RENK MODU (dev tuşu): elle seçilmiş renk yerine siyah cisim eğrisi.
// Kelvin değerleri Frostbite Tablo 4'ten: kibrit 1700-1800K, mum 1850-1930K,
// gündoğumu 2000-3000K, gün ışığı 5500-6500K.
// KANON ÇELİŞKİSİ: LORE §1.9 "kor beyaza yakın soluk" der; fizik 1100K'de
// derin kırmızı der. Kor için 2400K seçtim — akkor katının beyaza kaydığı
// bölge, yani kanonu fiziğin içinde tutan uzlaşma.
static const float KELVIN_OF[LK_COUNT] = {
    [LK_STEADY]  = 2400.0f,   // kor: akkor katı, beyaza kayan
    [LK_CANDLE]  = 1900.0f,   // mum alevi
    [LK_TORCH]   = 1700.0f,   // çıra
    [LK_HEARTH]  = 1600.0f,   // odun ateşi alev tabanı
    [LK_MAGIC]   = 9000.0f,   // kutsal: soğuk, doğaüstü
};
static bool s_physColor = false;
void light_set_physical_color(bool on) { s_physColor = on; }
bool light_physical_color(void) { return s_physColor; }

static const FlickerKind FLICKER[LK_COUNT] = {
    //              iAmp   rAmp   pAmp  freq  wAmp
    // wAmp = siluet genliği. Kor'da da SIFIR DEĞİL: §1.9'un "sabit yanar"
    // kuralı PARLAKLIK için geçerli, kenarın matematiksel oval olması için
    // değil. Gerçek hiçbir ışık pergelle çizilmiş daire değildir. Ama sıralama
    // kanonu koruyor: kor en az (0.055), odun ateşi en çok (0.17) oynuyor.
    // aniso/yOff: her kaynağın kendi şekli. Kandilin haznesi direğin 34 px
    // tepesinde → yukarı kaymış ve basık; kamp ateşi yerde → çok basık ve
    // geniş; meşale elde → orta; büyü doğaüstü → daire (kuralı yok).
    [LK_STEADY]  = { 0.05f, 0.02f, 0.0f, 1.2f, 0.090f, 0.66f, -30.0f },
    [LK_CANDLE]  = { 0.10f, 0.05f, 1.0f, 8.0f, 0.110f, 0.74f, -16.0f },
    [LK_TORCH]   = { 0.14f, 0.09f, 2.0f, 5.0f, 0.165f, 0.86f, -20.0f },
    [LK_HEARTH]  = { 0.12f, 0.12f, 2.5f, 2.4f, 0.215f, 0.52f,  -6.0f },
    [LK_MAGIC]   = { 0.07f, 0.04f, 0.0f, 3.0f, 0.060f, 1.00f,   0.0f },
};

// Konum-hash'li faz: Terraria deseni. Dizi indeksi KULLANILMAZ — bir ışık ekran
// dışına çıkınca sonrakilerin indeksi kayıyordu ve aynı ateşin titreşimi
// zıplıyordu. Konumdan türetilen faz kararlı: kaybolup gelse bile aynı.
static int light_seed(float x, float y)
{
    // unsigned: işaretli çarpım INT_MAX'i aşıyordu (x≥811 px için) = UB.
    // Alt 10 bit aynı olduğu için görsel fark yok, ama UBSan altında patlar.
    unsigned ix = (unsigned)(int)(x * 0.037f);
    unsigned iy = (unsigned)(int)(y * 0.053f);
    return (int)(((ix * 73856093u) ^ (iy * 19349663u)) & 0x3FFu);
}

// 0..1 sinyal, 1 = tam parlak. Frekans oranları irrasyonele yakın (φ ve e) —
// periyot pratikte kapanmıyor, göz kalıp yakalayamıyor.
static float flick01(float t, float freq, int seed)
{
    float p = (float)seed * 2.399963f;          // altın açı: fazlar iyi dağılır
    float w = SDL_sinf(t * freq * 6.2831853f * 1.0000f + p)        * 0.45f
            + SDL_sinf(t * freq * 6.2831853f * 1.6180f + p * 2.7f) * 0.32f
            + SDL_sinf(t * freq * 6.2831853f * 2.7183f + p * 4.1f) * 0.23f;
    float u = 0.5f + 0.5f * w;
    // Asimetri: üs<1 tepeyi yayar, dibi keskinleştirir → yavaş şiş, hızlı çök.
    return SDL_powf(u, 0.62f);
}

// Titreşimi bir ışığa uygula. Dört eksen birden oynar.
static R2DLight flicker_apply(R2DLight l, LightKind kind, float t, float zoom,
                              float worldX, float worldY)
{
    if (kind >= LK_COUNT) kind = LK_STEADY;
    const FlickerKind *f = &FLICKER[kind];
    int seed = light_seed(worldX, worldY);

    // Fiziksel renk modu: elle seçilmiş rengi siyah cisim eğrisiyle değiştir
    if (s_physColor)
        light_kelvin_rgb(KELVIN_OF[kind], &l.r, &l.g, &l.b);

    float u  = flick01(t, f->freq, seed);
    // Yarıçap parlaklığı GECİKMELİ izler: alev hacminin ataleti var.
    float uR = flick01(t - 0.09f, f->freq, seed);

    // AŞAĞI yönlü — kırpılma yemesin
    l.intensity *= 1.0f - f->iAmp * (1.0f - u);
    l.radius    *= 1.0f - f->rAmp * (1.0f - uR);

    if (f->pAmp > 0.0f) {
        // İki farklı frekans → Lissajous; tek frekans daire çizdirir.
        float jx = SDL_sinf(t * f->freq * 2.1f + (float)seed * 1.7f);
        float jy = SDL_cosf(t * f->freq * 1.4f + (float)seed * 3.1f);
        l.x += jx * f->pAmp * zoom;
        l.y += jy * f->pAmp * zoom - 0.4f * f->pAmp * zoom * u;  // parlarken yukarı
    }

    // Parlak = sıcak (≈ ±150 K): yeşil ve mavi yukarı, kırmızı sabit.
    float warm = (u - 0.5f) * 0.10f;
    l.g *= 1.0f + warm * 0.5f;
    l.b *= 1.0f + warm;

    // SİLUET fazı: yavaş dönen bir vektör. Loblar döndükçe halkanın kenarı
    // nefes alır. cos/sin burada hesaplanır — shader'da per-pixel olmasın.
    l.aniso = f->aniso;
    l.yOff  = f->yOff * zoom;   // ekran px — radius gibi zoom'la ölçeklenmeli
    if (f->wAmp > 0.0f) {
        float ph = t * (0.9f + 0.35f * (float)(seed & 7)) + (float)seed * 1.31f;
        l.wobAmp = f->wAmp;
        l.wobC   = SDL_cosf(ph);
        l.wobS   = SDL_sinf(ph);
    }
    return l;
}

// Oyuncu meşalesi (envanterde Meşale varsa)
#define TORCH_RADIUS 128.0f

int light_collect(R2DLight *out, int cap, const Camera *cam, int sw, int sh, bool staticLights,
                  float torchX, float torchY, bool torchOn, WeaponFxType weaponFx)
{
    float zoom = cam->zoom;
    float t = (float)SDL_GetTicks() / 1000.0f;   // titreşim saati (sn)
    int n = 0;

    for (int i = 0; staticLights && i < LIGHT_COUNT && n < cap; i++) {
        float sx = (LIGHTS[i].x - cam->x) * zoom;
        float sy = (LIGHTS[i].y - cam->y) * zoom;
        float sr = LIGHTS[i].radius * zoom;
        if (sx + sr < 0 || sx - sr > (float)sw ||
            sy + sr < 0 || sy - sr > (float)sh)
            continue;   // ekran dışı — shader döngüsüne sokma
        R2DLight wl = { sx, sy, sr, LIGHTS[i].r, LIGHTS[i].g, LIGHTS[i].b,
                        LIGHTS[i].intensity, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
        out[n++] = flicker_apply(wl, LIGHTS[i].kind, t, zoom,
                                 LIGHTS[i].x, LIGHTS[i].y);
    }

    float psx = (torchX - cam->x) * zoom;
    float psy = (torchY - cam->y) * zoom;

    if (torchOn && n < cap) {
        // Yağmur meşaleyi boğar: ışık yarıçapı %20'ye kadar kısalır
        float wetMul = 1.0f - 0.2f * (weather_is_wet() ? weather_intensity() : 0.0f);
        R2DLight tl = { psx, psy, TORCH_RADIUS * wetMul * zoom,
                        1.00f, 0.52f, 0.16f, 0.70f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };  // çıra
        // Oyuncu meşalesi hareket ettiği için konum-hash'i kullanılmaz —
        // sabit tohum, yoksa yürürken titreşim fazı sıçrar.
        out[n++] = flicker_apply(tl, LK_TORCH, t, zoom, 1234.0f, 5678.0f);
    }

    // Silah efekti geceye kendi renginde ışık verir: alev güçlü, diğerleri sönük parıltı
    if (weaponFx != WEAPON_FX_NONE && n < cap) {
        R2DLight l = { psx, psy, 120.0f * zoom, 1.0f, 1.0f, 1.0f, 0.40f,
                       0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
        float flicker = 0.06f;   // enerji efektleri hafif nefes alır
        switch (weaponFx) {
        case WEAPON_FX_FLAME:
            l.radius = 180.0f * zoom;
            l.r = 1.00f; l.g = 0.58f; l.b = 0.22f; l.intensity = 0.75f;
            flicker = 0.15f;   // alev sert titrer
            break;
        case WEAPON_FX_ICE:
            l.r = 0.50f; l.g = 0.72f; l.b = 1.00f;
            break;
        case WEAPON_FX_POISON:
            l.r = 0.38f; l.g = 0.95f; l.b = 0.32f;
            break;
        default: // WEAPON_FX_PINK
            l.r = 1.00f; l.g = 0.38f; l.b = 0.80f;
            break;
        }
        out[n++] = flicker_apply(l, (weaponFx == WEAPON_FX_FLAME) ? LK_TORCH : LK_MAGIC,
                                 t, zoom, 4321.0f, 8765.0f);
        (void)flicker;
    }
    return n;
}

// Kamp ateşi: meşale direğinden büyük, sert titreşimli sıcak ışık
int light_append_point(R2DLight *out, int n, int cap, const Camera *cam,
                       int sw, int sh, float x, float y, float radius,
                       float r, float g, float b, float intensity, LightKind kind)
{
    if (n >= cap) return n;
    float zoom = cam->zoom;
    float sx = (x - cam->x) * zoom;
    float sy = (y - cam->y) * zoom;
    float sr = radius * zoom;
    if (sx + sr < 0 || sx - sr > (float)sw ||
        sy + sr < 0 || sy - sr > (float)sh)
        return n;
    float t = (float)SDL_GetTicks() / 1000.0f;
    R2DLight l = { sx, sy, sr, r, g, b, intensity, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
    out[n++] = flicker_apply(l, kind, t, zoom, x, y);
    return n;
}

int light_append_campfire(R2DLight *out, int n, int cap, const Camera *cam,
                          int sw, int sh, float x, float y)
{
    if (n >= cap) return n;
    float zoom = cam->zoom;
    float sx = (x - cam->x) * zoom;
    float sy = (y - cam->y) * zoom;
    float sr = 168.0f * zoom;
    if (sx + sr < 0 || sx - sr > (float)sw ||
        sy + sr < 0 || sy - sr > (float)sh)
        return n;   // ekran dışı
    float t = (float)SDL_GetTicks() / 1000.0f;
    // Odun ateşi: kırmızıya doygun ve kaotik — kor'un tam tersi (LORE §1.9).
    // Ve bu ateş ölüyü BASTIRMAZ; görsel dili de onu söylemeli.
    R2DLight l = { sx, sy, sr, 1.00f, 0.42f, 0.12f, 0.70f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
    out[n++] = flicker_apply(l, LK_HEARTH, t, zoom, x, y);
    return n;
}
