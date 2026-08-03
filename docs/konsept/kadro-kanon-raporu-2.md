# İKİNCİ KANON DENETİMİ — YENİTAÇ KADROSU (rev. 2)

> Kanon editörünün ikinci turu, 2026-08-03. Görev: (a) birinci turun bulguları kapandı mı,
> (b) **revizyonun kendisi yeni çelişki doğurdu mu.** İkincisi asıl soruydu ve cevap: evet, beş tane.
>
> **İKİ BULGU BAYAT (denetçi LORE'u benim düzeltmemden önce okumuş):** KR1'in §8 yarısı ve
> KR8'in tamamı bu rapor yazılırken **çoktan uygulanmıştı** — LORE §8'e atıf bloğu ve nöbet
> taşı kanıtı, LORE §10'a beş ağız sayacı ve yeni kapı. KR1'in §6 yarısı gerçekten açıktı ve
> raporun ardından kapatıldı. Kalan bulgular geçerli.

Denetlenen: `kadro-yenitac-slotlar.md` (rev.2) · `kadro-isim-havuzu.md` (v2) · `kadro-kartlari-salon.md` · `kadro-kartlari-sehir.md`
Karşılaştırılan: `docs/LORE.md` (tamamı) · `docs/GDD.md` §1.2c/d/e, §1.4, §8.1, §8.2 · `assets/scenes/intro_demirboyun.txt` · `src/npc.c`, `npc.h`, `contract.c`, `craft.c`, `main.c`, `save.c`, `gametime.h`, `item.c`

**Kod iddiaları temiz** — üç iddia da doğrulandı, bu turda sayım doğru.

---

# KIRICI

## KR1 · Gökçe'nin yalanı kanona işlenmedi; LORE hâlâ tersini söylüyor

**Nerede:** `docs/LORE.md` §8 ("Nasıl" paragrafı) ve §6 (Gökçe / "Kimseye söylemediği") · `kadro-kartlari-sehir.md` slot 10.

> LORE §8: *"Gökçe arkasından koştu, buz gitti. **Gökçe elini uzattı, yakasını tuttu**, buz altında akıntı vardı."*
> LORE §6: *"Daha derinde: **gölün buzu kırıldığında Tuğrul'un yakası elindeydi.**"*
> Kart 10: *"Dokuz yıl önce buz kırıldığında Gökçe buzun üstüne **hiç çıkmadı. Kıyıda kaldı.**"*

İki cümle de LORE'da **anlatı** olarak duruyor — atfedilmemiş. §6'daki cümle üstelik "kimseye söylemediği" başlığı altında, yani karakterin **iç gerçeği** olarak yazılmış. Bu, projeyi üç kez ısırmış olan kuralın ("kanon değişirse ona dayanan dosyalar aynı commit'te güncellenir") dördüncü ihlali.

**Düzeltme:** LORE §8'in "Nasıl" bloğu iki parçaya bölünsün: **olan** ve **anlatılan**, kanıt satırıyla (Karduvar nöbet taşı + Ak Nöbet defteri). §6'daki cümle *"buzun üstüne hiç çıkmadı — ve dokuz yıldır tersini anlatıyor"* olsun.

> **DURUM: UYGULANDI.** §8 raporun yazılmasından önce, §6 raporun ardından. İkisi de kanıt satırını ve "her seferinde aynı kelimelerle" izini taşıyor.

## KR2 · Kırca Ana yalanı çürüten belgeyi kendi elinde tutuyor — LORE §8 bunu zaten yazmış

**Nerede:** LORE §8 (Kırca Ana maddesi) vs kart 10.

> LORE §8: *"**Kırca Ana için:** şehri suçlanıyor, oysa buzu sınayan adam hâlâ hayatta ve **o sabahın raporu duruyor**."*
> Kart 10: *"O sabahın adları Ak Nöbet defterine geçmiş… **Gökçe yok.**"* · *"…**on sekizinde Kırca Ana'ya kendisi anlattı**… dokuz yıl mektuplaştılar."*

Yalanın en muhtemel çürütücüsü, yalanın **anlatıldığı kişi** — ve o kadın belgeyi şehrinin savunması olarak saklıyor. Dokuz yıllık mektuplaşma, üç yıl önce baş başa yazılan taç şartları ve LORE §6'nın "mektuplar bu yüzden bu kadar dürüst" cümlesi bu boşluğun üstünde duruyor.

**Düzeltme (öneri (a)):** **(a) Kırca Ana biliyor ve susuyor.** LORE §6'ya: *"Buz raporunu okudu; kızın adı orada yok. Bunu Gökçe'ye hiç sormadı — sormadığı için istediği her şeyi isteyebiliyor."* Bu, "gerekirse yine rehin verir" satırını tehditten kaldıraca çevirir. **(b)** Defter yalnız buza **inenleri** yazıyor — ama bu kanıtı öldürür.

> **DURUM: (a) UYGULANDI**, LORE §6'ya işlendi.

## KR3 · Şirin'in eyerli atı artık onun kurmadığı ertelemeye bağlı

**Nerede:** LORE §6 (Şirin / "Oyuncu nasıl fark eder") vs kart 3 §4 ve kart 4 §4.

> LORE §6: *"'Bey rahatsız, Altı Ad haftaya' haberi salonda okunduğu sabah… atı çoktan eyerlenmiş bulur… **Hastalık sabah çıkar; o at gece hazırlanmış.**"*
> Kart 3 (Sadık): *"Üç ertelemenin sebeplerini o kurdu… **Dördüncüsünü kurmadı** — 'beyimizin sağlığı' bu sabah yazıldı ve **arkasında gerçekten hekimin raporu var**."*
> Kart 4 (Şirin), aynı turda: *"erteleme haberi sabah okunduğu gün ahırdaki at **gece eyerlenmiş** duruyor."*

K1'in çözümü oyunun **tek dedektiflik kanıtını** sahipsiz bıraktı: hastalık gerçekse, gece eyerlenmiş at Şirin'in izi olamaz. Kart 4 kanıtı korumuş, kart 3 zeminini yıkmış; ikisi aynı turda yazıldı.

**Düzeltme (öneri (a)):** **(a) Dördüncü sebep de Şirin'in** — raporu **Yusuf'tan** aldı (adamın bütün doktrini "hastanın iradesi hekimindir", yani o rapor bedava). Sadık'ın korkusu sertleşir: kadın bu kez **onu atladı**. At kalır, LORE §6 kalır. **(b)** İlan cümlesi *"Kırk çıkmadı, Altı Ad haftaya"* olsun — o zaman kart 4'ün "hastalık sabah çıktı" satırı silinmeli.

## KR4 · İlk yirmi dakikanın çizelgesi motor saatinde imkânsız — ve bu revizyon onu taşıyıcı yaptı

**Nerede:** `kadro-yenitac-slotlar.md` (D) çizelgesi ve bütün slotların "MEKÂN + SAAT" satırları vs `src/gametime.h`.

> `gametime.h:13-14`: `#define GAMETIME_DAY_SECONDS 1200.0f  // 20 dk = 24 saat` · `#define GAMETIME_START_HOUR 8.0f`

Bir gerçek dakika = **1,2 oyun saati.** Oyun 08:00'de başlıyor, yani "ilk yirmi dakika" tam bir oyun günü ve minute 20'de yine 08:00. Gerçek eşleme:

| Dakika | Oyun saati | Çizelgenin istediği | O saatte gerçekte olan |
|---|---|---|---|
| 3 | 11:36 | kalfanın 06-08 AmbientHook'u | pencere kapalı |
| 5 | 14:00 | çocuk iskele ayağında (06/15 durakları) | çocuk **çarşı arkasında** (10 durağı) |
| 7 | 16:24 | salonda beş ağız + sayman | Yusuf revirde, Şirin taşlıkta, Rüstem kışlada |
| 9 | 18:48 | seyis mahzen kapağında, kızak gelir | seyis ahırda (14 durağı), kapak kapalı |
| 15-18 | 02:00-05:36 | Sabri mahzende kandil tutuyor | Sabri sandalyenin yanında |
| 19 | 06:48 | kâtip panoda parayı sayar | kâtip `indoors` (20 durağı) |

Bu sorun eskiden **yoktu**: vinyet PNG paneliydi, salon dünyada yoktu, rutinler bu dakikalara asılmamıştı. Revizyon salonu canlı harita yaptı, vinyeti canlı mekâna aldı ve Sabri'nin durağını *"15-18. dakikanın ışığı onun elinde"* diye açıkça dakikaya bağladı. Tek iyi haber: vinyetin altı kişilik salon kadrosu **yalnız 08:00-09:00'da** toplanıyor — yani açılış saati doğru, sonrası kayıyor.

**Düzeltme:** İki dürüst yol var. **(a) Açılış günü saat ölçeği:** ilk gün için `GAMETIME_DAY_SECONDS`'a bir çarpan (ör. 3600s = 60 dk/gün) → 20 gerçek dakika ≈ 8 oyun saati, çizelge 08:00-16:00'a oturur. Tek sabit, save'e girmez. **(b) Çizelgeyi saate çevir:** (D)'nin sütunu "dakika" değil "oyun saati" olsun, GDD §1.2d'nin dili de öyle yazılsın, 23 rutin yeniden dizilsin. Hangisi olursa olsun **karar GDD §1.2d'ye yazılmalı.**

## KR5 · Beşinci dakikanın çocuğu: slot belgesi heceliyor, kart okuma bilmiyor

**Nerede:** slot 21 + (D) madde 3 vs kart 11 + REVİZYON NOTU.

> Slot 21: *"kız 'Kum-ka-pı'yı **heceler**, gerisini ona sorar."*
> Kart 11: *"**Okuması yok**… ilanı tellal bağırıyor, o dinliyor, ezberliyor."* · *"**Bir kâğıdı eline alıp bakmaz.**"*
> Kart notu: *"Ö1'in 'kervan ilanını hecelesin' önerisi **reddedildi**."*

Aynı turda revize edilen iki belge, GDD §1.2d'nin [ONAYLI] ilk seçim vuruşu hakkında zıt karar veriyor. Bir de yer tutmuyor: pano **kışlada**, tellal 09-12 **meydanda**, çocuk 10'da **çarşı arkasında** — üçü hiçbir saatte aynı yerde değil.

**Düzeltme:** Kart kazanır (klişe gerekçesi sağlam). Slot 21'den heceleme sahnesi silinsin, slot 11'in satırı *"hece barkı yok; ilanı tellaldan duyuyor"* olsun, (D)'nin 3. maddesi yeniden yazılsın.

## KR6 · Mareşalin maaş defterini "kimse okuyamaz" — ama kışla kâtibi defterin tamamını ezberlemiş

> Kart 2: *"Maaş defteri **kendi el yazısıyla** tutuluyor… Ölürse **o defteri kimse okuyamaz.**"*
> Kart 13 (Ragıp): *"**Defterin tamamı ezberinde.** Kırk iki kişinin adını, kalemini, hangi ay hangi gün geldiğini biliyor."*

LORE §6 yalnız *"maaş defterini kendi **imzalamak** istiyor"* diyor — kanon Ragıp'ı destekliyor. Revizyonun koruduğu beş "kurum çöker" iddiasından mareşalinki **çürük**, ve çürüten şey aynı turda yazılan öbür kart. Aynı yerde ikinci çatlak: Rüstem *"otuz iki isim"*, Ragıp *"kırk iki kişi"* ve *"kuyrukta on üç kişi"*.

**Düzeltme:** Kart 2'nin KAYBOLURSA'sı el yazısını bırakıp **iki mürekkebe** dayansın: *"Defteri Ragıp yazar, Rüstem imzalar. Ölürse imza gider — ve imzasız defterde 'hazine' yazan kalemlerin bir kısmının hazineden gelmediğini artık kimse gizleyemez."* Sayılar tek satırda ayrılsın (32 bugün gelen, 42 defterde kayıtlı, 13 sıradaki).

## KR7 · Altı ismin taşı: vinyet salonda, slot belgesi meydanda

> Vinyet panel 3 görsel notu: *"kâtibin defterinde… kenarda kurumuş keskisiyle **taş ustası ve hiç oyulmamış boş taş**"* + *"**Avluda tellal**"*
> Slot belgesi: *"### **MEYDAN** — ilanın ve **boş taşın** olduğu yer"*

Slot belgesi *"ilk üç paneli salonda"* diyor ve bunu tek harita geçişi olarak fiyatlıyor. Ama panel 3 kâtibin masasını (MAP_SALON) taş ustası + boş taş (meydan) + tellal ile aynı kareye koyuyor, ve tellalın rutininde salon avlusu **hiç yok**. LORE §7 taşın yerini söylemiyor; boşluğu iki dosya iki ayrı doldurdu.

**Düzeltme:** Karar LORE §7'ye yazılsın. Öneri: **taş meydanda kalsın** (slot 22-23'ün kavgası ona bağlı ve bedava), vinyet panel 3'ün görsel notundan taş ustası + boş taş çıkarılsın, replik panel 4'e taşınsın. O zaman vinyet gerçekten iki haritada geçer ve (A)'daki maliyet doğru olur.

## KR8 · Yedinci Ağız'ın kapısı değişti, LORE §10 hâlâ eski kapıyı yazıyor

> LORE §10: *"**altı ağzın hiçbirine sormadan**… sadece ***'sen de bakıyorsun'*** der."*
> Kart 6 §8: kapı = kaymanın düzeltilmesi · Salon kartı kuralı 3: *"sayaç **beş ağızdır**"*

Üç ayrı kanon cümlesi geçersiz: sayaç (altı→beş), kapının fiili, ve LORE'un tırnak içinde verdiği tek tanık repliği. §7'nin "gerçek işaretleri bilen tek kişi" cümlesi artık yalnızca yarısı doğru.

**Düzeltme:** LORE §10 iki cümle alsın (beş ağız + kapı tepsi değil, devralma). §7'nin Sabri hücresi: *"Gerçek işaretleri bilen tek kişi — ve bir tanesini iki yıldır yanlış biliyor."*

> **DURUM: UYGULANDI**, rapor yazılmadan önce. LORE §10 beş ağız sayacını, yeni kapıyı, üç izlenebilir kanalı ve kirlenmiş oyuncunun saptırılmasını taşıyor. §7'nin Sabri hücresi hâlâ borç.

## KR9 · "sikke" iki yerde elle tutulan bir madeni para, (F) ise "elde taşınmaz" diyor

> (F): *"**sikke — devlet ölçeği, yüz kese.** **Elde taşınmaz, masada durur.**"*
> LORE §4: *"O günden beri **kendi sikkesini basıyor**."* · Kart 19: *"ara sıra **bir güney sikkesi** görünüyor… *'Ben ağırlığa bakarım.'*"*

Merdiven (sikke = 100 kese = 100.000 akçe) bir madeni parayı imkânsız kılıyor: Yenitaç'ın kırk günlük tüm gideri kırk kese, yani bir sikke şehrin iki buçuk yılı eder.

**Düzeltme:** Merdiven kalsın, **iki kullanım ayrılsın.** (i) LORE §4: *"kendi ağırlığını, kendi damgasını basıyor; hesabını sikke ile tutuyor"*. (ii) Kart 19'daki nesne sikke olmasın: *"güney damgalı bir gümüş külçe — ağırlığı bir kese eder"*.

---

# ÖNEMLİ

## Ö-A · Sabri'nin dördüncü durağı iki belgede iki saat; Ö4 mekanik olarak kapanmadı
Slot 6: *"…15 mahzen · **20** sandalyenin yanı"* · Kart 6: *"…15 mahzen · **19** sandalyenin yanı… **20.00'de birlikte içeri girerler.**"*

`NPC_MAX_STOPS 4` ve `indoors` bir durak **alanı**. Sabri'nin dört durağının hiçbiri `indoors` değil, yani adam **hiç içeri girmiyor** — Sarp `20 indoors` ile kaybolmuşken boş sandalyenin yanında dikiliyor. Kartın "birlikte içeri girerler" cümlesi beşinci durak istiyor.
**Düzeltme:** Tek saat seçilsin (**19**) ve dördüncü durak `indoors = true` olsun; olmuyorsa mahzen durağı **sahneye** alınsın ve rutin üç durağa insin.

## Ö-B · Sarp yürüyor: dört durak, tek harita, sağ tarafı ölü bir adam
Slot 1 *"hiç yürümez"* diyor ama `npc_update` durak değişince NPC'yi hedefe **yürütüyor** ve avlu ile taht bölümü artık aynı haritada. (A)'nın ışınlanma çözümü burada işlemez — harita değişmiyor.
**Düzeltme:** Sarp'a **tek durak**; avlu sahnesi `npc_walk_to` ile Sabri üstünden oynanan bir sahne olsun. Ya da `NpcDef`'e bir `carried` bayrağı — bir bool, ve Sabri'nin bütün işi ona bağlı.

## Ö-C · Buzun aritmetiği tutmuyor — ve K4'ün keşif zinciri o aritmetiğe asılı
Kart 2 *"iki çuvala düştü, üç gerekiyor… haftada iki kez"* · Kart 15 *"haftada iki çuval"* ama barkı *"Buz iki çuval. **Dün üçtü.**"* Aynı kartın içinde tahsis hem haftalık hem günlük ölçekte. Haftalıksa oyuncu sayamaz ve kart 2'nin keşif yolu ölür.
**Düzeltme:** Ölçek **günlük** olsun (*"gün aşırı iki çuval, eskiden üçtü"*); Rüstem'in farkı haftada iki kez birer çuval, kapağa haftada altı ve aşağıda sekiz çuval iner, sayım yapılabilir hâle gelir.

## Ö-D · Slot 20'nin "beratla yazılı payı" LORE §2/§11'in berat kademesini bulandırıyor
LORE "berat"ı oyuncunun **üçüncü mülk kademesine** kilitlemiş. Bir bac tahsildarının gelir payını berat demek o kademeyi memur maaşıyla aynı kelimeye sokuyor.
**Düzeltme:** *"eski bir **temessükle**"* ya da *"kapı defterinde eski bir kayıtla"*. (Slot 20'nin §11-1 kullanımı — İzinli = bac muafiyeti — **doğru**; ona dokunulmasın.)

## Ö-E · "Yapı gereği kesilemeyen tek adam" ile Vedia'nın beş haftası birbirini yiyor
Bacın kaynağı kapıdan giren araba; arabaların yolu Rüstem'in adamlarının keseceği tahıl yolu. Memnun adamın güvencesi kadronun en büyük tehdidinin tam hedefinde — bedava bir ironi, kimse yazmamış.
**Düzeltme:** Slot 20'ye: *"Kesilemeyeceğini sanıyor. Bacı kesecek tek şey yolun kesilmesi, ve yolu kesecek adamların oğlu bu şehirde çalışıyor."*

## Ö-F · Gassalın toprağı: GDD §8.2 mezarlığı köye yazmış
Mezarlık kanonda ve kodda köyün; gassalın şikâyeti Yenitaç'ın toprağına dair. Önek kuralı da kendini bozuyor: köyün mezarlığında çalışan adam `yenitac_` alıyor.
**Düzeltme (iki iş bir taşla):** Mezarlık Yenitaç'ın kendi mezarlığı olsun ve zemini kartta hazır malzemeden gelsin — **Sabri'nin yanmış kasabası Yenitaç'ın altında.** Kırk üç mezar, adı olmayan bir kasabanın toprağının üstünde, ve o toprak hiç kutsanmadı. Kart 6'nın *"kendi çocukluğunun altında yürüyor"* satırıyla bedavaya birleşir.

## Ö-G · Gassalın istediği kâğıdın reddedilme sebebi yazılmamış
Oyunun ilk saatinin duygusal çekirdeği gömülmeyen çocuk; şimdi bir adam *"bir satır yeter"* diyor ve tapınağın niye yazmadığı hiçbir yerde yok.
**Düzeltme:** LORE §3'e: *"Tapınak toprağı da kutsamıyor — çünkü kutsanmamış bir kralın şehrinin toprağını kutsamak o krala imza atmaktır ve Yedisunak'ın tek şartı fetih tamamlanmadan hiçbir şeye imza atmamak."* Hem gassal, hem Rüstem'in şartı, hem LORE §3 aynı gerekçeye oturur ve oyuncunun kozu gerçek bir siyasi hamle olur.

## Ö-H · Kesedarın altı bin akçesi iskele borcunu satın alamaz
Kart 19 barkı *"Altı bin iki yüz"* (6,2 kese); şehrin kırk günlük gideri kırk kese. On bir yıllık bir şantiyenin borcu yüzlerce kese.
**Düzeltme:** *"Satın alacak parası yok — **masaya götürecek** kalemi arıyor… iskelenin borcu Yedinci Kese'nin parasıyla alınır ve o parayı ona ancak Kese Sahibi olursa verirler."* Bu "otuz yıldır Yazman"ı da açıklar.

## Ö-İ · Rüstem'in kartı, oğlunun son anını bir kez sorduğunu bilmiyor
Kadronun en yüklü tek ilişki gerçeği yalnız Gökçe'nin kartında yazılı, ve bu Rüstem'in bütün "Kaç?" karakterinin tek istisnası.
**Düzeltme:** Kart 2 §9'a: *"Gökçe'ye bir kere soru sordu, dokuz yıl önce, ve cevabı defterden almadı. Aldığı cevabı bir daha sınamadı — sınamamak onun tek zaafı ve kendisi bunu tedbir sayıyor."*

## Ö-J · Üç slotun ilk-yirmi-dakika vuruşu kendi durak saatiyle çelişiyor (KR4'ten bağımsız)
Gökçe 12'de salon kenarında ama çizelge 12-13'te yirmi beş metrede istiyor · gassal 12'de meydanda ama çizelge 11'de · bacdar 12'de gölgede ama çizelge 10-11'de.
**Düzeltme:** En ucuzu duraklar: Gökçe'nin ikinci durağı 14'e, gassalın meydanı 11'e, bacdarın gölgesi 10'a. (KR4 çözülürken tümü elden geçecek.)

## Ö-K · "Yedinci Sandalye" iki kadına birden verilmiş, ve iki belge formu farklı yazıyor
Havuz aynı sandalyeyi Hafize'ye ve Nefise'ye veriyor; havuz N4'ü "kabul edildi" ilan ediyor, kart "sahibe kaldı" diyor; ve LORE §10'un merdiveni sandalyeyi bir **rütbe** yapıyor (*"Kırkbirinci Sandalye"*) — Hafize yalnızca Yazman, yani sandalyesi olamaz.
**Düzeltme:** (iii) tartışmayı bitiriyor: **"Kese" kalsın** (hane işareti), sandalye rütbe kalsın. Havuz §2.4'ün N4 kararı geri alınsın; Nefise örneği *"Ondokuzuncu Kese'den"* olsun.

## Ö-L · Ö13 yarım kapandı: üç §3 listesi hâlâ kendi kuralını işaretsiz çiğniyor, sicil eksik

| Liste | Kural | İşaretsiz ihlal |
|---|---|---|
| KUMKAPI | *"sonu -ar/-an/-al/-uk/-ak"* | **Yedek** (-ek), **Havut** (-ut), **Serap** (-ap), **Zülal/Gülnar** (ü) |
| KARAOCAK | *"s/ş/h ile başlamaz"* | **Soğuk Pota** |
| KARDUVAR | *"kısa, iki hece"* | **Torunca** (üç hece; ayrıca `cayir_koy_torun` rolüyle çakışma) |
| ÇAYIR | *"Ad yılın adıdır."* | **Anbar · Evlek · Kesek · Petek** yıl adı değil |

Ve §5.7 sicili kendi kuralını tutmuyor: **Yolak, Muazzez, Vesile, Yünce, Yelce** hem sicilde hem §3 serbest listelerinde. Havuz **denetim aracı olarak hâlâ güvenilir değil** — ama v1'e göre çok daha yakın.
**Düzeltme:** Dört ihlal işaretlensin, sicile giren beş ad §3 listelerinden silinsin. Tek oturumluk temizlik.

## Ö-M · "Yamak" iskele ırgatına verilmiş — havuzun ve LORE §10'un ortak yasağı
Slot 9: *"yerine **yamak** geçer"* · Havuz §2.2: *"**Yamak** ırgat için kullanılamaz — Kılıçartıkları'nın ilk rütbesi"* · Kart 14: *"**Yamak** Uçkun"*
**Düzeltme:** *"yerine **ırgatlardan biri** geçer"* — devrolan ad detayı bedava: *"yerine Çat geçer, ve gelecek yıl başka biri Çat olur."*

## Ö-N · Slot 12'nin koruma mekanizması Rüstem'in kuralından çıkmıyor
Kural, sözü **hakkında konuşulan kişinin** yüzüne söyletiyor — yani makaracıya, ki bu kolay. Mareşale gitme zorunluluğu kuraldan türemiyor; üstelik makaracı iskele işçisi, kışla defterinde değil.
**Düzeltme:** Korumayı gerçek zincire bağlayın: *"Kalfa onu kışla kâtibine yazdıramıyor — çocuk iskele yevmiyesinde. Yevmiyeyi kesecek adam sayman, sayman kalem kapatmıyor. Ve babası hâlâ maaş kuyruğunda: adı defterden düşen ilk isim olmasın diye kalfa dört aydır erteliyor."*

## Ö-O · "On bir yalnız iskele, boş taş ve Rasih'te" — doğru değil
Salon kartının revizyon notu kendi uygulamasını yanlış rapor ediyor: Toygar'ın "on bir ay"ı ve havuz §4'ün "Kışlık" lakabı (*"on bir yıldır"*) duruyor.
**Düzeltme:** Toygar'ın sayısı **on üç** olsun; "Kışlık" **altı yıldır** olsun; revizyon notunun cümlesi düzeltilsin.

Aynı yerde: **"kırk" monokültürü ağırlaştı.** Kırk yıl savaş · Kırk Kese · kırk gümüş · kırk gün · kırk kese · Bekmez'in kırk kasesi · Baran'ın kırk adamı · Ragıp'ın kırk iki kişisi · gassalın kırk üç mezarı · kırk sandalye · Kırkbirinci Sandalye. Ö12'nin üçüncü ayağı hiç ele alınmadı. **Öneri:** gassal **otuz bir** gömsün, Bekmez **otuz altı** kase kursun (barkında zaten 36 var), Baran'ın adam sayısı **otuz iki** olsun.

## Ö-P · Çivi söken çocuğun 21:00 durağı kimliğini rutinle sızdırıyor
Şirin'in odası MAP_SALON'da, yani çocuk her akşam saray kapısına yürüyor. Gizliliği bozan şey bir ağız değil, rutin. Ayrıca (A)'nın "dört slot tek alanla kurtuluyor" sayımı **beşe** çıkıyor.
**Düzeltme:** Gece durağı iskele altındaki bir barınak olsun; "yatağı hâlâ annesinin odasında" bilgisi Şirin'in kartında kalsın ve oyuncuya sarayın içinden gelsin.

## Ö-Q · 14. dakikanın ezilmiş eli sahipsiz — ve o yatakta Toygar var
Ezilen elin sahibi 23 slotun hiçbiri değil; `npc_render_infirmary` yalnız NPC kayıtlarını çiziyor. Aynı saatte revirde **Toygar** var (eli sağlam) — oyuncunun göreceği tek yatan adam o.
**Düzeltme:** (i) ezilen adam bir NPC kaydı olsun (24. slot), ya da (ii) o yatak dekor olsun ve vuruş Toygar'ın yanındaki **boş yatak + duvarda asılı sargı**yla anlatılsın. (ii) sıfır kod istiyor.

---

# NOT

**N-1 · Kurak/Sulak havuzun kafiye kuralını çiğniyor, işaretsiz.** İstisna kasıtlı ve iyi ama yazılmamış; §0 işaretsiz ihlali hata sayıyor. Tek satır: *"Yıl adları kafiyelidir; köyün usulü tam olarak bu."* **Kumral** da Kurak'a yakın ve bir yol adı, yıl adı değil.

**N-2 · Dirlik bir Osmanlı maliye terimi, Petek işini söylüyor.** İkisi de savunulabilir ama gerekçeleri havuzda **yasak maddelerine referansla** yazılmalı, yoksa sonraki tur emsal sayar.

**N-3 · Cevriye: kaynağı olmayan ad, kapanmayan iplik, LORE §16'nın ikizi.** Kaynak *"§2.10 kadın adı"* gösteriliyor ama v2'nin §2.10'unda ad listesi yok. Daha önemlisi LORE §16'da *"ikinci isim taşla taşa vurulup silinmiş"* bir veraset taşı var; şimdi ikinci bir taş-üstünde-gizli-isim aracı doğdu. **Öneri:** nesne farklılaşsın (ad değil **tarih** oyulmuş bir taş), ya da LORE §17'ye *"bilerek açık; kimse çözmeyecek"* yazılsın.

**N-4 · Toygar/Tuğrul savunmasının dayanağı yanlış.** Havuz *"Tuğrul adı Yenitaç'ta hiç söylenmiyor"* diyor ama Rasih'in taşında bir çocuğun adı var, tepside Tuğrul'un çizmesi var. Mesafe gerçekten yeterli — **ad kalsın**, gerekçe *"mesafe dört sesbirim"* olarak yazılsın. Yanlış gerekçe yanlış emsal olur.

**N-5 · Sabri'nin "hayır düğümü" eski sürümden kalmış.** Kart 6 §4 düğümleri *"bey birine baktığında"* atıyor, §8 *"bir daha 'hayır' düğümü yok"* diyor. Cümle *"Sicim aynı kalır; bir düğüm eksilir — sabaha karşı attığı."* olsun.

**N-6 · Çatısız salonda arşiv ve defter.** Panel 1 *"açık gökten inen yağmur"* diyor, kart 3'te 11:00'de karıştırılabilir kâğıt demeti ve yarı yanmış yaprak var — oyunun tek yazılı kanıtı. Bir satır: *"Salonun **üstü açık, yanları kapalı**; kasa odası, arşiv nişi ve yazı masası **örtülü** yan hücrelerdir. Yağmurda kâtip demeti içeri alır — bunu oyuncu bir kez görür."*

**N-7 · Avlu/oda ayrımı LORE §7'yi yarım kurtarıyor.** Üstü açık bir odanın yanındaki avludaki gaziler yine duyar. Tek bağlayıcı satır: *"Oturum kapı kapalı yapılır ve avlu boşaltılır; Sarp avluya yalnız oturum olmayan saatte çıkarılır."* Rutinle zaten uyumlu.

**N-8 · Toygar: "üç aydır" mı "dört aydır" mı.** Kart 8 ikisini de kullanıyor, kart 13 "dört aydır" diyor. Tek sayıya inin (dört).

**N-9 · Erteleme sayacının temposu yazılmamış.** On bir yılda dört tarih, üçüncü ile dördüncü arasında iki yıl — ama sayaç "dünya durumunun sesli sayacı" ve oyun içinde beşinciye ilerliyor. Yüz saatlik bir oyunda iki yılda bir kıpırdayan ilerleme çubuğu kıpırdamaz. *"Felçten sonra tempo değişti: ilk üç tarih dokuz yıla yayıldı, dördüncüsü iki yıl bekledi, beşincisi kırk gün sonra."*

**N-10 · Gökçe'nin ilk sesi aksansız.** Kanonun en görünür karakter işareti (LORE §6: *"aksanı oralı ve düzeltmeye çalışmıyor"*) oyuncunun onu ilk duyduğu cümlede yok. Bedava düzeltme: *"…ipin yediği yerden akıyor, **he**!"* — bir hece, ve 12. dakikada yüzü göründüğünde oyuncu sesi tanır.

**N-11 · Salon kartının 4. borcu ödenmedi.** *"Şirin'in asıl sırrını Nazlı da biliyor olabilir"* — kart 11 bu konuda tek kelime etmiyor. Bilirse çocuğun üç randevuya gitmemesi bir **tercih** olur ve kart 11'in "haklı da olabilir"i sertleşir.

**N-12 · (E)'deki iki sayım eskimiş.** Şehir kartlarında KAYBOLURSA bölümü hiç yok (bilinçli), yani kesedar listede olmamalı; sayı dörde iner. Mareşalinki KR6 ile düşüyor → üçe iner.

**N-13 · Ö11 kısmen kapandı.** "Asıl sır" gerçekten adlardan taşındı (kepçe, iki mürekkep, sicim, sedir) — bu iyi. Ama ad hikâyesi motifi hâlâ yirmi kartın on birinde kimlik rengi olarak duruyor. Havuzun mimarisi bunu **garanti ediyor**, yani sorun havuzun kendisinde. **Öneri:** motifi **replikte seslendirmek** en fazla dört kartta olsun (bugün beş-altı). Gerisi kartta kalsın, ağızda geçmesin.

---

# KAPANDI

**K1** (§8 ve §6, uygulandı) · **K2** (kesedar yevmiye ödemiyor; ölçek Ö-H) · **K3** (`MAP_SALON`+`MAP_MAHZEN`, `save.c:490` enum-sonu güvenliği **doğrulandı**) · **K4** (buz gizli; aritmetik Ö-C) · **K5** (kalemi Şirin yazdırdı) · **K6** (Kıraç→Ayaz, örnek uygulama) · **K7** (iskele emirsiz) · **K8** (yedi ad karara bağlandı + köyün usulü yazıldı) · **Ö2** (çuvallar kapağın ağzında) · **Ö3** (para merdiveni; `item.c` karşılaştırması **doğrulandı**) · **Ö5** (sefer, iki kart kilitli) · **Ö6** (Gökçe sızıntıyı çiziyor) · **Ö7** (sur/palanka kuşağı, Sekiz'in yaşı tutuyor) · **Ö8** (kalfa on bir yıl, özne "defter") · **Ö9** (beş ağız + kapı saptırılıyor) · **Ö10** (mahzen Akkemer su yolu, mühür değil) · **Ö12-dokuz** (yalnız Tuğrul hattında) · **Ö14** (söylemiyorlar, dört sebep) · **Ö15** (seyis↔mareşal; yeni asimetri Ö-İ) · **Ö16** (Sadık'ın kelimesi "Yarım.", yedinci çivi kesildi) · **Ö17** (Kavurgan geri çekildi) · **Ö18** (Taş'ın siyasi barkı) · **Ö19** (Ela `cayir_koy_otaci`, GDD §1.2c-7 karşılandı) · **Ö20** (vinyet sayımı dosyayla doğrulandı: 12 satır, 10 ağızdan, yedisi bark) · **N1** (Rüstem 55, felç kışına oturdu) · **N2, N3, N5, N6, N7, N9, N10, N12, N13, N14, N15** (hepsi uygulandı) · **N11** (sayımı reddetmesi **haklıydı** — gerçekten dokuz çağrı var, sekizi etkileniyor, toplam **10** doğru).

**(E)'nin iki kod düzeltmesi doğru.** `contract_accept` gerçekten kullanılmıyor (`contract.c:88-90` gerekçesiyle duruyor); `contract_on_kill` kabul kontrolü yapmıyor. `npc_render_infirmary` yatakları yan yana diziyor (`npc.c:655`). İkisinin yerine yazılan iki gerçek borç da doğru: `ContractDef` yalnız `targetMob`/`fetchItem` ile kapanıyor, `contract_today` her gün rastgele seçiyor.

**Vinyet dosyası güncellendi** — salon kartının 1. borcu ödenmiş.

**Kapanmayan/açık:** N4 (Kese↔Sandalye, Ö-K) · Ö11 (kısmen, N-13) · Ö13 (kısmen, Ö-L) · Ö1 (kısmen: slot 21 hâlâ heceliyor, KR5) · Ö4 (anlatıda kapandı, veride kapanmadı, Ö-A) · Yenitaç'ın büyücü açığı (bilinçli).

---

# GDD/LORE BORCU

**`docs/LORE.md`**
1. **§3** — tapınağın toprağı da kutsamama gerekçesi (Ö-G). *En yüksek değerli tek satır: gassalı, Rüstem'in şartını ve oyuncunun kozunu aynı gerekçeye bağlıyor.*
2. **§4 Yenitaç** — salon mekân tanımı (üstü açık, yanları duvarlı, örtülü yan hücreler) + mahzen Akkemer su yolu + bir tarla ötesi tahıl köyü (K3, Ö-F, N-6)
3. **§4 Karaocak** — "kendi sikkesini basıyor" → sikke tanımına uygun yeniden yazım (KR9)
4. **§2** — akçe/kese/sikke üç satır + "kırk gümüş birim değil usul" · **berat** satırına *"yalnız kule/kale tahsisi"* (Ö-D)
5. **§6 Şirin** — dördüncü erteleme kararı (KR3-a raporu Yusuf'tan aldı, ya da KR3-b) + tempo satırı (N-9)
6. **§6 Rüstem** — *"defteri Ragıp yazar, o imzalar"* (KR6)
7. **§7** — altı ağız tablosu: Şirin ve **Sabri** hücreleri · oda fiziği (oturum kapalı, avlu boş) · altı ismin taşının **yeri** (KR7)
8. **§17** — Cevriye (N-3)

**`docs/GDD.md`**
9. **§1.2d** — **oyun saati faturası ve kararı** (KR4, en yüksek öncelikli borç) · *"5. dakika: çocuk okumaz"* (KR5) · salon/mahzen harita satırı
10. **§1.2c** — kural 4 phasing örneğine dipnot (Nazife→Kaymak, `.id` sabit) · kural 1'de Baran adının korunduğu bir kez yazılsın
11. **§4 ya da yeni §** — *"Tek para alanı `player.gold` = akçe; ikinci para birimi eklenmeyecek"*
12. **§8.1** — önek kanon yeridir · **[YENİ ALAN] `NpcStop.map`** (altı slot buna bağlı, Nazlı dahil) · **sabit NPC bayrağı** (Sarp, Ö-B)
13. **§8.2** — Yenitaç mezarlığı Yenitaç'ın sicilinde (Ö-F) · muhafız çoğullama cevabı GDD'ye taşınsın

**`assets/scenes/intro_demirboyun.txt`**
14. Panel 3'ten taş ustası + boş taş görsel notu çıkacak, replik panel 4'e (KR7)
15. Panel 1'in üst kat repliğine bir hece: *"…akıyor, he!"* (N-10)
