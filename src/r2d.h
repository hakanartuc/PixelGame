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
// Shader: MSL kaynak (macOS/Metal — SDL derleyiciyi içinde taşır).
// Vulkan/D3D için SPIR-V/DXIL ekleme noktası load_shader() içinde.

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
#define R2D_MAX_LIGHTS 32   // MSL'deki dizi boyutuyla AYNI kalmalı (r2d.c)

typedef struct {
    float x, y;        // ekran px (dünya→ekran dönüşümü çağıranda)
    float radius;      // ekran px — bu uzaklıkta katkı sıfırlanır
    float r, g, b;     // ışık rengi (0..1)
    float intensity;   // katkı gücü (0..1 tipik)
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
} R2DPostFX;

// Değerler kopyalanır (her kare çağırmak gerekmez). NULL = filtreyi kapat.
void r2d_postfx_set(R2D *ctx, const R2DPostFX *fx);

// --- Sprite ---
void r2d_draw_texture(R2D *ctx, R2DTexture *tex,
                      const SDL_FRect *src, const SDL_FRect *dst);
void r2d_draw_texture_flip(R2D *ctx, R2DTexture *tex, const SDL_FRect *src,
                           const SDL_FRect *dst, bool flipH);
void r2d_draw_9grid(R2D *ctx, R2DTexture *tex, const SDL_FRect *src,
                    float corner, float scale, const SDL_FRect *dst);

// --- Primitifler (aktif renkle — SDL_SetRenderDrawColor deseni) ---
void r2d_fill_rect(R2D *ctx, const SDL_FRect *r);
void r2d_rect(R2D *ctx, const SDL_FRect *r);            // 1px çerçeve
void r2d_line(R2D *ctx, float x1, float y1, float x2, float y2);
void r2d_point(R2D *ctx, float x, float y);

#endif
