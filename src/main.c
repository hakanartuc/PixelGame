#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h> // platformlar arası 'main' köprülemesi (mobil/web için de gerekli)
#include <stdbool.h>
#include "font.h"
#include "common.h"
#include "floattext.h"
#include "tilemap.h"
#include "player.h"
#include "mob.h"
#include "render.h"
#include "loot.h"
#include "inventory.h"
#include "equipment.h"
#include "item_textures.h"


int main(int argc, char *argv[])
{
    // 1) SDL'in video alt sistemini ayağa kaldır. SDL3 başarılıysa 'true' döner.
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init basarısız: %s", SDL_GetError());
        return 1; // Başarısızsa çık
    }

    if (!font_init()) {
        // hata — ama oyunu çökertme, font olmadan devam edilebilir
        SDL_Log("Font sistemi baslatilamadi");
    }
    TTF_Font *gameFont = font_load("assets/fonts/PublicPixel-rv0pA.ttf", 16.0f);
    if (gameFont == NULL) {
        SDL_Log("Oyun fontu yuklenemedi");
        // devam — draw_text NULL font'ta hiçbir şey çizmez (güvenli)
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

    SDL_SetRenderVSync(renderer, 1);

    SDL_Texture *crateTex = load_texture(renderer, "assets/loot/chest_small.png");
    if (crateTex == NULL) {
        SDL_Log("Kasa texture yuklenemedi");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture *itemIconTex = load_texture(renderer, "assets/icons/items/tool_icons.png");
    if (itemIconTex == NULL) {
        SDL_Log("Item texture yuklenemedi");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    // (yolu kendi konumuna göre ayarla)

    Loot loots[MAX_LOOT] = {0};   // hepsi inactive başlar
    int openLootIndex = -1;
    Inventory inventory = {0};   // tüm slotlar boş başlar (main'de, oyuncuyla birlikte)

    Equipment equipment = {0};   // tüm slotlar boş başlar

    ItemTextures itemTex;
    item_textures_load(&itemTex, renderer);


    // base → feet (ayakkabı) → legs (pantolon) → chest (gömlek) → hands (eller) → hair (saç)
    // Katmanlar ÇİZİM SIRASINDA (alttan üste). Yeni parça eklemek = buraya bir satır.
    // base katmanlar: body → head → face. { texture, z } — z = LPC dosya öneki (çizim sırası, küçük=arka).
    PlayerLayer layers[] = {
        { load_texture(renderer, "assets/characters/body/male/light.png"),         10  },
        { load_texture(renderer, "assets/characters/head/male/human_light.png"),   100 },
        { load_texture(renderer, "assets/characters/face/male/neutral_light.png"), 101 },
    };

    #define LAYER_COUNT ((int)(sizeof(layers) / sizeof(layers[0])))

    for (int i = 0; i < LAYER_COUNT; i++)
    if (!layers[i].tex) SDL_Log("Katman yuklenemedi (idx %d)", i);
    
    // Oyuncuyu oluştur + başlangıç durumunu ayarla
    Player player = {
        .rect = { .x = SPAWN_X, .y = SPAWN_Y, .w = 48.0f, .h = 64.0f },
        .drawW = 64.0f, .drawH = 64.0f,
        .facing = DIR_DOWN,        // başlangıç yönü
        .state = PLAYER_IDLE,
        .currentFrame = 0,
        .animTimer = 0.0f,
        .lastState = -1,           // ilk karede geçiş tetiklensin
        .lastFacing = -1,
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

    FloatText floatTexts[MAX_FLOAT_TEXTS] = {0};

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

    bool inventoryOpen = false;

    // 4) Oyun döngüsü
    bool running = true;

    generate_map(); // haritayı oluştur
    Camera camera = { .x = 0.0f, .y = 0.0f };

    Uint64 last = SDL_GetTicks();

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
                {
                    if (openLootIndex >= 0) {
                        loots[openLootIndex].state = LOOT_CLOSING;   // kasayı da kapat
                        openLootIndex = -1;
                    } else {
                        running = false;
                    }
                }
                else if(event.key.key == SDLK_SPACE){
                     // SALDIRI tetiklendi (anlık olay — basılı tutmak tekrar tetiklemez*)
                    attackPressed = true;
                }
                else if (event.key.key == SDLK_Z) {
                    targetPressed = true;   // hedef seçme tuşu
                }
                else if (event.key.key == SDLK_I) {
                    inventoryOpen = !inventoryOpen;   // aç/kapat
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    float worldX = event.button.x + camera.x;
                    float worldY = event.button.y + camera.y;

                    bool handled = false;

                    // 1) Popup açıksa: item'e mi, yoksa dışarı mı tıklandı?
                    if (openLootIndex >= 0) {
                        int itemIdx = loot_popup_item_at(&loots[openLootIndex], worldX, worldY);
                        if (itemIdx >= 0) {
                            // Item al
                            ItemStack *stack = &loots[openLootIndex].items[itemIdx];
                            if (stack->type != NULL) {
                                if (inventory_add(&inventory, stack->type, stack->count)) {
                                    stack->type = NULL;
                                    stack->count = 0;
                                    if (loot_is_empty(&loots[openLootIndex])) {
                                        loots[openLootIndex].active = false;   // boş kasa KAYBOLUR
                                        openLootIndex = -1;
                                    }
                                }
                            }
                            handled = true;
                        }
                        else {
                            // Popup açık ama item'e tıklanmadı → dışarı tıklama
                            // Kasayı kapat + popup'ı kapat
                            loots[openLootIndex].state = LOOT_CLOSING;
                            openLootIndex = -1;
                            handled = true;
                        }
                    }

                    // 2) Popup kapalıysa: kasa açma
                    if (!handled) {
                        int li = loot_find_at(loots, MAX_LOOT, worldX, worldY);
                        if (li >= 0 && loots[li].state == LOOT_CLOSED) {
                            loots[li].state = LOOT_OPENING;
                            loots[li].currentFrame = 0;
                            loots[li].animTimer = 0.0f;
                            openLootIndex = li;
                        }
                    }
                }
                 else if (event.button.button == SDL_BUTTON_RIGHT) {
                    if (inventoryOpen) {
                        int winW, winH;
                        SDL_GetWindowSize(window, &winW, &winH);
                        int panelX = (winW - 400) / 2;

                        // 1) Once ekipman paneline mi tiklandi? (panelY = 40)
                        EquipSlot eqSlot = equipment_slot_at((float)event.button.x,
                                                             (float)event.button.y, panelX, 40);
                        if (eqSlot != EQUIP_NONE) {
                            unequip_slot(&equipment, &inventory, eqSlot);   // cikar → envantere
                        }
                        else {
                            // 2) Degilse envantere mi? (panelY = 40+340)
                            int slot = inventory_slot_at((float)event.button.x, (float)event.button.y,
                                                         (float)panelX, (float)(40 + 340));
                            if (slot >= 0)
                                equip_from_inventory(&equipment, &inventory, slot);
                        }
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
                        player.facing = (tdx < 0) ? DIR_LEFT : DIR_RIGHT;
                    } else {
                        player.facing = (tdy < 0) ? DIR_UP : DIR_DOWN;
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
                       player.facing = (tdx < 0) ? DIR_LEFT : DIR_RIGHT;
                    } else {
                        player.facing = (tdy < 0) ? DIR_UP : DIR_DOWN;
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

                float dmg = 50.0f;
                if (tdist <= PLAYER_ATTACK_RANGE) {
                    target->e.health -= dmg;
                    target->e.hitFlash = 0.15f;

                    floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                    target->e.rect.x + target->e.rect.w/2.0f + 10.0f,
                                    target->e.rect.y, (int)dmg);

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


        floattext_update_all(floatTexts, MAX_FLOAT_TEXTS, dt);

        camera_update(&camera, player.rect.x + player.rect.w/2.0f,
                               player.rect.y + player.rect.h/2.0f, winW, winH);  // kamerayı güncelle - karakteri takip et

    

        SDL_SetRenderDrawColor(renderer, 155, 155, 155, 255); // arka plan rengini gri yap
        SDL_RenderClear(renderer); // ekranı temizle (arka plan rengini doldur)
        tilemap_render(renderer, tile_textures, &camera); // haritayı çiz

        PlayerLayer built[16];
        int builtCount = build_player_layers(layers, LAYER_COUNT, &equipment,
                                     itemTex.equipTex, built, 16);

        player_render(&player, renderer, built, builtCount, &camera); // oyuncuyu 
        
        for (int i = 0; i < mobCount; i++)
            mob_render(&mobs[i], renderer, mob_textures, &camera, player.targetMobId);

        if (openLootIndex >= 0)
            loot_popup_render(renderer, &loots[openLootIndex], itemIconTex, &itemTex, gameFont, &camera);

        
        loot_render_all(loots, MAX_LOOT, renderer, crateTex, &camera);   // ← kasalar (mob'lardan sonra, HUD'dan önce)

        // Hasar yazıları (mob'ların üstünde, dünya uzayında)
        floattext_render_all(floatTexts, MAX_FLOAT_TEXTS, renderer, gameFont, &camera);   // ← BURAYA (present'tan önce)

         // Hedef HP bar (seçili mob varsa)
        Mob *hpTarget = mob_find_by_id(mobs, mobCount, player.targetMobId);
        target_hp_bar_render(renderer, hpTarget, winW, gameFont);

        health_bar_render(&player, renderer);

        if (inventoryOpen) 
        {
           int panelX = (winW - 400) / 2;
            int panelY = 40;
            equipment_render(&equipment, renderer, itemIconTex, &itemTex,
                             built, builtCount, panelX, panelY);
            inventory_render(&inventory, renderer, itemIconTex, &itemTex, gameFont,
                             (float)panelX, (float)(panelY + 340));
        }

        SDL_RenderPresent(renderer); // Ekrana bas.
    }

    // 5) Her şeyi oluşturmanın TERS sırasıyla temizle. C'de bu bizim sorumluluğumuzda,
    // hiçbir destructor bunu kendiliğinden yapmaz.
    for (int i = 0; i < LAYER_COUNT; i++) {
        SDL_DestroyTexture(layers[i].tex);
    }
    for (int i = 0; i < TILE_TYPE_COUNT; i++) {
        if (tile_textures[i]) SDL_DestroyTexture(tile_textures[i]);  // NULL olanları atla
    }

    for (int i = 0; i < MOB_TYPE_COUNT; i++) {
        if (mob_textures[i]) SDL_DestroyTexture(mob_textures[i]);  // NULL olanları atla
    }

    item_textures_free(&itemTex);
    font_quit(gameFont);

    if (crateTex) 
        SDL_DestroyTexture(crateTex);
    SDL_DestroyRenderer(renderer); // Renderer'ı temizle
    SDL_DestroyWindow(window);     // Pencereyi temizle
    SDL_Quit();                    // SDL'i temizle
    return 0;                      // Başarılı çık
}