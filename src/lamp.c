#include "lamp.h"
#include "light.h"
#include "font.h"
#include <math.h>

#define T(n) ((n) * (float)TILE_SIZE)

// Köyün kandil hattı. Meydan direği SÖNÜK başlar — oyunun ilk gecesi bunun
// üstüne kurulu (LORE.md §13.2, Perde 1 düğüm 1). Han ve değirmen yolu yanık:
// oyuncu ilk on saniyede aynı ekranda yanan ve sönmüş direği birlikte görsün.
// graves = altında gömü var. Yalnız BOSTAN KENARI: köyün kendi mezarlığı
// oraya sırtını dayıyor (LORE §13.2 — ilk gece sahnesi bostanda geçer).
// Meydan sönük olsa da orada kimse kalkmaz; orası sadece karanlık kalır.
const LampDef LAMPS[] = {
    { .x = T(10), .y = T(8),  .place = "Meydan",        .startNights = 0, .graves = false },
    { .x = T(19), .y = T(6),  .place = "Han önü",       .startNights = 9, .graves = false },
    { .x = T(13), .y = T(10), .place = "Değirmen yolu", .startNights = 6, .graves = false },
    { .x = T(7),  .y = T(13), .place = "Bostan kenarı", .startNights = 0, .graves = true  },
};
const int LAMP_COUNT = (int)(sizeof(LAMPS) / sizeof(LAMPS[0]));

static int       s_nights[LAMP_MAX];
static LampOwner s_owner[LAMP_MAX];

const char *lamp_owner_name(LampOwner o)
{
    switch (o) {
        case LAMP_OWNER_PLAYER:  return "senin adına";
        case LAMP_OWNER_VILLAGE: return "köyün adına";
        case LAMP_OWNER_LORD:    return "vadi lordunun adına";
        case LAMP_OWNER_VEKIL:   return "yol vekilinin adına";
        case LAMP_OWNER_PYRE:    return "ölüyakanların adına";
        default:                 return "kimsenin adına";
    }
}

void lamp_reset(void)
{
    for (int i = 0; i < LAMP_MAX; i++) {
        s_nights[i] = (i < LAMP_COUNT) ? LAMPS[i].startNights : 0;
        s_owner[i]  = (i < LAMP_COUNT && LAMPS[i].startNights > 0)
                          ? LAMP_OWNER_VILLAGE : LAMP_OWNER_NONE;
    }
}

int lamp_nights(int i)
{
    if (i < 0 || i >= LAMP_MAX) return 0;
    return s_nights[i];
}

void lamp_set_nights(int i, int n)
{
    if (i < 0 || i >= LAMP_MAX) return;
    s_nights[i] = (n < 0) ? 0 : n;
}

LampOwner lamp_owner(int i)
{
    if (i < 0 || i >= LAMP_MAX) return LAMP_OWNER_NONE;
    return s_owner[i];
}

void lamp_set_owner(int i, LampOwner o)
{
    if (i < 0 || i >= LAMP_MAX) return;
    if (o < 0 || o >= LAMP_OWNER_COUNT) o = LAMP_OWNER_NONE;
    s_owner[i] = o;
}

bool lamp_lit(int i)
{
    return lamp_nights(i) > 0;
}

void lamp_add_kor(int i, int korCount)
{
    if (i < 0 || i >= LAMP_MAX || korCount <= 0) return;
    s_nights[i] += korCount * LAMP_NIGHTS_PER_KOR;
}

bool lamp_night_passed(void)
{
    bool wentOut = false;
    for (int i = 0; i < LAMP_COUNT; i++) {
        if (s_nights[i] <= 0) continue;
        s_nights[i]--;
        if (s_nights[i] == 0) {
            wentOut = true;
            s_owner[i] = LAMP_OWNER_NONE;   // sönen direk adını kaybeder
        }
    }
    return wentOut;
}

// Yakıta göre kısılan GERÇEK yarıçap. Görsel ışık da bastırma hesabı da
// BUNU kullanır — ayrı hesaplarsa sözleşme yakıta göre yön değiştirir
// (bir kez oldu: görsel ×0.65 kısılıyordu, bastırma ham 210'da kalıyordu).
static float lamp_radius(int i)
{
    int n = s_nights[i];
    if (n <= 0) return 0.0f;
    float fade = (n >= 3) ? 1.0f : (n == 2) ? 0.85f : 0.65f;
    return LAMP_RADIUS * fade;
}

// §1.5 — bu noktada kandil katkısı. light_atten() ile TEK eğri (light.h),
// ve dönen değer 0'dan büyükse oyuncu korunuyor demektir: eşik ayrı bir sayı
// değil, ışığın bittiği yerin kendisi (Don't Starve DARK_CUTOFF deseni).
float lamp_level_at(float x, float y)
{
    float best = 0.0f;
    for (int i = 0; i < LAMP_COUNT; i++) {
        if (!lamp_lit(i)) continue;
        float r = lamp_radius(i);
        float dx = x - LAMPS[i].x, dy = y - LAMPS[i].y;
        float a = light_atten(SDL_sqrtf(dx * dx + dy * dy), r);
        if (a > best) best = a;
    }
    return best;
}

int lamp_nearest(float px, float py)
{
    int   best  = -1;
    float bestD = LAMP_INTERACT_RANGE * LAMP_INTERACT_RANGE;
    for (int i = 0; i < LAMP_COUNT; i++) {
        float dx = px - LAMPS[i].x, dy = py - LAMPS[i].y;
        float d2 = dx * dx + dy * dy;
        if (d2 < bestD) { bestD = d2; best = i; }
    }
    return best;
}

int lamp_append_lights(R2DLight *out, int n, int cap, const Camera *cam,
                       int sw, int sh)
{
    for (int i = 0; i < LAMP_COUNT; i++) {
        if (!lamp_lit(i)) continue;
        // Yakıtı azalan direğin ışığı kısılır: sönmesi önceden görünsün.
        // (§9 — bir direğin notası ölümünden önce düşer; görsel karşılığı bu.)
        // Yarıçap lamp_radius()'tan gelir — bastırma hesabıyla AYNI kaynak.
        int n2 = s_nights[i];
        float fade = (n2 >= 3) ? 1.0f : (n2 == 2) ? 0.85f : 0.65f;
        // Kor SABİT yanar (LORE §1.9 "sürekli ve alçak perdeli"): flicker 0.03.
        // Kanon kuralı — seni koruyan şey durur, korumayan gezinir.
        n = light_append_point(out, n, cap, cam, sw, sh,
                               LAMPS[i].x, LAMPS[i].y,
                               lamp_radius(i),
                               1.00f, 0.72f, 0.46f,   // kor: beyaza doğru soluk
                               0.66f * fade, LK_STEADY);
    }
    return n;
}

void lamp_render(R2D *renderer, const Camera *camera, TTF_Font *font,
                 int nearestIdx, float timeSec)
{
    float z = camera->zoom;
    for (int i = 0; i < LAMP_COUNT; i++) {
        float sx = (LAMPS[i].x - camera->x) * z;
        float sy = (LAMPS[i].y - camera->y) * z;

        // Direk gövdesi: ince, uzun. Sprite gelene kadar çizim.
        float w = 5.0f * z, h = 34.0f * z;
        SDL_FRect post = { sx - w * 0.5f, sy - h, w, h };
        r2d_set_color(renderer, 46, 38, 32, 255);
        r2d_fill_rect(renderer, &post);

        // Hazne — yanıkta sıcak ve titrek, sönükte kurşun grisi
        float capW = 11.0f * z, capH = 9.0f * z;
        SDL_FRect cap = { sx - capW * 0.5f, sy - h - capH * 0.6f, capW, capH };
        if (lamp_lit(i)) {
            float pulse = 0.85f + 0.15f * SDL_sinf(timeSec * 6.0f + (float)i * 2.1f);
            r2d_set_color(renderer, (Uint8)(255.0f * pulse),
                          (Uint8)(170.0f * pulse), 70, 255);
        } else {
            r2d_set_color(renderer, 88, 88, 94, 255);
        }
        r2d_fill_rect(renderer, &cap);

        if (i == nearestIdx && font != NULL) {
            char buf[96];
            if (lamp_lit(i))
                SDL_snprintf(buf, sizeof(buf), "%s - %d gece (%s)",
                             LAMPS[i].place, s_nights[i], lamp_owner_name(s_owner[i]));
            else
                SDL_snprintf(buf, sizeof(buf), "%s - sonuk  [E] kor koy",
                             LAMPS[i].place);
            SDL_Color c = { 235, 225, 210, 255 };
            draw_text(renderer, font, buf, sx - 70.0f, sy - h - 26.0f * z, c);
        }
    }
}
