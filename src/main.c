#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h> // platformlar arası 'main' köprülemesi (mobil/web için de gerekli)
#include <stdbool.h>
#include <string.h>   // diyalog hedef karşılaştırmaları
#include "font.h"
#include "common.h"
#include "menu.h"
#include "floattext.h"
#include "skillfx.h"
#include "groundfx.h"
#include "tilemap.h"
#include "player.h"
#include "mob.h"
#include "render.h"
#include "loot.h"
#include "inventory.h"
#include "equipment.h"
#include "item_textures.h"
#include "fx.h"
#include "fxtex.h"
#include "skill.h"
#include "combat.h"
#include "save.h"
#include "hotbar.h"
#include "region.h"
#include "worldmap.h"
#include "lockpick.h"
#include "trap.h"
#include "scene.h"
#include "npc.h"
#include "dialog.h"
#include "shop.h"
#include "quest.h"
#include "journal.h"
#include "ecology.h"
#include "worldevent.h"
#include "weather.h"
#include "tooltip.h"
#include "ambience.h"
#include "title.h"
#include "ally.h"
#include "gametime.h"
#include "decor.h"
#include "shadow.h"
#include "decal.h"
#include "uilayout.h"   // HUD konumları (assets/ui/layout.json)
#include "uieditor.h"   // Ctrl+U: oyun içi yerleşim düzenleyicisi
#include "audio.h"
#include "music.h"
#include "minimap.h"
#include "ui_theme.h"
#include "perk.h"
#include "light.h"
#include "lamp.h"   // kandil direği — LORE §1.1/§1.4/§1.5
#include "crime.h"
#include "guard.h"
#include "sneak.h"
#include "resource.h"
#include "craft.h"
#include "contract.h"
#include "skilltree.h"
#include "assassin.h"

#define BASE_MAX_HP 100.0f   // oyuncu taban canı (perk bonusları üstüne biner)

// Bölge indeksi (dünya konumundan); bölge dışı → 0 (Yeşil Ova varsayılan)
static int region_of(float x, float y)
{
    // Suç defteri HARİTADAN gelir (MapDef.crimeRegion). Eskiden burada elle
    // yazılmış `if (map_current() == MAP_DOGU) return 5;` vardı ve bölgesiz
    // haritalarda 0 dönüyordu — yani zindanda, konakta, LOCADA işlenen suç
    // Köy'ün defterine yazılıyordu. Locada suikastçi ustasına vurunca köy
    // muhtarı bounty istiyordu.
    int cr = MAPS[map_current()].crimeRegion;
    if (cr == CRIME_LAWLESS) return REGION_KANUNSUZ;   // sahipsiz sicil
    if (cr >= 0) return (cr < MAX_REGIONS) ? cr : REGION_KOY;
    int r = region_at(x, y);   // CRIME_BY_RECT: dünya haritası, konumdan
    return (r < 0 || r >= MAX_REGIONS) ? REGION_KOY : r;
}

// Diyalog koşul değerlendirmesi: sert kilitler (gold/rep/bounty) + speech zar yüzdesi.
// Zar modeli: eşik üstü %100, altı (speech/eşik) × %70.
static void dialog_eval_opt(const DialogOpt *o, const Player *p, int npcRegion,
                            bool burned, DialogOptState *out)
{
    out->locked = false;
    out->burned = burned;
    out->hidden = false;
    out->chancePct = -1;

    for (int i = 0; i < o->reqCount; i++) {
        const DialogReq *r = &o->reqs[i];
        float cur = 0.0f;
        switch (r->var) {
            case DVAR_SPEECH: cur = (float)p->skills[SKILL_SPEECH].level; break;
            case DVAR_GOLD:   cur = (float)p->gold; break;
            case DVAR_REP:    cur = p->reputation[npcRegion]; break;
            case DVAR_BOUNTY: cur = (float)p->bounty[npcRegion]; break;
            case DVAR_KABA:   cur = (float)p->persKaba; break;
            case DVAR_KIBAR:  cur = (float)p->persKibar; break;
            case DVAR_KETUM:  cur = (float)p->persKetum; break;
            case DVAR_HINT:   cur = (float)p->storyFlags[SF_S3_HINTS]; break;
            case DVAR_EVENT:  cur = (float)worldevent_current(); break;
            case DVAR_WEATHER: cur = (float)weather_current(); break;
        }
        bool pass = r->lessEq ? (cur <= r->value) : (cur >= r->value);

        // Dünya-durumu koşulları (olay/hava): tutmuyorsa seçenek GİZLENİR —
        // "kilitli konu" değil, konuşulacak konu yok
        if ((r->var == DVAR_EVENT || r->var == DVAR_WEATHER) && !pass) {
            out->hidden = true;
            out->locked = true;
            continue;
        }

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
            // Kişilik izi (Faz B1): gizli sayaçlar — yalnız birikir (+)
            case DVAR_KABA:  if (x->op == '+') p->persKaba  += (int)v; break;
            case DVAR_KIBAR: if (x->op == '+') p->persKibar += (int)v; break;
            case DVAR_KETUM: if (x->op == '+') p->persKetum += (int)v; break;
            default: break;   // DVAR_SPEECH parser'da reddedilir
        }
    }
}

// Zindan sandığını kademesine göre doldur (tablolar: denge PROGRESS'te)
static void chest_fill(Loot *l, int tier, int today)
{
    (void)today;
    l->itemCount = 0;
    int n = 0;
    switch (tier) {
    case 2:   // USTA: garantili büyük ödül
        l->gold = 120 + rand() % 81;
        l->items[n++] = item_make(&ITEM_TYPES[(rand() % 2) ? ITEM_SCROLL_ARCHERY
                                                    : ITEM_SCROLL_ENCHANT], 1, 0);
        if (randf() < 0.25f)   // Hortum tomarı: türün tek kaynağı (Büyü dalgası 2)
            l->items[n++] = item_make(&ITEM_TYPES[ITEM_TOME_TORNADO], 1, 0);
        if (randf() < 0.15f)   // Kutsal tomarları (Faz HOLY) — USTA havuzu
            l->items[n++] = item_make(&ITEM_TYPES[(rand() % 2)
                                ? ITEM_TOME_HOLYBEAM : ITEM_TOME_HOLYZONE], 1, 0);
        l->items[n++] = item_make(&ITEM_TYPES[ITEM_INGOT_IRON], 1 + rand() % 2, 0);
        if (randf() < 0.5f)
            l->items[n++] = item_make(&ITEM_TYPES[ITEM_STONE_HP], 1, 0);
        break;
    case 1:   // SAĞLAM
        l->gold = 50 + rand() % 41;
        l->items[n++] = item_make(&ITEM_TYPES[ITEM_INGOT_IRON], 1, 0);
        if (randf() < 0.35f)
            l->items[n++] = item_make(&ITEM_TYPES[ITEM_BONE], 2 + rand() % 3, 0);
        if (randf() < 0.25f)
            l->items[n++] = item_make(&ITEM_TYPES[ITEM_POTION_HP], 2, 0);
        break;
    default:  // BASİT / kilitsiz
        l->gold = 20 + rand() % 21;
        if (randf() < 0.6f)
            l->items[n++] = item_make(&ITEM_TYPES[ITEM_BONE], 1 + rand() % 2, 0);
        if (randf() < 0.4f)
            l->items[n++] = item_make(&ITEM_TYPES[ITEM_POTION_HP], 1, 0);
        break;
    }
    l->itemCount = n;
}

// Aktif haritanın zindan sandıklarını loots[]'a yerleştir (harita girişinde).
// Bugün açılmışsa: açık + boş durur (yarın yeniden kilitlenip dolar).
static void place_dungeon_chests(Loot *loots, MapId map, int today)
{
    for (int c = 0; c < CHEST_COUNT; c++)
    {
        if (CHESTS[c].map != map) continue;
        int slot = -1;
        for (int i = MAX_LOOT - 1; i >= 0; i--)   // sondan: mob kasalarıyla yarışmasın
            if (!loots[i].active) { slot = i; break; }
        if (slot < 0) return;

        Loot *l = &loots[slot];
        *l = (Loot){
            .rect = { .x = CHESTS[c].x - 24.0f, .y = CHESTS[c].y - 24.0f,
                      .w = 48.0f, .h = 48.0f },
            .active = true,
            .kind = LOOT_KIND_CHEST,
            .chestId = (Sint8)c,
        };
        if (chest_opened_day(c) == today)
        {
            l->state = LOOT_OPEN;                  // bugün açılmış: boş sergi
            l->currentFrame = 7;
            l->lockTier = -1;
            l->itemCount = 0;
            l->gold = 0;
        }
        else
        {
            l->state = LOOT_CLOSED;
            l->lockTier = CHESTS[c].lockTier;
            chest_fill(l, CHESTS[c].lockTier, today);
        }
    }
}

// Aktif haritanın spawn bölgelerini kur: dünya REGIONS kamplarından,
// zindan katları worldmap kamplarından. Dönen: bölge sayısı.
static int build_spawn_zones(SpawnZone *zones, MapId map)
{
    int n = 0;
    if (map == MAP_WORLD)
    {
        for (int r = 0; r < REGION_COUNT; r++)
            for (int c = 0; c < REGIONS[r].campCount && n < MAX_SPAWN_ZONES; c++)
            {
                const RegionCamp *camp = &REGIONS[r].camps[c];
                zones[n++] = (SpawnZone){
                    .x = camp->x, .y = camp->y, .radius = camp->radius,
                    .type = camp->type, .maxCount = camp->maxCount, .respawnTimer = 0.0f,
                };
            }
    }
    else
    {
        int count = 0;
        const RegionCamp *camps = map_camps(map, &count);
        for (int c = 0; c < count && n < MAX_SPAWN_ZONES; c++)
            zones[n++] = (SpawnZone){
                .x = camps[c].x, .y = camps[c].y, .radius = camps[c].radius,
                .type = camps[c].type, .maxCount = camps[c].maxCount, .respawnTimer = 0.0f,
            };
    }
    return n;
}

// Mob ölümü — saldırıdan da kanamadan da aynı yol: animasyon + loot + görev.
// (İki yerden çağrılır; kopya kod desenkron olmasın diye tek fonksiyon)
// Bölünen türlerde (slime) yavrular BURADA doğar — kızgın doğarlar (aggro).
// Ünvan kazanımı bildirimi (M3): float + ses + günlük (idx -1 = yok)
static void title_notify(int idx, Player *p, FloatText *floatTexts)
{
    if (idx < 0) return;
    journal_add(JE_TITLE, idx);
    audio_play(SFX_LEVELUP);
    char buf[64];
    snprintf(buf, sizeof(buf), "ÜNVAN: %s!", TITLES[idx].name);
    SDL_Color gold = { 255, 210, 60, 255 };
    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                         p->rect.x + p->rect.w / 2.0f,
                         p->rect.y - 40.0f, buf, gold);
}

// REVİR masrafı (Faz REVIR): taban 200g + günlük %25 BİLEŞİK; önemli NPC
// (dükkancı/görev taşıyan) ×1.5; tekrarlanan yatış ×yatışSayısı — seri
// dayakçılık ekonomik olarak imkânsızlaşır (Hakan onaylı model)
static int infirmary_cost(int i, int today)
{
    int days = today - npc_infirm_day(i);
    if (days < 0) days = 0;
    if (days > 40) days = 40;   // taşma sigortası (30. günde köy zaten toplar)
    float c = 200.0f;
    for (int d = 0; d < days; d++) c *= 1.25f;
    if (NPCS[i].isVendor || NPCS[i].questId != NULL) c *= 1.5f;
    int beats = npc_beat_count(i);
    if (beats > 1) c *= (float)beats;
    return (int)c;
}

// SUİKASTÇİ: oyuncu hedefin ARKASINDA mı? (mob bakış vektörü ile
// mob→oyuncu vektörünün zıtlığı — 120° arka yay)
static bool player_behind_mob(const Mob *m, float px2, float py2)
{
    float fx2 = 0.0f, fy2 = 0.0f;
    if (m->e.facing == 2) { fx2 = m->e.facingLeft ? -1.0f : 1.0f; }
    else if (m->e.facing == 1) fy2 = -1.0f;
    else fy2 = 1.0f;
    float vx = px2 - (m->e.rect.x + m->e.rect.w / 2.0f);
    float vy = py2 - (m->e.rect.y + m->e.rect.h / 2.0f);
    float vl = SDL_sqrtf(vx * vx + vy * vy);
    if (vl < 1.0f) return false;
    return (fx2 * vx + fy2 * vy) / vl < -0.5f;   // cos(120°)
}

// İsimden tarif indeksi (Sessiz El tarifleri görev/sınıfla öğrenilir)
// Tarif indeksi KİMLİKLE bulunur, görünen adla DEĞİL: dört tarif adı iki kez
// geçiyor (kampateşi/simya varyantları) ve ada göre arama ilk eşleşmeyi
// döndürüyordu. Bulunamayan id sessiz kalmaz.
static int recipe_index_by_id(const char *rid)
{
    if (rid == NULL) return -1;
    for (int r = 0; r < RECIPE_COUNT; r++)
        if (RECIPES[r].id != NULL && SDL_strcmp(RECIPES[r].id, rid) == 0) return r;
    SDL_Log("craft: '%s' tarif id'si TABLODA YOK", rid);
    return -1;
}

// --- VFX HİS CİLASI köprüsü: isabet/ölüm/blok efektleri + hit-stop ---
// (g_fx deseni — imza şişirmeden derin çağrı yerlerinden erişim)
static AnimFx *g_animFx = NULL;
static R2DTexture *g_hitWeaponTex = NULL, *g_hitMagicTex = NULL,
                  *g_splatSmallTex = NULL, *g_splatBigTex = NULL,
                  *g_sparkTex = NULL;
static float g_hitStop = 0.0f;   // kalan donma süresi (dünya dt'si kısılır)
static const Equipment *g_eqBridge = NULL;   // mob_die Nişan kontrolü için
// Yer izi tipi için aktif silah efekti köprüsü (strike_fx_mob statik, player'a
// erişimi yok). Her kare player.weaponFx'ten senkronlanır.
static WeaponFxType g_fxBridge = WEAPON_FX_NONE;

// AKTİF BÜYÜNÜN ELEMANI. strike_fx_mob "magic" gelince şimdiye kadar HER büyüyü
// ARCANE sayıyordu — ateş topu moba çarpınca pembe yıldız çıkıyordu. Büyünün
// kendi imzası olması istendiği için (Hakan: "her skill in kendi tarzına göre")
// cast anında kurulur. Kalıcı alanlar (kutsal alan, hortum) son cast'in elemanını
// taşır; imza yanlış olmaz çünkü o alanlar da kendi cast'inde kuruldu.
static int g_spellElem = SKFX_EL_ARCANE;

// Büyü → eleman. Tabloda olmayan büyü ARCANE kalır (büyü sonuçta büyüdür).
static int spell_elem_of(int ab)
{
    switch (ab) {
        case ABILITY_FIREBOLT: case ABILITY_FIRESURGE:
        case ABILITY_FLAME_ARMOR:                        return SKFX_EL_FIRE;
        case ABILITY_ICE_NOVA: case ABILITY_ICETACLE:
        case ABILITY_ICE_ARROW:                          return SKFX_EL_ICE;
        case ABILITY_LIGHTNING: case ABILITY_LIGHTNING_CLAW: return SKFX_EL_BOLT;
        case ABILITY_SPIKES:                             return SKFX_EL_EARTH;
        case ABILITY_SNAKEBITE: case ABILITY_NEEDLE_STING: return SKFX_EL_POISON;
        default:                                         return SKFX_EL_ARCANE;
    }
}

// ZEMİN PARÇASI İNİŞ KANCASI (motor seam'i). groundfx "parça yere indi" der,
// TOZU bu katman seçiyor — groundfx partikül sistemine bağlanmıyor.
// Toz miktarı sınıfa göre: kaya çarpar ve toz kaldırır, çim usulca konar.
static ParticleSystem *g_landFx = NULL;
static void groundfx_on_land(float x, float y, int cls, float size)
{
    if (g_landFx == NULL) return;
    if (cls == 0) return;                     // bitki: toz kaldırmaz
    int n = (cls == 1) ? 2 : 1;               // kaya 2, tane 1
    for (int i = 0; i < n; i++)
        fx_spawn(g_landFx, x + (randf() * 2.0f - 1.0f) * size * 0.4f,
                 y + (randf() * 2.0f - 1.0f) * 3.0f,
                 (randf() * 2.0f - 1.0f) * 14.0f, -8.0f - randf() * 8.0f,
                 0.45f + randf() * 0.35f, 5.0f + randf() * 5.0f,
                 118, 104, 88, WEAPON_FX_SMOKE);
}

// PARTİKÜL ÖLÜM KANCASI (motor seam'i). fx.c "öldü" der, kalıntıyı BU katman
// seçer — motor decal/ses/hasar modüllerine bağlanmıyor. Şimdilik yalnız
// kıvılcım iz bırakıyor; ileride buz→kırağı, kan→leke, asit→çukur aynı
// kancadan geçecek.
static void fx_on_particle_death(float x, float y, WeaponFxType type, float size)
{
    if (type == WEAPON_FX_SPARK) {
        // Her kıvılcım değil: yaklaşık altıda biri iz bırakıyor, yoksa zemin
        // kısa sürede lekeye boğuluyor (decal havuzu da dolar).
        static unsigned sc = 0;
        if ((sc++ % 6u) == 0u)
            decal_scorch(x, y, size * 0.8f + 2.0f);
    }
}
// ── HİT-STOP BEKLEME SÜRESİ (motor koruması). Donma isteği hangi sistemden
// gelirse gelsin (silah, büyü, boss, tuzak) arka arkaya UYGULANAMAZ.
// NEDEN: Ateş Aslanı bir goblin KAMPINA girince 300px yolunda 4-6 gobline
// değiyor ve her isabet ~0.03 sn donma istiyordu; üst üste binince oyun
// takılıyor gibi oluyordu. Slime'larda görünmemesinin sebebi slime'ın dağınık
// olması (tek isabet). Çağrı yerlerini tek tek düzeltmek yetmez — bir daha
// unutulur; sınır MOTORDA duruyor.
static float g_hitStopCd = 0.0f;
static void hitstop(float s)
{
    if (s <= 0.0f) return;
    // Aktif donma varken yalnız DAHA GÜÇLÜSÜ ezebilir (uzatma değil, ezme).
    if (g_hitStop > 0.0f) { if (s > g_hitStop) g_hitStop = s; return; }
    if (g_hitStopCd > 0.0f) return;          // yeni donma için henüz erken
    g_hitStop = s;
    g_hitStopCd = 0.22f;                     // iki donma arası en az bu kadar
}

// Oyuncu merkezi — darbe YÖNÜ için (vuran → vurulan). strike_fx_mob derin bir
// çağrı yerinden çalışıyor; imza şişirmemek için g_fx deseni kullanılıyor.
static float g_playerCx = 0.0f, g_playerCy = 0.0f;

// İsabet efekti: silah patlaması / büyü çarpması + küçük kan + hit-stop.
// stop 0 = donma yok (alan büyüleri/DoT kaynaklı spam engellenir)
static void strike_fx_mob(Mob *m, bool magic, float stop)
{
    if (g_animFx == NULL || m == NULL) return;
    float cx = m->e.rect.x + m->e.rect.w / 2.0f;
    float cy = m->e.rect.y + m->e.rect.h / 2.0f;
    // YER İZİ: damganın tipi zemine ne bıraktığını belirler. Ayak hizasına
    // (rect'in altına) bırakılıyor — merkeze bırakılsa havada duruyor gibi olur.
    {
        float fy = m->e.rect.y + m->e.rect.h - 3.0f;
        switch (g_fxBridge) {
        case WEAPON_FX_FLAME:  decal_scorch(cx, fy, 16.0f); break;
        case WEAPON_FX_ICE:    decal_frost (cx, fy, 15.0f); break;
        case WEAPON_FX_POISON:
        case WEAPON_FX_VENOM:  decal_poison(cx, fy, 14.0f); break;
        default: break;
        }
    }
    // Isabet patlamaları IŞIK YAYAR: duvarı aydınlatır, gölgeyi kaydırır.
    // Renkler paletten — büyü soğuk mavi-beyaz, çelik sıcak kıvılcım; ikisi de
    // kandil Kelvin eğrisine yakın kalıyor ki aynı sahnede iki dünya olmasın.
    if (magic)
        animfx_spawn_lit(g_animFx, g_hitMagicTex, 49, 7, 100.0f, 100.0f,
                         cx, cy, 70.0f, 0.75f,
                         0.62f, 0.78f, 1.00f, 120.0f, 0.55f);
    else
        animfx_spawn_lit(g_animFx, g_hitWeaponTex, 36, 6, 100.0f, 100.0f,
                         cx, cy, 70.0f, 0.75f,
                         1.00f, 0.86f, 0.55f, 85.0f, 0.40f);
    animfx_spawn(g_animFx, g_splatSmallTex, 10, 10, 32.0f, 32.0f,
                 cx + (float)(rand() % 13 - 6), cy + (float)(rand() % 13 - 6),
                 30.0f, 1.3f);
    // ── PROSEDÜREL ÇARPMA (Ctrl+P): yönlü koni + darbe yayı + elemana özgü
    // ikincil katman + elemana göre hit-stop/sarsıntı. Sprite patlaması yukarıda
    // zaten çizildi; prosedürel katman onun ÜSTÜNE biniyor çünkü ikisi farklı
    // şey anlatıyor (sprite temas, prosedürel katman darbenin YÖNÜ ve KİMLİĞİ).
    if (skillfx_procedural()) {
        int el = SKFX_EL_STEEL;
        if (magic) el = g_spellElem;   // büyünün kendi imzası (ARCANE varsayılan)
        switch (g_fxBridge) {
            case WEAPON_FX_FLAME:  el = SKFX_EL_FIRE;   break;
            case WEAPON_FX_ICE:    el = SKFX_EL_ICE;    break;
            case WEAPON_FX_POISON:
            case WEAPON_FX_VENOM:  el = SKFX_EL_POISON; break;
            default: break;
        }
        // freeze = çağıranın stop isteği. stop=0 "donma yok" demek (alan
        // büyüleri/DoT spam'i) — prosedürel katman bu sözleşmeye uyuyor.
        skillfx_impact(cx, cy, cx - g_playerCx, cy - g_playerCy, el,
                       (stop > 0.0f) ? 1.0f : 0.55f, stop > 0.0f);
    }
    // ── MOB TEPKİSİ: gövde sarsıntısı + elemanın izi. Prosedürel moda BAĞLI
    // DEĞİL — "vurdum ve etkisi var" hissi bir A/B deneyi değil, oyunun temel
    // geri bildirimi (Hakan: "mob sarsılmalı ve her skill bir etki bırakmalı").
    {
        MobMark mk = MOB_MARK_NONE;
        if (magic) {
            switch (g_spellElem) {
                case SKFX_EL_FIRE:   mk = MOB_MARK_FIRE;   break;
                case SKFX_EL_ICE:    mk = MOB_MARK_ICE;    break;
                case SKFX_EL_BOLT:   mk = MOB_MARK_BOLT;   break;
                case SKFX_EL_EARTH:  mk = MOB_MARK_EARTH;  break;
                case SKFX_EL_POISON: mk = MOB_MARK_POISON; break;
                default:             mk = MOB_MARK_ARCANE; break;
            }
        } else {
            switch (g_fxBridge) {            // silah damgası
                case WEAPON_FX_FLAME: mk = MOB_MARK_FIRE;   break;
                case WEAPON_FX_ICE:   mk = MOB_MARK_ICE;    break;
                case WEAPON_FX_POISON:
                case WEAPON_FX_VENOM: mk = MOB_MARK_POISON; break;
                default: break;              // çıplak çelik iz bırakmaz
            }
        }
        mob_hit_react(m, mk, (stop > 0.0f) ? 1.25f : 0.7f);
    }
    if (stop > 0.0f) hitstop(stop);
}


static void mob_die(Mob *target, Mob *mobs, int mobCount, int *nextMobId,
                    Loot *loots, QuestStatus *questStatus,
                    FloatText *floatTexts, Player *player)
{
    target->e.alive = false;
    target->dying = true;
    target->state = MOB_DYING;
    target->currentFrame = 0;
    target->animTimer = 0.0f;

    // VFX: ölüm kan sıçraması + hit-stop (öldürme tatmini)
    if (g_animFx != NULL)
    {
        animfx_spawn(g_animFx, g_splatBigTex, 10, 10, 64.0f, 64.0f,
                     target->e.rect.x + target->e.rect.w / 2.0f,
                     target->e.rect.y + target->e.rect.h / 2.0f,
                     24.0f, 1.6f);
        // Kalıcı kan izi: sıçrama animasyonu söner, leke kalır. Boyut mob
        // gövdesiyle ölçekleniyor — trol lekesi kurt lekesinden büyük olsun.
        decal_blood(target->e.rect.x + target->e.rect.w / 2.0f,
                    target->e.rect.y + target->e.rect.h - 3.0f,
                    10.0f + target->e.rect.w * 0.22f);
        hitstop(0.09f);
    }

    // Bölünme (Faz 7): leşin iki yanına yavrular fırlar
    if (target->type->splitCount > 0)
    {
        for (int k = 0; k < target->type->splitCount; k++)
        {
            float ox = (k % 2 == 0) ? -26.0f : 26.0f;
            float oy = (float)(k / 2) * 22.0f - 8.0f;
            int cid = mob_spawn(mobs, mobCount, nextMobId,
                                &MOB_TYPES[target->type->splitChild],
                                target->e.rect.x + ox, target->e.rect.y + oy,
                                target->zoneId);   // yuvaya sayılır (E2)
            Mob *child = mob_find_by_id(mobs, mobCount, cid);
            if (child != NULL)
            {
                child->aggro = true;         // parçalar kızgın doğar
                child->suspicion = 1.0f;
                child->alertFlash = 0.5f;
                if (map_current() == MAP_WORLD)
                    ecology_note_spawn(target->zoneId, 1);   // pop muhasebesi doğru kalır
            }
        }
    }

    // Loot yalnız taşıyan türlerde (slime zinciri sonu Küçük'te — kasa spam'i olmaz)
    int li = -1;
    if (target->type->lootTableCount > 0 || target->type->goldMax > 0)
        li = loot_spawn(loots, MAX_LOOT, target->e.rect.x, target->e.rect.y,
                        target->type->lootTable, target->type->lootTableCount,
                        target->type->goldMin, target->type->goldMax);

    // BOSS ölümü: haftalık diriliş kaydı + İLK ödül (boss'a göre ayrışır)
    if (target->type->isBoss)
    {
        journal_add_unique(JE_BOSS_KILL, 0);
        SDL_Color gold = { 255, 210, 60, 255 };
        if (target->type == &MOB_TYPES[MOB_BRINGER])
        {
            // ÖLÜM GETİREN (mezarlık): İLK ölümde Nişan garanti düşer
            boss2_set_last_kill_day(gametime_day());
            if (!boss2_first_kill_done() && li >= 0 &&
                loots[li].itemCount < MAX_LOOT_ITEMS)
            {
                loots[li].items[loots[li].itemCount++] =
                    item_make(&ITEM_TYPES[ITEM_DEATH_SIGIL], 1, 0);
                boss2_set_first_kill_done(true);
            }
            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                 player->rect.x + player->rect.w / 2.0f,
                                 player->rect.y - 50.0f,
                                 "Ölüm Getiren dağıldı!", gold);
        }
        else
        {
            // İSKELET LORDU (K2)
            boss_set_last_kill_day(gametime_day());
            if (!boss_first_kill_done() && li >= 0 &&
                loots[li].itemCount < MAX_LOOT_ITEMS)
            {
                loots[li].items[loots[li].itemCount++] =
                    item_make(&ITEM_TYPES[ITEM_SWORD_LORD], 1, 2);   // +2 hazır düşer
                boss_set_first_kill_done(true);
            }
            // Ateş Aslanı tomarı: türün TEK kaynağı Lord leşi (%35 — haftalık kural)
            if (li >= 0 && loots[li].itemCount < MAX_LOOT_ITEMS && randf() < 0.35f)
                loots[li].items[loots[li].itemCount++] =
                    item_make(&ITEM_TYPES[ITEM_TOME_FIRESURGE], 1, 0);
            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                 player->rect.x + player->rect.w / 2.0f,
                                 player->rect.y - 50.0f,
                                 "İskelet Lordu devrildi!", gold);
        }
    }

    // SESSİZ EL KONTRATI (P3): etkin kontratın hedefi düştüyse ilerlet
    if (player->contractActive >= 0 &&
        player->contractActive < ASSASSIN_CONTRACT_COUNT)
    {
        const AssassinContract *ac = &ASSASSIN_CONTRACTS[player->contractActive];
        if (target->type == &MOB_TYPES[ac->targetMob] &&
            player->contractProgress < ac->count)
        {
            player->contractProgress++;
            SDL_Color vio11 = { 200, 160, 255, 255 };
            char cbuf[FLOAT_TEXT_LEN];
            if (player->contractProgress >= ac->count)
                snprintf(cbuf, sizeof(cbuf), "Kontrat tamam — tahtaya dön");
            else
                snprintf(cbuf, sizeof(cbuf), "Kontrat: %d/%d",
                         player->contractProgress, ac->count);
            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                 player->rect.x + player->rect.w / 2.0f,
                                 player->rect.y - 34.0f, cbuf, vio11);
        }
    }

    // İLK İMZA (Suikastçi P2): hedef düştü — zincir son adıma geçer
    if (target->type == &MOB_TYPES[MOB_TAX_RUNNER] &&
        player->storyFlags[SF_ASSASSIN] == 11)
    {
        player->storyFlags[SF_ASSASSIN] = 12;
        SDL_Color vio8 = { 200, 160, 255, 255 };
        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                             player->rect.x + player->rect.w / 2.0f,
                             player->rect.y - 40.0f,
                             "İmza atıldı. Neriman'a dön.", vio8);
    }

    // ÖLÜM GETİREN'İN NİŞANI: kuşanılıysa her öldürme 8 HP geri verir
    if (g_eqBridge != NULL &&
        g_eqBridge->slots[EQUIP_NECKLACE].type == &ITEM_TYPES[ITEM_DEATH_SIGIL] &&
        player->health > 0.0f)
    {
        player->health += 8.0f;
        if (player->health > player->maxHealth) player->health = player->maxHealth;
        SDL_Color lifered = { 255, 90, 120, 255 };
        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                             player->rect.x + player->rect.w / 2.0f,
                             player->rect.y - 24.0f, "+8", lifered);
    }

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

    // EKOLOJİ (E1): popülasyon düşer; reis/totem öldüyse yuva çöker
    if (map_current() == MAP_WORLD)
    {
        bool isChief = (target->type == &MOB_TYPES[MOB_GOBLIN_CHIEF] ||
                        target->type == &MOB_TYPES[MOB_ORC_TOTEM]);
        if (ecology_on_mob_death(target->zoneId, isChief, gametime_day()))
        {
            // Yuva çöktü: aynı sahanın kalanları kaçışır (savaşı bırakır)
            int cs = ecology_site_of_zone(target->zoneId);
            for (int j = 0; j < mobCount; j++)
            {
                if (!mobs[j].e.alive || &mobs[j] == target) continue;
                if (mobs[j].type->isStructure) continue;
                if (ecology_site_of_zone(mobs[j].zoneId) != cs) continue;
                mobs[j].scatter = true;
                mobs[j].fleeTimer = 2.5f;
                mobs[j].aggro = false;
                mobs[j].alertFlash = 0.6f;
            }
            journal_add(JE_NEST_CLEANSE,
                        target->type == &MOB_TYPES[MOB_GOBLIN_CHIEF] ? 0 : 1);
            title_notify(title_stat_add(player, TS_NESTS, 1), player, floatTexts);
            SDL_Color amber = { 255, 200, 120, 255 };
            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                 target->e.rect.x + target->e.rect.w / 2.0f,
                                 target->e.rect.y - 40.0f,
                                 "Yuva çöktü — kamp dağılıyor!", amber);
        }
    }

    // Ünvan sayaçları (M3): aile + gece kesimi
    {
        int midx = (int)(target->type - MOB_TYPES);
        if (midx >= 6 && midx <= 11)
            title_notify(title_stat_add(player, TS_KILL_SLIME, 1), player, floatTexts);
        else if (midx == 0 || midx == MOB_GOBLIN_CHIEF)
            title_notify(title_stat_add(player, TS_KILL_GOBLIN, 1), player, floatTexts);
        else if (midx == 1)
            title_notify(title_stat_add(player, TS_KILL_ORC, 1), player, floatTexts);
        else if (midx >= 2 && midx <= 5)
        {
            title_notify(title_stat_add(player, TS_KILL_ANIMAL, 1), player, floatTexts);
            // Usta Avcı: av çift ürün şansı — ikinci kasa düşer
            if (li >= 0 && randf() < title_double_loot_chance(player))
                loot_spawn(loots, MAX_LOOT,
                           target->e.rect.x + 20.0f, target->e.rect.y,
                           target->type->lootTable, target->type->lootTableCount,
                           target->type->goldMin, target->type->goldMax);
        }
        if (gametime_is_night())
            title_notify(title_stat_add(player, TS_NIGHT_KILLS, 1), player, floatTexts);
    }

    // BASKIN (Dünya Olayı): köyü savunan itibar kazanır — herkes görür
    if (target->raider)
    {
        int vRegion = region_of(13.5f * TILE_SIZE, 7.0f * TILE_SIZE);   // köy meydanı
        rep_add(&player->reputation[vRegion], 2.0f);
        SDL_Color green = { 120, 255, 120, 255 };
        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                             target->e.rect.x + target->e.rect.w / 2.0f,
                             target->e.rect.y - 40.0f,
                             "Köyü savundun! (+itibar)", green);
    }

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
                             "Söz yerine geldi — haber vermeliyim.", green);
    }
}

// Ağaç etkileri köprüsü: main() yereli perkFx'e statik yardımcılardan erişim
// (gather_finish/craft yardımcıları imza şişirmeden düğüm etkilerini okur)
static const PerkEffects *g_fx = NULL;
static bool g_natureBounty = false;   // hava: dün yağdı → bugün ot toplaması ×2

// ÇANTA SEKMELERİ: Eşya (ana) + Malzeme (craft) çantası. Hammadde (material
// bayrağı) Malzeme sekmesine düşer; sayım/tüketim İKİ çantadan (craft öncelik).
// Statik köprü: gather/craft yardımcıları imza şişirmeden erişsin (g_fx deseni).
static Inventory *g_mainBag = NULL, *g_craftBag = NULL;

static bool bag_add(const ItemType *t, int count, int plusLevel)
{
    if (t == NULL) return false;
    Inventory *dst = (t->material && g_craftBag != NULL) ? g_craftBag : g_mainBag;
    return inventory_add(dst, t, count, plusLevel);
}

static bool bag_add_stackp(const ItemStack *st)
{
    if (st == NULL || st->type == NULL) return false;
    Inventory *dst = (st->type->material && g_craftBag != NULL) ? g_craftBag : g_mainBag;
    return inventory_add_stack(dst, st);
}

static int bag_count(const ItemType *t)
{
    return inventory_count(g_craftBag, t) + inventory_count(g_mainBag, t);
}

static bool bag_consume(const ItemType *t)
{
    return inventory_consume(g_craftBag, t) || inventory_consume(g_mainBag, t);
}

// N adet düş — iki çantanın TOPLAMI yeterse (craft önce). Yetersizse hiç düşmez.
static bool bag_consume_n(const ItemType *t, int n)
{
    if (bag_count(t) < n) return false;
    for (int k = 0; k < n; k++)
        if (!inventory_consume(g_craftBag, t))
            inventory_consume(g_mainBag, t);
    return true;
}

// Çanta kademeleri → aktif slot sayıları (bagUpgrades bitleri)
static void bags_apply_caps(const Player *p, Inventory *mainInv, Inventory *craftInv)
{
    mainInv->cap = INV_MAIN_BASE
                 + ((p->bagUpgrades & 1) ? INV_MAIN_STEP : 0)
                 + ((p->bagUpgrades & 2) ? INV_MAIN_STEP : 0);
    craftInv->cap = INV_CRAFT_BASE
                  + ((p->bagUpgrades & 4) ? INV_CRAFT_STEP : 0)
                  + ((p->bagUpgrades & 8) ? INV_CRAFT_STEP : 0);
}

// BLOK geri bildirimi (Parça B): blok türüne göre yazı + ses + Block XP.
// (sersemletme/yansıtma ÇAĞRI YERİNDE — saldırgan pointer'ı orada)
static void on_skill_levelup(Player *player, SkillType s,
                             Hotbar *hotbar, FloatText *floatTexts);
static void block_feedback(int blockKind, Player *p, Hotbar *hb,
                           FloatText *floatTexts, const PerkEffects *fx)
{
    if (blockKind <= 0) return;
    SDL_Color c = (blockKind == 3) ? (SDL_Color){ 255, 230, 120, 255 }
                                   : (SDL_Color){ 170, 200, 255, 255 };
    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                         p->rect.x + p->rect.w / 2.0f, p->rect.y - 24.0f,
                         (blockKind == 3) ? "PERFECT BLOK!"
                       : (blockKind == 2) ? "Siper!" : "Blok!", c);
    audio_play(SFX_REFLECT);
    float xp = ((blockKind == 3) ? 12.0f : (blockKind == 2) ? 6.0f : 8.0f)
             * (fx->blockXpMult > 0.0f ? fx->blockXpMult : 1.0f);
    if (skill_add_xp(p->skills, SKILL_BLOCK, xp))
        on_skill_levelup(p, SKILL_BLOCK, hb, floatTexts);
}

// Tomarla öğrenilen büyüler (Büyü dalgası 2): ağaçtan bağımsız kalıcı açılış.
// tree_compute_effects perkFx'i SIFIRDAN kurar — her çağrının ardından bu da
// çağrılmalı (yoksa hotbar_purge_closed öğrenilmiş büyüyü söker).
static void spells_apply_learned(const Player *p, PerkEffects *fx)
{
    for (int a = 0; a < ABILITY_COUNT; a++)
        if (p->spellsLearned & (1ull << a))
            fx->abilityOpen[a] = true;
}

// YANKICI (gizli sınıf): ritüel tamamsa beceriler açık — ağaç/tomar dışı yol.
// tree_compute_effects sonrası spells_apply_learned ile birlikte çağrılmalı.
static void echo_apply_unlocks(const Player *p, PerkEffects *fx)
{
    if (p->storyFlags[SF_YANKICI] == 2)
    {
        fx->abilityOpen[ABILITY_WHISPER_BURST] = true;
        fx->abilityOpen[ABILITY_SHADOW_FORM] = true;
        fx->abilityOpen[ABILITY_LIFE_DRAIN] = true;    // P2
        fx->abilityOpen[ABILITY_ECHO_CALL] = true;
    }
    // SUİKASTÇİ (gizli sınıf 2): Sessiz El zinciri bitince 5'i birden
    if (p->storyFlags[SF_ASSASSIN] >= 13)
    {
        fx->abilityOpen[ABILITY_BACKSTAB] = true;
        fx->abilityOpen[ABILITY_SHADOWSTEP] = true;
        fx->abilityOpen[ABILITY_SMOKE_BOMB] = true;
        fx->abilityOpen[ABILITY_NEEDLE_STING] = true;
        fx->abilityOpen[ABILITY_HAMSTRING] = true;
    }
}

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
        snprintf(buf, sizeof(buf), "Geçildi: %s", ITEM_TYPES[found].name);
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
static void gather_finish(int res, Player *player,
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

    // Doğa beslenir (hava): dün yağmur yağdıysa bugün otlar ×2 verir
    if (g_natureBounty && rd->skill == SKILL_HERBALISM) count *= 2;

    // Kar: doğa uykuda — ot toplaması yarıya iner (en az 1)
    if (weather_is_snowy() && rd->skill == SKILL_HERBALISM)
    {
        count /= 2;
        if (count < 1) count = 1;
    }

    if (bag_add(&ITEM_TYPES[rd->itemIdx], count, 0))
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
static void craft_finish_recipe(int ri, Player *player,
                                Hotbar *hotbar, FloatText *floatTexts)
{
    const RecipeDef *r = &RECIPES[ri];
    SDL_Color red = { 255, 120, 100, 255 };

    // ÇANTA EŞYASI (kademe): zaten takılıysa üretme — malzeme yanmasın
    int bagBit = ITEM_TYPES[r->outItem].bagUpgrade;
    if (bagBit > 0 && (player->bagUpgrades & (1u << (bagBit - 1))))
    {
        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                             player->rect.x + player->rect.w / 2.0f,
                             player->rect.y - 10.0f, "Zaten takılı.", red);
        return;
    }

    if (!bag_consume_n(&ITEM_TYPES[r->in1Item], r->in1Count))
    {
        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                             player->rect.x + player->rect.w / 2.0f,
                             player->rect.y - 10.0f, "Malzeme eksik!", red);
        return;
    }
    if (r->in2Item >= 0 &&
        !bag_consume_n(&ITEM_TYPES[r->in2Item], r->in2Count))
    {
        // in2 eksikse in1'i GERİ koy — yarım tüketim malzeme yakmasın
        bag_add(&ITEM_TYPES[r->in1Item], r->in1Count, 0);
        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                             player->rect.x + player->rect.w / 2.0f,
                             player->rect.y - 10.0f, "Malzeme eksik!", red);
        return;
    }

    // ÇANTA EŞYASI: envantere girmez — kademe OTOMATİK takılır, slotlar açılır
    if (bagBit > 0)
    {
        player->bagUpgrades |= (Uint8)(1u << (bagBit - 1));
        bags_apply_caps(player, g_mainBag, g_craftBag);
        audio_play(SFX_PERK);   // kalıcı kazanım — törensel
        char buf[FLOAT_TEXT_LEN];
        snprintf(buf, sizeof(buf), "%s takıldı — +%d slot!",
                 ITEM_TYPES[r->outItem].name, (bagBit <= 2) ? 10 : 15);
        SDL_Color vio = { 200, 160, 255, 255 };
        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                             player->rect.x + player->rect.w / 2.0f,
                             player->rect.y - 24.0f, buf, vio);
        SkillType bsk = station_skill(r->station);
        if (skill_add_xp(player->skills, bsk, r->xp))
            on_skill_levelup(player, bsk, hotbar, floatTexts);
        return;
    }
    // Aşçı sayacı (M3): yemek pişirildi
    if (r->outItem == ITEM_MEAT_COOKED || r->outItem == ITEM_STEW ||
        r->outItem == ITEM_SOUP || r->outItem == ITEM_KAVURMA)
        title_notify(title_stat_add(player, TS_COOKED, 1), player, floatTexts);

    if (!bag_add(&ITEM_TYPES[r->outItem], r->outCount, 0))
    {
        // Çanta dolu: tüketilen malzemeyi geri koy (üretim gerçekleşmedi)
        bag_add(&ITEM_TYPES[r->in1Item], r->in1Count, 0);
        if (r->in2Item >= 0)
            bag_add(&ITEM_TYPES[r->in2Item], r->in2Count, 0);
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

// DÜZENLEYİCİ ÖN İZLEMESİ: seçili panel, kapalı olsa bile GERÇEK içeriğiyle
// çizilir. Tutamağa bakarak panel dizmek körlemedir — genişliği, satır sayısı
// ve metni görünmeden konumun doğru olduğu anlaşılamaz.
// Düzenleyici kapalıyken NULL döner, yani hiçbir çizim koşulu değişmez.
static bool ed_prev(const char *name)
{
    const char *p = uieditor_preview();
    return p != NULL && SDL_strcmp(p, name) == 0;
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;   // SDL_main bu imzayı şart koşar, argüman kullanmıyoruz
    // RNG tohumu: randf() rand() tabanlı — tohumsuz her açılış AYNI zar
    // dizisini üretiyordu (hava/olay şafak zarları oturumlar arası donuktu)
    srand((unsigned)(SDL_GetPerformanceCounter() & 0xFFFFFFFFu));
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
    // DEV kancası PIXE_HIDDEN=1: pencere GİZLİ açılır. Duman testi (aç, birkaç
    // saniye koş, kapat) ekranı kapatmasın diye. GPU yolu normal çalışmaya
    // devam eder — yalnız pencere görünmez, yani test ettiği şey aynı kalır.
    // (SDL_VIDEODRIVER=dummy yerine bu: dummy sürücüde SDL_GPU cihazı açılmaz.)
    Uint64 winFlags = 0;
    if (SDL_getenv("PIXE_HIDDEN") != NULL) winFlags |= SDL_WINDOW_HIDDEN;
    SDL_Window *window = SDL_CreateWindow("Pixe Game", 1600, 900, winFlags);
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

    // Post-fx ön ayarları — F7 her basışta sıradakine geçer (0 = kapalı).
    // Alan sırası: doygunluk, sıcaklık, kontrast, parlaklık, vinyet, v.yarıçap,
    //              scanline, gameboy, posterize, kromatik sapma, gren,
    //              BLOOM gücü, bloom eşiği, bloom yarıçapı, ISI genliği, ısı hızı.
    // Isı bükülmesi bir TARZ değil DURUM efekti — çoğu preset'te 0. Asıl kullanımı
    // volkan/gayzer yakınında dinamik olarak bindirmek; "VOLKAN" preset'i test için.
    // Bloom eşiği (2026-08-22): eskiden 0.72+ tutuluyordu çünkü post-fx tüm
    // kareyi (HUD dahil) filtreliyordu ve beyaz HUD yazısı taşıyordu. Sahne/HUD
    // ayrımı (r2d_postfx_flush, 2026-07-30) o gerekçeyi kaldırdı; eşikler toptan
    // -0.10 indirildi ki kandil göbeği ve büyüler GERÇEKTEN parlasın. Kar örtüsü
    // ve şimşek gibi sahne beyazları da artık hafifçe taşar — istenen davranış.
    // Tarz başına ince ayar Hakan'ın F7 turunda.
    // Sıralama: önce renk grade'leri, sonra film, en sonda retro/ekran efektleri.
    static const struct { const char *name; R2DPostFX fx; } POSTFX_PRESETS[] = {
        { "POST-FX KAPALI", { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },   // fx kullanılmaz
        // Hafif sıcak dokunuş — pixel-art'ı boğmayan canlılık (varsayılan)
        { "SICAK",        { 1.12f,  0.02f,  1.05f,  0.00f, 0.32f, 0.40f, 0, 0, 0, 0,       0, 0.30f, 0.64f, 0.0035f, 0, 0 } },

        // ── İMZA ADAYLARI (GDD §6.1 kalıcı görsel imza kararı, 2026-08-22) ──
        // Dördü de SICAK↔DARK FANTASY ekseninde, kanona bağlı birer iddia.
        // Test: her adayla gündüz/alacakaranlık/gece + zindan gezilir (F8 hava,
        // gün döngüsü); kandil/kamp ateşi başında bloom karakterine bakılır.
        // Karar verilince kazanan varsayılan olur, kalanlar listeden düşer.
        //
        // OCAK — "ışık = güvenlik" (LORE §1.5). Taban hafif sıcak ama sakin;
        // doygunluk nötr, vinyet orta. İddia: dünya kendini öne atmaz, ateş ve
        // kandil cömert bloom'la sahnenin yıldızı olur.
        { "IMZA: OCAK",       { 1.00f,  0.030f, 1.10f,  0.00f, 0.40f, 0.36f, 0, 0, 0, 0,   0,      0.42f, 0.58f, 0.0042f, 0, 0 } },
        // SESSIZ GUN — kanonun yası: kül tonu, çekilmiş doygunluk, çok ince
        // gren (havada kül). İddia: melankoli kalıcı ton olmalı; renk patlaması
        // yalnız büyüye ve ateşe ait.
        { "IMZA: SESSIZ GUN", { 0.88f, -0.015f, 1.12f, -0.01f, 0.44f, 0.34f, 0, 0, 0, 0,   0.025f, 0.36f, 0.60f, 0.0040f, 0, 0 } },
        // MASAL — Cute Fantasy sanatının kendi sesi: canlı, aydınlık, geniş
        // yumuşak ışıma, hafif vinyet. İddia: karanlığı kanon anlatsın, ekran
        // değil; tezat (şirin dünya + sert hikaye) bilinçli imza olur.
        { "IMZA: MASAL",      { 1.18f,  0.015f, 1.03f,  0.02f, 0.22f, 0.48f, 0, 0, 0, 0,   0,      0.34f, 0.62f, 0.0048f, 0, 0 } },
        // DEMIRBOYUN — kanon kilidinin adı: demir grisi, soğuk, sert kontrast,
        // ağır vinyet; bloom güçlü ki soğuk tabanda sıcak ışıklar bıçak gibi
        // ayrışsın. İddia: dünya baskıcı, her ışık kaynağı bir vaat.
        { "IMZA: DEMIRBOYUN", { 0.82f, -0.045f, 1.16f, -0.02f, 0.48f, 0.32f, 0, 0, 0, 0,   0.02f,  0.50f, 0.56f, 0.0045f, 0, 0 } },
        // ── imza adayları sonu ──

        // Diablo/Souls havası — soluk soğuk renkler, sert kontrast, ağır vinyet
        { "DARK FANTASY", { 0.78f, -0.04f,  1.18f, -0.03f, 0.50f, 0.30f, 0, 0, 0, 0,       0, 0.45f, 0.60f, 0.0045f, 0, 0 } },
        // Kasvetli kuzey — mavi ton, hafif soluk (Banished/Frostpunk havası)
        { "SOGUK",        { 0.95f, -0.035f, 1.08f, -0.01f, 0.38f, 0.38f, 0, 0, 0, 0,       0, 0.28f, 0.65f, 0.0035f, 0, 0 } },
        // Soluk masal kitabı — düşük doygunluk, yumuşak kontrast, aydınlık
        { "PASTEL",       { 0.82f,  0.01f,  0.92f,  0.04f, 0.18f, 0.50f, 0, 0, 0, 0,       0, 0.40f, 0.58f, 0.0055f, 0, 0 } },
        // Arcade — yüksek doygunluk + kontrast, renkler bağırsın
        { "CANLI",        { 1.35f,  0.00f,  1.15f,  0.00f, 0.20f, 0.45f, 0, 0, 0, 0,       0, 0.35f, 0.66f, 0.0032f, 0, 0 } },
        // Eski fotoğraf — griye çök + sıcak kahve tonu
        { "SEPYA",        { 0.05f,  0.06f,  1.05f,  0.02f, 0.35f, 0.40f, 0, 0, 0, 0,       0, 0.22f, 0.66f, 0.0038f, 0, 0 } },
        // Siyah-beyaz film — sert kontrast, koyu kenarlar + ince gren
        { "NOIR",         { 0.00f,  0.00f,  1.25f, -0.02f, 0.55f, 0.30f, 0, 0, 0, 0,       0.05f, 0.30f, 0.68f, 0.0040f, 0, 0 } },
        // Korku — renkler çekilmiş, karanlık, kumlanma + hafif renk kayması
        { "KORKU",        { 0.55f, -0.02f,  1.20f, -0.06f, 0.60f, 0.25f, 0, 0, 0, 0.0015f, 0.10f, 0.20f, 0.70f, 0.0050f, 0.0012f, 0.7f } },
        // Eski tüplü ekran — tarama çizgileri + canlı fosfor renkleri
        { "CRT",          { 1.15f,  0.00f,  1.08f,  0.02f, 0.35f, 0.45f, 0.35f, 0, 0, 0,   0, 0.42f, 0.62f, 0.0040f, 0, 0 } },
        // VHS kaset — soluk, tarama + kromatik sapma + kumlanma
        { "VHS",          { 0.85f,  0.01f,  0.95f,  0.03f, 0.30f, 0.45f, 0.20f, 0, 0, 0.0025f, 0.06f, 0.38f, 0.60f, 0.0050f, 0.0008f, 2.2f } },
        // 8-bit konsol — kanal başına 6 renk kademesi, canlı
        { "8-BIT",        { 1.25f,  0.00f,  1.10f,  0.00f, 0.15f, 0.50f, 0, 0, 6, 0,       0, 0.18f, 0.70f, 0.0028f, 0, 0 } },
        // Orijinal GameBoy — 4 kademeli DMG yeşili
        { "GAMEBOY",      { 1.00f,  0.00f,  1.05f,  0.00f, 0.15f, 0.50f, 0, 1, 0, 0,       0, 0, 0, 0, 0, 0 } },

        // --- Ek tarzlar + kombinasyonlar (mevcutları ezmeden döngü sonuna) ---
        // Gün batımı — sıcak turuncu, canlı, yumuşak vinyet (kırsal akşam)
        { "GUNBATIMI",    { 1.20f,  0.06f,  1.08f,  0.02f, 0.30f, 0.40f, 0, 0, 0, 0,       0, 0.48f, 0.58f, 0.0048f, 0, 0 } },
        // Ay ışığı — soğuk mavi, karanlık, sakin gece + çok ince gren
        { "AY ISIGI",     { 0.80f, -0.05f,  1.05f, -0.08f, 0.45f, 0.35f, 0, 0, 0, 0,       0.03f, 0.40f, 0.56f, 0.0055f, 0, 0 } },
        // Rüya — pastel + hafif renk kayması + ince gren (yumuşak, sisli)
        { "RUYA",         { 0.75f,  0.02f,  0.90f,  0.05f, 0.25f, 0.55f, 0, 0, 0, 0.0018f, 0.04f, 0.55f, 0.52f, 0.0070f, 0.0020f, 0.6f } },
        // Çorak toprak — soluk sıcak, ağır vinyet + gren (Fallout/Mad Max)
        { "CORAK",        { 0.60f,  0.05f,  1.15f, -0.02f, 0.55f, 0.30f, 0, 0, 0, 0,       0.07f, 0.25f, 0.68f, 0.0040f, 0.0015f, 1.4f } },
        // Retro arcade — CRT tarama + posterize + canlı (dolap oyunu; kombinasyon)
        { "RETRO ARCADE", { 1.30f,  0.00f,  1.12f,  0.02f, 0.25f, 0.45f, 0.30f, 0, 8, 0,   0, 0.35f, 0.64f, 0.0035f, 0, 0 } },
        // Eski film makarası — sepya + tarama + gren + ağır vinyet (kombinasyon)
        { "ESKI FILM",    { 0.10f,  0.06f,  1.10f,  0.00f, 0.55f, 0.30f, 0.15f, 0, 0, 0,   0.09f, 0.28f, 0.64f, 0.0045f, 0, 0 } },
        // Sentetik dalga — yüksek doygunluk + kromatik sapma (neon/synthwave)
        { "SYNTHWAVE",    { 1.40f,  0.02f,  1.15f,  0.02f, 0.35f, 0.40f, 0, 0, 0, 0.0022f, 0.03f, 0.60f, 0.50f, 0.0060f, 0, 0 } },
        // Kâbus — korku uçlara: renksiz, çok karanlık, ağır kumlanma + sapma (kombinasyon)
        // Volkan/gayzer: ısı bükülmesinin vitrini — sıcak grade + güçlü bükülme
        // + cömert bloom (lav zaten eşiğin üstünde).
        { "VOLKAN",       { 1.10f,  0.09f,  1.10f,  0.01f, 0.40f, 0.35f, 0, 0, 0, 0.0010f, 0.03f, 0.50f, 0.56f, 0.0050f, 0.0038f, 1.8f } },
        { "KABUS",        { 0.30f, -0.02f,  1.30f, -0.10f, 0.65f, 0.20f, 0, 0, 0, 0.0030f, 0.14f, 0.22f, 0.72f, 0.0060f, 0.0026f, 0.9f } },
    };
    const int POSTFX_COUNT = (int)(sizeof(POSTFX_PRESETS) / sizeof(POSTFX_PRESETS[0]));
    int postfxIdx = 1;   // SICAK ile başla
    r2d_postfx_set(renderer, &POSTFX_PRESETS[postfxIdx].fx);

    R2DTexture *crateTex = load_texture(renderer, "assets/decor/erw/crate.png");
    R2DTexture *chestTex = load_texture(renderer, "assets/decor/erw/chest_anim.png");
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
    Inventory inventory = {0}; // Eşya çantası (main'de, oyuncuyla birlikte)
    Inventory craftBag = {0};  // Malzeme çantası (hammadde otomatik buraya düşer)
    int invTab = 0;            // aktif sekme: 0 Eşya / 1 Malzeme (I döngüsü)
    int craftScroll = 0;       // craft paneli kaydırma (tekerlek; satır cinsinden)
    int trashTab = -1, trashSlot = -1;   // çöp onayı bekleyen item (-1 = yok)
    int dragHbSlot = -1;                 // hotbar'dan sürüklenen hücre (-1 = yok)
    int dragAbility = -1;                // K panelinden sürüklenen beceri (-1 = yok)
    // NPC başına BUGÜNKÜ yankesicilik denemesi (-15%/deneme).
    // DİKKAT: NPC_COUNT'a paralel — tavan npc.c NPC_STATE_CAP (24) ile aynı
    // tutulur. 12'de kalmıştı: Taşdurak NPC'sine (idx 16+) yankesicilik
    // STACK TAŞIYORDU (çıkışta SDL_QuitAudio SIGSEGV — Hakan crash raporu)
    #define PICK_TRIES_CAP 24
    int pickTries[PICK_TRIES_CAP] = {0};
    bool pickFailUsed = false; // Masum Yüz: günde bir affedilen başarısızlık kullanıldı mı
    int dragTab = 0;           // sürükleme HANGİ sekmeden başladı (I ile değişse de doğru)
    g_mainBag = &inventory;    // bag_* yardımcıları (yönlendirme + iki-çanta sayım)
    g_craftBag = &craftBag;
    inventory.cap = INV_MAIN_BASE;    // taban kapasiteler (player henüz yok —
    craftBag.cap = INV_CRAFT_BASE;    // kademeler yükleme/craft'ta işlenir)

    Equipment equipment = {0}; // tüm slotlar boş başlar
    g_eqBridge = &equipment;   // mob_die Nişan kill-heal kontrolü

    ItemTextures itemTex;
    item_textures_load(&itemTex, renderer);

    // base → feet (ayakkabı) → legs (pantolon) → chest (gömlek) → hands (eller) → hair (saç)
    // Katmanlar ÇİZİM SIRASINDA (alttan üste). Yeni parça eklemek = buraya bir satır.
    // base katmanlar: body → head → face. { texture, z } — z = LPC dosya öneki (çizim sırası, küçük=arka).
    PlayerLayer layers[] = {
        {load_texture(renderer, "assets/characters/body/male/light.png"), 10, -1, false, false, false},
        {load_texture(renderer, "assets/characters/head/male/human_light.png"), 100, -1, false, false, false},
        {load_texture(renderer, "assets/characters/face/male/neutral_light.png"), 101, -1, false, false, false},
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
        .contractActive = -1,   // Sessiz El: 0 geçerli kontrat — boş = -1
        // weaponFx her frame kusanili silahtan senkronlanir (bkz. player_update oncesi)
    };
    skills_init(player.skills);
    player.pendingAbility = -1;   // -1 = normal vuruş (0 geçerli ability olurdu)
    player.carriedTitle = -1;     // ünvan taşınmıyor (M3)

    // Skill ağaçları (Faz 21): hiç düğüm alınmamış başlar; etkiler nötr hesaplanır
    for (int s = 0; s < SKILL_COUNT; s++)
        player.treeTaken[s] = 0;
    PerkEffects perkFx;
    tree_compute_effects(player.treeTaken, player.fateTaken, &perkFx);
    spells_apply_learned(&player, &perkFx);
    echo_apply_unlocks(&player, &perkFx);
    g_fx = &perkFx;                        // statik yardımcı köprüsü
    skill_set_xp_mult(perkFx.xpGainMult);  // Kader XP çarpanı
    int treeOpenSkill = -1;   // açık ağaç paneli (K panelinden satır tıklanır)
    int treeSelNode = -1;     // seçili düğüm (ikinci tık = AL onayı)

    // Hotbar: açık ability'ler otomatik yerleşir (Power Strike lv1'de açık)
    Hotbar hotbar = {0};
    hotbar_place_unlocks(&hotbar, perkFx.abilityOpen);

    // Yeni oyun şablonu (Faz B1): "Yeni Oyun" bu bakir kopyadan başlar —
    // init bloğuyla asla desenkron olmaz (tek kaynak yukarıdaki init)
    const Player playerTemplate = player;

    // Dekorlar (ev/ağaç/çalı/saman): texture + boyutları (render/collision tek kaynaktan)
    R2DTexture *decor_textures[DECOR_TEX_CAP] = {0};
    for (int i = 0; i < DECOR_COUNT && i < DECOR_TEX_CAP; i++)
        decor_textures[i] = load_texture(renderer, DECORS[i].texPath);
    decor_set_texture_sizes(decor_textures);
    shadow_init(renderer);   // gölge lekesi dokusu (radyal alpha gradyanı)
    decal_init(renderer);    // yer izi dokusu (düzensiz kenarlı leke)
    // UI yerleşimi: yüklenemezse oyun çalışmaya devam eder, konumlar koddaki
    // varsayılanlara düşer (yerleşim verisi kolaylık, zorunluluk değil).
    uilayout_load(UI_LAYOUT_PATH);   // düzenleyici de aynı sabite kaydediyor
    r2d_light_set_model(1.0f, 0.0f, 0.0f, 41.0f);   // varsayılan: LİNEER toplama
    // Şekil artık ışık TİPİNE bağlı (light.c FLICKER tablosu). Global değerler
    // yalnız test çarpanı: 1.0 / 0 = tipin kendi şeklini kullan.
    r2d_light_set_shape(1.0f, 0.0f);
    // Işık eğrisi sözleşme testi (açılışta bir kez, ~1ms): CPU light_atten()
    // ile fs_light aynı matematik mi? Ayrışma sessiz kalmasın (GDD §6.2 Karar 1).
    if (light_curve_selftest("shaders/light.frag.hlsl") > 0)
        SDL_Log("UYARI: ışık eğrisi CPU/GPU sözleşmesi bozuk — yukarıdaki loglara bak");

    // Kaynak düğümleri (cevher/bitki/ağaç) — toplama sistemi
    R2DTexture *resource_textures[MAX_RESOURCES] = {0};
    for (int i = 0; i < RESOURCE_COUNT && i < MAX_RESOURCES; i++)
        resource_textures[i] = load_texture(renderer, RESOURCES[i].texPath);
    resource_reset();

    // Craft istasyonları (ocak/kazan)
    R2DTexture *station_textures[8] = {0};
    for (int i = 0; i < STATION_COUNT && i < 8; i++)
        station_textures[i] = load_texture(renderer, STATIONS[i].texPath);

    // EŞİKLİ ALEV (Faz ALEV, brief ADIM 3): kamp ateşi + demirci ocağı gövde
    // alevi. Gürültü + rampa fxtex'ten (açılışta bir kez), shader SPRITE-FX
    // yolundan. Rampa değerleri fx.c'deki DOĞRULANMIŞ kara cisim tablosundan
    // (ELLE DEĞER GİRMEK YASAK — GDD §6.6 Karar 10b): 1000K → 2700K.
    // Genişlik = durak sayısı + NEAREST → rampanın kendisi pixel-art merdiveni.
    R2DTexture *fxNoiseTex = fxtex_noise(renderer, 128, 20260822u);
    static const float FIRE_RGB[7][3] = {
        { 128/255.0f,  34/255.0f, 0.0f },   // ~1000K — dil ucu, sönme kızılı
        { 173/255.0f,  56/255.0f, 0.0f },   // 1150K
        { 189/255.0f,  72/255.0f, 0.0f },   // 1350K
        { 255/255.0f, 108/255.0f, 0.0f },   // 1500K
        { 255/255.0f, 132/255.0f, 0.0f },   // 1900K
        { 255/255.0f, 155/255.0f,  61/255.0f },   // 2400K
        { 255/255.0f, 167/255.0f,  87/255.0f },   // 2700K — çekirdek (B<90: ADD sarıya doyar, beyaza değil)
    };
    static const float FIRE_STOP[7] = { 0.0f, 0.14f, 0.30f, 0.48f, 0.68f, 0.86f, 1.0f };
    R2DTexture *fireRampTex = fxtex_ramp(renderer, FIRE_RGB, FIRE_STOP, 7, 7, 0.0f);
    int fxFlame = r2d_fx_register(renderer, "flame.frag", 1);
    if (fxFlame == 0 || fxNoiseTex == NULL || fireRampTex == NULL)
        SDL_Log("UYARI: esikli alev kurulamadi — istasyon alevleri partikul-only");

    // İlan panosu (Faz 6): köy meydanı, günlük kontratlar
    #define BOARD_X (13.5f * TILE_SIZE)
    #define BOARD_Y (7.0f * TILE_SIZE)
    #define BOARD_RANGE 90.0f
    R2DTexture *boardTex = load_texture(renderer, "assets/decor/erw/notice_board.png");
    R2DTexture *bookTex  = load_texture(renderer, "assets/ui/Book_UI.png");   // Yolculuk günlüğü
    R2DTexture *nestsTex = load_texture(renderer, "assets/decor/nests.png");   // yuva görselleri (E1)
    R2DTexture *abilityIconTex = load_texture(renderer, "assets/icons/abilities.png");  // beceri ikonları (Cila)
    R2DTexture *lanternTex = load_texture(renderer, "assets/decor/erw/lantern.png");  // K9 kandili (D1a)
    R2DTexture *sofraTex = load_texture(renderer, "assets/decor/sofra.png");  // K2 sofrası (D1b)
    R2DTexture *snakeTex  = load_texture(renderer, "assets/characters/allies/snake.png");   // M4
    R2DTexture *turtleTex = load_texture(renderer, "assets/characters/allies/turtle.png");
    bool boardOpen = false;

    // Büyü VFX sprite'ları (Faz 22): mermi küreleri + arcane patlama + alev
    R2DTexture *orbFireTex  = load_texture(renderer, "assets/fx/orb_fire.png");
    R2DTexture *orbBlueTex  = load_texture(renderer, "assets/fx/orb_blue.png");
    R2DTexture *arcaneTex   = load_texture(renderer, "assets/fx/arcane_burst.png");
    R2DTexture *flameTex    = load_texture(renderer, "assets/fx/flame_small.png");
    R2DTexture *shieldTex   = load_texture(renderer, "assets/fx/iceshield.png");
    R2DTexture *spikesTex   = load_texture(renderer, "assets/fx/spikes.png");
    R2DTexture *clawTex     = load_texture(renderer, "assets/fx/lightningclaw.png");  // 4×4 128px
    R2DTexture *icetacleTex = load_texture(renderer, "assets/fx/icetacle.png");
    R2DTexture *tornadoTex  = load_texture(renderer, "assets/fx/tornado.png");
    R2DTexture *darkburstTex = load_texture(renderer, "assets/fx/dark_burst.png");
    // VFX his cilası sheet'leri (isabet/kan/kıvılcım/cast halkası/slash yayı)
    R2DTexture *hitWeaponTex = load_texture(renderer, "assets/fx/hit_weapon.png");   // 6×6 @100
    R2DTexture *hitMagicTex  = load_texture(renderer, "assets/fx/hit_magic.png");    // 7×7 @100
    R2DTexture *splatSmallTex = load_texture(renderer, "assets/fx/splat_small.png"); // 10 @32
    R2DTexture *splatBigTex   = load_texture(renderer, "assets/fx/splat_big.png");   // 10 @64
    R2DTexture *sparkBlockTex = load_texture(renderer, "assets/fx/spark_block.png"); // 7 @48
    R2DTexture *castCircleTex = load_texture(renderer, "assets/fx/cast_circle.png"); // 9×9 @100
    R2DTexture *holyBurstTex  = load_texture(renderer, "assets/fx/holy_burst.png");   // 16 @48
    R2DTexture *holyImpactTex = load_texture(renderer, "assets/fx/holy_impact.png");  // 7 @32
    R2DTexture *echoWispTex   = load_texture(renderer, "assets/fx/echo_wisp.png");     // 10×2 @40×32
    R2DTexture *slashArcTex[4] = {   // facing sırası: UP, LEFT, DOWN, RIGHT
        load_texture(renderer, "assets/fx/slash_up.png"),
        load_texture(renderer, "assets/fx/slash_left.png"),
        load_texture(renderer, "assets/fx/slash_down.png"),
        load_texture(renderer, "assets/fx/slash_right.png"),
    };
    R2DTexture *snakebiteTex[3] = {   // 0 down / 1 up / 2 side (sola bakar)
        load_texture(renderer, "assets/fx/snakebite_down.png"),
        load_texture(renderer, "assets/fx/snakebite_up.png"),
        load_texture(renderer, "assets/fx/snakebite_side.png"),
    };
    // NOT: dosya adları firelion_* KALIYOR. Bunlar skill "Ateş Aslanı" iken
    // gelen ELLE ÇİZİLMİŞ aslan sheet'leri ve sprite A/B yolunda (Ctrl+P kapalı)
    // hâlâ çiziliyorlar. Skill 2026-08-05'te ALEV SELİ olarak yeniden
    // adlandırıldı (aslan silueti prosedürel olarak amatör duruyordu); asset
    // dosyalarını yeniden adlandırmak sanatı değiştirmiyor, sadece geçmişi
    // gizler. Prosedürel yol artık hat üstünde akan sarı sel çiziyor.
    R2DTexture *firesurgeTex[4] = {    // 0 down / 1 up / 2 left / 3 right
        load_texture(renderer, "assets/fx/firelion_down.png"),
        load_texture(renderer, "assets/fx/firelion_up.png"),
        load_texture(renderer, "assets/fx/firelion_left.png"),
        load_texture(renderer, "assets/fx/firelion_right.png"),
    };

    // Diken patlaması görselleri (yer efekti — 10 kare × 0.06sn)
    #define MAX_SPIKE_FX 6
    typedef struct { bool active; float x, y, t; } SpikeFxAnim;
    SpikeFxAnim spikeFx[MAX_SPIKE_FX] = {0};

    // Büyü dalgası 2: pençe/diken tek-atım animasyonları + hortum varlığı
    #define MAX_CLAW_FX 4
    SpikeFxAnim clawFx[MAX_CLAW_FX] = {0};      // lightningclaw sheet (16 kare)
    AnimFx animFxPool[MAX_ANIM_FX];             // VFX cilası: sheet-anim havuzu
    animfx_init(animFxPool);
    g_animFx = animFxPool;
    g_hitWeaponTex = hitWeaponTex; g_hitMagicTex = hitMagicTex;
    g_splatSmallTex = splatSmallTex; g_splatBigTex = splatBigTex;
    g_sparkTex = sparkBlockTex;
    #define MAX_ICETACLE_FX 4
    SpikeFxAnim icetacleFx[MAX_ICETACLE_FX] = {0};  // icetacle sheet (16 kare)
    #define MAX_SNAKEBITE_FX 4
    typedef struct { bool active; float x, y, t; int dirRow; bool flip; } SnakeFxAnim;
    SnakeFxAnim snakeFx[MAX_SNAKEBITE_FX] = {0};   // dirRow: 0 down/1 up/2 side
    #define MAX_FIRESURGES 3
    typedef struct {
        bool active;
        float x, y;          // merkez (dünya)
        float dirX, dirY;    // hamle yönü (birim)
        float t;             // yaş (ömür = FIRESURGE_LIFE)
        float damage;        // cast anında hesaplandı
        int hitIds[8];       // aynı moba İKİ KEZ vurmaz (tek dalga)
        int hitN;
        int facing;          // hangi sheet: 0 down/1 up/2 left/3 right
    } FireSurge;
    FireSurge firesurges[MAX_FIRESURGES] = {0};
    #define FIRESURGE_LIFE 0.7f
    #define FIRESURGE_SPEED 430.0f
    #define MAX_DARKBURST_FX 4
    SpikeFxAnim darkburstFx[MAX_DARKBURST_FX] = {0};   // dark_burst (16 kare 48×64)
    #define MAX_TORNADOES 3
    typedef struct {
        bool active;
        float x, y;         // merkez (dünya)
        float dirX, dirY;   // süzülme yönü (birim)
        float t, life;      // yaş / toplam ömür (sn)
        float tickTimer;    // sıradaki hasar tick'i
        float tickDamage;   // cast anında hesaplanan tick hasarı
    } Tornado;
    Tornado tornadoes[MAX_TORNADOES] = {0};

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
    int fatmaIdx = -1;   // K9 Kandil (D1a)
    int sifaciIdx = -1;  // Revir (Faz REVIR)
    int shopVendorIdx = -1;   // açık dükkanın sahibi (kırgınlık zammı okur)
    int nerimanIdx = -1, kemalIdx = -1;   // Suikastçi P1 (loca ustası + takip hedefi)
    bool locaBoardOpen = false;   // Sessiz El kontrat tahtası (P3)
    #define LOCABOARD_X (32.0f * TILE_SIZE)
    #define LOCABOARD_Y (19.0f * TILE_SIZE)
    #define LOCABOARD_RANGE 100.0f
    float tailNearTimer = 0.0f;   // takipte <100px yakınlık süresi (uyarı sayacı)
    int tailWarnings = 0;         // 3 uyarı = rapor sıfırlanır
    // ESO görev sunumu: kabul/aşama değişiminde ekran üstünde büyük bant
    const char *questBannerTitle = NULL, *questBannerSub = NULL;
    float questBannerTimer = 0.0f;
    // NPC indeksleri KİMLİKLE bulunur, görünen adla DEĞİL (npc.h'taki gerekçe:
    // ad hem gösterim hem kimlik olunca yeniden adlandırma zinciri sessizce
    // koparıyordu). Sekiz ayrı tarama döngüsü de tek yardımcıya indi.
    fatmaIdx    = npc_index_by_id("dunya_koy_kandilci");
    sifaciIdx   = npc_index_by_id("dunya_koy_sifaci");
    nerimanIdx  = npc_index_by_id("loca_salon_usta");
    kemalIdx    = npc_index_by_id("dunya_koy_ihtiyar");
    int nazifeIdx  = npc_index_by_id("dunya_koy_sofraci");   // K2 Son Müşteri (D1b)
    int kervanIdx  = npc_index_by_id("dunya_koy_kervanci");  // WE_CARAVAN gezgin tüccar
    int yabanciIdx = npc_index_by_id("dunya_ova_yabanci");   // Kör Kuyu tanıtıcısı (bir kez, gece)
    int nuhIdx     = npc_index_by_id("dunya_ova_sayan");     // S3 Sayan Adam (D1c)
    int zehraIdx   = npc_index_by_id("dunya_koy_torun");     // S3 — Nuh'un torunu
    int nuhLockDay = -1;   // yanlış cevap: o gün kilit (save'e girmez — zar kilidi gibi)
    #define SOFRA_X (25.5f * TILE_SIZE)   // han önü (köy GD, KÖY-YENİDEN) — gece kurulur
    #define SOFRA_Y (30.5f * TILE_SIZE)
    #define KAYIT_X (20.5f * TILE_SIZE)    // S3: kayıt odası kapısı (konak)
    #define KAYIT_Y (6.0f * TILE_SIZE)
    #define ECHO_K9 2                            // ECHOES[2] = Kerem (F2)
    #define KANDIL_X (144.0f * TILE_SIZE)        // geçit kemeri önü
    #define KANDIL_Y (8.5f * TILE_SIZE)
    int guardRegion = 0;
    if (guardIdx >= 0)
    {
        guard_init(guardIdx);
        guardRegion = region_of(NPCS[guardIdx].schedule[0].x, NPCS[guardIdx].schedule[0].y);
    }
    int lastSeenDay = gametime_day();   // günlük itibar toparlanması için gün takibi
    contract_new_day(lastSeenDay);      // panonun ilk günü (load bunu ezebilir)
    bool wasNightWE = gametime_is_night();   // Dünya Olayı: gece kenarı tespiti (baskın)
    float sealWhisperTimer = 20.0f;          // mühür sancısı: aralıklı fısıltı sayacı
    bool natureBounty = false;               // hava: dün yağdı → bugün ot toplaması ×2
    Weather lastWeather = WEATHER_CLEAR;     // değişim duyurusu (splash) için
    int graveSiteIdx = -1;                   // WE_GRAVE: huzursuz mezar sahası (-1 = yok)
    float stormBoltTimer = 6.0f;             // fırtına: sıradaki yıldırım düşüşü (sn)
    float lightningFlash = 0.0f;             // şimşek beyaz parlaması (sn, söner)
    #define RAIN_DROPS 140
    float rainPX[RAIN_DROPS], rainPY[RAIN_DROPS], rainPS[RAIN_DROPS];
    for (int i = 0; i < RAIN_DROPS; i++)     // normalize 0..1 ekran uzayı
    {
        rainPX[i] = randf();
        rainPY[i] = randf();
        rainPS[i] = 1.6f + randf() * 1.2f;   // ekran boyu/sn (hızlı damla önde)
    }
    ui_theme_init(renderer);   // panel/hücre/buton sprite'ları
    audio_init();      // başarısızsa oyun sessiz devam eder
    ambience_init();   // ortam sesi katmanı (Cila) — yoksa sessiz devam
    music_init();      // arka plan müziği (mood'a göre, crossfade)

    // Ölüm akışı: timer > 0 iken dünya donar, ekran kararır; bitince köyde diril
    float deathTimer = 0.0f;
    int deathGoldLost = 0;

    bool questLogOpen = false;   // J: Yolculuk günlüğü (kitap)
    int journalTab = 0;          // 0=Görevler 1=Günlük
    int journalPage = 0;
    int journalPageCount = 1;    // son render'ın döndürdüğü (klamp için)
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
    bool shopCaravan = false;   // açık dükkan kervan mı? (özel stok + %15 pahalı)
    bool shopLoca = false;      // İğne loca dükkanı (rütbe kapılı stok; lawless fiyat)
    bool shopDogu = false;      // Taşdurak tüccarı (bölgesel fiyat farkı — ticaret)
    ShopEntry caravanStock[12]; // aktif kervan stoğu: sabit liste + 2 rastgele tomar
    int caravanStockN = 0;      // (olayın startDay'i tohumlar — save gerekmez)
    int nearestNpc = -1;   // E prompt'u için (her kare güncellenir)
    int nearestRes = -1;   // E menzilindeki toplanabilir kaynak düğümü
    int gatherRes = -1;    // aktif toplama süreci (-1 = yok): savurma döngüsü çalışır

    // Craft durumu: panel + aktif üretim süreci (malzeme TAMAMLANINCA tüketilir)
    int nearestStation = -1;
    int nearestLamp = -1;      // kandil direği (LORE §1.1)
    float whisperTimer = 3.0f; // karanlığın sesi (LORE §1.2) — halka dışında işler
    int   lightMode = 0;       // kuantizasyon (F4): 0 serbest · 1/2 kademeli
    // Varsayılan LİNEER: sRGB uzayında toplamak bir HATA (merkez +0.22 fazla
    // parlayıp kırpılıyor). F1 ile eski hatalı moda dönülebilir — A/B (F12)
    // karşılaştırması için gerekli.
    int   lightGamma = 1;      // F1: 0 sRGB (eski/hatalı) · 1 lineer · 2 +ACES
    int   lightFall = 0;       // F2: 0 plato+ss · 1 Karis · 2 URP · 3 knee
    int   lightDither = 0;     // F3: 0 IGN · 1 Bayer · 2 dünya-çivili · 3 yok
    bool  lightSplit = false;  // F12: A/B karşılaştırma (sol orijinal, sağ seçili)
    int   lightShape = 0;      // Ctrl+L: 0 tipin kendi şekli · 1 basık · 2 dikey · 3 daire
    float lightOccl = 1.0f;    // O tuşu: ışık tıkama aç/kapa (0 = kapalı)
    // DEV: SU KATMAN ANAHTARI (Ctrl+B). 0 = hepsi açık, 1..8 = o katman YALNIZ
    // BAŞINA açık (izole mod). Katmanlar üst üste bindiği için biri bozuk olsa
    // hangisi olduğu görünmüyordu; izole bakış onu tek soruya indiriyor.
    // F1–F12 dolu olduğu için Ctrl+<harf> desenine uyuldu (Ctrl+L/O/V gibi).
    int   waterLayer = 0;
    // ATEŞ KATMANLARI (Ctrl+F izole modu). Su fazının dersi: efekti
    // ölçülebilir kılmadan ayar yapmak on tur harcıyor — bu kez izolasyon
    // İLK iş olarak kuruldu, ayarlardan önce.
    static const struct { int bit; const char *name; } FIRE_LAYERS[FX_L_COUNT] = {
        { FX_L_FLAME, "ATES: yalniz ALEV (ADD, bloom esigini gecer)" },
        { FX_L_SMOKE, "ATES: yalniz DUMAN (ALPHA, buyur ve soner)" },
        { FX_L_SPARK, "ATES: yalniz KIVILCIM (ADD, yercekimi kavisi)" },
        { FX_L_STEAM, "ATES: yalniz BUHAR (kazan)" },
        { FX_L_EMBER, "ATES: yalniz KOR (sabit parlar, yerinde)" },
        { FX_L_LICK,  "ATES: yalniz ALEV DILI (damla, yukari kivrilir)" },
        { FX_L_SOOT,  "ATES: yalniz IS PULU (savrulur, duser)" },
        { FX_L_DEEP,  "ATES: yalniz KIZIL ZARF (ALPHA, alevin dis govdesi)" },
        { FX_L_DARK,  "ATES: yalniz KOYU SILUET (sarinin USTUNE cizilir)" },
    };
    int fireLayer = 0;   // 0 = hepsi açık
    // DEV: silah efekti geçersiz kılma (Ctrl+G). COUNT = kapalı.
    WeaponFxType devFxOverride = WEAPON_FX_COUNT;
    // DEV: sınırsız mana/stamina (Ctrl+I). VFX ve skill testinde mana
    // hemen bitiyor ve efekti izlemeye vakit kalmıyordu.
    bool devInfinite = false;
    // DEV: skill FX A/B kıyas modu. Prosedürel motor artık VARSAYILAN
    // (2026-08-22); yol göstergesi splash'i normal oyunda gürültü olur.
    // Ctrl+P'ye ilk basış bu bayrağı açar, gösterge ancak o zaman konuşur.
    bool devSkillfxAB = false;

    static const struct { int bit; const char *name; } WATER_LAYERS[R2D_WATER_L_COUNT] = {
        { R2D_WATER_L_REFRACT, "SU: yalniz KIRILMA (sualti bukulmesi)" },
        { R2D_WATER_L_DEPTH,   "SU: yalniz DERINLIK ortmesi (su rengi)" },
        { R2D_WATER_L_CAUSTIC, "SU: yalniz KOSTIK (isik agi)" },
        { R2D_WATER_L_GLINT,   "SU: yalniz PARLAMA (gunes acisi)" },
        { R2D_WATER_L_FOAM,    "SU: yalniz KOPUK (kiyiya vuran dalga)" },
        { R2D_WATER_L_WET,     "SU: yalniz ISLAK BANT (kiyinin KARA tarafi)" },
        { R2D_WATER_L_DARK,    "SU: yalniz KOYU HAT (suyun silueti)" },
        { R2D_WATER_L_MENI,    "SU: yalniz MENISKUS (parlak kiyi cizgisi)" },
        { R2D_WATER_L_BANK,    "SU: yalniz KIYI GOLGESI (kuzey kiyinin cukuru)" },
        { R2D_WATER_L_DASH,    "SU: yalniz ISILTI CIZGILERI (ayrik ogeler)" },
        { R2D_WATER_L_ARC,     "SU: yalniz ARK SIRALARI (kiyiya paralel yanki)" },
        { R2D_WATER_L_RING,    "SU: yalniz HALKA DALGACIK (balik/damla izi)" },
        { R2D_WATER_L_LAMP,    "SU: yalniz LAMBA YANSIMASI (gece)" },
    };
    bool craftOpen = false;
    int craftStation = -1;    // panelin bağlı olduğu istasyon
    int craftRecipe = -1;     // süreçteki tarif (RECIPES indeksi)
    bool plusForge = false;   // '+basma' süreci (ocak, kuşanılı silah)
    float craftProgress = 0.0f;

    // Günlük ders damgaları (öğretmenler; save'e girmez — gün başına sıfır kabulü)
    int trainedDay[32];
    for (int i = 0; i < 32; i++) trainedDay[i] = -1;
    craft_reset_learned();   // blueprint tarifleri kapalı başlar (save v10 doldurur)
    lamp_reset();            // kandil hattı: meydan ve bostan SÖNÜK başlar (LORE §13.2)

    // Sunak durumu: hedef ekipman + aktif basma işlemi (0 yok, 1/2 rün, 10+s stat)
    EquipSlot enchTarget = EQUIP_WEAPON;
    int enchAction = 0;

    QuestStatus questStatus[MAX_QUESTS] = {0};   // hepsi NOT_STARTED

    // Başlangıç seti: tam demir set + kılıç KUŞANILI (+0), çantada 3 iksir
    equipment.slots[EQUIP_WEAPON] = item_make(&ITEM_TYPES[ITEM_LONG_SWORD], 1, 0);
    equipment.slots[EQUIP_HELMET] = item_make(&ITEM_TYPES[ITEM_HELMET_IRON], 1, 0);
    equipment.slots[EQUIP_CHEST]  = item_make(&ITEM_TYPES[ITEM_CHEST_IRON], 1, 0);
    equipment.slots[EQUIP_LEGS]   = item_make(&ITEM_TYPES[ITEM_LEGS_IRON], 1, 0);
    bag_add(&ITEM_TYPES[ITEM_POTION_HP], 3, 0);
        

    // static: 8192 partiküllük havuz ~460KB. main()'in yığınında durmasın
    // (yığın taşması sessiz ve teşhisi zor bir çökme sınıfı).
    static ParticleSystem weaponFx;
    fx_init(&weaponFx);
    fx_set_death_hook(fx_on_particle_death);
    skillfx_init();
    groundfx_init();

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
    bool prevAttacking = false; // slash yayı VFX tetiği (yükselen kenar)
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
    ecology_reset();   // taban yuvalar dolu (yeni oyun/yükleme bunu ezer)
    if (kervanIdx >= 0) npc_force_hidden(kervanIdx, true);   // kervan sadece olay günlerinde
    if (yabanciIdx >= 0) npc_force_hidden(yabanciIdx, true);  // Yabancı yalnız karşılaşmada
    int spawnZoneCount = ecology_populate(spawnZones, MAX_SPAWN_ZONES,
                                          mobs, MAX_MOBS, &nextMobId);

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
    // ZEMİN PARÇASI: karo dokularını bağla. Zemin kırıntısı bu dizideki gerçek
    // dokudan kesiliyor (düz renk değil), o yüzden bağlama dokular YÜKLENDİKTEN
    // sonra yapılmak zorunda.
    groundfx_bind_tiles(tile_textures);
    // Parça atlası: karo PNG'leri CPU'da okunup yumuşak-kenarlı parça
    // varyantları basılıyor. Bu olmadan parçalar keskin KARE olur.
    groundfx_bake(renderer);
    g_landFx = &weaponFx;
    groundfx_set_land_hook(groundfx_on_land);
    tilemap_load_details(renderer);   // çimen varyantları + kenar kaplamaları

    bool inventoryOpen = false;
    bool skillsOpen = false;
    // Sürükleme (Cila): sol tık basılı taşıma — kaynak envanter YA DA ekipman
    int dragInvSlot = -1;              // -1 = envanterden sürüklenmiyor
    EquipSlot dragEquipSlot = EQUIP_NONE;
    GameState gameState = GAME_TITLE;      // açılış: başlık ekranı → ana menü (Faz B1)
    MenuScreen menuScreen = MENU_ROOT;

    // Slot sistemi (Faz B1): aktif slot (F5/F9 hedefi) + kart özetleri
    int activeSlot = -1;   // -1 = slot seçilmedi (auto'dan yüklendi / hiç)
    SaveSummary slotSums[SAVE_SLOT_COUNT + 1];
    for (int i = 0; i <= SAVE_SLOT_COUNT; i++)
        save_read_summary(save_slot_path(i), &slotSums[i]);
    float titlePulse = 0.0f;   // "bir tuşa bas" nefes animasyonu

    // Karakter yaratma (lite): Yeni Oyun slotu seçilince açılır
    int createSlot = -1;
    char createName[24] = "";
    int createSkin = 0;      // 0 açık / 1 buğday / 2 koyu
    int createBg = 0;        // 0 Gezgin / 1 Asker / 2 Avcı / 3 Çırak

    // Ses ayarları (Cila): settings.cfg'den yüklenir, kaydırıcıyla kaydedilir
    float musicVol = 1.0f, sfxVol = 1.0f;
    {
        FILE *cf = fopen("settings.cfg", "r");
        if (cf != NULL)
        {
            if (fscanf(cf, "music=%f sfx=%f", &musicVol, &sfxVol) != 2)
            { musicVol = 1.0f; sfxVol = 1.0f; }
            fclose(cf);
        }
        music_set_volume(musicVol);
        ambience_set_volume(musicVol);
        audio_set_volume(sfxVol);
        menu_set_volumes(musicVol, sfxVol);
    }
    audio_play(SFX_WHISPER);   // başlık ekranı: fısıltı temayı ilk saniyede kurar

    // 4) Oyun döngüsü
    bool running = true;

    // Harita: Tiled CSV'den; yoksa eski test haritası (generate_map)
    if (!tilemap_load_csv("assets/maps/world.csv"))
        generate_map();
    minimap_init(renderer);   // harita boyutu belli olduktan SONRA
    worldmap_init(renderer);  // geçit görselleri
    trap_init(renderer);      // diken/plaka/arbalet sprite'ları (sandık günleri worldmap'te)
    scene_set_renderer(renderer);   // vinyet sahneleri görsel yükleyebilsin

    // Conjuration dostları (M4)
    Ally allies[ALLY_MAX];
    ally_reset(allies);
    bool allyWasActive[ALLY_MAX] = { false, false };

    // Cine modu (D1a): letterbox bantları + kamera hedefe süzülür; süre bitince
    // oyuncuya döner. Sahne araçlarının in-engine yarısı (vinyet tam ekranın aksine
    // dünya görünür kalır). cineTimer > 0 iken oyuncu girdisi kilitli (main gate'ler).
    bool cineActive = false;
    float cineX = 0.0f, cineY = 0.0f;
    float cineTimer = 0.0f;
    float cineBand = 0.0f;   // 0..1 bant animasyonu (yalnız cine/scene)
    float dlgBand = 0.0f;    // 0..1 diyalog üst bandı (alt panele binmez)

    // Mühür dalgası (zindan): periyodik toplu diriliş — sabit gerçek-zaman aralığı
    #define SEAL_WAVE_SECONDS 120.0f
    float sealWaveTimer = SEAL_WAVE_SECONDS;
    bool sealWaveWarned = false;

    // Harita geçişi: E ile geçit (pendingPortal) ya da yükleme sonrası (mapReload)
    int pendingPortal = -1;
    bool mapReloadNeeded = false;
    int nearPortal = -1;
    int nearestChest = -1;   // menzildeki kapalı sandık ([E] ile açılır)
    int nearestEcho = -1;    // menzildeki OKUNMAMIŞ yankı noktası ([E] ile yaşanır)
    int nearestRitual = -1;  // menzildeki ritüel yuvası (gölcük kurut / mezar mühürle)
    int activeEcho = -1;     // şu an yaşanan yankı (seçim yapılınca done işaretlenir)
    bool arenaClosed = false;   // boss arena kapısı (tile duvarı) indi mi
    // Ölüm Getiren: Kara Patlama telegraph'ları (1 sn büyür → patlar)
    struct { bool on; float x, y, t; } deathBlast[4] = {{0}};
    // Kutsal Alan (Faz HOLY): tek aktif çember — 6sn, undead yavaş + tick
    struct { bool on; float x, y, t, tick, r; } holyZone = {0};
    // Yankı Çağırma (P2): gölge kopya — hedefinin yanında süzülür, %35 vurur
    struct { bool on; float t, atkT, x, y; bool facingLeft; } echoShadow = {0};
    #define T_BOSS_X (22.0f * TILE_SIZE)
    #define T_BOSS_Y (23.0f * TILE_SIZE)

    // Bölge splash durumu: bölge değişince ad 3 sn görünür
    int currentRegion = -1;
    const char *splashText = NULL;
    // splashText bir İŞARETÇİ: yerel diziyi atamak use-after-scope olur.
    // Biçimlenmiş splash metinleri buraya yazılır (döngü boyunca yaşar).
    char splashOwn[96] = { 0 };
    float splashTimer = 0.0f;
    TTF_Font *splashFont = font_load("assets/fonts/PublicPixel-rv0pA.ttf", 32.0f);
    Camera camera = {.x = 0.0f, .y = 0.0f, .zoom = 1.0f};

    // His katmanı: ekran sarsıntısı + hasar flash'ı (oyuncu odaklı)
    float shakeTime = 0.0f, shakeDur = 1.0f, shakeAmp = 0.0f; // amp: ekran px
    float flashTime = 0.0f;                                    // kalan flash süresi
#define FLASH_DUR 0.35f

    Uint64 last = SDL_GetTicks();

    // DEV kancası: PIXE_AUTOLOAD=auto|<slotNo> → başlıkta durmadan kaydı yükle
    // (ASan/repro koşuları menüsüz oyuna girebilsin; normal oyunda etkisiz)
    const char *autoloadEnv = SDL_getenv("PIXE_AUTOLOAD");
    int autoloadSlot = SAVE_SLOT_AUTO;
    if (autoloadEnv != NULL && autoloadEnv[0] >= '1' && autoloadEnv[0] <= '9')
        autoloadSlot = SDL_atoi(autoloadEnv) - 1;   // insan sayısı → slot indeksi
    if (autoloadEnv != NULL &&
        load_game(save_slot_path(autoloadSlot), &player,
                  &inventory, &craftBag, &equipment, &hotbar, questStatus))
    {
        mapReloadNeeded = true;
        tree_compute_effects(player.treeTaken, player.fateTaken, &perkFx);
        spells_apply_learned(&player, &perkFx);
        echo_apply_unlocks(&player, &perkFx);
        skill_set_xp_mult(perkFx.xpGainMult);
        hotbar_purge_closed(&hotbar, perkFx.abilityOpen);
        hotbar_place_unlocks(&hotbar, perkFx.abilityOpen);
        if (guardIdx >= 0) guard_init(guardIdx);
        lastSeenDay = gametime_day();
        wasNightWE = gametime_is_night();
        bags_apply_caps(&player, &inventory, &craftBag);
        if (kervanIdx >= 0)
            npc_force_hidden(kervanIdx, worldevent_current() != WE_CARAVAN);
        gameState = GAME_PLAYING;
        SDL_Log("DEV: PIXE_AUTOLOAD — otomatik kayit yuklendi");
    }

    while (running)
    {
        bool attackPressed = false;
        bool targetPressed = false;
        int hotbarPressed = -1;   // bu kare basılan hotbar yuvası (tuş/tık; -1 = yok)
        // 4a) INPUT: OS'in bu kare için kuyruğa attığı tüm olayları boşalt.
        SDL_Event event;
        // Çapa çözümü pencere boyutunu izler ve düzenleyicinin TIKLAMA testi de
        // ona bakıyor — bu yüzden olaylardan ÖNCE güncelleniyor. (Çizim için
        // aşağıda winW/winH yeniden okunuyor; ikisi aynı kare içinde aynı değer.)
        {
            int ewW = 0, ewH = 0;
            SDL_GetWindowSize(window, &ewW, &ewH);
            uilayout_set_window(ewW, ewH);
        }

        while (SDL_PollEvent(&event))
        {

            if (event.type == SDL_EVENT_QUIT)
            {                    // Kullanıcı pencereyi kapatmak istedi mi?
                running = false; // Oyun döngüsünden çık
                continue;        // QUIT her şeyin önünde: düzenleyiciye sorulmaz
            }
            // YERLEŞİM DÜZENLEYİCİSİ (Ctrl+U) — QUIT'ten SONRA, oyun girdisinden
            // ÖNCE. Açıkken olayları yutar: sürüklerken karakter yürümesin, ok
            // tuşları günlük sayfası çevirmesin, rakamlar hotbar tetiklemesin.
            if (uieditor_handle_event(&event))
                continue;
            if (event.type == SDL_EVENT_TEXT_INPUT &&
                     gameState == GAME_CREATE)
            {
                // İsim girişi (UTF-8; 23 bayt tavan)
                if (SDL_strlen(createName) + SDL_strlen(event.text.text)
                        < sizeof(createName))
                    SDL_strlcat(createName, event.text.text, sizeof(createName));
            }
            else if (event.type == SDL_EVENT_KEY_DOWN)
            { // Bir tuşa basıldı mı?
                if (gameState == GAME_CREATE)
                {
                    if (event.key.key == SDLK_BACKSPACE)
                    {
                        // UTF-8 farkındalıklı son karakteri sil
                        int len = (int)SDL_strlen(createName);
                        while (len > 0 &&
                               ((unsigned char)createName[len - 1] & 0xC0) == 0x80)
                            len--;
                        if (len > 0) len--;
                        createName[len] = '\0';
                    }
                    else if (event.key.key == SDLK_ESCAPE)
                    {
                        SDL_StopTextInput(window);
                        gameState = GAME_MENU;
                        menuScreen = MENU_ROOT;
                    }
                    // Enter = Başla: fare işleyicisindeki butonla aynı yol —
                    // kod tekrarı olmasın diye buton tıklaması simüle edilir
                    else if (event.key.key == SDLK_RETURN)
                    {
                        int cwW2, cwH2;
                        SDL_GetWindowSize(window, &cwW2, &cwH2);
                        SDL_Event fake = { 0 };
                        fake.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
                        fake.button.button = SDL_BUTTON_LEFT;
                        fake.button.x = cwW2 / 2.0f;
                        fake.button.y = cwH2 * 0.78f + 20.0f;
                        SDL_PushEvent(&fake);
                    }
                }
                else if (gameState == GAME_TITLE)
                {
                    // Başlık ekranı: herhangi bir tuş → ana menü
                    gameState = GAME_MENU;
                    menuScreen = MENU_ROOT;
                    for (int i = 0; i <= SAVE_SLOT_COUNT; i++)
                        save_read_summary(save_slot_path(i), &slotSums[i]);
                    audio_play(SFX_UI);
                }
                else if (gameState == GAME_MENU)
                {
                    // Ana menüde ESC: alt ekrandan köke dön (kökte yutulur)
                    if (event.key.key == SDLK_ESCAPE && menuScreen != MENU_ROOT)
                        menuScreen = MENU_ROOT;
                }
                else if (gameState == GAME_PLAYING && scene_active() &&
                         event.key.key != SDLK_ESCAPE &&
                         event.key.key != SDLK_E && event.key.key != SDLK_SPACE &&
                         !(event.key.key >= SDLK_1 && event.key.key <= SDLK_8))
                {
                    // Sahne (vinyet) açıkken diğer TÜM tuşlar yutulur — panel
                    // açılması immersion'ı bozar. Sahnenin dili: 1-8 seçim,
                    // E/Space typewriter hızı (aşağıda ayrıca yutulur), ESC menü.
                }
                else if (trashSlot >= 0 && gameState == GAME_PLAYING &&
                         (event.key.key == SDLK_1 || event.key.key == SDLK_2 ||
                          event.key.key == SDLK_ESCAPE))
                {
                    // ÇÖP ONAYI: [1] yok et · [2]/ESC vazgeç
                    if (event.key.key == SDLK_1)
                    {
                        Inventory *tb2 = (trashTab == 1) ? &craftBag : &inventory;
                        ItemStack *ts2 = &tb2->slots[trashSlot];
                        if (trashSlot < tb2->cap && ts2->type != NULL)
                        {
                            char tbuf[FLOAT_TEXT_LEN];
                            snprintf(tbuf, sizeof(tbuf), "%s yok edildi",
                                     ts2->type->name);
                            ts2->type = NULL;
                            ts2->count = 0;
                            ts2->plusLevel = 0;
                            SDL_Color red = { 255, 120, 100, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f, tbuf, red);
                        }
                    }
                    trashTab = -1;
                    trashSlot = -1;
                    audio_play(SFX_UI);
                }
                else if (event.key.key == SDLK_ESCAPE)
                {
                    if (gameState == GAME_PAUSED)
                    {
                        // Menu acikken ESC: alt ekrandaysan MAIN'e, MAIN'deysen oyuna don
                        if (menuScreen != MENU_MAIN)
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
                    else if (locaBoardOpen)
                    {
                        locaBoardOpen = false; // 0a2) Sessiz El kontrat tahtası
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
                else if ((event.key.key == SDLK_E || event.key.key == SDLK_SPACE) &&
                         gameState == GAME_PLAYING && scene_active())
                {
                    // Sahne açıkken E/Space yalnız typewriter'ı hızlandırır
                    // (basılı tutma update'te poll edilir) — başka işe BAKMAZ
                }
                else if (event.key.key == SDLK_SPACE && gameState == GAME_PLAYING &&
                         lockpick_active())
                {
                    // Kilit minigame'i: Space = zamanlama denemesi
                    int li = lockpick_loot();
                    int res = lockpick_press(&perkFx, player.skills[SKILL_LOCKPICKING].level);
                    if (res == 1 && li >= 0 && loots[li].active)
                    {
                        // AÇILDI: keystone bedeli varsa başarıda da maymuncuk gider
                        if (perkFx.lockAlwaysConsume)
                            bag_consume(&ITEM_TYPES[ITEM_LOCKPICK]);
                        loots[li].lockTier = -1;
                        loots[li].state = LOOT_OPENING;
                        loots[li].currentFrame = 0;
                        loots[li].animTimer = 0.0f;
                        openLootIndex = li;
                        if (loots[li].chestId >= 0)
                            chest_set_opened_day((int)loots[li].chestId, gametime_day());
                        int tier = lockpick_tier();
                        if (skill_add_xp(player.skills, SKILL_LOCKPICKING,
                                         12.0f * (float)(tier + 1)))
                            on_skill_levelup(&player, SKILL_LOCKPICKING, &hotbar, floatTexts);
                        audio_play(SFX_LOOT);
                        SDL_Color green = { 120, 255, 120, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f, "Kilit açıldı!", green);
                    }
                    else if (res == -1)
                    {
                        // KIRILDI: maymuncuk gider (Sağlam/Usta Uç kurtarabilir) + GÜRÜLTÜ
                        bool saved = randf() < perkFx.pickSaveChance;
                        if (!saved)
                            bag_consume(&ITEM_TYPES[ITEM_LOCKPICK]);
                        if (skill_add_xp(player.skills, SKILL_LOCKPICKING, 4.0f))
                            on_skill_levelup(&player, SKILL_LOCKPICKING, &hotbar, floatTexts);
                        audio_play(SFX_REFLECT);   // metalik tinlama = gürültü
                        SDL_Color red = { 255, 120, 100, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f,
                                             saved ? "Kaydı! (maymuncuk kurtuldu)"
                                                   : "Maymuncuk kırıldı!", red);

                        // Gürültü: yakın mobların şüphesi dolar (Sessiz Çalışma yarılatır)
                        float noiseR = 260.0f * perkFx.lockNoiseMult;
                        for (int m = 0; m < MAX_MOBS; m++)
                        {
                            if (!mobs[m].e.alive) continue;
                            float ndx = (mobs[m].e.rect.x + mobs[m].e.rect.w / 2.0f)
                                      - (player.rect.x + player.rect.w / 2.0f);
                            float ndy = (mobs[m].e.rect.y + mobs[m].e.rect.h / 2.0f)
                                      - (player.rect.y + player.rect.h / 2.0f);
                            if (SDL_sqrtf(ndx * ndx + ndy * ndy) <= noiseR)
                            {
                                mobs[m].suspicion += 0.65f;
                                if (mobs[m].suspicion > 1.0f) mobs[m].suspicion = 1.0f;
                            }
                        }
                    }
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

                            // MÜSADERE (Kirli Mal): üstündeki kanıt toplanır —
                            // kirli altın + tüm ÇALINTI eşyalar muhafıza gider
                            int seized = player.dirtyGold;
                            player.dirtyGold = 0;
                            for (int b2 = 0; b2 < 2; b2++)
                            {
                                Inventory *bb = (b2 == 0) ? &inventory : &craftBag;
                                for (int sl2 = 0; sl2 < INVENTORY_SIZE; sl2++)
                                    if (bb->slots[sl2].type != NULL &&
                                        bb->slots[sl2].stolen)
                                    {
                                        seized += item_value(&bb->slots[sl2])
                                                * bb->slots[sl2].count / 2;
                                        bb->slots[sl2] = item_make(NULL, 0, 0);
                                    }
                            }
                            if (seized > 0)
                            {
                                char mbuf[FLOAT_TEXT_LEN];
                                snprintf(mbuf, sizeof(mbuf),
                                         "Kirli mal müsadere edildi (~%dg)", seized);
                                SDL_Color red2 = { 255, 120, 100, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 26.0f, mbuf, red2);
                            }
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
                else if (((event.key.key >= SDLK_1 && event.key.key <= SDLK_9) ||
                          event.key.key == SDLK_0) &&
                         gameState == GAME_PLAYING)
                {
                    if (scene_active())
                    {
                        // Vinyet açıkken sayılar SAHNE seçeneği seçer
                        SceneFx sfx;
                        int sel = (int)(event.key.key - SDLK_1);
                        if (scene_choose(sel, &sfx))
                        {
                            player.echoesWitnessed += sfx.witness;
                            player.echoesConsumed += sfx.consume;

                            // YANKICI çağrısı: 3. sömürüde fısıltı YER fısıldar
                            // (tek ipucu bu — taş halkası haritada işaretsiz)
                            if (sfx.consume > 0 && player.echoesConsumed >= 3 &&
                                player.storyFlags[SF_YANKICI] == 0)
                            {
                                player.storyFlags[SF_YANKICI] = 1;
                                audio_play(SFX_WHISPER);
                                SDL_Color vio = { 200, 160, 255, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                    player.rect.x + player.rect.w / 2.0f,
                                    player.rect.y - 40.0f,
                                    "\"Seni tanıyoruz artık. Taşların eskisine gel. Ay görmesin.\"",
                                    vio);
                                journal_add(JE_ECHO_CALL, 0);
                            }

                            // RİTÜEL kabulü: sınıf + silah açılır (geri dönüşsüz)
                            if (sfx.ritual == 1 &&
                                player.storyFlags[SF_YANKICI] < 2)
                            {
                                player.storyFlags[SF_YANKICI] = 2;
                                spells_apply_learned(&player, &perkFx);
                               echo_apply_unlocks(&player, &perkFx);
                                echo_apply_unlocks(&player, &perkFx);
                                hotbar_place_unlocks(&hotbar, perkFx.abilityOpen);
                                // Fısıltı Keskisi taşın dibinde belirir
                                int li2 = loot_spawn(loots, MAX_LOOT,
                                                     108.0f * TILE_SIZE,
                                                     7.5f * TILE_SIZE,
                                                     NULL, 0, 0, 0);
                                if (li2 >= 0 && loots[li2].itemCount < MAX_LOOT_ITEMS)
                                    loots[li2].items[loots[li2].itemCount++] =
                                        item_make(&ITEM_TYPES[ITEM_WHISPER_BLADE], 1, 0);
                                journal_add(JE_RITUAL, 0);
                                audio_play(SFX_PERK);
                                splashText = "YANKICI";
                                splashTimer = 4.0f;
                            }
                            if (sfx.s3 == 2)   // nöbet devralındı (S3)
                            {
                                player.storyFlags[SF_S3_STATE] = 2;
                                journal_add(JE_S3_CHOICE, 2);
                                title_notify(title_stat_add(&player, TS_RECORD, 1),
                                             &player, floatTexts);
                            }
                            else if (sfx.s3 == 3)   // kayıt kapatıldı
                            {
                                player.storyFlags[SF_S3_STATE] = 3;
                                journal_add(JE_S3_CHOICE, 3);
                            }
                            if ((sfx.witness > 0 || sfx.consume > 0) && activeEcho >= 0)
                            {
                                echo_set_done(activeEcho, true);   // anı bir kez yaşanır
                                journal_add(sfx.witness > 0 ? JE_ECHO_WITNESS
                                                            : JE_ECHO_CONSUME, activeEcho);
                                if (activeEcho == 0)   // K2: hangi hattı seçtin (kalıcı)
                                    player.storyFlags[SF_K2_CART] =
                                        sfx.witness > 0 ? 1 : 2;
                            }
                            if (sfx.gold > 0)
                                player.gold += sfx.gold;
                            if (sfx.essence > 0)
                            {
                                bag_add(&ITEM_TYPES[ITEM_ESSENCE_PALE],
                                              sfx.essence, 0);
                                SDL_Color purple = { 200, 150, 255, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f,
                                                     "+Soluk Öz", purple);
                            }
                        }
                    }
                    else if (dialogNpc >= 0 &&
                             !dialog_type_done(&npcDialogs[dialogNpc], dialogNode))
                    {
                        // Metin hâlâ akıyor: sayı da E gibi tamamlar (seçim yapmaz)
                        dialog_type_reveal();
                    }
                    else if (dialogNpc >= 0)
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
                                shopCaravan = (dialogNpc == kervanIdx);
                                shopLoca = (nerimanIdx >= 0 && dialogNpc >= 0 &&
                                    SDL_strcmp(NPCS[dialogNpc].id, "loca_hucre_zehirci") == 0);
                                shopDogu = (dialogNpc >= 0 &&
                                    SDL_strcmp(NPCS[dialogNpc].id, "tasdurak_dukkan_tuccar") == 0);
                                shopVendorIdx = dialogNpc;   // kırgınlık zammı için
                                if (shopDogu)
                                {
                                    // TAŞDURAK stoğu: çöl malları ucuz (ticaret
                                    // döngüsünün alım ayağı) + temel ihtiyaç
                                    caravanStockN = 0;
                                    caravanStock[caravanStockN++] =
                                        (ShopEntry){ &ITEM_TYPES[ITEM_DATE_FRUIT] };
                                    caravanStock[caravanStockN++] =
                                        (ShopEntry){ &ITEM_TYPES[ITEM_SPICE] };
                                    caravanStock[caravanStockN++] =
                                        (ShopEntry){ &ITEM_TYPES[ITEM_POTION_HP] };
                                    caravanStock[caravanStockN++] =
                                        (ShopEntry){ &ITEM_TYPES[ITEM_ARROW] };
                                    caravanStock[caravanStockN++] =
                                        (ShopEntry){ &ITEM_TYPES[ITEM_POTION_MANA] };
                                }
                                else if (shopLoca)
                                {
                                    // LOCA DÜKKANI (P3): rütbeye göre açılan mal
                                    int rank = assassin_rank(player.assassinPoints);
                                    caravanStockN = 0;
                                    caravanStock[caravanStockN++] =
                                        (ShopEntry){ &ITEM_TYPES[ITEM_NEEDLE_VENOM] };
                                    if (rank >= 1)   // El
                                        caravanStock[caravanStockN++] =
                                            (ShopEntry){ &ITEM_TYPES[ITEM_QUIET_VIAL] };
                                    if (rank >= 2)   // Bıçak — Çift Hançer seçeneği
                                        caravanStock[caravanStockN++] =
                                            (ShopEntry){ &ITEM_TYPES[ITEM_DAGGER_OFFHAND] };
                                    if (rank >= 4)   // Usta El — unique nişan
                                        caravanStock[caravanStockN++] =
                                            (ShopEntry){ &ITEM_TYPES[ITEM_DEATH_SIGIL] };
                                }
                                else if (shopCaravan)
                                {
                                    // Kervan stoğu: sabitler + havuzdan 2 FARKLI tomar.
                                    // startDay tohumlu → aynı ziyarette hep aynı ikili
                                    // (save/load değişmez), sonraki kervanda değişir.
                                    static const int TOME_POOL[3] = {
                                        ITEM_TOME_CLAW, ITEM_TOME_SNAKEBITE,
                                        ITEM_TOME_ICETACLE };
                                    int weCur, weStart, weLast;
                                    worldevent_save_get(&weCur, &weStart, &weLast);
                                    caravanStockN = 0;
                                    for (int c = 0; c < CARAVAN_STOCK_COUNT &&
                                                    caravanStockN < 10; c++)
                                        caravanStock[caravanStockN++] = CARAVAN_STOCK[c];
                                    int t1 = ((weStart % 3) + 3) % 3;
                                    int t2 = (t1 + 1 + (((weStart / 3) % 2) + 2) % 2) % 3;
                                    caravanStock[caravanStockN++] =
                                        (ShopEntry){ &ITEM_TYPES[TOME_POOL[t1]] };
                                    caravanStock[caravanStockN++] =
                                        (ShopEntry){ &ITEM_TYPES[TOME_POOL[t2]] };
                                }
                                inventoryOpen = true;   // satış için çanta da açık
                                dialogNpc = -1;
                            }
                            else if (strcmp(target, "kuyu:1") == 0)
                            {
                                player.storyFlags[SF_WELL] = 1;   // kuyu artık aktif
                                if (yabanciIdx >= 0)
                                    npc_force_hidden(yabanciIdx, true);   // hiç olmadı
                                journal_add(JE_WELL_INTRO, 0);
                                dialogNpc = -1;
                            }
                            else if (strcmp(target, "as1:ver") == 0)
                            {
                                // SINAV verildi: gece Baran'ın tezgahından
                                // Mühürlü Kutu — TANIKSIZ
                                player.storyFlags[SF_ASSASSIN] = 2;
                                dialogNpc = -1;
                                npc_bark(nerimanIdx, "Kutu. Tanıksız. Git.", 4.0f);
                                questBannerTitle = "SINAV: GÖRÜLMEDEN";
                                questBannerSub =
                                    "Gece, Baran'ın tezgahından Mühürlü Kutu'yu al — tanıksız";
                                questBannerTimer = 5.0f;
                                audio_play(SFX_PERK);
                            }
                            else if (strcmp(target, "as1:teslim") == 0)
                            {
                                if (bag_consume(&ITEM_TYPES[ITEM_SEALED_BOX]))
                                {
                                    player.storyFlags[SF_ASSASSIN] = 4;
                                    player.storyFlags[SF_TAIL_MARK] = 0;
                                    tailWarnings = 0;
                                    audio_play(SFX_PERK);
                                    npc_bark(nerimanIdx,
                                             "Temiz iş. Şimdi: Kemal. Gölge ol.", 5.0f);
                                    questBannerTitle = "GÖLGE OL";
                                    questBannerSub =
                                        "Kemal'i gün boyu izle — üç durağını işaretle, görünme";
                                    questBannerTimer = 5.0f;
                                }
                                dialogNpc = -1;
                            }
                            else if (strcmp(target, "as2:teslim") == 0)
                            {
                                player.storyFlags[SF_ASSASSIN] = 6;
                                player.gold += 150;
                                questBannerTitle = "GÖREV TAMAMLANDI";
                                questBannerSub = "Sessiz El'in güveni — locayı kazandın";
                                questBannerTimer = 5.0f;
                                audio_play(SFX_PERK);
                                SDL_Color hgold4 = { 255, 235, 150, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                    player.rect.x + player.rect.w / 2.0f,
                                    player.rect.y - 10.0f,
                                    "+150 g — Sessiz El'in güveni", hgold4);
                                npc_bark(nerimanIdx,
                                         "Gözün var. Sıradaki iş... hazırlanıyor.", 5.0f);
                                dialogNpc = -1;
                            }
                            else if (strcmp(target, "as3:ver") == 0)
                            {
                                player.storyFlags[SF_ASSASSIN] = 7;
                                int r3 = recipe_index_by_id("alch_dilsiz_sise");
                                if (r3 >= 0) craft_learn_recipe(r3);
                                questBannerTitle = "DİLSİZ ŞİŞE";
                                questBannerSub =
                                    "3 Zehirli Öz + 2 Mavi Ot — simya kazanında kaynat, İğne'ye götür";
                                questBannerTimer = 5.0f;
                                audio_play(SFX_PERK);
                                dialogNpc = -1;
                            }
                            else if (strcmp(target, "as3:teslim") == 0)
                            {
                                if (bag_consume(&ITEM_TYPES[ITEM_QUIET_VIAL]))
                                {
                                    player.storyFlags[SF_ASSASSIN] = 8;
                                    audio_play(SFX_PERK);
                                    questBannerTitle = "GÖREV GÜNCELLENDİ";
                                    questBannerSub = "Neriman yeni işi anlatacak — locaya";
                                    questBannerTimer = 4.0f;
                                }
                                dialogNpc = -1;
                            }
                            else if (strncmp(target, "as4:", 4) == 0)
                            {
                                // SAHTE ÖLÜM kararı: tanık ne olacak? (kişiliğe işler)
                                if (strcmp(target + 4, "sustur") == 0)
                                    player.persKaba += 1;
                                else if (strcmp(target + 4, "rusvet") == 0)
                                {
                                    player.gold -= 100;
                                    if (player.gold < 0) player.gold = 0;
                                    player.persKetum += 1;
                                }
                                else
                                    player.persKibar += 1;
                                player.storyFlags[SF_ASSASSIN] = 10;
                                audio_play(SFX_PERK);
                                questBannerTitle = "ÖLMEDEN ÖL — TAMAM";
                                questBannerSub = "Loca sahneyi kapattı; Neriman son işi bekliyor";
                                questBannerTimer = 5.0f;
                                dialogNpc = -1;
                            }
                            else if (strcmp(target, "as5:ver") == 0)
                            {
                                player.storyFlags[SF_ASSASSIN] = 11;
                                questBannerTitle = "İLK İMZA";
                                questBannerSub =
                                    "Kaçak Tahsildar — GECE kervan yolunda; nasıl öldüğü sana kalmış";
                                questBannerTimer = 5.0f;
                                audio_play(SFX_PERK);
                                dialogNpc = -1;
                            }
                            else if (strcmp(target, "as5:teslim") == 0)
                            {
                                // SINIF AÇILIR: Sessiz El seni imzaladı
                                player.storyFlags[SF_ASSASSIN] = 13;
                                bag_add(&ITEM_TYPES[ITEM_DAGGER_UNSIGNED], 1, 0);
                                bag_add(&ITEM_TYPES[ITEM_POISON_NEEDLE], 1, 0);
                                bag_add(&ITEM_TYPES[ITEM_NEEDLE_VENOM], 3, 0);
                                int r4 = recipe_index_by_id("alch_igne_zehri");
                                if (r4 >= 0) craft_learn_recipe(r4);
                                player.gold += 300;
                                tree_compute_effects(player.treeTaken,
                                                     player.fateTaken, &perkFx);
                                spells_apply_learned(&player, &perkFx);
                                echo_apply_unlocks(&player, &perkFx);
                                hotbar_place_unlocks(&hotbar, perkFx.abilityOpen);
                                audio_play(SFX_PERK);
                                questBannerTitle = "SUİKASTÇİ — SESSİZ EL'İN İMZASI";
                                questBannerSub =
                                    "İmzasız Bıçak + Zehirli İğne senin; 5 beceri açıldı (K defteri)";
                                questBannerTimer = 6.0f;
                                dialogNpc = -1;
                            }
                            else if (strcmp(target, "sifa:ode") == 0)
                            {
                                // REVİR ödemesi: TÜM yatanların masrafı tek seferde
                                int total = 0, cnt = 0;
                                for (int ni2 = 0; ni2 < NPC_COUNT; ni2++)
                                    if (npc_infirm(ni2))
                                    { total += infirmary_cost(ni2, gametime_day()); cnt++; }
                                SDL_Color hgold3 = { 255, 235, 150, 255 };
                                SDL_Color red3 = { 255, 120, 100, 255 };
                                char obuf[FLOAT_TEXT_LEN];
                                if (cnt == 0)
                                    npc_bark(sifaciIdx, "Yataklar boş. Böyle kalsın.", 4.0f);
                                else if (player.gold < total)
                                {
                                    snprintf(obuf, sizeof(obuf),
                                             "Altının yetmiyor (%d gerek)", total);
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                        player.rect.x + player.rect.w / 2.0f,
                                        player.rect.y - 10.0f, obuf, red3);
                                }
                                else
                                {
                                    player.gold -= total;
                                    for (int ni2 = 0; ni2 < NPC_COUNT; ni2++)
                                        if (npc_infirm(ni2))
                                            npc_infirmary_discharge(ni2, false);
                                    snprintf(obuf, sizeof(obuf),
                                             "-%d g — yaralılar ayağa kalktı", total);
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                        player.rect.x + player.rect.w / 2.0f,
                                        player.rect.y - 10.0f, obuf, hgold3);
                                    audio_play(SFX_PERK);
                                    npc_bark(sifaciIdx,
                                             "Parayla vicdan alınmaz ama yatak boşalır.", 5.0f);
                                }
                                dialogNpc = -1;
                            }
                            else if (strcmp(target, "sifa:tedavi") == 0)
                            {
                                // Şifacı tedavisi: 60g — tam can (oyuncu hizmeti)
                                SDL_Color red4 = { 255, 120, 100, 255 };
                                if (player.gold < 60)
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                        player.rect.x + player.rect.w / 2.0f,
                                        player.rect.y - 10.0f,
                                        "Altının yetmiyor (60 gerek)", red4);
                                else if (player.health >= player.maxHealth)
                                    npc_bark(sifaciIdx, "Sapasağlamsın. Git işine.", 4.0f);
                                else
                                {
                                    player.gold -= 60;
                                    player.health = player.maxHealth;
                                    audio_play(SFX_POTION);
                                    if (g_animFx != NULL)
                                        animfx_spawn(g_animFx, g_hitMagicTex, 49, 7,
                                                     100.0f, 100.0f,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y + player.rect.h / 2.0f,
                                                     70.0f, 0.8f);
                                    SDL_Color grn2 = { 150, 255, 150, 255 };
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                        player.rect.x + player.rect.w / 2.0f,
                                        player.rect.y - 10.0f,
                                        "-60 g — yaraların sarıldı", grn2);
                                }
                                dialogNpc = -1;
                            }
                            else if (strcmp(target, "akla:kervan") == 0)
                            {
                                // AKLAMA (kervan): kirli altın ×0.75 (+ağaç) temize;
                                // çalıntı eşya satılır — obje TAM, diğerleri ×0.6
                                float rate = 0.75f + perkFx.pickFenceBonus
                                           + (float)assassin_rank(player.assassinPoints) * 0.02f;
                                if (rate > 0.90f) rate = 0.90f;   // rütbe: loca kanalını temizler
                                int clean = (int)(player.dirtyGold * rate);
                                // Loca eşiği VERİLEN kirli değerle artar (kesinti
                                // öncesi — "hepsi sayılıyor"; Hakan kararı 2026-07-24)
                                int dirtyIn = player.dirtyGold;
                                int sold = 0;
                                for (int b2 = 0; b2 < 2; b2++)
                                {
                                    Inventory *bb = (b2 == 0) ? &inventory : &craftBag;
                                    for (int sl2 = 0; sl2 < INVENTORY_SIZE; sl2++)
                                    {
                                        ItemStack *ss = &bb->slots[sl2];
                                        if (ss->type == NULL || !ss->stolen) continue;
                                        bool trinket =
                                            ss->type == &ITEM_TYPES[ITEM_TRINKET_RING] ||
                                            ss->type == &ITEM_TYPES[ITEM_TRINKET_BROOCH] ||
                                            ss->type == &ITEM_TYPES[ITEM_TRINKET_WATCH];
                                        dirtyIn += item_value(ss) * ss->count;
                                        sold += (int)(item_value(ss) * ss->count
                                                      * (trinket ? 1.0f : 0.6f));
                                        *ss = item_make(NULL, 0, 0);
                                    }
                                }
                                player.launderedTotal += dirtyIn;
                                if (clean + sold <= 0 && player.dirtyGold <= 0)
                                {
                                    npc_bark(kervanIdx, "Kesen temizmiş, yolcu.", 4.0f);
                                }
                                else
                                {
                                    player.gold += clean + sold;
                                    player.dirtyGold = 0;
                                    audio_play(SFX_LOOT);
                                    char abuf[FLOAT_TEXT_LEN];
                                    snprintf(abuf, sizeof(abuf),
                                             "+%d g (aklandı)", clean + sold);
                                    SDL_Color vio = { 200, 160, 255, 255 };
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                         player.rect.x + player.rect.w / 2.0f,
                                                         player.rect.y - 10.0f, abuf, vio);
                                    npc_bark(kervanIdx, "Bende kalan bende kalır.", 4.0f);
                                }
                                dialogNpc = -1;
                            }
                            else if (strcmp(target, "k9:tomar") == 0)
                            {
                                // FATMA'NIN HEDİYESİ (Faz HOLY): tapınak kararının
                                // geç ödülü — Şifa Nuru tomarı, BİR KEZ. Çanta
                                // doluysa bayrak yanmaz (sonra tekrar dener).
                                ItemStack ts2 = item_make(&ITEM_TYPES[ITEM_TOME_HEAL], 1, 0);
                                if (bag_add_stackp(&ts2))
                                {
                                    player.storyFlags[SF_K9_TOME] = 1;
                                    audio_play(SFX_PERK);
                                    npc_bark(fatmaIdx,
                                             "Kerem'in nöbetine ışık gerek. Al.", 5.0f);
                                    SDL_Color hgold2 = { 255, 235, 150, 255 };
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                        player.rect.x + player.rect.w / 2.0f,
                                        player.rect.y - 10.0f,
                                        "Tomar: Şifa Nuru alındı", hgold2);
                                }
                                else
                                    npc_bark(fatmaIdx, "Önce çantanda yer aç, evlat.", 4.0f);
                                dialogNpc = -1;
                            }
                            else if (strncmp(target, "k9:", 3) == 0)
                            {
                                // K9 Kandil kararı (D1a): bayrak + günlük + sahne
                                int k9c = SDL_atoi(target + 3);
                                player.storyFlags[SF_K9_KANDIL] = (Uint8)k9c;
                                journal_add(JE_K9_CHOICE, k9c);
                                dialogNpc = -1;
                                if (k9c == 1)
                                {
                                    // Tapınak: kamera kandile süzülür, ateş büyür
                                    cineActive = true;
                                    cineX = KANDIL_X;
                                    cineY = KANDIL_Y;
                                    cineTimer = 3.0f;
                                    audio_play(SFX_PERK);
                                    npc_bark(fatmaIdx, "Nöbet artık bende.", 4.0f);
                                }
                                else if (k9c == 2)
                                {
                                    npc_bark(fatmaIdx, "Artık eve gidebilirim.", 4.0f);
                                    audio_play(SFX_UI);
                                }
                                else
                                    npc_bark(fatmaIdx, "Kandil bekler. Ben de.", 4.0f);
                            }
                            else if (strncmp(target, "k2:", 3) == 0)
                            {
                                // K2 Son Müşteri kararı (D1b)
                                int k2c = SDL_atoi(target + 3);
                                player.storyFlags[SF_K2_NAZIFE] = (Uint8)k2c;
                                journal_add(JE_K2_CHOICE, k2c);
                                dialogNpc = -1;
                                if (k2c == 1)
                                {
                                    // Son sofra vinyeti (D1b-ek): "ikimize" gecesi —
                                    // bark sahne bitince görünür (dünya donuk)
                                    scene_start("assets/scenes/k2_sofra.txt");
                                    npc_bark(nazifeIdx, "Sütü hep unuturdu...", 4.0f);
                                }
                                else if (k2c == 2)
                                    npc_bark(nazifeIdx, "Beklemek bitti.", 4.0f);
                                else
                                    npc_bark(nazifeIdx, "Sofra bekler.", 4.0f);
                                audio_play(SFX_UI);
                            }
                            else if (strncmp(target, "nuh:", 4) == 0)
                            {
                                // S3 bulmaca akışı (D1c)
                                const char *nt = target + 4;
                                if (strcmp(nt, "t1") == 0)
                                    player.storyFlags[SF_S3_TRUST] = 1;
                                else if (strcmp(nt, "t2") == 0)
                                    player.storyFlags[SF_S3_TRUST] = 2;
                                else if (strcmp(nt, "t3") == 0)
                                {
                                    player.storyFlags[SF_S3_TRUST] = 3;
                                    player.storyFlags[SF_S3_STATE] = 1;
                                    journal_add(JE_S3_STEP, 1);
                                    // Nuh konağa doğru "sayarak" yürür (sahne aracı)
                                    npc_walk_to(nuhIdx, 166.0f * TILE_SIZE,
                                                46.0f * TILE_SIZE);
                                    npc_bark(nuhIdx, "...bir, iki, üç...", 6.0f);
                                    float nwx, nwy;   // kamera onu kısaca izler
                                    if (npc_visible_pos(nuhIdx, &nwx, &nwy))
                                    {
                                        cineActive = true;
                                        cineX = nwx + 80.0f;
                                        cineY = nwy;
                                        cineTimer = 2.5f;
                                    }
                                }
                                else if (strcmp(nt, "kilit") == 0)
                                    nuhLockDay = gametime_day();
                                dialogNpc = -1;
                                audio_play(SFX_UI);
                            }
                            else if (strncmp(target, "zehra:", 6) == 0)
                            {
                                player.storyFlags[SF_S3_HINTS] = 1;
                                journal_add(JE_S3_STEP, 0);
                                dialogNpc = -1;
                                audio_play(SFX_UI);
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
                                        journal_add(JE_QUEST_ACCEPT, q);
                                        char buf[FLOAT_TEXT_LEN];
                                        snprintf(buf, sizeof(buf), "Üstlendim: %s", QUESTS[q].name);
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
                                    ready = bag_consume_n(
                                                &ITEM_TYPES[QUESTS[q].fetchItem],
                                                QUESTS[q].fetchCount);
                                }
                                if (ready)
                                {
                                    questStatus[q].state = QUEST_REWARDED;
                                    player.gold += QUESTS[q].rewardGold;

                                    // Ek item ödülü (örn. Demirci → Çekiç)
                                    if (QUESTS[q].rewardItem > 0 &&
                                        bag_add(
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

                                    journal_add(JE_QUEST_DONE, q);
                                    SDL_Color pers = { 235, 225, 200, 255 };
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                         player.rect.x + player.rect.w / 2.0f,
                                                         player.rect.y - 26.0f,
                                                         "Sözümü tuttum.", pers);
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
                                if (bag_add(
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
                    else if (event.key.key >= SDLK_1 && event.key.key <= SDLK_8)
                    {
                        // WoW düzeni: düz sayı = alt ana bar, Shift+sayı = üst
                        // ana bar, Alt/Option+sayı = sağ dikey bar. Sol dikey
                        // tuşsuz (tıklamayla kullanılır).
                        int hbRow = 0;
                        SDL_Keymod hbMod = SDL_GetModState();
                        if (hbMod & SDL_KMOD_SHIFT) hbRow = 1;
                        else if (hbMod & SDL_KMOD_ALT) hbRow = 2;
                        hotbarPressed = hbRow * HOTBAR_COLS
                                      + (int)(event.key.key - SDLK_1);
                    }
                }
                else if (event.key.key == SDLK_E && gameState == GAME_PLAYING &&
                         dialogNpc >= 0)
                {
                    // Diyalog açıkken E: akan metni anında tamamla
                    dialog_type_reveal();
                }
                else if (event.key.key == SDLK_E && gameState == GAME_PLAYING &&
                         !scene_active() && dialogNpc < 0 &&
                         map_current() == MAP_WORLD &&
                         player.storyFlags[SF_WELL] >= 1 &&
                         SDL_fabsf(player.rect.x + player.rect.w / 2.0f
                                   - (26.0f * TILE_SIZE + 32.0f)) < 110.0f &&
                         SDL_fabsf(player.rect.y + player.rect.h / 2.0f
                                   - (15.0f * TILE_SIZE + 48.0f)) < 110.0f)
                {
                    // KÖR KUYU: gece bırak / şafak sonrası kuyu başından TOPLA.
                    // wellDay = HAZIR olduğu gün (gece yarısı sonrası bırakma da
                    // aynı şafağa yetişir — bekleme cezası yok).
                    // Bekleyen kese YENİ bırakmayı ENGELLEMEZ — üstüne eklenir
                    // (Hakan raporu 2026-07-24: 2. parça sessizce yutuluyordu)
                    int wday; float whrs;
                    gametime_get(&wday, &whrs);
                    bool pouchReady = player.wellGold > 0 &&
                                      ((wday > player.wellDay) ||
                                       (wday == player.wellDay && whrs >= 5.0f));
                    if (pouchReady)
                    {
                        // TOPLAMA önce: kese hazırsa al (bırakma sonraki E'de)
                        player.gold += player.wellGold;
                        char wbuf[FLOAT_TEXT_LEN];
                        snprintf(wbuf, sizeof(wbuf),
                                 "Kuyu başında temiz bir kese: +%d g",
                                 player.wellGold);
                        SDL_Color vio2 = { 200, 160, 255, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 24.0f, wbuf, vio2);
                        player.wellGold = 0;
                        audio_play(SFX_LOOT);
                    }
                    else if (!gametime_is_night())
                    {
                        SDL_Color dim2 = { 190, 190, 200, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f,
                                             player.wellGold > 0
                                             ? "Henüz sayılıyor... şafağı bekle."
                                             : "Kuyu gündüz sağır. Gece gel.", dim2);
                    }
                    else
                    {
                    float rate = 0.50f + perkFx.pickFenceBonus
                              + (float)assassin_rank(player.assassinPoints) * 0.03f;
                    if (rate > 0.72f) rate = 0.72f;   // rütbe: kuyu kanalı iyileşir
                    int total = player.dirtyGold;
                    for (int b2 = 0; b2 < 2; b2++)
                    {
                        Inventory *bb = (b2 == 0) ? &inventory : &craftBag;
                        for (int sl2 = 0; sl2 < INVENTORY_SIZE; sl2++)
                        {
                            ItemStack *ss = &bb->slots[sl2];
                            if (ss->type == NULL || !ss->stolen) continue;
                            total += item_value(ss) * ss->count;
                            *ss = item_make(NULL, 0, 0);
                        }
                    }
                    if (total <= 0)
                    {
                        SDL_Color dim = { 190, 190, 200, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f,
                                             player.wellGold > 0
                                             ? "Henüz sayılıyor... şafağı bekle."
                                             : "Kuyu boş bakışını yansıtıyor.", dim);
                    }
                    else
                    {
                        // Loca eşiği BIRAKILAN kirli değerle artar (kesinti öncesi —
                        // "Hepsi sayılıyor"; Hakan kararı 2026-07-24)
                        player.launderedTotal += total;
                        player.wellGold += (int)(total * rate);
                        // Hazır günü: akşam bırakıldıysa ertesi gün şafak;
                        // gece yarısı SONRASI bırakıldıysa 05:00'ten önceyse bugünkü
                        // şafağa yetişir. Bekleyen kese daha geç güne bakıyorsa
                        // birleşik kese ONUN gününü bekler (erken sızma yok).
                        int readyDay = (whrs >= 5.0f) ? wday + 1 : wday;
                        if (readyDay < player.wellDay) readyDay = player.wellDay;
                        player.wellDay = readyDay;
                        player.dirtyGold = 0;
                        audio_play(SFX_WHISPER);
                        SDL_Color vio = { 200, 160, 255, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 24.0f,
                                             "Kuyunun dibinden: \"Sayılıyor. Hepsi sayılıyor.\"",
                                             vio);
                    }
                    }
                }
                else if (event.key.key == SDLK_E && gameState == GAME_PLAYING &&
                         !scene_active() && dialogNpc < 0 &&
                         map_current() == MAP_WORLD && gametime_is_night() &&
                         player.storyFlags[SF_YANKICI] == 1 &&
                         SDL_fabsf(player.rect.x + player.rect.w / 2.0f
                                   - 108.0f * TILE_SIZE) < 100.0f &&
                         SDL_fabsf(player.rect.y + player.rect.h / 2.0f
                                   - 7.0f * TILE_SIZE) < 100.0f)
                {
                    // RİTÜEL: taşların ortasında, ay görmeden (kabul/reddet vinyeti)
                    scene_start("assets/scenes/ritual.txt");
                }
                else if (event.key.key == SDLK_E && gameState == GAME_PLAYING &&
                         dialogNpc < 0 && nearestEcho >= 0)
                {
                    // Yankıya dokun: dünya bir nefeslik durur, sahne süzülür
                    scene_set_speech(player.skills[SKILL_SPEECH].level);
                    if (scene_start(ECHOES[nearestEcho].scenePath))
                    {
                        activeEcho = nearestEcho;
                        player.autoAttack = false;
                        player.attacking = false;
                        // Yazıt: ilk okuma günlüğe düşer (ANI seçimde işlenir)
                        if (ECHOES[nearestEcho].kind == ECHO_YAZIT)
                            journal_add_unique(JE_ECHO_YAZIT, nearestEcho);
                    }
                }
                else if (event.key.key == SDLK_E && gameState == GAME_PLAYING &&
                         map_current() == MAP_KONAK && dialogNpc < 0 &&
                         SDL_fabsf(player.rect.x + player.rect.w / 2.0f - KAYIT_X) < 70.0f &&
                         SDL_fabsf(player.rect.y + player.rect.h / 2.0f - KAYIT_Y) < 70.0f)
                {
                    // S3 kayıt odası: yalnız Nuh'la (STATE 1) açılır
                    Uint8 st3 = player.storyFlags[SF_S3_STATE];
                    if (st3 == 1)
                    {
                        scene_set_speech(player.skills[SKILL_SPEECH].level);
                        scene_start("assets/scenes/kayit_odasi.txt");
                    }
                    else if (st3 >= 2)
                    {
                        SDL_Color dim3 = { 170, 170, 190, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f,
                                             "Oda sessiz.", dim3);
                    }
                    else
                    {
                        SDL_Color red3 = { 255, 140, 100, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f,
                                             "Kilitli — Eşikçi mührü. Nuh'suz açılmaz.", red3);
                    }
                }
                else if (event.key.key == SDLK_E && gameState == GAME_PLAYING &&
                         dialogNpc < 0 && nearestRitual >= 0)
                {
                    // Ritüel: gölcük kurutma (meşale ister) / mezar mühürleme
                    const NestSiteDef *rs = ecology_site(nearestRitual);
                    bool isPool = rs->kind == NEST_POOL;
                    if (isPool && bag_count(&ITEM_TYPES[ITEM_TORCH]) <= 0)
                    {
                        SDL_Color red = { 255, 140, 100, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f,
                                             "Meşale gerekiyor", red);
                    }
                    else
                    {
                        ecology_do_ritual(nearestRitual, gametime_day());
                        journal_add(JE_NEST_CLEANSE, isPool ? 2 : 3);
                        title_notify(title_stat_add(&player, TS_NESTS, 1),
                                     &player, floatTexts);
                        audio_play(SFX_PERK);
                        fx_burst(&weaponFx, rs->x, rs->y, 22, 110.0f,
                                 isPool ? 255 : 140, isPool ? 170 : 220,
                                 isPool ? 60 : 235);
                        SDL_Color icy = { 150, 220, 255, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f,
                                             isPool ? "Gölcük kurudu."
                                                    : "Mezar mühürlendi.", icy);
                        nearestRitual = -1;
                    }
                }
                else if (event.key.key == SDLK_E && gameState == GAME_PLAYING &&
                         !scene_active() && dialogNpc < 0 &&
                         map_current() == MAP_WORLD &&
                         player.storyFlags[SF_ASSASSIN] == 2 &&
                         gametime_is_night() &&
                         SDL_fabsf(player.rect.x + player.rect.w / 2.0f
                                   - 10.2f * TILE_SIZE) < 80.0f &&
                         SDL_fabsf(player.rect.y + player.rect.h / 2.0f
                                   - 5.0f * TILE_SIZE) < 80.0f)
                {
                    // SINAV (Suikastçi P1): Mühürlü Kutu — TANIKSIZ alınmalı
                    float bx3 = 10.2f * TILE_SIZE, by3 = 5.0f * TILE_SIZE;
                    // victimAlive=FALSE: kurban yok — yalnız GERÇEK tanıklar
                    // sayılır (true geçmek "hep görüldü" bug'ıydı)
                    if (crime_witnessed(bx3, by3, -1, false,
                                        player.sneaking ? SNEAK_WITNESS_RANGE
                                                        : CRIME_WITNESS_RANGE))
                    {
                        int cr2 = region_of(bx3, by3);
                        player.bounty[cr2] += CRIME_BOUNTY_HIT;
                        player.lastCrimeDay = gametime_day();
                        rep_add(&player.reputation[cr2], REP_HIT);
                        SDL_Color red5 = { 255, 80, 80, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                            player.rect.x + player.rect.w / 2.0f,
                            player.rect.y - 24.0f,
                            "Görüldün! Usta bunu duymamalı...", red5);
                    }
                    else
                    {
                        ItemStack bx4 = item_make(&ITEM_TYPES[ITEM_SEALED_BOX], 1, 0);
                        if (bag_add_stackp(&bx4))
                        {
                            player.storyFlags[SF_ASSASSIN] = 3;
                            audio_play(SFX_UI);
                            questBannerTitle = "GÖREV GÜNCELLENDİ";
                            questBannerSub = "Mühürlü Kutu'yu Neriman'a götür";
                            questBannerTimer = 4.0f;
                            SDL_Color vio7 = { 200, 160, 255, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                player.rect.x + player.rect.w / 2.0f,
                                player.rect.y - 24.0f,
                                "Kutu sende. Kimse görmedi.", vio7);
                        }
                    }
                }
                else if (event.key.key == SDLK_E && gameState == GAME_PLAYING &&
                         !scene_active() && dialogNpc < 0 && nearestNpc < 0 &&
                         map_current() == MAP_WORLD &&
                         player.storyFlags[SF_ASSASSIN] == 4 && kemalIdx >= 0)
                {
                    // TAKİP raporu: 100-400px bandında işaretle (durak başına bir)
                    float knx2, kny2;
                    if (npc_visible_pos(kemalIdx, &knx2, &kny2))
                    {
                        float kdx2 = (player.rect.x + player.rect.w / 2.0f) - knx2;
                        float kdy2 = (player.rect.y + player.rect.h / 2.0f) - kny2;
                        float kd2 = SDL_sqrtf(kdx2 * kdx2 + kdy2 * kdy2);
                        int kday; float khrs;
                        gametime_get(&kday, &khrs);
                        int stopIdx = npc_current_stop_index(kemalIdx, khrs);
                        if (kd2 >= 100.0f && kd2 <= 400.0f && stopIdx >= 0)
                        {
                            Uint8 marks = player.storyFlags[SF_TAIL_MARK];
                            if (marks & (1u << stopIdx))
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                    player.rect.x + player.rect.w / 2.0f,
                                    player.rect.y - 24.0f,
                                    "Bu durağı zaten işaretledin.",
                                    (SDL_Color){ 170, 170, 190, 255 });
                            else
                            {
                                marks |= (Uint8)(1u << stopIdx);
                                player.storyFlags[SF_TAIL_MARK] = marks;
                                int cnt2 = 0;
                                for (int b2 = 0; b2 < 4; b2++)
                                    if (marks & (1u << b2)) cnt2++;
                                audio_play(SFX_UI);
                                char rbuf[FLOAT_TEXT_LEN];
                                if (cnt2 >= 3)
                                {
                                    player.storyFlags[SF_ASSASSIN] = 5;
                                    snprintf(rbuf, sizeof(rbuf),
                                             "Rapor tamam — locaya dön.");
                                    questBannerTitle = "GÖREV GÜNCELLENDİ";
                                    questBannerSub = "Neriman'a raporu götür";
                                    questBannerTimer = 4.0f;
                                }
                                else
                                    snprintf(rbuf, sizeof(rbuf),
                                             "Rapor işaretlendi (%d/3)", cnt2);
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                    player.rect.x + player.rect.w / 2.0f,
                                    player.rect.y - 24.0f, rbuf,
                                    (SDL_Color){ 200, 160, 255, 255 });
                            }
                        }
                    }
                }
                else if (event.key.key == SDLK_E && gameState == GAME_PLAYING &&
                         dialogNpc < 0 && nearPortal >= 0 &&
                         PORTALS[nearPortal].to != MAP_LOCA)
                {
                    // Geçit kullan (kapı/merdiven) — asıl geçiş update başında
                    // yapılır. Locaya İNİŞ hariç: kuyuda E aklamanın, R ipin
                    // (etiketler ayrışır — E çakışması fix'i)
                    pendingPortal = nearPortal;
                    audio_play(SFX_UI);
                }
                else if (event.key.key == SDLK_E && gameState == GAME_PLAYING &&
                         dialogNpc < 0 && nearestChest >= 0 && !lockpick_active())
                {
                    // Sandık aç (tıklamayla aynı akış — E oyunun genel etkileşim dili)
                    int li = nearestChest;
                    if (loots[li].lockTier >= 0)
                    {
                        if (bag_count(&ITEM_TYPES[ITEM_LOCKPICK]) <= 0)
                        {
                            SDL_Color red = { 255, 120, 100, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f,
                                                 "Maymuncuk yok!", red);
                        }
                        else
                        {
                            lockpick_start(loots[li].lockTier, li, &perkFx);
                            player.autoAttack = false;
                            audio_play(SFX_UI);
                        }
                    }
                    else
                    {
                        loots[li].state = LOOT_OPENING;
                        loots[li].currentFrame = 0;
                        loots[li].animTimer = 0.0f;
                        openLootIndex = li;
                        if (loots[li].chestId >= 0)
                            chest_set_opened_day((int)loots[li].chestId, gametime_day());
                    }
                }
                else if (event.key.key == SDLK_E && gameState == GAME_PLAYING &&
                         dialogNpc < 0 && map_current() != MAP_WORLD &&
                         map_current() != MAP_LOCA && map_current() != MAP_DOGU)
                {
                    // Zindanda E başka işe bakmaz (NPC/istasyon/pano dünyada;
                    // LOCA hariç — orada NPC'lerle konuşulur)
                }
                else if (event.key.key == SDLK_E && gameState == GAME_PLAYING &&
                         dialogNpc < 0)
                {
                    // NPC'nin bölgesinde bounty ya da dip itibar → kapılar kapalı
                    float npx = 0.0f, npy = 0.0f;
                    int npcRegion = 0;
                    if (nearestNpc >= 0 && npc_visible_pos(nearestNpc, &npx, &npy))
                        npcRegion = region_of(npx, npy);

                    // LOCA kanun tanımaz: bounty/itibar kapısı orada işlemez
                    // (kanunsuzun kapısını kanun kilitleyemez)
                    bool lawless = (map_current() == MAP_LOCA);
                    if (nearestNpc >= 0 && !lawless && player.bounty[npcRegion] > 0)
                    {
                        SDL_Color red = { 255, 120, 100, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             npx, npy - 100.0f,
                                             "Önce cezanı öde!", red);
                    }
                    else if (nearestNpc >= 0 && !lawless &&
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
                        // Köye varış karşılaması (Faz B1): muhafızla ilk konuşma
                        if (nearestNpc == guardIdx && !player.guardGreeted)
                        {
                            entry = "karsilama";
                            player.guardGreeted = true;   // bir kez yaşanır
                        }
                        else if (nearestNpc == nerimanIdx)
                        {
                            // Sessiz El zinciri: aşamaya göre düğüm (ESO akışı)
                            Uint8 as2 = player.storyFlags[SF_ASSASSIN];
                            if (as2 <= 1)      entry = "tanisma";
                            else if (as2 == 2) entry = "sinav_bekle";
                            else if (as2 == 3) entry = "kutu";
                            else if (as2 == 4) entry = "takip_bekle";
                            else if (as2 == 5) entry = "rapor";
                            else if (as2 == 6) entry = "gorev3";
                            else if (as2 == 7) entry = "sise_bekle";
                            else if (as2 == 8) entry = "gorev4";
                            else if (as2 == 10) entry = "gorev5";
                            else if (as2 == 11) entry = "imza_bekle";
                            else if (as2 == 12) entry = "imza_rapor";
                            else               entry = "usta";
                        }
                        else if (nearestNpc >= 0 &&
                                 SDL_strcmp(NPCS[nearestNpc].id, "loca_hucre_zehirci") == 0)
                        {
                            if (player.storyFlags[SF_ASSASSIN] == 7 &&
                                bag_count(&ITEM_TYPES[ITEM_QUIET_VIAL]) > 0)
                                entry = "teslim";
                            else if (player.storyFlags[SF_ASSASSIN] == 7)
                                entry = "bekliyor";
                            else if (player.storyFlags[SF_ASSASSIN] >= 13)
                                entry = "usta";
                        }
                        else if (nearestNpc == sifaciIdx)
                        {
                            // Şifacı: yatan varsa doğrudan hasta düğümü
                            bool anyInf = false;
                            for (int ni4 = 0; ni4 < NPC_COUNT; ni4++)
                                if (npc_infirm(ni4)) { anyInf = true; break; }
                            if (anyInf) entry = "yatan";
                        }
                        else if (nearestNpc == fatmaIdx)
                        {
                            // K9 Kandil: yankı görüldüyse karar diyaloğu; karar
                            // verildiyse kalıcı kapanış düğümü (flag'e göre)
                            Uint8 k9 = player.storyFlags[SF_K9_KANDIL];
                            if (k9 == 1)      entry = (player.storyFlags[SF_K9_TOME]
                                                       ? "son1" : "son1_tomar");
                            else if (k9 == 2) entry = "son2";
                            else if (k9 == 3) entry = "son3";
                            else if (echo_done(ECHO_K9)) entry = "bilgi";
                        }
                        else if (nearestNpc == nazifeIdx)
                        {
                            // K2 Son Müşteri: araba yankısının HATTI belirleyici —
                            // Tanık kanıt taşır, Sömüren taşıyamaz (ilk somut bedel)
                            Uint8 k2 = player.storyFlags[SF_K2_NAZIFE];
                            Uint8 cart = player.storyFlags[SF_K2_CART];
                            if (k2 == 1)      entry = "son1";
                            else if (k2 == 2) entry = "son2";
                            else if (k2 == 4) entry = "bos_tekrar";
                            else if (cart == 1) entry = "bilgi";
                            else if (cart == 2) entry = "bilgi_bos";
                        }
                        else if (nearestNpc == nuhIdx)
                        {
                            // S3: güvene/duruma göre tur — yanlış cevap gün kilidi
                            Uint8 st3 = player.storyFlags[SF_S3_STATE];
                            Uint8 tr3 = player.storyFlags[SF_S3_TRUST];
                            if (st3 == 2)      entry = "devraldin";
                            else if (st3 == 3) entry = "kapandi";
                            else if (st3 == 1) entry = "yolda";
                            else if (nuhLockDay == gametime_day()) entry = "kilit";
                            else if (tr3 == 1) entry = "tur2";
                            else if (tr3 >= 2) entry = "tur3";
                            // tr3 == 0 → start (tur1'e oradan akar)
                        }
                        else if (nearestNpc == zehraIdx)
                        {
                            if (player.storyFlags[SF_S3_HINTS] > 0)
                                entry = "tekrar";
                        }
                        else if (NPCS[nearestNpc].questId != NULL)
                        {
                            int q = quest_find(NPCS[nearestNpc].questId);
                            if (q >= 0)
                            {
                                // FETCH: malzeme tamamsa ACTIVE'de de "done" girişi
                                bool fetchReady = questStatus[q].state == QUEST_ACTIVE &&
                                    QUESTS[q].fetchCount > 0 &&
                                    bag_count(&ITEM_TYPES[QUESTS[q].fetchItem])
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
                            dialog_type_reset();         // metin akarak yazılsın
                            player.autoAttack = false;   // konuşurken savaş durur
                            gatherRes = -1;              // toplama da durur
                            craftOpen = false;           // craft paneli de kapanır
                            craftRecipe = -1;
                            plusForge = false;
                            enchAction = 0;
                            craftProgress = 0.0f;
                        }
                    }
                    // SESSİZ EL kontrat tahtası (P3): locada, sınıf açıldıysa
                    else if (map_current() == MAP_LOCA &&
                             player.storyFlags[SF_ASSASSIN] >= 13 && !locaBoardOpen &&
                             (LOCABOARD_X - (player.rect.x + player.rect.w / 2.0f)) *
                             (LOCABOARD_X - (player.rect.x + player.rect.w / 2.0f)) +
                             (LOCABOARD_Y - (player.rect.y + player.rect.h / 2.0f)) *
                             (LOCABOARD_Y - (player.rect.y + player.rect.h / 2.0f))
                                 <= LOCABOARD_RANGE * LOCABOARD_RANGE)
                    {
                        locaBoardOpen = true;
                        player.autoAttack = false;
                        gatherRes = -1;
                        audio_play(SFX_UI);
                    }
                    // İlan panosu (Faz 6): bounty/dip itibar panoya da kapı
                    else if (!boardOpen && map_current() == MAP_WORLD &&
                             SDL_sqrtf((BOARD_X - (player.rect.x + player.rect.w / 2.0f)) *
                                       (BOARD_X - (player.rect.x + player.rect.w / 2.0f)) +
                                       (BOARD_Y - (player.rect.y + player.rect.h / 2.0f)) *
                                       (BOARD_Y - (player.rect.y + player.rect.h / 2.0f)))
                                 <= BOARD_RANGE)
                    {
                        // KÖY İLAN PANOSU: yalnız DÜNYADA (locada koordinatı
                        // çakışıyordu — köy/loca tahtası karışıyordu, Hakan raporu)
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
                    // KANDİL DİREĞİ (LORE §1.1): sönük direğe kor koy.
                    // Bir kor = üç gece. Yanan halkada ölü kalkmaz (§1.4) —
                    // ama halkayı taşıyan da iki mil öteden görünür (§1.5).
                    else if (nearestLamp >= 0)
                    {
                        SDL_Color warm = { 255, 200, 120, 255 };
                        SDL_Color cold = { 255, 120, 100, 255 };
                        char buf[FLOAT_TEXT_LEN];
                        float ftx = player.rect.x + player.rect.w / 2.0f;
                        float fty = player.rect.y - 10.0f;

                        if (lamp_lit(nearestLamp))
                        {
                            snprintf(buf, sizeof(buf), "%s yaniyor - %d gece",
                                     LAMPS[nearestLamp].place, lamp_nights(nearestLamp));
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 ftx, fty, buf, warm);
                        }
                        else if (bag_count(&ITEM_TYPES[ITEM_ORE_COPPER]) <= 0)
                        {
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 ftx, fty, "Hazne bos - kor gerek", cold);
                        }
                        else
                        {
                            bool first = (lamp_owner(nearestLamp) == LAMP_OWNER_NONE);
                            bag_consume(&ITEM_TYPES[ITEM_ORE_COPPER]);
                            lamp_add_kor(nearestLamp, 1);
                            lamp_set_owner(nearestLamp, LAMP_OWNER_PLAYER);
                            if (first)
                                snprintf(buf, sizeof(buf), "%s SENIN adina yandi",
                                         LAMPS[nearestLamp].place);
                            else
                                snprintf(buf, sizeof(buf), "%s beslendi - %d gece",
                                         LAMPS[nearestLamp].place,
                                         lamp_nights(nearestLamp));
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 ftx, fty, buf, warm);
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
                            // Ad NPC tablosundan: istasyon tablosunda ad KOPYASI
                            // tutmuyoruz, yoksa NPC yeniden adlandırılınca eskir
                            char buf[FLOAT_TEXT_LEN];
                            int oi = npc_index_by_id(sd->ownerNpcId);
                            snprintf(buf, sizeof(buf), "Önce %s ile konuş",
                                     (oi >= 0) ? NPCS[oi].name : "sahibi");
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
                            craftScroll = 0;   // panel tepeden açılır
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
                else if (event.key.key == SDLK_I && gameState == GAME_PLAYING)
                {
                    // I: aç/kapa — sekme TAB ile değişir (açılış hep Eşya)
                    inventoryOpen = !inventoryOpen;
                    if (inventoryOpen) invTab = 0;
                    trashTab = -1; trashSlot = -1;   // bekleyen çöp onayı düşer
                    audio_play(SFX_UI);
                }
                else if (event.key.key == SDLK_TAB && gameState == GAME_PLAYING &&
                         inventoryOpen)
                {
                    // TAB: Eşya ↔ Malzeme (günlük sekmesinden ÖNCE — çanta öncelikli)
                    invTab = 1 - invTab;
                    audio_play(SFX_UI);
                }
                else if (event.key.key == SDLK_TAB && gameState == GAME_PLAYING &&
                         !questLogOpen && !scene_active() && dialogNpc < 0)
                {
                    // TAB: mob hedefle (modern hedefleme — Z'nin yerini aldı)
                    targetPressed = true;
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
                    if (questLogOpen) { journalTab = 0; journalPage = 0; }
                    audio_play(SFX_UI);
                }
                else if (event.key.key == SDLK_P && gameState == GAME_PLAYING &&
                         !scene_active() && dialogNpc < 0 && !shopOpen &&
                         (map_current() == MAP_WORLD ||
                          map_current() == MAP_DOGU) &&
                         nearestNpc >= 0 && nearestNpc < PICK_TRIES_CAP)
                {
                    // YANKESİCİLİK: eğilerek yaklaş + P. Zar: taban %35 +
                    // Pickpocket lvl×0.5 + eğilme +25 + meşgul +15, muhafız -30,
                    // her deneme (o gün, o NPC) -15. Tavan %90 — garanti yok.
                    int pv = nearestNpc;
                    float pnx, pny;
                    if (npc_visible_pos(pv, &pnx, &pny))
                    {
                        float ch = 0.35f
                                 + (float)player.skills[SKILL_PICKPOCKET].level * 0.005f
                                 + perkFx.pickChanceBonus
                                 + (player.sneaking ? 0.25f : 0.0f)
                                 + (npc_bark_active(pv) ? 0.15f : 0.0f)
                                 - (NPCS[pv].isGuard ? 0.30f : 0.0f)
                                 - 0.15f * (float)(pv < PICK_TRIES_CAP
                                                   ? pickTries[pv] : 0);
                        if (ch > 0.90f) ch = 0.90f;
                        if (pv < PICK_TRIES_CAP) pickTries[pv]++;
                        int pRegion = region_of(pnx, pny);
                        if (ch > 0.0f && randf() < ch)
                        {
                            // BAŞARI: kese KİRLİ keseye düşer (harcanamaz —
                            // aklanmalı); değerli objeler ÇALINTI damgalı
                            int gMin = 10, gMax = 30, itemId = -1, trinket = -1;
                            float itemCh = 0.0f, trinketCh = 0.08f;
                            trinket = ITEM_TRINKET_RING;
                            if (pv == kervanIdx)
                            { gMin = 50; gMax = 120; trinket = ITEM_TRINKET_WATCH;
                              trinketCh = 0.15f; }
                            else if (NPCS[pv].isVendor)
                            { gMin = 30; gMax = 80; itemId = ITEM_POTION_HP;
                              itemCh = 0.20f; trinketCh = 0.15f; }
                            else if (NPCS[pv].isGuard)
                            { gMin = 20; gMax = 50; itemId = ITEM_LOCKPICK;
                              itemCh = 0.25f; trinket = ITEM_TRINKET_BROOCH;
                              trinketCh = 0.10f; }
                            else if (pv == nuhIdx || pv == fatmaIdx)
                            { gMin = 5; gMax = 15; trinketCh = 0.0f; }   // yoksul
                            int gold = gMin + rand() % (gMax - gMin + 1);
                            player.dirtyGold += gold;
                            float im = (perkFx.pickItemMult > 0.0f)
                                     ? perkFx.pickItemMult : 1.0f;
                            if (itemId >= 0 && randf() < itemCh * im)
                            {
                                ItemStack st2 = item_make(&ITEM_TYPES[itemId], 1, 0);
                                st2.stolen = 1;
                                bag_add_stackp(&st2);
                            }
                            if (trinketCh > 0.0f && randf() < trinketCh * im)
                            {
                                ItemStack st2 = item_make(&ITEM_TYPES[trinket], 1, 0);
                                st2.stolen = 1;
                                bag_add_stackp(&st2);
                            }
                            if (perkFx.pickStealItem && randf() < 0.35f)   // GÖLGE ELİ
                            {
                                ItemStack st2 = item_make(&ITEM_TYPES[ITEM_TRINKET_RING],
                                                          1, 0);
                                st2.stolen = 1;
                                bag_add_stackp(&st2);
                            }
                            player.persKetum++;   // kişilik izi: sessiz iş
                            audio_play(SFX_LOOT);
                            char buf[FLOAT_TEXT_LEN];
                            snprintf(buf, sizeof(buf), "+%d KİRLİ altın", gold);
                            SDL_Color vio = { 200, 160, 255, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f, buf, vio);
                            if (skill_add_xp(player.skills, SKILL_PICKPOCKET, 25.0f))
                                on_skill_levelup(&player, SKILL_PICKPOCKET,
                                                 &hotbar, floatTexts);
                        }
                        else if (perkFx.pickFirstFailFree && !pickFailUsed)
                        {
                            // MASUM YÜZ: günde bir başarısızlık fark edilmez
                            pickFailUsed = true;
                            SDL_Color dim = { 190, 190, 200, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f,
                                                 "Elin kaydı... kimse görmedi.", dim);
                            if (skill_add_xp(player.skills, SKILL_PICKPOCKET, 8.0f))
                                on_skill_levelup(&player, SKILL_PICKPOCKET,
                                                 &hotbar, floatTexts);
                        }
                        else
                        {
                            // YAKALANDIN: hedef tanıktır — suç işler
                            float bm = (perkFx.pickBountyMult > 0.0f)
                                     ? perkFx.pickBountyMult : 1.0f;
                            player.bounty[pRegion] += (int)(15.0f * bm);
                            rep_add(&player.reputation[pRegion], -3.0f);
                            player.lastCrimeDay = gametime_day();
                            npc_bark(pv, "Hırsız! HIRSIZ var!", 5.0f);
                            audio_play(SFX_UI);
                            SDL_Color red = { 255, 120, 100, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 24.0f,
                                                 "Yakalandın!", red);
                            if (NPCS[pv].isGuard)
                            {
                                // Muhafızın cebinde yakalanmak: anında hesap sorar
                                catchOpen = true;
                                player.autoAttack = false;
                            }
                            if (skill_add_xp(player.skills, SKILL_PICKPOCKET, 8.0f))
                                on_skill_levelup(&player, SKILL_PICKPOCKET,
                                                 &hotbar, floatTexts);
                        }
                    }
                }
                else if (event.key.key == SDLK_R && gameState == GAME_PLAYING &&
                         !scene_active() && dialogNpc < 0 && nearPortal >= 0 &&
                         PORTALS[nearPortal].to == MAP_LOCA)
                {
                    // SESSİZ EL inişi: kuyu E'yi aklamaya bırakır, ip R'dedir
                    // (Hakan raporu: E çakışması — kuyuya bırakma hep kazanıyordu)
                    pendingPortal = nearPortal;
                    audio_play(SFX_UI);
                }
                else if (event.key.key == SDLK_R && gameState == GAME_PLAYING &&
                         !scene_active() && dialogNpc < 0 &&
                         map_current() == MAP_WORLD && nearestStation >= 0 &&
                         STATIONS[nearestStation].type == STATION_CAMPFIRE &&
                         !gametime_is_night())
                {
                    // Ateş başında ama henüz gece değil (gece 21:00'de başlar;
                    // ekran 19:00'dan itibaren kararır — sessiz kalma, söyle)
                    SDL_Color dim = { 200, 200, 210, 255 };
                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                         player.rect.x + player.rect.w / 2.0f,
                                         player.rect.y - 24.0f,
                                         "Uyumak için erken — gece (21:00) yeniden gel.", dim);
                }
                else if (event.key.key == SDLK_R && gameState == GAME_PLAYING &&
                         !scene_active() && dialogNpc < 0 &&
                         map_current() == MAP_WORLD && nearestStation >= 0 &&
                         STATIONS[nearestStation].type == STATION_CAMPFIRE &&
                         gametime_is_night())
                {
                    // DİNLENME (Dünya Olayları): ateş başında sabahı bekle.
                    // gametime_set sonrası day-rollover DOĞAL tetiklenir —
                    // olay zarı, ekoloji, otomatik kayıt hepsi sabah işler.
                    int wakeDay = gametime_day() +
                                  (gametime_hours() >= 7.0f ? 1 : 0);
                    gametime_set(wakeDay, 7.0f);
                    player.health = player.maxHealth;
                    player.stamina = player.maxStamina;
                    if (player.mana < player.maxMana * 0.5f)
                        player.mana = player.maxMana * 0.5f;   // uyku manayı yarılar, doldurmaz
                    player.autoAttack = false;
                    SDL_Color mor = { 200, 180, 255, 255 };
                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                         player.rect.x + player.rect.w / 2.0f,
                                         player.rect.y - 24.0f,
                                         "Sabah oldu.", mor);
                    audio_play(SFX_UI);
                    if (randf() < 0.25f)   // rüya: 3 havuzdan biri (imalar, cevap yok)
                    {
                        static const char *DREAMS[3] = {
                            "assets/scenes/ruya_muhur.txt",
                            "assets/scenes/ruya_cukur.txt",
                            "assets/scenes/ruya_fisilti.txt",
                        };
                        scene_start(DREAMS[(int)(randf() * 2.99f)]);
                    }
                }
                else if (questLogOpen && gameState == GAME_PLAYING &&
                         (event.key.key == SDLK_TAB ||
                          event.key.key == SDLK_LEFT ||
                          event.key.key == SDLK_RIGHT))
                {
                    // Günlük gezinme: Tab sekme, oklar yaprak (render klampler)
                    if (event.key.key == SDLK_TAB)
                    {
                        journalTab = (journalTab + 1) % 4;   // Görevler→Günlük→Ünvanlar→Kontrat
                        // Günlük sekmesi son yaprağa açılır (en taze yaşanmışlık)
                        journalPage = (journalTab == 1) ? 9999 : 0;   // Günlük son sayfa
                    }
                    else if (event.key.key == SDLK_LEFT && journalPage > 0)
                        journalPage--;
                    else if (event.key.key == SDLK_RIGHT &&
                             journalPage < journalPageCount - 1)
                        journalPage++;
                    audio_play(SFX_UI);
                }
                else if (event.key.key == SDLK_M && gameState == GAME_PLAYING)
                {
                    minimapOpen = !minimapOpen;
                    audio_play(SFX_UI);
                }
                else if (event.key.key == SDLK_F7)
                {
                    // DEV: post-fx preset döngüsü (kapalı dahil) — tarz denemesi.
                    // Topuzlar: POSTFX_PRESETS (r2d_init'in hemen altında).
                    postfxIdx = (postfxIdx + 1) % POSTFX_COUNT;
                    r2d_postfx_set(renderer, (postfxIdx > 0)
                                             ? &POSTFX_PRESETS[postfxIdx].fx : NULL);
                    splashText = POSTFX_PRESETS[postfxIdx].name;
                    splashTimer = 2.0f;
                }
                else if (event.key.key == SDLK_F8 && gameState == GAME_PLAYING)
                {
                    // DEV: havayı elle döndür (Açık→Yağmur→Sis→Fırtına) —
                    // hava etkilerini beklemeden test için. Yayında sökülecek.
                    weather_debug_cycle();
                    splashText = weather_name(weather_current());
                    splashTimer = 2.0f;
                }
                else if (event.key.key == SDLK_L &&
                         (event.key.mod & SDL_KMOD_CTRL) &&
                         gameState == GAME_PLAYING)
                {
                    // DEV: IŞIK ŞEKLİ (Ctrl+L / Ctrl+Shift+L geri).
                    // Kamera 3/4 eğik olduğu için zemin dikey
                    // kısaltılmış çiziliyor — zeminde dairesel olan alan ekranda
                    // yatay elips görünmeli. Ve kaynak zeminde değil (hazne/alev).
                    // Ctrl+L ileri, Ctrl+Shift+L geri
                    int dir = (event.key.mod & SDL_KMOD_SHIFT) ? 3 : 1;
                    lightShape = (lightShape + dir) % 4;
                    float an, yo;
                    switch (lightShape) {
                        case 1:  an = 0.70f; yo = -10.0f; break;  // daha basık
                        case 2:  an = 1.40f; yo =  12.0f; break;  // dikeye çek
                        case 3:  an = 1.90f; yo =   0.0f; break;  // tam daire (tipi ez)
                        default: an = 1.00f; yo =   0.0f; break;  // TİPİN kendi şekli
                    }
                    r2d_light_set_shape(an, yo);
                    splashText = (lightShape == 0) ? "SEKIL: TIPIN KENDISI (kor basik, ates genis)"
                               : (lightShape == 1) ? "SEKIL: daha BASIK"
                               : (lightShape == 2) ? "SEKIL: DIKEYE cek"
                                                   : "SEKIL: DAIREye yaklastir";
                    splashTimer = 2.6f;
                }
                else if (event.key.key == SDLK_V &&
                         (event.key.mod & SDL_KMOD_CTRL) &&
                         gameState == GAME_PLAYING)
                {
                    // DEV: SU VFX ANAHTARI (Ctrl+V) — dört durum döngüsü.
                    // Soru: kıyıda gördüğüm hareket ASSET animasyonundan mı
                    // geliyor, bizim kodumuzdan mı? İki kaynak bağımsız kısılıyor:
                    //   1) asset akışı (Water_Tile_1_Anim, 8 kare)
                    //   2) şerit kaydırma demosu (köy güneyi taş havuz — su
                    //      asset'i HİÇ kullanmıyor, yani hareketi kesin bizim)
                    tilemap_vfx_cycle();
                    splashText = tilemap_vfx_label();
                    splashTimer = 2.8f;
                }
                else if (event.key.key == SDLK_O &&
                         (event.key.mod & SDL_KMOD_CTRL) &&
                         gameState == GAME_PLAYING)
                {
                    // DEV: IŞIK TIKAMA aç/kapa (Ctrl+O). Nesne gövdeleri ışığı kesiyor;
                    // kapatınca ışık eskisi gibi duvarlardan geçer.
                    lightOccl = (lightOccl > 0.01f) ? 0.0f : 1.0f;
                    splashText = (lightOccl > 0.01f) ? "TIKAMA: ACIK (isik govdelerden gecmez)"
                                                     : "TIKAMA: KAPALI";
                    splashTimer = 2.4f;
                }
                else if (event.key.key == SDLK_B &&
                         (event.key.mod & SDL_KMOD_CTRL) &&
                         gameState == GAME_PLAYING)
                {
                    // DEV: SU KATMANI İZOLE (Ctrl+B / Ctrl+Shift+B geri).
                    // Sekiz katman + "hepsi açık" = 9 durum. İzole modda YALNIZ
                    // seçili katmanın biti sürülüyor, gerisi shader'da kapanıyor.
                    int dir = (event.key.mod & SDL_KMOD_SHIFT)
                              ? R2D_WATER_L_COUNT : 1;
                    waterLayer = (waterLayer + dir) % (R2D_WATER_L_COUNT + 1);
                    splashText = (waterLayer == 0) ? "SU: hepsi acik"
                                                   : WATER_LAYERS[waterLayer - 1].name;
                    splashTimer = 2.6f;
                }
                else if (event.key.key == SDLK_F &&
                         (event.key.mod & SDL_KMOD_CTRL) &&
                         gameState == GAME_PLAYING)
                {
                    // DEV: ATEŞ KATMANI İZOLE (Ctrl+F / Ctrl+Shift+F geri).
                    // Dört katman + "hepsi açık" = 5 durum.
                    int dir = (event.key.mod & SDL_KMOD_SHIFT) ? FX_L_COUNT : 1;
                    fireLayer = (fireLayer + dir) % (FX_L_COUNT + 1);
                    fx_set_layers(fireLayer == 0 ? FX_L_ALL
                                                 : FIRE_LAYERS[fireLayer - 1].bit);
                    splashText = (fireLayer == 0) ? "ATES: hepsi acik"
                                                  : FIRE_LAYERS[fireLayer - 1].name;
                    splashTimer = 2.6f;
                }
                else if (event.key.key == SDLK_G &&
                         (event.key.mod & SDL_KMOD_CTRL) &&
                         gameState == GAME_PLAYING)
                {
                    // DEV: SİLAH EFEKTİNİ DÖNGÜLE. Alev ve buz kodda var ama
                    // onları veren EŞYA yok (rün listesinde yalnız pembe ve
                    // zehir; item tablosunda weaponFx bilerek boş). Yani bu iki
                    // efekt bugün erişilemez bir yol — alev rünü eklemek içerik
                    // kararı, sahibine sorulmadan yapılmaz. Bu tuş yalnız GÖRSEL
                    // test için efekti doğrudan basar (kalıcı değil: silah
                    // değişince main 8258'deki atama üzerine yazar).
                    static const struct { WeaponFxType t; const char *n; } WFX[] = {
                        { WEAPON_FX_NONE,  "SILAH FX: yok" },
                        { WEAPON_FX_FLAME, "SILAH FX: ALEV" },
                        { WEAPON_FX_ICE,   "SILAH FX: BUZ" },
                        { WEAPON_FX_PINK,  "SILAH FX: pembe enerji" },
                        { WEAPON_FX_POISON,"SILAH FX: zehir" },
                        { WEAPON_FX_VENOM, "SILAH FX: igne zehri" },
                    };
                    static int wfxIdx = 0;
                    wfxIdx = (wfxIdx + 1) % (int)(sizeof(WFX) / sizeof(WFX[0]));
                    // GEÇERSİZ KILMA ŞART: player.weaponFx her karede
                    // kuşanılan silahtan yeniden atanıyor (aşağıda) ve
                    // buradaki değeri eziyordu — tuş işe yaramıyordu.
                    // 0. eleman (yok) seçilince geçersiz kılma KAPANIR.
                    devFxOverride = (wfxIdx == 0) ? WEAPON_FX_COUNT : WFX[wfxIdx].t;
                    player.weaponFx = WFX[wfxIdx].t;
                    splashText = WFX[wfxIdx].n;
                    splashTimer = 2.4f;
                }
                else if (event.key.key == SDLK_T &&
                         (event.key.mod & SDL_KMOD_CTRL) &&
                         gameState == GAME_PLAYING)
                {
                    // DEV: EĞİTİM KUKLASI koy — oyuncunun 2 karo önüne.
                    // Saldırmaz, kaçmaz, ölmez: VFX/hasar testi için sabit hedef
                    // (yaratıklar saldırdığı için efekt izlenemiyordu).
                    float dx0 = 0.0f, dy0 = 0.0f;
                    switch (player.facing) {
                        case DIR_UP:    dy0 = -128.0f; break;
                        case DIR_DOWN:  dy0 =  128.0f; break;
                        case DIR_LEFT:  dx0 = -128.0f; break;
                        default:           dx0 =  128.0f; break;
                    }
                    int did = mob_spawn(mobs, MAX_MOBS, &nextMobId,
                                        &MOB_TYPES[MOB_DUMMY],
                                        player.rect.x + dx0, player.rect.y + dy0, -1);
                    splashText = (did >= 0) ? "KUKLA kondu (Ctrl+T)"
                                            : "KUKLA: mob havuzu dolu";
                    splashTimer = 2.0f;
                }
                else if (event.key.key == SDLK_Y &&
                         (event.key.mod & SDL_KMOD_CTRL) &&
                         gameState == GAME_PLAYING)
                {
                    // DEV: sınırsız mana + stamina + can (test modu).
                    // Ctrl+I DEĞİL: I envanteri açıyor, tuş çakışıyordu.
                    devInfinite = !devInfinite;
                    splashText = devInfinite ? "DEV: sinirsiz mana/stamina/can ACIK"
                                             : "DEV: sinirsiz KAPALI";
                    splashTimer = 2.4f;
                }
                else if (event.key.key == SDLK_P &&
                         (event.key.mod & SDL_KMOD_CTRL) &&
                         gameState == GAME_PLAYING)
                {
                    // DEV A/B: PROSEDÜREL SKİLL EFEKTLERİ. Açık = kendi
                    // motorumuz (VARSAYILAN, 2026-08-22), kapalı = eski sprite
                    // sheet'leri (assets/fx/*.png). Karşılaştırma olmadan
                    // "daha iyi mi" sorusu cevaplanamaz; sprite yolu bu yüzden
                    // silinmiyor.
                    devSkillfxAB = true;   // gösterge artık her cast'te konuşur
                    skillfx_set_procedural(!skillfx_procedural());
                    splashText = skillfx_procedural()
                        ? "SKILL FX: PROSEDUREL (kendi motorumuz)"
                        : "SKILL FX: sprite sheet (eski)";
                    splashTimer = 2.8f;
                }
                else if (event.key.key == SDLK_F12 && gameState == GAME_PLAYING)
                {
                    // DEV: A/B KARŞILAŞTIRMA — ekranın SOL yarısı orijinal
                    // ayarlar, SAĞ yarısı seçili modlar. Varyantların çoğu tek
                    // başına ayırt edilemiyor; yan yana bakınca fark görünür.
                    lightSplit = !lightSplit;
                    splashText = lightSplit ? "A/B: sol=ORIJINAL  sag=SECILI"
                                            : "A/B: kapali";
                    splashTimer = 2.4f;
                }
                else if (event.key.key == SDLK_F1 && gameState == GAME_PLAYING)
                {
                    // DEV: GAMMA MODU — en büyük görsel fark.
                    // Işık katkıları sRGB kodunda toplanıyordu: merkez +0.22
                    // fazla parlayıp KIRPILIYOR (düz beyaz plato), dış halka
                    // 0.03 karanlık kalıyor. Fiziksel doğru olan lineer toplama.
                    lightGamma = (lightGamma + 1) % 3;
                    r2d_light_set_model((float)lightGamma, (float)lightFall,
                                        (float)lightDither,
                                        light_falloff_param());
                    splashText = (lightGamma == 0) ? "GAMMA: sRGB toplama (eski/hatali)"
                               : (lightGamma == 1) ? "GAMMA: LINEER toplama (fiziksel)"
                                                   : "GAMMA: lineer + ACES tone map";
                    splashTimer = 2.4f;
                }
                else if (event.key.key == SDLK_F2 && gameState == GAME_PLAYING)
                {
                    // DEV: DÜŞÜŞ EĞRİSİ. CPU ve GPU birlikte değişir — ayrışırsa
                    // oyuncu aydınlık gördüğü yerde ölüyle karşılaşır.
                    lightFall = (lightFall + 1) % 4;
                    float fp = (lightFall == 1) ? 41.0f    // Karis d0 (px)
                             : (lightFall == 2) ? 1.0f     // URP k (1 = lineer)
                                                : 0.0f;
                    light_set_falloff(lightFall, fp);
                    r2d_light_set_model((float)lightGamma, (float)lightFall,
                                        (float)lightDither, fp);
                    splashText = (lightFall == 0) ? "DUSUS: plato + smoothstep (mevcut)"
                               : (lightFall == 1) ? "DUSUS: Karis pencereli ters-kare"
                               : (lightFall == 2) ? "DUSUS: URP pow(1-t,k) k=1 lineer"
                                                  : "DUSUS: kenar dizi (knee)";
                    splashTimer = 2.4f;
                }
                else if (event.key.key == SDLK_F3 && gameState == GAME_PLAYING)
                {
                    // DEV: DITHER. Bayer = 8-bit dönemi deseni; dünya-çivili
                    // olan kamera hareketinde "crawl" etmiyor.
                    lightDither = (lightDither + 1) % 4;
                    r2d_light_set_model((float)lightGamma, (float)lightFall,
                                        (float)lightDither, light_falloff_param());
                    splashText = (lightDither == 0) ? "DITHER: IGN (gradient noise)"
                               : (lightDither == 1) ? "DITHER: Bayer 4x4 ordered"
                               : (lightDither == 2) ? "DITHER: dunya-civili IGN"
                                                    : "DITHER: yok";
                    splashTimer = 2.4f;
                }
                else if (event.key.key == SDLK_F11 && gameState == GAME_PLAYING)
                {
                    // DEV: FİZİKSEL RENK — ışık renkleri siyah cisim eğrisinden.
                    // Elle seçtiğimiz kandil rengi ≈3130K (tungsten lamba);
                    // fiziksel kor 2400K, mum 1900K, çıra 1700K.
                    bool on = !light_physical_color();
                    light_set_physical_color(on);
                    splashText = on ? "RENK: FIZIKSEL (Kelvin siyah cisim)"
                                    : "RENK: elle secilmis (sanat yonu)";
                    splashTimer = 2.4f;
                }
                else if (event.key.key == SDLK_F4 && gameState == GAME_PLAYING)
                {
                    // DEV: IŞIK GÖRSEL MODU — üç mod arasında döner.
                    // Yalnız görseli değiştirir; ölülerin bastırıldığı sınır
                    // CPU'da (lamp_level_at) ve bundan bağımsız.
                    lightMode = (lightMode + 1) % 3;
                    switch (lightMode) {
                        case 0:   // SERBEST: sürekli gradyan + dither
                            r2d_light_set_mode(0.0f, 0.0f);
                            splashText = "ISIK: SERBEST (surekli)";
                            break;
                        case 1:   // KADEMELİ ince: pixel-art hissi, teras kaba değil
                            // 5 basamak çok azdı: farklı tonlardaki komşu zemin
                            // karoları farklı basamağa düşüp karo ızgarasını
                            // ışığın içinde görünür yapıyordu. 14 kademe hem
                            // sürekli gradyanı kırıyor hem blok üretmiyor.
                            // Izgara 8 px: retina'da sanat pikseli 4 MANTIKSAL
                            // px = 8 framebuffer px (4 yazmak yarım karo oluyordu).
                            r2d_light_set_mode(14.0f, 8.0f);
                            splashText = "ISIK: KADEMELI 14 (sanat izgarasi)";
                            break;
                        default:  // KADEMELİ kaba: Rain World tarzı belirgin teras
                            r2d_light_set_mode(7.0f, 8.0f);
                            splashText = "ISIK: KADEMELI 7 (belirgin teras)";
                            break;
                    }
                    splashTimer = 2.2f;
                }
                else if (event.key.key == SDLK_F6 && gameState == GAME_PLAYING)
                {
                    // DEV: saati 2 saat ilerlet — kandil/gece testini beklemeden
                    // yapmak için (LORE §1.4). Yayında sökülecek.
                    gametime_update(GAMETIME_DAY_SECONDS / 24.0f * 2.0f);
                    gametime_format(splashOwn, sizeof(splashOwn));
                    splashText = splashOwn;
                    splashTimer = 1.6f;
                }
                else if (event.key.key == SDLK_F10 && gameState == GAME_PLAYING)
                {
                    // DEBUG: perk testi için XP pompala (One-Handed + 3 zırh skill'i)
                    // + meslek testi: craft malzemeleri ve meslek XP'si.
                    // Yayın öncesi kaldırılacak — commit'e girer ama işaretli.
                    // Eğri dik: seviye 25 ≈ 125k, 100 ≈ 4M XP → basış başına 1M
                    // (1 basış ~lv57, 2 ~lv75, 4 = tüm eşikler açık)
                    SkillType dbg[19] = { SKILL_ONE_HANDED, SKILL_LIGHT_ARMOR,
                                          SKILL_MEDIUM_ARMOR, SKILL_HEAVY_ARMOR,
                                          SKILL_SMITHING, SKILL_ALCHEMY, SKILL_MINING,
                                          SKILL_HERBALISM, SKILL_WOODCUTTING,
                                          SKILL_ENCHANTING, SKILL_ARCHERY,
                                          SKILL_SNEAK, SKILL_SPEECH,
                                          SKILL_DESTRUCTION, SKILL_STAFF,
                                          SKILL_CONJURATION,
                                          SKILL_TWO_HANDED,
                                          SKILL_BLOCK,
                                          SKILL_PICKPOCKET };   // ağaç testi
                    for (int d = 0; d < 19; d++)
                        if (skill_add_xp(player.skills, dbg[d], 1000000.0f))
                            on_skill_levelup(&player, dbg[d], &hotbar, floatTexts);

                    // Craft test seti: araçlar + bol malzeme
                    bag_add(&ITEM_TYPES[ITEM_PICKAXE], 1, 0);
                    bag_add(&ITEM_TYPES[ITEM_AXE], 1, 0);
                    bag_add(&ITEM_TYPES[ITEM_HAMMER], 1, 0);
                    bag_add(&ITEM_TYPES[ITEM_ORE_COPPER], 20, 0);
                    bag_add(&ITEM_TYPES[ITEM_ORE_IRON], 20, 0);
                    bag_add(&ITEM_TYPES[ITEM_INGOT_COPPER], 10, 0);
                    bag_add(&ITEM_TYPES[ITEM_INGOT_IRON], 10, 0);
                    bag_add(&ITEM_TYPES[ITEM_HERB_RED], 10, 0);
                    bag_add(&ITEM_TYPES[ITEM_HERB_BLUE], 10, 0);
                    bag_add(&ITEM_TYPES[ITEM_WOOD], 10, 0);
                    // Büyü dalgası 2 testi: 3 tomar çantaya (sağ tık = oku)
                    bag_add(&ITEM_TYPES[ITEM_TOME_CLAW], 1, 0);
                    bag_add(&ITEM_TYPES[ITEM_TOME_ICETACLE], 1, 0);
                    bag_add(&ITEM_TYPES[ITEM_TOME_TORNADO], 1, 0);
                    bag_add(&ITEM_TYPES[ITEM_TOME_SNAKEBITE], 1, 0);
                    bag_add(&ITEM_TYPES[ITEM_TOME_FIRESURGE], 1, 0);
                    // Kutsal testi (Faz HOLY): 3 tomar
                    bag_add(&ITEM_TYPES[ITEM_TOME_HEAL], 1, 0);
                    bag_add(&ITEM_TYPES[ITEM_TOME_HOLYBEAM], 1, 0);
                    bag_add(&ITEM_TYPES[ITEM_TOME_HOLYZONE], 1, 0);
                    bag_add(&ITEM_TYPES[ITEM_SPEAR], 1, 0);   // Two-Handed testi
                    // Suikastçi zinciri testi: aklama eşiği dolu sayılır
                    if (player.launderedTotal < 500) player.launderedTotal = 500;
                    // Kontrat sistemi testi: sınıf açıksa rütbe puanı ver
                    if (player.storyFlags[SF_ASSASSIN] >= 13)
                        player.assassinPoints += 5;
                    bag_add(&ITEM_TYPES[ITEM_SHIELD_IRON], 1, 0);   // Block testi
                    // YANKICI test yolu: sayaç basılır ama sınıf keşifle açılır
                    if (player.echoesConsumed < 3 &&
                        player.storyFlags[SF_YANKICI] == 0)
                    {
                        player.echoesConsumed = 3;
                        player.storyFlags[SF_YANKICI] = 1;
                        SDL_Color vio5 = { 200, 160, 255, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                            player.rect.x + player.rect.w / 2.0f,
                            player.rect.y - 40.0f,
                            "\"Taşların eskisine gel. Ay görmesin.\"", vio5);
                    }
                    bag_add(&ITEM_TYPES[ITEM_BP_SWORD_IRON], 1, 0);
                    bag_add(&ITEM_TYPES[ITEM_BP_HELMET_IRON], 1, 0);
                    bag_add(&ITEM_TYPES[ITEM_BP_POISON_OIL], 1, 0);
                    bag_add(&ITEM_TYPES[ITEM_SCROLL_SMITHING], 2, 0);
                    // Faz 4 sunak test seti: özler + tam kademe için taşlar
                    bag_add(&ITEM_TYPES[ITEM_ESSENCE_PALE], 12, 0);
                    bag_add(&ITEM_TYPES[ITEM_ESSENCE_TOXIC], 12, 0);
                    for (int st = ITEM_STONE_STR; st <= ITEM_STONE_STA; st++)
                        bag_add(&ITEM_TYPES[st], 6, 0);
                    // Faz 5 okçuluk test seti
                    bag_add(&ITEM_TYPES[ITEM_BOW], 1, 0);
                    bag_add(&ITEM_TYPES[ITEM_ARROW], 50, 0);
                    // Faz 6 av test seti: deri seti + yemek malzemesi
                    bag_add(&ITEM_TYPES[ITEM_LEATHER], 12, 0);
                    bag_add(&ITEM_TYPES[ITEM_MEAT], 8, 0);
                    // Faz 7 slime test seti: jel + hazır tuzak
                    bag_add(&ITEM_TYPES[ITEM_SLIME_GEL], 8, 0);
                    bag_add(&ITEM_TYPES[ITEM_STICKY_TRAP], 3, 0);
                    // Faz 8 iskelet test seti: kemik + premium ok örnekleri
                    bag_add(&ITEM_TYPES[ITEM_BONE], 6, 0);
                    // Faz 22 büyü test seti: asa + Bilgelik Taşları + havuzlar full
                    bag_add(&ITEM_TYPES[ITEM_STAFF], 1, 0);
                    bag_add(&ITEM_TYPES[ITEM_STONE_INT], 6, 0);
                    bag_add(&ITEM_TYPES[ITEM_POTION_MANA], 5, 0);
                    // M2 yemek test seti
                    bag_add(&ITEM_TYPES[ITEM_STEW], 3, 0);
                    bag_add(&ITEM_TYPES[ITEM_SOUP], 3, 0);
                    bag_add(&ITEM_TYPES[ITEM_KAVURMA], 3, 0);
                    player.health  = player.maxHealth;
                    player.mana    = player.maxMana;
                    player.stamina = player.maxStamina;
                    bag_add(&ITEM_TYPES[ITEM_ARROW_BONE], 20, 0);
                    bag_add(&ITEM_TYPES[ITEM_ARROW_IRON], 20, 0);
                    player.gold += 500;
                    // Ünvan testi (M3): sayaçlar eşiğin hemen altına — birkaç
                    // eylemle ünvan düşsün (bildirim/panel/pasif akışı görülür)
                    player.titleStats[TS_KILL_SLIME]  = 98;
                    player.titleStats[TS_KILL_GOBLIN] = 98;
                    player.titleStats[TS_KILL_ORC]    = 73;
                    player.titleStats[TS_SNEAK_HITS]  = 48;
                    player.titleStats[TS_NIGHT_KILLS] = 98;
                    player.titleStats[TS_NESTS]       = 4;
                    player.titleStats[TS_KILL_ANIMAL] = 98;
                    player.titleStats[TS_ARROWS]      = 297;
                    player.titleStats[TS_POTIONS]     = 48;
                    player.titleStats[TS_COOKED]      = 23;

                    // Ağaç testi: her ağaca 99 serbest puan (tüm düğümler alınabilsin)
                    tree_set_debug_points(99);
                    SDL_Log("DEBUG F10: +1M XP (savas+meslek) + craft malzeme seti + 500g + 99 agac puani");
                }
                else if (event.key.key == SDLK_F5 && gameState == GAME_PLAYING)
                {
                    // Hızlı kayıt: AKTİF slota (slot menüden seçilir).
                    // Aktif slot yoksa (otomatik kayıttan yüklendi) uyarı basmak
                    // yerine kayıt slotu ekranını AÇ — bir kez seçilince F5 hep
                    // oraya yazar (otomatik slotun üzerine yazılmaz).
                    const char *msg;
                    if (activeSlot < 0)
                    {
                        gameState = GAME_PAUSED;
                        menuScreen = MENU_SLOTS_SAVE;
                        audio_play(SFX_UI);
                        msg = NULL;
                    }
                    else if (save_game(save_slot_path(activeSlot), &player,
                                       &inventory, &craftBag, &equipment, &hotbar, questStatus))
                    {
                        msg = "Kaydedildi";
                        save_read_summary(save_slot_path(activeSlot),
                                          &slotSums[activeSlot]);
                    }
                    else
                        msg = "Kayıt HATASI";
                    if (msg != NULL)
                    {
                        SDL_Color white = { 255, 255, 255, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f, msg, white);
                    }
                }
                else if (event.key.key == SDLK_F9 && gameState == GAME_PLAYING)
                {
                    // Hızlı yükleme: AKTİF slottan (dünya durumu save dışı)
                    MapId mapBeforeLoad = map_current();
                    if (activeSlot >= 0 &&
                        load_game(save_slot_path(activeSlot), &player,
                                  &inventory, &craftBag, &equipment, &hotbar, questStatus))
                    {
                        mapReloadNeeded = true;   // ekoloji popülasyonu haritayı
                        (void)mapBeforeLoad;      // her yüklemede yeniden kurar
                        tree_compute_effects(player.treeTaken, player.fateTaken, &perkFx);   // seçimler değişti
                        spells_apply_learned(&player, &perkFx);
                        echo_apply_unlocks(&player, &perkFx);
                        skill_set_xp_mult(perkFx.xpGainMult);
                        hotbar_purge_closed(&hotbar, perkFx.abilityOpen);
                        hotbar_place_unlocks(&hotbar, perkFx.abilityOpen);
                        if (guardIdx >= 0) guard_init(guardIdx);            // beyin sıfırdan (nöbete)
                        catchOpen = false;
                        lastSeenDay = gametime_day();   // günlük blok tetiklenmesin
                                                        // (pano durumu save'den geldi)
                        wasNightWE = gametime_is_night();   // gece kenarı yeniden tetiklenmesin
                        bags_apply_caps(&player, &inventory, &craftBag);
                        if (kervanIdx >= 0)
                            npc_force_hidden(kervanIdx,
                                             worldevent_current() != WE_CARAVAN);
                        SDL_Color white = { 255, 255, 255, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f, "Yüklendi", white);
                    }
                    else if (activeSlot < 0)
                    {
                        // F9 da slotsuz sessiz kalmasın (F5 ile aynı durum)
                        SDL_Color gray = { 200, 200, 210, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f,
                                             "Aktif slot yok - önce F5 ile kaydet",
                                             gray);
                    }
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
                     event.button.button == SDL_BUTTON_LEFT &&
                     (dragHbSlot >= 0 || dragAbility >= 0))
            {
                // HOTBAR bırakma: hücre = takas/ata · ✕ = kaldır · dışarı = İPTAL
                int hbW, hbH;
                SDL_GetWindowSize(window, &hbW, &hbH);
                float ux2 = (float)event.button.x, uy2 = (float)event.button.y;
                int target = hotbar_slot_at(ux2, uy2, hbW, hbH);
                if (dragHbSlot >= 0)
                {
                    if (target >= 0 && target != dragHbSlot)
                    {
                        HotbarSlot tmp = hotbar.slots[target];
                        hotbar.slots[target] = hotbar.slots[dragHbSlot];
                        hotbar.slots[dragHbSlot] = tmp;   // takas (boşsa taşıma)
                        audio_play(SFX_UI);
                    }
                    else if (hotbar_remove_zone_at(ux2, uy2, hbW, hbH))
                    {
                        hotbar.slots[dragHbSlot] = (HotbarSlot){0};   // yalnız ✕'e bırakınca
                        audio_play(SFX_UI);
                    }
                    // dışarı bırakma: hiçbir şey olmaz (yanlışlıkla kaldırma yok)
                }
                else if (dragAbility >= 0 && target >= 0)
                {
                    // K panelinden beceri atama: hedef hücreye yaz (üstüne yazar)
                    hotbar.slots[target] = (HotbarSlot){ .kind = HOTBAR_SKILL,
                                                         .abilityId = (AbilityId)dragAbility };
                    audio_play(SFX_UI);
                }
                dragHbSlot = -1;
                dragAbility = -1;
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
                     event.button.button == SDL_BUTTON_LEFT &&
                     (dragInvSlot >= 0 || dragEquipSlot != EQUIP_NONE))
            {
                // --- SÜRÜKLEMEYİ BIRAK (Cila): hedefe göre taşı/kuşan/çıkar/ata ---
                int dwW, dwH;
                SDL_GetWindowSize(window, &dwW, &dwH);
                float ux = (float)event.button.x, uy = (float)event.button.y;
                float dpxF, dpyF;
                inventory_panel_origin(dwW, dwH, &dpxF, &dpyF);
                int dpx = (int)dpxF, dpy = (int)dpyF;
                int tInv = inventoryOpen
                         ? inventory_slot_at(ux, uy, (float)dpx, (float)(dpy + INV_EQUIP_H)) : -1;
                EquipSlot tEq = inventoryOpen
                              ? equipment_slot_at(ux, uy, dpx, dpy) : EQUIP_NONE;
                int tHb = hotbar_slot_at(ux, uy, dwW, dwH);

                if (dragInvSlot >= 0)
                {
                    Inventory *db = (dragTab == 1) ? &craftBag : &inventory;
                    Inventory *ob = (dragTab == 1) ? &inventory : &craftBag;
                    ItemStack *src = &db->slots[dragInvSlot];
                    // Sekme başlığına bırak = DİĞER çantaya taşı; şeridin SAĞ
                    // ucundaki Çöp hücresi = İMHA (geri dönüşü yok)
                    bool onTabs = inventoryOpen && ux >= dpx &&
                                  ux <= dpx + INV_PANEL_W &&
                                  uy >= dpy + INV_EQUIP_H - 28 && uy < dpy + INV_EQUIP_H;
                    bool onTrash = onTabs && ux >= dpx + INV_PANEL_W - 84.0f;
                    if (src->type != NULL)
                    {
                        if (onTrash)
                        {
                            // Onay iste — imha [1] ile kesinleşir, [2]/ESC vazgeçer
                            trashTab = dragTab;
                            trashSlot = dragInvSlot;
                            audio_play(SFX_UI);
                        }
                        else if (onTabs)
                        {
                            ItemStack moving = *src;
                            if (inventory_add_stack(ob, &moving))
                            {
                                src->type = NULL;
                                src->count = 0;
                                audio_play(SFX_UI);
                            }
                            else
                            {
                                SDL_Color red = { 255, 120, 100, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f,
                                                     "Diğer çanta dolu!", red);
                            }
                        }
                        else if (tInv >= 0 && tInv != dragInvSlot &&
                                 tInv < db->cap && dragTab == invTab)
                        {
                            // Sekme içi: yer değiştir (düzenleme)
                            ItemStack tmp = db->slots[tInv];
                            db->slots[tInv] = *src;
                            *src = tmp;
                            audio_play(SFX_UI);
                        }
                        else if (tEq != EQUIP_NONE && src->type->equipable)
                        {
                            if (src->stolen)
                            {
                                SDL_Color red = { 255, 120, 100, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f,
                                                     "Çalıntı — kuşanılamaz.", red);
                            }
                            else
                            {
                                equip_from_inventory(&equipment, db, dragInvSlot);
                                audio_play(SFX_UI);
                            }
                        }
                        else if (tHb >= 0 &&
                                 (src->type->consumable || src->type->trapItem))
                        {
                            if (src->stolen)
                            {
                                SDL_Color red = { 255, 120, 100, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f,
                                                     "Çalıntı — kullanılamaz.", red);
                            }
                            else
                            {
                                hotbar_assign_item(&hotbar, src->type);
                                audio_play(SFX_UI);
                            }
                        }
                    }
                }
                else if (dragEquipSlot != EQUIP_NONE)
                {
                    // Ekipmandan envantere (panelin herhangi bir yerine bırak = çıkar)
                    if (tInv >= 0 || (ux >= dpx && ux <= dpx + INV_PANEL_W && uy >= dpy + INV_EQUIP_H))
                    {
                        unequip_slot(&equipment, &inventory, dragEquipSlot);
                        audio_play(SFX_UI);
                    }
                }
                dragInvSlot = -1;
                dragEquipSlot = EQUIP_NONE;
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
                     event.button.button == SDL_BUTTON_LEFT)
            {
                dragInvSlot = -1;               // güvenlik: askıda sürükleme kalmasın
                dragEquipSlot = EQUIP_NONE;
            }
            else if (event.type == SDL_EVENT_MOUSE_WHEEL &&
                     gameState == GAME_PLAYING && craftOpen && craftStation >= 0)
            {
                // Craft paneli kaydırma: tarif listesi panele sığmayabilir
                craftScroll -= (int)event.wheel.y;
                int msc = craft_panel_max_scroll(craftStation);
                if (craftScroll < 0) craftScroll = 0;
                if (craftScroll > msc) craftScroll = msc;
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                // BAŞLIK EKRANI: tıklama da "bir tuş" sayılır
                if (gameState == GAME_TITLE)
                {
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                        gameState = GAME_MENU;
                        menuScreen = MENU_ROOT;
                        for (int i = 0; i <= SAVE_SLOT_COUNT; i++)
                            save_read_summary(save_slot_path(i), &slotSums[i]);
                        audio_play(SFX_UI);
                    }
                }
                // KARAKTER YARATMA (lite): tıklamalar — ten/geçmiş/Başla
                else if (gameState == GAME_CREATE &&
                         event.button.button == SDL_BUTTON_LEFT)
                {
                    int cwW, cwH;
                    SDL_GetWindowSize(window, &cwW, &cwH);
                    float cx0 = cwW / 2.0f, mxx = (float)event.button.x,
                          myy = (float)event.button.y;
                    // Ten kutuları (3 × 40px, y=cwH*0.36)
                    for (int t2 = 0; t2 < 3; t2++)
                    {
                        float sx = cx0 - 70.0f + t2 * 50.0f, sy = cwH * 0.36f;
                        if (mxx >= sx && mxx < sx + 40.0f &&
                            myy >= sy && myy < sy + 40.0f)
                        { createSkin = t2; audio_play(SFX_UI); }
                    }
                    // Geçmiş kartları (4 × 190px, y=cwH*0.48, h=96)
                    for (int b2 = 0; b2 < 4; b2++)
                    {
                        float bx2 = cx0 - 400.0f + b2 * 205.0f, by2 = cwH * 0.48f;
                        if (mxx >= bx2 && mxx < bx2 + 190.0f &&
                            myy >= by2 && myy < by2 + 96.0f)
                        { createBg = b2; audio_play(SFX_UI); }
                    }
                    // Başla butonu (240×56, y=cwH*0.78)
                    if (mxx >= cx0 - 120.0f && mxx < cx0 + 120.0f &&
                        myy >= cwH * 0.78f && myy < cwH * 0.78f + 56.0f)
                    {
                        SDL_StopTextInput(window);
                        audio_play(SFX_PERK);

                            // === YENİ OYUN: her şey şablondan/sıfırdan (Faz B1) ===
                            activeSlot = createSlot;
                            player = playerTemplate;
                            player.rect.x = 24.0f * TILE_SIZE;   // köy doğusu, yol kenarı —
                            player.rect.y = 8.0f * TILE_SIZE;    // varışta yankının yanından geçilir
                            inventory = (Inventory){0};
                            equipment = (Equipment){0};
                            // Gezgin çıkını: uyanan yolcunun üstündekiler — kılıç +
                            // DERİ set (hafif: ağırlık cezasız, 15 def — ilk goblinler
                            // öldürmesin ama demir set hâlâ hedef olarak kalsın)
                            equipment.slots[EQUIP_WEAPON] =
                                item_make(&ITEM_TYPES[ITEM_LONG_SWORD], 1, 0);
                            equipment.slots[EQUIP_HELMET] =
                                item_make(&ITEM_TYPES[ITEM_HELMET_LEATHER], 1, 0);
                            equipment.slots[EQUIP_CHEST] =
                                item_make(&ITEM_TYPES[ITEM_CHEST_LEATHER], 1, 0);
                            equipment.slots[EQUIP_BOOTS] =
                                item_make(&ITEM_TYPES[ITEM_BOOTS_LEATHER], 1, 0);
                            bag_add(&ITEM_TYPES[ITEM_POTION_HP], 3, 0);
                            bag_add(&ITEM_TYPES[ITEM_MEAT_COOKED], 2, 0);
                            bag_add(&ITEM_TYPES[ITEM_TORCH], 1, 0);
                            player.spellsLearned = 0;   // tomarlar yeni hayatta yeniden bulunur
                            player.bagUpgrades = 0;     // çantalar taban boyuta döner
                            craftBag = (Inventory){0};
                            invTab = 0;
                            bags_apply_caps(&player, &inventory, &craftBag);
                            tree_set_debug_points(0);   // F10 kalıntısı yeni oyuna taşınmaz
                            tree_compute_effects(player.treeTaken, player.fateTaken, &perkFx);
                            spells_apply_learned(&player, &perkFx);
                            echo_apply_unlocks(&player, &perkFx);
                            skill_set_xp_mult(perkFx.xpGainMult);
                            hotbar = (Hotbar){0};
                            hotbar_place_unlocks(&hotbar, perkFx.abilityOpen);
                            memset(questStatus, 0, sizeof(QuestStatus) * MAX_QUESTS);
                            for (int i = 0; i < MAX_MOBS; i++) mobs[i] = (Mob){0};
                            nextMobId = 1;
                            for (int i = 0; i < MAX_LOOT; i++) loots[i] = (Loot){0};
                            openLootIndex = -1;
                            for (int i = 0; i < MAX_ARROWS; i++) arrows[i].active = false;
                            for (int i = 0; i < MAX_TRAPS; i++) traps[i].active = false;
                            for (int i = 0; i < MAX_MOB_PROJS; i++) mobProjs[i].active = false;
                            for (int i = 0; i < MAX_SPELL_PROJS; i++) spellProjs[i].active = false;
                            for (int i = 0; i < MAX_BURSTS; i++) bursts[i].active = false;
                            for (int i = 0; i < MAX_SPIKE_FX; i++) spikeFx[i].active = false;
                            for (int i = 0; i < MAX_CLAW_FX; i++) clawFx[i].active = false;
                            for (int i = 0; i < MAX_ICETACLE_FX; i++) icetacleFx[i].active = false;
                            for (int i = 0; i < MAX_TORNADOES; i++) tornadoes[i].active = false;
                            for (int i = 0; i < MAX_SNAKEBITE_FX; i++) snakeFx[i].active = false;
                            for (int i = 0; i < MAX_FIRESURGES; i++) firesurges[i].active = false;
                            for (int i = 0; i < MAX_DARKBURST_FX; i++) darkburstFx[i].active = false;
                            memset(floatTexts, 0, sizeof(FloatText) * MAX_FLOAT_TEXTS);
                            deathTimer = 0.0f;
                            volleyLeft = 0;
                            activeAmmo = ITEM_ARROW;
                            castSpell = -1;
                            gametime_init();
                            lastSeenDay = gametime_day();
                            contract_new_day(lastSeenDay);
                            resource_reset();
                            npc_reset();
                            craft_reset_learned();
                            minimap_reset_explored();
                            for (int i = 0; i < ECHO_COUNT; i++) echo_set_done(i, false);
                            boss_set_last_kill_day(-999);
                            boss_set_first_kill_done(false);
                            boss2_set_last_kill_day(-999);
                            boss2_set_first_kill_done(false);
                            for (int i = 0; i < CHEST_COUNT; i++)
                                chest_set_opened_day(i, -999);
                            if (guardIdx >= 0) guard_init(guardIdx);
                            catchOpen = false;
                            journal_reset();
                            journal_add(JE_WAKE, 0);   // günlüğün ilk satırı
                            ecology_reset();
                            worldevent_reset();
                            weather_reset();
                            if (kervanIdx >= 0) npc_force_hidden(kervanIdx, true);
                            ally_reset(allies);
                            allyWasActive[0] = allyWasActive[1] = false;
                            map_set_current(MAP_WORLD);
                            mapReloadNeeded = true;   // geçiş bloğu kamp/sandık/tuzağı kurar
                            currentRegion = -2;

                            // Slot hemen mühürlenir (kart dolu görünsün, F5 hedefi belli)
                            save_game(save_slot_path(createSlot), &player, 
                                      &inventory, &craftBag, &equipment, &hotbar, questStatus);
                            save_read_summary(save_slot_path(createSlot),
                                              &slotSums[createSlot]);

                            // Sessiz Gün açılış vinyeti — bitince köy doğusunda "uyanış"
                            gameState = GAME_PLAYING;
                            scene_start("assets/scenes/intro_demirboyun.txt");
                        
                        // --- Kimlik uygulanır (yaratma ekranı) ---
                        SDL_strlcpy(player.name,
                                    createName[0] ? createName : "Gezgin",
                                    sizeof(player.name));
                        player.skinTone = (Sint8)createSkin;
                        player.background = (Sint8)createBg;
                        if (createBg == 1)        // ESKİ ASKER: miğfer + söylenti
                        {
                            equipment.slots[EQUIP_HELMET] =
                                item_make(&ITEM_TYPES[ITEM_HELMET_IRON], 1, 0);
                            rep_add(&player.reputation[0], -5.0f);
                        }
                        else if (createBg == 2)   // AVCI: yay, kılıç YOK
                        {
                            equipment.slots[EQUIP_WEAPON] =
                                item_make(&ITEM_TYPES[ITEM_BOW], 1, 0);
                            bag_add(&ITEM_TYPES[ITEM_ARROW], 30, 0);
                        }
                        else if (createBg == 3)   // ÇIRAK: iksir+ot, altın eksik
                        {
                            bag_add(
                                          &ITEM_TYPES[ITEM_POTION_MANA], 2, 0);
                            bag_add(
                                          &ITEM_TYPES[ITEM_HERB_RED], 3, 0);
                            player.gold -= 25;
                            if (player.gold < 0) player.gold = 0;
                        }
                        // Ten tonu: base katmanlara renk çarpanı (yüklemede de)
                        {
                            static const Uint8 tones[3][3] = {
                                { 255, 255, 255 }, { 225, 190, 155 },
                                { 185, 140, 105 } };
                            for (int L = 0; L < LAYER_COUNT; L++)
                                r2d_texture_set_color_mod(layers[L].tex,
                                    tones[createSkin][0], tones[createSkin][1],
                                    tones[createSkin][2]);
                        }
                        // Slot mührü kimlikle güncellensin (isim kartta görünsün)
                        save_game(save_slot_path(createSlot), &player, 
                                  &inventory, &craftBag, &equipment, &hotbar, questStatus);
                        save_read_summary(save_slot_path(createSlot),
                                          &slotSums[createSlot]);
                    }
                }
                else if (event.button.button == SDL_BUTTON_LEFT &&
                         gameState == GAME_PLAYING && dialogNpc >= 0)
                {
                    // ESO diyaloğu fareyle: akan metin tıkla tamamlanır;
                    // seçenek satırı tıklanınca sentetik sayı tuşu basılır
                    // (asıl seçim mantığı tek yerde kalsın — tuş zinciri)
                    if (!dialog_type_done(&npcDialogs[dialogNpc], dialogNode))
                        dialog_type_reveal();
                    else
                    {
                        int drow = dialog_opt_row_at((float)event.button.x,
                                                     (float)event.button.y);
                        if (drow >= 0)
                        {
                            SDL_Event ke;
                            SDL_zero(ke);
                            ke.type = SDL_EVENT_KEY_DOWN;
                            ke.key.key = (SDL_Keycode)(SDLK_1 + drow);
                            SDL_PushEvent(&ke);
                        }
                    }
                }
                // MENÜ AÇIKSA (ana menü ya da ESC): tıklama menüye gider
                else if (gameState == GAME_PAUSED || gameState == GAME_MENU)
                {
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                        int mw, mh;
                        SDL_GetWindowSize(window, &mw, &mh);
                        int actSlot = -1;
                        bool fromPause = (gameState == GAME_PAUSED);
                        MenuAction act = menu_handle_click(&menuScreen,
                                                           (float)event.button.x,
                                                           (float)event.button.y,
                                                           mw, mh, slotSums, &actSlot,
                                                           &musicVol, &sfxVol);
                        // ESC kökenli Yükle ekranından "Geri" ROOT verir — MAIN'e çevir
                        if (fromPause && menuScreen == MENU_ROOT)
                            menuScreen = MENU_MAIN;
                        if (act != MENU_ACTION_NONE)
                            audio_play(SFX_UI);

                        if (act == MENU_ACTION_VOLUME)
                        {
                            music_set_volume(musicVol);
                            ambience_set_volume(musicVol);
                            audio_set_volume(sfxVol);
                            FILE *cf = fopen("settings.cfg", "w");
                            if (cf != NULL)
                            {
                                fprintf(cf, "music=%.2f sfx=%.2f\n", musicVol, sfxVol);
                                fclose(cf);
                            }
                        }
                        else if (act == MENU_ACTION_RESUME)
                            gameState = GAME_PLAYING;
                        else if (act == MENU_ACTION_QUIT)
                            running = false;
                        else if (act == MENU_ACTION_TO_MAINMENU)
                        {
                            // Oyun içi panelleri kapat, ana menüye dön (dünya bellekte
                            // bayat kalır — Yeni Oyun/Yükle zaten her şeyi tazeler)
                            gameState = GAME_MENU;
                            menuScreen = MENU_ROOT;
                            dialogNpc = -1; shopOpen = false; inventoryOpen = false;
                            skillsOpen = false; questLogOpen = false; boardOpen = false;
                            locaBoardOpen = false;
                            craftOpen = false; craftRecipe = -1; catchOpen = false;
                            treeOpenSkill = -1; treeSelNode = -1;
                            player.autoAttack = false;
                            activeSlot = -1;
                            for (int i = 0; i <= SAVE_SLOT_COUNT; i++)
                                save_read_summary(save_slot_path(i), &slotSums[i]);
                        }
                        else if (act == MENU_ACTION_SAVE_SLOT && actSlot >= 0)
                        {
                            bool ok = save_game(save_slot_path(actSlot), &player, 
                                                &inventory, &craftBag, &equipment, &hotbar,
                                                questStatus);
                            if (ok)
                            {
                                activeSlot = actSlot;   // kaydedilen slot aktif olur
                                save_read_summary(save_slot_path(actSlot),
                                                  &slotSums[actSlot]);
                            }
                            const char *msg = ok ? "Kaydedildi" : "Kayıt HATASI";
                            SDL_Color white = { 255, 255, 255, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f, msg, white);
                            gameState = GAME_PLAYING;
                        }
                        else if (act == MENU_ACTION_LOAD_SLOT && actSlot >= 0)
                        {
                            MapId mapBeforeMenuLoad = map_current();
                            if (load_game(save_slot_path(actSlot), &player,
                                          &inventory, &craftBag, &equipment, &hotbar, questStatus))
                            {
                                // (F9 yükleme-sonrası dizisiyle aynı — desenkron etme!)
                                mapReloadNeeded = true;   // dünya ekolojiden kurulur
                                (void)mapBeforeMenuLoad;
                                tree_compute_effects(player.treeTaken,
                                                     player.fateTaken, &perkFx);
                                spells_apply_learned(&player, &perkFx);
                                echo_apply_unlocks(&player, &perkFx);
                                skill_set_xp_mult(perkFx.xpGainMult);
                                hotbar_purge_closed(&hotbar, perkFx.abilityOpen);
                                hotbar_place_unlocks(&hotbar, perkFx.abilityOpen);
                                if (guardIdx >= 0) guard_init(guardIdx);
                                catchOpen = false;
                                lastSeenDay = gametime_day();
                                wasNightWE = gametime_is_night();
                                bags_apply_caps(&player, &inventory, &craftBag);
                                if (kervanIdx >= 0)
                                    npc_force_hidden(kervanIdx,
                                                     worldevent_current() != WE_CARAVAN);
                                deathTimer = 0.0f;
                                // otomatik kayıttan yüklemede aktif slot seçilmemiş olur
                                activeSlot = (actSlot < SAVE_SLOT_COUNT) ? actSlot : -1;
                                gameState = GAME_PLAYING;
                                SDL_Color white = { 255, 255, 255, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f,
                                                     "Yüklendi", white);
                            }
                            // yükleme başarısızsa slot ekranında kalınır (bozuk dosya)
                        }
                        else if (act == MENU_ACTION_NEW_GAME && actSlot >= 0)
                        {
                            // Yeni Oyun: önce KİM olduğuna karar ver (yaratma ekranı)
                            createSlot = actSlot;
                            createName[0] = '\0';
                            createSkin = 0;
                            createBg = 0;
                            gameState = GAME_CREATE;
                            SDL_StartTextInput(window);
                        }

                    }
                    // menü açıkken oyun tıklaması işlenmez
                }
                else if (event.button.button == SDL_BUTTON_LEFT && !scene_active())
                {
                    float worldX = event.button.x / camera.zoom + camera.x;
                    float worldY = event.button.y / camera.zoom + camera.y;

                    bool handled = false;

                    // -4) HOTBAR tıklaması: düz tık = becerine BAS (click-to-cast),
                    //     SHIFT+tık = sürükleme başlat (takas/taşıma/✕ kaldırma —
                    //     bırakma ayrı blokta). Boş hücre tıklaması da yutulur
                    //     (barın üzerinden dünyaya tık geçmesin).
                    {
                        int hbW2, hbH2;
                        SDL_GetWindowSize(window, &hbW2, &hbH2);
                        int hs2 = hotbar_slot_at((float)event.button.x,
                                                 (float)event.button.y, hbW2, hbH2);
                        if (hs2 >= 0)
                        {
                            if (hotbar.slots[hs2].kind != HOTBAR_EMPTY)
                            {
                                if (SDL_GetModState() & SDL_KMOD_SHIFT)
                                    dragHbSlot = hs2;      // taşıma modu
                                else
                                    hotbarPressed = hs2;   // tıkla-bas
                            }
                            handled = true;
                        }
                    }

                    // -3) Günlük Ünvanlar sekmesi: satıra tıkla = taşı/bırak (M3)
                    if (questLogOpen && journalTab == 2)
                    {
                        int dwW2, dwH2;
                        SDL_GetWindowSize(window, &dwW2, &dwH2);
                        int trow = journal_title_at((float)event.button.x,
                                                    (float)event.button.y,
                                                    dwW2, dwH2);
                        if (trow >= 0 && title_unlocked(&player, trow))
                        {
                            player.carriedTitle =
                                (player.carriedTitle == trow) ? -1 : (Sint8)trow;
                            audio_play(SFX_UI);
                        }
                        handled = true;
                    }

                    // -2) Envanter açıkken SOL basış = sürükleme başlangıcı (Cila)
                    if (inventoryOpen)
                    {
                        int dwW, dwH;
                        SDL_GetWindowSize(window, &dwW, &dwH);
                        float dpxF, dpyF;
                        inventory_panel_origin(dwW, dwH, &dpxF, &dpyF);
                        int dpx = (int)dpxF, dpy = (int)dpyF;
                        // Sekme şeridine tıklama: sekme değiştir (drag başlatmaz)
                        {
                            float cx2 = (float)event.button.x, cy2 = (float)event.button.y;
                            float tabY = (float)(dpy + INV_EQUIP_H - 28);
                            if (cy2 >= tabY && cy2 < tabY + 26.0f)
                                for (int t = 0; t < 2; t++)
                                    if (cx2 >= dpx + t * 120.0f &&
                                        cx2 < dpx + t * 120.0f + 114.0f)
                                    {
                                        invTab = t;
                                        audio_play(SFX_UI);
                                        handled = true;
                                    }
                        }
                        Inventory *ab = (invTab == 1) ? &craftBag : &inventory;
                        int dsl = inventory_slot_at((float)event.button.x,
                                                    (float)event.button.y,
                                                    (float)dpx, (float)(dpy + INV_EQUIP_H));
                        if (!handled && dsl >= 0 && dsl < ab->cap &&
                            ab->slots[dsl].type != NULL)
                        {
                            dragInvSlot = dsl;
                            dragTab = invTab;
                            handled = true;
                        }
                        else
                        {
                            EquipSlot des = equipment_slot_at((float)event.button.x,
                                                              (float)event.button.y,
                                                              dpx, dpy);
                            if (des != EQUIP_NONE &&
                                equipment.slots[des].type != NULL)
                            {
                                dragEquipSlot = des;
                                handled = true;
                            }
                        }
                    }

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

                        // ALINMIŞ aktif düğüm: sürükleme başlat (hotbar'a atama)
                        if (node >= 0 && (tsTaken & (1u << node)))
                        {
                            const SkillTreeDef *tdd = tree_get(ts);
                            if (tdd != NULL && tdd->nodes[node].grantAbility >= 0)
                                dragAbility = tdd->nodes[node].grantAbility;
                        }
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
                                spells_apply_learned(&player, &perkFx);
                                echo_apply_unlocks(&player, &perkFx);
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
                                spells_apply_learned(&player, &perkFx);
                                echo_apply_unlocks(&player, &perkFx);
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
                                                  40.0f, 40.0f,
                                                  (shopCaravan||shopLoca||shopDogu) ? caravanStockN : 0);
                        if (entry >= 0)
                        {
                            const ItemType *it = shopCaravan ? caravanStock[entry].type
                                                             : SHOP_STOCK[entry].type;
                            // Fiyat itibara göre: dip itibarda ×1.5, saygın müşteriye ×0.9
                            // Kervan pazarlıksız: her şey %15 pahalı (nadir mal bedeli)
                            int shopRegion = region_of(player.rect.x, player.rect.y);
                            int buyPrice = (int)(it->value *
                                                 rep_buy_mult(player.reputation[shopRegion]) *
                                                 perkFx.shopBuyMult *
                                                 item_season_trade_mult(it) *   // kış pahalı / hasat ucuz
                                                 (shopCaravan ? 1.15f : 1.0f) *
                                                 ((!shopLoca && npc_grudge(shopVendorIdx))
                                                  ? 1.25f : 1.0f));   // REVİR kırgınlığı (loca hariç)
                            if (player.gold >= buyPrice &&
                                bag_add(it, 1, 0))
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
                    // SESSİZ EL kontrat tahtası (P3): üstlen / ödül al / vazgeç
                    if (!handled && locaBoardOpen)
                    {
                        bool cReady = player.contractActive >= 0 &&
                            player.contractProgress >=
                                ASSASSIN_CONTRACTS[player.contractActive].count;
                        int row = assassin_board_row_at(player.assassinPoints,
                                     player.contractActive, cReady,
                                     (float)event.button.x, (float)event.button.y,
                                     40.0f, 40.0f);
                        SDL_Color goldc2 = { 255, 210, 60, 255 };
                        SDL_Color red2 = { 255, 120, 100, 255 };
                        if (row >= 0)
                        {
                            if (player.contractActive == row && cReady)
                            {
                                // Ödülü al: altın + rütbe puanı + rütbe atlama
                                const AssassinContract *c =
                                    &ASSASSIN_CONTRACTS[row];
                                int before = assassin_rank(player.assassinPoints);
                                player.gold += c->rewardGold;
                                player.assassinPoints += c->rankPoints;
                                player.contractActive = -1;
                                player.contractProgress = 0;
                                audio_play(SFX_LOOT);
                                questBannerTitle = "KONTRAT TAMAM";
                                questBannerSub = c->name;
                                questBannerTimer = 4.0f;
                                int after = assassin_rank(player.assassinPoints);
                                if (after > before)
                                {
                                    questBannerTitle = "RÜTBE YÜKSELDİ";
                                    questBannerSub = assassin_rank_name(after);
                                    questBannerTimer = 5.0f;
                                    audio_play(SFX_PERK);
                                }
                            }
                            else if (player.contractActive == row)
                                locaBoardOpen = false;   // etkin kontrata tık = vazgeçme yok, kapat
                            else if (player.contractActive >= 0)
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                    player.rect.x + player.rect.w / 2.0f,
                                    player.rect.y - 10.0f,
                                    "Önce etkin kontratı bitir", red2);
                            else
                            {
                                player.contractActive = (Sint8)row;
                                player.contractProgress = 0;
                                audio_play(SFX_UI);
                                questBannerTitle = "KONTRAT ÜSTLENİLDİ";
                                questBannerSub = ASSASSIN_CONTRACTS[row].name;
                                questBannerTimer = 4.0f;
                                (void)goldc2;
                            }
                            handled = true;
                        }
                        else { locaBoardOpen = false; handled = true; }
                    }

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
                            else if (ci >= 0 && contract_state(slot) != CONTRACT_DONE)
                            {
                                const ContractDef *c = &CONTRACTS[ci];
                                // Getir kontratı: İKİ çantaya bak (malzeme craft
                                // çantasına düşer — panel de böyle sayıyor; eski
                                // kod yalnız ana çantaya bakıp teslimi engelliyordu)
                                bool ready = (c->targetMob != NULL)
                                    ? contract_progress(slot) >= c->count
                                    : bag_count(&ITEM_TYPES[c->fetchItem]) >= c->count;
                                if (ready)
                                {
                                    if (c->targetMob == NULL)
                                        bag_consume_n(
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
                                                        (float)event.button.y, 40.0f, 40.0f,
                                                        craftScroll);
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
                            else if (bag_count(&ITEM_TYPES[runeItem]) < 1)
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
                            else if (bag_count(&ITEM_TYPES[stone[s]]) < next ||
                                     bag_count(&ITEM_TYPES[ITEM_INGOT_IRON]) < next * 2)
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
                            else if (bag_count(&ITEM_TYPES[ITEM_HAMMER]) < 1)
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
                                    bag_count(&ITEM_TYPES[r->in1Item]) >= r->in1Count &&
                                    (r->in2Item < 0 ||
                                     bag_count(&ITEM_TYPES[r->in2Item]) >= r->in2Count);

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
                                if (l->kind != LOOT_KIND_CHEST)
                                    l->active = false;   // boş kasa kaybolur (sandık kalır)
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
                                if (bag_add_stackp(stack))
                                {
                                    audio_play(SFX_LOOT);
                                    stack->type = NULL;
                                    stack->count = 0;
                                    stack->plusLevel = 0;
                                    if (loot_is_empty(&loots[openLootIndex]))
                                    {
                                        if (loots[openLootIndex].kind == LOOT_KIND_CHEST)
                                            openLootIndex = -1;   // sandık açık sergilenir
                                        else
                                        {
                                            loots[openLootIndex].active = false; // boş kasa KAYBOLUR
                                            openLootIndex = -1;
                                        }
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
                        if (li >= 0 && loots[li].state == LOOT_CLOSED &&
                            loots[li].kind == LOOT_KIND_CHEST && loots[li].lockTier >= 0)
                        {
                            // Kilitli sandık: maymuncuk varsa minigame başlar
                            if (bag_count(&ITEM_TYPES[ITEM_LOCKPICK]) <= 0)
                            {
                                SDL_Color red = { 255, 120, 100, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                     player.rect.x + player.rect.w / 2.0f,
                                                     player.rect.y - 10.0f,
                                                     "Maymuncuk yok!", red);
                            }
                            else if (!lockpick_active())
                            {
                                lockpick_start(loots[li].lockTier, li, &perkFx);
                                player.autoAttack = false;
                                audio_play(SFX_UI);
                            }
                            handled = true;
                        }
                        else if (li >= 0 && loots[li].state == LOOT_CLOSED)
                        {
                            loots[li].state = LOOT_OPENING;
                            loots[li].currentFrame = 0;
                            loots[li].animTimer = 0.0f;
                            openLootIndex = li;
                            if (loots[li].kind == LOOT_KIND_CHEST && loots[li].chestId >= 0)
                                chest_set_opened_day((int)loots[li].chestId, gametime_day());
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
                        float panelXF, panelYF;
                        inventory_panel_origin(winW, winH, &panelXF, &panelYF);
                        int panelX = (int)panelXF;

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
                            // 2) Degilse envantere mi? Çanta ızgarası ekipman
                            //    bölümünün altında başlıyor. panelYF kullanılıyor:
                            //    burada sabit 20 yazılıydı ve panel konumu veriden
                            //    gelince tıklama çizimden kopardı.
                            Inventory *ab = (invTab == 1) ? &craftBag : &inventory;
                            int slot = inventory_slot_at((float)event.button.x, (float)event.button.y,
                                                         (float)panelX,
                                                         panelYF + (float)INV_EQUIP_H);
                            if (slot >= 0 && slot < ab->cap)
                            {
                                ItemStack *st = &ab->slots[slot];
                                if (st->type != NULL && st->stolen)
                                {
                                    // ÇALINTI: hiçbir kullanım yolu açık değil —
                                    // önce aklanmalı (kervancı ya da Kör Kuyu)
                                    SDL_Color red = { 255, 120, 100, 255 };
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                         player.rect.x + player.rect.w / 2.0f,
                                                         player.rect.y - 10.0f,
                                                         "Çalıntı — önce aklanmalı.", red);
                                }
                                else if (shopOpen && st->type != NULL)
                                {
                                    // Dükkan açıkken sağ tık = SAT (alışın yarısı × itibar, plus dahil)
                                    int sellRegion = region_of(player.rect.x, player.rect.y);
                                    // BÖLGESEL FİYAT FARKI (Faz DOGU — ticaret döngüsü):
                                    // köy malı Taşdurak'ta ×1.6, çöl malı köyde ×1.5
                                    float tradeMult = 1.0f;
                                    if (shopDogu &&
                                        (st->type == &ITEM_TYPES[ITEM_LEATHER] ||
                                         st->type == &ITEM_TYPES[ITEM_MEAT] ||
                                         st->type == &ITEM_TYPES[ITEM_WOOD]))
                                        tradeMult = 1.6f;
                                    else if (!shopDogu && !shopCaravan && !shopLoca &&
                                             (st->type == &ITEM_TYPES[ITEM_DATE_FRUIT] ||
                                              st->type == &ITEM_TYPES[ITEM_SPICE]))
                                        tradeMult = 1.5f;
                                    int price = (int)(item_value(st) / 2 *
                                                      rep_sell_mult(player.reputation[sellRegion]) *
                                                      perkFx.shopSellMult * tradeMult *
                                                      item_season_trade_mult(st->type));   // mevsim talebi
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
                                else if (st->type != NULL && st->type->tomeAbility > 0)
                                {
                                    // Büyü tomarı (Büyü dalgası 2): oku → kalıcı öğren.
                                    // Yıkım seviyesi yetmiyorsa tomar DURUR (yanmaz).
                                    int ta = st->type->tomeAbility;
                                    SDL_Color red2 = { 255, 120, 100, 255 };
                                    if (player.spellsLearned & (1ull << ta))
                                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                             player.rect.x + player.rect.w / 2.0f,
                                                             player.rect.y - 10.0f,
                                                             "Bu büyüyü zaten biliyorsun.", red2);
                                    else if (player.skills[ABILITIES[ta].skill].level
                                             < st->type->tomeReqLevel)
                                    {
                                        // Gate tomarın KENDİ okuluna bakar (Kutsal
                                        // tomarı Yıkım istemez — Faz HOLY geneli)
                                        char buf[FLOAT_TEXT_LEN];
                                        snprintf(buf, sizeof(buf),
                                                 "Harfler kayıyor... (%s %d gerek)",
                                                 skill_name(ABILITIES[ta].skill),
                                                 st->type->tomeReqLevel);
                                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                             player.rect.x + player.rect.w / 2.0f,
                                                             player.rect.y - 10.0f, buf, red2);
                                    }
                                    else
                                    {
                                        const ItemType *tt = st->type;
                                        if (bag_consume(tt))
                                        {
                                            player.spellsLearned |= (1ull << ta);
                                            perkFx.abilityOpen[ta] = true;
                                            hotbar_place_unlocks(&hotbar, perkFx.abilityOpen);
                                            audio_play(SFX_PERK);   // törensel — yeni güç
                                            char buf[FLOAT_TEXT_LEN];
                                            snprintf(buf, sizeof(buf), "Yeni büyü: %s!",
                                                     ABILITIES[ta].name);
                                            SDL_Color vio = { 200, 160, 255, 255 };
                                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                                 player.rect.x + player.rect.w / 2.0f,
                                                                 player.rect.y - 24.0f, buf, vio);
                                        }
                                    }
                                }
                                else if (st->type != NULL && st->type->scrollXp > 0.0f)
                                {
                                    // Öğretici parşömen: sağ tık = kullan (+XP, tükenir)
                                    SkillType sk = (SkillType)st->type->scrollSkill;
                                    float xp = st->type->scrollXp;
                                    const ItemType *sct = st->type;
                                    if (bag_consume(sct))
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
                                        bag_consume(&ITEM_TYPES[bpIdx]);
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
                                    if (bag_count(&ITEM_TYPES[ITEM_WOOD])
                                            < CAMPFIRE_WOOD_COST)
                                    {
                                        char buf[FLOAT_TEXT_LEN];
                                        snprintf(buf, sizeof(buf), "Odun gerekli (%d)",
                                                 CAMPFIRE_WOOD_COST);
                                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                             player.rect.x + player.rect.w / 2.0f,
                                                             player.rect.y - 10.0f, buf, red);
                                    }
                                    else if (weather_is_wet() &&
                                             map_current() == MAP_WORLD)
                                    {
                                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                             player.rect.x + player.rect.w / 2.0f,
                                                             player.rect.y - 10.0f,
                                                             "Yağmurda ateş tutmaz.", red);
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
                                    if (st->type == &ITEM_TYPES[ITEM_WHISPER_BLADE] &&
                                        player.storyFlags[SF_YANKICI] != 2)
                                    {
                                        SDL_Color vio3 = { 200, 160, 255, 255 };
                                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                            player.rect.x + player.rect.w / 2.0f,
                                            player.rect.y - 10.0f,
                                            "Sapı avucunda ISLANIYOR sanki... tutamıyorsun.",
                                            vio3);
                                    }
                                    else
                                    equip_from_inventory(&equipment, ab, slot);
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

        // HIT-STOP (VFX cilası): vuruş oturunca dünya bir anlığına yavaşlar
        // ("vuruş oturdu" hissi). Menü/ölüm ekranı gerçek zamanda kalır.
        if (g_hitStopCd > 0.0f) {
            g_hitStopCd -= dt;
            if (g_hitStopCd < 0.0f) g_hitStopCd = 0.0f;
        }
        if (g_hitStop > 0.0f)
        {
            if (gameState == GAME_PLAYING && deathTimer <= 0.0f)
            {
                g_hitStop -= dt;
                dt *= 0.08f;
            }
            else
                g_hitStop = 0.0f;
        }

        int winW, winH;
        SDL_GetWindowSize(window, &winW, &winH); // pencere boyutunu al (dinamik olarak, pencere yeniden boyutlandırılabilir)
        // uilayout_set_window BURADA DEĞİL: olay döngüsünden önce yapılıyor
        // (düzenleyicinin tıklama testi de çapa çözümüne bakıyor)

        // DEV kancası: PIXE_AUTOPLAY=1 → dolaş + sürekli saldır (ASan stres
        // koşusu savaş/FX/loot yollarını menüsüz tetikleyebilsin)
        if (autoloadEnv != NULL && SDL_getenv("PIXE_AUTOPLAY") != NULL &&
            gameState == GAME_PLAYING && deathTimer <= 0.0f)
        {
            static float apT = 0.0f, apPortalT = 0.0f;
            apT += dt; apPortalT += dt;
            player.autoAttack = true;
            player.rect.x += SDL_cosf(apT * 0.6f) * 90.0f * dt;
            player.rect.y += SDL_sinf(apT * 0.85f) * 90.0f * dt;
            if (apPortalT > 6.0f && pendingPortal < 0)   // harita geçiş stresi
            {
                apPortalT = 0.0f;
                pendingPortal = (map_current() == MAP_DOGU) ? 1 : 0;
            }
        }

        // ÖLÜM EKRANI: dünya donuk, sadece geri sayım (bitince köyde diril)
        if (deathTimer > 0.0f)
        {
            deathTimer -= dt;
            if (deathTimer <= 0.0f)
            {
                // WIPE: ölüm dünyayı tazeler — TÜM yaratıklar tam canla başa döner
                // (boss dahil: yarım bırakılan savaş kaldığı yerden sürmez)
                player.rect.x = SPAWN_X;
                player.rect.y = SPAWN_Y;
                player.health = player.maxHealth;
                player.state = PLAYER_IDLE;
                player.attacking = false;
                player.attackHit = false;
                player.targetMobId = 0;

                if (map_current() != MAP_WORLD)
                {
                    // Zindanda öldün: köye dönüş = dünya haritasına geçiş.
                    // Geçiş bloğu zindan moblarını zaten temizler; boss bir
                    // sonraki girişte TAM canla yeniden doğar (ölmediyse gün şartı yok).
                    map_set_current(MAP_WORLD);
                    mapReloadNeeded = true;
                }
                else
                {
                    // Dünyada öldün: kamplar MEVCUT popülasyonla tazelenir
                    // (tam can + konum; ölüler ekolojide zaten düşüldü)
                    for (int i = 0; i < MAX_MOBS; i++) mobs[i] = (Mob){0};
                    spawnZoneCount = ecology_populate(spawnZones, MAX_SPAWN_ZONES,
                                                      mobs, MAX_MOBS, &nextMobId);
                }
                for (int i = 0; i < MAX_MOB_PROJS; i++) mobProjs[i].active = false;
            }
        }

        // Vinyet sahnesi: dünya bir nefeslik durur (Sessiz Gün'ün küçük kopyası).
        // Yalnız sahne akar; E/Space basılıysa typewriter hızlanır (atlamaz).
        if (gameState == GAME_PLAYING && scene_active())
        {
            const bool *sk = SDL_GetKeyboardState(NULL);
            scene_update(dt, sk[SDL_SCANCODE_E] || sk[SDL_SCANCODE_SPACE]);
            if (!scene_active())
                activeEcho = -1;   // sahne kapandı — müzik duck'ı motor geri açtı
        }

        // !uieditor_active(): yerleşim düzenleyicisi açıkken oyuncu girdisi DURUR.
        // Olayı yutmak yetmiyor — hareket ve saldırı SDL_GetKeyboardState ile
        // anlık durumdan okunuyor, yani olay döngüsünü baypas ediyor. Kapı
        // burada kapanmasa düzenleyicide 'A' (çapa) tuşu karakteri sola yürütür,
        // 'S' (kaydet) aşağı indirirdi.
        if (gameState == GAME_PLAYING && deathTimer <= 0.0f && !scene_active() &&
            !uieditor_active())
        {

            const bool *keys = SDL_GetKeyboardState(NULL); // basılı tuşların anlık görüntüsü (SDL'e ait, free etme)

            // --- HARİTA GEÇİŞİ: E-geçit ya da yükleme sonrası harita değişimi ---
            if (pendingPortal >= 0 || mapReloadNeeded)
            {
                MapId to = mapReloadNeeded ? map_current() : PORTALS[pendingPortal].to;

                // 1) Haritayı yükle + minimap'ı yeni boyuta kur
                if (!tilemap_load_csv(MAPS[to].csvPath))
                    generate_map();   // dosya kayıpsa test haritası (oyun ölmesin)
                minimap_set_map(renderer, to == MAP_WORLD);

                // 2) Eski haritanın canlıları/ganimeti gider (dünya, dönüşte tazelenir —
                //    save zaten dünya durumunu tutmuyor, davranış tutarlı)
                for (int i = 0; i < MAX_MOBS; i++) mobs[i] = (Mob){0};
                for (int i = 0; i < MAX_LOOT; i++) loots[i] = (Loot){0};
                openLootIndex = -1;
                player.targetMobId = 0;
                player.autoAttack = false;
                gatherRes = -1;
                fx_init(&weaponFx);
    fx_set_death_hook(fx_on_particle_death);
    skillfx_init();   // partiküller haritayla gitsin
                decal_reset();        // yer izleri de haritaya ait

                // 3) Yeni haritanın kampları + sandıkları + tuzak durumu
                if (to == MAP_WORLD)
                    spawnZoneCount = ecology_populate(spawnZones, MAX_SPAWN_ZONES,
                                                      mobs, MAX_MOBS, &nextMobId);
                else
                {
                    // MÜHÜR İSTİSNASI: zindan ekolojiye dahil değil — ölüler
                    // "bitmez", her girişte eski usul tazelenir
                    spawnZoneCount = build_spawn_zones(spawnZones, to);
                    spawn_zones_fill(spawnZones, spawnZoneCount, mobs, MAX_MOBS, &nextMobId);
                }
                place_dungeon_chests(loots, to, gametime_day());
                trap_reset();
                lockpick_cancel();
                arenaClosed = false;   // kapı tile'ları CSV'den zaten zemin geldi

                // İskelet Lordu: haftada bir dirilir (öldüğü günden 7 oyun günü sonra)
                if (to == MAP_DUNGEON_F2 &&
                    (boss_last_kill_day() <= -999 ||
                     gametime_day() - boss_last_kill_day() >= 7))
                {
                    int bid = mob_spawn(mobs, MAX_MOBS, &nextMobId,
                                        &MOB_TYPES[15], T_BOSS_X, T_BOSS_Y, -1);
                    Mob *b = mob_find_by_id(mobs, MAX_MOBS, bid);
                    if (b != NULL) b->bossPhase = 1;
                }

                // SESSİZ EL (Suikastçi P1): ilk iniş — loca seni bekliyordu
                if (to == MAP_LOCA && player.storyFlags[SF_ASSASSIN] == 0)
                {
                    player.storyFlags[SF_ASSASSIN] = 1;
                    int celmeIdx2 = -1;
                    for (int ci = 0; ci < NPC_COUNT; ci++)
                        if (SDL_strcmp(NPCS[ci].id, "loca_tunel_gozcu") == 0)
                        { celmeIdx2 = ci; break; }
                    if (celmeIdx2 >= 0)
                        npc_bark(celmeIdx2,
                                 "Sonunda. Seni izliyorduk, aklayıcı.", 6.0f);
                }

                // ÖLÜM GETİREN: mezarlık boss'u — dev iskelet kalıntısının önü;
                // haftalık diriliş (boss2_* toleranslı save kuyruğu)
                if (to == MAP_MEZARLIK &&
                    (boss2_last_kill_day() <= -999 ||
                     gametime_day() - boss2_last_kill_day() >= 7))
                {
                    int bid2 = mob_spawn(mobs, MAX_MOBS, &nextMobId,
                                         &MOB_TYPES[MOB_BRINGER],
                                         30.5f * TILE_SIZE, 20.0f * TILE_SIZE, -1);
                    Mob *b2 = mob_find_by_id(mobs, MAX_MOBS, bid2);
                    if (b2 != NULL) { b2->bossPhase = 1; b2->bossTimer = 4.0f; }
                    for (int q = 0; q < 4; q++) deathBlast[q].on = false;
                }

                // 4) Oyuncu geçit hedefine düşer (yüklemede pos zaten save'den geldi)
                if (pendingPortal >= 0)
                {
                    if (map_is_dungeon(to))
                        journal_add_unique(JE_DUNGEON_FIRST, (int)to);
                    player.rect.x = PORTALS[pendingPortal].tx;
                    player.rect.y = PORTALS[pendingPortal].ty;
                }
                player.knockbackX = player.knockbackY = 0.0f;

                // 5) Dünya-özel sistemler kapanır/açılır + ambiyans
                npc_set_active(to == MAP_WORLD || to == MAP_LOCA ||
                               to == MAP_DOGU);
                npc_set_map((int)to);
                decor_set_active(to == MAP_WORLD || to == MAP_MEZARLIK ||
                                 to == MAP_LOCA || to == MAP_DOGU);
                decor_set_map((int)to);

                // SIKIŞMA EMNİYETİ (Faz EPIC-1, Hakan testi: "karakter
                // sıkışmış yürümüyor"): dünya değişir (dekor büyür, karo
                // katılaşır) ama kayıt konumu ESKİ dünyaya göre. Katı içinde
                // doğan oyuncu hiç kıpırdayamaz — en yakın boş noktaya sarmal
                // taramayla itilir. Kural HANDOVER'da yazılıydı ("eski save
                // konumu tuhaf düşebilir"); ERW dekor tabanları bunu ilk kez
                // gerçek bir tuzağa çevirdi.
                {
                    SDL_FRect pb = player.rect;
                    if (box_hits_solid(pb) || decor_blocks(pb)) {
                        bool freed = false;
                        for (int ring = 1; ring <= 24 && !freed; ring++) {
                            float rad = ring * 16.0f;
                            for (int a = 0; a < 16 && !freed; a++) {
                                float ang = (float)a * 0.3927f;   // 2π/16
                                SDL_FRect cand = player.rect;
                                cand.x = player.rect.x + SDL_cosf(ang) * rad;
                                cand.y = player.rect.y + SDL_sinf(ang) * rad;
                                if (!box_hits_solid(cand) && !decor_blocks(cand)) {
                                    player.rect.x = cand.x;
                                    player.rect.y = cand.y;
                                    freed = true;
                                }
                            }
                        }
                        if (freed) SDL_Log("oyuncu katidan cikarildi (%.0f,%.0f)",
                                           player.rect.x, player.rect.y);
                    }
                }
                map_set_current(to);
                // Bölgesel gündoğumu (DOGU-P2): doğu haritasında güneş erken
                gametime_set_sun_offset(MAPS[to].sunriseOffset);
                if (MAPS[to].name[0] != '\0')
                {
                    splashText = MAPS[to].name;
                    splashTimer = 3.0f;
                }
                currentRegion = -2;   // dünyaya dönüşte bölge splash'i yeniden tetiklensin
                boardOpen = false;    // köy panosu haritalar arası taşınmaz
                locaBoardOpen = false;
                sealWaveTimer = SEAL_WAVE_SECONDS;   // zindan girişinde dalga sıfırdan sayar
                sealWaveWarned = false;
                bool wasPortal = pendingPortal >= 0;
                pendingPortal = -1;
                mapReloadNeeded = false;

                // Otomatik kayıt (Faz B1): harita GEÇİŞİ kritik an sayılır
                // (yüklemeden gelen reload'da yazmaya gerek yok — durum aynı)
                if (wasPortal &&
                    save_game(save_slot_path(SAVE_SLOT_AUTO), &player, 
                              &inventory, &craftBag, &equipment, &hotbar, questStatus))
                {
                    save_read_summary(save_slot_path(SAVE_SLOT_AUTO),
                                      &slotSums[SAVE_SLOT_AUTO]);
                    SDL_Color dim = { 170, 170, 190, 255 };
                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                         player.rect.x + player.rect.w / 2.0f,
                                         player.rect.y - 24.0f,
                                         "Otomatik kayıt", dim);
                }
            }

            // Menzildeki geçit ([E] etiketi + kullanım için)
            nearPortal = map_portal_near(player.rect.x + player.rect.w / 2.0f,
                                         player.rect.y + player.rect.h / 2.0f);
            // SESSİZ EL kapısı: kuyudaki ip keşiften önce GİZLİ — koşul
            // (gece + 500g aklanmış) ya da loca zaten biliniyor
            if (nearPortal >= 0 && PORTALS[nearPortal].to == MAP_LOCA &&
                player.storyFlags[SF_ASSASSIN] == 0 &&
                !(gametime_is_night() && player.launderedTotal >= 500))
                nearPortal = -1;   // etiket de çıkmaz, E de işlemez

            // Menzildeki kapalı sandık ([E] ipucu + E ile açma)
            nearestChest = -1;
            {
                float pcx = player.rect.x + player.rect.w / 2.0f;
                float pcy = player.rect.y + player.rect.h / 2.0f;
                float best = 90.0f;
                for (int i = 0; i < MAX_LOOT; i++)
                {
                    if (!loots[i].active || loots[i].kind != LOOT_KIND_CHEST) continue;
                    if (loots[i].state != LOOT_CLOSED) continue;
                    float cdx = (loots[i].rect.x + loots[i].rect.w / 2.0f) - pcx;
                    float cdy = (loots[i].rect.y + loots[i].rect.h / 2.0f) - pcy;
                    float d = SDL_sqrtf(cdx * cdx + cdy * cdy);
                    if (d < best) { best = d; nearestChest = i; }
                }
            }

            // Menzildeki okunmamış yankı noktası ([E] ipucu + tetik)
            nearestEcho = -1;
            {
                float pcx = player.rect.x + player.rect.w / 2.0f;
                float pcy = player.rect.y + player.rect.h / 2.0f;
                float best = 70.0f;
                for (int i = 0; i < ECHO_COUNT; i++)
                {
                    if (ECHOES[i].map != map_current() || echo_done(i)) continue;
                    float edx = ECHOES[i].x - pcx, edy = ECHOES[i].y - pcy;
                    float d = SDL_sqrtf(edx * edx + edy * edy);
                    if (d < best) { best = d; nearestEcho = i; }
                }
            }

            // Menzildeki ritüel yuvası (E1): gölcük/mezar — tüm mobları ölüyken
            nearestRitual = (map_current() == MAP_WORLD)
                          ? ecology_ritual_near(player.rect.x + player.rect.w / 2.0f,
                                                player.rect.y + player.rect.h / 2.0f,
                                                mobs, MAX_MOBS)
                          : -1;

            // --- BOSS: faz mantığı + arena kapısı (yalnız K2) ---
            if (map_current() == MAP_DUNGEON_F2)
            {
                Mob *boss = NULL;
                for (int i = 0; i < MAX_MOBS; i++)
                    if (mobs[i].e.alive && mobs[i].type != NULL && mobs[i].type->isBoss)
                        { boss = &mobs[i]; break; }

                if (boss != NULL)
                {
                    // Faz 2 geçişi: can %50 — ok yağmuru + tek seferlik çağrı
                    if (boss->bossPhase == 1 &&
                        boss->e.health < boss->e.maxHealth * 0.5f)
                    {
                        boss->bossPhase = 2;
                        boss->bossTimer = 1.2f;
                        SDL_Color red = { 255, 120, 100, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             boss->e.rect.x + boss->e.rect.w / 2.0f,
                                             boss->e.rect.y - 40.0f,
                                             "\"Kemiklerim... KALKIN!\"", red);
                    }
                    if (boss->bossPhase == 2)
                    {
                        if (!boss->bossSummoned)
                        {
                            boss->bossSummoned = true;
                            for (int k = 0; k < 2; k++)
                            {
                                int sid = mob_spawn(mobs, MAX_MOBS, &nextMobId,
                                                    &MOB_TYPES[12],
                                                    boss->e.rect.x + (k ? 80.0f : -80.0f),
                                                    boss->e.rect.y, -1);
                                Mob *sm = mob_find_by_id(mobs, MAX_MOBS, sid);
                                if (sm != NULL) { sm->aggro = true; sm->suspicion = 1.0f; }
                            }
                        }
                        // Ok yağmuru: 3.5 sn'de bir 8 yöne (kaçılabilir)
                        boss->bossTimer -= dt;
                        if (boss->bossTimer <= 0.0f)
                        {
                            boss->bossTimer = 3.5f;
                            float bx = boss->e.rect.x + boss->e.rect.w / 2.0f;
                            float by = boss->e.rect.y + boss->e.rect.h / 2.0f;
                            for (int d8 = 0; d8 < 8; d8++)
                            {
                                float ang = (float)d8 * (6.2832f / 8.0f);
                                for (int pj = 0; pj < MAX_MOB_PROJS; pj++)
                                    if (!mobProjs[pj].active)
                                    {
                                        mobProjs[pj] = (MobProj){ .active = true,
                                            .x = bx, .y = by,
                                            .vx = SDL_cosf(ang) * 300.0f,
                                            .vy = SDL_sinf(ang) * 300.0f,
                                            .dmg = 16.0f, .kind = 0 };
                                        break;
                                    }
                            }
                            audio_play(SFX_REFLECT);
                        }
                    }
                }

                // Arena kapısı: içeri tam girince iner, boss ölünce/sen ölünce kalkar
                float pcy = player.rect.y + player.rect.h / 2.0f;
                float pcx = player.rect.x + player.rect.w / 2.0f;
                bool inArena = pcy > 19.6f * TILE_SIZE &&
                               pcx > 13.0f * TILE_SIZE && pcx < 32.0f * TILE_SIZE;
                if (!arenaClosed && boss != NULL && inArena)
                {
                    tilemap_set_tile(20, 18, 9);   // koridor girişine duvar iner
                    tilemap_set_tile(21, 18, 9);
                    arenaClosed = true;
                    audio_play(SFX_REFLECT);
                    SDL_Color red = { 255, 120, 100, 255 };
                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS, pcx,
                                         player.rect.y - 30.0f, "Kapı kapandı!", red);
                }
                // Boss düşünce kapı kalkar (ölümde zaten dünyaya çıkılır → wipe orada)
                if (arenaClosed && boss == NULL)
                {
                    tilemap_set_tile(20, 18, 8);   // duvar → zemin (koridor açılır)
                    tilemap_set_tile(21, 18, 8);
                    arenaClosed = false;
                }
            }

            // --- ÖLÜM GETİREN (mezarlık boss'u): 2 fazlı savaş ---
            // Faz 1: tırpan + 4 sn'de bir Kara Patlama (1 sn telegraph, kaçılır)
            // Faz 2 (%50): hızlanır + BİR KEZ 2 iskelet çağırır + patlama çift
            if (map_current() == MAP_MEZARLIK)
            {
                Mob *b2 = NULL;
                for (int i = 0; i < MAX_MOBS; i++)
                    if (mobs[i].e.alive && mobs[i].type == &MOB_TYPES[MOB_BRINGER])
                        { b2 = &mobs[i]; break; }

                float pcx2 = player.rect.x + player.rect.w / 2.0f;
                float pcy2 = player.rect.y + player.rect.h / 2.0f;

                if (b2 != NULL && b2->aggro)
                {
                    if (b2->bossPhase == 1 &&
                        b2->e.health < b2->e.maxHealth * 0.5f)
                    {
                        b2->bossPhase = 2;
                        b2->bossTimer = 1.0f;
                        SDL_Color purple = { 200, 140, 255, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             b2->e.rect.x + b2->e.rect.w / 2.0f,
                                             b2->e.rect.y - 50.0f,
                                             "\"Ölüm... hızlanır.\"", purple);
                    }
                    if (b2->bossPhase == 2)
                    {
                        // Hız buff'ı: slow kanalı hızlandırıcı olarak (her kare tazelenir)
                        b2->slowMult = 1.35f;
                        b2->slowTimer = 0.2f;
                        if (!b2->bossSummoned)
                        {
                            b2->bossSummoned = true;
                            for (int k = 0; k < 2; k++)
                            {
                                int sid = mob_spawn(mobs, MAX_MOBS, &nextMobId,
                                                    &MOB_TYPES[12],
                                                    b2->e.rect.x + (k ? 90.0f : -90.0f),
                                                    b2->e.rect.y, -1);
                                Mob *sm = mob_find_by_id(mobs, MAX_MOBS, sid);
                                if (sm != NULL) { sm->aggro = true; sm->suspicion = 1.0f; }
                            }
                        }
                    }
                    // Kara Patlama: oyuncunun altına telegraph bırak
                    b2->bossTimer -= dt;
                    if (b2->bossTimer <= 0.0f)
                    {
                        b2->bossTimer = (b2->bossPhase == 2) ? 3.2f : 4.0f;
                        int placed = 0;
                        for (int q = 0; q < 4 && placed < (b2->bossPhase == 2 ? 2 : 1); q++)
                        {
                            if (deathBlast[q].on) continue;
                            deathBlast[q].on = true;
                            deathBlast[q].t = 0.0f;
                            if (placed == 0) { deathBlast[q].x = pcx2; deathBlast[q].y = pcy2; }
                            else {   // faz 2 ikinci halka: kaçış yönünü kirletir
                                deathBlast[q].x = pcx2 + (randf() - 0.5f) * 220.0f;
                                deathBlast[q].y = pcy2 + (randf() - 0.5f) * 220.0f;
                            }
                            placed++;
                        }
                        audio_play(SFX_REFLECT);
                    }
                }
                // Telegraph'lar ilerler (boss ölse de aktif olanlar patlar)
                for (int q = 0; q < 4; q++)
                {
                    if (!deathBlast[q].on) continue;
                    deathBlast[q].t += dt;
                    if (deathBlast[q].t < 1.0f) continue;
                    deathBlast[q].on = false;
                    if (g_animFx != NULL)
                        animfx_spawn(g_animFx, darkburstTex, 16, 16, 48.0f, 64.0f,
                                     deathBlast[q].x, deathBlast[q].y - 16.0f,
                                     24.0f, 2.2f);
                    audio_play(SFX_THUNDER);
                    float bdx = pcx2 - deathBlast[q].x;
                    float bdy = pcy2 - deathBlast[q].y;
                    if (bdx * bdx + bdy * bdy <= 75.0f * 75.0f)
                    {
                        player.health -= 45.0f;
                        player.gaziTimer = 5.0f;
                        SDL_Color dark = { 190, 120, 255, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS, pcx2,
                                             player.rect.y - 24.0f,
                                             "Karanlık seni yuttu!", dark);
                    }
                }
            }

            // KUTSAL ALAN (Faz HOLY): çember yaşar — undead içindeyse yavaş +
            // saniyede bir ışık tick'i (yalnız undead yanar; canlılara nur zararsız)
            if (holyZone.on)
            {
                holyZone.t -= dt;
                holyZone.tick -= dt;
                if (holyZone.t <= 0.0f) holyZone.on = false;
                bool zoneTick = (holyZone.tick <= 0.0f);
                if (zoneTick) holyZone.tick = 1.0f;
                // Sığınak/IŞIK TAŞIYICISI: alanda duran oyuncuya nur damlar
                if (holyZone.on && perkFx.holyZoneSelfHeal > 0.0f)
                {
                    float sdx = (player.rect.x + player.rect.w / 2.0f) - holyZone.x;
                    float sdy = (player.rect.y + player.rect.h / 2.0f) - holyZone.y;
                    if (sdx * sdx + sdy * sdy <= holyZone.r * holyZone.r)
                    {
                        player.health += perkFx.holyZoneSelfHeal * dt;
                        if (player.health > player.maxHealth)
                            player.health = player.maxHealth;
                    }
                }
                for (int i = 0; i < mobCount && holyZone.on; i++)
                {
                    Mob *zm = &mobs[i];
                    if (!zm->e.alive || !zm->type->undead) continue;
                    float zdx = (zm->e.rect.x + zm->e.rect.w / 2.0f) - holyZone.x;
                    float zdy = (zm->e.rect.y + zm->e.rect.h / 2.0f) - holyZone.y;
                    if (zdx * zdx + zdy * zdy > holyZone.r * holyZone.r) continue;
                    zm->slowMult = 0.75f;   // nur bacaklara dolanır
                    zm->slowTimer = 0.4f;
                    if (!zoneTick) continue;
                    float td = ABILITIES[ABILITY_HOLY_ZONE].damageScale
                        * (1.0f + (float)(player.intelligence - 10) * 0.02f)
                        * (1.0f + (float)(player.skills[SKILL_LIGHT].level - 1) * 0.01f)
                        * perkFx.holyZoneTickMult * perkFx.holyDmgMult;
                    zm->e.health -= td;
                    zm->e.hitFlash = 0.12f;
                    zm->aggro = true;
                    floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                    zm->e.rect.x + zm->e.rect.w / 2.0f,
                                    zm->e.rect.y - 10.0f, (int)td, false);
                    if (g_animFx != NULL)
                        animfx_spawn(g_animFx, holyImpactTex, 7, 7, 32.0f, 32.0f,
                                     zm->e.rect.x + zm->e.rect.w / 2.0f,
                                     zm->e.rect.y + zm->e.rect.h / 2.0f, 24.0f, 1.2f);
                    float sxp = combat_xp_for_hit(td, zm->type->level,
                                    player.skills[SKILL_LIGHT].level);
                    if (skill_add_xp(player.skills, SKILL_LIGHT, sxp))
                        on_skill_levelup(&player, SKILL_LIGHT, &hotbar, floatTexts);
                    if (zm->e.health <= 0.0f)
                        mob_die(zm, mobs, MAX_MOBS, &nextMobId, loots,
                                questStatus, floatTexts, &player);
                }
            }

            // YANKI KOPYASI (P2): hedefin yanında süzülür, 1.2sn'de bir
            // senin vuruşunun %35'iyle vurur; süre bitince dağılır
            if (echoShadow.on)
            {
                echoShadow.t -= dt;
                if (echoShadow.t <= 0.0f)
                {
                    echoShadow.on = false;
                    fx_burst(&weaponFx, echoShadow.x, echoShadow.y,
                             14, 90.0f, 150, 90, 220);   // dağılma
                }
                else
                {
                    Mob *et = mob_find_by_id(mobs, mobCount, player.targetMobId);
                    float gx, gy;
                    if (et != NULL)
                    {
                        gx = et->e.rect.x + et->e.rect.w / 2.0f - 46.0f;
                        gy = et->e.rect.y + et->e.rect.h / 2.0f - 14.0f;
                    }
                    else
                    {
                        gx = player.rect.x + player.rect.w / 2.0f - 42.0f;
                        gy = player.rect.y + player.rect.h / 2.0f - 12.0f;
                    }
                    echoShadow.facingLeft = (gx > echoShadow.x + 2.0f) ? false
                                          : (gx < echoShadow.x - 2.0f) ? true
                                          : echoShadow.facingLeft;
                    echoShadow.x += (gx - echoShadow.x) * SDL_min(1.0f, dt * 4.0f);
                    echoShadow.y += (gy - echoShadow.y) * SDL_min(1.0f, dt * 4.0f);

                    echoShadow.atkT -= dt;
                    if (et != NULL && echoShadow.atkT <= 0.0f)
                    {
                        echoShadow.atkT = 1.2f;
                        HitResult eh = combat_player_hit(&player,
                            &equipment.slots[EQUIP_WEAPON], et->type->defense,
                            ABILITIES[ABILITY_ECHO_CALL].damageScale
                                * perkFx.echoDmgMult,
                            et->e.health / et->e.maxHealth, &perkFx);
                        et->e.health -= eh.damage;
                        strike_fx_mob(et, true, 0.0f);
                        et->e.hitFlash = 0.15f;
                        et->aggro = true;
                        floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                        et->e.rect.x + et->e.rect.w / 2.0f - 12.0f,
                                        et->e.rect.y, (int)eh.damage, eh.crit);
                        float sxp = combat_xp_for_hit(eh.damage, et->type->level,
                                        player.skills[SKILL_ECHO].level);
                        if (skill_add_xp(player.skills, SKILL_ECHO, sxp))
                            on_skill_levelup(&player, SKILL_ECHO,
                                             &hotbar, floatTexts);
                        if (et->e.health <= 0.0f)
                            mob_die(et, mobs, MAX_MOBS, &nextMobId, loots,
                                    questStatus, floatTexts, &player);
                    }
                }
            }

            // TAKİP uyarısı (Suikastçi P1): <100px'te oyalanırsan Kemal
            // şüphelenir — 3 uyarıda raporlar silinir (yeniden başlarsın)
            if (player.storyFlags[SF_ASSASSIN] == 4 &&
                map_current() == MAP_WORLD && kemalIdx >= 0)
            {
                float knx3, kny3;
                if (npc_visible_pos(kemalIdx, &knx3, &kny3))
                {
                    float kdx3 = (player.rect.x + player.rect.w / 2.0f) - knx3;
                    float kdy3 = (player.rect.y + player.rect.h / 2.0f) - kny3;
                    if (kdx3 * kdx3 + kdy3 * kdy3 < 100.0f * 100.0f &&
                        !player.sneaking && player.shadowTimer <= 0.0f)
                    {
                        tailNearTimer += dt;
                        if (tailNearTimer > 1.5f)
                        {
                            tailNearTimer = 0.0f;
                            tailWarnings++;
                            npc_bark(kemalIdx, tailWarnings >= 3
                                     ? "Sen... beni mi izliyorsun?!"
                                     : "Hep sen mi çıkıyorsun karşıma?", 4.0f);
                            if (tailWarnings >= 3)
                            {
                                tailWarnings = 0;
                                player.storyFlags[SF_TAIL_MARK] = 0;
                                SDL_Color red6 = { 255, 120, 100, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                    player.rect.x + player.rect.w / 2.0f,
                                    player.rect.y - 24.0f,
                                    "Takip bozuldu — raporlar silindi.", red6);
                            }
                        }
                    }
                    else if (tailNearTimer > 0.0f)
                        tailNearTimer -= dt * 0.5f;
                }
            }

            // İLK İMZA (Suikastçi P2): gece kervan yolunda hedef belirir
            bool taxContract = (player.contractActive >= 0 &&
                ASSASSIN_CONTRACTS[player.contractActive].targetMob == MOB_TAX_RUNNER &&
                player.contractProgress <
                    ASSASSIN_CONTRACTS[player.contractActive].count);
            if ((player.storyFlags[SF_ASSASSIN] == 11 || taxContract) &&
                map_current() == MAP_WORLD && gametime_is_night())
            {
                bool taxAlive = false;
                for (int i = 0; i < MAX_MOBS; i++)
                    if (mobs[i].e.alive &&
                        mobs[i].type == &MOB_TYPES[MOB_TAX_RUNNER])
                    { taxAlive = true; break; }
                static bool taxSpawnedTonight = false;
                if (!gametime_is_night()) taxSpawnedTonight = false;
                if (!taxAlive && !taxSpawnedTonight)
                {
                    taxSpawnedTonight = true;
                    mob_spawn(mobs, MAX_MOBS, &nextMobId,
                              &MOB_TYPES[MOB_TAX_RUNNER],
                              64.0f * TILE_SIZE, 19.0f * TILE_SIZE, -1);
                    mob_spawn(mobs, MAX_MOBS, &nextMobId,
                              &MOB_TYPES[MOB_TAX_GUARD],
                              63.0f * TILE_SIZE, 20.0f * TILE_SIZE, -1);
                    mob_spawn(mobs, MAX_MOBS, &nextMobId,
                              &MOB_TYPES[MOB_TAX_GUARD],
                              65.0f * TILE_SIZE, 20.0f * TILE_SIZE, -1);
                }
            }

            // Kilit minigame'i imleci
            lockpick_update(dt, &perkFx);

            // Diyalog typewriter'ı (metin akar, E/sayı tamamlar)
            if (dialogNpc >= 0)
                dialog_type_update(&npcDialogs[dialogNpc], dialogNode, dt);

            // Uzaklaşınca diyalog kapanır (Hakan isteği): NPC'den etkileşim
            // menzilinin ~2 katı uzaklaşınca konuşma kesilir
            if (dialogNpc >= 0)
            {
                float dnx2, dny2;
                if (npc_visible_pos(dialogNpc, &dnx2, &dny2))
                {
                    float ddx = (player.rect.x + player.rect.w / 2.0f) - dnx2;
                    float ddy = (player.rect.y + player.rect.h / 2.0f) - dny2;
                    float far = NPC_INTERACT_RANGE * 2.0f;
                    if (ddx * ddx + ddy * ddy > far * far)
                    {
                        npc_set_talking(-1);
                        dialogNpc = -1;
                    }
                }
                else
                    dialogNpc = -1;   // NPC görünmez oldu (içeri girdi/öldü)
            }

            // Tuzaklar: diken hasarı + plaka→arbalet atışları (yalnız zindanda)
            if (map_is_dungeon(map_current()))
            {
                TrapShot trapShots[4];
                int trapShotN = 0;
                SDL_FRect pFeet = feet_box(player.rect.x, player.rect.y,
                                           player.rect.w, player.rect.h);
                float spikeDmg = trap_update(dt, map_current(), pFeet,
                                             trapShots, 4, &trapShotN);
                if (spikeDmg > 0.0f)
                {
                    // Tuzak hasarı zırh delici (dikenler ayaktan girer)
                    player.health -= spikeDmg;
                    if (player.health < 0.0f) player.health = 0.0f;
                    flashTime = 0.35f;
                    audio_play(SFX_HIT);
                    char tbuf[FLOAT_TEXT_LEN];
                    snprintf(tbuf, sizeof(tbuf), "-%d", (int)spikeDmg);
                    SDL_Color red = { 255, 90, 70, 255 };
                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                         player.rect.x + player.rect.w / 2.0f,
                                         player.rect.y - 10.0f, tbuf, red);
                }
                for (int t = 0; t < trapShotN; t++)
                {
                    for (int pj = 0; pj < MAX_MOB_PROJS; pj++)
                        if (!mobProjs[pj].active)
                        {
                            mobProjs[pj] = (MobProj){ .active = true,
                                .x = trapShots[t].x, .y = trapShots[t].y,
                                .vx = trapShots[t].vx, .vy = trapShots[t].vy,
                                .dmg = trapShots[t].dmg, .kind = 0 };
                            break;
                        }
                    audio_play(SFX_REFLECT);   // kurulu yay boşalması (metalik)
                }
            }

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
                    bool needSpear = (ad->skill == SKILL_TWO_HANDED);
                    bool isSpell = (ad->skill == SKILL_DESTRUCTION ||
                                    ad->skill == SKILL_CONJURATION ||
                                    ad->skill == SKILL_ECHO ||
                                    ad->skill == SKILL_LIGHT);
                    bool isSummon = (hs->abilityId == ABILITY_SUMMON_SNAKE ||
                                     hs->abilityId == ABILITY_SUMMON_TURTLE);
                    AllyKind sumKind = (hs->abilityId == ABILITY_SUMMON_TURTLE)
                                     ? ALLY_TURTLE : ALLY_SNAKE;
                    if (isSummon && ally_active(allies, sumKind))
                    {
                        // Geri çağır: rezerve çözülür (maliyetsiz, anında)
                        ally_dismiss(allies, sumKind);
                        audio_play(SFX_UI);
                        SDL_Color dim2 = { 170, 200, 190, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f,
                                             "Dost öze döndü", dim2);
                        isSpell = false;   // cast akışına girme
                    }
                    SDL_Color red = { 255, 120, 100, 255 };

                    if (isSpell)
                    {
                        // BÜYÜ: silahtan bağımsız (battlemage özgürlüğü) — mana + hedef
                        float cost = ad->mana * perkFx.manaCostMult
                                   * (isSummon ? perkFx.summonCostMult : 1.0f)
                                   * (ad->skill == SKILL_LIGHT
                                      ? perkFx.holyManaMult : 1.0f);
                        bool needTarget = (hs->abilityId == ABILITY_LIFE_DRAIN ||
                                           hs->abilityId == ABILITY_HOLY_BEAM ||
                                           hs->abilityId == ABILITY_FIREBOLT ||
                                           hs->abilityId == ABILITY_LIGHTNING ||
                                           hs->abilityId == ABILITY_ICETACLE ||
                                           hs->abilityId == ABILITY_SNAKEBITE);
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
                        else if (ad->hpFrac > 0.0f &&
                                 player.health <= player.maxHealth * ad->hpFrac
                                                  * perkFx.echoCostMult + 5.0f)
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f,
                                                 "Kanın yetmiyor!", red);
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
                    else if (needSpear && !player.spearEquipped)
                    {
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f,
                                             "Mızrak gerekli!", red);
                    }
                    else if (!needSpear && needBow != player.bowEquipped)
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
                                 (needBow ? perkFx.bowStaminaMult
                                  : (needSpear && perkFx.spearStaminaMult > 0.0f)
                                          ? perkFx.spearStaminaMult : 1.0f))
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
                    else if (hs->abilityId == ABILITY_GATE_STANCE)
                    {
                        // KAPI TUTUCU (keystone): duruş AÇ/KAPA — yavaşlarsın ama
                        // menzile giren mob otomatik hedeflenip saplanır
                        player.gateStance = !player.gateStance;
                        player.abilityCd[ABILITY_GATE_STANCE] = 1.0f;
                        audio_play(SFX_UI);
                        SDL_Color st2 = { 255, 200, 120, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y - 10.0f,
                                             player.gateStance ? "Kapı tutuluyor."
                                                               : "Duruş bırakıldı.", st2);
                    }
                    else if (hs->abilityId == ABILITY_BACKSTAB &&
                             !player.attacking && !player.shooting)
                    {
                        // SIRTTAN BIÇAK: yalnız hedefin ARKASINDAN — pozisyon oyunu
                        Mob *ab = mob_find_by_id(mobs, mobCount, player.targetMobId);
                        float abd = 1e9f;
                        float pcx3 = player.rect.x + player.rect.w / 2.0f;
                        float pcy3 = player.rect.y + player.rect.h / 2.0f;
                        if (ab != NULL)
                        {
                            float adx = (ab->e.rect.x + ab->e.rect.w / 2.0f) - pcx3;
                            float ady = (ab->e.rect.y + ab->e.rect.h / 2.0f) - pcy3;
                            abd = SDL_sqrtf(adx * adx + ady * ady);
                        }
                        if (ab == NULL || abd > PLAYER_ATTACK_RANGE + 10.0f)
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS, pcx3,
                                                 player.rect.y - 10.0f,
                                                 ab == NULL ? "Hedef gerekli!"
                                                            : "Menzil dışı!", red);
                        else if (!player_behind_mob(ab, pcx3, pcy3))
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS, pcx3,
                                                 player.rect.y - 10.0f,
                                                 "Arkasından yaklaş!", red);
                        else
                        {
                            player.stamina -= ad->stamina
                                             * perkFx.assassinStaminaMult;
                            if (player.stamina < 0.0f) player.stamina = 0.0f;
                            player.abilityCd[ABILITY_BACKSTAB] = ad->cooldown;
                            const ItemStack *aw = &equipment.slots[EQUIP_WEAPON];
                            HitResult sh = combat_player_hit(&player, aw,
                                             ab->type->defense,
                                             ad->damageScale
                                                 + perkFx.backstabScaleBonus,
                                             ab->e.health / ab->e.maxHealth, &perkFx);
                            sh.damage *= perkFx.backstabDmgMult;
                            if (perkFx.backstabExecuteBonus > 0.0f &&
                                ab->e.health < ab->e.maxHealth * 0.25f)
                                sh.damage *= 1.0f + perkFx.backstabExecuteBonus;
                            player.forceCrit = false;
                            if (aw->type == &ITEM_TYPES[ITEM_DAGGER_UNSIGNED])
                                sh.damage *= 1.5f;   // İmzasız Bıçak arka pasifi
                            ab->e.health -= sh.damage;
                            strike_fx_mob(ab, false, 0.06f);
                            ab->e.hitFlash = 0.2f;
                            ab->aggro = true;
                            audio_play(SFX_CRIT);
                            floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                            ab->e.rect.x + ab->e.rect.w / 2.0f + 10.0f,
                                            ab->e.rect.y, (int)sh.damage, true);
                            float sxp = combat_xp_for_hit(sh.damage, ab->type->level,
                                            player.skills[SKILL_ASSASSIN].level);
                            if (skill_add_xp(player.skills, SKILL_ASSASSIN, sxp))
                                on_skill_levelup(&player, SKILL_ASSASSIN,
                                                 &hotbar, floatTexts);
                            if (ab->e.health <= 0.0f)
                                mob_die(ab, mobs, MAX_MOBS, &nextMobId, loots,
                                        questStatus, floatTexts, &player);
                        }
                    }
                    else if (hs->abilityId == ABILITY_SHADOWSTEP &&
                             !player.attacking && !player.shooting)
                    {
                        // GÖLGE ADIMI: hedefin arkasına ışınlan + garanti kritik
                        Mob *st2 = mob_find_by_id(mobs, mobCount, player.targetMobId);
                        float std2 = 1e9f;
                        float pcx4 = player.rect.x + player.rect.w / 2.0f;
                        float pcy4 = player.rect.y + player.rect.h / 2.0f;
                        if (st2 != NULL)
                        {
                            float sdx = (st2->e.rect.x + st2->e.rect.w / 2.0f) - pcx4;
                            float sdy = (st2->e.rect.y + st2->e.rect.h / 2.0f) - pcy4;
                            std2 = SDL_sqrtf(sdx * sdx + sdy * sdy);
                        }
                        if (st2 == NULL ||
                            std2 > 300.0f + perkFx.shadowstepRangeBonus)
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS, pcx4,
                                                 player.rect.y - 10.0f,
                                                 st2 == NULL ? "Hedef gerekli!"
                                                             : "Menzil dışı!", red);
                        else
                        {
                            player.stamina -= ad->stamina
                                             * perkFx.assassinStaminaMult;
                            if (player.stamina < 0.0f) player.stamina = 0.0f;
                            player.abilityCd[ABILITY_SHADOWSTEP] = ad->cooldown;
                            fx_burst(&weaponFx, pcx4, pcy4, 16, 100.0f, 120, 120, 140);
                            // Mobun bakışının TERSİNE, arkasına düş
                            float bx5 = st2->e.rect.x + st2->e.rect.w / 2.0f;
                            float by5 = st2->e.rect.y + st2->e.rect.h / 2.0f;
                            float ofx = 0.0f, ofy = 0.0f;
                            if (st2->e.facing == 2)
                                ofx = st2->e.facingLeft ? 44.0f : -44.0f;
                            else if (st2->e.facing == 1) ofy = 44.0f;
                            else ofy = -44.0f;
                            player.rect.x = bx5 + ofx - player.rect.w / 2.0f;
                            player.rect.y = by5 + ofy - player.rect.h / 2.0f;
                            player.forceCrit = true;   // sonraki vuruş oturur
                            fx_burst(&weaponFx, bx5 + ofx, by5 + ofy,
                                     16, 100.0f, 120, 120, 140);
                            audio_play(SFX_DODGE);
                            SDL_Color sgray = { 190, 190, 210, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 bx5 + ofx, by5 + ofy - 40.0f,
                                                 "Gölgeden çıktın.", sgray);
                        }
                    }
                    else if (hs->abilityId == ABILITY_SMOKE_BOMB)
                    {
                        // DUMAN PERDESİ: aggro sıfırlanır + 3sn görünmezlik
                        player.stamina -= ad->stamina
                                         * perkFx.assassinStaminaMult;
                        if (player.stamina < 0.0f) player.stamina = 0.0f;
                        player.abilityCd[ABILITY_SMOKE_BOMB] = ad->cooldown
                            - perkFx.smokeCdCut + perkFx.smokeCdPenalty;
                        player.smokeTimer = 3.0f + perkFx.smokeDurBonus;
                        if (perkFx.smokeFirstCrit)
                            player.forceCrit = true;   // HİÇ KİMSE: ilk vuruş oturur
                        player.autoAttack = false;
                        for (int m5 = 0; m5 < mobCount; m5++)
                        {
                            if (!mobs[m5].e.alive) continue;
                            mobs[m5].aggro = false;
                            mobs[m5].suspicion = 0.0f;
                        }
                        float pcx5 = player.rect.x + player.rect.w / 2.0f;
                        float pcy5 = player.rect.y + player.rect.h / 2.0f;
                        fx_burst(&weaponFx, pcx5, pcy5, 40, 140.0f, 160, 160, 170);
                        audio_play(SFX_DODGE);
                        SDL_Color smk = { 190, 190, 200, 255 };
                        floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS, pcx5,
                                             player.rect.y - 24.0f,
                                             "Dumana karıştın.", smk);
                    }
                    else if (hs->abilityId == ABILITY_NEEDLE_STING &&
                             !player.attacking && !player.shooting)
                    {
                        // İĞNE BATIR: doz biriktiren imza zehri (max 3 doz)
                        Mob *nt2 = mob_find_by_id(mobs, mobCount, player.targetMobId);
                        float ntd = 1e9f;
                        float pcx6 = player.rect.x + player.rect.w / 2.0f;
                        if (nt2 != NULL)
                        {
                            float ndx = (nt2->e.rect.x + nt2->e.rect.w / 2.0f) - pcx6;
                            float ndy = (nt2->e.rect.y + nt2->e.rect.h / 2.0f)
                                      - (player.rect.y + player.rect.h / 2.0f);
                            ntd = SDL_sqrtf(ndx * ndx + ndy * ndy);
                        }
                        bool hasNeedle = equipment.slots[EQUIP_SHIELD].type
                                         == &ITEM_TYPES[ITEM_POISON_NEEDLE];
                        if (!hasNeedle)
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS, pcx6,
                                                 player.rect.y - 10.0f,
                                                 "Zehirli İğne gerekli!", red);
                        else if (bag_count(&ITEM_TYPES[ITEM_NEEDLE_VENOM]) <= 0)
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS, pcx6,
                                                 player.rect.y - 10.0f,
                                                 "Zehir şarjın bitti — kazanda kaynat", red);
                        else if (nt2 == NULL || ntd > PLAYER_ATTACK_RANGE + 10.0f)
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS, pcx6,
                                                 player.rect.y - 10.0f,
                                                 nt2 == NULL ? "Hedef gerekli!"
                                                             : "Menzil dışı!", red);
                        else
                        {
                            player.stamina -= ad->stamina
                                             * perkFx.assassinStaminaMult;
                            if (player.stamina < 0.0f) player.stamina = 0.0f;
                            player.abilityCd[ABILITY_NEEDLE_STING] =
                                ad->cooldown - perkFx.stingCdCut;
                            bag_consume(&ITEM_TYPES[ITEM_NEEDLE_VENOM]);
                            if (nt2->venomDoses < 3) nt2->venomDoses++;
                            nt2->venomDoseDps = ad->damageScale * 0.75f
                                * (1.0f + (float)player.skills[SKILL_ALCHEMY].level
                                          * 0.15f * 0.1f)
                                * (1.0f + (float)player.skills[SKILL_ASSASSIN].level
                                          * 0.01f);
                            nt2->venomDoseDps *= perkFx.venomDpsMult;
                            nt2->venomTime = 6.0f + perkFx.venomDurBonus;
                            nt2->aggro = true;
                            strike_fx_mob(nt2, true, 0.0f);
                            audio_play(SFX_HIT);
                            char vbuf[FLOAT_TEXT_LEN];
                            snprintf(vbuf, sizeof(vbuf), "Zehir dozu %d/3",
                                     nt2->venomDoses);
                            SDL_Color vio9 = { 170, 100, 230, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                nt2->e.rect.x + nt2->e.rect.w / 2.0f,
                                nt2->e.rect.y - 26.0f, vbuf, vio9);
                        }
                    }
                    else if (hs->abilityId == ABILITY_HAMSTRING &&
                             !player.attacking && !player.shooting)
                    {
                        // KİRİŞ KESME: ×1.2 + kanatma + %40 yavaş (kaçanı yakalar)
                        Mob *ht2 = mob_find_by_id(mobs, mobCount, player.targetMobId);
                        float htd = 1e9f;
                        float pcx7 = player.rect.x + player.rect.w / 2.0f;
                        if (ht2 != NULL)
                        {
                            float hdx = (ht2->e.rect.x + ht2->e.rect.w / 2.0f) - pcx7;
                            float hdy = (ht2->e.rect.y + ht2->e.rect.h / 2.0f)
                                      - (player.rect.y + player.rect.h / 2.0f);
                            htd = SDL_sqrtf(hdx * hdx + hdy * hdy);
                        }
                        if (ht2 == NULL || htd > PLAYER_ATTACK_RANGE + 10.0f)
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS, pcx7,
                                                 player.rect.y - 10.0f,
                                                 ht2 == NULL ? "Hedef gerekli!"
                                                             : "Menzil dışı!", red);
                        else
                        {
                            player.stamina -= ad->stamina
                                             * perkFx.assassinStaminaMult;
                            if (player.stamina < 0.0f) player.stamina = 0.0f;
                            player.abilityCd[ABILITY_HAMSTRING] = ad->cooldown;
                            const ItemStack *hw = &equipment.slots[EQUIP_WEAPON];
                            HitResult sh = combat_player_hit(&player, hw,
                                             ht2->type->defense, ad->damageScale,
                                             ht2->e.health / ht2->e.maxHealth, &perkFx);
                            player.forceCrit = false;
                            ht2->e.health -= sh.damage;
                            ht2->bleedDps = sh.damage * 0.25f;
                            ht2->bleedTime = 4.0f;
                            ht2->slowMult = 0.6f;
                            ht2->slowTimer = 4.0f;
                            strike_fx_mob(ht2, false, 0.04f);
                            ht2->e.hitFlash = 0.15f;
                            ht2->aggro = true;
                            audio_play(SFX_HIT);
                            floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                            ht2->e.rect.x + ht2->e.rect.w / 2.0f + 10.0f,
                                            ht2->e.rect.y, (int)sh.damage, sh.crit);
                            float sxp = combat_xp_for_hit(sh.damage, ht2->type->level,
                                            player.skills[SKILL_ASSASSIN].level);
                            if (skill_add_xp(player.skills, SKILL_ASSASSIN, sxp))
                                on_skill_levelup(&player, SKILL_ASSASSIN,
                                                 &hotbar, floatTexts);
                            if (ht2->e.health <= 0.0f)
                                mob_die(ht2, mobs, MAX_MOBS, &nextMobId, loots,
                                        questStatus, floatTexts, &player);
                        }
                    }
                    else if (hs->abilityId == ABILITY_SHIELD_BASH &&
                             !player.attacking && !player.shooting)
                    {
                        // KALKAN VURUŞU (Block): kısa menzil tek hedef — asıl iş
                        // 2sn sersemletme (CC aracı). Kalkan ister.
                        bool hasSh = equipment.slots[EQUIP_SHIELD].type != NULL &&
                                     equipment.slots[EQUIP_SHIELD].type->weaponType
                                         == WEAPON_SHIELD;
                        Mob *bt = mob_find_by_id(mobs, mobCount, player.targetMobId);
                        float btd = 1e9f;
                        if (bt != NULL)
                        {
                            float bdx = (bt->e.rect.x + bt->e.rect.w / 2.0f)
                                      - (player.rect.x + player.rect.w / 2.0f);
                            float bdy = (bt->e.rect.y + bt->e.rect.h / 2.0f)
                                      - (player.rect.y + player.rect.h / 2.0f);
                            btd = SDL_sqrtf(bdx * bdx + bdy * bdy);
                        }
                        if (!hasSh)
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f,
                                                 "Kalkan gerekli!", red);
                        else if (bt == NULL || btd > PLAYER_ATTACK_RANGE + 10.0f)
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 10.0f,
                                                 bt == NULL ? "Hedef gerekli!"
                                                            : "Menzil dışı!", red);
                        else
                        {
                            player.stamina -= ad->stamina;
                            if (player.stamina < 0.0f) player.stamina = 0.0f;
                            player.abilityCd[ABILITY_SHIELD_BASH] = ad->cooldown;
                            const ItemStack *bw = &equipment.slots[EQUIP_WEAPON];
                            HitResult sh = combat_player_hit(&player, bw,
                                                             bt->type->defense,
                                                             ad->damageScale,
                                                             bt->e.health / bt->e.maxHealth,
                                                             &perkFx);
                            bt->e.health -= sh.damage;
                            strike_fx_mob(bt, false, 0.05f);
                            bt->e.hitFlash = 0.15f;
                            bt->aggro = true;
                            // SERSEMLETME: 2 sn — saldıramaz, sürünür
                            bt->slowMult = 0.05f;
                            bt->slowTimer = 2.0f;
                            bt->attackCooldown = 2.0f;
                            bt->alertFlash = 0.8f;
                            audio_play(SFX_REFLECT);
                            SDL_Color stc = { 255, 230, 120, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 bt->e.rect.x + bt->e.rect.w / 2.0f,
                                                 bt->e.rect.y - 26.0f,
                                                 "Sersemledi!", stc);
                            floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                            bt->e.rect.x + bt->e.rect.w / 2.0f + 10.0f,
                                            bt->e.rect.y, (int)sh.damage, sh.crit);
                            float sxp = combat_xp_for_hit(sh.damage, bt->type->level,
                                            player.skills[SKILL_BLOCK].level);
                            if (skill_add_xp(player.skills, SKILL_BLOCK, sxp + 6.0f))
                                on_skill_levelup(&player, SKILL_BLOCK,
                                                 &hotbar, floatTexts);
                            if (bt->e.health <= 0.0f)
                                mob_die(bt, mobs, MAX_MOBS, &nextMobId, loots,
                                        questStatus, floatTexts, &player);
                        }
                    }
                    else if (hs->abilityId == ABILITY_SWEEP &&
                             !player.attacking && !player.shooting)
                    {
                        // SÜPÜRME: bakış yönünde 140° yay — herkese ×1.0 + yavaş.
                        // Anında çözülür (Nova deseni) ama melee formülüyle vurur.
                        float fxv = (player.facing == DIR_LEFT)  ? -1.0f
                                  : (player.facing == DIR_RIGHT) ?  1.0f : 0.0f;
                        float fyv = (player.facing == DIR_UP)    ? -1.0f
                                  : (player.facing == DIR_DOWN)  ?  1.0f : 0.0f;
                        float pcx2 = player.rect.x + player.rect.w / 2.0f;
                        float pcy2 = player.rect.y + player.rect.h / 2.0f;
                        float swRange = PLAYER_SPEAR_RANGE + perkFx.spearRangeBonus;
                        float slowAmt = 0.30f * perkFx.sweepSlowMult;
                        if (slowAmt > 0.6f) slowAmt = 0.6f;
                        player.attacking = true;   // thrust animasyonu oynasın
                        player.state = PLAYER_THRUST;
                        player.currentFrame = 0;
                        player.animTimer = 0.0f;
                        player.stamina -= ad->stamina *
                            (perkFx.spearStaminaMult > 0.0f ? perkFx.spearStaminaMult : 1.0f);
                        if (player.stamina < 0.0f) player.stamina = 0.0f;
                        player.abilityCd[ABILITY_SWEEP] = ad->cooldown;
                        audio_play(SFX_HIT);
                        const ItemStack *swWpn = &equipment.slots[EQUIP_WEAPON];
                        for (int m2 = 0; m2 < mobCount; m2++)
                        {
                            Mob *nb = &mobs[m2];
                            if (!nb->e.alive) continue;
                            float ndx = (nb->e.rect.x + nb->e.rect.w / 2.0f) - pcx2;
                            float ndy = (nb->e.rect.y + nb->e.rect.h / 2.0f) - pcy2;
                            float nd = SDL_sqrtf(ndx * ndx + ndy * ndy);
                            if (nd > swRange || nd < 0.001f) continue;
                            // 140° yay: bakışla açı (cos 70° ≈ 0.34)
                            if ((ndx * fxv + ndy * fyv) / nd < 0.34f) continue;
                            float effDef2 = nb->type->defense;
                            if (swWpn->type != NULL && swWpn->type->armorPierce > 0.0f)
                                effDef2 *= 1.0f - swWpn->type->armorPierce
                                                - perkFx.pierceBonus;
                            HitResult sh = combat_player_hit(&player, swWpn, effDef2,
                                                             ad->damageScale,
                                                             nb->e.health / nb->e.maxHealth,
                                                             &perkFx);
                            sh.damage *= title_damage_mult(&player, nb->type,
                                                           gametime_is_night());
                            nb->e.health -= sh.damage;
                            strike_fx_mob(nb, false, 0.05f);
                            nb->e.hitFlash = 0.15f;
                            nb->slowMult = 1.0f - slowAmt;
                            nb->slowTimer = 2.0f;
                            nb->aggro = true;
                            floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                            nb->e.rect.x + nb->e.rect.w / 2.0f + 10.0f,
                                            nb->e.rect.y, (int)sh.damage, sh.crit);
                            float sxp = combat_xp_for_hit(sh.damage, nb->type->level,
                                            player.skills[SKILL_TWO_HANDED].level);
                            if (skill_add_xp(player.skills, SKILL_TWO_HANDED, sxp))
                                on_skill_levelup(&player, SKILL_TWO_HANDED,
                                                 &hotbar, floatTexts);
                            if (nb->e.health <= 0.0f)
                                mob_die(nb, mobs, MAX_MOBS, &nextMobId, loots,
                                        questStatus, floatTexts, &player);
                        }
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
                        bag_consume(hs->itemType))
                    {
                        // Akıcı: CD kısalır; İlk Yardım: iyileşme artar
                        player.itemCd[typeIdx] = hs->itemType->useCooldown * perkFx.potionCdMult;
                        audio_play(SFX_POTION);
                        char buf[FLOAT_TEXT_LEN];
                        SDL_Color green = { 120, 255, 120, 255 };

                        if (heals || manas)
                            title_notify(title_stat_add(&player, TS_POTIONS, 1),
                                         &player, floatTexts);
                        if (heals)
                        {
                            player.health += hs->itemType->healAmount
                                           * perkFx.potionEffMult
                                           * title_potion_mult(&player);
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
                        if (hs->itemType->foodBuffType != FB_NONE)
                        {
                            // Tok buffu (M2): TEK aktif — yenisi eskisini ezer
                            player.foodBuffType = hs->itemType->foodBuffType;
                            player.foodBuffPower = hs->itemType->foodBuffPower;
                            player.foodBuffTimer = hs->itemType->foodBuffDur
                                                  * title_food_dur_mult(&player);
                            player.foodBuffName = hs->itemType->name;
                            SDL_Color warm = { 255, 200, 120, 255 };
                            snprintf(buf, sizeof(buf), "Tok: %s", hs->itemType->name);
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 26.0f, buf, warm);
                        }
                        if (hs->itemType->rawRisk > 0.0f &&
                            randf() < hs->itemType->rawRisk)
                        {
                            // Çiğ yeme bedeli (M2): hafif zehirlenme
                            player.poisonDps = 1.5f;
                            player.poisonTimer = 8.0f;
                            SDL_Color pgreen = { 120, 230, 90, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 42.0f,
                                                 "Miden bulanıyor...", pgreen);
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
                        gather_finish(gatherRes, &player, &hotbar, floatTexts);
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
            if (locaBoardOpen && (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_A] ||
                                  keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_D]))
                locaBoardOpen = false;
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
                        float cost = sd->mana * perkFx.manaCostMult
                                   * ((castSpell == ABILITY_SUMMON_SNAKE ||
                                       castSpell == ABILITY_SUMMON_TURTLE)
                                      ? perkFx.summonCostMult : 1.0f)
                                   * (sd->skill == SKILL_LIGHT
                                      ? perkFx.holyManaMult : 1.0f);
                        const ItemStack *wpn = &equipment.slots[EQUIP_WEAPON];
                        float pcx = player.rect.x + player.rect.w / 2.0f;
                        float pcy = player.rect.y + player.rect.h / 2.0f;

                        if (!devInfinite) {
                            player.mana -= cost;
                            if (player.mana < 0.0f) player.mana = 0.0f;
                        }
                        player.abilityCd[castSpell] = sd->cooldown;
                        g_spellElem = spell_elem_of(castSpell);
                        // Nur Yolu CD ayarları: Rahibe kısar, IŞIK TAŞIYICISI bedeli
                        if (castSpell == ABILITY_HEAL_LIGHT)
                            player.abilityCd[castSpell] -= perkFx.healCdCut;
                        if (castSpell == ABILITY_HOLY_ZONE)
                            player.abilityCd[castSpell] += perkFx.holyZoneCdPenalty;
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
                                // Prosedürel mod: kendi mermi+patlama efektimiz
                                // (hasar/mermi mantığı DEĞİŞMİYOR — yalnız görsel)
                                if (skillfx_procedural()) {
                                    Mob *ft = mob_find_by_id(mobs, mobCount,
                                                             player.targetMobId);
                                    float fx2 = (ft != NULL)
                                        ? ft->e.rect.x + ft->e.rect.w / 2.0f : pcx;
                                    float fy2 = (ft != NULL)
                                        ? ft->e.rect.y + ft->e.rect.h / 2.0f : pcy - 120.0f;
                                    skillfx_cast(SKILLFX_FIREBOLT, pcx, pcy - 14.0f,
                                                 fx2, fy2, 1.0f);
                                }
                                break;
                            }
                            audio_play(SFX_HIT);
                        }
                        else if (castSpell == ABILITY_ICE_NOVA)
                        {
                            // ÇEVRENE buz patlaması: hasar + alan yavaşlatma
                            float novaR = 120.0f + perkFx.novaRadiusBonus;
                            // PROSEDÜREL SKİLL MOTORU (Ctrl+P açıkken): sprite
                            // yerine kendi efektimiz. Sprite yolu SİLİNMEDİ —
                            // anahtar kapalıyken eski görsel çalışıyor, yani
                            // A/B karşılaştırması her an mümkün.
                            if (skillfx_procedural()) {
                                // YER BÜYÜSÜ AYAK HİZASINDA doğar. pcy gövde
                                // MERKEZİ — halka oradan başlayınca zeminden ~28px
                                // yukarıda asılı kalıyor ve büyünün nereden çıktığı
                                // okunmuyor (Hakan: "çıkış-bitiş noktaları garip").
                                float pfy = player.rect.y + player.rect.h - 4.0f;
                                skillfx_cast(SKILLFX_ICE_NOVA, pcx, pfy, pcx, pfy,
                                             novaR / 120.0f);
                                skillfx_impact(pcx, pfy, 0.0f, -1.0f,
                                               SKFX_EL_ICE, 1.3f, true);
                            }
                            else
                                fx_burst(&weaponFx, pcx, pcy, 26, 150.0f, 150, 210, 255);
                            // BÜKÜLME (motor katmanı): buz kubbesi = MERCEK.
                            // Sistem ateş için kurulmuştu; ilk skill müşterisi
                            // burada — genel olup olmadığı ancak ikinci
                            // kullanıcıyla anlaşılır.
                            fx_distort_burst(pcx, pcy, R2D_DIST_LENS,
                                             novaR * 1.5f, 5.0f, 0.45f);
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
                                sh.damage *= title_damage_mult(&player, nb->type,
                                                               gametime_is_night());
                                sh.damage *= weather_ice_mult();   // buz kendi mevsiminde
                                nb->e.health -= sh.damage;
                                strike_fx_mob(nb, true, 0.0f);

                                // TERMAL ŞOK (M1): yanan hedefte nova — kalan yanma
                                // ANINDA patlar +%60 (yanma söner; buhar FX)
                                if (nb->burnTime > 0.0f)
                                {
                                    float shock = nb->burnDps * nb->burnTime * 1.6f;
                                    nb->e.health -= shock;
                                    nb->burnTime = 0.0f;
                                    fx_burst(&weaponFx,
                                             nb->e.rect.x + nb->e.rect.w / 2.0f,
                                             nb->e.rect.y + nb->e.rect.h / 2.0f,
                                             14, 130.0f, 220, 245, 255);
                                    SDL_Color steam = { 200, 240, 255, 255 };
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                        nb->e.rect.x + nb->e.rect.w / 2.0f,
                                        nb->e.rect.y - 34.0f, "Termal Şok!", steam);
                                    floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                        nb->e.rect.x + nb->e.rect.w / 2.0f - 14.0f,
                                        nb->e.rect.y - 16.0f, (int)shock, true);
                                }
                                nb->e.hitFlash = 0.15f;
                                nb->slowMult = 0.5f;
                                nb->slowTimer = 4.0f * weather_slow_dur_mult();
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
                                if (skillfx_procedural()) {
                                    skillfx_cast(SKILLFX_LIGHTNING, tx, ty, tx, ty, 1.0f);
                                    // Yıldırım GÖKTEN iner: darbe yönü aşağı
                                    skillfx_impact(tx, ty, 0.0f, 1.0f,
                                                   SKFX_EL_BOLT, 1.6f, true);
                                }
                                else
                                    fx_distort_burst(tx, ty, R2D_DIST_SHOCK,
                                                     170.0f, 7.0f, 0.34f);
                                HitResult sh = combat_spell_hit(&player, wpn,
                                                                sd->damageScale,
                                                                lt->type->defense, &perkFx);
                                sh.damage *= title_damage_mult(&player, lt->type,
                                                               gametime_is_night());
                                sh.damage *= weather_bolt_mult();   // ıslak iletken
                                lt->e.health -= sh.damage;
                                strike_fx_mob(lt, true, 0.0f);

                                // ZEHİR PATLAMASI (M1): zehirli hedefe yıldırım —
                                // kalan zehir ANINDA + 90px çevreye %50'si (yeşil bulut)
                                if (lt->poisonTime > 0.0f)
                                {
                                    float burst = lt->poisonDps * lt->poisonTime;
                                    lt->e.health -= burst;
                                    lt->poisonTime = 0.0f;
                                    fx_burst(&weaponFx, tx, ty, 18, 110.0f, 120, 235, 90);
                                    SDL_Color pgreen = { 140, 245, 110, 255 };
                                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                        tx, lt->e.rect.y - 34.0f, "Zehir Patlaması!", pgreen);
                                    floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                        tx - 14.0f, lt->e.rect.y - 16.0f, (int)burst, true);
                                    for (int m3 = 0; m3 < mobCount; m3++)
                                    {
                                        Mob *pb = &mobs[m3];
                                        if (!pb->e.alive || pb == lt) continue;
                                        float pdx = (pb->e.rect.x + pb->e.rect.w / 2.0f) - tx;
                                        float pdy = (pb->e.rect.y + pb->e.rect.h / 2.0f) - ty;
                                        if (pdx * pdx + pdy * pdy > 90.0f * 90.0f) continue;
                                        pb->e.health -= burst * 0.5f;
                                        pb->e.hitFlash = 0.15f;
                                        pb->aggro = true;
                                        if (pb->e.health <= 0.0f)
                                            mob_die(pb, mobs, MAX_MOBS, &nextMobId, loots,
                                                    questStatus, floatTexts, &player);
                                    }
                                }
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
                            if (skillfx_procedural()) {
                                // Dikenler ZEMİNDEN çıkar — gövde merkezinden
                                // değil ayak hizasından (bkz. buz novası notu)
                                float pfy = player.rect.y + player.rect.h - 4.0f;
                                skillfx_cast(SKILLFX_SPIKES, pcx, pfy, pcx, pfy, 1.0f);
                                // Dikenler YERDEN çıkar: darbe yönü yukarı
                                skillfx_impact(pcx, pfy, 0.0f, -1.0f,
                                               SKFX_EL_EARTH, 1.4f, true);
                            }
                            else
                                fx_distort_burst(pcx, pcy, R2D_DIST_SHOCK,
                                                 spikeR * 2.0f, 8.5f, 0.5f);
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
                                strike_fx_mob(nb, true, 0.0f);
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

                        else if (castSpell == ABILITY_LIGHTNING_CLAW)
                        {
                            // YILDIRIM PENÇESİ: yüzünün dönük olduğu yöne 90px koni —
                            // büyücünün yakın savunması. Islakta +%20 (iletken).
                            float fxv = (player.facing == DIR_LEFT)  ? -1.0f
                                      : (player.facing == DIR_RIGHT) ?  1.0f : 0.0f;
                            float fyv = (player.facing == DIR_UP)    ? -1.0f
                                      : (player.facing == DIR_DOWN)  ?  1.0f : 0.0f;
                            for (int q = 0; q < MAX_CLAW_FX; q++)
                                if (!clawFx[q].active)
                                {
                                    clawFx[q] = (SpikeFxAnim){ true,
                                        pcx + fxv * 60.0f, pcy + fyv * 60.0f, 0.0f };
                                    break;
                                }
                            audio_play(SFX_CRIT);   // çatırtı hissi
                            for (int m2 = 0; m2 < mobCount; m2++)
                            {
                                Mob *nb = &mobs[m2];
                                if (!nb->e.alive) continue;
                                float ndx = (nb->e.rect.x + nb->e.rect.w / 2.0f) - pcx;
                                float ndy = (nb->e.rect.y + nb->e.rect.h / 2.0f) - pcy;
                                float nd = SDL_sqrtf(ndx * ndx + ndy * ndy);
                                if (nd > 110.0f || nd < 0.001f) continue;
                                // Koni: bakış yönüyle hizalı olmalı (yaklaşık 90°)
                                float dot = (ndx * fxv + ndy * fyv) / nd;
                                if (dot < 0.35f) continue;
                                HitResult sh = combat_spell_hit(&player, wpn,
                                                                sd->damageScale,
                                                                nb->type->defense, &perkFx);
                                sh.damage *= title_damage_mult(&player, nb->type,
                                                               gametime_is_night());
                                sh.damage *= weather_bolt_mult();   // ıslak iletken
                                nb->e.health -= sh.damage;
                                strike_fx_mob(nb, true, 0.0f);
                                nb->e.hitFlash = 0.15f;
                                nb->aggro = true;
                                mob_alert_camp(mobs, mobCount, nb);
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
                        else if (castSpell == ABILITY_ICETACLE)
                        {
                            // BUZ DİKENLERİ: hedefin bulunduğu alanda (100px) yerden
                            // buz — hasar + %50 yavaş 3sn. Karda +%20, yavaş ×1.5.
                            Mob *it = mob_find_by_id(mobs, mobCount, player.targetMobId);
                            if (it != NULL)
                            {
                                float itx = it->e.rect.x + it->e.rect.w / 2.0f;
                                float ity = it->e.rect.y + it->e.rect.h / 2.0f;
                                for (int q = 0; q < MAX_ICETACLE_FX; q++)
                                    if (!icetacleFx[q].active)
                                    {
                                        icetacleFx[q] = (SpikeFxAnim){ true, itx, ity, 0.0f };
                                        break;
                                    }
                                audio_play(SFX_DODGE);   // buz whoosh
                                for (int m2 = 0; m2 < mobCount; m2++)
                                {
                                    Mob *nb = &mobs[m2];
                                    if (!nb->e.alive) continue;
                                    float ndx = (nb->e.rect.x + nb->e.rect.w / 2.0f) - itx;
                                    float ndy = (nb->e.rect.y + nb->e.rect.h / 2.0f) - ity;
                                    if (ndx * ndx + ndy * ndy > 100.0f * 100.0f) continue;
                                    HitResult sh = combat_spell_hit(&player, wpn,
                                                                    sd->damageScale,
                                                                    nb->type->defense, &perkFx);
                                    sh.damage *= title_damage_mult(&player, nb->type,
                                                                   gametime_is_night());
                                    sh.damage *= weather_ice_mult();   // buz kendi mevsiminde
                                    nb->e.health -= sh.damage;
                                    strike_fx_mob(nb, true, 0.0f);
                                    nb->e.hitFlash = 0.15f;
                                    nb->slowMult = 0.5f;
                                    nb->slowTimer = 3.0f * weather_slow_dur_mult();
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
                        }
                        else if (castSpell == ABILITY_TORNADO)
                        {
                            // HORTUM: bakış yönüne süzülen alan — 0.5sn'de bir tick
                            // hasarı + %30 yavaş. Fırtınada tick +%20, ömür +1sn.
                            float fxv = (player.facing == DIR_LEFT)  ? -1.0f
                                      : (player.facing == DIR_RIGHT) ?  1.0f : 0.0f;
                            float fyv = (player.facing == DIR_UP)    ? -1.0f
                                      : (player.facing == DIR_DOWN)  ?  1.0f : 0.0f;
                            HitResult sh = combat_spell_hit(&player, wpn,
                                                            sd->damageScale,
                                                            0.0f, &perkFx);   // defans tick'te
                            bool stormy = (weather_current() == WEATHER_STORM &&
                                           weather_intensity() > 0.5f);
                            for (int q = 0; q < MAX_TORNADOES; q++)
                                if (!tornadoes[q].active)
                                {
                                    tornadoes[q] = (Tornado){
                                        .active = true,
                                        .x = pcx + fxv * 40.0f, .y = pcy + fyv * 40.0f,
                                        .dirX = fxv, .dirY = fyv,
                                        .t = 0.0f,
                                        .life = stormy ? 5.0f : 4.0f,
                                        .tickTimer = 0.5f,
                                        .tickDamage = sh.damage * (stormy ? 1.2f : 1.0f),
                                    };
                                    break;
                                }
                            audio_play(SFX_DODGE);   // rüzgar whoosh
                        }
                        else if (castSpell == ABILITY_WHISPER_BURST)
                        {
                            // FISILTI PATLAMASI: bedel KAN (max HP %10) —
                            // çevrene gölge dalgası; Yankı seviyesi büyütür
                            float hpCost = player.maxHealth * sd->hpFrac
                                         * perkFx.echoCostMult;
                            player.health -= hpCost;
                            if (player.health < 1.0f) player.health = 1.0f;
                            for (int q = 0; q < MAX_DARKBURST_FX; q++)
                                if (!darkburstFx[q].active)
                                {
                                    darkburstFx[q] = (SpikeFxAnim){ true, pcx, pcy, 0.0f };
                                    break;
                                }
                            audio_play(SFX_WHISPER);
                            float wbR = 130.0f + perkFx.whisperRadiusBonus;
                            float wbDmg = sd->damageScale
                                        * (1.0f + (float)player.skills[SKILL_ECHO].level * 0.02f)
                                        * perkFx.echoDmgMult;
                            for (int m2 = 0; m2 < mobCount; m2++)
                            {
                                Mob *nb = &mobs[m2];
                                if (!nb->e.alive) continue;
                                float ndx = (nb->e.rect.x + nb->e.rect.w / 2.0f) - pcx;
                                float ndy = (nb->e.rect.y + nb->e.rect.h / 2.0f) - pcy;
                                if (ndx * ndx + ndy * ndy > wbR * wbR) continue;
                                float fd = wbDmg * (100.0f / (100.0f + nb->type->defense));
                                nb->e.health -= fd;
                                strike_fx_mob(nb, true, 0.0f);
                                nb->e.hitFlash = 0.15f;
                                nb->aggro = true;
                                floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                                nb->e.rect.x + nb->e.rect.w / 2.0f + 10.0f,
                                                nb->e.rect.y, (int)fd, false);
                                float sxp = combat_xp_for_hit(fd, nb->type->level,
                                                player.skills[SKILL_ECHO].level);
                                if (skill_add_xp(player.skills, SKILL_ECHO, sxp))
                                    on_skill_levelup(&player, SKILL_ECHO,
                                                     &hotbar, floatTexts);
                                if (nb->e.health <= 0.0f)
                                    mob_die(nb, mobs, MAX_MOBS, &nextMobId, loots,
                                            questStatus, floatTexts, &player);
                            }
                        }
                        else if (castSpell == ABILITY_SHADOW_FORM)
                        {
                            // GÖLGE FORMU: 5sn algılanmaz + hızlı — kanla öder
                            player.shadowTimer = 5.0f + perkFx.shadowDurBonus;
                            audio_play(SFX_WHISPER);
                            fx_burst(&weaponFx, pcx, pcy, 22, 120.0f, 150, 90, 220);
                            SDL_Color vio4 = { 200, 160, 255, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 pcx, player.rect.y - 24.0f,
                                                 "Gölgeye karıştın.", vio4);
                        }
                        else if (castSpell == ABILITY_LIFE_DRAIN)
                        {
                            // CAN EMME (P2): hedefe 4sn emiş DoT'u — çalınan
                            // her tik SANA akar (DoT bölümü işler)
                            Mob *dt2 = mob_find_by_id(mobs, mobCount, player.targetMobId);
                            if (dt2 != NULL)
                            {
                                float hpCost = player.maxHealth * sd->hpFrac
                                             * perkFx.echoCostMult;
                                player.health -= hpCost;
                                if (player.health < 1.0f) player.health = 1.0f;
                                dt2->drainDps = sd->damageScale
                                    * (1.0f + (float)player.skills[SKILL_ECHO].level * 0.02f)
                                    * perkFx.drainMult * perkFx.echoDmgMult;
                                dt2->drainTime = 4.0f + perkFx.drainDurBonus;
                                dt2->aggro = true;
                                mob_alert_camp(mobs, mobCount, dt2);
                                if (g_animFx != NULL)
                                    animfx_spawn(g_animFx, darkburstTex, 16, 16,
                                                 48.0f, 64.0f,
                                                 dt2->e.rect.x + dt2->e.rect.w / 2.0f,
                                                 dt2->e.rect.y + dt2->e.rect.h / 2.0f,
                                                 20.0f, 1.2f);
                                audio_play(SFX_WHISPER);
                                SDL_Color vio5 = { 200, 160, 255, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                    dt2->e.rect.x + dt2->e.rect.w / 2.0f,
                                    dt2->e.rect.y - 24.0f, "Damar açıldı.", vio5);
                            }
                        }
                        else if (castSpell == ABILITY_ECHO_CALL)
                        {
                            // YANKI ÇAĞIRMA (P2): gölge kopyan belirir —
                            // hedefinin yanında süzülür, vuruşunun %35'iyle vurur
                            float hpCost = player.maxHealth * sd->hpFrac
                                         * perkFx.echoCostMult;
                            player.health -= hpCost;
                            if (player.health < 1.0f) player.health = 1.0f;
                            echoShadow.on = true;
                            echoShadow.t = 12.0f + perkFx.echoCallDurBonus;
                            echoShadow.atkT = 0.8f;
                            echoShadow.x = pcx - 40.0f;
                            echoShadow.y = pcy - 10.0f;
                            audio_play(SFX_WHISPER);
                            fx_burst(&weaponFx, echoShadow.x, echoShadow.y,
                                     18, 100.0f, 150, 90, 220);
                            SDL_Color vio6 = { 200, 160, 255, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS, pcx,
                                                 player.rect.y - 24.0f,
                                                 "Yankın bedenlendi.", vio6);
                        }
                        else if (castSpell == ABILITY_HEAL_LIGHT)
                        {
                            // ŞİFA NURU: kendini iyileştir — INT + Kutsal ölçekler.
                            // XP = şifa edilen HP ×1.2 (taşan kısım saymaz)
                            float healBase = sd->damageScale
                                * (1.0f + (float)(player.intelligence - 10) * 0.02f)
                                * (1.0f + (float)(player.skills[SKILL_LIGHT].level - 1)
                                          * 0.01f)
                                * perkFx.healMult;
                            float before = player.health;
                            player.health += healBase;
                            if (player.health > player.maxHealth)
                                player.health = player.maxHealth;
                            float healed = player.health - before;
                            if (g_animFx != NULL)
                                animfx_spawn(g_animFx, holyBurstTex, 16, 16,
                                             48.0f, 48.0f,
                                             player.rect.x + player.rect.w / 2.0f,
                                             player.rect.y + player.rect.h / 2.0f,
                                             24.0f, 1.6f);
                            audio_play(SFX_POTION);
                            SDL_Color hgold = { 255, 235, 150, 255 };
                            char hbuf[24];
                            snprintf(hbuf, sizeof(hbuf), "+%d", (int)healed);
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 16.0f, hbuf, hgold);
                            if (healed > 0.5f &&
                                skill_add_xp(player.skills, SKILL_LIGHT, healed * 1.2f))
                                on_skill_levelup(&player, SKILL_LIGHT,
                                                 &hotbar, floatTexts);
                        }
                        else if (castSpell == ABILITY_HOLY_BEAM)
                        {
                            // KUTSAL HÜZME: gökten inen ışık — undead'e ×1.6
                            Mob *ht = mob_find_by_id(mobs, mobCount, player.targetMobId);
                            if (ht != NULL)
                            {
                                float tx = ht->e.rect.x + ht->e.rect.w / 2.0f;
                                float ty = ht->e.rect.y + ht->e.rect.h / 2.0f;
                                HitResult sh = combat_spell_hit(&player, wpn,
                                                                sd->damageScale,
                                                                ht->type->defense, &perkFx);
                                sh.damage *= title_damage_mult(&player, ht->type,
                                                               gametime_is_night());
                                sh.damage *= perkFx.holyDmgMult;
                                if (ht->type->undead)
                                    sh.damage *= 1.6f + perkFx.holyUndeadBonus;
                                ht->e.health -= sh.damage;
                                strike_fx_mob(ht, true, 0.0f);
                                ht->e.hitFlash = 0.2f;
                                ht->aggro = true;
                                mob_alert_camp(mobs, mobCount, ht);
                                if (g_animFx != NULL)
                                {
                                    animfx_spawn(g_animFx, holyBurstTex, 16, 16,
                                                 48.0f, 48.0f, tx, ty - 12.0f,
                                                 30.0f, 2.2f);   // ışık sütunu
                                    animfx_spawn(g_animFx, holyImpactTex, 7, 7,
                                                 32.0f, 32.0f, tx, ty, 24.0f, 1.6f);
                                }
                                audio_play(SFX_CRIT);
                                floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                                tx + 10.0f, ht->e.rect.y,
                                                (int)sh.damage, sh.crit);
                                float sxp = combat_xp_for_hit(sh.damage,
                                                ht->type->level,
                                                player.skills[SKILL_LIGHT].level);
                                if (skill_add_xp(player.skills, SKILL_LIGHT, sxp))
                                    on_skill_levelup(&player, SKILL_LIGHT,
                                                     &hotbar, floatTexts);
                                if (ht->e.health <= 0.0f)
                                    mob_die(ht, mobs, MAX_MOBS, &nextMobId, loots,
                                            questStatus, floatTexts, &player);
                            }
                        }
                        else if (castSpell == ABILITY_HOLY_ZONE)
                        {
                            // KUTSAL ALAN: ayakların altında 6sn ışık çemberi —
                            // içindeki undead yavaşlar + saniyede tick yer.
                            // Tek çember: yeniden atım eskisini taşır.
                            holyZone.on = true;
                            holyZone.x = player.rect.x + player.rect.w / 2.0f;
                            holyZone.y = player.rect.y + player.rect.h / 2.0f;
                            holyZone.t = 6.0f + perkFx.holyZoneDur;
                            holyZone.tick = 1.0f;
                            holyZone.r = 120.0f + perkFx.holyZoneRadius;
                            if (g_animFx != NULL)
                                animfx_spawn(g_animFx, holyBurstTex, 16, 16,
                                             48.0f, 48.0f, holyZone.x, holyZone.y,
                                             24.0f, 2.6f);
                            audio_play(SFX_PERK);
                        }
                        else if (castSpell == ABILITY_SNAKEBITE)
                        {
                            // YILAN ISIRIĞI: hedefe küçük vuruş + ağır zehir DoT
                            // (vuruşun %60'ı 4 sn'ye yayılır — kanalla ÜST ÜSTE biner).
                            // Zehir Patlaması (M1) artık salt-büyüyle kurulabilir.
                            Mob *sb = mob_find_by_id(mobs, mobCount, player.targetMobId);
                            if (sb != NULL)
                            {
                                float sx2 = sb->e.rect.x + sb->e.rect.w / 2.0f;
                                float sy2 = sb->e.rect.y + sb->e.rect.h / 2.0f;
                                HitResult sh = combat_spell_hit(&player, wpn,
                                                                sd->damageScale,
                                                                sb->type->defense, &perkFx);
                                sh.damage *= title_damage_mult(&player, sb->type,
                                                               gametime_is_night());
                                sb->e.health -= sh.damage;
                                strike_fx_mob(sb, true, 0.0f);
                                sb->e.hitFlash = 0.15f;
                                sb->aggro = true;
                                mob_alert_camp(mobs, mobCount, sb);
                                // Zehir kanalı: mevcut DoT'a EKLENİR (tam build meşru)
                                sb->poisonDps += sh.damage * 0.6f / 4.0f;
                                sb->poisonTime = 4.0f;
                                // Yön animasyonu: oyuncudan hedefe bakış
                                float sdx2 = sx2 - pcx, sdy2 = sy2 - pcy;
                                int row; bool flip = false;
                                if (SDL_fabsf(sdx2) > SDL_fabsf(sdy2))
                                { row = 2; flip = (sdx2 > 0); }   // side sola bakar
                                else row = (sdy2 < 0) ? 1 : 0;
                                for (int q = 0; q < MAX_SNAKEBITE_FX; q++)
                                    if (!snakeFx[q].active)
                                    {
                                        snakeFx[q] = (SnakeFxAnim){ true, sx2, sy2,
                                                                    0.0f, row, flip };
                                        break;
                                    }
                                audio_play(SFX_HIT);
                                floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                                sx2 + 10.0f, sb->e.rect.y,
                                                (int)sh.damage, sh.crit);
                                float sxp = combat_xp_for_hit(sh.damage, sb->type->level,
                                                player.skills[SKILL_DESTRUCTION].level);
                                if (skill_add_xp(player.skills, SKILL_DESTRUCTION, sxp))
                                    on_skill_levelup(&player, SKILL_DESTRUCTION,
                                                     &hotbar, floatTexts);
                                if (sb->e.health <= 0.0f)
                                    mob_die(sb, mobs, MAX_MOBS, &nextMobId, loots,
                                            questStatus, floatTexts, &player);
                            }
                        }
                        else if (castSpell == ABILITY_FIRESURGE)
                        {
                            // ATEŞ ASLANI: bakış yönüne hat dalgası — 300px koşar,
                            // çarptığını yakar (tek dalga: mob başına 1 vuruş).
                            // Islakta -%15 (fire mult) — Termal Şok'u besler.
                            float fxv = (player.facing == DIR_LEFT)  ? -1.0f
                                      : (player.facing == DIR_RIGHT) ?  1.0f : 0.0f;
                            float fyv = (player.facing == DIR_UP)    ? -1.0f
                                      : (player.facing == DIR_DOWN)  ?  1.0f : 0.0f;
                            HitResult sh = combat_spell_hit(&player, wpn,
                                                            sd->damageScale,
                                                            0.0f, &perkFx);
                            // PROSEDÜREL: sprite aslanın yerine koşan alev
                            // duvarı. Oyun tarafı (hasar, hareket, çarpışma)
                            // DEĞİŞMİYOR — yalnız görsel yol ayrılıyor.
                            if (skillfx_procedural()) {
                                // GÖRSEL ayak hizasından: aslan zeminde koşar.
                                // Oyun tarafı (hasar küresi) pcy'de kalıyor —
                                // o kürenin yeri aslanın gövdesinin içinde.
                                float pfy2 = player.rect.y + player.rect.h - 4.0f;
                                skillfx_cast(SKILLFX_FIRE_WAVE,
                                             pcx + fxv * 30.0f, pfy2 + fyv * 20.0f,
                                             fxv, fyv, 1.0f);
                            }
                            for (int q = 0; q < MAX_FIRESURGES; q++)
                                if (!firesurges[q].active)
                                {
                                    firesurges[q] = (FireSurge){
                                        .active = true,
                                        .x = pcx + fxv * 30.0f, .y = pcy + fyv * 30.0f,
                                        .dirX = fxv, .dirY = fyv,
                                        .t = 0.0f,
                                        .damage = sh.damage * weather_fire_mult(),
                                        .hitN = 0,
                                        .facing = (fyv > 0) ? 0 : (fyv < 0) ? 1
                                                : (fxv < 0) ? 2 : 3,
                                    };
                                    break;
                                }
                            audio_play(SFX_CRIT);   // kükreme yerine çatırtı
                        }
                        else if (castSpell == ABILITY_SUMMON_SNAKE ||
                                 castSpell == ABILITY_SUMMON_TURTLE)
                        {
                            // M4: dost çağırma — tek dost kuralı (keystone hariç)
                            AllyKind k = (castSpell == ABILITY_SUMMON_TURTLE)
                                       ? ALLY_TURTLE : ALLY_SNAKE;
                            AllyKind other = (k == ALLY_SNAKE) ? ALLY_TURTLE
                                                               : ALLY_SNAKE;
                            if (!perkFx.twoSummons && ally_active(allies, other))
                                ally_dismiss(allies, other);   // yenisi eskisini gönderir
                            ally_summon(allies, k, pcx, pcy,
                                        player.skills[SKILL_CONJURATION].level,
                                        perkFx.summonHpMult);
                            fx_burst(&weaponFx, pcx, pcy, 20, 100.0f, 120, 235, 110);
                            audio_play(SFX_PERK);
                            SDL_Color grn = { 140, 240, 130, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 pcx, player.rect.y - 10.0f,
                                                 k == ALLY_SNAKE ? "Öz Yılanı!"
                                                                 : "Öz Kabuğu!", grn);
                        }

                        // ── YOL GÖSTERGESİ (yalnız A/B kıyas modunda).
                        // ÜÇ TUR kaybettik çünkü Hakan "alev skili" derken
                        // ATEŞ ASLANI'nı deniyordu; o büyü sprite yolundaydı ve
                        // Ctrl+P ona hiç dokunmuyordu — bense her turda
                        // prosedürel ateş topunun biçimini düzeltiyordum.
                        // Bundan sonra oyun hangi yolun çalıştığını SÖYLÜYOR.
                        // Prosedürel artık varsayılan olduğundan kapı
                        // devSkillfxAB: Ctrl+P'ye hiç basılmadıysa susar
                        // (normal oyunda gürültü olmasın), basıldıysa iki
                        // yönde de konuşur.
                        if (devSkillfxAB && castSpell >= 0 &&
                            castSpell < ABILITY_COUNT) {
                            static char pathMsg[96];
                            bool proc = skillfx_procedural() &&
                                        (castSpell == ABILITY_FIREBOLT ||
                                         castSpell == ABILITY_ICE_NOVA ||
                                         castSpell == ABILITY_LIGHTNING ||
                                         castSpell == ABILITY_SPIKES ||
                                         castSpell == ABILITY_FIRESURGE);
                            SDL_snprintf(pathMsg, sizeof pathMsg, "%s -> %s",
                                         ABILITIES[castSpell].name,
                                         proc ? "PROSEDUREL (motor)"
                                              : "SPRITE (motor disi)");
                            splashText = pathMsg;
                            splashTimer = 2.0f;
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
                sh.damage *= weather_fire_mult();   // ıslak dünyada ateş boğulur
                ft->e.health -= sh.damage;
                strike_fx_mob(ft, true, 0.0f);
                ft->e.hitFlash = 0.15f;
                ft->aggro = true;
                mob_alert_camp(mobs, mobCount, ft);
                audio_play(sh.crit ? SFX_CRIT : SFX_HIT);
                fx_burst(&weaponFx, tx, ty, 12, 100.0f, 255, 150, 50);

                // YANMA: 3. bağımsız DoT kanalı (Közleme/Yangın düğümleri büyütür).
                // Yağmur yakmayı erken söndürür (süre ×0.5'e kadar iner)
                ft->burnDps = sh.damage * 0.30f * (1.0f + perkFx.burnBonus) / 3.0f;
                ft->burnTime = 3.0f * weather_burn_time_mult();

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
                        if (bag_consume_n(&ITEM_TYPES[ITEM_WOOD],
                                                CAMPFIRE_WOOD_COST))
                        {
                            // Yenisi eskisini söndürür (tek aktif oyuncu ateşi)
                            STATIONS[STATION_PLAYER_FIRE_IDX].x =
                                player.rect.x + player.rect.w / 2.0f;
                            STATIONS[STATION_PLAYER_FIRE_IDX].y =
                                player.rect.y + player.rect.h + 8.0f;   // ayak önü

                            // Ömür: şu andan 24 oyun saati sonra söner
                            // (karda 12 — soğuk odunu hızlı yer)
                            int cfDay; float cfHours;
                            gametime_get(&cfDay, &cfHours);
                            campfireOutAt = (float)cfDay * 24.0f + cfHours +
                                            (weather_is_snowy() ? 12.0f : 24.0f);
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
                            bag_consume_n(&ITEM_TYPES[fr->in1Item],
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
                            craft_finish_recipe(craftRecipe, &player,
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
                                bag_consume(&ITEM_TYPES[runeItem]))
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
                                bag_consume_n(&ITEM_TYPES[stone[s]], next) &&
                                bag_consume_n(
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
                                       : player.spearEquipped
                                         ? PLAYER_SPEAR_RANGE + perkFx.spearRangeBonus
                                         : PLAYER_ATTACK_RANGE;
                        if (SDL_sqrtf(adx * adx + ady * ady) <= atkRange)
                        {
                            attackPressed = true;
                            player.pendingAbility = -1;   // normal vuruş
                            player.attackTimer = combat_attack_interval(&equipment.slots[EQUIP_WEAPON], &perkFx);
                            // İkiz Bıçak (sol el): ana el hançerse +%15 hız
                            if (equipment.slots[EQUIP_SHIELD].type ==
                                    &ITEM_TYPES[ITEM_DAGGER_OFFHAND] &&
                                equipment.slots[EQUIP_WEAPON].type ==
                                    &ITEM_TYPES[ITEM_DAGGER_UNSIGNED])
                                player.attackTimer *= 0.85f;
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
                                   : player.spearEquipped
                                     ? PLAYER_SPEAR_RANGE + perkFx.spearRangeBonus
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
                                 ? perkFx.bowStaminaMult
                                 : (ABILITIES[player.pendingAbility].skill == SKILL_TWO_HANDED
                                    && perkFx.spearStaminaMult > 0.0f)
                                 ? perkFx.spearStaminaMult : 1.0f);
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
            // DEV geçersiz kılma (Ctrl+G) — silah atamasının ÜSTÜNE yazar
            if (devFxOverride != WEAPON_FX_COUNT)
                player.weaponFx = devFxOverride;
            // Yay kuşanımı: attackPressed slash yerine SHOOT başlatsın (player_update)
            player.bowEquipped = (wpnStack->type != NULL &&
                                  wpnStack->type->weaponType == WEAPON_BOW);
            player.staffEquipped = (wpnStack->type != NULL &&
                                    wpnStack->type->weaponType == WEAPON_STAFF);
            player.spearEquipped = (wpnStack->type != NULL &&
                                    wpnStack->type->thrustOversize);
            if (!player.spearEquipped) player.gateStance = false;   // duruş mızrakla yaşar
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
                {   // Ten tonu: base katman rengi (yaratma/yükleme farkı olmasın)
                    static const Uint8 tones[3][3] = {
                        { 255, 255, 255 }, { 225, 190, 155 }, { 185, 140, 105 } };
                    int sk2 = (player.skinTone >= 0 && player.skinTone < 3)
                            ? player.skinTone : 0;
                    for (int L = 0; L < LAYER_COUNT; L++)
                        r2d_texture_set_color_mod(layers[L].tex,
                            tones[sk2][0], tones[sk2][1], tones[sk2][2]);
                }
                player.strength     = 10 + gearStat[STAT_STR]
                                    + (player.background == 1 ? 2 : 0);   // Asker
                player.dexterity    = 10 + gearStat[STAT_DEX]
                                    + (player.background == 2 ? 2 : 0);   // Avcı
                player.intelligence = 10 + gearStat[STAT_INT]
                                    + (player.background == 3 ? 2 : 0);   // Çırak

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
                float newMaxHp = (BASE_MAX_HP + perkFx.maxHpBonus
                                  + (float)gearStat[STAT_HP]) * perkFx.echoHpMult;
                if (newMaxHp > player.maxHealth)
                    player.health += newMaxHp - player.maxHealth;
                player.maxHealth = newMaxHp;
                if (player.health > player.maxHealth) player.health = player.maxHealth;

                player.maxMana    = 50.0f  + (float)gearStat[STAT_MP]
                                  + perkFx.maxManaBonus
                                  + title_max_mana_bonus(&player);   // Kayıt Tutan
                // M4 rezerve: dost yaşarken max mananın bir kısmı BAĞLI kalır
                {
                    float reserve = perkFx.summonReserve
                                  * (float)ally_count(allies);
                    if (reserve > 0.9f) reserve = 0.9f;
                    player.manaReservedFrac = reserve;   // bar kilitli bölme çizer
                    player.maxMana *= (1.0f - reserve);
                    if (player.mana > player.maxMana)
                        player.mana = player.maxMana;
                }
                player.maxStamina = 100.0f + (float)gearStat[STAT_STA]
                                  + perkFx.maxStaminaBonus;
                // Takı hasar çarpanı: Ölüm Getiren'in Nişanı +%10 (melee+büyü)
                player.trinketDmgMult =
                    (equipment.slots[EQUIP_NECKLACE].type ==
                     &ITEM_TYPES[ITEM_DEATH_SIGIL]) ? 1.10f : 1.0f;
                // (DEF ekipmanı equipment_total_defense içinde toplanır)
            }

            // Cine sırasında oyuncu çakılı: boş tuş tablosu (dünya akmaya devam eder)
            static const bool cineNoKeys[SDL_SCANCODE_COUNT] = { false };
            player_update(&player, cineActive ? cineNoKeys : keys,
                          cineActive ? false : attackPressed, dt, &weaponFx);

            // SLASH YAYI (VFX cilası): savurma başladığı karede bakış yönünde
            // yay efekti — yalnız kılıç savurması (thrust/mızrakta yok)
            if (player.attacking && !prevAttacking &&
                player.state == PLAYER_SLASH && g_animFx != NULL)
            {
                float acx = player.rect.x + player.rect.w / 2.0f
                          + WEAPON_FX_OFFSET[player.facing].dx * 1.6f;
                float acy = player.rect.y + player.rect.h / 2.0f
                          + WEAPON_FX_OFFSET[player.facing].dy * 1.6f;
                animfx_spawn(g_animFx, slashArcTex[player.facing], 10, 10,
                             64.0f, 64.0f, acx, acy, 30.0f, 1.5f);
            }
            prevAttacking = player.attacking;

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
                        title_notify(title_stat_add(&player, TS_ARROWS, 1),
                                     &player, floatTexts);
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
                            title_notify(title_stat_add(&player, TS_ARROWS, 1),
                                         &player, floatTexts);
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
            if (player.foodBuffTimer > 0.0f)
            {
                player.foodBuffTimer -= dt;
                if (player.foodBuffTimer <= 0.0f)
                {
                    player.foodBuffType = FB_NONE;
                    SDL_Color dim = { 170, 170, 190, 255 };
                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                         player.rect.x + player.rect.w / 2.0f,
                                         player.rect.y - 10.0f, "Acıktın", dim);
                }
            }
            float foodManaMul = (player.foodBuffType == FB_MANA_REGEN)
                              ? player.foodBuffPower : 1.0f;
            player.mana += 2.0f * perkFx.manaRegenMult * armorManaMult
                         * foodManaMul * dt;
            if (player.mana > player.maxMana) player.mana = player.maxMana;
            float foodStaMul = (player.foodBuffType == FB_STA_SPEED)
                             ? player.foodBuffPower : 1.0f;
            player.stamina += 15.0f * perkFx.stamRegenMult * foodStaMul * dt;
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
            // ── ATEŞ YAYICILARI (VFX ateş paketi). Kaynaklar dünyada zaten
            // var: istasyonlar (kamp ateşi / ocak / kazan) ve kandil direkleri.
            // Ayrı emitter kaydı TUTULMUYOR — konumlar burada okunuyor, yani
            // istasyon taşınınca ya da kandil sönünce senkron borcu doğmaz.
            // Yalnız DÜNYA haritasında ve ekrana yakın kaynaklar işlenir.
            // BÜKÜLME ALANLARI (genel motor katmanı). Bu listeyi ateş kaynakları
            // dolduruyor AMA sistem onlara özel değil: skill (portal burgacı),
            // boss (yer vuruşu şok dalgası), doğa olayı (çöl serabı, volkan) ve
            // kalkan kubbesi (mercek) aynı diziye yazacak. Kare başına yenilenir.
            R2DDistort distorts[R2D_MAX_DISTORT];
            int distortN = 0;
            // DUVAR saati: efekt nefesi oyun saatinden bağımsız (ateş
            // geceleyin yavaşlamaz, mevsim onu söndürmez)
            float nowSec = (float)SDL_GetTicks() / 1000.0f;
            // ── RÜZGÂR (kuvvet alanı): hava sisteminden türüyor. Yön yavaş
            // döner (gün içinde kayar), güç yağmur/fırtına şiddetiyle artar.
            // Motor tarafı genel: patlama itmesi ya da boss savurması da aynı
            // alana yazabilir (o an rüzgârın üstüne yazılır).
            {
                float wAng = nowSec * 0.035f + 0.7f;
                float wStr = 12.0f + 46.0f * weather_intensity();
                fx_set_wind(SDL_cosf(wAng) * wStr, SDL_sinf(wAng) * wStr * 0.4f);
            }
            if (!map_is_dungeon(map_current())) {
                float exW = (float)winW / camera.zoom, exH = (float)winH / camera.zoom;
                float vx0 = camera.x - 96.0f, vy0 = camera.y - 96.0f;
                float vx1 = camera.x + exW + 96.0f, vy1 = camera.y + exH + 96.0f;
                for (int i = 0; i < STATION_COUNT; i++) {
                    float sxw = STATIONS[i].x, syw = STATIONS[i].y;
                    if (sxw < vx0 || sxw > vx1 || syw < vy0 || syw > vy1) continue;
                    // CAMPFIRE_PARKED NEGATİF (-99999): karşılaştırma >= idi ve
                    // TÜM kaynakları park sayıp atlıyordu — hiç partikül doğmadı.
                    if (sxw <= CAMPFIRE_PARKED + 1.0f) continue;   // sönük/park
                    switch (STATIONS[i].type) {
                        case STATION_CAMPFIRE: {
                            // NEFES: parçacık yoğunluğu VE ısı genliği aynı
                            // değerden türüyor — alev kabardığında havanın
                            // titremesi de kabarıyor. Işık da aynı fonksiyonu
                            // kullanacak (light.c tarafı sıradaki iş).
                            // Yağmur ateşi bastırır: hasar tarafında çarpan
                            // zaten vardı (weather_fire_mult), görsel tarafı yoktu.
                            float wet = weather_is_wet()
                                      ? (1.0f - 0.45f * weather_intensity()) : 1.0f;
                            float br = fx_breath(FIRE_HEARTH, (unsigned)i, nowSec) * wet;
                            fx_emit_fire(&weaponFx, sxw, syw - 14.0f,
                                         FIRE_HEARTH, br, dt);
                            if (distortN < R2D_MAX_DISTORT)
                                distorts[distortN++] = (R2DDistort){
                                    .x = (sxw - camera.x) * camera.zoom,
                                    .y = (syw - 26.0f - camera.y) * camera.zoom,
                                    .radius = 86.0f * camera.zoom,
                                    .strength = 3.4f * br * camera.zoom,
                                    .style = R2D_DIST_HEAT, .freq = 1.0f,
                                    .phase = (float)i * 0.37f, .aniso = 1.0f };
                            break; }
                        case STATION_SMITH: {
                            // Ocak kapalı mekânda sayılır: yağmurdan az etkilenir
                            float wetF = weather_is_wet()
                                       ? (1.0f - 0.15f * weather_intensity()) : 1.0f;
                            float br = fx_breath(FIRE_FORGE, (unsigned)i, nowSec) * wetF;
                            fx_emit_fire(&weaponFx, sxw, syw - 18.0f,
                                         FIRE_FORGE, 0.85f * br, dt);
                            if (distortN < R2D_MAX_DISTORT)
                                distorts[distortN++] = (R2DDistort){
                                    .x = (sxw - camera.x) * camera.zoom,
                                    .y = (syw - 34.0f - camera.y) * camera.zoom,
                                    .radius = 74.0f * camera.zoom,
                                    .strength = 4.2f * br * camera.zoom,   // ocak daha sıcak
                                    .style = R2D_DIST_HEAT, .freq = 1.35f,
                                    .phase = (float)i * 0.61f, .aniso = 1.0f };
                            break; }
                        case STATION_ALCH:
                            fx_emit_fire(&weaponFx, sxw, syw - 22.0f,
                                         FIRE_CAULDRON, 1.0f, dt);
                            if (distortN < R2D_MAX_DISTORT)
                                distorts[distortN++] = (R2DDistort){
                                    .x = (sxw - camera.x) * camera.zoom,
                                    .y = (syw - 30.0f - camera.y) * camera.zoom,
                                    .radius = 58.0f * camera.zoom,
                                    .strength = 2.2f * camera.zoom,   // buhar hafif
                                    .style = R2D_DIST_HEAT, .freq = 0.8f,
                                    .phase = (float)i * 0.23f, .aniso = 1.0f };
                            break;
                        default: break;
                    }
                }
                // Kandiller: yalnız YANANLAR (lamp_lit) ve gece daha güçlü
                float lampStr = 0.55f + 0.45f * gametime_darkness01();
                for (int i = 0; i < LAMP_COUNT; i++) {
                    if (!lamp_lit(i)) continue;
                    float lx = LAMPS[i].x, ly = LAMPS[i].y;
                    if (lx < vx0 || lx > vx1 || ly < vy0 || ly > vy1) continue;
                    fx_emit_fire(&weaponFx, lx, ly - 46.0f,
                                 FIRE_CANDLE, lampStr, dt);
                }
            // ── İZ SIZDIRMASI: iz taşıyan moblar partikül üretir. Ton değişimi
            // "durum var" der, partikül "durum SÜRÜYOR" der; ikisi birlikte
            // olmadan yanan mob yalnız turuncu bir sprite gibi duruyor.
            // Sızdırma İZİN GÜCÜYLE oranlı (iz solarken azalıyor) ve yalnız
            // görüş alanındaki moblar için — ekran dışında partikül harcamak
            // 5120 tavanı boşa yiyor.
            for (int i = 0; i < mobCount; i++) {
                Mob *mm = &mobs[i];
                if (!mm->e.alive || mm->mark == MOB_MARK_NONE) continue;
                if (mm->markTimer <= 0.0f || mm->markMax <= 0.0f) continue;
                float mcx = mm->e.rect.x + mm->e.rect.w / 2.0f;
                float mcy = mm->e.rect.y + mm->e.rect.h / 2.0f;
                if (mcx < vx0 || mcx > vx1 || mcy < vy0 || mcy > vy1) continue;
                float k = mm->markTimer / mm->markMax;          // 1 → 0
                float hw = mm->e.rect.w * 0.45f, hh = mm->e.rect.h * 0.42f;
                switch (mm->mark) {
                case MOB_MARK_FIRE:
                    // YANIYOR: gövdeden alev dili + kızıl zarf + is
                    fx_set_pal_mix(0, 2, 0.45f);
                    if (randf() < 26.0f * k * dt)
                        fx_spawn(&weaponFx, mcx + (randf() * 2.0f - 1.0f) * hw,
                                 mcy + (randf() * 2.0f - 1.0f) * hh,
                                 (randf() * 2.0f - 1.0f) * 14.0f, -70.0f - randf() * 50.0f,
                                 0.32f + randf() * 0.26f, 9.0f + randf() * 7.0f,
                                 255, 196, 96, WEAPON_FX_LICK);
                    if (randf() < 14.0f * k * dt)
                        fx_spawn(&weaponFx, mcx + (randf() * 2.0f - 1.0f) * hw,
                                 mcy + (randf() * 2.0f - 1.0f) * hh,
                                 (randf() * 2.0f - 1.0f) * 10.0f, -46.0f,
                                 0.30f + randf() * 0.20f, 17.0f + randf() * 10.0f,
                                 224, 82, 46, WEAPON_FX_FLAME_DEEP);
                    if (randf() < 7.0f * k * dt)
                        fx_spawn(&weaponFx, mcx + (randf() * 2.0f - 1.0f) * hw, mcy - hh,
                                 (randf() * 2.0f - 1.0f) * 12.0f, -30.0f,
                                 1.1f, 8.0f, 70, 64, 60, WEAPON_FX_SOOT);
                    fx_set_pal_variant(0);
                    break;
                case MOB_MARK_ICE:
                    // DONUYOR: kırağı buharı yukarı, kristal kırıntısı aşağı
                    if (randf() < 16.0f * k * dt)
                        fx_spawn(&weaponFx, mcx + (randf() * 2.0f - 1.0f) * hw,
                                 mcy + (randf() * 2.0f - 1.0f) * hh,
                                 (randf() * 2.0f - 1.0f) * 8.0f, -18.0f - randf() * 14.0f,
                                 0.9f + randf() * 0.6f, 10.0f + randf() * 8.0f,
                                 226, 242, 255, WEAPON_FX_STEAM);
                    if (randf() < 9.0f * k * dt)
                        fx_spawn(&weaponFx, mcx + (randf() * 2.0f - 1.0f) * hw,
                                 mcy + (randf() * 2.0f - 1.0f) * hh * 0.6f,
                                 (randf() * 2.0f - 1.0f) * 20.0f, 30.0f + randf() * 40.0f,
                                 0.5f + randf() * 0.4f, 7.0f + randf() * 6.0f,
                                 218, 240, 255, WEAPON_FX_ICE);
                    break;
                case MOB_MARK_BOLT:
                    // ÇARPILDI: yüklü toprak boşalır gibi tek tük kıvılcım
                    if (randf() < 30.0f * k * dt)
                        fx_spawn(&weaponFx, mcx + (randf() * 2.0f - 1.0f) * hw,
                                 mcy + (randf() * 2.0f - 1.0f) * hh,
                                 (randf() * 2.0f - 1.0f) * 60.0f, -80.0f - randf() * 60.0f,
                                 0.22f, 3.0f, 255, 250, 235, WEAPON_FX_SPARK);
                    break;
                case MOB_MARK_EARTH:
                    // TAŞ YEDİ: gövdeden kırıntı ve toz dökülür (AŞAĞI)
                    if (randf() < 18.0f * k * dt)
                        fx_spawn(&weaponFx, mcx + (randf() * 2.0f - 1.0f) * hw,
                                 mcy + (randf() * 2.0f - 1.0f) * hh * 0.5f,
                                 (randf() * 2.0f - 1.0f) * 26.0f, 20.0f + randf() * 30.0f,
                                 0.5f + randf() * 0.4f, 7.0f + randf() * 6.0f,
                                 120, 98, 76, WEAPON_FX_ROCK);
                    if (randf() < 8.0f * k * dt)
                        fx_spawn(&weaponFx, mcx + (randf() * 2.0f - 1.0f) * hw, mcy + hh,
                                 (randf() * 2.0f - 1.0f) * 12.0f, -8.0f,
                                 1.0f, 9.0f, 112, 98, 80, WEAPON_FX_SMOKE);
                    break;
                case MOB_MARK_POISON:
                    if (randf() < 12.0f * k * dt)
                        fx_spawn(&weaponFx, mcx + (randf() * 2.0f - 1.0f) * hw,
                                 mcy + (randf() * 2.0f - 1.0f) * hh,
                                 (randf() * 2.0f - 1.0f) * 8.0f, -26.0f - randf() * 18.0f,
                                 0.8f + randf() * 0.5f, 6.0f + randf() * 5.0f,
                                 150, 210, 110, WEAPON_FX_POISON);
                    break;
                default: break;
                }
            }
            }
            // Zamanlı patlamalar (skill/boss) ısı alanlarının ÜSTÜNE eklenir;
            // ikisi aynı tavanı paylaşıyor (8), patlamalar daha alakalı olduğu
            // için sonra yazılıyor — tavan dolarsa ısı düşer, patlama kalır.
            groundfx_update(dt);
            fx_distort_update(dt);
            distortN += fx_distort_collect(distorts + distortN,
                                           R2D_MAX_DISTORT - distortN,
                                           camera.x, camera.y, camera.zoom);
            r2d_distort_set(distorts, distortN);
            if (devInfinite) {
                player.mana = player.maxMana;
                player.stamina = player.maxStamina;
                player.health = player.maxHealth;
            }
            // Darbe yönü için oyuncu merkezi (strike_fx_mob okuyor)
            g_playerCx = player.rect.x + player.rect.w / 2.0f;
            g_playerCy = player.rect.y + player.rect.h / 2.0f;
            skillfx_update(&weaponFx, dt);
            // ── DARBE İSTEKLERİNİ TÜKET: motor istek bırakır, oyun katmanı
            // uygular (zaman ölçeği ve kamera oyunun malı, motorun değil).
            {
                float rs = skillfx_take_hitstop();
                if (rs > 0.0f) hitstop(rs);
                float sa = 0.0f, sd = 0.0f;
                skillfx_take_shake(&sa, &sd);
                if (sa > 0.0f && sa > shakeAmp * (shakeTime / SDL_max(shakeDur, 0.001f))) {
                    shakeAmp = sa; shakeTime = shakeDur = sd;
                }
            }
            fx_update(&weaponFx, dt);
            // CANLI SAYIM (izole modda): tuş anındaki sayım YANILTICIYDI —
            // yeni mod henüz partikül doğurmamışken okuyordu, ekranda önceki
            // modun kalıntısı görünüyordu. Şimdi her kare tazeleniyor, yani
            // "doğmuyor mu / doğuyor da görünmüyor mu" ayrımı gerçekten okunur.
            if (fireLayer != 0) {
                static char fireDiag[160];
                SDL_snprintf(fireDiag, sizeof(fireDiag),
                             "%s  [alev %d duman %d kivilcim %d buhar %d]",
                             FIRE_LAYERS[fireLayer - 1].name,
                             fx_count_active(&weaponFx, WEAPON_FX_FLAME),
                             fx_count_active(&weaponFx, WEAPON_FX_SMOKE),
                             fx_count_active(&weaponFx, WEAPON_FX_SPARK),
                             fx_count_active(&weaponFx, WEAPON_FX_STEAM));
                splashText = fireDiag;
                splashTimer = 0.4f;   // izole mod sürdükçe ekranda kalır
            }
            animfx_update(animFxPool, dt);
            decal_update(dt);

            gametime_update(dt);                      // dünya saati
            npc_set_talking(dialogNpc);               // konuşan NPC yerinde durur
            if (map_current() == MAP_WORLD || map_current() == MAP_LOCA ||
                map_current() == MAP_DOGU)
                npc_update(dt, gametime_hours());     // NPC rutinleri (loca+doğu)

            // Günlük itibar toparlanması: suçsuz geçen gün, negatif itibar 0'a doğru +1
            // (pozitif itibar zamanla artmaz — güven kazanmak iş ister)
            int today = gametime_day();
            if (today != lastSeenDay)
            {
                lastSeenDay = today;
                for (int pi = 0; pi < PICK_TRIES_CAP; pi++) pickTries[pi] = 0;   // cepler tazelenir
                pickFailUsed = false;

                // KANDİL YAKITI (LORE §1.1): gece geçti, yanan her direk bir
                // gece yaktı. Sönen olduysa oyuncu sabah öğrenir — ve o direğin
                // alanı bu akşamdan itibaren ölüyü bastırmaz (§1.4).
                if (lamp_night_passed())
                {
                    SDL_Color cold = { 190, 190, 205, 255 };
                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                         player.rect.x + player.rect.w / 2.0f,
                                         player.rect.y - 22.0f,
                                         "Bir direk sondu.", cold);
                }

                // REVİR: 30 gün ödenmeyen yatış — köylüler toplar, NPC döner
                // ama AFFETMEZ (kalıcı kırgınlık: soğuk selam + %25 zam) + itibar
                for (int ni3 = 0; ni3 < NPC_COUNT; ni3++)
                {
                    if (!npc_infirm(ni3) ||
                        today - npc_infirm_day(ni3) < 30) continue;
                    npc_infirmary_discharge(ni3, true);
                    int vilReg = region_of(12.0f * TILE_SIZE, 8.0f * TILE_SIZE);
                    rep_add(&player.reputation[vilReg], REP_HIT);
                    char mbuf2[FLOAT_TEXT_LEN];
                    snprintf(mbuf2, sizeof(mbuf2),
                             "%s ayağa kalktı — ama masrafı köy topladı...",
                             NPCS[ni3].name);
                    SDL_Color cold = { 180, 180, 210, 255 };
                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                        player.rect.x + player.rect.w / 2.0f,
                        player.rect.y - 44.0f, mbuf2, cold);
                }

                journal_day_tick(today);    // günlük atmosfer satırları (seyrek)
                resource_daily_respawn();   // doğa tazelenir (cevher/bitki/ağaç)
                contract_new_day(today);    // pano yenilenir (yarım kontratlar düşer)

                // HAVA (şafak zarı): dün yağdıysa doğa beslenir (ot toplaması ×2 bugün)
                natureBounty = weather_rained_today();   // weather_dawn bayrağı sıfırlar
                g_natureBounty = natureBounty;           // gather_finish köprüsü
                weather_dawn(today);
                if (natureBounty)
                {
                    SDL_Color grn = { 150, 255, 150, 255 };
                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                         player.rect.x + player.rect.w / 2.0f,
                                         player.rect.y - 36.0f,
                                         "Yağmur toprağa iyi geldi — otlar gür.", grn);
                }

                // DÜNYA OLAYI (şafak zarı): günün olayı belirlenir + köy duyurusu
                // HUZURSUZ MEZARLIK kararı (dünkü olay — worldevent_dawn'dan ÖNCE):
                // gece 2+ iskelet sağ kaldıysa mezar yeniden yuva olur
                {
                    int risenAlive = 0;
                    for (int m = 0; m < MAX_MOBS; m++)
                        if (mobs[m].e.alive && mobs[m].risen) risenAlive++;
                    if (graveSiteIdx >= 0 && risenAlive >= 2)
                    {
                        NestState *gns = ecology_state(graveSiteIdx);
                        gns->colonized = true;
                        gns->destroyed = false;
                        gns->chiefAlive = true;
                        gns->pop[0] = (Sint16)((risenAlive > 2) ? 2 : risenAlive);
                        gns->pop[1] = (Sint16)((risenAlive > 2) ? 1 : 0);
                        gns->destroyedDay = 0;
                        gns->migrateDay = 0;
                        gns->colonizedDay = (Sint16)today;   // pano 7 gün duyurur
                        journal_add(JE_GRAVE_RISE, 0);
                        splashText = "Ölüler mezarı yurt edindi.";
                        splashTimer = 3.0f;
                    }
                    graveSiteIdx = -1;
                    for (int m = 0; m < MAX_MOBS; m++)   // gün ışığı: geri çekilirler
                        if (mobs[m].e.alive && mobs[m].risen)
                        {
                            mobs[m].e.alive = false;
                            mobs[m].dying = false;   // sessiz despawn — mezara iner
                        }
                }

                WorldEvent newEv = worldevent_dawn(today);
                for (int m = 0; m < MAX_MOBS; m++)   // baskından sağ kalanlar çekilir
                    if (mobs[m].e.alive && mobs[m].raider)
                    {
                        mobs[m].e.alive = false;
                        mobs[m].dying = false;   // sessiz despawn — leş/loot yok
                    }
                if (newEv == WE_BOUNTY)
                    ecology_birth_bonus(2);      // bugünkü ecology_dawn'a işler
                if (kervanIdx >= 0)              // kervan yalnız kendi günlerinde meydanda
                    npc_force_hidden(kervanIdx, worldevent_current() != WE_CARAVAN);
                if (newEv != WE_NONE)
                {
                    // Görünür bir köylü günün haberini bağırır (muhafız/kervancı hariç)
                    const char *evBark =
                        newEv == WE_RAID    ? "Sınırda goblin izi var — gece kapını sürgüle!"
                      : newEv == WE_CARAVAN ? "Kervan gelmiş! Meydanda tezgah açmış."
                      : newEv == WE_BOUNTY  ? "Otlaklar dolup taşıyor — bereketli gün!"
                      : newEv == WE_GRAVE   ? "Mezarcı bu sabah mezarlıktan kaçmış — kıpırtı varmış!"
                      :                       "Taşlar gece parlıyormuş... hayra alamet değil.";
                    for (int ni = 0; ni < NPC_COUNT; ni++)
                    {
                        float bx, by;
                        if (ni == guardIdx || ni == kervanIdx) continue;
                        if (!npc_visible_pos(ni, &bx, &by)) continue;
                        npc_bark(ni, evBark, 6.0f);
                        break;
                    }
                }

                ecology_dawn(today, spawnZones, MAX_SPAWN_ZONES, &spawnZoneCount,
                             mobs, MAX_MOBS, &nextMobId,
                             map_current() == MAP_WORLD);
                                             // yuva başına +1 (tavana kadar) + göçler

                // Otomatik kayıt (Faz B1): her şafak auto slotuna yazılır
                if (save_game(save_slot_path(SAVE_SLOT_AUTO), &player, 
                              &inventory, &craftBag, &equipment, &hotbar, questStatus))
                {
                    save_read_summary(save_slot_path(SAVE_SLOT_AUTO),
                                      &slotSums[SAVE_SLOT_AUTO]);
                    SDL_Color dim = { 170, 170, 190, 255 };
                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                         player.rect.x + player.rect.w / 2.0f,
                                         player.rect.y - 24.0f,
                                         "Otomatik kayıt", dim);
                }
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

            // YANKICI gizliliği: ritüel tamamlanana dek skill defterinde yok
            skill_set_echo_visible(player.storyFlags[SF_YANKICI] == 2);
            skill_set_assassin_visible(player.storyFlags[SF_ASSASSIN] >= 13);

            // GÖLGE FORMU: kanla ödenen görünmezlik (4 HP/sn; 5 HP'de düşer)
            if (player.smokeTimer > 0.0f) player.smokeTimer -= dt;

            // İĞNE ŞARJ GÖRSELİ: iğne takılı + şarj varken silahtan KOYU
            // MOR-YEŞİL damlalar AŞAĞI damlar (rün zehri yukarı çıkar — ayrım)
            if (equipment.slots[EQUIP_SHIELD].type ==
                    &ITEM_TYPES[ITEM_POISON_NEEDLE] &&
                bag_count(&ITEM_TYPES[ITEM_NEEDLE_VENOM]) > 0 &&
                randf() < dt * 5.0f)
            {
                float wx3 = player.rect.x + player.rect.w / 2.0f
                          + WEAPON_FX_OFFSET[player.facing].dx;
                float wy3 = player.rect.y + player.rect.h / 2.0f
                          + WEAPON_FX_OFFSET[player.facing].dy;
                fx_spawn(&weaponFx, wx3, wy3,
                         (randf() - 0.5f) * 8.0f, 35.0f + randf() * 25.0f,
                         0.6f, 3.5f + randf() * 2.5f, 130, 60, 160,
                         WEAPON_FX_VENOM);
            }
            if (player.shadowTimer > 0.0f)
            {
                player.shadowTimer -= dt;
                player.health -= (4.0f - perkFx.shadowDrainCut) * dt;
                if (player.health <= 5.0f)
                {
                    player.health = 5.0f;
                    player.shadowTimer = 0.0f;   // kan bitti — gölge bırakır
                }
            }

            // AKTİF BLOK (SHIFT): kalkan takılıyken basılı tut — duruş.
            // Gelen hasar -%70 (ağaçla -%80), hız yarıya, auto-attack durur,
            // saniyede 4 stamina; ilk 0.25sn PERFECT penceresi (parry).
            {
                const bool *bkeys = SDL_GetKeyboardState(NULL);
                bool hasShield = equipment.slots[EQUIP_SHIELD].type != NULL &&
                                 equipment.slots[EQUIP_SHIELD].type->weaponType
                                     == WEAPON_SHIELD;
                bool wantBlock = hasShield && player.stamina > 0.0f &&
                                 (bkeys[SDL_SCANCODE_LSHIFT] ||
                                  bkeys[SDL_SCANCODE_RSHIFT]) &&
                                 dialogNpc < 0 && !scene_active() && !catchOpen;
                if (wantBlock && !player.blocking)
                    player.blockHold = 0.0f;   // pencere duruş BAŞLANGICINDA açılır
                player.blocking = wantBlock;
                if (player.blocking)
                {
                    player.blockHold += dt;
                    player.stamina -= 4.0f * dt *
                        (perkFx.stanceStaminaMult > 0.0f
                         ? perkFx.stanceStaminaMult : 1.0f);
                    if (player.stamina <= 0.0f)
                    {
                        player.stamina = 0.0f;
                        player.blocking = false;   // nefes bitti — duruş düşer
                    }
                    player.autoAttack = false;     // kalkan yukarıdayken vurulmaz
                }
            }

            // KÖR KUYU KARŞILAŞMASI: gece kuyu civarında gezinen oyuncuya
            // Yabancı BİR KEZ yaklaşır — kuyuyu anlatır, şüphe bırakır, kaybolur
            if (yabanciIdx >= 0 && player.storyFlags[SF_WELL] == 0 &&
                dialogNpc < 0 && !scene_active() && gametime_is_night() &&
                map_current() == MAP_WORLD)
            {
                float wdx2 = player.rect.x + player.rect.w / 2.0f
                           - (26.0f * TILE_SIZE + 32.0f);
                float wdy2 = player.rect.y + player.rect.h / 2.0f
                           - (15.0f * TILE_SIZE + 48.0f);
                if (SDL_fabsf(wdx2) < 200.0f && SDL_fabsf(wdy2) < 200.0f)
                {
                    npc_force_hidden(yabanciIdx, false);   // karanlıktan belirir
                    dialogNpc = yabanciIdx;
                    dialogNode = dialog_find(&npcDialogs[yabanciIdx], "start");
                    dialog_type_reset();
                    npc_set_talking(yabanciIdx);
                    player.autoAttack = false;
                    audio_play(SFX_WHISPER);
                }
            }

            // KAPI TUTUCU duruşu: menzile giren en yakın mob otomatik hedef
            // alınır ve KO auto-attack devreye girer (0.8sn iç tempo tavanı)
            if (player.gateStance && !player.attacking)
            {
                float gcx = player.rect.x + player.rect.w / 2.0f;
                float gcy = player.rect.y + player.rect.h / 2.0f;
                float gRange = PLAYER_SPEAR_RANGE + perkFx.spearRangeBonus;
                int bestId = -1;
                float bestD = gRange;
                for (int m2 = 0; m2 < mobCount; m2++)
                {
                    Mob *nb = &mobs[m2];
                    if (!nb->e.alive || nb->type->isStructure) continue;
                    if (nb->type->passive && !nb->aggro) continue;   // ava saplanmaz
                    float ndx = (nb->e.rect.x + nb->e.rect.w / 2.0f) - gcx;
                    float ndy = (nb->e.rect.y + nb->e.rect.h / 2.0f) - gcy;
                    float nd = SDL_sqrtf(ndx * ndx + ndy * ndy);
                    if (nd < bestD) { bestD = nd; bestId = nb->id; }
                }
                if (bestId >= 0)
                {
                    player.targetMobId = bestId;
                    player.autoAttack = true;
                    if (player.attackTimer > 0.8f)
                        player.attackTimer = 0.8f;   // duruş temposu: hızlı dürtme
                }
            }

            // HAVA (kare bazlı): akış + yağmur sesi + fırtına yıldırımı
            weather_update(dt, gametime_hours());
            {
                bool openSky = (map_current() == MAP_WORLD);
                Weather wnow = weather_current();
                bool raining = (wnow == WEATHER_RAIN || wnow == WEATHER_STORM);
                ambience_set_rain((openSky && raining) ? weather_intensity() : 0.0f);

                // SU KIYISI SESİ: en yakın ıslak karoya yaklaştıkça çırpıntı.
                // Tarama ucuz ama her kare gereksiz — ~0.25 sn'de bir yeter
                // (geçişi zaten ambience_update yumuşatıyor).
                {
                    static float waterScanT = 0.0f;
                    waterScanT -= dt;
                    if (waterScanT <= 0.0f)
                    {
                        waterScanT = 0.25f;
                        float wg = 0.0f, agit = 0.0f;
                        if (openSky)   // zindanda/iç mekânda kıyı yok
                        {
                            int pc = (int)((player.rect.x + player.rect.w / 2.0f) / TILE_SIZE);
                            int pr = (int)((player.rect.y + player.rect.h) / TILE_SIZE);
                            float best = 7.0f;   // 6 karodan uzak = duyulmaz
                            for (int rr = pr - 6; rr <= pr + 6; rr++)
                                for (int cc = pc - 6; cc <= pc + 6; cc++)
                                {
                                    if (!tilemap_is_water(cc, rr)) continue;
                                    float dc = (float)(cc - pc), dr = (float)(rr - pr);
                                    float d = SDL_sqrtf(dc * dc + dr * dr);
                                    if (d < best)
                                    {
                                        best = d;
                                        // en yakın suyun karakteri seçer:
                                        // deniz/göl dalgalı, gölet durgun
                                        agit = tilemap_water_agitation(cc, rr);
                                    }
                                }
                            wg = 1.0f - best / 6.0f;
                            if (wg < 0.0f) wg = 0.0f;
                            // Yağmur şiddetliyken kıs: iki ıslak ses üst üste çamur
                            if (raining) wg *= 1.0f - weather_intensity() * 0.5f;
                        }
                        ambience_set_water(wg * 0.7f, agit);
                    }
                }

                // Köylüler yağmur/fırtınada evlerine çekilir (kervancı gölgelikte)
                npc_set_shelter(raining && weather_intensity() > 0.4f, kervanIdx);

                // Değişim duyurusu: bölge adı gibi ekran ortasında belirir
                if (wnow != lastWeather)
                {
                    lastWeather = wnow;
                    if (openSky)
                    {
                        splashText = (wnow == WEATHER_RAIN)  ? "Yağmur bastırıyor..."
                                   : (wnow == WEATHER_STORM) ? "Fırtına!"
                                   : (wnow == WEATHER_FOG)   ? "Sis çöküyor..."
                                   : (wnow == WEATHER_SNOW)  ? "Kar başlıyor..."
                                   :                           "Gökyüzü açılıyor";
                        splashTimer = 3.0f;
                        if (wnow == WEATHER_RAIN && guardIdx >= 0)   // nöbetçi ıslanır
                            npc_bark(guardIdx, "Bereket yağmuru... Nöbet ıslak geçecek.", 5.0f);
                        else if (wnow == WEATHER_STORM && guardIdx >= 0)
                            npc_bark(guardIdx, "Fırtına geliyor — açıkta durma, yolcu!", 5.0f);
                        else if (wnow == WEATHER_SNOW && guardIdx >= 0)
                            npc_bark(guardIdx, "Kar... Kemiklerim kuzeyden kötü haber alır.", 5.0f);
                    }
                }

                // FIRTINA YILDIRIMI: açık alanda rastgele düşer — sığınak değerli.
                // 60px yakınına düşerse hasar (moba da oyuncuya da); uzağı gösteri.
                if (openSky && wnow == WEATHER_STORM && weather_intensity() > 0.5f)
                {
                    stormBoltTimer -= dt;
                    if (stormBoltTimer <= 0.0f)
                    {
                        stormBoltTimer = 8.0f + randf() * 12.0f;
                        lightningFlash = 0.15f;
                        audio_play(SFX_THUNDER);
                        float sx = player.rect.x + (randf() - 0.5f) * 1200.0f;
                        float sy = player.rect.y + (randf() - 0.5f) * 1200.0f;
                        fx_burst(&weaponFx, sx, sy, 30, 220.0f, 240, 245, 255);
                        for (int m = 0; m < mobCount; m++)
                        {
                            Mob *lb = &mobs[m];
                            if (!lb->e.alive) continue;
                            float ldx = (lb->e.rect.x + lb->e.rect.w / 2.0f) - sx;
                            float ldy = (lb->e.rect.y + lb->e.rect.h / 2.0f) - sy;
                            if (ldx * ldx + ldy * ldy > 60.0f * 60.0f) continue;
                            lb->e.health -= 25.0f;
                            strike_fx_mob(lb, true, 0.0f);
                            lb->e.hitFlash = 0.15f;
                            lb->aggro = true;
                            floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                            lb->e.rect.x + lb->e.rect.w / 2.0f,
                                            lb->e.rect.y - 10.0f, 25, false);
                            if (lb->e.health <= 0.0f)
                                mob_die(lb, mobs, MAX_MOBS, &nextMobId, loots,
                                        questStatus, floatTexts, &player);
                        }
                        float pdx = (player.rect.x + player.rect.w / 2.0f) - sx;
                        float pdy = (player.rect.y + player.rect.h / 2.0f) - sy;
                        if (pdx * pdx + pdy * pdy <= 60.0f * 60.0f)
                        {
                            player.health -= 20.0f;
                            player.gaziTimer = 5.0f;
                            SDL_Color el = { 240, 245, 160, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 player.rect.x + player.rect.w / 2.0f,
                                                 player.rect.y - 24.0f,
                                                 "Yıldırım çarptı!", el);
                        }
                    }
                }
                else
                    stormBoltTimer = 6.0f;   // fırtına yokken sayaç beklemede
            }
            if (lightningFlash > 0.0f) lightningFlash -= dt;

            // DÜNYA OLAYI (kare bazlı): baskın gece kenarında başlar; mühür
            // sancısı gecesi yankılar arada fısıldar (görsel büyüme render'da)
            {
                bool nightNow = gametime_is_night();

                // ══ SÖNÜK DİREK = KALKAN ÖLÜ (LORE §1.4) ══
                // Dünya olayına bağlı DEĞİL: kural her gece işler, çünkü kural
                // dünyanın fiziği. Yanan direğin halkasında hiçbir şey olmaz;
                // sönük direğin dibinde toprak kabarır. Oyuncu bunu ilk gece
                // tek kelime metin okumadan öğrenir — bir direk yanıyor, öbürü
                // sönük, ve fark ekranda duruyor.
                if (nightNow && !wasNightWE && map_current() == MAP_WORLD)
                {
                    int risenTotal = 0;
                    const char *risenPlace = NULL;
                    for (int li = 0; li < LAMP_COUNT; li++)
                    {
                        // §1.4: ölü GÖMÜLÜ OLDUĞU yerde kalkar. Direk yalnız
                        // bastırır, kendisi ölü üretmez — meydanda gömü yoktur.
                        if (!LAMPS[li].graves) continue;
                        if (lamp_lit(li)) continue;
                        // Komşu bir yanan direk buraya yetişiyorsa yine bastırır:
                        // oyuncu bir direği besleyip yanındakini de koruyabilir.
                        // Eşik AYRI BİR SAYI DEĞİL: ışık varsa bastırılmış.
                        // Böylece görsel halka ile korunan halka asla ayrışamaz.
                        if (lamp_level_at(LAMPS[li].x, LAMPS[li].y) > 0.0f) continue;
                        if (risenPlace == NULL) risenPlace = LAMPS[li].place;

                        int nRise = 1 + (randf() < 0.45f ? 1 : 0);
                        for (int r = 0; r < nRise; r++)
                        {
                            float ang = randf() * 6.2831853f;
                            float rad = 40.0f + randf() * 70.0f;
                            int gid = mob_spawn(mobs, MAX_MOBS, &nextMobId,
                                                &MOB_TYPES[12],
                                                LAMPS[li].x + SDL_cosf(ang) * rad,
                                                LAMPS[li].y + SDL_sinf(ang) * rad, -1);
                            Mob *gm = mob_find_by_id(mobs, MAX_MOBS, gid);
                            if (gm != NULL) { gm->risen = true; gm->alertFlash = 1.0f; }
                            if (gid >= 0) risenTotal++;
                        }
                    }
                    // Gece çökerken oyuncu NE olduğunu ve NEREDE olduğunu bilsin:
                    // sönük direklerin adı sayılır. Bilgi olmadan kural
                    // öğrenilemez (§14.4 beş dakika testi).
                    {
                        int dark = 0;
                        for (int li = 0; li < LAMP_COUNT; li++)
                            if (LAMPS[li].graves && !lamp_lit(li) &&
                                lamp_level_at(LAMPS[li].x, LAMPS[li].y) <= 0.0f) dark++;
                        if (dark > 0 && risenTotal > 0)
                            snprintf(splashOwn, sizeof(splashOwn),
                                     "Gece cokdu - %s sonuk. Toprak kabardi.",
                                     risenPlace ? risenPlace : "bir direk");
                        else if (dark > 0)
                            snprintf(splashOwn, sizeof(splashOwn),
                                     "Gece cokdu - %d direk sonuk.", dark);
                        else
                            snprintf(splashOwn, sizeof(splashOwn),
                                     "Gece cokdu - hat aydinlik.");
                        splashText = splashOwn;
                        splashTimer = 6.0f;
                        if (risenTotal > 0) audio_play(SFX_WHISPER);
                    }
                }

                if (nightNow && !wasNightWE && map_current() == MAP_WORLD &&
                    worldevent_current() == WE_GRAVE)
                {
                    // HUZURSUZ MEZARLIK: mühürlü/boş iskelet mezarı gece kıpırdar.
                    // Sabaha 2+ iskelet sağ kalırsa mezar YENİDEN yuva olur.
                    graveSiteIdx = -1;
                    for (int gi = 0; gi < NEST_SITE_COUNT; gi++)
                    {
                        const NestSiteDef *gs = ecology_site(gi);
                        const NestState *gst = ecology_state(gi);
                        if (gs->family != FAM_SKELETON) continue;
                        if (gst->destroyed || !gst->colonized) { graveSiteIdx = gi; break; }
                    }
                    // Uygun saha yoksa (ikisi de canlı): ilk mezarda gösteri —
                    // iskelet yükselir ama koloni kurulamaz (zaten yuva var)
                    int showSite = graveSiteIdx;
                    if (showSite < 0)
                        for (int gi = 0; gi < NEST_SITE_COUNT; gi++)
                            if (ecology_site(gi)->family == FAM_SKELETON)
                            { showSite = gi; break; }
                    if (showSite >= 0)
                    {
                        const NestSiteDef *gs = ecology_site(showSite);
                        for (int r = 0; r < 3; r++)
                        {
                            const MobType *gt = (r < 2) ? &MOB_TYPES[12]    // savaşçı
                                                        : &MOB_TYPES[13];   // okçu
                            int gid = mob_spawn(mobs, MAX_MOBS, &nextMobId, gt,
                                                gs->x + (randf() - 0.5f) * 120.0f,
                                                gs->y + (randf() - 0.5f) * 120.0f, -1);
                            Mob *gm = mob_find_by_id(mobs, MAX_MOBS, gid);
                            if (gm != NULL)
                            {
                                gm->risen = true;
                                gm->alertFlash = 1.0f;
                            }
                        }
                        splashText = "Mezarlık huzursuz...";
                        splashTimer = 3.0f;
                        audio_play(SFX_WHISPER);
                    }
                }
                if (nightNow && !wasNightWE && map_current() == MAP_WORLD &&
                    worldevent_current() == WE_RAID)
                {
                    int nRaid = 2 + (randf() < 0.5f ? 1 : 0);   // 2-3 goblin
                    for (int r = 0; r < nRaid; r++)
                    {
                        // Köyün doğu kenarı — meydana yürüme mesafesi
                        float rx = (19.0f + (float)r * 1.5f) * TILE_SIZE;
                        float ry = (4.0f + (float)r * 2.5f) * TILE_SIZE;
                        int rid = mob_spawn(mobs, MAX_MOBS, &nextMobId,
                                            &MOB_TYPES[0], rx, ry, -1);
                        Mob *rm = mob_find_by_id(mobs, MAX_MOBS, rid);
                        if (rm != NULL)
                        {
                            rm->raider = true;
                            rm->aggro = true;      // baskıncı doğuştan kızgın
                            rm->suspicion = 1.0f;
                            rm->alertFlash = 1.0f;
                        }
                    }
                    SDL_Color red = { 255, 120, 100, 255 };
                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                         player.rect.x + player.rect.w / 2.0f,
                                         player.rect.y - 40.0f,
                                         "Baskın! Köy kenarında goblinler!", red);
                    audio_play(SFX_UI);
                }
                wasNightWE = nightNow;

                if (nightNow && worldevent_current() == WE_SEAL)
                {
                    sealWhisperTimer -= dt;
                    if (sealWhisperTimer <= 0.0f)
                    {
                        audio_play(SFX_WHISPER);
                        sealWhisperTimer = 40.0f + randf() * 35.0f;
                    }
                }
            }

            // --- MUHAFIZ: bounty takibi / yakalama / düşmanlık (guard.c beyni) ---
            if (guardIdx >= 0 && map_current() == MAP_WORLD)
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
                    int gBlock = 0;
                    float dmg = combat_mob_damage(gt.attackDamage, &equipment, &perkFx,
                                                  player.state == PLAYER_IDLE,
                                                  player.health / player.maxHealth,
                                                  yaraliAyi,
                                                  player.skills[SKILL_BLOCK].level,
                                                  player.blocking, player.blockHold,
                                                  &dodged, &gBlock);
                    block_feedback(gBlock, &player, &hotbar, floatTexts, &perkFx);
                    if (gBlock > 0 && g_animFx != NULL)
                        animfx_spawn(g_animFx, g_sparkTex, 7, 7, 48.0f, 48.0f,
                                     player.rect.x + player.rect.w / 2.0f,
                                     player.rect.y + player.rect.h * 0.45f,
                                     30.0f, 1.2f);
                    if (gBlock == 3) hitstop(0.12f);   // PERFECT parry donması
                    // IŞIK TAŞIYICISI: kendi Kutsal Alanında blok %20 güçlü
                    if (gBlock > 0 && holyZone.on && perkFx.holyZoneBlockCut > 0.0f)
                    {
                        float hzdx = (player.rect.x + player.rect.w / 2.0f) - holyZone.x;
                        float hzdy = (player.rect.y + player.rect.h / 2.0f) - holyZone.y;
                        if (hzdx * hzdx + hzdy * hzdy <= holyZone.r * holyZone.r)
                            dmg *= 1.0f - perkFx.holyZoneBlockCut;
                    }
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

                    if (tdist <= (player.spearEquipped
                                  ? PLAYER_SPEAR_RANGE + perkFx.spearRangeBonus
                                  : PLAYER_ATTACK_RANGE))
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
                        // Delici silah (mızrak): hedef defansının bir kısmı yok
                        // sayılır (silah tabanı + ağaç Zırh Delici düğümleri)
                        float effDef = target->type->defense;
                        if (weapon->type != NULL && weapon->type->armorPierce > 0.0f)
                        {
                            float pierce = weapon->type->armorPierce + perkFx.pierceBonus;
                            if (pierce > 0.8f) pierce = 0.8f;   // tavan: delme %80
                            effDef *= 1.0f - pierce;
                        }
                        HitResult hit = combat_player_hit(&player, weapon,
                                                          effDef,
                                                          abilityScale,
                                                          target->e.health / target->e.maxHealth,
                                                          &perkFx);

                        hit.damage *= title_damage_mult(&player, target->type,
                                                        gametime_is_night());   // M3

                        // FISILTI KESKİSİ: eksik canın her %10'u +%8 — riskle
                        // keskinleşir (Yankıcı becerileri can yakar: sinerji)
                        if (weapon->type == &ITEM_TYPES[ITEM_WHISPER_BLADE])
                        {
                            float missing = 1.0f - player.health / player.maxHealth;
                            hit.damage *= 1.0f + 0.8f * missing;
                            float exp2 = combat_xp_for_hit(hit.damage,
                                            target->type->level,
                                            player.skills[SKILL_ECHO].level);
                            if (skill_add_xp(player.skills, SKILL_ECHO, exp2 * 0.5f))
                                on_skill_levelup(&player, SKILL_ECHO,
                                                 &hotbar, floatTexts);
                        }

                        // Sinsi vuruş: eğilmiş + fark etmemiş — YA DA hedef uyuyor (E2)
                        if ((player.sneaking || target->sleeping) && !target->aggro)
                        {
                            hit.damage *= perkFx.backstabMult
                                        + title_backstab_bonus(&player);   // Gölge (M3)
                            title_notify(title_stat_add(&player, TS_SNEAK_HITS, 1),
                                         &player, floatTexts);
                            SDL_Color purple = { 200, 140, 255, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 target->e.rect.x + target->e.rect.w / 2.0f,
                                                 target->e.rect.y - 26.0f, "Sinsi vuruş!", purple);
                            if (skill_add_xp(player.skills, SKILL_SNEAK, SNEAK_XP_BACKSTAB))
                                on_skill_levelup(&player, SKILL_SNEAK, &hotbar, floatTexts);
                        }

                        // İmzasız Bıçak: ARKADAN +%50 + Suikast XP (sınıf pasifi).
                        // HİÇ KİMSE: dumandaki ilk (garanti kritik) vuruş
                        // arkadan sayılır — pusudan tam suikast vuruşu
                        if (equipment.slots[EQUIP_WEAPON].type ==
                                &ITEM_TYPES[ITEM_DAGGER_UNSIGNED] &&
                            (player_behind_mob(target,
                                player.rect.x + player.rect.w / 2.0f,
                                player.rect.y + player.rect.h / 2.0f) ||
                             (perkFx.smokeFirstCrit && player.forceCrit &&
                              player.smokeTimer > 0.0f)))
                        {
                            hit.damage *= 1.5f * perkFx.backstabDmgMult;
                            if (perkFx.backstabExecuteBonus > 0.0f &&
                                target->e.health < target->e.maxHealth * 0.25f)
                                hit.damage *= 1.0f + perkFx.backstabExecuteBonus;
                            float sxpA = combat_xp_for_hit(hit.damage,
                                             target->type->level,
                                             player.skills[SKILL_ASSASSIN].level);
                            if (skill_add_xp(player.skills, SKILL_ASSASSIN, sxpA))
                                on_skill_levelup(&player, SKILL_ASSASSIN,
                                                 &hotbar, floatTexts);
                        }
                        player.forceCrit = false;   // Gölge Adımı garantisi tüketildi
                        target->e.health -= hit.damage;
                        strike_fx_mob(target, false, 0.05f);
                        target->e.hitFlash = 0.15f;
                        audio_play(hit.crit ? SFX_CRIT : SFX_HIT);

                        // Can çalma (Lord'un Kılıcı): hasarın bir payı cana döner
                        if (weapon->type != NULL && weapon->type->lifesteal > 0.0f)
                        {
                            player.health += hit.damage * weapon->type->lifesteal;
                            if (player.health > player.maxHealth)
                                player.health = player.maxHealth;
                        }

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

                        // Alev Rünü (M1): vuruş yakma da basar — %20/3sn.
                        // Melee-ateş build'i Termal Şok sinerjisine böyle girer
                        if (player.weaponFx == WEAPON_FX_FLAME)
                        {
                            target->burnDps = hit.damage * 0.20f / 3.0f;
                            target->burnTime = 3.0f * weather_burn_time_mult();
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

                        float kb = 180.0f * perkFx.kbDealtMult    // Ezici Ağırlık
                                 * (player.pendingAbility == ABILITY_STAB
                                    ? 2.0f : 1.0f);   // Saplama sert iter
                        if (tdist > 0.0f)
                        {
                            target->e.knockbackX = (tdx / tdist) * kb;
                            target->e.knockbackY = (tdy / tdist) * kb;
                        }

                        // HAT VURUŞU (mızrak): saplama çizgisindeki İKİNCİ mob
                        // hasarın %60'ını yer (ağaç: %75 + ek hedef)
                        if (player.spearEquipped && tdist > 0.0f)
                        {
                            float lineFrac = (perkFx.lineDmgFrac > 0.0f)
                                           ? perkFx.lineDmgFrac : 0.60f;
                            int lineLeft = 1 + perkFx.lineTargetBonus;
                            float lnx = tdx / tdist, lny = tdy / tdist;
                            for (int m2 = 0; m2 < mobCount; m2++)
                            {
                                Mob *lb = &mobs[m2];
                                if (!lb->e.alive || lb == target) continue;
                                float ldx = (lb->e.rect.x + lb->e.rect.w / 2.0f)
                                          - (player.rect.x + player.rect.w / 2.0f);
                                float ldy = (lb->e.rect.y + lb->e.rect.h / 2.0f)
                                          - (player.rect.y + player.rect.h / 2.0f);
                                float ld = SDL_sqrtf(ldx * ldx + ldy * ldy);
                                if (ld < 0.001f || ld > PLAYER_SPEAR_RANGE + 25.0f)
                                    continue;
                                if ((ldx * lnx + ldy * lny) / ld < 0.85f)
                                    continue;   // hatta değil
                                float ldmg = hit.damage * lineFrac;
                                lb->e.health -= ldmg;
                                strike_fx_mob(lb, false, 0.03f);
                                lb->e.hitFlash = 0.15f;
                                lb->aggro = true;
                                floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                                lb->e.rect.x + lb->e.rect.w / 2.0f + 10.0f,
                                                lb->e.rect.y, (int)ldmg, false);
                                if (lb->e.health <= 0.0f)
                                    mob_die(lb, mobs, MAX_MOBS, &nextMobId, loots,
                                            questStatus, floatTexts, &player);
                                if (--lineLeft <= 0) break;   // Şişleme: +1 hedef
                            }
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
                            // REVİR (Faz REVIR): sivil 1 canda BAYILIR — revire
                            // taşınır (öldü sayılmaz; masraf sayacı bugün başlar)
                            if (!isGuardVictim && !died &&
                                npc_health(victim) <= 1.01f && !npc_infirm(victim))
                            {
                                npc_send_infirmary(victim, gametime_day());
                                if (victim == targetNpc) targetNpc = -1;
                                char fbuf[FLOAT_TEXT_LEN];
                                snprintf(fbuf, sizeof(fbuf),
                                         "%s bayıldı — revire taşındı",
                                         NPCS[victim].name);
                                SDL_Color or2 = { 255, 180, 90, 255 };
                                floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                    player.rect.x + player.rect.w / 2.0f,
                                    player.rect.y - 40.0f, fbuf, or2);
                                if (sifaciIdx >= 0)
                                    npc_bark(sifaciIdx,
                                             "Bir yaralı daha... Getirin şunu.", 5.0f);
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
                        gather_finish(gatherRes, &player, &hotbar, floatTexts);
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
                                bag_consume_n(
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
                            craft_finish_recipe(craftRecipe, &player,
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

                hit.damage *= title_bow_mult(&player)                       // Nişancı
                            * title_damage_mult(&player, at->type,
                                                gametime_is_night());       // aile/gece (M3)
                if ((player.sneaking || at->sleeping) && !at->aggro)
                {
                    hit.damage *= perkFx.backstabMult
                                + title_backstab_bonus(&player);
                    title_notify(title_stat_add(&player, TS_SNEAK_HITS, 1),
                                 &player, floatTexts);
                    SDL_Color purple = { 200, 140, 255, 255 };
                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                         tx, at->e.rect.y - 26.0f,
                                         "Sinsi ok!", purple);
                    if (skill_add_xp(player.skills, SKILL_SNEAK, SNEAK_XP_BACKSTAB))
                        on_skill_levelup(&player, SKILL_SNEAK, &hotbar, floatTexts);
                }

                at->e.health -= hit.damage;
                strike_fx_mob(at, false, 0.03f);
                at->e.hitFlash = 0.15f;
                at->arrowsHit++;   // leşten ~%30 geri (mob_die)
                audio_play(hit.crit ? SFX_CRIT : SFX_HIT);
                if (hit.crit)
                    fx_burst(&weaponFx, tx, ty, 12, 90.0f, 255, 240, 120);

                // Alev rünü ok ucuna da işler (M1): yakma %20/3sn
                if (player.weaponFx == WEAPON_FX_FLAME)
                {
                    at->burnDps = hit.damage * 0.20f / 3.0f;
                    at->burnTime = 3.0f * weather_burn_time_mult();
                }
                // Zehir rünü/kaplaması ok ucuna da işler — kanamayla üst üste biner
                if (player.weaponFx == WEAPON_FX_POISON)
                {
                    at->poisonDps = hit.damage * 0.25f / 4.0f;
                    at->poisonTime = 4.0f;
                }

                // Buz Oku: %40 yavaşlatma 3 sn + buz kristali serpintisi
                // (karda süre ×1.5 — buz kendi mevsiminde tutar)
                if (arrows[a].kind == 1)
                {
                    at->slowMult = 0.6f;
                    at->slowTimer = 3.0f * weather_slow_dur_mult();
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
                        strike_fx_mob(nb, true, 0.0f);
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

            // Büyü dalgası 2 animasyonları (16 karelik tek atımlar)
            for (int q = 0; q < MAX_CLAW_FX; q++)
            {
                if (!clawFx[q].active) continue;
                clawFx[q].t += dt;
                if (clawFx[q].t >= 16 * 0.045f) clawFx[q].active = false;
            }
            for (int q = 0; q < MAX_ICETACLE_FX; q++)
            {
                if (!icetacleFx[q].active) continue;
                icetacleFx[q].t += dt;
                if (icetacleFx[q].t >= 16 * 0.06f) icetacleFx[q].active = false;
            }
            for (int q = 0; q < MAX_DARKBURST_FX; q++)
            {
                if (!darkburstFx[q].active) continue;
                darkburstFx[q].t += dt;
                if (darkburstFx[q].t >= 16 * 0.05f) darkburstFx[q].active = false;
            }

            // YILAN ISIRIĞI animasyonu (16 karelik tek atım)
            for (int q = 0; q < MAX_SNAKEBITE_FX; q++)
            {
                if (!snakeFx[q].active) continue;
                snakeFx[q].t += dt;
                if (snakeFx[q].t >= 16 * 0.05f) snakeFx[q].active = false;
            }

            // ATEŞ ASLANI: hat boyunca koşar — çarptığı HER moba bir kez vurur
            for (int q = 0; q < MAX_FIRESURGES; q++)
            {
                FireSurge *fl = &firesurges[q];
                if (!fl->active) continue;
                fl->t += dt;
                if (fl->t >= FIRESURGE_LIFE) { fl->active = false; continue; }
                fl->x += fl->dirX * FIRESURGE_SPEED * dt;
                fl->y += fl->dirY * FIRESURGE_SPEED * dt;
                for (int m2 = 0; m2 < mobCount; m2++)
                {
                    Mob *nb = &mobs[m2];
                    if (!nb->e.alive) continue;
                    float ndx = (nb->e.rect.x + nb->e.rect.w / 2.0f) - fl->x;
                    float ndy = (nb->e.rect.y + nb->e.rect.h / 2.0f) - fl->y;
                    if (ndx * ndx + ndy * ndy > 55.0f * 55.0f) continue;
                    bool already = false;
                    for (int h = 0; h < fl->hitN; h++)
                        if (fl->hitIds[h] == nb->id) { already = true; break; }
                    if (already) continue;
                    if (fl->hitN < 8) fl->hitIds[fl->hitN++] = nb->id;
                    float fdmg = fl->damage
                               * (100.0f / (100.0f + nb->type->defense))
                               * title_damage_mult(&player, nb->type,
                                                   gametime_is_night());
                    nb->e.health -= fdmg;
                    strike_fx_mob(nb, true, 0.0f);
                    nb->e.hitFlash = 0.15f;
                    nb->aggro = true;
                    mob_alert_camp(mobs, mobCount, nb);
                    // Yakma: Ateş Topu ile aynı kanal (Termal Şok'a hazırlar)
                    nb->burnDps = fdmg * 0.30f * (1.0f + perkFx.burnBonus) / 3.0f;
                    nb->burnTime = 3.0f * weather_burn_time_mult();
                    floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                    nb->e.rect.x + nb->e.rect.w / 2.0f + 10.0f,
                                    nb->e.rect.y, (int)fdmg, false);
                    float sxp = combat_xp_for_hit(fdmg, nb->type->level,
                                    player.skills[SKILL_DESTRUCTION].level);
                    if (skill_add_xp(player.skills, SKILL_DESTRUCTION, sxp))
                        on_skill_levelup(&player, SKILL_DESTRUCTION,
                                         &hotbar, floatTexts);
                    if (nb->e.health <= 0.0f)
                        mob_die(nb, mobs, MAX_MOBS, &nextMobId, loots,
                                questStatus, floatTexts, &player);
                }
            }

            // HORTUM: süzülür + 0.5 sn'de bir menzilindekileri döver
            for (int q = 0; q < MAX_TORNADOES; q++)
            {
                Tornado *tn = &tornadoes[q];
                if (!tn->active) continue;
                tn->t += dt;
                if (tn->t >= tn->life) { tn->active = false; continue; }
                if (tn->t > 0.3f)   // kalkış karelerinde yerinde döner
                {
                    tn->x += tn->dirX * 65.0f * dt;
                    tn->y += tn->dirY * 65.0f * dt;
                }
                tn->tickTimer -= dt;
                if (tn->tickTimer <= 0.0f)
                {
                    tn->tickTimer += 0.5f;
                    for (int m2 = 0; m2 < mobCount; m2++)
                    {
                        Mob *nb = &mobs[m2];
                        if (!nb->e.alive) continue;
                        float ndx = (nb->e.rect.x + nb->e.rect.w / 2.0f) - tn->x;
                        float ndy = (nb->e.rect.y + nb->e.rect.h / 2.0f) - tn->y;
                        if (ndx * ndx + ndy * ndy > 60.0f * 60.0f) continue;
                        float tdmg = tn->tickDamage
                                   * (100.0f / (100.0f + nb->type->defense));
                        nb->e.health -= tdmg;
                        strike_fx_mob(nb, true, 0.0f);
                        nb->e.hitFlash = 0.1f;
                        nb->slowMult = 0.7f;      // hortumun içinde debelenme
                        nb->slowTimer = 1.0f;
                        nb->aggro = true;
                        floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                        nb->e.rect.x + nb->e.rect.w / 2.0f + 10.0f,
                                        nb->e.rect.y, (int)tdmg, false);
                        float sxp = combat_xp_for_hit(tdmg, nb->type->level,
                                        player.skills[SKILL_DESTRUCTION].level);
                        if (skill_add_xp(player.skills, SKILL_DESTRUCTION, sxp))
                            on_skill_levelup(&player, SKILL_DESTRUCTION,
                                             &hotbar, floatTexts);
                        if (nb->e.health <= 0.0f)
                            mob_die(nb, mobs, MAX_MOBS, &nextMobId, loots,
                                    questStatus, floatTexts, &player);
                    }
                }
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
                int pBlock = 0;
                float dmg = combat_mob_damage(mobProjs[pj].dmg, &equipment, &perkFx,
                                              player.state == PLAYER_IDLE,
                                              player.health / player.maxHealth,
                                              yaraliAyi,
                                              player.skills[SKILL_BLOCK].level,
                                              player.blocking, player.blockHold,
                                              &dodged, &pBlock);
                block_feedback(pBlock, &player, &hotbar, floatTexts, &perkFx);
                if (pBlock > 0 && g_animFx != NULL)
                    animfx_spawn(g_animFx, g_sparkTex, 7, 7, 48.0f, 48.0f,
                                 player.rect.x + player.rect.w / 2.0f,
                                 player.rect.y + player.rect.h * 0.45f,
                                 30.0f, 1.2f);
                if (pBlock == 3) hitstop(0.12f);   // PERFECT parry donması
                // IŞIK TAŞIYICISI: kendi Kutsal Alanında blok %20 güçlü
                if (pBlock > 0 && holyZone.on && perkFx.holyZoneBlockCut > 0.0f)
                {
                    float hzdx = (player.rect.x + player.rect.w / 2.0f) - holyZone.x;
                    float hzdy = (player.rect.y + player.rect.h / 2.0f) - holyZone.y;
                    if (hzdx * hzdx + hzdy * hzdy <= holyZone.r * holyZone.r)
                        dmg *= 1.0f - perkFx.holyZoneBlockCut;
                }
                // (mermide sersemletme/yansıtma yok — atan uzakta)
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

            // Gece tehlikesi: algı menzili 1.5x (spawn çarpanı KALKTI —
            // dünya ekolojik, zindan mühür dalgasıyla dolar)
            float detectMul = (gametime_is_night() ? 1.5f * perkFx.nightDetectMult : 1.0f)
                            * perkFx.detectOnPlayerMult    // Hayalet/Hortlak düğümleri
                            * (map_current() == MAP_WORLD
                               ? weather_percept_mult() : 1.0f)    // sis örter
                            * (player.shadowTimer > 0.0f ? 0.05f : 1.0f)   // Gölge Formu
                            * (player.smokeTimer > 0.0f ? 0.05f : 1.0f);   // Duman Perdesi

            // Hız çarpanı: perk (Çevik/Rüzgar/Uyumlu) × gece (Gölge Adımı) × eğilme.
            // player_update tile hızıyla çarpar (doğrudan speed yazmak eziliyordu — bug'dı)
            player.speedMult = (player.shadowTimer > 0.0f
                                ? 1.2f + perkFx.shadowSpeedBonus : 1.0f)
                             * (player.smokeTimer > 0.0f
                                ? 1.0f + perkFx.smokeSpeedBonus : 1.0f)
                             * (player.blocking
                                ? (perkFx.stanceSlowMult < 0.0f ? 1.0f       // FALANKS
                                   : perkFx.stanceSlowMult > 0.0f
                                     ? 1.0f - 0.5f * perkFx.stanceSlowMult   // Kök Salma
                                     : 0.5f)                                 // taban ceza
                                : 1.0f)
                             * (player.gateStance ? 0.6f : 1.0f) // Kapı Tutucu duruşu
                             * (map_current() == MAP_WORLD
                                ? weather_speed_mult() : 1.0f)   // kar bastırıyor
                             * (player.foodBuffType == FB_STA_SPEED ? 1.05f : 1.0f)
                             * (1.0f - armorSpeedPenalty)   // zırh ağırlığı (STR telafili)
                             * perkFx.speedMult
                             * (gametime_is_night() ? perkFx.nightSpeedMult : 1.0f)
                             * (player.sneaking
                                ? (sneak_speed_mult(player.skills[SKILL_SNEAK].level) +
                                   (1.0f - sneak_speed_mult(player.skills[SKILL_SNEAK].level)) *
                                   perkFx.sneakSpeedRelief)
                                : 1.0f)
                             * (player.speedBuffTimer > 0.0f ? player.speedBuffMult : 1.0f);
            // --- MÜHÜR DALGASI (Cila düzeltmesi): zindanda anlık respawn YOK —
            // periyodik toplu diriliş. Uyarı önce gelir ("Mühür titriyor...");
            // dalga eksik zone'ları doldurur ama OYUNCUNUN DİBİNDE doğurmaz
            // (yakın odalar bir sonraki dalgayı bekler). BOSS dalgaya dahil
            // değil — haftalık diriliş kuralı ayrı işler (farm yok).
            if (map_is_dungeon(map_current()))
            {
                sealWaveTimer -= dt;
                if (!sealWaveWarned && sealWaveTimer <= 3.0f)
                {
                    sealWaveWarned = true;
                    audio_play(SFX_WHISPER);
                    shakeTime = shakeDur = 0.4f;
                    shakeAmp = 3.0f;
                    SDL_Color seal = { 170, 140, 235, 255 };
                    floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                         player.rect.x + player.rect.w / 2.0f,
                                         player.rect.y - 26.0f,
                                         "Mühür titriyor...", seal);
                }
                if (sealWaveTimer <= 0.0f)
                {
                    sealWaveTimer = SEAL_WAVE_SECONDS;
                    sealWaveWarned = false;
                    float wpx = player.rect.x + player.rect.w / 2.0f;
                    float wpy = player.rect.y + player.rect.h / 2.0f;
                    for (int z = 0; z < spawnZoneCount; z++)
                    {
                        SpawnZone *sz = &spawnZones[z];
                        float zdx = sz->x - wpx, zdy = sz->y - wpy;
                        if (zdx * zdx + zdy * zdy < 280.0f * 280.0f)
                            continue;   // oyuncunun gözü önünde dikilmek yok
                        int aliveZ = 0;
                        for (int m2 = 0; m2 < MAX_MOBS; m2++)
                            if (mobs[m2].zoneId == z &&
                                (mobs[m2].e.alive || mobs[m2].dying))
                                aliveZ++;
                        for (int k = aliveZ; k < sz->maxCount; k++)
                        {
                            float ang = randf() * 6.2832f;
                            float rr = randf() * sz->radius;
                            mob_spawn(mobs, MAX_MOBS, &nextMobId, sz->type,
                                      sz->x + SDL_cosf(ang) * rr,
                                      sz->y + SDL_sinf(ang) * rr, z);
                        }
                    }
                }
            }

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
                // M4 taunt: kabuk çekiyorsa mob hedefi kabuk olur
                float mtx = px, mty = py;
                if (mobs[i].tauntTimer > 0.0f && ally_active(allies, ALLY_TURTLE))
                {
                    mtx = allies[ALLY_TURTLE].x;
                    mty = allies[ALLY_TURTLE].y;
                }
                mob_update(&mobs[i], mobs, mobCount, mtx, mty, dt, detectMul * stealthMul);

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
                // CAN EMME (Yankıcı P2): emilen HP oyuncuya AKAR (mor sızıntı)
                if (mobs[i].drainTime > 0.0f && mobs[i].e.alive)
                {
                    mobs[i].drainTime -= dt;
                    float drained = mobs[i].drainDps * dt;
                    mobs[i].e.health -= drained;
                    if (player.health > 0.0f)
                    {
                        player.health += drained;
                        if (player.health > player.maxHealth)
                            player.health = player.maxHealth;
                    }
                    if (randf() < dt * 6.0f)
                        fx_spawn(&weaponFx,
                                 mobs[i].e.rect.x + mobs[i].e.rect.w / 2.0f
                                     + (randf() - 0.5f) * 20.0f,
                                 mobs[i].e.rect.y + mobs[i].e.rect.h / 2.0f,
                                 (randf() - 0.5f) * 30.0f, -40.0f - randf() * 30.0f,
                                 0.5f, 4.0f + randf() * 4.0f, 170, 100, 230,
                                 WEAPON_FX_NONE);
                    // Emiş XP'si: tik başına (küçük ama sürekli)
                    float sxp = combat_xp_for_hit(drained,
                                    mobs[i].type->level,
                                    player.skills[SKILL_ECHO].level);
                    if (skill_add_xp(player.skills, SKILL_ECHO, sxp))
                        on_skill_levelup(&player, SKILL_ECHO, &hotbar, floatTexts);
                    if (mobs[i].e.health <= 0.0f && mobs[i].e.alive)
                        mob_die(&mobs[i], mobs, MAX_MOBS, &nextMobId, loots,
                                questStatus, floatTexts, &player);
                }

                // İĞNE ZEHRİ (Suikastçi imzası): doz × DPS; 3 dozda SESSİZ SON
                // eşiği (%20 altı = anında ölüm; boss'ta eşik yok, DoT ×1.5)
                if (mobs[i].venomTime > 0.0f && mobs[i].e.alive)
                {
                    mobs[i].venomTime -= dt;
                    float vd = mobs[i].venomDoseDps * (float)mobs[i].venomDoses
                             * ((mobs[i].type->isBoss && mobs[i].venomDoses >= 3)
                                ? 1.5f : 1.0f);
                    mobs[i].e.health -= vd * dt;
                    if (randf() < dt * 5.0f)
                        fx_spawn(&weaponFx,
                                 mobs[i].e.rect.x + mobs[i].e.rect.w / 2.0f
                                     + (randf() - 0.5f) * 18.0f,
                                 mobs[i].e.rect.y + mobs[i].e.rect.h * 0.4f,
                                 (randf() - 0.5f) * 12.0f, 30.0f + randf() * 30.0f,
                                 0.5f, 4.0f + randf() * 3.0f, 130, 60, 160,
                                 WEAPON_FX_VENOM);
                    float sxpV = combat_xp_for_hit(vd * dt,
                                     mobs[i].type->level,
                                     player.skills[SKILL_ASSASSIN].level);
                    if (skill_add_xp(player.skills, SKILL_ASSASSIN, sxpV))
                        on_skill_levelup(&player, SKILL_ASSASSIN, &hotbar, floatTexts);
                    bool silentEnd = !mobs[i].type->isBoss &&
                                     mobs[i].venomDoses >= 3 &&
                                     mobs[i].e.health < mobs[i].e.maxHealth *
                                         (0.20f + perkFx.silentThresholdBonus);
                    if (mobs[i].e.health <= 0.0f || silentEnd)
                    {
                        if (silentEnd)
                        {
                            SDL_Color vio10 = { 200, 140, 255, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                mobs[i].e.rect.x + mobs[i].e.rect.w / 2.0f,
                                mobs[i].e.rect.y - 30.0f, "Sessiz Son.", vio10);
                            audio_play(SFX_WHISPER);
                        }
                        mob_die(&mobs[i], mobs, MAX_MOBS, &nextMobId, loots,
                                questStatus, floatTexts, &player);
                    }
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
                else if (mobs[i].attackTriggered &&
                         mobs[i].tauntTimer > 0.0f &&
                         ally_active(allies, ALLY_TURTLE))
                {
                    // M4: taunt'lu mob KABUĞU döver (oyuncu yerine)
                    allies[ALLY_TURTLE].hp -= mobs[i].type->damage;
                    fx_burst(&weaponFx, allies[ALLY_TURTLE].x,
                             allies[ALLY_TURTLE].y, 6, 60.0f, 120, 235, 110);
                }
                else if (mobs[i].attackTriggered)
                {
                    // Saldıran mob otomatik hedeflenir (yalnız hedef boşken —
                    // seçili mob/NPC hedefini savaş ortasında EZMEZ)
                    if (player.targetMobId == 0 && targetNpc < 0)
                        player.targetMobId = mobs[i].id;

                    bool dodged = false;
                    bool yaraliAyi = perkFx.yaraliAyi;
                    int mBlock = 0;
                    float dmg = combat_mob_damage(mobs[i].type->damage, &equipment,
                                                  &perkFx,
                                                  player.state == PLAYER_IDLE,
                                                  player.health / player.maxHealth,
                                                  yaraliAyi,
                                                  player.skills[SKILL_BLOCK].level,
                                                  player.blocking, player.blockHold,
                                                  &dodged, &mBlock);
                    block_feedback(mBlock, &player, &hotbar, floatTexts, &perkFx);
                    if (mBlock > 0 && g_animFx != NULL)
                        animfx_spawn(g_animFx, g_sparkTex, 7, 7, 48.0f, 48.0f,
                                     player.rect.x + player.rect.w / 2.0f,
                                     player.rect.y + player.rect.h * 0.45f,
                                     30.0f, 1.2f);
                    if (mBlock == 3) hitstop(0.12f);   // PERFECT parry donması
                    // IŞIK TAŞIYICISI: kendi Kutsal Alanında blok %20 güçlü
                    if (mBlock > 0 && holyZone.on && perkFx.holyZoneBlockCut > 0.0f)
                    {
                        float hzdx = (player.rect.x + player.rect.w / 2.0f) - holyZone.x;
                        float hzdy = (player.rect.y + player.rect.h / 2.0f) - holyZone.y;
                        if (hzdx * hzdx + hzdy * hzdy <= holyZone.r * holyZone.r)
                            dmg *= 1.0f - perkFx.holyZoneBlockCut;
                    }
                    if (mBlock == 3)
                    {
                        // PERFECT (parry): saldıran 1sn sersemler; SİPER yansıtır
                        mobs[i].slowMult = 0.05f;
                        mobs[i].slowTimer = 1.0f;
                        mobs[i].attackCooldown = 1.0f;
                        mobs[i].alertFlash = 0.6f;
                        if (perkFx.perfectReflectFrac > 0.0f)
                        {
                            float rf = mobs[i].type->damage * perkFx.perfectReflectFrac;
                            mobs[i].e.health -= rf;
                            floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                            mobs[i].e.rect.x + mobs[i].e.rect.w / 2.0f,
                                            mobs[i].e.rect.y - 10.0f, (int)rf, false);
                            if (mobs[i].e.health <= 0.0f)
                                mob_die(&mobs[i], mobs, MAX_MOBS, &nextMobId, loots,
                                        questStatus, floatTexts, &player);
                        }
                    }
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
                journal_add(JE_DEATH, 0);
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

            // --- MÜZİK MOOD'U: boss > savaş > bölge; gece açık arazi tekinsiz ---
            // Savaş: yakında seni kovalayan/sana saldıran mob var mı?
            // (boss bulunana dek tarar — boss müziği sıradan savaşı ezer)
            bool inCombat = false, bossCombat = false;
            for (int i = 0; i < mobCount && !bossCombat; i++)
            {
                if (!mobs[i].e.alive) continue;
                if (!mobs[i].aggro) continue;   // devriye/su ziyareti CHASE clip'i kullanır — savaş değil
                if (mobs[i].state != MOB_CHASE && mobs[i].state != MOB_ATTACK) continue;
                float mdx = (mobs[i].e.rect.x + mobs[i].e.rect.w / 2.0f) - px;
                float mdy = (mobs[i].e.rect.y + mobs[i].e.rect.h / 2.0f) - py;
                if (mdx * mdx + mdy * mdy < 500.0f * 500.0f)
                {
                    inCombat = true;
                    if (mobs[i].type->isBoss) bossCombat = true;
                }
            }

            MusicMood mood;
            if (inCombat)
            {
                mood = bossCombat ? MOOD_BOSS : MOOD_COMBAT;
            }
            else if (map_current() != MAP_WORLD)
            {
                mood = MAPS[map_current()].mood;   // zindan: sabit tekinsizlik
            }
            else
            {
                int mreg = region_at(px, py);
                mood = (mreg >= 0) ? REGIONS[mreg].mood : MOOD_WILD;
                if (mood == MOOD_WILD && gametime_is_night())
                    mood = MOOD_NIGHT;   // gece açık arazi: koyu pad (Cila)
            }
            music_set_mood(mood);
            ambience_set(map_current() != MAP_WORLD ? AMB_DUNGEON
                        : gametime_is_night() ? AMB_NIGHT : AMB_DAY);

            loot_update_all(loots, MAX_LOOT, dt);

            floattext_update_all(floatTexts, MAX_FLOAT_TEXTS, dt);

            if (cineActive)
            {
                cineTimer -= dt;
                if (cineTimer <= 0.0f) cineActive = false;
            }
            // ESO diyalog sunumu: konuşurken kamera ikilinin ortasına süzülür,
            // sinematik bantlar iner (cine ile aynı bant değişkeni)
            float dlgCamX = 0.0f, dlgCamY = 0.0f;
            bool dlgCam = false;
            if (dialogNpc >= 0)
            {
                float dnx, dny;
                if (npc_visible_pos(dialogNpc, &dnx, &dny))
                {
                    dlgCamX = (player.rect.x + player.rect.w / 2.0f + dnx) / 2.0f;
                    dlgCamY = (player.rect.y + player.rect.h / 2.0f + dny) / 2.0f - 30.0f;
                    dlgCam = true;
                }
            }
            cineBand += ((cineActive ? 1.0f : 0.0f) - cineBand)
                      * SDL_min(1.0f, dt * 5.0f);
            // Diyalog kendi ÜST bandını kullanır (alt bant panelin üstüne
            // biniyordu — "metnin üstünden geçen siyah gölge" bug'ı); kamera
            // kaydırması cineBand'den bağımsız (dlgCam) korunur
            dlgBand += ((dlgCam ? 1.0f : 0.0f) - dlgBand) * SDL_min(1.0f, dt * 6.0f);
            camera_update(&camera, cineActive ? cineX
                                 : dlgCam ? dlgCamX
                                 : player.rect.x + player.rect.w / 2.0f,
                          cineActive ? cineY
                                 : dlgCam ? dlgCamY
                                 : player.rect.y + player.rect.h / 2.0f,
                          winW, winH);   // kamera: cine > diyalog > oyuncu

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

            // --- M4: dostların güncellenmesi + vuruşları (tek noktadan) ---
            {
                AllyHit hits[ALLY_MAX];
                ally_update(allies, dt, px, py, mobs, MAX_MOBS,
                            player.skills[SKILL_CONJURATION].level,
                            perkFx.summonDmgMult, hits);
                for (int ai = 0; ai < ALLY_MAX; ai++)
                {
                    if (hits[ai].targetMobId == 0) continue;
                    Mob *tm = mob_find_by_id(mobs, MAX_MOBS, hits[ai].targetMobId);
                    if (tm == NULL || !tm->e.alive) continue;
                    float ad = hits[ai].damage
                             * title_damage_mult(&player, tm->type,
                                                 gametime_is_night());
                    tm->e.health -= ad;
                    tm->e.hitFlash = 0.12f;
                    if (hits[ai].poison)
                    {
                        tm->poisonDps = ad * 0.25f * perkFx.snakePoisonMult / 4.0f;
                        tm->poisonTime = 4.0f;
                    }
                    floattext_spawn(floatTexts, MAX_FLOAT_TEXTS,
                                    tm->e.rect.x + tm->e.rect.w / 2.0f + 8.0f,
                                    tm->e.rect.y - 6.0f, (int)ad, false);
                    float axp = combat_xp_for_hit(ad, tm->type->level,
                                    player.skills[SKILL_CONJURATION].level);
                    if (skill_add_xp(player.skills, SKILL_CONJURATION, axp))
                        on_skill_levelup(&player, SKILL_CONJURATION,
                                         &hotbar, floatTexts);
                    if (tm->e.health <= 0.0f)
                        mob_die(tm, mobs, MAX_MOBS, &nextMobId, loots,
                                questStatus, floatTexts, &player);
                }

                // Dost ölümü: CD basılır + KURBAN BAĞI bedeli
                for (int ai = 0; ai < ALLY_MAX; ai++)
                {
                    bool now = allies[ai].active;
                    if (allyWasActive[ai] && !now)
                    {
                        int abId = (ai == ALLY_TURTLE) ? ABILITY_SUMMON_TURTLE
                                                       : ABILITY_SUMMON_SNAKE;
                        player.abilityCd[abId] = ABILITIES[abId].cooldown;
                        if (perkFx.summonDeathHurt)
                        {
                            player.health -= 15.0f;   // KURBAN BAĞI: bağ koptu
                            SDL_Color red2 = { 255, 90, 80, 255 };
                            floattext_spawn_text(floatTexts, MAX_FLOAT_TEXTS,
                                                 px, player.rect.y - 26.0f,
                                                 "Bağ koptu!", red2);
                        }
                    }
                    allyWasActive[ai] = now;
                }
            }

            // E prompt'u için menzildeki en yakın NPC
            nearestNpc = npc_nearest_in_range(px, py);
            nearestRes = resource_nearest_available(px, py);
            nearestStation = station_nearest(px, py);
            nearestLamp = (map_current() == MAP_WORLD) ? lamp_nearest(px, py) : -1;

            // KARANLIĞIN SESİ (LORE §1.2): kandil halkasının dışında, geceleyin,
            // taşın kaydı duyulur hale gelir — ışığa girince kesilir. Kalıcı
            // uğultu katmanı yerine aralıklı fısıltı: yeni ses varlığı istemez.
            if (map_current() == MAP_WORLD && gametime_is_night())
            {
                // Fısıltı da AYNI sınırı kullanır: ışığın bittiği yer.
                // Üç ayrı eşik vardı (0.25 / 0.15 / ham yarıçap) — oyuncu
                // fısıltıdan bir sınır öğrenip ölülerde başka sınır buluyordu.
                if (lamp_level_at(px, py) <= 0.0f)
                {
                    whisperTimer -= dt;
                    if (whisperTimer <= 0.0f)
                    {
                        audio_play(SFX_WHISPER);
                        whisperTimer = 5.0f + randf() * 5.0f;
                    }
                }
                else whisperTimer = 2.5f;   // halkaya girince sus
            }

            // Köye varış (Faz B1): muhafız yaklaşan yabancıyı DURDURUR —
            // ilk karşılama diyaloğu kendiliğinden açılır (bir kez)
            if (!player.guardGreeted && guardIdx >= 0 && dialogNpc < 0 &&
                map_current() == MAP_WORLD && !scene_active() &&
                nearestNpc == guardIdx && npcDialogOk[guardIdx])
            {
                int kn = dialog_find(&npcDialogs[guardIdx], "karsilama");
                if (kn >= 0)
                {
                    dialogNpc = guardIdx;
                    dialogNode = kn;
                    dialog_type_reset();
                    player.autoAttack = false;
                    player.guardGreeted = true;
                    audio_play(SFX_UI);
                }
            }

            // --- BÖLGE TAKİBİ: yeni bölgeye girince adı belirir (yalnız dünya) ---
            int reg = (map_current() == MAP_WORLD) ? region_at(px, py) : currentRegion;
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

        if (gameState == GAME_TITLE || gameState == GAME_MENU ||
            gameState == GAME_CREATE)
        {
            music_set_mood(MOOD_MENU);   // başlık/menü/yaratma: mühür nabzı
            ambience_set(AMB_NONE);
        }
        music_update(dt);   // fade + loop besleme (menüde/ölümde de akar)
        ambience_update(dt);

        // Kare başlat (clear rengi gri); swapchain yoksa bu kare çizilmez
        if (!r2d_begin_frame(renderer, 155, 155, 155))
            continue;
        tilemap_render(renderer, tile_textures, &camera);     // haritayı çiz
        if (map_current() == MAP_WORLD)
            tilemap_vfxdemo_render(renderer, tile_textures, &camera);  // "su olmadan su" testi
        if (map_current() == MAP_WORLD || map_current() == MAP_DOGU)
            tilemap_render_scatter(renderer, &camera);         // süs serpiştirmesi
        // GÜNEŞ GÖLGELERİ: zeminin üstüne, dekorun ALTINA. Tek blok halinde
        // çizilir — böylece bütün gölgeler bütün sprite'ların altında kalır ve
        // mevcut çizim sırası bozulmaz (bir ağaç, bir karakterin gölgesini
        // kapatır: fiziksel olarak doğru).
        {
            // KAPALI MEKÂN KAPISI: MapDef.ambient > 0 = sabit loşluk (zindan,
            // konak, kuyu dibi) — güneş oraya girmez. Eskiden kuyunun dibinde
            // 4h uzunluğunda güneş gölgesi çiziliyordu.
            bool sunOK = (MAPS[map_current()].ambient == 0.0f);
            float sdx, sdy, sLen, sStr;
            if (sunOK && shadow_sun(&sdx, &sdy, &sLen, &sStr))
                decor_render_shadows(renderer, decor_textures, &camera,
                                     sdx, sdy, sLen, sStr);

            // Karakter gölgeleri: temas gölgesi HER ZAMAN (gece dahil) —
            // "zemine yapıştırılmış" hissini en çok karakterler veriyordu.
            // Güneş kuyruğu shadow_blob içinde, gündüzse kendiliğinden ekleniyor.
            // Sprite karesi ile GÖRÜNEN gövde aynı şey değil: ULPC 64px
            // karesinde ayaklar ~y=60'ta, gövde ~44px. Ham rect kullanmak
            // kuyruğu %45 uzatıyor ve lekeyi ayakların altına atıyordu.
            #define SH_FOOT_PAD 0.06f   // kare yüksekliğinin altındaki boşluk
            #define SH_BODY     0.70f   // görünen gövde / kare yüksekliği
            #define SH_WIDE     0.55f   // görünen omuz / kare genişliği
            shadow_blob(renderer, &camera,
                        player.rect.x + player.rect.w * 0.5f,
                        player.rect.y + player.rect.h * (1.0f - SH_FOOT_PAD),
                        player.rect.w * SH_WIDE, player.rect.h * SH_BODY);
            for (int mi = 0; mi < MAX_MOBS; mi++) {
                if (!mobs[mi].e.alive) continue;
                shadow_blob(renderer, &camera,
                            mobs[mi].e.rect.x + mobs[mi].e.rect.w * 0.5f,
                            mobs[mi].e.rect.y + mobs[mi].e.rect.h * (1.0f - SH_FOOT_PAD),
                            mobs[mi].e.rect.w * SH_WIDE,
                            mobs[mi].e.rect.h * SH_BODY);
            }
        }
        // YER İZLERİ: gölgelerden sonra, dekordan önce — zeminin üstünde ama
        // her şeyin altında. Gölgeden sonra olması bilinçli: gölge zemine ait,
        // iz onun üstüne biner (yanık taşın rengini değiştirir, gölgeyi değil).
        decal_render(renderer, &camera);

        decor_render_all(renderer, decor_textures, &camera);  // evler (zemin üstü, entity altı)
        if (map_is_dungeon(map_current()))
            trap_render(renderer, map_current(), &camera);   // dikenler/plaka/arbalet (zemin)
        map_portals_render(renderer, &camera, gameFont, nearPortal);  // geçit kapı/merdivenleri

        // SU YANSIMALARI: dekorlar (ağaç/çalı) suya aynalanır. Su geçişinden
        // ÖNCE olmak ZORUNDA — yansıma zemin katmanına girer ki kırılma onu
        // büksün, derinlik süzgeci koyultsun. Sonra çizilse cam üstüne
        // yapıştırılmış çıkartma gibi dururdu.
        decor_render_reflections(renderer, decor_textures, &camera);
        {   // Oyuncu yansıması — katman dizisi asıl çizim için 11163'te tekrar
            // kurulur; burada ikinci kez kurmak taşımaktan ucuz ve güvenli
            // (yalnız pointer montajı, kare başına iki kez zararsız).
            PlayerLayer rbuilt[24];
            int rn = build_player_layers(layers, LAYER_COUNT, &equipment,
                                         &itemTex, rbuilt, 24);
            player_render_reflection(&player, renderer, rbuilt, rn, &camera);
        }

        // SU HACMİ (Faz SU). Buraya kadar çizilen HER ŞEY zemin sayılır (karo,
        // gölge, yer izi, dekor, geçit) ve suyun ALTINA girer: batık taş, urgan,
        // çakıl hepsi bükülür. Varlıklar bundan SONRA çizildiği için oyuncu
        // suyun üstünde kalır (sığlıkta yürüyor, suyun altında değil).
        {
            static R2DTexture *wmTex = NULL;
            static int wmMap = -1, wmW = 0, wmH = 0;
            if (wmMap != (int)map_current()) {   // harita değişti → maske yenilenir
                wmMap = (int)map_current();
                if (wmTex) { r2d_texture_destroy(renderer, wmTex); wmTex = NULL; }
                const Uint8 *pix = NULL;
                if (wld_water_mask_fill(&wmW, &wmH, &pix)) {
                    wmTex = r2d_texture_from_pixels(renderer, pix, wmW, wmH);
                    // LINEAR ŞART: derinlik rampası ve köşesiz kıyı silueti
                    // örnekleyiciden geliyor. NEAREST olursa kıyı yine basamaklanır.
                    if (wmTex) r2d_texture_set_linear(wmTex, true);
                }
            }
            if (wmTex != NULL) {
                // Güneş azimutu: 6'da doğudan, 12'de tepede, 18'de batıdan.
                // Sabah ile akşam AYNI GÖRÜNMÜYOR — parlama yön değiştiriyor.
                float hrs = gametime_hours();
                float ang = (hrs - 6.0f) / 12.0f * 3.14159265f;
                float high = SDL_sinf(ang); if (high < 0.0f) high = 0.0f;
                float sx = SDL_cosf(ang), sy = -0.5f;
                float sl = SDL_sqrtf(sx * sx + sy * sy);
                float sunI = 1.0f - gametime_darkness01();
                // GECE LAMBA YANSIMASI: ışık listesi asıl pass için ileride
                // yeniden toplanır; burada ikinci toplama ucuz ve güvenli
                // (build_player_layers ile aynı gerekçe). En parlak iki ışık
                // ekran→dünya çevrilip shader'a verilir.
                float l1x = 0, l1y = 0, l1i = 0, l2x = 0, l2y = 0, l2i = 0;
                float darkNow = gametime_darkness01();
                if (darkNow > 0.05f) {
                    bool rTorch = false;
                    for (int ti = 0; ti < INVENTORY_SIZE; ti++)
                        if (inventory.slots[ti].type == &ITEM_TYPES[ITEM_TORCH]) { rTorch = true; break; }
                    R2DLight rl[R2D_MAX_LIGHTS];
                    int rn = light_collect(rl, R2D_MAX_LIGHTS, &camera, winW, winH,
                                           !map_is_dungeon(map_current()),
                                           player.rect.x + player.rect.w / 2.0f,
                                           player.rect.y + player.rect.h / 2.0f,
                                           rTorch, player.weaponFx);
                    int b1 = -1, b2 = -1;
                    for (int li = 0; li < rn; li++) {
                        float sc = rl[li].intensity * rl[li].radius;
                        if (b1 < 0 || sc > rl[b1].intensity * rl[b1].radius) { b2 = b1; b1 = li; }
                        else if (b2 < 0 || sc > rl[b2].intensity * rl[b2].radius) b2 = li;
                    }
                    if (b1 >= 0) {
                        l1x = rl[b1].x / camera.zoom + camera.x;
                        l1y = rl[b1].y / camera.zoom + camera.y;
                        l1i = rl[b1].intensity;
                    }
                    if (b2 >= 0) {
                        l2x = rl[b2].x / camera.zoom + camera.x;
                        l2y = rl[b2].y / camera.zoom + camera.y;
                        l2i = rl[b2].intensity;
                    }
                }
                R2DWater w = {
                    .mask = wmTex, .maskW = (float)wmW, .maskH = (float)wmH,
                    .tilePx = (float)TILE_SIZE,
                    .camX = camera.x, .camY = camera.y, .zoom = camera.zoom,
                    // DUVAR saati: su geceleyin yavaşlamaz, mevsim onu dondurmaz
                    .time = (float)SDL_GetTicks() / 1000.0f,
                    .sunX = sx / sl, .sunY = sy / sl, .sunI = sunI,
                    // Renk: tepede beyaza yakın, ufka inince ısınıyor
                    .sunR = 1.0f,
                    .sunG = 0.74f + 0.24f * high,
                    .sunB = 0.48f + 0.44f * high,
                    // Bir sanat pikselinin ekran pikseli — kırılma ofsetinin
                    // yuvarlandığı birim. Kesirli ofset = titreme.
                    .artPx = camera.zoom * ((float)TILE_SIZE / 16.0f),
                    .shallowR = 0.34f, .shallowG = 0.52f, .shallowB = 0.58f,
                    .shallowCover = 0.20f,     // sığda taban NET okunur
                    // Koyu uç MORA/MAVİYE kaydırıldı (0.07,0.21,0.34 → düz
                    // koyulaştırmaydı): pixel-art hue-shift kuralı — ALttP
                    // ölçümü de aynı şeyi söylüyor (gölge tonu morlaşıyor).
                    // Referans: göletin ortası OPAK KOYU LACİVERT (gri değil).
                    .deepR = 0.17f, .deepG = 0.27f, .deepB = 0.48f,
                    .deepCover = 0.80f,        // derin ama ışık geçiren (Hakan: açık+şeffaf)
                    .waveHeight = 0.10f,       // sırt sığ, çukur derin görünür
                    .refract = 14.0f,          // EKRAN px tepe ofseti; snap sonrası 2-3 sanat px
                    //   (7'de gölette 2.4 px kalıp sanat ızgarasında sıfıra yuvarlanıyordu)
                    .caustic = 0.14f,     // eklemeli: taşarsa su beyaza doyuyor
                    // KAPALI (Hakan: 'köşelerdeki beyaz hareketler') — kıyı
                    // atlası zaten çizili köpük taşıyor; bizim salınan beyaz
                    // bant onun üstüne binip kıyıyı kalabalıklaştırıyordu.
                    // Kod duruyor: fırtına/rüzgâr efekti gerekirse geri açılır.
                    .foam = 0.0f,
                    .glint = 0.16f,       // eklemeli: kostikle toplamı 1'i geçmemeli
                    .quant = 0.0f,             // renk kademesi kapalı
                    .foamSpeed = 0.9f,         // kıyıya vurup çekilme
                    .foamAmp = 0.16f,
                    // SU ÇİZGİSİ. Islak bant ve koyu hat ÇARPIMSAL (bütçe dışı),
                    // menisküs EKLEMELİ: 0.14 kostik + 0.16 parlama + 0.13 =
                    // 0.43 ≤ 0.45 tavanı. Tavan R8G8B8A8_UNORM yüzünden.
                    .darkLine = 1.0f,          // suyun silueti — tam güçte istiyoruz
                    .meniscus = 0.13f,
                    // KIYI GÖLGESİ — kamera 3/4 eğik, gölet bir çukur: kuzey
                    // kıyının iç duvarı suya gölge düşürür (maske A kanalı).
                    // ÇARPIMSAL, bütçe dışı. Gölgede kostik+parlama da söner.
                    .bankShadow = 0.68f,
                    // Işıltı çizgicikleri: pixel-su imzası, ayrık öğe. EKLEMELİ
                    // bütçede: 0.12 + kostik 0.14 + parlama 0.16 = 0.42 ≤ 0.45.
                    .dashes = 0.12f,
                    // Ark sıraları: kıyıya paralel koyu yankı çizgileri (ALttP).
                    .arcs = 1.0f,
                    .wallFace = 0.0f,          // duvar prototipi RAFTA (bkz. shader)
                    .lamp1X = l1x, .lamp1Y = l1y, .lamp1I = l1i,
                    .lamp2X = l2x, .lamp2Y = l2y, .lamp2I = l2i,
                    .darkness = darkNow,
                    // Yağmur halkaları: ıslak havada yüzey damlayla dolar
                    .rain = weather_is_wet() ? weather_intensity() : 0.0f,
                    // Islak bant 0.85 → 0.55: tam güçte göletin çevresine koyu
                    // bir çerçeve çekiyor ve "kakma pano" hissini büyütüyordu
                    // (ıslak toprak: koyulaşır + doygunlaşır).
                    .wetBand = 0.55f,
                    .layers = (waterLayer == 0)
                              ? (float)R2D_WATER_L_ALL
                              : (float)WATER_LAYERS[waterLayer - 1].bit,
                };
                r2d_water_flush(renderer, &w);
            }
        }

        // SU ÜSTÜ ÖRTÜSÜ: nilüferler — sudan SONRA (yüzeyin üstünde dururlar,
        // kırılma onları bükmez; bükülselerdi batık okunurlardı)
        tilemap_render_water_overlay(renderer, &camera);
        // Kumsal dalga vuruşu: kumun üstüne biner (kara efekti)
        tilemap_render_beach_swash(renderer, &camera);

        // SESSİZ EL kontrat tahtası [E] ipucu (locada, sınıf açık) — MAP_WORLD
        // bloğunun DIŞINDA (içindeyken locada hiç çizilmiyordu, Hakan raporu)
        if (gameFont != NULL && map_current() == MAP_LOCA &&
            player.storyFlags[SF_ASSASSIN] >= 13 && !locaBoardOpen)
        {
            float lbdx = LOCABOARD_X - (player.rect.x + player.rect.w / 2.0f);
            float lbdy = LOCABOARD_Y - (player.rect.y + player.rect.h / 2.0f);
            if (lbdx * lbdx + lbdy * lbdy <= LOCABOARD_RANGE * LOCABOARD_RANGE)
                draw_text(renderer, gameFont, "[E] Kontrat Tahtası",
                          (LOCABOARD_X - camera.x) * camera.zoom - 70.0f,
                          (LOCABOARD_Y - 70.0f - camera.y) * camera.zoom,
                          (SDL_Color){ 200, 160, 255, 255 });
        }

        if (map_current() == MAP_WORLD)
        {
            resource_render_all(renderer, resource_textures, &camera, gameFont,
                                (dialogNpc < 0 && nearestNpc < 0) ? nearestRes : -1);  // NPC önceliği
            // EŞİKLİ ALEV GÖVDESİ (Faz ALEV): istasyon sprite'ından ÖNCE —
            // odun yığını alevin dibini örter, alev yığının ARKASINDAN yükselir.
            // İlk sürüm alevi sprite'ın üstüne çiziyordu ve kampı tamamen
            // kapatıyordu (Hakan testi): çözüm üste almak değil DERİNLİK
            // (motor kuralının aynısı). Nefes değeri emit tarafıyla AYNI
            // fonksiyondan (fx_breath determinist — iki çağrı aynı kareyi
            // verir), yağmur bastırması da aynı formül: alev, partikül, ısı
            // ve ışık tek nefes.
            if (fxFlame > 0) {
                float fnSec = (float)SDL_GetTicks() / 1000.0f;
                float fWet = weather_is_wet()
                           ? (1.0f - 0.45f * weather_intensity()) : 1.0f;
                // YALNIZ KAMP ATEŞİ. Demirci ocağı ÇIKARILDI: onun ateşi
                // yapının içindeki küçük pencerede (furnace.png satır 23-26,
                // gömülü animasyon) — dış gövde alevi o mimariye yanlış, ve
                // arkaya çizilen alev opak gövdenin ardında hiç görünmez.
                for (int i = 0; i < STATION_COUNT; i++) {
                    if (STATIONS[i].x <= CAMPFIRE_PARKED + 1.0f) continue;  // sönük
                    if (STATIONS[i].type != STATION_CAMPFIRE) continue;
                    // ERW taş çemberli ocak (dalga-2): 79×60 × ölçek 2 →
                    // y-120..y. Alev taş çemberin İÇİNDEN, odun yatağının
                    // ortasından yükselir (taban y-52); oran dersi korunur:
                    // ocak geniş (158px), alev dar (32px).
                    float fw = 32.0f, fh = 48.0f;
                    float baseY = STATIONS[i].y - 52.0f;
                    SDL_FRect fdst = {
                        (STATIONS[i].x - fw * 0.5f - camera.x) * camera.zoom,
                        (baseY - fh - camera.y) * camera.zoom,
                        fw * camera.zoom, fh * camera.zoom,
                    };
                    if (fdst.x + fdst.w < 0 || fdst.x > (float)winW ||
                        fdst.y + fdst.h < 0 || fdst.y > (float)winH) continue;
                    float fbr = fx_breath(FIRE_HEARTH, (unsigned)i, fnSec) * fWet;
                    float fp[R2D_FX_PARAMS] = { 0 };
                    fp[0] = fnSec;             // zaman
                    fp[1] = 13.0f;             // akış Hz (brief: 12-15)
                    fp[2] = 1.6f;              // gürültü ölçek x
                    fp[3] = 1.1f;              // gürültü ölçek y
                    fp[4] = 0.30f;             // eşik CUT
                    fp[5] = fbr;               // nefes (fx_breath)
                    fp[6] = fw / ART_PX;       // sanat px sayısı w
                    fp[7] = fh / ART_PX;       // sanat px sayısı h
                    fp[8] = 1.6f;              // yatay daraltma
                    fp[9] = 1.0f;              // parlaklık
                    fp[10] = (float)i * 0.37f; // faz — her ateş farklı
                    r2d_fx_begin(renderer, fxFlame, fp, fireRampTex, NULL);
                    r2d_set_blend(renderer, R2D_BLEND_ADD);   // alev = emisyon (C.2 Katman 1)
                    r2d_draw_texture(renderer, fxNoiseTex, NULL, &fdst);
                    r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                    r2d_fx_end(renderer);
                }
            }

            station_render_all(renderer, station_textures, &camera, gameFont,
                               (dialogNpc < 0 && nearestNpc < 0 && !craftOpen) ? nearestStation : -1,
                               (craftOpen && (craftRecipe >= 0 || plusForge || enchAction > 0))
                                   ? craftStation : -1);

            // Kandil direkleri (LORE §1.1) — NPC/kaynak/istasyon önceliği
            lamp_render(renderer, &camera, gameFont,
                        (dialogNpc < 0 && nearestNpc < 0 && nearestRes < 0 &&
                         nearestStation < 0 && !craftOpen) ? nearestLamp : -1,
                        (float)SDL_GetTicks() / 1000.0f);

            // IŞIK MODU GÖSTERGESİ (dev): hangi varyantların açık olduğu
            // sürekli görünsün — yoksa hangi modda test ettiğin belirsiz kalıyor.
            if (gameFont != NULL) {
                char lmBuf[160];
                snprintf(lmBuf, sizeof(lmBuf),
                         "F1 gamma:%s  F2 dusus:%s  F3 dither:%s  F4 kademe:%s  F11 renk:%s  F12 A/B:%s  ^L sekil:%s  ^O tikama:%s",
                         (lightGamma == 0) ? "sRGB(eski)" : (lightGamma == 1) ? "LINEER" : "LIN+ACES",
                         (lightFall == 0) ? "plato+ss" : (lightFall == 1) ? "Karis" :
                         (lightFall == 2) ? "URP" : "knee",
                         (lightDither == 0) ? "IGN" : (lightDither == 1) ? "Bayer" :
                         (lightDither == 2) ? "dunya" : "yok",
                         (lightMode == 0) ? "yok" : (lightMode == 1) ? "14" : "7",
                         light_physical_color() ? "FIZIKSEL" : "elle",
                         lightSplit ? "ACIK" : "kapali",
                         (lightShape == 0) ? "tip" : (lightShape == 1) ? "basik" :
                         (lightShape == 2) ? "dikey" : "daire",
                         (lightOccl > 0.01f) ? "ACIK" : "kapali");
                SDL_Color lmC = { 190, 200, 215, 220 };
                draw_text(renderer, gameFont, lmBuf, 12.0f, (float)winH - 26.0f, lmC);
            }

            // KÖR KUYU ipucu: tanıtım sonrası — bekleyen kese / gece bırakma
            if (gameFont != NULL && dialogNpc < 0 &&
                player.storyFlags[SF_WELL] >= 1)
            {
                float wcx = 26.0f * TILE_SIZE + 32.0f;
                float wcy = 15.0f * TILE_SIZE + 48.0f;
                float pdx2 = player.rect.x + player.rect.w / 2.0f - wcx;
                float pdy2 = player.rect.y + player.rect.h / 2.0f - wcy;
                if (SDL_fabsf(pdx2) < 110.0f && SDL_fabsf(pdy2) < 110.0f)
                {
                    const char *wtxt = NULL;
                    SDL_Color wc = { 150, 150, 165, 255 };
                    if (player.wellGold > 0)
                    {
                        int wday3; float whrs3;
                        gametime_get(&wday3, &whrs3);
                        bool rdy = (wday3 > player.wellDay) ||
                                   (wday3 == player.wellDay && whrs3 >= 5.0f);
                        if (rdy)
                        {
                            wtxt = "[E] Keseni al";
                            wc = (SDL_Color){ 255, 210, 60, 255 };
                        }
                        else if (gametime_is_night())
                        {
                            // Bekleyen kese bırakmayı engellemez — üstüne eklenir
                            wtxt = (player.dirtyGold > 0)
                                 ? "[E] Kuyuya bırak (kirli kese) — sayım sürüyor"
                                 : "Sayılıyor...";
                            wc = (SDL_Color){ 200, 160, 255, 255 };
                        }
                        else
                        {
                            wtxt = "Sayılıyor...";
                            wc = (SDL_Color){ 255, 210, 60, 255 };
                        }
                    }
                    else if (gametime_is_night())
                    {
                        wtxt = (player.dirtyGold > 0)
                             ? "[E] Kuyuya bırak (kirli kese)"
                             : "[E] Kuyuya bırak";
                        if (player.dirtyGold > 0)
                            wc = (SDL_Color){ 200, 160, 255, 255 };
                    }
                    if (wtxt != NULL)
                        draw_text(renderer, gameFont, wtxt,
                                  (wcx - camera.x) * camera.zoom - 70.0f,
                                  (wcy - 110.0f - camera.y) * camera.zoom, wc);
                }
            }


            // SUİKASTÇİ P1 ipuçları: sınav kutusu (gece) + takip rapor bandı
            if (gameFont != NULL && map_current() == MAP_WORLD)
            {
                if (player.storyFlags[SF_ASSASSIN] == 2 && gametime_is_night())
                {
                    float bx2 = 10.2f * TILE_SIZE, by2 = 5.0f * TILE_SIZE;
                    float bdx2 = (player.rect.x + player.rect.w / 2.0f) - bx2;
                    float bdy2 = (player.rect.y + player.rect.h / 2.0f) - by2;
                    if (bdx2 * bdx2 + bdy2 * bdy2 < 80.0f * 80.0f)
                        draw_text(renderer, gameFont, "[E] Mühürlü Kutu'yu al",
                                  (bx2 - camera.x) * camera.zoom - 80.0f,
                                  (by2 - 60.0f - camera.y) * camera.zoom,
                                  (SDL_Color){ 200, 160, 255, 255 });
                }
                if (player.storyFlags[SF_ASSASSIN] == 4 && kemalIdx >= 0)
                {
                    float knx, kny;
                    if (npc_visible_pos(kemalIdx, &knx, &kny))
                    {
                        float kdx = (player.rect.x + player.rect.w / 2.0f) - knx;
                        float kdy = (player.rect.y + player.rect.h / 2.0f) - kny;
                        float kd = SDL_sqrtf(kdx * kdx + kdy * kdy);
                        if (kd >= 100.0f && kd <= 400.0f)
                            draw_text(renderer, gameFont, "[E] Rapor işaretle",
                                      20.0f, 160.0f,
                                      (SDL_Color){ 200, 160, 255, 255 });
                        else if (kd < 100.0f)
                            draw_text(renderer, gameFont, "Çok yakınsın!",
                                      20.0f, 160.0f,
                                      (SDL_Color){ 255, 120, 100, 255 });
                    }
                }
            }

            // Dinlenme ipucu: gece, ateş başında (E ipucunun bir üstü)
            if (gameFont != NULL && dialogNpc < 0 && nearestNpc < 0 && !craftOpen &&
                nearestStation >= 0 &&
                STATIONS[nearestStation].type == STATION_CAMPFIRE &&
                gametime_is_night())
            {
                const StationDef *rs = &STATIONS[nearestStation];
                SDL_Color rc = { 170, 220, 255, 255 };
                draw_text(renderer, gameFont, "[R] Dinlen",
                          (rs->x - camera.x) * camera.zoom - 40.0f,
                          (rs->y - rs->src.h * rs->scale - camera.y) * camera.zoom
                              - 44.0f, rc);
            }
        }

        // İlan panosu (dünya objesi): taban-orta hizalı, yaklaşınca [E] ipucu.
        // YALNIZ DÜNYADA çizilir — locada dünya-koordinatına denk gelip
        // "ikinci tahta" olarak görünüyordu (Hakan raporu)
        if (boardTex != NULL && map_current() == MAP_WORLD)
        {
            // ERW questboard (Faz EPIC-1): 59×46 kırpık — pano yatay tabela
            const float bScale = 2.0f, bw = 59.0f * bScale, bh = 46.0f * bScale;
            SDL_FRect bsrc = { 0, 0, 59, 46 };
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

        // Yuvalar (E1): gölcük + mezar zemin görselleri (yalnız dünya)
        if (nestsTex != NULL && map_current() == MAP_WORLD)
            for (int ni = 0; ni < NEST_SITE_COUNT; ni++)
            {
                const NestSiteDef *ns = &NEST_SITES[ni];
                const NestState *nst = ecology_state(ni);
                if (!nst->colonized) continue;
                if (ns->kind == NEST_POOL)
                {
                    if (nst->destroyed) continue;   // kurumuş gölcük görünmez
                    SDL_FRect nsrc = { 144, 0, 48, 48 };
                    SDL_FRect ndst = world_to_screen(ns->x - 36.0f, ns->y - 30.0f,
                                                     72.0f, 72.0f, &camera);
                    if (ns->family == FAM_SLIME_B)
                        r2d_texture_set_color_mod(nestsTex, 110, 160, 255);
                    else
                        r2d_texture_set_color_mod(nestsTex, 130, 235, 110);
                    r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                    r2d_draw_texture(renderer, nestsTex, &nsrc, &ndst);
                    r2d_texture_set_color_mod(nestsTex, 255, 255, 255);
                }
                else if (ns->kind == NEST_GRAVE)
                {
                    // Lanetli (hücre 1) → mühürlü (hücre 2, kalıcı görünür)
                    SDL_FRect nsrc = { nst->destroyed ? 96.0f : 48.0f, 0, 48, 48 };
                    SDL_FRect ndst = world_to_screen(ns->x - 36.0f, ns->y - 58.0f,
                                                     72.0f, 72.0f, &camera);
                    r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                    r2d_draw_texture(renderer, nestsTex, &nsrc, &ndst);
                }
            }

        // K9 kandili (D1a): geçit kapısında — duruma göre görünüm
        if (lanternTex != NULL && map_current() == MAP_WORLD)
        {
            Uint8 k9v = player.storyFlags[SF_K9_KANDIL];
            float ksc = (k9v == 1) ? 3.4f : 2.2f;   // tapınak ateşi daha büyük
            SDL_FRect ksrc = { 0, 0, 16, 16 };
            SDL_FRect kdst = world_to_screen(KANDIL_X - 8.0f * ksc,
                                             KANDIL_Y - 14.0f * ksc,
                                             16.0f * ksc, 16.0f * ksc, &camera);
            if (k9v == 2)       r2d_texture_set_color_mod(lanternTex, 95, 95, 105);
            else if (k9v == 1)  r2d_texture_set_color_mod(lanternTex, 160, 210, 255);
            else                r2d_texture_set_color_mod(lanternTex, 255, 255, 255);
            r2d_set_blend(renderer, R2D_BLEND_ALPHA);
            r2d_draw_texture(renderer, lanternTex, &ksrc, &kdst);
            r2d_texture_set_color_mod(lanternTex, 255, 255, 255);
        }

        // K2 sofrası (D1b): Nazife bilene kadar her gece kurulur
        if (sofraTex != NULL && map_current() == MAP_WORLD &&
            gametime_is_night() &&
            player.storyFlags[SF_K2_NAZIFE] != 1 &&
            player.storyFlags[SF_K2_NAZIFE] != 2)
        {
            SDL_FRect ssrc = { 0, 0, 40, 44 };
            SDL_FRect sdst = world_to_screen(SOFRA_X - 30.0f, SOFRA_Y - 40.0f,
                                             60.0f, 66.0f, &camera);
            r2d_set_blend(renderer, R2D_BLEND_ALPHA);
            r2d_draw_texture(renderer, sofraTex, &ssrc, &sdst);
        }

        // Toprak Dikenleri: zemin patlaması (moblardan/oyuncudan ÖNCE — yerden fışkırır)
        // PROSEDÜREL MODDA ÇİZİLMEZ (bkz. skillfx SPIKES tarifi)
        if (spikesTex != NULL && !skillfx_procedural())
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

        // Büyü dalgası 2: 4×4 grid 128px sheet'ler (satır-major kare seçimi)
        if (clawTex != NULL)
            for (int q = 0; q < MAX_CLAW_FX; q++)
            {
                if (!clawFx[q].active) continue;
                int fr = (int)(clawFx[q].t / 0.045f);
                if (fr > 15) fr = 15;
                SDL_FRect csrc = { (float)((fr % 4) * 128), (float)((fr / 4) * 128),
                                   128, 128 };
                SDL_FRect cdst = world_to_screen(clawFx[q].x - 64.0f,
                                                 clawFx[q].y - 96.0f,
                                                 128.0f, 128.0f, &camera);
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                r2d_draw_texture(renderer, clawTex, &csrc, &cdst);
            }
        if (icetacleTex != NULL)
            for (int q = 0; q < MAX_ICETACLE_FX; q++)
            {
                if (!icetacleFx[q].active) continue;
                int fr = (int)(icetacleFx[q].t / 0.06f);
                if (fr > 15) fr = 15;
                SDL_FRect csrc = { (float)((fr % 4) * 128), (float)((fr / 4) * 128),
                                   128, 128 };
                SDL_FRect cdst = world_to_screen(icetacleFx[q].x - 64.0f,
                                                 icetacleFx[q].y - 96.0f,
                                                 128.0f, 128.0f, &camera);
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                r2d_draw_texture(renderer, icetacleTex, &csrc, &cdst);
            }
        for (int q = 0; q < MAX_SNAKEBITE_FX; q++)
        {
            if (!snakeFx[q].active) continue;
            R2DTexture *stx = snakebiteTex[snakeFx[q].dirRow];
            if (stx == NULL) continue;
            int fr = (int)(snakeFx[q].t / 0.05f);
            if (fr > 15) fr = 15;
            SDL_FRect csrc = { (float)((fr % 4) * 128), (float)((fr / 4) * 128),
                               128, 128 };
            SDL_FRect cdst = world_to_screen(snakeFx[q].x - 64.0f,
                                             snakeFx[q].y - 96.0f,
                                             128.0f, 128.0f, &camera);
            r2d_set_blend(renderer, R2D_BLEND_ALPHA);
            if (snakeFx[q].flip)
                r2d_draw_texture_flip(renderer, stx, &csrc, &cdst, true);
            else
                r2d_draw_texture(renderer, stx, &csrc, &cdst);
        }
        for (int q = 0; q < MAX_FIRESURGES; q++)
        {
            const FireSurge *fl = &firesurges[q];
            if (!fl->active) continue;
            // Prosedürel modda sprite ÇİZİLMEZ. Ders (Hakan bildirdi): A/B
            // anahtarı yalnız SPAWN tarafını keserse çizim tarafı sprite'ı
            // basmaya devam eder ve "hala asset var" görünür.
            R2DTexture *ftx = firesurgeTex[fl->facing];
            if (ftx == NULL || skillfx_procedural()) continue;
            int fr = (int)(fl->t / (FIRESURGE_LIFE / 16.0f));
            if (fr > 15) fr = 15;
            SDL_FRect csrc = { (float)((fr % 4) * 128), (float)((fr / 4) * 128),
                               128, 128 };
            SDL_FRect cdst = world_to_screen(fl->x - 70.0f, fl->y - 100.0f,
                                             140.0f, 140.0f, &camera);
            r2d_set_blend(renderer, R2D_BLEND_ALPHA);
            r2d_draw_texture(renderer, ftx, &csrc, &cdst);
        }
        if (darkburstTex != NULL)
            for (int q = 0; q < MAX_DARKBURST_FX; q++)
            {
                if (!darkburstFx[q].active) continue;
                int fr = (int)(darkburstFx[q].t / 0.05f);
                if (fr > 15) fr = 15;
                SDL_FRect csrc = { (float)(fr * 48), 0, 48, 64 };
                SDL_FRect cdst = world_to_screen(darkburstFx[q].x - 60.0f,
                                                 darkburstFx[q].y - 110.0f,
                                                 120.0f, 160.0f, &camera);
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                r2d_draw_texture(renderer, darkburstTex, &csrc, &cdst);
            }
        if (tornadoTex != NULL)
            for (int q = 0; q < MAX_TORNADOES; q++)
            {
                const Tornado *tn = &tornadoes[q];
                if (!tn->active) continue;
                // Zaman çizgisi: kalkış 0-3 (ilk 0.3sn) → gövde döngüsü 4-11 →
                // son 0.4 sn dağılma 12-15
                int fr;
                if (tn->t < 0.3f)
                    fr = (int)(tn->t / 0.075f);
                else if (tn->t > tn->life - 0.4f)
                    fr = 12 + (int)((tn->t - (tn->life - 0.4f)) / 0.1f);
                else
                    fr = 4 + (int)(tn->t / 0.07f) % 8;
                if (fr > 15) fr = 15;
                SDL_FRect csrc = { (float)((fr % 4) * 128), (float)((fr / 4) * 128),
                                   128, 128 };
                SDL_FRect cdst = world_to_screen(tn->x - 55.0f, tn->y - 88.0f,
                                                 110.0f, 110.0f, &camera);
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                r2d_draw_texture(renderer, tornadoTex, &csrc, &cdst);
            }

        PlayerLayer built[24];
        int builtCount = build_player_layers(layers, LAYER_COUNT, &equipment,
                                             &itemTex, built, 24);

        // CAST HALKASI (VFX cilası): büyü kanalizasyonu boyunca ayak altında
        // dönen büyü çemberi (81 karelik sheet, castTimer ile döngü)
        if (castSpell >= 0 && castCircleTex != NULL && gameState == GAME_PLAYING)
        {
            int ccFr = (int)(castTimer * 45.0f) % 81;
            SDL_FRect ccSrc = { (float)(ccFr % 9) * 100.0f,
                                (float)(ccFr / 9) * 100.0f, 100.0f, 100.0f };
            float ccW = 90.0f, ccH = 90.0f;
            SDL_FRect ccDst = {
                (player.rect.x + player.rect.w / 2.0f - ccW / 2.0f - camera.x)
                    * camera.zoom,
                (player.rect.y + player.rect.h - ccH / 2.0f - 8.0f - camera.y)
                    * camera.zoom,
                ccW * camera.zoom, ccH * camera.zoom,
            };
            r2d_draw_texture(renderer, castCircleTex, &ccSrc, &ccDst);
        }
        // ── EFEKTİN ARKA YARISI (oyuncudan ÖNCE). Alan büyülerinde oyuncu
        // tamamen kayboluyordu çünkü tüm partiküller varlıklardan SONRA tek blok
        // halinde çiziliyordu. Çözüm "oyuncuyu üste al" DEĞİL — o zaman efektin
        // üstüne yapıştırılmış çıkartma gibi durur (Hakan bunu bilerek dışladı:
        // "direk skill efektlerinin üstüne çizim gibi değil, etkisinde gözükse").
        // 2.5B derinlik: ayak hizasından YUKARIDAKİ efekt oyuncunun ARKASINA,
        // aşağıdaki ÖNÜNE. Oyuncu efektin İÇİNDE duruyor — uzak taraf arkasında,
        // yakın taraf bacaklarının önünde. Işık pass'i de onu efektin rengiyle
        // boyadığı için "etkisinde" okuması tamamlanıyor.
        float fxSplitY = player.rect.y + player.rect.h;
        // SIRA: zemin kırıntısı GÖVDELERDEN ÖNCE. Kırıntı zeminin aynı
        // dokusundan geldiği için gövdenin üstüne binince onu yiyor (Hakan:
        // "dikenler gözükmüyor"). Diken ve buz kümesi KAHRAMAN nesne, kırıntı
        // onların artığı — sıralama bunu yansıtmak zorunda.
        groundfx_render_band(renderer, &camera, -1.0e9f, fxSplitY);
        skillfx_render_band(renderer, &camera, -1.0e9f, fxSplitY);
        fx_render_band(&weaponFx, renderer, glowTex, &camera, -1.0e9f, fxSplitY);
        player_render(&player, renderer, built, builtCount, &camera); // oyuncuyu
        
        for (int i = 0; i < mobCount; i++)
            mob_render(&mobs[i], renderer, mob_textures, &camera, player.targetMobId);

        ally_render(allies, renderer, snakeTex, turtleTex, &camera);   // M4 dostlar

        npc_render_infirmary(renderer, npc_textures, &camera, gameFont,
                             21.0f * TILE_SIZE, 10.5f * TILE_SIZE);
        npc_render_all(renderer, npc_textures, &camera, gameFont, nearestNpc,
                       player.sneaking);

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

        // Ateş Topu mermileri: piksel küre sprite'ı (Sog paketi).
        // PROSEDÜREL MODDA ÇİZİLMEZ: alev kuyruğu + kor + ısı bükülmesi
        // mermiyi zaten anlatıyor; sprite üstüne binince iki dil karışıyor.
        if (orbFireTex != NULL && !skillfx_procedural())
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
        // KUTSAL ALAN çemberi: altın halka (undead'e karşı güvenli bölge)
        if (holyZone.on && castCircleTex != NULL)
        {
            int zfr = (int)((6.0f - holyZone.t) * 45.0f) % 81;
            SDL_FRect zsrc = { (float)(zfr % 9) * 100.0f,
                               (float)(zfr / 9) * 100.0f, 100.0f, 100.0f };
            float zw = holyZone.r * 2.0f;
            SDL_FRect zdst = {
                (holyZone.x - zw / 2.0f - camera.x) * camera.zoom,
                (holyZone.y - zw / 2.0f - camera.y) * camera.zoom,
                zw * camera.zoom, zw * camera.zoom,
            };
            Uint8 za = (holyZone.t < 1.0f) ? (Uint8)(200.0f * holyZone.t) : 200;
            r2d_texture_set_color_mod(castCircleTex, 255, 220, 130);
            r2d_texture_set_alpha_mod(castCircleTex, za);
            r2d_draw_texture(renderer, castCircleTex, &zsrc, &zdst);
            r2d_texture_set_color_mod(castCircleTex, 255, 255, 255);
            r2d_texture_set_alpha_mod(castCircleTex, 255);
        }
        // Kara Patlama telegraph'ları: büyüyen karanlık halka (kaç!)
        if (map_current() == MAP_MEZARLIK && castCircleTex != NULL)
        {
            for (int q = 0; q < 4; q++)
            {
                if (!deathBlast[q].on) continue;
                float bt = deathBlast[q].t;
                int bfr = (int)(bt * 45.0f) % 81;
                SDL_FRect bsrc = { (float)(bfr % 9) * 100.0f,
                                   (float)(bfr / 9) * 100.0f, 100.0f, 100.0f };
                float bw = (40.0f + bt * 110.0f);   // 1 sn'de 40→150px çap
                SDL_FRect bdst = {
                    (deathBlast[q].x - bw / 2.0f - camera.x) * camera.zoom,
                    (deathBlast[q].y - bw / 2.0f - camera.y) * camera.zoom,
                    bw * camera.zoom, bw * camera.zoom,
                };
                r2d_texture_set_color_mod(castCircleTex, 120, 40, 160);
                r2d_texture_set_alpha_mod(castCircleTex, (Uint8)(120 + bt * 120.0f));
                r2d_draw_texture(renderer, castCircleTex, &bsrc, &bdst);
            }
            r2d_texture_set_color_mod(castCircleTex, 255, 255, 255);
            r2d_texture_set_alpha_mod(castCircleTex, 255);
        }
        // Yankı kopyası: mor tayf (bob salınımı + yön flip'i)
        if (echoShadow.on && echoWispTex != NULL)
        {
            int wfr = (int)(echoShadow.t * 12.0f) % 20;
            SDL_FRect wsrc = { (float)(wfr % 10) * 40.0f,
                               (float)(wfr / 10) * 32.0f, 40.0f, 32.0f };
            float bob = SDL_sinf(echoShadow.t * 3.0f) * 4.0f;
            SDL_FRect wdst = {
                (echoShadow.x - 40.0f - camera.x) * camera.zoom,
                (echoShadow.y - 32.0f + bob - camera.y) * camera.zoom,
                80.0f * camera.zoom, 64.0f * camera.zoom,
            };
            r2d_texture_set_alpha_mod(echoWispTex, 215);
            r2d_draw_texture_flip(renderer, echoWispTex, &wsrc, &wdst,
                                  echoShadow.facingLeft);
            r2d_texture_set_alpha_mod(echoWispTex, 255);
        }
        // ── EFEKTİN ÖN YARISI (oyuncudan SONRA) + yere ait olmayan geometri.
        // Yıldırım kolu ve isabet yayı havada duran ANLIK geometri: bir derinliğe
        // ait olmadıkları için yalnız bu tam-aralık çağrısında çiziliyorlar
        // (bant çağrılarında atlanıyorlar, yoksa iki kez çizilip toplamalı
        // katmanda iki kat parlıyorlar).
        groundfx_render_band(renderer, &camera, fxSplitY, 1.0e9f);
        skillfx_render_band(renderer, &camera, fxSplitY, 1.0e9f);
        skillfx_render_air(renderer, &camera);   // yıldırım kolu / isabet yayı
        fx_render_band(&weaponFx, renderer, glowTex, &camera, fxSplitY, 1.0e9f);
        animfx_render(animFxPool, renderer, &camera);

        if (openLootIndex >= 0)
            loot_popup_render(renderer, &loots[openLootIndex], itemIconTex, &itemTex, gameFont, &camera);

        loot_render_all(loots, MAX_LOOT, renderer, crateTex, chestTex, &camera); // ← kasa + sandıklar

        // Menzildeki sandığın baş üstü ipucu (kilitliyse kademesiyle)
        if (nearestChest >= 0 && gameFont != NULL && !lockpick_active())
        {
            const Loot *nc = &loots[nearestChest];
            const char *lbl = (nc->lockTier < 0)  ? "[E] Ac"
                            : (nc->lockTier == 0) ? "[E] Kilit ac (Basit)"
                            : (nc->lockTier == 1) ? "[E] Kilit ac (Saglam)"
                                                  : "[E] Kilit ac (Usta)";
            SDL_Color hint = { 255, 230, 80, 255 };
            int tw = 0, th = 0;
            TTF_GetStringSize(gameFont, lbl, 0, &tw, &th);
            draw_text(renderer, gameFont, lbl,
                      (nc->rect.x + nc->rect.w / 2.0f - camera.x) * camera.zoom - tw / 2.0f,
                      (nc->rect.y - 34.0f - camera.y) * camera.zoom, hint);
        }

        // Ritüel yuvası ipucu (E1): gölcük kurut / mezar mühürle
        if (nearestRitual >= 0 && gameFont != NULL && !scene_active())
        {
            const NestSiteDef *rs = ecology_site(nearestRitual);
            const char *rlbl = (rs->kind == NEST_POOL) ? "[E] Gölcüğü kurut"
                                                       : "[E] Mezarı mühürle";
            SDL_Color rhint = { 150, 220, 255, 255 };
            int tw = 0, th = 0;
            TTF_GetStringSize(gameFont, rlbl, 0, &tw, &th);
            draw_text(renderer, gameFont, rlbl,
                      (rs->x - camera.x) * camera.zoom - tw / 2.0f,
                      (rs->y - 70.0f - camera.y) * camera.zoom, rhint);
        }

        // Anlatı noktaları: yazıt taşları (decor) + yankıların mor soluğu
        map_echo_decor_render(renderer, &camera);
        for (int i = 0; i < ECHO_COUNT; i++)
        {
            if (ECHOES[i].map != map_current() || echo_done(i)) continue;

            if (ECHOES[i].kind == ECHO_ANI && glowTex != NULL)
            {
                // Mor soluk: nabız gibi titrer — ölünün son anı burada asılı.
                // Mühür sancısı gecesi (WE_SEAL) soluk gözle görülür BÜYÜR.
                float tsec = (float)((double)SDL_GetTicks() / 1000.0);
                float pulse = 0.62f + 0.38f * SDL_sinf(tsec * 2.4f);
                float sanci = (worldevent_current() == WE_SEAL &&
                               gametime_is_night()) ? 1.8f : 1.0f;
                float size = (34.0f + 10.0f * pulse) * sanci * camera.zoom;
                SDL_FRect gdst = { (ECHOES[i].x - camera.x) * camera.zoom - size / 2.0f,
                                   (ECHOES[i].y - camera.y) * camera.zoom - size / 2.0f,
                                   size, size };
                r2d_texture_set_color_mod(glowTex, 170, 110, 255);
                r2d_texture_set_alpha_mod(glowTex, (Uint8)(120.0f + 100.0f * pulse));
                r2d_draw_texture(renderer, glowTex, NULL, &gdst);
                r2d_texture_set_color_mod(glowTex, 255, 255, 255);
                r2d_texture_set_alpha_mod(glowTex, 255);
            }

            if (i == nearestEcho && gameFont != NULL && !scene_active())
            {
                bool ani = ECHOES[i].kind == ECHO_ANI;
                const char *lbl = ani ? "[E] Dokun" : "[E] Oku";
                SDL_Color hint = ani ? (SDL_Color){ 200, 160, 255, 255 }
                                     : (SDL_Color){ 255, 230, 80, 255 };
                float lift = ani ? 44.0f : 150.0f;   // yazıt taşı uzun — etiket tepede
                int tw = 0, th = 0;
                TTF_GetStringSize(gameFont, lbl, 0, &tw, &th);
                draw_text(renderer, gameFont, lbl,
                          (ECHOES[i].x - camera.x) * camera.zoom - tw / 2.0f,
                          (ECHOES[i].y - lift - camera.y) * camera.zoom, hint);
            }
        }

        // KAR ÖRTÜSÜ: dünya beyaza bürünür (ışık pass'inden önce — gece
        // karartması karın üstüne doğal biner; yağış durunca yavaş erir)
        if (map_current() == MAP_WORLD && weather_snow_cover() > 0.0f)
        {
            r2d_set_blend(renderer, R2D_BLEND_ALPHA);
            r2d_set_color(renderer, 235, 240, 248,
                          (Uint8)(60.0f * weather_snow_cover()));
            SDL_FRect snowFull = { 0, 0, (float)winW, (float)winH };
            r2d_fill_rect(renderer, &snowFull);
        }

        // --- HAVA GÖRSELLERİ: yağmur/kar/sis (IŞIK PASS'İNDEN ÖNCE) ---
        // Eskiden ışıktan SONRA çiziliyordu ve gece hiç kararmıyordu: zifiri
        // gecede sis parlak gri, kar bembeyaz kalıyordu (brief D.4 borcu).
        // Kar örtüsüyle aynı tarafa alındı — gece karartması yağışın üstüne
        // doğal biner, kandil yakınındaki damla ışık halkasında parlar.
        // Şimşek flaşı BİLEREK ışıktan sonra kaldı: o bir örtü değil IŞIK.
        if (map_current() == MAP_WORLD && weather_intensity() > 0.0f)
        {
            float wi = weather_intensity();
            Weather wn = weather_current();
            if (wn == WEATHER_RAIN || wn == WEATHER_STORM)
            {
                // Damlalar: hafif eğik ince çizgiler — ekran uzayında düşer
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                int drops = (int)(RAIN_DROPS * wi);
                for (int i = 0; i < drops; i++)
                {
                    rainPY[i] += rainPS[i] * dt;
                    rainPX[i] -= rainPS[i] * 0.22f * dt;   // rüzgar eğimi
                    if (rainPY[i] > 1.0f)
                    {
                        rainPY[i] -= 1.0f;
                        rainPX[i] = randf();
                    }
                    if (rainPX[i] < 0.0f) rainPX[i] += 1.0f;
                    float dx = rainPX[i] * (float)winW;
                    float dy = rainPY[i] * (float)winH;
                    Uint8 a = (Uint8)((90.0f + 60.0f * (rainPS[i] - 1.6f)) * wi);
                    r2d_set_color(renderer, 180, 200, 230, a);
                    SDL_FRect d1 = { dx, dy, 2.0f, 9.0f };
                    SDL_FRect d2 = { dx + 2.0f, dy - 7.0f, 2.0f, 8.0f };
                    r2d_fill_rect(renderer, &d1);
                    r2d_fill_rect(renderer, &d2);
                }
            }
            if (wn == WEATHER_SNOW)
            {
                // Kar taneleri: ağır süzülür, rüzgarla salınır (damla dizileri
                // yeniden kullanılır — yağmurla aynı anda asla aktif değil)
                float tsec2 = (float)((double)SDL_GetTicks() / 1000.0);
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                int flakes = (int)(RAIN_DROPS * 0.7f * wi);
                for (int i = 0; i < flakes; i++)
                {
                    rainPY[i] += rainPS[i] * 0.09f * dt;   // ağır düşüş
                    rainPX[i] += SDL_sinf(tsec2 * 1.3f + (float)i) * 0.02f * dt;
                    if (rainPY[i] > 1.0f)
                    {
                        rainPY[i] -= 1.0f;
                        rainPX[i] = randf();
                    }
                    if (rainPX[i] < 0.0f) rainPX[i] += 1.0f;
                    if (rainPX[i] > 1.0f) rainPX[i] -= 1.0f;
                    float dx = rainPX[i] * (float)winW;
                    float dy = rainPY[i] * (float)winH;
                    float fs = 2.0f + (rainPS[i] - 1.6f) * 1.5f;   // öndeki iri
                    r2d_set_color(renderer, 245, 248, 255,
                                  (Uint8)(170.0f * wi));
                    SDL_FRect fl = { dx, dy, fs, fs };
                    r2d_fill_rect(renderer, &fl);
                }
            }
            if (wn == WEATHER_FOG)
            {
                // Sis: iki katman gri örtü — yavaş nefes alır (tek renkte boğmaz)
                float tsec = (float)((double)SDL_GetTicks() / 1000.0);
                float breathe = 0.9f + 0.1f * SDL_sinf(tsec * 0.35f);
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                r2d_set_color(renderer, 190, 195, 205,
                              (Uint8)(95.0f * wi * breathe));
                SDL_FRect fogFull = { 0, 0, (float)winW, (float)winH };
                r2d_fill_rect(renderer, &fogFull);
                r2d_set_color(renderer, 210, 214, 222, (Uint8)(45.0f * wi));
                SDL_FRect fogBand = { 0, (float)winH * 0.25f,
                                      (float)winW, (float)winH * 0.5f };
                r2d_fill_rect(renderer, &fogBand);
            }
        }

        // Hasar yazıları (mob'ların üstünde, dünya uzayında; sisin ÜSTÜNDE
        // kalsın diye hava görsellerinden sonra)
        floattext_render_all(floatTexts, MAX_FLOAT_TEXTS, renderer, gameFont, &camera); // ← BURAYA (present'tan önce)

        // GÜN/GECE aydınlatması: shader ışık pass'i — dünyanın üstüne, HUD'un
        // altına (HUD gece de okunur). Ambient ay ışığı mavisine iner; ışık
        // kaynakları (pencere/direk/meşale) piksel başına sıcak renk katar.
        float dark01 = gametime_darkness01();
        if (map_current() == MAP_WORLD)
        {
            // Hava karartması: fırtına 0.45, yağmur 0.20 — geceyle yarışmaz (max)
            float wd = weather_darkness01();
            if (wd > dark01) dark01 = wd;
        }
        bool inDungeon = map_is_dungeon(map_current());
        if (dark01 > 0.0f || inDungeon)
        {
            float ambR, ambG, ambB;
            if (inDungeon)
            {
                // Zindan: saatten bağımsız sabit loşluk (soğuk taş tonu)
                float a = MAPS[map_current()].ambient;
                ambR = a;
                ambG = a * 1.02f;
                ambB = a * 1.35f;
            }
            else
            {
                // Zifiri gece çarpanı — KÜL GRİSİ, mavi değil.
                // Eskiden (0.30, 0.33, 0.55) doygun maviydi ve turuncu kandille
                // çarpıştığı için halkanın orta kuşağı çamura düşüyordu: iki
                // doygun tümleyen renk karıştığı yerde gri üretir. Nötr bir
                // zeminde ise kor rengi ÇOK DAHA doygun okunuyor (Albers kuralı).
                // Genel seviye de düştü: kandilin kontrastı arttı.
                ambR = 1.0f - dark01 * (1.0f - 0.24f);
                ambG = 1.0f - dark01 * (1.0f - 0.25f);
                ambB = 1.0f - dark01 * (1.0f - 0.30f);

                // Altın saat: alacakaranlık/şafak ortasında sahne turuncuya ısınır
                // (sin eğrisi: geçişin ortasında tepe, gündüz ve zifiri gecede sıfır)
                float golden = SDL_sinf(dark01 * SDL_PI_F) * 0.18f;
                ambR += golden;
                ambG += golden * 0.45f;

                // MEVSİM TONU: sonbahar sıcak, kış soluk, yaz doygun.
                // ESKİDEN gündüz ağırlığıyla (1-dark01) çarpılıyordu — yani
                // zifiri gecede etkisi MATEMATİKSEL OLARAK SIFIRDI. Oysa gece
                // ambient görüntünün tamamı, mevsimin okunabileceği tek yer
                // orası. Şimdi gece ağırlıklı: gündüz 0.4, gece 1.0.
                float dayW = 0.4f + 0.6f * dark01;
                switch (gametime_season()) {
                    case SEASON_AUTUMN:
                        ambR += 0.05f * dayW; ambB -= 0.07f * dayW; break;
                    case SEASON_WINTER:
                        ambR -= 0.07f * dayW; ambG -= 0.03f * dayW;
                        ambB += 0.05f * dayW; break;
                    case SEASON_SUMMER:
                        ambR += 0.04f * dayW; ambG += 0.02f * dayW; break;
                    default: break;   // ilkbahar: nötr taze
                }
            }

            // Meşale: envanterde varsa oyuncu ışık taşır
            bool hasTorch = false;
            for (int i = 0; i < INVENTORY_SIZE; i++)
                if (inventory.slots[i].type == &ITEM_TYPES[ITEM_TORCH]) { hasTorch = true; break; }

            R2DLight lights[R2D_MAX_LIGHTS];
            g_fxBridge = player.weaponFx;   // yer izi tipi için köprü senkronu
            int lightN = light_collect(lights, R2D_MAX_LIGHTS, &camera, winW, winH,
                                       !inDungeon,   // köy ışıkları yalnız dünyada
                                       player.rect.x + player.rect.w / 2.0f,
                                       player.rect.y + player.rect.h / 2.0f,
                                       hasTorch, player.weaponFx);
            if (!inDungeon)
            {
                // KANDİL DİREKLERİ (LORE §1.1): yanan direkler ışık verir,
                // sönükler vermez. Aynı yarıçap ölüyü de bastırır (§1.4) —
                // oyuncu gördüğü halka ile korunduğu halkayı ayırt etmesin.
                lightN = lamp_append_lights(lights, lightN, R2D_MAX_LIGHTS,
                                            &camera, winW, winH);
                lightN = skillfx_append_lights(lights, lightN, R2D_MAX_LIGHTS,
                                               &camera, winW, winH);

                // ── İSTASYON ATEŞLERİNİN IŞIĞI, PARÇACIKLARLA AYNI NEFESTEN.
                // Işık titremesi eskiden light.c'nin kendi gürültüsünden
                // geliyordu ve parçacıklardan bağımsızdı — alev kabarırken
                // ışık kabarmıyordu, ateş "iki ayrı şey" gibi okunuyordu.
                // fx_breath tek kaynak: parçacık yoğunluğu, ısı bükülmesi
                // genliği ve ışık şiddeti üçü birden ondan türüyor.
                // Bu bağ genel: skill/boss efektleri de aynı fonksiyonu
                // çağırıp kendi ışığını senkron sürebilir.
                {
                    float nSec = (float)SDL_GetTicks() / 1000.0f;
                    for (int i = 0; i < STATION_COUNT && lightN < R2D_MAX_LIGHTS; i++) {
                        float sxw = STATIONS[i].x, syw = STATIONS[i].y;
                        if (sxw <= CAMPFIRE_PARKED + 1.0f) continue;
                        FireKind fk;
                        float rad, ir, ig, ib, base;
                        switch (STATIONS[i].type) {
                            case STATION_CAMPFIRE:
                                fk = FIRE_HEARTH; rad = 190.0f; base = 0.62f;
                                ir = 1.0f; ig = 0.60f; ib = 0.26f; break;
                            case STATION_SMITH:
                                fk = FIRE_FORGE;  rad = 150.0f; base = 0.58f;
                                ir = 1.0f; ig = 0.72f; ib = 0.40f; break;
                            default: continue;   // kazan ışık vermez (buhar)
                        }
                        float br = fx_breath(fk, (unsigned)i, nSec);
                        lightN = light_append_point(lights, lightN, R2D_MAX_LIGHTS,
                                    &camera, winW, winH, sxw, syw - 18.0f,
                                    rad * (0.92f + 0.08f * br), ir, ig, ib,
                                    base * br,
                                    (fk == FIRE_HEARTH) ? LK_HEARTH : LK_STEADY);
                    }
                }

                // K9 kandili (D1a): duruma göre ışık — yalan(2) = sönmüş
                Uint8 k9L = player.storyFlags[SF_K9_KANDIL];
                if (k9L == 1)      // tapınak ateşi: büyük, mavi-beyaz
                    lightN = light_append_point(lights, lightN, R2D_MAX_LIGHTS,
                                &camera, winW, winH, KANDIL_X, KANDIL_Y,
                                300.0f, 0.55f, 0.75f, 1.0f, 0.75f, LK_MAGIC);
                else if (k9L != 2) // normal kandil: küçük, sıcak
                    lightN = light_append_point(lights, lightN, R2D_MAX_LIGHTS,
                                &camera, winW, winH, KANDIL_X, KANDIL_Y,
                                140.0f, 1.0f, 0.62f, 0.26f, 0.55f, LK_CANDLE);

                // K2 sofra mumu (D1b): sofra kuruluysa küçük titrek ışık
                if (player.storyFlags[SF_K2_NAZIFE] != 1 &&
                    player.storyFlags[SF_K2_NAZIFE] != 2)
                    lightN = light_append_point(lights, lightN, R2D_MAX_LIGHTS,
                                &camera, winW, winH, SOFRA_X, SOFRA_Y - 30.0f,
                                90.0f, 1.0f, 0.65f, 0.30f, 0.45f, LK_CANDLE);
            }
            if (inDungeon)
            {
                // Zindan duvar meşaleleri
                lightN = map_append_lights(map_current(), lights, lightN, R2D_MAX_LIGHTS,
                                           &camera, winW, winH);
            }
            else
            {
                // Kamp ateşleri: yanan her ateş (köy sabiti + oyuncununki) ışık verir
                for (int i = 0; i < STATION_COUNT; i++)
                    if (STATIONS[i].type == STATION_CAMPFIRE &&
                        STATIONS[i].x > CAMPFIRE_PARKED + 1.0f)
                        lightN = light_append_campfire(lights, lightN, R2D_MAX_LIGHTS,
                                                       &camera, winW, winH,
                                                       STATIONS[i].x, STATIONS[i].y - 24.0f);
            }
            r2d_light_set_camera(camera.x, camera.y);   // dünya-çivili dither
            r2d_light_set_split(lightSplit ? (float)winW * 0.5f : 0.0f);

            // IŞIK TIKAMA: nesnelerin gövdeleri ışığı kessin. Bu olmadan her
            // ışık duvarların içinden geçip kusursuz bir oval boyuyordu.
            // Dünyada dekor gövdeleri, zindanda kapalı (duvarlar tilemap'te —
            // ayrı iş). lightOccl = 0 ile kapatılabilir (dev tuşu ']').
            if (!inDungeon && lightOccl > 0.01f) {
                SDL_FRect occ[R2D_MAX_OCCLUDERS];
                int oc = decor_light_occluders(occ, R2D_MAX_OCCLUDERS, &camera,
                                               player.rect.x, player.rect.y);
                r2d_light_set_occluders(occ, oc, lightOccl);
            } else {
                r2d_light_set_occluders(NULL, 0, 0.0f);
            }
            // EFEKT IŞIKLARI: aktif AnimFx'ler sönen nokta ışık yayar.
            // En sonda ekleniyor — statik/oyuncu ışıkları tavanı doldurursa
            // kaybedilecek ilk şey geçici bir patlama parıltısı olsun.
            lightN = animfx_append_lights(g_animFx, lights, lightN,
                                          R2D_MAX_LIGHTS, &camera, winW, winH);
            r2d_light_pass(renderer, ambR, ambG, ambB, lights, lightN);
        }

        if (lightningFlash > 0.0f)
        {
            // Şimşek: tüm ekran beyaz parlar, 0.15 sn'de söner
            r2d_set_blend(renderer, R2D_BLEND_ALPHA);
            r2d_set_color(renderer, 235, 240, 255,
                          (Uint8)(180.0f * (lightningFlash / 0.15f)));
            SDL_FRect lfull = { 0, 0, (float)winW, (float)winH };
            r2d_fill_rect(renderer, &lfull);
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

        // ═══ SAHNE / HUD SINIRI ═══
        // Buraya kadar olan her şey DÜNYA: tilemap, varlıklar, ışık pass'i, hava,
        // hasar vinyeti. Post-fx filtresi yalnız onlara uygulanır ve burada
        // ekrana basılır. Bundan sonrası HUD — filtresiz, bükülmesiz, net.
        // Sebep: ısı bükülmesi UV kaydırıyordu ve hotbar/çubuklar dalgalanıyordu
        // (ekranın altı bükülmenin en güçlü olduğu yer). Shader HUD'u ayırt
        // edemez, o yüzden ayrım çizim sırasında yapılıyor.
        // Filtre kapalıysa bu çağrı hiçbir şey yapmaz.
        r2d_postfx_flush(renderer);

        // Tok buffu göstergesi (M2): sol üst barların altında
        if (player.foodBuffTimer > 0.0f && player.foodBuffName != NULL &&
            gameFont != NULL && gameState == GAME_PLAYING)
        {
            char tokBuf[64];
            snprintf(tokBuf, sizeof(tokBuf), "Tok: %s (%ds)",
                     player.foodBuffName, (int)player.foodBuffTimer + 1);
            SDL_Color warm = { 255, 205, 130, 230 };
            float tx, ty;
            uilayout_get("tok_buff", 16.0f, 96.0f, &tx, &ty);
            draw_text(renderer, gameFont, tokBuf, tx, ty, warm);
        }
        if (player.carriedTitle >= 0 && gameFont != NULL &&
            gameState == GAME_PLAYING)
        {
            char ttl[48];
            snprintf(ttl, sizeof(ttl), "- %s -", TITLES[player.carriedTitle].name);
            SDL_Color tgold = { 235, 195, 90, 210 };
            float ux, uy;
            uilayout_get("unvan", 16.0f, 118.0f, &ux, &uy);
            draw_text(renderer, gameFont, ttl, ux, uy, tgold);
        }

        // Hedef HP bar (seçili mob varsa)
        Mob *hpTarget = mob_find_by_id(mobs, mobCount, player.targetMobId);
        target_hp_bar_render(renderer, hpTarget, winW, gameFont);

        // BOSS barı: K2'de Lord hayattayken tepe-orta geniş bar
        if (map_current() == MAP_DUNGEON_F2)
        {
            for (int i = 0; i < MAX_MOBS; i++)
            {
                if (!mobs[i].e.alive || mobs[i].type == NULL || !mobs[i].type->isBoss)
                    continue;
                float bw = 420.0f, bh = 16.0f;
                float bx, by;
                uilayout_get_aligned("boss_bar", ((float)winW - bw) * 0.5f, 54.0f,
                                     bw, bh, &bx, &by);
                float bcx = bx + bw / 2.0f;   // ad bu merkeze hizalanır
                r2d_set_color(renderer, 30, 15, 40, 255);
                r2d_fill_rect(renderer, &(SDL_FRect){ bx, by, bw, bh });
                float ratio = mobs[i].e.health / mobs[i].e.maxHealth;
                if (ratio < 0.0f) ratio = 0.0f;
                r2d_set_color(renderer, 170, 90, 230, 255);   // mor — lord rengi
                r2d_fill_rect(renderer, &(SDL_FRect){ bx, by, bw * ratio, bh });
                r2d_set_color(renderer, 230, 210, 255, 255);
                r2d_rect(renderer, &(SDL_FRect){ bx, by, bw, bh });
                if (gameFont != NULL)
                {
                    int tw = 0, th = 0;
                    TTF_GetStringSize(gameFont, mobs[i].type->name, 0, &tw, &th);
                    SDL_Color purple = { 220, 190, 255, 255 };
                    // Ad BARIN merkezine hizalanır (pencerenin değil) — bar
                    // taşınırsa yazısı beraber gitsin
                    draw_text(renderer, gameFont, mobs[i].type->name,
                              bcx - (float)tw / 2.0f, by + bh + 4.0f, purple);
                }
                break;
            }
        }

        health_bar_render(&player, renderer, gameFont);

        // Altın göstergesi (HP barının altı) + saat (sağ üst)
        if (gameFont != NULL)
        {
            char goldBuf[48];
            snprintf(goldBuf, sizeof(goldBuf), "Gold %d", player.gold);
            SDL_Color goldCol = { 255, 210, 60, 255 };
            float gx, gy;
            uilayout_get("altin", 20.0f, 76.0f, &gx, &gy);   // mana/stamina barlarının altı
            draw_text(renderer, gameFont, goldBuf, gx, gy, goldCol);
            if (player.dirtyGold > 0)
            {
                // Kirli kese: harcanamaz — aklayıcıya götür (kırmızı ek).
                // Altının SAĞINA yapışır: kendi yerleşim girdisi yok, çünkü
                // konumu altın metninin genişliğine bağlı (bağımsız taşınamaz).
                char dirtyBuf[24];
                snprintf(dirtyBuf, sizeof(dirtyBuf), "(+%d kirli)", player.dirtyGold);
                int gw2 = 0, gh2 = 0;
                TTF_GetStringSize(gameFont, goldBuf, 0, &gw2, &gh2);
                SDL_Color dirtyCol = { 255, 110, 90, 255 };
                draw_text(renderer, gameFont, dirtyBuf, gx + 6.0f + (float)gw2, gy,
                          dirtyCol);
            }

            // Bounty + itibar göstergesi (bulunduğun bölgenin değerleri)
            int hudRegion = region_of(player.rect.x + player.rect.w / 2.0f,
                                      player.rect.y + player.rect.h / 2.0f);
            // Değişken yükseklikli YIĞIN: bounty / itibar / gizli sırayla alt alta
            // dizilir, koşulu sağlamayan satır yer tutmaz. Yerleşimde tek girdi
            // var (yığının başlangıcı); satır aralığı kodda kalıyor çünkü font
            // yüksekliğine bağlı, konuma değil.
            float hudX = 20.0f, hudY = 140.0f;
            uilayout_get("durum_yigin", 20.0f, 140.0f, &hudX, &hudY);
            if (player.bounty[hudRegion] > 0)
            {
                char bountyBuf[32];
                snprintf(bountyBuf, sizeof(bountyBuf), "Bounty %d", player.bounty[hudRegion]);
                SDL_Color bountyCol = { 255, 90, 90, 255 };
                draw_text(renderer, gameFont, bountyBuf, hudX, hudY, bountyCol);
                hudY += 24.0f;
            }
            if (player.reputation[hudRegion] <= -1.0f || player.reputation[hudRegion] >= 1.0f)
            {
                char repBuf[32];
                snprintf(repBuf, sizeof(repBuf), "İtibar %d", (int)player.reputation[hudRegion]);
                SDL_Color repCol = player.reputation[hudRegion] < 0.0f
                                 ? (SDL_Color){ 255, 140, 100, 255 }
                                 : (SDL_Color){ 140, 230, 140, 255 };
                draw_text(renderer, gameFont, repBuf, hudX, hudY, repCol);
                hudY += 24.0f;
            }

            // Eğilme göstergesi
            if (player.sneaking)
            {
                SDL_Color sneakCol = { 170, 170, 190, 255 };
                draw_text(renderer, gameFont, "[Gizli]", hudX, hudY, sneakCol);
            }

            char clockBuf[40];   // "Gün 999 · İlkbahar  23:59" UTF-8 sığsın
            gametime_format(clockBuf, sizeof(clockBuf));
            SDL_Color clockCol = { 220, 220, 235, 255 };
            int tw = 0, th = 0;
            TTF_GetStringSize(gameFont, clockBuf, 0, &tw, &th);
            // SAĞA HİZALI: genişliği veriyoruz, çıkarmayı yerleşim yapıyor
            float ckx, cky;
            uilayout_get_aligned("saat", (float)winW - (float)tw - 20.0f, 20.0f,
                                 (float)tw, (float)th, &ckx, &cky);
            draw_text(renderer, gameFont, clockBuf, ckx, cky, clockCol);

            // ESO HEDEF İŞARETÇİSİ: aktif görev hedefi (minimap + dünya oku)
            {
                float qmx = 0.0f, qmy = 0.0f;
                bool qmOn = false;
                Uint8 asm2 = player.storyFlags[SF_ASSASSIN];
                if (map_current() == MAP_WORLD)
                {
                    if (asm2 == 2 && gametime_is_night())
                    { qmx = 10.2f * TILE_SIZE; qmy = 5.0f * TILE_SIZE; qmOn = true; }
                    else if (asm2 == 3 || asm2 == 5)   // locaya dön → kuyu
                    { qmx = 26.0f * TILE_SIZE; qmy = 15.0f * TILE_SIZE; qmOn = true; }
                    else if (asm2 == 4 && kemalIdx >= 0)
                        qmOn = npc_visible_pos(kemalIdx, &qmx, &qmy);
                }
                else if (map_current() == MAP_LOCA &&
                         (asm2 == 3 || asm2 == 5 || asm2 == 6 || asm2 == 8 ||
                          asm2 == 10 || asm2 == 12) && nerimanIdx >= 0)
                    qmOn = npc_visible_pos(nerimanIdx, &qmx, &qmy);
                if (!qmOn && map_current() == MAP_WORLD)
                {
                    if (asm2 == 6 || asm2 == 8 || asm2 == 10 || asm2 == 12)
                    { qmx = 26.0f * TILE_SIZE; qmy = 15.0f * TILE_SIZE; qmOn = true; }
                    else if (asm2 == 11 && gametime_is_night())
                    { qmx = 64.0f * TILE_SIZE; qmy = 19.0f * TILE_SIZE; qmOn = true; }
                }
                minimap_set_quest(qmx, qmy, qmOn);

                // Dünya oku: hedefin üstünde süzülen altın chevron (3 sıra piksel)
                if (qmOn)
                {
                    float bobQ = SDL_sinf((float)SDL_GetTicks() / 250.0f) * 5.0f;
                    float qsx = (qmx - camera.x) * camera.zoom;
                    float qsy = (qmy - 92.0f + bobQ - camera.y) * camera.zoom;
                    r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                    r2d_set_color(renderer, 255, 210, 60, 235);
                    SDL_FRect c1 = { qsx - 12.0f, qsy, 24.0f, 5.0f };
                    SDL_FRect c2 = { qsx - 7.0f, qsy + 6.0f, 14.0f, 5.0f };
                    SDL_FRect c3 = { qsx - 2.0f, qsy + 12.0f, 4.0f, 5.0f };
                    r2d_fill_rect(renderer, &c1);
                    r2d_fill_rect(renderer, &c2);
                    r2d_fill_rect(renderer, &c3);
                }
            }

            // ESO GÖREV TAKİPÇİSİ (sağ üst, saatin altı): aktif hedef hep görünür
            {
                const char *qtTitle = NULL;
                char qtObj[96] = {0};
                Uint8 asq = player.storyFlags[SF_ASSASSIN];
                if (asq == 2) {
                    qtTitle = "Sınav: Görülmeden";
                    snprintf(qtObj, sizeof(qtObj),
                             "%s", gametime_is_night()
                                   ? "Baran'ın tezgahından kutuyu al (tanıksız)"
                                   : "Geceyi bekle — kutu gece alınır");
                }
                else if (asq == 3) {
                    qtTitle = "Sınav: Görülmeden";
                    snprintf(qtObj, sizeof(qtObj), "Kutuyu Neriman'a götür");
                }
                else if (asq == 4) {
                    int qm = 0;
                    for (int b3 = 0; b3 < 4; b3++)
                        if (player.storyFlags[SF_TAIL_MARK] & (1u << b3)) qm++;
                    qtTitle = "Gölge Ol";
                    snprintf(qtObj, sizeof(qtObj),
                             "Kemal'in duraklarını işaretle (%d/3) — görünme", qm);
                }
                else if (asq == 5) {
                    qtTitle = "Gölge Ol";
                    snprintf(qtObj, sizeof(qtObj), "Neriman'a rapor ver");
                }
                else if (asq == 6) {
                    qtTitle = "Sessiz El";
                    snprintf(qtObj, sizeof(qtObj), "Neriman'ın yeni işi var — locaya in");
                }
                else if (asq == 7) {
                    qtTitle = "Dilsiz Şişe";
                    snprintf(qtObj, sizeof(qtObj),
                             "3 Zehirli Öz + 2 Mavi Ot → simya kazanı; İğne'ye götür");
                }
                else if (asq == 8) {
                    qtTitle = "Sessiz El";
                    snprintf(qtObj, sizeof(qtObj), "Neriman'la konuş — yeni iş");
                }
                else if (asq == 10) {
                    qtTitle = "Sessiz El";
                    snprintf(qtObj, sizeof(qtObj), "Neriman son işi anlatacak");
                }
                else if (asq == 11) {
                    qtTitle = "İlk İmza";
                    snprintf(qtObj, sizeof(qtObj),
                             "%s", gametime_is_night()
                                   ? "Kaçak Tahsildar — kervan yolunda (doğu)"
                                   : "Geceyi bekle — Tahsildar gece yürür");
                }
                else if (asq == 12) {
                    qtTitle = "İlk İmza";
                    snprintf(qtObj, sizeof(qtObj), "İmza atıldı — Neriman'a dön");
                }
                else if (player.contractActive >= 0) {
                    const AssassinContract *ac2 =
                        &ASSASSIN_CONTRACTS[player.contractActive];
                    qtTitle = "Kontrat: ";  // başlık + isim tek satır
                    static char ctBuf[64];
                    snprintf(ctBuf, sizeof(ctBuf), "Kontrat — %s", ac2->name);
                    qtTitle = ctBuf;
                    if (player.contractProgress >= ac2->count)
                        snprintf(qtObj, sizeof(qtObj),
                                 "Tamam — kontrat tahtasına dön (loca)");
                    else
                        snprintf(qtObj, sizeof(qtObj), "%s: %d / %d",
                                 ac2->name, player.contractProgress, ac2->count);
                }
                if (qtTitle != NULL) {
                    SDL_Color qgold = { 255, 210, 60, 255 };
                    SDL_Color qwhite = { 225, 225, 235, 255 };
                    // Sağa hizalı İKİ satır: her satır kendi genişliğiyle
                    // hizalanır, o yüzden ayrı ayrı çözülüyor. Satır aralığı
                    // (22px) fonta bağlı, yerleşime değil — kodda kalıyor.
                    int qw = 0, qh = 0;
                    float qtx, qty;
                    TTF_GetStringSize(gameFont, qtTitle, 0, &qw, &qh);
                    uilayout_get_aligned("gorev_takip",
                                         (float)winW - (float)qw - 20.0f, 48.0f,
                                         (float)qw, (float)qh, &qtx, &qty);
                    draw_text(renderer, gameFont, qtTitle, qtx, qty, qgold);
                    TTF_GetStringSize(gameFont, qtObj, 0, &qw, &qh);
                    uilayout_get_aligned("gorev_takip",
                                         (float)winW - (float)qw - 20.0f, 48.0f,
                                         (float)qw, (float)qh, &qtx, &qty);
                    draw_text(renderer, gameFont, qtObj, qtx, qty + 22.0f, qwhite);
                }
            }

            // ESO GÖREV BANDI: kabul/aşama değişiminde ekran üstü büyük duyuru
            if (questBannerTimer > 0.0f && questBannerTitle != NULL)
            {
                questBannerTimer -= dt;
                float qa = questBannerTimer > 4.0f ? (5.0f - questBannerTimer)
                         : questBannerTimer < 1.0f ? questBannerTimer : 1.0f;
                if (qa < 0.0f) qa = 0.0f;
                if (qa > 1.0f) qa = 1.0f;
                Uint8 qalpha = (Uint8)(qa * 255.0f);
                int bw = 0, bh = 0;
                TTF_GetStringSize(gameFont, questBannerTitle, 0, &bw, &bh);
                SDL_Color bgold = { 255, 210, 60, qalpha };
                float qbx, qby;
                uilayout_get_aligned("gorev_bandi",
                                     (winW - (float)bw) / 2.0f, 90.0f,
                                     (float)bw, (float)bh, &qbx, &qby);
                draw_text(renderer, gameFont, questBannerTitle, qbx, qby, bgold);
                // süs çizgileri (ESO dokusu) — başlığın iki yanına, ona göre
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                r2d_set_color(renderer, 255, 210, 60, (Uint8)(qalpha / 2));
                float qly = qby + 9.0f;
                SDL_FRect ql = { qbx - 60.0f, qly, 50.0f, 2.0f };
                SDL_FRect qr = { qbx + (float)bw + 10.0f, qly, 50.0f, 2.0f };
                r2d_fill_rect(renderer, &ql);
                r2d_fill_rect(renderer, &qr);
                if (questBannerSub != NULL)
                {
                    TTF_GetStringSize(gameFont, questBannerSub, 0, &bw, &bh);
                    SDL_Color bwhite = { 230, 230, 240, qalpha };
                    // Alt satır başlığa GÖRE (26px altı): bandı taşıdığında
                    // ikisi beraber gitsin, ayrı yerleşim girdisi gerekmesin
                    float qsx, qsy;
                    uilayout_get_aligned("gorev_bandi",
                                         (winW - (float)bw) / 2.0f, 90.0f,
                                         (float)bw, (float)bh, &qsx, &qsy);
                    draw_text(renderer, gameFont, questBannerSub,
                              qsx, qsy + 26.0f, bwhite);
                }
            }
        }

        // Hotbar (alt-orta): skill CD'leri + pot adetleri
        // Hotbar sürüklemesi: ✕ kaldırma hedefi (yalnız sürüklerken görünür)
        if (dragHbSlot >= 0)
        {
            // Dikdörtgen hotbar'dan gelir: tıklama hedefiyle aynı kaynak
            SDL_FRect hbRm = hotbar_remove_zone_rect(winW, winH);
            ui_cell_gray(renderer, hbRm.x, hbRm.y, hbRm.w, hbRm.h);
            if (gameFont != NULL)
                draw_text(renderer, gameFont, "X", hbRm.x + 18.0f, hbRm.y + 14.0f,
                          (SDL_Color){ 255, 110, 90, 255 });
        }
        // İĞNE ŞARJ GÖSTERGESİ: iğne takılıyken barların altında 3 kare
        if (gameState == GAME_PLAYING &&
            equipment.slots[EQUIP_SHIELD].type == &ITEM_TYPES[ITEM_POISON_NEEDLE])
        {
            int vCount = bag_count(&ITEM_TYPES[ITEM_NEEDLE_VENOM]);
            // Eski konum (20,78) ALTIN göstergesiyle (20,76) çakışıyordu: iğne
            // takılıyken iki metin üst üste biniyordu. Yerleşimde yığının altına
            // alındı. Kareler etikete GÖRE duruyor (58,4) — tek girdi yeter.
            float nx, ny;
            uilayout_get("igne_sarj", 20.0f, 216.0f, &nx, &ny);
            if (gameFont != NULL)
                draw_text(renderer, gameFont, "Zehir", nx, ny,
                          (SDL_Color){ 170, 100, 230, 255 });
            for (int vq = 0; vq < 3; vq++)
            {
                SDL_FRect vc = { nx + 58.0f + (float)vq * 16.0f, ny + 4.0f,
                                 10.0f, 10.0f };
                r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                if (vq < vCount) r2d_set_color(renderer, 150, 70, 190, 255);
                else             r2d_set_color(renderer, 60, 45, 75, 255);
                r2d_fill_rect(renderer, &vc);
            }
        }

        // Silah uygunluğu: uymayan beceriler barda SOLUK çizilir (kullanım
        // kapılarının aynası — asıl engel hotbarPressed akışında duruyor)
        bool abilityUsable[ABILITY_COUNT];
        {
            bool hbSh = equipment.slots[EQUIP_SHIELD].type != NULL &&
                        equipment.slots[EQUIP_SHIELD].type->weaponType
                            == WEAPON_SHIELD;
            for (int ai = 0; ai < ABILITY_COUNT; ai++)
            {
                const AbilityDef *ad = &ABILITIES[ai];
                if (ad->skill == SKILL_DESTRUCTION ||
                    ad->skill == SKILL_CONJURATION || ad->skill == SKILL_ECHO ||
                    ad->skill == SKILL_LIGHT)
                    abilityUsable[ai] = true;   // büyü silahtan bağımsız
                else if (ai == ABILITY_SHIELD_BASH)
                    abilityUsable[ai] = hbSh;
                else if (ai == ABILITY_NEEDLE_STING)
                    abilityUsable[ai] =
                        (equipment.slots[EQUIP_SHIELD].type ==
                             &ITEM_TYPES[ITEM_POISON_NEEDLE]) &&
                        bag_count(&ITEM_TYPES[ITEM_NEEDLE_VENOM]) > 0;
                else if (ai == ABILITY_SMOKE_BOMB || ai == ABILITY_SHADOWSTEP)
                    abilityUsable[ai] = true;   // silahtan bağımsız
                else if (ad->skill == SKILL_TWO_HANDED)
                    abilityUsable[ai] = player.spearEquipped;
                else
                    abilityUsable[ai] =
                        ((ad->skill == SKILL_ARCHERY) == player.bowEquipped);
            }
        }
        skillfx_render_punch(renderer, winW, winH);   // ekran darbesi (HUD'dan ÖNCE)

        hotbar_render(&hotbar, renderer, itemIconTex, &itemTex, abilityIconTex, gameFont,
                      player.abilityCd, player.itemCd, &inventory, abilityUsable,
                      winW, winH);

        // ── HOTBAR İPUCU: farenin altındaki slotun ADI. Hücrelerde yalnız
        // kısaltma/ikon var, oyuncu (ve test eden) neyin ne olduğunu
        // anlayamıyordu. Beceride ad + bekleme, eşyada ad + adet.
        if (gameFont != NULL && gameState == GAME_PLAYING)
        {
            float hmx, hmy;
            SDL_GetMouseState(&hmx, &hmy);
            int hs = hotbar_slot_at(hmx, hmy, winW, winH);
            if (hs >= 0 && hs < HOTBAR_SLOTS)
            {
                const HotbarSlot *sl = &hotbar.slots[hs];
                char tip[96] = {0};
                if (sl->kind == HOTBAR_SKILL &&
                    sl->abilityId >= 0 && sl->abilityId < ABILITY_COUNT)
                {
                    const AbilityDef *ad = &ABILITIES[sl->abilityId];
                    float cd = player.abilityCd[sl->abilityId];
                    if (cd > 0.05f)
                        SDL_snprintf(tip, sizeof(tip), "%s  (%.1f sn)", ad->name, cd);
                    else if (ad->mana > 0.5f)
                        SDL_snprintf(tip, sizeof(tip), "%s  (%.0f mana)", ad->name, ad->mana);
                    else
                        SDL_snprintf(tip, sizeof(tip), "%s", ad->name);
                }
                else if (sl->kind == HOTBAR_ITEM && sl->itemType != NULL)
                {
                    int cnt = bag_count(sl->itemType);
                    SDL_snprintf(tip, sizeof(tip), "%s  x%d", sl->itemType->name, cnt);
                }
                if (tip[0] != '\0')
                {
                    // Kutu fareden YUKARI çizilir (imleç yazıyı kapatmasın) ve
                    // ekran kenarına taşmaz.
                    float tw = (float)SDL_strlen(tip) * 8.0f + 16.0f;
                    float tx2 = hmx - tw * 0.5f;
                    if (tx2 < 4.0f) tx2 = 4.0f;
                    if (tx2 + tw > (float)winW - 4.0f) tx2 = (float)winW - 4.0f - tw;
                    float ty2 = hmy - 34.0f;
                    r2d_set_color(renderer, 18, 16, 24, 225);
                    r2d_fill_rect(renderer, &(SDL_FRect){ tx2, ty2, tw, 24.0f });
                    r2d_set_color(renderer, 120, 110, 90, 255);
                    r2d_rect(renderer, &(SDL_FRect){ tx2, ty2, tw, 24.0f });
                    draw_text(renderer, gameFont, tip, tx2 + 8.0f, ty2 + 4.0f,
                              (SDL_Color){ 236, 228, 200, 255 });
                }
            }
        }

        // Yay kuşanılıyken aktif ok sayacı (hotbar'ın üstünde) — tür adıyla
        if (player.bowEquipped && gameFont != NULL)
        {
            int nArrow = bag_count(&ITEM_TYPES[activeAmmo]);
            char ammoBuf[40];
            snprintf(ammoBuf, sizeof(ammoBuf), "%s: %d",
                     ITEM_TYPES[activeAmmo].name, nArrow);
            SDL_Color ammoCol = ammo_any(&inventory)
                              ? (SDL_Color){ 230, 220, 180, 255 }
                              : (SDL_Color){ 255, 120, 100, 255 };
            float amx, amy;
            uilayout_get("ok_sayaci", winW / 2.0f - 40.0f, (float)winH - 96.0f,
                         &amx, &amy);
            draw_text(renderer, gameFont, ammoBuf, amx, amy, ammoCol);
        }

        lockpick_render(renderer, gameFont, winW, winH, &perkFx,
                        player.skills[SKILL_LOCKPICKING].level);

        // Vinyet sahnesi: dünyanın üstüne karartı + diorama + typewriter metin
        scene_render(renderer, gameFont, winW, winH);

        if (minimapOpen)
            minimap_render(renderer, player.rect.x + player.rect.w / 2.0f,
                           player.rect.y + player.rect.h / 2.0f,
                           mobs, mobCount, winW, winH);

        if (questLogOpen)
        {
            // Yolculuk günlüğü (Faz B2): kitap paneli — sayfa klampi burada
            journalPageCount = journal_render(renderer, gameFont, bookTex,
                                              questStatus, &player, journalTab,
                                              journalPage, winW, winH);
            if (journalPage >= journalPageCount)
                journalPage = journalPageCount - 1;
        }

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
            // DAVRANIŞ DEĞİŞTİ: eski y = winH*0.15 (oransal), yeni y = 108 (sabit).
            // 720px'te aynı yer; daha uzun pencerede splash aşağı kaymaz — üst
            // kenardan sabit uzaklıkta durur. Duyuru öğesi için doğrusu bu.
            float spx, spy;
            uilayout_get_aligned("bolge_splash", ((float)winW - (float)tw) * 0.5f,
                                 108.0f, (float)tw, (float)th, &spx, &spy);
            draw_text(renderer, splashFont, splashText, spx, spy, col);
        }
        

        if (inventoryOpen || ed_prev("envanter"))
        {
            float panelXF, panelYF;
            inventory_panel_origin(winW, winH, &panelXF, &panelYF);
            int panelX = (int)panelXF, panelY = (int)panelYF;
            equipment_render(&equipment, renderer, itemIconTex, &itemTex,
                             built, builtCount, panelX, panelY, gameFont);

            // Sekme şeridi: [Eşya] [Malzeme] — I döngüsü / tıklama / sürükle-bırak hedefi
            {
                float tabY = (float)(panelY + INV_EQUIP_H - 28);
                const char *tabNames[2] = { "Eşya", "Malzeme" };
                for (int t = 0; t < 2; t++)
                {
                    float tx = (float)panelX + t * 120.0f;
                    if (t == invTab) ui_cell(renderer, tx, tabY, 114.0f, 26.0f);
                    else             ui_cell_gray(renderer, tx, tabY, 114.0f, 26.0f);
                    SDL_Color tc = (t == invTab) ? (SDL_Color){ 255, 230, 120, 255 }
                                                 : (SDL_Color){ 170, 170, 180, 255 };
                    if (gameFont != NULL)
                        draw_text(renderer, gameFont, tabNames[t],
                                  tx + 12.0f, tabY + 4.0f, tc);
                }
                // Çöp hücresi: sürüklenen item buraya bırakılırsa İMHA
                {
                    float trX = (float)panelX + INV_PANEL_W - 84.0f;
                    ui_cell_gray(renderer, trX, tabY, 78.0f, 26.0f);
                    SDL_Color trc = (dragInvSlot >= 0)
                                  ? (SDL_Color){ 255, 120, 100, 255 }
                                  : (SDL_Color){ 150, 130, 130, 255 };
                    if (gameFont != NULL)
                        draw_text(renderer, gameFont, "Çöp", trX + 22.0f,
                                  tabY + 4.0f, trc);
                }
                if (gameFont != NULL)
                    draw_text(renderer, gameFont, "TAB: sekme", 
                              (float)panelX + 250.0f, tabY + 4.0f,
                              (SDL_Color){ 150, 150, 160, 255 });
            }

            inventory_render((invTab == 1) ? &craftBag : &inventory,
                             renderer, itemIconTex, &itemTex, gameFont,
                             (float)panelX, (float)(panelY + INV_EQUIP_H),
                             (invTab == 1) ? "Malzeme" : "Eşya");

            // Çöp onayı: küçük panel — [1] yok et · [2] vazgeç
            if (trashSlot >= 0)
            {
                Inventory *tb2 = (trashTab == 1) ? &craftBag : &inventory;
                const ItemStack *ts2 = &tb2->slots[trashSlot];
                if (ts2->type == NULL) { trashTab = -1; trashSlot = -1; }
                else
                {
                    float cw = 360.0f, ch = 92.0f;
                    float cx = (winW - cw) / 2.0f, cy = winH / 2.0f - 120.0f;
                    ui_panel(renderer, cx, cy, cw, ch);
                    if (gameFont != NULL)
                    {
                        char cbuf[96];
                        if (ts2->count > 1)
                            snprintf(cbuf, sizeof(cbuf), "%s x%d yok edilsin mi?",
                                     ts2->type->name, ts2->count);
                        else
                            snprintf(cbuf, sizeof(cbuf), "%s yok edilsin mi?",
                                     ts2->type->name);
                        SDL_Color wt = { 255, 255, 255, 255 };
                        SDL_Color red = { 255, 140, 120, 255 };
                        SDL_Color dim = { 170, 170, 180, 255 };
                        draw_text(renderer, gameFont, cbuf, cx + 16.0f, cy + 12.0f, wt);
                        draw_text(renderer, gameFont, "[1] Yok et", cx + 16.0f,
                                  cy + 48.0f, red);
                        draw_text(renderer, gameFont, "[2]/ESC Vazgeç", cx + 150.0f,
                                  cy + 48.0f, dim);
                    }
                }
            }
        }

        if (skillsOpen || ed_prev("beceri_paneli"))
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
                                  renderer, gameFont, abilityIconTex,
                                  40.0f + SKILL_PANEL_W + 10.0f, 40.0f);
            }
        }

        if (shopOpen || ed_prev("dukkan"))
            shop_render(renderer, gameFont, itemIconTex, &itemTex, player.gold,
                        rep_buy_mult(player.reputation[region_of(player.rect.x, player.rect.y)])
                            * (shopCaravan ? 1.15f : 1.0f)
                            * ((!shopLoca && npc_grudge(shopVendorIdx)) ? 1.25f : 1.0f),
                        40.0f, 40.0f,
                        (shopCaravan||shopLoca||shopDogu) ? caravanStock : NULL,
                        (shopCaravan||shopLoca||shopDogu) ? caravanStockN : 0);

        // Craft paneli + aktif sürecin ilerleme barı
        if ((craftOpen && craftStation >= 0) || ed_prev("craft_paneli"))
        {
            // Ön izlemede istasyon kapalı olabilir (craftStation = -1); panelin
            // çizilebilmesi için geçerli bir istasyon gerekiyor, 0'ıncısı seçilir.
            int edStation = (craftStation >= 0) ? craftStation : 0;
            // Sunakta 'hedef' satırı seçili ekipmanı gösterir; diğerlerinde kuşanılı silah
            const ItemStack *pw = (STATIONS[edStation].type == STATION_ENCHANT)
                                ? &equipment.slots[enchTarget]
                                : &equipment.slots[EQUIP_WEAPON];
            craft_panel_render(edStation, &craftBag, &inventory, player.skills,
                               craftScroll,
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
        if (locaBoardOpen || ed_prev("suikast_panosu"))
        {
            bool cReady2 = player.contractActive >= 0 &&
                player.contractProgress >=
                    ASSASSIN_CONTRACTS[player.contractActive].count;
            assassin_board_render(player.assassinPoints, player.contractActive,
                                  player.contractProgress, cReady2,
                                  renderer, gameFont, 40.0f, 40.0f);
        }
        if (boardOpen || ed_prev("ilan_panosu"))
            contract_panel_render(renderer, gameFont, &craftBag, &inventory,
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
            int dfs = NPCS[dialogNpc].frameSize > 0 ? NPCS[dialogNpc].frameSize : 64;
            float dlgMx, dlgMy;
            SDL_GetMouseState(&dlgMx, &dlgMy);
            dialog_render(&npcDialogs[dialogNpc], dialogNode, NPCS[dialogNpc].name,
                          dstates, renderer, gameFont, winW, winH,
                          npc_textures[dialogNpc], (float)dfs,
                          dlgMx, dlgMy);
        }

        // Muhafız yakalama paneli: öde / karşı koy / rüşvet
        if ((catchOpen || ed_prev("yakalama_paneli")) && gameFont != NULL)
        {
            // Bu panelin HEM genişliği HEM yüksekliği sabit (içerik üç seçenek,
            // satır sayısı değişmiyor) — yerleşimde tam boyutlu tek öğe.
            // Tıklama bağı yok: seçim 1/2/3 tuşlarıyla yapılıyor.
            float pw = 480.0f, ph = 168.0f;
            uilayout_get_size("yakalama_paneli", pw, ph, &pw, &ph);
            float pX, pY;
            uilayout_get_aligned("yakalama_paneli", ((float)winW - pw) / 2.0f,
                                 (float)winH / 3.0f, pw, ph, &pX, &pY);
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

        // Letterbox bantları (D1a — cine modu): üst/alt siyah şerit
        if (cineBand > 0.01f)
        {
            float bandH = 56.0f * cineBand;
            r2d_set_blend(renderer, R2D_BLEND_ALPHA);
            r2d_set_color(renderer, 0, 0, 0, 255);
            r2d_fill_rect(renderer, &(SDL_FRect){ 0, 0, (float)winW, bandH });
            r2d_fill_rect(renderer, &(SDL_FRect){ 0, winH - bandH, (float)winW, bandH });
        }
        // Diyalog üst bandı: YALNIZ üstte (alt panelin işi yok — sinematik his)
        else if (dlgBand > 0.01f)
        {
            float bandH = 44.0f * dlgBand;
            r2d_set_blend(renderer, R2D_BLEND_ALPHA);
            r2d_set_color(renderer, 0, 0, 0, 235);
            r2d_fill_rect(renderer, &(SDL_FRect){ 0, 0, (float)winW, bandH });
        }

        // Sürükleme hayaleti (Cila): taşınan item imleci izler
        if (dragInvSlot >= 0 || dragEquipSlot != EQUIP_NONE)
        {
            const ItemStack *ds = (dragInvSlot >= 0)
                                ? &((dragTab == 1) ? &craftBag : &inventory)->slots[dragInvSlot]
                                : &equipment.slots[dragEquipSlot];
            if (ds->type != NULL)
            {
                float gmx = 0.0f, gmy = 0.0f;
                SDL_GetMouseState(&gmx, &gmy);
                R2DTexture *gicon = item_icon_tex(&itemTex, ds->type);
                if (gicon != NULL)
                {
                    SDL_FRect gsrc = ds->type->iconSrc;
                    SDL_FRect gdst = { gmx - 20.0f, gmy - 20.0f, 40.0f, 40.0f };
                    r2d_texture_set_alpha_mod(gicon, 190);
                    r2d_set_blend(renderer, R2D_BLEND_ALPHA);
                    r2d_draw_texture(renderer, gicon, &gsrc, &gdst);
                    r2d_texture_set_alpha_mod(gicon, 255);
                }
            }
        }

        // Tooltip (Cila): imlecin altındaki hücrenin tam detayı — envanter/
        // ekipman/dükkan; ekipman karşılaştırması aynı slottaki kuşanılıyla
        if (gameState == GAME_PLAYING && !scene_active())
        {
            float tmx = 0.0f, tmy = 0.0f;
            SDL_GetMouseState(&tmx, &tmy);
            const ItemStack *tip = NULL, *tipEq = NULL;

            if (inventoryOpen)
            {
                const Inventory *tb = (invTab == 1) ? &craftBag : &inventory;
                float tpxF, tpyF;
                inventory_panel_origin(winW, winH, &tpxF, &tpyF);
                int tpx = (int)tpxF, tpy = (int)tpyF;
                int sl = inventory_slot_at(tmx, tmy, (float)tpx, (float)(tpy + INV_EQUIP_H));
                if (sl >= 0 && sl < tb->cap && tb->slots[sl].type != NULL)
                {
                    tip = &tb->slots[sl];
                    const ItemType *tt = tip->type;
                    if (tt->equipable && tt->equipSlot >= 0 &&
                        tt->equipSlot < EQUIP_SLOT_COUNT &&
                        equipment.slots[tt->equipSlot].type != NULL)
                        tipEq = &equipment.slots[tt->equipSlot];
                }
                if (tip == NULL)
                {
                    EquipSlot es = equipment_slot_at(tmx, tmy, tpx, tpy);
                    if (es >= 0 && es < EQUIP_SLOT_COUNT &&
                        equipment.slots[es].type != NULL)
                        tip = &equipment.slots[es];
                }
            }
            if (tip == NULL && shopOpen)
            {
                int se = shop_entry_at(tmx, tmy, 40.0f, 40.0f,
                                       (shopCaravan||shopLoca||shopDogu) ? caravanStockN : 0);
                if (se >= 0 &&
                    se < ((shopCaravan||shopLoca||shopDogu) ? caravanStockN : SHOP_STOCK_COUNT))
                {
                    static ItemStack shopTip;   // stok taze instance gibi gösterilir
                    shopTip = item_make(((shopCaravan||shopLoca||shopDogu) ? caravanStock : SHOP_STOCK)[se].type,
                                        1, 0);
                    tip = &shopTip;
                    if (shopTip.type->equipable && shopTip.type->equipSlot >= 0 &&
                        shopTip.type->equipSlot < EQUIP_SLOT_COUNT &&
                        equipment.slots[shopTip.type->equipSlot].type != NULL)
                        tipEq = &equipment.slots[shopTip.type->equipSlot];
                }
            }
            if (tip != NULL)
                tooltip_render(renderer, gameFont, tip, tipEq, tmx, tmy, winW, winH);
        }

        if (gameState == GAME_PAUSED || gameState == GAME_MENU)
            menu_render(renderer, gameFont, menuScreen, winW, winH, slotSums);
        else if (gameState == GAME_CREATE)
        {
            r2d_set_blend(renderer, R2D_BLEND_ALPHA);
            r2d_set_color(renderer, 8, 6, 12, 255);
            r2d_fill_rect(renderer, &(SDL_FRect){ 0, 0, (float)winW, (float)winH });

            float cx0 = winW / 2.0f;
            int tw = 0, th = 0;
            SDL_Color white = { 255, 255, 255, 255 };
            SDL_Color dimc  = { 150, 145, 165, 255 };
            SDL_Color goldc = { 235, 195, 90, 255 };

            const char *ct = "Kim olacaksın?";
            TTF_GetStringSize(splashFont, ct, 0, &tw, &th);
            draw_text(renderer, splashFont, ct, cx0 - tw / 2.0f,
                      winH * 0.10f, white);

            // İsim kutusu (imleç yanıp söner)
            draw_text(renderer, gameFont, "İsim:", cx0 - 180.0f,
                      winH * 0.24f, dimc);
            r2d_set_color(renderer, 30, 28, 38, 255);
            r2d_fill_rect(renderer, &(SDL_FRect){ cx0 - 110.0f,
                          winH * 0.24f - 6.0f, 290.0f, 30.0f });
            char nbuf[32];
            bool blink = (SDL_GetTicks() / 400) % 2 == 0;
            snprintf(nbuf, sizeof(nbuf), "%s%s",
                     createName[0] ? createName : "", blink ? "_" : "");
            draw_text(renderer, gameFont, nbuf, cx0 - 100.0f,
                      winH * 0.24f, white);

            // Ten tonları
            draw_text(renderer, gameFont, "Ten:", cx0 - 180.0f,
                      winH * 0.36f + 10.0f, dimc);
            static const Uint8 tns[3][3] = { { 236, 199, 165 },
                { 205, 165, 125 }, { 160, 115, 82 } };
            for (int t2 = 0; t2 < 3; t2++)
            {
                float sx = cx0 - 70.0f + t2 * 50.0f, sy = winH * 0.36f;
                r2d_set_color(renderer, tns[t2][0], tns[t2][1], tns[t2][2], 255);
                r2d_fill_rect(renderer, &(SDL_FRect){ sx, sy, 40.0f, 40.0f });
                if (t2 == createSkin)
                {
                    r2d_set_color(renderer, 255, 255, 255, 255);
                    r2d_rect(renderer, &(SDL_FRect){ sx - 3, sy - 3, 46, 46 });
                }
            }

            // Geçmiş kartları
            static const char *bgName[4] = { "Gezgin", "Eski Asker",
                                             "Avcı", "Çırak Simyacı" };
            static const char *bgPlus[4] = { "Dengeli başlangıç",
                "+2 STR, Demir Miğfer", "+2 DEX, Yay + 30 ok",
                "+2 INT, 2 mana iksiri" };
            static const char *bgCost[4] = { "Bedel yok",
                "Bedel: köy itibarı -5", "Bedel: kılıçsız",
                "Bedel: 25 altın eksik" };
            for (int b2 = 0; b2 < 4; b2++)
            {
                float bx2 = cx0 - 400.0f + b2 * 205.0f, by2 = winH * 0.48f;
                r2d_set_color(renderer, 30, 28, 38, 255);
                r2d_fill_rect(renderer, &(SDL_FRect){ bx2, by2, 190.0f, 96.0f });
                if (b2 == createBg)
                {
                    r2d_set_color(renderer, 235, 195, 90, 255);
                    r2d_rect(renderer, &(SDL_FRect){ bx2 - 2, by2 - 2, 194, 100 });
                }
                draw_text(renderer, gameFont, bgName[b2], bx2 + 10.0f,
                          by2 + 8.0f, b2 == createBg ? goldc : white);
                draw_text(renderer, gameFont, bgPlus[b2], bx2 + 10.0f,
                          by2 + 38.0f, dimc);
                draw_text(renderer, gameFont, bgCost[b2], bx2 + 10.0f,
                          by2 + 62.0f, dimc);
            }

            // Başla
            ui_button(renderer, cx0 - 120.0f, winH * 0.78f, 240.0f, 56.0f);
            TTF_GetStringSize(gameFont, "Başla", 0, &tw, &th);
            draw_text(renderer, gameFont, "Başla", cx0 - tw / 2.0f,
                      winH * 0.78f + 18.0f, UI_TEXT_DARK);
            draw_text(renderer, gameFont,
                      "[Enter] Başla   [ESC] Geri", cx0 - 130.0f,
                      winH * 0.78f + 70.0f, dimc);
        }
        else if (gameState == GAME_TITLE)
        {
            // Atmosferik başlık: tam karanlık + oyun adı + nefes alan çağrı satırı
            titlePulse += dt;
            r2d_set_blend(renderer, R2D_BLEND_ALPHA);
            r2d_set_color(renderer, 6, 4, 10, 255);
            SDL_FRect full = { 0, 0, (float)winW, (float)winH };
            r2d_fill_rect(renderer, &full);

            const char *tt = "PIXE";
            int tw = 0, th = 0;
            TTF_GetStringSize(splashFont, tt, 0, &tw, &th);
            SDL_Color pale = { 205, 195, 220, 255 };
            draw_text(renderer, splashFont, tt,
                      (winW - tw) / 2.0f, winH * 0.34f, pale);

            const char *st = "Bir fatih konuşamıyor; adına altı ağız konuşuyor.";
            TTF_GetStringSize(gameFont, st, 0, &tw, &th);
            SDL_Color dimc = { 120, 105, 145, 255 };
            draw_text(renderer, gameFont, st,
                      (winW - tw) / 2.0f, winH * 0.34f + 56.0f, dimc);

            // "bir tuşa bas" — kalp atışı ritminde soluk alır
            float a = 0.55f + 0.45f * SDL_sinf(titlePulse * 2.2f);
            SDL_Color call = { 220, 215, 235, (Uint8)(a * 255.0f) };
            const char *pr = "- devam etmek için bir tuşa bas -";
            TTF_GetStringSize(gameFont, pr, 0, &tw, &th);
            draw_text(renderer, gameFont, pr,
                      (winW - tw) / 2.0f, winH * 0.72f, call);
        }

        // YERLEŞİM DÜZENLEYİCİSİ en son çizilir: menü, tooltip, ölüm ekranı —
        // hepsinin üstünde olmalı, yoksa düzenlediğin tutamak altta kalır.
        // Post-fx ayrımının HUD tarafında olduğu için filtreden de etkilenmez
        // (ölçüm aracı filtrelenirse ölçüm bozulur).
        uieditor_render(renderer, gameFont, winW, winH);

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
    tilemap_free_details();

    for (int i = 0; i < MOB_TYPE_COUNT; i++)
    {
        if (mob_textures[i])
            r2d_texture_destroy(NULL, mob_textures[i]); // NULL olanları atla
    }

        
    for (int i = 0; i < NPC_COUNT; i++)
        if (npc_textures[i])
            r2d_texture_destroy(NULL, npc_textures[i]);

    for (int i = 0; i < DECOR_COUNT && i < DECOR_TEX_CAP; i++)
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
    if (bookTex)    r2d_texture_destroy(NULL, bookTex);
    if (nestsTex)   r2d_texture_destroy(NULL, nestsTex);
    if (abilityIconTex) r2d_texture_destroy(NULL, abilityIconTex);
    if (lanternTex) r2d_texture_destroy(NULL, lanternTex);
    if (sofraTex) r2d_texture_destroy(NULL, sofraTex);
    if (snakeTex) r2d_texture_destroy(NULL, snakeTex);
    if (turtleTex) r2d_texture_destroy(NULL, turtleTex);
    if (shieldTex)  r2d_texture_destroy(NULL, shieldTex);
    if (spikesTex)  r2d_texture_destroy(NULL, spikesTex);
    if (clawTex)    r2d_texture_destroy(NULL, clawTex);
    if (icetacleTex) r2d_texture_destroy(NULL, icetacleTex);
    if (tornadoTex) r2d_texture_destroy(NULL, tornadoTex);
    if (darkburstTex) r2d_texture_destroy(NULL, darkburstTex);
    if (hitWeaponTex)  r2d_texture_destroy(NULL, hitWeaponTex);
    if (hitMagicTex)   r2d_texture_destroy(NULL, hitMagicTex);
    if (splatSmallTex) r2d_texture_destroy(NULL, splatSmallTex);
    if (splatBigTex)   r2d_texture_destroy(NULL, splatBigTex);
    if (sparkBlockTex) r2d_texture_destroy(NULL, sparkBlockTex);
    if (castCircleTex) r2d_texture_destroy(NULL, castCircleTex);
    if (holyBurstTex)  r2d_texture_destroy(NULL, holyBurstTex);
    if (holyImpactTex) r2d_texture_destroy(NULL, holyImpactTex);
    if (echoWispTex)   r2d_texture_destroy(NULL, echoWispTex);
    for (int i = 0; i < 4; i++)
        if (slashArcTex[i]) r2d_texture_destroy(NULL, slashArcTex[i]);
    for (int i = 0; i < 3; i++)
        if (snakebiteTex[i]) r2d_texture_destroy(NULL, snakebiteTex[i]);
    for (int i = 0; i < 4; i++)
        if (firesurgeTex[i]) r2d_texture_destroy(NULL, firesurgeTex[i]);
    if (flameTex)   r2d_texture_destroy(NULL, flameTex);

    if (glowTex)
        r2d_texture_destroy(NULL, glowTex);
    if (vignetteTex)
        r2d_texture_destroy(NULL, vignetteTex);

    minimap_quit();
    ui_theme_quit();
    music_quit();
    ambience_quit();
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