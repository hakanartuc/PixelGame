Kodu okudum, sayıları saydım. Aşağıdaki her sayı `src/` içinden çıktı; çıkmayanı işaretledim.

---

# ÜÇ TOPLAMA MESLEĞİ — envanter, anlam, ağ

## 1. BUGÜN ELDE NE VAR (grep'lenmiş)

`RESOURCES[]` tablosunda **36 düğüm** var (`src/resource.c:15-175`, `MAX_RESOURCES 64` → 28 boş slot). Dağılım şöyle: madencilik **22** düğüm (13 bakır, 9 demir), bitkicilik **9** (6 kızılot, 3 ayotu), oduculuk **5** meşe. Yani tablonun %61'i tek meslek. Oduculuk %14 alıyor — ve `craft.c`'de **odun dokuz tarifin girdisi** (külçe kılıçlar, yay, ok, asa, mızrak, tahta kalkan, kemik/demir ok), yani kıtlığı en çok hissedilen malzemenin dünyada en az düğümü var. Bu bir yerleşim hatası, kurgudan bağımsız düzeltilmeli; düzeltmesi de bedava, tabloya satır eklemek.

XP tek satırda kesilmiş: madencilik 8, oduculuk 6, bitkicilik 5 (`main.c:858-859`). Hız `gather_mult` ile basamaklı, her 20 seviyede +%25, tavan 2.25× (`main.c:782`). Alet şartı KUŞANILI olmak zorunda ve `WEAPON_TOOL` saldıramıyor (`item.h:46`) — kazma/balta taşıyan adam o an silahsızdır, bu tasarım için kıymetli bir gerçek. Bitkicilik aletsiz.

Skill kapısı fiilen **kullanılmıyor**: 36 düğümün 9'unda `minSkill = 15` (demir), kalan 27'sinde sıfır. `work` değerleri: bakır 5, demir 10, kızılot 2.5, ayotu 4, meşe 7.

Mini ağaçlar (`skilltree.c:396-425`) üç mesleğin **aynı ağacı**: kök + üç hız düğümü + üç çift-ürün düğümü + keystone (çift +%25, hız −%20). Kelimeler değişiyor, yapı birebir aynı. `TREE_MAX_NODES 32`, yani her ağaçta 24 boş düğüm yeri duruyor.

Hava/mevsim yalnız **bitkiciliğe** dokunuyor: dün yağdıysa ot ×2, karda yarıya iner (`main.c:844-852`). Madencilik ve oduculuk için hava diye bir şey yok, `gametime_season()` var ve toplama tarafında hiç çağrılmıyor. Bir ek eşitsizlik: XP parşömeni madencilik ve bitkicilik için var, **oduculuk için yok** (`item.c:160,164`).

İki sessiz gerçek daha: düğümler her sabah topluca yenileniyor (`resource_daily_respawn`), ve `save.c` içinde "resource"/"harvest" geçen tek satır yok — yani toplanmışlık **kayda girmiyor**, kaydı yükleyen oyuncu tüm düğümleri dolu bulur. Kalıcı düğüm durumu isteyen her tasarım yeni bir save bloğu demek.

## 2. ÜÇÜNÜN KANONDAKİ ANLAMI

**Madencilik** bir cesedin içini kazıyor ve bunu kimse ilan etmiyor. Kilnmoor'da galeri açan adam ilk gün "kaya" der, üçüncü hafta kazmanın sesinin bir yerde değiştiğini fark eder — o duvar yivli bazalt, o boşluk iki kaburganın arası. Ocakdoğanlar bin yıldır biliyor ve buna "ocak" diyorlar; kimse "ejderha" kelimesini kullanmıyor çünkü kullandığın gün madenin adı değişir ve fiyatı da değişir. Madencinin fiili basit kalıyor (kaz), öğrendiği şey basit kalmıyor: bu damar rastgele dağılmış değil, **anatomik** dağılmış. Cevher nerede çoksa orada bir organ vardı.

**Oduculuk** ısı satmıyor, **ölçü** satıyor. Odun ateşi ölüyü bastırmadığı için bu dünyada odunun tek değeri biçimi: fitil sapının boyu tutmalı (direğe girmezse menzil almaz), tabutun eni tutmalı, Pyrewright çukurunun odunu belli uzunlukta kesilir yoksa yığın çöker, galeri payandası budaksız gövdeden yapılır çünkü budak yerinden kırılır. Ve bir yan fayda: bu kıtada **odun apolitiktir**. Kor almak taraf tutmaktır, odun kesmek değil. Meyhaneci Ranulf'un tek mumla oturmasının sebebi bu.

**Bitkicilik** iki şey veriyor ve ikisi de tuhaf biçimde bilgi. Tuzotu tuz demek, tuz da yemek değil **sahtekârlık** demek: tuzlu suda kalmış kor yanlış perdeden çıtırdar, ışık verir ama ölüyü bastırmaz, ve Kızıl Kanca kıtaya bunu satıyor. Tuzun tadını bilen adam sahte koru elinde tanır. Taşkulağı ise ancak yüzyıllarca ad okunmuş taşta bitiyor — bir bitki olarak neredeyse anlamsız, bir sayaç olarak kıymetli. Rünün oyuğuna dolgu olarak giriyor (hece taşa böyle tutunuyor), ama asıl işlevi bu değil: **nerede bittiğini söylemesi.**

## 3. HER MESLEK BİR OKUMA BECERİSİ

Burası işin özü. Üçünün de bugün tek çıktısı sayı: +2 bakır, +1 ot. Sayının yanına bir **cümle** koyuyoruz — dünya hakkında, yalnız o mesleği yükseltenin görebildiği bir cümle.

### Madenci: duvarın yönü ve kazmanın sesi

Oyuncu bunu diyalogla öğrenmiyor, **sesle** öğreniyor. Bugün her toplama vuruşunda tek bir `SFX_HIT` çalıyor (`main.c:9044`). Düğüme bir ses alanı eklendiğinde galeri derinliğine göre vuruş sesi taştan tok bir şeye kayıyor — oyuncu hiçbir şey okumadan yirmi vuruşta "burası kaya değil" hissini alıyor. Cevabı sonra bir Ocakdoğan'dan tek cümleyle duyuyor: "Ocak sıcaktır, sen de içindesin."

Yön okuması görsel: cevher düğümünün sprite'ı damarın gittiği yönü gösteriyor (dört varyant, `ores.png` sütunları) ve aralarda **toplanamayan damar izi dekoru** duruyor. Madencilik seviyesi yükseldikçe bu izler daha uzağa kadar görünür hale gelir — oyuncunun kazandığı şey harita bilgisi: "üç düğüm bir doğru çiziyor, dördüncüsü şu kayanın arkasında." Bir gün kazanır, bu yeter.

Göçük riski bir düğüm bayrağı olarak duruyor: payandasız açılan derin düğüm bitişte zar atar, tutmazsa oyuncu hasar alır ve **o galeri o gün kapanır**. Kayıp gerçek olduğu için oyuncu payandaya para verir — ve payanda odundur. Ağın kapandığı yer burası.

Kalıcı iz: bir derin düğümü ilk açan oyuncunun adı payandaya yazılıyor ve o galeri artık geçilebilir bir yol; Kilnmoor'un mühürlü galerisi buradan açılıyor.

### Bitkici: yamacın nüfus sayımı

Bu üçünün en güzeli, çünkü hiç yeni sistem istemiyor. Taşkulağı **yalnız taşın dibinde** doğuyor: yol taşı, direk kaidesi, höyük taşı. Oyuncu bunu beş dakikada, tek bir gözlemle öğreniyor — bulduğu her tutam bir taşın yanında.

Sonra bir yamaç görüyor. Yamaçta taş yok, ama taşkulağı **kaplamış**. Kimse açıklamıyor. Oyuncunun kafasında soru kendisi kuruluyor: taş yoksa bu neden burada? Cevap: taş var, altında. Ve yoğunluk kaç yüzyıl ad okunduğunu söylüyor, yani kaç ölü. Üç tutam bir köy mezarlığı, on iki tutam Barrowfell.

Bu bilginin **dört ayrı alıcısı** var ve hepsi mevcut sistemlere oturuyor: Brannock (Pyrewright) yakma işi için para verir, bir Coldhearth atlısı ordu için para verir, bir Söndürücü sözleşme için para verir, köylüler ise para vermez — itibar verir. Aynı tutam ot, dört farklı dünya cevabı. Oyuncu hangisine sattığını hatırlar çünkü sonucu görür: sattığı yamaç ya kazılır, ya yakılır, ya kararır.

Bir de yaprak rengi: bu mevsim adı okunmuş taşın dibindeki taşkulağı diri yeşil, kırk yıl okunmamışın dibindeki soluk. `flowers.png` hücresi değiştirmek yetiyor. Oyuncu böylece bir bölgeye bakıp "bu köy törenini bırakmış" diyebiliyor. Hiç metin yok.

### Oduncu: kütükteki halkalar

Oduncunun okuduğu şey **zaman**. Kandilin altında büyümüş ağacın halkaları düzgünsüzdür — kor ısısı geceyi de büyüme mevsimi yapar, halkalar geniş ve tuhaf; kandil söndükten sonrakiler daralır ve düzene döner. Yani her kütük, üstündeki direğin kaç yıl yandığını ve kaç yıl önce söndüğünü yazıyor.

Fiil: ağacı kes, kalan kütüğe **E**. Ekranda tek satır: "Kırk halka geniş, yirmi bir halka dar." Oyuncu bunu ilk gördüğünde anlamıyor. İkinci kütükte ("altmış geniş, üç dar") anlıyor, ve o an elinde kıtanın en ucuz karartma takvimi var: hangi direğin kaç yıl önce söndüğünü, kimseye sormadan, hattın tamamı için çıkarabiliyor. Ana zincirin "sönük direkler bir hat çiziyor" düğümü (hikaye işleyişi §2, düğüm 4) burada oduncu için **erkenden** açılıyor — kesip sayan adam Coldhearth'ın planını üç saat önce görür.

İkinci okuma daha kaba ve hoş: devrilen ağacın kökünde taş çıkar. Bir yamaçtaki eğri gövdeler kökün altında bir şey olduğunu söyler; oduncu POI bulucusudur ve kazmayı bilmesi gerekmez.

Üçüncüsü zanaat: gövdeye bakıp düz mü budaklı mı olduğunu bilmek. Aynı ağaç bir oyuncuda fitil sapı, başka birinde çukur odunu çıkarıyor — düğüm varyantı, sıfır yeni fiil.

## 4. ZİNCİRLER, KİLİTLER, VE AĞIN KAPANDIĞI YER

Bugün kodda kesişme **bir tane** var ve kimse fark etmemiş: İp kamp ateşinde ayotundan yapılıyor ve demirci ocağındaki çanta tariflerine girdi oluyor (`craft.c:168-190`). Yani bitkicilik→ateş→demircilik hattı zaten var. Emsal duruyor, çoğaltalım.

Halka şöyle kapanıyor: **derin damar payanda ister** (madenci oduncuya muhtaç), **fitil sapı lif ister** — mevcut İp (oduncu bitkiciye muhtaç), **taşkulağı kurutma sıcak taş ister** ve sıcak taş kordur (bitkici madenciye muhtaç). Üç toplama mesleği üçgen kuruyor ve hiçbiri diğerinin kaynağını kendi başına üretemiyor. `ResourceDef`'e bir `reqItem/reqCount` alanı eklemek bunu taşır; onbeş satırlık iş.

Sonra her mesleğin **başkasına muhtaç** halkası — ve önemli olan şu: üçü **üç ayrı hizbe** muhtaç, yani üç meslek oyuncuyu üç farklı siyasete sokuyor.

Madenci ham koru soğutamıyor (kanon): Ocakdoğan çukuru ya da Külhan avlusu. Slake sunağa yakmayanla konuşmuyor, Kavus musluğu kesebiliyor. Oduncu ölçüsünü kendi damgalayamıyor — fitil sapı bir menzil damgası olmadan satılmıyor, kalibrasyon Fitilci ayar taşında yapılıyor. Yani balta sallayan adam bir gün Nell Harrow'a bez taşımak zorunda kalıyor. Bitkicinin taşkulağı yaş toplandığı gibi üç günde çürüyor; kurutma yalnız Pyrewright çukurunun kenarında ya da Ocakdoğan sunağında oluyor, ve Brannock baş sayısına para alan bir adam olarak ot kurutmaya da fiyat kesiyor.

Üç mesleğin üçünde de oyuncu tek başına döngüyü kapatamıyor. Ticaret ve itibar buradan doğuyor; kimse ona "ticaret yap" demiyor.

## 5. HAT — toplama bir oyun tarzı mı

Çıkıyor, ama sınıf olarak değil. **Hat, dünyanın oyuncuya taktığı ad.** Mekaniği bir bayrak, bir unvan, ve birkaç fiyat/kapı değişimi; kutu değil.

**Açıcı** — madencilik + oduculuk + demircilik. Kilnmoor argosu; çukur kurulu ona "ağız" der. İlan anı sahici: mühürlü galerinin ağzına ilk payandayı çakan adamın adı payandanın üstüne yazılır, ve o payanda oradan sonra dünyada durur. Ondan sonra Ocakdoğanlar sunak şartını ona uygulamaz, Ironkiln kuyruğa sokmaz.

**Tabutçu** — oduculuk + gizlilik + kilit. Tabut ölçü işi, ve boş tabutu bilen adam ölü posta kutusunu da bilir. Söndürücüler sandık ister, Fitilciler kapak ister. Suikastçinin marangoz olması bu evrende hiç garip değil, aksine en temiz kapağı: elinde balta taşımak bir suç değil.

**Yol Otçusu** — bitkicilik + simya + konuşma. Bir köyün taşkulağı yamacını haritalayıp fitil ustasına teslim eden kişi. Menzil evleri ona bedava yatak verir, çünkü sabah kimin kalkacağını söyleyen adamı kimse yolda bırakmaz.

Kural: hatta girmek skill eşiği **artı bir fiil**. Sadece seviye yükselterek unvan alınmıyor; bir kere o işi dünyada yapman gerekiyor. Ve hattın kapısı role bakmıyor — büyücü de payanda çakabilir, şövalye de tabut ölçebilir.

## 6. BEŞ ROL, KİMSENİN TEKELİ YOK

Savaşçı **oduculuk** öğreniyor çünkü Brannock çukurun ağzını tutacak adama gecelik dört gümüş veriyor ve çukur odunu belli boyda kesilmek zorunda; bir de kırılan direği yeniden dikmek gövde ister. Büyücü **bitkicilik** öğreniyor, rün oyuğunun dolgusu taşkulağı — sunak zaten kodda. Suikastçi ikisini de alıyor: ayotu Zehir Yağı'nın girdisi (`craft.c:82`, mevcut), tabut ise kutu. Tüccar **madencilik** öğreniyor, çünkü Ironkiln kiloyla tartıyor ve damarın kalitesini gözle kesen adam teraziyi yener. Şövalye **oduculuk** — kendi diktiği direğe kendi damgasını çakmak, bileziğin en dürüst hali.

Üçünün her biri en az üç role hizmet ediyor, hiçbiri tek rolün aleti değil.

## 7. KOD FATURASI

**Sıfır kod:** eşya/skill/fiil isimleri ve ağaç düğümü metinleri (`resource.c` verb, `skilltree.c` string'leri). Kızılot→**Tuzotu**, Ayotu→**Taşkulağı**, Odun→**Gövde** yalnız metin. Düğüm yerleşimini yeniden dağıtmak da sıfır kod: 28 boş slot var, 22/9/5 dengesizliği tabloya satır ekleyip çıkararak düzelir. `minSkill` alanı zaten duruyor, kullanılmıyor — kapı koymak veri.

**Küçük:** `ResourceDef`'e `reqItem/reqCount` (payanda kilidi) ~15 satır. Düğüm başına ses alanı, birkaç satır. Hava/mevsimi madencilik ve oduculuğa açmak `gather_finish` içinde dört satır (`main.c:844-852` deseninin kopyası; `gametime_season()` hazır, çağrılmıyor). Meslek ağaçlarına okuma düğümleri: her ağaçta 24 boş yer var, her yeni etki bir `PerkEffects` alanı + bir `if` satırı.

**Küçük-orta:** kütük etkileşimi (kesilen ağacın yerinde kalan, **E** alan bir nesne — dekor + etkileşim). Damar izi dekorlarının skille görünür olması.

**Orta / [YENİ MEKANİK]:** göçük olayı ve kalıcı "açılmış galeri" durumu. Buradaki gerçek maliyet kod değil kayıt: `save.c`'de toplama durumu **hiç tutulmuyor** (grep: dosyada "resource"/"harvest" yok), yani kalıcı düğüm durumu yeni bir save bloğu ve sürüm artışı demek.

**Eşya slot bütçesi:** `MAX_ITEM_TYPES 96`, tablo **92** dolu (`item.c`'de 92 girdi) → **4 slot**. İki mevcut otun adını değiştirerek yeni slot harcamadan taşkulağı ve tuzotunu alıyoruz. Dördü şöyle harcanır: **Payanda**, **Fitil Sapı**, **Kuru Taşkulağı**, ve biri yedek. Daha fazlası gerekirse tavanı 128'e çekmek tek satır — paralel diziler makroyla boyutlanıyor (`item_textures.c:7`, `player.h:184 itemCd[MAX_ITEM_TYPES]`), `save.c` doğrulaması `ITEM_TYPE_COUNT` üzerinden (`save.c:71, 399`), yani format değişmiyor. **Şart:** enum'a ekleme daima **sona** — kayıtlar eşyayı indeksle tutuyor.

**[DOĞRULANMADI]:** `ores.png` ve `flowers.png` içinde yön/renk varyantı olarak kullanılabilecek hücrelerin gerçekten var olup olmadığına bakmadım; sprite üretilebilir olduğu için tasarımı buna bağlamadım.

---

İlgili dosyalar: `/Users/hakanartuc/PixelGame/src/resource.c` (36 düğüm, tablo), `/Users/hakanartuc/PixelGame/src/main.c:782-878` (`gather_mult`, `gather_finish`), `/Users/hakanartuc/PixelGame/src/skilltree.c:392-425` ve `:977-1010` (üç aynı mini ağaç), `/Users/hakanartuc/PixelGame/src/craft.c:40-215` (tarifler; tek çapraz bağ satır 168-190), `/Users/hakanartuc/PixelGame/src/item.h:8,183-297` (tavan + enum), `/Users/hakanartuc/PixelGame/src/save.c` (toplama durumu yok).