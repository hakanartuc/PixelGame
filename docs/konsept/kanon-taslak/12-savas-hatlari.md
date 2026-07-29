Okumam bitti: rol tanımı, iki kanon dosyası, `skill.h`, `skilltree.c` (yedi savaş ağacının tamamı), `ability.h`, `perk.h`, `item.h`/`item.c`, `mob.h`/`mob.c`, `combat.c`, `title.c`, `dialog.h` ve `main.c`'nin ilgili blokları.

---

# SAVAŞ SKILL'LERİ VE HATLARI — "Sönen Kandiller"

## 1. ELDE NE VAR (grep'lendi)

Yedi savaş skill'i de tanımlı ve altısının gerçek XP kaynağı var: silah skill'leri hasardan (`combat_xp_for_hit`), zırh skill'i **yenen** hasardan (`main.c:9530-9535` — kuşanılan her parçanın `armorClass`'ı için `dmg*0.15`), Block hem pasif blok hem Kalkan Vuruşu'ndan (`main.c:651`, `6478`). Ağaç puanı her 4 seviyede 1, yüzde tavanı 25 puan; ağaçlar 28-38 puan istiyor, yani hiçbiri tamamlanamıyor — kıtlık gerçek.

Ağaç doluluğu (tavan 32): Tek El 20, Yay 23, Çift El 18, Hafif 18, Orta 18, Ağır 18, **Kalkan 16**. Aktif beceri açan düğüm sayısı: Yay 5, Tek El 2, Çift El 3 (biri keystone-duruş), **Kalkan 1**, üç zırh ağacının hiçbirinde aktif yok. `ABILITY_COUNT` 36/64, hotbar 32 slot — beceri bütçesi rahat.

Sorunlu üç şey, sırayla:

**Orta Zırh ağacı öksüz.** `item.c`'de `ARMOR_MEDIUM` işaretli tek eşya yok — üç ağır (demir miğfer/zırh/pantolon), üç hafif (deri başlık/zırh/bot), orta sıfır. On sekiz düğümlük "Denge Yolu" ve iki keystone'u (`YAŞAYAN KALE`, `FEDAİ`) oyunda hiçbir zaman XP kazanamıyor. Zırh üçlüsünün üçte biri kâğıt üstünde.

**Çift El ağacının adı "Mızrak Eğitimi".** Tek `WEAPON_TWO_HAND` eşyası `ITEM_SPEAR`; düğüm metinlerinin yarısı "mızrak menzili", "hat vuruşu", "akıcı dürtüş" diyor. Balta ya da çift elli kılıç isteyen oyuncuya dünya *bir alet dayatıyor* — GDD §1.1'in doğrudan ihlali. Eldivende hiç eşya yok, iki set de eksik (ağır setin botu/eldiveni, hafif setin pantolonu yok).

**Kutsal hasar yalnız büyücünün.** `undead` bayrağının ×1.6'sı sadece `ABILITY_HOLY_BEAM` ve Kutsal Alan'da uygulanıyor (`main.c:7282`). Düşmanının çoğu ölü olan bir dünyada kılıçlı adamın ölüye karşı **hiçbir** çarpanı yok.

Bir de sevindirici keşif: `title.c`. On bir davranış ünvanı, gizli sayaçlardan kendiliğinden açılıyor, biri "taşınıyor" (pasifi ×2) ve **oyuncunun adının altına yazılıyor** (`main.c:11098`). Hat ilanı için ayrı sistem yazmaya gerek yok — sancak zaten kodda.

---

## 2. KANONUN SAVAŞA SOKTUĞU ÜÇ SORU

### Işık: bir kadran, iki yönü

Kod bu işi neredeyse hazır bırakmış. `main.c:9543`'te algı çarpanı zaten bir zincir: gece ×1.5, hava (`weather_percept_mult`), perk düğümleri, eğilme. Zincire tek terim daha girecek — **taşınan ışık**. Şu an meşale bedava: envanterde durduğu sürece oyuncu ışık taşıyor (`main.c:10943`) ve hiçbir bedeli yok.

Kor çeliğinin kadranı şöyle çalışsın. Kor çeliği silah (`ItemType`'a bir `emberGlow` bayrağı) gece hasarını +%18 artırır, algı çarpanını ×1.6 yapar. Silahı **kına sokmak** hotbar'dan bir tıkla — Söndürücü'den yürütülen **boğma kutusu** bunu yapan eşya, `trapItem`/`consumable` kalıbı hazır. Kına sokulmuş kor çeliği ne parlar ne ağır vurur.

Oyuncu bunu **kimsenin anlatmasına gerek kalmadan** öğrenir: goblin kampı çalınmış korla dolu olduğu için haritanın en aydınlık yeri, ve o kamp sürekli baskın yiyor. Sonra ilk gecesinde kor çeliğiyle yolda yürür ve ekranın kenarından bir şey ona **döner**. Üçüncü ders Söndürücü leşinden çıkan kutunun tooltip'i: "kor sessizce ölür."

Düğüm karşılıkları: Ağır Zırh'a **"Kapalı Kın"** (ışık cezası yarıya — çünkü lejyon artığı kını nasıl kapatacağını bilir), Hafif Zırh'a **"Karanlığa Alışkın"**ın devamı olarak *ışıksızken* gece hasarı +%10, Tek El'e **"Söndür ve Vur"** (kını kapattığın ilk 3 saniye saldırı aralığı -%20). Kavgayı çabuk bitirme baskısı buradan geliyor: kor çeliğiyle vurmaya başladın, artık iki mil öteden görünüyorsun, sayacın işliyor.

Ağır zırhın gece ses çıkarması ayrı kod istemiyor — `main.c:7951`'de zırh **ağırlığı** zaten hesaplanıyor (ağır 5 / orta 2 / hafif 1, STR telafi ediyor). O `weight` değerini algı zincirine bağlamak bir satır.

### Neyi öldürmeyeceğini bilmek

Bunun bir kısmı mob davranışı, bir kısmı **bilgi**, ve şaşırtıcı olan şu: XP kanalı zaten var. `main.c:9651` — eğilmiş, fark edilmemiş ve algı sınırına yakın oyuncuya Gizlilik XP'si akıyor. Kol'un `detectRange`'ini yüksek verirsen, bir kolonun yanından eğilerek geçen oyuncu **dövüşmeden** ilerliyor ve karşılığını alıyor. Sıfır kod.

Duran için Kalkan ağacına düğüm: **"HAT BİLGİSİ"** — bir Duran'a bir kez yaklaştıktan sonra savunduğu hat yerde soluk bir çizgi olarak görünür. Yerde bir çizgi çizmek küçük iş, ama oyunun anlamını değiştirir: kalkanlı adamın gördüğü şey, kalkansız adamın görmediği şey. Ve pahalı bir kısayol vermiyor — Duran'ı öldürmüyorsun, etrafından dolaşıyorsun.

Kol'un ön safını kırınca durması **kolun kendi davranışı** kalsın (bestiyer öyle diyor, doğru diyor). Perk yalnız süreyi uzatsın: Çift El'e **"Sıra Bozan"**ın üstüne bir dikkat çekici düğüm, kolun bocalama süresi +3 sn. Böylece kervanı geçiren adam kolu *öldürmüyor*, **duraklatıyor** ve kervan geçiyor. Kontrat panosu bunu ödüllendirebilir.

### Ölü düşmanın fiziği

Ölüler yorulmaz, korkmaz, dağılmaz. Bu bir veri kararı: ölü yuvalarında `NEST_CHIEF` ve `scatter` **kullanılmasın**. Reisi öldürünce çöken kamp goblinin işi; kolun reisi ölünce kol dağılmaz, düzeni tutar.

Formasyona ne işler? Kod net cevap veriyor. Mızrağın `armorPierce` 0.25'i ve hat vuruşu (`lineTargetBonus`, toplam 3 hedef) lejyon zırhlı bir sıra için biçilmiş kaftan. Yay için tuzak var: Kol kovalamadığı için menzil bedava görünür, ama Duran/Kol'un yüksek `defense`'i ucuz oku yer — **Demir Ok** (`ammoDmgMult` 1.3) ve kemik oku ekonomisi nihayet gerçek bir sebeple var oluyor. Yapışkan Tuzağı (`ITEM_STICKY_TRAP`) alan yavaşı, kolun "düzeni tut" davranışıyla üst üste bindiğinde kolonu yolun ortasında dondurur. Hepsi mevcut sistem.

Savaşçının kutsal kapısı **rün** olsun, büyü değil. `ITEM_RUNE_PINK`/`ITEM_RUNE_POISON` kalıbı var, `fx` instance'ta duruyor: **Kor Rünü** ekle, undead hedefe ×1.35. `combat_player_hit` içine bir dal. Böylece kılıçlı adam ölüye karşı silahsız kalmıyor ve bunu **demirciden/sunaktan** alıyor — yani bir başka oyuncu hattından.

---

## 3. SKILL'LERİN DÜNYADAKİ SAHİBİ

**Tek el** batının en yaygın silahı ve sebebi asalet değil, kandil direği: bir elin serbest kalması gerekiyor. Fitil ustası, kolcu, menzilci muhafızı — hepsi tek el taşır çünkü öbür el ya feneri ya kepçeyi tutuyor. Kılıç ustalığı ücretli kolculardan ve menzil evlerinin avlusunda para karşılığı ders veren emekli lejyonerlerden öğrenilir.

**Çift el** Kilnmoor'un çukurunda öğrenilir ve orada mızrak sevilmez — çukur dar. Ağacın "Mızrak Yolu" adını **"Ağır Silah"**a çevirin, hat vuruşunu ve delici bonuslarını mızrağın malı olarak bırakın, ama çift elli kor çeliği baltasını ekleyin. Ocakdoğan çukur kurulları çift el eğitimini para karşılığı vermez, borç karşılığı verir.

**Yay** kuzeyin işi. Kalkan Nöbeti üç bin kişi ve höyükler arasında yaşıyor; okla vurmak, karanlıkta kalkmayacağın mesafeden vurmak demek. Ruhsat satan halk, ruhsatı okçularla koruyor.

**Kalkan** en politik skill. Çukur ağzını tutmak, kapıyı dışarıdan beklemek, bir hattı geçmemek — Kapı Nöbeti yirmi yıldır bunu yapıyor ve dört ayrı güç para ödüyor. Ağaç en zayıf ağaç (16 düğüm) ve tam da güçlendirilmesi gereken ağaç.

**Ağır zırh** lejyon artığının. Gece ses çıkarır, sabah donmaz, ve hiçbir kolcu ağır zırhlı adamdan yol parası istemez. **Hafif zırh** fitilcinin ve höyük yağmacısının; kışın gece dışarıda kalmak için değil, sabaha dönmek için giyilir — bir kış gecesi hafif zırhla dışarıda durgun kalmak stamina yenilenmesini kessin. Mevsim sistemi var, soğuk sistemi **[DOĞRULANMADI]**, bu ekleme küçük-orta. **Orta zırh** Dimmarch'ın zırhı: pullu deri üstüne demir plaka, ne lejyon ne köylü, dokuz menzilin dolabında bulunur. İki eşya (göğüs + başlık) o on sekiz düğümü açar.

---

## 4. BEŞ SAVAŞ HATTI

**AĞIZ TUTAN** — Kalkan + Ağır Zırh + Konuşma (kontrat) + biraz Bloklu duruş.
Yakıcılar gece çukurdan korkuyor; kimse yakılan cesetlerin başında karanlıkta durmak istemiyor. Gecelik dört gümüş, çukur ağzını tutacak adama. İşe alan: Pyrewright çukur kurulu, Kapı Nöbeti, ve höyük tarlasının güney kenarında kazı yapan herkes. Açılan kapı: **ruhsat numarası**. Numara defterden çıkıyor, yani çukur bekçisi olan adam bir kâğıt zincirinin içine giriyor ve o zincirin öbür ucunda deftertutan var.
İlan anı: üçüncü sözleşmeden sonra çukur kurulu sana **kendi** kepçesini verir. Kepçe bir alet değil, bir işarettir; onu kemerinde gören köylü seninle daha az konuşur ve daha çok güvenir.
Çaresiz olduğu şey: menzil. Çığırtkan seni durduğun yerden çağırmaz, çevresindeki her şeyi kaldırır ve sen kalkanla ona yetişemezsin.

**SANCAKÇI** — Tek El + Konuşma + itibar + (isteğe bağlı) Orta Zırh.
Batıda kimse kimseye rütbe vermiyor, rütbeyi sancak veriyor. Höyük tarlasında yerde kalmış bir lejyon sancağını kaldıran adamın peşine adamlar takılır çünkü onlar **sancağı** tanıyor. İşe alan: borçlu menzilciler, House Millward gibi küçük haneler, Bilezik Savaşı'nın kaybeden tarafı. Açılan kapı: yol vekilliği merdiveninin ilk basamağı.
İlan anı doğrudan `title.c`'ye oturuyor: sancağı kaldırdığın gece "Sancakçı" ünvanı açılır, adının altına yazılır, ve *bir sonraki* menzil evinde bekçinin diyaloğu değişir — çünkü seçenek koşulu artık taşınan ünvana bakıyor. Sen kendini tanıtmıyorsun; o okuyor.
Çaresiz olduğu şey: sessizlik. Sancakçı gizlenemez. Suikastçi işi ona kapalı ve bunu bilir.

**KORSAP** — Çift El + Demircilik + Madencilik.
Kor çeliğinin sapı hiç tam soğumaz. Bu hattaki adamların sol avuçlarında aynı yerde bir yanık izi olur, ve Kilnmoor'da bu iz bir referans mektubudur. İşe alan: Marshal Ironkiln'in kor musluğunu koruyanlar, ve karşı taraftan Ocakdoğan çukur kurulları — ikisi de aynı adamı istiyor. Açılan kapı: **soğutulmuş kor kılıcı**. Kor çeliği karanlıkta ağır vurur ama seni gösterir; ölünün geri çekildiği kılıcı yalnız Ocakdoğanlar yapıyor ve sen o kılıcı **yapamazsın** — Kilnmoor iç kavgasında taraf tutup kazanman gerekiyor.
İlan anı: kendi ocağında ilk kor çeliğini dövdüğün gece Kilnmoor'da fiyatlar oynar ve iki taraftan da adam gelir.
Çaresiz olduğu şey: karanlık. Bu hattın adamı gece görünür, ve bunu bilerek yaşamayı öğrenmek zorunda.

**RUHSATLI** — Yay + Gizlilik + Bitkicilik.
Kalkan Nöbeti'nin satacak tek şeyi kazı ruhsatı. Ruhsatlı adam höyük tarlasına yasal olarak girer, ve yasal girmek gece orada kalmak demek. İşe alan: Nöbet'in kendisi, Coldhearth'in kuzey ciltlerini isteyen adamları, ve müzayedeci. Açılan kapı: mahzen.
Bitkicilik burada süs değil: ölüye zehir işlemez. Okçunun bitkiciden istediği şey **kor tozu kaplaması** — ölüye işleyen tek kaplama, ve `coatFx`/`coatDuration` kalıbı zaten var. Oyuncu bunu bir kez zehirli okla bir iskelete ateş ederek öğrenir; hiçbir sayı düşmez.
İlan anı: Nöbet'in kâhyası sana ruhsatı **yazılı** verir. O kâğıt envanterde durur, muhafız diyaloğunda seçenek açar.
Çaresiz olduğu şey: Kol. Yirmi kişilik kapalı kalkan formasyonuna karşı okçu sadece geriler.

**ÇUKUR ADI** — Tek El/çift hançer + Hafif Zırh + Yankesicilik ya da Gizlilik.
Kilnmoor'un çukurunda kimse gerçek adıyla dövüşmez. Sebebi kibir değil hesap: çukurda ölürsen adın yol taşına yazılır ve bir gece birisi onu okur. Herkesin bir çukur adı var, ve çukur adı öldüğünde kimse kalkmaz. İşe alan: çukur bahisçileri, Kızıl Kanca'nın Saltmere'deki adamları, ve seni tanımak istemeyen herkes. Açılan kapı: ismi olmayan adamın gidebildiği yerler.
İlan anı en güzeli: bir Dinleyen'e — köprü tahsildarı ölüsüne — kuruş verdiğinde adını deftere yazar. Çukur adıyla dövüşen oyuncu **hangi adı** verdiğini seçer. Yanlış seçim, gece seni arayan bir höyük iti demek.
Çaresiz olduğu şey: itibar. Bu hattın adamı bir orduya komuta edemez, çünkü sancağı olan adamın aksine kimse onun adını bilmiyor.

---

## 5. HATLAR BİRBİRİNİ NASIL BESLİYOR

Çukur sözleşmesi (Ağız Tutan ↔ Yakıcı): gecelik dört gümüş, `contract.c` sistemi taşır **[DOĞRULANMADI: tekrarlanabilir gecelik kontrat desteği]**. Yakıcı savaşçı olmadan çalışamaz, savaşçı ruhsat numarası olmadan meşru olamaz.

Kor çeliği (Korsap ↔ Ocakdoğan/madenci): soğutulmuş kor kılıcını oyuncu craft edemez, satın alır ya da bir hizip kazanır. Karşılığında demirci hammaddesini derin galeriden çıkaramaz — Kızıl Ana orada yatıyor.

Kazıyıcı (her savaş hattı ↔ kazıyıcı): bir adı taştan kazımak bir saatlik iş, ama höyük tarlasında yalnız yapılmaz. Savaşçı kazıyıcıyı koruyor, kazıyıcı o bölgede geceyi kalıcı olarak sakinleştiriyor — bölge durumu aracı, oyuncu değişimi ekranda görüyor.

Bitkici (Ruhsatlı ↔ bitkici): kor tozu kaplaması. Ve tersi: bitkici höyük tarlasının kenarındaki otları toplamak için okçuya muhtaç.

Ve hepsinin altındaki bağ: **fitil ustası**. Yakılan direk o gece o menzilde ölüyü yatırır. Yani en ağır zırhlı adam bile, hiç dövüşmeyen bir memurun akşam ne yaptığına bağlı.

---

## 6. BEŞ ROL DENETİMİ

Büyücü kalkan kullanabiliyor mu? Asa `WEAPON_STAFF` iki el, kalkanı kilitliyor — ama büyüler silahtan bağımsız (`T_DESTRUCTION` yorumu bunu açıkça söylüyor), yani tek el kılıç + kalkan + Yıkım ağacı meşru bir "savaş rahibi" ve keystone'u bile var. Serbest. Suikastçi hafif zırh + hançer + Çukur Adı hattı, serbest. Tüccar Korsap ve Sancakçı üzerinden savaşa giriyor. Şövalye Sancakçı ve Ağız Tutan.

**İhlal listesi** (yukarıda gerekçelendirildi): orta zırh eşyası yok → bir zırh kimliği oynanamıyor. Çift el = yalnız mızrak → dünya alet dayatıyor. Eldiven eşyası hiç yok, iki set eksik. Bunlar kapatılmadan "istediğin silahı ve zırhı kuşan" sözü tutulmuyor.

---

## 7. KOD FATURASI

**Sıfır kod:** Kol karşısındaki taktikler (mızrak hat vuruşu + `armorPierce` + Demir Ok + Yapışkan Tuzağı hepsi mevcut) · kolonun yanından eğilerek geçmenin XP'si (`main.c:9651` kanalı) · ölü yuvalarında `NEST_CHIEF`/`scatter` kullanmama · hat ilanının taşınan ünvanla gösterilmesi (`main.c:11098` zaten çiziyor) · çukur kontratları.

**Yalnız düğüm ekleme** (hepsi 32 tavanının altında kalıyor): Kalkan 16→21 (`HAT BİLGİSİ`, ön saf, +2 aktif adayı), Ağır 18→21 (`Kapalı Kın`), Hafif 18→21, Orta 18→20, Tek El 20→22 (`Söndür ve Vur`), Çift El 18→21, Yay 23→24. Rahat.

**Küçük kod:** ışık terimini algı zincirine bağlamak (`main.c:9543`) · zırh `weight`'ini aynı zincire bağlamak (`main.c:7951`'de hesaplı) · `emberGlow` bayrağı + gece hasar terimi (`combat.c:82` `perkScale` zinciri) · Kor Rünü'nün undead dalı · "vurulsa da işine devam eden" mob bayrağı (bestiyer de istiyor) · diyaloga `DVAR_TITLE` koşulu (`dialog.h` parser'ına bir enum + bir dal).

**Orta:** Duran'ın hattının yerde görünmesi · Kol'un gezici olay-mobu (bestiyer [YENİ] demiş, aynı fatura) · kış/hafif zırh soğuk cezası.

**Eşya slotu — dikkat.** `ITEM_TYPES` 92/96, dört slot kaldı. İstediklerim: çift elli kor çeliği baltası (1), orta zırh göğüs + başlık (2), Kor Rünü (1) = **tam dört**. Marj sıfır, eldiven eşyası bile sığmıyor. Öneri: başka hiçbir şey yapmadan önce `MAX_ITEM_TYPES`'ı 128'e çıkarın — ve **tüm paralel dizileri** grep'leyin; başlık dosyasının kendi yorumu 64'te bir taşma crash'i olduğunu söylüyor, `save.c` tamponları dahil dört ayrı yerde tavan var.

Beceri bütçesi sorun değil: `spellsLearned` 64 bitlik, 36 dolu.

---

**İlgili dosyalar:** `/Users/hakanartuc/PixelGame/src/skilltree.c` (yedi savaş ağacı, satır 24-226) · `/Users/hakanartuc/PixelGame/src/title.c` · `/Users/hakanartuc/PixelGame/src/combat.c` (satır 82-92 `perkScale` zinciri) · `/Users/hakanartuc/PixelGame/src/main.c` (satır 7951 zırh ağırlığı, 9543 algı zinciri, 9651 gizlilik XP kanalı, 10943 meşale ışığı, 7282 undead ×1.6) · `/Users/hakanartuc/PixelGame/src/item.c` (92 eşya) · `/Users/hakanartuc/PixelGame/src/dialog.h` (koşul enum'u).