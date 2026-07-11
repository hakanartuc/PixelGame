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
#include "light.h"
#include "crime.h"
#include "guard.h"
#include "sneak.h"
#include "resource.h"
#include "craft.h"
#include "contract.h"
#include "skilltree.h"

#define BASE_MAX_HP 100.0f   // oyuncu taban canı (perk bonusları üstüne biner)

// Bölge indeksi (dünya konumundan); bölge dışı → 0 (Yeşil Ova varsayılan)
static int region_of(float x, float y)
{
    int r = region_at(x, y);
    return (r < 0 || r >= MAX_REGIONS) ? 0 : r;
}

// Diyalog koşul değerlendirmesi: sert kilitler (gold/rep/bounty) + speech zar yüzdesi.
// Zar modeli: eşik üstü %100, altı (speech/eşik) × %70.
static void dialog_eval_opt(const DialogOpt *o, const Player *p, int npcRegion,
                            bool burned, DialogOptState *out)
{
    out->locked = false;
    out->burned = burned;
    out->chancePct = -1;

    for (int i = 0; i < o->reqCount; i++) {
        const DialogReq *r = &o->reqs[i];
        float cur = 0.0f;
        switch (r->var) {
            case DVAR_SPEECH: cur = (float)p->skills[SKILL_SPEECH].level; break;
            case DVAR_GOLD:   cur = (float)p->gold; break;
            case DVAR_REP:    cur = p->reputation[npcRegion]; break;
            case DVAR_BOUNTY: cur = (float)p->bounty[npcRegion]; break;
        }
        bool pass = r->lessEq ? (cur <= r->value) : (cur >= r->value);

        if (r->var == DVAR_SPEECH && !r->lessEq && r->value > 0.0f) {
            out->chancePct = pass ? 100 : (int)(cur / r->value * 70.0f);
            if (out->chancePct < 0) out->chancePct = 0;
        } else if (!pass) {
            out->locked = true;   // sert koşul — seçilemez
        }
    }
}

// Seçenek efektlerini uygula (gold/rep/bounty — NPC'nin bölgesi bağlamında)
static void dialog_apply_fx(const DialogOpt *o, Player *p, int npcRegion)
{
    for (int i = 0; i < o->fxCount; i++) {
        const DialogFx *x = &o->fx[i];
        float v = x->value;
        switch (x->var) {
            case DVAR_GOLD:
                if (x->op == '+') p->gold += (int)v;
                else if (x->op == '-') p->gold -= (int)v;
                else p->gold = (int)v;
                if (p->gold < 0) p->gold = 0;
                break;
            case DVAR_REP: {
                float d = (x->op == '+') ? v
                        : (x->op == '-') ? -v
                        : v - p->reputation[npcRegion];
                rep_add(&p->reputation[npcRegion], d);
                break;
            }
            case DVAR_BOUNTY:
                if (x->op == '+') p->bounty[npcRegion] += (int)v;
                else if (x->op == '-') p->bounty[npcRegion] -= (int)v;
                else p->bounty[npcRegion] = (int)v;
                if (p->bounty[npcRegion] < 0) p->bounty[npcRegion] = 0;
                break;
            default: break;   // DVAR_SPEECH parser'da reddedilir
        }
    }
}

// Mob ölümü — saldırıdan da kanamadan da aynı yol: animasyon + loot + görev.
// (İki yerden çağrılır; kopya kod desenkron olmasın diye tek fonksiyon)
// Bölünen türlerde (slime) yavrular BURADA doğar — kızgın doğarlar (aggro).
static void mob_die(Mob *target, Mob *mobs, int mobCount, int *nextMobId,
                    Loot *loots, QuestStatus *questStatus,
                    FloatText *floatTexts, const Player *player)
{
    target->e.alive = false;
    target->dying = true;
    target->state = MOB_DYING;
    target->currentFrame = 0;
    target->animTimer = 0.0f;

    // Bölünme (Faz 7): leşin iki yanına yavrular fırlar
    if (target->type->splitCount > 0)
    {
        for (int k = 0; k < target->type->splitCount; k++)
        {
            float ox = (k % 2 == 0) ? -26.0f : 26.0f;
            float oy = (float)(k / 2) * 22.0f - 8.0f;
            int cid = mob_spawn(mobs, mobCount, nextMobId,
                                &MOB_TYPES[target->type->splitChild],
                                target->e.rect.x + ox, target->e.rect.y + oy, -1);
            Mob *child = mob_find_by_id(mobs, mobCount, cid);
            if (child != NULL)
            {
                child->aggro = true;         // parçalar kızgın doğar
                child->suspicion = 1.0f;
                child->alertFlash = 0.5f;
            }
        }
    }

    // Loot yalnız taşıyan türlerde (slime zinciri sonu Küçük'te — kasa spam'i olmaz)
    int li = -1;
    if (target->type->lootTableCount > 0 || target->type->goldMax > 0)
        li = loot_spawn(loots, MAX_LOOT, target->e.rect.x, target->e.rect.y,
                        target->type->lootTable, target->type->lootTableCount,
                        target->type->goldMin, target->type->goldMax);

    // Ok geri kazanımı: saplanan okların her biri %30 şansla leşten çıkar
    if (li >= 0 && target->arrowsHit > 0)
    {
        int back = 0;
        for (int a = 0; a < target->arrowsHit; a++)
            if (randf() < 0.30f) back++;
        if (back > 0 && loots[li].itemCount < MAX_LOOT_ITEMS)
            loots[li].items[loots[li].itemCount++] =
                item_make(&ITEM_TYPES[ITEM_ARROW], back, 0);
    }
    target->arrowsHit = 0;

    contract_on_kill(target->type);   // pano kes-kontratları da sayar (Faz 6)

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
                             "Görev tamam! Ödül için dön.", green);
    }
}

// Ağaç etkileri köprüsü: main() yereli perkFx'e statik yardımcılardan erişim
// (gather_finish/craft yardımcıları imza şişirmeden düğüm etkilerini okur)
static const PerkEffects *g_fx = NULL;

// Ok kademeleri (Faz 8): tüketim sırası ucuzdan pahalıya (otomatik düşüş bunu izler)
static const int AMMO_ORDER[3] = { ITEM_ARROW, ITEM_ARROW_BONE, ITEM_ARROW_IRON };

// Envanterde HERHANGİ bir ok var mı? (yay geçerlilik kontrolü)
static bool ammo_any(const Inventory *inv)
{
    for (int k = 0; k < 3; k++)
        if (inventory_count(inv, &ITEM_TYPES[AMMO_ORDER[k]]) > 0)
            return true;
    return false;
}

// Aktif oktan 1 tüket; bittiyse eldeki başka türe OTOMATİK geç (bildirimli).
// Dönen: tüketilen türün hasar çarpanı (0 = hiç ok yok — atış yapılmaz)
static float ammo_consume(Inventory *inv, int *activeAmmo,
                          FloatText *floatTexts, float px, float py,
                          float saveChance)
{
    if (inventory_count(inv, &ITEM_TYPES[*activeAmmo]) < 1)
    {
        int found = -1;
        for (int k = 0; k < 3; k++)
            if (inventory_count(inv, &ITEM_TYPES[AMMO_ORDER[k]]) > 0)
            {
                found = AMMO_ORDER[k];
                break;
            }
        if (found < 0) return 0.0f;
        *activeAmmo = found;
        char buf[FLOAT_TEXT_LEN];
        snprintf(buf, sizeof(buf), "%s'na geçildi", ITEM_TYPES[found].name);
        SDL_Color dim = { 200, 200, 160, 255 };
        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS, px, py - 10.0f, buf, dim);
    }
    // Cephaneci düğümü: şansla ok tükenmez (atış yine yapılır)
    if (randf() >= saveChance)
        inventory_consume(inv, &ITEM_TYPES[*activeAmmo]);
    return ITEM_TYPES[*activeAmmo].ammoDmgMult;
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
    if (g_fx != NULL) hotbar_place_unlocks(hotbar, g_fx->abilityOpen);
    audio_play(SFX_LEVELUP);

    // Skill ağacı puanı: her 4 seviyede 1 (Faz 21)
    if (tree_has_content(sk) &&
        player->skills[sk].level % TREE_LEVELS_PER_POINT == 0)
    {
        SDL_Color gold = { 255, 210, 60, 255 };
        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                             player->rect.x + player->rect.w / 2.0f,
                             player->rect.y - 30.0f, "+1 skill puanı! (K)", gold);
    }

    // Kader puanı: TOPLAM seviyenin her 10'u (savaşmayan build de kazanır)
    int total = 0;
    for (int i = 0; i < SKILL_COUNT; i++) total += player->skills[i].level;
    if (total % FATE_LEVELS_PER_POINT == 0)
    {
        SDL_Color fate = { 200, 150, 255, 255 };
        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                             player->rect.x + player->rect.w / 2.0f,
                             player->rect.y - 50.0f, "+1 KADER puanı! (K)", fate);
    }
}

// Toplama hız çarpanı: kademeli eşik — her 20 seviyede +0.25 (L100 = 2.25x).
// Basamaklı bilerek: seviye eşiğini geçmenin karşılığı NET hissedilsin.
static float gather_mult(int level)
{
    return 1.0f + 0.25f * (float)(level / 20);
}

// '+basma' başarı şansı: +5'e kadar garanti, sonrası düşer; Smithing telafi eder.
// Hedef plus 1..8 (ITEM_MAX_PLUS) — dizin = hedef plus.
static float plus_chance(int targetPlus, int smithLevel)
{
    static const float base[ITEM_MAX_PLUS + 1] =
        { 0, 100, 100, 100, 100, 100, 80, 65, 50 };
    if (targetPlus < 1 || targetPlus > ITEM_MAX_PLUS) return 0.0f;
    float c = base[targetPlus] + (float)smithLevel * 0.2f;
    if (g_fx != NULL) c += g_fx->plusChanceBonus;   // Smithing ağacı (Usta Çekiç...)
    return (c > 95.0f) ? 95.0f : c;
}

// Büyü Kalkanı: vuruş hasarını HP yerine manadan emer (×1.2 mana bedeli).
// Mana yetmezse karşılayabildiğini emer, kalkan kırılır; kalan HP hasarı döner.
static float mana_shield_absorb(Player *p, float dmg, FloatText *floatTexts)
{
    if (p->manaShieldTimer <= 0.0f || dmg <= 0.0f) return dmg;
    SDL_Color icy = { 150, 220, 255, 255 };
    float need = dmg * 1.2f;
    if (p->mana >= need)
    {
        p->mana -= need;
        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                             p->rect.x + p->rect.w / 2.0f,
                             p->rect.y - 26.0f, "Emildi", icy);
        return 0.0f;
    }
    float absorbed = p->mana / 1.2f;
    p->mana = 0.0f;
    p->manaShieldTimer = 0.0f;
    p->shieldBreakT = 0.001f;   // kırılma animasyonu
    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                         p->rect.x + p->rect.w / 2.0f,
                         p->rect.y - 26.0f, "Kalkan kırıldı!", icy);
    return dmg - absorbed;
}

// Toplama tamamlandı: miktar rulosu + envanter + XP + bildirim.
// Dönen: süreç kapansın mı (çanta doluysa bar dolu-altında bekler, süreç yine kapanır).
static void gather_finish(int res, Player *player, Inventory *inv,
                          Hotbar *hotbar, FloatText *floatTexts)
{
    const ResourceDef *rd = &RESOURCES[res];
    int lvl = player->skills[rd->skill].level;
    int count = rd->minCount + (int)(randf() * (float)(rd->maxCount - rd->minCount + 1));
    if (count > rd->maxCount) count = rd->maxCount;
    if (randf() < (float)lvl * 0.004f) count++;   // seviye başına %0.4 bonus şansı

    // Meslek ağacı: çift ürün şansı (Zengin Damar / Doğanın Payı / Orman Hasadı)
    if (g_fx != NULL)
    {
        float dbl = (rd->skill == SKILL_MINING)      ? g_fx->mineDouble
                  : (rd->skill == SKILL_HERBALISM)   ? g_fx->herbDouble
                  : (rd->skill == SKILL_WOODCUTTING) ? g_fx->woodDouble : 0.0f;
        if (randf() < dbl) count *= 2;
    }

    if (inventory_add(inv, &ITEM_TYPES[rd->itemIdx], count, 0))
    {
        resource_harvest(res);
        audio_play(SFX_LOOT);
        float xp = (rd->skill == SKILL_MINING) ? 8.0f
                 : (rd->skill == SKILL_WOODCUTTING) ? 6.0f : 5.0f;
        if (skill_add_xp(player->skills, rd->skill, xp))
            on_skill_levelup(player, rd->skill, hotbar, floatTexts);
        char buf[FLOAT_TEXT_LEN];
        snprintf(buf, sizeof(buf), "+%d %s", count, ITEM_TYPES[rd->itemIdx].name);
        SDL_Color green = { 150, 255, 150, 255 };
        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                             player->rect.x + player->rect.w / 2.0f,
                             player->rect.y - 10.0f, buf, green);
    }
    else
    {
        // Çanta dolu: ödül bekler — yer açınca tek hamle tamamlar
        resource_add_progress(res, -0.6f);
        SDL_Color red = { 255, 120, 100, 255 };
        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                             player->rect.x + player->rect.w / 2.0f,
                             player->rect.y - 10.0f, "Çanta dolu!", red);
    }
}

// Üretim tamamlandı: malzemeler ŞİMDİ tüketilir (iptal malzeme yakmaz),
// çıktı + XP + bildirim. Çanta doluysa üretim gerçekleşmez (malzeme durur).
static void craft_finish_recipe(int ri, Player *player, Inventory *inv,
                                Hotbar *hotbar, FloatText *floatTexts)
{
    const RecipeDef *r = &RECIPES[ri];
    SDL_Color red = { 255, 120, 100, 255 };

    if (!inventory_consume_n(inv, &ITEM_TYPES[r->in1Item], r->in1Count))
    {
        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                             player->rect.x + player->rect.w / 2.0f,
                             player->rect.y - 10.0f, "Malzeme eksik!", red);
        return;
    }
    if (r->in2Item >= 0 &&
        !inventory_consume_n(inv, &ITEM_TYPES[r->in2Item], r->in2Count))
    {
        // in2 eksikse in1'i GERİ koy — yarım tüketim malzeme yakmasın
        inventory_add(inv, &ITEM_TYPES[r->in1Item], r->in1Count, 0);
        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                             player->rect.x + player->rect.w / 2.0f,
                             player->rect.y - 10.0f, "Malzeme eksik!", red);
        return;
    }
    if (!inventory_add(inv, &ITEM_TYPES[r->outItem], r->outCount, 0))
    {
        // Çanta dolu: tüketilen malzemeyi geri koy (üretim gerçekleşmedi)
        inventory_add(inv, &ITEM_TYPES[r->in1Item], r->in1Count, 0);
        if (r->in2Item >= 0)
            inventory_add(inv, &ITEM_TYPES[r->in2Item], r->in2Count, 0);
        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                             player->rect.x + player->rect.w / 2.0f,
                             player->rect.y - 10.0f, "Çanta dolu!", red);
        return;
    }

    audio_play(SFX_LOOT);
    SkillType sk = station_skill(r->station);
    if (skill_add_xp(player->skills, sk, r->xp))
        on_skill_levelup(player, sk, hotbar, floatTexts);
    char buf[FLOAT_TEXT_LEN];
    snprintf(buf, sizeof(buf), "+%d %s", r->outCount, ITEM_TYPES[r->outItem].name);
    SDL_Color green = { 150, 255, 150, 255 };
    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                         player->rect.x + player->rect.w / 2.0f,
                         player->rect.y - 10.0f, buf, green);
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
        {load_texture(renderer, "assets/characters/body/male/light.png"), 10, -1, false, false},
        {load_texture(renderer, "assets/characters/head/male/human_light.png"), 100, -1, false, false},
        {load_texture(renderer, "assets/characters/face/male/neutral_light.png"), 101, -1, false, false},
    };

    R2DTexture *glowTex = create_glow_texture(renderer, 64);
    R2DTexture *vignetteTex = create_vignette_texture(renderer, 256);   // hasar flash'ı
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
        .speedMult = 1.0f,
        .health = 100.0f,
        .maxHealth = 100.0f,
        .mana = 50.0f,
        .maxMana = 50.0f,
        .stamina = 100.0f,
        .maxStamina = 100.0f,
        .strength = 10,    // sabit taban — dağıtım yok, ileride ekipman/perk ekler
        .dexterity = 10,
        .gold = 50,        // başlangıç parası (vendor için)
        // weaponFx her frame kusanili silahtan senkronlanir (bkz. player_update oncesi)
    };
    skills_init(player.skills);
    player.pendingAbility = -1;   // -1 = normal vuruş (0 geçerli ability olurdu)

    // Skill ağaçları (Faz 21): hiç düğüm alınmamış başlar; etkiler nötr hesaplanır
    for (int s = 0; s < SKILL_COUNT; s++)
        player.treeTaken[s] = 0;
    PerkEffects perkFx;
    tree_compute_effects(player.treeTaken, player.fateTaken, &perkFx);
    g_fx = &perkFx;                        // statik yardımcı köprüsü
    skill_set_xp_mult(perkFx.xpGainMult);  // Kader XP çarpanı
    int treeOpenSkill = -1;   // açık ağaç paneli (K panelinden satır tıklanır)
    int treeSelNode = -1;     // seçili düğüm (ikinci tık = AL onayı)

    // Hotbar: açık ability'ler otomatik yerleşir (Power Strike lv1'de açık)
    Hotbar hotbar = {0};
    hotbar_place_unlocks(&hotbar, perkFx.abilityOpen);

    // Dekorlar (ev/ağaç/çalı/saman): texture + boyutları (render/collision tek kaynaktan)
    R2DTexture *decor_textures[32] = {0};
    for (int i = 0; i < DECOR_COUNT && i < 32; i++)
        decor_textures[i] = load_texture(renderer, DECORS[i].texPath);
    decor_set_texture_sizes(decor_textures);

    // Kaynak düğümleri (cevher/bitki/ağaç) — toplama sistemi
    R2DTexture *resource_textures[MAX_RESOURCES] = {0};
    for (int i = 0; i < RESOURCE_COUNT && i < MAX_RESOURCES; i++)
        resource_textures[i] = load_texture(renderer, RESOURCES[i].texPath);
    resource_reset();

    // Craft istasyonları (ocak/kazan)
    R2DTexture *station_textures[8] = {0};
    for (int i = 0; i < STATION_COUNT && i < 8; i++)
        station_textures[i] = load_texture(renderer, STATIONS[i].texPath);

    // İlan panosu (Faz 6): köy meydanı, günlük kontratlar
    #define BOARD_X (13.5f * TILE_SIZE)
    #define BOARD_Y (7.0f * TILE_SIZE)
    #define BOARD_RANGE 90.0f
    R2DTexture *boardTex = load_texture(renderer, "assets/decor/notice_board.png");
    bool boardOpen = false;

    // Büyü VFX sprite'ları (Faz 22): mermi küreleri + arcane patlama + alev
    R2DTexture *orbFireTex  = load_texture(renderer, "assets/fx/orb_fire.png");
    R2DTexture *orbBlueTex  = load_texture(renderer, "assets/fx/orb_blue.png");
    R2DTexture *arcaneTex   = load_texture(renderer, "assets/fx/arcane_burst.png");
    R2DTexture *flameTex    = load_texture(renderer, "assets/fx/flame_small.png");
    R2DTexture *shieldTex   = load_texture(renderer, "assets/fx/iceshield.png");
    R2DTexture *spikesTex   = load_texture(renderer, "assets/fx/spikes.png");

    // Diken patlaması görselleri (yer efekti — 10 kare × 0.06sn)
    #define MAX_SPIKE_FX 6
    typedef struct { bool active; float x, y, t; } SpikeFxAnim;
    SpikeFxAnim spikeFx[MAX_SPIKE_FX] = {0};

    // Tek atımlık patlama animasyonları (arcane 7 kare × 0.045sn)
    #define MAX_BURSTS 8
    typedef struct { bool active; float x, y, t, scale; } BurstFx;
    BurstFx bursts[MAX_BURSTS] = {0};

    // Oyuncu kamp ateşi kurulumu (Faz 6): Odun'a sağ tık başlatır, 3 sn süreç.
    // Konum STATIONS[STATION_PLAYER_FIRE_IDX]'te yaşar (sönük = CAMPFIRE_PARKED).
    // Ömür: kurulumdan itibaren 24 OYUN SAATİ (gece yarısı sınırı değil —
    // 23:50'de kurulan ateş ertesi gece 23:50'ye dek yanar)
    float campBuild = -1.0f;      // <0 = kurulum yok
    float campfireOutAt = -1.0f;  // sönme anı (toplam oyun saati: gün×24+saat)

    gametime_init();   // gün 1, 08:00
    npc_reset();       // NPC'ler canlı + tam canla başlar (statik durum sıfırdan gelir)

    // Muhafız beyni: NPCS içindeki isGuard girdisine bağlanır.
    // Yetki alanı = nöbet noktasının bölgesi (bounty bölgesel — muhafız kendi bölgesini tahsil eder)
    int guardIdx = -1;
    for (int i = 0; i < NPC_COUNT; i++)
        if (NPCS[i].isGuard) { guardIdx = i; break; }
    int guardRegion = 0;
    if (guardIdx >= 0)
    {
        guard_init(guardIdx);
        guardRegion = region_of(NPCS[guardIdx].schedule[0].x, NPCS[guardIdx].schedule[0].y);
    }
    int lastSeenDay = gametime_day();   // günlük itibar toparlanması için gün takibi
    contract_new_day(lastSeenDay);      // panonun ilk günü (load bunu ezebilir)
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
    int targetNpc = -1;    // saldırı hedefi NPC (yalnız TIKLAMAYLA seçilir — Z asla NPC seçmez)
    bool catchOpen = false; // muhafız yakalama paneli (öde / karşı koy / rüşvet)

    // Başarısız speech zarı: NPC+düğüm+seçenek O GÜN kilitli — kapat-aç spam'ı işlemez,
    // ertesi gün yeniden denenir (günlük itibar toparlanmasıyla aynı ritim). 0 = hiç yanmadı.
    int dlgBurnDay[8][DIALOG_MAX_NODES][DIALOG_MAX_OPTS] = {0};
    int dialogNode = -1;   // aktif diyalog düğümü
    bool shopOpen = false;
    int nearestNpc = -1;   // E prompt'u için (her kare güncellenir)
    int nearestRes = -1;   // E menzilindeki toplanabilir kaynak düğümü
    int gatherRes = -1;    // aktif toplama süreci (-1 = yok): savurma döngüsü çalışır

    // Craft durumu: panel + aktif üretim süreci (malzeme TAMAMLANINCA tüketilir)
    int nearestStation = -1;
    bool craftOpen = false;
    int craftStation = -1;    // panelin bağlı olduğu istasyon
    int craftRecipe = -1;     // süreçteki tarif (RECIPES indeksi)
    bool plusForge = false;   // '+basma' süreci (ocak, kuşanılı silah)
    float craftProgress = 0.0f;

    // Günlük ders damgaları (öğretmenler; save'e girmez — gün başına sıfır kabulü)
    int trainedDay[32];
    for (int i = 0; i < 32; i++) trainedDay[i] = -1;
    craft_reset_learned();   // blueprint tarifleri kapalı başlar (save v10 doldurur)

    // Sunak durumu: hedef ekipman + aktif basma işlemi (0 yok, 1/2 rün, 10+s stat)
    EquipSlot enchTarget = EQUIP_WEAPON;
    int enchAction = 0;

    QuestStatus questStatus[MAX_QUESTS] = {0};   // hepsi NOT_STARTED

    // Başlangıç seti: tam demir set + kılıç KUŞANILI (+0), çantada 3 iksir
    equipment.slots[EQUIP_WEAPON] = item_make(&ITEM_TYPES[ITEM_LONG_SWORD], 1, 0);
    equipment.slots[EQUIP_HELMET] = item_make(&ITEM_TYPES[ITEM_HELMET_IRON], 1, 0);
    equipment.slots[EQUIP_CHEST]  = item_make(&ITEM_TYPES[ITEM_CHEST_IRON], 1, 0);
    equipment.slots[EQUIP_LEGS]   = item_make(&ITEM_TYPES[ITEM_LEGS_IRON], 1, 0);
    inventory_add(&inventory, &ITEM_TYPES[ITEM_POTION_HP], 3, 0);
        

    ParticleSystem weaponFx;
    fx_init(&weaponFx);

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

    // Uçan oklar (Faz 5): hedefe güdümlü (KO auto-attack modeli — ıska yok),
    // hasar VARIŞTA hesaplanır (o anki stat/rün ile). Hedef ölürse ok söner.
    // kind: 0 normal, 1 Buz Oku (yavaşlatır), 2 Alan Oku (varışta patlar)
    // ammoMult: atıldığı ok kademesinin çarpanı (Faz 8)
    #define MAX_ARROWS 16
    #define ARROW_SPEED 420.0f
    #define AOE_ARROW_RADIUS 90.0f
    typedef struct { bool active; float x, y; int targetId;
                     float dmgScale; float ammoMult; Sint8 kind; } ArrowShot;
    ArrowShot arrows[MAX_ARROWS] = {0};
    int activeAmmo = ITEM_ARROW;   // sağ tıkla seçilir; bitince otomatik düşer

    // Yaylım Ateşi kuyruğu: release'te 1 ok + 0.15sn arayla 2 ok daha
    int volleyLeft = 0;
    float volleyTimer = 0.0f;

    // Yapışkan Tuzakları (Faz 7): yere kurulur, mob basınca alan yavaşı patlar.
    // Dolu dizide en eski ezilir (round-robin) — tuzak spam'i sınırlı kalır.
    #define MAX_TRAPS 4
    #define TRAP_TRIGGER_R 50.0f    // tetikleme yarıçapı (mob merkezi)
    #define TRAP_AOE_R 100.0f       // yavaşlatma alanı
    typedef struct { bool active; float x, y; } GooTrap;
    GooTrap traps[MAX_TRAPS] = {0};
    int nextTrap = 0;

    // Büyü sistemi (Faz 22): cast KÖKLÜ (~0.45sn × ağaç çarpanı), mana COMMIT'te.
    // Ateş Topu mermisi oyuncu okları gibi güdümlü (SpellProj).
    #define SPELL_RANGE 300.0f
    #define SPELL_CAST_TIME 0.45f
    #define MAX_SPELL_PROJS 8
    #define SPELL_PROJ_SPEED 380.0f
    typedef struct { bool active; float x, y; int targetId; } SpellProj;
    SpellProj spellProjs[MAX_SPELL_PROJS] = {0};
    int castSpell = -1;        // aktif cast (AbilityId; -1 yok)
    float castTimer = 0.0f;

    // Zırh ağırlığı (Faz 22): türetilmiş statlarda hesaplanır, hız/mana'ya işler
    float armorSpeedPenalty = 0.0f;   // 0..0.30 (STR telafi eder)
    float armorManaMult = 1.0f;       // ağır/orta parça mana yenilenmesini kısar

    // Mob mermileri (Faz 8): atış ANINDAKİ oyuncu konumuna DÜZ hat — güdüm yok,
    // yana yürüyerek KAÇILABİLİR (oyuncu oklarına bilinçli asimetri).
    // kind: 0 = iskelet oku (çizgi), 1 = büyü topu (mor iz)
    #define MAX_MOB_PROJS 24
    #define MOB_PROJ_MAX_DIST 520.0f
    typedef struct { bool active; float x, y, vx, vy;
                     float traveled; float dmg; Sint8 kind; } MobProj;
    MobProj mobProjs[MAX_MOB_PROJS] = {0};

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
    Camera camera = {.x = 0.0f, .y = 0.0f, .zoom = 1.0f};

    // His katmanı: ekran sarsıntısı + hasar flash'ı (oyuncu odaklı)
    float shakeTime = 0.0f, shakeDur = 1.0f, shakeAmp = 0.0f; // amp: ekran px
    float flashTime = 0.0f;                                    // kalan flash süresi
#define FLASH_DUR 0.35f

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
                    else if (catchOpen)
                    {
                        catchOpen = false; // 0) yakalama paneli — muhafız peşini bırakmaz, yine açar
                    }
                    else if (boardOpen)
                    {
                        boardOpen = false; // 0a) ilan panosu
                    }
                    else if (craftOpen)
                    {
                        craftOpen = false; // 0b) craft paneli (süreç iptal — malzeme yanmaz)
                        craftRecipe = -1;
                        plusForge = false;
                        enchAction = 0;
                        craftProgress = 0.0f;
                    }
                    else if (openLootIndex >= 0)
                    {
                        loots[openLootIndex].state = LOOT_CLOSING; // 1) once loot popup
                        openLootIndex = -1;
                    }
                    else if (treeOpenSkill >= 0)
                    {
                        treeOpenSkill = -1; // 2) skill ağacı paneli
                        treeSelNode = -1;
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
                else if (event.key.key == SDLK_C && gameState == GAME_PLAYING)
                {
                    // Eğilme (Sneak): algı/tanık azalır, hız düşer — kesin gizlilik değil
                    player.sneaking = !player.sneaking;
                    audio_play(SFX_UI);
                }
                else if (event.key.key == SDLK_SPACE && gameState == GAME_PLAYING)
                {
                    // Araç kuşanılıyken SALDIRI YOK — skill sistemi silahlarla işler
                    const ItemType *spcW = equipment.slots[EQUIP_WEAPON].type;
                    if (spcW != NULL && spcW->weaponType == WEAPON_TOOL)
                    {
                        SDL_Color red = { 255, 120, 100, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f, "Silah kuşan!", red);
                    }
                    // KO modeli: Space = auto-attack aç/kapa (hedef yoksa en yakın MOB'u seç;
                    // NPC hedefi yalnız tıklamayla gelir, Space onu da döngüye sokar)
                    else if (player.autoAttack)
                    {
                        player.autoAttack = false;
                    }
                    else
                    {
                        if (player.targetMobId == 0 && targetNpc < 0)
                        {
                            float px = player.rect.x + player.rect.w / 2.0f;
                            float py = player.rect.y + player.rect.h / 2.0f;
                            player.targetMobId = mob_find_nearest(mobs, mobCount, px, py);
                        }
                        if (player.targetMobId != 0 || targetNpc >= 0)
                        {
                            player.autoAttack = true;
                            player.attackTimer = 0.0f;   // ilk vuruş beklemesin
                        }
                    }
                }
                else if (catchOpen && gameState == GAME_PLAYING &&
                         event.key.key == SDLK_3)
                {
                    // [3] Rüşvet: bounty'nin yarısı; Speech eşiği altı zar.
                    // Başarı: borç silinir ama itibar düşer (kanunu satın aldın).
                    // Başarısızlık: karşı koyma sayılır — muhafız düşmanlaşır.
                    int cost = (int)(player.bounty[guardRegion] / 2 *
                                     perkFx.bribeCostMult);
                    int sp = player.skills[SKILL_SPEECH].level;
                    int pct = (sp >= CRIME_BRIBE_SPEECH) ? 100
                            : (int)((float)sp / CRIME_BRIBE_SPEECH * 70.0f);
                    if (player.gold < cost)
                    {
                        SDL_Color red = { 255, 90, 90, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f, "Altın yetmiyor!", red);
                    }
                    else if (randf() * 100.0f < (float)pct)
                    {
                        player.gold -= cost;
                        player.bounty[guardRegion] = 0;
                        rep_add(&player.reputation[guardRegion], REP_BRIBE);
                        if (skill_add_xp(player.skills, SKILL_SPEECH, 15.0f))
                            on_skill_levelup(&player, SKILL_SPEECH, &hotbar, floatTexts);
                        catchOpen = false;
                        audio_play(SFX_UI);
                        char buf[FLOAT_TEXT_LEN];
                        snprintf(buf, sizeof(buf), "-%d g (rüşvet)", cost);
                        SDL_Color white = { 255, 255, 255, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f, buf, white);
                    }
                    else
                    {
                        // "Beni satin alamazsin!" — karşı koyma yolu
                        player.bounty[guardRegion] += CRIME_BOUNTY_RESIST;
                        player.lastCrimeDay = gametime_day();
                        rep_add(&player.reputation[guardRegion], REP_FAILED_CHECK);
                        if (skill_add_xp(player.skills, SKILL_SPEECH, 3.0f))
                            on_skill_levelup(&player, SKILL_SPEECH, &hotbar, floatTexts);
                        guard_set_hostile(true);
                        catchOpen = false;
                        SDL_Color red = { 255, 80, 80, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 24.0f,
                                             "Beni satın alamazsın! +50 bounty", red);
                    }
                }
                else if (catchOpen && gameState == GAME_PLAYING &&
                         (event.key.key == SDLK_1 || event.key.key == SDLK_2))
                {
                    // Yakalama paneli: [1] cezayı öde  [2] karşı koy
                    // (ödeme yasal borcu siler — İTİBARI DÜZELTMEZ, karar defteri)
                    if (event.key.key == SDLK_1)
                    {
                        if (player.gold >= player.bounty[guardRegion])
                        {
                            char buf[FLOAT_TEXT_LEN];
                            snprintf(buf, sizeof(buf), "-%d g (ceza)", player.bounty[guardRegion]);
                            player.gold -= player.bounty[guardRegion];
                            player.bounty[guardRegion] = 0;
                            catchOpen = false;
                            audio_play(SFX_UI);
                            SDL_Color white = { 255, 255, 255, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f, buf, white);
                        }
                        else
                        {
                            SDL_Color red = { 255, 90, 90, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f, "Altın yetmiyor!", red);
                        }
                    }
                    else   // karşı koy
                    {
                        player.bounty[guardRegion] += CRIME_BOUNTY_RESIST;
                        player.lastCrimeDay = gametime_day();
                        guard_set_hostile(true);
                        catchOpen = false;
                        SDL_Color red = { 255, 80, 80, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 24.0f, "Karşı koydun! +50 bounty", red);
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
                            DialogOpt *opt = &node->opts[sel];
                            const char *target = opt->target;

                            // Koşullar: sert kilit engeller; speech eşiği altı zar atar
                            float dgx, dgy;
                            int dlgRegion = npc_visible_pos(dialogNpc, &dgx, &dgy)
                                          ? region_of(dgx, dgy) : 0;
                            DialogOptState ost;
                            bool burnedToday = dialogNpc < 8 &&
                                dlgBurnDay[dialogNpc][dialogNode][sel] == gametime_day();
                            dialog_eval_opt(opt, &player, dlgRegion, burnedToday, &ost);

                            bool proceed = !ost.locked && !ost.burned;
                            // Ağaç: zar bonusu (İkna dalı) — 100'ü aşamaz (garanti değil, şans)
                            float diceChance = (float)ost.chancePct + perkFx.speechDiceBonus;
                            if (diceChance > 99.0f) diceChance = 99.0f;
                            if (proceed && ost.chancePct >= 0 && ost.chancePct < 100 &&
                                randf() * 100.0f >= diceChance)
                            {
                                // Zar tutmadı: BUGÜN kilit + itibar cezası (Gümüş Dil 2x!) + az XP
                                proceed = false;
                                if (dialogNpc < 8)
                                    dlgBurnDay[dialogNpc][dialogNode][sel] = gametime_day();
                                rep_add(&player.reputation[dlgRegion],
                                        REP_FAILED_CHECK * perkFx.dicePenaltyMult);
                                if (skill_add_xp(player.skills, SKILL_SPEECH, 3.0f))
                                    on_skill_levelup(&player, SKILL_SPEECH, &hotbar, floatTexts);
                                SDL_Color red = { 255, 120, 100, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f,
                                                     "İkna edemedin...", red);
                            }
                            else if (proceed)
                            {
                                // Başarı: efektler + (zarlı seçenekse) Speech XP
                                dialog_apply_fx(opt, &player, dlgRegion);
                                float spTh;
                                if (dialog_opt_speech_req(opt, &spTh))
                                    if (skill_add_xp(player.skills, SKILL_SPEECH, 15.0f))
                                        on_skill_levelup(&player, SKILL_SPEECH, &hotbar, floatTexts);
                            }

                            if (!proceed)
                            {
                                // kilitli/yanmış/zar tutmadı — düğümde kal
                            }
                            else if (strcmp(target, "end") == 0)
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
                                // Dip itibarda güven yok — yeni görev verilmez
                                float dnx, dny;
                                int dRegion = npc_visible_pos(dialogNpc, &dnx, &dny)
                                            ? region_of(dnx, dny) : 0;
                                if (player.reputation[dRegion] <= REP_NO_QUESTS)
                                {
                                    SDL_Color red = { 255, 120, 100, 255 };
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                         player.rect.x + player.rect.w / 2.0f,
                                                         player.rect.y - 10.0f,
                                                         "Sana güven olmaz...", red);
                                    dialogNpc = -1;
                                }
                                else
                                {
                                    int q = quest_find(target + 7);
                                    if (q >= 0 && questStatus[q].state == QUEST_NOT_STARTED)
                                    {
                                        questStatus[q].state = QUEST_ACTIVE;
                                        questStatus[q].progress = 0;
                                        char buf[FLOAT_TEXT_LEN];
                                        snprintf(buf, sizeof(buf), "Görev: %s", QUESTS[q].name);
                                        SDL_Color cyan = { 90, 220, 255, 255 };
                                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                             player.rect.x + player.rect.w / 2.0f,
                                                             player.rect.y - 10.0f, buf, cyan);
                                    }
                                    int nx = dialog_find(dlg, "accepted");
                                    if (nx >= 0) dialogNode = nx; else dialogNpc = -1;
                                }
                            }
                            else if (strncmp(target, "complete:", 9) == 0)
                            {
                                int q = quest_find(target + 9);
                                bool ready = q >= 0 && questStatus[q].state == QUEST_DONE;
                                if (!ready && q >= 0 && questStatus[q].state == QUEST_ACTIVE &&
                                    QUESTS[q].fetchCount > 0)
                                {
                                    // FETCH teslimi: malzemeler ŞİMDİ düşülür (yoksa teslim yok)
                                    ready = inventory_consume_n(&inventory,
                                                &ITEM_TYPES[QUESTS[q].fetchItem],
                                                QUESTS[q].fetchCount);
                                }
                                if (ready)
                                {
                                    questStatus[q].state = QUEST_REWARDED;
                                    player.gold += QUESTS[q].rewardGold;

                                    // Ek item ödülü (örn. Demirci → Çekiç)
                                    if (QUESTS[q].rewardItem > 0 &&
                                        inventory_add(&inventory,
                                                      &ITEM_TYPES[QUESTS[q].rewardItem], 1, 0))
                                    {
                                        char ibuf[FLOAT_TEXT_LEN];
                                        snprintf(ibuf, sizeof(ibuf), "+%s",
                                                 ITEM_TYPES[QUESTS[q].rewardItem].name);
                                        SDL_Color cyan = { 90, 220, 255, 255 };
                                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                             player.rect.x + player.rect.w / 2.0f,
                                                             player.rect.y - 40.0f, ibuf, cyan);
                                    }

                                    // Bölgeye hizmet: itibar yükselir
                                    float qnx, qny;
                                    int qRegion = npc_visible_pos(dialogNpc, &qnx, &qny)
                                                ? region_of(qnx, qny) : 0;
                                    rep_add(&player.reputation[qRegion], REP_QUEST);

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
                            else if (strncmp(target, "train:", 6) == 0)
                            {
                                // Öğretmen dersi: altın → XP (günde skill başına 1;
                                // fiyat = seviye×15g, kazanç = sonraki seviyenin %40'ı)
                                SkillType sk = SKILL_NONE;
                                if (strcmp(target + 6, "smithing") == 0) sk = SKILL_SMITHING;
                                else if (strcmp(target + 6, "speech") == 0) sk = SKILL_SPEECH;
                                else if (strcmp(target + 6, "alchemy") == 0) sk = SKILL_ALCHEMY;

                                SDL_Color red = { 255, 120, 100, 255 };
                                if (sk == SKILL_NONE)
                                {
                                    SDL_Log("Bilinmeyen ders: %s", target);
                                }
                                else if (trainedDay[sk] == gametime_day())
                                {
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                         player.rect.x + player.rect.w / 2.0f,
                                                         player.rect.y - 10.0f,
                                                         "Bugün yeterince çalıştın", red);
                                }
                                else
                                {
                                    int price = (int)(player.skills[sk].level * 15 *
                                                      perkFx.trainCostMult);
                                    if (player.gold < price)
                                    {
                                        char buf[FLOAT_TEXT_LEN];
                                        snprintf(buf, sizeof(buf), "%dg gerekli!", price);
                                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                             player.rect.x + player.rect.w / 2.0f,
                                                             player.rect.y - 10.0f, buf, red);
                                    }
                                    else
                                    {
                                        player.gold -= price;
                                        trainedDay[sk] = gametime_day();
                                        float xp = player.skills[sk].xpToNext * 0.4f;
                                        if (skill_add_xp(player.skills, sk, xp))
                                            on_skill_levelup(&player, sk, &hotbar, floatTexts);
                                        audio_play(SFX_UI);
                                        char buf[FLOAT_TEXT_LEN];
                                        snprintf(buf, sizeof(buf), "-%dg — %s dersi",
                                                 price, skill_name(sk));
                                        SDL_Color cyan = { 90, 220, 255, 255 };
                                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                             player.rect.x + player.rect.w / 2.0f,
                                                             player.rect.y - 10.0f, buf, cyan);
                                    }
                                }
                                // düğümde kal (tekrar ders/çıkış oyuncunun elinde)
                            }
                            else if (strcmp(target, "buy_bp_chest") == 0)
                            {
                                // Demirci'den Demir Zırh çizimi (req: gold>=150 diyalogda)
                                if (inventory_add(&inventory,
                                                  &ITEM_TYPES[ITEM_BP_CHEST_IRON], 1, 0))
                                {
                                    player.gold -= 150;
                                    audio_play(SFX_UI);
                                    SDL_Color cyan = { 90, 220, 255, 255 };
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                         player.rect.x + player.rect.w / 2.0f,
                                                         player.rect.y - 10.0f,
                                                         "+Blueprint: Demir Zırh (-150g)", cyan);
                                }
                                else
                                {
                                    SDL_Color red = { 255, 120, 100, 255 };
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                         player.rect.x + player.rect.w / 2.0f,
                                                         player.rect.y - 10.0f, "Çanta dolu!", red);
                                }
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
                    // NPC'nin bölgesinde bounty ya da dip itibar → kapılar kapalı
                    float npx = 0.0f, npy = 0.0f;
                    int npcRegion = 0;
                    if (nearestNpc >= 0 && npc_visible_pos(nearestNpc, &npx, &npy))
                        npcRegion = region_of(npx, npy);

                    if (nearestNpc >= 0 && player.bounty[npcRegion] > 0)
                    {
                        SDL_Color red = { 255, 120, 100, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             npx, npy - 100.0f,
                                             "Önce cezanı öde!", red);
                    }
                    else if (nearestNpc >= 0 &&
                             player.reputation[npcRegion] <= REP_TALK_REFUSE)
                    {
                        // Ceza ödense bile toplum unutmaz — katil damgası
                        SDL_Color red = { 255, 120, 100, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             npx, npy - 100.0f,
                                             "Seninle işim olmaz!", red);
                    }
                    // Menzildeki NPC ile konuş — giriş düğümü görev durumuna göre
                    else if (nearestNpc >= 0 && npcDialogOk[nearestNpc])
                    {
                        const char *entry = "start";
                        if (NPCS[nearestNpc].questId != NULL)
                        {
                            int q = quest_find(NPCS[nearestNpc].questId);
                            if (q >= 0)
                            {
                                // FETCH: malzeme tamamsa ACTIVE'de de "done" girişi
                                bool fetchReady = questStatus[q].state == QUEST_ACTIVE &&
                                    QUESTS[q].fetchCount > 0 &&
                                    inventory_count(&inventory, &ITEM_TYPES[QUESTS[q].fetchItem])
                                        >= QUESTS[q].fetchCount;
                                if (fetchReady)
                                    entry = "done";
                                else switch (questStatus[q].state) {
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
                            gatherRes = -1;              // toplama da durur
                            craftOpen = false;           // craft paneli de kapanır
                            craftRecipe = -1;
                            plusForge = false;
                            enchAction = 0;
                            craftProgress = 0.0f;
                        }
                    }
                    // İlan panosu (Faz 6): bounty/dip itibar panoya da kapı
                    else if (!boardOpen &&
                             SDL_sqrtf((BOARD_X - (player.rect.x + player.rect.w / 2.0f)) *
                                       (BOARD_X - (player.rect.x + player.rect.w / 2.0f)) +
                                       (BOARD_Y - (player.rect.y + player.rect.h / 2.0f)) *
                                       (BOARD_Y - (player.rect.y + player.rect.h / 2.0f)))
                                 <= BOARD_RANGE)
                    {
                        int bRegion = region_of(BOARD_X, BOARD_Y);
                        SDL_Color red = { 255, 120, 100, 255 };
                        if (player.bounty[bRegion] > 0 ||
                            player.reputation[bRegion] <= REP_NO_QUESTS)
                        {
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f,
                                                 "Arananlara iş yok!", red);
                        }
                        else
                        {
                            boardOpen = true;
                            player.autoAttack = false;
                            gatherRes = -1;
                            audio_play(SFX_UI);
                        }
                    }
                    // İstasyon kullanımı (NPC yoksa): ÜÇ kapı — izin görevi + bounty + itibar.
                    // permQuest NULL = sahipsiz harabe: kapı yok (keşif ödülü)
                    else if (nearestStation >= 0)
                    {
                        const StationDef *sd = &STATIONS[nearestStation];
                        int pq = (sd->permQuest != NULL) ? quest_find(sd->permQuest) : -1;
                        int stRegion = region_of(sd->x, sd->y);
                        SDL_Color red = { 255, 120, 100, 255 };
                        bool owned = (sd->permQuest != NULL);

                        if (pq >= 0 && questStatus[pq].state != QUEST_REWARDED)
                        {
                            char buf[FLOAT_TEXT_LEN];
                            snprintf(buf, sizeof(buf), "Önce %s ile konuş", sd->ownerName);
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f, buf, red);
                        }
                        else if (owned && player.bounty[stRegion] > 0)
                        {
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f,
                                                 "Önce cezanı öde!", red);
                        }
                        else if (owned && player.reputation[stRegion] <= REP_NO_QUESTS)
                        {
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f,
                                                 "Ustanın sana güveni kalmadı", red);
                        }
                        else
                        {
                            craftOpen = true;
                            craftStation = nearestStation;
                            craftRecipe = -1;
                            plusForge = false;
                            enchAction = 0;
                            craftProgress = 0.0f;
                            player.autoAttack = false;
                            gatherRes = -1;
                            audio_play(SFX_UI);
                        }
                    }
                    // Kaynak toplama (NPC menzilde yoksa): SÜREÇ başlat/bırak.
                    // Araç KUŞANILI olmalı; savurma döngüsü + ilerleme update'te işler.
                    else if (nearestRes >= 0)
                    {
                        const ResourceDef *rd = &RESOURCES[nearestRes];

                        if (gatherRes == nearestRes)
                        {
                            gatherRes = -1;   // E tekrar = süreci bırak (ilerleme durur)
                        }
                        else
                        {
                            bool hasTool = (rd->toolItem < 0) ||
                                equipment.slots[EQUIP_WEAPON].type == &ITEM_TYPES[rd->toolItem];
                            if (!hasTool)
                            {
                                char buf[FLOAT_TEXT_LEN];
                                snprintf(buf, sizeof(buf), "%s kuşan!",
                                         ITEM_TYPES[rd->toolItem].name);
                                SDL_Color red = { 255, 120, 100, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f, buf, red);
                            }
                            else if (player.skills[rd->skill].level < rd->minSkill)
                            {
                                // Nadir düğüm seviye kapısı: bakırla geliş, demire yüksel
                                char buf[FLOAT_TEXT_LEN];
                                snprintf(buf, sizeof(buf), "Tecrüben yetmiyor (%s %d)",
                                         skill_name(rd->skill), rd->minSkill);
                                SDL_Color red = { 255, 120, 100, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f, buf, red);
                            }
                            else
                            {
                                gatherRes = nearestRes;
                                player.autoAttack = false;   // toplama savaşla yarışmaz
                                player.targetMobId = 0;
                                targetNpc = -1;
                            }
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
                    if (!skillsOpen) { treeOpenSkill = -1; treeSelNode = -1; }
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
                else if (event.key.key == SDLK_F10 && gameState == GAME_PLAYING)
                {
                    // DEBUG: perk testi için XP pompala (One-Handed + 3 zırh skill'i)
                    // + meslek testi: craft malzemeleri ve meslek XP'si.
                    // Yayın öncesi kaldırılacak — commit'e girer ama işaretli.
                    // Eğri dik: seviye 25 ≈ 125k, 100 ≈ 4M XP → basış başına 1M
                    // (1 basış ~lv57, 2 ~lv75, 4 = tüm eşikler açık)
                    SkillType dbg[15] = { SKILL_ONE_HANDED, SKILL_LIGHT_ARMOR,
                                          SKILL_MEDIUM_ARMOR, SKILL_HEAVY_ARMOR,
                                          SKILL_SMITHING, SKILL_ALCHEMY, SKILL_MINING,
                                          SKILL_HERBALISM, SKILL_WOODCUTTING,
                                          SKILL_ENCHANTING, SKILL_ARCHERY,
                                          SKILL_SNEAK, SKILL_SPEECH,
                                          SKILL_DESTRUCTION, SKILL_STAFF };   // ağaç testi
                    for (int d = 0; d < 15; d++)
                        if (skill_add_xp(player.skills, dbg[d], 1000000.0f))
                            on_skill_levelup(&player, dbg[d], &hotbar, floatTexts);

                    // Craft test seti: araçlar + bol malzeme
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_PICKAXE], 1, 0);
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_AXE], 1, 0);
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_HAMMER], 1, 0);
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_ORE_COPPER], 20, 0);
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_ORE_IRON], 20, 0);
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_INGOT_COPPER], 10, 0);
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_INGOT_IRON], 10, 0);
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_HERB_RED], 10, 0);
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_HERB_BLUE], 10, 0);
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_WOOD], 10, 0);
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_BP_SWORD_IRON], 1, 0);
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_BP_HELMET_IRON], 1, 0);
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_BP_POISON_OIL], 1, 0);
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_SCROLL_SMITHING], 2, 0);
                    // Faz 4 sunak test seti: özler + tam kademe için taşlar
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_ESSENCE_PALE], 12, 0);
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_ESSENCE_TOXIC], 12, 0);
                    for (int st = ITEM_STONE_STR; st <= ITEM_STONE_STA; st++)
                        inventory_add(&inventory, &ITEM_TYPES[st], 6, 0);
                    // Faz 5 okçuluk test seti
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_BOW], 1, 0);
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_ARROW], 50, 0);
                    // Faz 6 av test seti: deri seti + yemek malzemesi
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_LEATHER], 12, 0);
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_MEAT], 8, 0);
                    // Faz 7 slime test seti: jel + hazır tuzak
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_SLIME_GEL], 8, 0);
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_STICKY_TRAP], 3, 0);
                    // Faz 8 iskelet test seti: kemik + premium ok örnekleri
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_BONE], 6, 0);
                    // Faz 22 büyü test seti: asa + Bilgelik Taşları + havuzlar full
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_STAFF], 1, 0);
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_STONE_INT], 6, 0);
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_POTION_MANA], 5, 0);
                    player.health  = player.maxHealth;
                    player.mana    = player.maxMana;
                    player.stamina = player.maxStamina;
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_ARROW_BONE], 20, 0);
                    inventory_add(&inventory, &ITEM_TYPES[ITEM_ARROW_IRON], 20, 0);
                    player.gold += 500;
                    // Ağaç testi: her ağaca 99 serbest puan (tüm düğümler alınabilsin)
                    tree_set_debug_points(99);
                    SDL_Log("DEBUG F10: +1M XP (savas+meslek) + craft malzeme seti + 500g + 99 agac puani");
                }
                else if (event.key.key == SDLK_F5 && gameState == GAME_PLAYING)
                {
                    // Hızlı kayıt
                    const char *msg = save_game(SAVE_PATH, &player, &inventory, &equipment, &hotbar, questStatus)
                                    ? "Kaydedildi" : "Kayıt HATASI";
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
                        tree_compute_effects(player.treeTaken, player.fateTaken, &perkFx);   // seçimler değişti
                        skill_set_xp_mult(perkFx.xpGainMult);
                        hotbar_purge_closed(&hotbar, perkFx.abilityOpen);
                        hotbar_place_unlocks(&hotbar, perkFx.abilityOpen);
                        if (guardIdx >= 0) guard_init(guardIdx);            // beyin sıfırdan (nöbete)
                        catchOpen = false;
                        lastSeenDay = gametime_day();   // günlük blok tetiklenmesin
                                                        // (pano durumu save'den geldi)
                        SDL_Color white = { 255, 255, 255, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f, "Yüklendi", white);
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
                            {
                                tree_compute_effects(player.treeTaken, player.fateTaken, &perkFx);
                                skill_set_xp_mult(perkFx.xpGainMult);
                                hotbar_purge_closed(&hotbar, perkFx.abilityOpen);
                                hotbar_place_unlocks(&hotbar, perkFx.abilityOpen);
                                if (guardIdx >= 0) guard_init(guardIdx);   // beyin sıfırdan (nöbete)
                                catchOpen = false;
                                lastSeenDay = gametime_day();   // pano durumu save'den geldi
                            }
                            const char *msg = !ok ? ((act == MENU_ACTION_SAVE) ? "Kayıt HATASI" : "Kayıt bulunamadı")
                                            : (act == MENU_ACTION_SAVE) ? "Kaydedildi" : "Yüklendi";
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

                    // -1) Skill ağacı paneli açıksa: düğüm seç / ikinci tıkta AL / respec
                    if (treeOpenSkill >= 0)
                    {
                        float tpx = 40.0f + SKILL_PANEL_W + 10.0f, tpy = 40.0f;
                        int node = tree_panel_node_at(treeOpenSkill,
                                                      (float)event.button.x,
                                                      (float)event.button.y, tpx, tpy);
                        int ts = treeOpenSkill;
                        bool isFate = (ts == TREE_FATE);
                        Uint32 tsTaken = isFate ? player.fateTaken
                                                : player.treeTaken[ts];
                        int tsLevel = 0;
                        if (isFate)
                            for (int sl = 0; sl < SKILL_COUNT; sl++)
                                tsLevel += player.skills[sl].level;
                        else
                            tsLevel = player.skills[ts].level;
                        int earned = tree_points_earned(ts, tsLevel);
                        int spent  = tree_points_spent(ts, tsTaken);

                        if (node == TREE_CLICK_RESPEC)
                        {
                            // Sunak yakınında + harcanmış puan varsa: sıfırla (puan×50g)
                            float adx = STATIONS[2].x - (player.rect.x + player.rect.w / 2.0f);
                            float ady = STATIONS[2].y - (player.rect.y + player.rect.h / 2.0f);
                            bool nearAltar =
                                SDL_sqrtf(adx * adx + ady * ady) <= STATION_INTERACT_RANGE * 1.5f;
                            int cost = spent * 50;
                            SDL_Color red = { 255, 120, 100, 255 };
                            if (!nearAltar || spent <= 0)
                                ;   // satır görünmüyordu zaten — sessiz geç
                            else if (player.gold < cost)
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f,
                                                     "Altın yetersiz!", red);
                            else
                            {
                                player.gold -= cost;
                                if (isFate) player.fateTaken = 0;
                                else        player.treeTaken[ts] = 0;
                                tree_compute_effects(player.treeTaken, player.fateTaken, &perkFx);
                                skill_set_xp_mult(perkFx.xpGainMult);
                                hotbar_purge_closed(&hotbar, perkFx.abilityOpen);
                                hotbar_place_unlocks(&hotbar, perkFx.abilityOpen);
                                treeSelNode = -1;
                                audio_play(SFX_PERK);
                                char buf[FLOAT_TEXT_LEN];
                                snprintf(buf, sizeof(buf), "%s ağacı sıfırlandı (-%dg)",
                                         isFate ? "Kader" : skill_name((SkillType)ts), cost);
                                SDL_Color gold = { 255, 210, 60, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f, buf, gold);
                            }
                            handled = true;
                        }
                        else if (node >= 0)
                        {
                            bool avail = tree_node_available(ts, node, tsTaken,
                                                             earned - spent);
                            if (node == treeSelNode && avail)
                            {
                                // İkinci tık: AL — kalıcı seçim (respec sunağda)
                                if (isFate) player.fateTaken |= (1u << node);
                                else        player.treeTaken[ts] |= (1u << node);
                                tree_compute_effects(player.treeTaken, player.fateTaken, &perkFx);
                                skill_set_xp_mult(perkFx.xpGainMult);
                                hotbar_place_unlocks(&hotbar, perkFx.abilityOpen);
                                const SkillTreeDef *td = tree_get(ts);
                                char buf[FLOAT_TEXT_LEN];
                                snprintf(buf, sizeof(buf), "%s alındı!",
                                         td->nodes[node].name);
                                SDL_Color gold = { 255, 210, 60, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f, buf, gold);
                                audio_play(SFX_PERK);   // törensel arpej
                                fx_burst(&weaponFx,
                                         player.rect.x + player.rect.w / 2.0f,
                                         player.rect.y + player.rect.h / 2.0f,
                                         28, 110.0f, 255, 215, 90);
                            }
                            else
                            {
                                treeSelNode = node;   // ilk tık: incele
                                audio_play(SFX_UI);
                            }
                            handled = true;
                        }
                        // node == -1: panel dışı — K satırlarına düşebilsin (aşağıda)
                    }
                    // 0a) Skill paneli açıkken satıra tıklama → o skill'in ağacını aç
                    if (!handled && skillsOpen)
                    {
                        int row = skill_panel_row_at((float)event.button.x,
                                                     (float)event.button.y, 40.0f, 40.0f);
                        if (row >= 0)
                        {
                            int tid = (row == SKILL_COUNT) ? TREE_FATE : row;
                            if (tree_has_content(tid))
                            {
                                treeOpenSkill = tid;
                                treeSelNode = -1;
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
                            // Fiyat itibara göre: dip itibarda ×1.5, saygın müşteriye ×0.9
                            int shopRegion = region_of(player.rect.x, player.rect.y);
                            int buyPrice = (int)(it->value *
                                                 rep_buy_mult(player.reputation[shopRegion]) *
                                                 perkFx.shopBuyMult);
                            if (player.gold >= buyPrice &&
                                inventory_add(&inventory, it, 1, 0))
                            {
                                player.gold -= buyPrice;
                                rep_add(&player.reputation[shopRegion], REP_SHOPPING); // ekonomik bağ
                                audio_play(SFX_UI);
                                char buf[FLOAT_TEXT_LEN];
                                snprintf(buf, sizeof(buf), "-%d g", buyPrice);
                                SDL_Color red = { 255, 140, 120, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f, buf, red);
                            }
                            handled = true;
                        }
                    }

                    // 0a2) İlan panosu açıksa: satır = kabul/teslim, dışarısı = kapat
                    if (!handled && boardOpen)
                    {
                        int slot = contract_panel_slot_at((float)event.button.x,
                                                          (float)event.button.y, 40.0f, 40.0f);
                        int bRegion = region_of(BOARD_X, BOARD_Y);
                        SDL_Color red = { 255, 120, 100, 255 };
                        SDL_Color goldc = { 255, 210, 60, 255 };

                        if (slot >= 0)
                        {
                            int ci = contract_today(slot);
                            if (ci >= 0 &&
                                player.reputation[bRegion] < CONTRACT_TIER_REP[CONTRACTS[ci].tier])
                            {
                                char buf[FLOAT_TEXT_LEN];
                                snprintf(buf, sizeof(buf), "İtibar %d gerekli",
                                         (int)CONTRACT_TIER_REP[CONTRACTS[ci].tier]);
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f, buf, red);
                            }
                            else if (ci >= 0 && contract_state(slot) == CONTRACT_AVAILABLE)
                            {
                                contract_accept(slot);
                                audio_play(SFX_UI);
                                char buf[FLOAT_TEXT_LEN];
                                snprintf(buf, sizeof(buf), "Kontrat alındı: %s",
                                         CONTRACTS[ci].name);
                                SDL_Color cyan = { 90, 220, 255, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f, buf, cyan);
                            }
                            else if (ci >= 0 && contract_state(slot) == CONTRACT_ACCEPTED)
                            {
                                const ContractDef *c = &CONTRACTS[ci];
                                bool ready = (c->targetMob != NULL)
                                    ? contract_progress(slot) >= c->count
                                    : inventory_count(&inventory,
                                                      &ITEM_TYPES[c->fetchItem]) >= c->count;
                                if (ready)
                                {
                                    if (c->targetMob == NULL)
                                        inventory_consume_n(&inventory,
                                                            &ITEM_TYPES[c->fetchItem], c->count);
                                    contract_turn_in(slot);
                                    int cGold = (int)(c->gold * perkFx.contractGoldMult);
                                    player.gold += cGold;
                                    rep_add(&player.reputation[bRegion],
                                            c->rep * perkFx.contractRepMult);
                                    audio_play(SFX_LOOT);
                                    char buf[FLOAT_TEXT_LEN];
                                    snprintf(buf, sizeof(buf), "+%dg  itibar +%d",
                                             cGold, (int)(c->rep * perkFx.contractRepMult));
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                         player.rect.x + player.rect.w / 2.0f,
                                                         player.rect.y - 10.0f, buf, goldc);
                                }
                                else
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                         player.rect.x + player.rect.w / 2.0f,
                                                         player.rect.y - 10.0f,
                                                         "Henüz hazır değil", red);
                            }
                            handled = true;
                        }
                        else
                        {
                            boardOpen = false;   // panel dışı = kapat
                            handled = true;
                        }
                    }

                    // 0b) Craft paneli açıksa: tarif/basma satırı ya da dışarı tıklama
                    if (!handled && craftOpen && craftStation >= 0)
                    {
                        int hit = craft_panel_recipe_at(craftStation,
                                                        (float)event.button.x,
                                                        (float)event.button.y, 40.0f, 40.0f);
                        SDL_Color red = { 255, 120, 100, 255 };
                        bool smithSt = STATIONS[craftStation].type == STATION_SMITH;
                        bool enchSt  = STATIONS[craftStation].type == STATION_ENCHANT;

                        if (enchSt && hit == CRAFT_ROW_TARGET)
                        {
                            // Hedef döngüsü: dolu ekipman slotları arasında gez
                            for (int step = 1; step <= EQUIP_BOOTS + 1; step++)
                            {
                                EquipSlot cand = (EquipSlot)((enchTarget + step) % (EQUIP_BOOTS + 1));
                                if (equipment.slots[cand].type != NULL)
                                {
                                    enchTarget = cand;
                                    break;
                                }
                            }
                            audio_play(SFX_UI);
                            handled = true;
                        }
                        else if (enchSt && (hit == CRAFT_ROW_RUNE0 || hit == CRAFT_ROW_RUNE1))
                        {
                            // Rün basma: yalnız kuşanılı SİLAHA (garantili — üretimi şanslıydı)
                            int runeItem = (hit == CRAFT_ROW_RUNE0) ? ITEM_RUNE_PINK
                                                                    : ITEM_RUNE_POISON;
                            const ItemStack *w = &equipment.slots[EQUIP_WEAPON];
                            if (w->type == NULL || w->type->weaponType == WEAPON_TOOL)
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f, "Silah kuşan!", red);
                            else if (inventory_count(&inventory, &ITEM_TYPES[runeItem]) < 1)
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f, "Rün gerekli!", red);
                            else
                            {
                                enchAction = (hit == CRAFT_ROW_RUNE0) ? 1 : 2;
                                craftRecipe = -1;
                                craftProgress = 0.0f;
                            }
                            handled = true;
                        }
                        else if (enchSt && hit <= CRAFT_ROW_STAT_BASE &&
                                 hit > CRAFT_ROW_STAT_BASE - STAT_COUNT)
                        {
                            // Stat basma: garantili ama pahalı — kademe × (taş + 2 külçe)
                            int s = CRAFT_ROW_STAT_BASE - hit;   // 0..5
                            static const int stone[STAT_COUNT] = {
                                ITEM_STONE_STR, ITEM_STONE_DEX, ITEM_STONE_HP,
                                ITEM_STONE_DEF, ITEM_STONE_MP, ITEM_STONE_STA,
                                ITEM_STONE_INT };
                            ItemStack *tgt = &equipment.slots[enchTarget];

                            int cur = 0, used = 0;
                            for (int k = 0; k < 2; k++)
                            {
                                if (tgt->statType[k] == s) cur = tgt->statTier[k];
                                if (tgt->statType[k] >= 0) used++;
                            }
                            int next = cur + 1;

                            if (tgt->type == NULL)
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f, "Hedef boş!", red);
                            else if (cur >= STAT_MAX_TIER)
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f, "Zaten tavanda", red);
                            else if (cur == 0 && used >= 2)
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f, "Slotlar dolu (2/2)", red);
                            else if (inventory_count(&inventory, &ITEM_TYPES[stone[s]]) < next ||
                                     inventory_count(&inventory, &ITEM_TYPES[ITEM_INGOT_IRON]) < next * 2)
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f, "Malzeme eksik!", red);
                            else
                            {
                                enchAction = 10 + s;
                                craftRecipe = -1;
                                craftProgress = 0.0f;
                            }
                            handled = true;
                        }
                        else if (hit == CRAFT_ROW_PLUS && smithSt)
                        {
                            // '+basma': GERÇEK silah kuşanılı + Çekiç envanterde +
                            // (plus+1) Demir Külçe. Savurma kuşanılı silahla yapılır
                            // (dövülen kılıcın kendisi — Çekiç kuşanım istisnası).
                            ItemStack *w = &equipment.slots[EQUIP_WEAPON];
                            int cost = w->plusLevel + 1;
                            if (w->type == NULL || w->type->weaponType == WEAPON_TOOL)
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f, "Silah kuşan!", red);
                            else if (w->plusLevel >= ITEM_MAX_PLUS)
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f, "Zaten tavanda", red);
                            else if (inventory_count(&inventory, &ITEM_TYPES[ITEM_HAMMER]) < 1)
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f, "Çekiç gerekli!", red);
                            else if (inventory_count(&inventory,
                                                     &ITEM_TYPES[ITEM_INGOT_IRON]) < cost)
                            {
                                char buf[FLOAT_TEXT_LEN];
                                snprintf(buf, sizeof(buf), "%dx Demir Külçe gerekli!", cost);
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f, buf, red);
                            }
                            else
                            {
                                plusForge = true;
                                craftRecipe = -1;
                                craftProgress = 0.0f;
                            }
                            handled = true;
                        }
                        else if (hit >= 0)
                        {
                            const RecipeDef *r = &RECIPES[hit];
                            SkillType sk = station_skill(r->station);

                            if (!craft_recipe_learned(hit))
                            {
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f,
                                                     "Blueprint gerekli!", red);
                                handled = true;
                            }
                            else
                            {
                                bool haveMats =
                                    inventory_count(&inventory, &ITEM_TYPES[r->in1Item]) >= r->in1Count &&
                                    (r->in2Item < 0 ||
                                     inventory_count(&inventory, &ITEM_TYPES[r->in2Item]) >= r->in2Count);

                                if (player.skills[sk].level < r->minSkill)
                                {
                                    char buf[FLOAT_TEXT_LEN];
                                    snprintf(buf, sizeof(buf), "%s %d gerekli!",
                                             skill_name(sk), r->minSkill);
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                         player.rect.x + player.rect.w / 2.0f,
                                                         player.rect.y - 10.0f, buf, red);
                                }
                                else if (!haveMats)
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                         player.rect.x + player.rect.w / 2.0f,
                                                         player.rect.y - 10.0f, "Malzeme eksik!", red);
                                else if (smithSt && equipment.slots[EQUIP_WEAPON].type !=
                                                    &ITEM_TYPES[ITEM_HAMMER])
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                         player.rect.x + player.rect.w / 2.0f,
                                                         player.rect.y - 10.0f, "Çekiç kuşan!", red);
                                else
                                {
                                    craftRecipe = hit;   // süreç başlar (simya: kaynama;
                                    plusForge = false;   // ocak: çekiç savurması)
                                    enchAction = 0;
                                    craftProgress = 0.0f;
                                }
                                handled = true;
                            }
                        }
                        else
                        {
                            // Panel dışına tıklama: kapat (süreç iptal — malzeme yanmaz)
                            if (craftRecipe >= 0 || plusForge || enchAction > 0)
                            {
                                SDL_Color dim2 = { 255, 170, 120, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f,
                                                     "İşlem iptal edildi", dim2);
                            }
                            craftOpen = false;
                            craftRecipe = -1;
                            plusForge = false;
                            enchAction = 0;
                            craftProgress = 0.0f;
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
                            player.gold += (int)(l->gold * perkFx.goldFindMult);
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
                                if (inventory_add_stack(&inventory, stack))
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

                    // 2a) NPC'ye tıklama = saldırı hedefi (suç sistemi).
                    //     Bilinçli eylem: Z asla NPC seçmez, yalnız tıklama seçer.
                    if (!handled && gameState == GAME_PLAYING)
                    {
                        int ni = npc_at_point(worldX, worldY);
                        if (ni >= 0 && ni != dialogNpc)   // konuşulan NPC'yi tıklamak saldırı değil
                        {
                            targetNpc = ni;
                            player.targetMobId = 0;   // tek hedef ilkesi
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

                        // 1) Once ekipman paneline mi tiklandi? (panelY = 20 — render ile aynı;
                        //    35 slotluk çanta 720px pencereye ancak böyle sığıyor)
                        EquipSlot eqSlot = equipment_slot_at((float)event.button.x,
                                                             (float)event.button.y, panelX, 20);
                        if (eqSlot != EQUIP_NONE)
                        {
                            unequip_slot(&equipment, &inventory, eqSlot); // cikar → envantere
                        }
                        else
                        {
                            // 2) Degilse envantere mi? (panelY = 20+340)
                            int slot = inventory_slot_at((float)event.button.x, (float)event.button.y,
                                                         (float)panelX, (float)(20 + 340));
                            if (slot >= 0)
                            {
                                ItemStack *st = &inventory.slots[slot];
                                if (shopOpen && st->type != NULL)
                                {
                                    // Dükkan açıkken sağ tık = SAT (alışın yarısı × itibar, plus dahil)
                                    int sellRegion = region_of(player.rect.x, player.rect.y);
                                    int price = (int)(item_value(st) / 2 *
                                                      rep_sell_mult(player.reputation[sellRegion]) *
                                                      perkFx.shopSellMult);
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
                                else if (st->type != NULL && st->type->scrollXp > 0.0f)
                                {
                                    // Öğretici parşömen: sağ tık = kullan (+XP, tükenir)
                                    SkillType sk = (SkillType)st->type->scrollSkill;
                                    float xp = st->type->scrollXp;
                                    const ItemType *sct = st->type;
                                    if (inventory_consume(&inventory, sct))
                                    {
                                        if (skill_add_xp(player.skills, sk, xp))
                                            on_skill_levelup(&player, sk, &hotbar, floatTexts);
                                        audio_play(SFX_UI);
                                        char buf[FLOAT_TEXT_LEN];
                                        snprintf(buf, sizeof(buf), "+%d XP %s",
                                                 (int)xp, skill_name(sk));
                                        SDL_Color cyan = { 90, 220, 255, 255 };
                                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                             player.rect.x + player.rect.w / 2.0f,
                                                             player.rect.y - 10.0f, buf, cyan);
                                    }
                                }
                                else if (st->type != NULL &&
                                         st->type >= &ITEM_TYPES[ITEM_BP_SWORD_IRON] &&
                                         st->type <= &ITEM_TYPES[ITEM_BP_POISON_OIL])
                                {
                                    // Blueprint: sağ tık = tarifi KALICI öğren (tükenir)
                                    int bpIdx = (int)(st->type - ITEM_TYPES);
                                    bool learnedNew = false;
                                    const char *rName = NULL;
                                    for (int ri = 0; ri < RECIPE_COUNT && ri < MAX_RECIPES; ri++)
                                        if (RECIPES[ri].blueprintItem == bpIdx &&
                                            !craft_recipe_learned(ri))
                                        {
                                            craft_learn_recipe(ri);
                                            learnedNew = true;
                                            rName = RECIPES[ri].name;
                                        }
                                    if (learnedNew)
                                    {
                                        inventory_consume(&inventory, &ITEM_TYPES[bpIdx]);
                                        audio_play(SFX_UI);
                                        char buf[FLOAT_TEXT_LEN];
                                        snprintf(buf, sizeof(buf), "Tarif öğrenildi: %s", rName);
                                        SDL_Color cyan = { 90, 220, 255, 255 };
                                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                             player.rect.x + player.rect.w / 2.0f,
                                                             player.rect.y - 10.0f, buf, cyan);
                                    }
                                    else
                                    {
                                        SDL_Color dim = { 170, 170, 190, 255 };
                                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                             player.rect.x + player.rect.w / 2.0f,
                                                             player.rect.y - 10.0f,
                                                             "Zaten biliyorsun", dim);
                                    }
                                }
                                else if (st->type != NULL && st->type->ammoDmgMult > 0.0f)
                                {
                                    // Ok yığınına sağ tık: aktif mühimmat seçimi (Faz 8)
                                    activeAmmo = (int)(st->type - ITEM_TYPES);
                                    audio_play(SFX_UI);
                                    char buf[FLOAT_TEXT_LEN];
                                    snprintf(buf, sizeof(buf), "Aktif ok: %s",
                                             st->type->name);
                                    SDL_Color cyan = { 90, 220, 255, 255 };
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                         player.rect.x + player.rect.w / 2.0f,
                                                         player.rect.y - 10.0f, buf, cyan);
                                }
                                else if (st->type == &ITEM_TYPES[ITEM_WOOD])
                                {
                                    // Odun'a sağ tık: kamp ateşi kurulumu (3 Odun, 3 sn)
                                    SDL_Color red = { 255, 120, 100, 255 };
                                    if (inventory_count(&inventory, &ITEM_TYPES[ITEM_WOOD])
                                            < CAMPFIRE_WOOD_COST)
                                    {
                                        char buf[FLOAT_TEXT_LEN];
                                        snprintf(buf, sizeof(buf), "Odun gerekli (%d)",
                                                 CAMPFIRE_WOOD_COST);
                                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                             player.rect.x + player.rect.w / 2.0f,
                                                             player.rect.y - 10.0f, buf, red);
                                    }
                                    else
                                    {
                                        campBuild = 0.0f;          // kurulum başlar
                                        inventoryOpen = false;     // süreç görünür olsun
                                        player.autoAttack = false;
                                        gatherRes = -1;
                                        audio_play(SFX_UI);
                                    }
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
                    // Skill vuruşu: silah türü beceriyle eşleşmeli (PS/QS melee,
                    // GO/BO/GS/YA/AO yay) — yanlış silahla CD boşa yanmasın
                    const AbilityDef *ad = &ABILITIES[hs->abilityId];
                    bool needBow = (ad->skill == SKILL_ARCHERY);
                    bool isSpell = (ad->skill == SKILL_DESTRUCTION);
                    SDL_Color red = { 255, 120, 100, 255 };

                    if (isSpell)
                    {
                        // BÜYÜ: silahtan bağımsız (battlemage özgürlüğü) — mana + hedef
                        float cost = ad->mana * perkFx.manaCostMult;
                        bool needTarget = (hs->abilityId == ABILITY_FIREBOLT ||
                                           hs->abilityId == ABILITY_LIGHTNING);
                        Mob *spTarget = mob_find_by_id(mobs, mobCount, player.targetMobId);
                        float spDist = 1e9f;
                        if (spTarget != NULL)
                        {
                            float sdx = (spTarget->e.rect.x + spTarget->e.rect.w / 2.0f)
                                      - (player.rect.x + player.rect.w / 2.0f);
                            float sdy = (spTarget->e.rect.y + spTarget->e.rect.h / 2.0f)
                                      - (player.rect.y + player.rect.h / 2.0f);
                            spDist = SDL_sqrtf(sdx * sdx + sdy * sdy);
                        }

                        if (player.abilityCd[hs->abilityId] > 0.0f)
                            ;   // CD sürüyor — bar perdesi gösteriyor
                        else if (player.mana < cost)
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f,
                                                 "Manan yetmiyor!", red);
                        else if (needTarget && (spTarget == NULL || spDist > SPELL_RANGE))
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f,
                                                 spTarget == NULL ? "Hedef gerekli!"
                                                                  : "Menzil dışı!", red);
                        else if (!player.attacking && !player.shooting && castSpell < 0)
                        {
                            castSpell = (int)hs->abilityId;   // cast başlar (köklü)
                            castTimer = 0.0f;
                            player.autoAttack = false;
                            audio_play(SFX_UI);
                        }
                    }
                    else if (needBow != player.bowEquipped)
                    {
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f,
                                             needBow ? "Yay gerekli!"
                                                     : "Yakın dövüş silahı gerekli", red);
                    }
                    else if (player.abilityCd[hs->abilityId] > 0.0f)
                    {
                        // CD sürüyor — sessiz (bar perdesi zaten gösteriyor)
                    }
                    else if (player.stamina < ad->stamina *
                                 (needBow ? perkFx.bowStaminaMult : 1.0f))
                    {
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f,
                                             "Nefesin yetmiyor!", red);
                    }
                    else if (hs->abilityId == ABILITY_DISENGAGE)
                    {
                        // Geri Sıçrama: ok atmaz — hedeften (yoksa bakış yönünden)
                        // ters yöne anında kaçış. Köklü okçunun nefes alma aracı.
                        float ddx = 0.0f, ddy = 0.0f;
                        Mob *dTarget = mob_find_by_id(mobs, mobCount, player.targetMobId);
                        if (dTarget != NULL)
                        {
                            ddx = (player.rect.x + player.rect.w / 2.0f)
                                - (dTarget->e.rect.x + dTarget->e.rect.w / 2.0f);
                            ddy = (player.rect.y + player.rect.h / 2.0f)
                                - (dTarget->e.rect.y + dTarget->e.rect.h / 2.0f);
                        }
                        float dlen = SDL_sqrtf(ddx * ddx + ddy * ddy);
                        if (dlen < 1.0f)   // hedef yok/üstümüzde: bakışın tersine
                        {
                            ddx = (player.facing == DIR_LEFT) ? 1.0f
                                : (player.facing == DIR_RIGHT) ? -1.0f : 0.0f;
                            ddy = (player.facing == DIR_UP) ? 1.0f
                                : (player.facing == DIR_DOWN) ? -1.0f : 0.0f;
                            dlen = 1.0f;
                        }
                        ddx /= dlen; ddy /= dlen;

                        // 140px'e kadar 14px adımlarla ilerle — katıya çarpınca dur
                        float moved = 0.0f;
                        while (moved < 140.0f + perkFx.disengageBonus)
                        {
                            SDL_FRect nf = feet_box(player.rect.x + ddx * 14.0f,
                                                    player.rect.y + ddy * 14.0f,
                                                    player.rect.w, player.rect.h);
                            if (box_hits_solid(nf) || decor_blocks(nf)) break;
                            player.rect.x += ddx * 14.0f;
                            player.rect.y += ddy * 14.0f;
                            moved += 14.0f;
                        }
                        player.shooting = false;   // sıçrama çekmeyi bozar
                        player.stamina -= ad->stamina * perkFx.bowStaminaMult;
                        player.abilityCd[ABILITY_DISENGAGE] = ad->cooldown;
                        audio_play(SFX_DODGE);
                        fx_burst(&weaponFx,
                                 player.rect.x + player.rect.w / 2.0f,
                                 player.rect.y + player.rect.h, 10, 60.0f,
                                 200, 190, 160);   // toz bulutu
                    }
                    else if (!player.attacking && !player.shooting)
                    {
                        attackPressed = true;
                        player.pendingAbility = (int)hs->abilityId;
                    }
                }
                else if (hs->kind == HOTBAR_ITEM)
                {
                    // Tüketilebilir: tür CD'deyse / (iksirse can fulse) / yoksa geç
                    int typeIdx = (int)(hs->itemType - ITEM_TYPES);
                    bool heals = hs->itemType->healAmount > 0.0f;
                    bool manas = hs->itemType->manaAmount > 0.0f;
                    if (player.itemCd[typeIdx] <= 0.0f &&
                        (!heals || player.health < player.maxHealth) &&
                        (!manas || player.mana < player.maxMana) &&
                        inventory_consume(&inventory, hs->itemType))
                    {
                        // Akıcı: CD kısalır; İlk Yardım: iyileşme artar
                        player.itemCd[typeIdx] = hs->itemType->useCooldown * perkFx.potionCdMult;
                        audio_play(SFX_POTION);
                        char buf[FLOAT_TEXT_LEN];
                        SDL_Color green = { 120, 255, 120, 255 };

                        if (heals)
                        {
                            player.health += hs->itemType->healAmount * perkFx.potionEffMult;
                            if (player.health > player.maxHealth)
                                player.health = player.maxHealth;
                            snprintf(buf, sizeof(buf), "+%d HP",
                                     (int)hs->itemType->healAmount);
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f, buf, green);
                        }
                        if (manas)
                        {
                            player.mana += hs->itemType->manaAmount;
                            if (player.mana > player.maxMana)
                                player.mana = player.maxMana;
                            snprintf(buf, sizeof(buf), "+%d MP",
                                     (int)hs->itemType->manaAmount);
                            SDL_Color blue = { 120, 170, 255, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f, buf, blue);
                        }
                        if (hs->itemType->buffDuration > 0.0f)
                        {
                            // Hız buff'ı (süre tazelenir, üst üste BİNMEZ)
                            player.speedBuffMult = hs->itemType->buffSpeedMult;
                            player.speedBuffTimer = hs->itemType->buffDuration;
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f, "Hızlandın!", green);
                        }
                        if (hs->itemType->coatDuration > 0.0f)
                        {
                            // Silah kaplaması: süreli weaponFx (rün sisteminin öncüsü)
                            player.coatFx = hs->itemType->coatFx;
                            player.coatTimer = hs->itemType->coatDuration;
                            SDL_Color pgreen = { 120, 230, 90, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f,
                                                 "Silah kaplandı!", pgreen);
                        }
                        if (hs->itemType->foodDuration > 0.0f)
                        {
                            // Yemek: yavaş yenilenme (süre tazelenir, üst üste binmez)
                            player.foodRegenRate = hs->itemType->foodRegen;
                            player.foodTimer = hs->itemType->foodDuration;
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f, "Karnın doydu", green);
                        }
                        if (hs->itemType->trapItem)
                        {
                            // Yapışkan Tuzağı: ayak önüne kur (dolu ise en eskisi ezilir)
                            traps[nextTrap] = (GooTrap){
                                .active = true,
                                .x = player.rect.x + player.rect.w / 2.0f,
                                .y = player.rect.y + player.rect.h + 6.0f,
                            };
                            nextTrap = (nextTrap + 1) % MAX_TRAPS;
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f,
                                                 "Tuzak kuruldu", green);
                        }
                    }
                }
            }

            // --- TOPLAMA SÜRECİ: savurma döngüsü + iptaller ---
            if (gatherRes >= 0)
            {
                float gpx = player.rect.x + player.rect.w / 2.0f;
                float gpy = player.rect.y + player.rect.h / 2.0f;
                float gdx = RESOURCES[gatherRes].x - gpx;
                float gdy = RESOURCES[gatherRes].y - gpy;
                bool moving = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_A] ||
                              keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_D];

                if (!resource_available(gatherRes) || moving ||
                    SDL_sqrtf(gdx * gdx + gdy * gdy) > RESOURCE_INTERACT_RANGE * 1.4f)
                {
                    gatherRes = -1;   // hareket / düğüm bitti / uzaklaştı → süreç durur
                }
                else if (RESOURCES[gatherRes].toolItem < 0)
                {
                    // EL İLE toplama (bitkiler): savurma yok, bar zamanla dolar
                    // (toplama animasyonu bulunursa buraya bağlanacak)
                    const ResourceDef *rd = &RESOURCES[gatherRes];
                    resource_add_progress(gatherRes,
                        gather_mult(player.skills[rd->skill].level) *
                        ((rd->skill == SKILL_MINING)      ? perkFx.mineSpeedMult
                       : (rd->skill == SKILL_HERBALISM)   ? perkFx.herbSpeedMult
                       : (rd->skill == SKILL_WOODCUTTING) ? perkFx.woodSpeedMult
                                                          : 1.0f) * dt);
                    if (resource_progress(gatherRes) >= rd->work)
                    {
                        gather_finish(gatherRes, &player, &inventory, &hotbar, floatTexts);
                        gatherRes = -1;
                    }
                }
                else if (!player.attacking && !attackPressed)
                {
                    attackPressed = true;          // sıradaki savurma
                    player.pendingAbility = -1;
                }
            }

            // İlan panosu: yürüyünce kapanır (craft paneliyle aynı davranış)
            if (boardOpen && (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_A] ||
                              keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_D]))
                boardOpen = false;

            // Kamp ateşi ömrü: 24 oyun saati dolunca söner
            if (STATIONS[STATION_PLAYER_FIRE_IDX].x > CAMPFIRE_PARKED + 1.0f &&
                campfireOutAt >= 0.0f)
            {
                int cfDay; float cfHours;
                gametime_get(&cfDay, &cfHours);
                if ((float)cfDay * 24.0f + cfHours >= campfireOutAt)
                {
                    STATIONS[STATION_PLAYER_FIRE_IDX].x = CAMPFIRE_PARKED;
                    STATIONS[STATION_PLAYER_FIRE_IDX].y = CAMPFIRE_PARKED;
                    campfireOutAt = -1.0f;
                    // Ateşin başındaysa panel açık kalmasın (istasyon kayboldu)
                    if (craftOpen && craftStation == STATION_PLAYER_FIRE_IDX)
                    {
                        craftOpen = false;
                        craftRecipe = -1;
                        craftProgress = 0.0f;
                    }
                }
            }

            // --- BÜYÜ CAST'İ: köklü — hareket bozar (mana yanmaz); commit'te etki ---
            if (castSpell >= 0)
            {
                bool csMoving = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_A] ||
                                keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_D];
                if (csMoving)
                {
                    castSpell = -1;
                    SDL_Color dim = { 255, 170, 120, 255 };
                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                         player.rect.x + player.rect.w / 2.0f,
                                         player.rect.y - 10.0f, "Büyü bozuldu", dim);
                }
                else
                {
                    castTimer += dt;
                    if (randf() < dt * 14.0f)   // kanalize kıvılcımları
                        fx_spawn(&weaponFx,
                                 player.rect.x + player.rect.w / 2.0f + (randf() - 0.5f) * 24.0f,
                                 player.rect.y + player.rect.h / 2.0f - 10.0f - randf() * 20.0f,
                                 (randf() - 0.5f) * 15.0f, -25.0f - randf() * 20.0f,
                                 0.4f, 5.0f + randf() * 5.0f, 190, 120, 235, WEAPON_FX_NONE);

                    if (castTimer >= SPELL_CAST_TIME * perkFx.castSpeedMult)
                    {
                        const AbilityDef *sd = &ABILITIES[castSpell];
                        float cost = sd->mana * perkFx.manaCostMult;
                        const ItemStack *wpn = &equipment.slots[EQUIP_WEAPON];
                        float pcx = player.rect.x + player.rect.w / 2.0f;
                        float pcy = player.rect.y + player.rect.h / 2.0f;

                        player.mana -= cost;
                        if (player.mana < 0.0f) player.mana = 0.0f;
                        player.abilityCd[castSpell] = sd->cooldown;
                        for (int b = 0; b < MAX_BURSTS; b++)
                            if (!bursts[b].active)
                            {
                                bursts[b] = (BurstFx){ true, pcx, pcy - 20.0f, 0.0f, 0.7f };
                                break;
                            }

                        if (castSpell == ABILITY_FIREBOLT)
                        {
                            // Güdümlü ateş mermisi (hasar varışta)
                            for (int sp = 0; sp < MAX_SPELL_PROJS; sp++)
                            {
                                if (spellProjs[sp].active) continue;
                                spellProjs[sp] = (SpellProj){
                                    .active = true, .x = pcx, .y = pcy - 14.0f,
                                    .targetId = player.targetMobId,
                                };
                                break;
                            }
                            audio_play(SFX_HIT);
                        }
                        else if (castSpell == ABILITY_ICE_NOVA)
                        {
                            // ÇEVRENE buz patlaması: hasar + alan yavaşlatma
                            float novaR = 120.0f + perkFx.novaRadiusBonus;
                            fx_burst(&weaponFx, pcx, pcy, 26, 150.0f, 150, 210, 255);
                            audio_play(SFX_DODGE);   // buz whoosh
                            for (int m2 = 0; m2 < mobCount; m2++)
                            {
                                Mob *nb = &mobs[m2];
                                if (!nb->e.alive) continue;
                                float ndx = (nb->e.rect.x + nb->e.rect.w / 2.0f) - pcx;
                                float ndy = (nb->e.rect.y + nb->e.rect.h / 2.0f) - pcy;
                                if (ndx * ndx + ndy * ndy > novaR * novaR) continue;
                                HitResult sh = combat_spell_hit(&player, wpn,
                                                                sd->damageScale,
                                                                nb->type->defense, &perkFx);
                                nb->e.health -= sh.damage;
                                nb->e.hitFlash = 0.15f;
                                nb->slowMult = 0.5f;
                                nb->slowTimer = 4.0f;
                                nb->aggro = true;
                                floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                                nb->e.rect.x + nb->e.rect.w / 2.0f + 10.0f,
                                                nb->e.rect.y, (int)sh.damage, sh.crit);
                                float sxp = combat_xp_for_hit(sh.damage, nb->type->level,
                                                player.skills[SKILL_DESTRUCTION].level);
                                if (skill_add_xp(player.skills, SKILL_DESTRUCTION, sxp))
                                    on_skill_levelup(&player, SKILL_DESTRUCTION,
                                                     &hotbar, floatTexts);
                                if (nb->e.health <= 0.0f)
                                    mob_die(nb, mobs, MAX_MOBS, &nextMobId, loots,
                                            questStatus, floatTexts, &player);
                            }
                        }
                        else if (castSpell == ABILITY_LIGHTNING)
                        {
                            // Anında çarpma (hedef cast sırasında ölmediyse)
                            Mob *lt = mob_find_by_id(mobs, mobCount, player.targetMobId);
                            if (lt != NULL)
                            {
                                float tx = lt->e.rect.x + lt->e.rect.w / 2.0f;
                                float ty = lt->e.rect.y + lt->e.rect.h / 2.0f;
                                HitResult sh = combat_spell_hit(&player, wpn,
                                                                sd->damageScale,
                                                                lt->type->defense, &perkFx);
                                lt->e.health -= sh.damage;
                                lt->e.hitFlash = 0.2f;
                                lt->aggro = true;
                                mob_alert_camp(mobs, mobCount, lt);
                                fx_burst(&weaponFx, tx, ty, 18, 160.0f, 250, 250, 160);
                                for (int b = 0; b < MAX_BURSTS; b++)
                                    if (!bursts[b].active)
                                    {
                                        bursts[b] = (BurstFx){ true, tx, ty, 0.0f, 1.2f };
                                        break;
                                    }
                                audio_play(SFX_CRIT);
                                floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                                tx + 10.0f, lt->e.rect.y,
                                                (int)sh.damage, sh.crit);
                                float sxp = combat_xp_for_hit(sh.damage, lt->type->level,
                                                player.skills[SKILL_DESTRUCTION].level);
                                if (skill_add_xp(player.skills, SKILL_DESTRUCTION, sxp))
                                    on_skill_levelup(&player, SKILL_DESTRUCTION,
                                                     &hotbar, floatTexts);
                                if (lt->e.health <= 0.0f)
                                    mob_die(lt, mobs, MAX_MOBS, &nextMobId, loots,
                                            questStatus, floatTexts, &player);
                            }
                        }
                        else if (castSpell == ABILITY_FLAME_ARMOR)
                        {
                            player.flameArmorTimer = 15.0f;
                            fx_burst(&weaponFx, pcx, pcy, 20, 90.0f, 255, 150, 50);
                            audio_play(SFX_PERK);
                            SDL_Color warm = { 255, 170, 80, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 pcx, player.rect.y - 10.0f,
                                                 "Alev Zırhı!", warm);
                        }
                        else if (castSpell == ABILITY_MAGIC_SHIELD)
                        {
                            // Büyü Kalkanı: hasar HP yerine manadan emilir (×1.2)
                            player.manaShieldTimer = 12.0f;
                            player.shieldBreakT = 0.0f;
                            audio_play(SFX_PERK);
                            SDL_Color icy = { 150, 220, 255, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 pcx, player.rect.y - 10.0f,
                                                 "Büyü Kalkanı!", icy);
                        }
                        else if (castSpell == ABILITY_SPIKES)
                        {
                            // Toprak Dikenleri: çevrende patlama + yavaşlatma
                            float spikeR = 130.0f;
                            audio_play(SFX_HIT);
                            for (int k = 0; k < 3; k++)   // 3 diken kümesi görseli
                            {
                                float ang = (float)k * 2.094f + randf();
                                for (int q = 0; q < MAX_SPIKE_FX; q++)
                                    if (!spikeFx[q].active)
                                    {
                                        spikeFx[q] = (SpikeFxAnim){ true,
                                            pcx + SDL_cosf(ang) * 70.0f,
                                            pcy + SDL_sinf(ang) * 45.0f, 0.0f };
                                        break;
                                    }
                            }
                            for (int m2 = 0; m2 < mobCount; m2++)
                            {
                                Mob *nb = &mobs[m2];
                                if (!nb->e.alive) continue;
                                float ndx = (nb->e.rect.x + nb->e.rect.w / 2.0f) - pcx;
                                float ndy = (nb->e.rect.y + nb->e.rect.h / 2.0f) - pcy;
                                if (ndx * ndx + ndy * ndy > spikeR * spikeR) continue;
                                HitResult sh = combat_spell_hit(&player, wpn,
                                                                sd->damageScale,
                                                                nb->type->defense, &perkFx);
                                nb->e.health -= sh.damage;
                                nb->e.hitFlash = 0.15f;
                                nb->slowMult = 0.35f;   // dikenlere saplanma hissi
                                nb->slowTimer = 1.5f;
                                nb->aggro = true;
                                floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                                nb->e.rect.x + nb->e.rect.w / 2.0f + 10.0f,
                                                nb->e.rect.y, (int)sh.damage, sh.crit);
                                float sxp = combat_xp_for_hit(sh.damage, nb->type->level,
                                                player.skills[SKILL_DESTRUCTION].level);
                                if (skill_add_xp(player.skills, SKILL_DESTRUCTION, sxp))
                                    on_skill_levelup(&player, SKILL_DESTRUCTION,
                                                     &hotbar, floatTexts);
                                if (nb->e.health <= 0.0f)
                                    mob_die(nb, mobs, MAX_MOBS, &nextMobId, loots,
                                            questStatus, floatTexts, &player);
                            }
                        }

                        castSpell = -1;
                        castTimer = 0.0f;
                    }
                }
            }

            // Ateş Topu mermileri: hedefe güdümlü, varışta büyü vuruşu + YANMA
            for (int sp = 0; sp < MAX_SPELL_PROJS; sp++)
            {
                if (!spellProjs[sp].active) continue;
                Mob *ft = mob_find_by_id(mobs, mobCount, spellProjs[sp].targetId);
                if (ft == NULL) { spellProjs[sp].active = false; continue; }
                float tx = ft->e.rect.x + ft->e.rect.w / 2.0f;
                float ty = ft->e.rect.y + ft->e.rect.h / 2.0f;
                float fdx = tx - spellProjs[sp].x, fdy = ty - spellProjs[sp].y;
                float fdist = SDL_sqrtf(fdx * fdx + fdy * fdy);
                float fstep = SPELL_PROJ_SPEED * dt;

                // Ateş izi
                if (randf() < dt * 40.0f)
                    fx_spawn(&weaponFx, spellProjs[sp].x, spellProjs[sp].y,
                             (randf() - 0.5f) * 25.0f, (randf() - 0.5f) * 25.0f,
                             0.3f, 6.0f + randf() * 5.0f, 255, 160, 60, WEAPON_FX_NONE);

                if (fdist > fstep + 8.0f)
                {
                    spellProjs[sp].x += fdx / fdist * fstep;
                    spellProjs[sp].y += fdy / fdist * fstep;
                    continue;
                }

                spellProjs[sp].active = false;
                const ItemStack *wpn2 = &equipment.slots[EQUIP_WEAPON];
                HitResult sh = combat_spell_hit(&player, wpn2,
                                                ABILITIES[ABILITY_FIREBOLT].damageScale,
                                                ft->type->defense, &perkFx);
                ft->e.health -= sh.damage;
                ft->e.hitFlash = 0.15f;
                ft->aggro = true;
                mob_alert_camp(mobs, mobCount, ft);
                audio_play(sh.crit ? SFX_CRIT : SFX_HIT);
                fx_burst(&weaponFx, tx, ty, 12, 100.0f, 255, 150, 50);

                // YANMA: 3. bağımsız DoT kanalı (Közleme/Yangın düğümleri büyütür)
                ft->burnDps = sh.damage * 0.30f * (1.0f + perkFx.burnBonus) / 3.0f;
                ft->burnTime = 3.0f;

                floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                tx + 10.0f, ft->e.rect.y, (int)sh.damage, sh.crit);
                float sxp = combat_xp_for_hit(sh.damage, ft->type->level,
                                player.skills[SKILL_DESTRUCTION].level);
                if (skill_add_xp(player.skills, SKILL_DESTRUCTION, sxp))
                    on_skill_levelup(&player, SKILL_DESTRUCTION, &hotbar, floatTexts);
                if (ft->e.health <= 0.0f)
                    mob_die(ft, mobs, MAX_MOBS, &nextMobId, loots, questStatus,
                            floatTexts, &player);
            }

            // --- KAMP ATEŞİ KURULUMU: 3 sn süreç, hareket iptal eder ---
            if (campBuild >= 0.0f)
            {
                bool cbMoving = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_A] ||
                                keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_D];
                if (cbMoving)
                {
                    campBuild = -1.0f;   // odun yanmadı — tekrar denenebilir
                    SDL_Color dim = { 255, 170, 120, 255 };
                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                         player.rect.x + player.rect.w / 2.0f,
                                         player.rect.y - 10.0f,
                                         "İşlem iptal edildi", dim);
                }
                else
                {
                    campBuild += dt;
                    if (campBuild >= CAMPFIRE_BUILD_TIME)
                    {
                        campBuild = -1.0f;
                        if (inventory_consume_n(&inventory, &ITEM_TYPES[ITEM_WOOD],
                                                CAMPFIRE_WOOD_COST))
                        {
                            // Yenisi eskisini söndürür (tek aktif oyuncu ateşi)
                            STATIONS[STATION_PLAYER_FIRE_IDX].x =
                                player.rect.x + player.rect.w / 2.0f;
                            STATIONS[STATION_PLAYER_FIRE_IDX].y =
                                player.rect.y + player.rect.h + 8.0f;   // ayak önü

                            // Ömür: şu andan 24 oyun saati sonra söner
                            int cfDay; float cfHours;
                            gametime_get(&cfDay, &cfHours);
                            campfireOutAt = (float)cfDay * 24.0f + cfHours + 24.0f;
                            audio_play(SFX_PERK);
                            SDL_Color warm = { 255, 190, 90, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f,
                                                 "Kamp ateşi yandı", warm);
                            fx_burst(&weaponFx,
                                     STATIONS[STATION_PLAYER_FIRE_IDX].x,
                                     STATIONS[STATION_PLAYER_FIRE_IDX].y - 20.0f,
                                     14, 80.0f, 255, 170, 60);
                        }
                    }
                }
            }

            // --- CRAFT SÜRECİ: ocak = çekiç savurması, kazan = kaynatma süresi ---
            if (craftOpen && craftStation >= 0)
            {
                bool moving = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_A] ||
                              keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_D];
                if (moving)
                {
                    // Aktif süreç sessizce ölmesin: iptal görünür olsun
                    if (craftRecipe >= 0 || plusForge || enchAction > 0)
                    {
                        SDL_Color dim = { 255, 170, 120, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f,
                                             "İşlem iptal edildi", dim);
                    }
                    craftOpen = false;   // yürüyünce panel + süreç kapanır (malzeme yanmaz)
                    craftRecipe = -1;
                    plusForge = false;
                    enchAction = 0;
                    craftProgress = 0.0f;
                }
                else if (craftRecipe >= 0 &&
                         STATIONS[craftStation].type != STATION_SMITH)
                {
                    // Kaynatma/rün üretimi: bar zamanla dolar (seviye × ağaç çarpanı)
                    bool enchSt = STATIONS[craftStation].type == STATION_ENCHANT;
                    SkillType csk = station_skill(STATIONS[craftStation].type);
                    craftProgress += gather_mult(player.skills[csk].level) *
                                     (enchSt ? perkFx.enchSpeedMult
                                             : perkFx.alchSpeedMult) * dt;
                    if (randf() < dt * 8.0f)
                        fx_spawn(&weaponFx,
                                 STATIONS[craftStation].x + (randf() - 0.5f) * 30.0f,
                                 STATIONS[craftStation].y - 40.0f,
                                 (randf() - 0.5f) * 12.0f, -25.0f - randf() * 20.0f,
                                 0.6f, 6.0f + randf() * 5.0f,
                                 enchSt ? 190 : 120, enchSt ? 120 : 230,
                                 enchSt ? 255 : 120, WEAPON_FX_NONE);

                    if (craftProgress >= RECIPES[craftRecipe].work)
                    {
                        // Sunak üretimi ŞANSLI: tutmayan rünün özleri yanar
                        float ch = ENCH_CRAFT_BASE_CHANCE +
                                   (float)player.skills[SKILL_ENCHANTING].level * 0.4f +
                                   perkFx.runeChanceBonus;   // Enchanting ağacı
                        if (ch > 95.0f) ch = 95.0f;
                        if (enchSt && randf() * 100.0f >= ch)
                        {
                            const RecipeDef *fr = &RECIPES[craftRecipe];
                            inventory_consume_n(&inventory, &ITEM_TYPES[fr->in1Item],
                                                fr->in1Count);
                            SDL_Color red = { 255, 80, 80, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f,
                                                 "Rün tutmadı! Özler yandı", red);
                            if (skill_add_xp(player.skills, SKILL_ENCHANTING, 5.0f))
                                on_skill_levelup(&player, SKILL_ENCHANTING,
                                                 &hotbar, floatTexts);
                        }
                        else
                            craft_finish_recipe(craftRecipe, &player, &inventory,
                                                &hotbar, floatTexts);
                        craftRecipe = -1;
                        craftProgress = 0.0f;   // panel açık kalır — tekrar üretilebilir
                    }
                }
                else if (enchAction > 0 &&
                         STATIONS[craftStation].type == STATION_ENCHANT)
                {
                    // Basma töreni: taban süre sabit — yalnız Enchanting AĞACI hızlandırır
                    craftProgress += perkFx.enchSpeedMult * dt;
                    if (randf() < dt * 10.0f)
                        fx_spawn(&weaponFx,
                                 STATIONS[craftStation].x + (randf() - 0.5f) * 40.0f,
                                 STATIONS[craftStation].y - 50.0f - randf() * 40.0f,
                                 (randf() - 0.5f) * 15.0f, -20.0f - randf() * 25.0f,
                                 0.7f, 6.0f + randf() * 6.0f,
                                 190, 120, 255, WEAPON_FX_NONE);

                    if (craftProgress >= ENCH_RUNE_CAST_TIME)
                    {
                        char buf[FLOAT_TEXT_LEN];
                        SDL_Color gold = { 255, 210, 60, 255 };

                        if (enchAction == 1 || enchAction == 2)
                        {
                            // Rün basma: kuşanılı silahın instance efekti değişir
                            int runeItem = (enchAction == 1) ? ITEM_RUNE_PINK
                                                             : ITEM_RUNE_POISON;
                            ItemStack *w = &equipment.slots[EQUIP_WEAPON];
                            if (w->type != NULL &&
                                inventory_consume(&inventory, &ITEM_TYPES[runeItem]))
                            {
                                w->fx = (enchAction == 1) ? (Sint8)WEAPON_FX_PINK
                                                          : (Sint8)WEAPON_FX_POISON;
                                audio_play(SFX_LEVELUP);
                                snprintf(buf, sizeof(buf), "Rün basıldı: %s", w->type->name);
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 24.0f, buf, gold);
                                if (skill_add_xp(player.skills, SKILL_ENCHANTING, 10.0f))
                                    on_skill_levelup(&player, SKILL_ENCHANTING,
                                                     &hotbar, floatTexts);
                            }
                        }
                        else if (enchAction >= 10)
                        {
                            // Stat basma: garantili (maliyet tıklamada doğrulandı, yine kontrol)
                            int s = enchAction - 10;
                            static const int stone[STAT_COUNT] = {
                                ITEM_STONE_STR, ITEM_STONE_DEX, ITEM_STONE_HP,
                                ITEM_STONE_DEF, ITEM_STONE_MP, ITEM_STONE_STA,
                                ITEM_STONE_INT };
                            ItemStack *tgt = &equipment.slots[enchTarget];
                            int cur = 0;
                            int slotK = -1, emptyK = -1;
                            for (int k = 1; k >= 0; k--)
                            {
                                if (tgt->statType[k] == s) { cur = tgt->statTier[k]; slotK = k; }
                                if (tgt->statType[k] < 0) emptyK = k;
                            }
                            int next = cur + 1;
                            if (tgt->type != NULL && next <= STAT_MAX_TIER &&
                                (slotK >= 0 || emptyK >= 0) &&
                                inventory_consume_n(&inventory, &ITEM_TYPES[stone[s]], next) &&
                                inventory_consume_n(&inventory,
                                                    &ITEM_TYPES[ITEM_INGOT_IRON], next * 2))
                            {
                                int k = (slotK >= 0) ? slotK : emptyK;
                                tgt->statType[k] = (Sint8)s;
                                tgt->statTier[k] = (Sint8)next;
                                audio_play(SFX_LEVELUP);
                                snprintf(buf, sizeof(buf), "%s %d: %s",
                                         STAT_NAMES[s], next, tgt->type->name);
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 24.0f, buf, gold);
                                if (skill_add_xp(player.skills, SKILL_ENCHANTING, 12.0f))
                                    on_skill_levelup(&player, SKILL_ENCHANTING,
                                                     &hotbar, floatTexts);
                            }
                        }
                        enchAction = 0;
                        craftProgress = 0.0f;
                    }
                }
                else if ((craftRecipe >= 0 || plusForge) &&
                         !player.attacking && !attackPressed)
                {
                    attackPressed = true;   // sıradaki çekiç/dövme savurması
                    player.pendingAbility = -1;
                }
            }

            // --- AUTO-ATTACK: hedef seçiliyken silah hızında normal vuruş ---
            if (player.autoAttack)
            {
                // Vuruş noktası: mob hedefi ya da (tıklamayla seçilmiş) NPC hedefi
                Mob *aaTarget = mob_find_by_id(mobs, mobCount, player.targetMobId);

                // Mob hedefi öldüyse en yakın yeni mobu otomatik seç — savaş akışı kesilmesin
                // (targetMobId != 0 şartı: NPC hedefli akışa karışma, NPC'ye otomatik geçiş YOK)
                if (aaTarget == NULL && player.targetMobId != 0)
                {
                    player.targetMobId = mob_find_nearest(mobs, mobCount,
                                                          player.rect.x + player.rect.w / 2.0f,
                                                          player.rect.y + player.rect.h / 2.0f);
                    aaTarget = mob_find_by_id(mobs, mobCount, player.targetMobId);
                }

                float aimX = 0.0f, aimY = 0.0f;
                bool haveAim = false;
                if (aaTarget != NULL)
                {
                    aimX = aaTarget->e.rect.x + aaTarget->e.rect.w / 2.0f;
                    aimY = aaTarget->e.rect.y + aaTarget->e.rect.h / 2.0f;
                    haveAim = true;
                }
                else if (targetNpc >= 0 && npc_visible_pos(targetNpc, &aimX, &aimY))
                {
                    haveAim = true;   // NPC eve girerse (hidden) vuruş döngüsü durur
                }

                if (!haveAim)
                {
                    player.autoAttack = false;   // hedef öldü/yok → dur
                }
                else
                {
                    player.attackTimer -= dt;

                    // ASA taban atağı: Space döngüsü ATEŞ TOPU cast'i atar (silahın
                    // doğal atağı — kullanıcı kuralı: Space = kuşanılı silahın tabanı).
                    // Mana yetmezse aşağıdaki melee dürtme yoluna düşer.
                    bool staffCast = player.staffEquipped && player.targetMobId != 0 &&
                                     player.mana >= ABILITIES[ABILITY_FIREBOLT].mana *
                                                    perkFx.manaCostMult;
                    if (staffCast && castSpell < 0 && !player.attacking &&
                        player.abilityCd[ABILITY_FIREBOLT] <= 0.0f)
                    {
                        float adx = aimX - (player.rect.x + player.rect.w / 2.0f);
                        float ady = aimY - (player.rect.y + player.rect.h / 2.0f);
                        if (SDL_sqrtf(adx * adx + ady * ady) <= SPELL_RANGE)
                        {
                            castSpell = ABILITY_FIREBOLT;   // köklü cast başlar
                            castTimer = 0.0f;
                        }
                    }
                    else if (!staffCast &&
                             player.attackTimer <= 0.0f && !player.attacking &&
                             !player.shooting && !attackPressed && castSpell < 0)
                    {
                        float adx = aimX - (player.rect.x + player.rect.w / 2.0f);
                        float ady = aimY - (player.rect.y + player.rect.h / 2.0f);
                        float atkRange = player.bowEquipped
                                       ? PLAYER_BOW_RANGE + perkFx.bowRangeBonus
                                       : PLAYER_ATTACK_RANGE;
                        if (SDL_sqrtf(adx * adx + ady * ady) <= atkRange)
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
                // Vuruş noktası: mob ya da NPC hedefi (auto-attack ile aynı öncelik)
                Mob *target = mob_find_by_id(mobs, mobCount, player.targetMobId);
                float vAimX = 0.0f, vAimY = 0.0f;
                bool vHaveAim = false;
                if (target != NULL)
                {
                    vAimX = target->e.rect.x + target->e.rect.w / 2.0f;
                    vAimY = target->e.rect.y + target->e.rect.h / 2.0f;
                    vHaveAim = true;
                }
                else if (targetNpc >= 0 && npc_visible_pos(targetNpc, &vAimX, &vAimY))
                {
                    vHaveAim = true;
                }
                else if (gatherRes >= 0)
                {
                    // Toplama savurması: düğüme dön (menzil E başlangıcında sağlandı)
                    vAimX = RESOURCES[gatherRes].x;
                    vAimY = RESOURCES[gatherRes].y - 20.0f;
                    vHaveAim = true;
                }
                else if (craftOpen && craftStation >= 0 && (craftRecipe >= 0 || plusForge))
                {
                    // Dövme savurması: istasyona dön
                    vAimX = STATIONS[craftStation].x;
                    vAimY = STATIONS[craftStation].y - 20.0f;
                    vHaveAim = true;
                }

                // Yay kısıtları: ok şart; NPC'ye atış yok (suç sistemi yakın dövüş —
                // uzaktan tanıksız suç dengesi ayrı iş, şimdilik kapalı)
                if (player.bowEquipped && attackPressed)
                {
                    SDL_Color red2 = { 255, 120, 100, 255 };
                    if (target == NULL && targetNpc >= 0)
                    {
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f,
                                             "Yayla NPC hedeflenemez", red2);
                        attackPressed = false;
                    }
                    else if (!ammo_any(&inventory))
                    {
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f, "Ok yok!", red2);
                        attackPressed = false;
                        player.autoAttack = false;   // boş yayla döngü dönmesin
                    }
                }

                if (!vHaveAim)
                {
                    // Hedef yok → saldırma
                    attackPressed = false;
                }
                else if (attackPressed)
                {
                    // Hedef var → menzilde mi?
                    float tdx = vAimX - (player.rect.x + player.rect.w / 2.0f);
                    float tdy = vAimY - (player.rect.y + player.rect.h / 2.0f);
                    float tdist = SDL_sqrtf(tdx * tdx + tdy * tdy);
                    float atkRange = player.bowEquipped
                                   ? PLAYER_BOW_RANGE + perkFx.bowRangeBonus
                                   : PLAYER_ATTACK_RANGE;

                    if (tdist > atkRange)
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

                        // Skill vuruşu kesinleşti → cooldown + stamina ŞİMDİ yanar
                        // (vuruş savrulur; hedef kaçarsa da CD yanar, KO gibi)
                        // Perk: Fırtına PS'yi, Seri Vuruş QS'yi kısaltır
                        if (player.pendingAbility >= 0)
                        {
                            float cdMult = (player.pendingAbility == ABILITY_POWER_STRIKE)
                                         ? perkFx.psCdMult
                                         : (player.pendingAbility == ABILITY_QUICK_SLASH)
                                         ? perkFx.qsCdMult : 1.0f;
                            player.abilityCd[player.pendingAbility] =
                                ABILITIES[player.pendingAbility].cooldown * cdMult;
                            player.stamina -= ABILITIES[player.pendingAbility].stamina *
                                (ABILITIES[player.pendingAbility].skill == SKILL_ARCHERY
                                 ? perkFx.bowStaminaMult : 1.0f);
                            if (player.stamina < 0.0f) player.stamina = 0.0f;
                        }
                    }
                }
            }
            // Vuruş başlamadı ve savurma/çekme de yok → bekleyen skill çarpanını temizle.
            // (Savurma/çekme sürerken DOKUNMA: çarpan vuruş/release karesinde kullanılacak)
            if (!attackPressed && !player.attacking && !player.shooting)
                player.pendingAbility = -1;

            // Partikul efekti kusanili silahin INSTANCE'indan (rün buraya basılır).
            // Öncelik: kaplama (Zehir Yağı) > instance efekti
            const ItemStack *wpnStack = &equipment.slots[EQUIP_WEAPON];
            player.weaponFx = (wpnStack->type == NULL) ? WEAPON_FX_NONE
                            : (player.coatTimer > 0.0f) ? player.coatFx
                            : (WeaponFxType)wpnStack->fx;
            // Yay kuşanımı: attackPressed slash yerine SHOOT başlatsın (player_update)
            player.bowEquipped = (wpnStack->type != NULL &&
                                  wpnStack->type->weaponType == WEAPON_BOW);
            player.staffEquipped = (wpnStack->type != NULL &&
                                    wpnStack->type->weaponType == WEAPON_STAFF);
            player.drawSpeedMult = perkFx.drawSpeedMult;   // ağaç: çekme hızı

            // Kaynatma/tören animasyonu: kazan ve sunakta süreç sürerken spellcast;
            // kamp ateşi kurulumu da aynı duruşu kullanır (çömelip uğraşma hissi)
            player.casting = (craftOpen && craftStation >= 0 &&
                              STATIONS[craftStation].type != STATION_SMITH &&
                              (craftRecipe >= 0 || enchAction > 0)) ||
                             campBuild >= 0.0f || castSpell >= 0;

            // Türetilmiş statlar: taban + perk + sunak stat basımları (her kare).
            // STR/DEX taban 10 (combat STAT_BASE ile aynı); max havuzlar taban + gear.
            {
                int gearStat[STAT_COUNT] = {0};
                for (int s = 0; s < EQUIP_SLOT_COUNT; s++)
                {
                    const ItemStack *st = &equipment.slots[s];
                    if (st->type == NULL) continue;
                    for (int k = 0; k < 2; k++)
                        if (st->statType[k] >= 0)
                            gearStat[st->statType[k]] +=
                                STAT_TIER_VALUE[st->statType[k]] * st->statTier[k];
                }
                player.strength     = 10 + gearStat[STAT_STR];
                player.dexterity    = 10 + gearStat[STAT_DEX];
                player.intelligence = 10 + gearStat[STAT_INT];

                // Zırh ağırlığı (Faz 22): ağır 5 / orta 2 / hafif 1 puan;
                // STR (taban 10 üstü) puan başına 1 telafi eder. Kalan puan
                // başına hız -%2 (tavan -%30). Ağır/orta parça mana yenilemesini kısar
                // ("demir kutuda kanalize etmek zor" — Zihin/MP ile telafi edilir).
                {
                    int weight = 0, heavyN = 0, medN = 0;
                    for (int s2 = 0; s2 < EQUIP_SLOT_COUNT; s2++)
                    {
                        const ItemType *it2 = equipment.slots[s2].type;
                        if (it2 == NULL) continue;
                        if (it2->armorClass == ARMOR_HEAVY)  { weight += 5; heavyN++; }
                        else if (it2->armorClass == ARMOR_MEDIUM) { weight += 2; medN++; }
                        else if (it2->armorClass == ARMOR_LIGHT)  weight += 1;
                    }
                    int over = weight - (player.strength - 10);
                    if (over < 0) over = 0;
                    armorSpeedPenalty = 0.02f * (float)over;
                    if (armorSpeedPenalty > 0.30f) armorSpeedPenalty = 0.30f;
                    armorManaMult = 1.0f - (0.06f * (float)heavyN + 0.03f * (float)medN);
                    if (armorManaMult < 0.4f) armorManaMult = 0.4f;
                }

                // Max can: artış kadar iyileş (perk/gear takınca), azalışta kırp
                float newMaxHp = BASE_MAX_HP + perkFx.maxHpBonus + (float)gearStat[STAT_HP];
                if (newMaxHp > player.maxHealth)
                    player.health += newMaxHp - player.maxHealth;
                player.maxHealth = newMaxHp;
                if (player.health > player.maxHealth) player.health = player.maxHealth;

                player.maxMana    = 50.0f  + (float)gearStat[STAT_MP]
                                  + perkFx.maxManaBonus;
                player.maxStamina = 100.0f + (float)gearStat[STAT_STA]
                                  + perkFx.maxStaminaBonus;
                // (DEF ekipmanı equipment_total_defense içinde toplanır)
            }

            player_update(&player, keys, attackPressed, dt, &weaponFx); // oyuncuyu güncelle

            // Ok bırakışı: çekmenin release karesinde 1 ok tüket + güdümlü ok doğur.
            // Hedef çekme sırasında öldüyse ok çıkmaz (ok yanmaz).
            // Beceri oku: pendingAbility türüne göre çarpan/etki taşır.
            if (player.shooting && !player.shotReleased &&
                player.currentFrame >= PLAYER_SHOOT_RELEASE_FRAME)
            {
                player.shotReleased = true;
                Mob *sTarget = mob_find_by_id(mobs, mobCount, player.targetMobId);
                float ammoMult = 0.0f;
                if (sTarget != NULL)
                    ammoMult = ammo_consume(&inventory, &activeAmmo, floatTexts,
                                            player.rect.x + player.rect.w / 2.0f,
                                            player.rect.y, perkFx.ammoSaveChance);
                if (sTarget != NULL && ammoMult > 0.0f)
                {
                    int pa = player.pendingAbility;
                    for (int a = 0; a < MAX_ARROWS; a++)
                    {
                        if (arrows[a].active) continue;
                        arrows[a] = (ArrowShot){
                            .active = true,
                            .x = player.rect.x + player.rect.w / 2.0f,
                            .y = player.rect.y + player.rect.h / 2.0f - 14.0f, // göğüs hizası
                            .targetId = player.targetMobId,
                            .dmgScale = (pa >= 0) ? ABILITIES[pa].damageScale : 1.0f,
                            .ammoMult = ammoMult,
                            .kind = (pa == ABILITY_ICE_ARROW) ? 1
                                  : (pa == ABILITY_AOE_ARROW) ? 2 : 0,
                        };
                        break;
                    }
                    if (pa == ABILITY_VOLLEY)   // kalan oklar kuyruğa (0.15sn arayla)
                    {
                        volleyLeft = 2 + perkFx.volleyExtra;   // Çifte Yaylım +1
                        volleyTimer = 0.15f;
                    }
                }
                player.pendingAbility = -1;   // beceri bu atışta kullanıldı
            }

            // Yaylım kuyruğu: 0.15sn arayla kalan oklar; hedef ölürse en yakına geçer
            if (volleyLeft > 0)
            {
                volleyTimer -= dt;
                if (volleyTimer <= 0.0f)
                {
                    Mob *vTarget = mob_find_by_id(mobs, mobCount, player.targetMobId);
                    if (vTarget == NULL)
                    {
                        player.targetMobId = mob_find_nearest(mobs, mobCount,
                                                              player.rect.x + player.rect.w / 2.0f,
                                                              player.rect.y + player.rect.h / 2.0f);
                        vTarget = mob_find_by_id(mobs, mobCount, player.targetMobId);
                    }
                    float vAmmoMult = 0.0f;
                    if (vTarget != NULL)
                        vAmmoMult = ammo_consume(&inventory, &activeAmmo, floatTexts,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y, perkFx.ammoSaveChance);
                    if (vTarget != NULL && vAmmoMult > 0.0f)
                    {
                        for (int a = 0; a < MAX_ARROWS; a++)
                        {
                            if (arrows[a].active) continue;
                            arrows[a] = (ArrowShot){
                                .active = true,
                                .x = player.rect.x + player.rect.w / 2.0f,
                                .y = player.rect.y + player.rect.h / 2.0f - 14.0f,
                                .targetId = player.targetMobId,
                                .dmgScale = ABILITIES[ABILITY_VOLLEY].damageScale,
                                .ammoMult = vAmmoMult,
                                .kind = 0,
                            };
                            break;
                        }
                        volleyLeft--;
                        volleyTimer = 0.15f;
                    }
                    else
                        volleyLeft = 0;   // hedef ya da ok kalmadı — kuyruk söner
                }
            }

            // Havuz yenilenmesi — Kader düğümleri hızlandırır
            player.mana += 2.0f * perkFx.manaRegenMult * armorManaMult * dt;
            if (player.mana > player.maxMana) player.mana = player.maxMana;
            player.stamina += 15.0f * perkFx.stamRegenMult * dt;
            if (player.stamina > player.maxStamina) player.stamina = player.maxStamina;
            if (perkFx.hpRegenPerSec > 0.0f)   // Yaşam Gücü: pasif can
            {
                player.health += perkFx.hpRegenPerSec * dt;
                if (player.health > player.maxHealth) player.health = player.maxHealth;
            }

            // Süreli tüketilebilir etkileri sayaçları
            if (player.speedBuffTimer > 0.0f) player.speedBuffTimer -= dt;
            if (player.foodTimer > 0.0f)
            {
                // Yemek yenilenmesi: yavaş tick (iksirin anlık patlamasına alternatif)
                player.foodTimer -= dt;
                player.health += player.foodRegenRate * dt;
                if (player.health > player.maxHealth) player.health = player.maxHealth;
            }
            if (player.poisonTimer > 0.0f)
            {
                // Slime zehri: yavaş erime (ölüm kontrolü aşağıdaki genel yolda)
                player.poisonTimer -= dt;
                player.health -= player.poisonDps * dt;
            }
            if (player.manaShieldTimer > 0.0f)
            {
                player.manaShieldTimer -= dt;
                if (player.manaShieldTimer <= 0.0f)
                    player.shieldBreakT = 0.001f;   // kırılma animasyonu başlat
            }
            if (player.shieldBreakT > 0.0f) player.shieldBreakT += dt;
            if (player.shieldBreakT > 4 * 0.07f) player.shieldBreakT = 0.0f;
            if (player.flameArmorTimer > 0.0f)
            {
                player.flameArmorTimer -= dt;
                if (randf() < dt * 10.0f)   // aura közleri
                    fx_spawn(&weaponFx,
                             player.rect.x + randf() * player.rect.w,
                             player.rect.y + player.rect.h * (0.3f + randf() * 0.6f),
                             (randf() - 0.5f) * 20.0f, -30.0f - randf() * 20.0f,
                             0.35f, 5.0f + randf() * 5.0f, 255, 150, 50, WEAPON_FX_NONE);
            }
            if (player.coatTimer > 0.0f)
            {
                player.coatTimer -= dt;
                if (player.coatTimer <= 0.0f)
                {
                    SDL_Color dim = { 170, 170, 190, 255 };
                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                         player.rect.x + player.rect.w / 2.0f,
                                         player.rect.y - 10.0f, "Kaplama geçti", dim);
                }
            }
            fx_update(&weaponFx, dt);

            gametime_update(dt);                      // dünya saati
            npc_update(dt, gametime_hours());         // NPC rutinleri (yürü/eve gir)

            // Günlük itibar toparlanması: suçsuz geçen gün, negatif itibar 0'a doğru +1
            // (pozitif itibar zamanla artmaz — güven kazanmak iş ister)
            int today = gametime_day();
            if (today != lastSeenDay)
            {
                lastSeenDay = today;
                resource_daily_respawn();   // doğa tazelenir (cevher/bitki/ağaç)
                contract_new_day(today);    // pano yenilenir (yarım kontratlar düşer)
                spawn_zones_fill(spawnZones, spawnZoneCount, mobs, MAX_MOBS,
                                 &nextMobId);   // av sürüleri tazelenir (eksik dolar)
                // (kamp ateşi burada SÖNMEZ — kendi 24 saatlik ömrünü yaşar)
                if (player.lastCrimeDay < today - 1 || player.lastCrimeDay == 0)
                    for (int r = 0; r < MAX_REGIONS; r++)
                        if (player.reputation[r] < 0.0f)
                        {
                            player.reputation[r] += REP_DAILY;
                            if (player.reputation[r] > 0.0f)
                                player.reputation[r] = 0.0f;   // toparlanma 0'ı aşmaz
                        }
            }

            // --- MUHAFIZ: bounty takibi / yakalama / düşmanlık (guard.c beyni) ---
            if (guardIdx >= 0)
            {
                bool uiFrozen = catchOpen || dialogNpc >= 0;
                GuardTick gt = guard_update(dt,
                                            player.rect.x + player.rect.w / 2.0f,
                                            player.rect.y + player.rect.h / 2.0f,
                                            player.bounty[guardRegion], uiFrozen);
                if (gt.wantCatch && !catchOpen && !uiFrozen)
                {
                    catchOpen = true;
                    player.autoAttack = false;   // yakalama anında savaş durur
                    audio_play(SFX_UI);
                }
                if (gt.attackDamage > 0.0f)
                {
                    // Muhafız vuruşu mob vuruşuyla aynı savunma hattından geçer
                    bool dodged = false;
                    bool yaraliAyi = perkFx.yaraliAyi;
                    float dmg = combat_mob_damage(gt.attackDamage, &equipment, &perkFx,
                                                  player.state == PLAYER_IDLE,
                                                  player.health / player.maxHealth,
                                                  yaraliAyi, &dodged);
                    if (!dodged)
                        dmg = mana_shield_absorb(&player, dmg, floatTexts);
                    if (!dodged && dmg > 0.0f)
                    {
                        player.health -= dmg;
                        player.gaziTimer = 5.0f;
                        gatherRes = -1;   // muhafız vuruşu da süreçleri keser
                        craftOpen = false;
                        craftRecipe = -1;
                        plusForge = false;
                        enchAction = 0;
                        craftProgress = 0.0f;
                        shakeTime = shakeDur = 0.25f;
                        shakeAmp = 6.0f;
                        flashTime = FLASH_DUR;
                        floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                        player.rect.x + player.rect.w / 2.0f + 10.0f,
                                        player.rect.y - 10.0f, (int)dmg, false);
                        audio_play(SFX_HIT);
                    }
                }
            }

            if (targetPressed)
            {
                float px = player.rect.x + player.rect.w / 2.0f;
                float py = player.rect.y + player.rect.h / 2.0f;
                player.targetMobId = mob_find_nearest(mobs, mobCount, px, py);
                if (player.targetMobId != 0)
                    targetNpc = -1;   // Z mob seçti → NPC hedefi düşer

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
            // (asa dürtmesi THRUST oynar — hasar karesi farklı)
            if (player.attacking && !player.attackHit &&
                player.currentFrame == (player.state == PLAYER_THRUST
                                        ? PLAYER_THRUST_HIT_FRAME
                                        : PLAYER_ATTACK_HIT_FRAME))
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

                        // Sinsi vuruş: eğilmiş + hedef fark etmemiş → ×2 hasar + Sneak XP
                        if (player.sneaking && !target->aggro)
                        {
                            hit.damage *= perkFx.backstabMult;   // taban 2.0 + ağaç düğümleri
                            SDL_Color purple = { 200, 140, 255, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 target->e.rect.x + target->e.rect.w / 2.0f,
                                                 target->e.rect.y - 26.0f, "Sinsi vuruş!", purple);
                            if (skill_add_xp(player.skills, SKILL_SNEAK, SNEAK_XP_BACKSTAB))
                                on_skill_levelup(&player, SKILL_SNEAK, &hotbar, floatTexts);
                        }

                        target->e.health -= hit.damage;
                        target->e.hitFlash = 0.15f;
                        audio_play(hit.crit ? SFX_CRIT : SFX_HIT);

                        // SAVAŞ RAHİBİ: melee vuruş mana yeniler (battlemage döngüsü)
                        if (perkFx.meleeManaRegen > 0.0f)
                        {
                            player.mana += perkFx.meleeManaRegen;
                            if (player.mana > player.maxMana) player.mana = player.maxMana;
                        }

                        // Crit: hedefte sarı-beyaz kıvılcım patlaması + mini sarsıntı
                        if (hit.crit)
                        {
                            fx_burst(&weaponFx,
                                     target->e.rect.x + target->e.rect.w / 2.0f,
                                     target->e.rect.y + target->e.rect.h / 2.0f,
                                     12, 90.0f, 255, 240, 120);
                            if (shakeTime <= 0.0f)   // aktif (daha sert) sarsıntıyı ezme
                            {
                                shakeTime = shakeDur = 0.15f;
                                shakeAmp = 3.0f;
                            }
                        }

                        // Kanatma: vuruş hasarının bir kısmı DoT olarak yayılır
                        // (Kasap süreyi uzatır → toplam kanama artar)
                        if (perkFx.bleedFrac > 0.0f)
                        {
                            target->bleedDps = hit.damage * perkFx.bleedFrac / 3.0f;
                            target->bleedTime = 3.0f * perkFx.bleedPower;
                        }

                        // Zehir (rün/kaplama): 4 sn'de hasarın %25'i — kanamadan
                        // AYRI kanal, ÜST ÜSTE BİNER (tam DoT build'i meşru)
                        if (player.weaponFx == WEAPON_FX_POISON)
                        {
                            target->poisonDps = hit.damage * 0.25f / 4.0f;
                            target->poisonTime = 4.0f;   // süre tazelenir
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
                            mob_die(target, mobs, MAX_MOBS, &nextMobId, loots, questStatus, floatTexts, &player);
                    }
                }
                else if (targetNpc >= 0)   // NPC'ye saldırı (suç sistemi)
                {
                    float nx, ny;
                    if (npc_visible_pos(targetNpc, &nx, &ny))
                    {
                        float tdx = nx - (player.rect.x + player.rect.w / 2.0f);
                        float tdy = ny - (player.rect.y + player.rect.h / 2.0f);
                        if (SDL_sqrtf(tdx * tdx + tdy * tdy) <= PLAYER_ATTACK_RANGE)
                        {
                            // NPC savunmasız sivil: defans 0, ability çarpanı yok.
                            // Skill XP de YOK — köylü döverek skill kasılmasın.
                            const ItemStack *weapon = &equipment.slots[EQUIP_WEAPON];
                            HitResult hit = combat_player_hit(&player, weapon, 0.0f, 1.0f,
                                                              npc_health(targetNpc) / npc_max_health(targetNpc),
                                                              &perkFx);
                            int victim = targetNpc;
                            bool isGuardVictim = NPCS[victim].isGuard;
                            bool died = npc_damage(victim, hit.damage);
                            audio_play(hit.crit ? SFX_CRIT : SFX_HIT);
                            floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                            nx + 10.0f, ny - 60.0f, (int)hit.damage, hit.crit);

                            int crimeRegion = region_of(nx, ny);   // suç işlendiği bölgeye yazılır
                            if (isGuardVictim)
                            {
                                // Kanuna saldırı: tanık gerekmez (kanunun kendisi tanıktır).
                                // Her vuruş suçtur; öldürmenin bedeli ayrıca sabit.
                                int b = died ? CRIME_BOUNTY_GUARD_KILL : CRIME_BOUNTY_HIT;
                                player.bounty[crimeRegion] += b;
                                player.lastCrimeDay = gametime_day();
                                rep_add(&player.reputation[crimeRegion],
                                        died ? REP_GUARD_KILL : REP_HIT);
                                if (!died)
                                    guard_set_hostile(true);
                                char buf[FLOAT_TEXT_LEN];
                                snprintf(buf, sizeof(buf), died ? "Muhafız öldü! +%d bounty"
                                                                : "Kanuna saldırı! +%d bounty", b);
                                SDL_Color red = { 255, 80, 80, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 24.0f, buf, red);
                            }
                            // Suç: tanık varsa bounty + itibar kaybı (tanıksız suç ikisini de
                            // etkilemez); eğilirken tanık yarıçapı düşer (sneak'in suç ayağı)
                            else if (crime_witnessed(nx, ny, victim, !died,
                                                     player.sneaking ? SNEAK_WITNESS_RANGE
                                                                     : CRIME_WITNESS_RANGE))
                            {
                                int b = died ? CRIME_BOUNTY_MURDER : CRIME_BOUNTY_HIT;
                                player.bounty[crimeRegion] += b;
                                player.lastCrimeDay = gametime_day();
                                rep_add(&player.reputation[crimeRegion],
                                        died ? REP_MURDER : REP_HIT);
                                char buf[FLOAT_TEXT_LEN];
                                snprintf(buf, sizeof(buf), died ? "Cinayet! +%d bounty"
                                                                : "Suç! +%d bounty", b);
                                SDL_Color red = { 255, 80, 80, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 24.0f, buf, red);
                            }
                            else if (died && player.sneaking)
                            {
                                // Fark edilmeden tanıksız cinayet — Sneak'in ustalık anı
                                SDL_Color gray = { 170, 170, 190, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 24.0f, "Kimse görmedi...", gray);
                                if (skill_add_xp(player.skills, SKILL_SNEAK, SNEAK_XP_CRIME))
                                    on_skill_levelup(&player, SKILL_SNEAK, &hotbar, floatTexts);
                            }

                            if (died)
                            {
                                fx_burst(&weaponFx, nx, ny - 30.0f, 14, 90.0f, 200, 30, 30);
                                targetNpc = -1;
                                player.autoAttack = false;
                                if (dialogNpc == victim)   // konuşulan öldüyse UI kapanır
                                {
                                    dialogNpc = -1;
                                    shopOpen = false;
                                }
                            }
                        }
                    }
                }
                else if (gatherRes >= 0 && RESOURCES[gatherRes].toolItem >= 0)
                {
                    // Toplama savurması: vuruş = iş (seviye × meslek ağacı çarpanı)
                    const ResourceDef *rd = &RESOURCES[gatherRes];
                    resource_add_progress(gatherRes,
                        gather_mult(player.skills[rd->skill].level) *
                        ((rd->skill == SKILL_MINING)      ? perkFx.mineSpeedMult
                       : (rd->skill == SKILL_HERBALISM)   ? perkFx.herbSpeedMult
                       : (rd->skill == SKILL_WOODCUTTING) ? perkFx.woodSpeedMult
                                                          : 1.0f));
                    audio_play(SFX_HIT);
                    fx_burst(&weaponFx, rd->x, rd->y - 20.0f, 5, 60.0f, 210, 205, 190);

                    if (resource_progress(gatherRes) >= rd->work)
                    {
                        gather_finish(gatherRes, &player, &inventory, &hotbar, floatTexts);
                        gatherRes = -1;   // süreç bitti (dolu çantada da bırakılır)
                    }
                }
                else if (craftOpen && craftStation >= 0 && (craftRecipe >= 0 || plusForge))
                {
                    // Dövme vuruşu: ocakta iş ilerler (Smithing seviye × ağaç çarpanı)
                    craftProgress += gather_mult(player.skills[SKILL_SMITHING].level) *
                                     perkFx.smithSpeedMult;
                    audio_play(SFX_HIT);
                    fx_burst(&weaponFx, STATIONS[craftStation].x,
                             STATIONS[craftStation].y - 30.0f,
                             6, 70.0f, 255, 180, 80);   // ocak kıvılcımı

                    float work = plusForge ? 8.0f : RECIPES[craftRecipe].work;
                    if (craftProgress >= work)
                    {
                        if (plusForge)
                        {
                            // '+basma' rulosu: külçeler HER DURUMDA yanar (kumar bedeli)
                            ItemStack *w = &equipment.slots[EQUIP_WEAPON];
                            int cost = w->plusLevel + 1;
                            if (w->type != NULL && w->type->weaponType != WEAPON_TOOL &&
                                inventory_consume_n(&inventory,
                                                    &ITEM_TYPES[ITEM_INGOT_IRON], cost))
                            {
                                float ch = plus_chance(w->plusLevel + 1,
                                                       player.skills[SKILL_SMITHING].level);
                                char buf[FLOAT_TEXT_LEN];
                                if (randf() * 100.0f < ch)
                                {
                                    w->plusLevel++;
                                    audio_play(SFX_LEVELUP);
                                    snprintf(buf, sizeof(buf), "%s +%d!",
                                             w->type->name, w->plusLevel);
                                    SDL_Color gold = { 255, 210, 60, 255 };
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                         player.rect.x + player.rect.w / 2.0f,
                                                         player.rect.y - 24.0f, buf, gold);
                                    if (skill_add_xp(player.skills, SKILL_SMITHING, 12.0f))
                                        on_skill_levelup(&player, SKILL_SMITHING,
                                                         &hotbar, floatTexts);
                                }
                                else
                                {
                                    if (w->plusLevel > 0) w->plusLevel--;
                                    snprintf(buf, sizeof(buf), "Dövme tutmadı! %s +%d",
                                             w->type->name, w->plusLevel);
                                    SDL_Color red = { 255, 80, 80, 255 };
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                         player.rect.x + player.rect.w / 2.0f,
                                                         player.rect.y - 24.0f, buf, red);
                                    if (shakeTime <= 0.0f)
                                    {
                                        shakeTime = shakeDur = 0.15f;   // örs sarsıntısı
                                        shakeAmp = 3.0f;
                                    }
                                    if (skill_add_xp(player.skills, SKILL_SMITHING, 4.0f))
                                        on_skill_levelup(&player, SKILL_SMITHING,
                                                         &hotbar, floatTexts);
                                }
                            }
                            plusForge = false;
                            enchAction = 0;
                        }
                        else
                        {
                            craft_finish_recipe(craftRecipe, &player, &inventory,
                                                &hotbar, floatTexts);
                            craftRecipe = -1;   // panel açık — tekrar üretilebilir
                        }
                        craftProgress = 0.0f;
                    }
                }
                player.attackHit = true;      // bu saldırıda denendi (tekrar etmesin)
                player.pendingAbility = -1;   // skill çarpanı kullanıldı/harcandı
            }

            // --- OK UÇUŞU: hedefe güdümlü, hasar VARIŞTA (o anki stat/rün ile) ---
            for (int a = 0; a < MAX_ARROWS; a++)
            {
                if (!arrows[a].active) continue;
                Mob *at = mob_find_by_id(mobs, mobCount, arrows[a].targetId);
                if (at == NULL)
                {
                    arrows[a].active = false;   // hedef öldü/despawn — ok söner
                    continue;
                }
                float tx = at->e.rect.x + at->e.rect.w / 2.0f;
                float ty = at->e.rect.y + at->e.rect.h / 2.0f;
                float adx = tx - arrows[a].x, ady = ty - arrows[a].y;
                float adist = SDL_sqrtf(adx * adx + ady * ady);
                float step = ARROW_SPEED * dt;

                if (adist > step + 8.0f)
                {
                    arrows[a].x += adx / adist * step;   // yolda: hedefe yaklaş
                    arrows[a].y += ady / adist * step;
                    continue;
                }

                // VARIŞ: melee vuruşla aynı kurallar (sinsi ×2, crit, rün, XP);
                // ok kademesi çarpanı beceri çarpanıyla ÇARPILIR (Faz 8)
                arrows[a].active = false;
                const ItemStack *bowStack = &equipment.slots[EQUIP_WEAPON];
                float aScale = arrows[a].dmgScale *
                               (arrows[a].ammoMult > 0.0f ? arrows[a].ammoMult : 1.0f);
                HitResult hit = combat_player_hit(&player, bowStack,
                                                  at->type->defense,
                                                  aScale,
                                                  at->e.health / at->e.maxHealth,
                                                  &perkFx);

                if (player.sneaking && !at->aggro)
                {
                    hit.damage *= perkFx.backstabMult;   // taban 2.0 + ağaç düğümleri
                    SDL_Color purple = { 200, 140, 255, 255 };
                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                         tx, at->e.rect.y - 26.0f,
                                         "Sinsi ok!", purple);
                    if (skill_add_xp(player.skills, SKILL_SNEAK, SNEAK_XP_BACKSTAB))
                        on_skill_levelup(&player, SKILL_SNEAK, &hotbar, floatTexts);
                }

                at->e.health -= hit.damage;
                at->e.hitFlash = 0.15f;
                at->arrowsHit++;   // leşten ~%30 geri (mob_die)
                audio_play(hit.crit ? SFX_CRIT : SFX_HIT);
                if (hit.crit)
                    fx_burst(&weaponFx, tx, ty, 12, 90.0f, 255, 240, 120);

                // Zehir rünü/kaplaması ok ucuna da işler — kanamayla üst üste biner
                if (player.weaponFx == WEAPON_FX_POISON)
                {
                    at->poisonDps = hit.damage * 0.25f / 4.0f;
                    at->poisonTime = 4.0f;
                }

                // Buz Oku: %40 yavaşlatma 3 sn + buz kristali serpintisi
                if (arrows[a].kind == 1)
                {
                    at->slowMult = 0.6f;
                    at->slowTimer = 3.0f;
                    fx_burst(&weaponFx, tx, ty, 10, 70.0f, 150, 210, 255);
                    SDL_Color iceCol = { 150, 210, 255, 255 };
                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                         tx, at->e.rect.y - 26.0f,
                                         "Yavaşladı!", iceCol);
                }

                // Alan Oku: varışta patlar — çevredeki moblara ×0.6 sıçrama
                if (arrows[a].kind == 2)
                {
                    fx_burst(&weaponFx, tx, ty, 22, 140.0f, 255, 170, 60);
                    if (shakeTime <= 0.0f) { shakeTime = shakeDur = 0.15f; shakeAmp = 3.0f; }
                    for (int m2 = 0; m2 < mobCount; m2++)
                    {
                        Mob *nb = &mobs[m2];
                        if (!nb->e.alive || nb == at) continue;
                        float ndx = (nb->e.rect.x + nb->e.rect.w / 2.0f) - tx;
                        float ndy = (nb->e.rect.y + nb->e.rect.h / 2.0f) - ty;
                        float aoeR = AOE_ARROW_RADIUS + perkFx.aoeRadiusBonus;
                        if (ndx * ndx + ndy * ndy > aoeR * aoeR)
                            continue;
                        HitResult splash = combat_player_hit(&player, bowStack,
                                                             nb->type->defense,
                                                             0.6f * (arrows[a].ammoMult > 0.0f
                                                                     ? arrows[a].ammoMult : 1.0f),
                                                             nb->e.health / nb->e.maxHealth,
                                                             &perkFx);
                        nb->e.health -= splash.damage;
                        nb->e.hitFlash = 0.15f;
                        nb->aggro = true;
                        floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                        nb->e.rect.x + nb->e.rect.w / 2.0f + 10.0f,
                                        nb->e.rect.y, (int)splash.damage, splash.crit);
                        if (nb->e.health <= 0.0f)
                            mob_die(nb, mobs, MAX_MOBS, &nextMobId, loots, questStatus, floatTexts, &player);
                    }
                }

                at->aggro = true;
                mob_alert_camp(mobs, mobCount, at);
                floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                tx + 10.0f, at->e.rect.y, (int)hit.damage, hit.crit);

                float xp = combat_xp_for_hit(hit.damage, at->type->level,
                                             player.skills[SKILL_ARCHERY].level);
                if (skill_add_xp(player.skills, SKILL_ARCHERY, xp))
                    on_skill_levelup(&player, SKILL_ARCHERY, &hotbar, floatTexts);

                if (adist > 0.0f)   // hafif itme (melee 180'in yarısı — ok kütlesiz)
                {
                    at->e.knockbackX = adx / adist * 90.0f;
                    at->e.knockbackY = ady / adist * 90.0f;
                }
                if (at->e.health <= 0.0f)
                    mob_die(at, mobs, MAX_MOBS, &nextMobId, loots, questStatus, floatTexts, &player);
            }

            // Diken patlaması animasyonları: 10 kare × 0.06sn
            for (int q = 0; q < MAX_SPIKE_FX; q++)
            {
                if (!spikeFx[q].active) continue;
                spikeFx[q].t += dt;
                if (spikeFx[q].t >= 10 * 0.06f) spikeFx[q].active = false;
            }

            // Arcane patlamaları: 7 kare × 0.045sn, bitince söner
            for (int b = 0; b < MAX_BURSTS; b++)
            {
                if (!bursts[b].active) continue;
                bursts[b].t += dt;
                if (bursts[b].t >= 7 * 0.045f) bursts[b].active = false;
            }

            // --- YAPIŞKAN TUZAKLARI: mob basınca patlar, alandakileri yavaşlatır ---
            for (int tI = 0; tI < MAX_TRAPS; tI++)
            {
                if (!traps[tI].active) continue;
                bool fired = false;
                for (int mI = 0; mI < mobCount && !fired; mI++)
                {
                    if (!mobs[mI].e.alive) continue;
                    float tdx = (mobs[mI].e.rect.x + mobs[mI].e.rect.w / 2.0f) - traps[tI].x;
                    float tdy = (mobs[mI].e.rect.y + mobs[mI].e.rect.h / 2.0f) - traps[tI].y;
                    if (tdx * tdx + tdy * tdy <= TRAP_TRIGGER_R * TRAP_TRIGGER_R)
                        fired = true;
                }
                if (!fired) continue;

                traps[tI].active = false;
                audio_play(SFX_POTION);   // vıcık patlama (yakın ses)
                fx_burst(&weaponFx, traps[tI].x, traps[tI].y - 8.0f,
                         16, 90.0f, 120, 230, 90);   // yeşil jel sıçraması
                SDL_Color pgreen = { 120, 230, 90, 255 };
                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                     traps[tI].x, traps[tI].y - 20.0f,
                                     "Yapıştı!", pgreen);

                for (int mI = 0; mI < mobCount; mI++)
                {
                    if (!mobs[mI].e.alive) continue;
                    float tdx = (mobs[mI].e.rect.x + mobs[mI].e.rect.w / 2.0f) - traps[tI].x;
                    float tdy = (mobs[mI].e.rect.y + mobs[mI].e.rect.h / 2.0f) - traps[tI].y;
                    if (tdx * tdx + tdy * tdy > TRAP_AOE_R * TRAP_AOE_R) continue;
                    mobs[mI].slowMult = 0.4f;    // Buz Oku'ndan sert (%60 yavaş)
                    mobs[mI].slowTimer = 5.0f;
                }
            }

            // --- MOB MERMİLERİ: düz uçuş; duvara/menzile takılır, oyuncuya çarparsa
            // melee mob vuruşuyla AYNI savunma hattından geçer (dodge/defans/zırh XP)
            for (int pj = 0; pj < MAX_MOB_PROJS; pj++)
            {
                if (!mobProjs[pj].active) continue;
                float step = SDL_sqrtf(mobProjs[pj].vx * mobProjs[pj].vx +
                                       mobProjs[pj].vy * mobProjs[pj].vy) * dt;
                mobProjs[pj].x += mobProjs[pj].vx * dt;
                mobProjs[pj].y += mobProjs[pj].vy * dt;
                mobProjs[pj].traveled += step;

                // Büyü topu iz bırakır (mor kıvılcım)
                if (mobProjs[pj].kind == 1 && randf() < dt * 30.0f)
                    fx_spawn(&weaponFx, mobProjs[pj].x, mobProjs[pj].y,
                             (randf() - 0.5f) * 20.0f, (randf() - 0.5f) * 20.0f,
                             0.3f, 5.0f + randf() * 4.0f, 190, 110, 235, WEAPON_FX_NONE);

                // Menzil doldu ya da katıya çarptı → söner
                SDL_FRect pbox = { mobProjs[pj].x - 3.0f, mobProjs[pj].y - 3.0f, 6.0f, 6.0f };
                if (mobProjs[pj].traveled > MOB_PROJ_MAX_DIST || box_hits_solid(pbox))
                {
                    mobProjs[pj].active = false;
                    continue;
                }

                // Oyuncuya isabet (gövde kutusu, hafif tolerans)
                SDL_FRect hb = { player.rect.x - 4.0f, player.rect.y - 4.0f,
                                 player.rect.w + 8.0f, player.rect.h + 8.0f };
                if (mobProjs[pj].x < hb.x || mobProjs[pj].x > hb.x + hb.w ||
                    mobProjs[pj].y < hb.y || mobProjs[pj].y > hb.y + hb.h)
                    continue;

                mobProjs[pj].active = false;
                bool dodged = false;
                bool yaraliAyi = perkFx.yaraliAyi;
                float dmg = combat_mob_damage(mobProjs[pj].dmg, &equipment, &perkFx,
                                              player.state == PLAYER_IDLE,
                                              player.health / player.maxHealth,
                                              yaraliAyi, &dodged);
                float pcx = player.rect.x + player.rect.w / 2.0f;
                if (!dodged)
                    dmg = mana_shield_absorb(&player, dmg, floatTexts);
                if (dodged)
                {
                    SDL_Color cyan = { 90, 220, 255, 255 };
                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                         pcx, player.rect.y - 10.0f, "Kaçındın!", cyan);
                    audio_play(SFX_DODGE);
                }
                else if (dmg > 0.0f)
                {
                    // (melee mob vuruşu bloğunun kompakt eşi — desenkron etme!)
                    player.health -= dmg;
                    player.gaziTimer = 5.0f;
                    gatherRes = -1;
                    craftOpen = false;
                    craftRecipe = -1;
                    plusForge = false;
                    enchAction = 0;
                    craftProgress = 0.0f;
                    shakeTime = shakeDur = 0.25f;
                    shakeAmp = 6.0f;
                    flashTime = FLASH_DUR;
                    floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                    pcx + 10.0f, player.rect.y - 10.0f, (int)dmg, false);
                    audio_play(SFX_HIT);
                    if (mobProjs[pj].kind == 1)   // büyü çarpması: mor patlama
                        fx_burst(&weaponFx, pcx,
                                 player.rect.y + player.rect.h / 2.0f,
                                 12, 100.0f, 190, 110, 235);

                    // Zırh XP'si (melee blokla aynı kural: dmg × 0.15)
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
                }
            }

            // Gece tehlikesi: spawn 2x hızlı (dt şişirilir), algı menzili 1.5x
            float nightSpawnMul = gametime_is_night() ? 2.0f : 1.0f;
            float detectMul = (gametime_is_night() ? 1.5f * perkFx.nightDetectMult : 1.0f)
                            * perkFx.detectOnPlayerMult;   // Hayalet/Hortlak düğümleri

            // Hız çarpanı: perk (Çevik/Rüzgar/Uyumlu) × gece (Gölge Adımı) × eğilme.
            // player_update tile hızıyla çarpar (doğrudan speed yazmak eziliyordu — bug'dı)
            player.speedMult = (1.0f - armorSpeedPenalty)   // zırh ağırlığı (STR telafili)
                             * perkFx.speedMult
                             * (gametime_is_night() ? perkFx.nightSpeedMult : 1.0f)
                             * (player.sneaking
                                ? (sneak_speed_mult(player.skills[SKILL_SNEAK].level) +
                                   (1.0f - sneak_speed_mult(player.skills[SKILL_SNEAK].level)) *
                                   perkFx.sneakSpeedRelief)
                                : 1.0f)
                             * (player.speedBuffTimer > 0.0f ? player.speedBuffMult : 1.0f);
            spawn_zones_update(spawnZones, spawnZoneCount, mobs, MAX_MOBS, &nextMobId,
                               dt * nightSpawnMul);

            float px = player.rect.x + player.rect.w / 2.0f;
            float py = player.rect.y + player.rect.h / 2.0f;
            bool sneakNearDanger = false;   // use-based Sneak XP: tehlike yanında süzülme

            // Örtü: ayak bastığın tile ya da içine girdiğin dekor (çalı/saman) gizleyici mi
            float footY = player.rect.y + player.rect.h - 4.0f;
            int groundT = tile_at_point(px, footY);
            bool inConceal = (groundT >= 0 && groundT < TILE_TYPE_COUNT &&
                              TILE_TYPES[groundT].conceals)
                          || decor_conceals(px, footY);

            for (int i = 0; i < mobCount; i++)
            {
                // Algı: gece/perk çarpanı × gizlilik (eğilme + açı + siper + örtü)
                float stealthMul = perkFx.sneakDetectMult *
                                   sneak_detect_mul(player.sneaking,
                                                    player.skills[SKILL_SNEAK].level,
                                                    inConceal, px, py, &mobs[i]);
                mob_update(&mobs[i], mobs, mobCount, px, py, dt, detectMul * stealthMul);

                // Eğilmiş + fark edilmemiş + algı sınırına yakın → risk var, XP akar
                if (player.sneaking && mobs[i].e.alive && !mobs[i].aggro)
                {
                    float sdx = mobs[i].e.rect.x + mobs[i].e.rect.w / 2.0f - px;
                    float sdy = mobs[i].e.rect.y + mobs[i].e.rect.h / 2.0f - py;
                    if (SDL_sqrtf(sdx * sdx + sdy * sdy) <= mobs[i].type->detectRange * 1.2f)
                        sneakNearDanger = true;
                }

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
                }
                // Yanma: 3. bağımsız kanal (Ateş Topu) — turuncu közler
                if (mobs[i].burnTime > 0.0f && mobs[i].e.alive)
                {
                    mobs[i].burnTime -= dt;
                    mobs[i].e.health -= mobs[i].burnDps * dt;
                    if (randf() < dt * 7.0f)
                        fx_spawn(&weaponFx,
                                 mobs[i].e.rect.x + mobs[i].e.rect.w * (0.3f + randf() * 0.4f),
                                 mobs[i].e.rect.y + mobs[i].e.rect.h * 0.5f,
                                 (randf() - 0.5f) * 15.0f, -30.0f - randf() * 25.0f,
                                 0.4f, 6.0f + randf() * 6.0f, 255, 150, 50,
                                 WEAPON_FX_NONE);
                }
                // Zehir: kanamadan AYRI kanal — ikisi birden akar (tam DoT build'i)
                if (mobs[i].poisonTime > 0.0f && mobs[i].e.alive)
                {
                    mobs[i].poisonTime -= dt;
                    mobs[i].e.health -= mobs[i].poisonDps * dt;

                    // Yeşil zehir kabarcıkları: yukarı süzülür
                    if (randf() < dt * 6.0f)
                        fx_spawn(&weaponFx,
                                 mobs[i].e.rect.x + mobs[i].e.rect.w * (0.3f + randf() * 0.4f),
                                 mobs[i].e.rect.y + mobs[i].e.rect.h * 0.5f,
                                 (randf() - 0.5f) * 12.0f, -20.0f - randf() * 20.0f,
                                 0.5f, 6.0f + randf() * 5.0f, 90, 220, 60,
                                 WEAPON_FX_NONE);
                }
                // DoT ölümü (alive korumalı — direkt vuruş ölümüyle çakışmaz)
                if (mobs[i].e.alive && mobs[i].e.health <= 0.0f)
                    mob_die(&mobs[i], mobs, MAX_MOBS, &nextMobId, loots, questStatus, floatTexts, &player);

                // Düşman bu kare saldırdıysa ve oyuncu menzildeyse, hasarı BURADA uygula
                if (mobs[i].attackTriggered &&
                    mobs[i].type->attackType != ATTACK_MELEE)
                {
                    // Menzilli saldırı: hasar DEĞİL mermi doğar — isabet mermi varışında
                    if (player.targetMobId == 0 && targetNpc < 0)
                        player.targetMobId = mobs[i].id;

                    float sx = mobs[i].e.rect.x + mobs[i].e.rect.w / 2.0f;
                    float sy = mobs[i].e.rect.y + mobs[i].e.rect.h / 2.0f - 8.0f;
                    float pdx = px - sx, pdy = py - sy;
                    float plen = SDL_sqrtf(pdx * pdx + pdy * pdy);
                    if (plen > 1.0f)
                    {
                        for (int pj = 0; pj < MAX_MOB_PROJS; pj++)
                        {
                            if (mobProjs[pj].active) continue;
                            mobProjs[pj] = (MobProj){
                                .active = true, .x = sx, .y = sy,
                                .vx = pdx / plen * mobs[i].type->projSpeed,
                                .vy = pdy / plen * mobs[i].type->projSpeed,
                                .dmg = mobs[i].type->damage,
                                .kind = (mobs[i].type->attackType == ATTACK_MAGIC) ? 1 : 0,
                            };
                            break;
                        }
                    }
                }
                else if (mobs[i].attackTriggered)
                {
                    // Saldıran mob otomatik hedeflenir (yalnız hedef boşken —
                    // seçili mob/NPC hedefini savaş ortasında EZMEZ)
                    if (player.targetMobId == 0 && targetNpc < 0)
                        player.targetMobId = mobs[i].id;

                    bool dodged = false;
                    bool yaraliAyi = perkFx.yaraliAyi;
                    float dmg = combat_mob_damage(mobs[i].type->damage, &equipment,
                                                  &perkFx,
                                                  player.state == PLAYER_IDLE,
                                                  player.health / player.maxHealth,
                                                  yaraliAyi, &dodged);
                    if (!dodged)
                        dmg = mana_shield_absorb(&player, dmg, floatTexts);
                    if (dodged)
                    {
                        SDL_Color cyan = { 90, 220, 255, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             px, player.rect.y - 10.0f, "Kaçındın!", cyan);
                        audio_play(SFX_DODGE);
                        fx_burst(&weaponFx, px, py, 10, 120.0f, 235, 245, 255);   // beyaz iz
                    }
                    else if (dmg > 0.0f)
                    {
                        player.health -= dmg;
                        player.gaziTimer = 5.0f;   // Gazi buff penceresi (perk yoksa etkisiz)
                        gatherRes = -1;            // hasar toplama/craft sürecini keser
                        craftOpen = false;
                        craftRecipe = -1;
                        plusForge = false;
                        enchAction = 0;
                        craftProgress = 0.0f;

                        // Zehirli vuruş (mavi slime): süre tazelenir, üst üste binmez
                        if (mobs[i].type->hitPoisonDps > 0.0f)
                        {
                            bool fresh = player.poisonTimer <= 0.0f;
                            player.poisonDps = mobs[i].type->hitPoisonDps;
                            player.poisonTimer = mobs[i].type->hitPoisonTime;
                            if (fresh)
                            {
                                SDL_Color pgreen = { 120, 230, 90, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     px, player.rect.y - 26.0f,
                                                     "Zehirlendin!", pgreen);
                            }
                        }

                        // His katmanı: kısa sarsıntı + kırmızı kenar flash'ı
                        shakeTime = shakeDur = 0.25f;
                        shakeAmp = 6.0f;
                        flashTime = FLASH_DUR;

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

                        // Yansıtma: Karşılık (%) + Diken Zırh (sabit) + Alev Zırhı (büyü)
                        float reflect = dmg * perkFx.reflectFrac + perkFx.reflectFlat
                                      + (player.flameArmorTimer > 0.0f ? 8.0f : 0.0f);
                        if (reflect > 0.0f && mobs[i].e.alive)
                        {
                            mobs[i].e.health -= reflect;
                            audio_play(SFX_REFLECT);
                            fx_burst(&weaponFx,
                                     mobs[i].e.rect.x + mobs[i].e.rect.w / 2.0f,
                                     mobs[i].e.rect.y + mobs[i].e.rect.h / 2.0f,
                                     6, 70.0f, 255, 160, 60);   // turuncu kıvılcım
                            if (mobs[i].e.health <= 0.0f)
                                mob_die(&mobs[i], mobs, MAX_MOBS, &nextMobId, loots, questStatus, floatTexts, &player);
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
                                                 "Ölümden döndün!", gold);
                            audio_play(SFX_DEATHSAVE);
                            fx_burst(&weaponFx, px, py, 40, 150.0f, 255, 215, 90);   // altın patlama
                        }
                        if (player.health < 0.0f)
                            player.health = 0.0f;
                    }
                }
            }

            // Use-based Sneak XP: eğilmiş + fark edilmemiş + tehlike algı sınırında
            if (sneakNearDanger)
            {
                if (skill_add_xp(player.skills, SKILL_SNEAK, SNEAK_XP_TICK * dt))
                    on_skill_levelup(&player, SKILL_SNEAK, &hotbar, floatTexts);
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

            // Sarsıntı: kalan süreyle sönümlenen rastgele kamera itmesi
            if (shakeTime > 0.0f)
            {
                shakeTime -= dt;
                float k = (shakeTime > 0.0f) ? shakeTime / shakeDur : 0.0f;
                float a = shakeAmp * k / camera.zoom;   // ekran px → dünya px
                camera.x += (randf() * 2.0f - 1.0f) * a;
                camera.y += (randf() * 2.0f - 1.0f) * a;
            }
            if (flashTime > 0.0f)
                flashTime -= dt;

            // E prompt'u için menzildeki en yakın NPC
            nearestNpc = npc_nearest_in_range(px, py);
            nearestRes = resource_nearest_available(px, py);
            nearestStation = station_nearest(px, py);

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
        resource_render_all(renderer, resource_textures, &camera, gameFont,
                            (dialogNpc < 0 && nearestNpc < 0) ? nearestRes : -1);  // NPC önceliği
        station_render_all(renderer, station_textures, &camera, gameFont,
                           (dialogNpc < 0 && nearestNpc < 0 && !craftOpen) ? nearestStation : -1,
                           (craftOpen && (craftRecipe >= 0 || plusForge || enchAction > 0))
                               ? craftStation : -1);

        // İlan panosu (dünya objesi): taban-orta hizalı, yaklaşınca [E] ipucu
        if (boardTex != NULL)
        {
            const float bScale = 2.0f, bw = 27.0f * bScale, bh = 40.0f * bScale;
            SDL_FRect bsrc = { 0, 0, 27, 40 };
            SDL_FRect bdst = world_to_screen(BOARD_X - bw / 2.0f, BOARD_Y - bh,
                                             bw, bh, &camera);
            r2d_draw_texture(renderer, boardTex, &bsrc, &bdst);

            float bpdx = BOARD_X - (player.rect.x + player.rect.w / 2.0f);
            float bpdy = BOARD_Y - (player.rect.y + player.rect.h / 2.0f);
            if (!boardOpen && gameFont != NULL &&
                SDL_sqrtf(bpdx * bpdx + bpdy * bpdy) <= BOARD_RANGE)
            {
                SDL_FRect tp = world_to_screen(BOARD_X - 34.0f, BOARD_Y - bh - 22.0f,
                                               0, 0, &camera);
                SDL_Color hintCol = { 255, 255, 210, 255 };
                draw_text(renderer, gameFont, "[E] İlanlar", tp.x, tp.y, hintCol);
            }
        }

        // Yapışkan tuzaklar: zemin objesi (mobların/oyuncunun ALTINDA) —
        // yassılmış jel görseli (SLIME_GEL ikonu genişletilmiş)
        {
            R2DTexture *gooTex = item_icon_tex(&itemTex, &ITEM_TYPES[ITEM_SLIME_GEL]);
            for (int tI = 0; gooTex != NULL && tI < MAX_TRAPS; tI++)
            {
                if (!traps[tI].active) continue;
                SDL_FRect gsrc = ITEM_TYPES[ITEM_SLIME_GEL].iconSrc;
                SDL_FRect gdst = world_to_screen(traps[tI].x - 18.0f,
                                                 traps[tI].y - 10.0f,
                                                 36.0f, 14.0f, &camera);
                r2d_draw_texture(renderer, gooTex, &gsrc, &gdst);
            }
        }

        // Toprak Dikenleri: zemin patlaması (moblardan/oyuncudan ÖNCE — yerden fışkırır)
        if (spikesTex != NULL)
            for (int q = 0; q < MAX_SPIKE_FX; q++)
            {
                if (!spikeFx[q].active) continue;
                int sfr = (int)(spikeFx[q].t / 0.06f);
                if (sfr > 9) sfr = 9;
                SDL_FRect ssrc = { (float)(sfr * 64), 64, 64, 64 };   // satır 1: toprak kümesi
                SDL_FRect sdst = world_to_screen(spikeFx[q].x - 32.0f,
                                                 spikeFx[q].y - 52.0f,
                                                 64.0f, 64.0f, &camera);
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                r2d_draw_texture(renderer, spikesTex, &ssrc, &sdst);
            }

        PlayerLayer built[24];
        int builtCount = build_player_layers(layers, LAYER_COUNT, &equipment,
                                             &itemTex, built, 24);

        player_render(&player, renderer, built, builtCount, &camera); // oyuncuyu
        
        for (int i = 0; i < mobCount; i++)
            mob_render(&mobs[i], renderer, mob_textures, &camera, player.targetMobId);

        npc_render_all(renderer, npc_textures, &camera, gameFont, nearestNpc);

        // Hedeflenen NPC'nin can barı (isim etiketinin üstünde, dünya uzayı)
        if (targetNpc >= 0)
        {
            float nx, ny;
            if (npc_visible_pos(targetNpc, &nx, &ny))
            {
                float ratio = npc_health(targetNpc) / npc_max_health(targetNpc);
                SDL_FRect bg = world_to_screen(nx - 24.0f, ny - 116.0f, 48.0f, 6.0f, &camera);
                SDL_FRect fill = bg;
                fill.w *= ratio;
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                r2d_set_color(renderer, 20, 20, 25, 220);
                r2d_fill_rect(renderer, &bg);
                r2d_set_color(renderer, 220, 60, 60, 255);
                r2d_fill_rect(renderer, &fill);
            }
        }

        // Yanan moblarda alev (12 karelik piksel alev — Ateş Topu DoT görseli)
        if (flameTex != NULL)
            for (int i = 0; i < mobCount; i++)
            {
                if (!mobs[i].e.alive || mobs[i].burnTime <= 0.0f) continue;
                int ff = (int)((SDL_GetTicks() / 80) % 12);
                SDL_FRect fsrc = { (float)((ff % 4) * 16), (float)((ff / 4) * 24), 16, 24 };
                SDL_FRect fdst = world_to_screen(
                    mobs[i].e.rect.x + mobs[i].e.rect.w / 2.0f - 12.0f,
                    mobs[i].e.rect.y - 10.0f, 24.0f, 36.0f, &camera);
                r2d_draw_texture(renderer, flameTex, &fsrc, &fdst);
            }

        // Alev Zırhı aurası: oyuncunun iki yanında animasyonlu alevler
        if (flameTex != NULL && player.flameArmorTimer > 0.0f)
            for (int fside = 0; fside < 2; fside++)
            {
                int ff = (int)((SDL_GetTicks() / 90 + fside * 6) % 12);
                SDL_FRect fsrc = { (float)((ff % 4) * 16), (float)((ff / 4) * 24), 16, 24 };
                SDL_FRect fdst = world_to_screen(
                    player.rect.x + (fside == 0 ? -20.0f : player.rect.w + 2.0f),
                    player.rect.y + player.rect.h * 0.35f, 20.0f, 30.0f, &camera);
                r2d_draw_texture(renderer, flameTex, &fsrc, &fdst);
            }

        // Büyü Kalkanı kubbesi: oluşum(0-7) → dönme(8-11) → kırılma(12-15)
        if (shieldTex != NULL &&
            (player.manaShieldTimer > 0.0f || player.shieldBreakT > 0.0f))
        {
            int sf;
            if (player.shieldBreakT > 0.0f)
            {
                sf = 12 + (int)(player.shieldBreakT / 0.07f);
                if (sf > 15) sf = 15;
            }
            else
            {
                float elapsed = 12.0f - player.manaShieldTimer;
                if (elapsed < 8 * 0.06f) sf = (int)(elapsed / 0.06f);
                else                     sf = 8 + (int)(elapsed / 0.12f) % 4;
            }
            SDL_FRect ksrc = { (float)((sf % 4) * 128), (float)((sf / 4) * 128),
                               128, 128 };
            SDL_FRect kdst = world_to_screen(
                player.rect.x + player.rect.w / 2.0f - 45.0f,
                player.rect.y + player.rect.h / 2.0f - 55.0f,
                90.0f, 90.0f, &camera);
            r2d_texture_set_alpha_mod(shieldTex, 170);
            r2d_set_blend(renderer, R2D_BLEND_ALPHA);
            r2d_draw_texture(renderer, shieldTex, &ksrc, &kdst);
        }

        // Uçan oklar: hedefe dönük kısa çizgi (gövde koyu + uç açık)
        for (int a = 0; a < MAX_ARROWS; a++)
        {
            if (!arrows[a].active) continue;
            Mob *at = mob_find_by_id(mobs, mobCount, arrows[a].targetId);
            if (at == NULL) continue;
            float tx = at->e.rect.x + at->e.rect.w / 2.0f;
            float ty = at->e.rect.y + at->e.rect.h / 2.0f;
            float adx = tx - arrows[a].x, ady = ty - arrows[a].y;
            float alen = SDL_sqrtf(adx * adx + ady * ady);
            if (alen < 1.0f) continue;
            adx /= alen; ady /= alen;

            SDL_FRect tip  = world_to_screen(arrows[a].x, arrows[a].y, 0, 0, &camera);
            SDL_FRect tail = world_to_screen(arrows[a].x - adx * 12.0f,
                                             arrows[a].y - ady * 12.0f, 0, 0, &camera);
            r2d_set_blend(renderer, R2D_BLEND_ALPHA);
            r2d_set_color(renderer, 120, 85, 50, 255);     // ahşap gövde
            r2d_line(renderer, tail.x, tail.y, tip.x, tip.y);
            r2d_set_color(renderer, 230, 230, 240, 255);   // metal uç parıltısı
            r2d_line(renderer, tip.x - adx * 3.0f, tip.y - ady * 3.0f, tip.x, tip.y);
        }

        // Ateş Topu mermileri: piksel küre sprite'ı (Sog paketi)
        if (orbFireTex != NULL)
            for (int sp = 0; sp < MAX_SPELL_PROJS; sp++)
            {
                if (!spellProjs[sp].active) continue;
                SDL_FRect osrc = { 0, 0, 20, 24 };
                SDL_FRect odst = world_to_screen(spellProjs[sp].x - 15.0f,
                                                 spellProjs[sp].y - 18.0f,
                                                 30.0f, 36.0f, &camera);
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                r2d_draw_texture(renderer, orbFireTex, &osrc, &odst);
            }

        // Arcane patlamaları (Yıldırım çarpması + cast parlaması) — additive
        if (arcaneTex != NULL)
            for (int b = 0; b < MAX_BURSTS; b++)
            {
                if (!bursts[b].active) continue;
                int frame = (int)(bursts[b].t / 0.045f);
                if (frame > 6) frame = 6;
                SDL_FRect bsrc = { (float)(frame * 96), 0, 96, 96 };
                float bs = 96.0f * bursts[b].scale;
                SDL_FRect bdst = world_to_screen(bursts[b].x - bs / 2.0f,
                                                 bursts[b].y - bs / 2.0f,
                                                 bs, bs, &camera);
                r2d_set_blend(renderer, R2D_BLEND_ADD);
                r2d_draw_texture(renderer, arcaneTex, &bsrc, &bdst);
            }
        r2d_set_blend(renderer, R2D_BLEND_ALPHA);

        // Mob mermileri: iskelet oku = kemik rengi çizgi; büyü topu = mor çekirdek
        for (int pj = 0; pj < MAX_MOB_PROJS; pj++)
        {
            if (!mobProjs[pj].active) continue;
            float vlen = SDL_sqrtf(mobProjs[pj].vx * mobProjs[pj].vx +
                                   mobProjs[pj].vy * mobProjs[pj].vy);
            if (vlen < 1.0f) continue;
            float ndx = mobProjs[pj].vx / vlen, ndy = mobProjs[pj].vy / vlen;
            SDL_FRect tip  = world_to_screen(mobProjs[pj].x, mobProjs[pj].y, 0, 0, &camera);
            r2d_set_blend(renderer, R2D_BLEND_ALPHA);
            if (mobProjs[pj].kind == 0)
            {
                SDL_FRect tail = world_to_screen(mobProjs[pj].x - ndx * 12.0f,
                                                 mobProjs[pj].y - ndy * 12.0f, 0, 0, &camera);
                r2d_set_color(renderer, 210, 200, 180, 255);   // kemik rengi gövde
                r2d_line(renderer, tail.x, tail.y, tip.x, tip.y);
                r2d_set_color(renderer, 120, 120, 130, 255);   // demir uç
                r2d_line(renderer, tip.x - ndx * 3.0f, tip.y - ndy * 3.0f, tip.x, tip.y);
            }
            else if (orbBlueTex != NULL)
            {
                SDL_FRect bsrc = { 0, 0, 15, 18 };
                SDL_FRect core = world_to_screen(mobProjs[pj].x - 11.0f,
                                                 mobProjs[pj].y - 13.0f,
                                                 22.0f, 27.0f, &camera);
                r2d_draw_texture(renderer, orbBlueTex, &bsrc, &core);
            }
            else
            {
                SDL_FRect core = world_to_screen(mobProjs[pj].x - 4.0f,
                                                 mobProjs[pj].y - 4.0f, 8.0f, 8.0f, &camera);
                r2d_set_color(renderer, 200, 130, 245, 255);   // mor çekirdek (fallback)
                r2d_fill_rect(renderer, &core);
            }
        }

        // Partiküller dünya varlıklarının ÜSTÜNDE (kan damlası mobun önünde görünsün)
        fx_render(&weaponFx, renderer, glowTex, &camera);

        if (openLootIndex >= 0)
            loot_popup_render(renderer, &loots[openLootIndex], itemIconTex, &itemTex, gameFont, &camera);

        loot_render_all(loots, MAX_LOOT, renderer, crateTex, &camera); // ← kasalar (mob'lardan sonra, HUD'dan önce)

        // Hasar yazıları (mob'ların üstünde, dünya uzayında)
        floattext_render_all(floatTexts, MAX_FLOAT_TEXTS, renderer, gameFont, &camera); // ← BURAYA (present'tan önce)

        // GÜN/GECE aydınlatması: shader ışık pass'i — dünyanın üstüne, HUD'un
        // altına (HUD gece de okunur). Ambient ay ışığı mavisine iner; ışık
        // kaynakları (pencere/direk/meşale) piksel başına sıcak renk katar.
        float dark01 = gametime_darkness01();
        if (dark01 > 0.0f)
        {
            // Zifiri gecede sahne çarpanı (mavi ağırlıklı ay ışığı)
            float ambR = 1.0f - dark01 * (1.0f - 0.30f);
            float ambG = 1.0f - dark01 * (1.0f - 0.33f);
            float ambB = 1.0f - dark01 * (1.0f - 0.55f);

            // Altın saat: alacakaranlık/şafak ortasında sahne turuncuya ısınır
            // (sin eğrisi: geçişin ortasında tepe, gündüz ve zifiri gecede sıfır)
            float golden = SDL_sinf(dark01 * SDL_PI_F) * 0.18f;
            ambR += golden;
            ambG += golden * 0.45f;

            // Meşale: envanterde varsa oyuncu ışık taşır
            bool hasTorch = false;
            for (int i = 0; i < INVENTORY_SIZE; i++)
                if (inventory.slots[i].type == &ITEM_TYPES[ITEM_TORCH]) { hasTorch = true; break; }

            R2DLight lights[R2D_MAX_LIGHTS];
            int lightN = light_collect(lights, R2D_MAX_LIGHTS, &camera, winW, winH,
                                       player.rect.x + player.rect.w / 2.0f,
                                       player.rect.y + player.rect.h / 2.0f,
                                       hasTorch, player.weaponFx);
            // Kamp ateşleri: yanan her ateş (köy sabiti + oyuncununki) ışık verir
            for (int i = 0; i < STATION_COUNT; i++)
                if (STATIONS[i].type == STATION_CAMPFIRE &&
                    STATIONS[i].x > CAMPFIRE_PARKED + 1.0f)
                    lightN = light_append_campfire(lights, lightN, R2D_MAX_LIGHTS,
                                                   &camera, winW, winH,
                                                   STATIONS[i].x, STATIONS[i].y - 24.0f);
            r2d_light_pass(renderer, ambR, ambG, ambB, lights, lightN);
        }

        // Hasar flash'ı: kırmızı kenar vinyeti, hızla söner (HUD'un altında)
        if (flashTime > 0.0f && vignetteTex != NULL)
        {
            float k = flashTime / FLASH_DUR;
            r2d_texture_set_color_mod(vignetteTex, 200, 20, 20);
            r2d_texture_set_alpha_mod(vignetteTex, (Uint8)(k * 170.0f));
            r2d_set_blend(renderer, R2D_BLEND_ALPHA);
            SDL_FRect full = { 0, 0, (float)winW, (float)winH };
            r2d_draw_texture(renderer, vignetteTex, NULL, &full);
            r2d_texture_set_color_mod(vignetteTex, 255, 255, 255);
            r2d_texture_set_alpha_mod(vignetteTex, 255);
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
            draw_text(renderer, gameFont, goldBuf, 20.0f, 76.0f, goldCol);   // mana/stamina barlarının altı

            // Bounty + itibar göstergesi (bulunduğun bölgenin değerleri)
            int hudRegion = region_of(player.rect.x + player.rect.w / 2.0f,
                                      player.rect.y + player.rect.h / 2.0f);
            float hudY = 100.0f;
            if (player.bounty[hudRegion] > 0)
            {
                char bountyBuf[32];
                snprintf(bountyBuf, sizeof(bountyBuf), "Bounty %d", player.bounty[hudRegion]);
                SDL_Color bountyCol = { 255, 90, 90, 255 };
                draw_text(renderer, gameFont, bountyBuf, 20.0f, hudY, bountyCol);
                hudY += 24.0f;
            }
            if (player.reputation[hudRegion] <= -1.0f || player.reputation[hudRegion] >= 1.0f)
            {
                char repBuf[32];
                snprintf(repBuf, sizeof(repBuf), "İtibar %d", (int)player.reputation[hudRegion]);
                SDL_Color repCol = player.reputation[hudRegion] < 0.0f
                                 ? (SDL_Color){ 255, 140, 100, 255 }
                                 : (SDL_Color){ 140, 230, 140, 255 };
                draw_text(renderer, gameFont, repBuf, 20.0f, hudY, repCol);
                hudY += 24.0f;
            }

            // Eğilme göstergesi
            if (player.sneaking)
            {
                SDL_Color sneakCol = { 170, 170, 190, 255 };
                draw_text(renderer, gameFont, "[Gizli]", 20.0f, hudY, sneakCol);
            }

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

        // Yay kuşanılıyken aktif ok sayacı (hotbar'ın üstünde) — tür adıyla
        if (player.bowEquipped && gameFont != NULL)
        {
            int nArrow = inventory_count(&inventory, &ITEM_TYPES[activeAmmo]);
            char ammoBuf[40];
            snprintf(ammoBuf, sizeof(ammoBuf), "%s: %d",
                     ITEM_TYPES[activeAmmo].name, nArrow);
            SDL_Color ammoCol = ammo_any(&inventory)
                              ? (SDL_Color){ 230, 220, 180, 255 }
                              : (SDL_Color){ 255, 120, 100, 255 };
            draw_text(renderer, gameFont, ammoBuf,
                      winW / 2.0f - 40.0f, winH - 96.0f, ammoCol);
        }

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
            TTF_GetStringSize(splashFont, "Öldün", 0, &tw, &th);
            SDL_Color red = { 255, 80, 80, 255 };
            draw_text(renderer, splashFont, "Öldün",
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
            int panelY = 20;   // 35 slotluk çanta (5 satır) 720px'e ancak böyle sığıyor
            equipment_render(&equipment, renderer, itemIconTex, &itemTex,
                             built, builtCount, panelX, panelY, gameFont);
            inventory_render(&inventory, renderer, itemIconTex, &itemTex, gameFont,
                             (float)panelX, (float)(panelY + 340));
        }

        if (skillsOpen)
        {
            // Envanter paneliyle çakışmasın diye sol tarafta
            skill_panel_render(player.skills, renderer, gameFont, 40.0f, 40.0f);

            // Skill ağacı paneli (Faz 21): satıra tıklayınca sağda açılır.
            // Respec satırı yalnız sunak yakınında + puan harcanmışsa görünür.
            if (treeOpenSkill >= 0)
            {
                int ts = treeOpenSkill;
                bool isFate = (ts == TREE_FATE);
                Uint32 tsTaken = isFate ? player.fateTaken : player.treeTaken[ts];
                int tsLevel = 0;
                if (isFate)
                    for (int sl = 0; sl < SKILL_COUNT; sl++)
                        tsLevel += player.skills[sl].level;
                else
                    tsLevel = player.skills[ts].level;

                int spent = tree_points_spent(ts, tsTaken);
                float adx = STATIONS[2].x - (player.rect.x + player.rect.w / 2.0f);
                float ady = STATIONS[2].y - (player.rect.y + player.rect.h / 2.0f);
                bool nearAltar =
                    SDL_sqrtf(adx * adx + ady * ady) <= STATION_INTERACT_RANGE * 1.5f;
                int respecCost = (nearAltar && spent > 0) ? spent * 50 : -1;

                tree_panel_render(ts, tsTaken, tsLevel,
                                  treeSelNode, respecCost,
                                  renderer, gameFont,
                                  40.0f + SKILL_PANEL_W + 10.0f, 40.0f);
            }
        }

        if (shopOpen)
            shop_render(renderer, gameFont, itemIconTex, &itemTex, player.gold,
                        rep_buy_mult(player.reputation[region_of(player.rect.x, player.rect.y)]),
                        40.0f, 40.0f);

        // Craft paneli + aktif sürecin ilerleme barı
        if (craftOpen && craftStation >= 0)
        {
            // Sunakta 'hedef' satırı seçili ekipmanı gösterir; diğerlerinde kuşanılı silah
            const ItemStack *pw = (STATIONS[craftStation].type == STATION_ENCHANT)
                                ? &equipment.slots[enchTarget]
                                : &equipment.slots[EQUIP_WEAPON];
            craft_panel_render(craftStation, &inventory, player.skills,
                               craftRecipe,
                               plus_chance(equipment.slots[EQUIP_WEAPON].plusLevel + 1,
                                           player.skills[SKILL_SMITHING].level),
                               equipment.slots[EQUIP_WEAPON].plusLevel + 1, pw,
                               renderer, gameFont, 40.0f, 40.0f);

            if (craftRecipe >= 0 || plusForge || enchAction > 0)
            {
                float work = plusForge ? 8.0f
                           : (enchAction > 0) ? ENCH_RUNE_CAST_TIME
                           : RECIPES[craftRecipe].work;
                SDL_FRect bg = { 40.0f, 28.0f, 380.0f, 8.0f };
                SDL_FRect fill = bg;
                fill.w *= (craftProgress / work > 1.0f) ? 1.0f : craftProgress / work;
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                r2d_set_color(renderer, 25, 25, 30, 220);
                r2d_fill_rect(renderer, &bg);
                r2d_set_color(renderer, 255, 180, 80, 255);
                r2d_fill_rect(renderer, &fill);
            }
        }

        // İlan panosu paneli (Faz 6): günün kontratları + kabul/teslim
        if (boardOpen)
            contract_panel_render(renderer, gameFont, &inventory,
                                  player.reputation[region_of(BOARD_X, BOARD_Y)],
                                  40.0f, 40.0f);

        // Kamp ateşi kurulum barı (panelsiz süreç — karakterin üstünde)
        if (campBuild >= 0.0f)
        {
            SDL_FRect cbg = { winW / 2.0f - 100.0f, winH / 2.0f - 90.0f, 200.0f, 8.0f };
            SDL_FRect cfill = cbg;
            cfill.w *= campBuild / CAMPFIRE_BUILD_TIME;
            r2d_set_blend(renderer, R2D_BLEND_ALPHA);
            r2d_set_color(renderer, 25, 25, 30, 220);
            r2d_fill_rect(renderer, &cbg);
            r2d_set_color(renderer, 255, 170, 60, 255);
            r2d_fill_rect(renderer, &cfill);
        }

        if (dialogNpc >= 0)
        {
            // Seçenek durumları (kilit/zar yüzdesi) — render soluk + etiketli çizer
            DialogNode *dnode = &npcDialogs[dialogNpc].nodes[dialogNode];
            DialogOptState dstates[DIALOG_MAX_OPTS];
            float dnx, dny;
            int dRegion = npc_visible_pos(dialogNpc, &dnx, &dny)
                        ? region_of(dnx, dny) : 0;
            for (int oi = 0; oi < dnode->optCount; oi++)
                dialog_eval_opt(&dnode->opts[oi], &player, dRegion,
                                dialogNpc < 8 &&
                                dlgBurnDay[dialogNpc][dialogNode][oi] == gametime_day(),
                                &dstates[oi]);
            dialog_render(&npcDialogs[dialogNpc], dialogNode, NPCS[dialogNpc].name,
                          dstates, renderer, gameFont, winW, winH);
        }

        // Muhafız yakalama paneli: öde / karşı koy / rüşvet
        if (catchOpen && gameFont != NULL)
        {
            float pw = 480.0f, ph = 168.0f;
            float pX = (winW - pw) / 2.0f, pY = winH / 3.0f;
            ui_panel(renderer, pX, pY, pw, ph);

            SDL_Color white = { 240, 240, 245, 255 };
            SDL_Color gold  = { 255, 210, 60, 255 };
            char line[80];
            draw_text(renderer, gameFont, "Muhafız: Dur! Kanunu çiğnedin.",
                      pX + 24.0f, pY + 22.0f, white);
            snprintf(line, sizeof(line), "Ceza: %d altın (sende: %d)",
                     player.bounty[guardRegion], player.gold);
            draw_text(renderer, gameFont, line, pX + 24.0f, pY + 54.0f, gold);
            draw_text(renderer, gameFont, "[1] Cezayı öde    [2] Karşı koy",
                      pX + 24.0f, pY + 94.0f, white);

            int bribeSp = player.skills[SKILL_SPEECH].level;
            int bribePct = (bribeSp >= CRIME_BRIBE_SPEECH) ? 100
                         : (int)((float)bribeSp / CRIME_BRIBE_SPEECH * 70.0f);
            int bribeShown = (int)(player.bounty[guardRegion] / 2 *
                                   perkFx.bribeCostMult);   // Arabulucu indirimi dahil
            if (bribePct >= 100)
                snprintf(line, sizeof(line), "[3] Rüşvet teklif et (%dg) [Speech %d]",
                         bribeShown, CRIME_BRIBE_SPEECH);
            else
                snprintf(line, sizeof(line), "[3] Rüşvet teklif et (%dg) [Speech %d - %%%d]",
                         bribeShown, CRIME_BRIBE_SPEECH, bribePct);
            SDL_Color bribeCol = { 200, 170, 255, 255 };
            draw_text(renderer, gameFont, line, pX + 24.0f, pY + 122.0f, bribeCol);
        }

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

    for (int i = 0; i < DECOR_COUNT && i < 32; i++)
        if (decor_textures[i])
            r2d_texture_destroy(NULL, decor_textures[i]);
    for (int i = 0; i < RESOURCE_COUNT && i < MAX_RESOURCES; i++)
        if (resource_textures[i])
            r2d_texture_destroy(NULL, resource_textures[i]);
    for (int i = 0; i < STATION_COUNT && i < 8; i++)
        if (station_textures[i])
            r2d_texture_destroy(NULL, station_textures[i]);
    if (boardTex)
        r2d_texture_destroy(NULL, boardTex);
    if (orbFireTex) r2d_texture_destroy(NULL, orbFireTex);
    if (orbBlueTex) r2d_texture_destroy(NULL, orbBlueTex);
    if (arcaneTex)  r2d_texture_destroy(NULL, arcaneTex);
    if (shieldTex)  r2d_texture_destroy(NULL, shieldTex);
    if (spikesTex)  r2d_texture_destroy(NULL, spikesTex);
    if (flameTex)   r2d_texture_destroy(NULL, flameTex);

    if (glowTex)
        r2d_texture_destroy(NULL, glowTex);
    if (vignetteTex)
        r2d_texture_destroy(NULL, vignetteTex);

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