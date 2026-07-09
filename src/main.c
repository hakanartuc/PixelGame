#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h> // platformlar arası 'main' köprülemesi (mobil/web için de gerekli)
#include <stdbool.h>
#include <string.h>   // diyalog hedef karşılaştırmaları
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
#include "region.h"
#include "npc.h"
#include "dialog.h"
#include "shop.h"
#include "quest.h"
#include "gametime.h"
#include "decor.h"
#include "audio.h"
#include "music.h"
#include "minimap.h"
#include "ui_theme.h"
#include "perk.h"

#define BASE_MAX_HP 100.0f   // oyuncu taban canı (perk bonusları üstüne biner)

// Mob ölümü — saldırıdan da kanamadan da aynı yol: animasyon + loot + görev.
// (İki yerden çağrılır; kopya kod desenkron olmasın diye tek fonksiyon)
static void mob_die(Mob *target, Loot *loots, QuestStatus *questStatus,
                    FloatText *floatTexts, const Player *player)
{
    target->e.alive = false;
    target->dying = true;
    target->state = MOB_DYING;
    target->currentFrame = 0;
    target->animTimer = 0.0f;

    loot_spawn(loots, MAX_LOOT, target->e.rect.x, target->e.rect.y,
               target->type->lootTable, target->type->lootTableCount,
               target->type->goldMin, target->type->goldMax);

    int qDone = quest_on_kill(questStatus, target->type);
    for (int q = 0; q < QUEST_COUNT; q++)
    {
        if (QUESTS[q].targetMob != target->type) continue;
        if (questStatus[q].state != QUEST_ACTIVE) continue;
        char buf[FLOAT_TEXT_LEN];
        snprintf(buf, sizeof(buf), "%s %d/%d", QUESTS[q].name,
                 questStatus[q].progress, QUESTS[q].targetCount);
        SDL_Color cyan = { 90, 220, 255, 255 };
        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                             player->rect.x + player->rect.w / 2.0f,
                             player->rect.y - 10.0f, buf, cyan);
    }
    if (qDone >= 0)
    {
        SDL_Color green = { 120, 255, 120, 255 };
        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                             player->rect.x + player->rect.w / 2.0f,
                             player->rect.y - 30.0f,
                             "Gorev tamam! Odul icin don.", green);
    }
}

// Radyal partikül patlaması — perk seçimi, ölüm kurtarışı, crit gibi anlara
// "his" katar. Renk + adet + hız çağırandan; glow'lu fx sistemine basar.
static void fx_burst(ParticleSystem *fx, float cx, float cy, int count,
                     float speed, Uint8 r, Uint8 g, Uint8 b)
{
    for (int k = 0; k < count; k++)
    {
        float ang = randf() * 6.2832f;
        float spd = speed * (0.5f + randf() * 0.5f);
        fx_spawn(fx, cx + (randf() - 0.5f) * 8.0f, cy + (randf() - 0.5f) * 8.0f,
                 SDL_cosf(ang) * spd, SDL_sinf(ang) * spd - speed * 0.2f,
                 0.45f + randf() * 0.4f, 10.0f + randf() * 10.0f, r, g, b,
                 WEAPON_FX_NONE);
    }
}

// Skill seviye atlayınca ortak işler: bildirim + ability + perk hakkı uyarısı
static void on_skill_levelup(Player *player, SkillType sk, Hotbar *hotbar,
                             FloatText *floatTexts)
{
    char buf[FLOAT_TEXT_LEN];
    snprintf(buf, sizeof(buf), "%s %d!", skill_name(sk), player->skills[sk].level);
    SDL_Color green = { 120, 255, 120, 255 };
    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                         player->rect.x + player->rect.w / 2.0f,
                         player->rect.y - 10.0f, buf, green);
    hotbar_place_unlocks(hotbar, player->skills);
    audio_play(SFX_LEVELUP);

    if (perk_pending_tier(sk, player->skills[sk].level, player->perkChoice) >= 0)
    {
        SDL_Color gold = { 255, 210, 60, 255 };
        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                             player->rect.x + player->rect.w / 2.0f,
                             player->rect.y - 30.0f, "Perk hakki! (K)", gold);
    }
}

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
    // NOT: CuteFantasy-5x9.ttf glyph seti eksik çıktı (boşluk bile kutu basıyor) —
    // Public Pixel'e dönüldü. Cute font ancak eksik glyph'ler tamamlanırsa kullanılır.
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

    // 3) SDL_GPU tabanlı kendi renderer'ımız (r2d) — vsync r2d_init içinde
    R2D *renderer = r2d_init(window);
    if (renderer == NULL)
    {
        SDL_Log("r2d_init basarısız");
        SDL_DestroyWindow(window); // Pencereyi temizle
        SDL_Quit();                // SDL'i temizle
        return 1;                  // Başarısızsa çık
    }

    R2DTexture *crateTex = load_texture(renderer, "assets/loot/chest_small.png");
    if (crateTex == NULL)
    {
        SDL_Log("Kasa texture yuklenemedi");
        r2d_quit(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    R2DTexture *itemIconTex = load_texture(renderer, "assets/icons/items/tool_icons.png");
    if (itemIconTex == NULL)
    {
        SDL_Log("Item texture yuklenemedi");
        r2d_quit(renderer);
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

    R2DTexture *glowTex = create_glow_texture(renderer, 64);
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

    // Perk'ler: hepsi seçilmemiş başlar; etkiler nötr hesaplanır
    for (int s = 0; s < SKILL_COUNT; s++)
        for (int t = 0; t < PERK_TIERS; t++)
            player.perkChoice[s][t] = PERK_UNCHOSEN;
    PerkEffects perkFx;
    perk_compute_effects(player.perkChoice, &perkFx);
    int perkPickSkill = -1, perkPickTier = -1;   // açık perk seçim overlay'i

    // Hotbar: açık ability'ler otomatik yerleşir (Power Strike lv1'de açık)
    Hotbar hotbar = {0};
    hotbar_place_unlocks(&hotbar, player.skills);

    // Dekorlar (evler): texture + boyutları (render/collision tek kaynaktan)
    R2DTexture *decor_textures[16] = {0};
    for (int i = 0; i < DECOR_COUNT && i < 16; i++)
        decor_textures[i] = load_texture(renderer, DECORS[i].texPath);
    decor_set_texture_sizes(decor_textures);

    gametime_init();   // gün 1, 08:00
    ui_theme_init(renderer);   // panel/hücre/buton sprite'ları
    audio_init();      // başarısızsa oyun sessiz devam eder
    music_init();      // arka plan müziği (mood'a göre, crossfade)

    // Ölüm akışı: timer > 0 iken dünya donar, ekran kararır; bitince köyde diril
    float deathTimer = 0.0f;
    int deathGoldLost = 0;

    bool questLogOpen = false;
    bool minimapOpen = true;   // M ile kapatılabilir

    // NPC'ler: texture + diyalog dosyaları (yüklenemeyen NPC konuşamaz, oyun çökmez)
    R2DTexture *npc_textures[NPC_COUNT];
    Dialog npcDialogs[NPC_COUNT];
    bool npcDialogOk[NPC_COUNT];
    for (int i = 0; i < NPC_COUNT; i++)
    {
        npc_textures[i] = load_texture(renderer, NPCS[i].sheetPath);
        npcDialogOk[i] = dialog_load(&npcDialogs[i], NPCS[i].dialogFile);
    }
    int dialogNpc = -1;    // konuşulan NPC (-1 = diyalog kapalı)
    int dialogNode = -1;   // aktif diyalog düğümü
    bool shopOpen = false;
    int nearestNpc = -1;   // E prompt'u için (her kare güncellenir)

    QuestStatus questStatus[MAX_QUESTS] = {0};   // hepsi NOT_STARTED

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
    R2DTexture *mob_textures[MAX_MOB_TYPES] = {0};
    for (int i = 0; i < MOB_TYPE_COUNT; i++)
    {
        mob_textures[i] = load_texture(renderer, MOB_TYPES[i].texturePath);
        if (mob_textures[i] == NULL)
        {
            SDL_Log("Mob texture yuklenemedi: %s", MOB_TYPES[i].texturePath);
            for (int j = 0; j < LAYER_COUNT; j++)
                if (mob_textures[j])
                    r2d_texture_destroy(NULL, mob_textures[j]); // sadece yüklenmiş olanları yık
            r2d_quit(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }
    }

    FloatText floatTexts[MAX_FLOAT_TEXTS] = {0};

    Mob mobs[MAX_MOBS] = {0}; // hepsi sıfır başlar
    int mobCount = MAX_MOBS;

    int nextMobId = 1;

    // Spawn bölgeleri BÖLGE tanımlarından kurulur (region.c) — kamplar veride,
    // main sadece düzleştirir. Yeni kamp eklemek = region.c'ye bir satır.
    SpawnZone spawnZones[MAX_SPAWN_ZONES];
    int spawnZoneCount = 0;
    for (int r = 0; r < REGION_COUNT; r++)
    {
        for (int c = 0; c < REGIONS[r].campCount && spawnZoneCount < MAX_SPAWN_ZONES; c++)
        {
            const RegionCamp *camp = &REGIONS[r].camps[c];
            spawnZones[spawnZoneCount++] = (SpawnZone){
                .x = camp->x, .y = camp->y, .radius = camp->radius,
                .type = camp->type, .maxCount = camp->maxCount, .respawnTimer = 0.0f,
            };
        }
    }
    spawn_zones_fill(spawnZones, spawnZoneCount, mobs, MAX_MOBS, &nextMobId); // ← başlangıçta doldur

    // Tile texture'ları: TILE_TYPES tablosundaki path'lerden yükle.
    // texturePath == NULL olanlar (örn. taş) texture'sız kalır → render'da renk basılır.
    R2DTexture *tile_textures[MAX_TILE_TYPES] = {0}; // TILE_TYPE_COUNT yerine MAX_TILE_TYPES
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

    // Harita: Tiled CSV'den; yoksa eski test haritası (generate_map)
    if (!tilemap_load_csv("assets/maps/world.csv"))
        generate_map();
    minimap_init(renderer);   // harita boyutu belli olduktan SONRA

    // Bölge splash durumu: bölge değişince ad 3 sn görünür
    int currentRegion = -1;
    const char *splashText = NULL;
    float splashTimer = 0.0f;
    TTF_Font *splashFont = font_load("assets/fonts/PublicPixel-rv0pA.ttf", 32.0f);
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
                    else if (perkPickSkill >= 0)
                    {
                        perkPickSkill = -1; // 2) perk seçim overlay'i
                        perkPickTier = -1;
                    }
                    else if (dialogNpc >= 0)
                    {
                        dialogNpc = -1; // 3) sonra diyalog
                    }
                    else if (shopOpen)
                    {
                        shopOpen = false; // 3) sonra dukkan
                    }
                    else if (inventoryOpen)
                    {
                        inventoryOpen = false; // 4) sonra envanter
                    }
                    else if (skillsOpen)
                    {
                        skillsOpen = false; // 5) sonra skill paneli
                    }
                    else if (questLogOpen)
                    {
                        questLogOpen = false; // 6) sonra quest log
                    }
                    else
                    {
                        gameState = GAME_PAUSED; // 7) hicbiri yoksa MENU AC
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
                    if (dialogNpc >= 0)
                    {
                        // Diyalog açıkken sayılar SEÇENEK seçer (hotbar devre dışı)
                        Dialog *dlg = &npcDialogs[dialogNpc];
                        DialogNode *node = &dlg->nodes[dialogNode];
                        int sel = (int)(event.key.key - SDLK_1);
                        if (sel < node->optCount)
                        {
                            const char *target = node->opts[sel].target;
                            if (strcmp(target, "end") == 0)
                            {
                                dialogNpc = -1;
                            }
                            else if (strcmp(target, "shop") == 0)
                            {
                                shopOpen = true;
                                inventoryOpen = true;   // satış için çanta da açık
                                dialogNpc = -1;
                            }
                            else if (strncmp(target, "accept:", 7) == 0)
                            {
                                int q = quest_find(target + 7);
                                if (q >= 0 && questStatus[q].state == QUEST_NOT_STARTED)
                                {
                                    questStatus[q].state = QUEST_ACTIVE;
                                    questStatus[q].progress = 0;
                                    char buf[FLOAT_TEXT_LEN];
                                    snprintf(buf, sizeof(buf), "Gorev: %s", QUESTS[q].name);
                                    SDL_Color cyan = { 90, 220, 255, 255 };
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                         player.rect.x + player.rect.w / 2.0f,
                                                         player.rect.y - 10.0f, buf, cyan);
                                }
                                int nx = dialog_find(dlg, "accepted");
                                if (nx >= 0) dialogNode = nx; else dialogNpc = -1;
                            }
                            else if (strncmp(target, "complete:", 9) == 0)
                            {
                                int q = quest_find(target + 9);
                                if (q >= 0 && questStatus[q].state == QUEST_DONE)
                                {
                                    questStatus[q].state = QUEST_REWARDED;
                                    player.gold += QUESTS[q].rewardGold;
                                    char buf[FLOAT_TEXT_LEN];
                                    snprintf(buf, sizeof(buf), "+%d Gold", QUESTS[q].rewardGold);
                                    SDL_Color gold = { 255, 210, 60, 255 };
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                         player.rect.x + player.rect.w / 2.0f,
                                                         player.rect.y - 10.0f, buf, gold);
                                }
                                int nx = dialog_find(dlg, "rewarded");
                                if (nx >= 0) dialogNode = nx; else dialogNpc = -1;
                            }
                            else
                            {
                                // Normal düğüm geçişi
                                int nx = dialog_find(dlg, target);
                                if (nx >= 0) dialogNode = nx;
                                else { SDL_Log("Diyalog hedefi yok: %s", target); dialogNpc = -1; }
                            }
                        }
                    }
                    else
                    {
                        hotbarPressed = (int)(event.key.key - SDLK_1);   // '1' → slot 0
                    }
                }
                else if (event.key.key == SDLK_E && gameState == GAME_PLAYING &&
                         dialogNpc < 0)
                {
                    // Menzildeki NPC ile konuş — giriş düğümü görev durumuna göre
                    if (nearestNpc >= 0 && npcDialogOk[nearestNpc])
                    {
                        const char *entry = "start";
                        if (NPCS[nearestNpc].questId != NULL)
                        {
                            int q = quest_find(NPCS[nearestNpc].questId);
                            if (q >= 0)
                            {
                                switch (questStatus[q].state) {
                                    case QUEST_ACTIVE:   entry = "active";   break;
                                    case QUEST_DONE:     entry = "done";     break;
                                    case QUEST_REWARDED: entry = "rewarded"; break;
                                    default:             entry = "start";    break;
                                }
                            }
                        }
                        dialogNode = dialog_find(&npcDialogs[nearestNpc], entry);
                        if (dialogNode >= 0)
                        {
                            dialogNpc = nearestNpc;
                            player.autoAttack = false;   // konuşurken savaş durur
                        }
                    }
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
                    if (!skillsOpen) { perkPickSkill = -1; perkPickTier = -1; }
                    audio_play(SFX_UI);
                }
                else if (event.key.key == SDLK_J && gameState == GAME_PLAYING)
                {
                    questLogOpen = !questLogOpen;
                    audio_play(SFX_UI);
                }
                else if (event.key.key == SDLK_M && gameState == GAME_PLAYING)
                {
                    minimapOpen = !minimapOpen;
                    audio_play(SFX_UI);
                }
                else if (event.key.key == SDLK_F && gameState == GAME_PLAYING)
                {
                    // Efekt onizleme dongusu (test): kapali → NONE → PINK → ... → kapali
                    fxPreview++;
                    if (fxPreview >= WEAPON_FX_COUNT)
                        fxPreview = -1;
                }
                else if (event.key.key == SDLK_F10 && gameState == GAME_PLAYING)
                {
                    // DEBUG: perk testi için XP pompala (One-Handed + 3 zırh skill'i).
                    // Yayın öncesi kaldırılacak — commit'e girer ama işaretli.
                    // Eğri dik: seviye 25 ≈ 125k, 100 ≈ 4M XP → basış başına 1M
                    // (1 basış ~lv57, 2 ~lv75, 4 = tüm eşikler açık)
                    SkillType dbg[4] = { SKILL_ONE_HANDED, SKILL_LIGHT_ARMOR,
                                         SKILL_MEDIUM_ARMOR, SKILL_HEAVY_ARMOR };
                    for (int d = 0; d < 4; d++)
                        if (skill_add_xp(player.skills, dbg[d], 1000000.0f))
                            on_skill_levelup(&player, dbg[d], &hotbar, floatTexts);
                    SDL_Log("DEBUG F10: +1M XP (OneHanded+zirhlar)");
                }
                else if (event.key.key == SDLK_F5 && gameState == GAME_PLAYING)
                {
                    // Hızlı kayıt
                    const char *msg = save_game(SAVE_PATH, &player, &inventory, &equipment, &hotbar, questStatus)
                                    ? "Kaydedildi" : "Kayit HATASI";
                    SDL_Color white = { 255, 255, 255, 255 };
                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                         player.rect.x + player.rect.w / 2.0f,
                                         player.rect.y - 10.0f, msg, white);
                }
                else if (event.key.key == SDLK_F9 && gameState == GAME_PLAYING)
                {
                    // Hızlı yükleme (dünya durumu değişmez, sadece oyuncu+eşyalar)
                    if (load_game(SAVE_PATH, &player, &inventory, &equipment, &hotbar, questStatus))
                    {
                        perk_compute_effects(player.perkChoice, &perkFx);   // seçimler değişti
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
                        if (act != MENU_ACTION_NONE)
                            audio_play(SFX_UI);
                        if (act == MENU_ACTION_RESUME)
                            gameState = GAME_PLAYING;
                        else if (act == MENU_ACTION_QUIT)
                            running = false;
                        else if (act == MENU_ACTION_SAVE || act == MENU_ACTION_LOAD)
                        {
                            // F5/F9 ile aynı yol; bildirim oyuna dönünce görünür
                            bool ok = (act == MENU_ACTION_SAVE)
                                    ? save_game(SAVE_PATH, &player, &inventory, &equipment, &hotbar, questStatus)
                                    : load_game(SAVE_PATH, &player, &inventory, &equipment, &hotbar, questStatus);
                            if (ok && act == MENU_ACTION_LOAD)
                                perk_compute_effects(player.perkChoice, &perkFx);
                            const char *msg = !ok ? ((act == MENU_ACTION_SAVE) ? "Kayit HATASI" : "Kayit bulunamadi")
                                            : (act == MENU_ACTION_SAVE) ? "Kaydedildi" : "Yuklendi";
                            SDL_Color white = { 255, 255, 255, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f, msg, white);
                            gameState = GAME_PLAYING;   // işlem sonrası oyuna dön
                        }
                    }
                    // menu acikken oyun tiklamasi islenmez
                }
                else if (event.button.button == SDL_BUTTON_LEFT)
                {
                    float worldX = event.button.x / camera.zoom + camera.x;
                    float worldY = event.button.y / camera.zoom + camera.y;

                    bool handled = false;

                    // -1) Perk overlay'i açıksa: seçenek tıklaması (dışarısı kapatır)
                    if (perkPickSkill >= 0)
                    {
                        float ox = 40.0f + SKILL_PANEL_W + 10.0f, oy = 40.0f;
                        int opt = perk_choice_option_at((float)event.button.x,
                                                        (float)event.button.y, ox, oy);
                        if (opt >= 0)
                        {
                            player.perkChoice[perkPickSkill][perkPickTier] = (Sint8)opt;
                            perk_compute_effects(player.perkChoice, &perkFx);

                            // Max HP perk'leri: taban + bonus yeniden hesaplanır
                            float newMax = BASE_MAX_HP + perkFx.maxHpBonus;
                            if (newMax > player.maxHealth)
                                player.health += newMax - player.maxHealth;
                            player.maxHealth = newMax;

                            const PerkOption *po = perk_option((SkillType)perkPickSkill,
                                                               perkPickTier, opt);
                            char buf[FLOAT_TEXT_LEN];
                            snprintf(buf, sizeof(buf), "Perk: %s", po ? po->name : "?");
                            SDL_Color gold = { 255, 210, 60, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f, buf, gold);
                            audio_play(SFX_PERK);   // törensel arpej
                            fx_burst(&weaponFx,
                                     player.rect.x + player.rect.w / 2.0f,
                                     player.rect.y + player.rect.h / 2.0f,
                                     28, 110.0f, 255, 215, 90);   // altın halka
                        }
                        perkPickSkill = -1;
                        perkPickTier = -1;
                        handled = true;
                    }
                    // 0a) Skill paneli açıkken satıra tıklama → bekleyen perk seçimi aç
                    else if (skillsOpen)
                    {
                        int row = skill_panel_row_at((float)event.button.x,
                                                     (float)event.button.y, 40.0f, 40.0f);
                        if (row >= 0)
                        {
                            int tier = perk_pending_tier((SkillType)row,
                                                         player.skills[row].level,
                                                         player.perkChoice);
                            if (tier >= 0)
                            {
                                perkPickSkill = row;
                                perkPickTier = tier;
                                audio_play(SFX_UI);
                            }
                            handled = true;
                        }
                    }

                    // 0) Dükkan açıksa: stok satırına tıklama = satın al (ekran koordinatı)
                    if (shopOpen)
                    {
                        int entry = shop_entry_at((float)event.button.x, (float)event.button.y,
                                                  40.0f, 40.0f);
                        if (entry >= 0)
                        {
                            const ItemType *it = SHOP_STOCK[entry].type;
                            if (player.gold >= it->value &&
                                inventory_add(&inventory, it, 1, 0))
                            {
                                player.gold -= it->value;
                                audio_play(SFX_UI);
                                char buf[FLOAT_TEXT_LEN];
                                snprintf(buf, sizeof(buf), "-%d g", it->value);
                                SDL_Color red = { 255, 140, 120, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f, buf, red);
                            }
                            handled = true;
                        }
                    }

                    // 1) Popup açıksa: item'e mi, yoksa dışarı mı tıklandı?
                    if (!handled && openLootIndex >= 0)
                    {
                        int itemIdx = loot_popup_item_at(&loots[openLootIndex], worldX, worldY);
                        if (itemIdx == LOOT_CELL_GOLD)
                        {
                            // Altın hücresi: al + bildir
                            Loot *l = &loots[openLootIndex];
                            player.gold += l->gold;
                            audio_play(SFX_LOOT);
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
                                    audio_play(SFX_LOOT);
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
                                if (shopOpen && st->type != NULL)
                                {
                                    // Dükkan açıkken sağ tık = SAT (alışın yarısı, plus dahil)
                                    int price = item_value(st) / 2;
                                    player.gold += price;
                                    audio_play(SFX_UI);
                                    st->count--;
                                    if (st->count <= 0)
                                    {
                                        st->type = NULL;
                                        st->plusLevel = 0;
                                    }
                                    char buf[FLOAT_TEXT_LEN];
                                    snprintf(buf, sizeof(buf), "+%d g", price);
                                    SDL_Color gold = { 255, 210, 60, 255 };
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                         player.rect.x + player.rect.w / 2.0f,
                                                         player.rect.y - 10.0f, buf, gold);
                                }
                                else if (st->type != NULL && st->type->consumable)
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

        // ÖLÜM EKRANI: dünya donuk, sadece geri sayım (bitince köyde diril)
        if (deathTimer > 0.0f)
        {
            deathTimer -= dt;
            if (deathTimer <= 0.0f)
            {
                player.rect.x = SPAWN_X;
                player.rect.y = SPAWN_Y;
                player.health = player.maxHealth;
                player.state = PLAYER_IDLE;
                player.attacking = false;
                player.attackHit = false;
                player.targetMobId = 0;
                // (uzakta aggro'lu moblar leash'le kendiliğinden kampa döner)
            }
        }

        if (gameState == GAME_PLAYING && deathTimer <= 0.0f)
        {

            const bool *keys = SDL_GetKeyboardState(NULL); // basılı tuşların anlık görüntüsü (SDL'e ait, free etme)

            // --- COOLDOWN'LAR: ability + item türü başına geri sayım ---
            for (int a = 0; a < ABILITY_COUNT; a++)
                if (player.abilityCd[a] > 0.0f)
                    player.abilityCd[a] -= dt;
            for (int t = 0; t < ITEM_TYPE_COUNT; t++)
                if (player.itemCd[t] > 0.0f)
                    player.itemCd[t] -= dt;

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
                    // İksir: tür CD'deyse / can fulse / envanterde yoksa geç
                    int typeIdx = (int)(hs->itemType - ITEM_TYPES);
                    if (player.itemCd[typeIdx] <= 0.0f &&
                        player.health < player.maxHealth &&
                        inventory_consume(&inventory, hs->itemType))
                    {
                        // Akıcı: CD kısalır; İlk Yardım: iyileşme artar
                        player.itemCd[typeIdx] = hs->itemType->useCooldown * perkFx.potionCdMult;
                        audio_play(SFX_POTION);
                        player.health += hs->itemType->healAmount * perkFx.potionEffMult;
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
                            player.attackTimer = combat_attack_interval(&equipment.slots[EQUIP_WEAPON], &perkFx);
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
                        // Perk: Fırtına PS'yi, Seri Vuruş QS'yi kısaltır
                        if (player.pendingAbility >= 0)
                        {
                            float cdMult = (player.pendingAbility == ABILITY_POWER_STRIKE)
                                         ? perkFx.psCdMult : perkFx.qsCdMult;
                            player.abilityCd[player.pendingAbility] =
                                ABILITIES[player.pendingAbility].cooldown * cdMult;
                        }
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

            gametime_update(dt);                      // dünya saati
            npc_update(dt, gametime_hours());         // NPC rutinleri (yürü/eve gir)

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
                        // Combat A: hasar silah instance'ından (plus dahil) + stat + skill + perk.
                        // Bu vuruş bir ability ise çarpanı uygula (PS ×2, QS ×0.7)
                        float abilityScale = 1.0f;
                        if (player.pendingAbility >= 0)
                        {
                            abilityScale = ABILITIES[player.pendingAbility].damageScale;
                            // Ezici Darbe: PS çarpanını perk ezer
                            if (player.pendingAbility == ABILITY_POWER_STRIKE &&
                                perkFx.psScaleOverride > 0.0f)
                                abilityScale = perkFx.psScaleOverride;
                        }
                        const ItemStack *weapon = &equipment.slots[EQUIP_WEAPON];
                        HitResult hit = combat_player_hit(&player, weapon,
                                                          target->type->defense,
                                                          abilityScale,
                                                          target->e.health / target->e.maxHealth,
                                                          &perkFx);

                        target->e.health -= hit.damage;
                        target->e.hitFlash = 0.15f;
                        audio_play(hit.crit ? SFX_CRIT : SFX_HIT);

                        // Crit: hedefte sarı-beyaz kıvılcım patlaması
                        if (hit.crit)
                            fx_burst(&weaponFx,
                                     target->e.rect.x + target->e.rect.w / 2.0f,
                                     target->e.rect.y + target->e.rect.h / 2.0f,
                                     12, 90.0f, 255, 240, 120);

                        // Kanatma: vuruş hasarının bir kısmı DoT olarak yayılır
                        // (Kasap süreyi uzatır → toplam kanama artar)
                        if (perkFx.bleedFrac > 0.0f)
                        {
                            target->bleedDps = hit.damage * perkFx.bleedFrac / 3.0f;
                            target->bleedTime = 3.0f * perkFx.bleedPower;
                        }

                        // Vurulan mob + kamp arkadaşları alarma geçer (grup aggro)
                        target->aggro = true;
                        mob_alert_camp(mobs, mobCount, target);

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
                                on_skill_levelup(&player, sk, &hotbar, floatTexts);
                        }

                        float kb = 180.0f * perkFx.kbDealtMult;   // Ezici Ağırlık
                        if (tdist > 0.0f)
                        {
                            target->e.knockbackX = (tdx / tdist) * kb;
                            target->e.knockbackY = (tdy / tdist) * kb;
                        }
                        if (target->e.health <= 0.0f)
                            mob_die(target, loots, questStatus, floatTexts, &player);
                    }
                }
                player.attackHit = true;      // bu saldırıda denendi (tekrar etmesin)
                player.pendingAbility = -1;   // skill çarpanı kullanıldı/harcandı
            }

            // Gece tehlikesi: spawn 2x hızlı (dt şişirilir), algı menzili 1.5x
            float nightSpawnMul = gametime_is_night() ? 2.0f : 1.0f;
            float detectMul = (gametime_is_night() ? 1.5f : 1.0f)
                            * perkFx.detectOnPlayerMult;   // Hayalet perki

            // Hız perk'leri: Çevik/Rüzgar/Uyumlu + gece Gölge Adımı
            player.speed = PLAYER_SPEED * perkFx.speedMult
                         * (gametime_is_night() ? perkFx.nightSpeedMult : 1.0f);
            spawn_zones_update(spawnZones, spawnZoneCount, mobs, MAX_MOBS, &nextMobId,
                               dt * nightSpawnMul);

            float px = player.rect.x + player.rect.w / 2.0f;
            float py = player.rect.y + player.rect.h / 2.0f;
            for (int i = 0; i < mobCount; i++)
            {
                mob_update(&mobs[i], mobs, mobCount, px, py, dt, detectMul);

                // Kanama DoT'u (Kanatma perki) — ölüm loot/görev için main'de işlenir
                if (mobs[i].bleedTime > 0.0f && mobs[i].e.alive)
                {
                    mobs[i].bleedTime -= dt;
                    mobs[i].e.health -= mobs[i].bleedDps * dt;

                    // Kan damlaları: mob üstünden yere süzülür (saniyede ~8)
                    if (randf() < dt * 8.0f)
                        fx_spawn(&weaponFx,
                                 mobs[i].e.rect.x + mobs[i].e.rect.w * (0.3f + randf() * 0.4f),
                                 mobs[i].e.rect.y + mobs[i].e.rect.h * 0.4f,
                                 (randf() - 0.5f) * 15.0f, 40.0f + randf() * 30.0f,
                                 0.55f, 8.0f + randf() * 6.0f, 190, 20, 20,
                                 WEAPON_FX_NONE);

                    if (mobs[i].e.health <= 0.0f)
                        mob_die(&mobs[i], loots, questStatus, floatTexts, &player);
                }

                // Düşman bu kare saldırdıysa ve oyuncu menzildeyse, hasarı BURADA uygula
                if (mobs[i].attackTriggered)
                {
                    bool dodged = false;
                    bool yaraliAyi = (player.perkChoice[SKILL_MEDIUM_ARMOR][2] == 1);
                    float dmg = combat_mob_damage(mobs[i].type->damage, &equipment,
                                                  &perkFx,
                                                  player.state == PLAYER_IDLE,
                                                  player.health / player.maxHealth,
                                                  yaraliAyi, &dodged);
                    if (dodged)
                    {
                        SDL_Color cyan = { 90, 220, 255, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             px, player.rect.y - 10.0f, "Kacindin!", cyan);
                        audio_play(SFX_DODGE);
                        fx_burst(&weaponFx, px, py, 10, 120.0f, 235, 245, 255);   // beyaz iz
                    }
                    else if (dmg > 0.0f)
                    {
                        player.health -= dmg;
                        player.gaziTimer = 5.0f;   // Gazi buff penceresi (perk yoksa etkisiz)

                        // Zırh XP'si: giyili her zırh parçasının sınıf skill'i gelişir
                        for (int sl = 0; sl < EQUIP_SLOT_COUNT; sl++)
                        {
                            const ItemType *it = equipment.slots[sl].type;
                            if (it == NULL || it->armorClass == ARMOR_NONE) continue;
                            SkillType asRaw = (it->armorClass == ARMOR_LIGHT)  ? SKILL_LIGHT_ARMOR
                                            : (it->armorClass == ARMOR_MEDIUM) ? SKILL_MEDIUM_ARMOR
                                                                               : SKILL_HEAVY_ARMOR;
                            if (skill_add_xp(player.skills, asRaw, dmg * 0.15f))
                                on_skill_levelup(&player, asRaw, &hotbar, floatTexts);
                        }

                        // Yansıtma: Karşılık (%) + Diken Zırh (sabit)
                        float reflect = dmg * perkFx.reflectFrac + perkFx.reflectFlat;
                        if (reflect > 0.0f && mobs[i].e.alive)
                        {
                            mobs[i].e.health -= reflect;
                            audio_play(SFX_REFLECT);
                            fx_burst(&weaponFx,
                                     mobs[i].e.rect.x + mobs[i].e.rect.w / 2.0f,
                                     mobs[i].e.rect.y + mobs[i].e.rect.h / 2.0f,
                                     6, 70.0f, 255, 160, 60);   // turuncu kıvılcım
                            if (mobs[i].e.health <= 0.0f)
                                mob_die(&mobs[i], loots, questStatus, floatTexts, &player);
                        }

                        // Vuruş oyuncuyu iter (Sağlam Duruş yarılatır, Sarsılmaz sıfırlar)
                        float pdx = px - (mobs[i].e.rect.x + mobs[i].e.rect.w / 2.0f);
                        float pdy = py - (mobs[i].e.rect.y + mobs[i].e.rect.h / 2.0f);
                        float pd = SDL_sqrtf(pdx * pdx + pdy * pdy);
                        if (pd > 0.0f && perkFx.kbTakenMult > 0.0f)
                        {
                            player.knockbackX = pdx / pd * 160.0f * perkFx.kbTakenMult;
                            player.knockbackY = pdy / pd * 160.0f * perkFx.kbTakenMult;
                        }

                        // Ölüm kurtarışı: Yıkılmaz/Ölümsüz — ölümcül darbe 1 HP bırakır
                        if (player.health <= 0.0f &&
                            perkFx.deathSaveCooldown > 0.0f && player.deathSaveCd <= 0.0f &&
                            (perkFx.deathSaveCooldown > 60.0f || randf() < 0.20f))
                        {
                            player.health = 1.0f;
                            player.deathSaveCd = perkFx.deathSaveCooldown;
                            SDL_Color gold = { 255, 210, 60, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 px, player.rect.y - 30.0f,
                                                 "Olumden dondun!", gold);
                            audio_play(SFX_DEATHSAVE);
                            fx_burst(&weaponFx, px, py, 40, 150.0f, 255, 215, 90);   // altın patlama
                        }
                        if (player.health < 0.0f)
                            player.health = 0.0f;
                    }
                }
            }

            // Gazi buff'ı aktifken oyuncudan kızıl közler yükselir (görsel geri bildirim)
            if (player.gaziTimer > 0.0f && perkFx.gaziDmgBonus > 0.0f &&
                randf() < dt * 10.0f)
            {
                fx_spawn(&weaponFx,
                         player.rect.x + player.rect.w * randf(),
                         player.rect.y + player.rect.h * 0.7f,
                         (randf() - 0.5f) * 20.0f, -35.0f - randf() * 25.0f,
                         0.5f, 7.0f + randf() * 6.0f, 255, 90, 40,
                         WEAPON_FX_NONE);
            }

            // Perk zamanlayıcıları + oyuncu itilmesi (çarpışmalı, sönümlü)
            if (player.gaziTimer > 0.0f) player.gaziTimer -= dt;
            if (player.deathSaveCd > 0.0f) player.deathSaveCd -= dt;
            if (player.knockbackX != 0.0f || player.knockbackY != 0.0f)
            {
                SDL_FRect kfx = feet_box(player.rect.x + player.knockbackX * dt,
                                         player.rect.y, player.rect.w, player.rect.h);
                if (!box_hits_solid(kfx) && !decor_blocks(kfx))
                    player.rect.x += player.knockbackX * dt;
                SDL_FRect kfy = feet_box(player.rect.x,
                                         player.rect.y + player.knockbackY * dt,
                                         player.rect.w, player.rect.h);
                if (!box_hits_solid(kfy) && !decor_blocks(kfy))
                    player.rect.y += player.knockbackY * dt;

                float damp = 10.0f;
                player.knockbackX -= player.knockbackX * damp * dt;
                player.knockbackY -= player.knockbackY * damp * dt;
                if (SDL_fabsf(player.knockbackX) < 2.0f) player.knockbackX = 0.0f;
                if (SDL_fabsf(player.knockbackY) < 2.0f) player.knockbackY = 0.0f;
            }

            // ÖLÜM: can bitti → ölüm ekranı başlat + %10 altın cezası
            if (player.health <= 0.0f && deathTimer <= 0.0f)
            {
                deathTimer = 2.5f;
                deathGoldLost = player.gold / 10;
                player.gold -= deathGoldLost;
                player.autoAttack = false;
                player.targetMobId = 0;
                dialogNpc = -1;   // açık UI'lar ölümle kapanır
                shopOpen = false;
                audio_play(SFX_DEATH);
            }

            // Minimap keşfi (fog of war): oyuncunun çevresini aç
            minimap_reveal(renderer, px, py);

            // --- MÜZİK MOOD'U: savaş > bölge; gece açık arazi tekinsizleşir ---
            // Savaş: yakında seni kovalayan/sana saldıran mob var mı?
            bool inCombat = false;
            for (int i = 0; i < mobCount && !inCombat; i++)
            {
                if (!mobs[i].e.alive) continue;
                if (mobs[i].state != MOB_CHASE && mobs[i].state != MOB_ATTACK) continue;
                float mdx = (mobs[i].e.rect.x + mobs[i].e.rect.w / 2.0f) - px;
                float mdy = (mobs[i].e.rect.y + mobs[i].e.rect.h / 2.0f) - py;
                if (mdx * mdx + mdy * mdy < 500.0f * 500.0f)
                    inCombat = true;
            }

            MusicMood mood;
            if (inCombat)
            {
                mood = MOOD_COMBAT;
            }
            else
            {
                int mreg = region_at(px, py);
                mood = (mreg >= 0) ? REGIONS[mreg].mood : MOOD_WILD;
                if (mood == MOOD_WILD && gametime_is_night())
                    mood = MOOD_DANGER;   // gece açık arazi güvenli değil
            }
            music_set_mood(mood);

            loot_update_all(loots, MAX_LOOT, dt);

            floattext_update_all(floatTexts, MAX_FLOAT_TEXTS, dt);

            camera_update(&camera, player.rect.x + player.rect.w / 2.0f,
                          player.rect.y + player.rect.h / 2.0f, winW, winH); // kamerayı güncelle - karakteri takip et

            // E prompt'u için menzildeki en yakın NPC
            nearestNpc = npc_nearest_in_range(px, py);

            // --- BÖLGE TAKİBİ: yeni bölgeye girince adı belirir ---
            int reg = region_at(px, py);
            if (reg != currentRegion)
            {
                currentRegion = reg;
                if (reg >= 0)
                {
                    splashText = REGIONS[reg].name;
                    splashTimer = 3.0f;
                }
            }
            if (splashTimer > 0.0f)
                splashTimer -= dt;
        }

        music_update(dt);   // fade + loop besleme (menüde/ölümde de akar)

        // Kare başlat (clear rengi gri); swapchain yoksa bu kare çizilmez
        if (!r2d_begin_frame(renderer, 155, 155, 155))
            continue;
        tilemap_render(renderer, tile_textures, &camera);     // haritayı çiz
        decor_render_all(renderer, decor_textures, &camera);  // evler (zemin üstü, entity altı)

        PlayerLayer built[24];
        int builtCount = build_player_layers(layers, LAYER_COUNT, &equipment,
                                             &itemTex, built, 24);

        player_render(&player, renderer, built, builtCount, &camera); // oyuncuyu
        
        for (int i = 0; i < mobCount; i++)
            mob_render(&mobs[i], renderer, mob_textures, &camera, player.targetMobId);

        npc_render_all(renderer, npc_textures, &camera, gameFont, nearestNpc);

        // Partiküller dünya varlıklarının ÜSTÜNDE (kan damlası mobun önünde görünsün)
        fx_render(&weaponFx, renderer, glowTex, &camera);

        if (openLootIndex >= 0)
            loot_popup_render(renderer, &loots[openLootIndex], itemIconTex, &itemTex, gameFont, &camera);

        loot_render_all(loots, MAX_LOOT, renderer, crateTex, &camera); // ← kasalar (mob'lardan sonra, HUD'dan önce)

        // Hasar yazıları (mob'ların üstünde, dünya uzayında)
        floattext_render_all(floatTexts, MAX_FLOAT_TEXTS, renderer, gameFont, &camera); // ← BURAYA (present'tan önce)

        // GÜN/GECE karartması: dünyanın üstüne, HUD'un altına (HUD gece de okunur)
        Uint8 darkness = gametime_darkness();
        if (darkness > 0)
        {
            r2d_set_blend(renderer, R2D_BLEND_ALPHA);
            r2d_set_color(renderer, 10, 10, 45, darkness);
            SDL_FRect night = { 0, 0, (float)winW, (float)winH };
            r2d_fill_rect(renderer, &night);
        }

        // Hedef HP bar (seçili mob varsa)
        Mob *hpTarget = mob_find_by_id(mobs, mobCount, player.targetMobId);
        target_hp_bar_render(renderer, hpTarget, winW, gameFont);

        health_bar_render(&player, renderer);

        // Altın göstergesi (HP barının altı) + saat (sağ üst)
        if (gameFont != NULL)
        {
            char goldBuf[32];
            snprintf(goldBuf, sizeof(goldBuf), "Gold %d", player.gold);
            SDL_Color goldCol = { 255, 210, 60, 255 };
            draw_text(renderer, gameFont, goldBuf, 20.0f, 48.0f, goldCol);

            char clockBuf[24];
            gametime_format(clockBuf, sizeof(clockBuf));
            SDL_Color clockCol = { 220, 220, 235, 255 };
            int tw = 0, th = 0;
            TTF_GetStringSize(gameFont, clockBuf, 0, &tw, &th);
            draw_text(renderer, gameFont, clockBuf, winW - tw - 20.0f, 20.0f, clockCol);
        }

        // Hotbar (alt-orta): skill CD'leri + pot adetleri
        hotbar_render(&hotbar, renderer, itemIconTex, &itemTex, gameFont,
                      player.abilityCd, player.itemCd, &inventory, winW, winH);

        if (minimapOpen)
            minimap_render(renderer, player.rect.x + player.rect.w / 2.0f,
                           player.rect.y + player.rect.h / 2.0f,
                           mobs, mobCount, winW, winH);

        if (questLogOpen)
            quest_log_render(questStatus, renderer, gameFont,
                             (float)winW - 340.0f - 20.0f, 60.0f);

        // Ölüm ekranı: koyu kızıl karartma + yazılar (her şeyin üstünde, menü hariç)
        if (deathTimer > 0.0f && splashFont != NULL)
        {
            r2d_set_blend(renderer, R2D_BLEND_ALPHA);
            r2d_set_color(renderer, 40, 0, 0, 200);
            SDL_FRect dead = { 0, 0, (float)winW, (float)winH };
            r2d_fill_rect(renderer, &dead);

            int tw = 0, th = 0;
            TTF_GetStringSize(splashFont, "Oldun", 0, &tw, &th);
            SDL_Color red = { 255, 80, 80, 255 };
            draw_text(renderer, splashFont, "Oldun",
                      (winW - tw) / 2.0f, winH * 0.4f, red);

            if (gameFont != NULL && deathGoldLost > 0)
            {
                char buf[32];
                snprintf(buf, sizeof(buf), "-%d Gold", deathGoldLost);
                TTF_GetStringSize(gameFont, buf, 0, &tw, &th);
                SDL_Color gold = { 255, 210, 60, 255 };
                draw_text(renderer, gameFont, buf,
                          (winW - tw) / 2.0f, winH * 0.4f + 50.0f, gold);
            }
        }

        // Bölge adı splash'i (üst-orta, son 1 sn'de solar)
        if (splashTimer > 0.0f && splashText != NULL && splashFont != NULL)
        {
            int tw = 0, th = 0;
            TTF_GetStringSize(splashFont, splashText, 0, &tw, &th);
            float alpha = (splashTimer < 1.0f) ? splashTimer : 1.0f;
            SDL_Color col = { 240, 235, 210, (Uint8)(255 * alpha) };
            draw_text(renderer, splashFont, splashText,
                      (winW - tw) / 2.0f, winH * 0.15f, col);
        }
        

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
            perk_pips_render(player.skills, player.perkChoice, renderer, gameFont,
                             40.0f, 40.0f);
            if (perkPickSkill >= 0)
                perk_choice_render((SkillType)perkPickSkill, perkPickTier,
                                   renderer, gameFont,
                                   40.0f + SKILL_PANEL_W + 10.0f, 40.0f);
        }

        if (shopOpen)
            shop_render(renderer, gameFont, itemIconTex, &itemTex, player.gold, 40.0f, 40.0f);

        if (dialogNpc >= 0)
            dialog_render(&npcDialogs[dialogNpc], dialogNode, NPCS[dialogNpc].name,
                          renderer, gameFont, winW, winH);

        if (gameState == GAME_PAUSED)                          // ← EKLE
            menu_render(renderer, gameFont, menuScreen, winW, winH);

        r2d_end_frame(renderer); // batch'i GPU'ya gönder + ekrana bas
    }

    // 5) Her şeyi oluşturmanın TERS sırasıyla temizle. C'de bu bizim sorumluluğumuzda,
    // hiçbir destructor bunu kendiliğinden yapmaz.
    for (int i = 0; i < LAYER_COUNT; i++)
    {
        r2d_texture_destroy(NULL, layers[i].tex);
    }
    for (int i = 0; i < TILE_TYPE_COUNT; i++)
    {
        if (tile_textures[i])
            r2d_texture_destroy(NULL, tile_textures[i]); // NULL olanları atla
    }

    for (int i = 0; i < MOB_TYPE_COUNT; i++)
    {
        if (mob_textures[i])
            r2d_texture_destroy(NULL, mob_textures[i]); // NULL olanları atla
    }

        
    for (int i = 0; i < NPC_COUNT; i++)
        if (npc_textures[i])
            r2d_texture_destroy(NULL, npc_textures[i]);

    for (int i = 0; i < DECOR_COUNT && i < 16; i++)
        if (decor_textures[i])
            r2d_texture_destroy(NULL, decor_textures[i]);

    if (glowTex)
        r2d_texture_destroy(NULL, glowTex);

    minimap_quit();
    ui_theme_quit();
    music_quit();
    audio_quit();
    item_textures_free(&itemTex);
    font_close(gameFont);
    font_close(splashFont);
    font_shutdown();   // tüm fontlar kapandıktan sonra, bir kez

    if (crateTex)
        r2d_texture_destroy(NULL, crateTex);
    r2d_quit(renderer); // GPU kaynaklarını temizle
    SDL_DestroyWindow(window);     // Pencereyi temizle
    SDL_Quit();                    // SDL'i temizle
    return 0;                      // Başarılı çık
}