#ifndef LIGHT_H
#define LIGHT_H
#include "r2d.h"
#include "common.h"
#include "fx.h"   // WeaponFxType — silah efekti gece ışık verir

// Dünya-uzayı ışık kaynakları (gece ışık pass'ine girer).
// Gündüz ambient 1.0 olduğundan katkıları görünmez — açık/kapalı mantığı gerekmez.

// IŞIK TİPİ — titreşim karakterini belirler.
// Kanon kuralı (LORE §1.9): SENİ KORUYAN ŞEY DURUR, KORUMAYAN GEZİNİR.
// Kor sabit ve beyaza yakın; odun ateşi kaotik ve kırmızıya doygun. Fizik de
// aynısını söylüyor: kor akkor haldeki KATI (termal atalet → titremez), alev
// düşük-CRI emisyon (kaotik). Ve bu mekaniğe oturuyor: kor bastırır, odun bastırmaz.
typedef enum {
    LK_STEADY = 0,   // kor kandili — bilinçle sakin: halkanın kenarı OKUNMALI
    LK_CANDLE,       // mum / ev penceresi — hızlı ama küçük genlik
    LK_TORCH,        // meşale (oyuncu + zindan duvarı)
    LK_HEARTH,       // kamp ateşi / yakma çukuru — yavaş, geniş, kaotik
    LK_MAGIC,        // büyü ve silah efekti — fizik yok, nefes
    LK_COUNT,
} LightKind;

typedef struct {
    float x, y;        // dünya px
    float radius;      // dünya px — bu uzaklıkta katkı sıfır
    float r, g, b;     // ışık rengi (0..1)
    float intensity;   // katkı gücü (0..1 tipik)
    LightKind kind;    // titreşim karakteri
} LightDef;

extern const LightDef LIGHTS[];
extern const int LIGHT_COUNT;

// ═══ TEK DÜŞÜŞ EĞRİSİ — CPU ve GPU AYNI FORMÜLÜ KULLANIR ═══
//
// Bunu değiştirirsen r2d.c içindeki fs_light shader'ının karşılık gelen
// satırlarını da DEĞİŞTİR. İkisi ayrışırsa oyuncu aydınlık gördüğü yerde
// ölüyle karşılaşır (bir kez oldu: görsel 210px, bastırma 141px, fısıltı 158px).
//
// Eğrinin üç parçası ve gerekçeleri:
//   1. İç %35 PLATO — halkanın göbeği tam parlak; oyuncu "burası kesin güvenli"
//      diyebileceği bir çekirdek görsün.
//   2. Smoothstep düşüş — lineer çok mekanik, kare (t²) kenardan içeri fazla
//      yavaş yükseliyordu (her yer az aydınlık = sınır okunamaz).
//   3. KESİN KESİM (< LIGHT_EDGE_CUT → 0) — "eşik = ışığın bittiği yer"
//      kararının şartı. Smoothstep'in kuyruğu asimptotik sıfıra gider, yani
//      "ışık var mı yok mu" sorusu kenarda belirsiz kalırdı. Kuyruğu kesiyoruz:
//      artık ışığın bittiği yer gözle de kodla da aynı yer.
#define LIGHT_CORE_FRAC 0.65f   // t bu orana bölünür → iç %35 plato
#define LIGHT_EDGE_CUT  0.06f   // bunun altı sıfır sayılır (kesin kenar)

// Düşüş modu — GPU shader'ıyla AYNI kalmak zorunda (r2d_light_set_model).
// main.c dev tuşu ikisini birlikte değiştirir; ayrışırsa oyuncu aydınlık
// gördüğü yerde ölüyle karşılaşır.
void  light_set_falloff(int mode, float param);
int   light_falloff_mode(void);
float light_falloff_param(void);

float light_atten(float d, float r);

// Siyah cisim renk sıcaklığı → RGB (0..1). Fiziksel ışık rengi modu için.
void light_kelvin_rgb(float kelvin, float *r, float *g, float *b);

// Fiziksel renk modu (dev tuşu): ışık renkleri elle seçilmek yerine tipin
// Kelvin değerinden türetilir. Elle seçtiğimiz kandil rengi bu eğride
// ≈3130K'ye denk — yani tungsten masa lambası; fiziksel kor/mum çok daha kızıl.
void light_set_physical_color(bool on);
bool light_physical_color(void);

// Statik ışıklar + oyuncu ışıklarını (meşale + silah efekti) ekran-uzayına toplar.
// sw/sh: ekran boyutu (ekran dışı ışıklar atlanır). Dönen: out'a yazılan adet.
// staticLights=false: köy/sabit ışıklar atlanır (zindan — koordinat çakışması)
int light_collect(R2DLight *out, int cap, const Camera *cam, int sw, int sh, bool staticLights,
                  float torchX, float torchY, bool torchOn, WeaponFxType weaponFx);

// Kamp ateşi ışığı (Faz 6): konumu dinamik (oyuncu kurar) — çağıran dünya
// konumu verir, ekran dışıysa eklenmez. Dönen: yeni ışık adedi (n ya da n+1).
int light_append_campfire(R2DLight *out, int n, int cap, const Camera *cam,
                          int sw, int sh, float x, float y);

// Genel nokta ışığı (D1a — kandil/tapınak gibi durum bağımlı kaynaklar)
int light_append_point(R2DLight *out, int n, int cap, const Camera *cam,
                       int sw, int sh, float x, float y, float radius,
                       float r, float g, float b, float intensity, LightKind kind);

#endif
