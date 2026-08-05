#ifndef FX_H
#define FX_H

#include "r2d.h"
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "common.h"

// HAVUZ TAVANI. 512 → 2560: görkemli skill efektleri istendiğinde (Hakan
// 2026-08-05) tek büyü karede 150+ partikül doğuruyor ve 512'lik havuz ANINDA
// doluyordu — fx_spawn "havuz dolu → atla" diyerek spawn'ların çoğunu SESSİZCE
// düşürüyordu. Yani yoğunluk ne yazılsa aynı kalıyordu; sorun ayarda değil
// KAPASİTEDE. Bütçe: quad tavanı 16384 (r2d.c), partikül başına 1 quad →
// 2560 en kötü durumda %16'sı. Bellek: ~52 bayt × 2560 ≈ 130 KB (statik).
// Tavan tarihi: 512 (sessizce düşürüyordu → "cılız") → 2560 → 5120 → 8192.
// Yoğunluk isteği GPU sınırı değil BU dizi: parçacık simülasyonu CPU'da,
// çizim tek batch halinde GPU'da.
// ÜST SINIR NEREDEN: r2d quad bütçesi 16384 ve tilemap+dekor+varlıklar
// ~2000 quad yiyor. 8192 partikül + dünya ≈ 10.2k quad — güvenli pay kalıyor.
// Bu tavanı aşmak isteyen ÖNCE R2D_MAX_VERTS'i büyütmek zorunda, yoksa r2d
// sessizce quad düşürür (bu projede sessiz düşürme üç kez ısırdı).
// ÖNEMLİ: yayıcı hızları bu bütçeye göre yazılır. Kararlı hal = hız × ömür ×
// 62.5. Ateş Aslanı bir tur boyunca 12.856 partikül istiyordu ve %60'ı
// düşüyordu; yoğunluk kareden kareye değişiyordu. İstenen sayı bütçeye
// SIĞMAK zorunda — düşen partikül "yoğunluk" değil kumar.
#define MAX_PARTICLES 8192

// Silah efekt tipi — silahin item verisinden gelir; ileride run basma bu degeri degistirecek
typedef enum {
    WEAPON_FX_NONE = 0,    // efekt yok (normal silah)
    WEAPON_FX_PINK,        // pembe enerji (+8 tarzi)
    WEAPON_FX_FLAME,       // alev
    WEAPON_FX_ICE,         // buz (mavi-beyaz, agir dusen kristaller)
    WEAPON_FX_POISON,      // zehir (yesil, yukselen kabarciklar) — RÜN
    WEAPON_FX_VENOM,       // İĞNE zehri (koyu mor-yeşil, AŞAĞI damlar) — şarj görseli
    // ATEŞ PAKETİ tipleri. Ad "WeaponFx" ama artık silah dışı kaynaklar da
    // kullanıyor (ocak, kamp ateşi, kandil) — ADLANDIRMA BORCU, tip listesi
    // tek yerde olduğu için bilerek burada tutuldu (yeniden adlandırma 40+
    // çağrı yerini dokunur, kazancı yok).
    WEAPON_FX_SMOKE,       // duman: ALPHA, yükselir, BÜYÜR, soluk grileşir
    WEAPON_FX_SPARK,       // kıvılcım: ADD, küçük ve parlak, yükselip düşer
    WEAPON_FX_STEAM,       // buhar: ALPHA, beyaza yakın, hızlı büyür ve söner
    WEAPON_FX_LICK,        // ALEV DİLİ: ADD, damla biçimi, yukarı kıvrılır.
                           // ATEŞE ÖZGÜ biçim — ateş patlamasında kıvılcım
                           // (çizik + hıza göre uzama) kullanılıyordu ve
                           // YILDIRIM ÇİZGİSİ gibi okunuyordu (Hakan). Alev
                           // çizgi değil DİL: dibi geniş, ucu ince, yukarı kıvrık.
    WEAPON_FX_SOOT,        // İS PULU: ALPHA, koyu, havada savrulur ve düşer —
                           // yanmanın ARDINDA kalan şey (duman değil, kalıntı)
    WEAPON_FX_FLAME_DEEP,  // KIZIL ALEV GÖVDESİ: ALPHA (additive DEĞİL), büyük,
                           // yavaş, uzun ömürlü — alevin DIŞ ZARFI.
                           // NEDEN AYRI TİP: alev parçacıkları toplamalı (ADD)
                           // çiziliyor ve toplamalı karıştırmada 300 parçacık üst
                           // üste binince R önce 255'e doyuyor, sonra G tırmanıyor;
                           // sonuç kaçınılmaz olarak SARI-BEYAZ oluyor. Yani
                           // paleti ne kadar kızıla çeksek de ADD içinde kırmızı
                           // GÖRÜNEMEZ (Hakan: "kırmızılıkları hala göremedim").
                           // Kırmızı, hue'yu koruyan ALPHA katmanında ve parlak
                           // çekirdeğin ARKASINDA olmak zorunda. fx_render zaten
                           // iki geçişli (pass 0 ALPHA, pass 1 ADD) — zarf
                           // kendiliğinden arkada kalıyor. Gerçek ateşin anatomisi
                           // de bu: dışı koyu kızıl, çekirdeği sarı-beyaz.
    WEAPON_FX_FLAME_DARK,  // KOYU KIRMIZI SİLUET: ALPHA ama TOPLAMALI KATMANIN
                           // ÜSTÜNE çizilir (3. geçiş).
                           // NEDEN: Hakan "sarı efekt içinde kırmızı siluetler"
                           // istedi ve "kırmızı ile sarı İÇ İÇE olmalı, ayrı
                           // hareket değil" diye netleştirdi. İki koşul birden:
                           //  (a) kırmızı GÖRÜNMELİ → toplamalı sarının ALTINDA
                           //      kalırsa yutulur, üstüne çizilmek zorunda.
                           //  (b) AYRI HAREKET ETMEMELİ → ayrı bir geometri
                           //      katmanı sarının üstünde kayar ve "iki efekt"
                           //      okunur. Çözüm: aynı yayıcıda, AYNI konum ve
                           //      hızla doğan bir PARTİKÜL tipi olmak; yalnız
                           //      çizim sırası farklı. Fizik aynı kod, akış aynı.
    WEAPON_FX_ROCK,        // TAŞ/KAYA parçası: ALPHA, köşeli, hıza göre uzar.
                           // KENDİ TİPİ olmak zorunda — önce VENOM ödünç
                           // alınmıştı ve o tipe toprak paleti yazılınca
                           // silahın İĞNE ZEHRİ efekti kahverengi çizildi.
                           // Kestirme tip paylaşımı sessiz hata üretir.
    WEAPON_FX_EMBER,       // KOR: ADD, sabit parlar, yavaş söner, yükselmez
                           // (kanon LORE §1.9: seni koruyan şey DURUR).
                           // Genel: ateş dibi koru, magma çatlağı, rün ışıması,
                           // boss aurası zerresi, parlayan göz.
    WEAPON_FX_COUNT,       // sayac (onizleme dongusu icin)
} WeaponFxType;

// Tek bir partikul
typedef struct {
    float x, y;          // dunya konumu
    float vx, vy;        // hiz (px/sn)
    float life;          // kalan omur (sn)
    float maxLife;       // baslangic omru
    float size;          // baslangic boyutu (px, dunya olcegi)
    Uint8 r, g, b;       // renk
    bool active;
    WeaponFxType fxType;   // renk gecisi davranisi icin
    // EKLEMELİ Mİ (VFX borcu #2): partikuller ESKİDEN HEP ALPHA çiziliyordu.
    // Sonucu: alev post-fx'in bloom EŞİĞİNİ (0.74 luma) asla geçemiyor, yani
    // hiç parlamıyor — "ateş sıcak hissettirmiyor" şikâyetinin kökü buydu.
    // ADD'de üst üste binen partikuller toplanıp eşiği aşıyor ve halo doğuyor.
    bool additive;
    // TAKLA: açısal hız (rad/sn). Köşeli parçacıklar (buz kırığı, kaya) havada
    // DÖNER; dönmeyen kırık "yapıştırılmış üçgen" gibi duruyor. Yuvarlak
    // biçimlerde etkisi görünmez, o yüzden yalnız köşelilerde kullanılıyor.
    float spin;
    // Palet varyantı: aynı madde farklı büyüde farklı TON taşır
    // (ateş topu altın, ateş aslanı kızıl, arcane ateş mavi-beyaz).
    Uint8 palVar;
} Particle;

// Hangi efekt tipi EKLEMELİ çizilir: ışık YAYAN şeyler (alev, enerji) ADD;
// madde olan şeyler (zehir damlası, buz kristali, duman) ALPHA. Ateş paketi
// bu tabloya duman/kıvılcım ekleyecek.
static inline bool fx_type_additive(WeaponFxType t)
{
    return t == WEAPON_FX_FLAME || t == WEAPON_FX_PINK
        || t == WEAPON_FX_SPARK || t == WEAPON_FX_EMBER
        || t == WEAPON_FX_LICK;   // is (SOOT) madde → ALPHA
}

// ── KUVVET ALANI (genel motor katmanı). Rüzgâr partikülleri sürükler AMA
// hepsini aynı oranda değil: duman havada asılı olduğu için tamamen savrulur,
// kıvılcım ağır ve hızlı olduğu için az, alev kaynağına bağlı olduğu için
// neredeyse hiç. Bu tablo o duyarlılığı tutuyor.
// Genel kullanım: rüzgâr (hava sistemi), boss'un savurma dalgası, patlama
// itmesi, su akıntısı — hepsi aynı alana yazar.
static inline float fx_type_wind(WeaponFxType t)
{
    switch (t) {
        case WEAPON_FX_SMOKE: return 1.00f;   // tamamen savrulur
        case WEAPON_FX_STEAM: return 0.85f;
        case WEAPON_FX_SPARK: return 0.30f;   // ağır ve hızlı
        case WEAPON_FX_ROCK:  return 0.08f;   // taş rüzgârla savrulmaz
        case WEAPON_FX_LICK:  return 0.22f;   // alev dili kaynağa bağlı
        case WEAPON_FX_SOOT:  return 0.95f;   // is pulu tamamen savrulur
        case WEAPON_FX_EMBER: return 0.05f;   // yerinde durur
        case WEAPON_FX_FLAME: return 0.18f;   // kaynağa bağlı
        case WEAPON_FX_FLAME_DEEP: return 0.20f;
        case WEAPON_FX_FLAME_DARK: return 0.20f;
        default: return 0.35f;
    }
}

// Bu tip ömür boyunca BÜYÜR mü? Duman/buhar genişler (soğuyup yayılır),
// alev/kıvılcım küçülür (yanıp tükenir). Eskiden tek davranış vardı.
// Bu tip TOPLAMALI KATMANIN ÜSTÜNE mi çizilir? (3. geçiş)
static inline bool fx_type_over(WeaponFxType t)
{
    return t == WEAPON_FX_FLAME_DARK;
}
static inline bool fx_type_grows(WeaponFxType t)
{
    // KIZIL ZARF BU AİLEDE DEĞİL — bir tur denendi ve HATALIYDI. `grows` ailesi
    // şeffaf doğup ömrün %25'inde tepe alfaya çıkıyor VE küçük doğup büyüyor.
    // İkisi birleşince zarf, parlak çekirdek söndükten SONRA görünür hale
    // geliyordu: "kırmızılık alt tabaka halinde sonradan geliyor, garip duruyor"
    // (Hakan, ateş topu ve Ateş Aslanı'nda aynı sorun). Ateşin dış gövdesi
    // çekirdekle AYNI ANDA vardır; sonradan gelen şey duman olur, alev olmaz.
    return t == WEAPON_FX_SMOKE || t == WEAPON_FX_STEAM;
}

// ── ATEŞ YAYICI (emitter). Havuz YOK: kaynak konumları dünyada zaten var
// (istasyonlar, kandiller), ayrı kayıt tutmak senkron borcu doğurur. Bu
// fonksiyon durumsuz: her karede dt kadar olasılıkla partikül doğurur.
typedef enum {
    FIRE_HEARTH = 0,   // kamp ateşi: geniş, kaotik, bol duman + kıvılcım
    FIRE_FORGE,        // demirci ocağı: sıkı, beyaza yakın, çok kıvılcım
    FIRE_CANDLE,       // kandil/mum: küçük, sakin, dumanı yok denecek kadar
    FIRE_CAULDRON,     // kazan: alev yok, BUHAR
    FIRE_KIND_COUNT,
} FireKind;

// KATMAN MASKESİ — dev tuşu (Ctrl+F) tek katmanı izole eder. Suda Ctrl+B
// dersinin aynısı: efekti ölçülebilir kılmadan ayar yapmak on tur harcıyor.
#define FX_L_FLAME  1
#define FX_L_SMOKE  2
#define FX_L_SPARK  4
#define FX_L_STEAM  8
#define FX_L_EMBER 16
#define FX_L_LICK  32
#define FX_L_SOOT  64
#define FX_L_DEEP 128        // kızıl alev zarfı (Ctrl+F izolasyonu)
#define FX_L_DARK 256        // koyu kırmızı siluet (sarının üstü)
#define FX_L_ALL  511
#define FX_L_COUNT  9
// (fonksiyon bildirimleri aşağıda — ParticleSystem tanımından SONRA)

// Partikul havuzu
typedef struct {
    Particle items[MAX_PARTICLES];
    int nextFree;        // dairesel arama ipucu
} ParticleSystem;

// Yone gore kilic bolgesinin karakter merkezine gore kabaca ofseti (dunya px)
// LPC'de kilic sag elde; yone gore govdenin su tarafinda.
// facing: UP=0, LEFT=1, DOWN=2, RIGHT=3
static const struct { float dx, dy; } WEAPON_FX_OFFSET[4] = {
    {  -15.0f, -2.0f },  // UP    — kilic ust-sagda (arkada)
    { -14.0f,  -2.0f },  // LEFT  — kilic solda
    { -4.0f,  15.0f },  // DOWN  — kilic sol-onde
    {  14.0f,  -2.0f },  // RIGHT — kilic sagda
};

// Slash sirasinda kilicin kare-kare konumu (yon x kare, sprite px, karakter merkezine gore).
// Longsword oversize bloklarinin alpha-agirlikli merkezlerinden uretildi — savurma yayini izler.
static const struct { float dx, dy; } WEAPON_SLASH_FX_OFFSET[4][6] = {
    { {  -2.5f,   8.0f}, { -10.3f,  12.0f}, { -14.8f,   7.3f}, { -22.7f,   0.8f}, {   9.7f, -24.2f}, {  44.4f,  -6.5f} }, // UP
    { { -22.6f,  20.0f}, { -11.1f,  16.1f}, {   1.5f,  10.6f}, {   2.6f,  10.0f}, { -53.4f,  -0.2f}, { -37.0f,  -5.9f} }, // LEFT
    { {  -2.5f,  21.0f}, { -10.3f,  14.0f}, { -14.8f,   4.7f}, { -19.7f,  11.2f}, {   9.7f,  36.2f}, {  44.3f,  18.5f} }, // DOWN
    { {  21.6f,  20.0f}, {  10.1f,  16.1f}, {  -2.5f,  10.6f}, {  -3.7f,  10.0f}, {  52.4f,  -0.2f}, {  36.0f,  -5.9f} }, // RIGHT
};



// SHEET-ANİM HAVUZU (VFX his cilası): tek satırlık/grid sprite sheet'i
// dünyada bir kez oynatıp söner (isabet patlaması, kan, kıvılcım, slash yayı).
// Grid satır-major okunur; cols = satır başına kare.
#define MAX_ANIM_FX 48

typedef struct {
    R2DTexture *tex;
    int frames, cols;
    float fw, fh;        // kare boyutu (sheet px)
    float x, y;          // dünya MERKEZ noktası
    float t;             // geçen süre (sn)
    float fps;
    float scale;         // dünya ölçeği (çizim = fw*scale)
    bool active;
    // EFEKT IŞIĞI: efekt oynarken dünyaya ışık yayar ve SÖNEREK biter.
    // Bunun olmaması efekti "sahnenin üstüne yapıştırılmış çıkartma" yapıyordu;
    // ışık yayan efekt duvarı aydınlatır, gölgeyi kaydırır, atmosfere girer.
    // lightR/G/B 0 ise ışık yok (varsayılan — eski çağrılar aynen çalışır).
    float lightR, lightG, lightB;
    float lightRadius;      // dünya px, tepe değer
    float lightIntensity;   // tepe güç (0..1 tipik)
} AnimFx;

void animfx_init(AnimFx *pool);
void animfx_spawn(AnimFx *pool, R2DTexture *tex, int frames, int cols,
                  float fw, float fh, float x, float y, float fps, float scale);
// Işıklı sürüm: aynı iş + efekt boyunca sönen bir nokta ışık.
// Işık eğrisi: ilk %20'de hızlı yükselir, sonra efekt bitene kadar söner —
// patlama önce parlar, sonra kor gibi çekilir.
void animfx_spawn_lit(AnimFx *pool, R2DTexture *tex, int frames, int cols,
                      float fw, float fh, float x, float y, float fps, float scale,
                      float r, float g, float b, float radius, float intensity);
// Aktif efektlerin ışıklarını ekran-uzayı listesine ekler (light_collect'ten sonra).
// Dönen: out'a yazılan yeni adet (n ya da daha fazla).
int animfx_append_lights(const AnimFx *pool, R2DLight *out, int n, int cap,
                         const Camera *cam, int sw, int sh);
void animfx_update(AnimFx *pool, float dt);
void animfx_render(AnimFx *pool, R2D *renderer, const Camera *camera);

// Havuzu sifirla
void fx_init(ParticleSystem *fx);

// Tek partikul dogur (rastgele varyasyon CAGIRAN tarafindan verilir)
void fx_spawn(ParticleSystem *fx, float x, float y, float vx, float vy,
              float life, float size, Uint8 r, Uint8 g, Uint8 b, WeaponFxType fxType);

// Tum partikulleri ilerlet (hareket + omur + sonme)
void fx_update(ParticleSystem *fx, float dt);

// Tum partikulleri ciz (glow texture ile, kamera + zoom)
void fx_set_layers(int mask);   // 0 = hepsi açık (Ctrl+F izole modu)
// Canlı partikül sayımı (dev teşhis): tip başına aktif adet
int fx_count_active(const ParticleSystem *fx, WeaponFxType t);

// ── EFEKT NEFESİ (genel motor katmanı). Bir kaynağın O ANKİ şiddeti: aynı
// zaman kaynağından türeyen 0..1 arası değer. Parçacıklar da ışık da BUNU
// kullanır, böylece alev kabardığında ışık da kabarır — ikisi bağımsız
// gürültü kullanırsa ateş "iki ayrı şey" gibi okunuyor.
// Skill/boss/doğa olayı da aynı fonksiyonu çağırabilir: seed her kaynağı
// birbirinden ayırır (aynı köyde iki ateş aynı anda kabarmaz).
float fx_breath(FireKind kind, unsigned seed, float timeSec);

// Sıradaki spawn'ların PALET VARYANTI (0-2). Skill kendi tonunu kurar;
// fx_spawn imzası değişmiyor (100+ çağrı yeri korunuyor).
void fx_set_pal_variant(int v);

// Ton KARIŞIMI: spawn'ların altChance kadarı `alt` varyantından doğar. Aynı
// patlamada iki ton yan yana durur (altın alev arasında kızıl diller).
void fx_set_pal_mix(int base, int alt, float altChance);

// ── KUVVET ALANI: kare başına bir kez (dünya px/sn). Hava sistemi rüzgârı,
// patlama itmesi, boss savurması buraya yazar. (0,0) = durgun.
void fx_set_wind(float vx, float vy);

// ── ÖLÜM KANCASI (genel motor seam'i). Bir partikül öldüğünde çağrılır;
// çağıran isterse kalıntı bırakır (kıvılcım → küçük yanık izi, buz → kırağı,
// kan → leke). fx.c'nin decal/ses/hasar modüllerine BAĞLANMAMASI için kanca:
// motor "ne olduğunu" bildirir, "ne yapılacağını" oyun katmanı seçer.
typedef void (*FxDeathHook)(float x, float y, WeaponFxType type, float size);
void fx_set_death_hook(FxDeathHook fn);

// ── ZAMANLI BÜKÜLME PATLAMALARI (genel motor katmanı).
// r2d_distort_set kare başına yenilenen bir listeyi bekliyor, oysa skill/boss
// efektleri ANLIK OLAY: bir kez tetiklenir, kendi ömrünü yaşar. Bu havuz o
// boşluğu kapatıyor — çağıran patlamayı ateşler ve unutur, motor ömrü sürer.
//   · fx_distort_burst : olayı başlat (dünya koordinatı)
//   · fx_distort_update: ömürleri işle (kare başına)
//   · fx_distort_collect: R2DDistort listesine yaz (ekran koordinatına çevirir)
// SHOCK'ta faz otomatik 0→1 yürür (halka dışa açılır); HEAT/SWIRL/LENS'te
// genlik ömür boyunca sönümlenir.
#define FX_MAX_BURST 12
void fx_distort_burst(float wx, float wy, int style, float radius,
                      float strength, float durationSec);
void fx_distort_update(float dt);
int  fx_distort_collect(R2DDistort *out, int cap,
                        float camX, float camY, float zoom);
// Sürekli ateş kaynağı: her karede dt kadar partikül doğurur (durumsuz)
void fx_emit_fire(ParticleSystem *fx, float x, float y, FireKind kind,
                  float strength, float dt);

void fx_render(ParticleSystem *fx, R2D *renderer,
               R2DTexture *glowTex, const Camera *camera);

#endif