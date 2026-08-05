#ifndef SKILLFX_H
#define SKILLFX_H

#include "r2d.h"
#include "fx.h"
#include "common.h"
#include "light.h"
#include <SDL3/SDL.h>
#include <stdbool.h>

// ── SKILL EFEKT MOTORU (prosedürel).
//
// NEDEN VAR: büyü efektleri şimdiye kadar HAZIR SPRITE SHEET'lerden geliyordu
// (assets/fx/*.png — orb_fire, arcane_burst, iceshield, spikes...). Sheet'ler
// güzel ama üç şeyi vermiyor:
//   1) TUTARLILIK — her sheet başka bir sanatçının dilinde; oyun içinde
//      birbirine benzemiyor. Kendi ürettiğimiz efektler aynı dili konuşur.
//   2) TEPKİ — sheet sabit: hedefe, mesafeye, güce, hava durumuna, gündüz/
//      geceye göre değişemez. Prosedürel efekt bunların hepsini okur.
//   3) SAHİPLİK — dış pakete bağımlılık kalmaz; yeni büyü eklemek dosya
//      indirmek değil TARİF yazmak olur.
//
// MİMARİ: bu katman KENDİ ÇİZİM SİSTEMİNİ kurmuyor — mevcut motor
// katmanlarını besteliyor:
//   · fx.c partikülleri (ADD/ALPHA, kuvvet alanı, ölüm kancası)
//   · r2d bükülme alanları (ısı/şok/burgaç/mercek)
//   · light.c nokta ışıkları (nefesle senkron)
//   · r2d primitifleri (çizgi/dikdörtgen — yıldırım kolu, halka)
//   · decal.c kalıntı (yanık/kırağı)
// Yani "skill motoru" ayrı bir dünya değil, VFX motorunun üstünde bir DİL.
//
// A/B ANAHTARI (Ctrl+P): sprite yolu SİLİNMEDİ. Anahtar kapalıyken oyun eski
// sheet'leri çizer, açıkken prosedürel efektler devreye girer. Karşılaştırma
// olmadan "daha iyi mi" sorusu cevaplanamaz.

#define SKILLFX_MAX_EVENT 24

typedef enum {
    SKILLFX_ICE_NOVA = 0,   // çevrede genişleyen buz halkası
    SKILLFX_LIGHTNING,      // gökten inen kol + çarpma
    SKILLFX_FIREBOLT,       // uçan mermi + çarpmada patlama
    SKILLFX_SPIKES,         // yerden fışkıran taş dikenler + toz
    SKILLFX_IMPACT,         // İSABET: yönlü koni + darbe yayı (kısa ömür)
    SKILLFX_FIRE_WAVE,      // ATEŞ DALGASI: bakış yönünde koşan alev duvarı
    SKILLFX_KIND_COUNT,
} SkillFxKind;

void skillfx_init(void);

// PROSEDÜREL MOD (Ctrl+P). false = eski sprite yolu.
void skillfx_set_procedural(bool on);
bool skillfx_procedural(void);

// ── OLAY BAŞLAT. Çağıran ateşler ve unutur; motor ömrü sürer.
// ICE_NOVA / LIGHTNING: (x,y) hedef · FIREBOLT: (x,y) çıkış, (tx,ty) hedef.
// FIRE_WAVE: (x,y) çıkış, (tx,ty) BİRİM YÖN (hedef değil — dalga yol alır).
void skillfx_cast(SkillFxKind kind, float x, float y, float tx, float ty,
                  float power);

// Kare başına: hareket, partikül besleme, bükülme, kalıntı
void skillfx_update(ParticleSystem *fx, float dt);

// Çizim (varlıklardan SONRA, ışık pass'inden ÖNCE — efekt zeminde değil havada)
void skillfx_render(R2D *renderer, const Camera *camera);

// ── EKRAN DARBESİ (genel motor katmanı): çarpma anında tam ekran kısa flaş.
// Görkemli VFX'in en ucuz ve en etkili parçası — göz "bir şey oldu" der.
// HUD'dan SONRA çizilmez: yalnız sahne katmanına biner (r2d postfx ayrımı).
// Boss darbesi, patlama, ışınlanma da aynı çağrıyı kullanır.
void skillfx_punch(float r, float g, float b, float strength, float durSec);

// ── İSABET (genel motor katmanı). "Vurdum" hissinin çekirdeği YÖNDÜR: darbe
// efekti merkezden her yöne saçılırsa vuruşun nereden geldiği okunmaz.
// (dx,dy) darbenin GİDİŞ yönü (vuran → vurulan). power hasara göre ölçek.
// ELEMAN her skill'in kendi imzasını taşır — çarpma anı da büyünün dilinde
// olmalı, hepsi aynı kıvılcım bulutu olmamalı:
#define SKFX_EL_STEEL  0   // çelik: sarı-beyaz kıvılcım konisi + metalik yay
#define SKFX_EL_ARCANE 1   // arcane: yıldız parçacık + ince genişleyen halka
#define SKFX_EL_FIRE   2   // ateş: alev sıçraması + kor + duman tüyü
#define SKFX_EL_ICE    3   // buz: kırık + kırağı bulutu + beyaz çatlama flaşı
#define SKFX_EL_BOLT   4   // yıldırım: yoğun kıvılcım + kısa dal kolları
#define SKFX_EL_EARTH  5   // toprak: kaya kırıntısı + toz (ışık YOK, ağır)
#define SKFX_EL_POISON 6   // zehir: yeşil damla + asılı kalan pus (sönük)
// Silah vuruşu, büyü çarpması, boss darbesi, ok isabeti aynı çağrıyı kullanır.
// freeze: hit-stop İSTENSİN Mİ. Alan büyüleri, yol alan dalgalar ve DoT
// kaynaklı isabetler false geçmek ZORUNDA — yoksa tek büyü birkaç moba çarpınca
// arka arkaya hit-stop birikiyor ve oyun takılıyor gibi oluyor (Hakan: "skill
// moba çarpıp ilerlerken donmalar oluyor"). Bu sözleşme çıplak vuruş yolunda
// zaten vardı (`strike_fx_mob`'un stop=0 parametresi); prosedürel isabet katmanı
// eklenirken ihlal edilmişti. Sarsıntı ve flaş her durumda çalışıyor —
// bastırılan yalnız ZAMANIN durması.
void skillfx_impact(float x, float y, float dx, float dy, int elem, float power,
                    bool freeze);

// ── DARBE İSTEKLERİ. Hit-stop ve kamera sarsıntısı OYUN katmanının değişkenleri
// (zaman ölçeği ve kamera); motor onlara uzanmıyor, İSTEK bırakıyor ve main
// her karede tüketiyor. Böylece skillfx main'in statiklerine bağımlı olmuyor
// ve aynı istekleri boss/tuzak/düşme hasarı da bırakabiliyor.
// Değerler elemana göre: yıldırım keskin ve kısa, toprak ağır ve uzun.
float skillfx_take_hitstop(void);                       // 0 = istek yok
void  skillfx_take_shake(float *amp, float *dur);       // amp 0 = istek yok
void skillfx_render_punch(R2D *renderer, int winW, int winH);

// Işık listesine katkı: efektler kendi ışığını taşır (ateş topu uçarken
// yeri aydınlatır, yıldırım çarpınca sahne beyazlar)
int skillfx_append_lights(R2DLight *out, int n, int cap, const Camera *cam,
                          int sw, int sh);

#endif // SKILLFX_H
