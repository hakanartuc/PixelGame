// r2d duman testi — oyunun dışında, GPU yolunu uçtan uca doğrular:
// device + shader yukleme (secilen format) + pipeline + texture upload + batch + present.
// Kullanım: ./build/r2d_test (proje kökünden). 90 kare çizer, 0 ile çıkar.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "r2d.h"

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init basarisiz: %s", SDL_GetError());
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow("r2d test", 640, 360, 0);
    R2D *ctx = (window != NULL) ? r2d_init(window) : NULL;
    if (ctx == NULL) {
        SDL_Log("r2d test KURULUM HATASI");
        return 1;
    }

    R2DTexture *tex = r2d_texture_load(ctx, "assets/ui/UI_Frames.png");
    if (tex == NULL)
        SDL_Log("uyari: test texture yok (proje kokunden calistir) — primitiflerle devam");

    // Post-fx: birden fazla topuz acik — bloom, isi bukulmesi, vinyet, gren.
    // Bu, postfx.frag pipeline'ini VE sahne target'ini (offscreen RGBA8) surer.
    R2DPostFX fx = {
        .saturation = 1.05f, .warmth = 0.03f, .contrast = 1.08f, .brightness = 0.0f,
        .vignette = 0.30f, .vignetteRadius = 0.45f,
        .grain = 0.04f,
        .bloom = 0.35f, .bloomThreshold = 0.70f, .bloomRadius = 0.004f,
        .heat = 0.0025f, .heatSpeed = 1.5f,
    };
    r2d_postfx_set(ctx, &fx);

    for (int frame = 0; frame < 90; frame++) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {}   // kuyruk bosalt (pencere olayları)

        if (!r2d_begin_frame(ctx, 30, 30, 45))
            continue;

        // Primitifler
        r2d_set_color(ctx, 200, 60, 60, 255);
        r2d_fill_rect(ctx, &(SDL_FRect){ 20, 20, 120, 80 });
        r2d_set_color(ctx, 255, 255, 255, 255);
        r2d_rect(ctx, &(SDL_FRect){ 20, 20, 120, 80 });
        r2d_set_color(ctx, 90, 220, 255, 255);
        r2d_line(ctx, 20, 140, 300, 200);

        // Texture + 9-grid
        if (tex != NULL) {
            SDL_FRect src = { 10, 10, 28, 31 };
            SDL_FRect dst = { 200, 30, 56, 62 };
            r2d_draw_texture(ctx, tex, &src, &dst);

            SDL_FRect panelSrc = { 915, 4, 42, 41 };
            SDL_FRect panelDst = { 300, 30, 250, 140 };
            r2d_draw_9grid(ctx, tex, &panelSrc, 14.0f, 1.0f, &panelDst);
        }

        // Additive parlama testi
        r2d_set_blend(ctx, R2D_BLEND_ADD);
        for (int k = 0; k < 8; k++)
        {
            r2d_set_color(ctx, 255, 120, 40, 90);
            r2d_fill_rect(ctx, &(SDL_FRect){ 100.0f + k * 12.0f, 260, 40, 40 });
        }
        r2d_set_blend(ctx, R2D_BLEND_ALPHA);

        // IŞIK PASS'I — light.frag pipeline'i. Modlar kare kare dolasiliyor
        // ki dort dusus modu, dort dither modu, uc gamma modu, kuantizasyon,
        // izgaraya yapisma, A/B bolme ve tikama YOLLARININ HEPSI surulsun.
        R2DLight lights[3] = {
            { .x =  90, .y =  60, .radius = 120, .r = 1.00f, .g = 0.82f, .b = 0.45f,
              .intensity = 0.90f, .wobAmp = 0.18f, .wobC = 0.31f, .wobS = 0.95f,
              .aniso = 0.55f, .yOff = -10 },
            { .x = 420, .y = 240, .radius =  90, .r = 0.40f, .g = 0.60f, .b = 1.00f,
              .intensity = 0.75f, .wobAmp = 0.00f, .wobC = 1.00f, .wobS = 0.00f,
              .aniso = 1.00f, .yOff = 0 },
            { .x = 260, .y = 300, .radius = 160, .r = 1.00f, .g = 0.35f, .b = 0.10f,
              .intensity = 0.55f, .wobAmp = 0.30f, .wobC = -0.80f, .wobS = 0.60f,
              .aniso = 1.30f, .yOff = -18 },
        };
        SDL_FRect occ[2] = { { 150, 120, 60, 40 }, { 330, 180, 45, 30 } };
        r2d_light_set_camera(137.5f, -82.25f);
        r2d_light_set_occluders(occ, 2, 0.75f);
        r2d_light_set_model((float)((frame / 8) % 3), (float)((frame / 4) % 4),
                            (float)(frame % 4), 41.0f);
        r2d_light_set_mode((frame % 30 < 15) ? 0.0f : 6.0f,
                           (frame % 20 < 10) ? 0.0f : 4.0f);
        r2d_light_set_shape(0.85f, -5.0f);
        r2d_light_set_split((frame % 40 < 20) ? 0.0f : 320.0f);
        r2d_light_pass(ctx, 0.28f, 0.30f, 0.42f, lights, 3);

        // SAHNE / HUD SINIRI — buraya kadari filtrelenir, sonrasi filtresiz.
        // Bu ayrim iki ayri flush + postfx pass'i demek, yani en karmasik yol.
        r2d_postfx_flush(ctx);
        r2d_set_color(ctx, 240, 230, 200, 255);
        r2d_fill_rect(ctx, &(SDL_FRect){ 10, 320, 200, 24 });   // sozde HUD

        r2d_end_frame(ctx);
    }

    if (tex) r2d_texture_destroy(ctx, tex);
    r2d_quit(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    SDL_Log("R2D TEST OK");
    return 0;
}
