#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h> // platformlar arası 'main' köprülemesi (mobil/web için de gerekli)
#include <stdbool.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Bir PNG'yi yükleyip GPU texture'ına düşüştürür; hata olursa NULL döner.
// DİKKAT (sahiplik): bu fonksiyon yeni bir texture "yaratır"; onu yok etmek
// ARTIK ÇAĞIRANIN SORUMLULUĞUNDADIR.

static SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path)
{
    int w, h, channels;
    unsigned char *pixels = stbi_load(path, &w, &h, &channels, 4); // 4 = RGBA'ya zorla
    if(pixels == NULL){
        SDL_Log("Yüklenemedi: %s", stbi_failure_reason());
        return NULL;
    }

    SDL_Surface *surface = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, pixels, w * 4);
    if(surface == NULL){
        SDL_Log("Surface olusturulamadi: %s %s", path, SDL_GetError());
        stbi_image_free(pixels);
        return NULL;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    if(texture == NULL){
        SDL_Log("Texture olusturulamadi: %s %s", path, SDL_GetError());
        SDL_DestroySurface(surface);
        stbi_image_free(pixels);
        return NULL;
    }

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST); // pikselleri keskin tut, bulanıklaştırma yapma
    return texture;

}

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

    // base → feet (ayakkabı) → legs (pantolon) → chest (gömlek) → hands (eller) → hair (saç)
    // Katmanlar ÇİZİM SIRASINDA (alttan üste). Yeni parça eklemek = buraya bir satır.
    const char *layer_paths[] = {
        "assets/player/Player_Base/Player_Base_animations.png",
        "assets/player/Feet/Shoes_1_Brown.png",
        "assets/player/Legs/OG_Pants/Pants_1_Blue.png",
        "assets/player/Chest/OG_Shirt/Shirt_1_Green.png",
        "assets/player/Hands/Hands_1_Bare.png",
        "assets/player/Head/Hair_1/Hair_1_Brown.png",
    };

    #define LAYER_COUNT ((int)(sizeof(layer_paths) / sizeof(layer_paths[0])))

     SDL_Texture *layers[LAYER_COUNT] = {0};   // hepsi NULL ile başla (temizlik güvenliği)
    for (int i = 0; i < LAYER_COUNT; i++) {
        layers[i] = load_texture(renderer, layer_paths[i]);
        if (layers[i] == NULL) {
            SDL_Log("Katman yuklenemedi: %s", layer_paths[i]);
            for (int j = 0; j < LAYER_COUNT; j++)
                if (layers[j]) SDL_DestroyTexture(layers[j]);  // sadece yüklenmiş olanları yık
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }
    }
    
    SDL_FRect box = {.x = 300.0f, .y = 300.0f, .w = 256.0f, .h = 256.0f};
    Uint64 last = SDL_GetTicks();
    const float speed = 200.0f;

    // --- ANİMASYON DURUMU ---
    const int FRAME_W = 64, FRAME_H = 64; // sprite kare boyutu
    const float FRAME_DURATION = 0.1f;     // her kare 0.1 sn sürer

    int currentFrame = 0; // şu an hangi karedeyiz (0'dan başlar)
    float animTimer = 0.0f;     // kare değiştirmek için geçen süreyi sayar

    // Karakterin baktığı yön - kalıcı (dururken de korunur). 0=aşağı, 1=yukarı, 2=yan (sol/sağ flip ile)
    int facing = 0;
    bool facingLeft = false; // true = sola bakıyor, false = sağa bakıyor
    int lastDirectionRow = -1;   // önceki karenin animasyonu; -1 = "henüz yok" (ilk karede sıfırlar, zararsız)

    // 4) Oyun döngüsü
    bool running = true;

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

        bool moving = false;

        // --- NORMALİZE: yön vektörünü 1 birim uzunluğa indir ---
        // Ham vektör tek eksende uzunluk 1, çaprazda √2 → çapraz %41 hızlı olurdu.
        // length'e bölmek yönü korur, uzunluğu 1 yapar; böylece her yönde hız eşit.
        float length = SDL_sqrtf(dx * dx + dy * dy);
        if (length > 0) {                    // sıfır kontrolü ŞART: 0/0 = NaN, kutuyu sessizce yok eder
            dx = dx / length;
            dy = dy / length;
            box.x += dx * speed * dt;        // mesafe = hız × süre (kare hızından bağımsız)
            box.y += dy * speed * dt;
            moving = true;

            if (SDL_fabsf(dx) > SDL_fabsf(dy)) {   // yatay baskın → yan
                facing = 2;
                facingLeft = (dx < 0);
            } else {                                // dikey baskın → yukarı/aşağı
                facing = (dy < 0) ? 1 : 0;          // dy<0 yukarı, değilse aşağı
            }
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
        SDL_RenderClear(renderer);     // Renderer'ı temizle
        
        // (satır 0 = idle-aşağı, 6 kareli — aşağıdaki FRAME_COUNT ile uyumlu olmalı)
        int directionRow;
        int frameCount;
        if (moving) {
            // yürüyüş satırları: aşağı=6, yukarı=12, yan=9; hepsi 4 kare
            if (facing == 1)      directionRow = 5;  // yukarı
            else if (facing == 2) directionRow = 4;   // yan
            else                  directionRow = 3;   // aşağı
            frameCount = 6;
        } else {
            // idle de facing'e göre — karakter baktığı yönde dursun
            if (facing == 1)      directionRow = 2;   // idle yukarı   (oyunda doğrula)
            else if (facing == 2) directionRow = 1;   // idle yan      (oyunda doğrula)
            else                  directionRow = 0;   // idle aşağı
            frameCount = 6;
        }


         // ANİMASYON DEĞİŞTİ Mİ? Değiştiyse yeni animasyonu baştan başlat.
        if (directionRow != lastDirectionRow) {
            currentFrame = 0;       // ilk kareden başla
            animTimer = 0.0f;       // ilk kare tam süre dursun (yarım kalmasın)
            lastDirectionRow = directionRow;
        }

         // --- ANİMASYON: zamanlayıcıyı güncelle ---
        animTimer += dt;
        if (animTimer >= FRAME_DURATION) {
            animTimer -= FRAME_DURATION; // tam süre kadar fazlayı tut, böylece animasyon zamanlaması daha kararlı olur
            currentFrame = (currentFrame + 1) % frameCount; // sonraki kareye geç, son kareden sonra başa dön
        }

        // src'yi BİR KEZ hesapla: x = hangi kare, y = hangi yön satırı.
        // Bu, TÜM katmanlar için aynı — döngünün İÇİNDE değiştirmiyoruz.
        SDL_FRect src = {
            .x = currentFrame * FRAME_W,
            .y = directionRow * FRAME_H,
            .w = FRAME_W,
            .h = FRAME_H
        };

        // Döngü SADECE katmanı değiştirir; src sabit → katmanlar kilitli adımda.
        SDL_FlipMode flip = facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        for (int i = 0; i < LAYER_COUNT; i++) {
            SDL_RenderTextureRotated(renderer, layers[i], &src, &box, 0.0, NULL, flip);

        }

        SDL_RenderPresent(renderer); // Ekrana bas.
    }

    // 5) Her şeyi oluşturmanın TERS sırasıyla temizle. C'de bu bizim sorumluluğumuzda,
    // hiçbir destructor bunu kendiliğinden yapmaz.
    for (int i = 0; i < LAYER_COUNT; i++) {
        SDL_DestroyTexture(layers[i]);
    }
    SDL_DestroyRenderer(renderer); // Renderer'ı temizle
    SDL_DestroyWindow(window);     // Pencereyi temizle
    SDL_Quit();                    // SDL'i temizle
    return 0;                      // Başarılı çık
}