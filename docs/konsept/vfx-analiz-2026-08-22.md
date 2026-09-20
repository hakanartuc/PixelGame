# VFX / Shader Durum Analizi — 2026-08-22

Amaç: `vfx-teknik-brief.md` (2026-08-03) yazıldıktan ve Faz VFX + Faz SU
kapandıktan sonra motorun GERÇEK durumunu koddan çıkarmak; brief'in hangi
kalemlerinin kapandığını, hangilerinin açık kaldığını kayda geçirmek; yeni
iyileştirme/ekleme adaylarını önceliklendirmek. Yöntem: üç paralel kod taraması
(renderer+shader / VFX modülleri / ışık+atmosfer), kritik iddialar elle
doğrulandı. Satır numaraları 2026-08-22 itibarıyla `main` dalı.

> Brief'in A bölümündeki satır referansları MSL dönemine ait ve BAYAT;
> güncel özet bu doküman + `HANDOVER.md` "RENDER / VFX". Şüphede kaynak kod.

---

## (1) EN KRİTİK BULGU — prosedürel motor varsayılan KAPALI

`skillfx_init()` → `s_procedural = false` (`skillfx.c:484`); tek açma yolu
**Ctrl+P dev tuşu** (`main.c:4169`). Tüm `skillfx_*` girişleri kapalıyken erken
dönüyor (`skillfx.c:458, 539, 599, 1712, 2037`). Yani normal oyunda ŞUNLARIN
HİÇBİRİ YOK:

- 5 prosedürel büyü (buz novası / yıldırım / ateş topu / toprak dikenleri / alev seli)
- `skillfx_impact` isabet katmanı: yönlü koni + darbeye dik yay
- Elemana göre hit-stop/sarsıntı tablosu (`skillfx.c:1046-1066`)
- Ekran darbesi (`skillfx_punch`, `skillfx.c:456-478`)
- İsabet kaynaklı zemin parçaları

GDD §6.5/§6.6 "Hakan testi bekliyor" durumunda. **Her yeni VFX işinden önce bu
test yapılmalı ve varsayılan kararlaştırılmalı** — motorun yarısının kaderi bu
anahtarda.

## (2) BRİEF KALEMLERİNİN DURUMU (defter)

### Kapananlar
| Brief kalemi | Nerede kapandı |
|---|---|
| ADIM 0 — shader taşınabilirliği | Faz SHADER 2026-08-03; HLSL kaynak, offline compile, format probu (`r2d.c:194-208`) |
| Parçacıkta blend seçimi (ADIM 1.2) | 3 geçişli render: ALPHA/ADD/ALPHA-üstü (`fx.c:741-949`) |
| Parçacık rampası (ADIM 1.3) | `PAL[9][3][4][3]` + karacisim alev rampası (`fx.c:500-574`) |
| Parçacık kuvvetleri (ADIM 1.4) | Tip başına yerçekimi/kaldırma/sürtünme + rüzgâr alanı (`fx.c:664-732`, `fx.h:127-142`) |
| Sarsıntı + hit-stop + flaş (B.8) | `main.c:399-407` (0.22 sn soğumalı), mob beyaz-flaş (`mob.c:1193-1210`) |
| Bükülme (ADIM 4'ün nokta-kaynak hali) | 8 ekran-uzayı kaynak ISI/ŞOK/BURGAÇ/MERCEK (`postfx.frag.hlsl:55-104`, `r2d.h:201`) |
| Su (C.1'in ötesi) | 755 satırlık `water.frag.hlsl`, 13 katman, ayrı hedef üstünden kırılma |
| MULTIPLY altyapısı (yarım) | Pipeline kurulu ama **enum'da yok** — sentinel `R2D_BLEND_COUNT` (`r2d.c:276-281`) |

### Açık kalanlar (A.4 + ADIM 1 kalıntıları)
| Kalem | Durum | Not |
|---|---|---|
| Sprite başına fragment shader yolu | AÇIK | `sprite.frag.hlsl:15` sabit `tex*col`; yönlendirme blend enum'una bağlı. En büyük kapı: dissolve/eşik/outline/flow hepsi buna bakıyor. ~120-150 satır |
| REPEAT sampler | AÇIK | İki sampler da CLAMP (`r2d.c:406-416`). ~8 satır |
| 2.-3. doku slotu | AÇIK | `flush_to` tek doku bağlıyor. ~30 satır |
| MULTIPLY'ın public açılması | AÇIK | ~20 satır; ışık yolundaki sentinel'e dikkat |
| Post-fx'in offscreen hedefe çizilmesi | AÇIK | `postfxPipeline` yalnız swapchain formatı (`r2d.c:378`) → pass zincirlenemiyor, bloom tek geçişli kalıyor |
| bloomThreshold düşürme (ADIM 1.7) | **BU TURDA KAPANIYOR** | `main.c:1191` yorumu bayat — sahne/HUD ayrımı 2026-07-30'da geldi |
| Kamera yuvarlama (ADIM 1.8) | **BU TURDA KAPANDI — DERSLE** | Brief'in tarifi (ART_PX'e yuvarla) denendi ve GERİ ALINDI: kamera 4px ızgaraya oturunca sprite'lar oturmadığı için karakter 0-4px testere dişi çizdi ("karakter yürürken titriyor"). Doğrusu TAM dünya pikseline floor — dünya tutarlı, oyuncu sapması <1px. Gerçek sanat-ızgarası kilidi sprite tarafı da oturunca (düşük çözünürlüklü hedef borcuyla birlikte) gelir |
| Hava görselleri düzeltmesi (D.4) | **KISMEN BU TURDA** | Işık sırası bu turda; doku/sıçrama sonraya |
| HDR RGBA16F + sanat-çözünürlüklü sceneTex | AÇIK | Büyük iş, acil değil; su shader'ındaki 0.45 additive tavanının kök sebebi |
| ADIM 3-6 (eşikli alev, curl duman, palet ışıma) | AÇIK | Altyapı üçlüsü (REPEAT+doku üreticisi+shader yolu) önkoşul |
| GDD §6.1 kalıcı görsel imza | AÇIK KARAR | Brief E bölümü sonundaki uyarı geçerli: imza seçilmeden shader serisi yazılmamalı |

## (3) BORÇ GÜNÜ — bu turda kapatılan 6 kalem

1. **`animfx_render` blend kurmuyor** (`fx.c:1027-1044`) — yalnız çağrı sırası
   tesadüfüyle ALPHA; decal/shadow'u ısıran "sessiz additive" tuzağının açık
   üçüncü kopyası.
2. **Erişilmez kopya dal**: `fx.c:848-857` iki özdeş `WEAPON_FX_EMBER` else-if.
3. **Bloom eşiği bayat** (0.66-0.80 + geçersiz gerekçe yorumu, `main.c:1191`).
4. **Kamera yuvarlama yok** (`render.c:17-36`).
5. **Hava görselleri ışıktan sonra** (`main.c:12370` → `12374-12445`): gece
   yarısı sis parlak gri, kar bembeyaz. Şimşek flaşı bilinçli parlak kalacak.
6. **CPU/GPU ışık eğrisi sözleşmesi korumasız** (`light.c:13-48` ↔
   `light.frag.hlsl:131-154`) — oyun kuralı CPU'yu, göz GPU'yu okuyor; bir kez
   ayrıştığı GDD §6.2'de kayıtlı.

## (4) SİSTEMSEL İYİLEŞTİRMELER (sıradaki turlara aday)

- **Sessiz düşürme her yerde**: quad/draw taşması (`r2d.c:683-684`), ışık
  slotu dolunca (`light.c:311`), floattext (`floattext.c:27`). Üç "cılız"
  şikayetinin arkasından sessiz bütçe hatası çıktı — dev HUD'a taşma sayacı.
- **Işık bütçesinde öncelik yok**: 32 slot sabit sırayla; kalabalık meydan
  patlama flaşını aç bırakabilir. `intensity×radius/dist²` sıralaması ucuz.
- **`main.c`'de 8 elle yazılmış tek-atımlık FX havuzu** (`main.c:1453-1502`) —
  `AnimFx` varken kopya update/render döngüleri; birleştirme ~150 satır siler.
- **Decal baskısı**: tek buz novası 64 slotun 26'sını yiyor (`skillfx.c:816-822`).
- **Floattext v2 ihtiyacı**: sabit 1 sn ömür, üst üste binme, kontur yok, crit
  yalnız renk+ünlem, hasar tipi rengi yok, zoom'la ölçeklenmiyor.
- **Gölge lekesi LINEAR filtreli** (`shadow.c:104-127`) — NEAREST dünyada tek
  bulanık öğe; güneş kuyruğu düşük açıda 5 boncuk gibi okunuyor.
- **Zindanda ışık gölgelemesi kapalı** (`main.c:12357`) — meşale duvardan geçiyor.
- **Ölüm kancası az kullanılıyor**: `fx.h:290-292` buz→kırağı, kan→leke, asit→çukur
  vaat ediyor; yalnız kıvılcım→is bağlı (`main.c:381-390`).

## (5) EKLEME PAKETLERİ

### A. Game-feel (shader'sız, kod az, his büyük)
- Oyuncu ölümü TAM GÖRSELSİZ (`main.c:11035-11046`) — ses+günlük+ceza, ekranda hiçbir şey.
- Hasar geri bildirimi tutarsız: vinyet+sarsıntı yalnız 3 yakın-dövüş noktasında;
  DoT/tuzak/düşme/mana kalkanı kırılması sessiz.
- Crit'in görsel imzası yok (`combat.c` bayrağı üretiyor, tek tüketici floattext rengi).
- Level-up/unvan yalnız yazı (`main.c:905-938`) — `fx_burst` iki fonksiyon ötede.
- Mob çözülmesi partikülsüz (`mob.c:1151-1175`); savrulma görsel izsiz; ok izi yok.

### B. Dünya-tepki (groundfx/decal hazır, tüketici yok)
- Ayak tozu — `groundfx.h:70` "koşu tozu" tüketicisini zaten vaat ediyor;
  kar/çamur izi decal'i aynı kalem.
- Su etkileşimi: `groundfx.c:169-172` "sıçrama su sisteminin işi" diyor ama o
  sistem yok. Giriş halkası+sıçrama; halka altyapısı suda var (LB_RING).
- Yağmur yere çarpmıyor: sıçrama + kısa ömürlü ıslak koyulaşma decal'i.
  Islaklığın doğru reçetesi zaten yazılı: koyulaş+doygunlaştır
  (`water.frag.hlsl:261-264`), karaya uygulanmamış.
- Toplama/madencilik: 5 gri partikül; yonga/kıymık groundfx'ten bedava.
- Fırtına yıldırımı gökte görünmüyor — `skillfx.c:1724`'teki 11 parçalı
  dallanan kol olduğu gibi yeniden kullanılabilir.

### C. Atmosfer
- Sis v2: bugün iki düz gri dikdörtgen (`main.c:12430-12444`). Dünya-çivili IGN
  (ışık shader'ında hazır) + iki katman kayan gürültü; bölge yoğunluğu `region.c`'den.
- Gece ışık gölgeleri: gece HİÇBİR ŞEY gölge düşürmüyor (`shadow.c:52`);
  `shadow.h:12-14` çözümün ışık pass'inde olduğunu yazmış. Karakter ayak izleri
  occluder listesine → meşaleli oyuncuya hareketli gölge. Gecenin en büyük sıçraması.
- Ambient partiküller: ateşböceği, zindan hüzmesinde toz, baca dumanı
  (kanon: Karaocak, brief D.2), yaprak.
- Sepken (Karduvar, brief D.3): kar döngüsüne darbeli rüzgâr + soğukta ağız buharı.
- Bölge/mevsim grade'i: 22 preset elle F7; `region.c` mood'u görsel hiçbir şey
  sürmüyor; mevsim tonu `main.c:12249-12258`'de sabit switch. Grade'in
  bölge+mevsim+havadan türemesi biyomları ayrıştırır. Önkoşul: GDD §6.1 imza kararı.
- Gece suyu: yalnız kandil sütunu var; ay parıltısı yok.

### D. Shader hattı (sıra korunmalı)
1. Altyapı üçlüsü: REPEAT + gürültü/rampa CPU üreticisi + sprite shader yolu
2. Eşikli alev — beş damganın temeli (VENOM ışıksızlığı kanon kuralı, korunacak)
3. Curl türbülans + dither'lı duman
4. Palet ışıma (damar taşı "içten ışıyan kristal")
5. Shader yolu gelince gövde efektleri: dissolve/burn-edge ölüm, donma kaplaması

## (6) ÖNCELİK SIRASI (Hakan onayı 2026-08-22)

1. ~~Borç günü~~ ← **bu turda** (bölüm 3)
2. Kapı kararı: Ctrl+P testi → prosedürel varsayılan mı? (bölüm 1)
3. Game-feel paketi (A)
4. Dünya-tepki paketi (B)
5. Shader altyapısı + eşikli alev (D)
6. Atmosfer (C) — kalıcı grade/imza kararıyla birlikte

Kural hatırlatmaları: oyun içi test olmadan commit yok · "cılız" şikayetinde
önce tavan ölçülür · yeni FX modülü blend'ini KENDİ kurar · kararlı hal
bütçesi = hız × ömür × 62.5.
