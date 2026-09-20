#ifndef FXTEX_H
#define FXTEX_H
#include "r2d.h"

// fxtex — prosedürel doku üreticisi (CPU, açılışta bir kez çalışır).
// Brief ADIM 2.2'nin karşılığı: shader serisinin (eşikli alev, curl duman,
// dissolve) hammaddesi. Çalışma anında gürültü HESAPLANMAZ — bir kez üretilir,
// shader yalnız örnekler; üç arka uçta da (SPIR-V/MSL/DXIL) aynı görüntü
// (frac(sin()) dersinin CPU'daki karşılığı: tamsayı hash, determinist).

// DÖŞENEBİLİR value-noise, size×size RGBA (size 2'nin katı olmalı, 128 tipik).
// Kanal başına BİR OKTAV: R=4, G=8, B=16, A=32 hücre frekansı (hepsi size'ı
// böler → kenarlar dikişsiz döşenir). Shader oktavları kendi ağırlığıyla
// karıştırır (FBM için tipik ağırlık: 0.533/0.267/0.133/0.067) ya da tek
// kanalı farklı hızda kaydırır (alev/duman animasyonu).
// Dönen doku REPEAT işaretli gelir (amacı bu). seed: aynı seed = aynı doku.
R2DTexture *fxtex_noise(R2D *r2d, int size, unsigned int seed);

// RENK RAMPASI, width×1 RGBA. n adet durak rengi (rgb 0..1) ve konumu
// (stop 0..1, artan sırada) arasında doğrusal geçiş; shader yoğunluğu u
// koordinatı yapıp örnekler (eşikli alevin "rampadan renk" adımı).
// hueShift: rampanın SONUNA doğru toplam ton kayması (derece, 0 = yok) —
// tek paletten soğuk/sıcak varyant üretmenin ucuz yolu.
// Dönen doku CLAMP + NEAREST (kademeli pixel-art rengi; yumuşak isteyen
// r2d_texture_set_linear ile açar).
R2DTexture *fxtex_ramp(R2D *r2d, const float (*rgb)[3], const float *stop,
                       int n, int width, float hueShift);

#endif
