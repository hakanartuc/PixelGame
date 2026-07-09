#include <SDL3/SDL.h>
#include <stdlib.h> // rand, RAND_MAX
#include "common.h"
#include "tilemap.h"
#include "player.h"
#include "equipment.h" // ← EKLE: Equipment tam tanim + EQUIP_SLOT_COUNT + EquipSlot + slot_zorder
#include "item.h"      // ← EKLE: ItemStack + ITEM_TYPES

// 0.0..1.0 arasi rastgele float
static inline float randf(void)
{
    return (float)rand() / ((float)RAND_MAX + 1.0f);
}

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
};

// Silahin bir karedeki eldeki konumu
typedef struct
{
    float handX; // hucre icinde el X
    float handY; // hucre icinde el Y
    float angle; // silah acisi (0 = dik yukari)
} WeaponAnchor;

// SLASH animasyonu — DOWN (south) yonu, 6 kare.
// Kilic: yukari kalkar (0-1) → asagi-one savrulur (2-3) → biter (4-5)
// Bu degerler BASLANGIC TAHMINI — test edip ayarlayacagiz.
// SLASH anchor — [facing][frame]. facing: UP=0, LEFT=1, DOWN=2, RIGHT=3
// DOWN olculdu/ayarlandi. Digerleri tahmin — test edip ayarlanacak.
// SLASH anchor — [facing][frame]. facing: UP=0, LEFT=1, DOWN=2, RIGHT=3
// MANTIK: el SABIT (kilic elde durur), sadece ACI doner → sapin etrafinda savrulur.
static const WeaponAnchor SLASH_ANCHORS[4][6] = {
    // UP (0) — el sabit, kilic dar yay (yukari savurma)
    {
        {42.0f, 44.0f, 30.0f},
        {42.0f, 44.0f, 0.0f},
        {42.0f, 44.0f, -30.0f},
        {42.0f, 44.0f, -60.0f},
        {42.0f, 44.0f, -50.0f},
        {42.0f, 44.0f, -20.0f},
    },
    // LEFT (1) — el sabit, kilic sola dar yay
    {
        {23.0f, 44.0f, -110.0f},
        {23.0f, 44.0f, -140.0f},
        {23.0f, 44.0f, -180.0f},
        {23.0f, 44.0f, -210.0f},
        {23.0f, 44.0f, -200.0f},
        {23.0f, 44.0f, -160.0f},
    },
    // DOWN (2) — el sabit, kilic one dar yay
    {
        {20.0f, 45.0f, 20.0f},
        {20.0f, 45.0f, 50.0f},
        {20.0f, 45.0f, 90.0f},
        {20.0f, 45.0f, 120.0f},
        {20.0f, 45.0f, 110.0f},
        {20.0f, 45.0f, 70.0f},
    },
    // RIGHT (3) — el sabit, kilic saga dar yay
    {
        {28.0f, 45.0f, 110.0f},
        {28.0f, 45.0f, 140.0f},
        {28.0f, 45.0f, 180.0f},
        {28.0f, 45.0f, 210.0f},
        {28.0f, 45.0f, 200.0f},
        {28.0f, 45.0f, 160.0f},
    },
};

// Idle/walk icin yon-bazli silah durusu (kare yok — yone gore sabit)
// facing: UP=0, LEFT=1, DOWN=2, RIGHT=3
static const WeaponAnchor IDLE_ANCHORS[4] = {
    {42.0f, 44.0f, -20.0f}, // UP — kabza ele indi, kilic daha dik yukari
    {23.0f, 44.0f, -70.0f}, // LEFT — kilic biraz daha yatay/yukari
    {20.0f, 45.0f, -10.0f}, // DOWN — iyi (degismedi)
    {28.0f, 45.0f, 60.0f},  // RIGHT — iyi (hafif el ayari)
};

// Oyuncuyu çizer - sadece OKUR, değiştirmez.
void player_render(const Player *p, SDL_Renderer *renderer, const PlayerLayer *layers, int layerCount, const Camera *camera)
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

    for (int i = 0; i < layerCount; i++)
    {
        SDL_FRect layerSrc = src; // varsayilan: ortak src (state satiri + currentFrame)

        // Idle'da bu katmanin override'i varsa: farkli satir + sabit frame 0
        if (p->state == PLAYER_IDLE && layers[i].idleRowOverride >= 0)
        {
            int r2 = layers[i].idleRowOverride + (int)p->facing; // silah: walk row 8 + yon
            layerSrc.x = 0;                                      // frame 0 = ayakta durus
            layerSrc.y = r2 * FRAME_H;
        }

        SDL_RenderTexture(renderer, layers[i].tex, &layerSrc, &screen);
    }

#if DEBUG_DRAW
    SDL_FRect rectScreen = {p->rect.x - camera->x, p->rect.y - camera->y, p->rect.w, p->rect.h};
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderRect(renderer, &rectScreen);
    SDL_FRect fb = feet_box(p->rect.x, p->rect.y, p->rect.w, p->rect.h);
    SDL_FRect feetScreen = {fb.x - camera->x, fb.y - camera->y, fb.w, fb.h};
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_RenderRect(renderer, &feetScreen);
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
        p->speed = PLAYER_SPEED * TILE_TYPES[groundTile].speedMultiplier;
        if (TILE_TYPES[groundTile].damage > 0.0f)
            p->health -= TILE_TYPES[groundTile].damage * dt;
    }
    else
    {
        p->speed = PLAYER_SPEED;
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

        // X ekseni
        float newX = p->rect.x + dx * p->speed * dt;
        SDL_FRect fx = feet_box(newX, p->rect.y, p->rect.w, p->rect.h);
        if (!box_hits_solid(fx))
            p->rect.x = newX;

        // Y ekseni
        float newY = p->rect.y + dy * p->speed * dt;
        SDL_FRect fy = feet_box(p->rect.x, newY, p->rect.w, p->rect.h);
        if (!box_hits_solid(fy))
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

    // --- DURUM (state) belirleme ---
    if (p->attacking)
    {
        p->state = PLAYER_SLASH;
    }
    else if (moving)
    {
        p->state = PLAYER_WALK;
    }
    else
    {
        p->state = PLAYER_IDLE;
    }

    // --- Yeni saldırı başlat ---
    if (attackPressed && !p->attacking)
    {
        p->attacking = true;
        p->state = PLAYER_SLASH;
        p->currentFrame = 0;
        p->animTimer = 0.0f;
        p->attackHit = false;
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

    p->animTimer += dt;
    if (p->animTimer >= anim->frameDur)
    {
        p->animTimer -= anim->frameDur;
        p->currentFrame++;

        if (p->currentFrame >= anim->frameCount)
        {
            if (anim->loop)
            {
                p->currentFrame = 0; // döngüsel (idle/walk)
            }
            else
            {
                // tek-seferlik (slash) bitti
                p->currentFrame = anim->frameCount - 1;
                if (p->state == PLAYER_SLASH)
                {
                    p->attacking = false; // saldırı bitti
                }
            }
        }
    }

    // --- SILAH EFEKTI: karakter etrafinda partikul dogur ---
    if (p->weaponFx != WEAPON_FX_NONE)
    {
          // Karakter merkezi + yone gore kilic bolgesi ofseti
    float cx = p->rect.x + p->rect.w / 2.0f + WEAPON_FX_OFFSET[p->facing].dx;
    float cy = p->rect.y + p->rect.h / 2.0f + WEAPON_FX_OFFSET[p->facing].dy;

        int spawnPerFrame = 3;
        for (int k = 0; k < spawnPerFrame; k++)
        {
            // Dogum noktasi: kilic bolgesi civarinda dar sacilim (govde geneli degil)
    float ox = cx + (randf() - 0.5f) * 12.0f;
    float oy = cy + (randf() - 0.5f) * 24.0f;

            float vx, vy, life, size;
            Uint8 pr, pg, pb;

            if (p->weaponFx == WEAPON_FX_FLAME) {
                vx = (randf() - 0.5f) * 40.0f;
                vy = -60.0f - randf() * 60.0f;
                life = 0.3f + randf() * 0.3f;
                size = 8.0f + randf() * 10.0f;
                pr = 255; pg = (Uint8)(200 + rand() % 55); pb = (Uint8)(rand() % 80);
            } else {
                float angR = randf() * 6.2832f;
                float spd = 20.0f + randf() * 50.0f;
                vx = SDL_cosf(angR) * spd;
                vy = SDL_sinf(angR) * spd - 30.0f;
                life = 0.3f + randf() * 0.4f;
                size = 6.0f + randf() * 8.0f;
                pr = 255; pg = (Uint8)(40 + rand() % 60); pb = (Uint8)(180 + rand() % 50);
            }
            fx_spawn(fx, ox, oy, vx, vy, life, size, pr, pg, pb, p->weaponFx);
        }
    }
}

// Can barını ekranın köşesine çizer. Ekran-uzayına bağlıdır, kamera hareketinden etkilenmez.
void health_bar_render(const Player *p, SDL_Renderer *renderer)
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
    if (ratio < 0.0f)
        ratio = 0.0f;
    if (ratio > 1.0f)
        ratio = 1.0f;

    SDL_FRect fill = {barX, barY, barW * ratio, barH};  // genişlik oransal
    SDL_SetRenderDrawColor(renderer, 220, 50, 50, 255); // parlak kırmızı
    SDL_RenderFillRect(renderer, &fill);

    // 3) Çerçeve - barın kenarı
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &bg);
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
        SDL_Texture *texFront = item_equip_tex(itemTex, st->type);
        if (texFront != NULL && n < outCap)
        {
            out[n].tex = texFront;
            out[n].z = z;
            out[n].idleRowOverride = idleRow;
            n++;
        }

        // ARKA katman (varsa; z item verisinden — silahta body(10) altinda kalir)
        SDL_Texture *texBack = item_equip_tex_back(itemTex, st->type);
        if (texBack != NULL && n < outCap)
        {
            out[n].tex = texBack;
            out[n].z = st->type->equipZBack;
            out[n].idleRowOverride = idleRow;
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

// gripX/gripY: silah sprite'inda elin tuttugu nokta
// handX/handY: karakter hucresinde elin oldugu nokta
// angleDeg: silah acisi (0 = dik yukari)
// GECICI TEST: silahi karakterin eline anchor ile ciz
void weapon_render_test(Player *p, SDL_Renderer *renderer,
                        SDL_Texture *weaponTex, SDL_Texture *glowTex,
                        ParticleSystem *fx, const Camera *camera, bool backLayer)
{

    // Bu yon icin silah arkada mi cizilmeli?
    // UP (sirti donuk) ve LEFT (sag el arkada) → arka. DOWN/RIGHT → on.
    bool wantsBack = (p->facing == DIR_UP || p->facing == DIR_LEFT);

    // Parametre ile eslesmiyorsa bu cagrida cizme
    if (wantsBack != backLayer)
        return;

    // --- Silahin kendi grip noktasi (bu kilic icin olculdu) ---
    const float gripX = 31.0f;
    const float gripY = 54.0f;
    const int weaponW = 63;
    const int weaponH = 63;

    // --- El noktasi: state + kareye gore sec ---
    float handX, handY, angleDeg;

    if (p->state == PLAYER_SLASH)
    {
        int f = p->currentFrame;
        if (f < 0)
            f = 0;
        if (f > 5)
            f = 5;
        handX = SLASH_ANCHORS[p->facing][f].handX;
        handY = SLASH_ANCHORS[p->facing][f].handY;
        angleDeg = SLASH_ANCHORS[p->facing][f].angle;
    }
    else
    {
        // Idle/walk: yone gore durus
        handX = IDLE_ANCHORS[p->facing].handX;
        handY = IDLE_ANCHORS[p->facing].handY;
        angleDeg = IDLE_ANCHORS[p->facing].angle;
    }

    // Karakterin ekran konumu (player_render ile AYNI hesap + zoom)
    float zoom = camera->zoom;
    float scale = p->drawH / (float)FRAME_H;
    float footPadding = (FRAME_H - SPRITE_FEET_Y) * scale;

    float charWorldX = (p->rect.x + p->rect.w / 2.0f) - p->drawW / 2.0f;
    float charWorldY = (p->rect.y + p->rect.h) - p->drawH + footPadding;
    float charScreenX = (charWorldX - camera->x) * zoom;
    float charScreenY = (charWorldY - camera->y) * zoom;

    // El noktasinin ekran konumu (hucre-ici nokta * scale * zoom)
    float handScreenX = charScreenX + handX * scale * zoom;
    float handScreenY = charScreenY + handY * scale * zoom;

    // El noktasinin DUNYA konumu (partikul icin — kamera-bagimsiz)
    float handWorldX = charWorldX + handX * scale;
    float handWorldY = charWorldY + handY * scale;

    const float weaponScale = scale * 0.75f * zoom;

    SDL_FRect dst = {
        .x = handScreenX - gripX * weaponScale,
        .y = handScreenY - gripY * weaponScale,
        .w = weaponW * weaponScale,
        .h = weaponH * weaponScale};

    SDL_FPoint pivot = {gripX * weaponScale, gripY * weaponScale};

    // ========== PARTIKUL: silah efekt tipine gore zerre dogur ==========
    if (p->weaponFx != WEAPON_FX_NONE)
    {
        float radP = angleDeg * 3.14159265f / 180.0f;
        float pdirX = SDL_sinf(radP);
        float pdirY = -SDL_cosf(radP);
        float bladeWorldLen = gripY * scale;

        int spawnPerFrame = 3;
        for (int k = 0; k < spawnPerFrame; k++)
        {
            float frac = 0.4f + 0.6f * randf();
            float ox = handWorldX + pdirX * bladeWorldLen * frac;
            float oy = handWorldY + pdirY * bladeWorldLen * frac;

            float vx, vy, life, size;
            Uint8 pr, pg, pb;

            if (p->weaponFx == WEAPON_FX_FLAME)
            {
                // ALEV: yukari yukselir, sicak renk
                vx = (randf() - 0.5f) * 40.0f;
                vy = -60.0f - randf() * 60.0f;
                life = 0.3f + randf() * 0.3f;
                size = 8.0f + randf() * 10.0f;
                pr = 255;
                pg = (Uint8)(200 + rand() % 55);
                pb = (Uint8)(rand() % 80);
            }
            else
            {
                // PINK ENERGY: rastgele yon, hafif yukari
                float angR = randf() * 6.2832f;
                float spd = 20.0f + randf() * 50.0f;
                vx = SDL_cosf(angR) * spd;
                vy = SDL_sinf(angR) * spd - 30.0f;
                life = 0.3f + randf() * 0.4f;
                size = 6.0f + randf() * 8.0f;
                pr = 255;
                pg = (Uint8)(40 + rand() % 60);
                pb = (Uint8)(180 + rand() % 50);
            }

            fx_spawn(fx, ox, oy, vx, vy, life, size, pr, pg, pb, p->weaponFx);
        }
    }
    // ========== PARTIKUL SONU ==========

    // Asil kilici ciz
    SDL_RenderTextureRotated(renderer, weaponTex, NULL, &dst, angleDeg, &pivot, SDL_FLIP_NONE);

    // ========== AURA EFEKTI (kilic boyunca firtina/girdap) ==========
    // weaponFx NONE ise aura da CIZILMEZ
    if (glowTex != NULL && p->weaponFx != WEAPON_FX_NONE)
    {
        float t = SDL_GetTicks() / 1000.0f;
        float pulse = 0.5f + 0.5f * SDL_sinf(t * 4.0f);

        // Aura rengi efekt tipine gore
        Uint8 glowR, glowG, glowB;
        Uint8 sparkR, sparkG, sparkB;
        if (p->weaponFx == WEAPON_FX_FLAME)
        {
            glowR = 255;
            glowG = 140;
            glowB = 20; // turuncu aura
            sparkR = 255;
            sparkG = 220;
            sparkB = 40; // sari kivilcim
        }
        else
        {
            glowR = 255;
            glowG = 40;
            glowB = 180; // pembe aura
            sparkR = 255;
            sparkG = 20;
            sparkB = 140; // koyu pembe kivilcim
        }

        SDL_SetTextureBlendMode(glowTex, SDL_BLENDMODE_BLEND);

        float rad = angleDeg * 3.14159265f / 180.0f;
        float dirX = SDL_sinf(rad);
        float dirY = -SDL_cosf(rad);
        float perpX = -dirY;
        float perpY = dirX;

        float bladeLen = gripY * weaponScale;

        int points = 6;
        for (int i = 0; i < points; i++)
        {
            float frac = 0.45f + 0.55f * (float)i / (points - 1);

            float baseX = handScreenX + dirX * bladeLen * frac;
            float baseY = handScreenY + dirY * bladeLen * frac;

            float swirl = SDL_sinf(t * 6.0f + i * 1.3f) * (weaponW * weaponScale * 0.18f);
            float px = baseX + perpX * swirl;
            float py = baseY + perpY * swirl;

            float size = weaponW * weaponScale * (0.9f - frac * 0.2f) * (0.9f + pulse * 0.2f);
            Uint8 a = (Uint8)((95 + pulse * 50) * (1.0f - frac * 0.2f));

            SDL_SetTextureColorMod(glowTex, glowR, glowG, glowB);
            SDL_SetTextureAlphaMod(glowTex, a);

            SDL_FRect gdst = {
                .x = px - size * 0.5f,
                .y = py - size * 0.5f,
                .w = size,
                .h = size};
            SDL_RenderTexture(renderer, glowTex, NULL, &gdst);
        }

        // --- ICINDEKI PARILDAYAN NOKTACIKLAR ---
        int sparkCount = 8;
        for (int s = 0; s < sparkCount; s++)
        {
            float sFrac = 0.4f + 0.6f * ((s * 0.37f) - (int)(s * 0.37f));
            float sOff = SDL_sinf(s * 2.1f) * (weaponW * weaponScale * 0.15f);

            float twinkle = 0.5f + 0.5f * SDL_sinf(t * 8.0f + s * 1.7f);
            if (twinkle < 0.3f)
                continue;

            float driftFrac = sFrac + SDL_sinf(t * 2.0f + s) * 0.05f;
            float spx = handScreenX + dirX * bladeLen * driftFrac + perpX * sOff;
            float spy = handScreenY + dirY * bladeLen * driftFrac + perpY * sOff;

            float sparkSize = weaponW * weaponScale * (0.18f + twinkle * 0.12f);

            SDL_SetTextureColorMod(glowTex, sparkR, sparkG, sparkB);
            SDL_SetTextureAlphaMod(glowTex, (Uint8)(twinkle * 180));

            SDL_FRect sdst = {
                .x = spx - sparkSize * 0.5f,
                .y = spy - sparkSize * 0.5f,
                .w = sparkSize,
                .h = sparkSize};
            SDL_RenderTexture(renderer, glowTex, NULL, &sdst);
        }

        SDL_SetTextureColorMod(glowTex, 255, 255, 255);
        SDL_SetTextureAlphaMod(glowTex, 255);
        SDL_SetTextureBlendMode(glowTex, SDL_BLENDMODE_BLEND);
    }
    // ========== AURA SONU ==========

#if DEBUG_DRAW
    SDL_FRect handDot = {handScreenX - 2, handScreenY - 2, 4, 4};
    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
    SDL_RenderFillRect(renderer, &handDot);
#endif
}