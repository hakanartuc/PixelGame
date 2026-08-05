# PROGRESS — Yapılanlar ve Kalanlar

Güncelleme: 2026-07-30

## ✅ Tamamlanan fazlar

### Faz VFX — prosedürel VFX / skill motoru (2026-08-05)
Hakan'ın çerçevesi bu fazın tamamını belirledi: *"Bunları sadece silaha, kamp
ateşine alev, duman ekliyoruz gibi düşünme — biz aslında sistem/motor
oluşturuyoruz."* Her parça katman olarak yazıldı; ateş, büyü, boss darbesi,
hava olayı aynı çağrıları paylaşıyor. Görsel dil Hakan seçimi: KLASİK RPG
GÖRKEMİ. Detaylı gerekçeler GDD §6.6'da.

- **Blend borçları kapandı**: `decal_render` ve `shadow.c` blend kurmuyordu
  (miras alıyorlardı); partiküller HER ZAMAN ALPHA'ydı, bu yüzden alev bloom
  eşiğini (0.74) hiç geçemiyordu. Tip başına additive/alpha ayrımı geldi.
- **Bükülme alanı sistemi** (`r2d_distort_set`, 8 ekran-uzayı kaynak): ISI /
  ŞOK / BURGAÇ / MERCEK. Post-fx pass'inin İÇİNDE, `distortOn` bayrağı F7
  grade seçiminden AYRI (eskiden ısı bükülmesi VOLKAN preset'ine bağlıydı —
  efekt istemek tüm ekranı volkana çevirmek demekti).
- **Efekt nefesi** (`fx_breath`): partikül yoğunluğu + ısı genliği + ışık
  şiddeti aynı osilatörden. Ayrı hesaplanınca üç katman bağımsız titriyor ve
  göz tek bir ateş görmüyordu. Kanon bağı LORE §1.9.
- **Prosedürel skill motoru** (`skillfx.h/.c`, ~700 satır): buz novası,
  yıldırım, ateş topu, taş dikenler — mevcut katmanları BESTELİYOR (partikül
  + bükülme + ışık + r2d primitifleri + decal). Sprite yolu SİLİNMEDİ,
  **Ctrl+P** A/B anahtarıyla yan yana.
- **İsabet katmanı** (`skillfx_impact`, 7 eleman): yönlü koni + darbeye DİK
  yay + elemana göre hit-stop/sarsıntı (yıldırım keskin-kısa, toprak
  ağır-uzun, zehir neredeyse yok). Motor oyunun değişkenlerine uzanmıyor;
  `skillfx_take_hitstop/shake` ile İSTEK bırakıyor, main tüketiyor.
- **Parçacık biçim atlası** (7 hücre, kodda üretiliyor): puf, kıymık, çizik,
  yıldız, nokta, alev dili, is pulu + hıza göre uzama ve dönme.
- **Ton kimliği** (`fx_set_pal_variant`): palet satırı başına üç varyant,
  indeks her satırda aynı aileyi gösteriyor (0 kanonik / 1 soğuk / 2 sıcak).
  Olay başına tek indeks yetiyor. `fx_spawn` imzası korundu.
- **Dummy hedef** (`MOB_DUMMY`): 999999 HP, hareketsiz, pasif, loot yok —
  Hakan efektleri saldırı altında test edemiyordu.
- **Dev tuşları**: Ctrl+F partikül tipi izolasyonu (8 durum) · Ctrl+G silah
  damgası döngüsü (`devFxOverride` — damga her kare ekipmandan yeniden
  atandığı için doğrudan yazmak işlemiyordu) · Ctrl+P prosedürel A/B ·
  Ctrl+B/T/Y teşhis · hotbar üstünde skill adı ipucu.

- **ALEV SELİ** (`SKILLFX_FIRE_WAVE`, eski adı *Ateş Aslanı* — isim 2026-08-05
  değişti): düz hat üstünde akan sarı alev seli + İÇ İÇE koyu kırmızı siluetler.
  Hat boyunca yaşıyor (yalnız cephede değil), yoğunluk cepheye artıyor, yan
  salınım düzgün mekânsal fonksiyondan. Üç tur denedik: alev duvarı → maskeli
  aslan silueti → kodda basılmış aslan gövdesi; üçü de reddedildi.
  **Kalıcı ders:** prosedürel gövde SOYUT biçimlerde çalışıyor (diken, kristal,
  alev dili), TANINAN BİR CANLIYA benzemekte çalışmıyor — göz canlı
  anatomisindeki en küçük hatayı yakalıyor, taşta yakalamıyor.
  Oyun tarafı (hasar/hareket) hiç değişmedi; aslan sheet'leri sprite A/B
  yolunda, aslan kodu #if 0'da gerekçesiyle rafta.
- **KIZIL ZARF** (`WEAPON_FX_FLAME_DEEP`) ve **KOYU SİLUET**
  (`WEAPON_FX_FLAME_DARK`): alev "sarı" duruyordu ve palet ayarı işe yaramıyordu
  çünkü sorun karıştırma matematiğiydi — ADD'de 300 parçacık üst üste binince R
  doyup G tırmanıyor, sonuç kaçınılmaz sarı-beyaz. Kırmızı hue'yu koruyan ALPHA
  katmanında olmak zorunda. Zarf çekirdeğin ARKASINDA (pass 0), siluet ÜSTÜNDE
  (yeni pass 2) — ikisi de aynı yayıcıdan, aynı hızla doğuyor ki ayrı hareket
  etmesinler.
- **MOB TEPKİSİ** (`mob_hit_react`, motor katmanı): gövde sarsıntısı (yatay,
  elemana göre genlik) + eleman izi tonu + iz sızdırması (yanan mob alev dili,
  donan mob kırağı, çarpılan kıvılcım, taş yiyen kırıntı) + donma animasyonu.
  Mekanik kanallar zaten vardı; eksik olan görünmeleriydi.
- **TAKILMA ÜÇ KATMANDA KAPANDI**: (1) `skillfx_impact`'e `freeze` parametresi —
  yol alan dalga zamanı durdurmuyor (sözleşme `strike_fx_mob`'un stop=0'ında
  zaten yazılıydı, prosedürel katman onu ihlal etmişti); (2) `fx_spawn` havuz
  doluyken TÜM havuzu tarıyordu → kare başına ~2 milyon boşa iterasyon, artık
  128 slot probe; (3) motor çapında hit-stop bekleme süresi (0.22sn) — çağrı
  yerlerini tek tek düzeltmek yetmez, bir daha unutulur.
  Teşhisi Hakan'ın ipucu verdi: "slime'larda yok, goblinlerde var" — goblin
  KAMP halinde, dalga 300px yolunda 4-6 gobline değiyor; slime dağınık.
- **BÜTÇE DİSİPLİNİ**: havuz 8192 (yığından çıkarıldı — 460KB main'in
  stack'inde durmasın) ve yayıcı hızları bütçeye OTURTULDU. Alev Seli 12.856
  partikül istiyordu, %60'ı sessizce düşüyordu; yoğunluk kareden kareye kumardı.
  **Kararlı hal = hız × ömür × 62.5** — yayıcı yazarken bu hesap yapılacak.
- **Yol göstergesi**: prosedürel mod açıkken her büyü çağrısı ekrana
  `"<ad> -> PROSEDUREL"` / `"-> SPRITE (motor disi)"` yazıyor.

- **ZEMİN PARÇASI MOTORU** (`groundfx.c/h`, yeni modül): kırıntı, zeminin
  GERÇEK DOKUSUNDAN kesilmiş bir kare — düz renk değil. Karo 16×16 ve dünyada
  64px olduğu için 1 doku pikseli = 4 dünya px; 3-5 piksellik pencere doğal
  ölçekte 12-20px, hiç bulanıklık yok. Davranış sınıfı karo ALANLARINDAN
  türüyor (conceals→bitki, solid→kaya, diğer→tane, su→parça yok), id
  tablosundan DEĞİL: "harita her zaman değişebilir" (Hakan) kuralının kod
  karşılığı — yeni karo eklendiğinde kırıntısı kendiliğinden doğru.
  Tüketiciler: taş dikeni (dip + UÇ + çökme) · ateş topu patlaması · buz novası
  cephesi · isabet katmanı (yalnız ağır elemanlar). Havuz 512, tepe ~250.
- **TAŞ DİKENİ 2. TUR**: üç yüz (aydınlık/ara/gölge) · çentikli kenar (taş
  kırılır, yontulmaz) · kırık uç · DİP YAKASI (dikenin kırdığı toprak kabartısı)
  · %12 AŞMA (yerden fışkırma hissi) · aynalama + genişlik varyasyonu
  (3 kalıp × ayna = 6 görünüm; 14 diken üç kalıbı tekrar edince damga gibi).
- **DERİNLİK BÖLMESİ**: efekt oyuncunun ayak hizasında ikiye bölünüyor — uzak
  yarı oyuncudan önce, yakın yarı sonra. Oyuncu artık efektin İÇİNDE duruyor,
  ne kayboluyor ne de üstüne yapıştırılmış çıkartma gibi. Gövdeler banda tabi,
  havadaki anlık geometri (yıldırım kolu, isabet yayı) bir kez çiziliyor.
  Merkeze 22px açıklık: büyücü kaynağın kendisi.
- **BÜTÇE AŞIMI GERİYE DÖNÜK BULUNDU**: buz novası havuzun **15 KATINI**
  istiyordu (124.900 / 8.192), toprak dikenleri 5.7 katını. Partiküllerin %93'ü
  sessizce düşüyordu. Kuralı Alev Seli'nde bulmuş ama eski büyülere
  uygulamamışım. Nova 5.695'e, toprak ~5.100'e oturtuldu.
  **Görsel şikayetin arkasında bütçe hatası vardı:** "çemberin içi boş" —
  havuz cephede doluyor, iç alana sıra gelmiyordu.
- **DOLU DİSK**: nova artık halka değil ALAN (iç alan seyrek+alçak partikülle
  dolu, bant en parlak kenar olarak kalıyor; yarıçap sqrt ile = düzgün alan).
- **BUZ OLUŞUMU koni değil KÜME**: 6 plaka, farklı yükseklik/yatıklık, kırık
  uçlar, ortak taban buzu. Üç konik plaka "üçgen buzlar" gibi duruyordu.
- **BUZ NOVASI yer izi tamamlandı**: yerde duran buz oluşumları (3 varyant,
  kodda basılır, parlak kenar + saydam gövde), dalganın ardında yükseliyor,
  sonunda eriyor · kırağı artık cephe ilerledikçe bırakılıyor (eskiden tek
  seferde) · kar hissi (zemine yakın pus + düşen taneler) · ömür 1.75 → 2.70 sn.
  Leke bütçesi MAX_DECALS 64'e oturtuldu (mesafeye bağlı bırakma).

**Bu fazın dersleri (hepsi Hakan'ın bildirimiyle bulundu):**
- *"üçü de çok cılız"* → sorun ayar değildi: `MAX_PARTICLES` 512'ydi ve
  fazlası SESSİZCE düşüyordu. 2560. **"Cılız" şikayetinde önce tavanı ölç.**
- *"alevlerde yıldırım gibi çizgiler"* → ateş patlaması 110 KIVILCIM
  kullanıyordu; kıvılcım çizik biçimi + hıza göre uzama taşıyor. Alev dili ve
  is pulu tipleri eklendi.
- *"üçgen huni şeklinde parçacıklar çok kötü duruyor"* → alev dilinin
  genişliği y ile DOĞRUSAL daralıyordu, sonuç keskin koni. Profil yaprağa
  çevrildi (`pow(t,0.55)` uca sivrilik, `(1-0.34·t³)` dibi toplar).
- *"hala üçgenler var ve yıldırım etkisi gibi efektler var"* (ÜÇÜNCÜ tur) →
  üçgenler alev dili değil **İS PULU**'ydu: biçim `|dx|+|dy|` ölçüsüyle
  yazılmış, o ölçü eşkenar dörtgen çiziyor ve takla attığı için üçgen okunuyor.
  Yarıçap AÇIYA göre modüle edildi (decal.c'de kanıtlı numara). Yıldırım
  çizgileri de ateşin 26 kıvılcımıydı: **sayıyı azaltmak yetmez, TİP yanlış** —
  kıvılcım çelik/yıldırıma ait, ateş kor fırlatır. Ateşten kıvılcım tamamen
  çıktı, 44 kor geldi.
  **Prosedürel biçim koda girmeden önce siluetini ASCII bas** — üç turun üçü de
  tek dökümle kapanırdı. **L1 ölçüsü (|dx|+|dy|) düzensizlik değil GEOMETRİ
  verir**; düzensizlik açısal modülasyonla gelir.
- *"hala var. Yanlış mı bakıyorum?"* (DÖRDÜNCÜ tur) → **hayır, doğru bakıyordu;
  hata bendeydi.** "Alev skili" = ATEŞ ASLANI, F10'un verdiği tek ateş büyüsü;
  o büyü sprite yolundaydı ve Ctrl+P onu hiç kesmiyordu. Ben dört tur boyunca
  prosedürel ateş TOPUNUN biçimini düzelttim, o başka bir şeye bakıyordu.
  **Kural: kısmi kapsamlı A/B anahtarı kapsamı EKRANDA göstermek zorundadır** —
  yoksa test eden kişi kendi hatası sanır. Yol göstergesi bu yüzden eklendi.
- **Kendi iddiamı denetlemek bir hata buldu**: palet varyant tablosunda indeks
  anlamı satırlar arasında tutarsızdı (FLAME'de 1=kızıl, SPARK'ta 1=mavi) →
  yıldırım sıcak kahve duman, toprak soğuk gri duman alıyordu. Tam ters.
- *"pembe parlama beyaza dönmüş"* → PINK'i palet tablosuna koymuşum, silah
  kimliğini ezdi. Palet tablosu yalnız KENDİ rengi olmayan tipler için.
- *"hala asset kullanımı var gibi"* → yalnız SPAWN tarafını kapatmışım, ÇİZİM
  tarafı sprite'ları basmaya devam ediyordu. A/B anahtarı iki tarafı da kesmeli.
- Yıldırım kolu (200,215,255) ile buz halkası (175,222,255) neredeyse aynı
  renkti; iki büyü ekranda ayırt edilemiyordu → mor-beyaz / camgöbeği.

### Faz SU-4 — kıyı: kesit, kumsal, deniz sırtları, iki karakterli ses (2026-08-04)
Su fazının kıyı turu. Görsel test on tur sürdü; bulunan hataların çoğu ayar
değil YAPISAL hataydı ve hepsi koda gerekçesiyle yazıldı.

- **Kıyı kesiti** (`shore_bake_atlas`/`shore_cut_pass`): ALttP'den ölçülen
  anatomi çalışma zamanında boyanıp çift-ızgara ile kıyıya basılıyor —
  fistolu yamaç + dip konturu (kuzey), siluet dudağı + çim saçağı (güney),
  köşede kesintisiz dönen kontur. Yön "en yakın kara" yerine ıslaklık
  alanının GRADYANINDAN (kopuk tarak dişleri düzeldi).
  Kapalı olduğu yerler: vaha (kendi atlası var), harita kenarı dolgu suyu,
  ve KUMSAL (kesit yükseltilmiş kıyı grameri; düz sahilde koyu şerit olur).
- **Güney kumsalı**: haritanın alt kenarı gerçek deniz + 4 karo saf kum
  bandı, su çizgisi her sütunda satır 141 (düm düz). Doğu kenar dolgu suyu
  kaldırıldı (çöl geçidinin yanında yapay sahil çiziyordu).
- **Kumsal dalga vuruşu** (`tilemap_render_beach_swash`, KARA üstü geçiş):
  üç evreli döngü 6.5 sn (deniz ritminden AYRI), evreler ease-in-out;
  aktif swash BULANIK kumlu su (berrak değil — dalga kumu askıda taşır),
  çekilmiş bölge koyu ıslak kum + sütun başına kuruma HAFIZASI, köpük
  öbekleri kesik kesik. Köpük parlaklığı dalga ENERJİSİNDEN (kıyı
  çizgisinde 0) — sabit alfa "bir anda belirip kayboluyor" görünüyordu.
- **Deniz sırtları shader'a TAŞINDI** (LB_ARC yerine ilerleyen swell):
  CPU'da yedi tur denendi, hep "suyun üstüne yapıştırılmış çubuk" gibi
  durdu — sebep mimari: CPU geçişi su geçişinden SONRA çiziyor, sırtlar
  suyun renk hattından/geometrisinden geçmiyor. Shader'da sırt konumu kıyı
  uzaklığı alanından türüyor → kıyıya kendiliğinden paralel. Eski CPU kodu
  `#if 0` içinde gerekçesiyle duruyor.
- **Maske kıyı menzili 3 → 12 karo** (`WMASK_SHORE_HT` 6→24): alan 3 karoda
  DOYUYORDU, sırtlar orada donup kıyıya yakın şeride hapsoluyordu.
  Shader'daki SHORE_PX/SHORE_LINE formülünden yeniden ölçeklendi (su
  çizgisi aynı fiziksel yerde kaldı).
- **Derinlik bant kenarları organik**: kuantalamadan önce yumuşak mekânsal
  sapma — düz kıyıda bantlar ekranı kesen yatay teraslara dönüşüyordu.
- **Su sesi iki karakterli**: `amb_water.wav` (deniz: lap kabarmalı,
  0.33 Hz) + `amb_water_calm.wav` (gölet: 3 boğuk lap, 0.18 Hz, kısık),
  `ambience_set_water(gain, agit)` çapraz geçişli. Mizaç kaynağı VFX ile
  ORTAK: `tilemap_water_agitation()`.
- **Çırpıntı sözleşmesi**: agit < 0.05 ⇒ "bu gövdenin KENDİ kıyı sanatı var,
  shader kıyı bantları çizmez" (vaha). Yapay lacivert şeritler bundan.
- **Öğrenilen dört kural (koda yazılı):** (1) aynı sinüste t ile x birlikte
  görünürse desen yana yürür · (2) düzensizlik hash ile verilirse komşular
  kopar, düzgün mekânsal fonksiyon gerekir · (3) kıvrımın yatay türevi
  dikey türevi aşarsa eş-yükselti çizgisi dikleşir ve kıyıya dik artefakt
  çıkar (|dbend/dx| ≤ 0.35 × dph/dy) · (4) su efekti su geçişinden SONRA
  çizilirse suyun parçası olamaz.

### Faz SU-3 — su kapanışı: oyuncu yansıması + gece + yağmur (2026-08-04)
Su fazının kapanış turu (Hakan: "suyu son haline getirelim").

- **Oyuncu yansıması:** `player_render_reflection` — anlık animasyon karesi +
  TÜM giyili ekipman. İki hata düzeltildi: oversize bayrağı ÖZELLİK, durum
  değil (bayrağı görünce katmanı atlamak yansımayı ÇIPLAK bırakıyordu);
  alpha 84 renkleri yıkıyordu → 135.
- **Balık düzeltmesi:** zar tabanlı yerleşim gitti — maske kurulurken gövde
  başına GARANTİLİ çapa (raf halkasından, suyun şeffaf olduğu yerden).
  Gölette kesin 1 balık. Gövde + kuyruk lekesi (yüzme yönü okunur).
- **Su kenarı fenerleri:** gölet KD kıyısı + göl kıyısı (dekor + ışık aynı
  koordinat) — köy ışıklarının hiçbiri suya 110 px yaklaşmıyordu.
- **Gece lamba sütunu (LB_LAMP, Ctrl+B 14 durum):** en parlak 2 ışık suya
  titrek sıcak sütun düşürür. İKİ DERS: (1) ışık pass'i sahneyi ambient'la
  çarpar — sütun karanlıkta güçlenerek telafi eder; (2) telafi TAVANSIZ
  bırakılınca katkı 1.6'ya fırladı, doymuş turuncu blok çıktı — EKLEMELİ
  BÜTÇE ≤ 0.45 kuralı artık formülün içinde (3. doyma vakası).
- **Yağmur halkaları:** weather → su; ıslak havada halka yoğun/hızlı/kısa.
- **r2d_test su kapsaması:** sentetik maskeyle 5. pipeline da koşumda —
  Windows'ta ilk çalıştırmada beş shader birden doğrulanır.

### Faz SU-2 — su yansımaları + su yaşamı (2026-08-04)
Su 2. turu (Hakan seçimi; ateş paketi sıradaki).

- **Sprite yansımaları:** dekorlar (ağaç/çalı) suya aynalanır —
  `r2d_draw_texture_reflect` (dikey ayna + alfa; flip imzası bozulmadı) +
  `decor_render_reflections` (gölgenin kardeşi: ayak çizgisi opak sınırlardan,
  aşağı doğru, 0.82 kısaltma, KARO KARO ıslak kırpma — `tilemap_is_water`).
  Su geçişinden ÖNCE çizilir: kırılma büker, süzgeç koyultur. Oyuncu/NPC
  yansıması SONRAYA (anim kare tesisatı gerekir).
- **Balık gölgeleri:** 3×3'ü ıslak seyrek çapalarda dolanan koyu leke
  (scatter'da, duvar saati) — su geçişinden önce, yani suyun İÇİNDE okunur.
- **Halka dalgacık:** yeni shader katmanı (LB_RING, Ctrl+B 13 durum) —
  1 px halka tam piksel adımlarıyla genişleyip söner, ~3 sn'de yeni zar.
- **Nilüferler:** `tilemap_render_water_overlay` — su geçişinden SONRA çizilen
  ilk öğe (bükülmezler; bükülselerdi batık okunurdu) + temas gölgesi.
- **Göl kuzey kıyısına 3 meşe + çalı, gölete 1 meşe** (yansıma vitrini —
  kural: ağaç suyun KUZEYİNDE durmalı ki aynası suya düşsün).

### Faz SU — su hacmi: tek geçişli shader su sistemi (2026-08-04)
Motorun ilk gerçek shader efekti. Su artık doku değil HACİM: zemin katmanı
ayrı target'a alınır, `shaders/water.frag.hlsl` onu dalga normaline göre
kaydırarak örnekler (kırılma), üstüne katmanlar biner. GDD §1.2h.

- **11 katman:** kırılma · derinlik (süzgeç/örtü çift modeli) · kostik ·
  parlama · köpük (varsayılan KAPALI) · ıslak bant · koyu bant · menisküs ·
  kuzey kıyı gölgesi · köpük benekleri + yosun tutamları · ark sıraları.
  `Ctrl+B` her katmanı tek başına izole eder (12 durum).
- **Maske:** karo başına 2×2 teksel RGBA — R derinlik (gövde başına
  normalize), G kıyı uzaklığı, B çırpıntı (gövde mizacı), A kuzey kıyı.
  `wld_water_mask_fill` (tilemap.c), LINEAR örneklenir.
- **Mizaç tek kanaldan:** gölet (26) durgun/şeffaf/köpüksüz — deniz (255)
  dalgalı/örtülü/köpük atlaslı. Aynı shader, iki karakter.
- **Sualtı serpiştirme:** taş rafı + bitkiler (bush_1/2 renk moduyla), su
  geçişinden ÖNCE çizilir → bedava bükülür ve süzülür. id 2 ve 20.
- **Yeni karo [20] sualtı taban** (stone.png, katı değil, 0.72×) + köy
  güneyinde gölet (world.csv). Kıyı atlası gölette DENENDİ ve GERİ ALINDI
  (köpük halkası getirdi — koddaki nota bak).
- **Pixel-art disiplini:** her alan sanat ızgarasına çivili (`snap_art`),
  tamsayı hash (`frac(sin())` üç arka uçta farklı!), sert bant sınırları.
  Bayer dither derinlik bantlarında DENENDİ, İKİ KEZ dama artefaktı verdi,
  çıkarıldı (düz platoda dither sınırda değil her yerde dama üretir).
- **İki araştırma ajanı:** katman yığını reçetesi + ALttP/Stardew haritalarının
  piksel piksel ölçümü (Lake Hylia kıyı profili). Kritik bulgular: çukuru
  gölge değil ÇİZİLİ DUVAR taşır (karo sanatı borcu), parlak su çizgisi
  karaya yapışıktır, koyu bant ondan sonra ve geniştir.
- **Rafta:** kuzey duvar shader prototipi (`u.l.w`), şerit-kaydırma demosu
  (tilemap.c), shader köpüğü (`foam` parametresi). Üçü de gerekçeli yorumla.
- Kalan (SU 2. tur adayları): sprite yansımaları · balık gölgesi + halka
  dalgacık · nilüfer (su üstü öğe) · kıyı karo sanatı (Yol A) · gece lamba
  yansıması. KIŞYOLU (Karduvar buz gölü) harita fazında bu sistemi kullanır.

### Faz SHADER — MSL'den HLSL'e, Windows yolu açıldı (2026-08-03)
VFX fazının **sıfırıncı adımı**. Görsel değişiklik SIFIR ve tek kural buydu;
Hakan gözle doğruladı ("her şey aynı görünüyor" = geçme notu).

**Sorun neydi:** shader kaynağı `r2d.c`'de gömülü bir MSL string'iydi, format
sabit yazılmıştı. `SDL_GPU` API'si çok platformlu ama shader **bytecode**'unu
taşımıyor — her arka uç kendi formatını istiyor. Yani oyun Windows'ta
`SDL_CreateGPUShader`'da düşüyordu: "VFX çalışmaz" değil, **"oyun çalışmaz"**.
Steam kitlesinin büyük kısmı Windows olduğu için bu bir sevkiyat tıkacıydı.

**Çözüm:** HLSL kaynak dil, **derleme zamanında** üç formata çevrim, çalışma
anında `SDL_GetGPUShaderFormats()` ile seçim. Çalışma zamanı bağımlılığı
artmadı — blob'lar repoda, yalnız build makinesi derleyici istiyor. (Hakan'ın
açık isteği: *"kendi sistemimizi geliştirip dışa bağımlılığımızı yok edebiliriz."*)

**`select()` tuzağı — sessiz hata olurdu.** HLSL 2021'in `select(cond,a,b)`
imzası MSL'in `select(a,b,cond)` sırasının **tersi**, ve biz onu tam olarak
sRGB↔lineer dönüşümünde kullanıyorduk. Derleme hatası vermez, **sessizce yanlış
ışık rengi** verir. HLSL'de hiç kullanılmadı; açık `?:` yazıldı ve niye
kullanılmadığı yoruma yazıldı.

**Doğrulama:** eski elle yazılmış MSL ile üretilen MSL **gerçek GPU'da** çizilip
byte byte karşılaştırıldı — **801 vaka, 46.137.600 piksel, 0 farklı piksel.**
Üstüne MoltenVK kurulup SPIR-V yolu **fiilen koşturuldu**: Metal/MSL ile
Vulkan/SPIR-V 81 vakada birebir aynı, yani descriptor set eşlemesi doğru.

**Windows durumu:** DXIL üretilemedi, Microsoft DXC'nin macOS build'i yok. Kod
diski ölçtüğü için DXIL bayrağı verilmiyor ve Windows bugün **Vulkan/SPIR-V**
ile çizer. Bir Windows makinesinde DXIL eklendiğinde D3D12 kendiliğinden tercih
edilir, **kod değişmez**. Uyarı: DXC kullanılırken `dxil.dll` `dxc.exe`'nin
yanında olmalı, yoksa imzasız DXIL çıkar ve müşteride "Shader is corrupt".

**`docs/konsept/vfx-teknik-brief.md`** (1775 satır) de bu turda yazıldı: shader
trickleri, "su olmadan su" ve "alevin üstünde duman" vakaları adım adım, kanona
bağlı öneri listesi. **İki kırılganlık buldu, henüz düzeltilmedi:**
`decal_render`/`shadow.c` blend modu ayarlamıyor (ateş eklemek yanık lekesini
sessizce bozar) ve parçacıkların hepsi alpha çiziliyor (**alev bloom eşiğini
geçemiyor, o yüzden sıcak hissetmiyor**).

### Faz KADRO — Yenitaç NPC kadrosu, 23 slot (2026-08-03)
İçerik fazı. Kod yazılmadı; **8 doküman** yazıldı, 4 ajan turu + 3 denetçi çalıştı.
Tıkaç buydu: üç hikaye kartı seti, altı karşılaşma ve Bağlı hattının kurmacası
kadroya bağlıydı.

**Kapsam kararı:** altı şehrin tamamı DEĞİL, yalnız **Yenitaç** (başlangıç bölgesi).
Gerekçe: haritalar yok, ERW paketi gelmedi, ve ilk yirmi dakikanın tamamı Yenitaç'ta
geçiyor. Diğer beş şehir haritası çizilince kadro alır; şimdilik LORE §6'nın isimli
figürleri çapa olarak duruyor.

**Dosyalar:** `kadro-yenitac-slotlar.md` (23 slot + 18 NPC'nin göç haritası + dakika
dakika ilk 20 dk + para merdiveni) · `kadro-isim-havuzu.md` (katman/şehir/lakap
havuzları + çakışma sicili + yerelleştirme sözleşmesi) · `kadro-kartlari-salon.md`
(slot 1-8, 21-22) · `kadro-kartlari-sehir.md` (slot 9-11, 13-19) ·
`kadro-kartlari-yeni.md` (slot 12, 20, 23) · `kadro-kanon-raporu.md` ·
`kadro-klise-raporu.md` · `kadro-dil-raporu.md`.

**HAKAN'IN ÜÇ KARARI:**
1. **Yenitaç'ta büyü hocası YOK.** Çayır kenarında sahipsiz bir Akkemer sunağı
   (`STATION_ENCHANT` kodda zaten sahipsiz). Kadroya "büyücü hocası" eklemek "sen
   büyücüsün" demenin kibar hâli olurdu.
2. **Deli Sabri yanılıyor.** Yedinci Ağız'ın kapısı artık **oyuncunun onu
   düzeltmesiyle** açılıyor — kutsama değil devralma. Eski hâli ("Sabri sana *sen de
   bakıyorsun* der") iş önlüğü giymiş bir seçilmişlikti. Yanlış okunan işaret: beyin
   sağ tarafı ölü, gövdesi sağa kayıyor, Sabri onu "hayır dedi" diye okuyor. Bedeli
   en pahalı yerde — o yüzden beyi iki kıştır kuzey köşesine çıkarmıyor, yani
   **Sarp'ın tek arzusunu onu seven tek adam kapatıyor.** Sayaç altı değil **BEŞ**
   ağız (Sabri sayılmaz: yorum yapmıyor, gördüğünü söylüyor).
3. **Gökçe'nin buz hikâyesi bir YALAN.** Buzun üstüne hiç çıkmadı, kıyıda kaldı.
   LORE §8'in "elini uzattı, yakasını tuttu" cümlesi artık OLAN değil **ANLATILAN**;
   tek tanık kendisi. Kanıt Karduvar'daki nöbet taşı ve Ak Nöbet defteri — o sabahın
   adları yazılı, Gökçe yok. Kadroya çürütülebilir ilk yalanı kazandırdı.
4. **Para merdiveni:** `akçe` gündelik · `kese` = 1.000 akçe · `sikke` = 100 kese
   (devlet). Oyuncunun kesesinde **ikinci para birimi olmayacak** — `player.gold`
   tek alan. Bedava karakterizasyon: sayman kese konuşuyor, kesedar akçe.

**ÜÇ DENETÇİ NE BULDU (hepsi rapor dosyalarında, damgalı):**
- **Kanon editörü 8 kırıcı bulgu.** En ağırı benim kararımı çürüttü: "salon ayrı
  harita olmayacak" kararı bir **kod faturasından** türetilmişti (`NpcDef.map` tek
  int, rutin harita sınırını geçemiyor) — bu projede kod faturası kanon gerekçesi
  olamaz. Karar tersine çevrildi: **salon AYRI HARİTA**, çatısı yok ama duvarı ve
  kapısı var. Diğerleri: erteleme sayacı dört kaynakta dört farklı gerçek ·
  "kasada üç ay var" ile kesedarın verdikleri çelişiyor · Rüstem'in gizli buzu
  defterde kayıtlı bir lojistik operasyonu olmuş · Şirin'in borcunu LORE'da Şirin
  kapatıyor, kartlarda Vedia · **Kıraç ≈ Kırca** (ikisi de Karduvarlı, aynı beş
  sesbirim — havuz "Karca"yı bu gerekçeyle elemişti) · iskele parasız değil
  **emirsiz** duruyor · vinyet panosu 1. dakikada görev veriyor.
- **Klişe avcısı sayısal teşhis:** 20 kişiden 13'ü sayı sayıyor, 11'inin ad yarası
  var, 20'sinin sırrı var, **sıfırı memnun, sıfırı beceriksiz, sıfırı inançlı,
  sıfırı yalancı.** "Saklamak" tek karakter kurma fiili olmuş. Üç yeni slot bu
  yüzden eklendi (12 makaracı / 20 bac tahsildarı / 23 gassal) ve iki karakter
  "yerine biri konabilir" yapıldı.
- **Dil denetçisi:** 14 kart bir imza kelime ilan edip hiçbir replikte kullanmamış
  ("Rüstem *Kaç?* der" ama sormuyor); 12 karakter aynı sözdizimsel kalıbı
  kullanıyor; salon kadrosunun 8'de 8'i cümle ortasında kendini düzeltiyor. Hepsi
  düzeltildi ve kartlara **denetlenebilir üç kural** yazıldı (imza kuralı, aforizma
  tavanı, sayı ağzı dörtle sınırlı).

**Ajanlar üç önerimi reddetti ve üçünde de haklıydılar:** gazi için "Bürküt" (havuz
§7 onu ana hat dışında tutuyor → **Toygar**) · `main.c` çağrı sayımım (ne benim
"14"üm ne kanon editörünün "8"i; gerçek **10**) · gassal için "uğursuzluk sayan biri"
(uğur register'ı KORKULUK'a kayıyor → eksik imza).

**Kanona giren adlar:** Perçin · Ayaz · Toygar · Ragıp · Uçkun · Bekmez · Hafize ·
Sekiz · Rasih · Uz · Nafi · Sabit · Kert · Tav (Mahir'in ocak adı) · Cevriye ·
Kurak/Sulak/Dirlik/Kaymak/Petek/Ömür/Kumral (çayır köyü, gerçek tarihî adların
yerine). Lakaplar: Yaya · Sağır · Geçmedi · Kırk Adım.

**GDD/LORE/vinyet aynı turda güncellendi** (kanon kuralı: kanon değişirse ona
dayanan dosyalar aynı commit'te ya güncellenir ya damgalanır):
GDD §1.2d ([AÇIK]→[ONAYLI], salon ayrı harita + iptal gerekçesi) · **GDD §1.2f
yeni** (para ölçeği) · GDD §8.2 (muhafız hiyerarşisi [AÇIK] sorusu kapandı: sicil
sahibi haritadan değil KURUMDAN gelir) · LORE §6 (Rüstem bugün 55; öğrendiği kış
Sarp'ın felç kışı) · LORE §8 (Gökçe'nin anlattığı + nöbet taşı kanıtı) · LORE §10
(beş ağız + kapının yeni açılışı) · `intro_demirboyun.txt` dört replik.

### Faz PANO — kontratta kabul adımı kaldırıldı (2026-07-31)
GDD §1.2c kural 3'ün ilk uygulaması. Save formatı DEĞİŞMEDİ (durum enum'u aynı).

**Sorun:** `contract_on_kill` döngüsü `states[s] != CONTRACT_ACCEPTED` diye eliyordu —
yani panoya gitmediysen goblin öldürmen SAYILMIYORDU. Oyuncuyu memur yapan şey buydu:
iş yapmak için önce izin almak.

**Şimdi:** her öldürme sayılıyor (yalnız DONE elenir). Panoya sonradan bakınca ilerleme
görünüyor, satır vurgulanıyor ve *"> PARASINI AL"* yazıyor. Tek tıkla ödeme.
Tıklama akışı da birleşti: eskiden AVAILABLE'a tıklamak "kabul et", ACCEPTED'a tıklamak
"teslim et"ti — artık tek dal var, hazırsa öder, değilse *"Henüz hazır değil"* der.

**KENDİ ÇIKARDIĞIM AÇIK, aynı turda yakalandı:** `contract_turn_in` yalnız ACCEPTED'ı
DONE yapıyordu. Kabul adımı kalkınca bu bir **altın basma açığı** olurdu — hiç alınmamış
ama tamamlanmış ilan ödeme yapıp AVAILABLE kalır, oyuncu tekrar tıklar, para yine gelir.
Fonksiyon AVAILABLE'ı da kapatacak şekilde düzeltildi.

**Panel metinleri de eskimişti:** *"Kabul + teslim: satıra tıkla"* → *"İş görüldüyse
satıra tıkla, parasını al"*. Pano artık ilan panosu değil **fiyat listesi**.

`contract_accept()` SİLİNMEDİ (kanon kuralı) ve gerekçesi yazıldı: ZİNCİR görevlerinde
kabul adımı DURUYOR ("bir sözdür", GDD §1.2c kural 3), yani fonksiyon oraya bağlanabilir.
Suikastçi panosu (`player.contractActive`, Kısıklar loncası) ayrı sistem — dokunulmadı;
lonca kontratı bir söz olduğu için kabul adımı orada anlamlı.

### Faz AÇILIŞ — v0 kanon katmanı söküldü, açılış yeniden kuruldu (2026-07-31)
Kararlar GDD §1.2d (ilk yirmi dakika) ve §1.2e (karşılaşma sistemi)'nde.

**ÜÇ KATMANLI KANON YIĞINI BULUNDU.** Depo kökünde `STORY.md` ("Anlatı Tasarımı
**(kanon)**", omurga "Sessiz Gün + Yankı"), `LORE_SECRET.md` ("Sır **Anayasası**",
sır = "Geri Sayan") ve `STORY_CARDS.md` duruyordu — tarihleri **11-13 Temmuz**, yani
hem v1 "Sönen Kandiller"den (29 Tem) hem v2 DEMİRBOYUN'dan (30 Tem) ÖNCE. Başlıklarında
"kanon" ve "anayasa" yazıyordu ve **oyunda çalışan içeriği besliyorlardı**: "Sessiz Gün"
`intro_sessiz_gun.txt`, `elder.txt`, `zehra.txt`, `nuh.txt` ve `journal.c`'de geçiyor —
LORE v2'de ise SIFIR kez. Üçü `docs/konsept/arsiv-*-v0.md` olarak arşivlendi ve
başlarına gerekçeli HÜKÜMSÜZ uyarısı yazıldı (reddedilenler ve sebepleri dahil).
Bu, HANDOVER'a yeni bir çalışma kuralı doğurdu — kanon değişirse dosyalar da değişir.

**KURTARMA TARAMASI.** Dört maddelik ölçüt (DEMİRBOYUN'la çelişmeyecek · oyuncuya
SEÇİLMİŞLİK vermeyecek · fatura refleksi taşımayacak · rol özgürlüğünü bozmayacak).
Hakan **temiz sayfa** seçti: hikaye kartları NPC kadrosundan sonra tekrar bakılacak.
Kesin çöp ve sebepleri: **Sessiz Gün** (§1.9 kıyamet dili + ölçek şişmesi — *"bir tören
yapılamıyor diye kurulmuş dünyanın açılışına 'dünya durdu' konmaz"*) · **"Geri Sayan"**
(fatura refleksi + seçilmişlik + §1.8 + **silme testini geçemiyor**) · göğüsteki mühür
ve "…henüz değil" fısıltısı · amnezi (oyuncunun geçmişini oyun yazıyor) · Tanık/Sömüren
iki AYRI yazılmış hat (fikir değil, üretim kararı olarak çöp: her fazda iki hat yazma
faturası).

**YANKI = BAĞLI (kapsam düzeltmesi).** İlk soru eksik sorulmuştu: "Yankı" bir sahne
fiili değil, `SKILL_ECHO` adında **bir büyü okulu** — 26 kod referansı, 4 beceri
(Fısıltı Patlaması/Gölge Formu/Can Emme/Yankı Çağırma), kendi puanlı ağacı.
`skill.c:62` yorumu koddaki iki gizli sınıfı "Yankı + Suikast" diye sayıyor; kanondaki
ikisi ise "Bağlı + Yedinci Ağız". Suikast→Kısıklar zaten oturuyor. Karar: **okul, ağaç
ve dört beceri KALIR, kurgusu DEMİRBOYUN'a taşınır.**
Ve kodda hazır olan şey tam olarak gizli sınıfın mekaniği: `echoesWitnessed` /
`echoesConsumed` sayaçları, `echoesConsumed >= 3` eşiği, journal + save alanları. LORE
§10'a bugün yazdığımız Bağlı kapısı (*"taşı KIRMADAN kulak dayamak, üç ayrı mühürde"*)
zaten üçe sayıyor; "kaçırılmaz, kirletilir" kuralının kirlenme sayacı da orada.
Todo'daki "Yankıcı P1+P2 hiç test edilmedi" maddesi böylece kapandı — test edilerek
değil, yeniden yorumlanarak.

**AÇILIŞ VİNYETİ YENİDEN YAZILDI.** `intro_sessiz_gun.txt` → `intro_demirboyun.txt`
(dosya adı kanona yalan söylemesin), `main.c:4462` yolu ve `main.c:12506` başlık ekranı
alt yazısı güncellendi. Yeni alt yazı: *"Bir fatih konuşamıyor; adına altı ağız
konuşuyor."* Dört panel, ANLATICI YOK: her satır bir insanın ağzından ya duvardaki
yazıdan geliyor. Panel 2'de kimsenin ADI geçmiyor — oyuncu ilk 60 saniyede isim
öğrenmez, ROL sayar (altı ağız, bir sandalye). Panel 4'te buz ilanı **okunuyor,
alınmıyor** — pano 19. dakikada doğrulama olarak tanışılıyor.
`sfx:` hiç kullanılmadı: motorda yalnız `whisper` ve `heartbeat` var, ikisi de v0
sesiydi (fısıltı = seslenen şey, kalp atışı = dokuz atım).

**ECHO SAHNELERİ (3) DEMİRBOYUN'A ÇEVRİLDİ.** Mekanik dokunulmadı — `fx: witness` /
`fx: consume` / `fx: essence`, düğüm adları ve `opt:` hedefleri aynen. Kurgu değişti:
mor ışık MEKÂNDA (damar taşı, §2), oyuncunun bedeninde işaret yok. Karşılaştırma:
eskisi *"göğsündeki mühür ısınıyor — sanki bir şey nefes alıyor, **senin nefesinle
aynı anda**"*; yenisi *"bir soluk çıkıyor: yavaş, senden derin, **seninkine hiç
uymuyor**"* ve *"**Ses senin tarafına dönmüyor. Senin orada olduğunu bilmiyor.**"*
Aynı mekanik, ters anlam — merak var, seçilmişlik yok.
Sökme dalı uyarı kutusu koymuyor, ima ediyor: *"İki tarla ötede mezarlığın alçak
duvarı görünüyor. **Kuşlar orada oturmuyor.**"* (§9 damar taşı yan etkisi, kodda var.)
Gömme dalı mistisizmden arındı: *"Ellerin çamurlu, sırtın ağrıyor"* + *"Aşağıdan bir
katırcı geçti. **Bir şey demedi, yalnız yavaşladı.**"* — TANIK KURALI'nın küçük hâli
ve teşekkür DEĞİL.

**`k2_sofra.txt` temizlendi** ve sahne İYİLEŞTİ: fısıltı satırı kozmik makineyi içeri
sokup kapanışı zayıflatıyordu. Şimdi *"Han o gece boştu ama ilk kez terk edilmiş
kokmuyordu"* ile bitiyor.

**DİL KONTROLÜ İKİ KEZ ÇALIŞTI** (Hakan'ın açık talimatı: TR+EN yazım denetimi).
Vinyette 10, echo sahnelerinde 10 düzeltme. Öne çıkanlar:
- **"taş su alıyor"** yanlış deyim (o tekne deyimi) → *taş su ÇEKER*. Aynı aile:
  taş **çıkar/sökülür** ("gelmez") · tekerlek **gömülür** ("saplanmaz") · güneş
  **batar** ("inmez") · *katır sürücüsü* → **katırcı** (mesleğin gerçek adı).
- **"Asılan: yok"** tebeşir notu *"asılan (adam) yok"* diye okunuyordu → **"Asılı
  sancak: yok"** ve böylece §1.4'ün sancak SAYMA motifine bağlandı.
- Tellal **"okunacaktır"** diyordu; kanon taç giyenin adları **kendi sesiyle
  söylediğini** yazıyor → **"söylenecektir"**. Dil geçişinde çıkan kanon hatası.
- **"kimseye bakmadan"** üç panelde üç kez (§1.2b kalıp tekrarı + üçleme) → biri
  korundu, ikisi değişti; panel 4'te *bir ses* için zaten mantıksızdı.
- **Üç üçlemeden biri kesildi, ikisi bırakıldı** ve gerekçesi yazıldı: kesilen
  *"duymuyor, sormuyor, beklemiyor"* — ortadaki hem soyut hem ritmi AI kalıbına
  düşürüyordu; bırakılanlar son öğenin cinsini değiştirip kalıbı kırıyor.
- **Mantık hatası:** isim duvara *"çakıyla"* kazınmış ama çıkındaki çakı TAHTADAN.
  Alet belirsiz bırakıldı; tahta çakı çocuğun oyuncağı olarak kaldı.
- **LORE §0'ın kendi açılış cümlesinde ihlal:** *"sandalyesi**, ve** onun"* — Türkçede
  virgül + "ve" olmaz. Uzun çizgiyle düzeltildi, duraklama korundu.
- Çeviri kararları: `bey` ve `akçe` ÇEVRİLMİYOR ("lord" bey/kral ayrımını öldürür) ·
  `damar taşı` → **veinstone** ("bloodstone" gerçek mineral, ayrıca "kan" yanlış
  anlam — damar ejderhanın) · **"beyimiz" / "Bey" ayrımı TEKLEŞTİRİLMEYECEK**: Rüstem
  ve hekim "beyimiz" diyor, Şirin "Bey" diyor ve bu bir karakter beati (§7 — Şirin
  onun ADINA bir şey istemiyor).

**KALAN BORÇ (açıldı, kapanmadı):** `ritual.txt` (taş halkası ritüeli — Yankı okulunu
açıyor, KALMASI gerekiyor ama kurgusu revize edilecek) · `ruya_muhur/cukur/fisilti.txt`
(rüya vinyetleri — kurtarma taramasında ÇÖP: *"yabancının hayatını izlemek =
seçilmişlik + fatura"*; `main.c:3791`'de üçü birlikte çağrılıyor) · `kayit_odasi.txt`
("dokuz atım") · `elder/zehra/nuh.txt` + `journal.c`'de "Sessiz Gün" atıfları. Bunlar
Bağlı hattının kurgusuyla birlikte, NPC kadrosundan sonra çözülecek.

**AÇILIŞ GÖRSELLERİ ÜRETİLDİ — asistanın kendi hatası düzeltildi.** Hakan'a "dört
PNG'yi ben üretemiyorum, görsel üretme aracım yok" denmişti. **YANLIŞTI:**
`scripts/gen_intro.py` ve `gen_diorama.py` sahne panellerini PROSEDÜREL üretiyor —
saf Python (`zlib` + `struct`), dış bağımlılık yok, mevcut PNG'lerden parça kesip
`Canvas.blit/rect/glow/vignette` ile birleştiriyor. Yeni üretec yazıldı:
`scripts/gen_intro_demirboyun.py` → `intro_{bitmemis,sandalye,tarih,disari}.png`.
Kaynak: House_1 limestone duvarı, dungeon_arch, altı NPC sayfası (karartılarak silüet).

İki tur revizyon gerekti ve ikisi de göze bakarak yapıldı (üretilen PNG'ler okundu):
- **Altı çivi görünmüyordu** (2×3 px, meşgul taş dokusunda kayboluyordu) — oysa
  panelin BÜTÜN amacı §1.4'ün sancak sayacı. Çözüm: arkasına düz bir sıva bandı,
  çiviler 3×5 px parlak metal + eğik baş + gölge + zayıf parlama. Şimdi sayılabiliyor.
- **Duvar şeridi tekrarı** aynı kapı/pencereyi yedi kez basıp kolonad gibi
  okunuyordu — her kolonda kaynak ofseti değiştirildi.
- Panel 3'te üç çizik 1 px'ti, 2 px kırmızıya çıkarıldı; panel 2'de oturan adam
  tahtın gölgesinde kalıyordu, sandalye sağa alınıp adam aydınlatıldı.

`gen_intro.py` **HÜKÜMSÜZ damgası aldı** (silinmedi, §15.8): başına neden hükümsüz
olduğu ve yerine ne geçtiği yazıldı. Çıktıları (`intro_frozen/pulse/memory/wake.png`)
duruyor ama artık hiçbir sahne onlara başvurmuyor; `intro_pulse.png` kurtarılamaz
(eşmerkezli mor halka = mühür atımı).

**Bu görseller YER TUTUCU.** ERW asset paketi gelince yeniden üretilecek; panel başına
ressam briefi sahne dosyasında duruyor. **Taht salonu inşası kararı GEÇERLİ** ve ayrı
bir iş olarak duruyor — §1.4 sancak sayacı dünyada da gerekiyor ve vinyetin cine
moduyla canlı mekânda geçmesi hâlâ daha iyi. Ama artık siyah ekran engeli yok.
ERW'de taht salonu/kale iç mekânı YOK (en yakını yoldaki "The Headquarters"); salonun
ayırt edici parçaları prop (iskele, vinç, altı çivi), tileset değil — düzen/çarpışma/
kamera şimdi yapılır, karo kimlikleri paket gelince değişir.

### Faz ANLATIM-STANDARDI — "görev değil, hikayenin parçası" (2026-07-31, TASARIM)
Kod yazılmadı; yedi ONAYLI kural GDD §1.2c'de. Uygulama sıradaki fazlarda.

**Ölçüm önce yapıldı.** `ContractDef` yalnız `targetMob`/`fetchItem`/`count`/`gold`/
`rep`/`tier` taşıyor — dokuz kontrat, hepsi kes-ya-getir. `QuestDef` yalnız KILL ve
FETCH. Ama görev günlüğü metinleri niyeti ZATEN taşıyor: *"Baran kazanı için üç
kızılot istedi. Tüccar dediğin önce seni tartar — **otlar bahane sanki**."* Yani
anlatı bir BOYA, altındaki yapı sayaç. Buna karşılık K9/K2/S3/Suikastçi zincirleri
karar+tanık+kalıcı sonuç üstüne kurulu — **oyunda iki ayrı kalite vardı.**

**MİMARİ SORUSU YANLIŞ SORULDU (asistan hatası).** Ajana "hikaye anlatım mimarisini
tasarla" denildi; ESO/WoW zinciri kararının ZATEN alınmış olduğu (GDD §1.2, ESO
sunum v2 KODDA) söylenmedi. Ajan görev sistemini yeniden icat eden üç mimari önerdi
(Dosya / Talep / Sahne). Hakan düzeltti: *"ESO ve WOW gibi görev zincirleri
yapacaktık. Sunum ESO gibi olacaktı ama kendi tarzımızı katabiliriz."*
Mimari değişikliği REDDEDİLDİ; ajanların ürettiği kurallar zincirin İÇİNE giren
kalite kuralları olarak alındı.

**Teşhis (ajanın en iyi cümlesi):** yapı `kabul → sayaç → teslim`; sorun ortada
değil İKİ UÇTA. "Kabul" oyuncuyu memur yapıyor, "teslim" işi kapatıp dünyayı
değiştirmiyor. **Sayaç suçlu değil; sayacın etrafındaki tören suçlu.**

**Yedi karar** (ayrıntı GDD §1.2c): TANIK KURALI bağlayıcı · sayaç hedefse görünür
yöntemse gizli · kabul adımı zincirde var panoda yok · her zincir bir iz bırakır
(phasing) · duyum kanalı (`AmbientHook`), ünlem işareti en zayıf kanal · her işin
ikinci doğru tarafı KONUŞABİLİR (**yeni mekanik onaylı:** `mob.isTalkable` +
`dialogFile` + hostile→neutral) · kes/getir silinmez, hedef olmaktan çıkar.

**KENDİ TARZIMIZIN ADAYI: phasing.** ESO dünyayı değiştiremiyor (paylaşımlı dünya),
WoW phasing'i o kısıtın etrafından dolanan bir hile — oyun OFFLINE ve tek oyunculu
olduğu için bizde kalıcılık BEDAVA. Altyapı da hazır (storyFlags, dekor, bölgesel
fiyat çarpanı, dünya olayları) ve zaten çalışan bir örnek var: Nazife'nin sofrası
K2'den sonra sonsuza dek kurulu kalıyor.

**Üç somut dönüşüm yazıldı** (zincir senaristi, uygulanmayı bekliyor):
- "Goblin sorunu" → **Çivi Hesabı.** İş panoda değil: 06-08 iskelede kalfa
  bağırıyor, direkte tebeşir çentiği tutuyor. İki hırsız var ve **biri diğerini
  aklamıyor** — goblinler de çalıyor, Nazlı da. Sayaç HUD'da değil tebeşirde;
  kalıcı iz iskelenin bir katı + surda bir sıra daha meşale.
- "Baran'ın Kazanı" → **Ağrı Şerbeti.** Revirdeki gazi iş vermiyor HABER veriyor.
  Baran şerbeti parasız olana veriyor (tezgâh altında on yedi isim). Kızılot
  mezarlıkta tarla gibi — gece çok bulursun, ölü kalkar. İkinci alıcı altı kat
  veriyor ve HAKLI (sefer çıkarsa ağrı ilacı üçe katlanır). Satarsan izin yine
  verilir ama "evlat" biter, "müşteri" başlar.
- "Geçit temizliği" → **Geçidin Bacı.** Geçit ağzında üç yüklü araba bekliyor
  (manzara sayaç). İki kamp: gerçek orklar VE terhis edilmemiş on bir aç adam —
  anlaşmalı değil, birbirine muhtaç. Dört geri alınamaz çözüm. Ajanın kuralı:
  **acıma çözüm değil** — Zülfü'yü anlamak geçidi açmıyor, ona maaş bulmak açıyor.

**Kalan altı kontrata çeviri kuralı** ve "BU OYUNA UYMAZ" listesi GDD §1.2c'de.

### Faz KANON-KAPANIŞ — LORE §16'nın yedi açık sorusu kapandı (2026-07-31)
Kod değişmedi (tek istisna: suç defteri, aşağıda). Kararlar `docs/LORE.md`'de.

**Yöntem (kanon üretim kalıbı, HANDOVER'da tanımlı):** 4 paralel ajan yedi soruya
seçenek üretti → her seçenek KENDİ klişe eleştirisini yazdı (üç turun reddedilme
sebebi buydu) → Hakan seçti → kurgucu kanon metnini yazdı → **isim danışmanı**
terimleri düzeltti (TR+EN) → **Türkçe editörü** dil geçişi yaptı → **kanon editörü**
çelişki taradı → bulunan çelişkiler düzeltildi. Metni asistan YAZMADI (Hakan kuralı).

**Yedi karar:** S1 Bey→Kral, engel törende altı şehrin adını sesle saymak · S2 bir
imparator + 99 Ocak, vadeli Kumkapı Senedi · S3 Bağlı + Yedinci Ağız · S4 yedi mühür
KITAYA YAYILMIŞ (6 yeni harita gerekiyor) · S5 Şirin = Takvim · S6 kar sürmek + kalıp
zinciri · S7 yamalı mühür gövdesi + diri bağlı rengi.

**TERİM DÜZELTMELERİ — ikisi gerçek çakışmaydı, üslup değil:**
- "İşaret Okuyan" → **Yedinci Ağız** / *Seventh Mouth*. Sebep: "Okuyan" ZATEN
  Mühürcüler'in ikinci rütbesi (§10 tablosu). Yeni ad odanın aritmetiğinden.
- "Dinleyici" → **Bağlı** / *the Bound*. Sebep: **Skyrim'in Karanlık Kardeşlik lideri
  birebir "the Listener"** — kanonun açık hedefi Skyrim'den ayrılmak, bu ad tersini
  yapıyordu. "Bağlı" hem zinciri hem "bize bağlı çırak"ı taşıyor; kamuflaj çift-anlamı
  İngilizcede de sağ kalıyor (*bound apprentice*).
- "sesle saymak" → **Altı Ad** / *the Six Names*.

**DİL GEÇİŞİ 7 kategoride düzeltme yaptı:** özne-yüklem uyumu ("Ocaklar bildiği için"
→ biliyor), hâl ekleri ("göğsünde işlenmiş" → göğsüne), Türkçede olmayan ", ve" kalıbı,
devrik "sanıyor ki", sözde-terimler ("iki organı var ve birbirini istemiyor" → iki başı
var — organ istemez), gereksiz üçlemeler (§15.4), iç tutarlılık.

**KANON EDİTÖRÜ 6 KIRICI ÇELİŞKİ BULDU — dördü yeni yazılan metinden:**
1. **KORKULUK kendi bölümünde çiğnenmiş.** 2. mühür "kutsama hattını kesiyor"du, ama
   kutsama §1.4'e göre meşruiyetin üç somut nesnesinden biri — yani siyasi düğüm.
   Düzeltme: mahzen çöker, ARŞİV ve ŞİFA hattı kapanır; kutsama KARARI siyasi kalır.
2. **Aynı ihlal 6. mühürde.** "Karduvar kapısı kışın da açık kalır" — ama o kapı Kırca
   Ana'nın tek isteği, Sarp'ın tek derdi, §14 finalinin parçası. Düzeltme: buz üstünden
   yürünemez, kışlık rota tekneyle uzar; kapı siyasi kalır.
3. **7. mühür finali baştan belirliyordu** ("gökte kalıcı ejderha" vs §14 "büyük kemer
   yeniden mühürlenir"). Düzeltme: sayıma ANA KEMERİN İLK HALKASI girer; kemerin kaderi
   sayımın dışında, oyuncunun fiiline bağlı.
4. **Nazlı bir Yalınkılıç varisi çıkıyordu** — Rüstem'in "başka varis yok" sırrını ve
   §7'nin tıkanmasını çökertiyordu. Düzeltme: Nazlı Şirin'in İLK kocasından; Yalınkılıç
   olmadığı için fiyatı var (verilen şey mirasın parçası olmuyor, masa açılmıyor).
5. **Senedin üç kalemi Gündoğdu'nun malı değildi** — sürgün, sahip olmadığı geliri
   kiralamış görünüyordu. Düzeltme: senet VADELİ, kalemler "Hane tahtına oturduğu gün"
   devrolur; bugün Ocaklar çadır sarayının kendi kervan bacını topluyor. Senedin dişi
   böyle daha korkutucu: **fatura henüz kesilmedi.**
6. **Altı Ad kilidi ile Şirin'in ertelemeleri birbirini yiyordu** (imkânsız bir töreni
   üç kez ertelemek). Düzeltme: salon töreni iptal etmiyor, İLERİ TARİH koyuyor; Şirin'in
   ertelediği şey tören değil, "bey yapamaz" cümlesinin kurulacağı gün.

**Orta/küçük düzeltmeler:** 3 "dinleyici" kalıntısı temizlendi · fosil zaten açıktı
(çürüme 1. olayı "dibindeki İKİNCİ kapak" oldu) · Sarkaç'ın çift anlamı (takvim + tepe
rütbe) §2'de açıldı · §3'e "Gündoğdu sürgünü (21 yıl önce)" satırı (Ergün'ün doğumu ile
sürgün aynı yıla düşüyordu — fazla temiz tesadüf) · Karaocak valisi **Sancar** oldu ve
iki tek-kollu adam bilerek bağlandı (Rüstem'in kolunu Sancar kendi kesti; valinin
sikkesini kimsenin sormamasını da bu açıklıyor) · "kırk" enflasyonu (6 kullanım) ikiye
indirildi · §17'ye 4. açık soru: **hoca kim** (Senedi imzalatan, altıncı maddeyi yazan
adsız adam).

**§16 artık "KARDUVAR VERASET TAŞI"; açık sorular yeni §17'de, dört madde.**

### Faz SUÇ-DEFTERİ — bölgesiz haritalarda suç Köy'e yazılıyordu (2026-07-31)
Kararlar GDD §8.2'de. `MapDef.crimeRegion` eklendi; `region_of`'taki elle yazılmış
`if (map_current() == MAP_DOGU) return 5;` istisnası kalktı.

Gerçek hata: dört bölge (Ova/Orman/Bataklık/Geçit) dünyanın tamamını kaplıyor, yani
"kırsal suç" boşluğu yok. Boşluk **bölgesi olmayan haritalarda**: zindan F1/F2, konak,
mezarlık, **loca**. Locada Neriman'a vurunca bounty **Köy'ün** defterine yazılıyordu.

Dört yeni bölge: `REGION_LOCA`, `REGION_KONAK`, `REGION_KANUNSUZ` (zindan), mezarlık
köyün defterine bağlandı. Kanunsuz bölge **sahipsiz bir sicil** — bounty yazılır ama
toplayacak muhafız yok; 7 bounty çağrı yerine "burada suç işlemez" dalı eklemek yerine
bu seçildi (unutulacak kontrol kalmıyor). Bölge indekslerine ad verildi (`REGION_LOCA`
vb.) — çıplak `5` yazmak bu hatayı doğuran şeyin kendisiydi.
`MAX_REGIONS` 8→16; save.c bölge sayacını yazıp okuduğu için format bozulmadı (kontrol
edildi). Muhafız sistemine DOKUNULMADI — `guard.h`'a gerekçeli not bırakıldı.

### Faz KİMLİK — İÇERİK TABLOLARINDA .id + DİYALOG TAVANI (2026-07-31)
Kararlar GDD §8.1'de. Save DEĞİŞMEDİ. **Kanon yeniden adlandırmasının ÖN KOŞULU**
— DEMİRBOYUN'a geçerken adlar değişince zincirlerin kopmaması için önce yapıldı.

**NpcDef.id** — 18 NPC, biçim `<harita>_<yer>_<rol>`. Alt-mekânlar uydurulmadı,
tablodaki yorumlardan alındı (*salon başköşe* → `loca_salon_usta`, *tünel ağzı* →
`loca_tunel_gozcu`, *zehir hücresi* → `loca_hucre_zehirci`, *dükkan önü* →
`tasdurak_dukkan_tuccar`, *han kapısı* → `tasdurak_han_hanci`, *konak önü* →
`tasdurak_konak_katip`). Roller diyalog dosyası + görev bağından, yerler ilk
durağın koordinatından (Yabancı T(27,16) ve Nuh T(60,18) Köy dışında → Yeşil Ova).
13 `SDL_strcmp(NPCS[i].name, "...")` çevrildi; sekiz tarama döngüsü tek
`npc_index_by_id()`'ye indi.

**RecipeDef.id** — 41 tarif, biçim `<istasyon>_<slug>`. Neden istasyon öneki:
**dört tarif adı iki kez geçiyor** (Pişmiş Et, Avcı Yahnisi, Ot Çorbası, Gezgin
Kavurması — kampateşi + simya varyantları) ve `recipe_index_by_name` ilk
eşleşmeyi döndürüyordu, yani o varyantlardan biri FİİLEN ERİŞİLEMEZDİ. Ad bu
tabloda bugün de kimlik değil. İki konumsal tablo girişi designated forma
çevrildi (başa `.id` eklemek onları sessizce kaydırırdı).

**StationDef.ownerName → ownerNpcId** — ad KOPYASI kaldırıldı; yönlendirme
mesajı adı `NPCS[]`'ten okuyor. Kopya kalsa NPC yeniden adlandırılınca oyuncuya
var olmayan bir ad gösterilirdi.

**Bulunamayan id sessiz kalmaz** — `npc_index_by_id`/`recipe_index_by_id` loga
yazar (oturumda bir kez). Bu hata sınıfının bütün maliyeti sessiz olmasıydı.

**Diyalog tavanı** — `DIALOG_TEXT_LEN` 220→512, `DIALOG_OPT_LEN` 80→128.
Ölçüm: 134 replikten **26'sı** (en uzunu 325 bayt) ve **1 seçenek** (84 bayt,
nuh.txt:16) sessizce kesiliyordu — oyuncu yazarın cümlesinin sonunu hiç
görmüyordu. Kesme artık (a) loga `dosya:satır + bayt` yazıyor, (b) UTF-8
karakterini ortadan BÖLMÜYOR (`utf8_trim` devam baytlarından geriye yürür;
bölünmüş karakter geçersiz dizi bırakıp bozuk çiziliyordu).
Panel için ayrı taşma denetçisi YAZILMADI: `draw_markup_wrapped` metni ölçüyor
ve `panelH` ona göre büyüyor (tek sınır `winH*0.6`), 325 bayt ~3 satır — olmayan
soruna denetçi spekülatif olurdu.

**Doğrulama** — denetçi tavanı geçici 220'ye indirilerek test edildi: tam **26**
uyarı bastı, bağımsız statik ölçümle birebir aynı. 512'de 0. (Uyarı: `sed -i.bak`
+ `mv` başlığı ESKİ mtime'ıyla geri koyuyor, make yeniden derlemiyor — ilk ölçüm
bu yüzden yanlış çıktı, `touch` gerekti.)

### Faz UI-LAYOUT — HUD YERLEŞİMİ VERİYE + OYUN İÇİ DÜZENLEYİCİ (2026-07-31)
Kararlar GDD §7.1'de. Save DEĞİŞMEDİ (yerleşim kayda girmiyor).

**Altyapı**
- `vendor/cjson/` (MIT, stb_vorbis deseni: `-w` ile uyarısız derlenir)
- `src/jsonx` — cJSON üstüne üç eksik: (1) JSONC yorumları **boşlukla** değiştirilir,
  silinmez → karakter ofsetleri korunur, hata satırı ORİJİNAL dosyayla eşleşir;
  dize içindeki `//` korunur (`http://` bozulmasın). (2) satır/kolon + hatalı satır +
  "sondaki fazla virgül" ipucu. (3) varsayılanlı getter'lar — eksik alan çökertmez.
- `src/uilayout` — 9 çapa (`sol-ust`…`sag-alt`) + yatay `hiza` (sol/orta/sag) +
  dikey `dikey` (ust/orta/alt) + isteğe bağlı `w`/`h`. Eksik anahtar → kod varsayılanı
  + BİR KEZ uyarı. Bozuk JSON → oyun varsayılanlarla çalışmaya DEVAM eder (test edildi).
- `src/uieditor` — Ctrl+U. Tutamaklar yerleşim VERİSİNİ dolaşır, öğenin canlı olmasını
  beklemez (tok buffunu dizmek için yemek yemek gerekmiyor). Izgara 8/16/32 (G),
  hizalama kılavuzu (başka öğeyle aynı hizaya yapışır + yeşil çizgi — yapışmanın
  SEBEBİ görünmeli), Alt yapıştırmayı kapatır, `E` en uygun çapa, boyut tutamağı,
  `P` ön izleme (seçili panel kapalı olsa bile gerçek içeriğiyle çizilir),
  üst üste binen tutamaklarda tekrar-tıklama alttakini seçer.

**Taşınan 23 öğe** — barlar (can/mana/stamina AYRI), altın, tok, unvan, durum yığını,
iğne şarjı, saat, görev takipçisi, boss barı, görev bandı, bölge splash'i, hotbar,
ok sayacı, minimap (kutu), envanter, 5 panel (dükkan/ilan/beceri/craft/suikast — her
biri kendi genişliğiyle), yakalama paneli.

**Bulunan gerçek hatalar**
- main.c envanter yuva tıklaması `panelY` yerine sabit `20` kullanıyordu → panel
  konumu veriden gelince kesin ayrışacaktı. `340` sabitine ad verildi (`INV_EQUIP_H`
  — panel yüksekliği DEĞİL, ekipman bölümünün yüksekliği).
- Sol-üst kolonda çakışma: altın `y=76` ile iğne "Zehir" `y=78`, tok `y=96` ile
  durum yığını `y=100`. Kolon sıraya dizildi.
- Hotbar konum formülü ÜÇ yere kopyalanmıştı (`slot_rect` + kaldırma hedefi +
  main.c ✕); envanterde BEŞ. Hepsi tek kaynağa indi.
- Düzenleyici açıkken olayı yutmak yetmiyordu: hareket `SDL_GetKeyboardState` ile
  anlık durumdan okunuyor, olay döngüsünü baypas ediyor → oyuncu girdisi
  `!uieditor_active()` ile kapatıldı.
- Minimap kutu semantiğinde 42px yukarı kayıyordu (harita oranı kutuyu doldurmuyor);
  harita kutunun sağ-altına yapıştırıldı, eski konumla birebir aynı.

**Bilinçli KAPSAM DIŞI** — letterbox (düzenlenecek konumu yok, tek değişkeni bant
yüksekliği), envanter/panel YÜKSEKLİKLERİ (satır sayısı ve çanta kapasitesinden
türüyor), can barı GENİŞLİĞİ (max stat'la büyümesi kasıtlı geri bildirim), tam ekran
öğeler, imleci izleyenler, dünyaya bağlı barlar. Hâlâ kodda: beceri AĞACI paneli
(konumu beceri paneline göreli) + diyalog paneli.

**Ek**: çözünme ölümü (mob.c, 14 yatay şerit + hash eşikli dağılma, sağ kalan
şeritler yukarı süzülür) bu commit'te. DEV kancası `PIXE_HIDDEN=1` — duman testi
penceresi ekranda görünmez (SDL_WINDOW_HIDDEN; `SDL_VIDEODRIVER=dummy` çalışmaz,
dummy sürücüde SDL_GPU cihazı açılmıyor).

### Temel (GPU geçişi öncesi tarihçe)
- LPC paper-doll oyuncu (katmanlı ekipman, z-order, oversize slash, silah partikül fx)
- Envanter + ekipman UI, loot döngüsü, pause menüsü

### Faz 1 — Skill & Combat çekirdeği
- Use-based skill sistemi (14 kategori, XP eğrisi 100×level^1.5, K paneli)
- Combat A: silah+STR+skill+varyans+crit hasarı; defans azalan getiri 100/(100+def)
- XP yalnız hasar uygulanınca; düşük seviye mobda azalan getiri

### Faz 2 — Loot & Ekipman derinliği
- Item instance (+0..+8 plus, %10/plus), mob başına loot tablosu + geometrik plus roll
- Gold (popup'ta tıklanabilir hücre), Can İksiri (tür başına CD), başlangıç seti
- Save/load (F5/F9 + menü; alan-alan binary, v6'ya kadar evrildi)

### Faz 2.5 — KO Combat & Hotbar
- Space auto-attack (silah hızı döngüsü), 1-8 hotbar, aktif skill'ler (PS/QS, cooldown)
- Pot bara atanır, bardan içilir; CD perdeleri

### Faz 3 — Dünya
- Tiled CSV harita (200×150, GID-1, culling), 4+1 bölge (isim splash, kamplar, mood)

### Faz 4 — NPC & Diyalog & Quest & Vendor
- 3 NPC (Cute_Fantasy), txt diyalog formatı (+eylemler), KILL görevi, al/sat dükkanı

### Faz 5 — Yaşayan Dünya
- Gün/gece (10dk=24s, karartma, gece: algı×1.5 spawn×2), NPC başına saat rutini (gece eve girer)
- Evler (decor + taban collision), mob leash+hızlı heal, grup aggro, tile çarpışması

### Faz 6 — Ses & UI & Ölüm
- Prosedürel SFX (11 adet) — saf SDL3 stream, SDL_mixer YOK
- Adaptif müzik: 4 prosedürel parça (town/wild/danger/combat), bölge/gece/savaş mood'u, crossfade
- Fog-of-war minimap (M), quest log (J), ölüm ekranı + %10 altın cezası, CREDITS.md taslağı

### Faz 7 — UI Reskin
- Cute_Fantasy teması: 9-grid ahşap paneller, slot hücreleri, butonlar (ui_theme tek nokta)

### Faz 8 — Perk sistemi
- 48 perk (One-Handed + 3 zırh skill'i), eşik 25/50/75/100, 3-seçenekten-1, K panelinden seçim
- Zırh sınıfı + zırh XP'si; kanama/kaçınma/yansıtma/ölüm-kurtarışı/oyuncu-knockback mekanikleri
- Perk hissiyatı: 4 özel SFX + partikül patlamaları (kan damlası, crit kıvılcımı, Gazi közleri)
- F10 debug XP (yayında kaldırılacak)

### Faz 9 — SDL_GPU geçişi (GPU-1 + GPU-2)
- **r2d**: SDL_GPU üstünde kendi 2D renderer'ımız — MSL shader, sprite batch, 2 blend pipeline,
  9-grid, render target, ertelenmiş yıkım, src kırpma. SDL_Renderer tamamen söküldü.
- Oyunun tamamı portlandı (text upload, minimap target pass'leri, glow linear örnekleme)
- Duman testi: r2d_test. Bilinen port hataları düzeltildi (sandık smear, ev boyutları, minimap batch sınırı, fx çizim sırası)

### Faz 10 — GPU-3: Işık & His katmanı
- Shader gece aydınlatması: fs_light MSL + multiply pipeline, piksel başına ambient + 32 ışık,
  dither, altın saat; kaynaklar: ev penceresi / meydan direği / meşale (item) / silah efekti; titreşim
- Screen shake (hasar 0.25s/6px, crit 0.15s/3px) + kırmızı hasar vinyeti (0.35s)
- Gün döngüsü test için 5 dk'ya indirildi (tasarım değeri 10 dk — gametime.h)

### Faz 11 — Suç & İtibar paketi (TAMAMLANDI: Faz 1-4)
- NPC ölümü (kalıcı, save v7), tıklamayla bilinçli saldırı, tanık tespiti, bounty (+HUD)
- Muhafız döngüsü: yakala → öde/karşı koy, leash kaçışı, ertesi gün diriliş; bounty'liyken NPC konuşmaz
- Sneak: C toggle, algı = eğilme × görüş açısı × siper (dekor LoS) × örtü (conceals tile),
  sinsi vuruş ×2, çift kanal XP (süzülme tick + tanıksız suç); TileType.conceals (çimen)
- Hız çarpanı bug'ı düzeldi: player_update tile hızı dış çarpanı (perk/gece/sneak) eziyordu →
  Player.speedMult (perk hızları da fiilen İLK KEZ çalışıyor)
- Mekanik tamamlama (2026-07-10, "eksik adım bırakma" kararı):
  - Dekor genişledi: ağaçlar (siper/LoS, katı taban) + çalılar/saman (conceals, içine eğilerek girilir) —
    Cute_Fantasy'den kırpıldı (assets/decor/); DecorDef'e solidBase/conceals alanları
  - Mob ŞÜPHE sistemi: algı ikili değil — menzilde birikir (yakında ~0.3sn, sınırda ~2sn), dışında söner;
    dolunca aggro + "!" işareti; şüpheli mob oyuncuya döner (bakınır), baş üstü sarı şüphe çubuğu;
    saldırı yalnız alarmda başlar; leash şüpheyi de sıfırlar
- Bilinen bilinçli tercihler: muhafız sneak'ten etkilenmez (bounty sahibini ismen tanır);
  çim (zemin tile) örtüsü yalnız eğilirken işler
- Bölgesel bounty + itibar (save v8): suç işlendiği bölgeye; itibar -100..+100, ödeme düzeltmez,
  suçsuz gün +1 (0'a doğru); vendor fiyat çarpanı, ≤-50 konuşma reddi, ≤-25 görev kapatma

### Faz 12 — Diyalog koşulları & Speech
- Diyalog txt formatına `req:`/`fx:` satırları (speech/gold/rep/bounty koşulları; gold/rep/bounty efektleri)
- Speech zarı: eşik üstü %100, altı (speech/eşik)×%70; başarısızlık = o gün kilit + itibar -1;
  XP: başarı +15, başarısızlık +3; kilitli seçenek soluk + [Speech N - %şans] etiketi
- Muhafız rüşveti [3]: bounty/2, Speech 25; başarı itibar -5, başarısızlık = karşı koyma
- Saldıran mob otomatik hedeflenir (hedef boşken)
- Türkçe karakterler: tüm UI + diyaloglar düzgün Türkçe (PublicPixel glifleri doğrulandı)
- Not: zar kilidi save'e yazılmıyor (F9 save-scum kapısı — bilinçli erteleme)

### Faz 13 — Meslek paketi 1/5: Kaynak toplama
- 3 toplama skill'i (Mining/Herbalism/Woodcutting), 13 düğüm (günlük yenilenme, riskli yerleşim)
- Toplama süreci: araçla savurma döngüsü (vuruş=iş, düğümde bar) / bitkiler el ile (süre bazlı)
- Kademeli hız: 1 + 0.25×(seviye/20); iş: bakır5/demir10/ağaç7 vuruş, otlar 2.5-4sn; demir Mining 15 kapılı
- Araçlar silah slotuna kuşanılır (WEAPON_TOOL, saldıramaz); LPC Hand Tools smash128 katmanı (elde görünür)
- Kesinti: hareket/hasar/diyalog; yarım ilerleme gün sonuna dek düğümde
- Not: araç idle'da görünmez (sheet'te idle yok); bitki toplama animasyonu bulunursa bağlanacak

### Faz 14 — Meslek paketi 2/5: İstasyonlar & Craft
- Demirci Ocağı + Simya Kazanı; erişim ÜÇ kapılı: tanışma görevi (FETCH) + bounty yok + itibar > -25
- FETCH görev türü + görev item ödülü; Demirci Mahir (yeni NPC): 5 bakır → Çekiç + ocak izni; Baran: 3 Kızılot → kazan
- Süreçli craft: ocakta çekiç savurması / kazanda kaynatma; malzeme tamamlanınca tüketilir (iade güvenli)
- Tarifler: külçeler, Bakır Kılıç, Can/Büyük Can İksiri; '+basma' (şans tablosu + Smithing telafisi, plus-1 riski)
- Mana + Stamina havuzları (HUD) — tüketiciler ve stat basma Faz 4'te
- Save v9 (NPC listesi değişti); F10 debug craft test seti

### Faz 15 — Meslek paketi 3/5: Blueprint ekonomisi & Öğretmenler
- 6 blueprint'li tarif (Demir Kılıç/Zırh/Miğfer, Hız İksiri, Zehir Yağı); sağ tık = kalıcı öğrenme
- Kaynaklar: ork/goblin dropları + Demirci satışı (150g) + Baran dükkânı (100g) — "kesmenin anlamı"
- Öğretici parşömenler (%3, moba göre tür, +250 XP); öğretmen dersleri (seviye×15g, %40, günde 1)
- Buff altyapısı: süreli hız + silah kaplaması (Zehir Yağı → geçici POISON weaponFx)
- Animasyonlar: kazanda oyuncu spellcast (PLAYER_CAST) + kazan fokurdama + kabarcık; ocak alazlanması
- Save v10 (öğrenilen tarifler)

### Faz 16 — Meslek paketi 4/5: Kadim Sunak (Enchanting)
- Sunak: Karanlık Orman sınırında (T102,14) sahipsiz harabe — KAPI YOK (keşif ödülü);
  Yaşlı Kemal [sir] düğümü (Speech ≥ 20) yerini söyler; obelisk dekoru
- weaponFx ItemStack instance'ına taşındı (save v11); item_make() zorunlu (compound literal fx siler)
- Rün ÜRETİMİ şanslı: %60 + Enchanting×0.4 (tavan %95), tutmazsa özler yanar;
  BASMA garantili (3 sn tören, mor partikül + spellcast): Pembe (+%8 hasar) / Zehir (4sn %25 DoT, Kanatma ile binmez)
- Özler: goblin Soluk Öz %20 / ork Zehirli Öz %20
- STAT BASMA: 2 slot + kademe I-III; STR/DEX +2, HP +10, DEF +3, MP +8, STA +10 per kademe;
  maliyet: kademe × (1 stat taşı + 2 Demir Külçe); taşlar: mob dropu %2-3 (türe göre) + Baran (Yaşam Taşı 120g)
- Türetilmiş statlar her kare: taban + perk + gear (max can artışı kadar iyileşme); DEF düz defans
- Ekipman paneli: stat rozetleri (mor nokta) + toplam satırı; sunak hedef satırında [Rün: X] + [STAT n]
- Düzeltilen bug'lar: ekipman TAKASI instance siliyordu (rün/stat kaybı); sahipsiz sunak bounty/itibar
  kapısına takılıyordu; perk max-HP hesabı gear HP ile yanlış delta veriyordu; süreç iptali sessizdi
  ("İşlem iptal edildi" eklendi); F efekt önizleme tuşu KALDIRILDI (rün efektini eziyordu);
  Long Sword'un kalıcı PINK tür efekti kaldırıldı (efekt yalnız rün/kaplamadan)

### Faz 17 — Meslek paketi 5/5: Okçuluk & Yay becerileri
- WEAPON_BOW: iki el (kalkan kilidi), hasar DEX ölçekli (+%2/puan), skill Archery (hasar bazlı XP)
- Köklü atış: Space → çekme (LPC shoot 16-19, ~0.9sn, release kare 9); WASD çekmeyi İPTAL eder (ok yanmaz);
  menzil 340px (melee 80) — CD/stamina trigger'da yanar (KO "hedef kaçsa da yanar" kuralı)
- Güdümlü ok: hasar VARIŞTA (crit + sinsi ok ×2 + rün zehir/pembe + aggro + hafif itme); çizgi görseli
- Ok ekonomisi: atış 1 ok; saplananların ~%30'u leşten geri (Mob.arrowsHit); "Ok yok!" + sayaç HUD
- Tarifler (ocak, dizin SONU — save v10 indeks uyumu): Ok x10 = 1 Odun + 1 Bakır Külçe; Yay = 2 Odun + 1 Demir Külçe (Smithing 5)
- Vendor: Yay 180g + Ok 2g; F10: yay + 50 ok + Archery XP
- 5 yay becerisi (Archery merdiveni, hepsi STAMINA tüketir — havuzun ilk gerçek tüketicisi):
  GO(1: ×2.2, 6s, 20) · BO(10: %40 yavaş 3sn, 10s, 15) · GS(20: 140px geri sıçrama, 8s, 25) ·
  YA(30: tek hedefe 3 ok ×0.6, 12s, 30; hedef ölürse yakına) · AO(45: varışta patlama 90px ×0.6, 15s, 35)
- Silah-beceri eşleşmesi: PS/QS↔melee, yay becerileri↔yay ("Yay gerekli!" / CD boşa yanmaz);
  stamina yetmezse "Nefesin yetmiyor!" (CD yanmaz); Mob.slowMult/slowTimer altyapısı
- Envanter 20 → 35 slot (7×5; paneller y=20'ye alındı; save slot sayısını yazdığından uyum bozulmadı)
- ULPC great bow görseli (shoot satırları + gömülü nişan oku); yay yalnız çekmede görünür
  (bu varyantta walk/idle katmanı yok — normal/recurve bow indirilirse idle/walk görseli bağlanır)
- Yayla NPC hedeflenemez (uzaktan tanıksız suç dengesi ayrı iş — ertelendi)

### Faz 18 — Av & Kontrat Panosu & Kamp Ateşi
- 4 hayvan (Cute_Fantasy Animals): Koyun/Domuz/Tavuk pasif — vurulunca "!" + 4sn düz kaçış
  (hız 550 ≈ oyuncu ×1.1; melee köşeye sıkıştırır, yay doğal av aracı); sürü birlikte ürker
  (kamp alarmı kaçışa çevrilir); Yaban Domuzu (Pig_05) orman sınırında saldırgan (HP 90 / hasar 12)
- MobType.passive + dailyRespawn; MOB_FLEE_TIME; hayvan bölgeleri GÜN dönünce dolar
  (spawn_zones_fill artık eksiği sayarak doldurur); kaçış RETURN dokunulmazlığına girmez
- Loot: Koyun/Domuz 1-2 Deri + 1-2 Et; Tavuk 1 Et; Yaban Domuzu 2-3 Deri + 1-2 Et garantili
- Deri seti (ocak, ARMOR_LIGHT İLK item'ları): Başlık 2 Deri/def4, Zırh 4 Deri/def8, Bot 2 Deri/def3
  (görseller mevcut LPC sheet'lerinin deri-kahvesi boyaması — ULPC deri seti gelirse değişir)
- Yemek mekaniği: Pişmiş Et (kazan VE kamp ateşi, 1 Et) → 12sn +3 can/sn (iksire yavaş-ucuz alternatif);
  ItemType.foodRegen/foodDuration + Player.foodRegenRate/foodTimer
- İlan Panosu (köy meydanı, Signs.png): 9 kontratlık havuz, günlük 3 ilan (kademe başına 1);
  kademeler: Temel (herkes) / Güvenilir (itibar ≥10) / Saygın (≥25) — kilitli ???+eşik görünür;
  kes (kabul SONRASI kill sayacı) + getir (teslimde tüketilir); ödül altın + bölge itibarı;
  bounty/dip itibar → "Arananlara iş yok!"; gün dönünce yenilenir; havuz veri tablosu (satır ekle = çeşit)
- Kamp Ateşi: köyde sabit (pano yanı) + oyuncu KURABİLİR — envanterde Odun'a sağ tık,
  3 Odun + 3sn süreç (WASD iptal); tek aktif oyuncu ateşi (yenisi eskisini söndürür);
  ömür kurulumdan 24 OYUN SAATİ (gece yarısı sınırı DEĞİL); kapısız pişirme istasyonu
  (STATION_CAMPFIRE, XP→Alchemy) + gece ışık kaynağı (light_append_campfire, sert titreşim);
  ateş animasyonu HEP oynar; çıkışta söner (save'e yazılmaz — bilinçli)
- contract.c modülü (save v12: pano günü + slot durum/ilerleme; load sonrası lastSeenDay senkronu —
  yoksa günlük blok yüklenen panoyu ezerdi); STATIONS artık const değil (oyuncu ateşi konumu)
- Bilinen küçükler: hayvanlar minimap'te düşman noktası; otlarken gezinmiyorlar

### Faz 19 — Mob fazı 1: Slime ailesi
- 6 tür (Yeşil/Mavi × Büyük/Orta/Küçük; Cute_Fantasy 2 renk kullanıldı, K/S/P yeni biyomlara saklı)
- BÖLÜNME zinciri: Büyük(120/160) → 2 Orta(50/70) → 2'şer Küçük(20/25); yavrular leşin iki
  yanından KIZGIN doğar (aggro+alertFlash); MobType.splitCount/splitChild; mob_die imzası
  mobs/nextMobId aldı; MAX_MOBS dolarsa yavru sessizce doğmaz (backstop)
- Loot YALNIZ Küçük'te (kasa spam'i yok — mob_die loot'u tablo/altın yoksa atlar):
  Jel %80 + altın; Mavi Küçük +%10 Zehirli Öz (sunağa bataklık kaynağı)
- Mavi vuruşu ZEHİRLER: 2 dps × 3sn (Player.poisonDps/poisonTimer; "Zehirlendin!"; süre tazelenir)
- Türe özel gövde boyu (MobType.bodyW/H — Büyük 72×60, Küçük 22×18); sheet: yönsüz blob,
  idle 4 / zıplama 8 / parçalanma-ölüm 8 kare (SLIME_ANIMS makrosu); saldırı = hızlı zıplama
- Slime Jeli (6g) + Yapışkan Tuzağı (kazan: 2 Jel + 1 Ayotu): hotbar'dan ayak önüne kurulur,
  mob basınca 100px alandakiler 5sn %60 yavaşlar; max 4 tuzak (round-robin); zemin jel görseli
- Kamplar HARİTADAN DOĞRULANDI (CSV açıklık taraması — ilk konum gölün içi çıktı!):
  Yeşil T44,20 (domuz otlağı doğusu), Mavi T40,85 (bataklık girişi)
- Not: hammaddeler (jel/cevher/deri) hotbar'a KONMAZ — hotbar yalnız consumable alır (tasarım)

### Faz 20 — Mob fazı 2: İskelet ailesi & mob mermi sistemi & ok kademeleri
- 3 tür (Kayalık Geçit): Savaşçı (HP140/hasar12, 64×64 kılıç savurması) / Okçu (HP90/ok14,
  menzil 280 < oyuncu 340, ~2.2sn tempo) / Büyücü (HP70/büyü22, menzil 240, ~3sn — cam top)
- MOB MERMİ SİSTEMİ: atış anındaki oyuncu konumuna DÜZ hat — yana yürüyerek KAÇILABİLİR
  (oyuncu oklarına bilinçli asimetri: duran hedef yer, hareket eden kaçar); mermi duvara
  takılır → kaya siperi bedavaya LoS mekaniği; isabet melee mob hattından geçer
  (dodge/defans/zırh XP ×0.15); büyü topu mor izli; MobType.attackRecover + projSpeed
- Kamplar (harita doğrulamalı): giriş T118-122,80 (2 Savaşçı + Okçu),
  derin T148-152,95 (Savaşçı + Okçu + Büyücü); gece kuralıyla undead saati
- Kemik ekonomisi: Kemik dropu; parşömenler — Okçu: Archery %4 (TÜRÜN TEK KAYNAĞI),
  Büyücü: Enchanting %4 + Soluk Öz %15; Savaşçı: OneHand %3; scrolls.png +2 hücre (yeşil/mor)
- OK KADEMELERİ (kullanıcı isteği): standart en ucuz, premium'lar sert —
  Ok (1 Odun→10, ×1.0) / Kemik Oku (1 Kemik+1 Odun→10, ×1.15) / Demir Ok (1 Külçe+1 Odun→10,
  ×1.3, Smithing 10); sağ tık = aktif mühimmat, bitince ucuza otomatik düşüş (bildirimli);
  HUD sayacı tür adıyla; kademe çarpanı beceri/sıçrama çarpanlarıyla birleşir;
  leşten geri dönen ok HEP standart (premium uç kırılır — ekonomi koruması)
- Diriliş animasyon satırları sheet'lerde kullanılmadı (ileride "mezardan kalkma" spawn'ı olabilir)

### Faz 21 — DERİN SKILL AĞACI (eşik anketi → puanlı düğüm ağacı; save v13)
- ESKİ sistem söküldü (perk.c silindi, 48 perk düğümlere dönüştü): 25/50/75/100'de 3-seç-1
  yerine GERÇEK ağaç — düğüm/kenar/önkoşul (BİRİ yeter), dallar, keystone'lar
- PUAN: skill başına her 4 seviyede 1 (100'de 25); savaş ağacı ~28-38p → TAMAMI ALINAMAZ
  (bilinçli kıtlık = build kimliği); KADER: TOPLAM seviyenin her 10'unda 1 puan
- SINIF: Küçük 1p / Dikkat çekici 2p / AKTİF 2p (mavi — beceri açar) / KEYSTONE 3p (BEDELLİ)
- 14 AĞAÇ (~200 düğüm, skilltree.c):
  · Savaş/sosyal 7'lisi: OneHanded/Archery/3 zırh/Sneak/Speech (kök + 3 dal + 2 keystone)
  · Meslek mini 6'lısı: Mining/Herbalism/Woodcutting (hız + çift ürün), Smithing (+basma
    şansı), Alchemy (iksir gücü), Enchanting (rün şansı + tören hızı) — 8 düğüm, 12p, dolabilir
  · KADER (karakter-geneli): Beden (HP/stamina/pasif can 0.5/sn) / Zihin (mana + TÜM XP
    +%5/8) / Yaşam (kasa altını/satış/iksir CD); K panelinde mor "* Kader" satırı
- 14 KEYSTONE (güç+feda): BERSERKER, İNFAZCI, KESKİN NİŞANCI↔AKINCI, HAYALET, RÜZGARIN OĞLU,
  YAŞAYAN KALE, FEDAİ, TAŞ BEDEN, ÖLÜMSÜZ, GÖLGE, HORTLAK, GÜMÜŞ DİL, ALTIN AĞIZ,
  AZİM↔ŞANSLI YILDIZ + meslek keystone'ları (DERİN DAMAR, USTA ÇEKİÇ, RÜN HAKİMİ...)
- AKTİFLER AĞAÇTAN: PS/QS + 5 yay becerisi seviyeyle DEĞİL satın alımla açılır →
  hotbar_place_unlocks abilityOpen[] okur; respec kapananları bardan temizler
  (unlockLevel alanı artık kullanılmıyor); başlangıçta aktif yok — normal vuruş taban
- YENİ ETKİ KANCALARI: yay (çekme hızı/menzil/Yaylım+1/Cephaneci/stamina), sneak (algı/
  eğilme hızı/sinsi çarpanı/gece), speech (alış-satış/zar+ceza/kontrat/ders/rüşvet),
  zanaat (toplama-dövme-kaynatma-tören hızları, çift ürün, +basma/rün şansı), kader
  (havuz max+regen, XP çarpanı skill_set_xp_mult, pasif can, kasa altını goldFind)
- UI: düğüm-kenar haritası (ilk tık incele, ikinci tık AL), sınıf renkleri, puan başlığı;
  RESPEC yalnız sunak yakınında: harcanan×50g
- DoT AYRIŞMASI (kullanıcı kararı): zehir + kanama ÜST ÜSTE BİNER (mob'da ayrı kanallar,
  kırmızı damla + yeşil kabarcık) — tam DoT build'i meşru (Kan dalı %75 + zehir %25)
- Sonraki dalga notu: Lockpicking/Pickpocket mini ağaçları MEKANİKLERİYLE gelir
  (zindan + suç genişlemesi); Two-Handed/Block ağaçları item'larıyla

### Faz 22 — BÜYÜ SİSTEMİ (silahtan bağımsız büyüler + Asa + INT + zırh ağırlığı)
- ÖZGÜRLÜK İLKESİ (kullanıcı kararı): büyü SİLAHTAN BAĞIMSIZ — kılıçlı büyücü meşru;
  her zırh sınıfı serbest ama BEDELLİ ("her seçim bir vazgeçiştir")
- 2 YENİ SKILL (save-toleranslı append): **Destruction** (büyü hasarıyla gelişir, tam ağaç:
  3 dal + 2 keystone + 6 AKTİF düğüm) + **Staff** (asa melee'siyle, standart ağaç) → 16 ağaç
- BÜYÜLER (aktif düğümle açılır; Ateş Topu HER ZAMAN açık — büyünün "normal vuruşu"):
  · **Ateş Topu** (15 mana/2s): güdümlü mermi + %30 yakma DoT'u (yanma kanalı AYRI — zehir/
    kanamayla ÜST ÜSTE BİNER, tam DoT build'i)
  · **Buz Novası** (20/6s): çevre 120px hasar + %50 yavaş 4sn
  · **Yıldırım** (30/8s): tek hedef anında yüksek hasar
  · **Alev Zırhı** (25/20s): 15sn — vurana +8 yansıtma, yan alev aurası
  · **Büyü Kalkanı** (25/25s): 12sn hasar HP yerine MANADAN (×1.2 bedel); mana biterse
    kalan cana + kalkan KIRILIR; melee/ok/muhafız emilir, zehir DoT'u deler; buz kubbesi
    görseli (oluşum→dönme→kırılma)
  · **Toprak Dikenleri** (22/10s): çevre 130px hasar + %65 yavaş 1.5sn; yerden fışkıran
    diken animasyonu (zemin katmanı)
- HASAR FORMÜLÜ (kullanıcı kuralı: "MP sadece resource"): baz × INT (+%2/10 üstü puan) ×
  Destruction seviyesi × silah çarpanı (asa spellPower + PEMBE rün ×1.08) × ağaç × varyans;
  DEX krit geçerli — statlar/rünler/silah HEPSİ ölçekler (combat_spell_hit)
- CAST: 0.45sn köklenme (castSpeedMult ölçekler), hareket BOZAR ("Büyü bozuldu"),
  mana commit'te yanar; hedefli büyüler (Ateş Topu/Yıldırım) Z hedefi + 300px menzil ister
- ASA (craft: 2 Odun + 1 Soluk Öz, Smithing 5; vendor'da da var): iki elli; Space =
  Ateş Topu cast'i (mana/menzil yoksa melee dürtme fallback — "Space silahın base atağı"
  kuralı); melee'si SKILL_STAFF XP'si verir; spellPower 0.20; thrust animasyonu (satır 4-7)
- INT STAT: Bilgelik Taşı (sunak töreni, iskelet büyücü %3 drop, scrolls.png cyan hücre)
- ZIRH AĞIRLIĞI (kullanıcı tasarımı): ağır 5 / orta 2 / hafif 1 puan; taşıma = STR−10;
  aşım × %2 hız cezası (tavan %30) + mana rejeni çarpanı (ağır −%6, orta −%3, taban 0.4) —
  STR'siz ağır zırh büyücüsü YAVAŞ + mana rejensiz (bedelli özgürlük)
- MANA İKSİRİ ayrı içecek (40 mana, 10s CD; Alchemy: 2 Ayotu); envanter 32→40 slot (8×5)
- VFX (4 yeni paket, assets/fx/): ateş küresi mermisi (Sog), arcane patlama additive
  (Yıldırım + cast parlaması), yanan mobda 12-kare alev + Alev Zırhı yan alevleri (flames),
  buz kalkanı kubbesi + toprak dikenleri (magic_pack); iskelet büyücü mermisi mavi küre oldu
- DEBUG: F10 → asa + 6 INT taşı + 5 mana iksiri + havuzlar full + **her ağaca 99 puan**
  (tree_set_debug_points — kaydedilmez, restart'ta sıfır; yayında F10 ile birlikte gider)
- Gelecek dalga notu: magic_pack kalanları (tornado/firelion/icetacle/lightningclaw) yeni
  büyüler; snake/turtleshell sprite'ları summon/mob adayı; magic_icons.png ikon sistemine

### Faz 23 — ZİNDAN PAKETİ (çoklu harita + kilit/tuzak + boss; save v14-16)
- **Çoklu harita** (worldmap.c): harita id + geçit tablosu (E ile geçiş, kapı/merdiven
  görselleri); geçiş akışı CSV yükle + mob/loot/partikül temizle + kamp doldur + oyuncu varış
  + splash + ambiyans. Dünya-özel sistemler zindanda kapalı (NPC/dekor/kaynak/istasyon/pano/
  muhafız/bölge). Save v14: aktif harita. Minimap çift keşif tamponu (dünya kalıcı, zindan geçici)
- **Eski Geçit** (Karanlık Orman T145,7 taş kemer; Osman [haber] işaret eder): 2 kat el yapımı
  CSV (scripts/gen_dungeon.py — K1 44×34 Dungeon_1 iskelet aileleri, K2 40×30 Dungeon_2 boss).
  TILE_TYPES 6-10 (zindan döşeme/duvar/void). Sabit loş ambiyans + titreşimli duvar meşaleleri
  (light_collect staticLights bayrağı), DANGER müzik
- **Lockpicking** (lockpick.c + mini ağaç 8 düğüm): zamanlama barı minigame (3 kademe;
  bölge/imleç skill+ağaçla ölçeklenir, görsel=isabet hesabı); başarısızlık maymuncuk kırar +
  260px GÜRÜLTÜ (mob şüphesi dolar — sistem sinerjisi); KİLİT FISILDAYAN keystone.
  Maymuncuk: Baran 15g + ocak (1 Demir Külçe→3). Sandık E ya da sol tık + baş üstü ipucu
- **Zindan sandıkları** (6, günlük yeniden kilitlenir, save v15): kilitsiz/Basit/Sağlam/Usta
  kademe + renk rozeti; Loot.kind/lockTier/chestId; Chest_anim görseli; chest_fill loot taslağı
- **Tuzaklar** (trap.c): periyodik dikenler (uyarı evresi, 15 zırh-delici hasar) + basınç
  plakası/duvar arbaleti (kaçınılabilir bolt 20 hasar, mob mermi havuzu üzerinden)
- **İskelet Lordu** (boss, save v16): 3.4× mor Savaşçı, HP 900/kılıç 28; Faz 2 (%50)
  ok yağmuru + iskelet çağrısı; arena tile kapısı ("Kapı kapandı"); mor boss barı;
  haftalık diriliş; ilk ölümde **Lord'un Kılıcı +2** (%5 can çalma — ItemType.lifesteal)
- **ÖLÜM = WIPE**: zindanda ölünce dünyaya çıkış (eski "zindan içinde köy koordinatı" bug'ı
  çözüldü); tüm moblar tam canla başa döner (boss dahil); dünyada ölünce kamplar tazelenir

### Faz B1 — AÇILIŞ PAKETİ (başlık ekranı + ana menü + slot sistemi + Sessiz Gün; save v18)
- **AKIŞ**: atmosferik başlık ekranı ("PIXE" + fısıltı + nabızlı "bir tuşa bas") →
  ANA MENÜ (Yeni Oyun / Yükle / Çıkış) — dünya artık menüden önce başlamaz
- **SLOT SİSTEMİ** (kullanıcı: "1 PC'de birden çok oyuncu — kısıtlama"): 20 manuel +
  1 otomatik slot (`saves/slot_NN.dat`, `saves/auto.dat`; .gitignore'da); save v18
  başına ÖZET BLOĞU (toplam seviye/gün/altın/harita — save_read_summary tam yükleme
  yapmadan slot kartını doldurur); dolu slota yeni oyun = üzerine yazma onayı
- F5/F9 = AKTİF slot (girilen/kaydedilen slot aktif olur; auto'dan yüklemede aktif yok);
  OTOMATİK KAYIT: her şafak + her harita geçişi ("Otomatik kayıt" bildirimi)
- ESC menüsü: Devam / Kaydet (slot) / Yükle (slot) / Ana Menüye Dön (kayıpsız uyarısı) / Çık
- **YENİ OYUN RESET**: playerTemplate şablonu (init'le tek kaynak) + tüm dünya durumu
  sıfırlanır (mob/loot/quest/keşif/yankı/sandık/boss/pano/tarif/debug puanları);
  minimap_reset_explored eklendi
- **SESSİZ GÜN AÇILIŞ VİNYETİ** (scripts/gen_intro.py, 4 panel): donmuş dünya (havada
  asılı tavuk/yapraklar) → dokuz atım (iç içe mor nabız halkaları) → başkasının anısı
  (BULANIK: kazan eller, çukur, bakan şey — tek geçiş blur) → uyanış (şafak + göğüste
  mühür). Metin: assets/scenes/intro_sessiz_gun.txt
- **KÖYE VARIŞ**: yeni oyun spawn'ı köy doğusu yol kenarı (T24,8) — araba yankısının
  yanından geçilir (öğretici doğal yerde); meydanda MUHAFIZ OTOMATİK DURDURUR:
  3 kişilik cevabı → GİZLİ SAYAÇLAR (persKaba/Kibar/Ketum, save v18; dialog fx/req:
  `kaba/kibar/ketum` değişkenleri) — ileride NPC'ler oyuncuyu "tanır"; kibar +2 itibar
- **KEMAL KANCASI**: goblin görevi ödülü sonrası "Eski Geçit iması" düğümü
  (hayvanlar o yöne gitmiyor + gece uğultusu + "mühür bir an ısınıyor")
- **GEZGİN ÇIKINI** (kullanıcı: zırhsız başlangıç iade riski): Uzun Kılıç + DERİ SET
  (başlık/zırh/bot = 15 def, hafif → ağırlık cezasız) kuşanılı + 3 Can İksiri +
  2 Pişmiş Et + Meşale + 50g
- **ARABA GÖRSELİ** (kullanıcı tespiti: pakette cart sprite'ı YOK): gen_diorama.py'a
  piksel araba çizimi — devrik (havada dönen teker + yatık teker + kopmuş ok) ve
  sağlam (anı paneli) varyantları; 4 yankı paneli yeniden üretildi
- **SAHNE TUŞ KİLİDİ** (kullanıcı: immersion): vinyet açıkken TÜM panel/oyun tuşları
  yutulur — yalnız 1-8 (seçim), E/Space (typewriter hızı), ESC (menü) çalışır

### Faz B2 — YOLCULUK GÜNLÜĞÜ (J = kitap; birinci ağız + bildirim dili; save v19)
- **KİTAP PANELİ** (journal.c, Book_UI parşömen 4×): J artık liste değil kitap açar;
  iki yer imi şeritli sekme, [Tab] sekme / [←→] yaprak / [J] kapat; metin sayfalara
  kendiliğinden akar (TTF_GetStringSizeWrapped ölçümüyle sütun taşması)
- **GÖREVLER sekmesi**: üstlenilen işler BİRİNCİ AĞIZ (QuestDef.journal alanı) +
  durum satırı "( 3/5 )" / "Sözü yerine getirdim" / "Tamamlandı" (soluk mürekkep)
- **GÜNLÜK sekmesi** (kronolojik, "Gün N —" damgalı): uyanış (JE_WAKE, yeni oyunun
  ilk satırı) · görev alma/bitirme · YANKILAR (Tanık/Sömüren tonu seçime göre —
  sayı asla yazılmaz, sadece his) · yazıt ilk okuma · zindana ilk adım · boss ·
  ölümler · GÜN DÖNÜMÜ atmosfer satırları (3/7/14/21/30 — seyrek, spam değil)
- **KAYIT MODELİ**: girişler KOD+PARAM+GÜN (JournalEntry, JOURNAL_MAX 96) —
  metin render'da türetilir (dil değişse eski save doğru okunur); save v19
- **BİLDİRİM DİLİ** (ikisi bir arada): "Üstlendim: X" / "Söz yerine geldi —
  haber vermeliyim" / "Sözümü tuttum." kişisel + "+100 Gold"/"+Çekiç" sistemik
- Eski quest_log_render kaldırıldı (quest.c)

### Faz E1 — MOB EKOLOJİSİ ÇEKİRDEĞİ (KO slot spawn'ı söküldü; save v20)
- **YUVA/POPÜLASYON** (ecology.c, 26 saha: eski kamplar + aile başına boş göç
  sahaları): ölen mob GERİ GELMEZ; her şafak yuva +1 doğum (tavana kadar,
  yuva noktasından — göz önünde belirme yok); sürekli respawn dünyada KAPALI;
  popülasyon save v20'de kalıcı; ölüm WIPE'ı artık MAX'a değil MEVCUT nüfusa döner
- **AİLEYE GÖRE YIKIM** (kullanıcı: tekdüze olmasın):
  · GOBLİN: **Goblin Reisi** [MOB 16] (2.8×, kızıl, 260 HP) — ölünce kamp ÇÖKER,
    kalanlar kaçışır (Mob.scatter: kaç + sessiz despawn)
  · ORK: **Savaş Totemi** [MOB 17] (yapı-mob: isStructure, 300 HP, kımıldamaz;
    nests.png hücre 0) — vurunca kamp savunmaya koşar (grup aggro), yıkılınca çöker
  · SLİME: renkli sızıntı gölcüğü (zemin görseli, family tint) — hepsi ölüyken
    [E] kurutma (MEŞALE gerekir)
  · İSKELET: lanetli mezar görseli — hepsi ölüyken [E] MÜHÜRLEME (taş kalıcı
    mühürlü görünüme döner; camgöbeği motif = intro mühür dili); mühür GÖÇ ÜRETMEZ
  · HAYVAN: sürü modeli — tükenirse çayır boşalır, sürü göçer
- **GÖÇ**: çöken yuvanın ailesi 5-15 gün sonra BOŞ alternatif sahaya koloni kurar
  (pop 2'den başlar); iki saha da yok edilirse aile o dünyada TÜKENİR (bedel)
- **MÜHÜR İSTİSNASI**: zindan ekolojiye dahil değil — giriş başına tazelenme sürer
- Her temizlik günlüğe birinci ağız düşer (JE_NEST_CLEANSE ×4 metin)
- Kapasiteler: MAX_MOB_TYPES 16→24, MAX_MOBS 48→64 (taşma bug'ı — reis görünmüyordu)
- Bilinen E1 sadeleştirmeleri: koloni dünya YENİDEN KURULUNCA görünür (zindan
  gidiş-dönüş/ölüm/yükleme tetikler); slime bölünme çocukları ebeveyn zone'unu
  paylaşır (pop 0'da klamplenir — ritüel yine tüm ölüleri ister)
  → İKİSİ DE E2'DE KAPANDI (aşağıda)

### Faz E2 — MOB RUTİNLERİ & HABER AĞI (moblar yaşıyor; save v21)
- **GECE/GÜNDÜZ RİTMİ** (kullanıcı seçimi): goblin+reis GECECİ — gündüz kampta
  uyuklar (baş üstü soluyan "z", algı ×0.35); hayvanlar GECE uyur (otlamaz);
  orklar NÖBETLİ — uyumaz, gece yalnız yarısı devriyede (id çiftliği)
- **UYUYANA SİNSİ VURUŞ** (kullanıcı seçimi): uyuyan moba melee/ok vuruşu
  eğilmeden de sinsi sayılır — mevcut backstabMult + Sneak XP (gece baskını build'i)
- **DEVRİYE**: kamp mobu boştayken yuva çevresinde nokta nokta tur (yarı hız,
  patrolAngle/Timer); şüphe/alarm devriyeyi keser — "slotta bekleme" öldü
- **SU NOKTASI**: ova sürüleri (koyun/domuz) öğlen 10-14 göl kenarına iner
  (NestSiteDef.waterX/Y → mob'a atanır) — avcıya tahmin edilebilir pusu penceresi
- **HABER AĞI**: koloniler panoda "~ Söylentiler ~" bölümünde 7 gün duyurulur
  ("Goblinler Yeşil Ova taraflarına yerleşmiş" — ecology_recent_colonies +
  region_at); yoksa "Ortalık sakin... şimdilik."
- **DİNAMİK KONTRAT** (kullanıcı seçimi): nüfusu tükenen türün kes-kontratı
  çıkmaz — aynı kademeden geçerli ilan geçer; hiç yoksa "Bugün ilan yok"
- E1 kapanışları: koloni DÜNYADAYKEN de şafakta belirir (ecology_dawn zone
  kaydı + anında spawn); slime bölünme çocukları ebeveyn yuvasına sayılır
  (ecology_note_spawn — gölcük ritüeli çocuk canlıyken açılmaz)
- Save v21: NestState.colonizedDay (söylenti süresi)

### Faz CİLA — his + kullanılabilirlik paketi (roadmap YAKIN #1)
- **TOOLTIP** (tooltip.c): envanter/ekipman/dükkan hücresinde tam detay —
  ad(+plus), tür, hasar/defans (instance), rün/stat, tüketim etkileri, değer;
  KUŞANILIYLA KARŞILAŞTIRMA (yeşil ^/kırmızı v fark satırı); ekran içinde çevrilir
- **DRAG&DROP** (sol tık — sağ tık yolları duruyor): çanta içi taşı/takas,
  çantadan ekipmana kuşan, ekipmandan çantaya çıkar, tüketilebilir/tuzağı
  hotbar'a sürükle; yarı saydam ikon imleci izler
- **İKONLAR** (assets/icons/abilities.png ← gen_ability_icons.py): hotbar SKILL
  slotları + ağaç AKTİF düğümleri — büyüler magic_icons.png'den (64→32
  piksel-korumalı), kılıç/yay prosedürel glif
- **MÜZİK ×3** (gen_music.py artık repoda): MOOD_DUNGEON (dron+çan; zindan
  haritaları DANGER'dan ayrıldı), MOOD_NIGHT (gece açık arazi), MOOD_MENU
  (52bpm mühür nabzı — başlık ekranı teması)
- **AMBİYANS** (ambience.c + gen_ambience.py): gündüz kuş/rüzgar, gece cırcır,
  zindan uğultu/damla — müziğin altında, 2sn crossfade, menüde susar
- **SFX VARYASYON**: vuruş/krit/kaçınma/yansıtma/loot ±%8 perde oynaması
  (SDL_SetAudioStreamFrequencyRatio)
- **AYARLAR**: ESC + ana menüde Müzik/SFX kaydırıcıları → settings.cfg (kalıcı)
- **FIX (E2 regresyonu)**: savaş müziği MOB_CHASE'e bakıyordu — devriye/su
  ziyareti tetikliyordu; artık yalnız AGGRO moblar
- **MÜHÜR DALGASI** (kullanıcı tespiti: zindanda 3sn'de respawn): sürekli
  respawn + gece ×2 SÖKÜLDÜ (spawn_zones_update silindi) → her 120sn
  (SEAL_WAVE_SECONDS) toplu diriliş; 3sn önce "Mühür titriyor..." + fısıltı +
  sarsıntı; oyuncuya 280px'ten yakın doğum yok; girişte sayaç sıfırlanır;
  BOSS dalga dışı (haftalık diriliş — farm yok)

### Faz D1a — SAHNE ARAÇLARI + K9 KANDİL (ilk hikaye kartı oyunda; save v22)
- **SAHNE ARAÇLARI (Faz C — 4'ü de geldi):**
  · BARK: npc_bark(i, metin, sn) — baş üstü balon, son 0.5sn solar
  · CINE MODU: letterbox bantları + kamera hedefe süzülür (cineX/Y/Timer);
    oyuncu girdisi kilitli (boş tuş tablosu), dünya akmaya devam eder
  · NPC YÜRÜTME: npc_walk_to/walk_done/walk_cancel — rutini ezer (S3 için hazır)
  · VİNYET ZARI: sahne dosyasında `roll: speech 20 -> fail_id` (opt'a bağlanır);
    [Speech N] etiketi; diyalog zarı eğrisi; scene_set_speech köprüsü
- **K9 KANDİL** (STORY_CARDS K9 — en düşük maliyetli 🜁):
  · Fatma NPC (Bartender_Katy): gündüz meydan, 17:00'de geçit kapısına yürür
  · Kandil objesi (lantern.png + light_append_point) geçit kemeri önünde
  · Kerem yankısı: F2 nöbetçi odası T(20,6) — ECHOES[2]; Tanık (çocuğun kararı) /
    Sömüren ("kandilin nedenini eskisi kadar iyi hatırlamıyorsun") — 3 panel (gen_k9.py)
  · Karar (yankı sonrası Fatma diyaloğu, `k9:N` hedefleri): GERÇEK → tapınak ateşi
    (büyük mavi ışık + kandile cine + bark) / YALAN → kandil söner / SUS → yanar;
    kalıcı (storyFlags[SF_K9_KANDIL], save v22) + günlüğe 3 ayrı metin (JE_K9_CHOICE)
    + Fatma'nın kapanış diyaloğu seçime göre (son1/2/3)
- **DOĞALLIK FİXİ** (kullanıcı tespiti): konuşulan NPC artık DURUR —
  npc_set_talking(dialogNpc) her kare senkron; diyalog bitince rutine döner
- Altyapı: player.storyFlags[16] (kart durum kodları — save v22), light_append_point

### Faz D1b — K2 SON MÜŞTERİ (Sömüren'in ilk somut bedeli)
- **HAN** (köy GD köşesi T17,10 — House_1 red) + **Nazife** (Chef_Chloe): gündüz
  kapıda, 19:00'dan sonra sofra başında
- **SOFRA**: gece kurulur (gen_k2.py masa sprite'ı + mum ışığı) — Nazife gerçeği
  öğrenene kadar HER gece; öğrenince kalkar (görsel + ışık gider)
- **İKİ SIRA DA ÇALIŞIR**: araba yankısı hattı artık kalıcı (SF_K2_CART, seçim
  anında yazılır) — handan önce yankı / yankıdan önce han
- **TANIK hattı**: nasıl söylediğin her şey — YUMUŞAK (süt kokusu kanıtı: "Sütü
  hep unuturdu... o zaman gerçekten oydu", kibar+1) / DÜZ (ketum+1) / SUS
  (kalıcı değil, dönüp söyleyebilirsin)
- **SÖMÜREN hattı**: kanıt YOK — "Kokusunu söyle. Elini. Son sözünü. ...
  Bilmiyorsun." Sofra sonsuza dek kurulur; günlüğe itiraf ("bu benim yüzümden")
- Karar kalıcı (SF_K2_NAZIFE) + JE_K2_CHOICE 3 metin + kapanış diyalogları
- Kişilik fx'leri ilk kez kart diyaloğunda (kibar/ketum — okunma D1c'de genişler)
- **D1b-ek (2026-07-16, merge sonrası port)**: k2:1 artık 2 panellik SON SOFRA
  vinyetiyle kapanır ("ikimize" gecesi + ocakta süt devri + fısıltının ilk kez
  susması — huzur=sessizlik kuralı); han House_1 kopyasından gerçek Inn_Red
  sprite'ına yükseldi (0.8 ölçek, konum aynı); gen_k2.py yol fixi + paneller
  script'e eklendi. NOT: paralel oturum çakışmasında remote D1b esas alındı,
  yereldeki bağımsız D1b atıldı — bu iki parça oradan taşındı.

### Faz M1 — ELEMENT SİNERJİLERİ (mekanik-öncelikli hat #1)
- **TERMAL ŞOK**: yanan hedefe Buz Novası → kalan yanma ANINDA ×1.6 patlar,
  yanma söner (buhar FX + "Termal Şok!" + krit-stili sayı) — DoT'u erken
  bozdurma bedeli tempo seçimi oldu
- **ZEHİR PATLAMASI**: zehirli hedefe Yıldırım → kalan zehir ANINDA + 90px
  çevreye %50'si (yeşil bulut) — kalabalıkta zincir potansiyeli
- **ALEV RÜNÜ YAKIYOR** (kullanıcı onayı): melee + ok vuruşu %20/3sn yakma
  basar — hibrit build'ler (alev kılıç → Nova) sinerjiye girer; rün seçimi
  gerçek trade-off: Pembe %8 anında / Alev %20 DoT+şok / Zehir %25 DoT+patlama
- NOT (2026-07-14 kullanıcı kararı): hat MEKANİK-ÖNCELİKLİ — hikaye mekaniği
  taşıyan/test ettiren araç; D1c (S3) beklemede, "okunma" her faza serpiştirilir

### Faz M2 — PİŞİRME + YEMEK BUFFLARI (av döngüsü kapandı)
- **3 BUFF YEMEĞİ** (kamp ateşi + kazan; baştan bilinir): Avcı Yahnisi
  (2 Et+1 Kızılot → 180sn TÜM hasar +%8 — melee/ok/büyü), Ot Çorbası
  (2 Ayotu+1 Et → mana rejeni ×1.5), Gezgin Kavurması (2 Et+1 Ayotu →
  stamina ×1.6 + hız +%5)
- **TEK "TOK" BUFFU**: yenisi eskisini ezer; HUD sayacı ("Tok: X (Ns)") +
  "Acıktın" bildirimi; süreli buff save'e girmez (iksir deseni)
- **ÇİĞ ET YENİR** (kullanıcı onayı): +6 can ama %25 zehirlenme (8sn) —
  ItemType.rawRisk alanı (bedelli kötü seçenek)
- FoodBuff enum + ItemType foodBuff* alanları; FB_DMG combat_player_hit +
  combat_spell_hit'te; rejenler main'de; tooltip Tok/çiğ satırları; F10 seti
- Av döngüsü: avla → kamp kur → pişir → buff'la yola devam (ekoloji av
  kıtlığıyla et stratejik kaynak)

### Faz M3 — DAVRANIŞ ÜNVANLARI ("kimlik davranıştan doğar" vitrini; save v23)
- **10 ÜNVAN** (title.c): Slime Kasabı/Goblin Belası/Ork Kıran (aile +%5),
  Gölge (sinsi +0.1), Gece Kuşu (gece +%3), Yuva Yıkıcı (reis/totem +%10),
  Usta Avcı (av çift ürün %10 — ikinci kasa), Nişancı (yay +%3),
  Kadeh (iksir +%5), Aşçı (Tok süresi +%20)
- **HEPSİ PASİF + 1 TAŞINAN** (kullanıcı seçimi): açık ünvanların pasifi kalıcı;
  taşınan ünvanın pasifi ×2 + HUD'da isim ("- Gölge -"); kitapta tıkla = taşı/bırak
- **KİTABA 3. SEKME**: J → Tab döngüsü Görevler→Günlük→Ünvanlar; açıklar altın
  (taşınan kızıl), kilitliler ilerleme sayaçlı (87/100); journal_title_at hit testi
- Sayaç kancaları: aile kesimi (reis→goblin, totem→ork sayılır), gece kesimi,
  sinsi vuruş (melee+ok), yuva çöküşü/ritüeli, atılan ok, içilen iksir,
  pişirilen yemek; kazanımda bildirim + SFX + JE_TITLE günlük satırı
- Save v23: titleStats[12] + carriedTitle; F10 sayaçları eşik altına çeker
  (2-3 eylemle ünvan düşer — hızlı test)

### Faz M4 — SUMMON HATTI: CONJURATION ("Öz Bağı"; onaylı mekanikler kapandı)
- **17. SKILL + TAM AĞAÇ**: Conjuration — XP dostların verdiği hasardan (use-based);
  save bump GEREKMEDİ (skill/ağaç blokları sayı-toleranslı)
- **ÖZ YILANI** (bootstrap — Ateş Topu kuralı): 30 mana + %25 REZERVE; hızlı,
  zehir basar (60HP / 6dmg + %25 zehir 4sn); **ÖZ KABUĞU** (ağaçtan AKTİF):
  tank 220HP, 2sn'de taunt nabzı — moblar hedefini kabuğa çevirir ve ONU döver
- Dostlar EFEKT-varlık (yeşil öz; snake/turtleshell 128px belirme+hamle sheet'leri —
  yürüyen pet değil, süzülen büyü); ally.c: takip/hedef seçimi (yalnız SENİN
  aggro'n — sadık, sneak bozmaz)/ışınlanma/can şeridi
- **KURALLAR**: tek dost (yenisi gönderir), hotbar'a tekrar bas = geri çağır
  (rezerv çözülür), ölünce 10sn CD; REZERV bar'da MOR KİLİTLİ bölme (kullanıcı
  tespiti: oransal bar rezervi gizliyordu — görsel bölme eklendi)
- **KEYSTONE'lar**: SÜRÜ EFENDİSİ (iki dost, rezerv dost başına %30) /
  KURBAN BAĞI (dost hasarı +%40, dost ölünce SEN 15 hasar — "Bağ koptu!")
- Ünvan çarpanları dost hasarına da işler; ikon atlası 15 hücre; F10 Conjuration

### Faz D1c — S3 SAYAN ADAM (Dalga 1 kapanışı; MAP_KONAK)
- **NUH + ZEHRA** (Fisherman_Fin / Desert_Person_2, 32px frameSize): Nuh gündüz
  sunak yolunda sayarak volta (T60,18), gece köy kenarı; Zehra meydanda —
  3 ipucu verir (SF_S3_HINTS; yeni `hint` diyalog değişkeni req'te okunur)
- **SAYI BULMACASI** (3 tur): 41 (köyün nefesi) / 9 (Sessiz Gün atımları) /
  14.612 (kırk yıllık nöbet günleri); yanlış = O GÜN kilit (nuhLockDay —
  save'e girmez, zar kilidi kuralı); ipuçlu kolay seçenek + Speech 25 zar turu;
  DIALOG_MAX_OPTS 4→5
- **KONAK YOLculuğu**: 3. doğru → cine (kamera Nuh'u izler) + npc_walk ile
  sayarak yürüyüş (sahne araçlarının ilk gerçek kullanımı)
- **EŞİKÇİ KONAĞI** (MAP_KONAK, T168,45 kemer): loş ama zindan DEĞİL (dalga/mob
  yok; MOOD_NIGHT + 0.55 ambient); giriş salonu/koridor/büyük salon/kilitli
  KAYIT ODASI ("Nuh'suz açılmaz")
- **KARDEŞ YANKISI** (İdris): Tanık = kaydın anlamı; SÖMÜREN = fısıltı İLK KEZ
  yan hikayeye karışır: "...sayan adamdan uzak dur." (açıklanmaz — kanon sinyali)
- **KAYIT ODASI VİNYETİ** (nöbet devri): DEVRAL → "Kayıt Tutan" ünvanı (11. —
  eşiksiz, yalnız bu seçimle; +10 max mana, taşınırsa ×2; TS_RECORD) /
  KAPAT → aile huzuru / [Speech 30 ZAR] üçüncü yol: Zehra'ya öğret (kapı açık
  kalır — vinyet zarının ilk kullanımı; SceneFx.s3 alanı)
- Nuh/Zehra kapanış diyalogları kalıcı; JE_S3_STEP + JE_S3_CHOICE günlük
  metinleri; paneller gen_s3.py (6 panel); npcState tavanı 8→12 (9 NPC)
- **DALGA 1 KAPANDI**: K9 + K2 + S3 oyunda

### Faz KY — KARAKTER YARATMA-LİTE (save v24; roadmap YAKIN kapandı)
- **"Kim olacaksın?" ekranı** (GAME_CREATE): Yeni Oyun slotundan sonra, intro'dan
  önce; menü müziği (mühür nabzı) çalar; [Enter] Başla / [ESC] Geri
- **İSİM**: SDL text input (UTF-8/Türkçe, 23 bayt, yanıp sönen imleç, Backspace);
  boşsa "Gezgin"; save v24 ÖZET BLOĞUNA girer → SLOT KARTI BAŞLIĞI artık isim
- **TEN TONU**: 3 seçenek (açık/buğday/koyu) — LPC base katman color-mod,
  her kare uygulanır (yaratma/yükleme farkı yok)
- **GEÇMİŞLER** (hepsi bedelli — trade-off): Gezgin (nötr) / Eski Asker
  (+2 STR + Demir Miğfer; köy itibarı −5) / Avcı (+2 DEX + Yay + 30 ok;
  KILIÇSIZ) / Çırak Simyacı (+2 INT + 2 mana iksiri + 3 Kızılot; −25 altın);
  stat bonusları TÜRETİLMİŞ statlarda (gear ile doğru toplanır);
  player.background kalıcı — ileride diyaloglar okuyabilir
- Yeni oyun reset bloğu yaratma ekranının "Başla"sına taşındı

## 🔜 ROADMAP (2026-07-14 — RPG-ruhu analiziyle onaylandı)

Analiz özeti: sistem omurgası güçlü (build kimliği, ekonomi, suç/itibar, ekoloji,
ahlak ekseni); üç büyük boşluk → (1) hikaye kartları oyunda değil, (2) seçimler
birikmiyor değil ama OKUNMUYOR (kişilik/itibar/mühür diyaloglara dönmeli),
(3) dünya tek yerleşimli, POI/yankı yoğunluğu düşük. Öncelik: önce his + rol,
sonra genişleme, en son ana hattın kapanışı ("coğrafya hikayeyle büyür").

### Faz WE — DÜNYA OLAYLARI ÇEKİRDEĞİ (save v25)
- **worldevent.c**: her şafak %35 zar → günün olayı (aynısı üst üste gelmez);
  durum save v25'te (aktif olay + başlangıç günü + son olay). 4 olay:
  - **Goblin Baskını (WE_RAID)**: gece inince köyün doğu kenarına 2-3 goblin
    (zoneId -1 — ekolojiye sayılmaz, `Mob.raider`). Öldüren köy itibarı +2
    ("Köyü savundun!"); şafakta sağ kalanlar sessizce çekilir (leş/loot yok).
  - **Gezgin Kervan (WE_CARAVAN)**: 2 gün meydanda Kervancı Selim
    (Desert_Trader_1; `npc_force_hidden` API — rutin görünürlüğü ezilir).
    ÖZEL STOK (`CARAVAN_STOCK`): Bilgelik/Kuvvet Taşı, Soluk/Zehirli Öz, Ok,
    Mana İksiri — hepsi %15 pahalı (pazarlıksız kervan). shop.c aktif-stok
    düzenine geçti (render + tıklama + tooltip stok parametreli).
  - **Bereket (WE_BOUNTY)**: o şafak hayvan sürüleri +2 ekstra doğum
    (`ecology_birth_bonus` — tek şafaklık, hayvan aileleri FAM_SHEEP+).
  - **Mühür Sancısı (WE_SEAL)**: gece yankı parıltıları gözle görülür büyür
    (×1.8) + aralıklı fısıltı SFX'i (40-75 sn).
- **Duyuru ağı**: olay sabahı görünür bir köylü haberi bağırır (bark) + pano
  Söylentiler bölümüne olay satırı düşer (worldevent_rumor, en üst sıra).
- **Dinlenme [R]**: gece kamp ateşi başında `[R] Dinlen` → sabah 07:00'a atla
  (gametime_set — day-rollover DOĞAL tetiklenir: olay zarı/ekoloji/otokayıt).
  HP+STA tam, mana en az %50 (uyku doldurmaz, yarılar). %25 şansla RÜYA
  vinyeti: 3 havuz (ruya_muhur / ruya_cukur / ruya_fisilti — mühür/çukur/
  fısıltı imaları, cevap yok). Paneller `scripts/gen_dreams.py` (prosedürel).
- Kervan görünürlüğü yükleme/yeni oyunda senkronize (worldevent_reset + save).

### Faz WE2 — HAVA SİSTEMİ + DÜNYA OLAYLARI KALANLAR (save v26)
- **weather.c**: her şafak zar — Açık %55 / Yağmur %25 / Sis %15 / Fırtına %5;
  öğlen (12:00) %25 şansla hava döner. Yağmur 2-6 oyun saati sürer; sis 05-10
  arası yoğun, öğlen dağılır. `intensity` 0..1 yumuşak geçiş (bastırır/diner).
  Save v26: aktif hava + kalan saat + "bugün yağdı" bayrağı.
- **Savaş etkileri (M1 sinerjileriyle simetrik)**: ıslakta yanma DoT süresi
  ×0.5, Ateş Topu -%15, Yıldırım +%20 (Zehir Patlaması yağmurda altın çağ);
  siste mob algısı ×0.6 (sinsi fırsatı). FIRTINA: 8-20 sn'de bir yıldırım
  düşer (60px yakına: mob 25 / oyuncu 20 hasar) + şimşek flaşı + gök gürültüsü.
- **Çevre etkileri**: köylüler yağmurda evlerine çekilir (npc_set_shelter;
  kervancı gölgelikte kalır, muhafız nöbette ıslanır + bark); dün yağdıysa
  bugün ot toplaması ×2 ("Yağmur toprağa iyi geldi"); yağmurda kamp ateşi
  kurulamaz (köy ateşi çardaklı — çalışır); meşale ışığı ıslakta -%20;
  fırtınada pencere ışıkları titrer. MOB: yağmurda goblin devriyesi durur
  (yuvaya çekilir), hayvanlar su ziyaretine çıkmaz (su ayağına geldi).
- **Görsel/ses**: ekran-uzayı yağmur damlaları (rüzgar eğimli, 140 tavan),
  iki katmanlı sis örtüsü (nefes alır), fırtına karartması (gündüz loşlaşır),
  şimşek beyaz flaşı; amb_rain.wav yağmur katmanı (day/night'ın üstüne biner)
  + thunder.wav (scripts/gen_weather_audio.py — prosedürel).
- **WE_GRAVE — Huzursuz Mezarlık** (5. dünya olayı): gece mühürlü/boş iskelet
  mezarında 3 iskelet yükselir (risen); şafakta 2+ sağsa mezar YENİDEN yuva
  olur (ekoloji kolonisi + pano söylentisi + JE_GRAVE_RISE günlük girişi),
  sağ kalanlar gün ışığında mezara çekilir.
- **Mob arama (MOB_SEARCH)**: leash'te iz kaybeden avcı mob hemen dönmez —
  8 sn olduğu yerde bakınır (IDLE clip, rastgele yön), oyuncu yakına dönerse
  yeniden alarm; süre dolunca kampa döner. Sis + sinsilikle saklambaç derinleşir.
- **Haber ağı diyaloğa indi**: DVAR_EVENT (`event`) + DVAR_WEATHER (`hava`)
  diyalog değişkenleri (salt okunur; koşul tutmayan seçenek GİZLENİR —
  DialogOptState.hidden). Kemal olay yorumları (5 olay), Osman hava sohbeti
  (3 hava), Baran kervan kıskançlığı.
- **KAR (soğuk cephe)** — 5. hava: havuz Açık %50 / Yağmur %22 / Sis %13 /
  KAR %10 / Fırtına %5; 4-10 saat sürer. SAVAŞ: buz büyüleri +%20, buz
  yavaşlatma süresi ×1.5 (Buz Novası 4→6sn, Buz Oku 3→4.5sn), HERKES %12
  yavaş (oyuncu+mob eşit; av +%15 daha) — ateş/yıldırım NORMAL (kuru soğuk,
  Termal Şok karda altın çağ). ÇEVRE: zemin beyaza bürünür (snowCover —
  yağarken ~20sn birikir, dinince ~75sn erir), sürüler büzüşür (devriye
  70→32px), su ziyareti iptal (dondu), ot toplaması yarıya (doğa uykuda),
  kamp ateşi ömrü 24→12 saat, İZ SÜRME: arama modundaki mob karda bakınmaz —
  12 sn boyunca izinden SANA doğru yürür. Görsel: salınan kar taneleri +
  kapalı gökyüzü karartması; Osman kar sohbeti; F8 döngüsünde kar da var.
  Save v26 UYUMLU (enum sona eklendi — bump yok).

### Faz BD2a — BÜYÜ DALGASI 2, PARÇA 1: TOMAR EKONOMİSİ + 3 BÜYÜ (save v27)
- **Tomar ekonomisi**: kayıp büyü tomarları — envanterde sağ tık = oku →
  büyü KALICI açılır (Yıkım seviyesi yetmiyorsa tomar durur, yanmaz; "Harfler
  kayıyor..."). `Player.spellsLearned` bit maskesi (v27); ağaçtan bağımsız —
  tree_compute_effects sonrası her yerde `spells_apply_learned` köprüsü
  (yoksa hotbar_purge_closed öğrenilmiş büyüyü sökerdi). Yeni oyunda sıfırlanır.
- **3 yeni büyü** (magic_pack sheet'leri — 4×4 grid 128px, 16 kare):
  - **Yıldırım Pençesi** (YP, 35 taban/18 mana/5sn): bakış yönüne 110px koni —
    büyücünün panik butonu. Islakta +%20 (bolt mult). Tomar: Yıkım 15, 350g.
  - **Buz Dikenleri** (BD, 26/24/8sn): hedef alanda (100px) yerden buz —
    hasar + %50 yavaş 3sn. Karda +%20 ve yavaş ×1.5. Tomar: Yıkım 30, 550g.
  - **Hortum** (HO, 8×tick/35/14sn): bakış yönüne süzülen alan (65px/sn, 4sn) —
    0.5sn'de bir tick + %30 yavaş; kalkış karelerinde yerinde döner, son 0.4sn
    dağılır. Fırtınada tick ×1.2 + ömür 5sn. Tomar: Yıkım 40, 650g.
- **Kaynaklar**: kervan özel stoğuna Pençe+Diken tomarları (parça 2'de havuz
  rastgeleleşecek); Hortum tomarı YALNIZ zindan USTA sandığı (%25). F10 dev
  seti 3 tomarı çantaya basar.
- İkon atlası 15→18 hücre (gen_ability_icons.py — sheet karelerinden indirme).
- **Parça 2 TAMAM**: Yılan Isırığı (YS, 12/20/7sn — hedefe küçük vuruş + zehir
  DoT %60/4sn zehir kanalına EKLENİR → Zehir Patlaması salt-büyü kombo; yönlü
  snakebite sheet'i hedefte yükselip ısırır) + Ateş Aslanı (AA, 30/28/9sn —
  bakış yönüne 430px/sn hat dalgası 0.7sn, çarptığı HER moba 1 kez vurur
  [hitIds takibi] + Ateş Topu yanma kanalı basar → Termal Şok'u besler;
  ıslakta -%15; yönlü firelion sheet'leri). Tomarlar: Yılan Isırığı Yıkım 20
  (kervan 400g) / Ateş Aslanı Yıkım 50 (YALNIZ boss leşi %35 — haftalık kural).
  KERVAN TOMAR HAVUZU RASTGELE: her kervanda {Pençe, Isırık, Diken}
  havuzundan 2 FARKLI tomar — worldevent startDay tohumlu (save gerekmez,
  ziyaret boyunca sabit, sonraki kervanda değişir). İkon atlası 18→20.
  Item enum'ları SONA eklendi (v28 kayıtların item indeksleri kaymadı).
  BÜYÜ DALGASI 2 KAPANDI — magic_pack'in 5 büyüsü de oyunda.

### Faz ÇANTA — ÇANTA SEKMELERİ: Eşya + Malzeme (save v28)
- **İki çanta, tek panel**: I aç/kapa, TAB sekme değiştirir (Eşya ↔ Malzeme);
  sekme şeridi tıklanabilir (panel üstü 28px bant). Sürüklenen item DİĞER
  sekmenin başlığına bırakılınca taşınır; şeridin sağ ucundaki ÇÖP hücresine
  bırakılınca İMHA edilir. Craft paneline tekerlek kaydırması geldi (uzun
  tarif listeleri — çanta tarifleri artık görünür); pencere 1280×720 → 1600×900.
- **Otomatik ayıklama**: `ItemType.material` bayrağı (13 tür: cevher×2,
  külçe×2, ot×2, odun, jel, kemik, öz×2, deri, çiğ et + İp) — toplama/loot/
  satın alma hammaddeyi Malzeme sekmesine düşürür. Dolu sekme diğerine taşmaz.
- **bag_* katmanı** (main): bag_add/add_stackp (material yönlendirmesi),
  bag_count/consume/consume_n (İKİ çantadan, craft öncelikli) — ~70 çağrı
  noktası bu katmana geçti; craft/kontrat panelleri iki çantayı birden sayar.
- **Kademeler (craft'la kalıcı büyüme)**: Eşya 30 → +10 (Deri Çanta, Smithing
  15: 6 Deri+2 İp) → +10 (Gezgin Çantası, S40: 10 Deri+Demir Külçe) = max 50;
  Malzeme 25 → +15 (Malzeme Kesesi, S10: 4 Deri+2 İp) → +15 (Usta Kesesi,
  S35: 8 Deri+Demir Külçe) = max 55. Çanta eşyası üretilince OTOMATİK takılır
  (envantere girmez; "Zaten takılı" korumalı). İP yeni ara malzeme: kamp
  ateşinde 2 Ayotu → 1 İp. NOT: gümüş oyunda yok — toka = Demir Külçe.
- `Player.bagUpgrades` (bit0/1 Eşya, bit2/3 Malzeme) + Malzeme çantası içeriği
  save v28; Inventory.cap dinamik (INVENTORY_SIZE 60 dizi tavanı);
  MAX_RECIPES 32→48. Başlık "Eşya 12/30" doluluk gösterir.

### Faz TH-A — TWO-HANDED PARÇA A: MIZRAK (v28 uyumlu)
- **Mızrak** (ITEM_SPEAR, enum sonu): mesafe kontrolü silahı — menzil 130
  (kılıç 80; çoğu mobun saldırı menzilinin DIŞI), tempo 1.5sn, taban 34,
  DELİCİ (hedef defansının %25'i yok sayılır, ağaçla %40, tavan %80),
  HAT VURUŞU (saplama çizgisindeki 2. mob %60 hasar; ağaçla %75 + 3. hedef).
  İki el: WEAPON_TWO_HAND — kalkan kilitli. Kaynak: Demirci Ocağı (S20:
  3 Odun + 2 Demir Külçe) ya da vendor 260g. F10 setine girdi.
- **Animasyon**: PLAYER_THRUST (asa dürtmesiyle ortak state) + thrustOversize
  katmanı — salt-thrust sheet (1536×768, 8 kare × 4 yön 192px, ULPC
  thrust_oversize çıktısı; fg z150 + bg z1). Mızrak katmanı thrust dışında
  çizilmez (sheet'te walk satırı yok).
- **Two-Handed ağacı** (T_TWOHAND, 18 düğüm): Delici/Tempo/Hat dalları;
  aktifler AĞAÇTAN — Saplama (×2.2 + geri itme ×2, 8sn/25 STA), Süpürme
  (ön 140° yay ×1.0 + %30 yavaş 2sn — ağaçla %45; 12sn/35 STA; Nova deseni
  ama melee formülü + TH XP). Keystone KAPI TUTUCU: duruş AÇ/KAPA — hız %40
  düşer ama menzile giren mob otomatik hedeflenir + KO auto devreye girer
  (0.8sn tempo tavanı); mızrak çıkınca duruş düşer. combat_weapon_skill
  zaten TWO_HANDED eşliyordu — XP otomatik. PerkEffects: twoHandDmgMult /
  pierceBonus / lineTargetBonus / lineDmgFrac / sweepSlowMult /
  spearStaminaMult / spearRangeBonus. İkon atlası 20→23.
- **Parça B TAMAM — TWO-HANDED + BLOCK PAKETİ KAPANDI**:
  - Kalkanlar: Tahta (def 6, S10 craft: 4 Odun+1 Bakır Külçe, 120g) + Demir
    (def 12, S25: 3 Demir Külçe+2 Deri, 300g). Görsel: ULPC round_shield
    (z110, iki kalkan paylaşır — long_sword deseni). Prosedürel ikonlar.
  - MIZRAK GÖRSELİ TAM: kompozit sheet (1536×5760) — standart satırlar
    (yürüyüş/idle'da mızrak GÖRÜNÜR) + thrust-oversize bloğu @ y=4992
    (long_sword blok düzeni); player.c thrust'ta oversize bloktan örnekler.
    Eski salt-thrust sheet'leri silindi.
  - PASİF BLOK (kalkan takılıyken her zaman): şans %10 + Block lvl×0.2 + ağaç
    (tavan ~%44); blok = hasarın %60'ı emilir (ağaçla %70) + Block XP.
  - AKTİF BLOK (SHIFT basılı): gelen hasar -%70 (ağaçla -%80), hız yarıya
    (ağaçla %35 ceza / FALANKS sıfır), auto-attack durur, 4 STA/sn (stamina
    biterse duruş düşer). PERFECT: duruşun İLK 0.25sn'inde (ağaçla 0.35)
    gelen vuruş → hasar 0 + melee saldırgan 1sn SERSEMLER (parry); SİPER
    keystone: hasarın %50'si yansır. Mermide sersemletme/yansıtma yok.
  - Blok zarı combat_mob_damage'da (tek nokta) — 3 çağrı yeri (mob melee /
    mermi / muhafız) blok türünü işler; block_feedback ortak (yazı+ses+XP).
  - T_BLOCK ağacı (16 düğüm): Şans/Duruş/Emilim dalları + SİPER + FALANKS +
    AKTİF Kalkan Vuruşu (×0.6 + 2sn sersemletme, 10sn/20 STA — CC aracı).
  - Kule Kalkanı: kalkan slot defansına çarpan (equipment_total_defense).
  - F10: Demir Kalkan + Block ağacı puanı. İkon atlası 23→24.

### Faz PP — PICKPOCKET: GÖLGE ELİ (v28 uyumlu; son mekanik paket)
- **Etkileşim**: C eğil + NPC'ye yaklaş + [P] (eğilmişken '[P] Yankesicilik'
  ipucu). Zar: taban %35 + Pickpocket lvl×0.5 + ağaç; eğilme +%25, hedef
  bark'ta (meşgul) +%15, muhafız -%30; AYNI NPC'de her deneme (o gün) -%15
  — sınırsız ama hızla imkansızlaşır (şafakta sıfırlanır). Tavan %90.
- **Keseler** (role göre): Kervancı 25-60g +%20 öz · vendor 15-40g +%20
  iksir · muhafız 10-25g +%25 Maymuncuk (ironik) · Nuh/Fatma 3-8g (vicdan
  testi) · köylü 5-15g. Başarı persKetum +1 (kişilik izi) + 25 XP (sessiz).
- **Yakalanma**: hedef tanıktır — bounty +15 (ağaçla -%50) + itibar -3 +
  'Hırsız! HIRSIZ var!' bark'ı + lastCrimeDay; MUHAFIZIN cebinde yakalanmak
  = anında yakalama paneli. Başarısızlık da 8 XP (denemek öğretir).
- **T_PICKPOCKET mini ağacı** (10 düğüm): Şans (+%3/+%5/+%5/+%10) · Kese
  (altın +%25×2, item şansı 2x) · Sessizlik (bounty -%50, MASUM YÜZ: günde
  bir başarısızlık fark edilmez) · keystone GÖLGE ELİ (başarıda hedefin
  eşyası da çalınır — ek zar). F10 Pickpocket puanı basar.
- MEKANİK PAKETLER TAMAM: roadmap'in savaş/yaşam eksenindeki tüm onaylı
  sistemler oyunda (M1-M4, Dünya Olayları+hava, Büyü D2, çanta, TH+Block, PP).

### Faz KM — KİRLİ MAL & AKLAMA (save v29)
- **Kirli kese**: yankesicilik altını `player.dirtyGold`'a düşer — HARCANAMAZ
  (HUD'da altının yanında kırmızı "(+N kirli)"). Çalıntı eşyalar
  `ItemStack.stolen` damgalı (tooltip kırmızı ÇALINTI satırı): kuşanılamaz,
  satılamaz, tüketilemez, craft/kontrat SAYMAZ (inventory_count/consume
  çalıntıyı atlar; kirli-temiz aynı yığına girmez).
- **Keseler ×2 + değerli objeler**: Kervancı 50-120g (%15 Cep Saati 150g) ·
  vendor 30-80g (%20 iksir, %15 Yüzük 80g) · muhafız 20-50g (%25 Maymuncuk,
  %10 Broş 100g) · köylü 10-30g (%8 Yüzük) · Nuh/Fatma 5-15g. Objeler
  YALNIZ çalıntı bulunur — normal ekonomide yeri yok, aklayıcı tam öder.
- **MÜSADERE**: yakalama panelinde ceza ödersen kirli altın + tüm çalıntılar
  da gider ("kanıt") — biriktirme gerilimi (push your luck).
- **AKLAMA — iki kanal**:
  - Kervancı Selim "el altından" (kervan günleri, diyalog): kirli altın
    ×0.75, çalıntı eşya ×0.6, DEĞERLİ OBJE ×1.0 — sabırlının kanalı.
  - KÖR KUYU (köy dışı T26,15 — Well.png decor): yalnız GECE [E] — hepsi
    kuyuya, şafakta ×0.5 temiz kese ("Sayılıyor. Hepsi sayılıyor." fısıltısı).
    Aceleci kanal; kim aldığı İLERİDE: yeraltı örgütünün tohumu.
  - Ağaç: Keseci/Güvenilir El düğümleri artık AKLAMA kesintisini düşürür
    (kervan 0.75→0.85, kuyu 0.50→0.60).
- Save v29: ItemStack.stolen (write/read_stack +1 alan) + dirtyGold +
  wellGold/wellDay.

### Faz YK1 — YANKICI: GİZLİ SINIF, PARÇA 1 (v29 uyumlu — storyFlags)
- **Tamamen gizli**: SKILL_ECHO ("Yankı") skill defterinde/K panelinde ritüel
  tamamlanana dek YOK (skill_set_echo_visible — satır enum sonunda, Kader
  eşlemesi korunur). İpucu yok, '???' yok — yalnız fısıltı imaları.
- **Keşif (üç kapı)**: ① 3 yankı SÖMÜR (Tanık kalan hiç görmez) → fısıltı
  değişir: "Taşların eskisine gel. Ay görmesin." (JE_ECHO_CALL) ② GİZLİ TAŞ
  HALKASI: Karanlık Orman derinliği T(108,7) — haritada işaretsiz, prosedürel
  ritual_stones decor'u ③ GECE taşlarda [E] → RİTÜEL vinyeti (2 panel,
  kabul/reddet — reddedersen kapı açık kalır). Kabul: SF_YANKICI=2,
  splash "YANKICI", beceriler açılır, Fısıltı Keskisi taşın dibinde belirir
  (JE_RITUAL: "yabancı gelmiyor").
- **FISILTI KESKİSİ** (unique — satılmaz/craftlanmaz): hançer 16 dmg/0.7sn;
  EKSİK canın her %10'u +%8 hasar (riskle keskinleşir — beceriler can yakar:
  sinerji); vuruşlar Yankı XP'si de akıtır; ritüelsiz kuşanılamaz ("Sapı
  avucunda ISLANIYOR sanki").
- **Beceriler (bedel MANA DEĞİL KAN — AbilityDef.hpFrac)**:
  - Fısıltı Patlaması (FP, 8sn): max HP %10 → 130px gölge dalgası (taban 30 +
    Yankı lvl ×%2); dark_burst animasyonu (48×64, 16 kare). "Kanın yetmiyor!"
    koruması (HP eşiği altında atılmaz).
  - Gölge Formu (GF, 20sn): 5sn algı ×0.05 + hız +%20; 4 HP/sn yakar,
    5 HP'de düşer.
- **F10 test yolu**: sınıfı AÇMAZ — sömürü sayacını 3'e basar, fısıltı çağrısı
  gelir; taşları bulup ritüeli yine oyuncu yapar (keşif korunur).
- Save bump YOK: SF_YANKICI storyFlags[7] (v22 bloğu), echoesConsumed zaten
  kayıtlıydı, shadowTimer runtime. Parça 2 (sonra): Can Emme + Yankı Çağırma
  + T_ECHO ağacı + Dark VFX bolt.

### Faz HG-A — HARİTA GÖRSEL YENİLEME, FAZ A: DÜNYA (v29 uyumlu)
- **Kenar geçişleri (autotile)**: çimen komşu tabana "TAŞAR" — grass_edges.png
  atlası (12 hücre: 4 kenar + 4 dış köşe + 4 iç köşe; gen_tile_edges.py
  Grass_1_Middle'dan DALGALI alpha maskeyle üretir — jilet kenarlar bitti).
  Render: tile çizilirken 4 komşu + 4 çapraz bakılır; yalnız dünya dış mekan
  tabanlarına (yol/su/bataklık/diken) taşar — zindan dokunulmaz.
- **Çimen 4 varyant**: hash(col,row) deterministik — Grass_1..4_Middle
  serpiştirilir, tekdüzelik kırıldı (save'e girmez, hep aynı görünür).
- **Eksik dokular**: taş/bataklık/diken düz renkten kurtuldu (prosedürel
  16px dokular — assets/tiles/terrain/).
- **Süs serpiştirmesi** (tilemap_render_scatter — çarpışmasız, deterministik):
  çimende %6 çiçek/ot (flowers.png 10×10), yol kenarı çimende %10 küçük taş
  (Path_Decoration), su kıyısında %8 saz (Water_Decoration). Tile içi kayma
  hash'le — grid hissi yok. Yalnız dünya haritası.
- **Köy el yerleşimi**: fıçı kümeleri (dükkan+han), meydan/yol bankları,
  ocak arkası taş çit — rutin yollarının dışında.
- Faz B (sıradaki): LANETLİ MEZARLIK haritası (undead tileset).

### Faz HG-B — LANETLİ MEZARLIK HARİTASI (v29 uyumlu; MapId append-safe)
- **MAP_MEZARLIK** (60×45, açık hava zindanı): undead tileset paleti — ölü
  toprak [12] (2 varyant) + dikenli kaya [13] + ölü su [14]; gen_mezarlik.py
  sabit-tohumlu üretir (sınır kayalıkları, iç siperler, doğu gölcüğü).
- **Zindan MEKANİĞİ, açık hava GÖRÜNÜMÜ**: map_is_dungeon=true → mühür
  dalgası respawn, sabit alacakaranlık (ambient 0.45), MOOD_DUNGEON, NPC/hava
  kapalı. Boss YOK (Bringer-of-Death sonraki mob dalgasında buraya oturacak).
- **Portal**: Kayalık Geçit mezar sahaları yanı T(135,88) ↔ mezarlık güney
  girişi T(30,41); giriş görseli skull_door.
- **Map-bazlı dekor**: DecorDef.map alanı (0=dünya — eski girdiler değişmedi);
  render/collision/conceal filtreli; decor_set_map geçişte basılır. Mezarlık:
  DEV İSKELET KALINTISI (Lich, merkez landmark) + mezar taşları + kurumuş
  ağaçlar + harabe + kemik/kafatası yığınları + kristal.
- **İçerik**: MZ_CAMPS 5 iskelet kampı (savaşçı/okçu/büyücü, mühür dalgalı);
  2 sandık — kuzeybatı harabe (Sağlam) + dev iskeletin ardı (USTA → Hortum
  tomarının 2. kaynağı).

### Faz HB-WOW — HOTBAR WoW DÜZENİ (32) + TAB HEDEFLEME + TIKLA-BAS
- WoW düzeni, hepsi hep görünür (sıra/F-tuşu kavramı YOK — Hakan'ın evrilen
  isteği): 2 ana bar 8×1 alt alta (1-8 / Shift+1-8) + sağ dikey 2×4
  (Alt+1-8) + sol dikey 2×4 tuşsuz. slot_rect() tek kaynak (çizim+hit-test).
  Save count-toleranslı — eski 8/30'luk kayıtlar oturur, taşan düşer
  (açık aktifler place_unlocks ile geri gelir).
- TIKLA-BAS: dolu hücreye düz sol tık = tuşla basmakla aynı akış; bar
  üstündeki her tık yutulur (dünyaya geçmez). Taşıma/takas YALNIZ
  SHIFT+sürükle; kaldırma yalnız sürüklerken beliren ✕ hedefine (üst barın
  üstünde ortada) — dışarı bırakmak İPTAL (yanlışlıkla beceri atılamaz).
- Silah uygunluğu görseli: mevcut silaha uymayan beceri barda SOLUK
  (abilityUsable[] main'de her kare; büyüler silahtan bağımsız hep aktif).
- TAB: mob hedefleme (Z KALDIRILDI) — panel önceliği: envanter açıkken çanta
  sekmesi, günlük açıkken günlük sekmesi, yoksa hedef.
- K panelinde ALINMIŞ aktif düğümü bara sürükle = ata; item→hotbar yol korunur.
- NOT: "meslek paneli kısayolu barda" fikri KONUŞULDU ve VAZGEÇİLDİ (craft
  istasyona bağlı tasarımla çelişiyor; ileride Günlük'e Tarifler sekmesi olabilir).
- Crash fix: DECOR 37 > 32 texture dizisi taşması (DECOR_TEX_CAP 64 ortak
  sabit + döngü korumaları + aşım logu); portalTex 8→16 (tam doluydu).

### Faz VFX-1 — VFX HİS CİLASI (tam paket, onaylı sıra ③)
- **AnimFx havuzu** (fx.c/h): genel grid-sheet animasyon havuzu (MAX_ANIM_FX 48,
  satır-major, bir kez oynar söner) — animfx_init/spawn/update/render.
- **İsabet efektleri**: strike_fx_mob köprüsü (g_animFx deseni) — melee'de
  weaponhit (6×6@100), büyüde magickahit (7×7@100) + HER isabette küçük kan
  sıçraması (Hakan onayı); 17 hasar noktasına tek satır çağrı (DoT/yansıtma
  tick'leri hariç — spam yok).
- **Ölüm**: mob_die içinde büyük kan sıçraması + 90ms hit-stop (tüm ölüm
  yolları tek merkezden).
- **HIT-STOP**: g_hitStop — melee 50ms, ok/hat 30ms, ölüm 90ms, PERFECT parry
  120ms; dünya dt'si ×0.08 kısılır (menü/ölüm ekranı gerçek zaman).
- **Blok kıvılcımı**: 3 block_feedback noktasında (guard/mob/proje) sarı çarpma.
- **Cast halkası**: kanalizasyon boyunca ayak altında dönen çember (81 kare).
- **Slash yayı**: savurma başlangıç karesinde yöne göre yay (4 yön sheet'i
  90° döndürmeyle üretildi — slash_up/down/left/right.png).
- Assetler: Free Pixel Effects Pack (public domain) + Gigapack splatter/impact
  + Frostwindz Slash 1 → assets/fx/; CREDITS.md güncellendi.
- UX FIX (aynı commit): otomatik kayıttan yüklemede F5 "Aktif slot yok"
  uyarısında bırakıyordu — artık kayıt slotu ekranını doğrudan açar (seçilen
  slot aktif olur); F9 slotsuz durumda bildirim basar.

### Faz MÜZİK — GERÇEK MÜZİK SETİ (xDeviruchi, onaylı sıra ④)
- **stb_vorbis vendored** (src/stb_vorbis.c, public domain; CMake'te -w —
  vendored uyarıları bize sayılmaz). music.c önce assets/audio/music/*.ogg
  dener, yoksa eski prosedürel WAV'lara düşer (bozulmaz).
- **Eşleme (Hakan onayı)**: MENÜ=Title Theme · KÖY=Definitely Our Town ·
  KIR=Where The Winds Roam · GECE=Silent Forest · TEHLİKE=Calm Before The
  Storm · ZİNDAN/MEZARLIK=Lost Shrine · SAVAŞ=Battle 1 · BOSS=Decisive
  Battle 1 (YENİ MOOD_BOSS — aggro boss COMBAT'ı ezer, enum SONA eklendi).
- **LOOPSTART desteği**: intro'lar loop dışı (dokümandaki tablo) — ilk besleme
  intro dahil, sonraki beslemeler LOOPSTART byte ofsetinden; mood'a her
  girişte intro yeniden çalar (savaş girişi vurgulu).
- Lisans: atıf ZORUNLU — "Original music by Marllon Silva (xDeviruchi)"
  oyun kredilerine girecek (CREDITS.md'de hazır).
- 8 OGG ~37MB; crossfade/duck/volume mevcut sistemden aynen çalışır.

### Faz MOB-2 — MOB DALGASI + ÖLÜM GETİREN (onaylı sıra ⑤)
- **3 yeni tür (SONA — 21/24 MAX_MOB_TYPES)**: [18] Hortlak Büyücü (Evil
  Wizard 2 — mezarlık eliti, ATTACK_MAGIC 30 dmg), [19] Kan Canavarı (Blood
  Monster — ölü su, hızlı melee sürüsü), [20] ÖLÜM GETİREN (Bringer —
  isBoss, lv16 HP1600). Yan görünüşlü kompozit sheet'ler (3 yön aynı klip,
  flip motorda); Wizard kareleri aynalandı.
- **Boss savaşı (Hakan onayı — 2 fazlı)**: dev iskelet kalıntısı önünde
  T(30,20); Faz 1 tırpan + 4sn'de bir KARA PATLAMA (oyuncu altında 1sn
  büyüyen mor halka telegraph → 45 hasar, kaçılabilir); Faz 2 (%50) hız
  ×1.35 (slow kanalı ters kullanım) + BİR KEZ 2 İskelet Savaşçı + çift
  halka. Haftalık diriliş. MOOD_BOSS müziği otomatik.
- **Ödül**: İLK ölümde ÖLÜM GETİREN'İN NİŞANI garanti (unique kolye,
  EQUIP_NECKLACE ilk kullanımı): +%10 hasar (trinketDmgMult — melee+büyü,
  combat.c) + her öldürmede 8 HP (mob_die, g_eqBridge köprüsü). Sonraki
  ölümler: kemik+HP taşı+soluk öz garantili, tomar %60.
- **SAVE: toleranslı kuyruk deseni (YENİ)**: boss2 diriliş kaydı dosya
  SONUNA eklendi — eski v29 save'ler varsayılanla açılır, sürüm bump'sız.
  mob_die boss bloğu Lord/Bringer ayrımına bölündü.
- MZ_CAMPS +3 (2 Hortlak kampı + güney ölü su Kan Canavarı yuvası).
- SATYR + Demon_A sonraki dalgaya kaldı (backlog).

### Faz HOLY-2 — NUR YOLU AĞACI (Hakan onayı — tam 18 düğüm)
- T_HOLYTREE + TREE_DEF(SKILL_LIGHT): Şifa / Işık / Koruma şeritleri + AZİZ
  (şifa+%30, hasar+%15, stamina -15) ve IŞIK TAŞIYICISI (alan 4 HP/sn +
  alanda blok %20 güçlü, Alan CD +6sn) keystone'ları.
- PerkEffects +11 alan (holyDmgMult/healMult/holyManaMult/healCdCut/
  holyUndeadBonus/holyZone* — nötr init'e mult'lar eklendi).
- Uygulama noktaları: mana maliyeti (2 yer), CD ayarı (Rahibe/bedel), şifa
  gücü, hüzme çarpanları (undead 1.6+bonus), alan yarıçap/süre/tick/self-heal
  (holyZone.r alanı), 3 blok noktasında alan-blok sinerjisi.

### Faz HOLY — KUTSAL HATTI (onaylı sıra ⑥, Hakan onayları: 3 büyü + tomar/tapınak + denge)
- **SKILL_LIGHT "Kutsal"** (enum sona): XP = şifa ×1.2 + kutsal hasar.
  Panel satır eşlemesi düzeltildi (row_skill — ECHO gizliyken atlanır; eski
  "son satırı kes" varsayımı LIGHT eklenince kırılıyordu).
- **3 büyü** (ABILITY enum sona — spellsLearned 29/32 bit DOLULUK UYARISI):
  ŞİFA NURU (30 mana → 35 HP taban, INT+Kutsal ölçekler, CD 8sn) ·
  KUTSAL HÜZME (25 mana, 26 taban, UNDEAD ×1.6, CD 5sn, hedefli) ·
  KUTSAL ALAN (40 mana, CD 20sn; 6sn/120px çember — undead %25 yavaş +
  7/sn tick; tek çember, yeniden atım taşır).
- **MobType.undead** alanı: iskelet ailesi + Lord + Hortlak + Bringer işaretli.
- **Tomar gate GENELLEŞTİ**: okuma koşulu ABILITIES[ta].skill seviyesine bakar
  (eskiden Yıkım'a sabitti) — mevcut tomarlar etkilenmez (hepsi Yıkım).
- **Kaynaklar**: Şifa Nuru = kervan SABİT stok + FATMA HEDİYESİ (K9 tapınak
  kararı verenlere son1_tomar düğümü, SF_K9_TOME ile bir kez; çanta doluysa
  bayrak yanmaz) · Hüzme/Alan = Hortlak Büyücü %8/%6 + USTA sandık %15 havuzu.
- İkon atlası 24→29 hücre (gen_ability_icons.py — Yankıcı 24-25 boşluğu da
  dolduruldu) · Holy VFX sheet'leri assets/fx/ · F10 kitine 3 tomar.

### Faz YP2+TB64 — YANKICI PARÇA 2 + spellsLearned 64-bit (2026-07-20)
- **TB64 (teknik borç)**: Player.spellsLearned Uint32→Uint64; save v27 alanı
  ALT 32 bit olarak yerinde kalır, ÜST 32 bit toleranslı kuyruğa (boss2'den
  sonra) — eski save'ler bozulmaz. Tüm bit işlemleri 1ull.
- **CAN EMME** (hedefli): bedel max HP %8; hedefe 4sn emiş DoT'u (8/sn taban,
  Yankı sev. + ağaç ölçekler) — emilen her tik OYUNCUYA AKAR (mor sızıntı FX,
  Mob.drainDps/drainTime yeni kanal, tik başına Yankı XP).
- **YANKI ÇAĞIRMA**: bedel max HP %12; 12sn gölge kopya (echo_wisp 20 kare,
  bob+flip) — hedefinin yanında süzülür, 1.2sn'de bir SENİN vuruşunun %35'i
  (combat_player_hit ile — silah/perk/crit işler). Hedef yoksa yanında bekler.
- **T_ECHO "Fısıltı Yolu" ağacı (16 düğüm)**: Sömürü (bedel -%5/-%5/-%15,
  emiş +%10, +2sn) · Gölge (form +1/+2sn, hız +%10, bedel 4→2 HP/sn) ·
  Fısıltı (yarıçap +20px, hasar +%5/+%12, kopya +4sn) + keystone AÇ GÖLGE
  (Yankı +%25, max HP -%10). PerkEffects +10 alan; P1 büyüleri de kablolandı
  (whisper yarıçap/hasar/bedel, form süre/hız/bedel, maxHP çarpanı).
- İkon atlası 29→31; ritüel unlocks P2 büyülerini de açar (echo_apply_unlocks).
- TEST NOTU: P1 ritüeli HÂLÂ TEST EDİLMEDİ — P1+P2 tek turda test edilebilir
  (F10 → taş halkası T108,7 → gece [E] → ritüel → 4 büyü + K'da Yankı ağacı).

### Faz REBOOT — KANON SIFIRDAN: "DEMİRBOYUN" (2026-07-30)

**Hakan üç turu üst üste reddetti; dördüncü tur tuttu.** 24 aday, 6 kurgu mimarı × 4 tur,
her turda bağımsız klişe avcıları + tarafsız hakem.

- **Tur 1** (6 aday): altı mimarın altısı da "ölüler kalkıyor + ejderha + toprak" ailesine indi.
  Hakem 1.'si "Et Durmaz" (yaşlanmasızlık yasası). **Hepsi reddedildi.**
- **Tur 2** (6 aday): o aile yasaklandı, iskelet/mumya/hortlak görsellerine ölü-olmayan kimlik
  bulmak zorunlu kılındı (kabukçu, kemikçil, bedel, kurma, Solgun kabuk zırhı). Hakem 1.'si
  "Aynadeniz". **Hepsi reddedildi** — teşhis: *"Çok fazla fantastik kurgu üstüne kuruyorsun.
  Skyrim'de böyle mi? İnsan ilişkileri üzerine kur, fantastik merak ve ekstra güç sağlasa."*
- **Tur 3** (6 aday): insan omurgası + fantastik ≤%15 + **silme testi** (bütün fantastiği sil,
  hikâye ayakta mı?). Altısı da testi geçti. Hakem 1.'si "Kırkbin". **Hepsi reddedildi** —
  DÖRT kusur birden: ölçek çok küçük · fazla kuru/bürokratik · fantastik fazla azaldı ·
  büyüme hedefi yok.
- **Tur 4** (6 aday): krallık ölçeği + **iki katman** + 4+ örgüt + rütbe merdiveni + 3 "vay be" anı
  + zindan gerekçesi. Hakem elemesi: bir aday Skyrim'in birebir kopyası çıktı (Konuk Evlat), biri
  fantastiği gizlice omurga yapmış (Öte — Perde'yi ölen bir ejderha kraliçesi tutuyordu), biri
  kuruluk+isim kusurunu geri getirdi (Akocak). **Kazanan: DEMİRBOYUN.**

**DEMİRBOYUN — kanon:** Sarp Yalınkılıç 26 yıl önce altı şehri birleştirdi, birini hiç alamadı.
İki kış önce felç geçirdi — konuşamıyor, aklı yerinde. Taç giyme töreni hiç yapılmadı. Meşruiyet üç
somut nesne: çatısız taht salonuna asılacak altı sancak, sürgünün kaçırdığı taç, tapınağın
kutsaması — **ilerleme çubuğu duvarda asılı.** Odada altı ağız onun adına konuşuyor; "istediğini"
uydurabiliyorlar, **"istemediğini" uyduramıyorlar** (gözünü kapatıyor). Anahtar karakter: onu
sırtında taşıyan okuma yazma bilmeyen eski yoldaş.

İkinci katman paralel: ölü imparatorluk Akkemer ejderhaları öldüremedi, uyuttu ve mührü dinlemek
için altına adam bağladı; **Sarkaç** takvimi harita ömrünce 7 mühür çürütüyor — geciktirilebilir,
geri alınamaz. Damar taşı 14 becerinin hepsini besliyor ve **kazıldığı gece o civarın mezarlığı
kalkıyor** (kaynak toplama = tehdit, aynı fiil).

**Uygulanan hakem aşıları:** ikinci rehin çocuk Tuğrul (ilk saate ceset — mahzende gömülmemiş,
buz eriyor, "6 akçe" kontratı) · damar-hortlak yan etkisi · tören öncesi ateşkes bayramı gecesi.
**Uygulanan uyarı:** dünya asla "taç töreni" diye tanıtılmayacak — açılış cümlesi *"felçli fatihin
sandalyesi ve onun adına konuşan altı ağız."*

**Çıktılar:** `docs/LORE.md` v2 (16 bölüm) · `docs/konsept/reboot/` (24 aday + 4 değerlendirme) ·
Sudowrite Story Bible prompt paketi · eski kanon arşive alındı (silinmedi).

**B. SİSTEM TASARIMI — beceri/meslek/damga/hat (4 paralel tasarımcı)**
Hakan: *"22 olmak zorunluluğumuz yok, skill yapısı baştan sona tekrar çalışabilir. Ama varlıkları
olmalı. Balıkçılık da olmalı — oyuncu sürekli stresli olmamalı. Silaha rün basma olayı olmalı,
onun için VFX yaptık zaten."*
- **Hiçbir beceri silinmedi.** 23 → **25**: 3 yeniden adlandırma (ENCHANTING→**Damgacılık**,
  ECHO→**Ejder Sesi**, LIGHT→**Kutsama**, ayrıca SPEECH→Pazarlık, ASSASSIN→Kısık), 2 ağaç metni
  düzeltmesi (TWO_HANDED mızrak dayatması, MEDIUM_ARMOR kimliksizliği → Ak Nöbet kar zırhı),
  2 yeni (Balıkçılık, **Aşçılık** — `STATION_CAMPFIRE`'ın becerisi yoktu, XP Simya'ya düşüyordu).
- **Balıkçılık mevcut toplama sistemine BAĞLANMAYACAK:** `ResourceDef`'in sözleşmesi "hareket/hasar
  süreci keser" — gerginliğin kaynağı bu, rahat fiil onu miras almamalı. Ayrı durum makinesi
  (~50 satır), sesli işaret, kesintisiz, kaçırınca ceza yok, mob spawn'ı bastırılıyor.
  Kanon bağı: **Tuğrul'u öldüren delik balıkçı deliğiydi** (LORE §8).
- **Rahat oynanış boşa gitmiyor:** odun→kandil→gece güvenli ada; balık+yemek→soğuk direnci ve gece
  görüşü→**damar kazısını mümkün kılan şey.** Oyuncu mola veriyor sanıyor, ekipman üretiyor.
- **"Rün" gitti, "DAMGA" geldi.** Mühür büyük, damga onun kılıca sığanı; hammadde "öz" değil **harf**.
  Kanon gerekçesi: Akkemer'in mührü bir emirdir (*dur*); kılıca sığan yazı ancak bir şeyi ancak bir an
  tutar — kılıcın istediği tam olarak bu. **Beş VFX'in beşi de yerini buldu**; PINK = Sarkaç'ın
  çürüyen mühründen kesilen, olaya bağlı ve **sonlu** diri damga.
- **Damga Sarkaç'a dokunmuyor** — her damga basanı dünyayı bozmakla suçlamak fatura refleksi olurdu;
  bedel zaten dünyada (kazdığın gece mezarlık kalkıyor). Yalnız diri damga *hasadı* takvimi öne çeker.
- **Hat formülü değişti: EL + FİİL + TANIK.** Eski formül eksikti — kesişim sessiz, eylem kimliği
  duyurmuyor. Kural: kimlik davranıştan doğar, **ad başkasından gelir** (Kırca Ana unvanını hiç
  sormaz). **13 açık + 2 gizli hat** (her örgüte iki + bir örgütsüz). Hat kapanması yok, **sahne
  tükenir.** Barışçıl hattın ödülü güç değil **bilgi**.
- Detay: `docs/konsept/beceri-meslek-sinif-v2.md`, kararlar GDD §6.3.

**C. SİSTEM TASARIMI — suç/kelle/itibar** (Hakan: *"bounty vs duracak değil mi? Özgürlük sunuyoruz"*)
- **Kelle parası ceza değil BORÇ SENEDİ.** Yakalama panelindeki üç düğmeye **şehir başına değişen
  dördüncü düğme**: Yenitaç deftere yaz · Tuzkapı senet (ödemezsen liman kalıcı kapanır, sonradan
  ödemek AÇMAZ) · Yedisunak kefaret ("öde" gri) · Karaocak ocakta çalış · Karduvar mahkeme+**kefil** ·
  Kumkapı kan bedeli (vali siler, aşiret hatırlar).
- **Yargı çevresi ≠ coğrafi bölge:** `Region.lawId` + `MAX_LAWS 8` → coğrafi bölge serbest, save
  tamponuna dokunulmuyor (uyum kodda doğrulandı).
- **Haber yayılımı oynanış:** kurye NPC'si (haritada görünür, 1-3 gün). Kesersen kayıt yok olmaz,
  **sahibi değişir** — Kırk Kese bilgi olarak alır. Kanundan kurtulup örgüte borçlanırsın.
- **Hapis reddedildi** (odada bekletmek oynanış değil). Yerine borç işi/el koyma/sürgün; tek istisna
  **ölüm yerine yakalanmak** (karakolda uyanırsın, üç çıkış) — hapsi ödüle çevirir.
- **Kelle avcılığı vs suikast ayrımı tek kelime: KAYIT.** Avcılıkta tanık iyidir, suikastta ölümdür.
  Mevcut `crime_witnessed` sözleşmesine hiç dokunmadan çalışıyor.
- **Düşük itibar bir sınıf: "korkulan adam"** (rep ≤ −40: haydut saldırmaz, haraç diyalogları,
  muhafız takviye bekler; görev/şifa/Makam kapanır). **En zayıf konum orta itibar.**
- Detay: `docs/konsept/suc-kelle-itibar-v2.md`, kararlar GDD §6.4.
- NOT: kaosçu bölümü (§10-13) alt ajan **harcama limitine takıldığı için** orkestratör tarafından
  yazıldı, ajan denetimi geçmedi — belgede işaretli.

**Bulunan kod borçları (henüz düzeltilmedi):**
- `WEAPON_FX_VENOM` `light.c`'de default dalına düşüp **pembe ışık yayıyor** (gizlenme damgası için hata)
- `main.c:67` bölge `-1`'i **0'a katlıyor** → ıssız arazide işlenen suç Köy'ün defterine yazılıyor
- **Tanıksız cinayet tamamen bedava** → "kayıp" mekaniği önerildi (bir gün sonra NPC kayıp, rep −3)
- `guard_index()` tek NPC + tek statik state → altı şehir için `GuardUnit guards[12]`
- Damga eşyaları main.c'de iki taneye sabit (`CRAFT_ROW_RUNE0/RUNE1`) → paralel dizi tavanı dersi

**Kalan:** LORE §16'da 7 açık soru (Kavurgan'ın 99 yıllık şartı, gizli sınıf kapıları, 7 mührün
haritadaki yeri, Şirin Hatun'un planı, Kabukçular'ın haklı olduğu yer). Kod tarafı henüz
dokunulmadı — mevcut MVP kurgusu (isimler/diyaloglar/haritalar) yeni kanona göre yeniden
adlandırılacak; **boşa çıkan mekanik silinmeyecek, rafa kalkacak.**

### Faz KANON+IŞIK — "SÖNEN KANDİLLER" KANONU + IŞIK/GÖLGE ELDEN GEÇİRME (2026-07-29)

**A. Kanon kilidi (kod öncesi iş — Hakan direktifi "kod en son gelir")**
- `docs/LORE.md` kuruldu (14 bölüm): kanon artık orada kilitli, çelişkide LORE kazanır.
- Dört tur REDDEDİLDİ (bürokratik · soyut · **meslek dayatan** · alet dayatan);
  gerekçeleri LORE §0 iptal listesi + §14 yazım kuralları olarak kalıcılaştı.
- Hakan iki ucuzluk yakaladı ve ikisi de kalıcı kurala dönüştü:
  - **FATURA REFLEKSİ yasağı**: "geçmişte biri kolayı seçti, şimdi bedelini
    ödüyoruz" deseni. Panzehir: geçmiş karar HÂLÂ akıllı görünmeli, kriz onun
    *yan etkisi* olmalı. → Kandiller **askeri** sebeple dikildi (gece yürüyen
    ordular); ölülerin yatması **bilinmeyen bir yan etkiydi**.
  - **ROL ÖZGÜRLÜĞÜ**: kurgu asla meslek dayatmaz ("hiçbir oyuncu kürekle
    oynamak istemez ki"). Omurga dünyanın *durumu* olur, oyuncunun işi değil.
- Çekirdek: yolun taşında 2000 yıllık **isimler** birikti · kor gürültülü
  yandığı için ölü kendi adını duyamıyor · kandil sönünce duyar, kalkar ve
  **öldüğü an ne yapıyorsa onu yapar** · **AŞAĞIDA KİMSE YOK** (çağrı bir
  kayıt, iptal edilmemiş bir prosedür) · kor **korur ve ifşa eder** · **kor =
  soğumuş ejderha**. 22 hat + 2 gizli sınıf; sınıf = 3-4 skill kesişimi + bir
  dünya eylemi (tek skill sınıf yapmaz).
- 23 skill ağacının tamamı subagent'larla denetlendi; içerik kusurları çıktı
  (orta zırh ağacının item'ı yok · iki-el ağacı mızrak dayatıyor).

**B. Oynanabilir kandil demosu** (mevcut ışık/ekoloji üstüne)
- `lamp.c/h`: 16 direk, yakıt ömrü (3 gece/kor), `lamp_level_at()` bastırma.
- Ölüler YALNIZ gömüldükleri yerde kalkar (`graves` bayrağı) — ilk sürümde
  direklerin etrafında kalkıyordu, Hakan kasaba merkezinde iskelet gördü.
- Gün 300s → **1200s** (Hakan: "her şey çok hızlı gerçekleşiyor").

**C. Işık & gölge (GDD §6.2 — 6 karar + varyantlar)**
- **TEK DÜŞÜŞ EĞRİSİ**: `light_atten()` CPU'da, `fs_light` GPU'da aynı formül.
  Denetimde ÜÇ ayrı sınır bulundu (149/158/210 px) ve `LIGHT_EDGE_CUT` yalnız
  mod 0'da uygulanıyordu — F2 basmak bastırmayı sessizce 149→165'e taşıyıp
  oyuncuyu *görünür karanlıkta* koruyordu. Tek kaynağa indirildi.
- **Blinn 1988** düzlemsel gölge (tahmin değil, makale): yönlü ışıkta saf
  shear, `L = h·cot α`. Hakan: "tahmin etmesek, bunun formülü vardır."
- **Ayak noktası = alpha sınır kutusu** (sprite dikdörtgeninin altı değil):
  ağaç gölgesinin gövdeden kopmasının gerçek sebebi buydu.
- **"Zelda yalanı"**: gölge asla ekranda yukarı gitmez (kuzeye düşen gölge
  sprite'ın arkasına saklanır).
- **Titreşim aşağı yönlü** (çarpımsalda yukarı 1.0'da kırpılır), tip başına
  karakter; yarıçap modülasyonu parlaklıktan ~5.9× etkili.
- **Şekil ışık TİPİNE bağlı** (Hakan: "her ışık kaynağı farklı davranmaz mı?"):
  kandil basık+yukarı · kamp ateşi çok basık+yerde · meşale orta · büyü daire.
- **Işın-kutu tıkama** (16 gövde, slab testi): ışığın kusursuz elips olmasını
  kıran şey bu. Ağacın TACI kesmez (yapraktan sızar), gövdesi keser.
- Yarıçaplar kısıldı (Hakan: "çaplar çok geniş"): kandil 165→122 · meşale
  175→128 · kamp ateşi 230→168 · pencere 105→78 · zindan meşalesi 210→150.
- Gölge lekesi: 3 dikdörtgen oval taklidi ev boyutunda "kare kare" okunuyordu
  → kodda üretilen radyal alpha dokusu (LINEAR).
- **Gerçek hata düzeltmeleri**: `texture_create` `t->linear`'ı hiç set
  etmiyordu (her doku rastgele örnekleme — eski hata) · `light_seed` işaretli
  taşma (UB) · `lenMul = cotA * L` gölgeyi öğlen %58 kısaltıyordu ·
  `splashText` kapsam-sonrası kullanım · zindan/kuyu dibinde güneş gölgesi.
- **Test araçları** (Hakan "aynı gözüküyorlar, nereyi test edeceğim?" dedikten
  sonra): F12 A/B ekran bölme · kalıcı mod göstergesi · F1 gamma · F2 düşüş
  (CPU+GPU birlikte) · F3 dither · F4 quantize · F11 fiziksel renk (Kelvin) ·
  **Ctrl+L** şekil (Ctrl+Shift+L geri) · **Ctrl+O** tıkama.
  `[` `]` bırakıldı: Türkçe Q klavyede o konumda `ğ`/`ü` var, basılamıyordu.

**Kalan borç:** tilemap tıkaması (su `solid` olduğu için `blocksLight` ayrı
bayrak şart) · çift-karartma tek kompozit · düşük çözünürlüklü lightmap ·
kandil durumu save'e girmiyor · mezar taşı harflerinin ısınması · ışık terimi
`sneak.c`/`crime.c`'de yok (kanon §1.5'in yarısı) · kor renginin kanon/fizik
çatışması F11 ile Hakan'a bırakıldı.

### Faz POSTFX — TAM-EKRAN POST-PROCESS + 20 FİLTRE (2026-07-24, Hakan isteği)
- **Altyapı (r2d)**: SDL_GPU post-process pass'i. postfx açıkken kare pencere
  boyutlu offscreen target'a çizilir, `fs_postfx` shader'ıyla tam-ekran quad
  olarak swapchain'e basılır. Kapalıyken eski yol (doğrudan swapchain) — maliyet
  sıfır. Sahne target'ı resize'da yeniden kurulur. Işık pass'ine RGBA8 target
  varyantı eklendi (`lightPipelineTarget`) — gece aydınlatması filtre altında da
  çalışır. Gren animasyonu için r2d'de sarmalı kare sayacı (`frameNo`).
- **Shader yetenekleri** (R2DPostFX, hepsi parametrik/karışabilir): doygunluk,
  sıcaklık (turuncu↔mavi ekseni), kontrast, parlaklık, vinyet (+yarıçap),
  CRT scanline, GameBoy DMG palet kuantalama, posterize (kanal başına N kademe),
  kromatik sapma (R/B kayması), animasyonlu film greni.
- **20 ön ayar + kapalı** (main.c POSTFX_PRESETS): SICAK (varsayılan), DARK
  FANTASY, SOGUK, PASTEL, CANLI, SEPYA, NOIR, KORKU, CRT, VHS, 8-BIT, GAMEBOY,
  GUNBATIMI, AY ISIGI, RUYA, CORAK, RETRO ARCADE, ESKI FILM, SYNTHWAVE, KABUS.
  Kombinasyonlar: RETRO ARCADE (CRT+posterize), ESKI FILM (sepya+tarama+gren),
  VHS/KABUS (sapma+gren). Filtre HUD dahil TÜM kareye uygulanır.
- **F7** = preset döngüsü (kapalı dahil), splash'ta ad; oyun içi + menüde çalışır.
- NOT: SICAK açılışta aktif — kalıcı tarz henüz seçilmedi (Hakan test edecek).
  Değerler tek satır tabloda, ince ayar kolay. Vulkan/D3D için SPIR-V ekleme
  noktası load_shader()'da (yayın fazına ertelendi — MSL şimdilik yalnız Metal).

### Faz DOGU-P2 — MEVSİMLER + BÖLGESEL GÜNDOĞUMU (2026-07-24, onaylı 3 karar)
- **Hakan kararları**: 10 gün/mevsim (yıl 40 gün); DÖRT etki birden (gündüz
  uzunluğu + görsel/hava + ekonomi + ekoloji); harita başına offset altyapısı.
- **Çekirdek (gametime)**: Season enum (İlkbahar→Yaz→Sonbahar→Kış, günden
  türetilir — SAVE DEĞİŞMEDİ); mevsimsel güneş sınırları: yaz 05-21, kış
  08-17 (ilkbahar 06-19, sonbahar 07-19; şafak/alacakaranlık 2 saat).
  gametime_set_sun_offset(saat): MapDef.sunriseOffset — Taşdurak -0.75
  (45 dk erken doğar/erken batar); harita geçişinde main uygular.
  HUD saati: "Gün 13 · Sonbahar 08:30" (clockBuf 40 bayta büyüdü).
- **Görsel + hava**: gündüz ambient'ine ince mevsim tonu (sonbahar sıcak
  turuncu, kış soluk mavi, yaz doygun; gece etkilenmez — dayW ağırlığı).
  Şafak hava zarı MEVSİMSEL havuz: yaz kurak (%70 açık, kar yok), sonbahar
  yağmur+sis, kış %38 kar (mevcut kar örtüsü/snowCover sistemi kışı bedava
  karlı dünyaya çevirir).
- **Ekonomi**: item_season_trade_mult (item.c — main alım/satım + shop
  paneli gösterimi AYNI çarpanı kullanır): kış yiyecek+odun ×1.35, sonbahar
  hasadı yiyecek ×0.80. Yiyecek = foodDuration>0 (iksir etkilenmez).
- **Ekoloji**: şafak üremesi ilkbaharda +1 (bolluk), kışın hayvan sürüleri
  gün aşırı üremez (av kıtlığı) — canavar yuvaları kıştan etkilenmez.
- TEST: mevsim görmek için save day ilerletmeli (Gün 21-30 = Sonbahar,
  31-40 = Kış) ya da oyunda gün geçirmeli; Taşdurak'ta alacakaranlık
  dünyadan ~45 dk önce başlar.

### Faz MAPGEN-2 — LOCA + MEZARLIK YENİDEN ÜRETİM (2026-07-23, onaylı 3 karar)
- **Hakan kararları (AskUserQuestion)**: loca = mağara + taş salon karışımı;
  loca planı = yeni organik; mezarlık = Cave koyu seti + organik plan.
- **Loca v2 (gen_loca.py, 36×26)**: kuyu mağarası (noise oda) → dar kıvrımlı
  tünel (Çelme nöbet noktasından) → dik hatlı TAŞ SALON (inşa edilmiş — bilerek
  köşeli; batı kapısı) + zehir hücresi bölmesi (İğne, kapı 26,20). Yeni tile:
  [18] mağara zemini (düz 138,72,54), [19] mağara duvarı (üstü ten 225,163,122).
  Sabitler korunur: portal (5.5,5.5), Neriman (28,14), Çelme (21,12),
  İğne (27,22), tahta (32,19).
- **Mezarlık v2 (gen_mezarlik.py, 60×45)**: organik noise sınır + CA kuzey
  sırtı/yan yakalar (küçük alanda fill 0.58/steps 2 — 0.46/4 kenar erozyonuyla
  ÖLÜYOR, öğrenildi) + Poisson mezar siperleri + KAYA ÇERÇEVELİ ölü su
  gölcükleri (su düz doku kalır — ayrı su atlası gerekmez). Tile 13 artık
  dokusuz koyu (57,31,33) — görsel dual-grid'den. 24 sabit nokta korunur
  (portal/boss arenası/13 dekor/2 sandık/8 kamp).
- **MAĞARA DUAL-GRID (tilemap.c cave_dual_pass — parametreli)**: Cave_Walls
  atlası; ışık bloklar loca (CAVE_LUT), koyu bloklar mezarlık (MZ_LUT). Ring
  yalnız ZEMİNE bakan sınırda (duvar|void çizgisiz karanlığa karışır); güney
  duvar sırası kaya yığını YÜZÜ (satır 6); çapraz maskeler parçasız. Keyed
  atlas: bake_cave_ring.py koyu blokların düz arka planını (57,31,33)
  şeffaflar → parçalar mezarlığın gri zemininde halo bırakmaz
  (Cave_Walls_Keyed.png — türetilmiş asset, iki harita da bunu kullanır).
- NOT: mz_rock.png artık kullanılmıyor (tile 13 dokusuz); dosya duruyor.
- **DÜNYA SUYU KIYILARI (wld_water_pass)**: su(2) kıyıları Water_Tile_1
  dual-grid setiyle (çimen tutamlı toprak dudak; 1 genişlik nehirler dual'da
  doğal çalışır). Parça yalnız kıyının TAMAMI çimen(0) olduğunda basılır —
  yol/bataklık kıyısı sert kalır (parça bg'si çimen; ileride biyom başına
  set eklenebilir). Yol geçişleri ZATEN çimen-kenar sistemiyle kaplı;
  SONRAKI ADAY: taş(3) kütlelerine Kayalık Geçit için klif seti
  (Grass_Tiles_1 sağ blokları hazır bekliyor).

### Faz MAPGEN — HARİTA ÜRETİM KÜTÜPHANESİ + GÖRSEL İTERASYON (2026-07-22)
- **scripts/mapgen.py** (Hakan isteği: "daha doğal, el yapımı gibi"):
  value-noise (organik sınır/biyom) · drunkard's walk (kıvrımlı yol) ·
  cellular automata (doğal kaya/mağara kümeleri) · Poisson-disk serpiştirme ·
  stamp (el yapımı prefab) · PNG önizleme + işaretler.
- **GÖRSEL İTERASYON PROTOKOLÜ (yeni standart)**: her harita üretiminde
  script renkli önizleme kaydeder → ASİSTAN Read ile BAKAR → beğenmediğini
  değiştirir → yeniden üretir → Hakan'a son görsel sunulur. (Taşdurak v2
  iki iterasyonda: batı dokusu + güney yoğunluğu eklendi.)
- **Taşdurak v2**: organik dış sınır, CA kuzey sırtları + güney akrep
  adaları, kıvrımlı kervan yolu, noise kenarlı vaha; kod bağımlılıkları
  (giriş/kasaba/kamplar/tapınak tabanı) sabit-alan garantileriyle korunur.
- **DUAL-GRID AUTOTILE (2026-07-23, Hakan "yapay/eksik, autotile yok"
  eleştirisi üzerine)**: Cute Fantasy Desert atlasları köşe-örneklemeli
  dual-grid setmiş — tilemap.c'ye genel dsr_dual_pass eklendi: kaya(16) ve
  vaha suyu(17) hücreleri kum tabanına basılır, sınır görseli 16 parçalık
  LUT'tan yarım karo kaymış overlay'le çizilir (kenar + dış köşe + İÇ köşe
  tamam). Atlaslar: assets/tiles/desert/{water,cliff}_atlas.png.
  Eşlemeler önce PYTHON MOCK ile görsel doğrulandı (scratchpad
  mock_autotile.py) — iç köşe LUT'u ters çıktı, mock'ta yakalandı, C'ye
  doğrusu taşındı. mapgen.py'ye min2x2 eklendi (1 genişlik şerit süpürücü;
  convexify denendi — yoğun noise'da zincirleme büyüyüp haritayı yuttu,
  GERİ ALINDI). Not: dual-grid'de sınır hücre ORTASINDAN geçer — çarpışma
  hücre bazlı kaldı (yarım karo görsel tolerans, tür standardı).
- **KAYA = MESA/PLATO (Hakan 2-3. tur görsel geri bildirimi)**: koyu dolgu
  kenar ringiyle uyuşmuyordu (tuğla dikişi); rock.png dolgu da "eski tekrarlı"
  okundu. PAKET STİLİ NİHAİ: mesa üstü = KUM (zeminle aynı — sınırı dual
  dudak çizer) + güneye bakan kaya sırası KLİF YÜZÜ (atlas 9-11,2 çizgili
  duvar, L/M/R uçlu; yüz hücreleri dual pass'te kayadan sayılmaz —
  dsr_is_face) + ZEMİN BENEKLERİ (scatter, MAP_DOGU'da da çalışır: 4×3 koyu
  yama (7,7) / 2×2 açık yama (9,5) / 6 tekli çatlak (11-12,5-7) — grid'e
  hizalı). gen_dogu'ya fill_pockets (mesa içi <12 hücre kum delikleri
  kapanır — yüz kırıntısı üretmesin). Su değişmedi (Hakan onayladı).
- SONRAKİ: Hakan görsel onayı → aynı dille loca + mezarlık yeniden üretimi
  + çimen kenar sisteminin diğer biyomlara genellenmesi (çöl artık dual-grid
  ile çözüldü) + damga kütüphanesinin büyütülmesi.
- FIX: yankesicilik kapısı MAP_WORLD + nearestNpc<12'ye sabitti — çöl
  NPC'lerinde çalışmıyordu (Hakan raporu); dünya+Taşdurak + tavan 24
  (loca KASITLI kapalı — kendi loncandan çalamazsın).
- **ÇIKIŞ CRASH'İ 2. VAKA (2026-07-22 Hakan raporu, pickTries fix'li
  binary'de)**: ASan altında 1/17 reproduce; bozuk değerler bayt-sayacı
  deseninde, SDL'in mantıksal ses aygıtı listesinde çöp stream. 24 harita
  geçişi + savaş autoplay stresinde ASan TEMİZ → bizim tarafta taşma
  bulunamadı; şüphe SDL 3.4.8 ses kapanış yarışı. MİTİGASYON: üç quit
  fonksiyonu (music/ambience/audio) yok etmeden önce tüm stream'leri
  SDL_PauseAudioStreamDevice ile susturuyor. ARAÇLAR (kalıcı): CMake
  -DASAN=ON (yalnız game hedefi), env PIXE_AUTOLOAD=auto|slotNo (menüsüz
  yükleme), PIXE_AUTOPLAY=1 (dolaş+saldır+portal stresi). Tekrar gelirse:
  SDL sürüm güncellemesi gündeme alınacak.

### Faz DOGU — TAŞDURAK: DOĞU SINIR KASABASI (Kıta planı ilk adım; 2026-07-20)
- **TAVANLAR BÜYÜDÜ**: MAX_MOB_TYPES 24→32 (26 dolu), NPC_STATE_CAP 16→24
  (19 dolu) — kısıt kalktı.
- **MAP_DOGU** (100×70 çöl, gen_dogu.py seed 88): batı girişi → kasaba
  (x8-32) + vaha → doğu çölü; kuzey TAPINAK VADİSİ (mumyalar), güney akrep
  yuvaları, kervan yolu haydutları. Tile 15/16/17 (kum/kaya/vaha).
  Portal: dünya T(197,19) "Çöl Geçidi" ↔ Taşdurak T(4,35).
- **BÖLGE 6: Taşdurak** (REGIONS[5], rect 0 — region_of harita kontrolüyle
  döner): bounty/itibar köyden AYRI yaşar (sınır kasabası sicilini bilmez).
- **NPC'ler (Hakan onaylı kimlik — ticaret kasabası)**: Tüccar Ferah
  (bölgesel dükkan), Hancı Duman (söylentiler — tapınak/haydut ipuçları),
  KÂTİP RÜSTEM (Sultanlığın ilk resmî yüzü — Kıta planının tohum diyaloğu:
  "yedi beylik, üç vadi, bir taht"; sprite Çelme'yle ortak, yenisi gelince
  değişir). Diyaloglar ESO standardı.
- **ÇİFT YÖNLÜ TİCARET (onaylı)**: köy malı (deri/et/odun) Taşdurak'ta
  ×1.6 satılır; çöl malı (HURMA yeni yiyecek + BAHARAT) ucuz alınıp köyde
  ×1.5 satılır — taşıma riski = yol mobları. shopDogu bayrağı.
- **ÇÖL EKOLOJİSİ (onaylı 3 tür)**: [23] Mumya (undead — Kutsal sinerji),
  [24] Akrep (hızlı sürü), [25] Çöl Haydudu (Desert Warrior 64px düzen —
  anim offsetleri tahmini, testte doğrulanacak). 6 kamp (DOGU_CAMPS).
- CRASH FIX (Hakan raporu — çıkışta SDL_QuitAudio SIGSEGV): pickTries[12]
  NPC indeksiyle yazılıyordu, NPC_COUNT 19 olunca Taşdurak NPC'sine
  yankesicilik STACK taşırıyordu (gecikmeli patlama) → PICK_TRIES_CAP 24 +
  guard'lar. AYRICA save.c npcAliveFlag[16]+guard 16 → 19 NPC'li save
  YÜKLENEMİYORDU → 32'ye büyütüldü. Paralel-dizi dersi 4 vakaya çıktı
  (hafızada güncel); CSV GID+1 kuralı da gen_dogu'ya not düşüldü (zemin
  ölü-su okunuyordu — yürünmeme/karanlık harita raporunun nedeni).
- NOT: bölgesel gündoğumu/mevsim (vizyon) bu faza SIĞMADI — Doğu P2'ye;
  kervanci.txt'ye Taşdurak memleket referansı da P2'ye.

### Faz SUIKAST-P3 — LONCA KONTRAT SİSTEMİ + RÜTBE + DÜKKAN (2026-07-20)
- **assassin.c/h** (yeni modül): ASSASSIN_CONTRACTS tablosu (5 K1 kontratı —
  mevcut dünya mobları, yeni tür YOK; Kaçak Tahsildar kontratı gece kervan
  yolunda spawn); rütbe (Çırak/El/Bıçak/Gölge/Usta El, eşik 0/3/7/13/20);
  kontrat tahtası panel render + row_at.
- **Kontrat tahtası**: locada T(28,16), sınıf açıksa (SF≥13) [E] açar; birini
  üstlen → dünyada tamamla → tahtaya dön → "Ödülü al" (altın + rütbe puanı;
  rütbe atlarsa RÜTBE YÜKSELDİ bandı). Tek etkin kontrat kuralı.
- **mob_die kancası**: etkin kontrat hedefi düştükçe contractProgress++.
- **Loca dükkanı (İğne)**: rütbe kapılı stok — Çırak İğne Zehri, El Dilsiz
  Şişe, Bıçak İKİZ BIÇAK (sol el +%15 hız — iğnenin alternatifi, çift hançer
  build'i), Usta El Nişan. shopLoca bayrağı (lawless fiyat, kırgınlık yok).
- **Dünya etkisi**: aklama oranı rütbeyle iyileşir (kervan 0.75→0.90 cap,
  kuyu 0.50→0.72 cap) — rütbe loca kanallarını temizler.
- Player: assassinPoints/contractActive/contractProgress (save toleranslı
  kuyruk); ESC/yürü kapatma + görev takipçisi HUD satırı.
- Kızıl Kanca rakip ağı + haber ağı P4'e ertelendi (kapsam: çekirdek).
- TAVAN NOTU: MOB_TYPES 23/24, npcState 16/16 hâlâ DOLU — yeni tür/NPC
  eklemeden önce büyüt.
- FIX (Hakan test): (1) köy panosu SPRITE'ı (boardTex) + [E] ipucu harita
  koşulsuz çiziliyordu → locada "ikinci tahta" olarak görünüyordu; MAP_WORLD'e
  bağlandı. (2) köy panosu E-açma + harita geçişinde kapatma MAP_WORLD guard.
  (3) getir kontratı teslimi bag_count (iki çanta) — craft çantasındaki
  malzeme teslim edilemiyordu. (4) köy paneli 520px + ferah satır. (5) loca
  tahtası (32,19)'a taşındı (NPC'lerden uzak) + dairesel E menzili; E ipucu
  MAP_WORLD bloğundan çıkarıldı (locada hiç çizilmiyordu).
- OKUNABİLİRLİK (Hakan geri bildirimi): J günlüğüne 4. sekme KONTRATLAR
  (aktif kontrat + rütbe + NET hedef "nerede/nasıl"; sınıf yoksa ipucu);
  kontrat tahtası satırlarına konum ipucu satırı (AssassinContract.where);
  TAB döngüsü %3→%4; görev takipçisi zaten aktif kontratı gösterir.

### Faz SUIKAST-P2b — GÖLGE YOLU AĞACI + SİLAH GÖRSELİ (2026-07-20)
- T_ASSASSINTREE "Gölge Yolu" (16 düğüm — Hakan onaylı): Bıçak (sırttan
  +%3/+%5, hançer hızı, ×3.0→×3.6, stamina -%10, İnfazcı %25 altı +%25) ·
  Zehir (DPS +%10, süre +2sn, İğne CD 10→7, eşik %20→%25) · Gölge (Duman
  CD -10, süre 3→4.5, dumanda hız +%25, Adım menzili +100px) + keystone
  KARANLIK İMZA (eşik +%10, stamina -20) ve HİÇ KİMSE (dumanda ilk vuruş
  kritik + SIRTTAN SAYILIR — pusu suikasti; Duman CD +15).
- PerkEffects +15 alan; tüm beceri case'leri + pasif + Sessiz Son eşiği +
  speedMult kablolandı; Usta Bilek combat_attack_interval'da yalnız
  İmzasız Bıçak'a işler.
- İmzasız Bıçak el görseli: LPC tek-el sheet'i bağlandı (uzun kılıç katmanı
  — hançer LPC'si bulununca değişir; İğne sol el aracı, elde çizilmez).

### Faz SUIKAST-P2 — SESSİZ EL: ZİNCİR 3-5 + SINIF + 5 BECERİ (2026-07-20)
- **Hakan tasarım kararları**: Hançer+Şarjlı İğne modeli (çift hançer P3
  loca dükkanına); zehir imzası = ÖLÜMCÜL DOZ + SESSİZ SON; iğne görseli
  ründen ayrık (mor-yeşil AŞAĞI damlar, rün yeşil YUKARI çıkar).
- **GÖREV 3 "Dilsiz Şişe"**: tarif görevle öğrenilir (blueprintItem 9999 =
  yalnız craft_learn_recipe açar); 3 Zehirli Öz + 2 Mavi Ot @ simya kazanı;
  İğne'ye teslim. GÖREV 4 "Ölmeden Öl": karar diyalogda (sustur=kaba /
  rüşvet 100g=ketum / bırak=kibar — kişilik sayaçlarına işler). GÖREV 5
  "İlk İmza": Kaçak Tahsildar + 2 Koruma (MOB 21-22, Soldier sheet —
  23/24 MAX_MOB_TYPES DOLU SAYILIR!) gece kervan yolunda (T64,19) belirir;
  mob_die kancası SF 11→12.
- **SINIF AÇILIŞI (SF≥13)**: skill SUIKAST görünür (row_skill iki gizli
  satırı atlar), 5 beceri açılır, İmzasız Bıçak + Zehirli İğne + 3 şarj +
  300g + "İğne Zehri" tarifi (1 öz → 3 şarj).
- **BECERİLER**: Sırttan Bıçak (arka yay 120° kontrolü, ×3 + hançer pasifi
  ×1.5) · Gölge Adımı (arkaya ışınlanma + forceCrit) · Duman Perdesi
  (aggro sıfır + 3sn smokeTimer) · İğne Batır (doz sistemi) · Kiriş Kesme
  (kanatma + %40 yavaş). Hançer pasifi temel vuruşta da işler (+Suikast XP).
- **ZEHİR İMZASI**: Mob.venomDoses/venomDoseDps/venomTime — doz×DPS akar;
  3 dozlu hedef %20 altında SESSİZ SON (anında ölüm, boss hariç — boss'ta
  ×1.5); Alchemy ölçekler. WEAPON_FX_VENOM yeni partikül tipi; iğne+şarj
  varken silahtan damlar, şarj bitince kesilir; HUD'da 3 kare şarj göstergesi.
- F10: launderedTotal 500'e çekilir (zincir testi). abilityUsable: İğne
  Batır iğne+şarj ister; Duman/Gölge Adımı silahtan bağımsız.

### Faz ESO-V2 — GÖREV/DİYALOG SUNUMU (2026-07-20; Hakan 4 parçayı da seçti)
- **DİYALOG EKRANI**: tam genişlik alt perde (koyu zemin + altın çizgi),
  büyük portre bloğu, isim plakası (altın + alt çizgi), kamera konuşurken
  oyuncu-NPC ORTASINA süzülür + sinematik bantlar iner (cineBand paylaşımlı).
  Seçenekler numaralı + FARE hover şeridi + TIKLAMAYLA seçim (sentetik sayı
  tuşu push — seçim mantığı tek yerde kaldı); akan metin tıkla tamamlanır.
- **ANAHTAR KELİME VURGUSU**: diyalog metninde *yıldız* arası ALTIN çizilir
  (draw_markup_wrapped — kelime sarmalı + typewriter bütçeli, utf8 doğru);
  neriman.txt vurgulandı (diğer diyaloglar zamanla).
- **GÖREV GÜNLÜĞÜ**: J → Görevler sekmesine Sessiz El bloğu (aşama başına
  birinci ağız ESO metni; tamamlanınca soluk).
- **HEDEF İŞARETÇİSİ**: aktif hedef minimap'te yanıp sönen altın kare +
  dünyada süzülen altın chevron (tezgah/kuyu/Kemal/Neriman — harita farkında).
- v1'in bandı + sağ üst takipçisi korunur — v2 bunların üstüne oturur.
- DÜZELTMELER (Hakan test turu): panel yüksekliği metne göre ÖLÇÜLÜR (uzun
  metin seçenekleri taşırıyordu); satır kırma TAM kelime genişliğiyle sabit;
  ASIL BUG: markup çizicide bütçe negatife düşünce guard atlanıyordu — ilk
  kısmi kelimeden sonraki HER kelime tam çiziliyordu ("full metin + üstünden
  gölge" görüntüsü; limited/remaining modeliyle yeniden yazıldı). Diyalog
  bandı yalnız ÜSTTE (cine alt bandı panele biniyordu). UZAKLAŞINCA diyalog
  kapanır (menzil ×2 = 180px; NPC görünmez olunca da).

### Faz SUIKAST-P1 — SESSİZ EL LOCASI: KEŞİF + İLK 2 GÖREV (2026-07-20)
- **MAP_LOCA "Kuyunun Dibi"** (36×26, gen tarifi scripts/gen_loca.py): kuyu
  dibi odası → kıvrımlı tünel → loca salonu + zehir hücresi; MOOD_DUNGEON
  ambient 0.30 ama zindan DEĞİL (dalga/mob yok). Portal çifti kuyu↔loca.
- **KEŞİF (onaylı)**: launderedTotal (Player alanı — kervan+kuyu aklamaları
  sayar, save kuyruğunda) ≥500g VE GECE → kuyuda "[E] İpten aşağı in"
  belirir (nearPortal keşiften önce gizlenir — etiket sızmaz). İlk inişte
  SF_ASSASSIN=1 + Çelme: "Seni izliyorduk, aklayıcı."
- **NPC HARİTA FARKINDALIĞI (altyapı)**: NpcDef.map + npc_set_map (decor
  deseni) — update/render/nearest/visible_pos filtreli; npcState tavanı
  16→20 değil 16'da (13+3=16 TAM DOLU — sıradaki NPC'de 20'ye büyüt!).
- **KADRO (onaylı)**: 'Dilsiz' Neriman (usta — Pharaoh sheet), Gözcü Çelme
  (Desert_Person_1), Zehirci İğne (Desert_Person_4; dükkanı P3 rütbesiyle).
- **GÖREV 1 SINAV "Görülmeden" (onaylı)**: gece Baran'ın tezgahından Mühürlü
  Kutu (ITEM_SEALED_BOX) — crime_witnessed ile TANIK kontrolü: görülürsen
  bounty + görev durur (yeniden dene); tanıksız → SF=3 → Neriman'a teslim.
- **GÖREV 2 TAKİP "Gölge Ol" (onaylı)**: Kemal rutini 2→4 durağa zenginleşti
  (meydan/han önü/tarla/ev); 100-400px bandında [E] "Rapor işaretle" (durak
  bitmask SF_TAIL_MARK); <100px'te 1.5sn oyalanma = uyarı (sneak/gölge formu
  muaf), 3 uyarı = raporlar silinir; 3 durak → Neriman'a rapor → +150g,
  SF=6 (P1 SONU — P2: görev 3-5 + sınıf açılışı).
- Diyaloglar ESO standardı: neriman.txt (sorgulanabilir, ret dalı bedelli),
  celme.txt, igne.txt. SF_ASSASSIN/SF_TAIL_MARK storyFlags'te (v22 alanı).
- ESO SUNUM v1: görev bandı (kabul/aşama duyurusu) + sağ üst takipçi
  (canlı hedef metni). HAKAN: HÂLÂ YETERSİZ (2026-07-20) — sunum yeniden
  çalışılacak (muhtemel eksikler: diyalog EKRANI sunumu, görev günlüğü
  sekmesi, vurgulu anahtar kelimeler — Hakan'a soruldu).
- TB64 FIX: yüklemede spellsLearned üst 32 bit UYGULANMIYORDU — birleştirildi.

### Faz REVIR — NPC ŞİDDETİ + REVİR TAMAM (2026-07-20; plan bölümü uygulandı)
- npc_damage: SİVİL 1 canda kilitlenir ("0 ölüm demek" — Hakan); yalnız
  muhafız ölür. 1 canda main bayıltır: npc_send_infirmary (hidden + yatış
  günü + beatCount++). NPC saldırı/suç sistemi ZATEN VARDI — üstüne oturdu.
- ŞİFACI ELA (yeni NPC — npcState tavanı 12→16, NPC_COUNT 13): revir köy
  KB'sinde (3 yatak dekoru T3.6-5.4,6.2); yatanlar yataklarda SOLUK çizilir
  (npc_render_infirmary) + "(yatıyor)" etiketi.
- MASRAF: infirmary_cost — 200g × 1.25^gün × (önemli NPC 1.5) × yatışSayısı;
  ödeme sifaci.txt diyaloğundan ("sifa:ode" TÜM yatanları tek seferde
  taburcu eder); "sifa:tedavi" 60g tam can (oyuncu hizmeti — bonus).
- 30 GÜN ödenmezse şafakta köy toplar: NPC döner ama GRUDGE kalıcı —
  diyalog soğuk (bark), dükkan fiyatı sana +%25 (shopVendorIdx okur),
  köy itibarı düşer.
- sifaci.txt ESO anlatım standardının İLK örneği (kişilikli, sorgulanabilir,
  "defter" metaforu). Save: revir durumu toleranslı kuyrukta (NPC başına
  yatış/gün/sayı/kırgınlık — eski save'de herkes ayakta).

### VİZYON — KITA PLANI: DOĞU & BATI (2026-07-20, Hakan kararlarıyla SABİTLENDİ)
**Büyük resim (Hakan)**: Kıta ölçekli dünya — DOĞU ve BATI blokları + ortada
BAĞIMSIZ BÖLGE (mevcut köy = başlangıç). Oyuncu istediği tarafa hizmet/ihanet
eder, İKİLİ AJANLIK oynayabilir, köy hayatında kalabilir, siyasete hiç
karışmayabilir. İstihbarat + suikast birimleri her blokta; kanunsuzlar
(Sessiz El bu eksenin tohumu). Biyomlar: çöl, kar, akarsular, kurak
topraklar, tekinsiz bölgeler, taht toprakları.

**ONAYLI KARARLAR (2026-07-20)**:
1. **Harita modeli: BÖLGE HARİTALARI** — kıta, sınır geçişleriyle bağlı
   bölge haritalarından oluşur (her biri ~mevcut dünya boyutunda; portal/
   sınır geçişi mevcut harita altyapısıyla). Faz faz büyür.
2. **İlk genişleme: DOĞU (çöl sultanlığı)** — Cute Fantasy Desert hazır
   (Pharaoh/çöl halkı/Mummy ekolojisi). İkinci yerleşim = DOĞU SINIR
   KASABASI (kervan yolunun ucu — Kervancı Selim'in geldiği yer; mevcut
   kervan olayı bu yola bağlanır).
3. **Hizip modeli (Hakan genişletti)**: Doğu ve Batı BLOKLARI; her blok
   İÇİNDE BİRDEN FAZLA krallık/beylik/sultanlık + muhtemelen her blokta
   BİRER İMPARATORLUK (blokun ağırlık merkezi). Çekişme HER DÜZEYDE:
   bloklar arası + blok İÇİ (beylikler birbirine entrika çevirir,
   imparatorluğa diş biler). "Çok zengin evren" — itibar çok katmanlı:
   blok itibarı + tekil güç itibarları (bölgesel itibar sistemi genişler).
4. **Siyaset: OLAY TABANLI GERİLİM** — sınır gerginliği worldevent
   sistemiyle yaşar (baskın/elçi/ambargo/suçlama); BÜYÜK kırılmalar
   (savaş/barış) hikaye fazlarıyla. Oyuncu görevleri teraziyi OYNATIR
   (istihbarat/suikast sonuçları olaylara işler).

**MEVCUT SİSTEMLERLE BAĞ**: bağımsız bölge = mevcut köy (retrofit yok);
Perde 2 kalesi → Batı'nın sınır kalesi olarak konumlanabilir; Suikastçi
K3/K4 hedef kademeleri bu güçlerin figürlerine bağlanır; Kızıl Kanca rakip
ağı bloklar arası kaçakçılıkla anlam kazanır; ünvan/kişilik/itibar
diyaloglara blok algısı ekler ("Doğulu tüccar sana farklı fiyat verir").

**EK NOT (Hakan, 2026-07-20) — BÖLGESEL GÖKYÜZÜ + MEVSİMLER**:
· Doğu ve Batı'da GÜN DOĞUMU/BATIMI FARKLI olmalı (kıta genişliği hissi:
  Doğu'da güneş erken doğar, Batı'da geç batar) — bölge haritası başına
  gün eğrisi ofseti (gametime/light sistemine bölgesel parametre).
· MEVSİM SİSTEMİ: gün doğumu/batımı mevsime göre de kayar (kış geç doğar
  erken batar, yaz tersi). Mevsim; hava zarlarını (kar kışın sık), doğa
  kaynaklarını (ot mevsimi) ve ileride ekonomiyi (hasat) de besleyebilir.
· Teknik çapa: gametime'a SEASON katmanı (gün sayısından türetilir,
  örn. 30 günlük 4 mevsim) + MapDef/bölgeye sunriseOffset alanı; ambient
  ışık eğrisi bu ikisinden hesaplanır. Üretimi Doğu bölgesi fazına bağla.

**ÜRETİM İLKESİ**: vizyon sabit, üretim FAZ FAZ — sıradaki somut adım:
Doğu sınır kasabası (bölge haritası + sultanlık ilk teması + ticaret
ekonomisi). Batı, Perde 2 ile açılır. Hiçbir faz bu belgeye referanssız
"büyük dünya işi" diye açılmaz (kapsam disiplini).

### PLAN — REVİR SİSTEMİ: NPC ŞİDDETİ + HASTANE (2026-07-20 Hakan onayları)
**Konsept**: Suikast kontratları DIŞINDA da NPC'lere saldırılabilir; köy
NPC'sinin canı 0'A DÜŞMEZ — 1 CANDA KİLİTLENİR ve bayılır (Hakan netleşmesi
2026-07-20: "0 ölüm demek" — köylüde ölüm eşiğine hiç inilmez), REVİRE
taşınır. Suikast hedefleri ise gerçekten 0'a iner ve ölür (loca zehri) —
sıradan şiddet ile suikastın kurgusal ve MEKANİK ayrımı net.

**ÖNKOŞUL**: NPC'lere saldırı mekaniği (şu an yok) — NPC canı + oyuncu
vuruşu + TANIK entegrasyonu (tanıklı saldırı = mevcut suç sistemi: bounty +
bölge itibarı; hastane masrafı buna EK cezadır).

**REVİR DÖNGÜSÜ**:
- Yaralanan NPC revire taşınır: dükkanı/görevleri/diyalogları KAPALI —
  masraf ödenene kadar dünya gerçekten aksar (asıl ceza bu).
- MASRAF (onaylı): taban 200g + günlük +%25 BİLEŞİK artış
  (3. gün ~312g, 7. gün ~763g, 10. gün ~1490g); önemli NPC (dükkancı/görev
  taşıyan) ×1.5. Ödeme Şifacı'ya yapılır; ödenince NPC ertesi şafak döner.
- ÖDENMEZSE (onaylı — "30 günde köy toplar, ama..."): 30. gün köylüler
  aralarında toplar, NPC döner AMA AFFETMEZ: diyalogları kalıcı soğur
  (yeni kırgın diyalog varyantları), fiyatları oyuncuya kalıcı zamlı
  (+%25), köy itibarı düşer. Ekonomik ceza sosyal cezaya dönüşür.
- Aynı NPC'ye tekrarlanan şiddet: masraf çarpanı katlanır (×2, ×3...) —
  seri dayakçılık ekonomik olarak imkânsızlaşır.

**ŞİFACI + REVİR (onaylı — yeni kurum)**:
- Köye küçük REVİR binası + ŞİFACI NPC (yeni — npcState tavanı 12→16
  BÜYÜTÜLECEK, paralel-dizi dersi!). Masraf ödeme diyaloğu onda.
- Bonus derinlik: Şifacı oyuncuya da paralı tedavi satar (anında tam can /
  zehir-kanama temizliği); ileride iksir stoku. Yatan NPC'ler revirde
  YATAKTA GÖRÜNÜR (görsel geri bildirim — kim yatıyorsa görürsün).

**SAVE**: revir durumu (yatan NPC'ler + yatış günü + ödenmemiş masraf +
kırgınlık bayrakları) toleranslı kuyruk deseniyle.
**SIRALAMA**: Suikastçi P1'den ÖNCE ya da birlikte yapılabilir — NPC
saldırı mekaniği suikast kontratlarının da önkoşulu (ortak altyapı).

### PLAN — SUİKASTÇİ SINIFI: "SESSİZ EL LOCASI" (2026-07-20 Hakan onayları)
**Konsept**: 2. gizli sınıf. Yankıcı = mistik/bedel-CAN büyücüsü; SUİKASTÇİ =
FİZİKSEL gölge savaşçısı (stamina + zehir + pozisyon + patlama hasarı).
Keşif davranıştan doğar (tasarım felsefesi) — anlatım ESO standardında.

**KEŞİF + AÇILIŞ (onaylı)**: Kör Kuyu'nun DİBİ = locanın kapısı. Kirli Mal
sistemiyle organik bağ: toplam aklanan altın eşiği aşınca (öneri: 500g+)
bir gece kuyudan İP sarkar — "seni izliyorduk". İnen oyuncu SESSİZ EL
locasını bulur (yeni yer altı haritası: MAP_LOCA — kuyu dibi + kaçakçı
tünelleri). Sınıf, 5 görevlik zincir TAMAMLANINCA açılır.

**GÖREV ZİNCİRİ (5 görev, WoW yapısı — ardışık, her adım öncekini okur;
anlatım ESO: zengin diyalog, seçenekli sorgulama, kişilikli NPC ağzı)**:
1. SINAV — "Görülmeden": loca ustası (adı sonra) köyden bir eşyayı TANIKSIZ
   çalmani ister (sneak+pickpocket sınavı; yakalanırsan görev sıfırlanır)
2. TAKİP — "Gölge Ol": bir NPC'yi gün boyu takip et, 3 durağını rapor et
   (rutin sistemi zaten saat bazlı — takip mesafesi + görülmeme koşulu)
3. TEDARİK — "Dilsiz Şişe": zehir hammaddesi topla + simya kazanında kaynat
   (Alchemy sinerjisi; tarif locadan)
4. SAHTE ÖLÜM — "Ölmeden Öl": locanın sahnelediği ölümde figüran ol (vinyet
   + karar: tanığı sustur / rüşvet / bırak — kişilik sayaçlarına işler)
5. BÜYÜK HEDEF — "İlk İmza": gerçek suikast (aşağıdaki kontrat kurallarıyla).
   Tamamlanınca: SINIF AÇILIR + Suikastçi silahı (özel hançer — Yankıcı
   kuralı gibi: keşiften önce HİÇBİR kaynaktan düşmez)

**BECERİLER (onaylı 5'li — SKILL_ASSASSIN yeni okul, XP: gizli öldürme +
sırttan hasar; ağaç sonra ayrı fazda)**:
- SIRTTAN BIÇAK (KO Backstab): arkadan ×3.0 — pozisyon oyunu
- GÖLGE ADIMI (WoW Shadowstep): hedefin arkasına ışınlan + garantili kritik
- DUMAN PERDESİ (Vanish): anında görünmez + aggro sıfır, 3sn
- ZEHİR SÜRME: 60sn silaha zehir DoT (Alchemy iksirinden üretim)
- KİRİŞ KESME: kanatma + %40 yavaş 4sn (kaçanı yakalar)

**LONCA KONTRAT SİSTEMİ + RÜTBE (Hakan ek isteği 2026-07-20)**:
- Loca ARA ARA görev/suikast ister (pano deseni: contract.c benzeri ama
  locaya özel — hedef gerçek NPC ya da adlı mob; rastgele değil KURGULU havuz)
- RÜTBE: kontrat tamamladıkça yükselir (Çırak → El → Bıçak → Gölge → Usta El)
- **HEDEF MERDİVENİ (onaylı 2026-07-20 — rütbe + dünya büyümesi birlikte;
  "en başından kral öldüremezsin")**:
  · K1 ÇIRAK/EL "ısınma işleri": dünyaya OLAY olarak doğan hedefler — haydut
    elebaşıları, göç kolonisi reisleri (ekoloji), kervana sızmış dolandırıcı.
    Köy dokusu bozulmaz; etki bölge güvenliği/ekolojide.
  · K2 BIÇAK "ağ savaşı": gezgin tüccarlar + RAKİP AKLAMA AĞI "KIZIL KANCA"
    (onaylı): kervan yollarında adamları belirir (worldevent deseni);
    kontratlarla zayıflar → aklama oranı + loca fiyatları iyileşir, yol
    güvenliği artar. MİSİLLEME: rakip sana suikastçi yollar (gece pusuları)
    — dünya iki yönlü yaşar.
  · K3 GÖLGE "şehrin içi" (İKİNCİ YERLEŞİM önkoşul): yeni yerleşimin ileri
    gelenleri — lonca ustası, tefeci, yozlaşmış yargıç; ölümleri o yerleşimin
    ekonomisini/diyaloglarını/görevlerini KALICI değiştirir.
  · K4 USTA EL "tahtın gölgesi" (PERDE 2 kale/krallık önkoşul): muhafız
    komutanı, vezir, taht hattı — çok aşamalı BÜYÜK AV kontratları (keşif →
    takip → zehir → infaz tek kontratta). KRAL = finale adayı.
  · Kademeler hem rütbe hem İÇERİK kapısı: coğrafya büyüdükçe loca büyür.
- **YUVADA AVLANMAZ (onaylı)**: köy NPC'lerine kontrat ÇIKMAZ — loca kuralı
  kurguya yazılır (köy locanın gölgesi; Morag Tong ilkeselliği). Köy içeriği
  korunur, loca karakter kazanır.
- DÜNYAYA ETKİ (zorunlu tasarım kuralı):
  · Öldürülen NPC DÜNYADAN KALKAR (npc_set_alive kalıcı) — dükkanı kapanır,
    diyalogları/görevleri ölür, köy ekonomisi etkilenir (fiyat/stok)
  · TANIK varsa suç sistemi işler: bounty + bölge itibarı düşer; temiz iş
    = loca övgüsü + ekstra ödeme
  · Haber ağı: ertesi şafak NPC'ler cinayeti konuşur (worldevent haber deseni)
  · Rütbe etkileri: loca dükkanı kademeli açılır (zehirler/suikast ekipmanı),
    kuyu aklama oranı rütbeyle iyileşir (0.5 → 0.65), yüksek rütbede gece
    sokakta loca selamı (bazı NPC'ler farklı konuşur), muhafız şüphesi artar
- Kontrat REDDİ de kimlik: art arda ret → loca soğur, kontratlar seyrekleşir

**ANLATIM STANDARDI (bundan sonra TÜM görevlerde — ESO tarzı, ses yok)**:
görev veren kişilikli anlatır (kısa değil, dünyasını açar); oyuncuya
SORGULAMA seçenekleri (görevi almadan "neden ben?", "kim bu adam?" dalları);
görev metni günlüğe NPC'nin ağzından düşer; tamamlama diyaloğu sonucu okur
(nasıl yaptıysan ona göre tepki). Eski görevler dokunulmaz, zamanla elden geçer.

**TEKNİK NOTLAR**: MAP_LOCA enum SONA; skill/ability enum SONA (spellsLearned
64-bit'e genişletildi — bkz. Faz TB64); loca NPC'leri npcState tavanına DİKKAT
(11/12 dolu — tavan büyütme gerekecek); rütbe+kontrat durumu save'e toleranslı
kuyrukla. Sıralama önerisi: P1 harita+keşif+zincir 1-2 → P2 zincir 3-5 +
sınıf+beceriler → P3 kontrat sistemi+rütbe+dünya etkileri.

### ASSET SIRASI (2026-07-17 — Hakan'ın indirdiği paketler, onaylı işleme sırası)
İndirilenler (~/Downloads):
- **Harita**: Free-Undead-Tileset (açık hava lanetli arazi — LANETLİ MEZARLIK
  haritası onaylı) + Cute Fantasy tile/dekor (dünya görsel yenileme ONAYLI:
  autotile geçişler + varyant + doku + dekor serpiştirme)
- **VFX**: Super Pixel Effects Gigapack (Lightning/Bursts/Spells/Explosions/
  Impacts) + Free Pixel Effects Pack (20 sheet: weaponhit/magickahit/freezing/
  felspell/phantom/flamelash...) + Brackeys VFX + Pixel Fire v1.2 + Ice Effect
  01 + Warped Shooting FX + Holy VFX 01-02 + Dark VFX 01-02 (Yankıcı'da kısmen
  kullanılıyor)
- **Mob/Boss**: Skeleton enemy · Evil Wizard 2 · Bringer-of-Death (boss adayı)
  · SATYR · Tiny RPG Demon_A + Blood Monster_A (2 saldırılı — elit/mini-boss)
- **Müzik**: xDeviruchi 16-bit Fantasy & Adventure (loopable — gerçek müzik seti)
- **UI**: Pixel UI pack 3 (panel/çerçeve yenileme)
- **Diğer**: Dimensional_Portal.png (geçit/Perde 2 görseli) · Destructible
  Objects sheet (kırılabilir varil/kasa — harita fazında loot objesi mekaniği
  adayı) · Pixel Art Slashes (melee vuruş yayları — VFX cilasında weaponhit
  ile birlikte) · 'Free' Part 16-23 (renk varyantlı patlamalar) · DG Fire Zone (volkan/ateş
  bölgesi tile'ları — ileriki ateş biyomu adayı)

ONAYLI SIRA: ① Dünya görsel yenileme (BAŞLADI — keşif bitti) → ② Lanetli
Mezarlık haritası → ③ VFX his cilası (tüm savaş efektleri) → ④ Müzik seti
değişimi → ⑤ Yeni mob dalgası + boss → ⑥ Kutsal hattı (Holy) → UI yenileme
uygun araya. YANKICI Parça 1 iskeleti kodda (tamamlanacak).

### YAKIN — "oyun gibi hissettir" (his + rol)
1. ~~CİLA PAKETİ~~ TAMAM (2026-07-14 — bkz. Faz CİLA bölümü); kalan küçük
   UX borcu: speech zar kilidi save'i (uygun faza serpilir)
2. **HİKAYE KARTLARI DALGA 1** (K9+K2+S3 seçildi 2026-07-14; kart başına commit):
   ~~D1a: sahne araçları + K9 Kandil~~ TAMAM (bkz. Faz D1a bölümü)
   ~~D1b: K2 Son Müşteri~~ TAMAM (bkz. Faz D1b bölümü)
   ~~D1c: S3 Sayan Adam~~ TAMAM (bkz. Faz D1c) — **DALGA 1 KAPANDI**
   + kalan alışkanlık: kişilik+itibar sayaçlarının mevcut köy diyaloglarına
     serpilmesi (`req: kaba >= 3` — her dokunulan diyalogda sürer)
3. ~~KARAKTER YARATMA-LİTE~~ TAMAM (bkz. Faz KY) — **YAKIN KADEMESİ KAPANDI**

### ORTA — "dünyayı büyüt"
4. **İKİNCİ YERLEŞİM + İLK YENİ BİYOM** (çöl ya da mantar): yeni NPC'ler +
   bölgesel ekonomi (fiyat farkları → ticaret), yeni ekoloji aileleri
   (Bombschroom/Mummy vb. buraya), POI + yankı serpiştirme, kartlar 2. dalga;
   mob fazları devamı bu şemsiyede (Shroomling → Mummy/Cowling/Flying Skull;
   kırmızı/sarı/pembe slime; iskelet "mezardan kalkma" animasyonu)
5. **ONAYLI 4 MEKANİK** (2026-07-11) — MEKANİK-ÖNCELİKLİ hat (2026-07-14 pivotu:
   sıra öne çekildi; savaş+yaşam üçlüsü: M1 sinerji ✓ → M2 pişirme → M3 ünvanlar):
   · ~~Element sinerjileri~~ TAMAM (bkz. Faz M1)
   · ~~Pişirme + yemek buffları~~ TAMAM (bkz. Faz M2)
   · ~~Davranış ünvanları~~ TAMAM (bkz. Faz M3) — savaş+yaşam üçlüsü KAPANDI
   · ~~Summon hattı~~ TAMAM (bkz. Faz M4) — ONAYLI 4 MEKANİK TAMAMLANDI
   · Summon hattı (yılan DoT + kaplumbağa tank; bedel: max mananın %25'i rezerve;
     sprite'lar: `~/Downloads/snake_right.png, turtleshell_right.png`)
   · Pişirme + yemek buffları (kamp ateşi + av döngüsü)
   · Davranış ünvanları ("Slime Kasabı" — kimlik davranıştan doğar vitrini)
6. ~~**DÜNYA OLAYLARI tam sistemi**~~ TAMAM (bkz. Faz WE + WE2): çekirdek,
   hava sistemi, haber ağı diyalogda, Huzursuz Mezarlık, mob arama

### UZAK — "derinleştir ve kapat"
7. **ANA HAT PERDE 2**: Lord sonrası fısıltının çağrısı + ikinci büyük zindan/boss
   (boss scroll/taş dropları burada anlam kazanır)
8. **Kalan sistemler**: Two-Handed hattı (spear — asset:
   `~/Downloads/lpc_male_item_animations_2026-07-11T13-46-16`; Block/kalkan aynı aile),
   ~~Pickpocket~~ TAMAM (bkz. Faz PP), Büyü dalgası 2 ~~parça 1 (pençe/diken/hortum + tomar)~~ TAMAM (bkz. Faz BD2a) — ~~kalan parça 2~~ TAMAM (magic_pack kalanları:
   tornado/firelion/icetacle/lightningclaw/snakebite), kalan saga'lar + finale bağları
9. **YAYIN HAZIRLIĞI**: F10 söküm, CREDITS lisans teyitleri, Vulkan/SPIR-V,
   mobil build (SDL_GPU→Metal/Vulkan), harita editörü akışı

Ertelenen küçükler (uygun faza serpilir): yay walk/idle görseli (asset gelince),
Combat'a aktif savunma sorusu (dodge/block — Two-Handed/Block fazında tartışılır).

## Karar defteri (değişmez)
- Sınıf yok; kimlik davranıştan: use-based skill + perk (3-seç-1) + bölge/hizip itibarı (global karma YOK)
- Suç = tanık varsa; tam muhafız döngüsü onaylı
- Bölge-sabit zorluk, level-scaling yok · Combat A (ıska yok) · KO auto-attack + hotbar
- Perk eşikleri 25/50/75/100 · Ölüm: köyde diriliş + %10 altın
