#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h> // platformlar arası 'main' köprülemesi (mobil/web için de gerekli)
#include <stdbool.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main(int argc, char *argv[])
{
    // 1) SDL'in video alt sistemini ayağa kaldır. SDL3 başarılıysa 'true' döner.
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init basarısız: %s", SDL_GetError());
        return 1; // Başarısızsa çık
    }

    // 2) Pencere oluştur.
    SDL_Window *window = SDL_CreateWindow("Pixe Game", 1280, 720, 0);
    if (window == NULL)
    {
        SDL_Log("SDL_CreateWindow basarısız: %s", SDL_GetError());
        SDL_Quit(); // SDL'i temizle
        return 1;   // Başarısızsa çık
    }

    // 3) Pencereye bağlı bir renderer oluştur.
    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (renderer == NULL)
    {
        SDL_Log("SDL_CreateRenderer basarısız: %s", SDL_GetError());
        SDL_DestroyWindow(window); // Pencereyi temizle
        SDL_Quit();                // SDL'i temizle
        return 1;                  // Başarısızsa çık
    }

    // --- SPRITE YÜKLE: dosya → CPU surface → GPU texture ---
    int imgW, imgH, channels;
    // Son parametre 4 = "kaynak ne olursa olsun bana 4 kanal (RGBA) ver."
    // RGB bir PNG'ye bile alfa ekler; böylece format tahmini derdi biter.
    // 'channels' kaynağın ORİJİNAL kanal sayısını söyler, ama tampon her zaman 4 kanaldır.
    unsigned char *pixels = stbi_load("assets/player.png", &imgW, &imgH, &channels, 4);
    if (pixels == NULL) {
        SDL_Log("Sprite yuklenemedi: %s", stbi_failure_reason());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Ham pikselleri bir SDL_Surface (CPU tarafı görüntü) olarak SARMALA.
    // Dikkat: kopyalamaz, sadece 'pixels'e işaret eder — o yüzden 'pixels' bu
    // surface kullanılana kadar canlı kalmalı.
    // RGBA32: bellekte bayt sırası R,G,B,A — stb'nin verdiği sırayla birebir eşleşir.
    // pitch = bir piksel satırının bayt uzunluğu = genişlik × 4.
    SDL_Surface *surface = SDL_CreateSurfaceFrom(imgW, imgH, SDL_PIXELFORMAT_RGBA32,
                                                 pixels, imgW * 4);
    if (surface == NULL) {
        SDL_Log("Surface olusturulamadi: %s", SDL_GetError());
        stbi_image_free(pixels);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Surface'i (CPU/RAM) bir Texture'a (GPU/VRAM) yükle. Bu adım pikselleri GPU'ya KOPYALAR.
    SDL_Texture *sprite = SDL_CreateTextureFromSurface(renderer, surface);
    if (sprite == NULL) {
        SDL_Log("Texture olusturulamadi: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        stbi_image_free(pixels);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }


    SDL_SetTextureScaleMode(sprite, SDL_SCALEMODE_NEAREST);

    // Geçici CPU verilerini bırak — texture artık kendi GPU kopyasına sahip.
    SDL_DestroySurface(surface);   // SDL3: SDL2'deki SDL_FreeSurface'in yeni adı
    stbi_image_free(pixels);

    // 4) Oyun döngüsü
    bool running = true;

    SDL_FRect box = {.x = 300.0f, .y = 300.0f, .w = 32.0f, .h = 32.0f};
    Uint64 last = SDL_GetTicks();
    const float speed = 200.0f;

    while (running)
    {
        // 4a) INPUT: OS'in bu kare için kuyruğa attığı tüm olayları boşalt.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {                    // Kullanıcı pencereyi kapatmak istedi mi?
                running = false; // Oyun döngüsünden çık
            }
            else if (event.type == SDL_EVENT_KEY_DOWN)
            { // Bir tuşa basıldı mı?
                if (event.key.key == SDLK_ESCAPE)
                {                    // ESC tuşuna basıldı mı?
                    running = false; // Oyun döngüsünden çık
                }
            }
        }

        // 4b) UPDATE: şimdilik boş, ileride oyun durumunu güncellemek için kullanacağız.
        // --- ZAMAN: bu karenin süresini ölç (delta-time) ---
        Uint64 now = SDL_GetTicks();
        float dt = (now - last) / 1000.0f;  // ms'yi saniyeye çevir; 1000.0f (float!) yoksa tamsayı bölme dt'yi 0 yapar
        last = now;                          // pencereyi bu kareye kaydır; unutulursa dt sürekli büyür

        // dt SPIKE koruması: pencere sürükleme/askıya alma sonrası dev bir dt gelirse
        // kutuyu ışınlamasın diye bir tavanda kes. 0.05 sn = 20 FPS'lik bir kareye denk;
        // bundan yavaş giden her kare 0.05 olarak ele alınır.
        if (dt > 0.05f) dt = 0.05f;

        const bool *keys = SDL_GetKeyboardState(NULL);  // basılı tuşların anlık görüntüsü (SDL'e ait, free etme)

        // --- YÖN: basılı tuşlardan bir yön vektörü (dx, dy) kur ---
        // Her eksen bağımsız: zıt tuşlar (A+D) birbirini götürür, çapraz ikisini birden set eder.
        // y AŞAĞI doğru büyür, o yüzden yukarı = NEGATİF y.
        float dx = 0.0f, dy = 0.0f;
        if (keys[SDL_SCANCODE_W]) dy -= 1;  // yukarı
        if (keys[SDL_SCANCODE_S]) dy += 1;  // aşağı
        if (keys[SDL_SCANCODE_A]) dx -= 1;  // sol
        if (keys[SDL_SCANCODE_D]) dx += 1;  // sağ

        // --- NORMALİZE: yön vektörünü 1 birim uzunluğa indir ---
        // Ham vektör tek eksende uzunluk 1, çaprazda √2 → çapraz %41 hızlı olurdu.
        // length'e bölmek yönü korur, uzunluğu 1 yapar; böylece her yönde hız eşit.
        float length = SDL_sqrtf(dx * dx + dy * dy);
        if (length > 0) {                    // sıfır kontrolü ŞART: 0/0 = NaN, kutuyu sessizce yok eder
            dx = dx / length;
            dy = dy / length;
            box.x += dx * speed * dt;        // mesafe = hız × süre (kare hızından bağımsız)
            box.y += dy * speed * dt;
        }
        // tuş yoksa length=0, blok atlanır, kutu yerinde kalır — ekstra 'else' gerekmez

        // --- KISIT: kutuyu pencere içine geri çek (önce hareket, sonra clamp) ---
        // Köşe sol-üstte; sağ/alt sınırdan kutunun boyutunu (w/h) çıkar ki gövdesi taşmasın.
        int winW, winH;
        SDL_GetWindowSize(window, &winW, &winH);
        if (box.x < 0.0f)         box.x = 0.0f;          // sol kenar
        if (box.y < 0.0f)         box.y = 0.0f;          // üst kenar
        if (box.x > winW - box.w) box.x = winW - box.w;  // sağ kenar
        if (box.y > winH - box.h) box.y = winH - box.h;  // alt kenar

        // 4c) RENDER: ekranı temizle -> kutuyu çiz -> bitmiş kareyi göster.
        SDL_SetRenderDrawColor(renderer, 155, 155, 155, 255); // RGBA
        SDL_RenderClear(renderer);                         // Renderer'ı temizle

        SDL_RenderTexture(renderer, sprite, NULL, &box);

        SDL_RenderPresent(renderer); // Ekrana bas.
    }

    // 5) Her şeyi oluşturmanın TERS sırasıyla temizle. C'de bu bizim sorumluluğumuzda,
    // hiçbir destructor bunu kendiliğinden yapmaz.
    SDL_DestroyTexture(sprite);
    SDL_DestroyRenderer(renderer); // Renderer'ı temizle
    SDL_DestroyWindow(window);     // Pencereyi temizle
    SDL_Quit();                    // SDL'i temizle
    return 0;                      // Başarılı çık
}