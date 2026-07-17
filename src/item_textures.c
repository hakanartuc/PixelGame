#include "item_textures.h"
#include "render.h"   // load_texture

void item_textures_load(ItemTextures *t, R2D *r)
{
    // hepsini NULL'la (yuklenemeyen / kusanilamaz slotlar NULL kalir)
    for (int i = 0; i < MAX_ITEM_TYPES; i++) {
        t->equipTex[i] = NULL;
        t->equipTexBack[i] = NULL;   // ← EKLE
        t->iconTex[i]  = NULL;
    }
    // ITEM_TYPES'i gez, her item kendi yolunu soyluyor.
    // Klamp: tablo tavanı aşarsa sessiz taşma yerine yüksek sesle logla
    if (ITEM_TYPE_COUNT > MAX_ITEM_TYPES)
        SDL_Log("UYARI: ITEM_TYPE_COUNT (%d) > MAX_ITEM_TYPES (%d) — tavani buyut!",
                ITEM_TYPE_COUNT, MAX_ITEM_TYPES);
    for (int i = 0; i < ITEM_TYPE_COUNT && i < MAX_ITEM_TYPES; i++) {
        const ItemType *it = &ITEM_TYPES[i];
        if (it->equipable && it->equipSpritePath != NULL) {
            t->equipTex[i] = load_texture(r, it->equipSpritePath);
            if (!t->equipTex[i])
                SDL_Log("UYARI: '%s' equip sprite yuklenemedi: %s", it->name, it->equipSpritePath);
        }
        if (it->equipable && it->equipSpritePathBack != NULL) {   // ← EKLE: arka katman
            t->equipTexBack[i] = load_texture(r, it->equipSpritePathBack);
            if (!t->equipTexBack[i])
                SDL_Log("UYARI: '%s' arka sprite yuklenemedi: %s", it->name, it->equipSpritePathBack);
        }
        if (it->iconSpritePath != NULL) {
            t->iconTex[i] = load_texture(r, it->iconSpritePath);
            if (!t->iconTex[i])
                SDL_Log("UYARI: '%s' ikon yuklenemedi: %s", it->name, it->iconSpritePath);
        }
    }
}

void item_textures_free(ItemTextures *t)
{
    for (int i = 0; i < MAX_ITEM_TYPES; i++) {
        if (t->equipTex[i])     { r2d_texture_destroy(NULL, t->equipTex[i]);     t->equipTex[i] = NULL; }
        if (t->equipTexBack[i]) { r2d_texture_destroy(NULL, t->equipTexBack[i]); t->equipTexBack[i] = NULL; }   // ← EKLE
        if (t->iconTex[i])      { r2d_texture_destroy(NULL, t->iconTex[i]);      t->iconTex[i] = NULL; }
    }
}

// type → ITEM_TYPES indeksi (pointer farki). Gecersizse -1.
static int item_index(const ItemType *type)
{
    if (type == NULL) return -1;
    int idx = (int)(type - ITEM_TYPES);
    if (idx < 0 || idx >= ITEM_TYPE_COUNT || idx >= MAX_ITEM_TYPES) return -1;
    return idx;
}

R2DTexture *item_equip_tex(const ItemTextures *t, const ItemType *type)
{
    int idx = item_index(type);
    return (idx < 0) ? NULL : t->equipTex[idx];
}

R2DTexture *item_icon_tex(const ItemTextures *t, const ItemType *type)
{
    int idx = item_index(type);
    return (idx < 0) ? NULL : t->iconTex[idx];
}

R2DTexture *item_equip_tex_back(const ItemTextures *t, const ItemType *type)
{
    int idx = item_index(type);
    return (idx < 0) ? NULL : t->equipTexBack[idx];
}