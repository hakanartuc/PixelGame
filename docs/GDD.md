# PIXE GAME — Oyun Tasarım Dokümanı (GDD)

> **Belge kılavuzu:** Her madde bir statü taşır:
> **[KODDA]** oyunda çalışıyor · **[ONAYLI]** Hakan onayladı, üretim sırada ·
> **[TASLAK]** asistan önerisi — Hakan onayı bekler · **[AÇIK]** karar verilmedi.
> Bu belge canlıdır: her tasarım kararı önce buraya işlenir, kod onu izler.
> Bakış TEPEDEN: burada iskelet kurulur; karakter/görev yazımı iskelete sonra asılır.
> Üretim geçmişi PROGRESS.md'de, teknik devir HANDOVER.md'de, derin kurgu **LORE.md**'de.
> **LORE.md kuruldu (2026-07-28) ve kanon kilidi orasıdır** — dünya/halk/örgüt/ölü/zanaat/
> hat/final çelişkisi çıkarsa LORE.md kazanır. Bu belge **tasarım kararlarını**, LORE.md
> **dünyayı** tutar. Sıra: LORE → GDD → yapı → hikaye → görev → **kod en son**.

> ## ⚠️ MVP RESET — 2026-07-24 [Hakan direktifi]
> **Oyundaki mevcut KURGU sıfırlandı.** Bugüne kadarki hikaye, görevler,
> karakterler, hizip ve yer isimleri (Taşdurak, Sessiz El, Neriman, Fatma/
> Kerem, K9 Kandil, mühür hattı, mevcut zincirler...) **MVP denemesiydi —
> bağlayıcı DEĞİL, referans alınmaz.** Kurgu sıfırdan, "canlı gerçek"
> tasarlanacak; yeni kanon LORE.md'de doğacak ve kod ona uyarlanacak
> (harita/NPC/diyalog isimleri yeni kurguya göre yeniden adlandırılır).
>
> **SIFIRLANMAYAN — aynen geçerli:**
> - §1 tasarım sütunları · §2 döngüler · §4 **tüm mekanikler** (savaş,
>   gizlilik, itibar/suç/revir, ekoloji, ekonomi, hava/mevsim, kontrat...)
> - İSKELET modelleri: §3.1 kıta modeli · §3.2 bölge şablonu · §3.4 zincir
>   mimarisi · §3.5 ortak düşman çerçevesi · §3.6 çoğul örgüt modeli
> - §6 sanat/ses · §7 UI · §8 teknik
>
> Aşağıdaki §3.3 ırk matrisi ve somut yer/hizip adları **MVP kalıntısıdır**;
> yeni kanon geldiğinde tamamen değişecek.
>
> ### ✅ YENİ KANON GELDİ — "SÖNEN KANDİLLER" (2026-07-28)
> Beş tur sonunda kanon `docs/LORE.md`'ye kilitlendi. Dört tur reddedildi
> (bürokratik · soyut · meslek dayatan · alet dayatan) ve gerekçeleri
> LORE.md §0 + §14'te kalıcı kural olarak duruyor.
> **Çekirdek:** kandiller askeri sebeple dikildi, ölülerin yatması bilinmeyen
> bir yan etkiydi; yolun taşında iki bin yıllık **isimler** birikti, kor
> gürültülü yandığı için ölü kendi adını duyamıyor. **Aşağıda kimse yok** —
> çağrı bir kayıt. Kor **korur ve ifşa eder**. Kor **soğumuş ejderhadır**.
>
> **§3.3 ırk matrisi ve MVP yer adları artık LORE.md §4/§6 ile değiştirilmiştir**
> (metin aşağıda duruyor, silinmedi — bkz. §10 doküman disiplini ve LORE §14.8).
> **§3.5'in [AÇIK] bırakılan "ortak düşman hikaye kimliği" kapandı:** ortak
> düşman bir ırk ya da ordu değil, **sönen kandil ve altındaki kayıt.**

---

## 1. VİZYON

**Tek cümle:** Çok bölgeli, yaşayan BÜYÜK bir kıtada kimliğini oynayışınla
inşa ettiğin, offline, top-down 2D pixel-art RPG — "Skyrim ruhu, KO dövüşü".

### 1.1 Tasarım sütunları (değişmez — Karar Defteri) [ONAYLI]
1. **Kimlik davranıştan doğar** — sınıf seçimi yok; use-based skill +
   perk (3-seç-1, eşik 25/50/75/100) + bölge/hizip itibarı. Global karma YOK.
2. **Derinlik + özgürlük** — kısıtlayıcı denge kuralları reddedilir;
   dünya level-scaling yapmaz (bölge-sabit zorluk).
3. **Yaşayan dünya** — ekoloji, hava, mevsim, dünya olayları, NPC rutinleri.
4. **Eylemin bedeli** — suç=tanık; bounty+itibar; sivil ölümü YOK (revir).
5. **Savaş: Combat A** — ıska yok, KO auto-attack + hotbar; ölüm: köyde
   diriliş + %10 altın.
6. **[ONAYLI — 2026-07-24] GENİŞLETİLEBİLİR TEMEL** — dünya ŞABLONLARLA
   büyür (bölge şablonu §3.2, zincir mimarisi §3.4, örgüt çerçevesi §3.6):
   yeni harita-ırk-görev eklemek = şablon doldurmak. Temel düzgünse üstüne
   eklemek ucuz ve tutarlıdır; hiçbir içerik şablonsuz, elle "tek seferlik"
   eklenmez.

### 1.2 Anlatım standardı [KODDA]
ESO tarzı sunum: kişilikli, sorgulanabilir diyaloglar; sinematik perde,
*altın vurgu*, typewriter; voice acting yok; Türkçe metin, dosya tabanlı.

### 1.2b İNSAN ELİ KURALI [ONAYLI — Hakan 2026-07-24]
**Oyundaki ve belgelerdeki her metin insan yazmış gibi okunmalı.** Yapay
zekâ metninin tanınabilir izleri yasaktır:

- **Simetri yasağı.** Her halkın üç özelliği, her gücün bir zaafı, her
  bölümün eşit uzunluğu olmasın. İnsan yazarı dengesiz yazar: bir şeye
  bayılır ve üç paragraf ayırır, başka bir şeyi bir cümlede geçer.
- **Tablo, anlatının yerine geçmez.** Tablo veri içindir (sayı, konum,
  eşleşme). Bir halkı, bir karakteri, bir yeri **paragrafla** anlat.
- **"Sadece X değil, aynı zamanda Y"** ve benzeri denge kalıpları,
  gereksiz üçlemeler ("hızlı, güçlü ve akıllı"), her paragrafı bağlayan
  sonuç cümlesi — hepsi AI kokar. At.
- **Her şeyi tanımlama.** Bazı şeyler sezdirilir, açıklanmaz. Okur
  boşlukları kendi doldurabilmeli; her kavramın parantez içinde etimolojisi
  olmasın.
- **Somut ve beklenmedik detay** soyut nitelemeye yeğdir: "gururlu bir
  halk" değil, "misafire önce atının suyunu sorarlar".
- **Kişisel ses.** Metnin bir bakışı olsun; ansiklopedi maddesi değil,
  bir şeyi anlatmayı seven birinin sesi.
- Diyalogda: eksik cümle, laf arasında bölünme, konu değiştirme, aynı
  şeyi iki kez farklı söyleme — insanlar böyle konuşur.

Bu kural belgelere de uygulanır: LORE.md bir insan tarafından yazılmış
kitap gibi okunmalı, üretilmiş bir rapor gibi değil.

### 1.3 Hedef / platform [AÇIK]
Masaüstü (SDL3 taşınabilir); mobil ihtimali lisans teyidine bağlı.
Fiyat/kitle/pazar konumu kararsız.

### 1.4 ÇOK DİLLİLİK [ONAYLI — Hakan 2026-07-24]
Oyun **çok dilli** olacak: **Türkçe + İngilizce kesin**; ileride İspanyolca/
Almanca/Fransızca eklenebilir. Bunun tasarıma ve koda etkileri:

- **İsimlendirme kuralı** [ONAYLI]: özel isimler (kişi/yer/örgüt) her iki
  dilde de **telaffuz edilebilir** olmalı. Türkçe'ye özgü karakterlerde
  (ç ğ ı ö ş ü) ölçülü olunur — bir ismin İngilizce konuşan oyuncunun
  ağzında dağılmaması esastır. **Özel isimler ÇEVRİLMEZ**, aynı kalır;
  yalnız unvan/sıfat kısımları çevrilir ("Gözcü Çelme" → "Çelme the
  Watcher" kalıbı).
- **Metin yazımı** [ONAYLI]: diyalog ve anlatı metinleri çeviriye uygun
  yazılır — kelime oyunu, yerel deyim ve argo ölçülü kullanılır; kullanılan
  yerde çeviri notu bırakılır. Cinsiyet/çoğul eki varsayımı yapan cümle
  kurulmaz (İngilizce/Almanca'da bozulur).
- **Teknik** [AÇIK — borç]: şu an metinler tek dilde ve dosyalara gömülü
  (`assets/dialogs/*.txt`, kod içi HUD/UI stringleri). Lokalizasyon
  altyapısı gerekecek: dil başına metin dizini + anahtar tabanlı string
  tablosu + UTF-8 font kapsaması (aksanlı Latin harfleri) + değişken
  uzunluklu metne dayanıklı UI. **Yeni yazılan her metin bu geçişe hazır
  olmalı** (metin koda gömülmez, dosyadan gelir).

---

## 2. OYUN DÖNGÜLERİ

- **An-be-an** [KODDA]: keşfet → tehdit → savaş/gizlen/kaç → yağma → güçlen.
- **Oturum** [KODDA]: şafak zarı (hava+olay+ekoloji) → gün planı (görev/
  kontrat/ticaret/craft) → gece işleri ya da güven → dinlen [R].
- **Meta (kimlik)** [KODDA/büyüyor]: davranış → skill/perk/ünvan → gizli
  yollar → örgüt rütbeleri → bölge itibarları → blok siyaseti terazisi.
- **Ekonomik** [KODDA]: kaynak → craft → bölgeler arası ticaret →
  mevsim dalgası → altın → ekipman/masraf.
- **Anlatı döngüsü** [ONAYLI — yeni]: lore sayfası/söylenti bul → zincir
  düğümü açılır → bölge zinciri ilerler → ana zincire kanca + dünya olayı
  tetiklenir → yeni bölge/ilişki açılır (bkz. §3.4).

---

## 3. DÜNYA (ana iskelet)

### 3.1 Kıta modeli [ONAYLI — 2026-07-20 + 2026-07-24 genişletme]
- **DOĞU ve BATI blokları** + ortada **BAĞIMSIZ BÖLGE** (mevcut köy).
- Her blokta birden fazla krallık/beylik/sultanlık + **birer İMPARATORLUK**.
- Çekişme her düzeyde (bloklar arası + blok içi) **+ üçüncü eksen: ORTAK
  DÜŞMAN (§3.5)** — iki bloğun da üstünde, hikayesi sonra belirlenecek.
- **Ölçek [ONAYLI]: ÇOK BÜYÜK dünya** — bölge ve yerleşim sayısı mevcut
  7 haritanın çok üstüne çıkacak; kıta, sınır geçişleriyle bağlı bölge
  haritalarından oluşur ve faz faz büyür.
- Siyaset olay tabanlı (worldevent) + büyük kırılmalar hikaye zincirleriyle.

### 3.2 BÖLGE ŞABLONU [ONAYLI — genişletilebilir temelin kalbi]
Her yeni bölge şu paketi doldurarak doğar (eksik paketle bölge açılmaz):

| Alan | İçerik |
|---|---|
| **Harita(lar)** | mapgen script + sabit nokta garantileri + biyom/autotile seti |
| **Yerli ırk(lar)** | bölgeye ÖZGÜ ırk/halk + arkaplan hikayesi (§3.3) |
| **Yerleşim(ler)** | kasaba/kale/kamp; dükkan-ekonomi bağı; NPC çekirdeği |
| **Bölge hikaye zinciri** | bölgenin kendi arc'ı (§3.4) + ana zincire kancası |
| **Örgüt varlığı** | hangi örgütler burada faal (§3.6) |
| **Ekoloji paketi** | yuva/sürü aileleri, boss(lar), av-kaynak döngüsü |
| **Lore sayfaları** | bölgede düşen işlevli sayfalar (§4.9) + LORE.md kaydı |
| **Ortak düşman izi** | bölgede en az BİR belirti/ima (§3.5 — erken tohum) |
| **Bölgesel kimlik** | itibar hattı, gündoğumu offset'i, müzik mood'u, hava eğilimi |

### 3.3 Irklar: HER BÖLGEYE ÖZGÜ model [ONAYLI]
İlke: ırklar Doğu/Batı ikilisine değil BÖLGEYE bağlıdır — her harita kendi
ırk(lar)ını, onların arkaplanını ve görev zincirini taşır. Fantastik ırklar
serbest (Hakan örnekleri: Batı'da ejderhalar, Doğu'da mumyalar).

Mevcut + taslak matris (satırlar bölge şablonu dolduruldukça büyür):

| Bölge | Yerli ırk / halk | Statü | Arkaplan çekirdeği |
|---|---|---|---|
| Köy/Yeşil Ova | Orta halkı (insan) | [KODDA] | Bağımsız çiftçi kültürü; "kimseye eğilmeden geçinmek" |
| Karanlık Orman | Goblin kabileleri | [KODDA-mob → ONAYLI ırka terfi] | Reis düzeni; baskın kültürü; ormanın eski sahipleri mi? [AÇIK] |
| Bataklık/Kayalık Geçit | Ork boyları | [KODDA-mob → ONAYLI ırka terfi] | Totem inancı; savaş kültürü |
| Lanetli Mezarlık | Undead (lanet — ırk değil) | [KODDA] | Mühür hattına bağlı (§3.5 adayı) |
| Taşdurak/Doğu çölü | Çöl halkı (insan-Sultanlık) + **Mumya hanedanı** | [KODDA + TASLAK] | Kadim hanedan tapınaklarda uyuyor; Sultanlık onların mirasında oturuyor — gerilim hazır |
| Batı toprakları | Şövalye halkı (insan) + **Ejderhalar** | [TASLAK] | Ejderha: ırk mı tehdit mi tercihe bağlı — öneri: KONUŞAN, az sayıda, kadim ırk (Batı zincirinin belkemiği) |
| Kuzey/kar biyomu | [AÇIK] | [AÇIK] | — |
| Akarsu vadileri, tekinsiz bölgeler | [AÇIK] | [AÇIK] | — |

Kural [ONAYLI]: ırk = arkaplan + görev zinciri + (uygunsa) itibar hattı
paketiyle gelir; "yalnız mob çeşidi" olarak ırk eklenmez.

### 3.4 HİKAYE MİMARİSİ: ZİNCİR MODELİ [ONAYLI]
Bütün anlatı ZİNCİRLERDEN kurulur; siyasi olaylar, gerilimler ve savaşlar
zincirlere yedirilir (serbest yüzen tek görev yok).

- **ANA ZİNCİR (omurga)**: chapter'lara bölünür (Perde 1: bağımsız bölge →
  Perde 2: Batı/blok siyaseti → Perde 3: ortak düşman açılımı [TASLAK sıra]).
  Chapter eşikleri dünya durumunu kalıcı değiştirir (savaş/barış, sınır
  açılışı, örgüt dengesi).
- **BÖLGE ZİNCİRLERİ**: her bölgenin kendi arc'ı (yerli ırkın meselesi) —
  bağımsız oynanabilir AMA en az bir düğümü ana zincire kanca atar ve en
  az bir düğümü ortak düşman izi taşır.
- **ÖRGÜT ZİNCİRLERİ**: rütbe merdivenli (Sessiz El modeli [KODDA] şablon:
  keşif → üyelik → kontrat/görev döngüsü → rütbe → dünyaya etki).
- **OLAY DOKUSU**: worldevent'ler zincir düğümlerinin YANKISIDIR (zincirde
  savaş kızıştıysa baskın olayları sıklaşır) ve bazen TETİKLEYİCİDİR
  (olay → söylenti → zincir düğümü açılır).
- **LORE SAYFALARI** zincirlere kapı açar (§4.9).
- Teknik çapa [TASLAK]: zincir = veri modeli (düğüm listesi + önkoşul +
  etki bayrakları); quest.c/journal.c bu modele evrilir — İskelet Fazı'nda
  (§9-A0) kurulur.

### 3.5 ORTAK DÜŞMAN — mekanik iskelet [ONAYLI; hikaye kimliği AÇIK]
Doğu-Batı çekişmesinin ÜSTÜNDE üçüncü bir tehdit. Hikayesi (kim/ne)
sonra; mekanik varlığı baştan kurulur:

1. **Erken imalar (Perde 1'den itibaren, ufak ufak)**: mevcut tohumlar
   ZATEN KODDA — mühür dalgaları, mühür sancısı olayı, yankı fısıltıları,
   rüya vinyetleri (mühür/çukur/fısıltı). Bunlar ortak düşman hattına
   BAĞLANIR (retrofit yok, anlam kazanır).
2. **Bölgesel izler**: her bölge zincirinde en az bir düğüm/lore sayfası
   bu tehdide dokunur (§3.2 şablon zorunluluğu) — oyuncu parçaları
   bölge bölge toplar.
3. **Chapter tırmanışı**: chapter eşiklerinde tehdit görünürleşir
   (yeni olay türleri, yeni mob aileleri, bölge durum değişimleri).
4. **Geç oyun mekaniği**: Doğu-Batı İŞBİRLİĞİ ekseni — iki blokla da
   ayrı ayrı kurulan itibar, ortak düşman fazında birleşik teraziye
   dönüşür (ikili ajanlık burada meyve verir). [TASLAK detay]
5. Aday kimlikler [AÇIK — sonra]: mühür altındaki şey / undead lanetinin
   kaynağı / kadim hanedan-ejderha bağlantısı... LORE.md'de tartışılacak.

### 3.6 ÖRGÜTLER: ÇOĞUL model [ONAYLI]
İstihbarat/suikast örgütü TEK değil — üç katman:

| Katman | Örnek | Statü |
|---|---|---|
| **İmparatorluk örgütleri** | Her imparatorluğun kendi istihbarat + suikast kolu (Doğu ve Batı'da birer çift) | [ONAYLI çerçeve, isimler AÇIK] |
| **Bağımsız örgütler** | Sessiz El [KODDA], Kızıl Kanca [ONAYLI-P4] | çekirdek hazır |
| **Bölgesel/yerel yapılar** | kasaba loncaları, kaçakçı ağları | [AÇIK] |

Kurallar [TASLAK — onaya]:
- Her örgüt = örgüt zinciri şablonu (§3.4) + rütbe + dünyaya etki + diğer
  örgütlerle İLİŞKİ matrisi (düşman/rakip/kayıtsız).
- Oyuncu birden çok örgütte yer alabilir; ÇATIŞAN üyelikler gerilim üretir
  (ifşa riski, çifte kontrat) — ikili ajanlık vizyonunun mekaniği.
- Kızıl Kanca P4 bu çerçevenin "bağımsız rakip" örneği olarak üretilir;
  imparatorluk örgütleri blok haritalarıyla gelir.

---

## 4. MEKANİKLER

### 4.1 Karakter gelişimi [KODDA]
Use-based 16 skill ağacı · puanlı düğüm ağaçları (aktif beceriler satın
alınır, keystone bedelli, respec sunak) · perk 3-seç-1 · ünvanlar ·
gizli kişilik sayaçları (kaba/kibar/ketum).

### 4.2 Gizli yollar [KODDA]
Yankıcı (ritüel keşfi; Can Emme, Yankı Çağırma; Fısıltı Yolu) ·
Suikastçi (Kör Kuyu zinciri; hançer+iğne, 3 doz zehir + SESSİZ SON imzası;
Gölge Yolu) · Kutsal (okul+tomar ekonomisi; Nur Yolu; undead sinerjisi) ·
Büyü: silahtan bağımsız cast, INT+Destruction+spellPower+rün.

### 4.3 Savaş [KODDA]
Combat A · KO auto-attack + hotbar · his katmanı (hit-stop/kan/sarsıntı) ·
telegraph'lı boss tasarımı (Ölüm Getiren şablonu) · mühür dalgası ·
hava-savaş sinerjileri.

### 4.4 Suç & toplum [KODDA]
Tanıklı suç → bounty + bölge itibarı + muhafız döngüsü · revir sistemi
(sivil 1 canda bayılır; 200g×1.25^gün; 30. gün köy toplar ama kalıcı
kırgınlık) · yankesicilik · kilit/tuzak/sneak.

### 4.5 Ekonomi & ticaret [KODDA]
Bölgesel dükkanlar + kervan + loca dükkanı (rütbe kapılı) · bölgeler arası
kâr marjları (köy↔Taşdurak) · itibar + mevsim fiyatları · kirli para +
aklama · craft istasyonları + blueprint.

### 4.6 Yaşayan dünya [KODDA]
Ekoloji (yuva/göç/rutin/dinamik kontrat) · dünya olayları (şafak zarı) ·
hava (5 tür + kar örtüsü, mevsimsel havuz) · mevsimler (10 gün; gündüz
uzunluğu/ton/ekonomi/ekoloji) · bölgesel gündoğumu · NPC rutinleri.

### 4.7 Görev & anlatım araçları [KODDA]
ESO diyalog perdesi · questBanner + takipçi + işaretçi · J kitabı (4 sekme)
· panolar (ilan/söylenti/kontrat) · sahne araçları (bark/cine/vinyet/rüya).
[TASLAK] J kitabına 5. sekme: **Lore** (§4.9 sayfaları burada arşivlenir).

### 4.8 Kontrat sistemleri [KODDA + ONAYLI]
Sessiz El K1 [KODDA] · P4 [ONAYLI]: süreli kontrat yarışı (KAPILDI),
isimli K2 hedefleri, tırmanan pusular · hedef merdiveni K1→K4 (kral en
sonda; "yuvada avlanmaz") · köy panosu av kontratları [KODDA].

### 4.9 LORE SAYFALARI — işlevli keşif [ONAYLI konsept, tasarım TASLAK]
Kural [ONAYLI]: **boşa sayfa düşmez** — her lore sayfası mekanik iş görür:
- **Zincir anahtarı**: bir görev zinciri BAŞLATIR ya da kilitli düğümü açar;
- **Aydınlatıcı**: hikaye açığını kapatır → J/Lore'da bir "?" çözülür
  (tamamlanan setler küçük kalıcı bonus/ünvan verebilir [TASLAK]);
- **İşaretçi**: haritaya yer/POI işler (kazı yeri, gizli geçit);
- **Diyalog kapısı**: belirli NPC'de yeni seçenek açar ("Bu sayfayı
  nereden buldun?").
Dağıtım: boss/sandık/kazı/POI keşfi/örgüt ödülü; bölge şablonunun zorunlu
alanı (§3.2). Teknik çapa [TASLAK]: sayfa = item + loreId; okununca
J/Lore sekmesine işlenir, etkisi zincir bayrağına yazılır.

---

## 5. İÇERİK ENVANTERİ (2026-07-24)
Haritalar 7 (hepsi mapgen'li) · mob 26/32 · NPC 19/24 · item 65+ ·
beceri: 5 yay + 5 büyü + 13 sınıf becerisi + meslek pasifleri.
(Bölge şablonu devreye girince envanter bölge başına raporlanır.)

## 6. SANAT & SES [KODDA]
Cute Fantasy ailesi (16px, 4x) + ULPC oyuncu · dual-grid autotile standardı
· mapgen görsel iterasyon protokolü · xDeviruchi müzik (mood sistemi) ·
15 SFX + ambiyans. Borçlar: Rüstem sprite, haydut offset, Kayalık klifleri.

### 6.1 Görsel tarz / post-fx grade [AÇIK — kalıcı tarz seçilmedi]
Tam-ekran post-process filtre altyapısı kodda (bkz. §8, F7 döngüsü); 20 hazır
ön ayar tarz denemesi için (SICAK varsayılan · DARK FANTASY · CRT · VHS · NOIR
· GAMEBOY · SYNTHWAVE · KORKU · kombinasyonlar). Oyunun KALICI görsel imzası
Hakan testinden sonra seçilecek — muhtemelen SICAK/DARK FANTASY ekseni. Filtre
şu an HUD dahil tüm kareye uygulanıyor (sahne-only varyantı açık soru).

### 6.2 Işık & gölge modeli [KODDA — varyantlar test bekliyor]
Işık bu oyunda dekor değil **mekanik**: LORE §1.5 "eşik = ışığın bittiği yer".
O yüzden görsel eğri ile oyun kuralı **tek formülden** gelmek zorunda.

**Karar 1 — TEK DÜŞÜŞ EĞRİSİ (bağlayıcı).** `light_atten()` (CPU) ile
`fs_light` shader'ı (GPU) aynı eğriyi uygular; ayrışırsa oyuncu *aydınlık
gördüğü yerde* ölüyle karşılaşır. Bu bir kez oldu (görsel 210px / bastırma
141px / fısıltı 158px). Eğriyi değiştiren, iki yeri birlikte değiştirir.
İç %35 plato + smoothstep düşüş + **kesin kenar** (`LIGHT_EDGE_CUT`) —
kuyruk asimptotik gitse "ışık var mı yok mu" kenarda belirsiz kalırdı.

**Karar 2 — GÖLGE = Blinn düzlemsel projeksiyonu (bağlayıcı).** Tahmin değil:
Blinn 1988, yönlü ışıkta matris saf **shear**'e iner, `L = h·cot α`. Ayak
noktası sprite dikdörtgeninin altı DEĞİL, **alpha sınır kutusunun** altıdır
(ağaç sprite'ında altta şeffaf boşluk var → gölge tabandan kopuyordu).

**Karar 3 — "ZELDA YALANI" (bağlayıcı).** Top-down'da dünya-kuzeyi ile
dünya-yukarısı ekranda ayırt edilemez, yani kuzeye düşen gölge sprite'ın
arkasına saklanır. Gölgenin dikey bileşeni pozitif tabanda tutulur; gölge
**asla ekranda yukarı gitmez**. Top-down oyunların bunu yapmasının sebebi bu.

**Karar 4 — TİTREŞİM AŞAĞI YÖNLÜDÜR.** Çarpımsal ışıkta yukarı titreşim
1.0'da kırpılır ve sinyal kaybolur (Doom 1993 deseni). Genlik **yarıçapta**
parlaklıktan ~5.9× daha etkili; dalga asimetrik (yavaş kabarma, hızlı çöküş).
Karakter tipe bağlı (`FlickerKind`): kor sakin — LORE §1.9 "seni koruyan şey
durur, korumayan gezinir" hem fizik (akkor katının termal ataleti) hem
mekanik (kor bastırır, odun bastırmaz) ile aynı yere çıkıyor.

**Karar 5 — ŞEKİL IŞIK TİPİNE BAĞLIDIR.** Kamera 3/4 eğik, bu yüzden zemin
dikey olarak kısalıyor ve kaynaklar zeminin ÜSTÜNDE duruyor. Tek global şekil
her kaynağı aynı gösteriyordu; artık `aniso`/`yOff` tipin kendi alanında:
kandil basık ve yukarı kaymış (hazne direğin tepesinde) · kamp ateşi çok basık
ve yerde · meşale orta (elde) · kutsal büyü daire (fiziği yok).

**Karar 6 — GAMMA.** Katkılar **lineer** uzayda toplanır; sRGB'de toplamak
merkezi +0.218 fazla parlatıp kırpıyordu.

**[AÇIK] Kor'un rengi — kanon/fizik çatışması.** LORE korun beyaza yakın
yandığını söylüyor; siyah cisim eğrisinde o sıcaklık koyu kızıl. Şu an elle
seçilmiş renk ≈3130K (tungsten masa lambası). F11 iki modu değiştiriyor;
**kalıcı seçim Hakan testinden sonra.**

**Borç:** tilemap tıkaması (su karosu `solid` olduğu için `blocksLight` ayrı
bayrak şart — yoksa göl opaklaşır) · çift-karartma için ayrı hedefe çizip tek
kompozit (Factorio'nun belgelenmiş çözümü) · düşük çözünürlüklü lightmap
(1 texel = 1 sanat pikseli) · kandil durumu save'e girmiyor.

## 7. UI / UX [KODDA]
WoW hotbar · HUD (mevsimli saat, kontrat takipçisi) · panel ailesi
(tooltip/drag&drop) · minimap · menü + 20+1 slot + yaratma-lite.

## 8. TEKNİK [KODDA]
C + SDL3 (vendored) + r2d GPU renderer (SDL_GPU/Metal; MSL shader — Vulkan/D3D
SPIR-V yayın fazına ertelendi) · post-fx pass (offscreen target + fs_postfx:
grade/vinyet/scanline/posterize/kromatik sapma/gren; F7 preset döngüsü) ·
ışık pass'i (32 nokta ışık; tip başına titreşim+şekil; ışın-kutu tıkama, 16
gövde) + Blinn gölgeleri (§6.2) ·
save: sabit v29 + toleranslı kuyruk · dev araçları (ASAN, PIXE_AUTOLOAD/AUTOPLAY,
F8/F10) ·
disiplinler: paralel dizi tavanları (5 vaka), sıfır yeni uyarı, CSV=GID+1,
her committe doküman güncellemesi.

---

## 9. YOL HARİTASI [TASLAK v2 — onaya]

### A0 — İSKELET FAZI (yeni — temel önce) [Hakan direktifi 2026-07-24]
1. **LORE.md kurulumu**: dünya kutsal kitabı — bölge/ırk/örgüt/zaman
   çizelgesi iskeleti + ortak düşman aday dosyası (hikaye kararı için zemin).
2. **Zincir veri modeli**: quest sistemini zincir mimarisine (§3.4) taşıyan
   altyapı (düğüm/önkoşul/etki bayrakları; mevcut görevler ilk zincirlere
   sarılır — davranış değişmez, iskelet oturur).
3. **Lore sayfası sistemi** (§4.9): item + J/Lore sekmesi + zincir tetiği;
   ilk sayfalar mühür hattına bağlanır (ortak düşman erken iması).
4. **Örgüt çerçevesi**: örgüt tanım tablosu (ilişki matrisi + rütbe
   şablonu); Sessiz El mevcut haliyle çerçeveye oturtulur.

### A — SÜREN İŞLER
5. Suikastçi P4 [ONAYLI]: Kızıl Kanca — çerçevedeki ilk "rakip örgüt".
6. Test birikimi kapanışı + görsel borçlar.

### B — DOĞU DERİNLEŞMESİ (bölge şablonunun ilk tam uygulaması)
7. Doğu iç bölgesi: yeni harita + **Mumya hanedanı ırk paketi** (arkaplan +
   bölge zinciri + lore sayfaları + ortak düşman izi) — şablonun kanıtı.
8. Sultanlık siyaseti: beylik entrika olayları + K3 kontratları +
   Doğu imparatorluk örgütü (ilk imparatorluk örgütü).
9. İstihbarat ekseni mekaniği [AÇIK tasarım — bilgi toplama/satma].

### C — PERDE 2: BATI
10. Batı sınır bölgesi + şövalye halkı + **Ejderha ırk paketi** [TASLAK].
11. Batı imparatorluk örgütü + bloklar arası gerilim olayları + K4 hattı.
12. Ana zincir Perde 2 + ortak düşman tırmanışı (chapter eşiği).

### Sürekli şerit
Doküman disiplini (GDD→PROGRESS→HANDOVER→LORE) · cila · teknik borç ·
ekonomi dengesi tablosu (§10).

## 10. DOKÜMAN AİLESİ
- **GDD.md** (bu belge): iskelet + kararlar. [KODDA]
- **LORE.md**: dünya kutsal kitabı — ırk arkaplanları, örgütler, zaman
  çizelgesi, ortak düşman dosyası. [ONAYLI — A0'da kurulacak]
- **Ekonomi dengesi tablosu** [TASLAK — şiddetle önerilir].
- **Görev/zincir yazım kılavuzu** [TASLAK — A0 zincir modeliyle birlikte].
- **Test planı** · **Ses envanteri** · **Yayın planı** [TASLAK/AÇIK].
