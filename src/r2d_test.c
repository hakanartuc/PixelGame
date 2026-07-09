// r2d duman testi — oyunun dışında, GPU yolunu uçtan uca doğrular:
// device + MSL shader + pipeline + texture upload + batch + present.
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

        r2d_end_frame(ctx);
    }

    if (tex) r2d_texture_destroy(ctx, tex);
    r2d_quit(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    SDL_Log("R2D TEST OK");
    return 0;
}
