#ifndef UILAYOUT_H
#define UILAYOUT_H
#include <SDL3/SDL.h>   // SDL_FRect (uilayout_panel)
#include <stdbool.h>

// UI YERLEŞİMİ — HUD öğelerinin konumu koddan çıkıp veriye taşındı.
//
// Eskiden konum kodda sabit sayıydı: draw_text(..., 16.0f, 96.0f, warm).
// Sorun sadece "editör yazamıyoruz" değildi: pencere yeniden boyutlanınca
// sağa/alta çivilenmiş öğeler elle hesaplanıyordu ve her biri ayrı formülle.
//
// ÇAPA (anchor) bu yüzden merkezde: konum, pencerenin hangi köşesine/kenarına
// göre ölçüleceğini söyler. Böylece bir öğeyi "sağ üstten 12 px içeride" diye
// tanımlarsın ve yeniden boyutlandırma bedavaya doğru çalışır.
//
// Dosya: assets/ui/layout.json — JSONC (yorum yazılabilir), "version" alanı
// taşır (save sisteminin sürümleme dersi).
//
// EKSİK ANAHTAR ÇÖKERTMEZ: bilinmeyen ad sorulursa kodda verilen varsayılan
// döner ve BİR KEZ uyarı basılır. Sessiz kalmak da hata; her karede bağırmak da.

// HİZA: çapa+ofsetle çözülen noktanın öğenin NERESİ olduğu.
//   sol  → nokta sol kenar (varsayılan)
//   orta → nokta yatay merkez (boss barı, splash)
//   sag  → nokta sağ kenar (saat, görev takipçisi — sağ kenara yapışıklar)
//
// Neden veride: bu bilgi olmadan her çağıran genişliği kendi çıkarıyordu
// (`winW - tw - 20`). Beş kopya = beş sapma noktası; üstelik düzenleyici de
// tutamağı yanlış yere çizerdi. Tek kaynak burası.
typedef enum {
    UI_ALIGN_LEFT = 0,
    UI_ALIGN_CENTER,
    UI_ALIGN_RIGHT,
    UI_ALIGN_COUNT,
} UiAlign;

// DİKEY HİZA: hiza'nın simetriği. Alt kenara bağlı öğelerde çözülen nokta ALT
// kenardır ve öğe onun ÜSTÜNE çizilir — minimap, alta yapışık paneller. Bu
// olmadan "sag-alt çapa + boyut" birleşimi yanlış yere düşer.
//   ust  → nokta üst kenar (varsayılan; draw_text'in doğal davranışı)
//   orta → nokta dikey merkez
//   alt  → nokta alt kenar
typedef enum {
    UI_VALIGN_TOP = 0,
    UI_VALIGN_MIDDLE,
    UI_VALIGN_BOTTOM,
    UI_VALIGN_COUNT,
} UiVAlign;

typedef enum {
    UI_ANCHOR_TOP_LEFT = 0,
    UI_ANCHOR_TOP_CENTER,
    UI_ANCHOR_TOP_RIGHT,
    UI_ANCHOR_MID_LEFT,
    UI_ANCHOR_CENTER,
    UI_ANCHOR_MID_RIGHT,
    UI_ANCHOR_BOTTOM_LEFT,
    UI_ANCHOR_BOTTOM_CENTER,
    UI_ANCHOR_BOTTOM_RIGHT,
    UI_ANCHOR_COUNT,
} UiAnchor;

// Yerleşim dosyasının yolu. TEK yerde duruyor: main.c yüklerken, düzenleyici
// kaydederken aynı sabiti kullanıyor — iki string'in ayrışma ihtimali yok.
#define UI_LAYOUT_PATH "assets/ui/layout.json"

// Dosyayı yükler. Yoksa/bozuksa false döner ama oyun ÇALIŞMAYA DEVAM EDER —
// bütün sorgular koddaki varsayılanlara düşer. Yerleşim verisi bir kolaylık,
// zorunluluk değil; bozuk bir JSON oyunu açılmaz hale getirmemeli.
bool uilayout_load(const char *path);
void uilayout_free(void);

// Pencere boyutunu bildir (her kare ya da resize'da). Çapa çözümü buna bakar.
void uilayout_set_window(int w, int h);

// Öğenin ÇÖZÜLMÜŞ ekran konumu. name bulunamazsa defX/defY döner (sol-üst çapa
// varsayılır) ve bir kez uyarı basılır.
// Çözüm: çapa noktası + ofset. Sağ/alt çapalarda ofset genelde NEGATİF olur
// (içeri doğru), ama zorunlu değil — dışarı taşırmak da mümkün (bilinçli).
void uilayout_get(const char *name, float defX, float defY, float *outX, float *outY);

// Hizayı UYGULAYARAK sol-üst köşeyi döndürür: çizim doğrudan *outX,*outY'ye
// yapılır. contentW/H = öğenin ölçülmüş boyutu (metinse TTF_GetStringSize).
// sol/ust → nokta olduğu gibi; orta → nokta - boyut/2; sag/alt → nokta - boyut.
// Çok satırlı sağa hizalı öğelerde satır başına çağrılır (her satır kendi
// genişliğiyle hizalanır); tarama düzine öğe üstünde, maliyeti yok.
void uilayout_get_aligned(const char *name, float defX, float defY,
                          float contentW, float contentH,
                          float *outX, float *outY);

// Yalnız düzenleyici/denetim için: kayıtlı öğe sayısı ve i. öğenin adı.
int         uilayout_count(void);
const char *uilayout_name_at(int i);
// BOYUT (isteğe bağlı). Dosyada "w"/"h" yoksa def döner — çoğu öğenin boyutu
// koddan gelir ve gelmelidir: can barının genişliği max stat'la BÜYÜYOR, metin
// genişliği fonttan çıkıyor. Boyut yalnız panellerde (envanter, dükkan, craft)
// gerçekten bir tasarım kararı.
void uilayout_get_size(const char *name, float defW, float defH,
                       float *outW, float *outH);

// PANEL DESENİ (dükkan, ilan panosu, beceri listesi, craft, suikast panosu).
// Bu panellerin hepsinde aynı yapı var: konum ve GENİŞLİK tasarım kararı,
// YÜKSEKLİK satır sayısından türüyor (stockCount × ROW_H + pay). O yüzden
// yükseklik hiç sorulmuyor.
//
// Dönen: {x, y, w, 0}. Çizim ve tıklama testi AYNI çağrıyı yapar; ikisi de
// yerleşimden geçtiği için panel taşındığında tıklama beraber taşınır.
// defX/defY/defW = yerleşimde anahtar yoksa geçerli olacak eski davranış.
SDL_FRect uilayout_panel(const char *name, float defX, float defY, float defW);

// Nokta ekranın hangi üçte-birine düşüyorsa o çapayı döndürür. Düzenleyicideki
// "en uygun çapa" komutu bunu kullanıyor: öğeyi sürükledikten sonra çapasını
// elle seçmek zorunda kalmıyorsun, yanlış çapa seçme riski kapanıyor.
UiAnchor uilayout_best_anchor(float px, float py);

// Ofset pencereye göre şüpheli büyük mü (çapa muhtemelen yanlış)? Düzenleyici
// bunu uyarı olarak gösterir — yanlış çapanın belirtisi görüntüde değil, ancak
// pencere boyutlandırılınca ortaya çıkıyor.
bool uilayout_offset_suspect(int i);

// i. öğenin ham verisi (çapa + hiza + ofset). Düzenleyici bunları oynatacak.
bool  uilayout_raw_at(int i, UiAnchor *anchor, UiAlign *align, UiVAlign *valign,
                      float *ox, float *oy);
void  uilayout_set_raw(int i, UiAnchor anchor, UiAlign align, UiVAlign valign,
                       float ox, float oy);
// i. öğenin ÇÖZÜLMÜŞ çapa noktası (hiza uygulanmadan) — düzenleyici tutamağı
// buraya çizer. Öğe canlı olmasa bile çalışır: yerleşim verisi görünürlükten
// bağımsızdır, düzenleyicinin bütün değeri de bu.
bool  uilayout_point_at(int i, float *outX, float *outY);
// i. öğenin boyutu. Dönen değer: boyut BELİRTİLMİŞ mi (w>0 && h>0). Düzenleyici
// yeniden boyutlandırma tutamaklarını yalnız belirtilmiş öğelerde gösteriyor.
bool  uilayout_size_at(int i, float *w, float *h);
void  uilayout_set_size(int i, float w, float h);
// Belleğdeki hali dosyaya yazar (düzenleyici kaydeder). Yorumlar KORUNMAZ —
// cJSON yorumu tutmuyor; kaydedince el yazısı notlar gider. Düzenleyici bunu
// kullanıcıya söylemek zorunda.
bool uilayout_save(const char *path);

const char *uilayout_anchor_name(UiAnchor a);   // "sag-ust" gibi (dosya ve UI)
const char *uilayout_align_name(UiAlign a);     // "sol" / "orta" / "sag"
const char *uilayout_valign_name(UiVAlign a);   // "ust" / "orta" / "alt"
// Çapanın ekrandaki noktası. Düzenleyici çapa değiştirirken buna ihtiyaç duyar:
// öğeyi yerinde TUTMAK için yeni ofset = eski nokta - yeni çapa noktası. Yoksa
// çapa değiştirdiğinde öğe ekranın öbür ucuna zıplar.
void uilayout_anchor_point(UiAnchor a, float *outX, float *outY);

#endif
