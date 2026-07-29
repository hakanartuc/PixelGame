#ifndef SHADOW_H
#define SHADOW_H
#include "r2d.h"
#include "common.h"
#include <stdbool.h>

// GÜNEŞ GÖLGESİ — gündüz yönlü gölge (LORE'un dışında, saf görsel derinlik).
//
// Top-down kamerada gerçek perspektif yok; gölge sprite'ın "yatık kopyası".
// Tabanı ayak hizasında çakılı kalır, tepesi güneşten uzağa yatar.
//
// Gece bu sistem kapanır: gece gölgesi çok kaynaklı bir iştir ve ışık pass'inin
// İÇİNDE çözülmek zorunda (dışında çizilen gölgeyi ışık pass'i kendi katkısıyla
// siler). Yani gündüz ve gece iki ayrı teknik, tek geçiş ağırlığıyla harmanlanır.

// Güneşin gölge parametreleri. false = güneş yok (gece), çizim yapılmaz.
//   dx, dy   : birim gölge yönü (ekran/dünya eksenleri, +y aşağı)
//   lenMul   : gölge uzunluğu, nesne YÜKSEKLİĞİNİN katı (0.35 öğlen .. 1.60 ufuk)
//   strength : 0..1 koyuluk (öğlen güçlü, şafak/akşam zayıf, gece 0)
// Gölge lekesi dokusunu üretir (radyal alpha gradyanı). Bir kez, doku
// yüklemelerinden sonra çağrılır. Çağrılmazsa leke çizilmez (gölge kaybolur).
void shadow_init(R2D *renderer);

bool shadow_sun(float *dx, float *dy, float *lenMul, float *strength);

// Karakter/nesne gölgesi — sprite'ı olmayan hareketli şeyler için.
// İki katman birden çizer:
//   1. TEMAS gölgesi: ayak altında yatık, küçük, HER ZAMAN (gece de) —
//      "zemine yapıştırılmış" hissini kıran şey bu, ve derinlik verir.
//   2. GÜNEŞ gölgesi: gündüz, yönlü ve uzun (shadow_sun parametreleriyle).
// footX/footY: ayak hizası (dünya px). w: sprite genişliği, h: yüksekliği.
void shadow_blob(R2D *renderer, const Camera *camera,
                 float footX, float footY, float w, float h);

// YALNIZ taban lekesi — güneş kuyruğu yok. Dekor gölgeleri için: sprite'ın
// eğik kopyası, dikdörtgenin alt kenarından başlıyor ve ağaç sprite'larında
// altta şeffaf boşluk olduğu için gövdenin dibinden KOPUK görünüyordu.
// Bu leke gölgeyi ayak izine çiviliyor.
void shadow_ground_blob(R2D *renderer, const Camera *camera,
                        float footX, float footY, float w, float strength);

#endif
