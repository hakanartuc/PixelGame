#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h> // platformlar arası 'main' köprülemesi (mobil/web için de gerekli)
#include <stdbool.h>
#include "font.h"
#include "common.h"
#include "menu.h"
#include "floattext.h"
#include "tilemap.h"
#include "player.h"
#include "mob.h"
#include "render.h"
#include "loot.h"
#include "inventory.h"
#include "equipment.h"
#include "item_textures.h"
#include "fx.h"
#include "skill.h"
#include "combat.h"
#include "save.h"
#include "hotbar.h"

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;   // SDL_main bu imzayı şart koşar, argüman kullanmıyoruz
    // 1) SDL'in video alt sistemini ayağa kaldır. SDL3 başarılıysa 'true' döner.
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init basarısız: %s", SDL_GetError());
        return 1; // Başarısızsa çık
    }

    if (!font_init())
    {
        // hata — ama oyunu çökertme, font olmadan devam edilebilir
        SDL_Log("Font sistemi baslatilamadi");
    }
    TTF_Font *gameFont = font_load("assets/fonts/PublicPixel-rv0pA.ttf", 16.0f);
    if (gameFont == NULL)
    {
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
    if (crateTex == NULL)
    {
        SDL_Log("Kasa texture yuklenemedi");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture *itemIconTex = load_texture(renderer, "assets/icons/items/tool_icons.png");
    if (itemIconTex == NULL)
    {
        SDL_Log("Item texture yuklenemedi");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    // (yolu kendi konumuna göre ayarla)

    Loot loots[MAX_LOOT] = {0}; // hepsi inactive başlar
    int openLootIndex = -1;
    Inventory inventory = {0}; // tüm slotlar boş başlar (main'de, oyuncuyla birlikte)

    Equipment equipment = {0}; // tüm slotlar boş başlar

    ItemTextures itemTex;
    item_textures_load(&itemTex, renderer);

    // base → feet (ayakkabı) → legs (pantolon) → chest (gömlek) → hands (eller) → hair (saç)
    // Katmanlar ÇİZİM SIRASINDA (alttan üste). Yeni parça eklemek = buraya bir satır.
    // base katmanlar: body → head → face. { texture, z } — z = LPC dosya öneki (çizim sırası, küçük=arka).
    PlayerLayer layers[] = {
        {load_texture(renderer, "assets/characters/body/male/light.png"), 10, -1, false},
        {load_texture(renderer, "assets/characters/head/male/human_light.png"), 100, -1, false},
        {load_texture(renderer, "assets/characters/face/male/neutral_light.png"), 101, -1, false},
    };

    SDL_Texture *glowTex = create_glow_texture(renderer, 64);
    if (glowTex == NULL) {
        SDL_Log("Glow texture olusturulamadi");
        // kritik degil, devam (glow olmadan)
    }

#define LAYER_COUNT ((int)(sizeof(layers) / sizeof(layers[0])))

    for (int i = 0; i < LAYER_COUNT; i++)
        if (!layers[i].tex)
            SDL_Log("Katman yuklenemedi (idx %d)", i);

    // Oyuncuyu oluştur + başlangıç durumunu ayarla
    Player player = {
        .rect = {.x = SPAWN_X, .y = SPAWN_Y, .w = 48.0f, .h = 64.0f},
        .drawW = 64.0f,
        .drawH = 64.0f,
        .facing = DIR_DOWN, // başlangıç yönü
        .state = PLAYER_IDLE,
        .currentFrame = 0,
        .animTimer = 0.0f,
        .lastState = -1, // ilk karede geçiş tetiklensin
        .lastFacing = -1,
        .speed = PLAYER_SPEED,
        .health = 100.0f,
        .maxHealth = 100.0f,
        .strength = 10,    // sabit taban — dağıtım yok, ileride ekipman/perk ekler
        .dexterity = 10,
        .gold = 50,        // başlangıç parası (vendor için)
        // weaponFx her frame kusanili silahtan senkronlanir (bkz. player_update oncesi)
    };
    skills_init(player.skills);
    player.pendingAbility = -1;   // -1 = normal vuruş (0 geçerli ability olurdu)

    // Hotbar: açık ability'ler otomatik yerleşir (Power Strike lv1'de açık)
    Hotbar hotbar = {0};
    hotbar_place_unlocks(&hotbar, player.skills);

    // Başlangıç seti: tam demir set + kılıç KUŞANILI (+0), çantada 3 iksir
    equipment.slots[EQUIP_WEAPON] = (ItemStack){ &ITEM_TYPES[ITEM_LONG_SWORD], 1, 0 };
    equipment.slots[EQUIP_HELMET] = (ItemStack){ &ITEM_TYPES[ITEM_HELMET_IRON], 1, 0 };
    equipment.slots[EQUIP_CHEST]  = (ItemStack){ &ITEM_TYPES[ITEM_CHEST_IRON], 1, 0 };
    equipment.slots[EQUIP_LEGS]   = (ItemStack){ &ITEM_TYPES[ITEM_LEGS_IRON], 1, 0 };
    inventory_add(&inventory, &ITEM_TYPES[ITEM_POTION_HP], 3, 0);
        

    ParticleSystem weaponFx;
    fx_init(&weaponFx);
    int fxPreview = -1; // -1 = kapali; F ile efekt onizleme (silahin efektini gecici ezer, test)

    // Mob texture'ları: her tür için sprite yükle (TILE_TYPES gibi)
    SDL_Texture *mob_textures[MAX_MOB_TYPES] = {0};
    for (int i = 0; i < MOB_TYPE_COUNT; i++)
    {
        mob_textures[i] = load_texture(renderer, MOB_TYPES[i].texturePath);
        if (mob_textures[i] == NULL)
        {
            SDL_Log("Mob texture yuklenemedi: %s", MOB_TYPES[i].texturePath);
            for (int j = 0; j < LAYER_COUNT; j++)
                if (mob_textures[j])
                    SDL_DestroyTexture(mob_textures[j]); // sadece yüklenmiş olanları yık
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }
    }

    FloatText floatTexts[MAX_FLOAT_TEXTS] = {0};

    Mob mobs[MAX_MOBS] = {0}; // hepsi sıfır başlar
    int mobCount = MAX_MOBS;

    int nextMobId = 1;

    // Spawn bölgeleri — KO'daki "kamp"lar: bir alanda birden çok mob.
    SpawnZone spawnZones[] = {
        {.x = 1000.0f, .y = 300.0f, .radius = 200.0f, .type = &MOB_TYPES[0], .maxCount = 1, .respawnTimer = 0.0f}, // goblin kampı (5 mob)
        {.x = 1500.0f, .y = 700.0f, .radius = 150.0f, .type = &MOB_TYPES[1], .maxCount = 1, .respawnTimer = 0.0f}, // ork kampı (3 mob)
    };
    int spawnZoneCount = (int)(sizeof(spawnZones) / sizeof(spawnZones[0]));
    spawn_zones_fill(spawnZones, spawnZoneCount, mobs, MAX_MOBS, &nextMobId); // ← başlangıçta doldur

    // Tile texture'ları: TILE_TYPES tablosundaki path'lerden yükle.
    // texturePath == NULL olanlar (örn. taş) texture'sız kalır → render'da renk basılır.
    SDL_Texture *tile_textures[MAX_TILE_TYPES] = {0}; // TILE_TYPE_COUNT yerine MAX_TILE_TYPES
    for (int i = 0; i < TILE_TYPE_COUNT; i++)
    {
        if (TILE_TYPES[i].texturePath != NULL)
        {
            tile_textures[i] = load_texture(renderer, TILE_TYPES[i].texturePath);
            // not: yüklenemezse NULL kalır, render renk fallback'ine düşer — oyunu çökertme
        }
    }

    bool inventoryOpen = false;
    bool skillsOpen = false;
    GameState gameState = GAME_PLAYING;
    MenuScreen menuScreen = MENU_MAIN;

    // 4) Oyun döngüsü
    bool running = true;

    generate_map(); // haritayı oluştur
    Camera camera = {.x = 0.0f, .y = 0.0f, .zoom = 2.0f};

    Uint64 last = SDL_GetTicks();

    while (running)
    {
        bool attackPressed = false;
        bool targetPressed = false;
        int hotbarPressed = -1;   // bu kare basılan hotbar tuşu (0..7, -1 = yok)
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
                    if (gameState == GAME_PAUSED)
                    {
                        // Menu acikken ESC: onaydaysan ana menuye, ana menudeysen oyuna don
                        if (menuScreen == MENU_CONFIRM_QUIT)
                            menuScreen = MENU_MAIN;
                        else
                            gameState = GAME_PLAYING;
                    }
                    else if (openLootIndex >= 0)
                    {
                        loots[openLootIndex].state = LOOT_CLOSING; // 1) once loot popup
                        openLootIndex = -1;
                    }
                    else if (inventoryOpen)
                    {
                        inventoryOpen = false; // 2) sonra envanter
                    }
                    else if (skillsOpen)
                    {
                        skillsOpen = false; // 3) sonra skill paneli
                    }
                    else
                    {
                        gameState = GAME_PAUSED; // 4) hicbiri yoksa MENU AC
                        menuScreen = MENU_MAIN;
                    }
                }
                else if (event.key.key == SDLK_SPACE && gameState == GAME_PLAYING)
                {
                    // KO modeli: Space = auto-attack aç/kapa (hedef yoksa en yakını seç)
                    if (player.autoAttack)
                    {
                        player.autoAttack = false;
                    }
                    else
                    {
                        if (player.targetMobId == 0)
                        {
                            float px = player.rect.x + player.rect.w / 2.0f;
                            float py = player.rect.y + player.rect.h / 2.0f;
                            player.targetMobId = mob_find_nearest(mobs, mobCount, px, py);
                        }
                        if (player.targetMobId != 0)
                        {
                            player.autoAttack = true;
                            player.attackTimer = 0.0f;   // ilk vuruş beklemesin
                        }
                    }
                }
                else if (event.key.key >= SDLK_1 && event.key.key <= SDLK_8 &&
                         gameState == GAME_PLAYING)
                {
                    hotbarPressed = (int)(event.key.key - SDLK_1);   // '1' → slot 0
                }
                else if (event.key.key == SDLK_Z && gameState == GAME_PLAYING)
                {
                    targetPressed = true;
                }
                else if (event.key.key == SDLK_I && gameState == GAME_PLAYING)
                {
                    inventoryOpen = !inventoryOpen;
                }
                else if (event.key.key == SDLK_K && gameState == GAME_PLAYING)
                {
                    skillsOpen = !skillsOpen;
                }
                else if (event.key.key == SDLK_F && gameState == GAME_PLAYING)
                {
                    // Efekt onizleme dongusu (test): kapali → NONE → PINK → ... → kapali
                    fxPreview++;
                    if (fxPreview >= WEAPON_FX_COUNT)
                        fxPreview = -1;
                }
                else if (event.key.key == SDLK_F5 && gameState == GAME_PLAYING)
                {
                    // Hızlı kayıt
                    const char *msg = save_game(SAVE_PATH, &player, &inventory, &equipment, &hotbar)
                                    ? "Kaydedildi" : "Kayit HATASI";
                    SDL_Color white = { 255, 255, 255, 255 };
                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                         player.rect.x + player.rect.w / 2.0f,
                                         player.rect.y - 10.0f, msg, white);
                }
                else if (event.key.key == SDLK_F9 && gameState == GAME_PLAYING)
                {
                    // Hızlı yükleme (dünya durumu değişmez, sadece oyuncu+eşyalar)
                    if (load_game(SAVE_PATH, &player, &inventory, &equipment, &hotbar))
                    {
                        SDL_Color white = { 255, 255, 255, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f, "Yuklendi", white);
                    }
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                // MENU ACIKSA: tiklama oyuna degil menuye gider
                if (gameState == GAME_PAUSED)
                {
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                        int mw, mh;
                        SDL_GetWindowSize(window, &mw, &mh);
                        MenuAction act = menu_handle_click(&menuScreen,
                                                           (float)event.button.x, (float)event.button.y, mw, mh);
                        if (act == MENU_ACTION_RESUME)
                            gameState = GAME_PLAYING;
                        else if (act == MENU_ACTION_QUIT)
                            running = false;
                    }
                    // menu acikken oyun tiklamasi islenmez
                }
                else if (event.button.button == SDL_BUTTON_LEFT)
                {
                    float worldX = event.button.x / camera.zoom + camera.x;
                    float worldY = event.button.y / camera.zoom + camera.y;

                    bool handled = false;

                    // 1) Popup açıksa: item'e mi, yoksa dışarı mı tıklandı?
                    if (openLootIndex >= 0)
                    {
                        int itemIdx = loot_popup_item_at(&loots[openLootIndex], worldX, worldY);
                        if (itemIdx == LOOT_CELL_GOLD)
                        {
                            // Altın hücresi: al + bildir
                            Loot *l = &loots[openLootIndex];
                            player.gold += l->gold;
                            char buf[FLOAT_TEXT_LEN];
                            snprintf(buf, sizeof(buf), "+%d Gold", l->gold);
                            SDL_Color goldCol = { 255, 210, 60, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 l->rect.x + l->rect.w / 2.0f,
                                                 l->rect.y - 20.0f, buf, goldCol);
                            l->gold = 0;
                            if (loot_is_empty(l))
                            {
                                l->active = false;   // boş kasa kaybolur
                                openLootIndex = -1;
                            }
                            handled = true;
                        }
                        else if (itemIdx >= 0)
                        {
                            // Item al
                            ItemStack *stack = &loots[openLootIndex].items[itemIdx];
                            if (stack->type != NULL)
                            {
                                if (inventory_add(&inventory, stack->type, stack->count,
                                                  stack->plusLevel))
                                {
                                    stack->type = NULL;
                                    stack->count = 0;
                                    stack->plusLevel = 0;
                                    if (loot_is_empty(&loots[openLootIndex]))
                                    {
                                        loots[openLootIndex].active = false; // boş kasa KAYBOLUR
                                        openLootIndex = -1;
                                    }
                                }
                            }
                            handled = true;
                        }
                        else
                        {
                            // Popup açık ama item'e tıklanmadı → dışarı tıklama
                            // Kasayı kapat + popup'ı kapat
                            loots[openLootIndex].state = LOOT_CLOSING;
                            openLootIndex = -1;
                            handled = true;
                        }
                    }

                    // 2) Popup kapalıysa: kasa açma
                    if (!handled)
                    {
                        int li = loot_find_at(loots, MAX_LOOT, worldX, worldY);
                        if (li >= 0 && loots[li].state == LOOT_CLOSED)
                        {
                            loots[li].state = LOOT_OPENING;
                            loots[li].currentFrame = 0;
                            loots[li].animTimer = 0.0f;
                            openLootIndex = li;
                            // (altın popup'ta hücre olarak durur, tıklayınca alınır)
                        }
                    }
                }
                else if (event.button.button == SDL_BUTTON_RIGHT)
                {
                    if (inventoryOpen)
                    {
                        int winW, winH;
                        SDL_GetWindowSize(window, &winW, &winH);   // ← EKLE
                        int panelX = (winW - 400) / 2;

                        // 1) Once ekipman paneline mi tiklandi? (panelY = 40)
                        EquipSlot eqSlot = equipment_slot_at((float)event.button.x,
                                                             (float)event.button.y, panelX, 40);
                        if (eqSlot != EQUIP_NONE)
                        {
                            unequip_slot(&equipment, &inventory, eqSlot); // cikar → envantere
                        }
                        else
                        {
                            // 2) Degilse envantere mi? (panelY = 40+340)
                            int slot = inventory_slot_at((float)event.button.x, (float)event.button.y,
                                                         (float)panelX, (float)(40 + 340));
                            if (slot >= 0)
                            {
                                ItemStack *st = &inventory.slots[slot];
                                if (st->type != NULL && st->type->consumable)
                                {
                                    // Tüketilebilir: hotbar'a ata (içmek hotbar tuşuyla)
                                    hotbar_assign_item(&hotbar, st->type);
                                }
                                else
                                {
                                    equip_from_inventory(&equipment, &inventory, slot);
                                }
                            }
                        }
                    }
                }
            }
        }

        // 4b) UPDATE: şimdilik boş, ileride oyun durumunu güncellemek için kullanacağız.
        // --- ZAMAN: bu karenin süresini ölç (delta-time) ---
        Uint64 now = SDL_GetTicks();
        float dt = (now - last) / 1000.0f; // ms'yi saniyeye çevir; 1000.0f (float!) yoksa tamsayı bölme dt'yi 0 yapar
        last = now;                        // pencereyi bu kareye kaydır; unutulursa dt sürekli büyür

        // dt SPIKE koruması: pencere sürükleme/askıya alma sonrası dev bir dt gelirse
        // kutuyu ışınlamasın diye bir tavanda kes. 0.05 sn = 20 FPS'lik bir kareye denk;
        // bundan yavaş giden her kare 0.05 olarak ele alınır.
        if (dt > 0.05f)
            dt = 0.05f;

        int winW, winH;
        SDL_GetWindowSize(window, &winW, &winH); // pencere boyutunu al (dinamik olarak, pencere yeniden boyutlandırılabilir)

        if (gameState == GAME_PLAYING)
        {

            const bool *keys = SDL_GetKeyboardState(NULL); // basılı tuşların anlık görüntüsü (SDL'e ait, free etme)

            // --- COOLDOWN'LAR: ability başına geri sayım ---
            for (int a = 0; a < ABILITY_COUNT; a++)
                if (player.abilityCd[a] > 0.0f)
                    player.abilityCd[a] -= dt;

            // --- HOTBAR KULLANIMI (1-8) ---
            if (hotbarPressed >= 0)
            {
                HotbarSlot *hs = &hotbar.slots[hotbarPressed];
                if (hs->kind == HOTBAR_SKILL)
                {
                    // Skill vuruşu: CD bitti + şu an savurmuyor → bu vuruşu skill yap.
                    // (Menzil/hedef doğrulaması aşağıdaki ortak blokta; geçemezse iptal olur)
                    if (player.abilityCd[hs->abilityId] <= 0.0f && !player.attacking)
                    {
                        attackPressed = true;
                        player.pendingAbility = (int)hs->abilityId;
                    }
                }
                else if (hs->kind == HOTBAR_ITEM)
                {
                    // İksir: can fulse harcama, envanterde yoksa sessiz geç
                    if (player.health < player.maxHealth &&
                        inventory_consume(&inventory, hs->itemType))
                    {
                        player.health += hs->itemType->healAmount;
                        if (player.health > player.maxHealth)
                            player.health = player.maxHealth;

                        char buf[FLOAT_TEXT_LEN];
                        snprintf(buf, sizeof(buf), "+%d HP", (int)hs->itemType->healAmount);
                        SDL_Color green = { 120, 255, 120, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f, buf, green);
                    }
                }
            }

            // --- AUTO-ATTACK: hedef seçiliyken silah hızında normal vuruş ---
            if (player.autoAttack)
            {
                Mob *aaTarget = mob_find_by_id(mobs, mobCount, player.targetMobId);
                if (aaTarget == NULL)
                {
                    player.autoAttack = false;   // hedef öldü/yok → dur
                }
                else
                {
                    player.attackTimer -= dt;
                    // Menzil dışıysa zamanlayıcı 0'da bekler, menzile girince hemen vurur
                    if (player.attackTimer <= 0.0f && !player.attacking && !attackPressed)
                    {
                        float adx = (aaTarget->e.rect.x + aaTarget->e.rect.w / 2.0f) - (player.rect.x + player.rect.w / 2.0f);
                        float ady = (aaTarget->e.rect.y + aaTarget->e.rect.h / 2.0f) - (player.rect.y + player.rect.h / 2.0f);
                        if (SDL_sqrtf(adx * adx + ady * ady) <= PLAYER_ATTACK_RANGE)
                        {
                            attackPressed = true;
                            player.pendingAbility = -1;   // normal vuruş
                            player.attackTimer = combat_attack_interval(&equipment.slots[EQUIP_WEAPON]);
                        }
                    }
                }
            }

            // --- SALDIRI GEÇERLİLİĞİ (KO tarzı): hedef yoksa ya da menzil dışıysa saldırma ---
            if (attackPressed)
            {
                Mob *target = mob_find_by_id(mobs, mobCount, player.targetMobId);
                if (target == NULL)
                {
                    // Hedef yok → saldırma
                    attackPressed = false;
                }
                else
                {
                    // Hedef var → menzilde mi?
                    float tdx = (target->e.rect.x + target->e.rect.w / 2.0f) - (player.rect.x + player.rect.w / 2.0f);
                    float tdy = (target->e.rect.y + target->e.rect.h / 2.0f) - (player.rect.y + player.rect.h / 2.0f);
                    float tdist = SDL_sqrtf(tdx * tdx + tdy * tdy);

                    if (tdist > PLAYER_ATTACK_RANGE)
                    {
                        // Menzil dışı → saldırma (boşa savurma yok)
                        attackPressed = false;
                    }
                    else
                    {
                        // Geçerli saldırı → oyuncuyu hedefe döndür
                        if (SDL_fabsf(tdx) > SDL_fabsf(tdy))
                        {
                            player.facing = (tdx < 0) ? DIR_LEFT : DIR_RIGHT;
                        }
                        else
                        {
                            player.facing = (tdy < 0) ? DIR_UP : DIR_DOWN;
                        }

                        // Skill vuruşu kesinleşti → cooldown ŞİMDİ başlar
                        // (vuruş savrulur; hedef kaçarsa da CD yanar, KO gibi)
                        if (player.pendingAbility >= 0)
                            player.abilityCd[player.pendingAbility] =
                                ABILITIES[player.pendingAbility].cooldown;
                    }
                }
            }
            // Vuruş başlamadı ve savurma da yok → bekleyen skill çarpanını temizle.
            // (Savurma sürerken DOKUNMA: çarpan vuruş karesinde kullanılacak)
            if (!attackPressed && !player.attacking)
                player.pendingAbility = -1;

            // Partikul efekti kusanili silahtan: silah yoksa efekt yok.
            // fxPreview aktifse silahin efektini gecici ezer (yine silah sarti var).
            const ItemType *wpnType = equipment.slots[EQUIP_WEAPON].type;
            player.weaponFx = (wpnType == NULL) ? WEAPON_FX_NONE
                            : (fxPreview >= 0) ? (WeaponFxType)fxPreview
                            : wpnType->weaponFx;

            player_update(&player, keys, attackPressed, dt, &weaponFx); // oyuncuyu güncelle
            fx_update(&weaponFx, dt);

            if (targetPressed)
            {
                float px = player.rect.x + player.rect.w / 2.0f;
                float py = player.rect.y + player.rect.h / 2.0f;
                player.targetMobId = mob_find_nearest(mobs, mobCount, px, py);

                // Hedef seçildiyse oyuncuyu ona döndür (KO tarzı)
                if (player.targetMobId != 0)
                {
                    Mob *target = mob_find_by_id(mobs, mobCount, player.targetMobId);
                    if (target != NULL)
                    {
                        float tdx = (target->e.rect.x + target->e.rect.w / 2.0f) - px;
                        float tdy = (target->e.rect.y + target->e.rect.h / 2.0f) - py;
                        if (SDL_fabsf(tdx) > SDL_fabsf(tdy))
                        {
                            player.facing = (tdx < 0) ? DIR_LEFT : DIR_RIGHT;
                        }
                        else
                        {
                            player.facing = (tdy < 0) ? DIR_UP : DIR_DOWN;
                        }
                    }
                }
            }

            // --- HEDEF GEÇERLİLİĞİ: seçili mob öldüyse/yoksa hedefi temizle ---
            if (player.targetMobId != 0)
            {
                Mob *target = mob_find_by_id(mobs, mobCount, player.targetMobId);
                if (target == NULL)
                {
                    player.targetMobId = 0; // hedef artık yok → temizle
                }
            }

            // --- SALDIRI HASARI: vuruş karesinde, hedef hâlâ menzildeyse ---
            if (player.attacking && !player.attackHit && player.currentFrame == PLAYER_ATTACK_HIT_FRAME)
            {
                Mob *target = mob_find_by_id(mobs, mobCount, player.targetMobId);
                if (target != NULL)
                {
                    float tdx = (target->e.rect.x + target->e.rect.w / 2.0f) - (player.rect.x + player.rect.w / 2.0f);
                    float tdy = (target->e.rect.y + target->e.rect.h / 2.0f) - (player.rect.y + player.rect.h / 2.0f);
                    float tdist = SDL_sqrtf(tdx * tdx + tdy * tdy);

                    if (tdist <= PLAYER_ATTACK_RANGE)
                    {
                        // Combat A: hasar silah instance'ından (plus dahil) + stat + skill.
                        // Bu vuruş bir ability ise çarpanı uygula (PS ×2, QS ×0.7)
                        float abilityScale = (player.pendingAbility >= 0)
                                           ? ABILITIES[player.pendingAbility].damageScale
                                           : 1.0f;
                        const ItemStack *weapon = &equipment.slots[EQUIP_WEAPON];
                        HitResult hit = combat_player_hit(&player, weapon,
                                                          target->type->defense,
                                                          abilityScale);

                        target->e.health -= hit.damage;
                        target->e.hitFlash = 0.15f;

                        floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                        target->e.rect.x + target->e.rect.w / 2.0f + 10.0f,
                                        target->e.rect.y, (int)hit.damage, hit.crit);

                        // Use-based XP: hasar GERÇEKTEN uygulandığında (havayı kesmek saymaz)
                        SkillType sk = combat_weapon_skill(weapon->type);
                        if (sk != SKILL_NONE)
                        {
                            float xp = combat_xp_for_hit(hit.damage, target->type->level,
                                                         player.skills[sk].level);
                            if (skill_add_xp(player.skills, sk, xp))
                            {
                                // Seviye atlandı — oyuncunun üstünde bildirim
                                char buf[FLOAT_TEXT_LEN];
                                snprintf(buf, sizeof(buf), "%s %d!",
                                         skill_name(sk), player.skills[sk].level);
                                SDL_Color green = { 120, 255, 120, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f, buf, green);

                                // Yeni seviye ability açtıysa hotbar'a otomatik koy
                                hotbar_place_unlocks(&hotbar, player.skills);
                            }
                        }

                        float kb = 180.0f;
                        if (tdist > 0.0f)
                        {
                            target->e.knockbackX = (tdx / tdist) * kb;
                            target->e.knockbackY = (tdy / tdist) * kb;
                        }
                        if (target->e.health <= 0.0f)
                        {
                            // Anında kaybolma YERİNE ölüm animasyonu başlat
                            target->e.alive = false; // AI/hedef dışı
                            target->dying = true;    // death animasyonu başlasın
                            target->state = MOB_DYING;
                            target->currentFrame = 0;
                            target->animTimer = 0.0f;

                            // Ganimet düşür (mob'un konumunda, türün loot tablosundan)
                            loot_spawn(loots, MAX_LOOT, target->e.rect.x, target->e.rect.y,
                                       target->type->lootTable, target->type->lootTableCount,
                                       target->type->goldMin, target->type->goldMax);
                        }
                    }
                }
                player.attackHit = true;      // bu saldırıda denendi (tekrar etmesin)
                player.pendingAbility = -1;   // skill çarpanı kullanıldı/harcandı
            }

            spawn_zones_update(spawnZones, spawnZoneCount, mobs, MAX_MOBS, &nextMobId, dt);

            float px = player.rect.x + player.rect.w / 2.0f;
            float py = player.rect.y + player.rect.h / 2.0f;
            for (int i = 0; i < mobCount; i++)
            {
                mob_update(&mobs[i], mobs, mobCount, px, py, dt);

                // Düşman bu kare saldırdıysa ve oyuncu menzildeyse, hasarı BURADA uygula
                if (mobs[i].attackTriggered)
                {
                    // Mob hasarı oyuncunun zırh defansından geçer (azalan getiri)
                    player.health -= combat_mob_damage(mobs[i].type->damage, &equipment);
                    if (player.health < 0.0f)
                        player.health = 0.0f;
                }
            }

            loot_update_all(loots, MAX_LOOT, dt);

            floattext_update_all(floatTexts, MAX_FLOAT_TEXTS, dt);

            camera_update(&camera, player.rect.x + player.rect.w / 2.0f,
                          player.rect.y + player.rect.h / 2.0f, winW, winH); // kamerayı güncelle - karakteri takip et
        }

        SDL_SetRenderDrawColor(renderer, 155, 155, 155, 255); // arka plan rengini gri yap
        SDL_RenderClear(renderer);                            // ekranı temizle (arka plan rengini doldur)
        tilemap_render(renderer, tile_textures, &camera);     // haritayı çiz

        PlayerLayer built[24];
        int builtCount = build_player_layers(layers, LAYER_COUNT, &equipment,
                                             &itemTex, built, 24);

        player_render(&player, renderer, built, builtCount, &camera); // oyuncuyu
        
        fx_render(&weaponFx, renderer, glowTex, &camera);

        for (int i = 0; i < mobCount; i++)
            mob_render(&mobs[i], renderer, mob_textures, &camera, player.targetMobId);

        if (openLootIndex >= 0)
            loot_popup_render(renderer, &loots[openLootIndex], itemIconTex, &itemTex, gameFont, &camera);

        loot_render_all(loots, MAX_LOOT, renderer, crateTex, &camera); // ← kasalar (mob'lardan sonra, HUD'dan önce)

        // Hasar yazıları (mob'ların üstünde, dünya uzayında)
        floattext_render_all(floatTexts, MAX_FLOAT_TEXTS, renderer, gameFont, &camera); // ← BURAYA (present'tan önce)

        // Hedef HP bar (seçili mob varsa)
        Mob *hpTarget = mob_find_by_id(mobs, mobCount, player.targetMobId);
        target_hp_bar_render(renderer, hpTarget, winW, gameFont);

        health_bar_render(&player, renderer);

        // Altın göstergesi (HP barının altı)
        if (gameFont != NULL)
        {
            char goldBuf[32];
            snprintf(goldBuf, sizeof(goldBuf), "Gold %d", player.gold);
            SDL_Color goldCol = { 255, 210, 60, 255 };
            draw_text(renderer, gameFont, goldBuf, 20.0f, 48.0f, goldCol);
        }

        // Hotbar (alt-orta): skill CD'leri + pot adetleri
        hotbar_render(&hotbar, renderer, itemIconTex, &itemTex, gameFont,
                      player.abilityCd, &inventory, winW, winH);
        

        if (inventoryOpen)
        {
            int panelX = (winW - 400) / 2;
            int panelY = 40;
            equipment_render(&equipment, renderer, itemIconTex, &itemTex,
                             built, builtCount, panelX, panelY);
            inventory_render(&inventory, renderer, itemIconTex, &itemTex, gameFont,
                             (float)panelX, (float)(panelY + 340));
        }

        if (skillsOpen)
        {
            // Envanter paneliyle çakışmasın diye sol tarafta
            skill_panel_render(player.skills, renderer, gameFont, 40.0f, 40.0f);
        }

        if (gameState == GAME_PAUSED)                          // ← EKLE
            menu_render(renderer, gameFont, menuScreen, winW, winH);

        SDL_RenderPresent(renderer); // Ekrana bas.
    }

    // 5) Her şeyi oluşturmanın TERS sırasıyla temizle. C'de bu bizim sorumluluğumuzda,
    // hiçbir destructor bunu kendiliğinden yapmaz.
    for (int i = 0; i < LAYER_COUNT; i++)
    {
        SDL_DestroyTexture(layers[i].tex);
    }
    for (int i = 0; i < TILE_TYPE_COUNT; i++)
    {
        if (tile_textures[i])
            SDL_DestroyTexture(tile_textures[i]); // NULL olanları atla
    }

    for (int i = 0; i < MOB_TYPE_COUNT; i++)
    {
        if (mob_textures[i])
            SDL_DestroyTexture(mob_textures[i]); // NULL olanları atla
    }

        
    if (glowTex)
        SDL_DestroyTexture(glowTex);

    item_textures_free(&itemTex);
    font_quit(gameFont);

    if (crateTex)
        SDL_DestroyTexture(crateTex);
    SDL_DestroyRenderer(renderer); // Renderer'ı temizle
    SDL_DestroyWindow(window);     // Pencereyi temizle
    SDL_Quit();                    // SDL'i temizle
    return 0;                      // Başarılı çık
}