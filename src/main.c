#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h> // platformlar arası 'main' köprülemesi (mobil/web için de gerekli)
#include <stdbool.h>
#include "common.h"
#include "tilemap.h"
#include "player.h"
#include "enemy.h"

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

// Can barını ekranın köşesine çizer. Ekran-uzayına bağlıdır, kamera hareketinden etkilenmez.
static void health_bar_render(const Player *p, SDL_Renderer *renderer)
{
    // Bar ölçüleri (ekran pikseli - doğrudan, kamera yok)
    float barX = 20.0f, barY = 20.0f;  // sol üst köşeden offset
    float barW = 200.0f, barH = 24.0f; // tam barın boyutu

    // 1) Arka plan (boş bar) - koy kırmızı/gri
    SDL_FRect bg = {barX, barY, barW, barH};
    SDL_SetRenderDrawColor(renderer, 60, 20, 20, 255);
    SDL_RenderFillRect(renderer, &bg);

    // 2) Dolu kısım- cana oranlı genişlik
    float ratio = p->health / p->maxHealth; // 0..1
    // ratio'yu 0..1 arasında sıkıştır (can negatif ya da max'tan büyük olamaz)
    if(ratio < 0.0f) ratio = 0.0f;
    if(ratio > 1.0f) ratio = 1.0f;

    SDL_FRect fill = {barX, barY, barW * ratio, barH};  // genişlik oransal
    SDL_SetRenderDrawColor(renderer, 220, 50, 50, 255); // parlak kırmızı
    SDL_RenderFillRect(renderer, &fill);

    // 3) Çerçeve - barın kenarı
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &bg);
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
        .rect = { .x = SPAWN_X, .y = SPAWN_Y, .w = 48.0f, .h = 64.0f }, // dünya ayak izi (küçük)
        .drawW = 128.0f, .drawH = 128.0f, // ekranda çizim boyutu (büyük)
        .facing = 0,
        .facingLeft = false,
        .animRow = 0,
        .currentFrame = 0,
        .animTimer = 0.0f,
        .lastRow = -1,
        .speed = PLAYER_SPEED,
        .health = 100.0f,
        .maxHealth = 100.0f
    };

    Enemy enemies[MAX_ENEMIES] = {0};   // hepsi sıfır başlar
    int enemyCount = 0;

    enemies[0] = (Enemy){
        .e = {

        .rect = { .x = 500.0f, .y = 200.0f, .w = 48.0f, .h = 64.0f },
        .health = 100.0f,
        .alive = true,
        .hitFlash = 0.0f,
        .knockbackX = 0.0f,
        .knockbackY = 0.0f,
        .speed = 200.0f, // oyuncudan (500) yavaş — kaçabilirsin
        .maxHealth = 100.0f
        },
        .state = ENEMY_IDLE,
        .attackCooldown = 0.0f
    };
    enemyCount = 1;

    enemies[1] = (Enemy){
        .e = {
            .rect = { .x = 800.0f, .y = 500.0f, .w = 48.0f, .h = 64.0f },
            .health = 100.0f, .maxHealth = 100.0f,
            .alive = true, .speed = 250.0f,   // farklı hız!
        },
        .state = ENEMY_IDLE,
        .attackCooldown = 0.0f,
    };
    enemyCount = 2;

    // Tile texture'ları: TILE_TYPES tablosundaki path'lerden yükle.
    // texturePath == NULL olanlar (örn. taş) texture'sız kalır → render'da renk basılır.
    SDL_Texture *tile_textures[MAX_TILE_TYPES] = {0};   // TILE_TYPE_COUNT yerine MAX_TILE_TYPES
    for (int i = 0; i < TILE_TYPE_COUNT; i++) {
        if (TILE_TYPES[i].texturePath != NULL) {
            tile_textures[i] = load_texture(renderer, TILE_TYPES[i].texturePath);
            // not: yüklenemezse NULL kalır, render renk fallback'ine düşer — oyunu çökertme
        }
    }

    Uint64 last = SDL_GetTicks();

    // 4) Oyun döngüsü
    bool running = true;

    generate_map(); // haritayı oluştur
    Camera camera = { .x = 0.0f, .y = 0.0f };

    while (running)
    {
        bool attackPressed = false;
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
                else if(event.key.key == SDLK_SPACE){
                     // SALDIRI tetiklendi (anlık olay — basılı tutmak tekrar tetiklemez*)
                    attackPressed = true;
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

        player_update(&player, keys, attackPressed, dt); // oyuncuyu güncelle

        // --- SALDIRI ÇARPIŞMASI ---
        if (player.attacking && !player.attackHit) {
            SDL_FRect hb = attack_hitbox(&player);
            for (int i = 0; i < enemyCount; i++) {
                if (!enemies[i].e.alive) continue;   // ölüleri atla
                if (SDL_HasRectIntersectionFloat(&hb, &enemies[i].e.rect)) {
                    enemies[i].e.health -= 25.0f;
                    player.attackHit = true;

                    enemies[i].e.hitFlash = 0.15f;
                    float kb = 180.0f;
                    if (player.facing == 2)
                        enemies[i].e.knockbackX = player.facingLeft ? -kb : kb;
                    else if (player.facing == 1)
                        enemies[i].e.knockbackY = -kb;
                    else
                        enemies[i].e.knockbackY = kb;

                    if (enemies[i].e.health <= 0.0f)
                        enemies[i].e.alive = false;

                    break;   // bir saldırıda bir düşmana vur (isteğe bağlı)
                }
            }
        }

        float px = player.rect.x + player.rect.w/2.0f;
        float py = player.rect.y + player.rect.h/2.0f;
        for (int i = 0; i < enemyCount; i++) {
            enemy_update(&enemies[i], px, py, dt);

            // Düşman bu kare saldırdıysa ve oyuncu menzildeyse, hasarı BURADA uygula
            if (enemies[i].attackTriggered) {
                player.health -= 10.0f;
                if (player.health < 0.0f) player.health = 0.0f;
            }
        }

        camera_update(&camera, &player, winW, winH);  // kamerayı güncelle - karakteri takip et

        SDL_SetRenderDrawColor(renderer, 155, 155, 155, 255); // arka plan rengini gri yap
        SDL_RenderClear(renderer); // ekranı temizle (arka plan rengini doldur)
        tilemap_render(renderer, tile_textures, &camera); // haritayı çiz
        player_render(&player, renderer, layers, LAYER_COUNT, &camera); // oyuncuyu 
        
        for (int i = 0; i < enemyCount; i++)
            enemy_render(&enemies[i], renderer, &camera);
        
        health_bar_render(&player, renderer);
        SDL_RenderPresent(renderer); // Ekrana bas.
    }

    // 5) Her şeyi oluşturmanın TERS sırasıyla temizle. C'de bu bizim sorumluluğumuzda,
    // hiçbir destructor bunu kendiliğinden yapmaz.
    for (int i = 0; i < LAYER_COUNT; i++) {
        SDL_DestroyTexture(layers[i]);
    }
    for (int i = 0; i < TILE_TYPE_COUNT; i++) {
        if (tile_textures[i]) SDL_DestroyTexture(tile_textures[i]);  // NULL olanları atla
    }
    SDL_DestroyRenderer(renderer); // Renderer'ı temizle
    SDL_DestroyWindow(window);     // Pencereyi temizle
    SDL_Quit();                    // SDL'i temizle
    return 0;                      // Başarılı çık
}