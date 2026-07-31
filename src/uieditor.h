#ifndef UIEDITOR_H
#define UIEDITOR_H
#include "r2d.h"
#include "uilayout.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

// OYUN İÇİ YERLEŞİM DÜZENLEYİCİSİ (Ctrl+U).
//
// Neden oyun içinde ve neden web/masaüstü bir araç değil: konumun doğruluğu
// FONTUN kendisine bağlı. PublicPixel'in gerçek ölçüsü, gerçek pencere boyutu,
// gerçek renk ve gerçek post-fx altında bakmadan "16 mı 20 mi" sorusu
// cevaplanamaz. Dışarıda yapılan her ölçüm bir tahmindir.
//
// TASARIMIN ÇEKİRDEK KARARI: tutamaklar ÖĞENİN CANLI OLMASINI BEKLEMEZ.
// Tok buffunu konumlamak için yemek yemek, görev takipçisi için suikast
// zincirini 4. adıma getirmek gerekmiyor. Düzenleyici yerleşim VERİSİNİ dolaşır
// (canlı çizimleri değil), o yüzden her öğe her zaman elde. Bu, düzenleyicinin
// bütün değeri: aksi halde HUD'un yarısını hiç dizemezdin.
//
// SINIR: yalnız layout.json'DA OLAN öğeler düzenlenebilir. Kodda sorulup
// dosyada bulunmayan bir ad tutamak almaz (çözüm: dosyaya girdiyi ekle).
// Ayrıca öğelerin GERÇEK boyutu bilinmiyor — tutamak nokta + ad etiketidir,
// kutu değil. Metnin taşıp taşmadığını canlı öğeye bakarak görürsün.

void uieditor_toggle(void);
bool uieditor_active(void);

// ÖN İZLEME (P tuşu). Paneller yalnız açıkken çiziliyor (inventoryOpen,
// shopOpen…) ve düzenleyici açıkken oyun girdisi kapalı — yani paneli normal
// yolla açmak mümkün değil. Tutamağa bakarak panel konumlamak ise körlemedir:
// panelin gerçek genişliği, satır sayısı ve metni görünmeden "burada mı
// duruyor" sorusu cevaplanamaz.
//
// Dönen: ön izleme AÇIKSA seçili öğenin adı, değilse NULL. main.c panel çizim
// koşullarına bunu ekliyor — panel GERÇEK içeriğiyle çiziliyor.
const char *uieditor_preview(void);

// Olayı düzenleyici tüketirse true döner — çağıran o olayı OYUNA GEÇİRMEMELİ
// (yoksa sürüklerken karakter yürür, ok tuşları günlük sayfası çevirir).
// Pencere boyutu parametre DEĞİL: çapa çözümü uilayout'un içinde ve boyutu o
// tutuyor (uilayout_set_window). Buraya da geçirmek ikinci kaynak olurdu.
bool uieditor_handle_event(const SDL_Event *ev);

// Her şeyin üstüne çizer (post-fx ayrımından sonra çağrılmalı: tutamaklar
// filtreden etkilenmesin — ölçüm aracı filtrelenirse ölçüm bozulur).
void uieditor_render(R2D *renderer, TTF_Font *font, int winW, int winH);

#endif
