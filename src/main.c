#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h> // platformlar arası 'main' köprülemesi (mobil/web için de gerekli)
#include <stdbool.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Karakterin TÜM kalıcı durumu tek bir yapıda.
typedef struct{
    SDL_FRect rect;   // ekran pozisyonu ve boyutu
    int facing;       // 0=aşağı, 1=yukarı, 2=yan
    bool facingLeft;  // true = sola bakıyor, false = sağa bakıyor
    int animRow;      // o an çizilecek sheet satırı (update yazar, render okur)
    int currentFrame; // animasyonun hangi karesinde olduğumuz.
    float animTimer;  // kare ilerleme sayacı
    int lastRow;      // önceki karenin animRow'u; animasyon değiştiğinde sıfırlamak için
} Player;

#define PLAYER_SPEED   200.0f
#define FRAME_W        64
#define FRAME_H        64
#define FRAME_DURATION 0.1f

// Animasyon satır tablosu: [hareket durumu][yön] → sheet satırı
// hareket: 0 = idle, 1 = yürüyüş
// yön:     0 = aşağı, 1 = yukarı, 2 = yan
static const int ANIM_ROWS[2][3] = {
    // aşağı  yukarı  yan
    {   0,      2,     1  },   // [0] idle
    {   3,      5,     4  },   // [1] yürüyüş
};

/// -- Harita tanımları ---

#define TILE_SIZE 64          // bir tile ekranda kaç piksel (şimdilik)
#define MAP_W     12          // harita kaç tile geniş
#define MAP_H     8           // harita kaç tile yüksek

// Harita: her hücre bir tile ID (0=çimen, 1=toprak yol, 2=su)
// Dizinin görünümü = haritanın görünümü (toprak çerçeve içinde su gölü)
static const int tilemap[MAP_H][MAP_W] = {
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,1,1,1,1,1,1,0,0,0,0},
    {0,0,1,0,0,0,0,1,0,0,0,0},
    {0,0,1,0,2,2,0,1,0,0,0,0},
    {0,0,1,0,2,2,0,1,0,0,0,0},
    {0,0,1,1,1,1,1,1,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
};

static void tilemap_render(SDL_Renderer *renderer)
{
    // --- HARİTA RENDER: her hücre için uygun tile'ı çiz ---
    for (int row = 0; row < MAP_H; row++)
    {
        for (int col = 0; col < MAP_W; col++)
        {
            int tileID = tilemap[row][col];
            SDL_FRect dst = { col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE };
            switch (tileID)
            {
                case 0: // çimen
                    SDL_SetRenderDrawColor(renderer, 50, 200, 50, 255);
                    SDL_RenderFillRect(renderer, &dst);
                    break;
                case 1: // toprak yol
                    SDL_SetRenderDrawColor(renderer, 150, 100, 50, 255);
                    SDL_RenderFillRect(renderer, &dst);
                    break;
                case 2: // su
                    SDL_SetRenderDrawColor(renderer, 50, 50, 200, 255);
                    SDL_RenderFillRect(renderer, &dst);
                    break;
            }
        }
    }
}

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

// Oyuncuyu çizer - sadece OKUR, değiştirmez.
static void player_render(const Player *p, SDL_Renderer *renderer, SDL_Texture **layers, int layerCount)
{
    SDL_FRect src = {
        .x = p->currentFrame * FRAME_W,
        .y = p->animRow * FRAME_H,
        .w = FRAME_W,
        .h = FRAME_H
    };

    SDL_FlipMode flip = p->facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    for (int i = 0; i < layerCount; i++)
    {
        SDL_RenderTextureRotated(renderer, layers[i], &src, &p->rect, 0.0, NULL, flip);
    }

}

// Oyuncuyu günceller
static void player_update(Player *p, const bool *keys, float dt, int winW, int winH)
{
    // --- YÖN: basılı tuşlardan bir yön vektörü (dx, dy) kur ---
    float dx = 0.0f, dy = 0.0f;
    if (keys[SDL_SCANCODE_W]) dy -= 1;  // yukarı
    if (keys[SDL_SCANCODE_S]) dy += 1;  // aşağı
    if (keys[SDL_SCANCODE_A]) dx -= 1;  // sol
    if (keys[SDL_SCANCODE_D]) dx += 1;  // sağ

    bool moving = false;

    // --- NORMALİZE: yön vektörünü 1 birim uzunluğa indir ---
    float length = SDL_sqrtf(dx * dx + dy * dy);
    if (length > 0) {                    // sıfır kontrolü ŞART: 0/0 = NaN, kutuyu sessizce yok eder
        dx = dx / length;
        dy = dy / length;
        p->rect.x += dx * PLAYER_SPEED * dt;        // mesafe = hız × süre (kare hızından bağımsız)
        p->rect.y += dy * PLAYER_SPEED * dt;
        moving = true;

        if (SDL_fabsf(dx) > SDL_fabsf(dy)) {   // yatay baskın → yan
            p->facing = 2;
            p->facingLeft = (dx < 0);
        } else {                                // dikey baskın → yukarı/aşağı
            p->facing = (dy < 0) ? 1 : 0;          // dy<0 yukarı, değilse aşağı
        }
    }

    // --- KISIT: kutuyu pencere içine geri çek (önce hareket, sonra clamp) ---
    if (p->rect.x < 0.0f)         p->rect.x = 0.0f;          // sol kenar
    if (p->rect.y < 0.0f)         p->rect.y = 0.0f;          // üst kenar
    if (p->rect.x > winW - p->rect.w) p->rect.x = winW - p->rect.w;  // sağ kenar
    if (p->rect.y > winH - p->rect.h) p->rect.y = winH - p->rect.h;  // alt kenar

    // --- ANİMASYON SATIRI: facing + moving'den satırı TABLODAN oku ---
    p->animRow = ANIM_ROWS[moving][p->facing];
    int frameCount = 6;

    // --- GEÇİŞ: animasyon değiştiyse baştan başlat (ÖNCE) ---
    if (p->animRow != p->lastRow) {
        p->currentFrame = 0;
        p->animTimer = 0.0f;
        p->lastRow = p->animRow;
    }

    // --- SAYAÇ: kareyi zamanla ilerlet (SONRA) ---
    p->animTimer += dt;
    if (p->animTimer >= FRAME_DURATION) {
        p->animTimer -= FRAME_DURATION;
        p->currentFrame = (p->currentFrame + 1) % frameCount;
    }

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
    
    // Oyuncuyu oluştur + başlangıç durumunu ayarla
    Player player = 
    {
        .rect = { .x = 300.0f, .y = 300.0f, .w = 256, .h = 256 },
        .facing = 0,
        .facingLeft = false,
        .animRow = 0,
        .currentFrame = 0,
        .animTimer = 0.0f,
        .lastRow = -1
    };

    Uint64 last = SDL_GetTicks();

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
        int winW, winH;
        SDL_GetWindowSize(window, &winW, &winH); // pencere boyutunu al (dinamik olarak, pencere yeniden boyutlandırılabilir)

        player_update(&player, keys, dt, winW, winH); // oyuncuyu güncelle

        SDL_SetRenderDrawColor(renderer, 155, 155, 155, 255); // arka plan rengini gri yap
        SDL_RenderClear(renderer); // ekranı temizle (arka plan rengini doldur)
        tilemap_render(renderer); // haritayı çiz
        player_render(&player, renderer, layers, LAYER_COUNT); // oyuncuyu çiz
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