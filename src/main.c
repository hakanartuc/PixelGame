#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h> // platformlar arası 'main' köprülemesi (mobil/web için de gerekli)
#include <stdbool.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define DEBUG_DRAW 1

// Karakterin TÜM kalıcı durumu tek bir yapıda.
typedef struct{
    SDL_FRect rect;     // DÜNYA konumu + MANTIKSAL boyut (hareket, clamp, ileride çarpışma)
    float drawW, drawH; // EKRANDA çizim boyutu (sadece render kullanır)
    int facing;         // 0=aşağı, 1=yukarı, 2=yan
    bool facingLeft;    // true = sola bakıyor, false = sağa bakıyor
    int animRow;        // o an çizilecek sheet satırı (update yazar, render okur)
    int currentFrame;   // animasyonun hangi karesinde olduğumuz.
    float animTimer;    // kare ilerleme sayacı
    int lastRow;        // önceki karenin animRow'u; animasyon değiştiğinde sıfırlamak için
} Player;

#define PLAYER_SPEED   500.0f
#define FRAME_W        64
#define FRAME_H        64
#define FRAME_DURATION 0.1f

// Ayak kutusu: rect'in ALT kısmında, dar bir collision alanı.
// (top-down'da karakter ayağıyla çarpışır, tüm gövdesiyle değil)
#define FEET_H 20.0f   // ayak kutusunun yüksekliği
#define FEET_INSET 8.0f // yanlardan içeri ne kadar daraltılsın
#define SPRITE_FEET_Y 40 // sprite hücresinde karakterin ayağının olduğu satır, ölçülen değer

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
#define MAP_W     40          // harita kaç tile geniş
#define MAP_H     30          // harita kaç tile yüksek
static int tilemap[MAP_H][MAP_W]; // harita dizisi, her hücre bir tile ID tutar

typedef struct {
    float x,y; // kameranın dünya uzayındaki sol üst köşesi
} Camera;

// Basit bir dünya üret: kenarlar su (2), içi çimen (0), biraz da toprak desen
static void generate_map(void)
{
    for (int row = 0; row < MAP_H; row++) {
        for (int col = 0; col < MAP_W; col++) {
            if (row == 0 || col == 0 || row == MAP_H-1 || col == MAP_W-1)
                tilemap[row][col] = 2;        // kenar çerçevesi: su
            else
                tilemap[row][col] = 0;        // iç: çimen
        }
    }
}

// Bir tile ID'si geçilemez mi?
static bool tile_is_solid(int tileID)
{
    switch (tileID)
    {
        case 0: return false; // çimen
        case 1: return false; // toprak yol
        case 2: return true;  // su
        default: return true; // bilinmeyen tile'lar geçilemez
    }
}

// Verilen DÜNYA kutusu (x, y, w, h) katı bir tile ile çarpışıyor mu?
static bool box_hits_solid(SDL_FRect box)
{
    // Kutunun kapladığı tile aralığı (sütun ve satır olarak)
    int colL = (int)(box.x / TILE_SIZE); // sol kenar -> ilk sütun
    int colR = (int)((box.x + box.w -1) / TILE_SIZE); // sağ kenar -> son sütun (-1: kutunun sağ kenarı tam olarak bir tile'ın sol kenarına denk gelirse, o tile'ı da kapsasın)
    int rowT = (int)(box.y / TILE_SIZE); // üst kenar -> ilk satır
    int rowB = (int)((box.y + box.h -1) / TILE_SIZE); // alt kenar -> son satır

    // Kutunun kapsadığı tüm tile'ları kontrol et
    for (int row = rowT; row <= rowB; row++) {
        for (int col = colL; col <= colR; col++) {
            // SINIR KONTROLÜ: harita dışı = katı say (oyuncu dünyadan çıkamasın)
            if (col < 0 || col >= MAP_W || row < 0 || row >= MAP_H)
                return true; // harita dışı

            if(tile_is_solid(tilemap[row][col])) {
                return true; // çarpışma bulundu
            }
        }
    }

    return false;
}

// Verilen rect konumu için ayak kutusunu (collision alanı) hesaplar.
static SDL_FRect feet_box(float rectX, float rectY, float rectW, float rectH)
{
    SDL_FRect f = {
        rectX + FEET_INSET,
        rectY + rectH - FEET_H,
        rectW - FEET_INSET * 2.0f,
        FEET_H
    };
    return f;
}

static void tilemap_render(SDL_Renderer *renderer, const Camera *camera)
{
    // --- HARİTA RENDER: her hücre için uygun tile'ı çiz ---
    for (int row = 0; row < MAP_H; row++)
    {
        for (int col = 0; col < MAP_W; col++)
        {
            int tileID = tilemap[row][col];
            SDL_FRect dst = { 
                col * TILE_SIZE - camera->x, 
                row * TILE_SIZE - camera->y, 
                TILE_SIZE, 
                TILE_SIZE 
            };

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
static void player_render(const Player *p, SDL_Renderer *renderer, SDL_Texture **layers, int layerCount, const Camera *camera)
{
    SDL_FRect src = {
        .x = p->currentFrame * FRAME_W,
        .y = p->animRow * FRAME_H,
        .w = FRAME_W,
        .h = FRAME_H
    };
    
    // Sprite'ın görsel ayağı (hücrede y=40), rect'in ALTINA hizalanmalı.
    // Çizim kutusunun altındaki boşluk: (FRAME_H - 40) * ölçek = (64-40) * (drawH/FRAME_H)
    float scale = p->drawH / (float)FRAME_H;          // 128/64 = 2
    float footPadding = (FRAME_H - SPRITE_FEET_Y) * scale;       // (64-40)*2 = 48px boşluk
    SDL_FRect screen = {
        .x = (p->rect.x + p->rect.w / 2.0f) - p->drawW / 2.0f - camera->x,
        .y = (p->rect.y + p->rect.h) - p->drawH + footPadding - camera->y,  // +footPadding: çizimi aşağı kaydır
        .w = p->drawW,
        .h = p->drawH
    };

    SDL_FlipMode flip = p->facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    for (int i = 0; i < layerCount; i++)
    {
        SDL_RenderTextureRotated(renderer, layers[i], &src, &screen, 0.0, NULL, flip);
    }

    #if DEBUG_DRAW
    // --- DEBUG: collision kutularını çiz (dünya → ekran: kamerayı çıkar) ---
    // 1) rect (tüm ayak izi) — kırmızı çerçeve
    SDL_FRect rectScreen = {
        p->rect.x - camera->x,
        p->rect.y - camera->y,
        p->rect.w,
        p->rect.h
    };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);   // kırmızı
    SDL_RenderRect(renderer, &rectScreen);

    // 2) ayak kutusu (gerçek collision) — sarı çerçeve
    //    feetY = rect.y + rect.h - FEET_H, feetX = rect.x + FEET_INSET, feetW = rect.w - FEET_INSET*2
    SDL_FRect fb = feet_box(p->rect.x, p->rect.y, p->rect.w, p->rect.h);
    SDL_FRect feetScreen = { fb.x - camera->x, fb.y - camera->y, fb.w, fb.h };

    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // sarı
    SDL_RenderRect(renderer, &feetScreen);

    #endif
}

// Oyuncuyu günceller
static void player_update(Player *p, const bool *keys, float dt)
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
        moving = true;

        // X EKSENİ
        float newX = p->rect.x + dx * PLAYER_SPEED * dt;
        SDL_FRect fx = feet_box(newX, p->rect.y, p->rect.w, p->rect.h);
        if (!box_hits_solid(fx))
            p->rect.x = newX;

        // Y EKSENİ
        float newY = p->rect.y + dy * PLAYER_SPEED * dt;
        SDL_FRect fy = feet_box(p->rect.x, newY, p->rect.w, p->rect.h);
        if (!box_hits_solid(fy))
            p->rect.y = newY;
        

        if (SDL_fabsf(dx) > SDL_fabsf(dy)) {   // yatay baskın → yan
            p->facing = 2;
            p->facingLeft = (dx < 0);
        } else {                                // dikey baskın → yukarı/aşağı
            p->facing = (dy < 0) ? 1 : 0;          // dy<0 yukarı, değilse aşağı
        }
    }

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

// Kamera karakteri ekranda ortalayacak şekilde ayarlanır. Clamp ile harita dışına çıkmaz.
static void camera_update(Camera *camera, const Player *p, int winW, int winH)
{
    // Karakterin GÖRSEL merkezi (çizim kutusunun ortası)
    // Çizim, ayak izinin alt-ortasına hizalı; merkezi oradan drawH/2 yukarıda.
    float centerX = p->rect.x + p->rect.w / 2.0f;
    float centerY = p->rect.y + p->rect.h / 2.0f;   // ayak izi merkezi — çizim boyutundan BAĞIMSIZ

    // kamera = hedef_merkez - ekran_yarısı -> karakter ekran ortasında görünür.
    camera->x = centerX - winW / 2.0f;
    camera->y = centerY - winH / 2.0f; 

    // --- KISIT: kamera görüş alanını harita sınırları içine çek ---
    float worldW = MAP_W * TILE_SIZE;
    float worldH = MAP_H * TILE_SIZE;
    if (camera->x < 0.0f)         camera->x = 0.0f;          // sol kenar
    if (camera->y < 0.0f)         camera->y = 0.0f;          // üst kenar
    if (camera->x > worldW - winW) camera->x = worldW - winW;  // sağ kenar
    if (camera->y > worldH - winH) camera->y = worldH - winH;  // alt kenar
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
        .rect = { .x = 300.0f, .y = 300.0f, .w = 48.0f, .h = 64.0f }, // dünya ayak izi (küçük)
        .drawW = 128.0f, .drawH = 128.0f, // ekranda çizim boyutu (büyük)
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

    generate_map(); // haritayı oluştur
    Camera camera = { .x = 0.0f, .y = 0.0f };

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

        player_update(&player, keys, dt); // oyuncuyu güncelle
        camera_update(&camera, &player, winW, winH);  // kamerayı güncelle - karakteri takip et

        SDL_SetRenderDrawColor(renderer, 155, 155, 155, 255); // arka plan rengini gri yap
        SDL_RenderClear(renderer); // ekranı temizle (arka plan rengini doldur)
        tilemap_render(renderer, &camera); // haritayı çiz
        player_render(&player, renderer, layers, LAYER_COUNT, &camera); // oyuncuyu 
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