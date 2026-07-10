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
    "    float4 ambient;\n"          // rgb = ortam çarpanı
    "    float4 count;\n"            // x = aktif ışık sayısı
    "    float4 pos[32];\n"          // xy = ekran px, z = yarıçap px
    "    float4 col[32];\n"          // rgb = renk, w = yoğunluk
    "};\n"
    "fragment float4 fs_light(VSOut in [[stage_in]],\n"
    "                         constant LightU &lu [[buffer(0)]]) {\n"
    "    float3 c = lu.ambient.rgb;\n"
    "    int n = int(lu.count.x);\n"
    "    for (int i = 0; i < n; i++) {\n"
    "        float d = distance(in.pos.xy, lu.pos[i].xy);\n"
    "        float att = clamp(1.0 - d / lu.pos[i].z, 0.0, 1.0);\n"
    "        att *= att;\n"                                          // yumuşak düşüş
    "        c += lu.col[i].rgb * (att * lu.col[i].w);\n"
    "    }\n"
    // Dither: 8-bit hedefte yavaş gradyan banding yapar; ±0.75/255 gürültü kırar
    "    float dn = fract(sin(dot(in.pos.xy, float2(12.9898, 78.233))) * 43758.5453);\n"
    "    c += (dn - 0.5) * (1.5 / 255.0);\n"
    "    return float4(clamp(c, float3(0.0), float3(1.0)), 1.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// Durum
// ---------------------------------------------------------------------------
static SDL_GPUDevice *gpu = NULL;
static SDL_Window *win = NULL;
static SDL_GPUGraphicsPipeline *pipeline[R2D_BLEND_COUNT];        // swapchain formatı
static SDL_GPUGraphicsPipeline *targetPipeline[R2D_BLEND_COUNT];  // RGBA8 target formatı
static SDL_GPUGraphicsPipeline *lightPipeline = NULL;             // ışık pass'i (yalnız swapchain)

// Işık uniform'ları — MSL'deki LightU ile birebir aynı düzen (float4 hizalı).
// Tek statik kopya: kare başına bir ışık pass'i (r2d_light_pass yorumu).
typedef struct {
    float ambient[4];
    float count[4];
    float pos[R2D_MAX_LIGHTS][4];
    float col[R2D_MAX_LIGHTS][4];
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
    if (lightPipeline == NULL) {
        SDL_Log("r2d: isik pipeline kurulamadi: %s", SDL_GetError());
        return NULL;
    }
    SDL_ReleaseGPUShader(gpu, fsLight);

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

    if (sampler == NULL || samplerLinear == NULL ||
        vertexBuf == NULL || transferBuf == NULL) {
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
    if (transferBuf) SDL_ReleaseGPUTransferBuffer(gpu, transferBuf);
    if (vertexBuf) SDL_ReleaseGPUBuffer(gpu, vertexBuf);
    if (sampler) SDL_ReleaseGPUSampler(gpu, sampler);
    if (samplerLinear) SDL_ReleaseGPUSampler(gpu, samplerLinear);
    for (int b = 0; b < R2D_BLEND_COUNT; b++) {
        if (pipeline[b]) SDL_ReleaseGPUGraphicsPipeline(gpu, pipeline[b]);
        if (targetPipeline[b]) SDL_ReleaseGPUGraphicsPipeline(gpu, targetPipeline[b]);
    }
    if (lightPipeline) SDL_ReleaseGPUGraphicsPipeline(gpu, lightPipeline);
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
    return t;
}

R2DTexture *r2d_texture_from_pixels(R2D *ctx, const void *rgba, int w, int h)
{
    (void)ctx;
    R2DTexture *t = texture_create(w, h, false);
    if (t == NULL) return NULL;

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
    for (int i = 0; i < count; i++) {
        lightU.pos[i][0] = lights[i].x;
        lightU.pos[i][1] = lights[i].y;
        lightU.pos[i][2] = (lights[i].radius > 1.0f) ? lights[i].radius : 1.0f;
        lightU.pos[i][3] = 0.0f;
        lightU.col[i][0] = lights[i].r;
        lightU.col[i][1] = lights[i].g;
        lightU.col[i][2] = lights[i].b;
        lightU.col[i][3] = lights[i].intensity;
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
        if (draws[i].blend == R2D_BLEND_COUNT) {
            if (pipes != pipeline) continue;   // target içinde desteklenmez
            SDL_BindGPUGraphicsPipeline(pass, lightPipeline);
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

void r2d_end_frame(R2D *ctx)
{
    (void)ctx;
    if (cmdBuf == NULL) return;

    flush_to(cmdBuf, swapTex, (float)swapW, (float)swapH, true, clearColor,
             pipeline, 0);
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
