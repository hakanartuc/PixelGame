#include <SDL3/SDL.h>
#include "skillfx.h"
#include "decal.h"
#include "groundfx.h"

// ── PROSEDÜREL SKİLL EFEKTLERİ. Görsel dil: KLASİK RPG GÖRKEMİ (Hakan kararı
// 2026-08-04) — büyük, parlak, bol partikül, uzun ömür. Pixel-art disiplini
// yine geçerli (sanat ızgarası, kuantalanmış palet, sert kenar) çünkü onlar
// motor kuralı; görkem ÖLÇEKTEN ve SÜREDEN geliyor, bulanıklıktan değil.

#define ART_PX 4.0f     // 1 sanat pikseli = 4 dünya px (TILE 64 / doku 16)

typedef struct {
    SkillFxKind kind;
    float x, y;             // konum (FIREBOLT'ta mermi konumu)
    float tx, ty;           // hedef
    float life, maxLife;
    float power;            // 1.0 = temel; perk/seviye ölçekler
    float travel;           // FIREBOLT: 0→1 yol oranı
    bool  impacted;         // FIREBOLT: çarptı mı · ICE_NOVA: kırağı bırakıldı mı
    bool  cracked;          // gecikmeli ikinci olay (buz çatlaması) bir kez
    bool  flashed;          // doğum flaşı bir kez tetiklensin (ayrı bayrak:
                            // impacted'la paylaşınca flaş birkaç kare üst üste
                            // ateşleniyordu — sessiz iki kat partikül)
    unsigned seed;
    bool  active;
    float emitAcc;          // partikül besleme birikimi
    // TON KİMLİĞİ: aynı madde farklı büyüde farklı renk taşır. 0 = maddenin
    // kanonik tonu, 1 = SOĞUK aile (mavi/gri), 2 = SICAK aile (kızıl/kum).
    // Olay başına tek indeks yeterli çünkü fx paleti her satırda aynı indeksi
    // aynı aile olarak tanımlıyor — yıldırımın kıvılcımı mavi olurken dumanı
    // da soğuk griye kayıyor, kendiliğinden uyumlu kalıyor.
    int pal;
    // ELEMAN. İlk yazımda `seed` alanına GİZLENİYORDU (0xE1E1+elem) ve iki şeyi
    // birden bozuyordu: (a) seed'in rastgelelik görevi yok oluyordu, (b) yazma
    // yanlış olaya gidiyordu — skillfx_impact "seed'i henüz damgalanmamış ilk
    // aktif isabeti" arıyordu, o da genellikle ESKİ bir isabet oluyordu. Yeni
    // isabet çöp seed ile kalıyor, elem çöpten türeyip 0'a (ÇELİK) düşüyordu.
    // ÇELİK kıvılcım kullanır, kıvılcım hıza göre 4× uzar → ateş patlamasında
    // sivri kamalar: Hakan'ın dört tur bildirdiği "üçgenler" ve "yıldırım
    // yayılımı" tam olarak buydu. Alan açmaktan kaçınmanın bedeli.
    int elem;
} SkillEvent;

// ── TAŞ DİKENİ DOKUSU (kodda basılır, 3 varyant).
// NEDEN GEOMETRİ: buzda çizgi yanlıştı çünkü halka SOYUT bir şekildi; diken ise
// gerçekten KATI BİR NESNE. Partikül bulutu "toz" anlatır, gövde "DİKEN" anlatır
// (Hakan: "toprak dikeni değil toprak tozu fırlatmış gibi"). Kural şu: nesne olan
// şey gövdeyle, olay olan şey partikülle anlatılır.
// Doku ALFA değil RGBA: ışıklı yüz / gölge yüz / koyu kontur baked — tek renk
// tint ile üç yüz elde edilemez, taş da düz renk olursa kağıt gibi duruyor.
#define SFX_SPK_W 26
#define SFX_SPK_H 44
#define SFX_SPK_N 3
static R2DTexture *s_spikeTex = NULL;

static void sf_bake_spikes(R2D *renderer)
{
    static Uint8 px[SFX_SPK_W * SFX_SPK_N * SFX_SPK_H * 4];
    const int W = SFX_SPK_W * SFX_SPK_N;
    // ── HATA GEÇMİŞİ: ilk sürüm TEK KONİK gövdeydi, iki yüzlü (ışıklı/gölge) ve
    // düz kenarlı. Buzdaki "üçgen" sorununun taş versiyonu. Dört düzeltme:
    //  1) ÜÇ YÜZ (aydınlık / ara / gölge): iki yüz düz katlanmış karton gibi
    //     duruyor, üçüncü yüz hacmi getiriyor.
    //  2) ÇENTİKLİ KENAR: genişlik üç frekanslı modülasyonla dalgalanıyor.
    //     Düzgün konik kenar taşta olmaz — taş kırılır, yontulmaz.
    //  3) KIRIK UÇ (varyant 1 ve 2): kusursuz sivri uç yapay duruyor.
    //  4) DİP YAKASI: dikenin KIRDIĞI toprak kabartısı. Yaka olmadan diken
    //     zemine saplanmış gibi değil, üstünde duruyor gibi okunuyor.
    static const float LEAN[SFX_SPK_N] = { 0.00f, 0.14f, -0.18f };
    static const float CHIP[SFX_SPK_N] = { 0.00f, 0.10f,  0.06f };
    static const float WID[SFX_SPK_N]  = { 0.30f, 0.27f,  0.34f };
    for (int c = 0; c < SFX_SPK_N; c++)
    for (int y = 0; y < SFX_SPK_H; y++)
    for (int x = 0; x < SFX_SPK_W; x++) {
        float ux = ((float)x + 0.5f) / SFX_SPK_W;
        float uy = ((float)y + 0.5f) / SFX_SPK_H;
        Uint8 *o = &px[((y * W) + c * SFX_SPK_W + x) * 4];
        o[0] = o[1] = o[2] = o[3] = 0;
        // DİP YAKASI önce: gövdenin altında kalır
        if (uy > 0.88f) {
            float yw = 0.42f * SDL_sqrtf((uy - 0.88f) / 0.12f);
            if (SDL_fabsf(ux - 0.5f) < yw) {
                float k = (uy - 0.88f) / 0.12f;         // dibe doğru koyulaşır
                o[0] = (Uint8)(118 - 34.0f * k);
                o[1] = (Uint8)( 98 - 28.0f * k);
                o[2] = (Uint8)( 76 - 22.0f * k);
                o[3] = (Uint8)(255 - 40.0f * k);
            }
        }
        float t = 1.0f - uy;                            // 0 dip → 1 uç
        if (t > 1.0f - CHIP[c]) continue;               // KIRIK uç
        float hw = WID[c] * SDL_powf(1.0f - t, 0.62f);
        hw *= 1.0f + 0.16f * SDL_sinf(t * 11.0f + (float)c * 2.0f)
                   + 0.09f * SDL_sinf(t * 23.0f + (float)c);
        if (hw < 0.004f) continue;
        float cx = 0.5f + LEAN[c] * t * t;
        float d = (ux - cx) / hw;
        if (SDL_fabsf(d) >= 1.0f) continue;
        float sh = 1.0f - t * 0.14f;                    // uçta hafif kararma
        if (SDL_fabsf(d) > 0.86f) {                     // koyu kontur
            o[0] = 52; o[1] = 44; o[2] = 34; o[3] = 255;
        } else if (d < -0.30f) {                        // AYDINLIK yüz
            o[0] = (Uint8)(178 * sh); o[1] = (Uint8)(160 * sh);
            o[2] = (Uint8)(130 * sh); o[3] = 255;
        } else if (d < 0.18f) {                         // ARA yüz
            o[0] = (Uint8)(140 * sh); o[1] = (Uint8)(124 * sh);
            o[2] = (Uint8)(100 * sh); o[3] = 255;
        } else {                                        // GÖLGE yüz
            o[0] = (Uint8)( 96 * sh); o[1] = (Uint8)( 84 * sh);
            o[2] = (Uint8)( 68 * sh); o[3] = 255;
        }
    }
    s_spikeTex = r2d_texture_from_pixels(renderer, px, W, SFX_SPK_H);
}

// Diken yerini olay tohumundan türet (dizi tutmuyoruz: aynı tohum aynı yer).
static void sf_spike_site(const SkillEvent *e, int idx, float *ox, float *oy,
                          float *odelay, float *oh, int *ovar)
{
    unsigned h = (unsigned)idx * 2654435761u ^ e->seed;
    float a = (float)idx / 14.0f * 6.28318f
            + (float)((h >> 9) & 255) / 255.0f * 0.22f;
    float rr = (46.0f + (float)((h >> 3) & 127) / 127.0f * 104.0f) * e->power;
    *ox = e->x + SDL_cosf(a) * rr;
    *oy = e->y + SDL_sinf(a) * rr * 0.62f;
    *odelay = 0.06f + (rr / (150.0f * e->power)) * 0.22f;
    *oh = 46.0f + (float)((h >> 17) & 63) / 63.0f * 34.0f;
    *ovar = (int)((h >> 25) % SFX_SPK_N);
}

// ── RAFTA: ALEVDEN ASLAN (maske + gövde dokusu). 2026-08-05'te iki tur
// denendi, ikisi de reddedildi:
//   1) yalnız maskeli partikül spawn'ı → ölçü tutmuyor (aslan 168px, fırça
//      12-42px; siluet dört blob genişliğinde) → "havan 2-3 parçalı iniyor gibi"
//   2) kodda basılmış gövde dokusu → "gerçekten aslan çizmeye çalışmışsın ama
//      çok kötü amatör durmuş"
// DERS (kalıcı): prosedürel gövde SOYUT biçimlerde çalışıyor (taş dikeni, buz
// kristali, alev dili); TANINAN BİR CANLIYA benzemekte çalışmıyor — göz canlı
// anatomisindeki en küçük hatayı yakalıyor. Canlı görseli elle çizilmiş asset
// ister. Kod SİLİNMİYOR (boşa çıkan kodu silme kuralı): bir gün elle çizilmiş
// bir aslan sheet'i gelirse maske hâlâ yele/ağız bölgesi verir, ya da soyut bir
// "çağrılan yaratık" silueti gerekirse iskelet burada.
#if 0
// ── ALEVDEN ASLAN SİLUETİ (Hakan kararı 2026-08-05).
// Sprite bir ASLAN çiziyordu; ilk prosedürel karşılık "koşan alev duvarı"ydı ve
// Hakan'ın istediği bu değildi. Adı aslan olan büyü aslan olmalı.
// YÖNTEM: siluet kodda basılır ve partiküller YALNIZ siluetin içinde doğar.
// Maske GPU dokusu DEĞİL — spawn konumu CPU'da seçildiği için bellekte bir
// bayt tablosu yeterli. Kanallar: 1 gövde (kızıl zarf), 2 yele (alev dili,
// dışa savrulur), 3 çekirdek/ağız (en parlak + ışık).
// Üç görünüm: dalga dört yöne gidiyor, yan profil sağ/sol için (sol aynalanır),
// önden aşağı, arkadan yukarı. Tek profille yukarı/aşağı yürüyen aslan yandan
// görünürdü.
#define LION_W 40
#define LION_H 30
#define LION_VIEWS 3      // 0 yan(sağ) · 1 önden · 2 arkadan
typedef struct { Uint8 x, y, ch; } LionCell;
static LionCell s_lion[LION_VIEWS][LION_W * LION_H];
static int s_lionN[LION_VIEWS];
static bool s_lionBaked = false;

static inline float lion_ell(float x, float y, float cx, float cy,
                             float rx, float ry)
{
    float dx = (x - cx) / rx, dy = (y - cy) / ry;
    return dx * dx + dy * dy;
}

static int lion_at(int view, float x, float y)
{
    if (view == 0) {                                  // YAN PROFİL (sağa bakar)
        if (lion_ell(x, y, 0.90f, 0.46f, 0.075f, 0.065f) < 1.0f) return 3;
        bool head = lion_ell(x, y, 0.76f, 0.44f, 0.135f, 0.135f) < 1.0f;
        if (lion_ell(x, y, 0.74f, 0.44f, 0.235f, 0.245f) < 1.0f && !head) return 2;
        if (head) return 3;
        if (lion_ell(x, y, 0.42f, 0.55f, 0.315f, 0.215f) < 1.0f) return 1;
        if (lion_ell(x, y, 0.28f, 0.86f, 0.055f, 0.145f) < 1.0f) return 1;
        if (lion_ell(x, y, 0.56f, 0.86f, 0.055f, 0.145f) < 1.0f) return 1;
        if (lion_ell(x, y, 0.10f, 0.40f, 0.075f, 0.055f) < 1.0f) return 2;  // kuyruk
        return 0;
    }
    if (view == 1) {                                  // ÖNDEN (kameraya doğru)
        if (lion_ell(x, y, 0.50f, 0.50f, 0.085f, 0.062f) < 1.0f) return 3;
        bool head = lion_ell(x, y, 0.50f, 0.43f, 0.175f, 0.165f) < 1.0f;
        if (lion_ell(x, y, 0.50f, 0.42f, 0.315f, 0.295f) < 1.0f && !head) return 2;
        if (head) return 3;
        if (lion_ell(x, y, 0.50f, 0.78f, 0.245f, 0.215f) < 1.0f) return 1;
        if (lion_ell(x, y, 0.33f, 0.93f, 0.055f, 0.095f) < 1.0f) return 1;
        if (lion_ell(x, y, 0.67f, 0.93f, 0.055f, 0.095f) < 1.0f) return 1;
        return 0;
    }
    // ARKADAN (uzaklaşır): ağız yok, yele halkası + sırt
    bool back = lion_ell(x, y, 0.50f, 0.39f, 0.155f, 0.145f) < 1.0f;
    if (lion_ell(x, y, 0.50f, 0.38f, 0.305f, 0.275f) < 1.0f && !back) return 2;
    if (back) return 1;
    if (lion_ell(x, y, 0.50f, 0.74f, 0.265f, 0.245f) < 1.0f) return 1;
    if (lion_ell(x, y, 0.50f, 0.55f, 0.045f, 0.075f) < 1.0f) return 2;   // kuyruk
    return 0;
}

// Dolu hücreleri listeye topla: örnekleme O(1) olsun (reddetme örneklemesi
// yoğun spawn'da boşa tur atıyor).
static void lion_bake(void)
{
    for (int v = 0; v < LION_VIEWS; v++) {
        int n = 0;
        for (int j = 0; j < LION_H; j++)
            for (int i = 0; i < LION_W; i++) {
                int ch = lion_at(v, ((float)i + 0.5f) / LION_W,
                                    ((float)j + 0.5f) / LION_H);
                if (ch == 0) continue;
                s_lion[v][n].x = (Uint8)i;
                s_lion[v][n].y = (Uint8)j;
                s_lion[v][n].ch = (Uint8)ch;
                n++;
            }
        s_lionN[v] = n;
    }
    s_lionBaked = true;
}

// ── ASLAN GÖVDESİ (dokusu kodda basılır).
// HATA GEÇMİŞİ: aslan önce YALNIZ maskeli partikül spawn'ıyla denendi.
// Ölçü tutmuyordu: aslan 168px genişken fırça (partikül) 12-42px, yani siluet
// dört blob genişliğinde kalıyor ve tanınmıyor; üstüne partiküller 30-90px yol
// alıp şekli doğduğu anda dağıtıyor. Hakan'ın tarifi tam isabetliydi: "alev
// topunun büyüğü gibi, havan 2-3 parçalı iniyormuş gibi".
// Kendi kuralımıza dönüldü (bkz. taş dikeni): NESNE olan şey GÖVDEYLE anlatılır,
// olay olan şey partikülle. Koşan aslan bir nesnedir.
// İki katman: KIZIL GÖVDE (ALPHA — hue'yu taşır) + PARLAK ÇEKİRDEK (ADD — baş
// ve ağız, nefesle titrer). Tek katman olsa ya sarıya doyar ya cansız durur.
// Üç görünüm × üç koşu karesi; sanat ızgarası 42×32 art px = 168×128 dünya px.
#define LN_W 42
#define LN_H 32
#define LN_FR 3          // koşu karesi
#define LN_VW 3          // 0 yan(sağ) · 1 önden · 2 arkadan
static R2DTexture *s_lionTex = NULL;

static int lion_zone(int view, float x, float y, int f)
{
    float fw = (f == 0) ? 0.0f : ((f == 1) ? 0.055f : -0.055f);
    if (view == 0) {                       // YAN PROFİL (sağa koşar)
        if (lion_ell(x, y, 0.905f, 0.455f, 0.070f, 0.062f) < 1.0f) return 3;
        bool head = lion_ell(x, y, 0.765f, 0.445f, 0.125f, 0.130f) < 1.0f;
        if (lion_ell(x, y, 0.745f, 0.445f, 0.225f, 0.240f) < 1.0f && !head) return 2;
        if (head) return 3;
        if (lion_ell(x, y, 0.435f, 0.545f, 0.300f, 0.200f) < 1.0f) return 1;
        if (lion_ell(x, y, 0.615f + fw, 0.855f, 0.050f, 0.150f) < 1.0f) return 1;
        if (lion_ell(x, y, 0.285f - fw, 0.855f, 0.050f, 0.150f) < 1.0f) return 1;
        if (lion_ell(x, y, 0.115f, 0.395f, 0.085f, 0.050f) < 1.0f) return 2;
        if (lion_ell(x, y, 0.185f, 0.470f, 0.075f, 0.035f) < 1.0f) return 1;
        return 0;
    }
    if (view == 1) {                       // ÖNDEN (kameraya koşar)
        if (lion_ell(x, y, 0.50f, 0.505f, 0.080f, 0.058f) < 1.0f) return 3;
        bool head = lion_ell(x, y, 0.50f, 0.435f, 0.170f, 0.160f) < 1.0f;
        if (lion_ell(x, y, 0.50f, 0.425f, 0.300f, 0.285f) < 1.0f && !head) return 2;
        if (head) return 3;
        if (lion_ell(x, y, 0.50f, 0.775f, 0.235f, 0.205f) < 1.0f) return 1;
        if (lion_ell(x, y, 0.355f + fw, 0.930f, 0.052f, 0.095f) < 1.0f) return 1;
        if (lion_ell(x, y, 0.645f - fw, 0.930f, 0.052f, 0.095f) < 1.0f) return 1;
        return 0;
    }
    bool back = lion_ell(x, y, 0.50f, 0.395f, 0.150f, 0.140f) < 1.0f;
    if (lion_ell(x, y, 0.50f, 0.385f, 0.295f, 0.270f) < 1.0f && !back) return 2;
    if (back) return 1;
    if (lion_ell(x, y, 0.50f, 0.740f, 0.255f, 0.235f) < 1.0f) return 1;
    if (lion_ell(x, y, 0.50f, 0.560f, 0.042f, 0.072f) < 1.0f) return 2;
    if (lion_ell(x, y, 0.355f + fw, 0.930f, 0.052f, 0.095f) < 1.0f) return 1;
    if (lion_ell(x, y, 0.645f - fw, 0.930f, 0.052f, 0.095f) < 1.0f) return 1;
    return 0;
}

static void sf_bake_lion(R2D *renderer)
{
    const int CELLS = 2 * LN_VW * LN_FR;          // katman × görünüm × kare
    static Uint8 px[2 * LN_VW * LN_FR * LN_W * LN_H * 4];
    const int TW = CELLS * LN_W;
    SDL_memset(px, 0, sizeof px);
    for (int layer = 0; layer < 2; layer++)
    for (int v = 0; v < LN_VW; v++)
    for (int f = 0; f < LN_FR; f++) {
        int cell = layer * (LN_VW * LN_FR) + v * LN_FR + f;
        for (int j = 0; j < LN_H; j++)
        for (int i = 0; i < LN_W; i++) {
            float ux = ((float)i + 0.5f) / LN_W, uy = ((float)j + 0.5f) / LN_H;
            int z = lion_zone(v, ux, uy, f);
            Uint8 *o = &px[((j * TW) + cell * LN_W + i) * 4];
            if (z == 0) continue;
            // KONTUR: komşusu boşsa koyu kenar (pixel-art siluet keskinliği)
            bool edge = false;
            for (int d = 0; d < 4 && !edge; d++) {
                float ex = ux + ((d == 0) ? 1.0f : (d == 1 ? -1.0f : 0.0f)) / LN_W;
                float ey = uy + ((d == 2) ? 1.0f : (d == 3 ? -1.0f : 0.0f)) / LN_H;
                if (lion_zone(v, ex, ey, f) == 0) edge = true;
            }
            if (layer == 1) {
                // ÇEKİRDEK KATMANI: yalnız baş/ağız, additive çizilecek
                if (z != 3 || edge) continue;
                float d2 = (uy < 0.5f) ? 1.0f : 0.82f;
                o[0] = (Uint8)(255 * d2); o[1] = (Uint8)(238 * d2);
                o[2] = (Uint8)(180 * d2); o[3] = 255;
                continue;
            }
            // GÖVDE KATMANI
            float shade = 1.0f - uy * 0.30f;              // üstü aydınlık
            if (edge) { o[0] = 88; o[1] = 20; o[2] = 12; o[3] = 255; continue; }
            if (z == 3) {                                  // baş/ağız
                o[0] = (Uint8)(255 * shade); o[1] = (Uint8)(206 * shade);
                o[2] = (Uint8)(112 * shade);
            } else if (z == 2) {                           // yele
                o[0] = (Uint8)(252 * shade); o[1] = (Uint8)(146 * shade);
                o[2] = (Uint8)( 58 * shade);
            } else {                                       // gövde/bacak/kuyruk
                o[0] = (Uint8)(220 * shade); o[1] = (Uint8)( 70 * shade);
                o[2] = (Uint8)( 38 * shade);
            }
            o[3] = 255;
        }
    }
    s_lionTex = r2d_texture_from_pixels(renderer, px, TW, LN_H);
}

#endif

// ── YERDE KALAN BUZ OLUŞUMLARI (dokusu kodda basılır, 3 varyant).
// NEDEN VAR: üç büyüyü yan yana koyunca buzun eksiği ortaya çıktı —
// toprak yere GERÇEK GÖVDE (diken) bırakıyor, ateş YANAN ZEMİN bırakıyor, buz
// ise yalnız düz bir kırağı lekesi bırakıyordu. Hacim yok (Hakan: "vurduğu
// alanda yerde buz etkileri bırakmalı gibi, bir şeyi eksik").
// Buz saydam olduğu için gövde ALPHA ~205 ve kenarları PARLAK: buzu buz yapan
// şey kütlesi değil kenarındaki ışık. Taş dikeninin tersi — orada ışıklı/gölge
// yüz vardı, burada parlak KENAR + soluk gövde.
#define SFX_ICE_W 28
#define SFX_ICE_H 22
#define SFX_ICE_N 3
static R2DTexture *s_iceTex = NULL;

static void sf_bake_ice(R2D *renderer)
{
    static Uint8 px[SFX_ICE_W * SFX_ICE_N * SFX_ICE_H * 4];
    const int TW = SFX_ICE_W * SFX_ICE_N;
    SDL_memset(px, 0, sizeof px);
    // ── HATA GEÇMİŞİ: ilk sürüm ÜÇ konik plakaydı ve Hakan'ın tarifiyle
    // "üçgen buzlar, çok basit duruyor". Buz kristali tek koni değil KÜME:
    // farklı yükseklikte ALTI plaka, farklı yönlere yatık, bazılarının ucu
    // KIRIK (düz kesik — kusursuz sivri uç yapay duruyor), hepsi ortak bir
    // TABAN BUZUNDAN çıkıyor. Taban olmadan plakalar havada duran kıymıklar
    // gibi okunuyordu.
    // Plaka: {merkez, taban_y, genişlik, yükseklik, yatıklık, kırık_pay}
    static const float CL[SFX_ICE_N][6][6] = {
      {{0.50f,0.98f,0.150f,0.72f, 0.02f,0.00f},{0.30f,0.99f,0.115f,0.46f,-0.16f,0.30f},
       {0.68f,0.99f,0.125f,0.54f, 0.13f,0.00f},{0.16f,1.00f,0.085f,0.24f,-0.10f,0.45f},
       {0.84f,1.00f,0.078f,0.20f, 0.12f,0.00f},{0.42f,1.00f,0.070f,0.30f,-0.05f,0.55f}},
      {{0.44f,0.98f,0.140f,0.62f,-0.14f,0.00f},{0.62f,0.99f,0.158f,0.76f, 0.10f,0.35f},
       {0.26f,0.99f,0.098f,0.34f,-0.08f,0.00f},{0.80f,1.00f,0.090f,0.28f, 0.16f,0.40f},
       {0.52f,1.00f,0.068f,0.22f, 0.04f,0.00f},{0.12f,1.00f,0.062f,0.16f,-0.06f,0.00f}},
      {{0.56f,0.98f,0.170f,0.66f, 0.08f,0.25f},{0.34f,0.99f,0.132f,0.52f,-0.15f,0.00f},
       {0.72f,0.99f,0.104f,0.38f, 0.18f,0.45f},{0.20f,1.00f,0.082f,0.26f,-0.12f,0.00f},
       {0.46f,1.00f,0.072f,0.32f,-0.02f,0.00f},{0.86f,1.00f,0.060f,0.18f, 0.10f,0.30f}},
    };
    for (int c = 0; c < SFX_ICE_N; c++)
    for (int j = 0; j < SFX_ICE_H; j++)
    for (int i = 0; i < SFX_ICE_W; i++) {
        float ux = ((float)i + 0.5f) / SFX_ICE_W;
        float uy = ((float)j + 0.5f) / SFX_ICE_H;
        int best = 0;
        // TABAN BUZU: alt %14, yayvan — plakaları birbirine bağlar
        if (uy > 0.86f) {
            float w = 0.44f * SDL_sqrtf((uy - 0.86f) / 0.14f);
            if (w > 0.02f && SDL_fabsf(ux - 0.5f) / w < 1.0f) best = 2;
        }
        for (int k = 0; k < 6; k++) {
            float cx = CL[c][k][0], base = CL[c][k][1], w = CL[c][k][2];
            float h = CL[c][k][3], lean = CL[c][k][4], chip = CL[c][k][5];
            if (uy < base - h || uy > base) continue;
            float t = (base - uy) / h;                 // 0 taban → 1 uç
            if (chip > 0.0f && t > 1.0f - chip) continue;   // KIRIK uç
            float xx = ux - lean * t * 1.2f;
            float ww = w * (1.0f - t * 0.72f);
            float d = SDL_fabsf(xx - cx) / ww;
            if (d >= 1.0f) continue;
            int v = (d > 0.74f) ? 1 : ((d < 0.26f) ? 3 : 2);
            if (v > best) best = v;
        }
        if (best == 0) continue;
        Uint8 *o = &px[((j * TW) + c * SFX_ICE_W + i) * 4];
        // Buzu buz yapan KENAR IŞIĞI (taş dikeninin tersi: orada ışıklı/gölge
        // yüz vardı, burada parlak kenar + soluk saydam gövde)
        if (best == 1) {          // parlak kenar
            o[0] = 238; o[1] = 253; o[2] = 255; o[3] = 240;
        } else if (best == 3) {   // omurga: kalın buz mavileşir
            o[0] = 122; o[1] = 184; o[2] = 232; o[3] = 210;
        } else {                  // gövde: soluk camgöbeği, saydam
            o[0] = 178; o[1] = 226; o[2] = 247; o[3] = 194;
        }
    }
    s_iceTex = r2d_texture_from_pixels(renderer, px, TW, SFX_ICE_H);
}

// Oluşum yeri olay tohumundan (dizi tutmuyoruz)
static void sf_ice_site(const SkillEvent *e, int idx, float *ox, float *oy,
                        float *orad, float *oh, int *ovar)
{
    unsigned h = (unsigned)idx * 2246822519u ^ (e->seed * 3266489917u);
    float a = (float)idx / 13.0f * 6.28318f
            + (float)((h >> 7) & 255) / 255.0f * 0.30f;
    float rr = (34.0f + (float)((h >> 3) & 127) / 127.0f * 128.0f) * e->power;
    *ox = e->x + SDL_cosf(a) * rr;
    *oy = e->y + SDL_sinf(a) * rr * 0.62f;
    *orad = rr;                      // dalga buraya varınca oluşum başlar
    *oh = 34.0f + (float)((h >> 19) & 63) / 63.0f * 30.0f;
    *ovar = (int)((h >> 26) % SFX_ICE_N);
}

static SkillEvent s_ev[SKILLFX_MAX_EVENT];
static bool s_procedural = false;
static unsigned s_rng = 0x51ED270Bu;

static inline float sf_rnd(void)
{
    s_rng = s_rng * 1664525u + 1013904223u;
    return (float)((s_rng >> 8) & 0xFFFFu) / 65535.0f;
}
static inline float sf_sym(void) { return sf_rnd() * 2.0f - 1.0f; }

// Sanat ızgarasına çivi — prosedürel konumlar sanat pikselinde dursun
static inline float sf_snap(float v) { return SDL_floorf(v / ART_PX) * ART_PX; }

// ── EKRAN DARBESİ durumu (tek örnek: üst üste gelen darbeler EN GÜÇLÜSÜ kalır,
// toplanmaz — toplanınca ekran beyaza yanıyor).
static float s_punchLife = 0.0f, s_punchMax = 0.0f, s_punchStr = 0.0f;
static float s_punchR = 1.0f, s_punchG = 1.0f, s_punchB = 1.0f;

// Darbe istekleri (main tüketir). EN GÜÇLÜ istek kazanır — toplanmıyor:
// aynı karede beş mob vurulunca ekran kilitlenmesin.
static float s_reqStop = 0.0f, s_reqShakeAmp = 0.0f, s_reqShakeDur = 0.0f;

float skillfx_take_hitstop(void)
{
    float v = s_reqStop; s_reqStop = 0.0f; return v;
}
void skillfx_take_shake(float *amp, float *dur)
{
    if (amp) *amp = s_reqShakeAmp;
    if (dur) *dur = s_reqShakeDur;
    s_reqShakeAmp = 0.0f; s_reqShakeDur = 0.0f;
}
static void sf_request(float stop, float amp, float dur)
{
    if (stop > s_reqStop) s_reqStop = stop;
    if (amp  > s_reqShakeAmp) { s_reqShakeAmp = amp; s_reqShakeDur = dur; }
}

void skillfx_punch(float r, float g, float b, float strength, float durSec)
{
    if (!s_procedural) return;
    float cur = (s_punchMax > 0.0f) ? s_punchStr * (s_punchLife / s_punchMax) : 0.0f;
    if (strength <= cur) return;          // daha zayıf darbe mevcudu ezmesin
    s_punchR = r; s_punchG = g; s_punchB = b;
    s_punchStr = strength; s_punchLife = durSec; s_punchMax = durSec;
}

void skillfx_render_punch(R2D *renderer, int winW, int winH)
{
    if (s_punchLife <= 0.0f || s_punchMax <= 0.0f) return;
    float u = s_punchLife / s_punchMax;
    // Kareli sönüm: flaş ANİ doğar, hızlı gider (doğrusal sönüm "yanık" gibi durur)
    float a = s_punchStr * u * u;
    if (a <= 0.004f) return;
    r2d_set_blend(renderer, R2D_BLEND_ADD);
    r2d_set_color(renderer, (Uint8)(s_punchR * 255.0f), (Uint8)(s_punchG * 255.0f),
                  (Uint8)(s_punchB * 255.0f), (Uint8)(a * 255.0f));
    r2d_fill_rect(renderer, &(SDL_FRect){ 0.0f, 0.0f, (float)winW, (float)winH });
    r2d_set_blend(renderer, R2D_BLEND_ALPHA);
    r2d_set_color(renderer, 255, 255, 255, 255);
}

void skillfx_init(void)
{
    s_punchLife = 0.0f; s_punchMax = 0.0f; s_punchStr = 0.0f;
    for (int i = 0; i < SKILLFX_MAX_EVENT; i++) s_ev[i].active = false;
    s_procedural = false;
}

void skillfx_set_procedural(bool on) { s_procedural = on; }
bool skillfx_procedural(void) { return s_procedural; }

// Ömürler: görkem SÜREDEN geliyor — sprite'lar 0.5-0.8 sn oynuyordu,
// prosedürel efektler daha uzun yaşıyor ve evre evre gelişiyor.
static float sf_life(SkillFxKind k)
{
    switch (k) {
        // 1.75 → 2.70: halkalar ~1.4 sn'de bitiyor, kalanı DONMUŞ ZEMİN evresi
        // (oluşumlar ayakta, pus geziniyor, sonunda erime). Yer büyüsünün izi
        // bir an değil bir SÜRE olmalı.
        case SKILLFX_ICE_NOVA:  return 2.70f;
        case SKILLFX_LIGHTNING: return 0.85f;
        case SKILLFX_FIREBOLT:  return 2.20f;   // uçuş + patlama
        // 1.10 → 1.45: son dikenin gecikme+yükselme+duruş+çökme toplamı 0.97'ye
        // dayanıyordu; ömür sınırında olay ölünce gövde ANİDEN kayboluyordu.
        case SKILLFX_SPIKES:    return 1.45f;
        case SKILLFX_IMPACT:    return 0.34f;   // kısa: darbe ANIDIR
        // Dalga 0.70 sn koşuyor (oyun tarafıyla aynı), kalanı yanan iz
        case SKILLFX_FIRE_WAVE: return 1.90f;
        default: return 1.0f;
    }
}

// Olayı kurar ve SLOT'unu döndürür. skillfx_impact'in kendi yarattığı olaya
// ulaşabilmesi için gerekli (arama yapmak yanlış olayı buluyordu).
static int sf_new_event(SkillFxKind kind, float x, float y, float tx, float ty,
                        float power)
{
    int slot = -1;
    for (int i = 0; i < SKILLFX_MAX_EVENT; i++)
        if (!s_ev[i].active) { slot = i; break; }
    if (slot < 0) {
        // Havuz dolu: en az ömrü kalanı ez (yeni olay daha alakalı)
        slot = 0;
        for (int i = 1; i < SKILLFX_MAX_EVENT; i++)
            if (s_ev[i].life < s_ev[slot].life) slot = i;
    }
    float L = sf_life(kind);
    s_ev[slot] = (SkillEvent){
        .kind = kind, .x = x, .y = y, .tx = tx, .ty = ty,
        .life = L, .maxLife = L, .power = (power > 0.05f) ? power : 1.0f,
        .travel = 0.0f, .impacted = false, .cracked = false, .flashed = false,
        .seed = (unsigned)(slot * 2654435761u) ^ s_rng,
        .active = true, .emitAcc = 0.0f, .elem = SKFX_EL_STEEL,
    };
    return slot;
}

void skillfx_cast(SkillFxKind kind, float x, float y, float tx, float ty,
                  float power)
{
    if (!s_procedural) return;
    int slot = sf_new_event(kind, x, y, tx, ty, power);
    // Büyünün TONU: yıldırım elektrik mavisi, taş diken sıcak kum-kızıl,
    // ateş ve buz kendi kanonik tonlarında. Bu ayrımı yapmazsak dört büyü de
    // aynı sarı-turuncu kıvılcım/gri duman ailesine düşüyor ve ekranda
    // "hangi büyüyü attım" okunmuyor (Hakan: "her skill de farklı renk
    // tonları olmalı").
    switch (kind) {
        case SKILLFX_LIGHTNING: s_ev[slot].pal = 1; break;  // soğuk: mavi kıvılcım
        case SKILLFX_SPIKES:    s_ev[slot].pal = 2; break;  // sıcak: kızıl kil + toz
        // Ateş Aslanı KIZIL, Ateş Topu ALTIN: iki ateş büyüsü aynı tonu
        // taşımasın (Hakan: "her skill de farklı renk tonları olmalı")
        case SKILLFX_FIRE_WAVE: s_ev[slot].pal = 2; break;
        default:                s_ev[slot].pal = 0; break;  // ateş altın, buz camgöbeği
    }

    // Doğum anında bükülme: her büyünün kendi imzası
    switch (kind) {
        case SKILLFX_ICE_NOVA:
            // MERCEK + ŞOK birlikte: kubbe şişer VE bir darbe halkası dışa
            // yürür. Tek bükülme cılız duruyordu (Hakan) — görkem katman
            // sayısından geliyor, tek katmanın şiddetinden değil.
            fx_distort_burst(x, y, R2D_DIST_LENS, 215.0f, 12.0f, 0.70f);
            fx_distort_burst(x, y, R2D_DIST_SHOCK, 200.0f, 9.0f, 0.55f);
            break;
        case SKILLFX_LIGHTNING:
            // ŞOK: çarpma halkası, keskin ve hızlı
            fx_distort_burst(tx, ty, R2D_DIST_SHOCK, 210.0f, 9.0f, 0.40f);
            break;
        case SKILLFX_FIREBOLT:
            // ÇIKIŞ AĞZI: fırlatma anında büyücünün elinde kısa ve sert bir
            // parlama + geri tepme dumanı. Mermi bir yerden ÇIKMALI; yoksa
            // "havada belirdi" okunuyor.
            fx_distort_burst(x, y, R2D_DIST_HEAT, 90.0f, 5.0f, 0.22f);
            break;
        case SKILLFX_FIRE_WAVE:
            // ÇIKIŞ: ağızdan/elden fışkırma — sert ısı, kısa
            fx_distort_burst(x, y, R2D_DIST_HEAT, 120.0f, 7.0f, 0.30f);
            break;
        case SKILLFX_SPIKES:
            // YER VURUŞU: geniş ve yavaş şok halkası (boss yer vuruşu da
            // aynı çağrıyı kullanacak — stil paylaşımı sistemin amacı)
            fx_distort_burst(x, y, R2D_DIST_SHOCK, 300.0f, 9.5f, 0.55f);
            break;
        default: break;
    }
}

// ── PARTİKÜL BESLEME + HAREKET
// İsabet tonunu elemandan türet: yıldırım soğuk aile, toprak sıcak aile.
static int sf_elem_pal(int elem)
{
    if (elem == SKFX_EL_BOLT) return 1;
    if (elem == SKFX_EL_EARTH) return 2;
    return 0;
}

void skillfx_impact(float x, float y, float dx, float dy, int elem, float power,
                    bool freeze)
{
    if (!s_procedural) return;
    float len = SDL_sqrtf(dx * dx + dy * dy);
    if (len < 0.01f) { dx = 0.0f; dy = -1.0f; } else { dx /= len; dy /= len; }
    // Yön (tx,ty) alanında taşınıyor: olay havuzunu genişletmeye gerek yok
    int slot = sf_new_event(SKILLFX_IMPACT, x, y, dx, dy, power);
    // KENDİ slotuna yazıyoruz. Arama yapmak (eski hali) yanlış olayı buluyordu.
    s_ev[slot].elem = (elem >= 0 && elem < 7) ? elem : SKFX_EL_STEEL;
    s_ev[slot].pal  = sf_elem_pal(s_ev[slot].elem);
    // Hit-stop izni olayda taşınıyor (cracked alanı bu olay tipinde boş).
    s_ev[slot].cracked = !freeze;      // true = hit-stop BASTIRILDI
}

void skillfx_update(ParticleSystem *fx, float dt)
{
    if (s_punchLife > 0.0f) s_punchLife -= dt;

    for (int i = 0; i < SKILLFX_MAX_EVENT; i++) {
        SkillEvent *e = &s_ev[i];
        if (!e->active) continue;
        e->life -= dt;
        if (e->life <= 0.0f) { e->active = false; continue; }
        float u = 1.0f - e->life / e->maxLife;   // 0 → 1 ilerleme
        // Bu olayın tonu: bundan sonraki TÜM spawn'lar bu aileden doğuyor.
        // Olay başına kurulduğu için olaylar arasında sızma olmuyor.
        // KARIŞIM: tek varyant tek rampa demek ve alev "sarı" okunuyordu
        // (Hakan). Her büyü artık İKİ ton taşıyor; oran maddeye göre:
        //   · ateş topu ALTIN gövde + %42 KIZIL dil (sıcak çekirdek, kızıl uç)
        //   · ateş dalgası KIZIL gövde + %30 altın kalp (aslanın kükreyen ağzı)
        //   · buz CAMGÖBEĞİ + %32 derin mavi (kalın buzun içi koyudur)
        //   · toprak KIZIL KİL + %38 koyu toprak (tek renk kaya plastik durur)
        //   · yıldırım mavi-beyaz + %25 kanonik (çakma anı beyaza kaçsın)
        switch (e->kind) {
            case SKILLFX_FIREBOLT:  fx_set_pal_mix(0, 2, 0.42f); break;
            // Alev Seli ATEŞ TOPUYLA AYNI KARIŞIM (Hakan: "ateş topunun
            // renkleri daha doğru, ondaki renkleri alev seline uygulasan
            // olmaz mı?"). İki büyü aynı paleti kullanıyordu ama oranlar
            // TERSTİ: ateş topu %58 sıcak rampa, sel %72 SOĞUK rampa. Farkı
            // yaratan buydu — renk tablosu değil, karışım.
            // Tonal ayrım artık karışımdan değil KOYU SİLUET katmanından
            // geliyor (sel onu taşıyor, ateş topu taşımıyor); iki büyü aynı
            // ateşten ama biri akan sel, öteki patlama.
            case SKILLFX_FIRE_WAVE: fx_set_pal_mix(0, 2, 0.42f); break;
            case SKILLFX_ICE_NOVA:  fx_set_pal_mix(0, 1, 0.32f); break;
            case SKILLFX_SPIKES:    fx_set_pal_mix(2, 0, 0.38f); break;
            case SKILLFX_LIGHTNING: fx_set_pal_mix(1, 0, 0.25f); break;
            default:                fx_set_pal_variant(e->pal); break;
        }

        if (e->kind == SKILLFX_ICE_NOVA) {
            // ── TOPLANMA (ömrün ilk %13'ü): parçacıklar DIŞTAN İÇE akıyor,
            // merkeze yaklaştıkça hızlanıyor. Oyun hissinin en güçlü öğesi
            // budur: patlamadan önce bir HAZIRLIK olması, patlamanın "bir
            // yerden geldiği" duygusunu veriyor. Toplanma olmadan efekt
            // "birden belirdi" okunuyor — sebep-sonuç kaybı.
            if (u < 0.13f) {
                int n = (int)(90.0f * dt / 0.016f);
                for (int k = 0; k < n; k++) {
                    float a = sf_rnd() * 6.28318f;
                    float rr = 120.0f + sf_rnd() * 130.0f;
                    float px = e->x + SDL_cosf(a) * rr;
                    float py = e->y + SDL_sinf(a) * rr * 0.62f;
                    // Hız MERKEZE doğru; ömür kısa (merkeze varınca ölür)
                    fx_spawn(fx, px, py,
                             -SDL_cosf(a) * (rr * 3.4f), -SDL_sinf(a) * (rr * 2.1f),
                             0.24f + sf_rnd() * 0.10f, 7.0f + sf_rnd() * 7.0f,
                             225, 245, 255, WEAPON_FX_ICE);
                }
                continue;   // toplanma bitene kadar halkalar başlamaz
            }
            // ÜÇ HALKA DALGASI, gecikmeli — tek halka cılız duruyordu.
            // Her dalga kendi zamanında doğar, kendi hızında genişler:
            // ön dalga hızlı ve ince, arkadakiler yavaş ve kalın.
            // Toplanma sonrası zaman: halkalar u=0.13'te sıfırdan başlar
            float ub = (u - 0.13f) / 0.87f;
            for (int w = 0; w < 3; w++) {
                float wu = ub - (float)w * 0.13f;           // dalga gecikmesi
                if (wu <= 0.0f || wu >= 1.0f) continue;
                // Alan 330 → 190: dar alanda AYNI partikül sayısı = yoğun
                // buz duvarı. Geniş halkada partiküller seyreliyor ve
                // "birkaç nokta" gibi duruyordu (Hakan).
                float R = 26.0f + (190.0f - (float)w * 34.0f) * wu * e->power;
                // KALIN BANT: halka artık çizgiyle çizilmediği için dalganın
                // yerini KÜTLE anlatmak zorunda. Bant kalınlığı ±16 → ±30 ve
                // sayı 2.3× arttı; kristaller de büyüdü. İnce bantta partiküller
                // "birkaç nokta", kalın bantta BUZ DUVARI okunuyor.
                // ── BÜTÇE (kararlı hal = kare_spawn × ömür × 62.5).
                // ÖLÇÜM: bant 600/kare × 1.125sn = 42.188 partikül istiyordu ve
                // havuz 8192. Mızrak + iç alan ile toplam istek 124.900, yani
                // 15 KAT AŞIM — partiküllerin %93'ü sessizce düşüyordu. Bu
                // "çemberin içi boş" görüntüsünün de sebebi: havuz cephede
                // doluyor, iç alana hiç sıra gelmiyordu.
                // Bütçe kuralını Alev Seli'ne uygulamış, NOVAYA UYGULAMAMIŞIM.
                // Nova payı ~6400 (alan büyüsü hak ediyor, gerisi diğerlerine).
                int n = (int)((17.0f - (float)w * 5.0f) * dt / 0.016f
                              * (1.0f - wu * 0.35f));
                for (int k = 0; k < n; k++) {
                    float a = sf_rnd() * 6.28318f;
                    float rr = R + sf_sym() * 30.0f;
                    // ÇİVİLEME YOK. sf_snap sanat ızgarası disiplini ama YOĞUN
                    // bulutta yüzlerce kristali 4px kafese oturtuyor ve göz
                    // "kare kare, sahte" görüyor (Hakan). Tek tek duran nesne
                    // çivilenir (diken gövdesi), kalabalık çivilenmez.
                    float px = e->x + SDL_cosf(a) * rr;
                    float py = e->y + SDL_sinf(a) * rr * 0.62f;
                    fx_spawn(fx, px, py,
                             SDL_cosf(a) * 70.0f, SDL_sinf(a) * 42.0f - 46.0f,
                             0.70f + sf_rnd() * 0.85f, 12.0f + sf_rnd() * 16.0f,
                             (Uint8)(200 + sf_rnd() * 55), 232, 255, WEAPON_FX_ICE);
                }
                // ── DOLU DİSK. HATA: yalnız BANT dolduruluyordu (R ± 30) ve
                // çemberin içi BOŞ kalıyordu — Hakan: "çember içerisinde boşluk
                // olmadan, skill efekti sırasında çemberin içi dolu olmalı".
                // Alan büyüsü halka değil ALAN: cephenin gerisi de donmuş
                // olmalı. Bant en parlak KENAR olarak kalıyor (dalganın yeri
                // okunsun), iç alan daha seyrek ve daha alçak dolduruluyor.
                // Yarıçap sqrt ile seçiliyor: düzgün ALAN dağılımı (doğrusal
                // seçim merkeze yığıyor).
                {
                    int nf = (int)((17.0f - (float)w * 5.0f) * dt / 0.016f
                                   * (1.0f - wu * 0.30f));
                    for (int k = 0; k < nf; k++) {
                        float a = sf_rnd() * 6.28318f;
                        float rr = R * SDL_sqrtf(sf_rnd());
                        // MERKEZ AÇIKLIĞI: büyücü kaynağın KENDİSİ, buz onun
                        // ETRAFINDA büyür. Açıklık olmadan iç dolgu doğrudan
                        // gövdesinin üstünde doğuyor ve derinlik bölmesine
                        // rağmen siluetini yiyor.
                        if (rr < 22.0f) continue;
                        float px = e->x + SDL_cosf(a) * rr;
                        float py = e->y + SDL_sinf(a) * rr * 0.62f;
                        // İç alan ALÇAK ve yavaş: kenar kabarır, iç donar
                        fx_spawn(fx, px, py,
                                 SDL_cosf(a) * 22.0f, -26.0f - sf_rnd() * 34.0f,
                                 0.55f + sf_rnd() * 0.65f,
                                 9.0f + sf_rnd() * 13.0f,
                                 (Uint8)(206 + sf_rnd() * 49), 236, 255,
                                 WEAPON_FX_ICE);
                        // Zemin pusu: alanın "donmuş yüzeyi"
                        if (sf_rnd() < 0.30f)
                            fx_spawn(fx, px, py,
                                     sf_sym() * 10.0f, -8.0f - sf_rnd() * 8.0f,
                                     1.1f + sf_rnd() * 0.8f,
                                     14.0f + sf_rnd() * 12.0f,
                                     234, 248, 255, WEAPON_FX_STEAM);
                    }
                }
                // KRİSTAL MIZRAKLAR: eskiden 36 ADD ÇİZGİ ile çizilen radyal
                // ışınlar, şimdi 9 azimutta yoğun kristal kümesi. Küme dalga
                // cephesinde yerden yukarı fışkırıyor — "yerden çıkan buz".
                for (int sp2 = 0; sp2 < 9; sp2++) {
                    float a = (float)sp2 / 9.0f * 6.28318f
                            + (float)w * 0.23f + 0.11f;
                    // 46 → 1: 9 azimut × 3 dalga = 27/kare (eskiden 1242!)
                    int m = (int)(1.0f * dt / 0.016f + 0.5f);
                    for (int k = 0; k < m; k++) {
                        float t2 = sf_rnd();                  // mızrak boyunca
                        float rr = R * (0.55f + 0.45f * t2) + sf_sym() * 9.0f;
                        float aa = a + sf_sym() * 0.10f;
                        fx_spawn(fx, e->x + SDL_cosf(aa) * rr,
                                 e->y + SDL_sinf(aa) * rr * 0.62f,
                                 SDL_cosf(aa) * 40.0f,
                                 -170.0f - sf_rnd() * 150.0f + t2 * 60.0f,
                                 0.55f + sf_rnd() * 0.55f,
                                 14.0f + sf_rnd() * 14.0f,
                                 236, 250, 255, WEAPON_FX_ICE);
                    }
                }
            }
            // DOĞUM FLAŞI: ilk 0.12'de merkezden dışa 70 kırık — patlama hissi
            if (!e->flashed) {
                e->flashed = true;
                for (int k = 0; k < 520; k++) {
                    float a = sf_rnd() * 6.28318f;
                    float sp = 90.0f + sf_rnd() * 170.0f;   // daralan alan
                    fx_spawn(fx, e->x, e->y,
                             SDL_cosf(a) * sp, SDL_sinf(a) * sp * 0.55f - 60.0f,
                             0.7f + sf_rnd() * 0.8f, 11.0f + sf_rnd() * 11.0f,
                             235, 246, 255, WEAPON_FX_ICE);
                }
                skillfx_punch(0.62f, 0.82f, 1.0f, 0.30f, 0.16f);   // soğuk flaş
                // Yukarı fışkıran buz sütunu (dikey okuma: alan büyüsü hissi)
                for (int k = 0; k < 170; k++)
                    fx_spawn(fx, e->x + sf_sym() * 22.0f, e->y + sf_sym() * 12.0f,
                             sf_sym() * 34.0f, -210.0f - sf_rnd() * 170.0f,
                             0.8f + sf_rnd() * 0.6f, 9.0f + sf_rnd() * 10.0f,
                             215, 240, 255, WEAPON_FX_ICE);
            }
            // ── KALICI KIRAĞI PUSU: halkalar geçtikten sonra donmuş zeminden
            // ağır, yavaş bir pus kalkıyor ve ömür sonuna kadar sürüyor.
            if (u > 0.55f) {
                float rem = (1.0f - u) / 0.45f;
                if (sf_rnd() < 26.0f * rem * dt) {
                    float a = sf_rnd() * 6.28318f;
                    float rr = sf_rnd() * 160.0f * e->power;
                    fx_spawn(fx, e->x + SDL_cosf(a) * rr,
                             e->y + SDL_sinf(a) * rr * 0.62f,
                             sf_sym() * 6.0f, -8.0f - sf_rnd() * 6.0f,
                             2.0f + sf_rnd() * 1.2f, 15.0f + sf_rnd() * 9.0f,
                             222, 240, 255, WEAPON_FX_STEAM);
                }
            }
            // Merkezde soğuk buhar — daha yoğun ve uzun
            // 46 → 5/kare: merkez buharı 1.8sn yaşıyor, 46/kare = 5175 partikül
            for (int k = 0; k < (int)(5.0f * dt / 0.016f + 0.5f); k++)
                fx_spawn(fx, e->x + sf_sym() * 44.0f, e->y + sf_sym() * 24.0f,
                         sf_sym() * 14.0f, -24.0f, 1.8f, 20.0f,
                         218, 238, 255, WEAPON_FX_STEAM);
            // ── DONAN ZEMİN, DALGA İLERLERKEN. Kırağı eskiden TEK SEFERDE,
            // u>0.25'te bırakılıyordu: zemin efektin ortasında bir anda
            // beyazlıyordu ve büyünün geri kalanında hiçbir şey olmuyordu.
            // Hakan: "yere beyaz-buz alanları, kar hissi bırakmalı; skill efekti
            // ESNASINDA da öyle olmalı, sadece sonda değil."
            // Şimdi kırağı dalga cephesinin ARDINA bırakılıyor: zemin dalga
            // geçtikçe, gerçek zamanlı beyazlıyor. Sebep-sonuç görünür oluyor.
            if (u > 0.13f) {
                float ubF = (u - 0.13f) / 0.87f;
                float fR = 26.0f + 190.0f * ubF * e->power;
                // ── BÜTÇE UYARISI: MAX_DECALS 64 ve havuz dolunca EN ESKİYİ
                // eziyor. Kırağıyı "saniyede N leke" diye bırakmak (ilk yazım:
                // 120/sn → nova ömründe ~280 leke) havuzu döver, kendi izini ve
                // başka efektlerin yanık izlerini siler. Bu projede sessiz
                // düşürme üç kez ısırdı; leke MESAFEYE bağlı bırakılıyor:
                // cephe her 22px ilerlediğinde bir leke → sweep boyunca ~9 leke.
                // Toplam nova bütçesi: 9 (kademeli) + 1 (merkez) + 16 (halka) = 26.
                if (fR > e->emitAcc) {
                    e->emitAcc = fR + 15.0f;
                    // Dalga geçtiği yerde ZEMİNİ de kırıyor: donan toprak
                    // çatlayıp kalkıyor. Mesafe kapısı leke ile ortak, yani
                    // parça sayısı da kendiliğinden bütçeli.
                    float ga = sf_rnd() * 6.28318f;
                    groundfx_burst(e->x + SDL_cosf(ga) * fR,
                                   e->y + SDL_sinf(ga) * fR * 0.62f,
                                   2, 0.70f, 0.55f);
                    // Leke ALANA yayılıyor (rim'e değil): çemberin içi de
                    // beyazlamalı. sqrt = düzgün alan dağılımı.
                    for (int q = 0; q < 2; q++) {
                        float a = sf_rnd() * 6.28318f;
                        float rr = fR * SDL_sqrtf(sf_rnd());
                        decal_frost(e->x + SDL_cosf(a) * rr,
                                    e->y + SDL_sinf(a) * rr * 0.62f,
                                    26.0f + sf_rnd() * 18.0f);
                    }
                }
                // KAR HİSSİ: zemine yakın, yavaş sürüklenen beyaz pus + tek tük
                // düşen taneler. Kırağı lekesi "boya", pus + tane "hava soğuk"
                // diyor; ikisi birlikte kar okunuyor.
                if (sf_rnd() < 70.0f * dt) {
                    float a = sf_rnd() * 6.28318f;
                    float rr = sf_rnd() * fR;
                    fx_spawn(fx, e->x + SDL_cosf(a) * rr,
                             e->y + SDL_sinf(a) * rr * 0.62f,
                             SDL_cosf(a) * 16.0f, -4.0f - sf_rnd() * 6.0f,
                             1.4f + sf_rnd() * 1.0f, 13.0f + sf_rnd() * 12.0f,
                             240, 250, 255, WEAPON_FX_STEAM);
                }
                if (sf_rnd() < 55.0f * dt) {
                    float a = sf_rnd() * 6.28318f;
                    float rr = sf_rnd() * fR;
                    fx_spawn(fx, e->x + SDL_cosf(a) * rr,
                             e->y + SDL_sinf(a) * rr * 0.62f - 40.0f
                               - sf_rnd() * 50.0f,
                             sf_sym() * 14.0f, 26.0f + sf_rnd() * 22.0f,
                             1.2f + sf_rnd() * 0.9f, 5.0f + sf_rnd() * 5.0f,
                             246, 252, 255, WEAPON_FX_ICE);
                }
            }
            // Kırağı kalıntısı bir kez (merkez + halka: izin ŞEKLİ)
            if (!e->impacted && u > 0.25f) {
                e->impacted = true;
                // HALKA KALINTISI: tek büyük leke yerine çember boyunca 14
                // küçük kırağı. Tek leke "boya lekesi" gibi duruyor; halka
                // efektin ŞEKLİNİ zeminde bırakıyor, yani iz anlam taşıyor.
                decal_frost(e->x, e->y, 62.0f * e->power);
                for (int k = 0; k < 8; k++) {   // 14 → 8: leke bütçesi (yukarı bkz.)
                    float a = (float)k / 8.0f * 6.28318f + sf_sym() * 0.16f;
                    float rr = 150.0f * e->power;
                    decal_frost(e->x + SDL_cosf(a) * rr,
                                e->y + SDL_sinf(a) * rr * 0.62f,
                                26.0f + sf_rnd() * 16.0f);
                    // Ara halka: tek halka "çember", iki halka ALAN okunuyor
                    decal_frost(e->x + SDL_cosf(a + 0.24f) * rr * 0.62f,
                                e->y + SDL_sinf(a + 0.24f) * rr * 0.62f * 0.62f,
                                22.0f + sf_rnd() * 14.0f);
                }
            }
            // GECİKMELİ ÇATLAMA: halkalar geçtikten sonra donan zemin ÇATLAR.
            // Tek patlama "büyü yapıldı" der; gecikmeli ikinci olay "büyünün
            // ARDI var" der — görkem zamanda katmanlanmaktan da geliyor.
            if (!e->cracked && u > 0.45f) {
                e->cracked = true;
                for (int k = 0; k < 280; k++) {
                    float a = sf_rnd() * 6.28318f;
                    float rr = sf_rnd() * 150.0f * e->power;
                    fx_spawn(fx, e->x + SDL_cosf(a) * rr,
                             e->y + SDL_sinf(a) * rr * 0.62f,
                             SDL_cosf(a) * 40.0f, -150.0f - sf_rnd() * 120.0f,
                             0.6f + sf_rnd() * 0.7f, 10.0f + sf_rnd() * 11.0f,
                             225, 244, 255, WEAPON_FX_ICE);
                }
                fx_distort_burst(e->x, e->y, R2D_DIST_SHOCK, 230.0f, 6.5f, 0.45f);
                skillfx_punch(0.70f, 0.86f, 1.0f, 0.20f, 0.14f);
            }
        }
        else if (e->kind == SKILLFX_LIGHTNING) {
            // Çarpma anında kıvılcım yağmuru + is; sonra sönen art parıltı
            if (!e->impacted) {
                e->impacted = true;
                skillfx_punch(0.92f, 0.96f, 1.0f, 0.52f, 0.13f);   // beyaz çakma
                for (int k = 0; k < 190; k++) {
                    float a = sf_rnd() * 6.28318f;
                    float sp = 120.0f + sf_rnd() * 300.0f;
                    fx_spawn(fx, e->tx, e->ty,
                             SDL_cosf(a) * sp, SDL_sinf(a) * sp * 0.5f - 90.0f,
                             0.5f + sf_rnd() * 0.55f, 3.0f + sf_rnd() * 3.0f,
                             255, 244, 210, WEAPON_FX_SPARK);
                }
                // Zemin patlaması: beyaz-mavi enerji halkası (ADD)
                for (int k = 0; k < 90; k++) {
                    float a = sf_rnd() * 6.28318f;
                    fx_spawn(fx, e->tx, e->ty,
                             SDL_cosf(a) * 240.0f, SDL_sinf(a) * 140.0f,
                             0.4f + sf_rnd() * 0.4f, 9.0f + sf_rnd() * 8.0f,
                             255, 250, 235, WEAPON_FX_PINK);
                }
                for (int k = 0; k < 26; k++)
                    fx_spawn(fx, e->tx + sf_sym() * 22.0f, e->ty - 6.0f,
                             sf_sym() * 16.0f, -30.0f - sf_rnd() * 20.0f,
                             1.5f + sf_rnd(), 13.0f, 78, 74, 84, WEAPON_FX_SMOKE);
                decal_scorch(e->tx, e->ty, 26.0f * e->power);
            }
            // ── ARTIK ÇITIRTI: çarpmadan sonra zeminde tek tük kıvılcım
            // atlıyor (yüklü toprak boşalıyor). Kısa ama karakteristik.
            if (u > 0.20f && sf_rnd() < 16.0f * (1.0f - u) * dt) {
                float a = sf_rnd() * 6.28318f;
                float rr = sf_rnd() * 70.0f;
                fx_spawn(fx, e->tx + SDL_cosf(a) * rr,
                         e->ty + SDL_sinf(a) * rr * 0.62f,
                         sf_sym() * 60.0f, -110.0f - sf_rnd() * 70.0f,
                         0.3f, 3.0f, 255, 250, 235, WEAPON_FX_SPARK);
            }
        }
        else if (e->kind == SKILLFX_IMPACT) {
            if (!e->flashed) {
                e->flashed = true;
                // Eskiden burada `elem > 3 → 0` kırpması vardı. SKFX_EL_BOLT=4,
                // EARTH=5, POISON=6 olduğu için o kırpma yıldırım/toprak/zehir
                // isabetlerini SESSİZCE ÇELİĞE çeviriyordu — üç elemanın imzası
                // hiç çalışmamış.
                int elem = e->elem;
                float dx = e->tx, dy = e->ty;          // darbenin gidiş yönü
                float ang0 = SDL_atan2f(dy, dx);
                // ── YÖNLÜ KONİ: parçacıklar darbenin GİTTİĞİ yöne fırlıyor.
                // Koni açısı 0.75 rad (~43°) — daha geniş olursa yön kaybolur,
                // daha dar olursa "sprey" gibi durur.
                Uint8 pr, pg, pb; WeaponFxType pt;
                float coneMul = 1.0f;
                switch (elem) {
                    case SKFX_EL_ARCANE: pr=235;pg=245;pb=255; pt=WEAPON_FX_PINK;  break;
                    case SKFX_EL_FIRE:   pr=255;pg=200;pb=90;  pt=WEAPON_FX_FLAME; break;
                    case SKFX_EL_ICE:    pr=225;pg=245;pb=255; pt=WEAPON_FX_ICE;   break;
                    case SKFX_EL_BOLT:   pr=255;pg=250;pb=225; pt=WEAPON_FX_SPARK;
                                         coneMul = 1.8f; break;   // yıldırım en yoğun
                    case SKFX_EL_EARTH:  pr=150;pg=132;pb=104; pt=WEAPON_FX_ROCK;  break;
                    case SKFX_EL_POISON: pr=150;pg=210;pb=110; pt=WEAPON_FX_POISON;
                                         coneMul = 0.6f; break;   // zehir sönük ve az
                    default:             pr=255;pg=236;pb=190; pt=WEAPON_FX_SPARK; break;
                }
                int n = (int)((42.0f + 46.0f * e->power) * coneMul);
                for (int k = 0; k < n; k++) {
                    float a = ang0 + sf_sym() * 0.75f;
                    float sp = 150.0f + sf_rnd() * 330.0f * e->power;
                    fx_spawn(fx, e->x, e->y,
                             SDL_cosf(a) * sp, SDL_sinf(a) * sp * 0.62f - 40.0f,
                             0.22f + sf_rnd() * 0.30f, 5.0f + sf_rnd() * 7.0f,
                             pr, pg, pb, pt);
                }
                // GERİ SIÇRAMA: az sayıda parçacık TERS yöne (çarpma sekmesi).
                // Hepsi ileri giderse darbe "üfleme" gibi duruyor; sekme
                // temasın sertliğini anlatıyor.
                for (int k = 0; k < n / 5; k++) {
                    float a = ang0 + 3.14159f + sf_sym() * 0.9f;
                    float sp = 90.0f + sf_rnd() * 150.0f;
                    fx_spawn(fx, e->x, e->y,
                             SDL_cosf(a) * sp, SDL_sinf(a) * sp * 0.6f - 60.0f,
                             0.20f + sf_rnd() * 0.22f, 4.0f + sf_rnd() * 5.0f,
                             pr, pg, pb, pt);
                }
                // ── İKİNCİL KATMAN: her elemanın kendi imzası. Koni ortaktır
                // (darbenin yönü), ikincil katman büyünün KİMLİĞİDİR.
                switch (elem) {
                    case SKFX_EL_FIRE:
                        for (int k = 0; k < 14; k++)   // kor: temas noktasında kalır
                            fx_spawn(fx, e->x + sf_sym() * 12.0f, e->y + sf_sym() * 10.0f,
                                     sf_sym() * 14.0f, sf_sym() * 12.0f,
                                     1.3f + sf_rnd() * 0.8f, 6.0f, 255, 150, 60,
                                     WEAPON_FX_EMBER);
                        for (int k = 0; k < 10; k++)   // duman tüyü: yukarı
                            fx_spawn(fx, e->x + sf_sym() * 10.0f, e->y - 6.0f,
                                     sf_sym() * 12.0f, -40.0f - sf_rnd() * 26.0f,
                                     1.2f + sf_rnd() * 0.7f, 11.0f, 76, 70, 66,
                                     WEAPON_FX_SMOKE);
                        skillfx_punch(1.0f, 0.70f, 0.34f, 0.16f * e->power, 0.10f);
                        break;
                    case SKFX_EL_ICE:
                        for (int k = 0; k < 18; k++)   // kırağı pusu: asılı kalır
                            fx_spawn(fx, e->x + sf_sym() * 16.0f, e->y + sf_sym() * 12.0f,
                                     sf_sym() * 10.0f, -12.0f, 1.1f + sf_rnd() * 0.6f,
                                     13.0f, 226, 242, 255, WEAPON_FX_STEAM);
                        skillfx_punch(0.66f, 0.84f, 1.0f, 0.20f * e->power, 0.09f);
                        break;
                    case SKFX_EL_BOLT: {
                        // KISA DAL KOLLARI: temastan çıkan 3 mini yıldırım
                        // (çizim tarafında yay var; bunlar parçacık kolları)
                        for (int br = 0; br < 3; br++) {
                            float ba = sf_rnd() * 6.28318f;
                            for (int k = 0; k < 10; k++) {
                                float d2 = (float)k * 7.0f;
                                fx_spawn(fx, e->x + SDL_cosf(ba) * d2 + sf_sym() * 5.0f,
                                         e->y + SDL_sinf(ba) * d2 * 0.6f + sf_sym() * 4.0f,
                                         0.0f, 0.0f, 0.16f, 5.0f,
                                         255, 252, 240, WEAPON_FX_PINK);
                            }
                        }
                        skillfx_punch(0.94f, 0.97f, 1.0f, 0.30f * e->power, 0.08f);
                        break;
                    }
                    case SKFX_EL_EARTH:
                        for (int k = 0; k < 16; k++)   // toz: yerden kalkar
                            fx_spawn(fx, e->x + sf_sym() * 18.0f, e->y + 6.0f,
                                     sf_sym() * 18.0f, -16.0f - sf_rnd() * 12.0f,
                                     1.3f + sf_rnd() * 0.8f, 12.0f, 108, 94, 76,
                                     WEAPON_FX_SMOKE);
                        break;                         // toprak flaş vermez
                    case SKFX_EL_POISON:
                        for (int k = 0; k < 12; k++)   // pus: yavaş, aşağı çöker
                            fx_spawn(fx, e->x + sf_sym() * 14.0f, e->y + sf_sym() * 8.0f,
                                     sf_sym() * 8.0f, 6.0f + sf_rnd() * 8.0f,
                                     1.5f + sf_rnd() * 0.8f, 12.0f, 120, 180, 90,
                                     WEAPON_FX_SMOKE);
                        break;
                    case SKFX_EL_ARCANE:
                        skillfx_punch(0.82f, 0.86f, 1.0f, 0.16f * e->power, 0.09f);
                        break;
                    default:                           // çelik: kısa metalik çınlama
                        skillfx_punch(1.0f, 0.94f, 0.80f, 0.10f * e->power, 0.07f);
                        break;
                }
                // ── HİT-STOP ve SARSINTI da ELEMANA GÖRE (Hakan): yıldırım
                // keskin ve kısa, toprak ağır ve uzun, zehir neredeyse yok.
                // Hepsi power ile ölçekli — güçlü vuruş daha çok durdurur.
                {
                    float st, sa, sd;
                    switch (elem) {
                        case SKFX_EL_BOLT:   st=0.090f; sa=7.5f; sd=0.12f; break;
                        case SKFX_EL_EARTH:  st=0.100f; sa=9.5f; sd=0.28f; break;
                        case SKFX_EL_ICE:    st=0.075f; sa=4.0f; sd=0.15f; break;
                        case SKFX_EL_FIRE:   st=0.060f; sa=5.5f; sd=0.17f; break;
                        case SKFX_EL_ARCANE: st=0.050f; sa=3.5f; sd=0.12f; break;
                        case SKFX_EL_POISON: st=0.020f; sa=1.5f; sd=0.08f; break;
                        default:             st=0.045f; sa=3.0f; sd=0.10f; break;
                    }
                    float pw = (e->power < 2.5f) ? e->power : 2.5f;
                    // Bastırılmışsa zaman durmuyor VE sarsıntı kısılıyor: yol
                    // alan bir dalga beş moba değdiğinde kamera beş kez tam
                    // güçle sarsılmamalı, o da "takılma" gibi okunuyor.
                    sf_request(e->cracked ? 0.0f : st * pw,
                               e->cracked ? sa * pw * 0.40f : sa * pw, sd);
                }
                // ZEMİN TEMASI: yalnız AĞIR elemanlar yeri kırar. Zehir ve
                // arcane yeri kırmaz (biri kimyasal, öteki enerji) — eleman
                // ayrımı burada da geçerli.
                if (elem == SKFX_EL_EARTH || elem == SKFX_EL_BOLT ||
                    elem == SKFX_EL_STEEL)
                    groundfx_burst(e->x, e->y,
                                   (elem == SKFX_EL_EARTH) ? 6 : 3,
                                   0.85f * ((e->power < 2.0f) ? e->power : 2.0f),
                                   0.40f);
                // Darbe noktasında kısa ısı/şok bükülmesi (temas sertliği).
                // Toprakta daha geniş ve yavaş: ağır darbe.
                fx_distort_burst(e->x, e->y, R2D_DIST_SHOCK,
                                 (elem == SKFX_EL_EARTH ? 150.0f : 90.0f)
                                 + 70.0f * e->power,
                                 5.0f * e->power,
                                 elem == SKFX_EL_EARTH ? 0.34f : 0.20f);
            }
        }
        else if (e->kind == SKILLFX_SPIKES) {
            // ── YERDEN FIŞKIRAN TAŞ.
            // HATA GEÇMİŞİ: 200 küçük kaya 150px yarıçapa EŞİT DAĞITILMIŞTI ve
            // yerçekimi yoktu. Sonuç: seyrek, havada asılı taneler + baskın toz
            // → Hakan *"parçacıklı yoğun değil, sadece toprak toz efekti gibi"*.
            // DOĞRUSU: taş DİKEN halinde çıkar, yani KÜMELENİR. 14 diken yeri,
            // her birinde yoğun bir kaya sütunu; sütunlar merkezden dışa doğru
            // GECİKMELİ açılıyor (dalga okuması). Kayaya yerçekimi eklendi
            // (fx.c) — fırlıyor, yavaşlıyor, düşüyor.
            // ── HAZIRLIK (ilk %5): zemin TİTRER — çakıllar zıplar, ince toz
            // kalkar. Buz novasının toplanma evresinin toprak karşılığı: diken
            // hiç yoktan belirmemeli, bir yerden GELDİĞİ okunmalı (Hakan:
            // "çıkış-bitiş noktaları garip").
            if (u < 0.05f) {
                int n = (int)(70.0f * dt / 0.016f);
                for (int k = 0; k < n; k++) {
                    float a = sf_rnd() * 6.28318f;
                    float rr = sf_rnd() * 150.0f * e->power;
                    fx_spawn(fx, e->x + SDL_cosf(a) * rr,
                             e->y + SDL_sinf(a) * rr * 0.62f,
                             sf_sym() * 20.0f, -40.0f - sf_rnd() * 50.0f,
                             0.22f + sf_rnd() * 0.12f, 5.0f + sf_rnd() * 5.0f,
                             (Uint8)(120 + sf_rnd() * 30), 98, 76, WEAPON_FX_ROCK);
                }
                if (sf_rnd() < 30.0f * dt)
                    fx_spawn(fx, e->x + sf_sym() * 120.0f, e->y + sf_sym() * 66.0f,
                             sf_sym() * 10.0f, -10.0f, 0.9f, 9.0f,
                             108, 94, 76, WEAPON_FX_SMOKE);
            }
            // DİKEN YERLERİ çizimle ORTAK fonksiyondan geliyor (sf_spike_site):
            // gövde ile partikül ayrı hesaplanırsa toz dikenin yanında patlıyor.
            for (int sIdx = 0; sIdx < 14; sIdx++) {
                float px, py, delay, hgt; int var;
                sf_spike_site(e, sIdx, &px, &py, &delay, &hgt, &var);
                // ── ZEMİN PARÇASI (tek seferlik, çıkış ANINDA). Diken hangi
                // zeminden çıkıyorsa onun DOKUSUNDAN parça savuruyor — çimde
                // çim, kumda kum, kayada taş (Hakan fikri; "basit bir renk değil
                // doku gibi olmalı").
                // TEK SEFERLİK TESPİTİ: siteler durumsuz (tohumdan türüyor), o
                // yüzden "bu kare pencereye GİRDİ mi" diye önceki kareyle
                // karşılaştırılıyor — ayrı bayrak dizisi tutmaya gerek yok.
                float prevU = u - dt / e->maxLife;
                if (u >= delay && prevU < delay) {
                    // DİP: zemin kırılıyor — bol, her yöne, güçlü
                    groundfx_burst(px, py, 5, 1.0f, 0.45f);
                    // UÇ: dikenin YUKARI taşıdığı malzeme, tepeden dökülür.
                    // upBias yüksek: yukarı fırlayıp geri düşsün.
                    groundfx_burst(px, py - hgt * 0.90f, 3, 0.85f, 0.90f);
                }
                // ÇÖKME ANI zemin parçası: gövde geri gömülürken de kırar
                float holdT = delay + 0.13f + 0.34f;
                if (u >= holdT && prevU < holdT)
                    groundfx_burst(px, py, 3, 0.75f, 0.30f);
                // ÇIKIŞ ANI: yalnız gövde yerden çıkarken kırıntı fışkırır.
                // Sürekli kaya sağmak dikeni gizliyordu — kırıntı OLAYIN
                // kendisi değil, olayın ARTIĞI.
                if (u >= delay && u < delay + 0.13f) {
                    // BÜTÇE: ~5.7 diken yeri eşzamanlı yayıyor. 46/kare × 5.7 ×
                    // 1.1sn = 18.000 partikül isteği (havuz 8192) → aşım.
                    // 13/kare ile toplam ~5100. (Kararlı hal = spawn × ömür × 62.5)
                    int n = (int)(13.0f * dt / 0.016f + 0.5f);
                    for (int k = 0; k < n; k++) {
                        float ox = sf_sym() * 13.0f;
                        fx_spawn(fx, sf_snap(px + ox), sf_snap(py + sf_sym() * 6.0f),
                                 ox * 6.0f + sf_sym() * 50.0f,
                                 -210.0f - sf_rnd() * 170.0f,
                                 0.75f + sf_rnd() * 0.5f, 8.0f + sf_rnd() * 10.0f,
                                 (Uint8)(118 + sf_rnd() * 40), 96, 74,
                                 WEAPON_FX_ROCK);
                    }
                    // TABAN TOZU: zemin yarılırken dipte kalkan toz halkası
                    if (sf_rnd() < 55.0f * dt)
                        fx_spawn(fx, px + sf_sym() * 18.0f, py + sf_sym() * 8.0f,
                                 sf_sym() * 34.0f, -12.0f - sf_rnd() * 10.0f,
                                 1.1f + sf_rnd() * 0.8f, 11.0f + sf_rnd() * 8.0f,
                                 104, 90, 74, WEAPON_FX_SMOKE);
                }
                // ÇÖKME ANI: gövde yere gömülürken kırılan parçalar dağılır
                float hold = delay + 0.13f + 0.34f;
                if (u >= hold && u < hold + 0.10f && sf_rnd() < 90.0f * dt) {
                    for (int k = 0; k < 9; k++) {
                        float a2 = sf_rnd() * 6.28318f;
                        fx_spawn(fx, px + sf_sym() * 10.0f, py - hgt * 0.4f,
                                 SDL_cosf(a2) * 110.0f, -70.0f - sf_rnd() * 90.0f,
                                 0.7f + sf_rnd() * 0.5f, 10.0f + sf_rnd() * 12.0f,
                                 (Uint8)(112 + sf_rnd() * 36), 92, 72,
                                 WEAPON_FX_ROCK);
                    }
                }
            }
            if (!e->flashed) {
                e->flashed = true;
                // MERKEZ KIRILMASI: zemin ilk burada açılıyor (dikenler onu
                // takip ediyor) — yoğun, büyük parçalı
                for (int k = 0; k < 150; k++) {
                    float a = sf_rnd() * 6.28318f;
                    float rr = sf_rnd() * 46.0f * e->power;
                    fx_spawn(fx, sf_snap(e->x + SDL_cosf(a) * rr),
                             sf_snap(e->y + SDL_sinf(a) * rr * 0.62f),
                             SDL_cosf(a) * 130.0f, -230.0f - sf_rnd() * 190.0f,
                             0.8f + sf_rnd() * 0.6f, 15.0f + sf_rnd() * 17.0f,
                             (Uint8)(122 + sf_rnd() * 40), 98, 76, WEAPON_FX_ROCK);
                }
                skillfx_punch(0.80f, 0.68f, 0.50f, 0.30f, 0.22f);
                decal_scorch(e->x, e->y, 24.0f * e->power);
                for (int k = 0; k < 12; k++) {
                    float a = (float)k / 12.0f * 6.28318f + sf_sym() * 0.2f;
                    float rr = 110.0f * e->power;
                    decal_scorch(e->x + SDL_cosf(a) * rr,
                                 e->y + SDL_sinf(a) * rr * 0.62f,
                                 14.0f + sf_rnd() * 10.0f);
                }
            }
            // ── İKİNCİ DALGA: dikenler kırılıp MOLOZ olarak yağıyor. Yerçekimi
            // artık işlediği için düşen taş gerçekten düşüyor; bu katman
            // olmadan efekt "yukarı fışkırdı ve bitti" diye kesiliyor.
            if (u > 0.30f && u < 0.72f) {
                int n = (int)(34.0f * dt / 0.016f);
                for (int k = 0; k < n; k++) {
                    float a = sf_rnd() * 6.28318f;
                    float rr = sf_rnd() * 150.0f * e->power;
                    fx_spawn(fx, e->x + SDL_cosf(a) * rr,
                             e->y + SDL_sinf(a) * rr * 0.62f - 60.0f,
                             sf_sym() * 60.0f, -40.0f + sf_rnd() * 40.0f,
                             0.7f + sf_rnd() * 0.5f, 10.0f + sf_rnd() * 12.0f,
                             (Uint8)(112 + sf_rnd() * 36), 92, 72, WEAPON_FX_ROCK);
                }
            }
            // ÇÖKEN TOZ: taş indikten sonra (sayı düşük — toz baskın olmayacak)
            if (u > 0.45f && sf_rnd() < 8.0f * (1.0f - u) * dt)
                fx_spawn(fx, e->x + sf_sym() * 110.0f, e->y + sf_sym() * 60.0f,
                         sf_sym() * 8.0f, 10.0f + sf_rnd() * 8.0f,
                         2.2f, 14.0f, 116, 102, 84, WEAPON_FX_SMOKE);
            if (sf_rnd() < 10.0f * dt)
                fx_spawn(fx, e->x + sf_sym() * 100.0f, e->y + sf_sym() * 56.0f,
                         sf_sym() * 16.0f, -12.0f, 1.4f, 12.0f,
                         112, 98, 80, WEAPON_FX_SMOKE);
        }
        else if (e->kind == SKILLFX_FIREBOLT) {
            float flightT = 0.55f;                 // uçuş süresi
            if (!e->flashed) {
                e->flashed = true;
                // Namlu alevi: hedefe DOĞRU koni (yön mermi yolundan)
                float ang0 = SDL_atan2f(e->ty - e->y, e->tx - e->x);
                for (int k = 0; k < 70; k++) {
                    float a = ang0 + sf_sym() * 0.55f;
                    float sp = 90.0f + sf_rnd() * 210.0f;
                    fx_spawn(fx, e->x, e->y,
                             SDL_cosf(a) * sp, SDL_sinf(a) * sp * 0.6f - 20.0f,
                             0.28f + sf_rnd() * 0.3f, 9.0f + sf_rnd() * 9.0f,
                             255, 205, 95, WEAPON_FX_FLAME);
                }
                for (int k = 0; k < 16; k++)
                    fx_spawn(fx, e->x + sf_sym() * 10.0f, e->y + sf_sym() * 8.0f,
                             sf_sym() * 20.0f, -26.0f - sf_rnd() * 18.0f,
                             1.1f, 11.0f, 86, 80, 74, WEAPON_FX_SMOKE);
                skillfx_punch(1.0f, 0.74f, 0.40f, 0.16f, 0.10f);
            }
            if (e->travel < 1.0f) {
                e->travel += dt / flightT;
                if (e->travel > 1.0f) e->travel = 1.0f;
                float px = e->x + (e->tx - e->x) * e->travel;
                float py = e->y + (e->ty - e->y) * e->travel;
                // Alev kuyruğu: ADD alev + kor, yol boyunca
                int n = (int)(85.0f * dt / 0.016f);
                for (int k = 0; k < n; k++)
                    fx_spawn(fx, sf_snap(px + sf_sym() * 8.0f),
                             sf_snap(py + sf_sym() * 8.0f),
                             sf_sym() * 26.0f, -18.0f + sf_sym() * 20.0f,
                             0.34f + sf_rnd() * 0.3f, 9.0f + sf_rnd() * 8.0f,
                             255, 186, 70, WEAPON_FX_FLAME);
                if (sf_rnd() < 26.0f * dt)
                    fx_spawn(fx, px + sf_sym() * 6.0f, py + sf_sym() * 6.0f,
                             sf_sym() * 10.0f, sf_sym() * 10.0f,
                             1.1f, 5.0f, 255, 150, 60, WEAPON_FX_EMBER);
                // Mermi kızıl bir gövde taşıyor: uçarken de ateş rengi olsun
                if (sf_rnd() < 55.0f * dt)
                    fx_spawn(fx, px + sf_sym() * 5.0f, py + sf_sym() * 5.0f,
                             sf_sym() * 24.0f, -24.0f,
                             0.5f + sf_rnd() * 0.4f, 20.0f + sf_rnd() * 12.0f,
                             226, 84, 48, WEAPON_FX_FLAME_DEEP);
                // Uçarken havayı büker (ısı) — mermi konumunda, kısa ömür
                if (sf_rnd() < 24.0f * dt)
                    fx_distort_burst(px, py, R2D_DIST_HEAT, 70.0f, 3.0f, 0.18f);
            } else if (e->impacted) {
                // ── YANAN ZEMİN (kalıcı iz). Patlamadan sonra ömrün kalanında
                // zeminde küçük alevler ve kor sönerek yanmaya devam ediyor.
                // "Skill bitince etrafta etki bırakması" (Hakan): efekt bir AN
                // değil bir OLAY olur — oyuncu geçtiği yerde ne yaptığını görür.
                float rem = e->life / (e->maxLife - 0.55f);   // 1 → 0
                if (sf_rnd() < 40.0f * rem * dt) {
                    float a = sf_rnd() * 6.28318f;
                    float rr = sf_rnd() * 60.0f * e->power;
                    fx_spawn(fx, e->tx + SDL_cosf(a) * rr,
                             e->ty + SDL_sinf(a) * rr * 0.62f,
                             sf_sym() * 8.0f, -34.0f - sf_rnd() * 22.0f,
                             0.45f + sf_rnd() * 0.35f, 9.0f + sf_rnd() * 7.0f,
                             255, 198, 96, WEAPON_FX_LICK);
                }
                // Yanan zeminin kızıl gövdesi (dil altında duran sıcak kütle)
                if (sf_rnd() < 24.0f * rem * dt) {
                    float a = sf_rnd() * 6.28318f;
                    float rr = sf_rnd() * 58.0f * e->power;
                    fx_spawn(fx, e->tx + SDL_cosf(a) * rr,
                             e->ty + SDL_sinf(a) * rr * 0.62f,
                             sf_sym() * 10.0f, -40.0f,
                             0.40f + sf_rnd() * 0.32f, 18.0f + sf_rnd() * 14.0f,
                             216, 76, 44, WEAPON_FX_FLAME_DEEP);
                }
                if (sf_rnd() < 22.0f * rem * dt) {
                    float a = sf_rnd() * 6.28318f;
                    float rr = sf_rnd() * 55.0f * e->power;
                    fx_spawn(fx, e->tx + SDL_cosf(a) * rr,
                             e->ty + SDL_sinf(a) * rr * 0.62f,
                             0.0f, 0.0f, 1.8f, 6.0f, 255, 150, 60, WEAPON_FX_EMBER);
                }
                if (sf_rnd() < 10.0f * rem * dt)
                    fx_spawn(fx, e->tx + sf_sym() * 40.0f, e->ty + sf_sym() * 22.0f,
                             sf_sym() * 12.0f, -26.0f, 1.9f, 13.0f,
                             78, 72, 68, WEAPON_FX_SMOKE);
                // Isı bükülmesi de sürüyor: yanan zemin havayı ısıtıyor
                if (sf_rnd() < 8.0f * rem * dt)
                    fx_distort_burst(e->tx, e->ty, R2D_DIST_HEAT,
                                     110.0f, 2.6f * rem, 0.30f);
            }
            if (!e->impacted && e->travel >= 1.0f) {
                // ── PATLAMA: görkemin doruğu. Üç dalga partikül + şok halkası
                // + duman kolonu + yanık izi.
                e->impacted = true;
                // ÇİFT KATMAN: her alev için biri KÜÇÜK-HIZLI (çekirdek),
                // biri BÜYÜK-YAVAŞ (hale). Tek boy alev düz bir bulut gibi
                // duruyor; iki ölçek üst üste binince hacim doğuyor.
                for (int k = 0; k < 320; k++) {
                    float a = sf_rnd() * 6.28318f;
                    float sp = 60.0f + sf_rnd() * 340.0f;
                    fx_spawn(fx, e->tx, e->ty,
                             SDL_cosf(a) * sp, SDL_sinf(a) * sp * 0.55f - 40.0f,
                             0.45f + sf_rnd() * 0.75f, 10.0f + sf_rnd() * 12.0f,
                             255, (Uint8)(140 + sf_rnd() * 90), 60, WEAPON_FX_FLAME);
                }
                for (int k = 0; k < 90; k++) {
                    float a = sf_rnd() * 6.28318f;
                    float sp = 30.0f + sf_rnd() * 130.0f;
                    fx_spawn(fx, e->tx, e->ty,
                             SDL_cosf(a) * sp, SDL_sinf(a) * sp * 0.5f - 26.0f,
                             0.9f + sf_rnd() * 0.8f, 26.0f + sf_rnd() * 20.0f,
                             255, 170, 70, WEAPON_FX_FLAME);
                }
                for (int k = 0; k < 70; k++)
                    fx_spawn(fx, e->tx + sf_sym() * 30.0f, e->ty + sf_sym() * 16.0f,
                             sf_sym() * 22.0f, -34.0f - sf_rnd() * 26.0f,
                             2.0f + sf_rnd() * 1.6f, 15.0f + sf_rnd() * 10.0f,
                             74, 68, 64, WEAPON_FX_SMOKE);
                // ALEV DİLLERİ (kıvılcım DEĞİL): 110 kıvılcım "yıldırım
                // çizgisi" gibi okunuyordu çünkü kıvılcım çizik biçimi + hıza
                // göre uzama kullanıyor. Ateşin imzası dil: dibi geniş, ucu
                // ince, yukarı kıvrık. Kıvılcım azaltıldı (gerçek ateş de az
                // kıvılcım atar), yerine is pulları geldi.
                // ── KIZIL ZARF: patlamanın DIŞ GÖVDESİ. Toplamalı çekirdeğin
                // arkasında ALPHA katmanı olarak duruyor; kırmızı ancak burada
                // görünür kalıyor (ADD içinde doyup sarıya kaçıyordu). Büyük,
                // yavaş, uzun ömürlü — dillerle yarışmıyor, onları taşıyor.
                // Hız çekirdeğin (60-260) hemen altında, ömür ONDAN KISA
                // (0.45-0.95 vs dilin 0.55-1.15): zarf çekirdekten önce
                // sönüyor, böylece "kırmızı sonradan geldi" olmuyor.
                for (int k = 0; k < 170; k++) {
                    float a = sf_rnd() * 6.28318f;
                    float sp = 55.0f + sf_rnd() * 185.0f;
                    fx_spawn(fx, e->tx, e->ty,
                             SDL_cosf(a) * sp, SDL_sinf(a) * sp * 0.5f - 95.0f,
                             0.45f + sf_rnd() * 0.5f, 28.0f + sf_rnd() * 26.0f,
                             230, 90, 50, WEAPON_FX_FLAME_DEEP);
                }
                for (int k = 0; k < 120; k++) {
                    float a = sf_rnd() * 6.28318f;
                    float sp = 60.0f + sf_rnd() * 200.0f;
                    fx_spawn(fx, e->tx, e->ty,
                             SDL_cosf(a) * sp, SDL_sinf(a) * sp * 0.45f - 130.0f,
                             0.55f + sf_rnd() * 0.6f, 14.0f + sf_rnd() * 14.0f,
                             255, 210, 110, WEAPON_FX_LICK);
                }
                // ATEŞ KIVILCIM KULLANMIYOR (Hakan iki tur bildirdi:
                // "yıldırım etkisi gibi efektler var"). Sebep tip seçimi:
                // KIVILCIM çizik biçimi taşıyor VE hıza göre 4× uzuyor —
                // patlamadan hızla saçılınca merkezden çıkan parlak ÇİZGİLER
                // oluyor, bu da yıldırımın imzası. Kıvılcım çelik ve yıldırıma
                // ait; ateş KOR fırlatır (nokta biçim, uzama yok). Sayı 26'dan
                // 44'e çıktı çünkü kor daha küçük ve sessiz okunuyor.
                for (int k = 0; k < 44; k++) {
                    float a = sf_rnd() * 6.28318f;
                    float sp = 150.0f + sf_rnd() * 340.0f;
                    fx_spawn(fx, e->tx, e->ty,
                             SDL_cosf(a) * sp, SDL_sinf(a) * sp * 0.5f - 40.0f,
                             0.5f + sf_rnd() * 0.5f, 3.5f + sf_rnd() * 2.5f,
                             255, 190, 90, WEAPON_FX_EMBER);
                }
                for (int k = 0; k < 34; k++) {   // İS PULLARI: yanmanın kalıntısı
                    float a = sf_rnd() * 6.28318f;
                    fx_spawn(fx, e->tx + sf_sym() * 20.0f, e->ty + sf_sym() * 12.0f,
                             SDL_cosf(a) * 90.0f, SDL_sinf(a) * 60.0f - 90.0f,
                             1.6f + sf_rnd() * 1.4f, 6.0f + sf_rnd() * 5.0f,
                             70, 64, 60, WEAPON_FX_SOOT);
                }
                for (int k = 0; k < 40; k++)
                    fx_spawn(fx, e->tx + sf_sym() * 34.0f, e->ty + sf_sym() * 20.0f,
                             0.0f, 0.0f, 2.4f, 7.0f, 255, 150, 60, WEAPON_FX_EMBER);
                // ZEMİN TEMASI: patlama yeri kırıyor — o zeminin dokusundan
                // parça savuruyor (kayada taş, kumda kum). Aynı motor katmanı.
                groundfx_burst(e->tx, e->ty, 10, 1.25f, 0.35f);
                skillfx_punch(1.0f, 0.66f, 0.30f, 0.44f, 0.18f);   // sıcak flaş
                // Çarpma: mermi YÖNÜNDE koni (uçuş vektöründen)
                // Ateş topunun PATLAMASI tek ve kesin bir olay: donma hakkı var.
                skillfx_impact(e->tx, e->ty, e->tx - e->x, e->ty - e->y,
                               SKFX_EL_FIRE, 1.7f, true);
                // İKİNCİL ŞOK: ilk halkanın ardından geniş ve yavaş bir
                // yankı. Tek halka "patladı" der, iki halka "YERİ SARSTI".
                fx_distort_burst(e->tx, e->ty, R2D_DIST_SHOCK,
                                 250.0f * e->power, 11.0f, 0.5f);
                fx_distort_burst(e->tx, e->ty, R2D_DIST_SHOCK,
                                 420.0f * e->power, 6.0f, 0.85f);
                decal_scorch(e->tx, e->ty, 46.0f * e->power);
            }
        }
        else if (e->kind == SKILLFX_FIRE_WAVE) {
            // ── ATEŞ DALGASI (Ateş Aslanı'nın motor karşılığı).
            // Sprite bir ASLAN çiziyordu; prosedürel karşılığı aslan taklidi
            // yapmaya çalışmıyor — kanonu "hat dalgası" ve yapılabileni bir
            // KOŞAN ALEV DUVARI. Taklit edilemeyeni taklit etmek, iyi
            // yapılabileni kaçırmaktan kötüdür.
            const float SPD = 430.0f, RUN = 0.70f;   // oyun tarafıyla AYNI
            float age = e->maxLife - e->life;
            bool running = (age < RUN);
            float dist = SDL_min(age, RUN) * SPD;
            float fx0 = e->x + e->tx * dist;         // cephe konumu
            float fy0 = e->y + e->ty * dist;
            // Cepheye DİK eksen: duvar buna yayılıyor
            float nx = -e->ty, ny = e->tx;
            float HW = 52.0f * e->power;             // yarım genişlik

            if (running) {
                // ── ALEVDEN ASLAN. Partiküller siluetin İÇİNDE doğuyor
                // (bkz. lion_at/lion_bake). Duvar sürümü aşağıda #if 0 içinde
                // gerekçesiyle rafta — kanon değişince kod silinmiyor.
                // ── SARI ALEV SELİ, DÜZ HAT ÜSTÜNDE. Aslan reddedildi;
                // tarif: "düz bir hat üstünde ilerliyor, sarı efekt içerisinde
                // kırmızı siluetler" + "kırmızı ve sarı İÇ İÇE olmalı, ayrı
                // hareket değil" (Hakan).
                // Bu yüzden kırmızı ayrı bir katman DEĞİL: aynı döngüde, AYNI
                // konum ve AYNI hızla doğan bir partikül (WEAPON_FX_FLAME_DARK)
                // ve yalnız çizim sırası farklı (3. geçiş, sarının üstü).
                // Sel yalnız cephede değil, GEÇTİĞİ HATTIN TAMAMINDA yaşıyor —
                // yoksa "uçan top" okunuyor. Yoğunluk cepheye doğru artıyor.
                {
                    // Yan salınım DÜZGÜN mekânsal fonksiyon (hash DEĞİL): su
                    // fazının 2. kuralı — hash düzensizliği komşuları koparıyor
                    // ve sel "kopuk lekeler" oluyor.
                    int n = (int)(150.0f * dt / 0.016f);
                    for (int k = 0; k < n; k++) {
                        // Cepheye yakın daha yoğun (t=1 cephe)
                        float r1 = sf_rnd();
                        float t2 = 1.0f - r1 * r1 * 1.15f;
                        if (t2 < 0.0f) t2 = 0.0f;
                        float along = t2 * dist;
                        float px = e->x + e->tx * along;
                        float py = e->y + e->ty * along;
                        // ── YUVARLAK SİLUET. Hat kalınlığı doğrusal
                        // artıyordu (0.45 + 0.55·t) ve sel dik kenarlı bir kama
                        // gibi kesiliyordu — Hakan: "köşe hatları çizgisel,
                        // daha yuvarlak hatlar olmalı".
                        // Şimdi ELİPTİK zarf: cephede kubbe gibi yuvarlanıyor,
                        // kuyrukta yumuşak sivriliyor. sqrt profili kenarı
                        // yuvarlatan, t² profili kuyruğu toplayan terim.
                        float env = SDL_sqrtf(t2) * (1.0f - 0.30f * t2 * t2);
                        // Cephenin son %12'si kubbe: burun düz kesilmesin
                        if (t2 > 0.88f) env *= SDL_sqrtf(1.0f - (t2 - 0.88f)
                                                          / 0.12f * 0.55f);
                        float wq = (0.30f + 0.85f * env) * HW * 0.62f;
                        float snake = SDL_sinf(along * 0.035f + age * 9.0f) * 0.35f
                                    + SDL_sinf(along * 0.011f - age * 4.0f) * 0.30f;
                        float lat = (sf_sym() * 0.7f + snake) * wq;
                        px += nx * lat; py += ny * lat;
                        // Yükseklik: hattın üstünde alev sütunu
                        // Yükseklik de eliptik: kenarda alçak, ortada yüksek.
                        // Düz dağılım tavanı düz kesiyordu (kutu görüntüsü).
                        float hr = sf_rnd();
                        float hgt = (1.0f - hr * hr) * 46.0f * e->power
                                  * (0.55f + 0.45f * env);
                        py -= hgt;
                        // Ortak hız alanı: ileri + hafif dışa + yukarı
                        float vxq = e->tx * (110.0f + sf_rnd() * 120.0f)
                                  + nx * lat * 1.2f;
                        float vyq = e->ty * (75.0f + sf_rnd() * 80.0f)
                                  - 60.0f - sf_rnd() * 70.0f;
                        // SARI ÇEKİRDEK (toplamalı). Ön uçta ömür kısa, boy küçük.
                        // DÜZELTME: bir önceki yorumda "kısa ömür sıcak kademede
                        // tutar" yazmıştım, YANLIŞTI — palet kademesi ömrün
                        // ORANINDAN seçiliyor (k = (1-life01)*4), yani her
                        // parçacık ömrü ne olursa olsun dört kademeyi de geziyor.
                        // Kısa ömür rengi değiştirmez, yalnız parçacığı çabuk
                        // yok eder. Cephenin gerçekten sıcak olması için varyant
                        // AÇIKÇA kurulmak zorunda (aşağıda burunda yapılıyor).
                        float hot = 0.55f + 0.45f * (1.0f - t2);
                        fx_spawn(fx, px, py, vxq, vyq,
                                 (0.20f + sf_rnd() * 0.20f) * hot,
                                 (11.0f + sf_rnd() * 12.0f)
                                   * (0.80f + 0.30f * t2),
                                 255, 216, 128, WEAPON_FX_FLAME);
                        // KIRMIZI SİLUET: aynı AKIŞ (aynı hat konumu, aynı
                        // salınım, aynı hız alanı) ama sarının ÇEKİRDEĞİNDE
                        // duruyor — Hakan: "kırmızılığı biraz azalt, hacimsel
                        // olarak yükseklik ve genişliği, sarı alanın içerisinde
                        // daha az bulunsun".
                        // Üç koldan kısıldı: pay 0.38 → 0.20, yan yayılım ve
                        // yükseklik %52'ye, boy 18-36 → 13-25. Hız alanı AYNI
                        // kalıyor; "iç içe" özelliği ondan geliyor, yerinden
                        // oynatılırsa kırmızı yine ayrı hareket eder.
                        if (sf_rnd() < 0.20f) {
                            float dlat = lat * 0.52f;
                            float dpx = e->x + e->tx * along + nx * dlat;
                            float dpy = e->y + e->ty * along + ny * dlat
                                      - hgt * 0.52f;
                            fx_spawn(fx, dpx, dpy, vxq * 0.92f, vyq * 0.92f,
                                     0.24f + sf_rnd() * 0.18f,
                                     13.0f + sf_rnd() * 12.0f,
                                     176, 44, 26, WEAPON_FX_FLAME_DARK);
                        }
                        // BURUN: cephenin son %10'u sıkı ve en sıcak. Sel
                        // baştan sona eşit yoğunlukta olunca "boru" gibi
                        // duruyor; burun kabarması ona YÖN veriyor.
                        if (t2 > 0.90f && sf_rnd() < 0.55f) {
                            // Burun SAF SICAK rampada (2700K başlangıç): varyant
                            // geçici olarak kuruluyor, hemen sonra olayın
                            // karışımına dönülüyor.
                            fx_set_pal_variant(0);
                            fx_spawn(fx, px + nx * sf_sym() * 8.0f,
                                     py + ny * sf_sym() * 8.0f,
                                     vxq * 1.15f, vyq * 0.85f,
                                     0.12f + sf_rnd() * 0.10f,
                                     8.0f + sf_rnd() * 8.0f,
                                     255, 220, 140, WEAPON_FX_FLAME);
                            fx_set_pal_mix(0, 2, 0.42f);
                        }
                        // KIZIL ZARF: ateş topunun sıcak HACMİNİ veren katman
                        // selde hiç yoktu (ateş topunda beş yerde var). Sarının
                        // ALTINDA (pass 0), aynı akıştan doğuyor — sel artık
                        // "ince sarı çizgi" değil gövdesi olan bir ateş.
                        if (sf_rnd() < 0.30f)
                            fx_spawn(fx, px, py, vxq * 0.88f, vyq * 0.88f,
                                     0.26f + sf_rnd() * 0.22f,
                                     22.0f + sf_rnd() * 18.0f,
                                     232, 104, 20, WEAPON_FX_FLAME_DEEP);
                        // Alev dili: selin tepesinden yukarı kıvrılan diller
                        if (sf_rnd() < 0.22f)
                            fx_spawn(fx, px, py, vxq * 0.6f, vyq - 60.0f,
                                     0.24f + sf_rnd() * 0.22f,
                                     10.0f + sf_rnd() * 10.0f,
                                     255, 176, 90, WEAPON_FX_LICK);
                    }
                }
                // KOR: pençelerden savrulan tanecikler
                if (sf_rnd() < 70.0f * dt)
                    fx_spawn(fx, fx0 + sf_sym() * 60.0f, fy0 - sf_rnd() * 20.0f,
                             e->tx * 160.0f + sf_sym() * 110.0f,
                             e->ty * 110.0f - 130.0f, 0.7f + sf_rnd() * 0.5f,
                             3.5f + sf_rnd() * 2.0f, 255, 180, 90, WEAPON_FX_EMBER);
                // İS: geride kalan yanma artığı
                if (sf_rnd() < 40.0f * dt)
                    fx_spawn(fx, fx0 + sf_sym() * 60.0f, fy0 - sf_rnd() * 40.0f,
                             -e->tx * 40.0f + sf_sym() * 30.0f, -60.0f - sf_rnd() * 40.0f,
                             1.5f + sf_rnd() * 1.2f, 6.0f + sf_rnd() * 5.0f,
                             70, 64, 60, WEAPON_FX_SOOT);
                // ISI: aslanın kendisi havayı büküyor, onunla yürüyor
                if (sf_rnd() < 30.0f * dt)
                    fx_distort_burst(fx0, fy0 - 40.0f, R2D_DIST_HEAT,
                                     150.0f, 6.0f, 0.22f);
                // İZ: pençe bastığı yer yanıyor
                e->emitAcc += dt;
                if (e->emitAcc > 0.075f) {
                    e->emitAcc = 0.0f;
                    decal_scorch(fx0, fy0, 30.0f * e->power);
                    fx_spawn(fx, fx0 + sf_sym() * HW, fy0 + sf_sym() * 14.0f,
                             sf_sym() * 20.0f, -34.0f - sf_rnd() * 26.0f,
                             1.9f + sf_rnd() * 1.5f, 15.0f + sf_rnd() * 11.0f,
                             78, 72, 68, WEAPON_FX_SMOKE);
                }
#if 0
                // ── RAFTA: KOŞAN ALEV DUVARI (ilk prosedürel karşılık).
                // Teknik olarak çalışıyordu ve yoğundu, ama Hakan'ın istediği
                // bu değildi: "ateş aslanı efekt olarak tam istediğim gibi
                // değil". Kanon "hat dalgası" ama büyünün ADI aslan ve sprite
                // da aslan çiziyordu; siluet yolu seçildi (2026-08-05).
                // Duvar sürümü boss yer dalgası / lav akıntısı gibi bir yerde
                // yeniden işe yarayabilir — o yüzden SİLİNMİYOR.
                // CEPHE: alev dilleri duvar boyunca, dışa doğru hafif yelpaze.
                // 150 → 90: bütçe hesabı (bkz. fx.h MAX_PARTICLES notu)
                int n = (int)(90.0f * dt / 0.016f);
                for (int k = 0; k < n; k++) {
                    float t2 = sf_sym();                     // -1..1 duvar boyu
                    float ex = fx0 + nx * t2 * HW;
                    float ey = fy0 + ny * t2 * HW;
                    // Dil yukarı kıvrılır; yatay bileşen dalga yönünde + yelpaze
                    fx_spawn(fx, sf_snap(ex), sf_snap(ey),
                             e->tx * (90.0f + sf_rnd() * 130.0f) + nx * t2 * 70.0f,
                             e->ty * (60.0f + sf_rnd() * 90.0f) - 120.0f
                                     - sf_rnd() * 90.0f,
                             0.40f + sf_rnd() * 0.45f, 12.0f + sf_rnd() * 12.0f,
                             255, 150, 80, WEAPON_FX_LICK);
                }
                // ÇEKİRDEK: duvarın gövdesi (dil değil kütle) — dil tek başına
                // "saçaklı" duruyor, arkasında kütle olmalı
                n = (int)(70.0f * dt / 0.016f);
                // Duvarın KIZIL gövdesi: alpha katmanı, dillerin arkasında.
                // Ateş Aslanı kızıl tonlu olduğu için burada pay daha yüksek.
                int nd = (int)(65.0f * dt / 0.016f);
                // CEPHEDE doğuyor (eskiden 30px GERİDE doğuyordu ve duvarın
                // arkasında sürünen kırmızı bir kuyruk gibi duruyordu) ve dalga
                // hızının bir payını taşıyor ki cepheyle birlikte yürüsün.
                for (int k = 0; k < nd; k++) {
                    float t2 = sf_sym();
                    fx_spawn(fx, fx0 + nx * t2 * HW, fy0 + ny * t2 * HW,
                             e->tx * (140.0f + sf_rnd() * 90.0f) + nx * t2 * 40.0f,
                             e->ty * (95.0f + sf_rnd() * 60.0f) - 80.0f,
                             0.34f + sf_rnd() * 0.30f, 30.0f + sf_rnd() * 22.0f,
                             228, 80, 46, WEAPON_FX_FLAME_DEEP);
                }
                for (int k = 0; k < n; k++) {
                    float t2 = sf_sym();
                    fx_spawn(fx, fx0 + nx * t2 * HW * 0.85f - e->tx * sf_rnd() * 26.0f,
                             fy0 + ny * t2 * HW * 0.85f - e->ty * sf_rnd() * 26.0f,
                             e->tx * 40.0f + sf_sym() * 40.0f,
                             e->ty * 30.0f - 30.0f + sf_sym() * 30.0f,
                             0.34f + sf_rnd() * 0.30f, 16.0f + sf_rnd() * 14.0f,
                             255, 170, 90, WEAPON_FX_FLAME);
                }
                // KOR: cepheden savrulan tanecikler (kıvılcım DEĞİL — ateş kor atar)
                if (sf_rnd() < 90.0f * dt)
                    fx_spawn(fx, fx0 + nx * sf_sym() * HW, fy0 + ny * sf_sym() * HW,
                             e->tx * 160.0f + sf_sym() * 110.0f,
                             e->ty * 110.0f - 130.0f, 0.7f + sf_rnd() * 0.5f,
                             3.5f + sf_rnd() * 2.0f, 255, 180, 90, WEAPON_FX_EMBER);
                // İS: yanmanın kalıntısı, geride kalır
                if (sf_rnd() < 40.0f * dt)
                    fx_spawn(fx, fx0 + nx * sf_sym() * HW, fy0 + ny * sf_sym() * HW,
                             -e->tx * 40.0f + sf_sym() * 30.0f, -60.0f - sf_rnd() * 40.0f,
                             1.5f + sf_rnd() * 1.2f, 6.0f + sf_rnd() * 5.0f,
                             70, 64, 60, WEAPON_FX_SOOT);
                // ISI: cephe havayı büküyor, cepheyle birlikte YÜRÜYOR
                if (sf_rnd() < 30.0f * dt)
                    fx_distort_burst(fx0, fy0, R2D_DIST_HEAT, 130.0f, 5.0f, 0.22f);
                // İZ: geçtiği yer yanıyor (aralıklı, üst üste binmesin)
                e->emitAcc += dt;
                if (e->emitAcc > 0.075f) {
                    e->emitAcc = 0.0f;
                    decal_scorch(fx0, fy0, 30.0f * e->power);
                    fx_spawn(fx, fx0 + sf_sym() * HW, fy0 + sf_sym() * 14.0f,
                             sf_sym() * 20.0f, -34.0f - sf_rnd() * 26.0f,
                             1.9f + sf_rnd() * 1.5f, 15.0f + sf_rnd() * 11.0f,
                             78, 72, 68, WEAPON_FX_SMOKE);
                }

#endif
            } else if (!e->impacted) {
                // ── ASLAN DAĞILIYOR: yolun sonunda gövde çözülüyor. Aslan
                // "duruyor" değil, kükreyip savruluyor — bu yüzden dağılma
                // YUKARI ve İLERİ, gövde yüksekliği boyunca.
                e->impacted = true;
                for (int k = 0; k < 130; k++) {
                    float a = sf_rnd() * 6.28318f;
                    float hh = sf_rnd() * 110.0f * e->power;
                    fx_spawn(fx, fx0 + sf_sym() * 70.0f * e->power,
                             fy0 - hh,
                             SDL_cosf(a) * 130.0f + e->tx * 70.0f,
                             SDL_sinf(a) * 70.0f - 150.0f,
                             0.5f + sf_rnd() * 0.5f, 14.0f + sf_rnd() * 12.0f,
                             255, 160, 90, WEAPON_FX_LICK);
                }
                // Kızıl gövde de dağılıyor: zarf katmanı olmadan çözülme
                // "sarı bir bulut" oluyor
                for (int k = 0; k < 90; k++) {
                    float a = sf_rnd() * 6.28318f;
                    fx_spawn(fx, fx0 + sf_sym() * 78.0f * e->power,
                             fy0 - sf_rnd() * 110.0f * e->power,
                             SDL_cosf(a) * 90.0f, SDL_sinf(a) * 50.0f - 80.0f,
                             0.40f + sf_rnd() * 0.34f, 26.0f + sf_rnd() * 20.0f,
                             228, 82, 46, WEAPON_FX_FLAME_DEEP);
                }
                skillfx_punch(1.0f, 0.52f, 0.24f, 0.26f, 0.14f);
                fx_distort_burst(fx0, fy0 - 40.0f, R2D_DIST_SHOCK,
                                 220.0f, 7.5f, 0.42f);
            } else {
                // YANAN İZ: dalga geçtikten sonra yol boyunca sönen alev
                float rem = e->life / (e->maxLife - RUN);
                if (sf_rnd() < 55.0f * rem * dt) {
                    float t3 = sf_rnd();                    // yol boyunca
                    float px = e->x + e->tx * (t3 * RUN * SPD);
                    float py = e->y + e->ty * (t3 * RUN * SPD);
                    fx_spawn(fx, px + nx * sf_sym() * HW, py + ny * sf_sym() * HW * 0.6f,
                             sf_sym() * 8.0f, -30.0f - sf_rnd() * 22.0f,
                             0.45f + sf_rnd() * 0.35f, 9.0f + sf_rnd() * 7.0f,
                             255, 170, 90, WEAPON_FX_LICK);
                }
                if (sf_rnd() < 16.0f * rem * dt) {
                    float t3 = sf_rnd();
                    fx_spawn(fx, e->x + e->tx * (t3 * RUN * SPD) + sf_sym() * HW,
                             e->y + e->ty * (t3 * RUN * SPD) + sf_sym() * 16.0f,
                             sf_sym() * 12.0f, -26.0f, 1.9f, 13.0f,
                             78, 72, 68, WEAPON_FX_SMOKE);
                }
            }
        }
    }
    // Ton global'ini bırakırken sıfırla: kamp ateşi, silah izi, hava olayları
    // skillfx'ten SONRA spawn ediyor — onların kendi tonu kalmalı.
    fx_set_pal_variant(0);

}

// ── ÇİZİM. Partiküller fx_render'da çizildiği için burada YALNIZ partikülle
// anlatılamayan geometri var: yıldırım kolu (kırıklı çizgi) ve buz halkası
// (kuantalı çember). Primitifler mevcut r2d ile — yeni pipeline yok.
// bodies: yere ait gövdeler (taş dikeni, buz kümesi) — derinlik bandına tabi.
// air:    havada duran anlık geometri (yıldırım kolu, isabet yayı) — bandı yok.
// Ayrım ŞART: gövdeler iki bant çağrısında çizildiği için, üçüncü bir tam-aralık
// çağrısı onları ÜÇÜNCÜ kez çizerdi (toplamalı katmanda çift parlaklık).
static void sf_draw(R2D *renderer, const Camera *camera,
                    float yMin, float yMax, bool bodies, bool air)
{
    if (!s_procedural) return;
    float z = camera->zoom;

    for (int i = 0; i < SKILLFX_MAX_EVENT; i++) {
        SkillEvent *e = &s_ev[i];
        if (!e->active) continue;
        float u = 1.0f - e->life / e->maxLife;

        // Yıldırım kolu ve isabet yayı HAVADA duran anlık geometri: derinlik
        // bandına girmiyorlar (bir yere ait değiller). Bant çağrılarında
        // atlanıyor, yalnız tam aralıkta çiziliyorlar — yoksa iki kez çizilip
        // toplamalı katmanda iki kat parlıyorlar.
        if (e->kind == SKILLFX_LIGHTNING) {
            if (!air) continue;
            // KOL: gökten hedefe kırıklı yol. Ömrün ilk %35'inde görünür ve
            // her karede yeniden kırılır (yıldırım titrer, sabit durmaz).
            if (u < 0.35f) {
                float fade = 1.0f - u / 0.35f;
                int segs = 11;
                float x0 = e->tx + sf_sym() * 26.0f;
                float y0 = e->ty - 460.0f;
                float px = x0, py = y0;
                r2d_set_blend(renderer, R2D_BLEND_ADD);
                for (int sIdx = 1; sIdx <= segs; sIdx++) {
                    float t2 = (float)sIdx / (float)segs;
                    float nx = x0 + (e->tx - x0) * t2 + sf_sym() * 34.0f * (1.0f - t2);
                    float ny = y0 + (e->ty - y0) * t2;
                    // Üç kat: geniş soluk halo + orta + ince beyaz çekirdek
                    // (görkem: tek çizgi cılız durur, üç kat "ışık" olur)
                    for (int lay = 0; lay < 3; lay++) {
                        Uint8 a = (Uint8)((lay == 0 ? 70.0f : lay == 1 ? 150.0f : 255.0f) * fade);
                        int w = (lay == 0) ? 5 : (lay == 1) ? 3 : 1;
                        r2d_set_color(renderer,
                                      // TON: mor-beyaz. Eskiden (200,215,255)
                                      // mavi-beyazdı ve buz novasının halkasıyla
                                      // neredeyse AYNI renkti — iki büyü ekranda
                                      // ayırt edilemiyordu. Yıldırımın kanonu
                                      // beyaz çekirdek + MOR hâle.
                                      (Uint8)(198 + lay * 30), (Uint8)(172 + lay * 42), 255, a);
                        for (int o = -w / 2; o <= w / 2; o++) {
                            r2d_line(renderer,
                                     (sf_snap(px) + (float)o * ART_PX - camera->x) * z,
                                     (sf_snap(py) - camera->y) * z,
                                     (sf_snap(nx) + (float)o * ART_PX - camera->x) * z,
                                     (sf_snap(ny) - camera->y) * z);
                        }
                    }
                    // DALLANMA: gövdenin ortalarından kopan kısa yan kollar.
                    // Gerçek yıldırım tek çizgi değil AĞAÇTIR; dal olmadan
                    // "beyaz sopa" okunuyor. Dallar daha ince, daha soluk ve
                    // ana kolun akış yönünden SAPARAK gidiyor.
                    if (sIdx >= 3 && sIdx <= segs - 2 && ((sIdx + (int)e->seed) % 3) == 0) {
                        float bl = 46.0f + sf_rnd() * 64.0f;
                        float ba = (sf_rnd() < 0.5f ? -1.0f : 1.0f)
                                 * (0.6f + sf_rnd() * 0.7f);   // sapma açısı
                        float bx = nx, by = ny;
                        for (int bs = 0; bs < 3; bs++) {
                            float nbx = bx + SDL_cosf(ba) * bl * 0.45f + sf_sym() * 10.0f;
                            float nby = by + SDL_sinf(SDL_fabsf(ba)) * bl * 0.30f + bl * 0.34f;
                            Uint8 ba2 = (Uint8)(140.0f * fade * (1.0f - (float)bs * 0.3f));
                            r2d_set_color(renderer, 202, 178, 255, ba2);   // dal da mor aileden
                            for (int o = -1; o <= 1; o++)
                                r2d_line(renderer,
                                    (sf_snap(bx) + (float)o * ART_PX - camera->x) * z,
                                    (sf_snap(by) - camera->y) * z,
                                    (sf_snap(nbx) + (float)o * ART_PX - camera->x) * z,
                                    (sf_snap(nby) - camera->y) * z);
                            bx = nbx; by = nby;
                        }
                    }
                    px = nx; py = ny;
                }
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                r2d_set_color(renderer, 255, 255, 255, 255);
            }
        }
        else if (e->kind == SKILLFX_IMPACT) {
            if (!air) continue;
            // ── DARBE YAYI: temas noktasında, darbeye DİK açılan kısa kavis.
            // Kılıç/çarpma hissinin geometrik imzası: parçacık bulutu "toz"
            // anlatır, yay "KESTİ/ÇARPTI" anlatır. Hızla açılıp söner (0.34 sn
            // ömrün ilk yarısı) — uzun yaşarsa dekor gibi durur.
            if (u < 0.55f) {
                float fade = 1.0f - u / 0.55f;
                float ang0 = SDL_atan2f(e->ty, e->tx);
                float R = (26.0f + 54.0f * e->power) * (0.35f + 0.65f * (u / 0.55f));
                int elem = e->elem;
                // Yayın rengi ELEMANDAN gelir. Önce yalnız arcane ve buz için
                // özel renk vardı, kalan beş eleman aynı altın yayı çiziyordu —
                // "her skill kendi tonu" ilkesinin en görünür ihlali buydu.
                Uint8 cr2 = 255, cg2 = 240, cb2 = 205;      // çelik: altın-beyaz
                switch (elem) {
                    case SKFX_EL_ARCANE: cr2 = 232; cg2 = 206; cb2 = 255; break; // eflatun
                    case SKFX_EL_FIRE:   cr2 = 255; cg2 = 186; cb2 =  96; break; // turuncu
                    case SKFX_EL_ICE:    cr2 = 196; cg2 = 244; cb2 = 252; break; // camgöbeği
                    case SKFX_EL_BOLT:   cr2 = 214; cg2 = 188; cb2 = 255; break; // mor-beyaz
                    case SKFX_EL_EARTH:  cr2 = 226; cg2 = 198; cb2 = 148; break; // kum
                    case SKFX_EL_POISON: cr2 = 186; cg2 = 246; cb2 = 138; break; // asit yeşili
                    default: break;
                }
                r2d_set_blend(renderer, R2D_BLEND_ADD);
                const int SEG = 14;
                for (int lay = 0; lay < 2; lay++) {
                    float rr = R - (float)lay * ART_PX * 1.5f;
                    if (rr < 3.0f) continue;
                    r2d_set_color(renderer, cr2, cg2, cb2,
                                  (Uint8)((lay == 0 ? 120.0f : 240.0f) * fade));
                    for (int sIdx = 0; sIdx < SEG; sIdx++) {
                        // Yay darbeye DİK: merkez açı ang0+90°, genişlik ~150°
                        float sweep = 2.6f;
                        float a0 = ang0 + 1.5708f - sweep * 0.5f
                                 + sweep * (float)sIdx / SEG;
                        float a1 = ang0 + 1.5708f - sweep * 0.5f
                                 + sweep * (float)(sIdx + 1) / SEG;
                        r2d_line(renderer,
                                 (sf_snap(e->x + SDL_cosf(a0) * rr) - camera->x) * z,
                                 (sf_snap(e->y + SDL_sinf(a0) * rr * 0.62f) - camera->y) * z,
                                 (sf_snap(e->x + SDL_cosf(a1) * rr) - camera->x) * z,
                                 (sf_snap(e->y + SDL_sinf(a1) * rr * 0.62f) - camera->y) * z);
                    }
                }
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                r2d_set_color(renderer, 255, 255, 255, 255);
            }
        }
        else if (e->kind == SKILLFX_FIRE_WAVE) {
            // Bu dalda ARTIK GÖVDE ÇİZİLMİYOR. Aslan gövdesi denendi ve
            // reddedildi (Hakan, 2026-08-05): "gerçekten aslan çizmeye
            // çalışmışsın ama çok kötü amatör durmuş. Bu skillin adını ve
            // efektini değiştirelim." Yeni tarif: DÜZ HAT üstünde ilerleyen
            // SARI alev seli, İÇİNDE KIRMIZI SİLUETLER. Sarı sel partikülle
            // (bu dosyanın update'i), kırmızı siluetler skillfx_render_over'da
            // (partikülün ÜSTÜNE, yoksa toplamalı sarı onları yutuyor).
            // Aslan kodu aşağıda #if 0 içinde duruyor — silinmiyor; bir gün
            // "çağrılan ateş yaratığı" gibi bir yerde işe yarayabilir, ama
            // gövde sanatı elle çizilmiş bir asset olmadan amatör duruyor ve
            // DERS bu: prosedürel gövde soyut biçimlerde (diken, kristal)
            // çalışıyor, TANINAN BİR CANLIYA benzemekte çalışmıyor.
#if 0
            // ── ASLAN GÖVDESİ. Partikül siluet çizemiyordu (fırça çok kalın,
            // bkz. sf_bake_lion notu); gövde dokuyla, ateş dokusu partikülle.
            const float SPD = 430.0f, RUN = 0.70f;
            float age2 = e->maxLife - e->life;
            if (age2 > RUN + 0.10f) { /* dağıldı: gövde yok */ }
            else {
                if (s_lionTex == NULL) sf_bake_lion(renderer);
                if (s_lionTex != NULL) {
                    float dist = SDL_min(age2, RUN) * SPD;
                    float lx = e->x + e->tx * dist;
                    float ly = e->y + e->ty * dist;
                    int view; bool mir = false;
                    if (SDL_fabsf(e->tx) >= SDL_fabsf(e->ty)) {
                        view = 0; mir = (e->tx < 0.0f);
                    } else {
                        view = (e->ty > 0.0f) ? 1 : 2;
                    }
                    int fr = (int)(age2 * 15.0f) % LN_FR;
                    float LW2 = 168.0f * e->power, LH2 = 128.0f * e->power;
                    // KOŞU ZIPLAMASI: gövde dikeyde salınır (sabit yüksekte
                    // süzülen aslan "uçuyor" okunuyor)
                    float bob = SDL_sinf(age2 * 21.0f) * 5.0f;
                    // Doğum/dağılma kenarlarında solma: aniden belirmesin
                    float fade = 1.0f;
                    if (age2 < 0.09f) fade = age2 / 0.09f;
                    else if (age2 > RUN) fade = 1.0f - (age2 - RUN) / 0.10f;
                    if (fade < 0.0f) fade = 0.0f;
                    SDL_FRect dst = {
                        (sf_snap(lx - LW2 * 0.5f) - camera->x) * z,
                        (sf_snap(ly - LH2 + 10.0f + bob) - camera->y) * z,
                        LW2 * z, LH2 * z
                    };
                    // GÖVDE (ALPHA): kızıl, hue'yu taşıyan katman
                    int base = view * LN_FR + fr;
                    SDL_FRect src = { (float)(base * LN_W), 0.0f,
                                      (float)LN_W, (float)LN_H };
                    r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                    r2d_texture_set_alpha_mod(s_lionTex, (Uint8)(fade * 255.0f));
                    r2d_texture_set_color_mod(s_lionTex, 255, 255, 255);
                    r2d_draw_texture_flip(renderer, s_lionTex, &src, &dst, mir);
                    // ÇEKİRDEK (ADD): baş/ağız, nefesle titrer — canlı olsun
                    float br = 0.72f + 0.28f * fx_breath(FIRE_HEARTH, e->seed, age2);
                    SDL_FRect src2 = { (float)((LN_VW * LN_FR + base) * LN_W), 0.0f,
                                       (float)LN_W, (float)LN_H };
                    r2d_set_blend(renderer, R2D_BLEND_ADD);
                    r2d_texture_set_alpha_mod(s_lionTex,
                                              (Uint8)(fade * br * 235.0f));
                    r2d_draw_texture_flip(renderer, s_lionTex, &src2, &dst, mir);
                    r2d_texture_set_alpha_mod(s_lionTex, 255);
                    r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                }
            }
        }
#endif
        }
        else if (e->kind == SKILLFX_SPIKES) {
            if (!bodies) continue;
            // ── DİKEN GÖVDELERİ. Yükselme: ease-out (yerden ÇIKAR, süzülmez);
            // tepede kısa duruş; sonra ÇÖKME + moloz. Partikül tozu bunun
            // üstüne biniyor — toz artık ana anlatıcı değil, EŞLİKÇİ.
            if (s_spikeTex == NULL) sf_bake_spikes(renderer);
            if (s_spikeTex != NULL) {
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                for (int sIdx = 0; sIdx < 14; sIdx++) {
                    float sx2, sy2, delay, hgt; int var;
                    sf_spike_site(e, sIdx, &sx2, &sy2, &delay, &hgt, &var);
                    if (sy2 < yMin || sy2 >= yMax) continue;   // derinlik bandı
                    float p2 = (u - delay) / 0.13f;          // yükselme oranı
                    if (p2 <= 0.0f) continue;
                    float hold = delay + 0.13f + 0.34f;
                    float grow, fade = 1.0f;
                    if (p2 < 1.0f) {
                        // AŞMA (overshoot): diken hedef boyunu %12 aşıp geri
                        // oturuyor. Düz ease-out "büyüdü" der, aşma "YERDEN
                        // FIŞKIRDI" der — darbe hissinin ucuz ve güçlü parçası.
                        grow = 1.0f - (1.0f - p2) * (1.0f - p2);
                        grow *= 1.0f + 0.12f * SDL_sinf(p2 * 3.14159f);
                    } else if (u < hold) {
                        grow = 1.0f;
                    } else {
                        // ÇÖKME: gövde yere gömülür ve solar (yok olmaz — çöker)
                        float k2 = (u - hold) / 0.22f;
                        if (k2 >= 1.0f) continue;
                        grow = 1.0f - k2 * 0.75f;
                        fade = 1.0f - k2;
                    }
                    float hh = hgt * grow;
                    if (hh < 3.0f) continue;
                    // Genişlik de yere göre: aynı oranda 14 diken yapay duruyor
                    unsigned wh = (unsigned)sIdx * 2246822519u;
                    float ww = (float)SFX_SPK_W
                             * (1.05f + (float)((wh >> 11) & 63) / 63.0f * 0.45f);
                    SDL_FRect src = { (float)(var * SFX_SPK_W), 0.0f,
                                      (float)SFX_SPK_W, (float)SFX_SPK_H };
                    // TABAN sabit: diken yukarı büyür, ortası sabit kalmaz
                    SDL_FRect dst = {
                        (sf_snap(sx2 - ww * 0.5f) - camera->x) * z,
                        (sf_snap(sy2 - hh) - camera->y) * z,
                        ww * z, hh * z
                    };
                    r2d_texture_set_alpha_mod(s_spikeTex, (Uint8)(fade * 255.0f));
                    // AYNALAMA: 3 kalıp × ayna = 6 görünüm, bedava. 14 diken
                    // üç kalıbı tekrar edince "damga basılmış" okunuyordu.
                    bool smir = (((unsigned)sIdx * 2654435761u) & 0x10000u) != 0;
                    r2d_draw_texture_flip(renderer, s_spikeTex, &src, &dst, smir);
                }
                r2d_texture_set_alpha_mod(s_spikeTex, 255);
            }
        }
        else if (e->kind == SKILLFX_ICE_NOVA) {
            if (!bodies) continue;
            // ── ÇİZİM YOK. HATA GEÇMİŞİ: burada 3 halka × 2 kat × 56 segment
            // ADD çizgi + 12 radyal mızrak × 3 kaydırma çiziliyordu; toplam
            // ~370 parlak çizgi. Sonuç Hakan'ın tarifiyle *"çok çizgisel daire,
            // buz etkisi gibi değil beyaz-mavi çizgiler gibi"*.
            // DERS: BUZ ÇİZGİ DEĞİL KÜTLEDİR. Halkanın yeri artık partikül
            // bandıyla anlatılıyor (bkz. update: kalın bant + kristal mızrak
            // kümeleri). Primitif çizgi, partikülle anlatılamayan geometri için
            // (yıldırım kolu) saklı — buzda o gerekçe yoktu.
            // ── AMA YERDE KALAN OLUŞUM GÖVDEYLE çizilir: buz kütlesi bir NESNE
            // (taş dikeni kuralı). Oluşum, genişleyen dalga kendi yarıçapına
            // VARINCA yerden çıkıyor — buz dalganın ARDINDA donuyor, hepsi
            // birden belirmiyor.
            if (s_iceTex == NULL) sf_bake_ice(renderer);
            if (s_iceTex != NULL) {
                float ubD2 = (u - 0.13f) / 0.87f;
                float frontR = 26.0f + 190.0f * ubD2 * e->power;
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                for (int sIdx = 0; sIdx < 13; sIdx++) {
                    float ix, iy, irad, ih; int ivar;
                    sf_ice_site(e, sIdx, &ix, &iy, &irad, &ih, &ivar);
                    if (iy < yMin || iy >= yMax) continue;      // derinlik bandı
                    if (frontR < irad) continue;               // dalga gelmedi
                    // Yükselme: dalga geçtikten sonra ~0.16 sn ease-out
                    float since = (frontR - irad) / (190.0f * e->power)
                                * (e->maxLife * 0.87f);
                    float grow = since / 0.16f;
                    if (grow > 1.0f) grow = 1.0f;
                    grow = 1.0f - (1.0f - grow) * (1.0f - grow);
                    // ERİME: ömrün son %22'sinde küçülüp solar
                    float fade = 1.0f;
                    if (u > 0.78f) {
                        float m = (u - 0.78f) / 0.22f;
                        fade = 1.0f - m;
                        grow *= (1.0f - m * 0.55f);
                    }
                    float hh = ih * grow;
                    if (hh < 3.0f || fade <= 0.02f) continue;
                    float ww = (float)SFX_ICE_W * 2.1f * (0.75f + 0.25f * grow);
                    SDL_FRect src = { (float)(ivar * SFX_ICE_W), 0.0f,
                                      (float)SFX_ICE_W, (float)SFX_ICE_H };
                    SDL_FRect dst = {
                        (sf_snap(ix - ww * 0.5f) - camera->x) * z,
                        (sf_snap(iy - hh) - camera->y) * z,
                        ww * z, hh * z
                    };
                    r2d_texture_set_alpha_mod(s_iceTex, (Uint8)(fade * 255.0f));
                    r2d_draw_texture(renderer, s_iceTex, &src, &dst);
                }
                r2d_texture_set_alpha_mod(s_iceTex, 255);
            }
        }
    }
}

void skillfx_render(R2D *renderer, const Camera *camera)
{
    sf_draw(renderer, camera, -1.0e9f, 1.0e9f, true, true);
}

void skillfx_render_band(R2D *renderer, const Camera *camera,
                         float yMin, float yMax)
{
    sf_draw(renderer, camera, yMin, yMax, true, false);
}

void skillfx_render_air(R2D *renderer, const Camera *camera)
{
    sf_draw(renderer, camera, -1.0e9f, 1.0e9f, false, true);
}

// ── IŞIK: efekt kendi ışığını taşır. Ateş topu uçarken yeri aydınlatır,
// yıldırım çarpınca sahne bir an beyazlar, buz novası soğuk mavi basar.
int skillfx_append_lights(R2DLight *out, int n, int cap, const Camera *cam,
                          int sw, int sh)
{
    if (!s_procedural) return n;
    for (int i = 0; i < SKILLFX_MAX_EVENT && n < cap; i++) {
        SkillEvent *e = &s_ev[i];
        if (!e->active) continue;
        float u = 1.0f - e->life / e->maxLife;
        float wx, wy, rad, r, g, b, inten;
        if (e->kind == SKILLFX_LIGHTNING) {
            if (u > 0.45f) continue;
            wx = e->tx; wy = e->ty;
            rad = 420.0f; r = 0.85f; g = 0.92f; b = 1.0f;
            inten = 1.15f * (1.0f - u / 0.45f);      // ani parlama, hızlı sönüm
        } else if (e->kind == SKILLFX_ICE_NOVA) {
            wx = e->x; wy = e->y;
            rad = 420.0f; r = 0.50f; g = 0.76f; b = 1.0f;
            // Doğumda FLAŞ (ilk %15'te 1.4'e kadar), sonra sönüm
            inten = (u < 0.15f) ? (0.95f + 0.45f * (1.0f - u / 0.15f))
                                : (1.0f * (1.0f - (u - 0.15f) / 0.85f));
        } else if (e->kind == SKILLFX_IMPACT) {
            if (u > 0.5f) continue;
            int elem = e->elem;
            wx = e->x; wy = e->y;
            rad = 130.0f + 90.0f * e->power;
            r = (elem == 3) ? 0.55f : 1.0f;
            g = (elem == 3) ? 0.80f : 0.84f;
            b = (elem == 3) ? 1.0f  : 0.52f;
            inten = 0.75f * (1.0f - u / 0.5f) * e->power;
        } else if (e->kind == SKILLFX_SPIKES) {
            continue;   // toprak ışık vermez (alev/buz/yıldırım verir)
        } else if (e->kind == SKILLFX_FIRE_WAVE) {
            // Işık CEPHEYLE yürür: duvarın önündeki zemin aydınlanıp geçtikten
            // sonra kararmalı. Sabit noktada bırakılırsa dalga ışığı arkasında
            // bırakıp gidiyor ve "ateş" okuması kopuyor.
            const float SPD = 430.0f, RUN = 0.70f;
            float age = e->maxLife - e->life;
            float dist = SDL_min(age, RUN) * SPD;
            wx = e->x + e->tx * dist; wy = e->y + e->ty * dist;
            rad = 300.0f * e->power;
            // Işık rengi de kara cisim eğrisinden: 2400K = (255,155,61).
            // Elle "kızıl" yazmak (0.48, 0.20) eğri dışıydı ve zemin mor-kızıl
            // aydınlanıyordu.
            r = 1.0f; g = 0.61f; b = 0.24f;      // 2400K
            inten = (age < RUN) ? 1.05f
                  : 0.85f * SDL_max(0.0f, e->life / (e->maxLife - RUN));
        } else {
            float px = e->x + (e->tx - e->x) * e->travel;
            float py = e->y + (e->ty - e->y) * e->travel;
            wx = px; wy = py;
            if (e->impacted) {
                rad = 360.0f; inten = 1.0f * SDL_max(0.0f, 1.0f - (u - 0.25f) / 0.6f);
            } else {
                rad = 190.0f; inten = 0.8f;
            }
            r = 1.0f; g = 0.65f; b = 0.34f;   // 2700K (ateş topu daha sıcak)
        }
        if (inten <= 0.02f) continue;
        n = light_append_point(out, n, cap, cam, sw, sh, wx, wy,
                               rad, r, g, b, inten, LK_MAGIC);
    }
    return n;
}
