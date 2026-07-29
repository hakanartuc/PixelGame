`src/quest.c`, `src/quest.h`, `src/player.h`, `src/save.c`, `src/assassin.c`, `src/dialog.h`, `src/common.h` ve `docs/GDD.md` §3.2/§3.4/§4'ü okudum. Aşağıdaki mimari o kodun bugün yapabildiğine göre yazıldı; farkı §7'de fatura olarak veriyorum.

---

# HİKÂYE İŞLEYİŞİ — "Sönen Kandiller"

## 1. ZİNCİR MİMARİSİ

Beş tür zincir var ama hepsini tek bir görünür sayaç birbirine bağlıyor: **kaç direk yanıyor ve kimin adına.**

Bu benim tek büyük yapısal kararım, gerekçesi de şu: ana zincirle bölge zincirlerini "önce şunu bitir sonra şunu" diye kilitlemek istemiyorum, ama ana zincirin de boşlukta ilerlemesini istemiyorum. Direk sayacı ikisini de çözüyor. Oyuncu ne yaparsa yapsın — bir bölge zincirini bitirse de, Külhan'dan kor kaçırıp satsa da, bir mahalleye kendi adına tören yapsa da — sonuç aynı yerde birikiyor. Ana zincirin perde eşikleri "şu görevi bitir" değil, "**şu hattın şu kadarı aydınlık olsun**" diyor. Ekranda görünür (direk ya yanıyor ya sönük), tek fiil (kor koy, fitili değiştir), ve beş dakikada anlaşılır.

**Ana zincir** üç perde, 34 düğüm: Perde 1 Millrun'da 12, Perde 2 kıtaya yayılmış 13, Perde 3 Ashmarch–Emberhold'da 9. Perdeler arası geçiş kilitli, perde içi neredeyse tamamen serbest.

**Bölge zincirleri** yedi tane, 5–9 düğüm. Kural: son iki düğümü hariç hepsi ana zincire dokunmadan oynanır. Son iki düğüm bölgenin kalıcı durumunu yazdığı için ana zincirin ilgili perdesini bekler — yoksa oyuncu Perde 1'de Barrowfell'i bitirir ve Perde 2'nin bütün gerilimi ölür.

**Örgüt zincirleri** beş tane, rütbe başına 2–4 düğüm. Bunlar hiçbir perdeye bağlı değil; oyuncu isterse baştan sona sadece Fitilci olarak oynar, ana zincir arka planda ilerlemez ve bu bir kayıp değil, bir seçim (dünya karanlığa gömülürken sen mesaj taşıyorsun).

**Hane zincirleri** kısa ve keskin: Millward, Coldhearth, Ironkiln, Kavus. Üçer düğüm — bir iş, bir sınav, bir yemin. Şövalye/tüccar oyuncunun sahne aldığı yer.

**Kişisel zincir**: dört düğüm, "İsim". Oyuncunun adının bir yere kaydedilmesi. Sonuna kadar fark etmez, sonra çok fark eder.

**Düğüm ne demek?** Kodda bugün quest = bir sayaç (KILL veya FETCH). Düğüm bundan büyük: **bir bayrak değeri.** `chain[X] = 7` demek "X zincirinin yedinci durumundayız" demek; bir düğüm bazen tek bir görev, bazen sadece bir konuşma, bazen sadece bir yerde bulunulması. Bu desen kodda **zaten var** — `SF_ASSASSIN` 0'dan 13'e sayıyor ve Sessiz El zinciri onunla yürüyor (`src/main.c:2308–2408`). Yani mimari yeni bir icat değil, çalışan tek örneğin genelleştirilmesi.

Zincirler paralel. Oyuncu ana zinciri hiç ellemeden Millrun, Kilnmoor ve Saltmere zincirlerinin ilk üçte ikisini, üç örgütün orta rütbelerini ve iki hane yeminini oynayabilir — kabaca yirmi saatlik içerik.

---

## 2. ANA ZİNCİR

Her düğümde şu şablon: **en az üç çıkış yolu.** Kes / gizlen / öde / ikna et / oku. Beş rol bunun içine kendiliğinden oturuyor. Aşağıda ilk perdeyi tam açıyorum, ikinci ve üçüncü perdeyi düğüm başlıkları + kritik dallanmalarla veriyorum (yoksa bu metin bir kitap olur).

**PERDE 1 — VADİ (12 düğüm)**

1. **Bostandaki El.** Meydan kandili sönük, toprakta hareket var. Tetikleyici: oyun başlıyor. Fiil: sabaha kadar bir avuç kor bul ve fitile koy — ya da bulma. Çıkışlar: satın al (tüccar), vergi arabasından çal (hırsız), Pyrewright'ı ikna et (şövalye/büyücü), değirmencinin yadigârını rehin al, ya da kimseyi çağırmayıp elin sahibini kendin karşıla (savaşçı — sabaha üç ölü kalkar, ikisi köyün kendi ölüsüdür ve halk bunu unutmaz). Etki: `chain_main = 1`, direk sayacı ya +1 ya 0.
2. **Kül Tavası Boş.** Direğin dibindeki tava kazınmış. Kandil kendi kendine sönmedi, korunu biri aldı. Fiil: soruştur. Büyücü tavadaki fitil külünün tazeliğinden anlar, hırsız kilidin çizilmişliğinden, tüccar üç hafta önce köyden kim mal aldığını hesap defterinden bulur.
3. **Fitilin Altındaki Bez.** Direğin altında sıkışmış renkli bir bez. Kimse ne olduğunu bilmiyor. Fiil: al ve taşı. Çıkış: bezi üç kasaba öteye götürürsen Fitilciler zincirinin kapısı açılır; yakarsan hiçbir şey olmaz (ve bu geri alınamaz — o hat bir daha o mesajı göndermez).
4. **Sönük Direkler Bir Hat Çiziyor.** Vadide üç direk daha sönük ve dördü aynı doğrultuda. Fiil: yürü ve gör. Yanından geçtiği mil taşında "1.847" yazıyor; kimse açıklamıyor.
5. **Fitil Kancası.** Hattın ucunda eyerinde kanca taşıyan bir Coldhearth atlısı. Fiil seçimi: dövüş / izle / satın al / Millward'a ihbar et. Dördü de farklı bilgi verir — dövüşen kancayı alır (eşya), izleyen sonraki hedefi öğrenir (harita işareti), satın alan Coldhearth'la kanal açar, ihbar eden hane itibarı kazanır.
6. **Kurgan Ağzı.** Vadinin altındaki ilk höyük açılmış. Fiil: Pyrewright'la pazarlık. Burada oyunun asıl dersi veriliyor — çukurda yakmak kalıcı ama üç gün ve pahalı; kandil ucuz ve geçici. Savaşçı çukurun ağzını kalkanla tutarak parasını indirir, büyücü ateş büyüsüyle Pyrewright'a ihtiyaç bırakmaz (ama onların loncasına kapıyı da kapatır), tüccar toptan fiyat çıkarır.
7. **Lampwake.** Kalabalığın önünde kor fitile konur. Fiil: töreni kimin adına yaptığını **seç** — kendi adın, Millward, Coldhearth, Pyrewright'lar, ya da "kimsenin". İlk kalıcı yazı: o direk bundan sonra o adı taşır ve NPC'ler seni o adla anar.
8. **Değirmen Borcu.** Millward'ın kor borcu var ve Coldhearth'ın teklifi masada: kor karşılığı diz. Fiil: defterlere bak (kilit açma ya da ikna), borcu gör. Çıkış: borcu kendin kapat (tüccar için pahalı ama hattı sahiplenir), Coldhearth'ın temsilcisini gece boğ (geri alınamaz), Kilnmoor'la evlilik pazarlığı aç, ya da karışma.
9. **Vadinin Gecesi.** Coldhearth kalabalığı gelir, üç direği aynı gece kırar. Fiil: savun / bırak / kırılan direği kendi adına yeniden yak. İlk büyük savaş sahnesi ve ilk büyük ekonomik seçim (savunmak korun tamamını yakar).
10. **Hat Kimin.** Vadinin hattı bir ada yazılır: Millward, Coldhearth, kendin, ya da kimse. **Geri dönüşsüz.** Kimse'yi seçen oyuncu ilginç bir dünyada yaşar — vadi gece kapanır, gündüz ticaret üçe katlanır, kervanlar sadece şafakla geçer.
11. **Kurgan Defteri, Yarısı.** Yanan bir lejyon kampında yarı yanmış bir defter: sütun sütun isim. Fiil: oku. Oyuncu burada kuralın ikinci yarısını öğrenir — **ölü kendi adını duyamaz, kor gürültülü yanar.** Bunu bir kitaptan değil, defterdeki bir ismi karanlıkta yüksek sesle okuyan bir Pyrewright çırağından öğrenir; toprak cevap verir. Etki: defter parçası artık envanterde bir kaynak; Söndürücüler onu ister, Barrowfell zinciri onunla açılır.
12. **Doğuya Bakan Direk.** Vadinin sınırındaki en son direk. Perde 1 kapanır, perde eşiği: **vadi hattında en az sekiz direk yanıyor** (kimin adına olursa olsun). Etki: Kilnmoor, Barrowfell ve Saltmere haritaları açılır.

**PERDE 2 — KOR KİMİN (13 düğüm).** Sırası serbest, üç koldan geliyor.

13–16 **Kilnmoor kolu**: çukur dövüşü (giriş bileti), Ocakdoğan sunağı, mühürlü galeri kapısı, Ironkiln'in teklifi. 17–20 **Barrowfell kolu**: höyük tarlasında yürüyen kalabalık, Coldhearth'ın kalkan ölü ordusuna verdiği emri **bir bezden** okuma, kayıp lejyon kampı, Kurgan Defterleri deposu. 21–22 **Külhan kolu**: Sultan'ın musluğu ve kervan güzergâhı pazarlığı. 23 **Saltmere**: sular altındaki kütüphane — büyücünün "sönmüş kandil kalıcı yanar mı" sorusunun cevabı burada, yarım.

24–25 **[EJDERHA-AÇIK] Wyrmcrag.** Yapısal işlevi sabit, sebebi paralel kurgucuda: ejderhalar **merkez ocağın adresini ve kapı yasasını bilen tek varlık.** Emberhold'a üç giriş var; biri onlardan geliyor. Düğüm 24 dört ejderhadan birine ulaşmak (savaşçı dövüşerek, büyücü tabletten öğrendiği dille konuşarak, suikastçi uykusundaki pulun altına girerek, tüccar sürüsüne mal getirerek, şövalye ahit taşının önünde açık yemin ederek), düğüm 25 birine söz vermek — **geri dönüşsüz, kalanı üçü düşman.** Sebep açık kaldığı için bu iki düğümün metni yazılmadı, kancaları yazıldı.

Perde 2 eşiği: **İlk Yol üzerinde yirmi direk yanıyor + Kurgan Defteri parçası + Emberhold kapı bilgisinin üçünden biri.**

**PERDE 3 — MÜHÜRLÜ ŞEHİR (9 düğüm).** 26–29 Ashmarch'ın boş posta menzilleri: her menzilde bir sönmüş direk ve bir kayıt sandığı, dördü bir arada kapı yasasını tamamlıyor. 30 **Emberhold'a giriş** — dört yol (kapıyı kır / lağım / muhafız kalıntısını satın al / mühür yasasını okuyarak yasal olarak açtır). 31–33 şehrin içi: kayıt odası, merkez ocağın koridoru, ve **son imparator.**

Burada verdiğim kanon kararı şu: kimse imparatoru yakmayı engellemedi. **Kendisi reddetti.** Ocak yasası "kapanışı imparatorun yakılışı mühürler" der; yakılırsa kıtanın bütün kandilleri bir gecede sönerdi. Yakılmayınca mühür yarı kapandı ve yirmi yıl boyunca birikmiş korla yavaş yavaş, batıdan doğuya söndü. Yani adam kıtaya yirmi yıl kazandırdı ve o yirmi yılı da o harcadı. Karar hâlâ akıllı görünüyor, kriz onun hatası değil yan etkisi. Oyuncu onu ocağın en parlak yerinde oturmuş halde bulur — kendi adını duymamak için gürültünün içine yerleşmiş, ne ölü ne diri.

34 **Final.** Üç seçenek (besle / temelli söndür / kendine al). Üçüncüsü koşullu: **kıta hattında en az kırk direk senin adına yanıyor olmalı.** Yoksa o seçenek ekranda yok, çünkü kimse seni tanımıyor.

---

## 3. BÖLGE ZİNCİRLERİ

**Millrun** (5 düğüm): *Vadi, kor için diz çökmek zorunda mı?* Kalıcı: hattın sahibi + gece kapalılık. Ana zincirle kesişme: düğüm 8–10 zaten ana zincirin içinde — Millrun bilerek en çok kaynaşan bölge, çünkü öğretici bölge.

**Barrowfell** (9): *Yirmi yıl önce sönen kuzeyde ölü sayısı canlı sayısını geçti; Pyrewright'lar yakmak, Coldhearth kullanmak, köylüler unutmak istiyor.* Kalıcı ve tek: **höyük tarlası yakılır ya da yakılmaz.** Yakılırsa Coldhearth'ın ordusu iki hafta içinde erir (ana zincir Perde 2'yi çok kolaylaştırır) ama **Kurgan Defterleri kül olur.** Yakılmazsa isim ekonomisi ayakta kalır, kuzey ordu olur.

**Kilnmoor** (7): *Kor kimin fiyat listesinde — madeni işletenin mi, madeni ilan edenin mi?* Kalıcı: kor birim fiyatı ve kor çeliği craft izni. Ana zincirle kesişme: Ironkiln yemini Perde 3'te muhafız kalıntısını satın alma yolunu ucuzlatır.

**Wyrmcrag** (6, [EJDERHA-AÇIK]): mesele boş, kancalar dolu — dört isim, dört ayrı istek, biri seçilir.

**Saltmere** (7): *Kitaplar suyun altında; suyu boşaltmak kitapları kurtarır ve Kızıl Kanca'nın iskelelerini karaya oturtur.* Kalıcı: büyücü için üç tomar + tablet dili / kaçak kor akışı. İkisi bir arada olmaz.

**Ashmarch** (5): *Yolda kimse kalmadı ama kayıt hâlâ tutuluyor.* Kalıcı: Emberhold kapı bilgisinin dördüncü, en zahmetsiz yolu.

**Ateşkuyu/Külhan** (8): *Sultan musluğu kesmiyor, çünkü kesmek satmaktan az kazandırır — ama bir gün hesap değişir.* Kalıcı: doğu arzı açık, kısık ya da yalnız sana açık.

**Ortak düşman izi — her bölgede aynı nesne.** Her bölgede bir tane **ismi kazınmış direk** var. Taşındaki iki bin yıllık isim sütunları keski ile silinmiş, ve o direk sönükken bile çevresinde kimse kalkmıyor. Oyuncu yedincisini bulduğunda bir şey anlıyor: silmeyi biri sistematik yapmış ve batıdan doğuya doğru yapmış — kandillerin söndüğü sırayla. Yedinci direk Emberhold kapı bilgisinin üçüncü parçasıdır. Tek nesne, yedi bölge, sıfır metin duvarı.

---

## 4. ÖRGÜT ZİNCİRLERİ VE ÇİFTE ÜYELİK

**Fitilciler** — sınav: bir bezi üç kasaba öteye götür, **açmadan** (açarsan öğrenirsin ama ilk rütbede kalırsın). Rütbeler: Fitil, Bez, Direk, Hat, Kayıt. En üstün bedeli: bütün hattın kaydı senden geçer, yani ağdaki her kutu senin nerede olduğunu bilir. Seni aramaya çıkan biri ağı sorgular ve ağ cevap verir.

**Söndürücüler** — sınav: bir kandili ve bir adamı aynı gece. Rütbeler: Islak Fitil, Söndüren, Kara Cam, Son Nefes. En üstün bedeli: imzan artık tanınıyor; bölge bounty'si ödemekle silinmiyor (`reputation[]` zaten ödemeyle düzelmiyor — `src/player.h:143`).

**Kuyu Gözleri** — sınav: bir batı kervanının güzergâhını sat. Üç rütbe: Göz, Kuyu, Ağız. Bedeli: üstünde Sultan mührü taşımak batıda asılma sebebi, ve mührü taşımadan rapor veremezsin.

**Sessiz El** — kodda çalışıyor, aynen kalıyor: beş rütbe (Çırak/El/Bıçak/Gölge/Usta El, `src/assassin.c:41-43`), kontrat panosu, puan eşikleri. Yeni evrende Saltmere ve Millrun'a yerleşiyor.

**Kızıl Kanca** — sınav: kayıtsız bir sandık kor taşı. Üç rütbe. Bedeli: Ocakdoğanlar seni tanır ve sunaklarına yaklaştırmaz; craft kapısı kapanır.

**Çifte üyelik, somut hatalar.** İkisi de fiziksel nesne üzerinden, ikisi de hava sistemine bağlı:

*Kendi Kutun* (Fitilci Direk + Söndürücü Söndüren): hattın bakımı sana geçtiği gece direği açarsın, içinde iki renkli bir bez var — ıslak fitil imzası ve hedef adı. Hedef senin Fitilci ustan. Üç çıkış: kutuda bırak (usta ölür, sen temiz kalırsın, Fitilci rütben donar), yak (Söndürücü rütbesi bir düşer, ikinci kez yaparsan atılırsın), ustayı uyar (Fitilcilerde Hat rütbesine atlarsın, Söndürücülerde ifşa olursun — **geri dönüşsüz**).

*Yanlış Kutu* (Fitilciler + Kuyu Gözleri): batı ağı bezle konuşur, doğu ağı mumlu defter yaprağıyla. İkisi de envanterde aynı slotta durur ve ikisi de bozulur — bez yağmurda ıslanır, mum sıcakta erir. Oyuncu acele eder. Yanlış kutuya doğru mesajı bırakınca üç gün sonra iki ağ da aynı kişiyi arar; sokakta iki ayrı adam sana aynı soruyu sorar. [YENİ MEKANİK GEREKLİ: kutuya nesne bırakma + doğrulama; bozulan taşınabilir nesne.]

**İhanet/atılma:** rütbe sıfırlanmaz, **negatife düşer.** Atılan üye ağın hedef listesinde bir satırdır; loca dükkanı kapanır, o örgütün bulunduğu her bölgede bounty tavanı artar, ama örgütün rakibi kapısını açar. Yani ihanet bir kayıp değil, bir taraf değişikliği.

---

## 5. KESİŞME HARİTASI

Bölge kararı örgüt zincirini kapatıyor: **Barrowfell'de höyükleri yakarsan Kurgan Defterleri kül olur ve Söndürücülerin üst iki rütbesi kapanır** — isimsiz ölü çağrılamaz, Kara Cam ve Son Nefes sözleşmelerinin tamamı çağırmaya dayanır. Aynı karar ana zincirin Perde 2'sini bir düğüm kısaltır. Kuzeyi temizlemenin bedeli suikast kariyerinin tavanıdır ve oyuncu bunu kararı verirken bilmez, üç saat sonra öğrenir.

Aynı anda yürümeyen ikililer, dördü: Fitilci Kayıt + Söndürücü Son Nefes (aynı ağın hem kaydı hem kesicisi olamazsın). Kızıl Kanca ustalığı + Ocakdoğan craft izni. Saltmere'de suyu boşaltmak + Kızıl Kanca'nın orta rütbeleri. Ironkiln yemini + Ocakdoğan sunağı.

**Hiç oynamayan ne kaybeder?** Oyun bölge zincirlerinin hiçbiri oynanmadan bitebilir — direkleri satın alınan korla yakmak mümkün, sadece çok pahalı. Kaybedilen: finalin üçüncü seçeneği (kırk direk senin adına yanmıyorsa "kendine al" ekranda görünmez), ejderha kolu, ve Emberhold'a dört giriş yolundan üçü. Yani atlayan oyuncu oyunu bitirir, ama kapıyı kırarak bitirir.

**Geri dönüşü olmayan kararlar — on tane:** Lampwick'in ilk gecesi · vadi hattının adı · Barrowfell höyükleri · Kilnmoor sunağı ile Ironkiln yemini arası · Saltmere'in suyu · Kendi Kutun · ejderha sözü · Külhan'a satılan güzergâh · kendi adını bir taşa yazdırmak · merkez ocak. Onu geçmesin — her biri bir dünya durumu yazıyor ve hepsi tek bir kayıt satırında saklanabiliyor.

---

## 6. İLK İKİ SAAT

**0–4. dakika.** Karanlık bostan. Komşu kadın fenerini kaldırır, topraktan çıkan el durur; fener iner, el kıpırdar. Hiç metin yok. Oyuncu bunu iki kez izler ve kuralı öğrenir.

**4–15.** Meydan direği sönük, kül tavası boş. Beş yol açık ve hepsi işaretli: bakkalın rafında bir kor parçası (12 gümüş, oyuncunun 7'si var), vergi arabasının kilidi, Pyrewright çırağının çadırı, değirmencinin kolyesi, ve kimseye gitmemek.

**15–20.** İlk Lampwake. Kalabalık toplanır, kor fitile girer, ışık gelir. Oyuncuya tek soru sorulur: *kimin adına?* Cevap direğe yazılır ve o gece kimse bostana bakmaz.

**20–35.** Kazınmış tava. Kandil kendi kendine sönmedi. Direğin dibindeki bez bulunur — kimse ne olduğunu bilmiyor, biri "üç kasaba öteye götürene iki gümüş" der.

**35–50.** Sönük direkler bir hat çiziyor. Oyuncu yürür, "1.847" yazan mil taşının yanından geçer, üçüncü sönük direkte ilk kurgan ağzını görür.

**50–65.** Coldhearth atlısı. Dört fiil, dört farklı bilgi. İlk gerçek dövüş burada, ilk gerçek gizlilik de.

**65–85.** Pyrewright çukuru. Üç gün, altı adam, kırk gümüş — ya da bir gecelik kandil, üç gümüş. Oyunun bütün siyaseti bu iki fiyat arasında ve oyuncu bunu bir pazarlık ekranında öğreniyor.

**85–105.** Millward'ın defteri. Borç görünür, Coldhearth'ın teklifi masada.

**105–120.** **Vadinin Gecesi** — üç direk aynı gece kırılır. İlk büyük dallanma, ve oyuncunun elindeki bütün kor bir gecede yanabilir.

Bu iki saatte hiçbir kural anlatılmadı. Kor ışığında ölü kalkmaz (dakika 2), kalıcı çözüm yakmadır (dakika 70), direk bir isim taşır (dakika 18), sönen direkler bir hat çizer (dakika 40), bez mesaj taşır (dakika 30).

---

## 7. KOD FATURASI

`quest.c` bugün 3 görev tanımı taşıyor ve tek bildiği şey iki tür sayaç: KILL (`targetMob` + `targetCount`, `quest_on_kill` sayar) ve FETCH (`fetchItem` + `fetchCount`). **Önkoşul alanı yok, dallanma yok, düğüm sırası yok, birden fazla çıkış yok, zincir kimliği yok.** `MAX_QUESTS 16`. Yani bu mimarinin hiçbir parçası quest.c'de yaşamıyor.

Zincir mantığı bugün **main.c'de elle yazılmış**: `storyFlags[SF_ASSASSIN]` 0'dan 13'e kadar sayıyor ve her adım ayrı bir `if` bloğu (`src/main.c:2308, 2321, 2336, 2353, 2367, 2388, 2397, 2408`). Bu desen doğru, ölçeklenmiyor.

**`storyFlags` yetmiyor.** `Uint8 storyFlags[16]` (`src/player.h:201`), 11 indeksi dolu (`SF_K9_KANDIL`…`SF_TAIL_MARK`), 5 boş. Bu mimarinin ihtiyacı: ana zincir 3 + bölge 7 + örgüt 5 + hane 4 + kişisel 1 + direk sayacı ve sahiplik 2 + ifşa/çifte üyelik 2 ≈ **24–28 slot**, artı direk sahipliği başına bölge. `save.c` bunu sabit 16'lık döngüyle yazıyor (`src/save.c:252-253`) ve okuyor (`:531`) — tavanı büyütmek save formatı değişikliği demek (bugün `SAVE_VERSION 29`).

**Rütbe tek örgüt için var.** `player.assassinPoints` bir int, `assassin_rank()` beş eşikli. Beş örgüt için `int factionPoints[8]` + örgüt başına eşik tablosu + `assassin.c`'nin genelleştirilmesi gerekiyor. Negatif rütbe (atılma) bugün temsil edilemiyor ama int olduğu için bedava geliyor.

**Diyalog koşulları sabit enum.** `DialogVar` on değer taşıyor ve tek storyFlag kancası `DVAR_HINT` — o da elle `SF_S3_HINTS`'e bağlı (`src/main.c:89`). Zincir düğümlerinin diyalogdan okunabilmesi için genel bir `DVAR_FLAG` + indeks parametresi gerekiyor; `DialogReq` bugün sadece `var/lessEq/value` taşıyor, indeks alanı yok. Ayrıca `DIALOG_MAX_OPTS 5` — beş rolün beş ayrı çıkışını tek düğümde sunmak istediğimiz yerlerde bu sınır tam sınırında, muhtemelen 6–7'ye çıkması gerekecek.

**`MAX_REGIONS 8`** (`src/common.h:14`) ve `REGIONS[]` bugün 6 kayıt taşıyor. Yedi yeni bölge + köy bu tavanı taşırıyor; `bounty[]`/`reputation[]` dizileri ve save bloğu (`src/save.c:201-203`) buna bağlı.

**Lore sayfası sistemi yok.** src içinde `loreId` ya da lore item araması yalnız `minimap_reset_explored`'a çarptı. GDD §4.9 onaylı ama kodda sıfır. Kurgan Defteri parçaları ve kapı yasası sayfaları bu sisteme bağlı.

**Gereken yeni mekanikler:** [YENİ MEKANİK GEREKLİ: kandil direği dünya nesnesi — durum, yakıt, sahiplik adı, ışık yarıçapı, gece etkisi] · [YENİ MEKANİK GEREKLİ: ölü posta kutusu — direğe nesne bırak/al, yanlış nesne doğrulaması] · [YENİ MEKANİK GEREKLİ: bozulan taşınabilir nesne — havaya/güne bağlı] · [YENİ MEKANİK GEREKLİ: isim kaydı — bir ismin taşta olup olmaması, silinmesi, yakılması] · [DOĞRULANMADI: post-fx karartma katmanının direk ışığıyla noktasal çalışıp çalışacağı — `docs/GDD.md` §6.1 post-fx'i tam ekran filtre olarak tanımlıyor, noktasal ışık maskesi gördüğüm kadarıyla yok].

Direk sahipliği ve final koşulu (kırk direk) tek başına `Uint8` bayrak dizisine sığmaz — bölge başına küçük bir sayaç dizisi gerekiyor. En ucuz yol: `Uint8 lampLit[MAX_REGIONS]` + `Uint8 lampOwner[MAX_REGIONS]`, iki save alanı, on altı bayt.