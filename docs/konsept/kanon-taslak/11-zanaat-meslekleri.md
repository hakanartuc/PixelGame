Kodu ve dört kanon dosyasını okudum. Aşağıdakiler grep'le doğrulanmış sayıların üstüne kurulu; uydurduğum hiçbir mekanik yok, yeni olanları işaretledim.

# ÜÇ ZANAAT — AĞ, ANLAM, KARAR

## 1. ELİMDE NE VAR (doğrulandı)

Beş istasyon **slotu** ama dört **tür**: `STATION_SMITH`, `STATION_ALCH`, `STATION_ENCHANT`, `STATION_CAMPFIRE` (biri köy sabiti, biri oyuncunun kurduğu, park pozisyonu `x = -99999`). Kritik ve tasarımı doğrudan bağlayan detay: `station_recipe()` tarifleri **istasyon TÜRÜNE göre** filtreliyor, indekse göre değil (`src/craft.c:301-318`). Yani iki sunak koyarsan ikisi de aynı tarifleri ve aynı özel satırları gösterir. Coğrafi ayrıcalık — "bunu yalnız Kilnmoor yapar" — tarif tablosuyla değil, **yeni bir StationType** ile kurulur. Bu, aşağıdaki her kararın altındaki teknik gerçek.

41 tarif: ocak 22, kazan 12, kamp ateşi 5, sunak **2** (Pembe Rün, Zehir Rünü). Sunak neredeyse boş bir istasyon; asıl işini tarifle değil özel satırlarla yapıyor — hedef seçimi, iki rün basma, yedi stat basma.

XP: külçe 6–9, kılıç 18–22, rün üretimi 15–18, rün basma **10**, stat basma **12**, tutmayan rün **5** (yani başarısızlık da öğretiyor, bu iyi). `minSkill` kapıları 5/10/12/15/20/25/35/40 arasında dağılmış. Blueprint kilidi 5 tarifte gerçek eşya, 2 tarifte `9999` — hiçbir eşyanın öğretmediği, yalnız zincirin açtığı tarif (Dilsiz Şişe, İğne Zehri). Bu desen benim için hazır bir alet: **zincirin açtığı tarif** diye ayrı sistem yazmaya gerek yok.

Kumarlar: '+basma' hedef +6'ya kadar garanti, sonra 80/65/50, Smithing×0.2 ve ağaç bonusu telafi ediyor, tavan %95; başarısızlıkta külçeler **her durumda** yanar ve +1 geriler. Rün üretimi 60 + Enchanting×0.4 + ağaç, tavan %95. Stat basma garantili: kademe *n* için *n* taş + *n*×2 **Demir Külçe** — büyüleme→demircilik bağı zaten burada, tek bir satırda.

Üç mini ağaç sekiz düğümlü ve üçü de aynı iskelet: dört hız düğümü + üç şans/etki düğümü + bedelli keystone (Usta Çekiç +%15 şans / dövme %20 yavaş; Rün Hakimi +%15 / tören %30 yavaş). Fena değil ama üçü **birbirinden habersiz** — ağın yokluğu ağaçlarda da görünüyor.

Tavanlar: `MAX_RECIPES 48` (41 dolu, **7 boş**), `MAX_ITEM_TYPES 96` (92 dolu, **4 boş**), `MAX_RESOURCES 64` (36 dolu). Ve doğruladım: `station_textures[8]` sabit, yükleme `i < STATION_COUNT && i < 8` klampli ama `station_render_all` `STATION_COUNT` boyunca dönüp `textures[i]` okuyor (`craft.c:268`). Bugün 5 istasyon olduğu için sorun yok; **dokuzuncu istasyonda dizi dışı okuma**. Yeni istasyon eklemeden önce iki satır düzeltilecek.

## 2. AĞI KAPATAN HALKALAR

Proje sahibi haklı: üç hat paralel gidiyor. Ama ağı kurmanın önünde bir duvar var ve onu söylemem lazım — `RecipeDef` **en çok iki girdi** taşıyor (`in1Item`, `in2Item`). Yani "kor + tuz + macun" diye üç malzemeli tarif yazamıyorum. Bu kısıt aslında hediye: ağ, geniş tarifle değil **ara ürün zinciriyle** kurulacak. Her zanaat bir sonrakinin ara ürününü üretiyor, ve zincir kapanıyor.

### Halka 1 — Söndürme Tuzu: simya, demirciliğin kapısını açıyor

Ham kor sıcak ve kararsız çıkıyor; soğutulmadan dövülmez, ve doğuda soğutma avluları Sultan'ın elinde (§1). Batıda kendi başına dövmek isteyen adam soğutmayı kendi yapmak zorunda, ve bu bir ısı işi değil bir **tuz** işi.

Kazanda: `Ayotu ×2 + Slime Jeli ×1 → Söndürme Tuzu ×2`. Ocakta: `Ham Kor ×2 + Söndürme Tuzu ×1 → Kor Çeliği Külçesi ×1`. İkisi de iki girdili, ikisi de mevcut eşyaları kullanıyor, sıfır yeni mekanik.

Oyuncu bunu nasıl öğrenir: Kilnmoor'da bir çukur ustası tarifini vermez, **gösterir** — oyuncunun cebindeki ham koru alır, tuzsuz suya atar, kor çatlar, adam güler. Ondan sonra o tarif panelde "???" olmaktan çıkar (blueprint 9999 deseni, kodda hazır). Fiil: bataklıktan ot topla, kazana git, tuz kaynat, ocağa git, dövme vuruşunu yap. Dünyanın cevabı fiyat: Sultan'ın soğutulmuş korunun birim fiyatı tuzun maliyetinden **yüksek**, yani kendi tuzunu kaynatan oyuncu doğuya olan bağımlılığını gözle görülür şekilde kırıyor — ve Külhan'daki Kuyu Gözleri bunu fark eder.

### Halka 2 — Keski Ucu: demircilik, büyülemenin ön şartı

Kanon net: oyuk **tek vuruşta** açılmalı, ikinci vuruş iki kenar açar. Bir taşı tek vuruşta çatlatmadan kesen tek şey kor çeliği keskisi.

Ocakta: `Kor Çeliği Külçesi ×1 → Keski Ucu ×3`. Sunakta her basma töreni (rün ya da stat) **1 Keski Ucu** yer.

Ama bunu sert kapı yapmıyorum, çünkü büyücü oyuncuyu demirciye mecbur bırakmak rol özgürlüğünü keser. Uç yoksa basmaya yine izin var — töreni **kumara** çeviriyor. Körelmiş uçla vurursun, ikinci kenar açılır, kor iki notayı birden bulur: %55 tutar, tutmazsa rün yanar **ve hedefin +kademesi bir geriler**. Bu tam olarak '+basma'nın simetriği ve oyuncu bunu ilk kez yaşadığında hiç kimse ona kural anlatmamış olacak; bir kılıç bozulacak. Ondan sonra ocakla arasında bir ilişki olur — ya kendi keskisini döver, ya bir demirciden alır. İkisi de ağ.

Kod: sunak töreninde tek `bag_consume` + bir `randf` dalı (`main.c:7622` bloğu). Küçük.

### Halka 3 — Taşkulağı Macunu: simya, büyülemenin içine giriyor

Stat basma bugün `n taş + n×2 Demir Külçe` istiyor. Buna **`n` Taşkulağı Macunu** ekliyorum (kazan: `Kızılot ×2 + Ayotu ×1 → Macun ×2`). Gerekçe kanonda: oyuk açık kalırsa öz akar, macun tutar. Panelde tek satır uzar, kodda tek `bag_consume_n` daha (`main.c:7667`).

Bunun sonucu şu: **bir silaha stat basmak artık üç zanaatı birden istiyor** — taş (büyüleme ürünü), külçe (demircilik), macun (simya). Tek bir satırda üçü kesişiyor ve oyuncu bunu panelin malzeme satırında yeşil/gri olarak *görüyor*. Ağın kapandığı yer burası ve maliyeti üç satır.

Rün için ayrı ve daha pahalı bir halka öneriyorum, çünkü kanonda ayrı: **Okunmuş Kül**. Yağmurda basılan çıplak rün dağılır. Külle sıvanan rün kalır. Kül bir tarifle değil bir **fiille** geliyor — temizlenmiş mezar sahasında `NEST_GRAVE` ritüeli (kodda var, `ecology_do_ritual`, E tuşu) Enchanting eşiğini geçmiş oyuncuya Okunmuş Kül veriyor, çünkü yakarken adı okuyabilen adam gerekiyor. Yani **büyüleme, simyaya hammadde üretiyor** — ağın ters yönü de kapanıyor, ve dünyadaki bir ekoloji fiili zanaat ekonomisine bağlanıyor. Sıvanmamış rünün yağmurda gitmesi için `ItemStack`'e bir bayt lazım (`sealed`) + save sürümü. [YENİ MEKANİK — küçük-orta.] Bu halkanın ucuz versiyonu da var: kül olmadan basılan rünün **ışığı** kısılmaz (aşağıya bak), sıvanmış rün sessiz olur. O sıfır yeni alan ister.

### Bonus halka — hangi ocak

`STATION_SMITH_EMBER` (Ocak Çukuru, Kilnmoor). Kor çeliği külçesi ve ocakdoğan silahları **yalnız** orada dövülür, çünkü ham koru sıcakken oyuğa geçirmek çıplak elle kor tutabilen bir halkın işi. Köy ocağı soğutulmuş korla çalışır: keser, ama sesi düzdür. Kanon zaten söylüyor — ölünün geri çekildiği kılıcı yalnız Kilnmoor yapar. Mekanikte fark: ocakdoğan kılıcının `fx`i vardır, köy kılıcının yoktur.

## 3. ÜÇ ZANAATIN ANLAMI

**Demircilik bir kadran, ceza değil.** Ve bunun için hiçbir şey yazmam gerekmiyor, çünkü kodda duruyor: `light.c:71-93` — silahın `fx`i varsa oyuncu gece **kendi rengiyle ışık kaynağı olur**. Alev 180px yarıçap, %75 şiddet; zehir yeşil, pembe pembe. Yani "adı olan şey duyulur, adı olan kılıç karanlıkta bulunur" cümlesi bir kurgu süsü değil, çalışan bir ışık. Eksik olan tek şey `sneak.c` ve `crime.c`'nin bu ışığı okumaması. O eklenirse rün basmış bir suikastçi *gerçekten* yakalanır ve kararı kendi verir: hasar mı, karanlık mı. Aynı avuç korun iki yüzü, iki dosya, iki terim.

Vuruş = iş mekaniği de kanona oturuyor. Ocakta dövme, çekiç savurmasıyla ilerliyor (`main.c:9053`, `gather_mult(Smithing) × smithSpeedMult`). Kor gece daha ağır vurur — bu çarpanın gece ×1.25 olması tek satır. Bedeli kendiliğinden geliyor: ocak ışığı geceleyin iki mil öteden görünür, ve gece dövülen bir kılıcın nerede dövüldüğünü bilen biri olur. [DOĞRULANMADI: `crime.c` tanık yarıçapının ışıkla ölçeklenip ölçeklenmediğine bakmadım.]

Siyasete bağlanma yeri damga kalıbı. Bir menzilin bileziğine kimin damgası vuruluysa toprak onun (§1). Damga kalıbı bir **nesne**dir, ve nesne dövülür. Demirci oyuncu kendi kalıbını yapar; başkasının kalıbını **taklit** edebilir, ve taklit kalıpla vurulmuş bir bilezik ayar taşında ele verir. Bir zanaat becerisi, kıtanın tapu sisteminin sahtekârlığı oluyor. Tüccar ve şövalye bu nesneyi satın alır ya da çalar; kimse dövmek zorunda değil.

**Simya yanık merheminden doğdu.** İki bin yıl ateşle çalışan bir yolun en çok gördüğü yara yanık, o yüzden bu dünyanın ilk iksiri şifa değil merhem. Kazana iki tarif daha koyuyorum ve ikisi de kanon: **Yanık Merhemi** (Kızılot + Okunmuş Kül — ateş hasarına direnç, Pyrewright çukurunda çalışan oyuncunun gerçek ihtiyacı) ve **Soğuk Köz** (Söndürme Tuzu + ham kor tozu — yara yok, kan yok, adam üşür ve sabah ölür; Söndürücülerin malı). Soğuk Köz'ün oyundaki karşılığı `hitPoisonDps` deseni, sistem hazır. Kül ile tuzu ayırt etmek bir usta sırrı: **iki eşya panelde birbirine benzer** ve yanlışını kullanan bir tarifi yakar. Sahte kor tespiti de simyacının işi ama sunakta olacak — bir sonraki bölüm.

**Büyüleme harf ustalığıdır**, ve bugün kodda bir eksiği var: sunağın hedefi yalnızca kuşanılı ekipman (`enchTarget` ekipman slotları arasında dönüyor). Kanon **Taş**'ı istiyor. Hedef döngüsüne "Taş" girdiği an sunak iki yeni fiil kazanıyor: bir mezar taşındaki adı **silmek** (o ölü bir daha çağrılamaz — bestiyerdeki en ucuz kalıcı çözüm) ve bir adı **yazmak** (Cardew Halse sahnesi buna oturuyor). Bu, oyunun en siyasi zanaat fiili: bir hanenin ordusunu bir keskiyle yok etmek. [YENİ MEKANİK — orta; taş bir dünya nesnesi olmak zorunda, ve o nesne kandil direği sistemiyle aynı sistemdir, yani zaten yazılacak.]

## 4. GİZLİ SUNAK — KARAR

Üçünden **ikisi kalıyor**, biri düşüyor.

**Ayar Taşı kalıyor ve merkezî oluyor.** Dimmarch'ta, damgasız bir direğin dibinde, üstünde hiçbir ad yazılı değil. İki işlevi var ve aslında tek işlev: **ölçmek** ve **adlandırmak**. Karşılaştıracak bir yazısı olmadığı için orada kesilen oyuk, keserken yüksek sesle söylediğin adı alır — kendi adını ya da başkasının adını basabileceğin tek yer. Aynı taş, üstünde ad olmadığı için bir mihenktir: sahte koru, taklit damgayı ve başka bir sunakta kesilmiş rünü doğrular. Salthook'un ifşası bestiyerde zaten buraya bağlanmış. Kimsenin malı değil, Dimmarch'ta, iki ağın da fitil parası ödediği şeritte — yani hiçbir hizip kapatamaz.

**Kırık Ad kalıyor**, ve tek işi sessizlik: tuz yemiş taşta kesilen rün ışık ve ses vermez. Söndürücülerin imzası, hırsızın kadranı kapatma yolu, ve bedeli piyasa: ayar taşında doğrulanamaz, meşru pazar o silahı almaz (satış fiyatı düşer, bazı tüccarlar reddeder). Erişimi de bedava vermiyorum — iki metre tuzlu suyun altında, ve Saltmere zincirinin "suyu boşalt" kararı onu **açıyor**, aynı karar Kızıl Kanca'nın iskelelerini karaya oturtuyor. Bir bölge kararı bir istasyon açıyor; kalıcı iz orada.

**Sağır Sunak düşüyor.** Sebebi duygusal değil, çakışma: sessiz rün işlevini Kırık Ad'dan, kendi adını basma işlevini Ayar Taşı'ndan alıyor, ve karşılığında bir istasyon slotu, bir StationType ve mühürlü galerinin dramatik yükünü istiyor. O galerinin altında zaten Kızıl Ana var ve orası kıtanın kor fiyatının belirlendiği yer — dördüncü bir sunak oraya sığmaz. Ama fikri ölmüyor: "kendi adını bas, bir daha çağrılamayasın" Ayar Taşı'nda yapılabilir ve **defterde satırın boş kalır** sonucu aynen duruyor. Boğma kutusu da bestiyerde Söndürücü lootu olarak yaşıyor.

İstasyon bütçesi: mevcut 5 + Ocak Çukuru + Kırık Ad + Ayar Taşı = **8**. Tam tavanda. Yani `station_textures[8]`'i 12'ye çıkarmak ve `station_render_all`'ı klamplamak **zorunlu**, opsiyonel değil.

## 5. HATLAR

Hat bir kutu değil, üç-dört skill'in kesiştiği bir yer; kimse oraya kaydolmuyor, dünya bir gün ona öyle demeye başlıyor.

**Kor Ustası** — Demircilik + Madencilik + Büyüleme. Ham koru kendi çıkarır, kendi soğutur, oyuğa sıcakken geçirir. İlan anı: Kilnmoor çukur kurulu ona kendi damga kalıbını vurdurur, ve o günden sonra kor fiyatı listesinde onun adı geçer.

**Yol Eczacısı** — Simya + Bitkicilik + Konuşma. Yanık merhemiyle Pyrewright çukurlarında çalışan adamları ayakta tutuyor. İlan anı yok, birikme var: üç menzilde fitil ustaları ona borçlanır ve bir gece onun için kapı açarlar. Bu hattın gücü itibar ve fiyat, silah değil.

**Kazıcı** — Büyüleme + Gizlenme + Konuşma. Taştan ad siler. Hukuken meşru bir zanaat, sosyal olarak cinayetten kötü. İlan anı bir pazarlık: bir aile ona kendi dedesinin taşını siler mi diye sorar.

**Söndürücü Zanaatkârı** — Simya + Büyüleme + Suikast. Soğuk köz döker, Kırık Ad'da sessiz rün keser, ruhsat numarasını deftertutandan alır.

**Sahtekâr** — Demircilik + Yankesicilik + Konuşma. Taklit damga kalıbı, ıslak defter sayfası, Kızıl Kanca'nın tuzlu koru. Ayar Taşı onun tek gerçek düşmanı, ve o taş kimsenin malı olmadığı için satın alınamıyor.

Beş oyuncu testi: savaşçı kor çeliği kılıcı için ağın çıktısını **satın alır** (hiçbir zanaatı öğrenmesi gerekmez, tuz ve keski uçları piyasada), büyücü sunakta yaşar, hırsız Kırık Ad'a gider, tüccar üç zanaatın ara ürünlerinin tümünü **taşır** ve ağın kendisi onun kârıdır, şövalye damga kalıbıyla toprak sahibi olur.

## 6. ÜÇ KAPI

Kodda kapı üç şart: izin görevi REWARDED + bölgede bounty yok + itibar > `REP_NO_QUESTS`. Kanona oturuşu:

Köy ocağı ve kazanı Demirci Mahir'in ve Baran'ın elinde — tanışma görevi. Kadim Sunak sahipsiz harabe, `permQuest = NULL`, kapı yok; keşif ödülü olarak kalmalı, çünkü büyücünün ilk basamağının kimseye borçlu olmaması lazım. **Ocak Çukuru** Ocakdoğanların: izni Slake veriyor ve şartı fiyat değil ritüel — sunağa git, yak, gel. Kızıl Kanca üyeliği bu kapıyı kapatıyor (kanonda yazılı), ve kod bunu bugün bounty/itibarla yapamaz; hizip rütbesini okuyan dördüncü bir şart gerekiyor. [YENİ MEKANİK — küçük: kapı kontrolüne bir hizip koşulu.] **Kırık Ad** bölge kararıyla açılıyor, üyelikle değil. **Ayar Taşı** kapısız, ve kapısız olması onun siyasi anlamı.

Kapının kapanması da hikâye taşıyor: bounty yüzünden kapanan ocak, oyuncuya "cezanı öde" derken aslında Marshal'ın vergi defterinden konuşuyor.

## 7. FATURA VE SLOT BÜTÇESİ

**Sıfır kod:** Söndürme Tuzu, Kor Çeliği Külçesi, Keski Ucu, Macun, Yanık Merhemi, Soğuk Köz, Damga Kalıbı tarifleri (yeni tablo satırı, hepsi iki girdili).

**Küçük:** stat basmaya macun girdisi (1 satır); sunak töreninde keski ucu tüketimi + körelmiş uç kumarı (~8 satır); gece dövme çarpanı (1 satır); `sneak.c`/`crime.c`'ye silah `fx` ışık terimi (2 satır + sorgu); ocak kapısına hizip koşulu; `station_textures` 12'ye + render klampı (2 satır).

**Orta:** iki yeni StationType (`SMITH_EMBER`, `ENCHANT_MUTE`/`ENCHANT_ASSAY`) — veri ucuz ama `craft.c`'de `bool smith`/`bool ench` kontrolleri ~6 yerde yardımcı fonksiyona dönmeli, yoksa yeni sunaklar özel satırlarını göstermez. Sunak hedefine **Taş** eklenmesi (dünya nesnesi + `enchTarget` sentinel). `ItemStack.sealed` bayrağı + yağmurda dağılan rün (save sürümü artar).

**Slot bütçesi.** 11 yeni eşya öneriyorum: Ham Kor, Söndürme Tuzu, Kor Çeliği Külçesi, Keski Ucu, Taşkulağı Macunu, Okunmuş Kül, Yanık Merhemi, Soğuk Köz, Damga Kalıbı, Sağır Rün, Ocakdoğan Kılıcı. **Dört boş slot var, yetmiyor.** İyi haber: `MAX_ITEM_TYPES`'ı 128'e çıkarmak tek `#define`; `item_textures.c` zaten tavanı aşarsa uyarı basıyor ve klampli, `save.c` eşya indeksini `Sint32` yazıp `ITEM_TYPE_COUNT`'a karşı doğruluyor (`save.c:71`) — yeni eşyalar enum'un **sonuna** eklendiği sürece save formatı değişmiyor. Maliyeti `player.itemCd[MAX_ITEM_TYPES]` dizisinin büyümesi (32 float). [DOĞRULANMADI: `itemCd`'nin save'e yazılıp yazılmadığına bakmadım; geçici bir alan olduğu için yazılmıyor olması muhtemel.]

10 yeni tarif; 7 boş slot var, **yetmiyor**. `MAX_RECIPES` 64'e çıkacak: `craft.c`'deki `learned[]` ve `save.c:471`'deki `recipeFlag[MAX_RECIPES]` iki tampon. Tarifler tablonun **sonuna** eklendiği sürece (craft.c:97'deki uyarı bunu zaten söylüyor) `learned[]` indeks sırası korunur, eski kayıtlar kaymaz, sürüm artmaz.

İstasyon: 3 yeni, tavan 8 — tam doluyor, dizi büyütülmeden yapılmamalı.

Tekrar oynanabilirlik: ağ kapandığında dünyada üç kalıcı iz kalıyor. Kendi tuzunu kaynatan bölge Sultan'ın musluğundan çıkıyor (kor fiyatı). Kırık Ad açılırsa Saltmere'in suyu bir daha dolmuyor. Ve bir kılıcın karanlıkta parlıyor olması, o kılıcı yapanın kim olduğunu, kaç gece sonra bile, gören herkese söylüyor.

**Değinilen dosyalar:** `/Users/hakanartuc/PixelGame/src/craft.c`, `/Users/hakanartuc/PixelGame/src/craft.h`, `/Users/hakanartuc/PixelGame/src/item.h`, `/Users/hakanartuc/PixelGame/src/light.c`, `/Users/hakanartuc/PixelGame/src/skilltree.c`, `/Users/hakanartuc/PixelGame/src/save.c`, `/Users/hakanartuc/PixelGame/src/main.c` (üç kapı 3358-3403, sunak töreni 7541-7695, dövme vuruşu 9053-9122, istasyon dokusu 1196-1198).