#ifndef R2D_H
#define R2D_H
#include <SDL3/SDL.h>
#include <stdbool.h>

// r2d — SDL_GPU üstünde KENDİ 2D renderer'ımız ("no hidden machinery":
// sprite batch, pipeline, shader hepsi burada, gözümüzün önünde).
//
// Mimari: çizim çağrıları CPU'da quad'lara birikir (batch); kare sonunda
// tek upload + tek render pass, texture/blend değişimlerinde draw bölünür.
// Koordinatlar piksel uzayında — NDC dönüşümü vertex shader'da (uniform).
//
// API bir R2D* bağlamı taşır (SDL_Renderer geçişini mekanik kılar; bugün tek
// global bağlam var, param ileride çoklu pencere/thread kapısı).
//
// Shader: kaynak dil HLSL (shaders/*.hlsl). shaders/compile.sh derleme zamanında
// üç formata çevirir (SPIR-V/MSL/DXIL → assets/shaders/); çalışma anında cihazın
// tükettiği format seçilip dosyadan yüklenir (r2d.c load_shader).

typedef struct R2D R2D;                 // opak bağlam
typedef struct R2DTexture R2DTexture;   // opak — GPU texture + boyut + mod

typedef enum {
    R2D_BLEND_ALPHA = 0,   // normal alfa karışımı (varsayılan)
    R2D_BLEND_ADD,         // additive (glow/partikül)
    R2D_BLEND_COUNT,
} R2DBlend;

// --- Işık pass'i (gece aydınlatması) ---
// Sahnenin üstüne multiply blend'li tam-ekran quad: shader piksel başına
// ambient + Σ ışık katkısı hesaplar, sahne rengiyle çarpılır (gerçek renk karışımı).
#define R2D_MAX_LIGHTS 32   // shaders/light.frag.hlsl dizi boyutuyla AYNI kalmalı
#define R2D_MAX_OCCLUDERS 16  // ışığı kesen dikdörtgen sayısı (light.frag.hlsl ile AYNI)

typedef struct {
    float x, y;        // ekran px (dünya→ekran dönüşümü çağıranda)
    float radius;      // ekran px — bu uzaklıkta katkı sıfırlanır
    float r, g, b;     // ışık rengi (0..1)
    float intensity;   // katkı gücü (0..1 tipik)
    // SİLUET DALGALANMASI: halkanın kenarı kusursuz daire olmasın. Gerçek alev
    // nefes alan bir amip gibidir. amp = genlik (yarıçap oranı), phaseC/S =
    // CPU'da hesaplanmış dönen faz vektörü (shader'da atan2 yapmamak için).
    float wobAmp, wobC, wobS;
    // ŞEKİL — ışık başına. Her kaynağın zemine düşen lekesi farklıdır:
    // kandilin haznesi direğin tepesinde (yukarı kaymış, basık), kamp ateşi
    // yerde (çok basık ve geniş), meşale elde (orta). Global tek şekil hepsini
    // aynı gösteriyordu.
    //   aniso <1 = basık elips · 1 = daire · >1 dikey uzama
    //   yOff  <0 = merkez yukarı (kaynağın gerçek yüksekliği)
    float aniso, yOff;
} R2DLight;

// --- Kurulum / yaşam döngüsü ---
R2D *r2d_init(SDL_Window *window);   // NULL = hata (loglar)
void r2d_quit(R2D *ctx);

// begin: swapchain al + batch sıfırla; false = bu kare çizim yok (minimize vb.)
bool r2d_begin_frame(R2D *ctx, Uint8 clearR, Uint8 clearG, Uint8 clearB);
void r2d_end_frame(R2D *ctx);

// Çizim hedefinin boyutu (pencere ya da aktif render target)
void r2d_output_size(R2D *ctx, int *w, int *h);

// --- Texture ---
R2DTexture *r2d_texture_load(R2D *ctx, const char *path);            // stb_image
R2DTexture *r2d_texture_from_pixels(R2D *ctx, const void *rgba, int w, int h);
R2DTexture *r2d_target_create(R2D *ctx, int w, int h);               // çizilebilir
void r2d_texture_destroy(R2D *ctx, R2DTexture *tex);   // kare içinde güvenli (ertelenir)
void r2d_texture_size(const R2DTexture *tex, float *w, float *h);

// OPAK SINIRLAR (alpha bounding box) — yüklemede taranır, sprite px cinsinden.
// Gölge ayak noktası için: sprite PNG'sinin alt kenarı nesnenin zemine değdiği
// yer değildir (ağaçta altta şeffaf boşluk var). Ayak çizgisi = opY + opH.
void r2d_texture_opaque_bounds(const R2DTexture *tex, float *x, float *y,
                               float *w, float *h);

// SDL_SetTextureColorMod/AlphaMod karşılığı — çizimde vertex rengiyle çarpılır
void r2d_texture_set_color_mod(R2DTexture *tex, Uint8 r, Uint8 g, Uint8 b);
void r2d_texture_set_alpha_mod(R2DTexture *tex, Uint8 a);

// Örnekleme: varsayılan NEAREST (pixel-art); glow gibi yumuşak şeyler LINEAR
void r2d_texture_set_linear(R2DTexture *tex, bool linear);

// --- Render target: hedefe AYRI pass ile çiz (kare içinde/dışında çağrılabilir).
// begin..end arası çizimler hedefe gider; end anında GPU'ya gönderilir.
void r2d_target_begin(R2D *ctx, R2DTexture *target, bool clear,
                      Uint8 r, Uint8 g, Uint8 b);
void r2d_target_end(R2D *ctx);

// --- Çizim durumu ---
void r2d_set_blend(R2D *ctx, R2DBlend blend);
void r2d_set_color(R2D *ctx, Uint8 r, Uint8 g, Uint8 b, Uint8 a);   // primitif rengi

// Işık pass'ini batch'e koy — o ana kadar çizilenlerin ÜSTÜNE uygulanır,
// sonrası (HUD) etkilenmez. Kare başına EN FAZLA BİR kez; target içinde çalışmaz.
// ambient: sahne çarpanı (gece ~0.3, gündüz 1 → çağırma, görünmez).
// Işık GÖRSEL modu — F4 ile döngülenir, oynanışı ETKİLEMEZ.
//   quantSteps: 0 = serbest (sürekli gradyan) · >0 = ışık alanı N basamağa iner
//   gridPx:     0 = yok · >0 = ışık bu ızgaranın merkezinden örneklenir
//               (sanat pikseline yapıştırma; zemin 16px sanat × 4 ölçek = 4 px)
void r2d_light_set_mode(float quantSteps, float gridPx);

// IŞIK MODELİ VARYANTLARI (dev tuşları) — hepsi yalnız GÖRSELİ etkiler.
//   gammaMode   0 = sRGB uzayında topla (eski, HATALI: merkez +0.22 kırpılıyor)
//               1 = LİNEER uzayda topla (fiziksel doğru — Karis/Frostbite PBR)
//               2 = lineer + ACES filmic tone map (Narkowicz 2016)
//   falloffMode 0 = iç plato + smoothstep + kenar kesimi (mevcut)
//               1 = Karis pencereli ters-kare (UE4 2013 dnk.9) — falloffParam = d0 px
//               2 = URP 2D pow(1-t, k) ailesi — falloffParam = k (1.0 = lineer)
//               3 = kenar dizi (knee): dış %25'te ani çöküş, sınır çok okunur
//   ditherMode  0 = interleaved gradient noise · 1 = Bayer 4×4 ordered
//               2 = dünya-çivili IGN (kamera hareketinde "crawl" etmez) · 3 = yok
// DİKKAT: falloffMode değişirse light.h light_atten() de AYNI eğriye geçmeli,
// yoksa oyuncu aydınlık gördüğü yerde ölüyle karşılaşır.
void r2d_light_set_model(float gammaMode, float falloffMode, float ditherMode,
                        float falloffParam);

// Dünya-çivili dither için kamera konumu (her kare, ışık pass'inden önce)
void r2d_light_set_camera(float camX, float camY);

// A/B KARŞILAŞTIRMA: ekranı dikey bölüp SOL yarıya orijinal ayarları, SAĞ
// yarıya seçili modları uygular (bölme çizgisi sarı çizilir). 0 = kapalı.
// Varyantların çoğu tek başına bakıldığında ayırt edilemiyor; yan yana görünür.
void r2d_light_set_split(float screenX);

// IŞIK ŞEKLİ — kamera tam kuş bakışı değil (3/4 eğik: evler yandan görünüyor).
// Eğik perspektifte zemin dikey olarak kısaltılır, yani zeminde DAİRESEL olan
// aydınlık alan ekranda YATAY ELİPS görünmeli. Ayrıca kaynak zeminde değil
// (kandil haznesi, alev) — yükseklik ekranda yukarı çizilir.
//   aniso  1.0 = daire · <1 basık elips (zemin kısaltması) · >1 dikey uzama
//   yOffset  0 = merkez ayak hizasında · <0 merkez yukarı (kaynak yüksekliği)
void r2d_light_set_shape(float aniso, float yOffset);

// IŞIK TIKAMA (occlusion) — ışığın nesnelerden GEÇMEMESİ. Bu olmadan her ışık,
// önünde ne olursa olsun kusursuz bir geometrik şekil boyar; "hep yuvarlak
// görünüyor" şikâyetinin asıl sebebi buydu.
// Dikdörtgenler EKRAN uzayında (x, y, w, h) ve nesnelerin AYAK İZİdir (gövde),
// sprite'ın tamamı değil — ağacın tacı ışığı kesmez, gövdesi keser.
//   strength 0 = kapalı · 1 = tam gölge · 0.75 tipik (gölgeye ışık sızar)
void r2d_light_set_occluders(const SDL_FRect *rects, int count, float strength);

void r2d_light_pass(R2D *ctx, float ambientR, float ambientG, float ambientB,
                    const R2DLight *lights, int count);

// --- Post-fx: tam ekran "grade" filtresi ---
// Etkinken kare offscreen target'a çizilir, kare sonunda fs_postfx shader'ıyla
// ekrana basılır. HUD dahil TÜM kare filtrelenir. Tüm alanlar nötr değerdeyken
// görüntü değişmez — tarzı bulmak için topuzlar tek tek oynatılabilir.
typedef struct {
    float saturation;      // 1 = nötr; <1 soluk, >1 canlı
    float warmth;          // 0 = nötr; + sıcak (turuncu), - soğuk (mavi); ±0.05 tipik
    float contrast;        // 1 = nötr; 0.5 civarına göre açılır
    float brightness;      // 0 = nötr; ± doğrudan eklenir
    float vignette;        // 0 = yok; kenar karartma gücü (0.3 tipik)
    float vignetteRadius;  // karartmanın başladığı merkez uzaklığı 0..1 (0.4 tipik)
    float scanline;        // 0 = yok; CRT tarama çizgisi koyuluğu (0.35 tipik)
    float gameboy;         // >0.5 = DMG 4-renk yeşil palet (grade üstüne biner)
    float posterize;       // 0 = yok; kanal başına renk kademesi (6-8 tipik, min 2)
    float aberration;      // 0 = yok; kromatik sapma, uv birimi (0.002 tipik)
    float grain;           // 0 = yok; animasyonlu film greni gücü (0.06 tipik)
    // BLOOM — parlak yerlerin taşması. Skill efektlerinin "muhteşem" hissinin
    // büyük kısmı buradan gelir: additive katman tek başına düz durur, taşma
    // onu ışık yapar. Tek geçişli yaklaşım (ayrı render target YOK): parlak-geçiş
    // eşiğinin üstündeki pikseller iki halka halinde 12 tap ile toplanır.
    // DİKKAT: post-fx tüm kareyi (HUD dahil) filtreler — eşik yüksek tutulmalı,
    // yoksa HUD'un beyaz yazısı da taşar. Sahne-only ayrımı: GDD §6.1 açık soru.
    float bloom;           // 0 = yok; taşma gücü (0.35 tipik, 1.0 aşırı)
    float bloomThreshold;  // parlak-geçiş eşiği, luma 0..1 (0.75 tipik)
    float bloomRadius;     // taşma yarıçapı, uv birimi (0.004 tipik)
    // ISI BÜKÜLMESİ — ekran-uzayı UV kaydırma. Volkan, gayzer, lav, alev damgası
    // ve sıcak hava için. Ekranın ALTINA doğru güçlenir (sıcak hava yerden kalkar),
    // böylece gökyüzü/HUD üstü daha az etkilenir.
    // Dalga: iki farklı frekansta sinüs — tek frekans "akordeon" gibi duruyor.
    float heat;            // 0 = yok; bükülme genliği uv birimi (0.0025 tipik)
    float heatSpeed;       // dalga hızı (1.5 tipik)
} R2DPostFX;

// Değerler kopyalanır (her kare çağırmak gerekmez). NULL = filtreyi kapat.
void r2d_postfx_set(R2D *ctx, const R2DPostFX *fx);

// SAHNE-ONLY FİLTRE. Bu noktaya kadar çizilen her şey (dünya) offscreen'e akar,
// filtreden geçer ve ekrana basılır; BUNDAN SONRAKİ çizimler (HUD, panel, yazı)
// doğrudan ekrana gider ve FİLTRELENMEZ.
//
// Neden gerekli: ısı bükülmesi UV kaydırıyor, yani filtre HUD'u da büküyordu —
// hotbar ve durum çubukları dalgalanıyordu. Shader hangi pikselin HUD olduğunu
// bilemez, o yüzden ayrım çizim SIRASINDA yapılmak zorunda.
//
// Filtre kapalıysa hiçbir şey yapmaz (güvenli çağrı). Kare başına bir kez.
void r2d_postfx_flush(R2D *ctx);

// --- SU HACMİ (shaders/water.frag.hlsl) ------------------------------------
// KARE ORTASINDA çağrılır: o ana kadar çizilmiş olan ZEMİN katmanı ayrı bir
// target'a alınır, su shader'ı onu KENDİ DALGA NORMALİNE GÖRE KAYDIRARAK
// örnekler, bileşik sonuç batch'in başına tek tam-ekran quad olarak geri
// konur. Sonrasında normal çizime devam edilir; çağıranın başka bir şey
// bilmesi gerekmez (ışık, sprite, post-fx, HUD ayrımı hepsi değişmeden çalışır).
//
// NEDEN BÖYLE: su altındaki şeyi ÖRTMEK sualtı hissi vermiyor — sualtı,
// tanıdık bir nesnenin BÜKÜLDÜĞÜNÜ görmektir. Bükmek için altındaki görüntüyü
// örneklemek gerekir, örneklemek için de o görüntünün bir doku olması gerekir.
//
// mask: derinlik maskesi — KARO BAŞINA 2×2 TEKSEL, R kanalı 0 (kara) → 1 (en
// derin). LINEAR örneklenir; teksel merkezleri arası yumuşak rampa oradan gelir,
// yani kıyının köşeli basamak silueti bu yüzden yok.
//
// KATMAN BİT MASKESİ (layers): her katman kendi bitiyle kapanabilir. DEV
// anahtarı (main.c Ctrl+B) tek bir biti bırakıp gerisini söndürerek katmanı
// İZOLE gösteriyor. Bu sayılar shaders/water.frag.hlsl LB_* sabitleriyle BİREBİR
// olmak zorunda — biri kayarsa yanlış katman kapanır ve sessiz hata olur.
#define R2D_WATER_L_REFRACT  1    // kırılma (sualtı bükülmesi)
#define R2D_WATER_L_DEPTH    2    // derinlik örtmesi / su rengi
#define R2D_WATER_L_CAUSTIC  4    // kostik ışık ağı
#define R2D_WATER_L_GLINT    8    // güneş parlaması
#define R2D_WATER_L_FOAM     16   // kıyı köpüğü
#define R2D_WATER_L_WET      32   // ıslak bant (kıyının kara tarafı)
#define R2D_WATER_L_DARK     64   // koyu hat (suyun silueti)
#define R2D_WATER_L_MENI     128  // menisküs (parlak kıyı çizgisi)
#define R2D_WATER_L_BANK     256  // kıyı gölgesi (kuzey kıyının çukura düşen gölgesi)
#define R2D_WATER_L_DASH     512  // ışıltı çizgicikleri (seyrek, ayrık, sürüklenen)
#define R2D_WATER_L_ARC      1024 // dalga-yankı arkları (kıyıya paralel koyu sıralar)
#define R2D_WATER_L_RING     2048 // halka dalgacık (balık/damla izi)
#define R2D_WATER_L_ALL      4095
#define R2D_WATER_L_COUNT    12   // izole modda dolaşılacak katman sayısı

typedef struct {
    R2DTexture *mask;
    float maskW, maskH;          // maske boyutu, TEKSEL cinsinden (karo değil)
    float tilePx;                // bir karonun dünya pikseli
    float camX, camY, zoom;
    float time;                  // saniye — DUVAR saati (su geceleyin yavaşlamaz)
    float sunX, sunY;            // güneş yönü, birim vektör
    float sunI;                  // güneş şiddeti: 0 gece, 1 tepe
    float sunR, sunG, sunB;      // güneş rengi, 0..1
    float artPx;                 // bir sanat pikselinin EKRAN pikseli (snap için)
    float shallowR, shallowG, shallowB, shallowCover;   // sığ su rengi + örtme
    float deepR, deepG, deepB, deepCover;               // derin su rengi + örtme
    float waveHeight;            // dalganın derinliğe etkisi (sırt sığ, çukur derin)
    float refract;               // kırılma gücü, dünya px
    float caustic;               // kostik ışık ağı gücü
    float foam;                  // kıyı köpüğü gücü
    float glint;                 // güneş parlaması gücü
    float quant;                 // renk kademesi (0 = kapalı, örn. 24 = paletli)
    float foamSpeed;             // köpüğün kıyıya vurup çekilme hızı (rad/sn)
    float foamAmp;               // köpüğün ne kadar içeri girdiği (derinlik biriminde)
    // SU ÇİZGİSİ — üç bant. Sahibinin "gölge/derinlik yok" dediği şeyin karşılığı.
    float wetBand;               // ıslak bant gücü (kıyının KARA tarafı)
    float darkLine;              // koyu hat gücü (suyun kendi silueti, çarpımsal)
    float meniscus;              // menisküs gücü (EKLEMELİ — bütçeye dahil, ≤0.13)
    // KIYI GÖLGESİ — kamera 3/4 eğik olduğu için gölet bir ÇUKUR: kuzey kıyının
    // iç duvarı suya gölge düşürür (maskenin A kanalı taşır). Gölgede kostik ve
    // parlama da söner — güneş yoksa ışık oyunu da yok.
    float bankShadow;            // kıyı gölgesi gücü (0 = kapalı, ~0.55 tipik)
    float dashes;                // ışıltı çizgicikleri gücü (EKLEMELİ, ~0.12)
    float arcs;                  // dalga-yankı arkları gücü (ÇARPIMSAL, 0..1)
    float wallFace;              // kuzey duvar prototipi (0 = RAFTA; karo sanatı gelince kıyas için 1)
    float layers;                // katman bit maskesi; 0 = hepsi açık (bkz. R2D_WATER_L_*)
} R2DWater;
void r2d_water_flush(R2D *ctx, const R2DWater *w);

// --- Sprite ---
void r2d_draw_texture(R2D *ctx, R2DTexture *tex,
                      const SDL_FRect *src, const SDL_FRect *dst);
void r2d_draw_texture_flip(R2D *ctx, R2DTexture *tex, const SDL_FRect *src,
                           const SDL_FRect *dst, bool flipH);
// Su yansıması: dikey aynalı + soluk (su geçişinden ÖNCE zemine çizilir,
// kırılma/derinlik süzgeci üstüne bedava biner)
void r2d_draw_texture_reflect(R2D *ctx, R2DTexture *tex, const SDL_FRect *src,
                              const SDL_FRect *dst, Uint8 alpha);
// Serbest 4 köşeli quad (eksen-hizalı DEĞİL) — gölge izdüşümü için.
// Köşe sırası TL, TR, BL, BR (ekran px). UV eşlemesi sprite'ın tabanını
// BL/BR'ye, tepesini TL/TR'ye bağlar: yani gölge kendiliğinden doğru yatar.
void r2d_draw_texture_quad(R2D *ctx, R2DTexture *tex, const SDL_FRect *src,
                           const SDL_FPoint corners[4], SDL_Color col);

void r2d_draw_9grid(R2D *ctx, R2DTexture *tex, const SDL_FRect *src,
                    float corner, float scale, const SDL_FRect *dst);

// --- Primitifler (aktif renkle — SDL_SetRenderDrawColor deseni) ---
void r2d_fill_rect(R2D *ctx, const SDL_FRect *r);
void r2d_rect(R2D *ctx, const SDL_FRect *r);            // 1px çerçeve
void r2d_line(R2D *ctx, float x1, float y1, float x2, float y2);
void r2d_point(R2D *ctx, float x, float y);

#endif
