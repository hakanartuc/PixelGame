#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h> // platformlar arası 'main' köprülemesi (mobil/web için de gerekli)
#include <stdbool.h>
#include "common.h"
#include "tilemap.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define DEBUG_DRAW 1

// Dünyada var olan, hareket eden, hasar alabilen her şeyin ortak çekirdeği.
typedef struct {
    SDL_FRect rect;            // konum + boyut (collision/hurtbox)
    float health, maxHealth;   // can
    bool alive;                // yaşıyor mu
    int facing;                // 0=aşağı, 1=yukarı, 2=yan
    bool facingLeft;
    float speed;               // hareket hızı
    float hitFlash;            // vuruş flaşı (saniye, geri sayar)
    float knockbackX, knockbackY;  // geri itilme hızı
} Entity;

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

    // Durum sistemleri
    float speed;        // anlık hareket hızı
    float health;       // anlık can
    float maxHealth;    // maksimum can

    // Saldırı durumu
    bool attacking;       // şu an saldırı animasyonu/penceresi aktif mi?
    float attackTimer;    // saldırının kalan süresi (saniye)
    bool attackHit;       // bu saldırıda dummy'ye değdi mi (tekrar vurmasın)

} Player;

// Düşmanın davranış durumu
typedef enum {
    ENEMY_IDLE,    // bekliyor
    ENEMY_CHASE,   // oyuncuyu kovalıyor
    ENEMY_ATTACK,  // saldırı menzilinde
} EnemyState;

// Düşman = ortak çekirdek + AI'a özel alanlar
typedef struct {
    Entity e;                 // ortak çekirdek (konum, can, hareket, vuruş tepkisi)
    EnemyState state;         // AI durumu
    float attackCooldown;     // bir sonraki saldırıya kalan süre
    bool attackTriggered;
} Enemy;

#define MAX_ENEMIES 10

#define SPAWN_X 300.0f
#define SPAWN_Y 300.0f

#define FRAME_W        64
#define FRAME_H        64
#define FRAME_DURATION 0.1f

// Ayak kutusu: rect'in ALT kısmında, dar bir collision alanı.
// (top-down'da karakter ayağıyla çarpışır, tüm gövdesiyle değil)

#define SPRITE_FEET_Y 40 // sprite hücresinde karakterin ayağının olduğu satır, ölçülen değer
#define PLAYER_SPEED 500.0f

// Animasyon satır tablosu: [hareket durumu][yön] → sheet satırı
// hareket: 0 = idle, 1 = yürüyüş
// yön:     0 = aşağı, 1 = yukarı, 2 = yan
static const int ANIM_ROWS[2][3] = {
    // aşağı  yukarı  yan
    {   0,      2,     1  },   // [0] idle
    {   3,      5,     4  },   // [1] yürüyüş
};


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

static void enemy_render(const Enemy *d, SDL_Renderer *renderer, const Camera *camera)
{
    if (!d->e.alive) return;

    SDL_FRect screen = {
        d->e.rect.x - camera->x,
        d->e.rect.y - camera->y,
        d->e.rect.w,
        d->e.rect.h
    };

    if (d->e.hitFlash > 0.0f)
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);  // flaş
    else
        SDL_SetRenderDrawColor(renderer, 180, 40, 160, 255);   // normal mor

    SDL_RenderFillRect(renderer, &screen);
}

static void enemy_update(Enemy *d, float targetX, float targetY, float dt)
{
    if (!d->e.alive) return;

    // Flaş ve knockback
    if (d->e.hitFlash > 0.0f) d->e.hitFlash -= dt;
    d->e.rect.x += d->e.knockbackX * dt;
    d->e.rect.y += d->e.knockbackY * dt;
    float damping = 12.0f;
    d->e.knockbackX -= d->e.knockbackX * damping * dt;
    d->e.knockbackY -= d->e.knockbackY * damping * dt;

    // AI: hedefe mesafe (hedef merkezi parametre olarak geldi)
    float dx = targetX - (d->e.rect.x + d->e.rect.w/2.0f);
    float dy = targetY - (d->e.rect.y + d->e.rect.h/2.0f);
    float dist = SDL_sqrtf(dx*dx + dy*dy);

    // Durum kararı
    float detectRange = 300.0f;
    float attackRange = 60.0f;
    if (dist < attackRange)       d->state = ENEMY_ATTACK;
    else if (dist < detectRange)  d->state = ENEMY_CHASE;
    else                          d->state = ENEMY_IDLE;

    // Cooldown azalt
    if (d->attackCooldown > 0.0f) d->attackCooldown -= dt;

    // Davranış
    if (d->state == ENEMY_CHASE) {
        if (dist > 0.0f) {                       // sıfır bölme koruması
            float nx = dx / dist, ny = dy / dist;
            d->e.rect.x += nx * d->e.speed * dt;
            d->e.rect.y += ny * d->e.speed * dt;
        }
    }
    
    d->attackTriggered = false;   // her kare sıfırla
    if (d->state == ENEMY_ATTACK) {
        if (d->attackCooldown <= 0.0f) {
            d->attackTriggered = true;   // "saldırdım" — hasarı dışarısı versin
            d->attackCooldown = 1.0f;
        }
    }
}

// Oyuncunun saldırı hitbox'ını (baktığı yönde) hesaplar.
static SDL_FRect attack_hitbox(const Player *p)
{
    float w = 40.0f, h = 40.0f;   // hitbox boyutu
    // Oyuncunun merkezinden başla
    float cx = p->rect.x + p->rect.w / 2.0f;
    float cy = p->rect.y + p->rect.h / 2.0f;

    float hx = cx, hy = cy;
    // Yöne göre hitbox'ı oyuncunun ÖNÜNE kaydır:
    if (p->facing == 2) {            // yan
        hx = p->facingLeft ? (p->rect.x - w) : (p->rect.x + p->rect.w);
        hy = cy - h / 2.0f;
    } else if (p->facing == 1) {     // yukarı
        hx = cx - w / 2.0f;
        hy = p->rect.y - h;
    } else {                          // aşağı (facing == 0)
        hx = cx - w / 2.0f;
        hy = p->rect.y + p->rect.h;
    }
    SDL_FRect box = { hx, hy, w, h };
    return box;
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

    if (p->attacking) {
        SDL_FRect hb = attack_hitbox(p);
        SDL_FRect hbScreen = { hb.x - camera->x, hb.y - camera->y, hb.w, hb.h };
        SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);  // mavi
        SDL_RenderRect(renderer, &hbScreen);
    }

    #endif
}

// Oyuncuyu günceller
static void player_update(Player *p, const bool *keys, bool attackPressed, float dt)
{
    // --- TILE TEPKİSİ: bastığın tile'ın özelliklerine göre hız/can ayarla ---
    SDL_FRect fb = feet_box(p->rect.x, p->rect.y, p->rect.w, p->rect.h);
    int groundTile = tile_at_point(fb.x + fb.w / 2.0f, fb.y + fb.h / 2.0f);

    if (groundTile >= 0 && groundTile < TILE_TYPE_COUNT) {
        // hız: temel hız × tile'ın çarpanı (her kare yeniden hesaplanır)
        p->speed = PLAYER_SPEED * TILE_TYPES[groundTile].speedMultiplier;

        // hasar: tile saniyede 'damage' veriyorsa, dt kadarını uygula
        if (TILE_TYPES[groundTile].damage > 0.0f)
            p->health -= TILE_TYPES[groundTile].damage * dt;

    } else {
        p->speed = PLAYER_SPEED;   // geçersiz tile → normal hız
    }

    if (p->health <= 0.0f) {
        p->rect.x = SPAWN_X;
        p->rect.y = SPAWN_Y;
        p->health = p->maxHealth;
        p->attacking = false;      // yarım kalan saldırıyı iptal et
        p->attackTimer = 0.0f;
    }

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
        float newX = p->rect.x + dx * p->speed * dt;
        SDL_FRect fx = feet_box(newX, p->rect.y, p->rect.w, p->rect.h);
        if (!box_hits_solid(fx))
            p->rect.x = newX;

        // Y EKSENİ
        float newY = p->rect.y + dy * p->speed * dt;
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

    // --- SALDIRI DURUMU ---
    // Yeni saldırı başlat (sadece şu an saldırmıyorsa — üst üste binmesin)
    if (attackPressed && !p->attacking) {
        p->attacking = true;
        p->attackTimer = 0.3f;   // saldırı penceresi süresi
        p->attackHit = false;
    }
    // Süreyi azalt, bitince kapat
    if (p->attacking) {
        p->attackTimer -= dt;
        if (p->attackTimer <= 0.0f)
            p->attacking = false;
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