Kodu ve kanonu okudum. Aşağıdakinin tamamı mevcut beş okulun üstüne kuruldu; "aşağıda kimse yok" kuralına hiçbir yerde dokunmadım.

---

# BÜYÜ — BEŞ OKUL

## 1. ENVANTER (grep'lendi)

**Yıkım** en dolu okul: 23 düğüm (16 pasif, 2 keystone, 5 aktif) ve **11 büyü** — Ateş Topu doğuştan (bootstrap; yoksa XP kazanılamaz), Buz Novası/Yıldırım/Alev Zırhı/Büyü Kalkanı/Toprak Dikenleri ağaçtan, Yıldırım Pençesi/Buz Dikenleri/Hortum/Yılan Isırığı/Ateş Aslanı tomardan (`item.c:349-387`, 350-750 gümüş, Yıkım 15/20/30/40/50 kapıları). Hepsi mana. XP büyü hasarından (`main.c` yedi ayrı yerde).

**Kutsal** 17 düğüm, ağaçta aktif yok, üç büyü tomardan geliyor (`ITEM_TOME_HEAL` reqLevel **0** — yani okulu hiç açmamış biri de okuyabilir; bunu bilinçli bir kapı olarak kullanacağım). XP şifadan (`healed*1.2f`) ve kutsal hasardan. `mob.h:91` undead'e ×1.6.

**Yankı** 16 düğüm, tek keystone, dört beceri (Fısıltı Patlaması, Gölge Formu, Can Emme, Yankı Çağırma), bedel `hpFrac` — kod düzeyinde tek CAN'la ödeyen okul. Sınıf satırı ritüele kadar gizli (`skill_set_echo_visible`).

**Çağırma** 19 düğüm ama **yalnız iki varlık** (Öz Yılanı doğuştan, Öz Kabuğu ağaçtan). XP dostun verdiği hasardan.

**Asa zayıf halka: 18 düğüm, SIFIR aktif.** Tamamı pasif (mana havuzu, rejen, asa melee). XP asayla melee vurmaktan geliyor (`combat.c:27`) — yani "tam büyücü silah hattı" oyuncuyu asayla **dövüşmeye** zorluyor. Kurgusal cevabımın yarısı bu boşluğu dolduruyor.

`spellsLearned` 64 bitlik, 36 dolu → **28 boş yer.** Yeni büyü eklemenin tavan maliyeti yok. Asa tarifi zaten kanona hediye: 2 Odun + 1 **Soluk Öz**, demirci tezgâhı, Smithing 5 (`craft.c:141`). Rün 3 öz istiyor (`craft.c:88`).

---

## 2. BÜYÜNÜN KAYNAĞI

Tek cümle: **büyücü kor yakmaz, koru soğutur — ve soğuyan kor susar.**

Elinde bir kor parçası olmadan büyü yapamazsın; ama her büyüde onu tüketmezsin de. Bir büyü, korun ısısını bir anda dışarı çeker. Parça kararır, sonra kendi kendine yeniden ısınır. Kodda mana havuzu ve rejen tam olarak bu: **mana çubuğu senin korunun ısısıdır.** Mana İksiri bir körük şişesi. Bilgelik Taşı, ısıyı daha derine gömen bir kalıp.

Buradan bedava ve zalim bir sonuç çıkıyor: **manası boşalan büyücü kendi gecesini söndürmüştür.** Kor susmuşsa çevredeki ölü adını yeniden duyabilir. Bir büyücü dövüşü kazanıp mana çubuğu dipteyken karanlıkta durduğunda, kazandığı dövüşün ödülünü kaybeder. Kandille büyü aynı yakıtı yakıyor ve aynı sesi çıkarıyor.

Ses konusu: **evet, büyü duyulur.** Kanonda ses her şey; büyü de bir istisna değil. Ama batıda deyim büyücü için tersine döner. Kılıcın karnındaki kor kimsenin gecesini aydınlatmaz — büyücünün eli ise bir saniye boyunca bütün mahalleyi aydınlatır, sonra karanlık geri gelir ve **gürültü kalır.** Pyrewright'ların sözü: *"Bir büyü bir gece değildir."* Kandil sabaha kadar susturur; büyü bir kere bağırır. Yani büyücü korun iki yüzünden çoğunlukla sadece birini alır: **ifşa.** Tek istisna Kutsal Alan, ve bütün Kutsal okulunun kıymeti oradan geliyor.

Kodla çakışmayan iki ekleme, ikisi de mevcut desenin kopyası: **[YENİ MEKANİK, ucuz]** her büyü atımı maymuncuk gürültüsü bloğunu (`main.c:1911-1925`, yarıçap 260, `suspicion += 0.65`) tekrarlasın — Ateş Topu 200 px, Yıldırım 500, Kutsal Alan 700. Ve atım anında bir saniyelik `light_append_point` (`light.h:33`, fonksiyon hazır). **[YENİ MEKANİK, orta]** gece + mana %20 altı + yakında mezar yuvası → uyanma şansı.

---

## 3. OKULLARIN KANONDAKİ YERİ

### Yıkım — kaçak yakıcı
Ateş burada kutsal ve pahalı çünkü yakma çukuru on bir saat doğru perdede yanmak zorunda. Bir Ateş Topu cesedi yakmaz, **kavurur** — ve yarım yanmış ölü kül sayılmaz. Yani büyücü Pyrewright'ın işini yapamıyor... bir tanesine kadar. Bir büyücü *bir* cesedi, bir saatte, bedava bitirir. Bunun kimin ekmeğini eldiğini görmek için Brannock Teel'in fiyat listesine bakmak yeterli: baş üç gümüş, çocuk bedava. Yirmi altı haneli bir köy hesabı yapar. Lonca da hesabı yapar ve büyücüye bir isim takar: **kaçak yakıcı.** Ruhsat numarası defterden çıktığı için (Söndürücü sözleşmesiyle aynı defter) numarasız yakan adam hukuken suçlu, teamül olarak da uğursuz.

Ama Yıkım büyücüsünün siyasi ağırlığı bundan büyük. Coldhearth'in bütün savaş planı tek bir cümlede duruyor: karanlık ucuz. Bir menzilde büyü bilen bir adam varsa karanlık ucuz olmaktan çıkar — ateşi olan köy üçüncü gecede diz çökmez. Bu yüzden Yol Vekilliği'nin karartma ekipleri fitil ustasını değil, **köyün büyücüsünü** önce arar: ya satın alır, ya öldürür, ya iftirayla sürdürür. Bilezik Savaşı'nın en somut sahnesi bu: oyuncu Kırk Dört'e vardığında meydanda yanmış bir el arabası ve duvarda tebeşirle bir çizik bulur.

### Asa — bir ağız
Asa silah değil. Asa, taşıdığın korun senin yerine konuşmasını sağlayan şey. Ucundaki oyuğa bir **öz** gömülür ve kanon bunu zaten söylüyor: rün bir harf, öz bir hece. Yani her asanın içinde tek bir hece var, ve iyi asa iyi heceye sahip olandır.

Yapanlar: Kilnmoor'un Ocakdoğan çırakları (özü çıplak elle tutabilen tek halk) ve Saltmere'in ıslak defter kaçakçıları. İkinci grup asıl mesele — **tuzlu asa hece söyler.** Tuzda kalmış öz yanlış perdeden çıtırdar; o asayla gece büyü yapan adam, farkında olmadan bir hece okumuş olur. Kimse çağırmıyor, kimse ceza vermiyor: sadece yanlış frekans. Terk edilmiş menzillerdeki Taklit kuşuyla aynı mekanik — ne yaptığını bilmeyen bir ses. Ekranda: yeşilimsi bir çıtırtı, sonra yakındaki taşın harflerinin ısınması.

Asa Yolu'nun sıfır aktifi olması artık bir eksiklik değil, kimlik: **Asa okulu taşıma okulu.** Ağaçtaki her düğüm mana havuzu, rejen ve tutuş — yani "koru ne kadar sıcak taşıyorsun". Yine de iki aktif önereceğim, ikisi de kanonun kalbinden: **Ocak Vuruşu** (asa dürtmesi + kısa ışık patlaması — ölüyü bir saniye yatırır) ve **Kısık Ağız** (duruş: 10 saniye büyülerin ses çıkarmaz, mana maliyeti ×1.5). İkinci beceri `ABILITY_GATE_STANCE`'in aç/kapa desenini birebir kullanır ve oyundaki en kanon-yüklü butondur.

### Çağırma — hayvanın işi
Burası tehlikeli yer, o yüzden net olacağım. Çağırma ile ad okuma **aynı fiil**: yüksek sesle bir şey okumak. Farkı okuduğun şeyin ne olduğu. Ad, yol taşına yazılmış bir insanın kaydıdır. **Öz ise kimsenin adı değil — bir hece.**

Ve hecenin karşılığı şu: imparatorluk hayvanlarını kayda geçmedi, o yüzden hayvan adıyla çağrılmıyor. Ama hayvanın bir **işi** vardı, ve iş bir hecedir; kurgan tarlasında, cüruf gölcüklerinde, totem diplerinde taş gibi kalmış hâlde bulunur. Çağırıcı o heceyi ısıtır. Ortaya çıkan şey emir almaz, sadıklık göstermez, aşağıdan gelmez: **öldüğünde ne yapıyorduysa onu yapar.** Yılan ısırır. Kaplumbağa bekler — beklemek onun işiydi, o yüzden üstüne çekiyor. Kodda dost varlıkların oyuncunun etrafında kalması da bedava açıklanıyor: ortalıkta en sıcak şey senin korun.

Yani ölülerle aynı tek kural, hayvana uygulanmış hâli. Yeni bir metafizik yok, irade yok.

Ve ikisinin birleşme noktası bir tehdit olarak duruyor: elinde tuzlu öz ya da bir kurgan defteri sayfası olan çağırıcı, hece yerine **ad** okuyabilir. O gece yılan değil bir adam kalkar. Bu bir zincir düğümü, ceza değil — ve yüksek seviyeli bir çağırıcının Coldhearth için neden bir hanenin ordusundan değerli olduğunu açıklıyor.

### Kutsal — taşınabilir kandil
Kutsal Alan altı saniyelik bir kandildir ve **aynı bedeli tam olarak öder.** Ölüyü bastırır; kervanı, devriyeyi, kolcuyu, ejderhayı ve yolun üstündeki her karaltıyı kendine çevirir. Kutsal büyücü mahallenin gecesidir, ve mahalle onu görür.

Pyrewright'ların iki okulun da hocası olması bundan çıkıyor. Onların gerçek zanaatı ateş değil **ölçü**: çukurun on bir saat doğru perdede yanmasını sağlamak. Halk buna kutsallık diyor; Brannock buna ölçü diyor ve bir zanaatkâr olduğu için ısrarla düzeltir. Aynı ölçü şifayı da açıklıyor — çukur başında çalışan adam yanığı, kanın soğumasını, eti sarmayı bilir; ortada tanrı yok, ısı var. Lonca Kutsal'ı **öğretir** çünkü müşterisini artırır: çukurun ağzını tutabilen bir Kutsal büyücü Brannock'un gece yirmi adım geride durmasını mümkün kılan adamdır. Yıkım'ı öğretmeyi ise reddeder. Aynı hocanın verdiği ve vermediği ders.

Somut kanca, kodda hazır: `ITEM_TOME_HEAL` reqLevel 0. Brannock, çukurun ağzını üç gece tutan oyuncuya Şifa Nuru tomarını verir. (Kodda hediyenin sahibi eski MVP'den kalma "Fatma" — devredilecek.)

### Yankı — kulak
Yankı hiçbir okulun rakibi değil, hepsinin kulağıdır. Kor gürültüsü ölünün adını bastırıyor; Yankıcı kendi kanını kısıp bastırılanı duyuyor. Duyduğu şey bir varlık değil, taşın geri verdiği kayıt — bu yüzden bedeli mana değil CAN, ve bu yüzden kodda tek `hpFrac` okulu olması doğru.

Değeri şu: kurgan defteri olmayanın defteri odur. Coldhearth defter satın alıyor; bir Yankıcı bulduğunda satın almıyor, **kaçırıyor.**

---

## 4. HATLAR

**Ocakçı** — Kutsal + Bitkicilik + Konuşma (şövalye varyantı: Kutsal + Blok + Ağır Zırh, "çukur ağzı"). Taşınabilir kandil. İşe alan: Pyrewright'lar, kervancılar, ve asıl müşteri Dimmarch'ın dokuz menzili — hem batıdan hem doğudan fitil parası alan, kimsenin sahiplenmediği yerler. İlan anı: kandili sönmüş bir köyün meydanında Kutsal Alan'ı sabaha kadar üç kez yenilemek; ertesi sabah köy senin adını sorar, çünkü sokakta ölü kalmamıştır. Zayıflığı acımasız: alan 6 saniye, 40 mana. Sabaha kadar tutmak matematiksel olarak imkânsız, yani Ocakçı **hangi sokak** diye seçmek zorundadır ve seçmediği sokak sabah onu bekler.

**Kısık Ağız** — Asa + Gizlenme + Yıkım. Büyü yapan duyulur; bu adam duyulmadan yakar. Aleti sağır kor ve asa duruşu. İşe alan: Söndürücüler (boğma kutusuna gerek bırakmıyor), Sessiz El, ve gizlice Coldhearth'in karartma ekipleri. İlan anı — Fitilci Nell Harrow yolda önünü keser: *"Otuz Dördüncü'nün direğini sen söndürdün. Ses yoktu. Bunu yapabilen üç kişi vardı, ikisi öldü."* Zayıflığı hattın kendi cezası: sessizlik ×1.5 mana. Kısık Ağız gece yarısı elinde soğuk bir korla kalır, ve tam o anda ölü onu duyar.

**Ad Okuyan** — Çağırma + Yankı + Büyüleme + Konuşma. Heceyle başlar, adla biter. İşe alan: Kuzey Yürüyüşü'nde nişan almak isteyen Coldhearth; tam tersi sebeple Kalkan Nöbeti (kimin okuduğunu bilmek istiyor); ve Tibb gibi defter tüccarları. İlan anı bu oyunun en iyi ekran görüntüsü olabilir: höyük tarlasında bir kolun önüne geçip yüksek sesle bir ad okumak. Sen duruyorsun, dokuz yüz ölü **dönüyor.** İki zayıflığı var: yüksek sesle okuyanın adı da okunmuştur (höyük iti seni özellikle arar), ve okuduğu ad tükenir — silinen ad bir daha çağrılmaz, defter kapanan bir cephaneliktir.

**Sunak Okuyan** kısa bir hat: Büyüleme + Asa + Konuşma. Harf basar. Ocakdoğanlar, çukur kurulları ve kılıcına harf isteyen Harrowgate süvarisi müşterisi. Slake'in tek repliği zaten iş sözleşmesi: *"Sunağa git, yak, gel."*

---

## 5. BÜYÜ NEREDEN ÖĞRENİLİR

Akademi on iki yıl önce kapandı, o yüzden hoca yok — **satıcı** var. Bir tomar 300 ile 750 gümüş arası, yani iyi bir atın iki katı; Emberhold arşivinden yağmalanmış tek bir sayfa. Pyrewright'lar Kutsal'ı ruhsat karşılığı öğretiyor (çukurun ağzını üç gece tut). Ocakdoğanlar asa ve rün öğretiyor, ücreti sunakta yakılan bir öz. Selde Mott, Saltmere'in batmış liman kütüphanesinden ıslak sayfalar çıkarıyor ve bütün ilişki tek kelime: ipi tut. Külhan'ın Kuyu Gözleri batıdan getirilen her doğu cildine bir tomar veriyor — bu yüzden batıda "büyücü doğudan gelir" önyargısı var, ve önyargı bir bakıma doğru.

Tek gerçek hoca akademinin son okutmanı; şimdi Otuz Dördüncü Menzil'de yazıcılık yapıyor ve ders ücreti para değil, **bir hece.** Getirdiğin özleri ne yaptığını söylemiyor. [Karakter-yazarına bırakıyorum.]

Suç mu? **Büyü suç değil, gürültü suç.** Batıda gece köy içinde büyü yapmak menzil teamülünde *kandil bozmak* sayılır — tanık varsa itibar cezası (kodda tanık-temelli suç zaten var). Gündüz serbest, hatta gösteri. Doğuda hepsi serbest, çünkü doğuda kimse kalkmıyor. Ruhsatsız ceset yakmak ise gerçekten suç.

---

## 6. BEŞ ROL

Savaşçı bir tomar okur ve iki butonu olur: Alev Zırhı ve Kutsal Hüzme — çünkü kurgan tarlasında kılıç kâr etmiyor, undead ×1.6 kâr ediyor. Şifa Nuru'nun kapısı reqLevel 0 olduğu için okula hiç girmemiş adam da girebilir; bu bir hata değil, kapı. Suikastçi Yankı'yı alır (bedel can, stamina değil — hançerle çakışmıyor) ve Kısık Ağız hattına oturur. Tüccar büyü öğrenmeden büyünün ekonomisine girer: öz ve tomar taşır, asa döver, Ocakçı kiralar, ıslak öz satana kefil olmaz. Şövalye Kutsal'ı alır — Işık Taşıyıcısı keystone'u (alanda blok +%20) kalkanlı adam için yazılmış — ve karanlık bir köye ışıkla girer. Büyücü hepsini.

---

## 7. NE BOZULUYOR, NE GEREKİYOR

**Sıfır maliyet:** mana = korun ısısı; asa = odun + hece (tarif var); rün = harf; öz = bir hayvanın işi; Çağırma'nın kanona oturması; tomar ekonomisi; Kutsal Alan = taşınabilir kandil; Pyrewright = hoca; büyü suçunun tanık/itibar sistemine binmesi; Asa okulunun aktifsizliğinin kimliğe dönüşmesi.

**Ucuz yeni:** büyü gürültüsü (`main.c:1911`'in kopyası, ~15 satır); atım anı ışığı; düşük manada gece uyanma riski.

**Pahalı yeni:** Kısık Ağız duruşu ve Ocak Vuruşu (iki aktif — 28 boş bit var, tavan sorunu yok); ad okuma fiilinin Kol'u döndürmesi (bestiyerin zaten [YENİ] işaretlediği gezici olay-mob sistemiyle aynı kalem); tuzlu asa/öz yan etkisi.

**Kendi işaretlediğim çelişkiler:** (1) Kanon "ışık kalkmayı engeller, kalkmış olanı durdurmaz" diyor (04-npc, Cardew) ama Kutsal Alan kalkmış ölüye tick hasarı veriyor. Kanonu düzeltmeyi öneriyorum: *ışık kalkmışı durdurmaz, yakar.* (2) Yankı'nın kurgusal özü **duymak**, XP kaynağı ise hasar — uyuşmuyor. Idris'in taş dinleme mekaniği Echo XP'si vermeli. (3) `ability.c:160`'ta Can Emme ve Yankı Çağırma "KUTSAL okulu" yorumunun altında duruyor ama `SKILL_ECHO`'ya bağlı — yorum yanlış, kod doğru. (4) `ITEM_TOME_HEAL`'in vericisi eski MVP karakteri.

**[AÇIK]:** Ateş Topu'nun doğuştan açık olması kurguda ne demek — herkes bir kıvılcım yakabiliyor mu, yoksa oyuncu bunu prologda mı öğrendi? · Ocakdoğanların çıplak elle öz tutması onları Çağırma'da doğuştan usta yapıyor mu (ırk bonusu kapısı) · sağır korun mana rejenini kesmesi gerekir mi (kurguya göre evet, dengeye göre bilmiyorum).