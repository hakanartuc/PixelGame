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
// SHADER'LAR ARTIK BURADA DEĞİL — kaynak dil HLSL, yükleme dosyadan.
//
// Eskiden bu noktada `static const char *MSL_SOURCE` adlı 288 satırlık gömülü
// bir Metal Shading Language string'i vardı (dört giriş noktası: vs_main,
// fs_main, fs_light, fs_postfx). O string KALDIRILDI — silinmedi, TAŞINDI:
//
//   shaders/sprite.vert.hlsl   ← vs_main
//   shaders/sprite.frag.hlsl   ← fs_main
//   shaders/light.frag.hlsl    ← fs_light
//   shaders/postfx.frag.hlsl   ← fs_postfx
//
// Yorumlar, matematik ve tüm modlar birebir taşındı; MSL sürümünün son hâli
// git geçmişinde e563b23:src/r2d.c içinde duruyor (karşılaştırma gerekirse).
//
// NEDEN KALDIRILDI: gömülü MSL, oyunun sağlayabildiği tek shader formatının MSL
// olması demekti — yani Windows'ta ne D3D12 ne Vulkan bu shader'ı tüketebiliyordu
// ve oyun HİÇ çizmiyordu. Tek kaynak dilden üç formata (DXIL/SPIR-V/MSL) çıkmanın
// yolu HLSL; SDL_GPU'nun bağlama sözleşmesi de HLSL için dokümante edilmiş
// (vendor/SDL/include/SDL3/SDL_gpu.h, SDL_CreateGPUShader doküman bloğu).
//
// MSL artık ELLE YAZILAN değil ÜRETİLEN bir çıktı: shaders/compile.sh HLSL'i
// SPIR-V'ye (glslang) ve oradan MSL'e (spirv-cross) çeviriyor, sonuç
// assets/shaders/{spirv,msl,dxil}/ altında repoda duruyor ve oyunla paketleniyor.
// Derleme OFFLINE — çalışma anında hiçbir shader derleyicisine bağımlılık yok.
// Shader'a dokunulduğunda shaders/compile.sh yeniden çalıştırılmalı (hot-reload yok).
// ---------------------------------------------------------------------------

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
// BÜKÜLME post-fx GEÇİŞİNDE uygulanıyor (sahne dokusu ve tam ekran quad orada).
// Ama bükülme bir MOTOR katmanı, görsel tarz ayarı değil — F7 ile tarz
// "kapalı"ya gelince ısı titremesi ve şok dalgaları da kayboluyordu. Gizli
// bağımlılık: bu bayrak onu kırıyor, bükülme varsa geçiş tarzdan bağımsız çalışır.
static bool distortOn = false;
// shaders/postfx.frag.hlsl PostU ile BİREBİR. src/meta: bükülme alanları
// (genel motor katmanı — ısı, şok, burgaç, mercek aynı sistemden).
typedef struct {
    float a[4]; float b[4]; float c[4]; float d[4]; float e[4];
    float f[4];                          // x=aktif bükülme sayısı, y,z,w=(boş)
    float src[R2D_MAX_DISTORT][4];       // x,y=ekran px · z=yarıçap · w=genlik
    float meta[R2D_MAX_DISTORT][4];      // x=stil · y=faz · z=frekans · w=aniso
} R2DPostFXU;
static R2DPostFXU postfxU;
// Sahne-only ayrımı bu karede yapıldı mı? Yapıldıysa end_frame kalan çizimleri
// (HUD) doğrudan swapchain'e, TEMİZLEMEDEN basar — postfx_pass her pikseli
// çoktan yazdı, temizlemek sahneyi silerdi.
static bool postfxSplit = false;
// Ayrımın yapıldığı andaki vertex sayısı. flush_to sayacı SIFIRLAMAZ (sıfırlama
// r2d_begin_frame'de), aralığı firstVert ile seçer — r2d_target_begin'in deseni.
// Bu olmadan son flush firstVert=0 ile DÜNYAYI FİLTRESİZ YENİDEN ÇİZİYOR ve
// filtreli görüntünün üstünü kapatıyordu (bütün filtreler çalışmaz görünüyordu).
static int postfxSplitVert = 0;
static Uint32 frameNo = 0;   // gren animasyonu için kare sayacı (postfx zamanı)

// Su hacmi: zemin katmanı floorTex'e alınır, fs_water onu okuyup waterTex'e
// yazar, sonuç batch'in başına tek quad olarak döner. İki target şart —
// aynı dokudan okuyup ona yazmak tanımsız davranış.
static SDL_GPUGraphicsPipeline *waterPipeline = NULL;
static R2DTexture *floorTex = NULL;   // zemin katmanı (okunan)
static R2DTexture *waterTex = NULL;   // bileşik sonuç (yazılan)
static bool waterDone = false;        // kare başına en fazla bir su geçişi
typedef struct {                      // shaders/water.frag.hlsl WaterU ile BİREBİR
    float a[4]; float b[4]; float c[4]; float d[4];
    float e[4]; float f[4]; float g[4]; float h[4];
    float k[4];                       // shader'da da 'k': i/j bilinçli atlandı
    float l[4];                       // x=gölge y=ışıltı z=ark w=duvar
    float m[4];                       // lamba1 xy + lamba2 xy (dünya)
    float n[4];                       // lamba şiddetleri + karanlık
} R2DWaterU;

// Işık uniform'ları — shaders/light.frag.hlsl LightU ile birebir aynı düzen
// (hepsi float4, 16 bayt stride; üretilen SPIR-V üye ofsetleriyle doğrulandı).
// Tek statik kopya: kare başına bir ışık pass'i (r2d_light_pass yorumu).
typedef struct {
    float ambient[4];
    float count[4];
    float mode[4];      // light.frag.hlsl LightU.mode ile AYNI SIRADA olmak zorunda
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
// Shader formatı ARTIK SABİT DEĞİL. Üç format da derleme zamanında üretilip
// assets/shaders/ altında paketleniyor; hangisinin yükleneceğine cihaz karar
// veriyor (SDL_GetGPUShaderFormats).
//
// Dosya adı kalıbı: assets/shaders/<dizin>/<ad>.<uzantı>   (ad örn. "light.frag")
// Giriş noktası formata göre DEĞİŞİR: SPIR-V ve DXIL'de "main", MSL'de "main0"
// — Metal `main` adına izin vermiyor, spirv-cross onu main0'a çeviriyor.
static const char *shaderDir   = NULL;
static const char *shaderExt   = NULL;
static const char *shaderEntry = NULL;
static SDL_GPUShaderFormat shaderFmt = 0;

// Repoda hangi format setleri GERÇEKTEN var? Cihaza yalnız sağlayabildiğimiz
// formatları bildirmek şart: SDL'in arka uç deneme sırası Metal → D3D12 → Vulkan
// ve ilk hazırlanabilen kazanıyor. DXIL bayrağını verip DXIL dosyası koymazsak
// Windows'ta D3D12 seçilir ve shader yüklenemez; bayrağı vermezsek SDL D3D12'yi
// hiç denemez ve Vulkan/SPIR-V ile çizer. Yani bu ölçüm, DXIL üretilmemiş bir
// dağıtımda Windows'un yine de çizmesini sağlıyor.
static SDL_GPUShaderFormat shader_formats_available(void)
{
    static const struct { SDL_GPUShaderFormat fmt; const char *probe; } SETS[] = {
        { SDL_GPU_SHADERFORMAT_SPIRV, "assets/shaders/spirv/sprite.vert.spv"  },
        { SDL_GPU_SHADERFORMAT_MSL,   "assets/shaders/msl/sprite.vert.msl"    },
        { SDL_GPU_SHADERFORMAT_DXIL,  "assets/shaders/dxil/sprite.vert.dxil"  },
    };
    SDL_GPUShaderFormat mask = 0;
    for (int i = 0; i < (int)SDL_arraysize(SETS); i++) {
        SDL_PathInfo info;
        if (SDL_GetPathInfo(SETS[i].probe, &info) && info.type == SDL_PATHTYPE_FILE)
            mask |= SETS[i].fmt;
    }
    return mask;
}

// Cihazın tüketebildiği formatlarla bizim paketlediklerimizin kesişiminden birini seç
static bool shader_format_pick(void)
{
    SDL_GPUShaderFormat have = SDL_GetGPUShaderFormats(gpu) & shader_formats_available();
    if (have & SDL_GPU_SHADERFORMAT_SPIRV) {
        shaderDir = "spirv"; shaderExt = "spv";  shaderEntry = "main";
        shaderFmt = SDL_GPU_SHADERFORMAT_SPIRV;
    } else if (have & SDL_GPU_SHADERFORMAT_MSL) {
        shaderDir = "msl";   shaderExt = "msl";  shaderEntry = "main0";
        shaderFmt = SDL_GPU_SHADERFORMAT_MSL;
    } else if (have & SDL_GPU_SHADERFORMAT_DXIL) {
        shaderDir = "dxil";  shaderExt = "dxil"; shaderEntry = "main";
        shaderFmt = SDL_GPU_SHADERFORMAT_DXIL;
    } else {
        return false;
    }
    return true;
}

// name örn. "light.frag" — uzantısız, formattan bağımsız ad
static SDL_GPUShader *load_shader(SDL_GPUShaderStage stage, const char *name,
                                  int numUniformBufs, int numSamplers)
{
    char path[256];
    SDL_snprintf(path, sizeof(path), "assets/shaders/%s/%s.%s",
                 shaderDir, name, shaderExt);
    size_t size = 0;
    void *code = SDL_LoadFile(path, &size);
    if (code == NULL) {
        SDL_Log("r2d: shader okunamadi (%s): %s", path, SDL_GetError());
        return NULL;
    }
    SDL_GPUShaderCreateInfo ci = {
        .code = (const Uint8 *)code,
        .code_size = size,
        .entrypoint = shaderEntry,
        .format = shaderFmt,
        .stage = stage,
        .num_uniform_buffers = (Uint32)numUniformBufs,
        .num_samplers = (Uint32)numSamplers,
    };
    SDL_GPUShader *sh = SDL_CreateGPUShader(gpu, &ci);
    if (sh == NULL) SDL_Log("r2d: shader kurulamadi (%s): %s", path, SDL_GetError());
    SDL_free(code);
    return sh;
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

    // Sağlayabildiğimiz TÜM formatları bildir, arka ucu SDL seçsin (Metal →
    // D3D12 → Vulkan sırasıyla dener). Hangi formatların paketlendiği diske
    // bakılarak ölçülüyor — bkz. shader_formats_available().
    SDL_GPUShaderFormat fmts = shader_formats_available();
    if (fmts == 0) {
        SDL_Log("r2d: derlenmis shader bulunamadi — 'sh shaders/compile.sh' "
                "calistirin ve oyunu proje kokunden baslatin");
        return NULL;
    }
    gpu = SDL_CreateGPUDevice(fmts, false, NULL);
    if (gpu == NULL) {
        SDL_Log("r2d: GPU cihazi acilamadi: %s", SDL_GetError());
        return NULL;
    }
    if (!shader_format_pick()) {
        SDL_Log("r2d: cihazin (%s) tuketebildigi bir shader formati paketlenmemis",
                SDL_GetGPUDeviceDriver(gpu));
        return NULL;
    }
    if (!SDL_ClaimWindowForGPUDevice(gpu, win)) {
        SDL_Log("r2d: pencere GPU'ya baglanamadi: %s", SDL_GetError());
        return NULL;
    }
    SDL_SetGPUSwapchainParameters(gpu, win, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                  SDL_GPU_PRESENTMODE_VSYNC);

    SDL_GPUShader *vs = load_shader(SDL_GPU_SHADERSTAGE_VERTEX, "sprite.vert", 1, 0);
    SDL_GPUShader *fs = load_shader(SDL_GPU_SHADERSTAGE_FRAGMENT, "sprite.frag", 0, 1);
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
    SDL_GPUShader *fsLight = load_shader(SDL_GPU_SHADERSTAGE_FRAGMENT, "light.frag", 1, 0);
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
    SDL_GPUShader *fsPost = load_shader(SDL_GPU_SHADERSTAGE_FRAGMENT, "postfx.frag", 1, 1);
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

    // Su hacmi: İKİ doku (zemin + derinlik maskesi) + 1 uniform buffer.
    // Hedef formatı RGBA8 — su ekrana değil waterTex'e yazıyor.
    // DİKKAT sıra: (uniform tampon, sampler). İlk yazımda (2,1) yazıldı, yani
    // "2 uniform tampon + 1 sampler" — maske dokusu HİÇ BİLDİRİLMEDİ, sample
    // sessizce 0 döndü ve su hiç görünmedi. Derleyici de sürücü de uyarmıyor.
    SDL_GPUShader *fsWater = load_shader(SDL_GPU_SHADERSTAGE_FRAGMENT, "water.frag", 1, 2);
    if (fsWater == NULL) {
        SDL_Log("r2d: su shader'i derlenemedi: %s", SDL_GetError());
        return NULL;
    }
    waterPipeline = make_pipeline(vs, fsWater, R2D_BLEND_ALPHA,
                                  SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM);
    if (waterPipeline == NULL) {
        SDL_Log("r2d: su pipeline kurulamadi: %s", SDL_GetError());
        return NULL;
    }
    SDL_ReleaseGPUShader(gpu, fsWater);

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

    // Sürücü + shader formatı BİRLİKTE loglanıyor: taşımadan sonra bu satır
    // ilk teşhis aracı ("metal/MSL" mi "direct3d12/DXIL" mi "vulkan/SPIR-V" mi).
    SDL_Log("r2d: hazir (surucu: %s, shader: %s)",
            SDL_GetGPUDeviceDriver(gpu), shaderDir);
    return &CTX;
}

void r2d_quit(R2D *ctx)
{
    (void)ctx;
    if (gpu == NULL) return;
    SDL_WaitForGPUIdle(gpu);
    if (whiteTex) { SDL_ReleaseGPUTexture(gpu, whiteTex->tex); SDL_free(whiteTex); }
    if (sceneTex) { SDL_ReleaseGPUTexture(gpu, sceneTex->tex); SDL_free(sceneTex); }
    if (floorTex) { SDL_ReleaseGPUTexture(gpu, floorTex->tex); SDL_free(floorTex); }
    if (waterTex) { SDL_ReleaseGPUTexture(gpu, waterTex->tex); SDL_free(waterTex); }
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
    if (waterPipeline) SDL_ReleaseGPUGraphicsPipeline(gpu, waterPipeline);
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

// SU YANSIMASI: dikey aynalı + soluk çizim. Ayrı fonksiyon, çünkü
// r2d_draw_texture_flip imzasına flipV eklemek onlarca çağrı yerini bozar
// ve tek müşteri bu. Yansıma su geçişinden ÖNCE zemine çizilir: kırılma ve
// derinlik süzgeci üstüne kendiliğinden biner (ekstra shader yok).
void r2d_draw_texture_reflect(R2D *ctx, R2DTexture *tex, const SDL_FRect *src,
                              const SDL_FRect *dst, Uint8 alpha)
{
    (void)ctx;
    if (tex == NULL || dst == NULL) return;
    SDL_FRect s = (src != NULL) ? *src : (SDL_FRect){ 0, 0, tex->w, tex->h };
    SDL_FRect d = *dst;
    if (s.x < 0) { d.x -= s.x * (d.w / s.w); d.w += s.x * (d.w / s.w); s.w += s.x; s.x = 0; }
    if (s.y < 0) { d.y -= s.y * (d.h / s.h); d.h += s.y * (d.h / s.h); s.h += s.y; s.y = 0; }
    if (s.x + s.w > tex->w) { float over = s.x + s.w - tex->w; d.w -= over * (d.w / s.w); s.w -= over; }
    if (s.y + s.h > tex->h) { float over = s.y + s.h - tex->h; d.h -= over * (d.h / s.h); s.h -= over; }
    if (s.w <= 0.0f || s.h <= 0.0f || d.w <= 0.0f || d.h <= 0.0f) return;

    float u0 = s.x / tex->w;
    float v0 = (s.y + s.h) / tex->h;   // V TAKAS: dikey ayna
    float u1 = (s.x + s.w) / tex->w;
    float v1 = s.y / tex->h;
    push_quad(tex, d.x, d.y, d.w, d.h, u0, v0, u1, v1,
              (SDL_Color){ 255, 255, 255, alpha });
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

    // float4 — sprite.vert.hlsl'deki cbuffer ile birebir. zw kullanılmıyor,
    // dolgu: 8 baytlık bir uniform buffer Vulkan'da bağlama aralığını blok
    // boyutunun altına düşürebiliyor (bkz. shaders/sprite.vert.hlsl yorumu).
    float viewport[4] = { outW, outH, 0.0f, 0.0f };
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
    waterDone = false;

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

// Sahne target'ını pencere boyutunda hazırla (resize'da yeniden kurulur)
static void scene_target_ensure(void)
{
    if (sceneTex == NULL ||
        (Uint32)sceneTex->w != swapW || (Uint32)sceneTex->h != swapH) {
        if (sceneTex) r2d_texture_destroy(NULL, sceneTex);   // ertelenir
        sceneTex = texture_create((int)swapW, (int)swapH, true);
    }
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
    float viewport[4] = { 1.0f, 1.0f, 0.0f, 0.0f };   // quad pos 0..1 — NDC dönüşümü birebir
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

void r2d_postfx_flush(R2D *ctx)
{
    (void)ctx;
    // curTarget: bir render target pass'i açıkken ayrım yapılamaz (o pass kendi
    // vertex aralığını tutuyor, araya girmek ikisini de bozar).
    if (cmdBuf == NULL || (!postfxOn && !distortOn) ||
        postfxSplit || curTarget != NULL) return;
    scene_target_ensure();
    if (sceneTex == NULL) return;
    // Sahneyi offscreen'e yaz (temizleyerek), filtreden geçirip ekrana bas.
    flush_to(cmdBuf, sceneTex->tex, (float)swapW, (float)swapH, true,
             clearColor, targetPipeline, 0);
    postfx_pass();
    postfxSplit = true;        // bundan sonrası doğrudan ekrana, filtresiz
    postfxSplitVert = vertCount;   // HUD bu noktadan sonra başlar
}

// ---------------------------------------------------------------------------
// Su hacmi
// ---------------------------------------------------------------------------
static void water_targets_ensure(void)
{
    if (floorTex == NULL ||
        (Uint32)floorTex->w != swapW || (Uint32)floorTex->h != swapH) {
        if (floorTex) r2d_texture_destroy(NULL, floorTex);   // ertelenir
        if (waterTex) r2d_texture_destroy(NULL, waterTex);
        floorTex = texture_create((int)swapW, (int)swapH, true);
        waterTex = texture_create((int)swapW, (int)swapH, true);
    }
}

// fs_water geçişi: floorTex + maske → waterTex, tam ekran quad
static void water_pass(const R2DWaterU *wu, R2DTexture *mask)
{
    SDL_GPUColorTargetInfo target = {
        .texture = waterTex->tex,
        .load_op = SDL_GPU_LOADOP_DONT_CARE,   // quad her pikseli yazar
        .store_op = SDL_GPU_STOREOP_STORE,
    };
    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(cmdBuf, &target, 1, NULL);
    SDL_BindGPUGraphicsPipeline(pass, waterPipeline);
    float viewport[4] = { 1.0f, 1.0f, 0.0f, 0.0f };   // quad pos 0..1 — NDC birebir
    SDL_PushGPUVertexUniformData(cmdBuf, 0, viewport, sizeof(viewport));
    SDL_PushGPUFragmentUniformData(cmdBuf, 0, wu, sizeof(*wu));
    SDL_GPUBufferBinding vb = { .buffer = fsQuadBuf, .offset = 0 };
    SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
    // Zemin NEAREST (pixel-art keskinliği), maske LINEAR (yumuşak derinlik
    // rampası ve köşesiz kıyı silueti TAM OLARAK buradan geliyor).
    SDL_GPUTextureSamplerBinding tsb[2] = {
        { .texture = floorTex->tex, .sampler = sampler },
        { .texture = mask->tex,     .sampler = samplerLinear },
    };
    SDL_BindGPUFragmentSamplers(pass, 0, tsb, 2);
    SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
}

void r2d_water_flush(R2D *ctx, const R2DWater *w)
{
    // Kare başına EN FAZLA BİR kez: ikinci çağrı o arada çizilen NPC/oyuncuyu
    // da zemin sanıp suyun altına alırdı.
    if (cmdBuf == NULL || waterDone || postfxSplit || curTarget != NULL) return;
    if (w == NULL || w->mask == NULL || w->maskW <= 0.0f || w->maskH <= 0.0f) return;
    water_targets_ensure();
    if (floorTex == NULL || waterTex == NULL) return;

    R2DWaterU wu;
    wu.a[0] = w->time;   wu.a[1] = w->zoom;  wu.a[2] = (float)swapW; wu.a[3] = (float)swapH;
    wu.b[0] = w->camX;   wu.b[1] = w->camY;  wu.b[2] = w->maskW;     wu.b[3] = w->maskH;
    wu.c[0] = w->sunX;   wu.c[1] = w->sunY;  wu.c[2] = w->sunI;      wu.c[3] = w->artPx;
    wu.d[0] = w->shallowR; wu.d[1] = w->shallowG; wu.d[2] = w->shallowB; wu.d[3] = w->shallowCover;
    wu.e[0] = w->deepR;    wu.e[1] = w->deepG;    wu.e[2] = w->deepB;    wu.e[3] = w->deepCover;
    wu.f[0] = w->waveHeight; wu.f[1] = w->refract; wu.f[2] = w->caustic; wu.f[3] = w->foam;
    wu.g[0] = w->glint;  wu.g[1] = w->quant; wu.g[2] = w->foamSpeed; wu.g[3] = w->foamAmp;
    wu.h[0] = w->sunR;   wu.h[1] = w->sunG;  wu.h[2] = w->sunB;      wu.h[3] = w->tilePx;
    // layers = 0 → HEPSİ AÇIK. Designated-initializer ile çağıran alanı hiç
    // yazmazsa 0 gelir; "0 = hepsi kapalı" olsaydı su sessizce kaybolurdu.
    float lay = (w->layers > 0.5f) ? w->layers : (float)R2D_WATER_L_ALL;
    wu.k[0] = w->wetBand; wu.k[1] = w->darkLine; wu.k[2] = w->meniscus; wu.k[3] = lay;
    wu.l[0] = w->bankShadow; wu.l[1] = w->dashes; wu.l[2] = w->arcs; wu.l[3] = w->wallFace;
    wu.m[0] = w->lamp1X; wu.m[1] = w->lamp1Y; wu.m[2] = w->lamp2X; wu.m[3] = w->lamp2Y;
    wu.n[0] = w->lamp1I; wu.n[1] = w->lamp2I; wu.n[2] = w->darkness; wu.n[3] = w->rain;

    // 1) O ana kadar çizilmiş zemin katmanı floorTex'e (temizleyerek)
    flush_to(cmdBuf, floorTex->tex, (float)swapW, (float)swapH, true,
             clearColor, targetPipeline, 0);
    // 2) Su geçişi
    water_pass(&wu, w->mask);
    // 3) Batch'i SIFIRLA ve bileşik sonucu tek quad olarak geri koy. Her
    //    flush_to kendi vertex aralığını ayrıca yüklüyor (bkz. flush_to), yani
    //    kare ortasında sıfırlamak güvenli — sonrası için kare sanki tek büyük
    //    arka plan sprite'ıyla başlamış gibi görünüyor ve ışık/sprite/post-fx/
    //    HUD yolları hiç değişmeden çalışıyor.
    vertCount = 0;
    drawCount = 0;
    R2DBlend savedBlend = curBlend;
    curBlend = R2D_BLEND_ALPHA;
    SDL_FRect full = { 0.0f, 0.0f, (float)swapW, (float)swapH };
    r2d_draw_texture(ctx, waterTex, NULL, &full);
    curBlend = savedBlend;
    waterDone = true;
}

void r2d_end_frame(R2D *ctx)
{
    (void)ctx;
    if (cmdBuf == NULL) return;

    // Sahne-only ayrımı yapıldıysa: kalan çizimler HUD'dur, doğrudan ekrana ve
    // TEMİZLEMEDEN gider (postfx_pass her pikseli yazdı).
    if (postfxSplit) {
        flush_to(cmdBuf, swapTex, (float)swapW, (float)swapH, false, clearColor,
                 pipeline, postfxSplitVert);   // YALNIZ HUD aralığı
        SDL_SubmitGPUCommandBuffer(cmdBuf);
        cmdBuf = NULL;
        inFrame = false;
        postfxSplit = false;
        postfxSplitVert = 0;
        for (int i = 0; i < deadCount; i++) {
            SDL_ReleaseGPUTexture(gpu, deadList[i]->tex);
            SDL_free(deadList[i]);
        }
        deadCount = 0;
        return;
    }

    // Post-fx açıkken sahne offscreen target'a akar, oradan filtreyle ekrana.
    // Sahne target'ı pencere boyutunu izler (resize'da yeniden kurulur).
    if (postfxOn || distortOn) scene_target_ensure();
    if ((postfxOn || distortOn) && sceneTex != NULL) {
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
void r2d_distort_set(const R2DDistort *arr, int count)
{
    if (count < 0) count = 0;
    if (count > R2D_MAX_DISTORT) count = R2D_MAX_DISTORT;
    postfxU.f[0] = (float)count;
    distortOn = (count > 0);
    for (int i = 0; i < count; i++) {
        postfxU.src[i][0] = arr[i].x;
        postfxU.src[i][1] = arr[i].y;
        postfxU.src[i][2] = (arr[i].radius > 1.0f) ? arr[i].radius : 1.0f;
        postfxU.src[i][3] = arr[i].strength;
        postfxU.meta[i][0] = arr[i].style;
        postfxU.meta[i][1] = arr[i].phase;
        postfxU.meta[i][2] = (arr[i].freq > 0.01f) ? arr[i].freq : 1.0f;
        postfxU.meta[i][3] = (arr[i].aniso > 0.01f) ? arr[i].aniso : 1.0f;
    }
}

void r2d_postfx_set(R2D *ctx, const R2DPostFX *fx)
{
    (void)ctx;
    if (fx == NULL) {
        postfxOn = false;   // sceneTex dursun — tekrar açılışta hazır
        // NÖTR GRADE: bükülme yüzünden geçiş yine çalışabilir, o durumda
        // filtrenin görüntüyü DEĞİŞTİRMEMESİ lazım (doygunluk/kontrast 1,
        // gerisi 0). Bu satırlar olmadan tarz kapalıyken sahne kararıyordu.
        SDL_memset(&postfxU, 0, sizeof(postfxU));
        postfxU.a[0] = 1.0f;   // doygunluk
        postfxU.a[2] = 1.0f;   // kontrast
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
    postfxU.d[0] = fx->bloom;
    postfxU.d[1] = fx->bloomThreshold;
    postfxU.d[2] = fx->bloomRadius;
    postfxU.d[3] = fx->heat;
    postfxU.e[0] = fx->heatSpeed;
    postfxU.e[1] = postfxU.e[2] = postfxU.e[3] = 0.0f;
    postfxOn = true;
}
