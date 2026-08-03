# YAZIM VE DİL DENETİMİ — KADRO KARTLARI

> Dil denetçisi ajanının raporu, 2026-08-03. Ölçüt: `docs/LORE.md` §15.
> Örneklem iddiaları grep'le doğrulandı: Perçin'in 14/11 yıl çelişkisi · Hafize'nin
> 940→900→980 dizisi · Sekiz'in birinci barkında replik metninin hiç olmaması ·
> isim havuzunun kendi §7'sinin Kıraç'ı ana hat dışına koyması — dördü de gerçek.
> Bu bir DENETİM raporu; düzeltmeler kartlara işlendikçe buraya damga düşülecek.

---

## A. DÜZELTİLECEKLER

### A.1 Repliklerde (oyuna girecek metin)

| Dosya | Karakter | Hatalı metin | Düzeltilmiş metin | Gerekçe |
|---|---|---|---|---|
| sehir:281 | Baran | `"Defterde adın yok evlat."` | `"Defterde adın yok, evlat."` | Hitap sözünden önce virgül konur (TDK). Belgede Balaban'ın *"Bacı, kepçeyi…"* ve Vedia'nın *"Sayman, rakamı…"* satırlarında kural doğru uygulanmış — burada atlanmış. |
| sehir:317 | Bekmez | `"Bir kase iki akçe canım."` | `"Bir kase iki akçe, canım."` | Aynı kural. |
| sehir:327 | Bekmez | `"Bu akşam otuz beş kişi yedik canım."` | `"Bu akşam otuz beş kişi yedik, canım."` | Aynı kural. |
| sehir:59 | Gökçe | `"Atın su içti mi kardaş?"` | `"Atın su içti mi, kardaş?"` | Aynı kural. Bu, oyundaki en çok duyulacak ilk cümlelerden biri. |
| salon:59 | Rüstem | `"Kâtip, bu gün de kayarsa dördüncü tarihi ben yazdırırım."` | `"Kâtip, bu tarih de kayarsa dördüncü tarihi ben yazdırırım."` | "Bugün" zarf olarak bitişik yazılır; ayrı yazılan "bu gün" yalnız belirli bir günü işaret eder. Cümle "bugün mü, bu tarih mi" arasında asılı kalıyor — anlam "tarih" olduğu için kelime değiştirilmeli. |
| sehir:360 | Hafize | `"Dokuz yüz. Dün dokuz yüz kırktı. Yarın dokuz yüz seksen olacak…"` | `"Dokuz yüz altmış. Dün dokuz yüz kırktı. Yarın dokuz yüz seksen olacak…"` | İki ayrı hata. (1) Kartın kendi kuralı: *"Rakamı yuvarlamaz — dokuz yüz kırk der, bin demez."* Aynı satırda "dokuz yüz" diyor. (2) Aritmetik: 940 → 900 → 980 dizisi işleyen bir vadeyi anlatmıyor, sayı önce düşüp sonra fırlıyor. |
| sehir:31 | Perçin | `"Benimkini on bir yıldır yazıyor…"` | `"Benimkini on dört yıldır yazıyor…"` | Aynı kartın KİMLİK bölümü: *"on dört yıl önce geldi… on dört yıl geçti."* Replik 11 diyor. (Ya replik 14 olmalı, ya deftere 11 yıl önce girdiği kartta açıklanmalı.) |
| sehir:95 / 105 | Nazlı | `"Ne bakıyorsun."` ve `"Niye bakıyorsun?"` | Tek noktalama seçin | Aynı karakterin aynı hamlesi iki farklı noktalamayla yazılmış. İkisi de savunulabilir, ikisi birden olmaz. |

### A.2 Belge genelinde (anlatım, ama yine de hata)

| Dosya · satır | Hatalı | Doğru | Gerekçe |
|---|---|---|---|
| slotlar:264 | `hikayeyi` | `hikâyeyi` | Düzeltme işareti. Belgenin geri kalanı (`sehir:260`, isim-havuzu 4 yerde) "hikâye" yazıyor — tek kaçak bu. |
| slotlar:260, 276 | `döğüşür` (×2) | `dövüşür` | "döğüş-" ölçünlü yazım değil. |
| isim-havuzu:186 | `Türkçe'nin` | `Türkçenin` | Dil ve millet adlarına gelen çekim ekleri kesme ile ayrılmaz. |
| isim-havuzu:259 | `Türkçe'de` | `Türkçede` | Aynı. |
| isim-havuzu:265 | `Farsça'da` | `Farsçada` | Aynı. |
| isim-havuzu:311 | `İngilizce'de` | `İngilizcede` | Aynı. Not: aynı belge 296. satırda "İngilizce sürümde" yazarak doğru yapmış. |
| salon (3 yer) vs sehir (7 yer) | `...` / `…` | Tek karakter seçin | Salon dosyası üç nokta (`...`), şehir dosyası tek karakter üç nokta (`…`) kullanıyor. Bu metinler string dosyalarına girecek; iki ayrı karakter iki ayrı arama-değiştir demek. |
| sehir:26, 361 vs salon:82 | `19:00`, `15:00` / `11.00` | `19.00`, `15.00`, `11.00` | Saat gösteriminde nokta kullanılır; belge içinde iki ayrı biçim var. |

**Temiz çıkanlar (denetlendi, hata yok):** Soru ekleri istisnasız ayrı yazılmış (`var mı`, `içti mi`, `gördünüz mü`, `sen mi kırdın`, `soğuk muydu`, `açayım mı` — 24 örnek). "de/da" bağlaç–ek ayrımı istisnasız doğru (`sen de gör`, `ben de biliyorum`, `Defterdesin`, `öğlen de` — 13 örnek). Özel adlarda kesme işareti doğru (`Yenitaç'ta`, `Sarp'a`, `Nazlı'nın`, `Kırk Kese'nin`). Düzeltme işaretleri doğru: `kâğıt`, `kâtip`, `hâlâ`, `hâline`, `şikâyet`, `rüzgâr`, `dükkân`, `mekân`, `kâr`, ve doğru biçimde işaretsiz `lazım`. Repliklerdeki sayılar istisnasız yazıyla, anlatımdakiler rakamla — tutarlı.

---

## B. AĞIZ İHLALLERİ

Kartın kendi "SES / AĞIZ" veya "NE YAPMAZ / NE DEMEZ" tarifine aykırı replikler.

| Karakter | Kartın kuralı | İhlal eden replik | Önerilen replik |
|---|---|---|---|
| **Şirin** *(en ağırı)* | §10: *"odada asla 'beyimiz şunu istiyor' demez; o cümleyi kurmadığı için Sarp'ın gözü onu yakalayamıyor."* Sarp'ın kartı da bunu doğruluyor: *"çünkü Şirin onun adına bir şey istemiyor."* | BARK 1: *"Bey dinlenmek istiyor. Bugün olmaz."* — Sarp adına bir istek beyan ediyor. İki kart birbirini yalanlıyor ve buna bir mekanik bağlı. | *"Bugün olmaz. Hekim öyle söyledi."* — "bugün" tiki durur, istek Sarp'a değil Yusuf'a bağlanır, kural ekranda görünür hâle gelir. |
| **Gökçe** | *"Kendinden bahsederken 'ben' yerine iş adı kullanır: 'burada duran adam', 'ip tutan'."* | Beş repliğin üçünde dört kez "ben": *"…**ben** susuz adamla konuşmam"* / *"…**ben** gördüm, sen de gör!"* / *"**Ben** demedim. Yine **ben** demiş oldum."* İş adıyla kendine gönderme hiçbir replikte yok. | *"…konuşacak bir şeyimiz yok, susuz adamla konuşulmaz."* / *"Per! Sağ yaka tüylenmiş. Yukarıdan görünüyor, sen de gör!"* / Üçüncüsü kasıtlı bir kırılmaysa (konu zaten "benim adıma konuşuyorlar") karta **not düşülmeli**; değilse: *"Demedim. Yine demiş oldum."* |
| **Vedia** | *"Kimsenin unvanını kullanmıyor: Rüstem, Sadık, Yusuf, Şirin — düz ad, ve o odada bu neredeyse şiddet."* | BARK 2: *"**Kâtip**, dördüncü tarihi yazarken yanına fiyatını da yaz."* | *"Sadık, dördüncü tarihi yazarken yanına fiyatını da yaz. Kırk gün, bin altı yüz akçe."* |
| **Vedia** | *"Rakam, şimdiki zaman, **sıfat yok**."* | DEĞİŞİM: *"Defterde olmak **iyi** bir şey değil, ama **kötü** de değil."* İki sıfat, bir de değerlendirme — üçü de yasaklı. | *"Altı akçe. Defterdesin artık. Defterden çıkmak altı akçeden pahalı."* |
| **Sadık** | *"Uzun cümle, yan cümlecik, ara düzeltme. Konuşurken kendi kendine dikte eder…"* | Beş repliğin üçü telgraf: *"Bu da düz adla gelmiş. Unvansız. Kayda geçtim."* ve özellikle DEĞİŞİM: *"Buz. Altı akçe. Bir kere. Devamı var mı?"* | *"Buz taşımışsın — altı akçe, bir kere, kayıt on dokuzunda; ikinci bir satırın yok, o zaman sorum şu: devamı var mı?"* |
| **Ragıp** | *"Tam cümle kurar, sonuna kadar."* | İLK CÜMLE: *"Bu iş görülmüş. Altı akçe. Elini uzat, sayacağım."* ve BARK 2: *"Defter açık. Sabahtan beri açık. Sıra bende değil, kasada."* | *"Bu iş görülmüş ve karşılığı altı akçe; elini uzat, sayacağım."* — kısa hâli asıl DEĞİŞİM satırında (mareşal cümlesi) zaten doğru kurulmuş, örnek orada. |
| **Kıraç** | *"**Soru sormaz.**"* | DEĞİŞİM: *"Sen indin mi? … İyi."* | *"İnmişsin. … İyi."* — "-miş" çıkarımı zaten onun kipi (*"Doru terlemiş"*, *"Kayış eskimiş"*), soru düşer, karakter güçlenir. |
| **Uçkun** | *"Kendini rütbeyle tanıtır çünkü elindeki tek şey o."* | İLK CÜMLE: *"Kışla. Kapısı bu. **Ben Uçkun**."* — rütbe yok, düz ad var. | *"Kışla. Kapısı bu. Yamak Uçkun."* |
| **Uçkun** | *"**'Bilmem' der, çok der.**"* (kartta koyu) | Sekiz repliğinin hiçbirinde "bilmem" geçmiyor. | En az bir barka konmalı — örneğin akşam barkının sonuna: *"Aşağıya kim indi? Kapağı kim açık bıraktı? … Bilmem. Benim işim kapı."* |
| **Yusuf** | *"'**Ücret**' kelimesini kullanmaz, 'hakkım' der."* | İLK CÜMLE: *"Üçüncüsü **ücretsiz** ve ben üçüncüsünü sevmem."* | *"Üçüncüsünden hakkım çıkmaz ve ben üçüncüsünü sevmem."* |
| **Balaban** | *"**Hiçbir şeyi doğrudan istemez**: ekmek isteyeceği zaman ekmeğin fiyatından bahseder ve bekler."* | BARK 2: *"Bacı, **kepçeyi dibe daldır**."* — doğrudan emir kipiyle yemek istiyor. | *"Bacı, üstü su, dibi yemek. Ben de kepçe tuttum, biliyorum."* — emir düşer, ima ve Bekmez'in kepçe sırrına yapılan gönderme kalır. |
| **Rüstem** | *"**Emir vermez, tespit eder.**"* | İLK CÜMLE'de iki emir arka arkaya: *"…**sıraya girme, doğrudan söyle**. Sıra beklemeye değmez."* | *"Bugün otuz iki kişi geldi, otuz ikisi de para istedi. Sen otuz üçüncüsün. Sıra beklemeye değmez."* — iki emir de düşer, cümle tespit olarak kapanır. |
| **Rüstem** | *"Tek soru tipi sorar: **'Kaç?'** — başka soru sormaz."* | DEĞİŞİM: *"Aşağısı soğuk muydu?"* | Bu kırılma **kasıtlı ve iyi** görünüyor (kartın kendisi "cevabı bekler" diyor) ama işaretlenmemiş. Ya karta bir cümle eklenmeli — *"Hayatında ikinci kez 'Kaç?' dışında bir soru sorar"* — ya replik *"Kaç çuval indirdin?"* olmalı. Şu hâliyle okuyucu hata sanar. |
| **Perçin** | *"Cümlelerinin çoğunda bir rakam var ve **rakamı cümlenin başına koyar**."* | İLK CÜMLE'de hiç rakam yok. BARK 1'de rakam başta değil: *"**Dün** yüz elli çivi saydım, bu sabah yüz on iki."* | *"Yüz elli. Dün yüz elliydi, bu sabah yüz on iki."* — imza kalıp ilk barkta kurulmuş olur. |
| **Perçin** | *"Emir kipi kısa, **açıklama yok** — 'Çekil', 'Tut', 'Bırakma'."* | İLK CÜMLE iki açıklama taşıyor: *"Elini uzatma, **ip geliyor**. Geçeceksen soldan geç, **sağ taraf benim sayımım**."* | *"Elini uzatma. İp geliyor. Soldan geç."* — açıklamalar kalkar, ses tarife oturur. |
| **Bekmez** | *"**Sıcak, hızlı, kesintisiz.** Herkese 'canım' der, gazilere 'aslanım'."* + *"ölçüyü tartıyla değil, avuçla söylüyor"* | Üç barkı da soğuk sayım: *"Yirmi iki. Kazan bana büyük gelmeye başladı."* Hiçbirinde "canım" yok, "aslanım" hiç geçmiyor, avuç ölçüsü hiç geçmiyor. | *"Yirmi iki canım, yirmi iki. Kazan bana büyük gelmeye başladı."* En az bir barka avuç ölçüsü girmeli: *"Bir avuç arpa bir kase eder, iki avuç bir adam doyurur."* — Yedisunak izi ancak böyle duyulur. |
| **Sekiz** | *"Kendi sesi: mırıltı, **yarım cümle, sonu getirilmez**."* | İLK CÜMLE'nin kendi-ses kısmı iki tam cümle: *"— ...Bana bakma, kâğıtta öyle yazıyor. Ben de yolumu değiştirmedim, bak buradayım."* | *"— ...Bana bakma. Kâğıtta öyle. Ben de değiştirmedim yolumu, bak…"* — BARK 2 (*"…davetler geçerlidir, kimse yolunu— davetler…"*) bu işin nasıl yapılacağının doğru örneği, ilk cümle ona benzemeli. |
| **Sekiz** | Okuma yazması yok (kartın merkezî sırrı). | BARK 3: *"**Bir:** hasat sonrası. **İki:** kırktan sonra. **Üç:** bey şifa bulunca."* — iki nokta üst üsteyle liste kuruyor; bu yazıya ait bir noktalama, okuyamayan adamın ağzına ait değil. | *"Biri hasat sonrasıydı. İkincisi kırktan sonra. Üçü bey şifa bulunca. Dördü… dördünü daha yazmadılar."* |
| **Sadık** | *"Her konuşmayı **aynı soruyla açar**: sebebi ne?"* | İLK CÜMLE'de "sebebi ne" en sonda, üç sorunun üçüncüsü. Konuşma "Bir dakika —" ile açılıyor. | Küçük; ya kural *"her konuşmayı aynı soruyla kapatır"* olmalı, ya replik sıralaması değişmeli. |
| **Şirin** | *"**Herkese unvanıyla sesleniyor** — Mareşal, Kâtip, Hekim, Sayman."* | Dört repliğinin hiçbirinde tek bir unvan yok. | Salon barkı ya da taşlık barkı bunu taşımalı; yoksa karakterin en ayırt edici işareti hiç duyulmaz. |

---

## C. GENEL TESPİTLER

### C.1 Ağız ayrımı: yarısı tutuyor, yarısı tek kalemden çıkmış

**Yan yana konduğunda kim olduğu anlaşılanlar (dokuz karakter):** Sabri, Nazlı, Kıraç, Sekiz, Rasih, Baran, Mahir, Balaban, Hafize. Bunların ağzı sözdiziminden değil **dünya görüşünden** çıkıyor — Kıraç insanları atlarıyla anıyor, Rasih insan sorulunca taştan bahsediyor, Nazlı sorulmadan yalan söylüyor, Mahir her cümleye bir malzeme koyuyor. Yöntem doğru ve dört karakterde kusursuz işlemiş.

**Birbirine karışanlar:** On iki slot aynı sözdizimsel kalıbı kullanıyor —

> **[Rakam]. [Dün/karşılaştırma rakamı]. [Kısa kapanış cümlesi.]**

| Karakter | Kalıbı kullanan replik |
|---|---|
| Perçin | *"Dün yüz elli çivi saydım, bu sabah yüz on iki."* |
| Kıraç | *"Buz iki çuval. Dün üçtü."* |
| Mahir | *"Kömür bir çuval geldi. Defterde iki yazıyor."* |
| Baran | *"Üç yüz kırk. Dün üç yüz otuz sekizdi. Kimse ölmedi, kimse ödemedi."* |
| Bekmez | *"Kırk kase kurdum, otuz altı geldi."* + iki bark daha |
| Hafize | *"Dokuz yüz. Dün dokuz yüz kırktı. Yarın dokuz yüz seksen…"* |
| Ragıp | *"Kırk iki kişi, otuz dokuz kalem. Fark üç…"* + *"Otuz dokuz kalem, otuz dokuz kişi…"* |
| Vedia | *"Mum otuz iki, kandil on dokuz, meşale yüz kırk."* |
| Rüstem | *"Bugün otuz iki kişi geldi, otuz ikisi de para istedi."* |
| Balaban | *"…dördüncüyüm. Sabah da dördüncüydüm, öğlen de."* |
| Sabri | *"Bugün pencereye baktı. Dün bakmadı."* (rakamsız varyantı) |
| Sadık | *"Buz. Altı akçe. Bir kere. Devamı var mı?"* |

Belgenin kendi niyeti bu kalıbı **dört ağza** ayırmış (`KADRO NOTLARI`: 9, 12, 17, 18 — kalfa, kâtip, aşçı, kesedar). Kalıp on ikiye sızmış. Sonuç: bu on iki repliği isimleri silip karıştırıp önünüze koysam, dördünü doğru eşleştiremezsiniz.

**En keskin çakışma: iki kâtip.** Sadık *"uzun cümle, yan cümlecik, ara düzeltme"* diye, Ragıp *"tam cümle kurar, sonuna kadar"* diye tarif edilmiş — yani ikisi de aynı şeyle tarif edilmiş, ve ikisi de tarifin tam tersini konuşuyor. Şu an Sadık'ın *"Bu da düz adla gelmiş. Unvansız. Kayda geçtim."* satırıyla Ragıp'ın *"Defter açık. Sabahtan beri açık. Sıra bende değil, kasada."* satırı arasında hiçbir fark yok. Salon kâtibiyle kışla kâtibinin ayrılması gereken yer tam burası. Öneri: Sadık'ın uzun-cümle kuralı **istisnasız** uygulansın (dikte anları dâhil, çünkü dikte de onun düşünme biçimi); Ragıp'ın "tam cümle" kuralı ise "kısa ama eksiksiz cümle"ye çevrilsin — ikisi zaten farklı iki şey.

**"Üç kısa cümle" ritmi:** yirmi karakterin on üçünde en az bir replik "X. Y. Z." biçiminde. Bu, LORE §15.4'ün yasakladığı *"üçleme ritmi"*nin ta kendisi. Yasak jenerik fantezi cümleleri için konmuş ama kural biçimsel: kadro şu an aynı üçlemeyi rakamla yapıyor.

### C.2 "On bir yıl" tiki

`on bir yıl` ifadesi üç belgede **19 kez** geçiyor. Bazıları zorunlu ve doğru (iskele 11 yaşında — Rasih, Baran, Mahir, Sarp'ın odası, Rasih'in getirilişi hepsi aynı olaya bağlı). Ama **Sadık'ın Yedisunak arşivi**, **Vedia'nın kese defteri**, **Perçin'in defterdeki adı** ve **Bekmez–Baran pazarlık şakası** 11 olmak zorunda değil, hepsi 11. Sayı bir kimlik taşıması gerekirken artık taşımıyor. Bu dördünden en az üçü başka bir sayıya kaydırılmalı — Sadık'ın on bir yılı özellikle, çünkü kartı *"bütün on bir yılı taşıyan direk"* diyerek sayıya ahlaki ağırlık yüklüyor ve o ağırlık şehrin inşaat takviminden çalınıyor.

### C.3 Kartın vaat ettiği, ekranın göstermediği imzalar

Yirmi kartın **on dördü** SES/AĞIZ bölümünde bir imza kelime/tik ilan ediyor ve o kelime karakterin hiçbir repliğinde geçmiyor:

| Karakter | İlan edilen imza | Repliklerde |
|---|---|---|
| Rüstem | *"Kaç?"* / kadınlara "kızım", adamlarına "evlat" | Hiçbiri yok |
| Şirin | Unvanla hitap (Mareşal, Kâtip, Hekim, Sayman) | Yok |
| Vedia | Bilmiyorsa *"bilmiyorum"* der | Yok (buna karşılık yasaklı unvan var) |
| Sabri | *"Bilmem"* der | Yok |
| Yusuf | *"hakkım"*, *"yaşar"*, *"kalır"* | Yok (buna karşılık yasaklı "ücretsiz" var) |
| Balaban | Dolaylı isteme (fiyattan bahsedip bekleme) | Yok (buna karşılık doğrudan emir var) |
| Rasih | *"Bekliyorum demez, duruyorum der"* | İkisi de yok |
| Gökçe | *"öyle"* (evet yerine) + iş adıyla kendine gönderme | Yok |
| Nazlı | Üçüncü tekile kaçma (*"Biri almış işte"*) | Yok |
| Uçkun | *"Bilmem"* (kartta koyu) | Yok |
| Kıraç | Mareşal = *"Bacaklı"* | Yok (yalnız "Doru" var) |
| Mahir | *"dengeli"* (güzel yerine) | Yok |
| Bekmez | *"aslanım"* + avuçla ölçü | İkisi de yok |
| Sekiz | Tellal sesinde *"herkesin unvanı tam"* | Yalnız "beyimiz" var, tam unvan yok |

Bu, kartların en sistematik açığı. Kart bir kural yazıyor, replikler o kuralın **negatifini** gösteriyor ("şunu demez") ama **pozitifini** hiç göstermiyor ("bunu der"). Oyuncu bir karakteri yokluklardan tanıyamaz. Öneri, ucuz ve mekanik: **her kartta SES/AĞIZ'da geçen her pozitif imza, o karakterin en az bir repliğinde fiilen geçmeli.** Yirmi kart için yaklaşık yirmi satırlık bir iş.

Ek olarak: **Sekiz'in ÜÇ BARK'ının birincisinde metin yok** (`salon:289` — *"Kışla avlusunda, aynı ilan, aynı tonda, kimse dönüp bakmadan."*). Sahne yönergesi var, replik yok. Karakterin iki barkı var, üç değil.

### C.4 Jenerik fantezi dili ve anakronizm: neredeyse kusursuz

**Repliklerde tek bir yasaklı kelime yok.** "Kadim", "fısıldayan", "kehanet", "kader", "seçilmiş", "karanlık bir güç" — yirmi karakterin hiçbirinin ağzından çıkmıyor. Bu belgelerin en güçlü tarafı ve kolay bir başarı değil.

İki küçük not:
- `slotlar:230` (anlatım): *"ikinci katmanın ilk **fısıltısı** olur"* — LORE §15.4 "fısıldayan" enflasyonunu yasaklıyor ve `isim-havuzu:260` bu yasağı gerekçe göstererek "Fısıldayan"ı reddediyor. Tasarım belgesinin kendi anlatımında kelimeyi kullanması küçük bir tutarsızlık; *"ikinci katmanın ilk çentiği"* yeter.
- Anakronizm taraması repliklerde temiz: "kategori", "prosedür", "argüman", "gösterge", "istasyon" — hepsi anlatımda kalmış, kurala uygun. Tek replik-içi risk Yusuf'un "ücretsiz"i (zaten B'de). Ayrıca **"kontrat"** dünyanın kendi terimi olarak kullanılıyor (`kontrat panosu`, `contract.c`); anlatımda sorun değil ama arayüzde oyuncuya görünecekse ortaçağ ağzında bir karşılık gerekir — *senet*, *tellal kâğıdı*, *pano kaydı*.

### C.5 Doğallık: yüksek sesle okunduğunda

**Ağızda dönenler (dokunmayın):** Kıraç'ın hepsi. Rasih'in *"Kırk gün. — Kırk gün kışa girer, kışta taş çatlar. Neyse."* — "Neyse" tek başına adamı anlatıyor. Balaban'ın *"Güldü, 'katır da olur' dedi. — Ben gülmedim."* Sabri'nin *"Bugün pencereye baktı. Dün bakmadı."* Hafize'nin *"Otur. Ayakta konuşanın ya acelesi vardır ya yalanı, ikisi de bana pahalıya gelir."* Baran'ın *"Kapatsam bir ay yerim. Kapatmasam bir yıl dururum."* Nazlı'nın hepsi.

**Kâğıt kokanlar:**
- Yusuf, BARK 1: *"Beyimiz konuşamıyor, **dolayısıyla** istemiyor."* — "dolayısıyla" yazılı dilin bağlacı, salonda söylenmez. Öneri: *"Beyimiz konuşamıyor. Konuşamayan adam istemez de. Hastanın iradesi hekimindir."*
- Vedia: *"Defterde olmak iyi bir şey değil, ama kötü de değil."* — aforizma tadında; sayman aforizma kurmaz.
- Sadık: *"Buz. Altı akçe. Bir kere."* — konuşma değil, fiş.
- Sekiz: iki nokta üst üsteyle kurulan liste (B'de).
- Rasih ilk cümle: *"…ikimiz de vakit kaybetmeyiz"* — biraz fazla düzgün, ama on bir yıldır aynı cümleyi kuran bir adam için savunulabilir; bırakılabilir.

### C.6 İsim havuzunun kendi kurallarıyla çakışan iki ad

- **Kıraç (slot 14).** `isim-havuzu:313` şunu yazıyor: *"Dikkat gerektirenler: Argın, **Kıraç**, Sırrı, Tırtıl, Pıtırak, Bürküt (ı/ü yığılması) — bunlar **ikincil kadroda kalsın, ana hat NPC'si olmasın**."* Oysa Kıraç ana hat: Karduvar hattının Yenitaç'taki ikinci ayağı, Gökçe'nin karşılığı, Şirin'in gece eyerlenmiş atının tek tanığı, 9. dakikanın ilk talebinin sahibi. Ayrıca §7'nin *"adda en fazla bir özel karakter"* kuralını iki karakterle (ı + ç) çiğniyor. Ya kural gevşetilmeli ya ad değişmeli — havuzun kendi Karduvar hava-adları kümesinden **Ayaz** veya **Boran** birebir yerine geçer, ikisi de özel karaktersiz, `.id` değişmez.
- **"Yarım Kuyu"** (Baran'ın kuyu adı, `sehir:292`). "Yarım" §4 lakap havuzunda **Tuzkapı lakabı** olarak ayrılmış (*"borcunu hep yarım öder"*), "Kuyu Dibi" ise **Kumkapı lakabı**. Ad iki ayrılmış jetonu birleştiriyor ve okuyucu Baran'ı Tuzkapılı sanabilir. §3'ün kendi Kumkapı örneği **"İkinci Kova"** aynı işi çakışmasız görür — ve borç temalı bir kartta daha iyi durur.

### C.7 İngilizceye çeviride kırılacak yerler

**a. Sen/siz'e dayanan üç an — İngilizcede sıfır.**
1. Uçkun, DEĞİŞİM: *"Geç. … Geçin."* Bu iki kelimede karakterin bütün utancı var; İngilizcede "Go. … Go." — hiçbir şey.
2. Ragıp, DEĞİŞİM: *"oyuncuya 'sen' demeyi bırakıp 'siz' der. Bu, incinmenin defter usulü hâli."* Bir karakter kavisinin tamamı tek bir zamir farkına yaslanmış.
3. Şirin, BARK 2: hizmetçiye *"Kızı gördünüz mü? Görmediniz."*

Üçü de İngilizcede yeni bir işaret ister — mesafeyi taşıyacak bir şey (unvanla hitaba geçiş, tam ad kullanımı, cümle uzunluğunun resmileşmesi). **Bu kararın çeviri aşamasına bırakılmaması gerekir; kartlara şimdiden ikinci bir işaret yazılmalı.**

**b. En pahalı kırılma: "beyimiz" / "bey".** Salonun bütün dilbilgisi tek bir iyelik ekine bağlı. Şirin'in ayırt edici tek özelliği *"Bey"* demesi, Sabri'nin ayırt edici tek özelliği *"bey"* demesi (kart: *"iyelik eki yok… çünkü kimseyle paylaşmıyor"*), Sarp'ın KAYBOLURSA'sı *"'beyimiz istiyor' kalıbını kimse kuramaz"*. İngilizce "our lord" / "the lord" ayrımını yapabilir ama görünmez kalır — Türkçede ek sessizce çalışır, İngilizcede iki fazladan kelime ve oyuncu farkı seçmez. Üstelik **iki ayrı karakterin (Şirin, Sabri) tek ayırt edici işareti aynı ek**; çeviride ikisi tek karaktere çöker.

**c. Ad kısaltma sistemi.** Per / Rag / Haf / Mah / Ved. Tuzkapı kuralı (*"bir adamın adını kısaltmak, adının mürekkebe değmediğini söylemektir"*) İngilizce oyuncu için tamamen görünmez — İngilizcede takma ad kısaltmak samimiyettir, hakaret değil. İki somut kaza: **"Rag"** İngilizcede "paçavra" okunur (bir kâtip için tesadüfen çok sert bir hakaret — kalacaksa bilinçli karar olmalı), **"Haf"** ise "half" gibi okunur ve Hafize'nin "Yarım" lakaplı bir tefeciyle karıştırılmasına yol açar.

**d. §7 ile LORE §15.5 birbirini kesiyor — karar verilmeli.** §7: *"Özel adlar çevrilmez."* LORE §15.5: *"İsim… ilk duyuşta anlaşılır."* Kadronun yarısının adı anlam taşıyor: **Sekiz, Bekmez, Perçin, Uçkun, Kıraç, Yaya, Kıymet, Yarım Kuyu, Cüruf, Soğuk Pota**. Çevrilmezlerse İngiliz oyuncu için anlamsız hece olurlar ve §15.5 düşer; çevrilirlerse §7 düşer. En keskin vaka **Sekiz**: adın tamamı bir sayı ve şaka *"sekizinci direk dikildiği yıl doğdu"*ya bağlı — "Sekiz" olarak bırakılırsa şaka hiç var olmaz, "Eight" olursa §7 çiğnenir. Öneri: §7'ye açık bir istisna maddesi yazın — *"anlamı olay örgüsüne ya da bir kültür kuralına bağlı adlar hedef dilde yeniden kurulur (Sekiz → Eight, Kıymet oğlu → son of Worth)"* — ve hangi adların bu istisnaya girdiğini şimdiden listeleyin. Bu karar ne kadar geç verilirse o kadar pahalıya mal olur.

**e. Kadına hitabın dört katmanı.** §2.10 dört ayrı hitap kuruyor: **Ana** (yetkiyle kazanılan), **Hatun** (kocadan gelen), **Bacı** (ordu/iskele, eşitleyici, "asılmıyorum" ilanı), **Kadın** (Tuzkapı teknik unvanı, adın arkasına gelir, "kendi mührü var" demek). İngilizcede dördü de "ma'am / madam / sister / mistress" havuzuna çöker ve sistem yok olur. En zoru **Kadın**: "Hafize Kadın" → "Hafize Woman" saçma; *"Hafize of the Seventh Purse"* gibi bir yeniden kurgu gerekiyor ve o zaman da Yenitaç'ın "Haf" kısaltmasının hakareti başka bir yere oturuyor. **Dört hitap için bir eşleme tablosu, ilk replik çevrilmeden önce yazılmalı.**

**f. Karaocak anne-adı kuralı.** Mahir'in barkı — *"Kıymet oğlu, kancayı ver."* — kadronun en sessiz darbelerinden biri. "Son of Kıymet" işi görür, ama **Kıymet = değer/fiyat** ve havuz onu *"her şeyin fiyatını bilir"* diye tanımlıyor: para temalı bir kadroda, adını imzalayamayan adamın adının "Değer'in oğlu" olması Türkçede bedava bir ironi, İngilizcede sıfır.

**g. Gökçe ile Kıraç'ın ortak ağzı.** İkisi de Karduvarlı ve ikisinin tek ortak işareti dil: *"kardaş"*, *"…he?"*, *"öyle"* (evet yerine). Bunlar sözlük anlamı değil **register** taşıyor. İngilizcede bir bölge ağzı seçilmeli ("aye", "eh?") ve seçim **iki karakter için de aynı** olmalı, yoksa hemşehrilikleri kaybolur. Ek bir nokta: `sehir:63` Gökçe'nin barkı *"Kar geliyor **he.**"* — kartın kendi kuralı *"…he?"* diyor, soru işaretiyle. Belgedeki tek örnek kuralın dışında.

**h. Yeniden hecelenmesi gereken replikler.** Nazlı'nın *"Mah-ze-ne buz ta-şı"* barkı hece yapısına dayanıyor; İngilizcede kelimesi kelimesine çeviri değil, hedef dilde yeniden hecelenmiş bir ilan gerekir. Aynı şey *"Kıçımın direği"* için: calque etmeyin, çocuk ağzına uyan bir küfür bulun (LORE'un "çocukça bozuk" tarifi korunmalı).

**i. Rasih'in adının eskimişliği.** §2.9 bütün bir katmanı *"adı duyunca bu adamın getirildiğini anlarsın"* üstüne kurmuş. Türk oyuncu "Rasih"in modasının geçtiğini duyar; İngiliz oyuncu hiçbir şey duymaz ve katman sessizleşir. Telafi ya söyleyişte (arkaik bir İngilizce ad havuzu) ya bir replikte kurulmalı — Rasih'in kendi ağzından değil, başka birinin ağzından.
