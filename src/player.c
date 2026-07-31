#include <SDL3/SDL.h>
#include <stdlib.h> // rand, RAND_MAX
#include "common.h"
#include "tilemap.h"
#include "decor.h"   // ev tabanları katı (hareket kontrolü)
#include "player.h"
#include "equipment.h" // ← EKLE: Equipment tam tanim + EQUIP_SLOT_COUNT + EquipSlot + slot_zorder
#include "item.h"      // ← EKLE: ItemStack + ITEM_TYPES
#include "font.h"      // HUD bar değer yazıları (draw_text)
#include "uilayout.h"  // can/mana/stamina barlarının konumu (layout.json)
#include <stdio.h>     // snprintf (bar değerleri)

// 0.0..1.0 arasi rastgele float
// LPC animasyon tanımı: her durum için başlangıç satırı + kare sayısı
// Yön, başlangıç satırına eklenir (UP=0, LEFT=1, DOWN=2, RIGHT=3)
typedef struct
{
    int startRow;   // animasyonun ilk yönünün (UP) satırı
    int frameCount; // kaç kare
    float frameDur; // kare süresi
    bool loop;      // döngüsel mi (idle/walk) yoksa tek-seferlik mi (slash)
} LpcAnim;

// PlayerState sırasıyla: IDLE, WALK, SLASH
static const LpcAnim PLAYER_ANIMS[] = {
    [PLAYER_IDLE] = {.startRow = 22, .frameCount = 2, .frameDur = 0.4f, .loop = true},
    [PLAYER_WALK] = {.startRow = 8, .frameCount = 9, .frameDur = 0.08f, .loop = true}, // satır 8-11
    [PLAYER_SLASH] = {.startRow = 12, .frameCount = 6, .frameDur = 0.08f, .loop = false},
    [PLAYER_CAST] = {.startRow = 0, .frameCount = 7, .frameDur = 0.12f, .loop = true}, // spellcast 0-3
    [PLAYER_SHOOT] = {.startRow = 16, .frameCount = 13, .frameDur = 0.07f, .loop = false}, // yay 16-19 (~0.9sn)
    [PLAYER_THRUST] = {.startRow = 4, .frameCount = 8, .frameDur = 0.08f, .loop = false},   // asa dürtmesi 4-7
};

// Oyuncuyu çizer - sadece OKUR, değiştirmez.
void player_render(const Player *p, R2D *renderer, const PlayerLayer *layers, int layerCount, const Camera *camera)
{
    const LpcAnim *anim = &PLAYER_ANIMS[p->state];
    int row = anim->startRow + (int)p->facing; // durum satırı + yön

    SDL_FRect src = {
        .x = p->currentFrame * FRAME_W,
        .y = row * FRAME_H,
        .w = FRAME_W,
        .h = FRAME_H};

    // Ayak hizalama (LPC: ayak hücrede SPRITE_FEET_Y'de)
    float zoom = camera->zoom;
    float scale = p->drawH / (float)FRAME_H;
    float footPadding = (FRAME_H - SPRITE_FEET_Y) * scale;

    // Once dunya-uzayinda sol-ust kose, sonra zoom ile ekrana
    float worldX = (p->rect.x + p->rect.w / 2.0f) - p->drawW / 2.0f;
    float worldY = (p->rect.y + p->rect.h) - p->drawH + footPadding;
    SDL_FRect screen = {
        .x = (worldX - camera->x) * zoom,
        .y = (worldY - camera->y) * zoom,
        .w = p->drawW * zoom,
        .h = p->drawH * zoom};

    // Egilirken karakter yari saydam (gizlilik geri bildirimi)
    Uint8 sneakAlpha = p->sneaking ? 160 : 255;

    for (int i = 0; i < layerCount; i++)
    {
        // Arac katmani (smash128) yalniz savurma sirasinda gorunur:
        // sheet'te walk/idle satiri YOK, baska state'te cizmek copluk basar
        if (layers[i].smash128 && p->state != PLAYER_SLASH)
            continue;

        SDL_FRect layerSrc = src;    // varsayilan: ortak src (state satiri + currentFrame)
        SDL_FRect layerDst = screen; // varsayilan: ortak dst
        r2d_texture_set_alpha_mod(layers[i].tex, sneakAlpha);

        // Idle'da bu katmanin override'i varsa: farkli satir + sabit frame 0
        if (p->state == PLAYER_IDLE && layers[i].idleRowOverride >= 0)
        {
            int r2 = layers[i].idleRowOverride + (int)p->facing; // silah: walk row 8 + yon
            layerSrc.x = 0;                                      // frame 0 = ayakta durus
            layerSrc.y = r2 * FRAME_H;
        }

        // Arac (smash128): kendi 128 grid'i — satir = yon, 2x merkezli dst
        if (p->state == PLAYER_SLASH && layers[i].smash128)
        {
            layerSrc = (SDL_FRect){
                .x = p->currentFrame * SMASH_FRAME,
                .y = (int)p->facing * SMASH_FRAME,
                .w = SMASH_FRAME,
                .h = SMASH_FRAME};
            layerDst.x -= screen.w / 2.0f;
            layerDst.y -= screen.h / 2.0f;
            layerDst.w = screen.w * 2.0f;   // 128/64
            layerDst.h = screen.h * 2.0f;
        }

        // Mizrak katmani: kompozit sheet — thrust'ta oversize bloktan (uzun
        // erisim gorunur), diger state'lerde standart satirdan (yuruyus vs.)
        if (p->state == PLAYER_THRUST && layers[i].thrustOversize)
        {
            layerSrc = (SDL_FRect){
                .x = p->currentFrame * OVERSIZE_FRAME,
                .y = THRUST_OVERSIZE_Y + (int)p->facing * OVERSIZE_FRAME,
                .w = OVERSIZE_FRAME,
                .h = OVERSIZE_FRAME};
            layerDst.x -= screen.w;
            layerDst.y -= screen.h;
            layerDst.w = screen.w * 3.0f;
            layerDst.h = screen.h * 3.0f;
        }

        // Slash'ta oversize katman: 192px blok + 3x buyutulmus merkezli dst
        // (karakter oversize hucrenin merkezindeki 64px kutuda → offset sabit)
        if (p->state == PLAYER_SLASH && layers[i].slashOversize)
        {
            layerSrc = (SDL_FRect){
                .x = p->currentFrame * OVERSIZE_FRAME,
                .y = OVERSIZE_Y + (int)p->facing * OVERSIZE_FRAME,
                .w = OVERSIZE_FRAME,
                .h = OVERSIZE_FRAME};
            layerDst.x -= screen.w;
            layerDst.y -= screen.h;
            layerDst.w = screen.w * 3.0f; // 192/64
            layerDst.h = screen.h * 3.0f;
        }

        r2d_draw_texture(renderer, layers[i].tex, &layerSrc, &layerDst);
        r2d_texture_set_alpha_mod(layers[i].tex, 255);   // paylasilan texture — geri al
    }

#if DEBUG_DRAW
    SDL_FRect rectScreen = {p->rect.x - camera->x, p->rect.y - camera->y, p->rect.w, p->rect.h};
    r2d_set_color(renderer, 255, 0, 0, 255);
    r2d_rect(renderer, &rectScreen);
    SDL_FRect fb = feet_box(p->rect.x, p->rect.y, p->rect.w, p->rect.h);
    SDL_FRect feetScreen = {fb.x - camera->x, fb.y - camera->y, fb.w, fb.h};
    r2d_set_color(renderer, 255, 255, 0, 255);
    r2d_rect(renderer, &feetScreen);
#endif
}

// Oyuncuyu günceller
void player_update(Player *p, const bool *keys, bool attackPressed, float dt, ParticleSystem *fx)
{
    // --- TILE TEPKİSİ (aynı) ---
    SDL_FRect fb = feet_box(p->rect.x, p->rect.y, p->rect.w, p->rect.h);
    int groundTile = tile_at_point(fb.x + fb.w / 2.0f, fb.y + fb.h / 2.0f);
    if (groundTile >= 0 && groundTile < TILE_TYPE_COUNT)
    {
        p->speed = PLAYER_SPEED * TILE_TYPES[groundTile].speedMultiplier * p->speedMult;
        if (TILE_TYPES[groundTile].damage > 0.0f)
            p->health -= TILE_TYPES[groundTile].damage * dt;
    }
    else
    {
        p->speed = PLAYER_SPEED * p->speedMult;
    }

    if (p->health <= 0.0f)
    {
        p->rect.x = SPAWN_X;
        p->rect.y = SPAWN_Y;
        p->health = p->maxHealth;
        p->attacking = false;
    }

    // --- YÖN: WASD'den hareket vektörü ---
    float dx = 0.0f, dy = 0.0f;
    if (keys[SDL_SCANCODE_W])
        dy -= 1;
    if (keys[SDL_SCANCODE_S])
        dy += 1;
    if (keys[SDL_SCANCODE_A])
        dx -= 1;
    if (keys[SDL_SCANCODE_D])
        dx += 1;

    bool moving = (dx != 0.0f || dy != 0.0f);

    // Normalize + hareket
    if (moving)
    {
        float len = SDL_sqrtf(dx * dx + dy * dy);
        dx /= len;
        dy /= len;

        // X ekseni (tile + dekor/ev tabanı katı)
        float newX = p->rect.x + dx * p->speed * dt;
        SDL_FRect fx = feet_box(newX, p->rect.y, p->rect.w, p->rect.h);
        if (!box_hits_solid(fx) && !decor_blocks(fx))
            p->rect.x = newX;

        // Y ekseni
        float newY = p->rect.y + dy * p->speed * dt;
        SDL_FRect fy = feet_box(p->rect.x, newY, p->rect.w, p->rect.h);
        if (!box_hits_solid(fy) && !decor_blocks(fy))
            p->rect.y = newY;

        // YÖN belirleme (4 yön — saldırı sırasında değiştirme)
        if (!p->attacking)
        {
            // Baskın eksene göre yön (LPC: 4 ayrı yön)
            if (SDL_fabsf(dx) > SDL_fabsf(dy))
            {
                p->facing = (dx < 0) ? DIR_LEFT : DIR_RIGHT;
            }
            else
            {
                p->facing = (dy < 0) ? DIR_UP : DIR_DOWN;
            }
        }
    }

    // Köklü atış: çekme sürerken hareket atışı İPTAL eder (ok tüketilmez)
    if (p->shooting && moving)
        p->shooting = false;

    // --- DURUM (state) belirleme ---
    if (p->attacking)
    {
        p->state = (p->staffEquipped || p->spearEquipped)
                 ? PLAYER_THRUST : PLAYER_SLASH;
    }
    else if (p->shooting)
    {
        p->state = PLAYER_SHOOT;
    }
    else if (moving)
    {
        p->state = PLAYER_WALK;
    }
    else if (p->casting)
    {
        p->state = PLAYER_CAST;   // kaynatma/karistirma (main yazar)
    }
    else
    {
        p->state = PLAYER_IDLE;
    }

    // --- Yeni saldırı başlat: yay kuşanılıysa çekme, değilse savurma ---
    if (attackPressed && !p->attacking && !p->shooting)
    {
        if (p->bowEquipped)
        {
            p->shooting = true;
            p->state = PLAYER_SHOOT;
            p->shotReleased = false;
        }
        else
        {
            p->attacking = true;
            p->state = (p->staffEquipped || p->spearEquipped)
                 ? PLAYER_THRUST : PLAYER_SLASH;
            p->attackHit = false;
        }
        p->currentFrame = 0;
        p->animTimer = 0.0f;
    }

    // --- ANİMASYON ilerlet ---
    const LpcAnim *anim = &PLAYER_ANIMS[p->state];

    // Geçiş: durum ya da yön değişti mi → kareyi sıfırla
    if (p->state != p->lastState || p->facing != p->lastFacing)
    {
        p->currentFrame = 0;
        p->animTimer = 0.0f;
        p->lastState = p->state;
        p->lastFacing = p->facing;
    }

    // Çekme hızı düğümleri: yalnız SHOOT karelerini ölçekler (Akıncı/Keskin Nişancı)
    float frameDur = anim->frameDur;
    if (p->state == PLAYER_SHOOT && p->drawSpeedMult > 0.0f)
        frameDur *= p->drawSpeedMult;

    p->animTimer += dt;
    if (p->animTimer >= frameDur)
    {
        p->animTimer -= frameDur;
        p->currentFrame++;

        if (p->currentFrame >= anim->frameCount)
        {
            if (anim->loop)
            {
                p->currentFrame = 0; // döngüsel (idle/walk)
            }
            else
            {
                // tek-seferlik (slash/thrust/shoot) bitti
                p->currentFrame = anim->frameCount - 1;
                if (p->state == PLAYER_SLASH || p->state == PLAYER_THRUST)
                {
                    p->attacking = false; // saldırı bitti
                }
                else if (p->state == PLAYER_SHOOT)
                {
                    p->shooting = false;  // çekme bitti (ok release karesinde çıktı)
                }
            }
        }
    }

    // --- SILAH EFEKTI: karakter etrafinda partikul dogur ---
    if (p->weaponFx != WEAPON_FX_NONE)
    {
        // Yone gore kilic bolgesi ofseti; slash'ta kare-kare savurma konumu
        float dx = WEAPON_FX_OFFSET[p->facing].dx;
        float dy = WEAPON_FX_OFFSET[p->facing].dy;
        if (p->state == PLAYER_SLASH)
        {
            float s = p->drawH / (float)FRAME_H; // sprite px → dunya px
            dx = WEAPON_SLASH_FX_OFFSET[p->facing][p->currentFrame].dx * s;
            dy = WEAPON_SLASH_FX_OFFSET[p->facing][p->currentFrame].dy * s;
        }

    float cx = p->rect.x + p->rect.w / 2.0f + dx;
    float cy = p->rect.y + p->rect.h / 2.0f + dy;

        int spawnPerFrame = (p->weaponFx == WEAPON_FX_ICE) ? 6 : 3; // firtina daha yogun
        for (int k = 0; k < spawnPerFrame; k++)
        {
            // Dogum noktasi: kilic bolgesi civarinda dar sacilim (govde geneli degil)
    float ox = cx + (randf() - 0.5f) * 12.0f;
    float oy = cy + (randf() - 0.5f) * 24.0f;

            float vx, vy, life, size;
            Uint8 pr, pg, pb;

            switch (p->weaponFx) {
            case WEAPON_FX_FLAME: // yukari savrulan alev dilleri
                vx = (randf() - 0.5f) * 40.0f;
                vy = -60.0f - randf() * 60.0f;
                life = 0.3f + randf() * 0.3f;
                size = 8.0f + randf() * 10.0f;
                pr = 255; pg = (Uint8)(200 + rand() % 55); pb = (Uint8)(rand() % 80);
                break;
            case WEAPON_FX_ICE: // silah cevresinde donen buz firtinasi
            {
                // Silah-boyu sacilimin (ox/oy) ustune kucuk cember: girdap silah boyunca dagilir
                float ang = randf() * 6.2832f;
                float rad = 3.0f + randf() * 5.0f;
                ox += SDL_cosf(ang) * rad;
                oy += SDL_sinf(ang) * rad * 0.6f;
                float spin = 28.0f + randf() * 18.0f;
                vx = -SDL_sinf(ang) * spin;
                vy = SDL_cosf(ang) * spin * 0.6f + 10.0f;
                life = 0.25f + randf() * 0.25f;
                size = 5.0f + randf() * 7.0f;
                if (rand() % 2) { // beyaz kristal / mavi yelpaze — yari yariya
                    pr = 255; pg = 255; pb = 255;
                } else {
                    float tone = randf(); // 0 = koyu buz mavisi, 1 = acik mavi
                    pr = (Uint8)(60 + tone * 120.0f);
                    pg = (Uint8)(140 + tone * 90.0f);
                    pb = 255;
                }
                break;
            }
            case WEAPON_FX_POISON: // yavas yukselen zehir kabarciklari
            {
                vx = (randf() - 0.5f) * 25.0f;
                vy = -25.0f - randf() * 35.0f;
                life = 0.4f + randf() * 0.4f;
                size = 6.0f + randf() * 8.0f;
                float tone = randf(); // uc ayri ton grubu — koyu / toksik / sari-yesil
                if (tone < 0.33f)      { pr = (Uint8)(25 + rand() % 20);  pg = (Uint8)(110 + rand() % 40); pb = 30; }
                else if (tone < 0.66f) { pr = (Uint8)(50 + rand() % 30);  pg = (Uint8)(190 + rand() % 50); pb = 50; }
                else                   { pr = (Uint8)(140 + rand() % 40); pg = 255;                        pb = 60; }
                break;
            }
            default: // WEAPON_FX_PINK — her yone sacilan enerji
            {
                float angR = randf() * 6.2832f;
                float spd = 20.0f + randf() * 50.0f;
                vx = SDL_cosf(angR) * spd;
                vy = SDL_sinf(angR) * spd - 30.0f;
                life = 0.3f + randf() * 0.4f;
                size = 6.0f + randf() * 8.0f;
                pr = 255; pg = (Uint8)(40 + rand() % 60); pb = (Uint8)(180 + rand() % 50);
                break;
            }
            }
            fx_spawn(fx, ox, oy, vx, vy, life, size, pr, pg, pb, p->weaponFx);
        }
    }
}

// Can barını ekranın köşesine çizer. Ekran-uzayına bağlıdır, kamera hareketinden etkilenmez.
void health_bar_render(const Player *p, R2D *renderer, TTF_Font *font)
{
    // Bar ölçüleri (ekran pikseli - doğrudan, kamera yok).
    // GENİŞLİK max değerle büyür: takı/stat taşı takınca bar UZAR (görünür
    // geri bildirim — Hakan isteği); taban 200px, tavan 340px.
    // Konum yerleşim dosyasından ("can_bar"). GENİŞLİK veriden GELMİYOR: max
    // değerle büyümesi kasıtlı bir geri bildirim, onu dosyaya çivilemek özelliği
    // öldürürdü. Yerleşim nereye, kod ne kadar geniş.
    float barX, barY;
    uilayout_get("can_bar", 20.0f, 20.0f, &barX, &barY);
    float barW = 200.0f + (p->maxHealth - 100.0f) * 0.6f;
    float barH = 24.0f;
    if (barW < 200.0f) barW = 200.0f;
    if (barW > 340.0f) barW = 340.0f;

    // 1) Arka plan (boş bar) - koy kırmızı/gri
    SDL_FRect bg = {barX, barY, barW, barH};
    r2d_set_color(renderer, 60, 20, 20, 255);
    r2d_fill_rect(renderer, &bg);

    // 2) Dolu kısım- cana oranlı genişlik
    float ratio = p->health / p->maxHealth; // 0..1
    // ratio'yu 0..1 arasında sıkıştır (can negatif ya da max'tan büyük olamaz)
    if (ratio < 0.0f)
        ratio = 0.0f;
    if (ratio > 1.0f)
        ratio = 1.0f;

    SDL_FRect fill = {barX, barY, barW * ratio, barH};  // genişlik oransal
    r2d_set_color(renderer, 220, 50, 50, 255); // parlak kırmızı
    r2d_fill_rect(renderer, &fill);

    // 3) Çerçeve - barın kenarı
    r2d_set_color(renderer, 255, 255, 255, 255);
    r2d_rect(renderer, &bg);

    // 3b) Sayısal değer: "87 / 120" bar içinde ortalanır
    if (font != NULL) {
        char buf[24];
        snprintf(buf, sizeof(buf), "%d / %d", (int)p->health, (int)p->maxHealth);
        int tw = 0, th = 0;
        TTF_GetStringSize(font, buf, 0, &tw, &th);
        SDL_Color white = { 255, 255, 255, 255 };
        draw_text(renderer, font, buf,
                  barX + (barW - (float)tw) / 2.0f,
                  barY + (barH - (float)th) / 2.0f, white);
    }

    // 4) Mana (mavi) + Stamina (sari) — ince barlar HP'nin altinda.
    //    Tuketen sistemler gelecekte; stat basma havuzlari buyutecek (Faz 4)
    float mRatio = (p->maxMana > 0.0f) ? p->mana / p->maxMana : 0.0f;
    float sRatio = (p->maxStamina > 0.0f) ? p->stamina / p->maxStamina : 0.0f;
    if (mRatio > 1.0f) mRatio = 1.0f;
    if (sRatio > 1.0f) sRatio = 1.0f;

    float mBarW = 200.0f + (p->maxMana - 50.0f) * 0.6f;
    if (mBarW < 120.0f) mBarW = 120.0f;
    if (mBarW > 340.0f) mBarW = 340.0f;
    // Mana AYRI öğe: varsayılanı can barının altı, ama bağımsız taşınabilir
    float mX, mY;
    uilayout_get("mana_bar", barX, barY + barH + 4.0f, &mX, &mY);
    SDL_FRect mBg = { mX, mY, mBarW, 8.0f };
    r2d_set_color(renderer, 20, 25, 55, 255);
    r2d_fill_rect(renderer, &mBg);
    // M4: dost rezervi — barın sonu KİLİTLİ bölme (mor); dolum kalan bölgeye
    float usableW = mBarW * (1.0f - p->manaReservedFrac);
    SDL_FRect mFill = { mBg.x, mBg.y, usableW * mRatio, mBg.h };
    r2d_set_color(renderer, 70, 120, 255, 255);
    r2d_fill_rect(renderer, &mFill);
    if (p->manaReservedFrac > 0.0f) {
        SDL_FRect mLock = { mBg.x + usableW, mBg.y, mBarW - usableW, mBg.h };
        r2d_set_color(renderer, 110, 70, 160, 255);   // öz bağının rengi
        r2d_fill_rect(renderer, &mLock);
    }

    // Mana değeri: barın sağında küçük yazı
    if (font != NULL) {
        char mbuf[24];
        snprintf(mbuf, sizeof(mbuf), "%d/%d", (int)p->mana, (int)p->maxMana);
        SDL_Color mcol = { 150, 185, 255, 255 };
        draw_text(renderer, font, mbuf, mBg.x + mBarW + 6.0f, mBg.y - 4.0f, mcol);
    }

    float sBarW = 200.0f + (p->maxStamina - 100.0f) * 0.6f;
    if (sBarW < 120.0f) sBarW = 120.0f;
    if (sBarW > 340.0f) sBarW = 340.0f;
    // Stamina de ayrı öğe: varsayılanı mananın altı
    float sX, sY;
    uilayout_get("stam_bar", mBg.x, mBg.y + 12.0f, &sX, &sY);
    SDL_FRect sBg = { sX, sY, sBarW, 8.0f };
    r2d_set_color(renderer, 50, 45, 15, 255);
    r2d_fill_rect(renderer, &sBg);
    SDL_FRect sFill = { sBg.x, sBg.y, sBarW * sRatio, sBg.h };
    r2d_set_color(renderer, 235, 200, 60, 255);
    r2d_fill_rect(renderer, &sFill);
    // Stamina değeri: barın sağında küçük yazı
    if (font != NULL) {
        char sbuf[24];
        snprintf(sbuf, sizeof(sbuf), "%d/%d", (int)p->stamina, (int)p->maxStamina);
        SDL_Color scol = { 235, 210, 130, 255 };
        draw_text(renderer, font, sbuf, sBg.x + sBarW + 6.0f, sBg.y - 4.0f, scol);
    }
}

int build_player_layers(const PlayerLayer *base, int baseCount,
                        const Equipment *eq, const ItemTextures *itemTex,
                        PlayerLayer *out, int outCap)
{
    int n = 0;

    // 1) Base katmanlar (body/head/face) — oldugu gibi kopyala
    for (int i = 0; i < baseCount; i++)
    {
        if (n < outCap)
            out[n++] = base[i];
    }

    // 2) Giyili ekipman slotlari → katman
    for (int s = 0; s < EQUIP_SLOT_COUNT; s++)
    {
        const ItemStack *st = &eq->slots[s];
        if (st->type == NULL)
            continue;

        // z ve idle satiri item verisinden; equipZ=0 → slot varsayilani
        int z = (st->type->equipZ > 0) ? st->type->equipZ : slot_zorder((EquipSlot)s);
        int idleRow = (st->type->equipIdleRow > 0) ? st->type->equipIdleRow : -1;

        // ON katman (normal ekipman + silah onu)
        R2DTexture *texFront = item_equip_tex(itemTex, st->type);
        bool isTool = (st->type->weaponType == WEAPON_TOOL);   // smash128 katmani

        if (texFront != NULL && n < outCap)
        {
            out[n].tex = texFront;
            out[n].z = z;
            out[n].idleRowOverride = idleRow;
            out[n].slashOversize = st->type->slashOversize;
            out[n].smash128 = isTool;
            out[n].thrustOversize = st->type->thrustOversize;
            n++;
        }

        // ARKA katman (varsa; z item verisinden — silahta body(10) altinda kalir)
        R2DTexture *texBack = item_equip_tex_back(itemTex, st->type);
        if (texBack != NULL && n < outCap)
        {
            out[n].tex = texBack;
            out[n].z = st->type->equipZBack;
            out[n].idleRowOverride = idleRow;
            out[n].slashOversize = st->type->slashOversize;
            out[n].smash128 = isTool;
            out[n].thrustOversize = st->type->thrustOversize;
            n++;
        }
    }

    // 3) z'ye gore artan sirala (insertion sort — n kucuk, basit, stabil)
    //    '>' kullaniyoruz ('>=' degil) → esit z'lerde sira korunur (omuz parcalari icin onemli)
    for (int i = 1; i < n; i++)
    {
        PlayerLayer key = out[i];
        int j = i - 1;
        while (j >= 0 && out[j].z > key.z)
        {
            out[j + 1] = out[j];
            j--;
        }
        out[j + 1] = key;
    }

    return n;
}

