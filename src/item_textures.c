#include "item_textures.h"
#include "render.h"   // load_texture

void item_textures_load(ItemTextures *t, SDL_Renderer *r)
{
    // hepsini NULL'la (yuklenemeyen / kusanilamaz slotlar NULL kalir)
    for (int i = 0; i < MAX_ITEM_TYPES; i++) {
        t->equipTex[i] = NULL;
        t->iconTex[i]  = NULL;
    }
    // ITEM_TYPES'i gez, her item kendi yolunu soyluyor
    for (int i = 0; i < ITEM_TYPE_COUNT; i++) {
        const ItemType *it = &ITEM_TYPES[i];
        if (it->equipable && it->equipSpritePath != NULL)
            t->equipTex[i] = load_texture(r, it->equipSpritePath);
        // ikon ayri yukleniyor (paylasim YOK → free'de double-free riski yok)
        if (it->iconSpritePath != NULL)
            t->iconTex[i] = load_texture(r, it->iconSpritePath);
    }
}

void item_textures_free(ItemTextures *t)
{
    for (int i = 0; i < MAX_ITEM_TYPES; i++) {
        if (t->equipTex[i]) { SDL_DestroyTexture(t->equipTex[i]); t->equipTex[i] = NULL; }
        if (t->iconTex[i])  { SDL_DestroyTexture(t->iconTex[i]);  t->iconTex[i]  = NULL; }
    }
}

// type → ITEM_TYPES indeksi (pointer farki). Gecersizse -1.
static int item_index(const ItemType *type)
{
    if (type == NULL) return -1;
    int idx = (int)(type - ITEM_TYPES);
    if (idx < 0 || idx >= ITEM_TYPE_COUNT) return -1;
    return idx;
}

SDL_Texture *item_equip_tex(const ItemTextures *t, const ItemType *type)
{
    int idx = item_index(type);
    return (idx < 0) ? NULL : t->equipTex[idx];
}

SDL_Texture *item_icon_tex(const ItemTextures *t, const ItemType *type)
{
    int idx = item_index(type);
    return (idx < 0) ? NULL : t->iconTex[idx];
}