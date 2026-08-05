#include "shadow.h"
#include "gametime.h"
#include "r2d.h"
#include <SDL3/SDL.h>

// ═══════════════════════════════════════════════════════════════════════════
// GÖLGE GEOMETRİSİ — Blinn'in düzlemsel gölge projeksiyonu (1988)
//
// Kaynak: James F. Blinn, "Me and My (Fake) Shadow", IEEE CG&A 8(1), Ocak 1988.
// Genel matris: M = (n·L)·I − L ⊗ n   (n = zemin düzlemi, L = homojen ışık)
//
// GÜNEŞ yönlü bir ışıktır (w=0, sonsuz uzaklık), o yüzden n·L = sin α sabit
// çıkar → perspektif bölmesi yok → dönüşüm AFİN, ve saf bir SHEAR'e indirgenir:
//
//     X' = X − Z·cot(α)·sin(θ)
//     Y' = Y − Z·cot(α)·cos(θ)
//
// Yani gölge boyu:  L = h / tan(α)  = h·cot(α)
// Bu, güneş enerjisi/mimarlık literatüründeki standart bağıntının aynısı —
// iki bağımsız kaynak aynı sonuca çıkıyor.
//
// TOP-DOWN'A İNDİRME. Ekran: x sağa, y AŞAĞI. Dikey kısaltma faktörü k,
// yükseklik ölçeği c=1 (pixel-art'ta sanatçı yüksekliği doğrudan ekran px
// olarak çizer). Ayak noktasına göre ekran uzayı ötelemesi:
//
//     dx = −(h·cot α)·sin θ
//     dy = +k·(h·cot α)·cos θ
//
// ── "ZELDA YALANI" (kritik) ──────────────────────────────────────────────
// Top-down'da zemin karosu ekranda kare çizilir (k≈1) ve yükseklik de 1:1
// yukarı çizilir. Yani DÜNYA-KUZEYİ ile DÜNYA-YUKARISI ekranda ayırt edilemez.
// Sonuç: kuzeye düşen gölge sprite'ın TAM ARKASINA denk gelir ve GÖRÜNMEZ olur.
// Top-down oyunların gölgeyi asla ekranda yukarı atmamasının gerçek sebebi bu.
// Çözüm: azimutun dikey bileşenini pozitif tabanda tut (cos θ → max(cos θ, ε)).
//
// KLAMPLAMA. α→0 iken cot α→∞. Standart pratik 2° altında hesap yapmamak;
// biz 14° klamplıyoruz → maksimum boy ≈ 4h, ve altında alpha'yı sıfıra
// fade ediyoruz (RimWorld de gölgeleri günün saatine göre fade ediyor).
// ═══════════════════════════════════════════════════════════════════════════

#define SUN_ALT_MIN_DEG   14.0f   // altında gölge sonsuza gider → klampla + fade
#define SHADOW_DY_FLOOR   0.42f   // cos θ tabanı: gölge asla ekranda yukarı gitmez
// NOT: azimut şafaktan akşama 90°→270° tarandığı için cos(azi) hep ≤ 0,
// yani dikey bileşen klampı GÜN BOYU aktif ve vy sabit SHADOW_DY_FLOOR.
// Bu bir kusur değil, "Zelda yalanı" düzeltmesinin ta kendisi: top-down'da
// kuzeye düşen gölge sprite'ın arkasına saklanır. Dikey kısaltma faktörü (k)
// bu yüzden ölü kaldı ve kaldırıldı — klamp onun işini yapıyor.

bool shadow_sun(float *dx, float *dy, float *lenMul, float *strength)
{
    float t = gametime_sun_t();
    if (t < 0.0f) return false;   // gece — güneş gölgesi yok

    // Güneş yüksekliği α: şafakta 0, öğlen tepede, akşam 0.
    // Mevsim tavanı kimliği bedavaya veriyor — kışın güneş hiç yükselmediği
    // için gölgeler bütün gün uzun kalır.
    Season s = gametime_season();
    float altMaxDeg = (s == SEASON_SUMMER) ? 72.0f
                    : (s == SEASON_WINTER) ? 34.0f : 55.0f;
    float altDeg = altMaxDeg * SDL_sinf(t * SDL_PI_F);

    // Fade: ufka çok yakınken gölge hem sonsuza gider hem gerçekçi olmaz.
    float fade = 1.0f;
    if (altDeg < SUN_ALT_MIN_DEG) {
        fade = altDeg / SUN_ALT_MIN_DEG;      // 0'da tamamen kaybolur
        if (fade < 0.0f) fade = 0.0f;
        altDeg = SUN_ALT_MIN_DEG;             // uzunluğu klampla
    }

    // Azimut θ: kuzeyden saat yönünde. Şafakta doğu (90°), akşam batı (270°).
    float aziDeg = 90.0f + 180.0f * t;
    float azi    = aziDeg * (SDL_PI_F / 180.0f);
    float alt    = altDeg * (SDL_PI_F / 180.0f);

    // ── Blinn shear'i ──
    float cotA = SDL_cosf(alt) / SDL_sinf(alt);      // cot α = 1/tan α
    float vx   = -SDL_sinf(azi);
    float vyRaw =  SDL_cosf(azi);
    // Zelda yalanı düzeltmesi: dikey bileşeni pozitif tabanda tut.
    // (Kuzeye düşen gölge sprite'ın arkasına saklanır ve görünmez olur.)
    float vy   = (vyRaw < SHADOW_DY_FLOOR) ? SHADOW_DY_FLOOR : vyRaw;

    float L = SDL_sqrtf(vx * vx + vy * vy);
    *dx = vx / L;
    *dy = vy / L;

    // lenMul: nesne YÜKSEKLİĞİNİN katı — SAF cot(α). cot(14°)=4.01,
    // cot(55°)=0.70, cot(72°)=0.32 → sabah/akşam uzun, öğlen kısa.
    // DİKKAT: eskiden "cotA * L" yazıyordu. L, dikey klamptan sonraki vektör
    // boyu; onunla çarpmak gölgeyi KISALTIYORDU (öğlen %58 kısa). Klampın işi
    // yönü çevirmek, boyu değiştirmek değil — yön ve boy ayrı hesaplanır.
    *lenMul   = cotA;
    *strength = fade * (0.35f + 0.65f * SDL_sinf(alt));
    return true;
}

// ── GÖLGE LEKESİ DOKUSU ──
// Önce üç kademeli dikdörtgenle oval taklidi yapıyordum; küçük nesnelerde
// idare ediyordu ama EV boyutunda o dikdörtgenler kaba merdiven olarak
// okunuyordu ("kare kare"). Stardew gerçek bir gölge sprite'ı kullanıyor
// (LooseSprites/shadow); biz de aynısını yapıyoruz — ama asset beklemek
// yerine dokuyu kodda üretiyoruz: radyal alpha gradyanı, LINEAR örnekleme.
// Böylece her ölçekte yumuşak kalıyor ve tek çizimde bitiyor.
static R2DTexture *s_blobTex = NULL;

void shadow_init(R2D *renderer)
{
    if (s_blobTex != NULL) return;
    const int W = 64, H = 64;
    static unsigned char px[64 * 64 * 4];
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float nx = ((float)x + 0.5f) / (float)W * 2.0f - 1.0f;
            float ny = ((float)y + 0.5f) / (float)H * 2.0f - 1.0f;
            float d  = SDL_sqrtf(nx * nx + ny * ny);
            float a  = 1.0f - d;
            if (a < 0.0f) a = 0.0f;
            if (a > 1.0f) a = 1.0f;
            a = a * a * (3.0f - 2.0f * a);        // yumuşak kenar
            int i = (y * W + x) * 4;
            px[i + 0] = 255; px[i + 1] = 255; px[i + 2] = 255;
            px[i + 3] = (unsigned char)(a * 255.0f);
        }
    }
    s_blobTex = r2d_texture_from_pixels(renderer, px, W, H);
    if (s_blobTex != NULL) r2d_texture_set_linear(s_blobTex, true);
}

// Basıklık 1.37:1 — Stardew'un ağaç gölgesinden ölçülmüş (41×30 px).
// Klasik "2:1" varsayımı o oyunda DOĞRULANMADI.
static void blob_ellipse(R2D *r, float cx, float cy, float rw, float rh, Uint8 a)
{
    if (s_blobTex == NULL || a == 0) return;
    // BLEND AÇIKÇA (VFX borcu #1): gölge asla eklemeli çizilmemeli — ADD'de
    // gölge KARARTMAK yerine AYDINLATIR. Eskiden mod miras alınıyordu.
    r2d_set_blend(r, R2D_BLEND_ALPHA);
    r2d_texture_set_color_mod(s_blobTex, 16, 18, 28);
    r2d_texture_set_alpha_mod(s_blobTex, a);
    SDL_FRect dst = { cx - rw, cy - rh, rw * 2.0f, rh * 2.0f };
    r2d_draw_texture(r, s_blobTex, NULL, &dst);
}

void shadow_ground_blob(R2D *renderer, const Camera *camera,
                        float footX, float footY, float w, float strength)
{
    float z  = camera->zoom;
    float sx = (footX - camera->x) * z;
    float sy = (footY - camera->y) * z;
    // Genişlik = opak genişliğin %85'i (Stardew ağaç: 41/48). Basıklık 1.37:1.
    float rw = w * 0.425f * z;
    blob_ellipse(renderer, sx, sy, rw, rw / 1.37f, (Uint8)(90.0f * strength));
}

void shadow_blob(R2D *renderer, const Camera *camera,
                 float footX, float footY, float w, float h)
{
    // Temas gölgesi — HER ZAMAN, gece dahil. Akademik karşılığı ambient
    // occlusion yaklaşımı: nesneyi zemine oturtur, fiziksel bir olgu değil
    // okunabilirlik aracıdır. Factorio: "varlıkları zemine oturtmanın en
    // önemli unsuru" ve gece bile görünür kalmalı.
    shadow_ground_blob(renderer, camera, footX, footY, w, 1.0f);

    // Güneş kuyruğu — gündüz, Blinn shear'iyle.
    float dx, dy, lenMul, str;
    if (!shadow_sun(&dx, &dy, &lenMul, &str)) return;

    float z  = camera->zoom;
    float sx = (footX - camera->x) * z;
    float sy = (footY - camera->y) * z;
    float rw = w * 0.40f * z;

    int steps = 5;
    for (int i = 1; i <= steps; i++) {
        float f  = (float)i / (float)steps;
        float ox = dx * lenMul * h * f * z;
        float oy = dy * lenMul * h * f * z;
        // Tam piksele yuvarla: NEAREST örneklemede kesirli kayma her karede
        // farklı merdiven basar ve kenar titrer.
        ox = SDL_roundf(ox);
        oy = SDL_roundf(oy);
        float fade = (1.0f - f * 0.80f) * str;
        blob_ellipse(renderer, sx + ox, sy + oy,
                     rw * (1.0f - 0.45f * f), rw * (1.0f - 0.45f * f) / 1.37f,
                     (Uint8)(70.0f * fade));
    }
}
