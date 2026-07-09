#include <SDL3/SDL.h>
#include <stdlib.h>
#include "common.h"
#include "font.h"
#include "loot.h"
#include "item.h"
#include "item_textures.h"


#define CRATE_SCALE  1.2f

// Kasa açılma animasyonu clip'i (Crate_Anim.png: 6 kare, 48×48, 48px aralık)
// Şimdilik tek clip — kapalı=kare 0, açılma=kare 0→5.
static const AnimClip CRATE_CLIP = {
    .x = 0, .y = 0,
    .frameW = 48, .frameH = 48,
    .frameCount = 6,
    .frameDuration = 0.08f,
    .feetY = 33,   // kasa alt hizası (ölçtüğümüz son kare ~33)
};

// Kasanın dünyadaki görsel dikdörtgenini hesaplar (çizim + tıklama aynı kaynaktan).
static SDL_FRect loot_visual_rect(const Loot *l)
{
    const AnimClip *clip = &CRATE_CLIP;
    float scale = CRATE_SCALE;   // ← sabit yap (aşağıda)
    float drawW = clip->frameW * scale;
    float drawH = clip->frameH * scale;
    float feetOffset = clip->feetY * scale;

    SDL_FRect r = {
        .x = (l->rect.x + l->rect.w/2.0f) - drawW/2.0f,
        .y = (l->rect.y + l->rect.h) - feetOffset,
        .w = drawW,
        .h = drawH
    };
    return r;
}

// Plus roll: her kademe için %35 devam şansı — +0 sık, yüksek plus nadir.
// (0.35^3 ≈ %4 → +3; tavan hem tablodan hem ITEM_MAX_PLUS'tan)
static int roll_plus(int maxPlus)
{
    if (maxPlus > ITEM_MAX_PLUS) maxPlus = ITEM_MAX_PLUS;
    int plus = 0;
    while (plus < maxPlus && randf() < 0.35f)
        plus++;
    return plus;
}

bool loot_spawn(Loot *loots, int maxLoot, float x, float y,
                const LootEntry *table, int tableCount, int goldMin, int goldMax)
{
    // Boş slot bul
    int slot = -1;
    for (int i = 0; i < maxLoot; i++) {
        if (!loots[i].active) { slot = i; break; }
    }
    if (slot == -1) return false;   // doluysa düşürme

    // 1) Kasayı kur
    loots[slot] = (Loot){
        .rect = { .x = x, .y = y, .w = 48.0f, .h = 48.0f },
        .active = true,
        .state = LOOT_CLOSED,
        .currentFrame = 0,        // kapalı kasa karesi
        .animTimer = 0.0f,
    };

    // 2) İçeriği tablodan roll'la: her girdi bağımsız zar
    int n = 0;
    for (int k = 0; k < tableCount && n < MAX_LOOT_ITEMS; k++) {
        if (randf() >= table[k].chance) continue;   // düşmedi
        loots[slot].items[n].type = table[k].type;
        loots[slot].items[n].count = 1;
        loots[slot].items[n].plusLevel = roll_plus(table[k].maxPlus);
        n++;
    }
    loots[slot].itemCount = n;

    // 3) Altın (her zaman düşer — kasa asla tamamen boş olmaz)
    if (goldMax < goldMin) goldMax = goldMin;
    loots[slot].gold = goldMin + rand() % (goldMax - goldMin + 1);

    return true;
}

void loot_render_all(Loot *loots, int maxLoot, SDL_Renderer *renderer, SDL_Texture *crateTex, const Camera *camera)
{
    for (int i = 0; i < maxLoot; i++) {
        Loot *l = &loots[i];
        if (!l->active) continue;

        const AnimClip *clip = &CRATE_CLIP;

        // Kaynak: currentFrame'inci kare
        SDL_FRect src = {
            .x = clip->x + l->currentFrame * clip->frameW,
            .y = clip->y,
            .w = clip->frameW,
            .h = clip->frameH
        };

       float zoom = camera->zoom;
        SDL_FRect vr = loot_visual_rect(l);
        SDL_FRect screen = 
        {
            .x = (vr.x - camera->x) * zoom,
            .y = (vr.y - camera->y) * zoom,
            .w = vr.w * zoom,
            .h = vr.h * zoom
        };
        SDL_RenderTexture(renderer, crateTex, &src, &screen);
    }
}

int loot_find_at(Loot *loots, int maxLoot, float worldX, float worldY)
{
    for (int i = 0; i < maxLoot; i++) {
        Loot *l = &loots[i];
        if (!l->active) continue;
        // Nokta kasanın rect'i içinde mi?
        SDL_FRect vr = loot_visual_rect(l);
        if (worldX >= vr.x && worldX <= vr.x + vr.w && worldY >= vr.y && worldY <= vr.y + vr.h) 
        {
            return i;
        }
    }
    return -1;
}

void loot_update_all(Loot *loots, int maxLoot, float dt)
{
    for (int i = 0; i < maxLoot; i++) {
        Loot *l = &loots[i];
        if (!l->active) continue;

        if (l->state == LOOT_OPENING) {
            // Açılma: currentFrame ileri (0→5)
            l->animTimer += dt;
            if (l->animTimer >= CRATE_CLIP.frameDuration) {
                l->animTimer -= CRATE_CLIP.frameDuration;
                l->currentFrame++;
                if (l->currentFrame >= CRATE_CLIP.frameCount) {
                    l->currentFrame = CRATE_CLIP.frameCount - 1;
                    l->state = LOOT_OPEN;
                }
            }
        }
        else if (l->state == LOOT_CLOSING) {
            // Kapanma: currentFrame geri (5→0)
            l->animTimer += dt;
            if (l->animTimer >= CRATE_CLIP.frameDuration) {
                l->animTimer -= CRATE_CLIP.frameDuration;
                l->currentFrame--;
                if (l->currentFrame <= 0) {
                    l->currentFrame = 0;
                    l->state = LOOT_CLOSED;   // tamamen kapandı
                }
            }
        }
    }
}


void loot_popup_render(SDL_Renderer *renderer, const Loot *loot,
                       SDL_Texture *itemIconTex, const ItemTextures *itemTex,
                       TTF_Font *font, const Camera *camera)
{
    if (loot == NULL || loot->state != LOOT_OPEN) return;

    // Altın da bir hücre kaplar (item'lardan sonra, son hücre)
    int cells = loot->itemCount + (loot->gold > 0 ? 1 : 0);

    int cols = 3;
    float cellSize = 48.0f;
    float pad = 12.0f;
    int rows = (cells + cols - 1) / cols;
    if (rows < 1) rows = 1;

    float panelW = cols * cellSize + pad * 2;
    float panelH = rows * cellSize + pad * 2;

    // Panel konumu: kasanın dünya konumu − kamera (dünyaya sabit)
    // Kasanın biraz ÜSTÜNDE/yanında göster (kasanın üstüne çıksın)
    float zoom = camera->zoom;
    float panelX = ((loot->rect.x + loot->rect.w / 2.0f) - panelW / 2.0f - camera->x) * zoom;
    float panelY = (loot->rect.y - panelH - 10.0f - camera->y) * zoom;

    // Panel arka planı (boyutlar da zoom'lu)
    SDL_SetRenderDrawColor(renderer, 30, 30, 40, 235);
    SDL_FRect panel = { panelX, panelY, panelW * zoom, panelH * zoom };
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 200, 200, 210, 255);
    SDL_RenderRect(renderer, &panel);

    for (int k = 0; k < loot->itemCount; k++) {
        if (loot->items[k].type == NULL) continue;
        int col = k % cols, row = k / cols;
        float cx = panelX + (pad + col * cellSize) * zoom;
        float cy = panelY + (pad + row * cellSize) * zoom;

        SDL_SetRenderDrawColor(renderer, 60, 60, 75, 255);
        SDL_FRect cell = { cx, cy, (cellSize - 4) * zoom, (cellSize - 4) * zoom };
        SDL_RenderFillRect(renderer, &cell);

        const ItemType *it = loot->items[k].type;
        SDL_FRect dst = { cx + 6 * zoom, cy + 6 * zoom, 32 * zoom, 32 * zoom };

        SDL_Texture *icon = item_icon_tex(itemTex, it);
        if (icon != NULL) {
            SDL_RenderTexture(renderer, icon, &it->iconSrc, &dst);
        } else {
            SDL_FRect src = { it->iconIndex * 16.0f, 0, 16, 16 };
            SDL_RenderTexture(renderer, itemIconTex, &src, &dst);
        }

        // Plus seviyesi (envanterle aynı dil: sağ üst, camgöbeği)
        if (loot->items[k].plusLevel > 0 && font != NULL) {
            char buf[8];
            snprintf(buf, sizeof(buf), "+%d", loot->items[k].plusLevel);
            SDL_Color cyan = { 90, 220, 255, 255 };
            draw_text(renderer, font, buf, cx + (cellSize - 24) * zoom, cy, cyan);
        }
    }

    // Altın hücresi (son hücre): sarı kese + miktar
    if (loot->gold > 0) {
        int k = loot->itemCount;
        int col = k % cols, row = k / cols;
        float cx = panelX + (pad + col * cellSize) * zoom;
        float cy = panelY + (pad + row * cellSize) * zoom;

        SDL_SetRenderDrawColor(renderer, 60, 60, 75, 255);
        SDL_FRect cell = { cx, cy, (cellSize - 4) * zoom, (cellSize - 4) * zoom };
        SDL_RenderFillRect(renderer, &cell);

        // Kese: içi dolu altın karesi (ikon gelene kadar temsili)
        SDL_SetRenderDrawColor(renderer, 255, 210, 60, 255);
        SDL_FRect coin = { cx + 12 * zoom, cy + 8 * zoom, 20 * zoom, 20 * zoom };
        SDL_RenderFillRect(renderer, &coin);

        if (font != NULL) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", loot->gold);
            SDL_Color goldCol = { 255, 210, 60, 255 };
            draw_text(renderer, font, buf, cx + 4 * zoom, cy + (cellSize - 20) * zoom, goldCol);
        }
    }
}

// Popup'ta tıklanan item'in indeksini döndürür (-1 = hiçbiri).
// loot_popup_render ile AYNI konum hesabını kullanır (tutarlılık şart).
int loot_popup_item_at(const Loot *loot, float worldX, float worldY)
{
    if (loot == NULL || loot->state != LOOT_OPEN) return -1;

    // loot_popup_render ile AYNI hücre matematiği (altın dahil)
    int cells = loot->itemCount + (loot->gold > 0 ? 1 : 0);

    int cols = 3;
    float cellSize = 48.0f;
    float pad = 12.0f;
    int rows = (cells + cols - 1) / cols;
    if (rows < 1) rows = 1;

    float panelW = cols * cellSize + pad * 2;
    float panelH = rows * cellSize + pad * 2;
    // DÜNYA koordinatı (kamera yok — tıklama da dünya noktasıyla gelir)
    float panelX = (loot->rect.x + loot->rect.w / 2.0f) - panelW / 2.0f;
    float panelY = loot->rect.y - panelH - 10.0f;

    for (int k = 0; k < cells; k++) {
        bool isGoldCell = (k == loot->itemCount);
        if (!isGoldCell && loot->items[k].type == NULL) continue;
        int col = k % cols, row = k / cols;
        float cx = panelX + pad + col * cellSize;
        float cy = panelY + pad + row * cellSize;
        if (worldX >= cx && worldX <= cx + cellSize - 4 &&
            worldY >= cy && worldY <= cy + cellSize - 4) {
            return isGoldCell ? LOOT_CELL_GOLD : k;
        }
    }
    return -1;
}

// Kasada bir şey kaldı mı? (item'lar VE altın alındıysa boş)
bool loot_is_empty(const Loot *loot)
{
    if (loot->gold > 0) return false;
    for (int k = 0; k < loot->itemCount; k++) {
        if (loot->items[k].type != NULL) return false;
    }
    return true;
}