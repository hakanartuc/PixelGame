#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h> // platformlar arası 'main' köprülemesi (mobil/web için de gerekli)
#include <stdbool.h>
#include "common.h"
#include "tilemap.h"
#include "player.h"
#include "mob.h"
#include "render.h"
#include "loot.h"


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

    SDL_Texture *crateTex = load_texture(renderer, "assets/Loot/sheetlist_48x32_smallchest_1.png");
    if (crateTex == NULL) {
        SDL_Log("Kasa texture yuklenemedi");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    Loot loots[MAX_LOOT] = {0};   // hepsi inactive başlar

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

    // Mob texture'ları: her tür için sprite yükle (TILE_TYPES gibi)
    SDL_Texture *mob_textures[MAX_MOB_TYPES] = {0};
    for (int i = 0; i < MOB_TYPE_COUNT; i++) {
        mob_textures[i] = load_texture(renderer, MOB_TYPES[i].texturePath);
        if (mob_textures[i] == NULL) {
            SDL_Log("Mob texture yuklenemedi: %s", MOB_TYPES[i].texturePath);
            for (int j = 0; j < LAYER_COUNT; j++)
                if (mob_textures[j]) SDL_DestroyTexture(mob_textures[j]);  // sadece yüklenmiş olanları yık
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
        return 1;
        }
    }

    Mob mobs[MAX_MOBS] = {0};   // hepsi sıfır başlar
    int mobCount = MAX_MOBS;

    int nextMobId = 1;

    // Spawn bölgeleri — KO'daki "kamp"lar: bir alanda birden çok mob.
    SpawnZone spawnZones[] = {
        { .x = 1000.0f, .y = 300.0f, .radius = 200.0f, .type = &MOB_TYPES[0], .maxCount = 1, .respawnTimer = 0.0f },  // goblin kampı (5 mob)
        { .x = 1500.0f, .y = 700.0f, .radius = 150.0f, .type = &MOB_TYPES[1], .maxCount = 1, .respawnTimer = 0.0f },  // ork kampı (3 mob)
    };
    int spawnZoneCount = (int)(sizeof(spawnZones) / sizeof(spawnZones[0]));
    spawn_zones_fill(spawnZones, spawnZoneCount, mobs, MAX_MOBS, &nextMobId);   // ← başlangıçta doldur

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
        bool targetPressed = false;
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
                else if (event.key.key == SDLK_Z) {
                    targetPressed = true;   // hedef seçme tuşu
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) 
            {
                if (event.button.button == SDL_BUTTON_LEFT) 
                {
                    float worldX = event.button.x + camera.x;
                    float worldY = event.button.y + camera.y;
                    int li = loot_find_at(loots, MAX_LOOT, worldX, worldY);
                    
                    if (li >= 0 && loots[li].state == LOOT_CLOSED) 
                    {
                        loots[li].state = LOOT_OPENING;   // aç
                        loots[li].currentFrame = 0;
                        loots[li].animTimer = 0.0f;
                    }
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

        // --- SALDIRI GEÇERLİLİĞİ (KO tarzı): hedef yoksa ya da menzil dışıysa saldırma ---
        if (attackPressed) {
            Mob *target = mob_find_by_id(mobs, mobCount, player.targetMobId);
            if (target == NULL) {
                // Hedef yok → saldırma
                attackPressed = false;
            } else {
                // Hedef var → menzilde mi?
                float tdx = (target->e.rect.x + target->e.rect.w/2.0f) - (player.rect.x + player.rect.w/2.0f);
                float tdy = (target->e.rect.y + target->e.rect.h/2.0f) - (player.rect.y + player.rect.h/2.0f);
                float tdist = SDL_sqrtf(tdx*tdx + tdy*tdy);

                if (tdist > PLAYER_ATTACK_RANGE) {
                    // Menzil dışı → saldırma (boşa savurma yok)
                    attackPressed = false;
                } else {
                    // Geçerli saldırı → oyuncuyu hedefe döndür
                    if (SDL_fabsf(tdx) > SDL_fabsf(tdy)) {
                        player.facing = 2;
                        player.facingLeft = (tdx < 0);
                    } else {
                        player.facing = (tdy < 0) ? 1 : 0;
                    }
                }
            }
        }

        player_update(&player, keys, attackPressed, dt); // oyuncuyu güncelle

        if (targetPressed) {
            float px = player.rect.x + player.rect.w/2.0f;
            float py = player.rect.y + player.rect.h/2.0f;
            player.targetMobId = mob_find_nearest(mobs, mobCount, px, py);

            // Hedef seçildiyse oyuncuyu ona döndür (KO tarzı)
            if (player.targetMobId != 0) {
                Mob *target = mob_find_by_id(mobs, mobCount, player.targetMobId);
                if (target != NULL) {
                    float tdx = (target->e.rect.x + target->e.rect.w/2.0f) - px;
                    float tdy = (target->e.rect.y + target->e.rect.h/2.0f) - py;
                    if (SDL_fabsf(tdx) > SDL_fabsf(tdy)) {
                        player.facing = 2;
                        player.facingLeft = (tdx < 0);
                    } else {
                        player.facing = (tdy < 0) ? 1 : 0;
                    }
                }
            }
        }

        // --- HEDEF GEÇERLİLİĞİ: seçili mob öldüyse/yoksa hedefi temizle ---
        if (player.targetMobId != 0) {
            Mob *target = mob_find_by_id(mobs, mobCount, player.targetMobId);
            if (target == NULL) {
                player.targetMobId = 0;   // hedef artık yok → temizle
            }
        }

        // --- SALDIRI HASARI: vuruş karesinde, hedef hâlâ menzildeyse ---
        if (player.attacking && !player.attackHit && player.currentFrame == PLAYER_ATTACK_HIT_FRAME) {
            Mob *target = mob_find_by_id(mobs, mobCount, player.targetMobId);
            if (target != NULL) {
                float tdx = (target->e.rect.x + target->e.rect.w/2.0f) - (player.rect.x + player.rect.w/2.0f);
                float tdy = (target->e.rect.y + target->e.rect.h/2.0f) - (player.rect.y + player.rect.h/2.0f);
                float tdist = SDL_sqrtf(tdx*tdx + tdy*tdy);

                if (tdist <= PLAYER_ATTACK_RANGE) {
                    target->e.health -= 25.0f;
                    target->e.hitFlash = 0.15f;
                    float kb = 180.0f;
                    if (tdist > 0.0f) {
                        target->e.knockbackX = (tdx / tdist) * kb;
                        target->e.knockbackY = (tdy / tdist) * kb;
                    }
                    if (target->e.health <= 0.0f)
                    {
                        // Anında kaybolma YERİNE ölüm animasyonu başlat
                        target->e.alive = false;      // AI/hedef dışı
                        target->dying = true;          // death animasyonu başlasın
                        target->state = MOB_DYING;
                        target->currentFrame = 0;
                        target->animTimer = 0.0f;

                        // Ganimet düşür (mob'un konumunda)
                        loot_spawn(loots, MAX_LOOT, target->e.rect.x, target->e.rect.y);
                    }
                        
                }
            }
            player.attackHit = true;   // bu saldırıda denendi (tekrar etmesin)
        }

        spawn_zones_update(spawnZones, spawnZoneCount, mobs, MAX_MOBS, &nextMobId, dt);

        float px = player.rect.x + player.rect.w/2.0f;
        float py = player.rect.y + player.rect.h/2.0f;
        for (int i = 0; i < mobCount; i++) {
            mob_update(&mobs[i],mobs, mobCount, px, py, dt);

            // Düşman bu kare saldırdıysa ve oyuncu menzildeyse, hasarı BURADA uygula
            if (mobs[i].attackTriggered) {
                player.health -= mobs[i].type->damage;   // türe göre hasar
                if (player.health < 0.0f) player.health = 0.0f;
            }
        }
        
        loot_update_all(loots, MAX_LOOT, dt);

        camera_update(&camera, player.rect.x + player.rect.w/2.0f,
                               player.rect.y + player.rect.h/2.0f, winW, winH);  // kamerayı güncelle - karakteri takip et

    

        SDL_SetRenderDrawColor(renderer, 155, 155, 155, 255); // arka plan rengini gri yap
        SDL_RenderClear(renderer); // ekranı temizle (arka plan rengini doldur)
        tilemap_render(renderer, tile_textures, &camera); // haritayı çiz
        player_render(&player, renderer, layers, LAYER_COUNT, &camera); // oyuncuyu 
        
        for (int i = 0; i < mobCount; i++)
            mob_render(&mobs[i], renderer, mob_textures, &camera, player.targetMobId);

        
        loot_render_all(loots, MAX_LOOT, renderer, crateTex, &camera);   // ← kasalar (mob'lardan sonra, HUD'dan önce)

         // Hedef HP bar (seçili mob varsa)
        Mob *hpTarget = mob_find_by_id(mobs, mobCount, player.targetMobId);
        target_hp_bar_render(renderer, hpTarget, winW);

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

    for (int i = 0; i < MOB_TYPE_COUNT; i++) {
        if (mob_textures[i]) SDL_DestroyTexture(mob_textures[i]);  // NULL olanları atla
    }

    if (crateTex) 
        SDL_DestroyTexture(crateTex);
    SDL_DestroyRenderer(renderer); // Renderer'ı temizle
    SDL_DestroyWindow(window);     // Pencereyi temizle
    SDL_Quit();                    // SDL'i temizle
    return 0;                      // Başarılı çık
}