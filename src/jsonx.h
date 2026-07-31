#ifndef JSONX_H
#define JSONX_H
#include "cJSON.h"
#include <stdbool.h>

// cJSON ÜSTÜNE İNCE SARMALAYICI. cJSON sağlam bir ayrıştırıcı (unicode kaçışları,
// sayı uç durumları, Tiled'ın büyük iç içe çıktısı) ama üç şeyi vermiyor ve bu
// projede üçü de gerekli:
//
//   1) YORUM. JSON'da yorum yok, oysa bu projenin veri tabloları değerli "neden"
//      yorumlarıyla dolu (resource.h toplama sürecini, item.h taşma kazasını
//      anlatıyor). Yorumsuz veriye taşımak o bilgiyi buharlaştırır.
//      Çözüm: ayrıştırmadan ÖNCE // ve /* */ temizlenir (JSONC kabul edilir).
//      Dize İÇİNDEKİ // korunur — yoksa "http://" bozulurdu.
//
//   2) NET HATA. cJSON hata yerini işaretçi olarak verir; elle düzenlenen bir
//      dosyada işe yaramaz. Sarmalayıcı SATIR ve KOLON hesaplayıp söyler.
//      (JSON'un elle düzenlemedeki asıl zayıflığı sondaki fazla virgüldür ve
//      hata mesajı bunu açıkça göstermek zorunda.)
//
//   3) VARSAYILANLI OKUMA. Eksik alan çökertmemeli; varsayılana düşmeli ve
//      istenirse uyarmalı. Kısmi/eski dosyalar çalışmaya devam etsin.
//
// Ek kural: her veri dosyası bir "version" alanı taşır — save sisteminin
// sürümleme dersi (v29 toleranslı kuyruk) JSON'a da uygulanıyor.

// Dosyayı oku, yorumları temizle, ayrıştır. Hata olursa NULL döner ve
// SDL_Log ile satır/kolon + bağlam basar. Dönen ağaç cJSON_Delete ile silinir.
cJSON *jsonx_load(const char *path);

// Metinden ayrıştır (test/gömülü veri için). name yalnız hata mesajında kullanılır.
cJSON *jsonx_parse(const char *text, const char *name);

// --- Varsayılanlı getter'lar. obj NULL olabilir; her durumda def döner. ---
double      jsonx_num (const cJSON *obj, const char *key, double def);
int         jsonx_int (const cJSON *obj, const char *key, int def);
float       jsonx_flt (const cJSON *obj, const char *key, float def);
bool        jsonx_bool(const cJSON *obj, const char *key, bool def);
// Dönen işaretçi AĞACIN İÇİNE bakar — ağaç silinince geçersizleşir. Kopyalanmalı.
const char *jsonx_str (const cJSON *obj, const char *key, const char *def);

// Dizi/nesne alanı; yoksa NULL (döngüde güvenle kullanılır)
const cJSON *jsonx_arr(const cJSON *obj, const char *key);
const cJSON *jsonx_obj(const cJSON *obj, const char *key);

// Dosyaya yaz. pretty = girintili (git diff'i okunur olsun; veri dosyaları
// commit'lenir, tek satır JSON diff'i işe yaramaz). true = başarı.
bool jsonx_save(const char *path, const cJSON *root, bool pretty);

#endif
