# PIXE GAME — Oyun Tasarım Dokümanı (GDD)

> **Belge kılavuzu:** Her madde bir statü taşır:
> **[KODDA]** oyunda çalışıyor · **[ONAYLI]** Hakan onayladı, üretim sırada ·
> **[TASLAK]** asistan önerisi — Hakan onayı bekler · **[AÇIK]** karar verilmedi.
> Bu belge canlıdır: her tasarım kararı önce buraya işlenir, kod onu izler.
> Bakış TEPEDEN: burada iskelet kurulur; karakter/görev yazımı iskelete sonra asılır.
> Üretim geçmişi PROGRESS.md'de, teknik devir HANDOVER.md'de, derin kurgu **LORE.md**'de.
> ## ⚠️ KANON REBOOT — 2026-07-30 [Hakan direktifi]
> **"Sönen Kandiller" kanonu (LORE v1) HÜKÜMSÜZ.** Dört turluk bir elemede 24 aday üretildi;
> kazanan **DEMİRBOYUN** ve LORE.md v2 onu tutuyor. Eski kanon
> `docs/konsept/arsiv-sonen-kandiller-lore-v1.md`'de rafta (silinmedi).
>
> **Reddedilme gerekçeleri sırayla:** tur 1-2 fantastik mekanizmayı omurga yaptı ("Skyrim'de böyle
> mi?"); tur 3 insan omurgasına geçti ama köy ölçeğinde kaldı — dört kusur birden: ölçek çok küçük ·
> fazla kuru/bürokratik · fantastik fazla azaldı · büyüme hedefi yok. Referans **Skyrim**.
>
> **DEMİRBOYUN modeli — bağlayıcı yapı:** omurga İNSAN siyasi çatışması (felçli fatihin sandalyesi,
> onun adına konuşan altı ağız, hiç yapılmamış taç töreni) · **paralel** ve görkemli fantastik katman
> (Akkemer mühürleri çürüyor, ejderhalar yerden çıkıyor; Sarkaç geri alınamaz) · 6 şehir · 6
> katılınabilir örgüt (rütbe merdivenli, 22 hat + 2 gizli sınıfa yuva) · 4 kademeli mülk/komuta
> merdiveni. Fantastik ana zincirin hiçbir düğümünde ZORUNLU değil.
>
> Süreç ve elenen 23 aday: `docs/konsept/reboot/`. Sudowrite prompt paketi aynı klasörde.

> **LORE.md v2 kanon kilidi (2026-07-30)** — dünya/halk/örgüt/ölü/zanaat/
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
> ### ~~YENİ KANON GELDİ — "SÖNEN KANDİLLER" (2026-07-28)~~ [HÜKÜMSÜZ — bkz. reboot 2026-07-30]
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

### 1.2c ZİNCİR KALİTE KURALLARI [ONAYLI — Hakan 2026-07-31]
Hakan'ın direktifi: *"Oyuncu **görev yapıyorum** değil, **hikayenin parçasıyım**
diye hissetmeli. 'Git 10 orc kes, 5 çiçek topla' değil — bunlar belki yolda
giderken yaptığı, belki hikaye parçası olarak yaptığı işler olmalı."*

**MİMARİ DEĞİŞMEDİ.** ESO/WoW tarzı görev zinciri + ESO sunumu (§1.2) yerinde.
Görev sistemini "dosya" ya da "talep" modeliyle DEĞİŞTİRME önerisi reddedildi —
aşağıdakiler zincirin İÇİNE giren kalite kuralları, mimari değil.

**Teşhis:** yapı `kabul → sayaç → teslim`. Sorun ortada değil, **iki uçta**:
"kabul" oyuncuyu memur yapıyor, "teslim" işi kapatıp dünyayı değiştirmiyor.
Sayaç suçlu değil; sayacın etrafındaki tören suçlu.

**1. [ONAYLI] TANIK KURALI — bağlayıcı ve test edilebilir.**
> Oyuncu bir zinciri ilerlettiğinde, o ilerlemeyi **ondan istememiş en az bir
> NPC** bunu fark etmiş olmalı ve bir sonraki karşılaşmada **ilk cümlesi**
> değişmeli. Fark eden yoksa o iş görevdir, hikaye değildir.

Değişen cümle **teşekkür OLMAMALI** (ucuz versiyonu "teşekkür bandı"). Kasap
fiyatı düşürür, seyis bakışını kaçırır, gardiyan yol açar, Baran "evlat" demeyi
bırakıp "müşteri" der.

**2. [ONAYLI] SAYAÇ: hedefse görünür, yöntemse gizli.**
- Envanterde ZATEN sayılan şey (3 ot, 8 deri) görünür — saklamak sahtekârlık.
- **"Kaç tane öldür" asla hedef değildir** ve görünmez. Hedef yuva, totem, reis,
  sandık, adam ya da karardır. Sayı direncin ölçüsü.
- Zincir görevinde sayaç görünür çünkü **verilen sözün kaydı** — süre işliyorsa
  ya da bir kademe (izin/berat/rütbe) hedefleniyorsa da görünür.
- **ZORUNLU:** HUD sayacı olmayan her iş, ilerlemeyi DÜNYADA okunabilir bırakmak
  zorunda — direkteki tebeşir çentiği, yamaçta bekleyen araba sayısı, dolan raf.

**3. [ONAYLI] KABUL ADIMI: zincirde var, panoda yok.**
Pano bir ilan panosu, NPC bir insan — iki farklı şey, iki farklı kural.
- **Zincir (NPC):** kabul VAR. NPC anlatır, oyuncu kabul eder, bu bir sözdür.
- **Pano (kontrat):** kabul YOK. Her öldürme sayılır; panoya hiç bakmadıysan da
  iş görülmüş olur, sonradan bakınca "bu iş görülmüş" der ve parası tezgâhta
  bekler. `contract_on_kill`'deki "kabul edilmiş mi" kontrolü kalkacak.

**4. [ONAYLI] PHASING — her zincir bir iz bırakır.**
> Bir zincir bittiğinde harita eskisiyle aynı görünüyorsa o zincir bitmemiş,
> **kapatılmıştır.**

Çıkışta bir `storyFlag → decorPatch` (bayrak açıkken şu karo bloğu şu prop
setiyle çizilir) VE mümkünse kalıcı bir fiyat / olay-sıklığı / yol değişimi.
**Bu bizim yapısal avantajımız:** ESO dünyayı değiştiremiyor (paylaşımlı),
WoW phasing'i o kısıtın etrafından dolanan bir hile; oyun OFFLINE ve tek
oyunculu olduğu için bizde kalıcılık bedava. Altyapı hazır: storyFlags, dekor
sistemi, bölgesel fiyat çarpanı, dünya olayları. Zaten çalışan örnek: Nazife'nin
sofrası K2'den sonra sonsuza dek kurulu kalıyor.

**5. [ONAYLI] DUYUM KANALI — ünlem işareti en zayıf kanal.**
İşi panodan ya da ünlem işaretinden ÖNCE dünyada duyarsın. `AmbientHook
{ npcId, hourFrom, hourTo, dialogNode }` — NPC yakınından geçerken saat
aralığına bağlı kendiliğinden tek satır. Örnekler: 06-08 iskelede kalfa
*"Dün yüz elli çivi saydım, bu sabah yüz on iki"*; öğleden sonra revirde gazi
*"Baran'a söyle bu ay gelmeyeceğim, maaş çıkmadı"*; geçit ağzında üç yüklü araba
(replik yok — manzara soruyor). Pano ilan değil **doğrulama** olur: oyuncu işi
insanın rutininde ya da manzarada görür, panoda adını okur.

**6. [ONAYLI] Her işin ikinci bir DOĞRU tarafı var ve o taraf KONUŞABİLİR.**
Sivil değil ama düşman da olmayan biri: alıcı, bölük başı, çocuk. Kaba/açgözlü
oyuncuya kapı KAPANMAZ — hitap değişir, fiyat değişir, kışlada kimse yanına
oturmaz. Kaybedilen içerik değil, **sıcaklık**.
Bunun için **YENİ MEKANİK** onaylandı: `mob.isTalkable` + `dialogFile` +
"hostile → neutral" geçişi. İkinci taraf konuşamazsa öldürmekten başka yol
kalmıyor; Speech/itibar/altın çözümleri bu kapıdan geçiyor.

**7. [ONAYLI] Kes/getir SİLİNMEZ, hedef olmaktan çıkar.**
Beş goblin hâlâ ölür — ama sayısı değil **reis** umurundadır; yolda gelenler
ölür çünkü aralarından geçmen gerekir. Üç ot hâlâ toplanır — ama toplama yeri
mezarlık olduğu için toplamanın kendisi sahnedir ve otun **ikinci bir alıcısı**
olduğu için üç ot bir bilet değil bir karardır.

**BU OYUNA UYMAZ [ONAYLI]:** tam sesli diyalog (metin tavanı zaten taşıyordu;
ses girerse yazar cümleyi kısaltır ve doku ölür) · dallanan sinematikler (on
karar otuz sahne eder; karar aracımız kalıcı NPC tavrı, o bedava) · anlatıcı
sesi / açılış prologu (LORE §0 dünyayı "tören" diye açmayı yasaklıyor, anlatıcı
tam onu yapmak zorunda) · **ekranda ahlaki seçim kutusu** (kişilik sayaçları
GİZLİ oldukları için çalışıyor; etiket koyduğun an oyuncu sayacı optimize eder)
· "SÜRE DOLDU" kutusu (dünyanın değişmiş hâli haberdir) · harita ikonu çorbası
(işaretçi ancak oyuncu bir yer adını DUYDUYSA düşer).

### 1.2d İLK YİRMİ DAKİKA [ONAYLI — Hakan 2026-07-31]
Hakan'ın gerekçesi: *"İlk dakikalar aslında tutorial gibi adaptasyon. **Panodan iş
almak angarya hissettirir.** İlk 15-20 dk çok önemli — kullanıcı devam veya bırakma
kararını alacak, sevmezse oyunu bırakıp iade edebilir."*

- **[ONAYLI] Panodan iş alma ilk 20 dakikada YOK.** Pano dünyada durur ama açılışın
  omurgası olamaz. **19. dakikada** ilk kez görünür ve üstünde oyuncunun ZATEN yaptığı
  işin ilanı asılıdır (*"Mahzene buz taşı — 6 akçe"*). Oyuncu ilanı okur ve bir işi
  **almadan** yaptığını anlar — pano böylece görev kaynağı değil **doğrulama** olarak
  tanışılır (§1.2c kural 5). Bu tanışma sırası sistemin kalan ömrünü belirliyor.
- **[ONAYLI] 90 SANİYE KURALI:** ekranda hiçbir an **cevapsız bir görüntü olmadan**
  90 saniye geçmez. Can sıkıntısı sessizlikten değil, **soru üretmeyen sessizlikten**
  doğar. Çizelge: 90 sn havada kıpırdamayan taş taht · 3. dk ilk insan sesi (sana
  DEĞİL) · 5. dk seni gören ilk göz · 7. dk oyunun konusu (oda) · 9. dk ilk talep.
- **[ONAYLI] Tutorial metin kutusu YASAK.** "WASD ile yürü" yok. Öğretme dünyanın
  içinden gelir: tek çıkış yol olduğu için yürüme, nöbetçi cümlesini bitirip
  sustuğu için E, kız ayakta yaklaşınca kaçtığı için **çömelme** öğrenilir.
- **[ONAYLI] İlk seçim 5. dakikada** (iskele ayağından çivi söken çocuk) ve
  **bedeli yok, hafızası var.** Hiçbir kapı kapanmaz; sadece o çocuk oyuncu
  hakkında bir şey bilir. Oyuncu kaybetmeyi öğrenmeden **hatırlanmayı** öğrenir.
- **[ONAYLI] İlk ölüm riski 15-18. dakika ve kasten yumuşak:** tek düşman, geniş
  koridor, açık kaçış, refakatçi kandili tutuyor. **Korku sesten gelir, mobtan
  değil** — duvarın ardındaki soluk. Zorluk coğrafyaya sabit (level scaling yok),
  ama Yenitaç mahzeni coğrafyanın en yumuşak noktası.
- **[ONAYLI] Mahzene DÖRT giriş** (rol özgürlüğü): kızak kayışına yardım eden ·
  akşam kışla kapısından geçerken nöbetçinin barkını duyan · gece gizlenerek giren
  hırsız (refakatçiyi hiç görmez, hikayeyi yalnız çizmeden alır — kısıtlı değil,
  **daha soğuk** bir versiyon) · panodan öğrenen (20. dakikadan sonra, en zayıf).
- **[ONAYLI] 20. dakikada oyuncunun elinde:** bir soru (neden gömülmüyor), bir
  kelime (çizmenin içine kazınmış *"Ben rehin değilim"*), bir ses (duvarın ardı),
  bir tanıdık, ve **kimsenin vermediği iki hedef**. Hiçbir zincir başlatılmadı,
  hiçbir günlük satırı bir kişinin ricası değil.
- **[ONAYLI] Kasten ÖĞRETİLMEYENLER:** craft istasyonları, kontrat kademeleri,
  beceri ağacı, itibar sayısı, bounty, mevsim, örgütler, yankesicilik. Bunlar
  oyuncunun SEÇTİĞİ rolün araçları; ilk çeyrek saatte açılırsa oyun bir menü oyunu
  gibi başlar ve "kimlik davranıştan doğar" sütunu ilk yirmi dakikada yalanlanır.
  **Oyuncu kim olacağına karar vermeden ona alet kutusu göstermiyoruz.**
- **[ONAYLI 2026-08-03] TAHT SALONU AYRI HARİTA — çatısı yok ama duvarı ve kapısı
  var.** Vinyet PNG panelleri yerine cine moduyla **canlı mekânda** geçecek; salon
  zaten zorunlu, §1.4 "salona her girişte kaç sancak asılı olduğunu sayarsın" diyor.
  **Bir önceki karar (salonu dünya haritasında yarım avlu yapmak) İPTAL** ve iptal
  gerekçesi bu projede kalıcı bir ders: o karar `NpcDef.map`'in tek int olmasından,
  yani bir **kod faturasından** türetilmişti. Kod faturası kanon gerekçesi olamaz
  (§10 doküman akışı: kod en son gelir). Üstelik kendi belgesiyle çelişiyordu —
  Sarp'ın yazılı tek arzusu "öğlen avluya çıkarılmak"; salon zaten avluysa o istek
  buhar oluyor, onunla birlikte onu taşıyan adamın bütün işi. Ve LORE §7'nin fiziği
  **kapalı bir odaya** dayanıyor: kırk adım ötede ip çeken işçinin duyabildiği yerde
  "söylenemeyen cümle" diye bir şey olamaz. Salon iki alan: kapalı taht bölümü +
  duvarlı **avlu** (öğlen oradan kuzey yolu görünüyor), aralarında bir kapı — ve
  kapının hangi tarafında durduğun kimin ne duyduğunu belirliyor. `MAP_MAHZEN` de
  ayrı; ikisi de enum'un SONUNA eklenir (`save.c` harita indeksini `MAP_COUNT`'a
  karşı sınırlıyor, sona ekleme format-güvenli).
  **Mekanik borç (fatura, karar değil):** `NpcStop`'a bir `map` alanı + `npc.c`'de
  beş noktanın ona bağlanması. Durak haritası değişince NPC yürümez, **ışınlanır** —
  kapıdan geçişini kimse izlemiyor. Bu tek alan dört slotu birden açıyor (mareşalin
  salon→kışla, hekimin salon→revir, varisin iskele→salon, bakıcının salon→mahzen
  hattı) ve "ikiz NPC kaydı" yamasını gereksiz kılıyor.

### 1.2h SU HACMİ ve KIŞYOLU [ONAYLI — Hakan 2026-08-03]

Motorda su artık bir doku değil bir **hacim**: zemin katmanı ayrı bir target'a
alınıyor, su shader'ı onu kendi dalga normaline göre kaydırarak örnekliyor
(`shaders/water.frag.hlsl`, `r2d_water_flush`). Tek geçişte kırılma, derinlik
gradyanı, güneş açısına göre parlama, kostik ve kıyıya vurup çekilen köpük.

**Gerekçe (kaydı önemli):** shader'sız katman yığmak denendi ve cevabı verdi —
suyun altındaki şeyi ÖRTEBİLİYORSUN ama BÜKEMİYORSUN. Sualtı hissi tanıdık bir
nesnenin büküldüğünü görmekten doğar; örten katman sualtı değil mavi levhadır.
Bu yüzden sığda örtme oranı düşük tutuluyor: kayayı göremezsen büküldüğünü de
göremezsin.

- **Kalıcı yeri KIŞYOLU** — Karduvar'ın donmuş gölü. Orada su manzara değil,
  üstünde yürüdüğün zemin; Karduvar halkı buzu her sabah zaten sınıyor.
  Tuzkapı limanı yüzey vitrini olarak kalır (VFX brief C.1) — dibi gösterme
  yeteneği Kışyolu'nda kanon taşır. Adı Karduvarlı vermedi: o "göl" der.
- **[ONAYLI] Suya GİRİLMEZ, üstünden bakılır.** Tuğrul'un ölümü serbestçe
  girilebilen bir gölde ağırlığını kaybeder; "delilin üstünde yürümek" sahnesi
  ancak girilmezse çalışır; tepeden bakan kamerada mercek YÜZEYDİR, içine
  girildiğinde mercek gider. Suya girme kanonda Tuzkapı körfezi/lağımına ait.
- **[ONAYLI] Buzun okunabilirliği bir OYUN KURALI:** duru buz = sağlam + dip
  görünür · ak/köpüklü buz = çürük + dip görünmez. Tutorial metni YOK, kural
  yürüyerek öğrenilir. Görsel sistem kuralı kendiliğinden anlatıyor.
- Sığlıkta batık dizi: ağırlık taşları + urgan + üç ayrı yıl damgalı kurşun
  mühürler. İçindeki tuz çözüldü — **ganimet yok.** Bağlantı LORE §8: Gedik
  Beyi'nin sessizliği bir cinayeti değil bir FİYATI koruyor (göl dört ay
  Karduvar'ın kısa yolu; yol kapanırsa kışlık tuz pahalanır).
- Derin uçta LORE §9 mühür 6 görünür olur (kemer + kımıldamayan şey + Ak
  Nöbet'in yem deliği). Kanonda zaten vardı; su hacmi onu görülebilir kılıyor.
- **[AÇIK]** Kımıldamayan şeyin dokuz yılı ile Tuğrul'un dokuz yılı arasında
  nedensel bağ var mı — kurulmadı, kanon bağlamıyor.
- **Bilinen gerilim (mekanik, kanonik değil):** §9'un altıncı çürüme olayı buzu
  kalıcı kırıyor, yani "buzun üstünde yürüyerek dibe bakmak" içeriğinin son
  tarihi var. Kanona dokunmadan çözülür: aynı yer, kırılmadan sonra açık su
  olarak ikinci hayatını yaşar (kanıtlar tekneden alınır — §9 korkuluğu).

### 1.2g OYUN SAATİ — gün 60 dakika [ONAYLI — Hakan 2026-08-03]
Kanon denetiminin ikinci turu, kadro yazıldıktan sonra, çizelgenin **motor saatinde
imkânsız** olduğunu buldu. `GAMETIME_DAY_SECONDS 1200` (20 dk/gün) ile bir gerçek
dakika = 1,2 oyun saati; yani "ilk yirmi dakika" tam bir oyun günüydü ve
9. dakikanın ilk talebi **18:48**'e, mahzen sahnesi (15-18. dk) gece **03:00**'e,
19. dakikanın panosu **06:48**'e düşüyordu — kâtip o saatte `indoors`.

- **[ONAYLI] `GAMETIME_DAY_SECONDS` 1200 → 3600 (60 dakika/gün).** Çizelge
  **08:36-15:36** arasına oturuyor; dokuz vuruşun dokuzu gündüz, hepsi rutinlerle
  örtüşüyor. Değişiklik güvenli: sabiti kullanan üç yer de oranı ondan türetiyor
  (`gametime.c:30`, `weather.c:88` → `24/DAY_SECONDS`; `main.c`'nin saat atlama
  tuşu → `DAY_SECONDS/24*2`). Mevsimler GÜN sayısıyla çalışıyor (`SEASON_DAYS 10`),
  oran değişmiyor — yalnız bir yılın gerçek süresi 13 saatten 40 saate çıkıyor.
  Save'e girmiyor.
- **ASIL GEREKÇE ÇİZELGE DEĞİL, OKUNABİLİRLİK.** Yirmi dakikalık günde **oyuncu bir
  rutin öğrenemiyor**: bir saatte üç gün görüyor. Kadro tasarımının tamamı bunun
  tersine dayanıyor — 23 NPC'nin dört durağı, yedi AmbientHook penceresi (*"kalfa
  06-08 arası çivi sayar"*, *"aşçı 17'de kazan kurar"*, *"kesedar 15'te iskele
  dibinde direk sayar"*) ve gece kandil hattı, oyuncunun **bir saati bilip oraya
  gitmesini** istiyor. Öğrenilemeyen pencere bilgi değil engeldir.
- **[ONAYLI 2026-08-03] `NpcDef.carried` — bir bool.** İşaretli NPC durak saati gelince
  hedefe **yürümez, konumuna geçer.** Sarp için zorunlu: `npc_update` durak değişince NPC'yi
  yürütüyor ve avlu ile taht bölümü **aynı haritada**, yani `NpcStop.map` onu kurtarmıyor —
  felçli bir adamın kendi ayağıyla avluya çıktığını oyuncu her öğlen izlerdi. Taşınmayı oyuncu
  görmüyor ve bu bir eksik değil: sedyeyi kimse seyretmiyor, sandalye bir yerdeydi, sonra
  başka yerdedir. Alternatifi (Sarp'a tek durak + avluyu sahneye almak) reddedildi: öğlen
  avlusu kartta yazılı **tek arzusu**, bakıcının en pahalı sırrının bedeli, ve Yedinci Ağız'ın
  kapısı oyuncunun o sahneyi **her gün tekrar tekrar** izleyebilmesini istiyor.
- **[BAĞLAYICI] Yeni rutin veya AmbientHook penceresi yazan, saatin bu ölçekte
  olduğunu varsayacak.** Gün uzunluğu bir daha değişirse §1.2d'nin dakika
  çizelgesi ve 23 slotun durak saatleri **aynı commit'te** yeniden hesaplanır.

### 1.2f PARA ÖLÇEĞİ — üç birim, tek keseli oyuncu [ONAYLI — Hakan 2026-08-03]
Kanon denetimi ölçeğin tanımsız olduğunu buldu: aynı dünyada bir kase çorba **2**,
bir tefecinin BÜTÜN defteri **900**, ama mareşal **12.000** adamı kendi payından
besliyor. Rakamlar birbirini yalanlıyordu ve kadronun beş ağzı sayıyla konuşuyor.

- **`akçe` — gündelik ölçek.** Elle tutulan para. Kase çorba 2 · meşale 5 · bir ot 6 ·
  işçi yevmiyesi 6-8 · mahzene buz taşımak 6 (yani tam bir günlük yevmiye — ilanın
  altı akçesi bu yüzden hem az hem çok). **Oyuncunun kesesindeki her şey akçe:**
  kodda tek para alanı var (`player.gold`) ve **ikinci para birimi EKLENMEYECEK.**
  Kese ile sikke envanterde görünmez; onlar **konuşma** ölçeğidir.
- **`kese` — bin akçe.** Tuzkapı'nın kesecileri adını buradan alıyor; "Kırk Kese" bir
  meclisin adı olmadan önce bir hacimdi. Şehir bütçesi, iskele kalemi, saray kasası
  kese ile konuşulur.
- **`sikke` — devlet ölçeği, yüz kese.** Elde taşınmaz, masada durur; gümrük, bac,
  kira ve ganimet sikke ile yazılır. Bedava yan etki: Karaocak'ın **kendi sikkesini
  basması** (LORE §4) küçük para basmak değil, **devlet ölçeğinin kendisini** basmak
  demek. Mareşalin sormaması bu yüzden bu kadar ağır. Aynı şekilde kesedarın
  terazisindeki şey ise bu yüzden sikke DEĞİL, **güney damgalı bir gümüş külçe** (avuç içi
  kadar, ağırlığı bir kese eder). Bu madde ilk yazıldığında örnek olarak "güney sikkesi"
  demişti ve kendi tanımıyla çelişiyordu: yüz kese eden bir madeni para bir tefecinin
  terazisinde tartılmaz, şehrin iki buçuk yılı eder. Külçe aynı işi yapıyor — bir tefecinin masasında devlet ölçeğinde para
  durması, orada olmaması gereken bir şeyi kimse açıklamadan söylüyor.
- **BEDAVA KARAKTERİZASYON — aynı odada iki kadın, iki birim.** Sayman devlet rakamını
  asla akçeye bölmez (*"Kırk gün, kırk kese."*); kesedar aynı rakamı *"kırk bin akçe"*
  der ve kese'ye yuvarlamayı reddeder, çünkü onun defterinde yuvarlanan her akçe
  birinin bir günü. İkisi aynı sayıyı iki dille söylüyor ve oyuncu hangisinin nereden
  geldiğini kimse anlatmadan duyuyor. Ve bir tavan: masaya **kese diliyle** konuşmayan
  yazmanı Kese Sahibi yapmıyorlar — kadının kariyerini tıkayan şey bir hata değil, bir
  birim tercihi, ve tercihinden vazgeçmiyor.
- **LORE'un "kırk gümüş"ü bir birim değil, bir usul.** Karduvar diyet kalemini kırk
  parça gümüş olarak gönderir; sayılır, tartılmaz. Bir çocuğun bedeli akçeye
  çevrilmiyor ve çevrilmediği için acıtıyor.
- **Rebalance gerekmiyor** (denetlendi): `item.c` fiyatlarıyla demir kılıç 320 akçe =
  kırk günlük yevmiye, ortaçağ için doğru oran. Tek göze batan `goblin_derdi`'nin 100
  akçesi — bir işçinin on altı günü — ve **bilerek böyle kalıyor.** Maceracının
  kazandığı para şehri kızdıracak kadar çok olmalı; kadronun yarısının oyuncuya
  bakışının sebebi bu, sistemin çelişkisi değil.

### 1.2e KARŞILAŞMA SİSTEMİ [ONAYLI — Hakan 2026-07-31]
Hakan'ın isteği: *"Görevler sadece NPC'ye giderek veya pano aracılığı ile değil, bir
NPC'nin yanına yaklaşıp ondan yardım istemesi… Skyrim gibi bir yere giderken başına
başka olay gelmeli ve başka yönlere gitmeli. (tabi zorunlu olarak değil)"*

**Teşhis — Skyrim neden şakaya döndü:** *"eskiden ben de maceracıydım"* üç hatanın
birleşimiydi: replik bir **havuzdan** çekildi, konuşan aktörün **kimliği yoktu**, ve
üretim **sonsuzdu**. Tekrarın sebebi içeriğin azlığı değil — içeriğin *bir yere ve
bir kişiye bağlı olmaması*. Buradan çıkan kural:

> **Karşılaşma ya bir YERE ait olur ya bir KİMLİĞE. İkisine de ait değilse gürültüdür.**

- **[ONAYLI] YOL DÜĞÜMÜ omurga (yere ait).** `RoadNode { mapId, x, y, radius,
  hourFrom, hourTo, reqFlags, weatherMask, seasonMask, kind, npcId|sceneFile,
  traceDecor }`. Elle yerleştirilmiş, SONLU (ilk sürüm 20-24). Bir kez çalışır,
  sonra yerinde `traceDecor` kalır (sönmüş ateş, devrilmiş tekerlek) — onuncu
  geçişinde yol sana kendi geçmişini gösterir. Tür örnekleri: UYARI · İMDAT ·
  EMANET · SUÇLAMA · TANIKLIK · İŞE ALMA · MEYDAN OKUMA · SATIŞ/BORÇ.
- **[ONAYLI] GEZGİN KADRO doku (kimliğe ait).** 8-12 rotalı NPC (`NpcDef`'e
  `traveler` + `TravelLeg route[8]`). İçerik havuzdan DEĞİL, **gezginin o anki
  durumundan** türer: dün hangi bölgedeydi, orada hangi `WorldEvent` yaşandı,
  oyuncuyla kaç kez konuştu, o bölgedeki itibarı. Bu yüzden aynı gezgin iki kez
  aynı şeyi söylemez ve **değişimin sebebi sayılabilir**. Yaklaşma biçimi farklı:
  sana doğru yürümez, **yanında yürür** (30-40 sn eşlik, iki bark, sonra yol ayrılır).
  `worldevent.c`'yi konuşan bir ağza çevirir — Karaocak galerisi çöktüyse bunu
  panodan değil, yolda karşılaştığın adamdan **çelik fiyatı yükselmeden ÖNCE** duyarsın.
- **[ONAYLI] PERDE OYUNCUNUN ELİNDE — bağlayıcı yasa.**
  > Hiçbir karşılaşma oyuncu girdisi olmadan diyalog perdesi açmaz, kamerayı almaz,
  > oyuncuyu durdurmaz. NPC en fazla **iki bark** atar ve **iki-üç adım** yaklaşır.
  > 12 saniyede cevap gelmezse rutinine döner.

  Bu, Hakan'ın *"tabi zorunlu olarak değil"* cümlesinin mekanik garantisi.
- **[ONAYLI] IMMERSION KİLİTLERİ (hepsi zorunlu).** NPC yalnız kendi haritasında ve
  rutin poligonunda yaklaşır — **zindanda, mağarada, mühür odasında karşılaşma YOK.**
  `indoors` saatinde yaklaşma yok. Fırtınada yol kenarı boş (`npc_set_shelter` zaten
  yapıyor). **Ekran dışı spawn YASAK.** Ve ulak kuralı: **kimse seni aramaya gelmez**
  — yolda denk gelir. (Skyrim'in ulağı seni mağarada buluyor; bizde bulmaz.)
- **[ONAYLI] SIKLIK ve SUSMA.** 15 dakikada en fazla **2** karşılaşma; aynı türden
  ikisi arası en az 20 dakika (iki "yardım isteyen" arka arkaya gelirse dünya yaşayan
  değil **dilenen** olur). İlk 20 dakikada iki tane, asla üç; zincir başlatan
  karşılaşma ilk yarım saatte ÇIKMAZ.
  **En önemli kural: bütçe doldurmak için karşılaşma UYDURULMAZ** — koşulu tutan
  düğüm yoksa o gün sessiz geçer. İhlal edilirse sistem Skyrim'e döner.
  **Sistem şu üç anda susar:** (1) oyuncu aktif zincir düğümü taşıyorsa (bir hedefe
  yürüyor, elinde teslim edilecek bir şey var) → yalnız MANZARA tipi devam eder,
  çünkü manzara müdahale istemez · (2) dövüşten çıkalı 60 sn olmamışsa ya da can
  yarının altındaysa — *kanlı oyuncudan kimse yardım istemez; hem gerçekçi hem
  merhametli* · (3) gece, kandil hattının dışında → yalnız manzara ve tehdit.
- **[ONAYLI] SAPMA: ikinci hedef İŞARETÇİ ALMAZ.** J kitabına satır düşer, dünyada
  görünür bir iz kalır (duman, kandil, iz dekoru). Takipçi bandı değişmez; oyuncu
  isterse J'den kendi değiştirir. Orijinal hedefte hiçbir sayaç işlemez — süre
  baskısı yok, "kaçırdın" kutusu yok.
- **[ONAYLI] TEKRAR DEĞİL, İLERLEME.** Hiçbir karşılaşma aynı repliğiyle ikinci kez
  gelmez: ya bir kademe ileri **sayar** (yolcu: "çizelge üç haftadır boş" → "çizelgeyi
  kaldırmışlar" → "sunağın birini örttüler"), ya **başka bir insana devreder**, ya
  bir daha hiç gelmez. *"Eskiden ben de maceracıydım" tuzağı tekrardan değil,
  DEĞİŞMEYEN tekrardan çıkar.*
- **[ONAYLI] Karşılaşma da iz bırakır** ama zincir kadar sert değil: her karşılaşma
  ya bir **dekor izi** ya bir **NPC ilk-cümle değişimi** bırakmak zorunda. İkisinden
  hiçbiri yoksa o karşılaşma kesilir. Fiyat/olay-sıklığı değişimi karşılaşma
  seviyesinde gerekmiyor (o zincirlerin işi — §1.2c kural 4).
- **[ONAYLI] KALICILIK ve SAVE.** Düğüm başına 2 bit (yaşanmadı/yaşandı/kaçırıldı/
  sonucu görüldü). İleriye pay: `encState[32]` (24 kullanılır), `travelerState[16]`
  (12 kullanılır) — save sürümü **bir kez** artar, bir daha artmaz.
  **UYARI:** save.c'nin TÜM paralel tamponları grep'lenip büyütülecek. Bu tuzak bu
  projede DÖRT kez ısırdı (item/decor/pickTries/save tamponu).
- **"NPC oyuncuya gelir"in yarısı BEDAVA:** `npc_walk_to`, `npc_bark`,
  `npc_walk_cancel`, `npc_set_shelter` zaten `npc.h`'ta ve çalışıyor. Eksik olan
  tek şey **tetik**.

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
- **Aklama/loca eşiği [ONAYLI 2026-07-24]**: launderedTotal aklamaya VERİLEN
  kirli değerle artar (kesinti ÖNCESİ; kuyuda bırakma anında, kervanda satış
  anında) — örgüt kestiğini de sayar ("Hepsi sayılıyor"). Kuyuda bekleyen
  kese yeni bırakmayı engellemez: üstüne eklenir, birleşik kese geç olanın
  şafağını bekler. İp eşiği 500g (kirli) + gece.

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
Hakan testinden sonra seçilecek — muhtemelen SICAK/DARK FANTASY ekseni. ~~Filtre
şu an HUD dahil tüm kareye uygulanıyor (sahne-only varyantı açık soru).~~
**ÇÖZÜLDÜ 2026-07-30:** `r2d_postfx_flush()` ile sahne/HUD ayrımı yapıldı — bkz. §6.5 Karar 1b.

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

### 6.3 Beceri / meslek / damga / hat sistemi v2 [TASLAK — onaya]
Kanon reboot'undan sonra baştan çalıştırıldı (Hakan: *"22 olmak zorunluluğumuz yok, mantığı baştan
sona değişebilir, ama varlıkları olmalı"*). Tam tasarım: `docs/konsept/beceri-meslek-sinif-v2.md`.

**Karar 1 — HİÇBİR BECERİ SİLİNMEZ.** 23 mevcut beceri kalıyor; 3 yeniden adlandırılıyor
(ENCHANTING→**Damgacılık**, ECHO→**Ejder Sesi**, LIGHT→**Kutsama**, ayrıca SPEECH→Pazarlık,
ASSASSIN→Kısık), 2 ağaç metni düzeltiliyor (TWO_HANDED mızrak dayatması, MEDIUM_ARMOR kimliksizliği),
**2 yeni** eklenir → **25 beceri**. Sayı hedefi değil fiil hedefi: bir istasyon = bir beceri.

**Karar 2 — BALIKÇILIK mevcut toplama sistemine BAĞLANMAZ.** `ResourceDef`'in sözleşmesi
"hareket/hasar süreci keser" — gerginliğin kaynağı bu. Rahat fiil o kuralı miras almamalı.
Ayrı `FishingSpot` durum makinesi (~50 satır): sesli işaret, kesintisiz, tek tuş, kaçırınca ceza yok.
Kanon bağı sert: **Tuğrul'u öldüren delik balıkçı deliğiydi** (LORE §8).

**Karar 3 — RAHAT OYNANIŞ BOŞA GİTMEZ.** Odun → kandil → gece güvenli ada; balık + yemek → soğuk
direnci ve gece görüşü → **damar kazısını mümkün kılan şey.** Oyuncu mola veriyor sanıyor, aslında
bir sonraki gerginliğin ekipmanını üretiyor. `STATION_CAMPFIRE`'ın becerisi yoktu (XP Simya'ya
düşüyordu) → **SKILL_COOKING**.

**Karar 4 — "RÜN" GİTTİ, "DAMGA" GELDİ.** Mühür büyük, **damga** onun kılıca sığanı; "damga basmak"
hazır Türkçe fiil. Hammadde "öz" değil **harf**. Kanon gerekçesi: Akkemer'in mührü bir emirdir
(*dur*); bir kılıca sığan yazı ancak küçük bir şeyi ancak bir an tutar — **kılıcın istediği tam
olarak bu.** Beş VFX'in hepsi bir yere oturuyor: FLAME=Ocak (volkan) · ICE=Buz (donmuş göl, "dur"un
en dürüst görüntüsü) · POISON=Ağı (harf gerektirmez, sistemin giriş kapısı) · VENOM=Kısıklar
(ışıksız) · PINK=**Diri Damga** (yalnız taze mühür çatlağından, olaya bağlı ve **sonlu**).

**Karar 5 — DAMGA SARKAÇ'A DOKUNMAZ.** Her damga basanı dünyayı bozmakla suçlamak fatura refleksidir
(LORE §15.1). Bedel zaten dünyada: **damar taşı kazdığın gece mezarlık kalkıyor.** Yalnız diri damga
**hasadı** takvimi öne çekiyor — mühür sayısını değiştirmiyor.

**Karar 6 — HAT FORMÜLÜ: EL + FİİL + TANIK.** Eski formül ("beceri kesişimi + dünya eylemi") eksikti:
kesişim sessiz, eylem kimliği duyurmuyor. Kural şu: kimlik davranıştan doğar, **ad başkasından gelir**
(Kırca Ana unvanını hiç sormaz; §11'in ilk kademesi "nöbetçinin selamı"). Tanık çoğunlukla örgüt
rütbesidir **ama şart değil** — yoksa altı örgüte üyelik zorunlu olur ve rol özgürlüğü ölür.

**Karar 7 — 13 AÇIK + 2 GİZLİ HAT.** 22 fazla (yarısı "%5 hasar" olur, unvan sisteminin kopyası
çıkar), 6 az. Her örgüte iki hat (dışa/içe dönük yüz) + **bir örgütsüz hat** (Ocak Sahibi — sistemin
örgüt gerektirmediğinin kanıtı). Barışçıl oynanışın da hattı var: **Buz Balıkçısı** güç değil
**bilgi** veriyor — yüz saat kimseyi öldürmemiş oyuncu elinde siyasi bir bomba ile törene girebilir.

**Karar 8 — HAT KAPANMASI YOK, SAHNE TÜKENİR.** Dünya eylemleri geri alınamaz ve tek seferlik.
Derinlik kısıtlayıcı kuraldan değil **fiilin tükenmesinden** geliyor; kapıyı tasarımcı değil
oyuncunun eli kapatıyor (§15.7 korunur).

**[AÇIK]** Asa/Yıkım birleştirme kararı · Orta Zırh item'ı yazılacak mı yoksa Ağır Zırh'a mı
katılacak · Buz Balıkçısı hat mı unvan mı · Zehirci'nin gecikmeli ölüm sistemi yapılacak mı.

**MVP:** 25 beceri + balıkçılık + aşçılık + üç damga (tek yuva, yalnız silah) + **4 açık hat
(Siper Tutan / Lağımcı / İz Süren / Kemer Okuyan) + 1 gizli (Dinleyici)**.

**Kod borcu — dikkat:** `WEAPON_FX_VENOM` şu an `light.c`'de default dalına düşüp **pembe ışık
yayıyor** (gizlenme damgası için hata) · damga eşyaları main.c'de iki taneye sabit
(`CRAFT_ROW_RUNE0/RUNE1`) → paralel dizi tavanları dersi: ITEM_TYPES + panel + **save.c tamponları
birlikte**.

### 6.4 Suç / kelle / itibar sistemi v2 [TASLAK — onaya]
Tam tasarım: `docs/konsept/suc-kelle-itibar-v2.md`. Hakan: *"speech, pickpocketing, bounty duracak
değil mi? Onlar olmalı. Oyuncuya özgürlük sunuyoruz."* — üçü de duruyor, sistem büyüyor.

**Karar 1 — KELLE PARASI CEZA DEĞİL, BORÇ SENEDİ.** Devredilebilir, satılabilir, işle ödenebilir,
affedilebilir. Oyuncu "yakalandı" değil, **bir alacaklı buldu.**

**Karar 2 — ALTI ŞEHİR, ALTI HUKUK.** Yakalama panelindeki üç düğmeye (öde/direnç/rüşvet) **şehir
başına değişen dördüncü düğme** eklenir — bir enum + altı diyalog dalı, ama tadı bütün sistemi
taşıyor: Yenitaç *deftere yaz* (kontrat borcuna çevir) · Tuzkapı *senet imzala* (ödemezsen liman
kalıcı kapanır, sonradan ödemek AÇMAZ) · Yedisunak *kefaret* ("öde" düğmesi gri, akçe geçmez) ·
Karaocak *ocakta çalış* (krallık akçesiyle ×1.5) · Karduvar **mahkeme + kefil** (kelle parası hiç
yok, kapı kapanır) · Kumkapı *kan bedeli* (vali siler, **aşiret hatırlar**).

**Karar 3 — YARGI ÇEVRESİ ≠ COĞRAFİ BÖLGE.** `Region`'a `lawId` eklenir; `bounty[]`/`reputation[]`
`MAX_LAWS 8`'e bağlanır. Coğrafi bölge sayısı serbest kalır, **save tamponuna dokunmadan** (uyum
kodda doğrulandı: save.c sayıyı yazıyor, yalnız fazlasını reddediyor). Paralel dizi tavanı dersi
baştan çözülür.

**Karar 4 — HABER YAYILIMI OYNANIŞ.** Bounty kopyalanmaz; **kurye NPC'si** yola çıkar (haritada
görünür, 1-3 gün). Ulaşırsa hedef şehirde bounty'nin %40'ı yazılır. Kuryeyi kesebilirsin — ama kayıt
yok olmaz, **sahibi değişir**: Kırk Kese onu bilgi olarak alır. Kanundan kurtulup örgüte borçlanırsın.

**Karar 5 — HAPİS YOK.** Oyuncuyu odada bekletmek oynanış değil (§15.7 ihlali). Yerine: borç işi ·
zorla el koyma · sürgün. Tek istisna **ölüm yerine yakalanmak** — karakolda uyanırsın, eşyalar
veznede, üç çıkış. Hapsi ödüle çevirir.

**Karar 6 — KELLE AVCILIĞI ve SUİKAST AYRIMI TEK KELİME: KAYIT.** Avcılıkta tanık **iyidir**
(kontrat panoda, ödeme veznede, öldürme suç değil); suikastta tanık **ölümdür**. Aynı fiil, ters
işaretli tanık — mevcut `crime_witnessed` sözleşmesine birebir oturuyor, yeni sistem gerekmiyor.

**Karar 7 — DÜŞÜK İTİBAR BİR SINIF: "KORKULAN ADAM".** rep ≤ −40'ta haydutlar saldırmaz, haraç
diyalogları açılır, muhafız **takviye bekler**; karşılığında görev/şifa/Makam kapanır. Ve en zayıf
konum **orta itibar** — ne sevilen ne korkulan adam hiçbir kapıyı açmıyor. `REP_DAILY` yalnız
`|rep| < 50` aralığında çalışsın, yoksa korkulan adam olmak "itibar bakımı" grind'ine döner.

**Karar 8 — MAKAM: kanunun kendisi olmak.** Kapı ağası olunca başkasının bounty'sini görürsün, geçir
ya da çevir; geçirdiğin adam suç işlerse bir dilim **sana** yazılır (Karduvar kefil mantığı oyuncunun
elinde). Af yetkisi denetimli, ve **kendi adını kendi defterine yazamazsın.**

**[AÇIK]** Kaosçu bölümü (§10-13) harcama limiti yüzünden ajan denetiminden geçmedi — `REP_TALK_REFUSE
−50` duvarının yumuşatılması ("konuşmaz ama korkar") ve on kaos eyleminin dengesi doğrulanmalı.

**MVP:** dördüncü düğme (6 varyant) · `lawId` · `guards[]` dizisi · kurye (tek hat) · Kırk Kese
aklama + Kılıçartıkları kontrat eritmesi · kelle panosu (3 hedef) · kayıp mekaniği · REP_DAILY eşiği.

**Kod borcu — dikkat:**
- ⚠ [main.c:67](src/main.c#L67) bölge `-1`'i **0'a katlıyor** → ıssız arazide işlenen suç Köy'ün
  defterine yazılıyor. `-1 → LAW_NONE` olmalı.
- ⚠ **Tanıksız cinayet şu an tamamen bedava** → "kayıp" mekaniği (bir gün sonra NPC kayıp sayılır,
  bağlı NPC'ler rep −3). Suç değil, iz.
- ⚠ `guard_index()` tek NPC + tek statik state → altı şehir için `GuardUnit guards[12]` (şehir başına
  2 + kırsal devriye), birim başına `postX/postY` + `lawId`.

### 6.5 VFX his katmanı v2 [KODDA — Hakan testi bekliyor]
Hedef (Hakan): *"Muhteşem skill efektlerimiz olmalı. Oyun ve atmosfer ile uyumlu, pürüzsüz."*

**Karar 1 — BLOOM tek geçişli, ayrı render target YOK.** Post-fx shader'ında iki halka × 6 tap = 12
örnek. Parlak-geçiş eşiğinin üstündeki fazla, **luma oranıyla ölçeklenerek** eklenir — böylece kırmızı
alev kırmızı taşar, beyaza doymaz. Grade'den ÖNCE uygulanır: grade kontrast/parlaklık oynatınca eşik
kayar, ham sahneden okumak kararlıdır. 21 preset'in her birine ayrı ayar (GAMEBOY'da 0 — 4 renk
paleti taşmayı yutar; 8-BIT'te düşük — posterize ile çatışmasın).
**Karar 1b — SAHNE-ONLY FİLTRE (§6.1'in açık sorusu KAPANDI).** Hakan testi: *"VOLKAN'da tüm UI
dalgalanıyor normal mi?"* Hayır — ve en kötü hali: bükülme ekranın altına doğru güçlendiği için
(sıcak hava yerden kalkar) hotbar ve durum çubukları tam bükülmenin merkezinde kalıyordu.
Shader hangi pikselin HUD olduğunu bilemez, o yüzden ayrım **çizim sırasında** yapılmak zorunda.
`r2d_postfx_flush()` eklendi: o noktaya kadarki çizimler (tilemap, varlıklar, ışık pass'i, hava,
hasar vinyeti) offscreen'e akar, filtreden geçer, ekrana basılır; sonraki çizimler (HUD, panel, menü)
doğrudan ekrana gider ve **filtrelenmez**. Sınır main.c'de hasar vinyetinin hemen ardında.
Uygulama detayı: HUD flush'ı `clear=false` ile yapılıyor — `postfx_pass` her pikseli çoktan yazdı,
temizlemek sahneyi silerdi. Ayrım flush'ı ise `clear=true`, böylece hiç çizim olmayan karede
(menü ekranı) bayat sahne örneklenmiyor. Filtre kapalıysa çağrı hiçbir şey yapmaz.
**Yan kazanç:** bloom eşikleri artık HUD'u korumak zorunda değil — istenirse düşürülebilir.
**Tuzak (bir kez ısırdı):** `flush_to()` vertex sayacını SIFIRLAMAZ — sıfırlama `r2d_begin_frame`'de.
Aralık `firstVert` ile seçilir. İlk denemede son HUD flush'ı `firstVert=0` ile çağrıldı ve **bütün
dünyayı filtresiz yeniden çizip filtreli görüntünün üstünü kapattı** → "hiçbir filtre çalışmıyor"
görünümü. Doğru desen `r2d_target_begin`'de zaten vardı (`savedVertCount`). Kare içinde ikinci bir
flush yapan her kod bu sayacı saklamak zorunda.

**Karar 2 — EFEKT IŞIĞI: her AnimFx sönen bir nokta ışık yayar.** Bu turun en değerli parçası:
ışık yaymayan efekt "sahnenin üstüne yapıştırılmış çıkartma" gibi duruyordu; ışık yayan efekt duvarı
aydınlatır, **gölgeyi kaydırır**, atmosfere girer. Eğri ilk %20'de yükselip kare eğriyle söner —
patlama önce parlar, sonra kor gibi çekilir (düz sönüş "kapandı" gibi duruyor).
`animfx_spawn_lit()` eklendi, eski `animfx_spawn()` ona delege ediyor → **17 çağrı noktası bozulmadı.**
Işıklar listenin SONUNDA ekleniyor: tavan dolarsa kaybedilecek ilk şey geçici parıltı olsun, kandil değil.

**Karar 3 — BEYAZ-SICAK FLAŞ additive üst geçişle.** Çarpımsal `color_mod` beyaza ÇIKAMAZ (255 =
tint yok); parlaklık ancak toplamayla gelir. İsabetin ilk ~2 karesinde silüetin üstüne additive ikinci
çizim, sonra mevcut kırmızı tint.
**Ayar borcu:** `hitFlash` kodda üç değerle kuruluyor (0.12/0.15/0.2) → beyaz faz uzunluğu tutarsız.

**Karar 4 — ISI BÜKÜLMESİ durum efekti, tarz değil.** Post-fx'te ekran-uzayı UV kaydırma, iki
frekanslı sinüs (tek sinüs akordeon gibi duruyor) ve **ekranın altına doğru güçleniyor** (sıcak hava
yerden kalkar) — böylece HUD üstü daha az etkilenir. En başta uygulanıyor: sapma ve bloom bükülmüş
görüntü üstünden çalışıyor, fiziksel sıra bu. Vinyet bilinçli olarak bükülmüyor (ekran kenarı sabittir).
Test için **VOLKAN** preset'i eklendi; KORKU/RUYA/CORAK/VHS/KABUS'a küçük değerler.

**Karar 5 — YER İZLERİ (yeni sistem: `decal.h/.c`).** AnimFx patlamanın KENDİSİ, decal patlamanın
BIRAKTIĞI şey. Doku kodda üretiliyor (asset beklemez) ve kenarı **bilinçli düzensiz** — üç farklı
frekansta açı modülasyonu; kusursuz daire yapay duruyor (ışık halkalarından öğrenilen ders).
Çizim sırası: gölgelerden SONRA, dekordan ÖNCE — gölge zemine ait, iz onun üstüne biner.
Ömürler bilinçli farklı (yanık 45s taş kararır · kan 30s kurur · kırağı 18s erir · zehir 12s
buharlaşır); aynı süre verilseydi hepsi "aynı sistem" gibi okunurdu. Solma ömrün son %35'inde başlar —
iz bir süre gerçekten DURSUN. Havuz dolarsa en eskiyi ezer: oyuncunun az önce yaptığı şeyin izi
sessizce kaybolmasın.
Tetikleyici damga tipine bağlı (`g_fxBridge`): alev→yanık · buz→kırağı · zehir/ağı→birikinti ·
ölüm→kan. Harita geçişinde temizlenir.

**Zaten kodda olduğu bulunanlar (eklenmedi):** hit-stop (`g_hitStop`, `dt *= 0.08f` — dondurmak yerine
yavaşlatıyor, daha iyisi) · ekran sarsıntısı sönümüyle · `mob.e.hitFlash`.

**Kapatılan kod borçları:**
- ⚠ `CMakeLists.txt:4-5` **`C_MAKE_STANDARD` → `CMAKE_C_STANDARD`** (iki satır). Değişken adı yanlış
  olduğu için **C standardı hiç uygulanmıyordu**; artık `-std=gnu11` gerçekten geçiyor ve derleme
  yine sıfır uyarı.
- ⚠ `WEAPON_FX_VENOM` `default:` dalına düşüp **pembe ışık yayıyordu** → artık hiç ışık yaymıyor
  (Kısıklar damgası gizlenme damgası; LORE: Kabukçu damgası parlar ve çağırır, Kısık sessizdir).
- ⚠ `light.c` içinde `flicker` hesaplanıp `(void)flicker` ile çöpe atılıyordu (**ölü kod**) → artık
  yarıçap nefesi olarak kullanılıyor.
- ⚠ `main.c` `"%s'na geçildi"` → **`"Geçildi: %s"`**. Eskisi ekranda `Yay'na`, `Balta'na` yazıyordu;
  Türkçe ünlü uyumu kodla üretilemez (dil standardının 1 numaralı kuralı).
- `lamp.h` bayat yorum (165 → 122).

### 6.6 VFX / SKİLL MOTORU v3 — prosedürel [KODDA — Hakan testi bekliyor]
Hakan'ın çerçevesi (2026-08-04, bağlayıcı): *"Bunları sadece silaha, kamp ateşine alev, duman
ekliyoruz gibi düşünme — biz aslında sistem/motor oluşturuyoruz. Skillerde, doğa olaylarında,
bosslarda da kullanacağız."* Yani bu bölümün her kararı **tek bir tüketici için değil, katman
olarak** yazılır; ateş, büyü, boss darbesi, hava olayı aynı çağrıları paylaşır.
Görsel dil kararı: **KLASİK RPG GÖRKEMİ** (Hakan seçimi) — büyük, parlak, bol partikül, uzun ömür.
Pixel-art disiplini (sanat ızgarası, kuantalanmış palet, sert kenar) yine bağlayıcı: görkem
ÖLÇEKTEN ve SÜREDEN gelir, bulanıklıktan gelmez.

**Karar 1 — BÜKÜLME ALANI genel motor katmanı (`r2d_distort_set`, 8 kaynak).** Isı bükülmesi
§6.5'te tarz preset'ine bağlıydı: efekt isteyince tüm ekranı VOLKAN'a çevirmek gerekiyordu.
Artık bükülme **ekran-uzayı kaynak listesi**: stil (ISI / ŞOK / BURGAÇ / MERCEK), yarıçap, şiddet,
ömür. Post-fx pass'inin İÇİNDE uygulanıyor (sıra: bükülme → sapma → bloom → grade), `distortOn`
bayrağı F7 grade seçiminden ayrı. Dört tüketici sistemi doğruladı: kamp ateşi ısısı, büyü şoku,
buz kubbesi merceği, mermi izi. Boss vuruşu ve patlama da aynı çağrıyı kullanacak.

**Karar 2 — EFEKT NEFESİ tek kaynaktan (`fx_breath`).** Bir ateşin partikül yoğunluğu, ısı
bükülmesi genliği ve ışık şiddeti AYNI osilatörden okunuyor. Ayrı ayrı hesaplanınca üç katman
birbirinden bağımsız titriyor ve göz "tek bir ateş" görmüyor. Kanon bağı (LORE §1.9): koruyan şey
durur, korumayan gezinir — kandil sakin, ocak sıkı, kamp ateşi kaotik.

**Karar 3 — MOTOR/OYUN DİKİŞLERİ istek kuyruğuyla.** Motor katmanı oyunun değişkenlerine uzanmaz;
İSTEK bırakır, oyun katmanı tüketir: `skillfx_take_hitstop()` (zaman ölçeği main'in), 
`skillfx_take_shake()` (kamera main'in), `fx_set_death_hook()` (motor "öldü" der, oyun kalıntıyı
seçer). Böylece boss, tuzak, düşme hasarı da aynı kuyruğu kullanabiliyor.

**Karar 4 — PROSEDÜREL SKİLL EFEKTLERİ (`skillfx.h/.c`), sprite yolu SİLİNMEDİ.** Hazır sheet'ler
üç şeyi vermiyordu: tutarlılık (her sheet başka sanatçının dili), tepki (sheet mesafeye/güce/hava
durumuna göre değişemez), sahiplik (yeni büyü = dosya indirmek). Prosedürel efekt mevcut katmanları
BESTELİYOR — kendi çizim sistemini kurmuyor: fx partikülleri + bükülme + ışık + r2d primitifleri +
decal kalıntı. **Ctrl+P A/B anahtarı**: karşılaştırma olmadan "daha iyi mi" sorusu cevaplanamaz;
eski yol rafta duruyor (BOŞA ÇIKAN KODU SİLME kuralı).

**Karar 5 — İSABET ELEMANA GÖRE (`skillfx_impact`, 7 eleman).** "Vurdum" hissinin çekirdeği
YÖNDÜR: darbe her yöne saçılırsa vuruşun nereden geldiği okunmaz. Koni darbe yönünde açılır, yay
darbeye DİK çizilir (bulut "toz" anlatır, yay "kesti/çarptı" anlatır). Hit-stop ve sarsıntı
elemandan gelir: yıldırım keskin-kısa (0.090/7.5), toprak ağır-uzun (0.100/9.5), zehir neredeyse
yok (0.020/1.5) — hepsi hasarla ölçekli, tavanı 2.5.

**Karar 6 — PARÇACIK BİÇİM ATLASI kodda üretiliyor (7 hücre).** Tek yumuşak parıltı dokusuyla her
efekt aynı görünüyordu. Biçimler: PUF (duman) · KIYMIK (buz) · ÇİZİK (kıvılcım, hıza göre uzar) ·
YILDIZ (arcane) · NOKTA · **ALEV DİLİ** · İS PULU.
*Hata geçmişi (Hakan ÜÇ tur üst üste bildirdi — bu bölümün en pahalı dersi):*
(a) ateş patlaması 110 KIVILCIM kullanıyordu; kıvılcım çizik biçimi + hıza göre uzama taşıdığı için
alev **yıldırım çizgisi** gibi okunuyordu → alev dili ve is pulu tipleri eklendi, kıvılcım 26'ya indi.
(b) alev dilinin genişliği y ile DOĞRUSAL daralıyordu, sonuç keskin bir **üçgen huni** oluyordu
(*"çok kötü duruyor"*) → profil yaprağa çevrildi: `pow(t,0.55)` uca sivrilik verir, `(1-0.34·t³)`
en dibi toplar, en geniş yer dipte değil dibin biraz ÜSTÜNDE kalır.
(c) **üçgenler GİTMEDİ** (*"alev skilinde hala üçgenler var ve yıldırım etkisi gibi efektler var"*).
İki ayrı sebep vardı ve ikinci turda yanlış yere bakmıştım:
  · İS PULU biçimi `|dx|+|dy|` ölçüsüyle yazılmıştı. O ölçü matematiksel olarak **eşkenar dörtgen**
    çizer — sert kenarlı, eksene hizalı elmas; takla attığı için birçok açıda ÜÇGEN okunuyor. Her
    patlama 34 tanesini fırlatıyordu. Düzeltme: yarıçapı **açıya göre** modüle et (üç frekans),
    böylece düz kenar ve köşe hiç oluşmaz. Aynı numara `decal.c`'de yer izi kenarında zaten
    kanıtlıydı — bilinen çözümü yeni yere taşımamak hatanın kendisiydi.
  · Ateş HÂLÂ 26 kıvılcım atıyordu. Sayıyı azaltmak yetmez: **tip yanlış.** Kıvılcım çelik ve
    yıldırıma ait; ateş KOR fırlatır (nokta biçim, uzama yok). Ateş bloğundan kıvılcım tamamen
    çıkarıldı, yerine 44 kor geldi.
**İki ders:** (1) prosedürel biçim koda girmeden önce siluetini ASCII bas — üç turun üçü de tek bir
ASCII dökümüyle kapanırdı. (2) `|dx|+|dy|` (L1) ölçüsü "düzensiz" değil GEOMETRİK bir şekil verir;
düzensizlik isteniyorsa açısal modülasyon kullanılır.
*Bilinçli istisna:* KIYMIK (buz/kaya) eşkenar dörtgen KALIYOR — buz kırığı ve kaya parçası gerçekten
köşelidir. Yuvarlak biçim yalnız ateş ve is için doğru.

**Karar 7 — TON KİMLİĞİ palet varyantıyla (`fx_set_pal_variant`).** Hakan: *"her skill de farklı
renk tonları olmalı."* Palet tablosu satır başına üç varyant taşıyor ve indeks her satırda AYNI
aileyi gösteriyor: 0 = maddenin kanonik tonu, 1 = SOĞUK aile, 2 = SICAK aile. Bu yüzden olay başına
tek indeks yeterli — yıldırımın kıvılcımı maviye giderken dumanı da soğuk griye kayıyor,
uyum kendiliğinden kalıyor. `fx_spawn` imzası değişmedi (100+ çağrı yeri korundu); varyant "şu anki
ton" global'i olarak spawn anında parçacığa geçiyor ve olay bitince sıfırlanıyor (kamp ateşi ile
büyü aynı karede farklı ton taşır).
*Bulunan çakışma:* yıldırım kolu (200,215,255) ile buz novası halkası (175,222,255) neredeyse AYNI
renkti — iki büyü ekranda ayırt edilemiyordu. Yıldırım **mor-beyaza**, buz **camgöbeğine** çekildi.
İsabet yayı da yedi elemanın tamamı için renklendi (önce yalnız ikisi özeldi, kalan beşi aynı altın
yayı çiziyordu).

**Karar 8 — PARÇACIK TAVANI gerçek darboğazdı.** Hakan *"üçü de çok cılız"* dediğinde sorun ayar
değildi: `MAX_PARTICLES` 512'ydi ve fazlası SESSİZCE düşüyordu. 2560'a çıkarıldı (quad bütçesi
16384). Ders: "cılız" şikayetinde önce tavanı ölç, sonra sayı büyüt.

**Karar 9 — KAPSAMA BOŞLUĞU GÖRÜNÜR OLMALI (yol göstergesi).** Üçgen şikayeti dördüncü tura
girdiğinde sebep biçim değildi: Hakan *"alev skili"* derken **ATEŞ ASLANI**'nı (`ABILITY_FIRELION`)
deniyordu — F10'un verdiği tek ateş büyüsü o, ve o büyü sprite yolundaydı; Ctrl+P yalnız ateş
topu ile taş dikeni kesiyordu. Ben her turda prosedürel ateş topunun biçimini düzeltiyordum,
o başka bir şeye bakıyordu. **Hata benim: A/B anahtarı kısmi kapsamı gizliyordu.**
İki düzeltme: (a) ateş dalgası motora alındı (aşağıda), (b) prosedürel mod açıkken her büyü
çağrısı ekrana `"<büyü adı> -> PROSEDUREL (motor)"` ya da `"-> SPRITE (motor disi)"` yazıyor.
Kapalı modda susuyor. **Kural: kısmi kapsamlı bir A/B anahtarı, kapsamı ekranda göstermek
zorundadır** — yoksa test eden kişi kendi hatası sanır (*"yanlış mı bakıyorum?"*).

**Karar 10 — ALEV SELİ (`SKILLFX_FIRE_WAVE`), eski adı Ateş Aslanı.** Bu karar ÜÇ turda oturdu ve
üçü de aynı sorunun farklı yüzüydü: *prosedürel gövde neyi anlatabilir?*
- **Tur 1 — koşan alev duvarı.** Sprite bir ASLAN çiziyordu; ilk prosedürel karşılık aslan taklidi
  yapmaya çalışmadı, "hat dalgası" kanonundan bir alev duvarı kurdu. Hakan: *"tam istediğim gibi
  değil"*.
- **Tur 2 — maskeli aslan silueti.** Siluet kodda basıldı, partiküller yalnız maskenin içinde
  doğdu. Başarısız oldu ve sebebi ÖLÇÜ: aslan 168px genişken fırça (partikül) 12-42px, yani siluet
  dört blob genişliğinde kalıyor; üstüne partiküller 30-90px yol alıp şekli doğduğu anda dağıtıyor.
  Hakan'ın tarifi tam isabetliydi: *"alev topunun büyüğü gibi, havan 2-3 parçalı iniyormuş gibi"*.
- **Tur 3 — kodda basılmış gövde dokusu** (taş dikeninin yolu: nesne olan şey gövdeyle anlatılır).
  Teknik olarak çalıştı — 3 görünüm × 3 koşu karesi, kızıl gövde ALPHA + parlak çekirdek ADD,
  nefesle titreyen baş. Ama: *"gerçekten aslan çizmeye çalışmışsın ama çok kötü amatör durmuş."*

**KALICI DERS:** prosedürel gövde **soyut** biçimlerde çalışıyor (taş dikeni, buz kristali, alev
dili, is pulu); **tanınan bir canlıya** benzemekte çalışmıyor. Göz, canlı anatomisindeki en küçük
hatayı yakalıyor — taşta yakalamıyor. Canlı görseli elle çizilmiş asset ister. Bu, taş dikeni
kararının (Karar 8b) sınırını da tanımlıyor: "nesne gövdeyle anlatılır" doğru, ama nesne CANLI ise
prosedürel gövde yetmez.

**BENİMSENEN ÇÖZÜM — sarı sel + iç içe kırmızı siluetler.** Hakan'ın tarifi: *"düz bir hat üstünde
ilerliyor ve sarı efekt içerisinde kırmızı silüetler olmalı"*, ardından iki netleştirme: *"kırmızı
ve sarı iç içe olmalı, birbirinden ayrı hareket değil"* ve *"sarı üstüne kırmızılar gibi"*.
Bu iki koşul birbiriyle çelişiyor gibi görünüyor ve mimariyi belirleyen de o çelişki:
- kırmızı GÖRÜNMELİ → toplamalı (ADD) sarının altında kalırsa yutulur (R doyunca hue kaybolur),
  demek ki sarının ÜSTÜNE çizilmeli;
- ama AYRI HAREKET ETMEMELİ → sarının üstünde kayan ayrı bir geometri katmanı "iki efekt" okunur.

Çözüm ikisini birleştiriyor: kırmızı bir katman değil, **aynı yayıcıda AYNI konum ve AYNI hızla
doğan bir PARTİKÜL TİPİ** (`WEAPON_FX_FLAME_DARK`) — fizik aynı kod, akış aynı, yalnız çizim sırası
farklı. `fx_render` bu yüzden **üç geçişli** oldu: 0 ALPHA (madde) → 1 ADD (ışık) → 2 ALPHA-ÜSTÜ
(koyu siluet). Sel yalnız cephede değil geçtiği HATTIN TAMAMINDA yaşıyor (yoksa "uçan top"
okunuyor), yoğunluk cepheye doğru artıyor, yan salınım düzgün mekânsal fonksiyondan geliyor
(hash DEĞİL — su fazının 2. kuralı: hash düzensizliği komşuları koparır).
**İSİM:** mekanik değişmediği için isim de değişmek zorunda değildi ama görsel artık aslan değil;
`Ateş Aslanı` → **`Alev Seli`** (Hakan kararı). 22 yer güncellendi; kod tanımlayıcıları da
(`ABILITY_FIRESURGE`). Yetenek İNDEKSİ yerinde kaldı — `spellsLearned` bit maskesi sırayla
saklanıyor, indeks kaydırmak eski kayıtları bozardı.
*Rafta duran:* alev duvarı ve aslan (maske + gövde dokusu) `#if 0` içinde gerekçeleriyle. Elle
çizilmiş bir aslan sheet'i gelirse maske hâlâ yele/ağız bölgesi veriyor.

**Karar 10b — ATEŞ RENKLERİ KARA CİSİM EĞRİSİNDEN (araştırma, deneme-yanılma DEĞİL).**
Hakan üç tur renk ayarı istedikten sonra doğru kararı verdi: *"Sen internetten alev renklerini biraz
araştır istersen deneme yanılma yapmayalım."* Alev rengi keyfi değil FİZİK: ısınan cismin ışıması
kara cisim eğrisi üstündedir.
Ölçülen eğri (Tanner Helland yaklaşımı; vendian.org ve firgelliauto tablolarıyla doğrulandı —
2000K ve 3000K'da birebir):
| K | RGB | K | RGB |
|---|---|---|---|
| 1100 | (255, 77, 0) | 2100 | (255,142, 27) |
| 1500 | (255,108, 0) | 2500 | (255,159, 70) |
| 1900 | (255,132, 0) | 2900 | (255,174,103) |

**KÖK SEBEP:** eğride **mavi kanal 2000K altında SIFIR**, 3000K'da bile ancak ~110. Elle uydurduğum
"sıcak" tonlarda B=48..132 vardı — yani eğrinin DIŞINDA. Bu tek hata iki şikayeti birden üretiyordu:
sarı beyaza kaçıyordu ve *"iç alev rengi biraz garip"* (mat/pembemsi) duruyordu.
**YAN KAZANÇ, ayarla elde edilemeyecek olan:** B≈0 olduğu için toplamalı (ADD) üst üste binme artık
beyaza değil **sarı-turuncuya** doyuyor. Yoğun alevin beyazlaması matematiksel olarak ortadan kalktı;
bu yüzden "sıcak ton payı" gibi telafi ayarları da geri açılabildi.
**KURAL:** yeni bir ateş tonu gerekiyorsa elle RGB girilmez — sıcaklık seçilir ve eğriden hesaplanır
(formül `fx.c` palet tablosunun yorumunda). Bilinçli istisnalar etiketli: arcane mavi-beyaz ateş ve
mor ateş eğri DIŞI (büyü, ateş değil); çelik kıvılcımı beyaza yakın kalıyor (yanan demir gerçekten
~3000K üstü).

**Karar 11 — VARYANT AİLESİ HER SATIRDA AYNI ANLAMA GELMEK ZORUNDA.** Karar 7'nin ilk yazımında
tablo karışıktı: FLAME'de varyant 1 = kızıl (sıcak) ama SPARK'ta 1 = mavi (soğuk). Sonuç ters
çıkıyordu — **yıldırım sıcak kahve duman, toprak soğuk gri duman** alıyordu. Olay başına tek
indeks kullanmanın bedeli bu tutarlılık: 0 = kanonik, 1 = SOĞUK aile, 2 = SICAK aile, istisnasız.

**Açık kalan:** kamp ateşi sprite alevi kalsın mı (karar Hakan'da) · alev/buz rünü eşya olarak
eklenecek mi (mekanik kodda, hiçbir eşya damgayı vermiyor — şu an yalnız Ctrl+G ile test edilebilir)
· kalan 13 büyünün prosedürel motora göçü (beşi onaylanırsa). Göçmemiş olanlar artık ekranda
kendini `SPRITE (motor disi)` diye ilan ediyor — hangi büyünün sırada olduğu tahmine kalmıyor.

## 7. UI / UX [KODDA]
WoW hotbar · HUD (mevsimli saat, kontrat takipçisi) · panel ailesi
(tooltip/drag&drop) · minimap · menü + 20+1 slot + yaratma-lite.

### 7.1 YERLEŞİM VERİDEN GELİR [KODDA 2026-07-31]
HUD konumları koddan çıktı: `assets/ui/layout.json` (23 öğe) + oyun içi
düzenleyici (Ctrl+U). Kararlar:

- **[ONAYLI] ÇAPA merkezde.** Konum, pencerenin hangi kenarına/köşesine göre
  ölçüldüğünü söyler (9 çapa: `sol-ust` … `sag-alt`) + yatay `hiza` +
  dikey `dikey`. Yeniden boyutlandırma bedavaya doğru çalışır; öğe başına
  formül yazılmaz. **Yanlış çapanın belirtisi görüntüde YOK** — ancak pencere
  boyutlanınca ortaya çıkar; bu yüzden düzenleyicide "en uygun çapa" (E) ve
  şüpheli-ofset uyarısı var.
- **[ONAYLI] BOYUT çoğu öğede veriye ALINMAZ.** Can barı genişliği max stat'la
  BÜYÜR (kasıtlı geri bildirim), panel yükseklikleri satır sayısından türer
  (`stockCount × ROW_H`), envanter yüksekliği çanta kapasitesinden. Bunları
  dosyaya çivilemek özelliği öldürürdü. Veriye giren: panel **genişlikleri**
  (SHOP_W/CT_W/… gerçek tasarım kararı), minimap kutusu, yakalama paneli
  (tek sabit-boyutlu panel).
- **[ONAYLI] Düzenleyici oyun içinde, web/masaüstü değil.** Konumun doğruluğu
  FONTUN kendisine bağlı; gerçek ölçü, gerçek pencere, gerçek post-fx altında
  bakılmadan "16 mı 20 mi" cevaplanamaz. Dışarıda yapılan her ölçüm tahmindir.
- **[ONAYLI] Tutamaklar öğenin CANLI olmasını beklemez** — düzenleyici yerleşim
  *verisini* dolaşır. Tok buffunu dizmek için yemek yemek gerekmiyor. Paneller
  için ön izleme (P): seçili panel kapalı olsa bile gerçek içeriğiyle çizilir.
- **[ONAYLI] Tek kaynak zorunluluğu.** Konum formülü çizim VE tıklama testini
  besliyorsa iki yerde durmaz. Hotbar'da üç, envanterde beş kopya vardı;
  panelleri taşınabilir yapmak onları sessiz sapma noktasına çevirirdi (panel
  taşınır, tıklama eski yerde kalır, derleyici susar).
- **[ONAYLI] Yorum kuralı.** Değere ait not → alan (`"_not"`); dosyaya ait not
  → koda/GDD'ye. Yorum yalnız oyunun ÜSTÜNE YAZMADIĞI dosyalarda; `layout.json`
  düzenleyici tarafından yeniden yazıldığı için yorumsuz (`jsonx` JSONC
  desteği duruyor — çeviri dosyaları gibi elle-yönetilen dosyalar için).
- **[AÇIK]** Tam ekran öğeler (ölüm ekranı, menü, yaratma, vinyet sahnesi),
  imleci izleyenler (tooltip, sürükleme hayaleti), dünyaya bağlı olanlar
  (hedef HP barı, kamp ateşi barı) ve letterbox yerleşim dışı — letterbox'ın
  düzenlenecek konumu yok, tek değişkeni sinematik bant yüksekliği.
- **[AÇIK]** Beceri AĞACI paneli (`tree_panel_render`) hâlâ kodda: konumu
  beceri paneline göreli (`40 + SKILL_PANEL_W + 10`). Diyalog paneli de
  taşınmadı (`dialog_render` kendi yerleşimini winW/winH'ten kuruyor).

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

### 8.1 KİMLİK ALANI ZORUNLU [ONAYLI 2026-07-31]
İçerik tablolarında **görünen ad kimlik değildir.** Kod `.id` ile arar, `.name`
yalnız gösterimdir.

- **Biçim — NPC:** `<harita>_<yer>_<rol>` (`dunya_koy_demirci`,
  `loca_salon_usta`, `tasdurak_han_hanci`). Rol seçildi çünkü ad değişse bile
  rol aynı kalır; önek seçildi çünkü altı şehirli kanonda iki demirci olacak.
- **Biçim — tarif:** `<istasyon>_<slug>` (`alch_pismis_et`,
  `campfire_pismis_et`). İstasyon öneki ZORUNLU: dört tarif adı iki kez geçiyor
  ve ada göre arama ilk eşleşmeyi döndürüyordu — varyantların biri erişilemezdi.
- **Neden:** ad hem gösterim hem kimlik olunca yeniden adlandırma zinciri
  SESSİZCE koparıyor (derleyici susar, oyun çökmez, NPC konuşmaz). DEMİRBOYUN
  kanonuna geçerken tüm adlar değişeceği için bu bomba önce söküldü.
- **Ad KOPYASI tutulmaz:** başka tablo bir NPC'ye atıfta bulunacaksa id tutar,
  adı `NPCS[]`'ten okur (`StationDef.ownerNpcId`). Kopya, NPC yeniden
  adlandırılınca eskir ve oyuncuya var olmayan bir ad gösterir.
- **Bulunamayan id SESSİZ KALMAZ** — `npc_index_by_id` / `recipe_index_by_id`
  loga yazar. Bu hata sınıfının bütün maliyeti sessiz olmasıydı.
- **Save'e girmez:** kayıtta NPC yok, görevler `QUESTS[]` SIRASIYLA tutuluyor.
  Donmuş olan şey ad değil SIRA — id'ler tek commit + grep ile yeniden
  adlandırılabilir. Tabloya ORTADAN giriş eklemek save'i bozar, sona eklemek güvenli.
- **[AÇIK]** item/mob/unvan/görev tablolarında henüz `.id` yok — bugün ada göre
  aranmıyorlar. Metin katmanı geldiğinde çeviri anahtarı olarak gerekecek.

### 8.2 SUÇ DEFTERİ HARİTADAN GELİR [KODDA 2026-07-31]
`MapDef.crimeRegion` üç durumdan biri: `CRIME_BY_RECT` (dünya — konumdan
`region_at`), `CRIME_LAWLESS` (zindan katları), ya da sabit bölge indeksi.

- **[ONAYLI] Kanunsuz bölge SAHİPSİZ BİR SİCİL, kapalı bir kapı değil.**
  `REGION_KANUNSUZ` gerçek bir bölge girdisi; bounty oraya yazılır ama onu
  toplayacak muhafız, ödenecek tahsildar, kırılacak itibar yok. Alternatif
  (7 ayrı bounty çağrı yerine "burada suç işlemez" dalı eklemek) reddedildi:
  unutulacak yedi kontrol yerine unutulacak hiçbir kontrol.
- **[ONAYLI 2026-08-03] MUHAFIZ ÇOĞULLANIR, HİYERARŞİ KADRODAN GELİR.** Bugün tek
  `static int gIdx` var. Üç ayrı sahip tanınacak: Yenitaç'ın kapı nöbetçileri
  **kışlaya** bağlı (Kılıçartıkları, mareşalin defteri) · çayır köyünün devriyesi
  **bölgeye** bağlı (yerel, maaşını Yenitaç'tan alır ve gecikir) · Karduvar'ınki
  Ak Nöbet'e bağlı olacak ve krallık defterine **hiç yazmaz**. Yani `guards[N]` bir
  `owner` alanı istiyor; bounty kimin defterine yazılacağı o alandan okunur. Bu,
  §8.2'nin geri kalanının mantığının aynısı: sicil sahibi haritadan değil KURUMDAN
  geliyor, ve bir gizli locanın suçunun köy muhtarına yazılmaması gibi, Karduvar'ın
  suçunun da krallığa yazılmaması gerekiyor.
- **[ONAYLI 2026-08-03] YENİTAÇ'IN MEZARLIĞI YENİTAÇ'IN SİCİLİNDE.** Kadro fazı
  Yenitaç'a kendi mezarlığını verdi (`yenitac_mezarlik_gassal`, ölü yıkayıcı) ve o
  mezarlığın zemini kanondan geliyor: **Sabri'nin yanmış kasabası Yenitaç'ın altında.**
  Köyün mezarlığı ayrı kalıyor ve sicili köyün. İki mezarlık, iki sicil — ve önek kuralı
  (§8.1: önek kanon yeridir) böylece doğrulanıyor.
- **[ONAYLI] Loca ve konak kendi sicilini tutar, köy mezarlığı köyün.** Gerekçe
  kurgusal: gizli bir locanın suçunun köy muhtarına yazılması saçmaydı ve tam
  olarak bu oluyordu. Mezarlık köyün kenarında, sicili köy tutar.
- **Bölge indeksleri ADLANDIRILDI** (`REGION_LOCA` vb.). Eskiden main.c'de
  `return 5;  // REGIONS[5] = Taşdurak` yazıyordu; tabloya ortadan bir bölge
  eklenirse o 5 sessizce başka şehri işaret ederdi.
- `MAX_REGIONS` 8 → 16. **Save güvenli:** save.c bölge sayacını dosyaya yazıyor
  ve okurken o sayı kadar okuyor — eski kayıtlar 8 çift okur, kalanı sıfır kalır.
  DEMİRBOYUN altı şehir getiriyor, yer gerekiyordu.
- **[AÇIK] Muhafız tek kişi.** Çoğullanacak ama yapıyı kanon kadrosu belirleyecek
  (şehir başına kaç, kime bağlı, Ak Nöbet ayrı mı). Şimdi `guards[N]` yazmak
  hiyerarşiyi tahminle kurmak olurdu. Not `guard.h`'ta.

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
