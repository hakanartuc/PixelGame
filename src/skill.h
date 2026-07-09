#ifndef SKILL_H
#define SKILL_H
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

// Use-based skill sistemi (Skyrim tarzı): sınıf yok, stat dağıtımı yok.
// Skill KULLANARAK gelişir — hasar verince One-Handed, hasar yerken zırh skill'i vb.

// Skill kategorileri. Hepsi baştan tanımlı; henüz XP kaynağı olmayanlar
// (Sneak, Alchemy...) panelde görünür ama level 1'de bekler.
typedef enum {
    SKILL_ONE_HANDED,     // tek el silah
    SKILL_TWO_HANDED,     // çift el silah
    SKILL_ARCHERY,        // yay
    SKILL_BLOCK,          // kalkan
    SKILL_LIGHT_ARMOR,    // hafif zırh
    SKILL_MEDIUM_ARMOR,   // orta zırh
    SKILL_HEAVY_ARMOR,    // ağır zırh
    SKILL_SNEAK,          // gizlenme
    SKILL_LOCKPICKING,    // kilit açma
    SKILL_PICKPOCKET,     // yankesicilik
    SKILL_SPEECH,         // konuşma/pazarlık
    SKILL_ALCHEMY,        // iksir
    SKILL_SMITHING,       // demircilik
    SKILL_ENCHANTING,     // büyüleme
    SKILL_COUNT,
    SKILL_NONE = -1,      // skill'e bağlı olmayan eylem (örn. silahsız vuruş)
} SkillType;

#define SKILL_MAX_LEVEL 100

// Tek bir skill'in durumu. xp bu seviyedeki birikimdir (toplam değil);
// seviye atlayınca taşan kısım yeni seviyeye devreder.
typedef struct {
    int   level;      // 1..SKILL_MAX_LEVEL
    float xp;         // bu seviyede biriken XP
    float xpToNext;   // bir sonraki seviyenin eşiği
} Skill;

// Tüm skill'leri başlangıç durumuna getir (level 1, eşikler hesaplı).
void skills_init(Skill *skills);

// XP ekle. Seviye atlandıysa true döner (UI bildirimi tetiklemek için).
// Tek seferde birden çok seviye atlanabilir; taşan XP kaybolmaz.
bool skill_add_xp(Skill *skills, SkillType t, float xp);

// Panelde/loglarda gösterilecek isim.
const char *skill_name(SkillType t);

// Skill paneli (K tuşu). Tüm skill'leri level + XP çubuğuyla listeler.
void skill_panel_render(const Skill *skills, SDL_Renderer *renderer,
                        TTF_Font *font, float panelX, float panelY);

#endif
