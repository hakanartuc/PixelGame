#include <SDL3/SDL.h>
#include "sneak.h"
#include "decor.h"

float sneak_speed_mult(int level)
{
    float m = 0.5f + (float)level * 0.002f;   // 100 seviyede 0.7
    return (m > 0.7f) ? 0.7f : m;
}

bool sneak_sight_blocked(float x1, float y1, float x2, float y2)
{
    // Çizgi boyunca örnekle: herhangi bir örnek dekor tabanına düşerse görüş yok.
    // (8 örnek ~ ev tabanı boyutları için yeterli sıklık)
    const int STEPS = 8;
    for (int s = 1; s < STEPS; s++) {
        float t = (float)s / STEPS;
        float sx = x1 + (x2 - x1) * t;
        float sy = y1 + (y2 - y1) * t;
        SDL_FRect probe = { sx - 2.0f, sy - 2.0f, 4.0f, 4.0f };
        if (decor_blocks(probe))
            return true;
    }
    return false;
}

// Mob bakış yönü vektörü (Entity.facing: 0=aşağı, 1=yukarı, 2=yan)
static void mob_facing_dir(const Mob *m, float *fx, float *fy)
{
    switch (m->e.facing) {
        case 0:  *fx = 0.0f;  *fy = 1.0f;  break;
        case 1:  *fx = 0.0f;  *fy = -1.0f; break;
        default: *fx = m->e.facingLeft ? -1.0f : 1.0f; *fy = 0.0f; break;
    }
}

float sneak_detect_mul(bool sneaking, int sneakLevel, bool inConceal,
                       float px, float py, const Mob *m)
{
    float mul = 1.0f;

    float mx = m->e.rect.x + m->e.rect.w / 2.0f;
    float my = m->e.rect.y + m->e.rect.h / 2.0f;

    // 1) Eğilme: taban 0.45, skill'le iyileşir (min 0.30)
    if (sneaking) {
        float crouch = 0.45f - (float)sneakLevel * 0.0015f;
        if (crouch < 0.30f) crouch = 0.30f;
        mul *= crouch;

        // 4) Örtü: gizleyen tile'ın İÇİNE eğilmek (ayakta üstünden görünürsün)
        if (inConceal)
            mul *= 0.6f;
    }

    // 2) Görüş açısı: mobun arkasındaysan yarı algı (duyma payı — sıfır değil)
    float fx, fy;
    mob_facing_dir(m, &fx, &fy);
    if (fx * (px - mx) + fy * (py - my) < 0.0f)
        mul *= 0.5f;

    // 3) Siper: ev tabanı araya giriyorsa yalnız duyar
    if (sneak_sight_blocked(px, py, mx, my))
        mul *= 0.4f;

    return mul;
}
