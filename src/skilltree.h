#ifndef SKILLTREE_H
#define SKILLTREE_H
#include "r2d.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "skill.h"
#include "ability.h"
#include "perk.h"   // PerkEffects (birleşik etkiler — combat/main okur)

// Derin skill ağacı (Faz 21): eşik anketinin yerini alan puanlı düğüm ağacı.
//  - PUAN: skill ağaçları her 4 seviyede 1; KADER ağacı toplam seviyenin her 10'unda 1
//  - AĞAÇ: düğümler kenarlı; almak için önkoşullardan BİRİ alınmış olmalı (kök hariç)
//  - SINIF: Küçük 1p / Dikkat çekici 2p / AKTİF 2p (beceri açar, hotbar'a düşer) /
//           KEYSTONE 3p (build tanımlar, BEDELİ var)
//  - KITLIK: savaş ağaçları tam alınamaz; meslek mini ağaçları yüksek seviyede dolar
//  - Durum: ağaç başına 32-bit maske (Player.treeTaken + Player.fateTaken — save v13)

#define TREE_MAX_NODES 32       // maske 32-bit: ağaç başına düğüm tavanı
#define TREE_LEVELS_PER_POINT 4    // skill ağaçları
#define FATE_LEVELS_PER_POINT 10   // Kader: TOPLAM seviye başına

// Ağaç kimliği: SkillType değerleri + Kader için özel kimlik
#define TREE_FATE ((int)SKILL_COUNT)

typedef enum {
    NODE_MINOR = 0,     // 1 puan
    NODE_NOTABLE,       // 2 puan
    NODE_ACTIVE,        // 2 puan — aktif beceri AÇAR (hotbar'a düşer)
    NODE_KEYSTONE,      // 3 puan
} NodeClass;

typedef struct {
    const char *name;
    const char *desc;      // panel alt kutusunda gösterilen etki metni
    NodeClass cls;
    Sint8 prereqA, prereqB; // düğüm indeksleri (-1 yok): BİRİ alınmışsa yol açık
    Sint8 gx, gy;           // panel ızgara konumu (çizim + tıklama aynı kaynak)
    Sint8 grantAbility;     // NODE_ACTIVE: açtığı AbilityId (-1 = yok)
} TreeNode;

typedef struct {
    int treeId;             // SkillType ya da TREE_FATE
    const TreeNode *nodes;
    int nodeCount;
} SkillTreeDef;

// Bu kimliğin ağacı var mı? (savaş 5'lisi + Sneak/Speech + 6 meslek + Kader)
bool tree_has_content(int treeId);
const SkillTreeDef *tree_get(int treeId);

int tree_node_cost(NodeClass c);                     // 1/2/2/3
int tree_points_earned(int treeId, int level);       // skill: /4, kader: /10
int tree_points_spent(int treeId, Uint32 taken);     // alınanların toplam maliyeti

// DEBUG (F10): her ağaca ekstra puan — kaydedilmez, yayında hep 0
void tree_set_debug_points(int bonus);

// Düğüm alınabilir mi: yol açık + puan yeter + zaten alınmamış
bool tree_node_available(int treeId, int node, Uint32 taken, int freePoints);

// Tüm ağaçlardan birleşik etkiler (aktif açılışları dahil — fx->abilityOpen)
void tree_compute_effects(const Uint32 taken[SKILL_COUNT], Uint32 fateTaken,
                          PerkEffects *out);

// --- Panel: düğüm-kenar haritası (çizim ve tıklama AYNI yerleşim) ---
// level: skill seviyesi (Kader için TOPLAM seviye). selNode: seçili düğüm (-1).
// respecCost: >=0 ise sunak yakınında — "Sıfırla" satırı çizilir.
void tree_panel_render(int treeId, Uint32 taken, int level,
                       int selNode, int respecCost,
                       R2D *renderer, TTF_Font *font, float px, float py);
// Tıklanan düğüm indeksi; TREE_CLICK_RESPEC = sıfırla satırı; -1 = boşluk
#define TREE_CLICK_RESPEC (-2)
int tree_panel_node_at(int treeId, float x, float y, float px, float py);

#endif
