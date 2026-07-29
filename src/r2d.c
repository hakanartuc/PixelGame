#include <string.h>
#include "r2d.h"
#include "stb_image.h"   // implementasyon render.c'de derili — burada sadece bildirimler

// ---------------------------------------------------------------------------
// Sabitler / tipler
// ---------------------------------------------------------------------------
#define R2D_MAX_VERTS  (6 * 16384)   // kare başına max quad: 16384 (6 vert/quad)
#define R2D_MAX_DRAWS  2048          // texture/blend kırılımı sayısı
#define R2D_MAX_DEAD   256           // kare içinde ertelenen texture yıkımı

// Vertex düzeni: pos(2) + uv(2) + renk(4) = 8 float (32 byte)
typedef struct {
    float x, y, u, v;
    float r, g, b, a;
} R2DVertex;

struct R2DTexture {
    SDL_GPUTexture *tex;
    float w, h;
    Uint8 modR, modG, modB, modA;   // color/alpha mod (çizimde çarpılır)
    bool isTarget;
    bool linear;                    // örnekleme: false=NEAREST (pixel-art)
    // OPAK SINIRLAR (alpha bounding box) — yüklemede bir kez taranır.
    // Gölge ayak noktası için gerekli: sprite dikdörtgeninin alt kenarı
    // nesnenin zemine değdiği yer DEĞİL (ağaç PNG'sinde altta şeffaf boşluk
    // var). Unity'nin Sprite Editor "Trim" işlevinin yaptığı şey budur.
    float opX, opY, opW, opH;
};

typedef struct {
    R2DTexture *tex;
    R2DBlend blend;
    int first, count;
} R2DDraw;

// Tek global bağlam — R2D* parametresi geçiş kolaylığı + gelecek kapısı
struct R2D { int unused; };
static struct R2D CTX;

// ---------------------------------------------------------------------------
// MSL shader — SDL_GPU Metal bağlama düzeni:
//   vertex uniforms → [[buffer(0)]], vertex girdisi → [[stage_in]]
//   fragment texture → [[texture(0)]], sampler → [[sampler(0)]]
// ---------------------------------------------------------------------------
static const char *MSL_SOURCE =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct VSIn {\n"
    "    float2 pos [[attribute(0)]];\n"
    "    float2 uv  [[attribute(1)]];\n"
    "    float4 col [[attribute(2)]];\n"
    "};\n"
    "struct VSOut {\n"
    "    float4 pos [[position]];\n"
    "    float2 uv;\n"
    "    float4 col;\n"
    "};\n"
    "struct Uniforms { float2 viewport; };\n"
    "vertex VSOut vs_main(VSIn in [[stage_in]],\n"
    "                     constant Uniforms &u [[buffer(0)]]) {\n"
    "    VSOut out;\n"
    "    float2 ndc = float2(in.pos.x / u.viewport.x * 2.0 - 1.0,\n"
    "                        1.0 - in.pos.y / u.viewport.y * 2.0);\n"
    "    out.pos = float4(ndc, 0.0, 1.0);\n"
    "    out.uv = in.uv;\n"
    "    out.col = in.col;\n"
    "    return out;\n"
    "}\n"
    "fragment float4 fs_main(VSOut in [[stage_in]],\n"
    "                        texture2d<float> tex [[texture(0)]],\n"
    "                        sampler smp [[sampler(0)]]) {\n"
    "    return tex.sample(smp, in.uv) * in.col;\n"
    "}\n"
    // Işık pass'i: piksel başına ambient + Σ ışık katkısı; multiply blend ile
    // sahne rengiyle çarpılır. Dizi boyutu R2D_MAX_LIGHTS (r2d.h) ile AYNI.
    "struct LightU {\n"
    "    float4 ambient;\n"          // rgb = ortam çarpanı, w = kamera ofset X
    "    float4 count;\n"            // x=ışık sayısı y=kuantize z=ızgara w=kamera ofset Y
    "    float4 mode;\n"             // x=gamma y=düşüş z=dither w=düşüş parametresi
    "    float4 mode2;\n"            // x=A/B bölme y=anizotropi z=dikey ofset w=tıkama
    "    float4 pos[32];\n"          // xy = ekran px, z = yarıçap px
    "    float4 col[32];\n"          // rgb = renk, w = yoğunluk
    "    float4 shape[32];\n"        // x = siluet genliği, yz = dönen faz vektörü
    "    float4 occ[16];\n"          // ışığı kesen dikdörtgenler: xy=köşe, zw=boyut
    "};\n"
    // Segment (pa→pb) bir AABB'yi kesiyor mu — slab testi, dallanmasız.
    // Işık tıkama için: pikselden ışığa giden doğru bir gövdeden geçiyorsa
    // o piksel gölgede. Ray-box ucuz (birkaç karşılaştırma), doku okuması yok.
    "static bool seg_hits_box(float2 pa, float2 pb, float4 b) {\n"
    "    float2 d = pb - pa;\n"
    "    float2 ad = abs(d);\n"
    "    if (ad.x < 1e-4) d.x = (d.x < 0.0 ? -1e-4 : 1e-4);\n"
    "    if (ad.y < 1e-4) d.y = (d.y < 0.0 ? -1e-4 : 1e-4);\n"
    "    float2 t0 = (b.xy - pa) / d;\n"
    "    float2 t1 = (b.xy + b.zw - pa) / d;\n"
    "    float2 tmin = min(t0, t1), tmax = max(t0, t1);\n"
    "    float tn = max(tmin.x, tmin.y);\n"
    "    float tf = min(tmax.x, tmax.y);\n"
    "    return (tf >= tn) && (tn < 1.0) && (tf > 0.0);\n"
    "}\n"
    // sRGB ⇄ lineer (IEC 61966-2-1). Işık katkıları FİZİKSEL olarak lineer
    // uzayda toplanır; sRGB kodunda toplamak merkezi +0.22 fazla parlatıp
    // kırpıyor (düz beyaz plato) ve dış halkayı 0.03 karartıyor. Yani ışık
    // "kubbe" değil "kenarı sıkıştırılmış patlamış disk" gibi okunuyordu.
    // Not: çarpma gama ile komütatiftir, o yüzden multiply kompozisyonu doğru;
    // hata YALNIZCA toplama satırındaydı.
    "static float3 s2l(float3 c) {\n"
    "    return select(pow((c + 0.055) / 1.055, float3(2.4)), c / 12.92,\n"
    "                  c <= 0.04045);\n"
    "}\n"
    "static float3 l2s(float3 c) {\n"
    "    return select(1.055 * pow(c, float3(1.0 / 2.4)) - 0.055, 12.92 * c,\n"
    "                  c <= 0.0031308);\n"
    "}\n"
    "fragment float4 fs_light(VSOut in [[stage_in]],\n"
    "                         constant LightU &lu [[buffer(0)]]) {\n"
    "    int   n      = int(lu.count.x);\n"
    "    float qsteps = lu.count.y;\n"
    "    float grid   = lu.count.z;\n"
    "    int   gmode  = int(lu.mode.x);\n"   // 0=sRGB toplama 1=lineer 2=lineer+ACES
    "    int   fmode  = int(lu.mode.y);\n"   // 0=plato+ss 1=Karis 2=URP pow 3=knee
    "    int   dmode  = int(lu.mode.z);\n"   // 0=IGN 1=Bayer 2=dünya-IGN 3=yok
    "    float fparam = lu.mode.w;\n"
    // A/B KARŞILAŞTIRMA: bölme çizgisinin SOLU daima orijinal ayarlar, sağı
    // seçili modlar. Tek ekranda yan yana fark görülür — varyantların çoğu
    // tek başına bakıldığında ayırt edilemiyor.
    "    float splitX = lu.mode2.x;\n"
    "    bool  isLeft = (splitX > 0.5 && in.pos.x < splitX);\n"
    "    if (isLeft) { gmode = 0; fmode = 0; dmode = 0; fparam = 0.0; }\n"
    "    bool  lin    = (gmode > 0);\n"
    "    float3 c = lin ? s2l(lu.ambient.rgb) : lu.ambient.rgb;\n"
    // Kuantize modda ışığı SANAT pikselinin merkezinden örnekle
    "    float2 sp = in.pos.xy;\n"
    "    if (grid > 0.5) sp = floor(sp / grid) * grid + grid * 0.5;\n"
    "    for (int i = 0; i < n; i++) {\n"
    // EĞİK PERSPEKTİF DÜZELTMESİ. Kamera tam kuş bakışı DEĞİL (evler yandan
    // görünüyor, çatıları var) — zemin dikey olarak kısaltılmış çiziliyor.
    // Yani zeminde dairesel olan aydınlık alan ekranda YATAY ELİPS olmalı.
    // aniso < 1 → dikey yönde ışık daha çabuk biter (basık elips)
    // aniso > 1 → dikey uzar (alevin yukarı yayılması)
    // yOff  < 0 → ışık merkezi yukarı kayar (kaynak zeminde değil, hazne/alev
    //             yüksekliğinde) — 3/4 perspektifte yükseklik ekranda yukarıdır
    "        float2 v = sp - lu.pos[i].xy;\n"
    // Şekil ışık BAŞINA (pos.w = dikey ofset, shape.w = anizotropi);
    // mode2.y/z global çarpan-ekleyen (dev tuşu ile test için).
    "        v.y -= lu.pos[i].w + lu.mode2.z;\n"
    "        float aniso = max(lu.shape[i].w * lu.mode2.y, 0.05);\n"
    "        v.y /= aniso;\n"
    "        float d = length(v);\n"
    "        float r = lu.pos[i].z;\n"
    "        float amp = lu.shape[i].x;\n"
    "        if (d >= r * (1.0 + amp)) continue;\n"
    // SİLUET: halkanın kenarı kusursuz daire olmasın. Açıya bağlı loblar,
    // ama atan2 YOK — çift/üçlü açı özdeşlikleriyle (nx²−ny² = cos2a vb.).
    // Faz vektörü CPU'dan gelir, yani per-pixel transcendental hiç yok.
    "        if (amp > 0.0) {\n"
    "            float2 nv = v / max(d, 0.001);\n"
    "            float c2 = nv.x * nv.x - nv.y * nv.y;\n"
    "            float s2 = 2.0 * nv.x * nv.y;\n"
    "            float c3 = nv.x * (2.0 * c2 - 1.0);\n"
    "            float s3 = nv.y * (2.0 * c2 + 1.0);\n"
    "            float cp = lu.shape[i].y, sp2 = lu.shape[i].z;\n"
    "            float wob = (c2 * cp + s2 * sp2) * 0.6\n"
    "                      + (c3 * cp - s3 * sp2) * 0.4;\n"
    "            r *= 1.0 + amp * wob;\n"
    "        }\n"
    "        if (d >= r) continue;\n"
    "        float att;\n"
    "        if (fmode == 1) {\n"
    // KARIS pencereli ters-kare (UE4 SIGGRAPH 2013, dnk.9; Frostbite dnk.26).
    // Referans uzunluk şart: piksel biriminde +1 hiç iş görmez.
    "            float u = d / fparam;\n"
    "            float w = saturate(1.0 - pow(d / r, 4.0));\n"
    "            att = (w * w) / (u * u + 1.0);\n"
    "        } else if (fmode == 2) {\n"
    // URP 2D Light2D ailesi: att = (1-t)^k, k=1 tam lineer
    "            att = pow(1.0 - d / r, fparam);\n"
    "        } else if (fmode == 3) {\n"
    // KENAR DİZİ: iç bölge smoothstep, dış %25'te ani çöküş → sınır okunur
    "            float b = 1.0 - d / r;\n"
    "            float a = b * b * (3.0 - 2.0 * b);\n"
    "            float w = saturate(b / 0.25);\n"
    "            att = a * w * w;\n"
    "        } else {\n"
    // MEVCUT: iç %35 plato → smoothstep
    "            float t = min((1.0 - d / r) / 0.65, 1.0);\n"
    "            att = t * t * (3.0 - 2.0 * t);\n"
    "        }\n"
    // KESİN KENAR — HER MODA. light.h light_atten() ile aynı eşik (0.06).
    // Yalnız mod 0'da olması bastırma yarıçapını moda göre kaydırıyordu.
    "        if (att < 0.06) continue;\n"
    // ── IŞIK TIKAMA ── pikselden ışığa giden doğru bir gövdeden geçiyorsa
    // bu piksel gölgede. Bu olmadan ışık duvarların içinden geçiyordu ve her
    // halka kusursuz bir oval oluyordu. Tam karanlık değil: gölgeye ışık sızar
    // (yansıma yaklaşımı), yoksa siyah kamalar oyuncuyu kör eder.
    "        float occN = lu.mode2.w;\n"
    "        if (occN > 0.5 && !isLeft) {\n"
    "            int on = int(occN);\n"
    "            for (int k = 0; k < on; k++) {\n"
    "                if (seg_hits_box(in.pos.xy, lu.pos[i].xy, lu.occ[k])) {\n"
    "                    att *= 0.22;\n"
    "                    break;\n"
    "                }\n"
    "            }\n"
    "        }\n"
    "        float3 lc = lin ? s2l(lu.col[i].rgb) : lu.col[i].rgb;\n"
    "        c += lc * (att * lu.col[i].w);\n"
    "    }\n"
    // ACES filmic (Narkowicz 2016). DİKKAT: çıktı clamp'li ve blend çarpımsal
    // olduğu için bu mod 1.0 tavanını AÇMIYOR — ACES(1.0)=0.804, ACES(2.0)=0.915,
    // yani tepeyi ~%20 karartıp yumuşak bir omuz veriyor. Gerçek overbright için
    // RGBA16F hedef gerekir (ayrı iş). Lineer girdi ister.
    "    if (gmode == 2)\n"
    "        c = clamp((c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14),\n"
    "                  0.0, 1.0);\n"
    "    if (lin) c = l2s(c);\n"
    // KUANTİZE — l2s SONRASINDA olmak zorunda. sRGB kodlaması kendisi kabaca
    // algısal olarak düzgün olduğu için eşit aralıklı kuantizasyon burada
    // algısal olarak da doğru; lineer uzayda kuantalarsak bantlar tepe ışıkta
    // yığılır, gölgede 1-2 kademeye düşer.
    "    if (qsteps > 0.5) {\n"
    "        c = floor(c * qsteps + 0.5) / qsteps;\n"
    "        return float4(clamp(c, float3(0.0), float3(1.0)), 1.0);\n"
    "    }\n"
    // DITHER — 8-bit hedefte banding kırar
    "    if (dmode != 3) {\n"
    "        float dn;\n"
    "        if (dmode == 1) {\n"
    // Bayer 4×4 ordered: 8/16-bit döneminin görsel dili, kasıtlı desen
    "            float B[16] = {  0.0,  8.0,  2.0, 10.0, 12.0,  4.0, 14.0,  6.0,\n"
    "                             3.0, 11.0,  1.0,  9.0, 15.0,  7.0, 13.0,  5.0 };\n"
    "            int2 bi = int2(in.pos.xy) & 3;\n"
    "            dn = B[bi.y * 4 + bi.x] / 16.0;\n"
    "        } else {\n"
    // IGN (Jimenez, CoD:AW 2014). dmode==2: indeks DÜNYA koordinatından —
    // ekran uzayı deseni kamera hareketinde "crawl" ediyor ("kirli cam").
    "            float2 q = in.pos.xy;\n"
    "            if (dmode == 2) q = floor((q + float2(lu.ambient.w,\n"
    "                                                 lu.count.w)) / 8.0);\n"
    "            dn = fract(52.9829189 * fract(dot(q, float2(0.06711056,\n"
    "                                                        0.00583715))));\n"
    "        }\n"
    "        c += (dn - 0.5) * (2.0 / 255.0);\n"
    "    }\n"
    "    if (splitX > 0.5 && abs(in.pos.x - splitX) < 1.5)\n"
    "        c = float3(1.0, 0.85, 0.2);\n"        // bölme çizgisi
    "    return float4(clamp(c, float3(0.0), float3(1.0)), 1.0);\n"
    "}\n"
    // Post-fx "grade": sahne target'ını tam ekran örnekleyip doygunluk +
    // sıcaklık + kontrast/parlaklık + retro modlar + vinyet uygular.
    // Düzen R2DPostFXU ile aynı.
    "struct PostU {\n"
    "    float4 a;\n"   // x=doygunluk y=sıcaklık z=kontrast w=parlaklık
    "    float4 b;\n"   // x=vinyet gücü y=vinyet yarıçapı z=scanline w=gameboy
    "    float4 c;\n"   // x=posterize y=kromatik sapma z=gren w=zaman (kare sayacı)
    "};\n"
    "fragment float4 fs_postfx(VSOut in [[stage_in]],\n"
    "                          constant PostU &u [[buffer(0)]],\n"
    "                          texture2d<float> tex [[texture(0)]],\n"
    "                          sampler smp [[sampler(0)]]) {\n"
    // Kromatik sapma: R sola, B sağa kaymış örneklenir (VHS/retro kenar taşması)
    "    float3 c;\n"
    "    if (u.c.y > 0.0) {\n"
    "        c.r = tex.sample(smp, in.uv - float2(u.c.y, 0.0)).r;\n"
    "        c.g = tex.sample(smp, in.uv).g;\n"
    "        c.b = tex.sample(smp, in.uv + float2(u.c.y, 0.0)).b;\n"
    "    } else {\n"
    "        c = tex.sample(smp, in.uv).rgb;\n"
    "    }\n"
    "    float luma = dot(c, float3(0.299, 0.587, 0.114));\n"
    "    c = mix(float3(luma), c, u.a.x);\n"
    "    c += float3(u.a.y, 0.0, -u.a.y);\n"
    "    c = (c - 0.5) * u.a.z + 0.5 + u.a.w;\n"
    // Posterize: kanal başına N kademe (8-bit görünüm)
    "    if (u.c.x > 1.5) {\n"
    "        float n = u.c.x - 1.0;\n"
    "        c = round(clamp(c, 0.0, 1.0) * n) / n;\n"
    "    }\n"
    // GameBoy modu: parlaklığı 4 kademeye kuantala, DMG yeşil paletine bas
    "    if (u.b.w > 0.5) {\n"
    "        float3 pal[4] = { float3(0.06, 0.22, 0.06), float3(0.19, 0.38, 0.19),\n"
    "                          float3(0.55, 0.67, 0.06), float3(0.61, 0.74, 0.06) };\n"
    "        float l = dot(clamp(c, 0.0, 1.0), float3(0.299, 0.587, 0.114));\n"
    "        c = pal[clamp(int(l * 4.0), 0, 3)];\n"
    "    }\n"
    // CRT tarama çizgisi: her 3 piksel satırının biri koyulaşır
    "    if (u.b.z > 0.0 && fract(in.pos.y / 3.0) < 0.334) c *= 1.0 - u.b.z;\n"
    // Film greni: kare sayacıyla kayan hash gürültüsü (animasyonlu kumlanma)
    "    if (u.c.z > 0.0) {\n"
    "        float gn = fract(sin(dot(in.pos.xy + u.c.w,\n"
    "                                 float2(12.9898, 78.233))) * 43758.5453);\n"
    "        c += (gn - 0.5) * u.c.z;\n"
    "    }\n"
    // Vinyet: uv merkez uzaklığı 0 (orta) → 1 (köşe); radius'tan köşeye yumuşak iner
    "    float d = length(in.uv - 0.5) * 1.41421356;\n"
    "    c *= 1.0 - u.b.x * smoothstep(u.b.y, 1.0, d);\n"
    "    return float4(clamp(c, float3(0.0), float3(1.0)), 1.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// Durum
// ---------------------------------------------------------------------------
static SDL_GPUDevice *gpu = NULL;
static SDL_Window *win = NULL;
static SDL_GPUGraphicsPipeline *pipeline[R2D_BLEND_COUNT];        // swapchain formatı
static SDL_GPUGraphicsPipeline *targetPipeline[R2D_BLEND_COUNT];  // RGBA8 target formatı
static SDL_GPUGraphicsPipeline *lightPipeline = NULL;             // ışık pass'i (swapchain)
static SDL_GPUGraphicsPipeline *lightPipelineTarget = NULL;       // ışık pass'i (RGBA8 target)

// Post-fx: açıkken kare önce sceneTex'e çizilir, sonra fs_postfx ile ekrana basılır
static SDL_GPUGraphicsPipeline *postfxPipeline = NULL;
static SDL_GPUBuffer *fsQuadBuf = NULL;      // statik tam-ekran quad (pos 0..1)
static R2DTexture *sceneTex = NULL;          // pencere boyutunda; resize'da yeniden kurulur
static bool postfxOn = false;
typedef struct { float a[4]; float b[4]; float c[4]; } R2DPostFXU;   // MSL PostU ile birebir
static R2DPostFXU postfxU;
static Uint32 frameNo = 0;   // gren animasyonu için kare sayacı (postfx zamanı)

// Işık uniform'ları — MSL'deki LightU ile birebir aynı düzen (float4 hizalı).
// Tek statik kopya: kare başına bir ışık pass'i (r2d_light_pass yorumu).
typedef struct {
    float ambient[4];
    float count[4];
    float mode[4];      // MSL LightU.mode ile AYNI SIRADA olmak zorunda
    float mode2[4];     // x = A/B bölme çizgisi (ekran px), 0 = kapalı
    float pos[R2D_MAX_LIGHTS][4];
    float col[R2D_MAX_LIGHTS][4];
    float shape[R2D_MAX_LIGHTS][4];   // x=siluet genliği y=cos(faz) z=sin(faz)
    float occ[R2D_MAX_OCCLUDERS][4];  // ışığı kesen dikdörtgenler (ekran px)
} R2DLightU;
static R2DLightU lightU;
static SDL_GPUSampler *sampler = NULL;        // NEAREST (pixel-art)
static SDL_GPUSampler *samplerLinear = NULL;  // LINEAR (glow vb.)
static SDL_GPUBuffer *vertexBuf = NULL;
static SDL_GPUTransferBuffer *transferBuf = NULL;
static R2DTexture *whiteTex = NULL;

static R2DVertex verts[R2D_MAX_VERTS];
static int vertCount = 0;
static R2DDraw draws[R2D_MAX_DRAWS];
static int drawCount = 0;
static R2DBlend curBlend = R2D_BLEND_ALPHA;
static SDL_Color curColor = { 255, 255, 255, 255 };
// Sonraki push yeni draw girdisi açsın (pass sınırlarında eski girdiye
// eklenmesin — target pass'in çizimi önceki karenin girdisine karışıyordu)
static bool forceNewDraw = false;

// Aktif kare
static SDL_GPUCommandBuffer *cmdBuf = NULL;
static SDL_GPUTexture *swapTex = NULL;
static Uint32 swapW = 0, swapH = 0;
static SDL_FColor clearColor;
static bool inFrame = false;

// Aktif render target pass'i (r2d_target_begin/end)
static R2DTexture *curTarget = NULL;
static bool targetClear = false;
static SDL_FColor targetClearColor;
// Target pass'i ana kare batch'inden ayrı tutmak için yedek sayaçlar
static int savedVertCount = 0, savedDrawCount = 0;

// Kare içinde yok edilmesi istenen texture'lar (submit sonrasına ertelenir)
static R2DTexture *deadList[R2D_MAX_DEAD];
static int deadCount = 0;

// ---------------------------------------------------------------------------
// Kurulum
// ---------------------------------------------------------------------------
static SDL_GPUShader *load_shader(SDL_GPUShaderStage stage, const char *entry,
                                  int numUniformBufs, int numSamplers)
{
    SDL_GPUShaderCreateInfo ci = {
        .code = (const Uint8 *)MSL_SOURCE,
        .code_size = strlen(MSL_SOURCE),
        .entrypoint = entry,
        .format = SDL_GPU_SHADERFORMAT_MSL,
        .stage = stage,
        .num_uniform_buffers = (Uint32)numUniformBufs,
        .num_samplers = (Uint32)numSamplers,
    };
    return SDL_CreateGPUShader(gpu, &ci);
}

// blend == R2D_BLEND_COUNT dahili anlam taşır: MULTIPLY (ışık pass'i,
// out = src*dst — sahne ışık haritasıyla çarpılır)
static SDL_GPUGraphicsPipeline *make_pipeline(SDL_GPUShader *vs, SDL_GPUShader *fs,
                                              R2DBlend blend,
                                              SDL_GPUTextureFormat format)
{
    SDL_GPUColorTargetBlendState bs = {
        .enable_blend = true,
        .color_blend_op = SDL_GPU_BLENDOP_ADD,
        .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
        .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
        .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
        .dst_color_blendfactor = (blend == R2D_BLEND_ADD)
                               ? SDL_GPU_BLENDFACTOR_ONE
                               : SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .dst_alpha_blendfactor = (blend == R2D_BLEND_ADD)
                               ? SDL_GPU_BLENDFACTOR_ONE
                               : SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
    };
    if (blend == R2D_BLEND_COUNT) {   // multiply: sahne * ışık, alpha dokunulmaz
        bs.src_color_blendfactor = SDL_GPU_BLENDFACTOR_DST_COLOR;
        bs.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
        bs.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
        bs.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    }
    SDL_GPUColorTargetDescription target = { .format = format, .blend_state = bs };
    SDL_GPUVertexBufferDescription vbd = {
        .slot = 0,
        .pitch = sizeof(R2DVertex),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
    };
    SDL_GPUVertexAttribute attrs[3] = {
        { .location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = 0 },
        { .location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = 8 },
        { .location = 2, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset = 16 },
    };
    SDL_GPUGraphicsPipelineCreateInfo ci = {
        .vertex_shader = vs,
        .fragment_shader = fs,
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_input_state = {
            .vertex_buffer_descriptions = &vbd,
            .num_vertex_buffers = 1,
            .vertex_attributes = attrs,
            .num_vertex_attributes = 3,
        },
        .target_info = {
            .color_target_descriptions = &target,
            .num_color_targets = 1,
        },
    };
    return SDL_CreateGPUGraphicsPipeline(gpu, &ci);
}

R2D *r2d_init(SDL_Window *window)
{
    win = window;

    gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL, false, NULL);
    if (gpu == NULL) {
        SDL_Log("r2d: GPU cihazi acilamadi: %s", SDL_GetError());
        return NULL;
    }
    if (!SDL_ClaimWindowForGPUDevice(gpu, win)) {
        SDL_Log("r2d: pencere GPU'ya baglanamadi: %s", SDL_GetError());
        return NULL;
    }
    SDL_SetGPUSwapchainParameters(gpu, win, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                  SDL_GPU_PRESENTMODE_VSYNC);

    SDL_GPUShader *vs = load_shader(SDL_GPU_SHADERSTAGE_VERTEX, "vs_main", 1, 0);
    SDL_GPUShader *fs = load_shader(SDL_GPU_SHADERSTAGE_FRAGMENT, "fs_main", 0, 1);
    if (vs == NULL || fs == NULL) {
        SDL_Log("r2d: shader derlenemedi: %s", SDL_GetError());
        return NULL;
    }
    SDL_GPUTextureFormat swapFmt = SDL_GetGPUSwapchainTextureFormat(gpu, win);
    for (int b = 0; b < R2D_BLEND_COUNT; b++) {
        pipeline[b] = make_pipeline(vs, fs, (R2DBlend)b, swapFmt);
        targetPipeline[b] = make_pipeline(vs, fs, (R2DBlend)b,
                                          SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM);
        if (pipeline[b] == NULL || targetPipeline[b] == NULL) {
            SDL_Log("r2d: pipeline kurulamadi: %s", SDL_GetError());
            return NULL;
        }
    }
    // Işık pass'i: kendi fragment shader'ı (1 uniform buffer, texture yok)
    SDL_GPUShader *fsLight = load_shader(SDL_GPU_SHADERSTAGE_FRAGMENT, "fs_light", 1, 0);
    if (fsLight == NULL) {
        SDL_Log("r2d: isik shader'i derlenemedi: %s", SDL_GetError());
        return NULL;
    }
    lightPipeline = make_pipeline(vs, fsLight, R2D_BLEND_COUNT, swapFmt);
    lightPipelineTarget = make_pipeline(vs, fsLight, R2D_BLEND_COUNT,
                                        SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM);
    if (lightPipeline == NULL || lightPipelineTarget == NULL) {
        SDL_Log("r2d: isik pipeline kurulamadi: %s", SDL_GetError());
        return NULL;
    }
    SDL_ReleaseGPUShader(gpu, fsLight);

    // Post-fx: kendi fragment shader'ı (1 uniform buffer + sahne texture'ı)
    SDL_GPUShader *fsPost = load_shader(SDL_GPU_SHADERSTAGE_FRAGMENT, "fs_postfx", 1, 1);
    if (fsPost == NULL) {
        SDL_Log("r2d: postfx shader'i derlenemedi: %s", SDL_GetError());
        return NULL;
    }
    postfxPipeline = make_pipeline(vs, fsPost, R2D_BLEND_ALPHA, swapFmt);
    if (postfxPipeline == NULL) {
        SDL_Log("r2d: postfx pipeline kurulamadi: %s", SDL_GetError());
        return NULL;
    }
    SDL_ReleaseGPUShader(gpu, fsPost);

    SDL_ReleaseGPUShader(gpu, vs);   // pipeline'lara bağlandı — referansımız gereksiz
    SDL_ReleaseGPUShader(gpu, fs);

    SDL_GPUSamplerCreateInfo sci = {
        .min_filter = SDL_GPU_FILTER_NEAREST,   // pixel-art: keskin
        .mag_filter = SDL_GPU_FILTER_NEAREST,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    };
    sampler = SDL_CreateGPUSampler(gpu, &sci);

    sci.min_filter = SDL_GPU_FILTER_LINEAR;
    sci.mag_filter = SDL_GPU_FILTER_LINEAR;
    samplerLinear = SDL_CreateGPUSampler(gpu, &sci);

    SDL_GPUBufferCreateInfo bci = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = sizeof(verts),
    };
    vertexBuf = SDL_CreateGPUBuffer(gpu, &bci);

    SDL_GPUTransferBufferCreateInfo tci = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = sizeof(verts),
    };
    transferBuf = SDL_CreateGPUTransferBuffer(gpu, &tci);

    // Post-fx'in tam-ekran quad'ı: pos 0..1 (viewport uniform'u {1,1} basılır,
    // NDC dönüşümü birebir kalır) — pencere boyutundan bağımsız, bir kez yüklenir
    static const R2DVertex FSQUAD[6] = {
        { 0, 0, 0, 0, 1, 1, 1, 1 }, { 1, 0, 1, 0, 1, 1, 1, 1 },
        { 0, 1, 0, 1, 1, 1, 1, 1 }, { 1, 0, 1, 0, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1 }, { 0, 1, 0, 1, 1, 1, 1, 1 },
    };
    SDL_GPUBufferCreateInfo qci = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = sizeof(FSQUAD),
    };
    fsQuadBuf = SDL_CreateGPUBuffer(gpu, &qci);
    if (fsQuadBuf != NULL) {
        SDL_GPUTransferBufferCreateInfo qtci = {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = sizeof(FSQUAD),
        };
        SDL_GPUTransferBuffer *qtb = SDL_CreateGPUTransferBuffer(gpu, &qtci);
        void *map = SDL_MapGPUTransferBuffer(gpu, qtb, false);
        memcpy(map, FSQUAD, sizeof(FSQUAD));
        SDL_UnmapGPUTransferBuffer(gpu, qtb);
        SDL_GPUCommandBuffer *cb = SDL_AcquireGPUCommandBuffer(gpu);
        SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cb);
        SDL_GPUTransferBufferLocation srcLoc = { .transfer_buffer = qtb };
        SDL_GPUBufferRegion dstReg = { .buffer = fsQuadBuf, .size = sizeof(FSQUAD) };
        SDL_UploadToGPUBuffer(cp, &srcLoc, &dstReg, false);
        SDL_EndGPUCopyPass(cp);
        SDL_SubmitGPUCommandBuffer(cb);
        SDL_ReleaseGPUTransferBuffer(gpu, qtb);
    }

    if (sampler == NULL || samplerLinear == NULL ||
        vertexBuf == NULL || transferBuf == NULL || fsQuadBuf == NULL) {
        SDL_Log("r2d: kaynaklar kurulamadi: %s", SDL_GetError());
        return NULL;
    }

    Uint32 white = 0xFFFFFFFFu;
    whiteTex = r2d_texture_from_pixels(&CTX, &white, 1, 1);
    if (whiteTex == NULL) return NULL;

    SDL_Log("r2d: hazir (surucu: %s)", SDL_GetGPUDeviceDriver(gpu));
    return &CTX;
}

void r2d_quit(R2D *ctx)
{
    (void)ctx;
    if (gpu == NULL) return;
    SDL_WaitForGPUIdle(gpu);
    if (whiteTex) { SDL_ReleaseGPUTexture(gpu, whiteTex->tex); SDL_free(whiteTex); }
    if (sceneTex) { SDL_ReleaseGPUTexture(gpu, sceneTex->tex); SDL_free(sceneTex); }
    if (fsQuadBuf) SDL_ReleaseGPUBuffer(gpu, fsQuadBuf);
    if (transferBuf) SDL_ReleaseGPUTransferBuffer(gpu, transferBuf);
    if (vertexBuf) SDL_ReleaseGPUBuffer(gpu, vertexBuf);
    if (sampler) SDL_ReleaseGPUSampler(gpu, sampler);
    if (samplerLinear) SDL_ReleaseGPUSampler(gpu, samplerLinear);
    for (int b = 0; b < R2D_BLEND_COUNT; b++) {
        if (pipeline[b]) SDL_ReleaseGPUGraphicsPipeline(gpu, pipeline[b]);
        if (targetPipeline[b]) SDL_ReleaseGPUGraphicsPipeline(gpu, targetPipeline[b]);
    }
    if (lightPipeline) SDL_ReleaseGPUGraphicsPipeline(gpu, lightPipeline);
    if (lightPipelineTarget) SDL_ReleaseGPUGraphicsPipeline(gpu, lightPipelineTarget);
    if (postfxPipeline) SDL_ReleaseGPUGraphicsPipeline(gpu, postfxPipeline);
    SDL_ReleaseWindowFromGPUDevice(gpu, win);
    SDL_DestroyGPUDevice(gpu);
    gpu = NULL;
}

void r2d_output_size(R2D *ctx, int *w, int *h)
{
    (void)ctx;
    if (curTarget != NULL) {
        *w = (int)curTarget->w;
        *h = (int)curTarget->h;
    } else if (inFrame) {
        *w = (int)swapW;
        *h = (int)swapH;
    } else {
        SDL_GetWindowSizeInPixels(win, w, h);
    }
}

// ---------------------------------------------------------------------------
// Texture
// ---------------------------------------------------------------------------
static R2DTexture *texture_create(int w, int h, bool isTarget)
{
    SDL_GPUTextureCreateInfo tci = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER
               | (isTarget ? SDL_GPU_TEXTUREUSAGE_COLOR_TARGET : 0),
        .width = (Uint32)w,
        .height = (Uint32)h,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };
    SDL_GPUTexture *gtex = SDL_CreateGPUTexture(gpu, &tci);
    if (gtex == NULL) {
        SDL_Log("r2d: texture olusturulamadi: %s", SDL_GetError());
        return NULL;
    }
    R2DTexture *t = SDL_malloc(sizeof(R2DTexture));
    t->tex = gtex;
    t->w = (float)w;
    t->h = (float)h;
    t->modR = t->modG = t->modB = t->modA = 255;
    t->isTarget = isTarget;
    // HATA DÜZELTMESİ: SDL_malloc sıfırlamaz — bu alan atanmadığı için her yeni
    // doku ÇÖP değerle geliyordu, yani rastgele NEAREST ya da LINEAR örnekleniyordu
    // (pixel-art'ta LINEAR = bulanık sprite). Varsayılan NEAREST.
    t->linear = false;
    t->opX = 0.0f; t->opY = 0.0f;
    t->opW = (float)w; t->opH = (float)h;   // varsayılan: tüm alan opak sayılır
    return t;
}

R2DTexture *r2d_texture_from_pixels(R2D *ctx, const void *rgba, int w, int h)
{
    (void)ctx;
    R2DTexture *t = texture_create(w, h, false);
    if (t == NULL) return NULL;

    // ALPHA BOUNDING BOX taraması (Unity "Trim" muadili). Gölge ayak noktası
    // ve blob genişliği bundan çıkar; alpha eşiği 8/255 (neredeyse şeffaf sayılır).
    {
        const unsigned char *px = (const unsigned char *)rgba;
        int minX = w, minY = h, maxX = -1, maxY = -1;
        for (int yy = 0; yy < h; yy++) {
            for (int xx = 0; xx < w; xx++) {
                if (px[(yy * w + xx) * 4 + 3] > 8) {
                    if (xx < minX) minX = xx;
                    if (xx > maxX) maxX = xx;
                    if (yy < minY) minY = yy;
                    if (yy > maxY) maxY = yy;
                }
            }
        }
        if (maxX >= minX && maxY >= minY) {
            t->opX = (float)minX;          t->opY = (float)minY;
            t->opW = (float)(maxX - minX + 1);
            t->opH = (float)(maxY - minY + 1);
        }
    }

    Uint32 byteCount = (Uint32)(w * h * 4);
    SDL_GPUTransferBufferCreateInfo up = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = byteCount,
    };
    SDL_GPUTransferBuffer *tb = SDL_CreateGPUTransferBuffer(gpu, &up);
    void *map = SDL_MapGPUTransferBuffer(gpu, tb, false);
    memcpy(map, rgba, byteCount);
    SDL_UnmapGPUTransferBuffer(gpu, tb);

    SDL_GPUCommandBuffer *cb = SDL_AcquireGPUCommandBuffer(gpu);
    SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cb);
    SDL_GPUTextureTransferInfo srcInfo = { .transfer_buffer = tb };
    SDL_GPUTextureRegion dstInfo = {
        .texture = t->tex, .w = (Uint32)w, .h = (Uint32)h, .d = 1,
    };
    SDL_UploadToGPUTexture(cp, &srcInfo, &dstInfo, false);
    SDL_EndGPUCopyPass(cp);
    SDL_SubmitGPUCommandBuffer(cb);
    SDL_ReleaseGPUTransferBuffer(gpu, tb);
    return t;
}

R2DTexture *r2d_texture_load(R2D *ctx, const char *path)
{
    int w, h, comp;
    unsigned char *pixels = stbi_load(path, &w, &h, &comp, 4);   // hep RGBA
    if (pixels == NULL) {
        SDL_Log("Yuklenemedi (%s): %s", path, stbi_failure_reason());
        return NULL;
    }
    R2DTexture *t = r2d_texture_from_pixels(ctx, pixels, w, h);
    stbi_image_free(pixels);
    return t;
}

R2DTexture *r2d_target_create(R2D *ctx, int w, int h)
{
    (void)ctx;
    return texture_create(w, h, true);
}

void r2d_texture_destroy(R2D *ctx, R2DTexture *tex)
{
    (void)ctx;
    if (tex == NULL) return;
    if (inFrame && deadCount < R2D_MAX_DEAD) {
        deadList[deadCount++] = tex;   // batch hâlâ referans tutuyor — ertele
        return;
    }
    SDL_ReleaseGPUTexture(gpu, tex->tex);
    SDL_free(tex);
}

void r2d_texture_opaque_bounds(const R2DTexture *tex, float *x, float *y,
                               float *w, float *h)
{
    if (tex == NULL) { if (x) *x = 0; if (y) *y = 0; if (w) *w = 0; if (h) *h = 0; return; }
    if (x) *x = tex->opX;
    if (y) *y = tex->opY;
    if (w) *w = tex->opW;
    if (h) *h = tex->opH;
}

void r2d_texture_size(const R2DTexture *tex, float *w, float *h)
{
    *w = tex->w;
    *h = tex->h;
}

void r2d_texture_set_color_mod(R2DTexture *tex, Uint8 r, Uint8 g, Uint8 b)
{
    if (tex == NULL) return;
    tex->modR = r; tex->modG = g; tex->modB = b;
}

void r2d_texture_set_alpha_mod(R2DTexture *tex, Uint8 a)
{
    if (tex == NULL) return;
    tex->modA = a;
}

void r2d_texture_set_linear(R2DTexture *tex, bool linear)
{
    if (tex == NULL) return;
    tex->linear = linear;
}

// ---------------------------------------------------------------------------
// Batch
// ---------------------------------------------------------------------------
void r2d_set_blend(R2D *ctx, R2DBlend blend)
{
    (void)ctx;
    curBlend = blend;
}

void r2d_set_color(R2D *ctx, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    (void)ctx;
    curColor = (SDL_Color){ r, g, b, a };
}

static void push_quad(R2DTexture *tex, float dx, float dy, float dw, float dh,
                      float u0, float v0, float u1, float v1, SDL_Color c)
{
    if (vertCount + 6 > R2D_MAX_VERTS || drawCount >= R2D_MAX_DRAWS)
        return;   // kare bütçesi doldu — sessiz düşür

    R2DDraw *d = (drawCount > 0) ? &draws[drawCount - 1] : NULL;
    if (d == NULL || forceNewDraw || d->tex != tex || d->blend != curBlend) {
        d = &draws[drawCount++];
        d->tex = tex;
        d->blend = curBlend;
        d->first = vertCount;
        d->count = 0;
        forceNewDraw = false;
    }

    // Texture mod (SDL_SetTextureColorMod deseni) vertex rengine işlenir
    float r = (c.r / 255.0f) * (tex->modR / 255.0f);
    float g = (c.g / 255.0f) * (tex->modG / 255.0f);
    float b = (c.b / 255.0f) * (tex->modB / 255.0f);
    float a = (c.a / 255.0f) * (tex->modA / 255.0f);
    R2DVertex q[6] = {
        { dx,      dy,      u0, v0, r, g, b, a },
        { dx + dw, dy,      u1, v0, r, g, b, a },
        { dx,      dy + dh, u0, v1, r, g, b, a },
        { dx + dw, dy,      u1, v0, r, g, b, a },
        { dx + dw, dy + dh, u1, v1, r, g, b, a },
        { dx,      dy + dh, u0, v1, r, g, b, a },
    };
    memcpy(&verts[vertCount], q, sizeof(q));
    vertCount += 6;
    d->count += 6;
}

// Serbest 4 köşeli quad — push_quad'ın eksen-hizalı olmayan kardeşi.
// Motorda rotation/shear yoktu; gölge izdüşümü için gerekli (sprite'ın tabanı
// yerinde kalır, tepesi güneşten uzağa yatar). Köşe sırası: TL, TR, BL, BR.
static void push_quad_corners(R2DTexture *tex, const SDL_FPoint c4[4],
                              float u0, float v0, float u1, float v1, SDL_Color c)
{
    if (vertCount + 6 > R2D_MAX_VERTS || drawCount >= R2D_MAX_DRAWS)
        return;

    R2DDraw *d = (drawCount > 0) ? &draws[drawCount - 1] : NULL;
    if (d == NULL || forceNewDraw || d->tex != tex || d->blend != curBlend) {
        d = &draws[drawCount++];
        d->tex = tex;
        d->blend = curBlend;
        d->first = vertCount;
        d->count = 0;
        forceNewDraw = false;
    }

    float r = (c.r / 255.0f) * (tex->modR / 255.0f);
    float g = (c.g / 255.0f) * (tex->modG / 255.0f);
    float b = (c.b / 255.0f) * (tex->modB / 255.0f);
    float a = (c.a / 255.0f) * (tex->modA / 255.0f);
    R2DVertex q[6] = {
        { c4[0].x, c4[0].y, u0, v0, r, g, b, a },   // TL
        { c4[1].x, c4[1].y, u1, v0, r, g, b, a },   // TR
        { c4[2].x, c4[2].y, u0, v1, r, g, b, a },   // BL
        { c4[1].x, c4[1].y, u1, v0, r, g, b, a },   // TR
        { c4[3].x, c4[3].y, u1, v1, r, g, b, a },   // BR
        { c4[2].x, c4[2].y, u0, v1, r, g, b, a },   // BL
    };
    memcpy(&verts[vertCount], q, sizeof(q));
    vertCount += 6;
    d->count += 6;
}

void r2d_draw_texture_quad(R2D *ctx, R2DTexture *tex, const SDL_FRect *src,
                           const SDL_FPoint corners[4], SDL_Color col)
{
    (void)ctx;
    if (tex == NULL || corners == NULL) return;
    float u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;
    if (src != NULL) {
        u0 = src->x / tex->w;             v0 = src->y / tex->h;
        u1 = (src->x + src->w) / tex->w;  v1 = (src->y + src->h) / tex->h;
    }
    push_quad_corners(tex, corners, u0, v0, u1, v1, col);
}

void r2d_draw_texture_flip(R2D *ctx, R2DTexture *tex, const SDL_FRect *src,
                           const SDL_FRect *dst, bool flipH)
{
    (void)ctx;
    if (tex == NULL || dst == NULL) return;

    // SDL_RenderTexture davranışı: kaynak dikdörtgen texture sınırlarını
    // aşarsa KIRP ve dst'yi orantılı küçült (aşan bölge kenar pikselini
    // smear etmesin — chest_small 48'lik clip / 32'lik sheet vakası)
    SDL_FRect s = (src != NULL) ? *src : (SDL_FRect){ 0, 0, tex->w, tex->h };
    SDL_FRect d = *dst;
    if (s.x < 0) { d.x -= s.x * (d.w / s.w); d.w += s.x * (d.w / s.w); s.w += s.x; s.x = 0; }
    if (s.y < 0) { d.y -= s.y * (d.h / s.h); d.h += s.y * (d.h / s.h); s.h += s.y; s.y = 0; }
    if (s.x + s.w > tex->w) { float over = s.x + s.w - tex->w; d.w -= over * (d.w / s.w); s.w -= over; }
    if (s.y + s.h > tex->h) { float over = s.y + s.h - tex->h; d.h -= over * (d.h / s.h); s.h -= over; }
    if (s.w <= 0.0f || s.h <= 0.0f || d.w <= 0.0f || d.h <= 0.0f) return;

    float u0 = s.x / tex->w;
    float v0 = s.y / tex->h;
    float u1 = (s.x + s.w) / tex->w;
    float v1 = (s.y + s.h) / tex->h;
    if (flipH) { float t = u0; u0 = u1; u1 = t; }
    push_quad(tex, d.x, d.y, d.w, d.h, u0, v0, u1, v1,
              (SDL_Color){ 255, 255, 255, 255 });
}

void r2d_draw_texture(R2D *ctx, R2DTexture *tex,
                      const SDL_FRect *src, const SDL_FRect *dst)
{
    r2d_draw_texture_flip(ctx, tex, src, dst, false);
}

void r2d_draw_9grid(R2D *ctx, R2DTexture *tex, const SDL_FRect *src,
                    float corner, float scale, const SDL_FRect *dst)
{
    float c = corner, cd = corner * scale;
    float sx[4] = { src->x, src->x + c, src->x + src->w - c, src->x + src->w };
    float sy[4] = { src->y, src->y + c, src->y + src->h - c, src->y + src->h };
    float dx[4] = { dst->x, dst->x + cd, dst->x + dst->w - cd, dst->x + dst->w };
    float dy[4] = { dst->y, dst->y + cd, dst->y + dst->h - cd, dst->y + dst->h };

    for (int ry = 0; ry < 3; ry++) {
        for (int rx = 0; rx < 3; rx++) {
            SDL_FRect s = { sx[rx], sy[ry], sx[rx+1] - sx[rx], sy[ry+1] - sy[ry] };
            SDL_FRect d = { dx[rx], dy[ry], dx[rx+1] - dx[rx], dy[ry+1] - dy[ry] };
            if (d.w <= 0.0f || d.h <= 0.0f) continue;
            r2d_draw_texture(ctx, tex, &s, &d);
        }
    }
}

// Işık görsel modu (F4 ile döngü) — yalnız GÖRSELİ etkiler.
// Oynanış sorgusu (lamp_level_at) CPU'da ve bundan bağımsız: mod değişince
// ölülerin bastırıldığı sınır DEĞİŞMEZ, sadece nasıl göründüğü değişir.
static float s_lightQuantSteps = 0.0f;   // 0 = serbest (sürekli gradyan)
static float s_lightGridPx     = 0.0f;   // 0 = ızgara yok
static float s_lightGamma      = 0.0f;   // 0=sRGB toplama 1=lineer 2=lineer+ACES
static float s_lightFalloff    = 0.0f;   // 0=plato+ss 1=Karis 2=URP 3=knee
static float s_lightDither     = 0.0f;   // 0=IGN 1=Bayer 2=dünya-IGN 3=yok
static float s_lightFParam     = 41.0f;  // Karis d0 / URP k
static float s_lightCamX = 0.0f, s_lightCamY = 0.0f;
static float s_lightSplitX = 0.0f;   // 0 = A/B karşılaştırma kapalı
static float s_lightAniso  = 1.0f;   // 1 = daire · <1 basık elips · >1 dikey
static float s_lightYOff   = 0.0f;   // ışık merkezinin dikey kayması (ekran px)

void r2d_light_set_split(float screenX) { s_lightSplitX = screenX; }

void r2d_light_set_shape(float aniso, float yOffset)
{
    s_lightAniso = aniso;
    s_lightYOff  = yOffset;
}

static SDL_FRect s_occ[R2D_MAX_OCCLUDERS];
static int   s_occCount = 0;
static float s_occStrength = 0.0f;

void r2d_light_set_occluders(const SDL_FRect *rects, int count, float strength)
{
    if (count > R2D_MAX_OCCLUDERS) count = R2D_MAX_OCCLUDERS;
    if (count < 0) count = 0;
    for (int i = 0; i < count; i++) s_occ[i] = rects[i];
    s_occCount    = count;
    s_occStrength = strength;
}

void r2d_light_set_mode(float quantSteps, float gridPx)
{
    s_lightQuantSteps = quantSteps;
    s_lightGridPx     = gridPx;
}

void r2d_light_set_model(float gammaMode, float falloffMode, float ditherMode,
                        float falloffParam)
{
    s_lightGamma   = gammaMode;
    s_lightFalloff = falloffMode;
    s_lightDither  = ditherMode;
    s_lightFParam  = falloffParam;
}

void r2d_light_set_camera(float camX, float camY)
{
    s_lightCamX = camX;
    s_lightCamY = camY;
}

void r2d_light_pass(R2D *ctx, float ambientR, float ambientG, float ambientB,
                    const R2DLight *lights, int count)
{
    if (lightPipeline == NULL || !inFrame || curTarget != NULL)
        return;   // yalnız ekran karesi içinde
    if (count > R2D_MAX_LIGHTS) count = R2D_MAX_LIGHTS;

    lightU.ambient[0] = ambientR;
    lightU.ambient[1] = ambientG;
    lightU.ambient[2] = ambientB;
    lightU.ambient[3] = 1.0f;
    lightU.count[0] = (float)count;
    lightU.count[1] = s_lightQuantSteps;   // 0 = serbest mod
    lightU.count[2] = s_lightGridPx;       // 0 = ızgaraya yapışma yok
    lightU.count[3] = s_lightCamY;         // dünya-çivili dither için
    lightU.ambient[3] = s_lightCamX;
    lightU.mode[0] = s_lightGamma;
    lightU.mode[1] = s_lightFalloff;
    lightU.mode[2] = s_lightDither;
    lightU.mode[3] = s_lightFParam;
    lightU.mode2[0] = s_lightSplitX;
    lightU.mode2[1] = s_lightAniso;
    lightU.mode2[2] = s_lightYOff;
    lightU.mode2[3] = (s_occStrength > 0.01f) ? (float)s_occCount : 0.0f;
    for (int i = 0; i < R2D_MAX_OCCLUDERS; i++) {
        bool live = (i < s_occCount);
        lightU.occ[i][0] = live ? s_occ[i].x : 0.0f;
        lightU.occ[i][1] = live ? s_occ[i].y : 0.0f;
        lightU.occ[i][2] = live ? s_occ[i].w : 0.0f;
        lightU.occ[i][3] = live ? s_occ[i].h : 0.0f;
    }
    for (int i = 0; i < count; i++) {
        lightU.pos[i][0] = lights[i].x;
        lightU.pos[i][1] = lights[i].y;
        lightU.pos[i][2] = (lights[i].radius > 1.0f) ? lights[i].radius : 1.0f;
        lightU.pos[i][3] = lights[i].yOff;
        lightU.col[i][0] = lights[i].r;
        lightU.col[i][1] = lights[i].g;
        lightU.col[i][2] = lights[i].b;
        lightU.col[i][3] = lights[i].intensity;
        lightU.shape[i][0] = lights[i].wobAmp;
        lightU.shape[i][1] = lights[i].wobC;
        lightU.shape[i][2] = lights[i].wobS;
        lightU.shape[i][3] = (lights[i].aniso > 0.05f) ? lights[i].aniso : 1.0f;
    }

    // Tam-ekran quad'ı R2D_BLEND_COUNT işaretli draw olarak batch'e koy —
    // flush bu işareti görünce light pipeline + fragment uniform'larını bağlar.
    R2DBlend saved = curBlend;
    curBlend = R2D_BLEND_COUNT;
    forceNewDraw = true;
    int w, h;
    r2d_output_size(ctx, &w, &h);
    push_quad(whiteTex, 0, 0, (float)w, (float)h, 0, 0, 1, 1,
              (SDL_Color){ 255, 255, 255, 255 });
    forceNewDraw = true;   // sonraki normal çizim de taze girdiyle başlasın
    curBlend = saved;
}

void r2d_fill_rect(R2D *ctx, const SDL_FRect *r)
{
    (void)ctx;
    if (r == NULL) return;
    push_quad(whiteTex, r->x, r->y, r->w, r->h, 0, 0, 1, 1, curColor);
}

void r2d_rect(R2D *ctx, const SDL_FRect *r)
{
    if (r == NULL) return;
    r2d_fill_rect(ctx, &(SDL_FRect){ r->x, r->y, r->w, 1 });
    r2d_fill_rect(ctx, &(SDL_FRect){ r->x, r->y + r->h - 1, r->w, 1 });
    r2d_fill_rect(ctx, &(SDL_FRect){ r->x, r->y, 1, r->h });
    r2d_fill_rect(ctx, &(SDL_FRect){ r->x + r->w - 1, r->y, 1, r->h });
}

void r2d_line(R2D *ctx, float x1, float y1, float x2, float y2)
{
    (void)ctx;
    float dx = x2 - x1, dy = y2 - y1;
    float len = SDL_sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return;
    float nx = -dy / len * 0.5f, ny = dx / len * 0.5f;

    if (vertCount + 6 > R2D_MAX_VERTS || drawCount >= R2D_MAX_DRAWS) return;
    R2DDraw *d = (drawCount > 0) ? &draws[drawCount - 1] : NULL;
    if (d == NULL || forceNewDraw || d->tex != whiteTex || d->blend != curBlend) {
        d = &draws[drawCount++];
        d->tex = whiteTex; d->blend = curBlend;
        d->first = vertCount; d->count = 0;
        forceNewDraw = false;
    }
    float r = curColor.r / 255.0f, g = curColor.g / 255.0f;
    float b = curColor.b / 255.0f, a = curColor.a / 255.0f;
    R2DVertex q[6] = {
        { x1 + nx, y1 + ny, 0, 0, r, g, b, a },
        { x2 + nx, y2 + ny, 1, 0, r, g, b, a },
        { x1 - nx, y1 - ny, 0, 1, r, g, b, a },
        { x2 + nx, y2 + ny, 1, 0, r, g, b, a },
        { x2 - nx, y2 - ny, 1, 1, r, g, b, a },
        { x1 - nx, y1 - ny, 0, 1, r, g, b, a },
    };
    memcpy(&verts[vertCount], q, sizeof(q));
    vertCount += 6;
    d->count += 6;
}

void r2d_point(R2D *ctx, float x, float y)
{
    r2d_fill_rect(ctx, &(SDL_FRect){ x, y, 1, 1 });
}

// ---------------------------------------------------------------------------
// Batch'i verilen GPU texture'ına boşalt (kare ve target pass'lerin ortak yolu)
// ---------------------------------------------------------------------------
static void flush_to(SDL_GPUCommandBuffer *cb, SDL_GPUTexture *dst,
                     float outW, float outH, bool clear, SDL_FColor cc,
                     SDL_GPUGraphicsPipeline **pipes, int firstVert)
{
    int count = vertCount - firstVert;
    if (count <= 0 && !clear) return;

    // 1) Bu aralığın vertexlerini GPU'ya kopyala
    if (count > 0) {
        void *map = SDL_MapGPUTransferBuffer(gpu, transferBuf, true);
        memcpy(map, &verts[firstVert], (size_t)count * sizeof(R2DVertex));
        SDL_UnmapGPUTransferBuffer(gpu, transferBuf);

        SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cb);
        SDL_GPUTransferBufferLocation srcLoc = { .transfer_buffer = transferBuf };
        SDL_GPUBufferRegion dstReg = {
            .buffer = vertexBuf, .size = (Uint32)(count * sizeof(R2DVertex)),
        };
        SDL_UploadToGPUBuffer(cp, &srcLoc, &dstReg, true);
        SDL_EndGPUCopyPass(cp);
    }

    // 2) Render pass
    SDL_GPUColorTargetInfo target = {
        .texture = dst,
        .clear_color = cc,
        .load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD,
        .store_op = SDL_GPU_STOREOP_STORE,
    };
    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(cb, &target, 1, NULL);

    float viewport[2] = { outW, outH };
    SDL_PushGPUVertexUniformData(cb, 0, viewport, sizeof(viewport));

    SDL_GPUBufferBinding vb = { .buffer = vertexBuf, .offset = 0 };
    SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);

    R2DBlend boundBlend = (R2DBlend)-1;
    for (int i = 0; i < drawCount; i++) {
        if (draws[i].first < firstVert) continue;   // önceki pass'in aralığı

        // Işık pass'i işareti: light pipeline + fragment uniform, sampler yok
        // (hedef formatına göre swapchain/RGBA8 varyantı — post-fx sahne target'ı)
        if (draws[i].blend == R2D_BLEND_COUNT) {
            SDL_BindGPUGraphicsPipeline(pass, (pipes == pipeline)
                                              ? lightPipeline : lightPipelineTarget);
            SDL_PushGPUFragmentUniformData(cb, 0, &lightU, sizeof(lightU));
            SDL_DrawGPUPrimitives(pass, (Uint32)draws[i].count, 1,
                                  (Uint32)(draws[i].first - firstVert), 0);
            boundBlend = (R2DBlend)-1;   // pipeline değişti — sonraki draw yeniden bağlasın
            continue;
        }

        if (draws[i].blend != boundBlend) {
            boundBlend = draws[i].blend;
            SDL_BindGPUGraphicsPipeline(pass, pipes[boundBlend]);
        }
        SDL_GPUTextureSamplerBinding tsb = {
            .texture = draws[i].tex->tex,
            .sampler = draws[i].tex->linear ? samplerLinear : sampler,
        };
        SDL_BindGPUFragmentSamplers(pass, 0, &tsb, 1);
        SDL_DrawGPUPrimitives(pass, (Uint32)draws[i].count, 1,
                              (Uint32)(draws[i].first - firstVert), 0);
    }
    SDL_EndGPURenderPass(pass);
}

// ---------------------------------------------------------------------------
// Kare yaşam döngüsü
// ---------------------------------------------------------------------------
bool r2d_begin_frame(R2D *ctx, Uint8 clearR, Uint8 clearG, Uint8 clearB)
{
    (void)ctx;
    vertCount = 0;
    drawCount = 0;
    curBlend = R2D_BLEND_ALPHA;

    cmdBuf = SDL_AcquireGPUCommandBuffer(gpu);
    if (cmdBuf == NULL) {
        SDL_Log("r2d: komut tamponu alinamadi: %s", SDL_GetError());
        return false;
    }
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdBuf, win, &swapTex, &swapW, &swapH) ||
        swapTex == NULL) {
        SDL_SubmitGPUCommandBuffer(cmdBuf);   // bu kare çizim yok
        cmdBuf = NULL;
        return false;
    }
    clearColor = (SDL_FColor){ clearR / 255.0f, clearG / 255.0f, clearB / 255.0f, 1.0f };
    inFrame = true;
    return true;
}

// Post-fx pass'i: sceneTex'i tam-ekran quad'la swapchain'e bas (fs_postfx)
static void postfx_pass(void)
{
    SDL_GPUColorTargetInfo target = {
        .texture = swapTex,
        .load_op = SDL_GPU_LOADOP_DONT_CARE,   // quad her pikseli yazar
        .store_op = SDL_GPU_STOREOP_STORE,
    };
    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(cmdBuf, &target, 1, NULL);
    SDL_BindGPUGraphicsPipeline(pass, postfxPipeline);
    float viewport[2] = { 1.0f, 1.0f };   // quad pos 0..1 — NDC dönüşümü birebir
    SDL_PushGPUVertexUniformData(cmdBuf, 0, viewport, sizeof(viewport));
    frameNo = (frameNo + 1) & 1023;   // float hassasiyeti için sarmalı sayaç
    postfxU.c[3] = (float)frameNo;
    SDL_PushGPUFragmentUniformData(cmdBuf, 0, &postfxU, sizeof(postfxU));
    SDL_GPUBufferBinding vb = { .buffer = fsQuadBuf, .offset = 0 };
    SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
    SDL_GPUTextureSamplerBinding tsb = { .texture = sceneTex->tex, .sampler = sampler };
    SDL_BindGPUFragmentSamplers(pass, 0, &tsb, 1);
    SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
}

void r2d_end_frame(R2D *ctx)
{
    (void)ctx;
    if (cmdBuf == NULL) return;

    // Post-fx açıkken sahne offscreen target'a akar, oradan filtreyle ekrana.
    // Sahne target'ı pencere boyutunu izler (resize'da yeniden kurulur).
    if (postfxOn) {
        if (sceneTex == NULL ||
            (Uint32)sceneTex->w != swapW || (Uint32)sceneTex->h != swapH) {
            if (sceneTex) r2d_texture_destroy(NULL, sceneTex);   // ertelenir
            sceneTex = texture_create((int)swapW, (int)swapH, true);
        }
    }
    if (postfxOn && sceneTex != NULL) {
        flush_to(cmdBuf, sceneTex->tex, (float)swapW, (float)swapH, true,
                 clearColor, targetPipeline, 0);
        postfx_pass();
    } else {
        flush_to(cmdBuf, swapTex, (float)swapW, (float)swapH, true, clearColor,
                 pipeline, 0);
    }
    SDL_SubmitGPUCommandBuffer(cmdBuf);
    cmdBuf = NULL;
    inFrame = false;

    // Kare içinde istenmiş yıkımlar (SDL GPU tarafında da güvenli-ertelemeli)
    for (int i = 0; i < deadCount; i++) {
        SDL_ReleaseGPUTexture(gpu, deadList[i]->tex);
        SDL_free(deadList[i]);
    }
    deadCount = 0;
}

// ---------------------------------------------------------------------------
// Render target pass'leri — begin..end arası çizimler hedef texture'a gider.
// Kendi komut tamponuyla ANINDA gönderilir (ana karenin batch'i bozulmaz).
// ---------------------------------------------------------------------------
void r2d_target_begin(R2D *ctx, R2DTexture *target, bool clear,
                      Uint8 r, Uint8 g, Uint8 b)
{
    (void)ctx;
    if (target == NULL || !target->isTarget || curTarget != NULL) return;
    curTarget = target;
    targetClear = clear;
    targetClearColor = (SDL_FColor){ r / 255.0f, g / 255.0f, b / 255.0f, 1.0f };
    savedVertCount = vertCount;   // ana batch'in sonu = target aralığının başı
    savedDrawCount = drawCount;
    forceNewDraw = true;          // önceki pass'in girdisine EKLENME
}

void r2d_target_end(R2D *ctx)
{
    (void)ctx;
    if (curTarget == NULL) return;

    SDL_GPUCommandBuffer *cb = SDL_AcquireGPUCommandBuffer(gpu);
    if (cb != NULL) {
        flush_to(cb, curTarget->tex, curTarget->w, curTarget->h,
                 targetClear, targetClearColor, targetPipeline, savedVertCount);
        SDL_SubmitGPUCommandBuffer(cb);
    }

    // Target aralığındaki vertex/draw'ları ana batch'ten geri al
    vertCount = savedVertCount;
    drawCount = savedDrawCount;
    curTarget = NULL;
    forceNewDraw = true;   // sonraki ana-kare çizimi de taze girdiyle başlasın
}

// ---------------------------------------------------------------------------
// Post-fx
// ---------------------------------------------------------------------------
void r2d_postfx_set(R2D *ctx, const R2DPostFX *fx)
{
    (void)ctx;
    if (fx == NULL) {
        postfxOn = false;   // sceneTex dursun — tekrar açılışta hazır
        return;
    }
    postfxU.a[0] = fx->saturation;
    postfxU.a[1] = fx->warmth;
    postfxU.a[2] = fx->contrast;
    postfxU.a[3] = fx->brightness;
    postfxU.b[0] = fx->vignette;
    postfxU.b[1] = fx->vignetteRadius;
    postfxU.b[2] = fx->scanline;
    postfxU.b[3] = fx->gameboy;
    postfxU.c[0] = fx->posterize;
    postfxU.c[1] = fx->aberration;
    postfxU.c[2] = fx->grain;
    // c[3] = zaman — her kare postfx_pass içinde basılır
    postfxOn = true;
}
