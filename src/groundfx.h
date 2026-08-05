#ifndef GROUNDFX_H
#define GROUNDFX_H

#include "r2d.h"
#include "common.h"
#include <SDL3/SDL.h>
#include <stdbool.h>

// ── ZEMİN PARÇASI MOTORU (ground chunk system).
//
// NEDEN VAR (Hakan fikri, 2026-08-05): "Dikenlerin üst uç kısımlarına çıktığı
// zemine göre parçacıklar gelse. Çimden çıkıyorsa ucunda çim parçacıkları, kum,
// çöl veya toprak, taş gibi yerden çıkıyorsa ona göre parçacıklar olsa nasıl
// olur?" Ardından iki netleştirme:
//   · "Bu yapıyı muhtemelen başka yerlerde de kullanırız" → tek büyüye gömülmez,
//     MOTOR KATMANI olur.
//   · "Harita her zaman değişebilir. Basit bir renk değil DOKU gibi olmalı."
//
// TASARIMIN ÇEKİRDEĞİ: parça, zeminin GERÇEK DOKUSUNDAN kesilmiş bir kare.
// Düz renkle boyanmış parçacık iki şeyi kaybediyor: (1) haritayı takip etmez —
// karo sanatı değişince kırıntı yanlış kalır; (2) "boya" gibi durur, malzeme
// gibi durmaz. Doku parçası ikisini birden çözüyor ve BEDAVA güncellenir:
// yeni bir karo eklendiğinde kırıntısı kendiliğinden doğru olur.
//
// ÖLÇÜ ŞANSI: karo dokusu 16×16, dünyada 64px kaplıyor → 1 doku pikseli = 4
// dünya pikseli (ART_PX). 3-5 piksellik bir pencere doğal ölçekte 12-20 dünya
// px eder, yani hiç ölçekleme bulanıklığı olmadan gerçek zemin parçası.
//
// NEDEN AYRI MODÜL (fx.c'ye eklenmedi): fx.c tek bir atlas dokusundan çiziyor
// ve batch'i buna dayanıyor. Zemin parçası KARO BAŞINA farklı doku istiyor;
// fx.c'ye sokmak onun tek-doku batch'ini bozardı. Ayrı havuz + ayrı çizim.
//
// DAVRANIŞ SINIFI KARO ALANLARINDAN TÜRETİLİYOR, id tablosundan DEĞİL —
// "harita her zaman değişebilir" kuralının kod karşılığı bu:
//   · conceals (içine girilip gizlenilen: çim, saman) → BİTKİ: hafif, savrulur
//   · solid    (taş, kaya, duvar)                     → KAYA: ağır, hızlı düşer
//   · diğer    (toprak, kum, yol, döşeme)             → TANE: orta, bol
//   · su karosu                                        → parça yok, sıçrama var
// Yeni bir karo eklendiğinde bu üç alan zaten doldurulduğu için davranış da
// kendiliğinden makul çıkıyor; ayrıca elle bakım gerektiren bir tablo yok.

#define GROUNDFX_MAX 512

void groundfx_init(void);

// ── KARO DOKULARINI BİR KEZ BAĞLA (main açılışta çağırır). Dizinin sahibi main;
// groundfx yalnız okur. Bağlamak yerine her çağrıya doku dizisi taşımak, aradaki
// her katmanı (skillfx, mob, boss...) doku dizisiyle kirletirdi — tüketici çok
// olacağı için bağlama doğru olan.
void groundfx_bind_tiles(R2DTexture **tileTextures);

// ── ZEMİN PATLAMASI. (wx,wy) noktasındaki karonun dokusundan parça savurur.
// tileTextures: main'in sahip olduğu karo doku dizisi (tilemap_render ile aynı).
// count  : istenen parça sayısı (havuz dolarsa daha az doğar)
// power  : hız/boy ölçeği (1.0 = normal)
// upBias : 0 = her yöne, 1 = ağırlıklı YUKARI (yerden fışkırma)
// Tüketiciler: taş dikeni (uç + dip), boss yer vuruşu, patlama zemin teması,
// düşme hasarı, ok saplanması, ileride koşu tozu.
void groundfx_burst(float wx, float wy, int count, float power, float upBias);

void groundfx_update(float dt);

// Çizim DERİNLİK BANDIYLA: parça yere ait bir nesne, karakterin ayak hizasına
// göre önüne/arkasına düşmeli (bkz. fx_render_band gerekçesi).
void groundfx_render_band(R2D *renderer, const Camera *camera,
                          float yMin, float yMax);

// Teşhis: kaç parça yaşıyor
int groundfx_active(void);

#endif // GROUNDFX_H
