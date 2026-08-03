# KADRO — YENİTAÇ SLOT İSKELETİ (rev. 2)

*Üç denetim raporundan sonra baştan yazıldı: `kadro-kanon-raporu.md` (K3, Ö1, Ö2, Ö19, Ö20, N7-N11, N15), `kadro-klise-raporu.md` (A4), `kadro-dil-raporu.md`. En büyük değişiklik: **taht salonu ayrı harita.** Eski (A) bölümündeki karar iptal.*

*İsim durumu: LORE'da yazılı adlar (Sarp, Gökçe, Rüstem, Şirin, Sadık, Vedia, Sabri, Yusuf, Nazlı) sabit. Kart dosyalarında üretilen adların bir kısmı denetimde düştü (Kıraç → K6, Balaban → N13) ya da karar bekliyor; bu belge slotları **id ve rolle** anıyor, ad kararlarını (C) tablosunun son sütununda topluyor. Üç yeni slotun (12, 20, 23) adı bilerek yazılmadı.*

---

## (A) HARİTA / ID ÖNEKİ KARARI

**Önek: `yenitac`. Bütün Yenitaç kadrosu — sur dibindeki işçi de, sandalyedeki adam da, mahzende kandili tutan da — `yenitac_<yer>_<rol>` alır.**

**1. `dunya` bir yer değil.** Bugünkü tablodaki `dunya_koy_demirci` öneki motor haritasının adını taşıyor (map 0), kanon yerin adını değil. Altı şehir gelince "dunya" hepsini kapsayacak ve önek tekilliği yapı gereği sağlama işini bırakacak — `npc.h`'ta yazan gerekçenin ("altı şehirli kanonda iki demirci olacak") çöktüğü nokta burası. Yenitaç'ın demircisi ile Karaocak'ın demircisi ancak şehir adıyla ayrılır.

**2. Önek KANON YERİ, motor harita indeksi değil.** `NpcDef.map` zaten motor haritasını tutuyor; id'nin o bilgiyi ikinci kez taşımasına gerek yok, taşırsa yalan söyler. `yenitac_salon_bakici` kaydı gün içinde iki ayrı motor haritasında yaşayabilir ve id yine doğru yeri gösterir. Mevcut `loca_*` ve `tasdurak_*` zaten bu kuralı izliyor; tek kural dışı olan `dunya_*`.

**Komşu köy `cayir_` önekini alır.** Köy silinmiyor, taşınmıyor: Yenitaç'ın bir tarla ötesindeki **tahıl köyü** oluyor. Rüstem'in "yarın sefer yoksa öbür gün bu adamlar yol kesecek ve ilk kesecekleri yol Yenitaç'ın tahıl yolu" cümlesi, oyuncunun ilk saatte yürüdüğü yolun adı hâline geliyor.

### KARAR: salon ve mahzen AYRI HARİTA

**`MAP_SALON`** — çatısı yok, duvarı ve kapısı var. İçinde iki alan: kapalı taht bölümü (tahtın arkasındaki sandalye, kâtibin masası, kasa odasının kapısı) ve duvarla çevrili **avlu**. Bir de **`MAP_MAHZEN`**: karanlık, dar, duvarın ardındaki soluk oradan geliyor. İkisi de enum'un **sonuna** eklenir (`worldmap.h`'taki `MapId`, `MAP_COUNT` önünde) — `save.c:490` kayıttaki harita indeksini `MAP_COUNT`'a karşı sınırlıyor, yani sona ekleme güvenli.

Eski karar tersiydi: "salon ayrı harita olmayacak, çünkü `NpcDef.map` tek bir int ve rutin harita sınırını geçemez." O gerekçe bir kod faturasıydı ve bu projede kod faturası kanon gerekçesi olamaz. Kendi belgesiyle de çelişiyordu: Sarp'ın kartta yazılı **tek arzusu** öğlen avluya çıkarılmak. Salon zaten avluysa o istek buhar oluyor — ve onunla birlikte Sabri'nin bütün işi, çünkü Sabri'nin taşıdığı adamın gidecek yeri kalmıyor.

İkisi de duvar istiyor: vinyetin görsel notu *"sağ duvarda ALTI BOŞ ÇİVİ"* diyor, GDD §1.4 *"salona her girişte kaç sancak asılı olduğunu sayarsın"* diyor. Giriş var demektir. Ve LORE §7'nin bütün fiziği kapalı bir odaya dayanıyor: kim duyduğu bilinen, sayısı belli bir dinleyici. Kırk adım ötede ip çeken işçinin duyabildiği yerde "söylenemeyen cümle" diye bir şey olamaz.

Avlunun bir işi daha var: Sarp öğlen oraya çıkarılınca **kuzey yolu görünüyor** ve adam ona bakıyor. Aynı avluda tören provası yapılıyor, gaziler o duvarın dibinde bekliyor. Salonu ayrı harita yapmak sahneyi kaybettirmiyor, ikiye ayırıyor: bir kapalı oda, bir avlu, aralarında bir kapı — ve kapının hangi tarafında durduğun kimin ne duyduğunu belirliyor.

### MEKANİK BORÇ (kanon kararı değil, fatura)

Bu karar üç kalem borç açıyor. Hiçbiri yapılmadan salon çalışmaz; üçü de küçük.

**1. NPC harita geçişi — küçük/orta.** Bugün `npc.c:278`'de tek satır var: `#define NPC_ON_MAP(i) (NPCS[i].map == npcMap)`. Makro dört yerde kullanılıyor (305 rutin, 416 seçim/konuşma, 497 ve 520 çizim); beşinci nokta 350. satırdaki yağmur sığınması, `NPCS[i].map == 0` diye elle yazılmış. Gereken: `NpcStop`'a bir `map` alanı (varsayılan 0, geriye uyumlu), duraktan haritayı okuyan bir yardımcı, o beş yerin bu yardımcıya bağlanması. Durak haritası değiştiğinde NPC **yürümez, ışınlanır** — kapıdan geçişini kimse izlemiyor ve izlemediği için bedava. Save'e girmiyor (kayıtta NPC yok). Bu alan geldiğinde Rüstem'in salon→kışla hattı, Yusuf'un salon→revir hattı, Gökçe'nin iskele→salon hattı ve Sabri'nin salon→mahzen hattı aynı anda açılıyor: **dört slot tek alanla kurtuluyor.**

**2. İkiz kayıt yamasına artık gerek yok.** Eski belge Sabri için `yenitac_mahzen_bakici` diye ikinci bir kayıt öneriyordu ve kendisi de "ucuz ama çirkin" diyordu: iki isim bir adam olacaktı. O yama iptal. Mekanik yazılınca Sabri tek kayıt, dört durak, iki harita.

**3. Vinyet iki harita üstünde geçiyor — küçük.** `intro_demirboyun.txt`'nin ilk üç paneli salonda, dördüncüsü sur dibinde. Cine modu canlı mekânda oynayacaksa panel 3 → panel 4 geçişinde perde açıkken harita yüklenmesi gerekiyor. Portal akışı bunu zaten yapıyor (`main.c`'deki geçiş bloğu mob/loot/minimap sıfırlıyor); iş, o akışı perde arkasından çağırmak.

### Yeniden adlandırmanın fiyatı — sayım düzeltildi

`src/npc.c`'de **12 id** değişiyor (12 `dunya_*` kaydının hepsi; `loca_*` ve `tasdurak_*` duruyor). Etkilenen çağrı yeri, grep'le sayılmış hâliyle:

- `main.c:1409-1417` — dokuz `npc_index_by_id` çağrısı yan yana; **sekizi** etkileniyor, 1411 (`loca_salon_usta`) etkilenmiyor.
- `craft.c:18` ve `craft.c:22` — `ownerNpcId`, ikisi de değişiyor.
- `main.c:2329, 2331, 3320, 5736` — dört id karşılaştırması daha var, hepsi `loca_*`/`tasdurak_*`, bu turda **etkilenmiyor**.
- `main.c:3511` — `npc_index_by_id(sd->ownerNpcId)`, literal yok, kendiliğinden doğru kalıyor.

Toplam: **8 (main.c) + 2 (craft.c) = 10 çağrı yeri.** Eski belgedeki "14 çağrı yeri, dokuz satır hepsi yan yana" ifadesi yanlıştı; kanon raporunun "6 + 2 = 8" düzeltmesi de eksik saymış (o aralıkta yedi değil dokuz çağrı var). Save'e girmiyor, tek commit.

---

## (B) SLOT LİSTESİ — 23 SLOT

Saatler rutin durağıdır, tavan dört (`NPC_MAX_STOPS`). "Vinyet" derken §1.2d'nin canlı mekânda geçecek açılışını kastediyorum (`assets/scenes/intro_demirboyun.txt`).

Yedi slotta aynı cümle yazılıydı: *"kaybolursa kurum çöker."* Herkes vazgeçilmezse kimse sıradan değildir. İki slotta bunu tersine çevirdim ve gerekçesi o slotların içinde duruyor: **9** ve **21**. İkisi de yerine biri konabileceğini biliyor, ikisi de bunu kimseye söylemiyor, ikisi de oyuncunun ilk on bir dakikada tanıştığı adamlar. Seçim sebebi şu: bu ikisinin tuttuğu kurum aslında **işlemiyor** — dinlenmeyen bir ilan ile bitirilmesi yasak olan bir inşaat. Vazgeçilmezlikleri baştan yalandı, yalanı kendileri de biliyor.

---

### SALON — `MAP_SALON`: çatısız ama duvarlı, altı ağız ve bir sandalye

#### 1 · `yenitac_salon_bey`
**ROL** — Sandalyedeki adam. Konuşmaz, işaret eder; oyunun en pahalı bulmacası onun önündeki boş tepsi.
**MEKÂN + SAAT** — 07 tahtın arkasındaki sandalye · **12 avlu** (kuzey yoluna bakan köşe — kartta yazılı tek arzusu bu) · 15 tahtın arkası, sandalye döndürülmüş çünkü gölge kaydı · 20 `indoors`. Hiç yürümez; durakları Sabri'nin taşıdığı yerlerdir ve dördü de `MAP_SALON` içinde.
**MEKANİK BAĞ** — Vendor değil, quest vermiyor, muhafız değil. Diyalog dosyası var ama **düğümleri envanterle açılıyor**: tepsiye konan üç nesne. `DialogVar`'da envanter koşulu yok — **[YENİ MEKANİK: `DVAR_ITEM` koşulu + envanter efekti — küçük]**; aynı mekaniği slot 7'nin üç otu da kullanıyor, tek iş iki yeri açıyor. Reddetme kanalı (göz kapanınca oda durur) `npc_bark` + bir storyFlag ile taşınabilir.
**İLK YİRMİ DAKİKA** — Vinyet paneli 2 ve 7. dakika. İkisinde de tek kelime etmez. Oyuncu 20. dakikada onun adını bilmez, sandalyesini bilir.
**TARAF** — Hiçbiri. Altı ağzın kendi tarafı var; adam tarafsız kaldığı için değil, konuşamadığı için taraf tutmuyor.
**DEVRALDIĞI KAYIT** — Yeni.

#### 2 · `yenitac_salon_maresal`
**ROL** — Mareşal; odanın en yüksek sesi, Kılıç tarafının bütün ağırlığı.
**MEKÂN + SAAT** — 07 salon · 12 kışla avlusu (maaş defteri; kuyruk onun için bekliyor) · 18 salon · 22 `indoors`. İki harita, dolayısıyla (A)'daki alana muhtaç.
**MEKANİK BAĞ** — Kılıçartıkları rütbe hattının tepesi; ilerideki zincir sahibi, ilk sürümde questId yok. Bir RoadNode'un sahibi **değil** — mareşal yolda denk gelinmez, o gelinir. Haftada iki kez mahzene indiği kanonik ama `NpcStop` yalnız saat biliyor, gün bilmiyor: **[YENİ MEKANİK: durak gün maskesi — küçük]**. Maske yoksa buz çuvalı her akşam iner ve adam gülünç olur; ilk sürümde buz sahnesi rutinden değil storyFlag'li bir sahneden gelsin. Kanon raporunun K4'ü burada bağlanıyor: **resmî tahsis iki çuvala düştü, üç gerekiyor**; farkı adam kendi kesesinden alıp kendi sırtında indiriyor ve defterde o kalem yok.
**İLK YİRMİ DAKİKA** — Vinyetin ilk salon repliği: "Beyimiz sefer istiyor." 7. dakikada aynı cümleyi canlı duyar, çünkü adam gerçekten her gün onu söylüyor. Oyuncuya bakmaz.
**TARAF** — Kılıç.
**DEVRALDIĞI KAYIT** — Yeni.

#### 3 · `yenitac_salon_katip`
**ROL** — Saray kâtibi. Takvime tarih yazar, üstünü çizer, yenisini yazar. Yenitaç'ın **unvan tescili** de onun defterinden geçer (LORE §4).
**MEKÂN + SAAT** — 06 salon kenarındaki yazı masası · 11 arşiv nişi · 17 masa · 21 `indoors`. Dört durak, üçü aynı odanın üç köşesi; oyuncu onu her geçişte hemen hemen aynı yerde bulur ve bu bilerek böyle.
**MEKANİK BAĞ** — Oyuncunun unvanını/beratını kaydeden ağız (§11 merdiveninin kâğıt tarafı). Erteleme sayacı bir storyFlag; diyalogda okunabilmesi için **[YENİ MEKANİK: genel bayrak koşulu `DVAR_FLAG` — küçük]** (bugün yalnız `DVAR_HINT` var, tek bayrağa gömülü — `dialog.h:49`). AmbientHook ağzı: kendi kendine dikte ediyor.
**İLK YİRMİ DAKİKA** — Vinyet paneli 3, dördüncü tarihi yazarken. Sonra 20. dakikaya kadar sessiz.
**TARAF** — Tarafsız görünür, arşive taraftar. Pratikte odanın frenidir.
**DEVRALDIĞI KAYIT** — Yeni. *(`tasdurak_konak_katip`'in görünen adı "Kâtip Rüstem" — mareşalle çakışıyor. `.name` değişecek, id duracak; §8.1'in ne işe yaradığının canlı örneği.)*

#### 4 · `yenitac_salon_hatun`
**ROL** — İkinci eş. Odada hiçbir şeye hayır demeyen tek ağız; yalnız "bugün olmaz" der.
**MEKÂN + SAAT** — 08 salon · 13 kendi odasının önündeki taşlık (kızı orada olmalı, değil) · 19 salon · 22 `indoors`. Dördü de `MAP_SALON`.
**MEKANİK BAĞ** — Erteleme storyFlag'ini o ilerletir; ahırdaki eyerlenmiş at onun izi (slot 15). Kızı deftere yazıldığı gün tavrı topyekûn döner — **kalıcı NPC tavrı** aracıyla, ekstra sistem istemez.
**İLK YİRMİ DAKİKA** — Vinyet paneli 2, üçüncü replik. Ayağa kalkar, kimseye bakmaz. Oyuncu onu bir daha 20. dakikaya kadar görmez.
**TARAF** — Kendi. Üç tarafın hiçbirine yazılmaz ve bu tam olarak onun gücü.
**DEVRALDIĞI KAYIT** — Yeni.

#### 5 · `yenitac_salon_sayman`
**ROL** — Sayman. Odada gerçekten konuşan tek kişi: kasada ne var, ne kadar yeter.
**MEKÂN + SAAT** — 07 kasa odası kapısı · 12 salon · 16 kasa · 20 `indoors`. Salona yalnız öğlen çıkar, çünkü rakam okunacaksa öğlen okunur.
**MEKANİK BAĞ** — İskele ödemelerinin, işçi yevmiyesinin ve gazi maaşının kaynağı. Yenitaç'ın **bölgesel fiyat çarpanı** ve "para bitti" dünya olayının kapısı burada (`docs/konsept/reboot/22-demirboyun.md` §G, "Yenitaç düşerken"). Şimdilik questId yok. Devlet rakamını **kese** ile söyler — bkz. (F).
**İLK YİRMİ DAKİKA** — 7. dakika, ve cümle **ambient**: odaya söylenir, oyuncuya bakılmaz. Kasada üç ay var. **Oyunun konusu bu cümleyle iniyor** — taç değil, para ve zaman. Oyuncu E'ye basarsa ancak o zaman kendisine dönülür.
**TARAF** — Kasa. Uygulamada Taş'a yakın çünkü rakam ona hak veriyor; kendini öyle tarif etmez.
**DEVRALDIĞI KAYIT** — Yeni.

#### 6 · `yenitac_salon_bakici`
**ROL** — Beyin eski yoldaşı, bakıcısı. Okuma yazması yok. İşaretleri bilen tek adam ve kimse ona sormuyor.
**MEKÂN + SAAT** — 06 sandalyenin yanı · 12 avlu (bey güneşe çıkarılır) · **15 mahzen** (`MAP_MAHZEN`) · 20 sandalyenin yanı. Mahzen durağı öğleden sonraya alındı: eski çizelgede adam 19'dan 23'e mahzendeydi ve Sarp'ı 21'de içeri kimse sokamıyordu. Artık 20'de yukarı çıkıyor, adamı yatırıyor. **İkiz kayıt yok** — (A)'daki alan bunu taşıyor.
**MEKANİK BAĞ** — **Yedinci Ağız** gizli sınıfının tanığı (LORE §10). Altı ağzın ipucu düğümlerine girildiyse kapı kirlenir; kirlenme sessizdir, sayaç ömür boyu durur. Mahzende **kandili tutan refakatçi** — 15-18. dakikanın ışığı onun elinde.
**İLK YİRMİ DAKİKA** — İki vuruş taşıyor ve aralarındaki mesafe bu tasarımın en iyi yeri. 7. dakikada odanın kenarında adam taşıyan, kimsenin dinlemediği biri. 16. dakikada mahzende kandil tutan adam. Oyuncu ikisinin **aynı kişi olduğunu kendi fark eder** — kimse söylemez, ve iki mekân artık iki harita olduğu için fark etmesi daha zor, dolayısıyla daha değerli.
**TARAF** — Tarafsız; tarafların olduğunu bile önemsemiyor.
**DEVRALDIĞI KAYIT** — Yeni.

---

### REVİR — hekimin iki hastası var: bir bey, bir kalabalık

#### 7 · `yenitac_revir_hekim`
**ROL** — Altıncı ağız ve şehrin tek eğitimli hekimi. Sarayda "hastanın iradesi hekimindir" diyen adam, revirde kimin sıraya gireceğine de kendi karar veriyor.
**MEKÂN + SAAT** — 07 salon (bey muayenesi) · 10 revir · 17 revir · 21 salon. Gece revir **kapalı** ve bu bilerek: gece yaralanan adam sabahı bekler.
**MEKANİK BAĞ** — `npc_render_infirmary` sahibi (revir yatağı Yenitaç'a taşınır) · paralı tedavi · **`STATION_ALCH` sahibi**: `craft.c:22`'deki `ownerNpcId` bu slota geçer, kazan revire taşınır. `permQuest = simya_masa` duruyor ama GDD §1.2c-7'yi karşılayacak biçimde yeniden kuruluyor:
- **Toplama yeri mezarlık.** Kızılot (`ITEM_HERB_RED`) spawn'ı mezarlığa kaydırılıyor — kriptalar Akkemer kemerlerinin üstünde (LORE §12), yani üç ot toplamak yürüyüş değil sahne.
- **İkinci alıcı var ve yerinde duruyor:** `cayir_koy_otaci`. Otacı ile hekim birbirinden nefret ediyor (isim havuzu §2.7: iki ayrı meslek, iki ayrı havuz, birbirlerini sevmiyorlar). Üç otu hangisine verirsen ötekini kaybediyorsun; hekime verirsen kazan izni açılır, otacıya verirsen kazan kapalı kalır ama eline başka bir şey geçer.
- Bu dal envanterden ot düşürmeyi ve kabul edilmiş görevi teslim etmeden kapatmayı istiyor: **[YENİ MEKANİK: `DVAR_ITEM` koşulu + envanter efekti — küçük]** ve **[YENİ MEKANİK: görevi başka yolla kapatma durumu — küçük]**.

Böylece üç ot bir bilet değil bir karar oluyor ve karar bedava geliyor: rakip zaten kadroda.
**İLK YİRMİ DAKİKA** — Vinyet paneli 2, ikinci replik. Sonra ilk 20 dk DIŞI. Mahzendeki buz talebi ondan çıktığı için Sabri'nin oradaki varlığının sebebi o; ve slot 12'nin ezdiği elin sahibi onun yatağında yatıyor.
**TARAF** — Tarafsız, kendi otoritesine taraftar.
**DEVRALDIĞI KAYIT** — **`dunya_koy_sifaci` (Şifacı Ela) DEĞİL.** Bu slot yeni. Ela rafa kalkmıyor, taşınmıyor: `cayir_koy_otaci` olarak yerinde kalıyor ve yukarıdaki ikinci alıcı o. Eski belgenin "Ela'yı hekime dönüştür, cadı-şifacı hattını rafa kaldır" kararı iptal.

#### 8 · `yenitac_revir_gazi`
**ROL** — Maaşı çıkmamış, işi olmayan, yatağı boş olduğu için revirde kalan gazi. Yenitaç'ın "gazi" nüfusunun yüzü.
**MEKÂN + SAAT** — 08 kışla kuyruğu (defter açılmıyor, beşinci kez sayıyor) · 14 revir · 19 aşçının kazanı önü · 23 revir. Dört durağı da bir işi olmadığını anlatıyor.
**MEKANİK BAĞ** — AmbientHook ağzı, GDD §1.2c-5'te örnek verilen cümlenin sahibi: *"Baran'a söyle bu ay gelmeyeceğim, maaş çıkmadı."* Aynı zamanda **Gündoğdu'nun Yenitaç'taki tek varlığı**: çölden gelen biri ona maaş teklif etmiş ve adam bunu gizlemiyor, çünkü utanacak bir şey görmüyor. Oyuncu haber verirse Kılıç itibarı, susarsa adam bir gün ortadan kaybolur (kalıcı boş yatak = dekor izi).
**İLK YİRMİ DAKİKA** — **Vinyet paneli 4'te konuşuyor** (*"Beşinci kez sayıyorum. Defter açılmadan sıra ilerlemiyor."*) — eski çizelge onu 0-2. dakika kadrosuna yazmayı atlamıştı. Sonra 8. dakikada kışla kuyruğunda canlı görülür; oyuncu neyi beklediklerini bilmez.
**TARAF** — Kılıç, ödenmediği kadar.
**DEVRALDIĞI KAYIT** — Yeni.

---

### İSKELE — on bir yıllık şantiye

#### 9 · `yenitac_iskele_kalfa`
**ROL** — İskele kalfası. Çivi sayar, ip verir, gece surda meşale dizer çünkü meşale nöbeti ayrı ödeniyor.
**YERİNE** — Konabilir ve bunu biliyor. İskele **emirsiz** duruyor, parasız değil (LORE §7: bitmemiş bir çatı değil, söylenemeyen bir cümle). Yapılmayan bir işin kalfası ustalık istemiyor; ekibin ihtiyacı ustabaşı değil, ödeme yapan biri. Adam bunu bir kez söyler ve bir daha açmaz; şikâyet olarak değil, hava durumu gibi. Kaybolursa iskele **durmaz** — yerine ırgatlardan biri geçer (havuzun devrolan iş adı kümesi hazır: yerine Çat geçer, ve gelecek yıl başka biri Çat olur), çivi sayımı bir hafta tutulmaz, sonra yeniden tutulur. *(Ö-M: "yamak" kullanılamaz — Kılıçartıkları'nın ilk rütbesi ve Uçkun ilk yirmi dakikada kendini onunla tanıtıyor.)*
**MEKÂN + SAAT** — 06 iskele dibi (sayım) · 13 vinç ayağı · 19 sur yürüyüş yolu (meşaleler) · 23 `indoors`. Yirmi saatlik bir gün.
**MEKANİK BAĞ** — **AmbientHook**'un ana ağzı: 06-08 arası *"Dün yüz elli çivi saydım, bu sabah yüz on iki."* Gece durağı **kandil hattını öğretir** — oyuncu meşalelerin bittiği yeri görür, kimse "burası tehlikeli" demez. **[YENİ MEKANİK: AmbientHook — küçük/orta; GDD §1.2c-5'te onaylı, kodda yok.]**
**İLK YİRMİ DAKİKA** — **3. dakika: ilk insan sesi ve sana değil.** Çivi cümlesi 5. dakikadaki çocuğun kurulumudur; iki vuruş arasında hiçbir bağ kurulmaz, oyuncu kendi kurar. Vinyet paneli 1'de yukarıya cevap veren de o: *"İndirme emri yok. On bir yıl havada durdu, bir kış daha durur."* "İpin yediği yer" onun imza deyimi; yukarıdan gelen ses onun değil (slot 10).
**TARAF** — Tarafsız, alacaklı. Yevmiyesi üç haftadır gecikiyor.
**DEVRALDIĞI KAYIT** — Yeni.

#### 10 · `yenitac_iskele_varis`
**ROL** — Varis. Teknik olarak hiçbir şey. Eldivensiz iskeleye tırmanıyor ve şehir bunu seviyor.
**MEKÂN + SAAT** — 08 iskelenin üst katı (yirmi beş metre; yerde değil, **yukarıda** görülür) · 12 salon kenarı, konuşulanı dinler · 17 sur burcu · 22 `indoors`.
**MEKANİK BAĞ** — İlk sürümde questId **yok** ve bu bilinçli. İlk cümlesi bir şey vermez: atın su içip içmediğini sorar. Kişilik sayaçları (kaba/kibar/ketum) bu ilk konuşmada sessizce işler; ileride Karduvar hattının kapı bekçisi olur.
**İLK YİRMİ DAKİKA** — **Oyuncunun duyduğu ilk insan sesi bu.** Vinyet paneli 1'in üst kat repliği ona ait: *"Örtüyü üstüne çekin! Taş su çekiyor, ipin yediği yerden akıyor!"* — iskelenin üst katındaki tek kanonik kişi o, ve varisin ilk sözünün bir taşı kurtarma emri olması bedava karakterizasyon. Yüzünü 12-13. dakikada görür; o zaman da hiçbir şey başlatmaz. Ulak kuralı: oyuncuyu aramaz, çağırmaz, seçilmiş ilan etmez. Yirmi dakikanın sonundaki "kimsenin vermediği iki hedef"ten biri, isterse, bu kadına ikinci kez gitmektir.
**TARAF** — Hanedan; üç tarafın da onu istediği ayrı bir dosya.
**DEVRALDIĞI KAYIT** — Yeni.

#### 11 · `yenitac_iskele_civici`
**ROL** — İskele ayağından çivi söken on iki yaşındaki çocuk. Huysuz, ağzı bozuk, hırsız. Kimse oyuncuya kim olduğunu söylemez.
**MEKÂN + SAAT** — Rutini kaçış üstüne kurulu: 06 iskele ayağı · 10 çarşı arkası · 15 iskele ayağı · 21 `indoors` (iskele ayağının yanındaki barınak). *(Ö-P: gece durağı eskiden "annesinin odası"ydı ve kimliği bir ağız değil **RUTİN** sızdırıyordu — Şirin'in odası `MAP_SALON`'da, yani çocuk her akşam saray kapısına yürüyordu. "Yatağı hâlâ annesinin odasında" bilgisi Şirin'in kartında kalıyor ve oyuncuya sarayın içinden geliyor. Bu slot da harita geçişi alanına muhtaç: (A)'daki "dört slot" sayımı **altı** olacak.)*
**MEKANİK BAĞ** — Ayakta yaklaşınca kaçar, çömelince kaçmaz: **çömelme buradan öğrenilir, metin kutusu olmadan**. `npc_walk_to` var, tetik yok — **[YENİ MEKANİK: yaklaşma tetikli kaçış — küçük]**. Yankesicilik hedefi (çivi üstünde) ama **öğretilmez** (§1.2d kasten öğretilmeyenler listesi).
**İLK YİRMİ DAKİKA** — **5. dakika: seni gören ilk göz.** Bakar, donar, kaçar. Bedeli yok, hafızası var: kovaladıysan / çiviyi geri istediysen / bıraktıysan bir sonraki karşılaşmada ilk cümlesi değişir. Oyuncu kaybetmeyi öğrenmeden hatırlanmayı öğrenir. Pano önündeki hece barkı **buz ilanını okumuyor** — bkz. (D)'deki vinyet talimatı ve slot 21.
**TARAF** — Yok. Kaçmak istiyor.
**DEVRALDIĞI KAYIT** — Yeni.

#### 12 · `yenitac_iskele_makaraci` *(YENİ SLOT — beceriksiz)*
**ROL** — Makara ve halat adamı, kırk yaşında, işini gerçekten kötü yapıyor. Düğümü kayıyor. Günde üç kez yükü indirip yeniden bağlıyor ve her seferinde aynı düğümü atıyor. Kötü niyeti yok, tembel de değil — dikkati bir yerde durmuyor ve kimse ona bunu öğretmedi, çünkü öğretmek de birinin işi olurdu.
**Kimse kovamıyor:** mareşalin eski bölüğünden bir gazinin oğlu. Kovmak için mareşalin yüzüne "adamının oğlu işe yaramıyor" demek gerekiyor — Rüstem'in kendi kuralı ("arkasından söylediğini yüzüne söyleyeceksin") bu adamı koruyor. Kalfa dört aydır bunu yapmayı erteliyor.
**MEKÂN + SAAT** — 07 vinç ayağı · 12 iskele dibi (yükü yeniden bağlıyor) · 16 vinç ayağı · 21 `indoors`.
**MEKANİK BAĞ** — Mekanik bağı yok ve olmasına gerek yok: onun taşıdığı şey **karşılaştırma**. Oyuncu 9. dakikada seyisin kayışını tutuyor ve doğru bağlanmış bir yükü eliyle hissediyor; 12-13. dakikada iskelede aynı işin yanlış yapıldığını görüyor. Kimse "bak, bu adam kötü" demiyor. AmbientHook ağzı, ama sayı taşımıyor — söylediği tek şey *"Tuttu"* ve tutmuyor.
**İLK YİRMİ DAKİKA** — Görülür, konuşulmaz. Sonucu ise ilk dakikadan itibaren ekranda: revirdeki dolu yatak. Oyuncu ikisini 14. dakikada revirden geçerken birleştirir; birleştirmezse hiç birleştirmez, kimse yardım etmez. `npc_render_infirmary` yatakları yan yana diziyor (`npc.c:655`), yani ezilen elin sahibi ile başka bir hasta aynı odada durabilir.
**TARAF** — Yok. Kimse onu istemiyor, kimse gönderemiyor.
**DEVRALDIĞI KAYIT** — Yeni. *Şehrin acısının bir kısmı sistemden değil bir salaktan gelmeli; bu slot onun için var.*

---

### KIŞLA — bitmemiş, ama işleyen tek kurum

#### 13 · `yenitac_kisla_katip`
**ROL** — Kışla kâtibi. Maaş defteri, kontrat kaydı, panonun çivisi onda.
**MEKÂN + SAAT** — 07 pano yanındaki masa · 12 kışla ambarı · 16 masa · 20 `indoors`.
**MEKANİK BAĞ** — **Kontrat panosunun ağzı** (`contract.c`). GDD §1.2c-3'ün istediği "panoda kabul yok" adımı **kodda hâlihazırda sökülmüş** (`contract.c:88-113`; `contract_accept` duruyor ama kullanılmıyor, `contract_on_kill` artık kabul kontrolü yapmıyor). Eski belgenin "kadro tasarımının koda dokunan tek zorunlu talebi" dediği şey bitmiş. Kalan iki gerçek borç 19. dakika vuruşunun altında:
- Buz işi bir **kes** kontratı değil; `ContractDef` bugün ya `targetMob` ya `fetchItem` ile tamamlanıyor. "Mahzene buz taşı" ise çuvalın kâtibe teslimi değil, **mahzene indirilmesi**. **[YENİ MEKANİK: storyFlag'le tamamlanan kontrat — küçük]**.
- Pano her gün rastgele üç ilan seçiyor (`contract_new_day`). Buz ilanının birinci gün panoda olması **garanti** olmalı: **[YENİ MEKANİK: rotasyona girmeyen sabit ilan slotu — küçük]**.

Kılıçartıkları rütbe kaydı ileride buradan.
**İLK YİRMİ DAKİKA** — **19. dakika.** Oyuncu mahzenden çıkar, panoya bakar, üç saat önce yaptığı işin ilanını okur: *"Mahzene buz taşı — 6 akçe."* Kâtip parayı verir ve teşekkür etmez. **Pano böylece görev kaynağı değil doğrulama olarak tanışır.** Çuvalı aşağı götürmediyse para yok, tek cümle var: *"Çuval kapıda duruyor."* Ders aynı, bedel sıfır.
**TARAF** — Kılıç, memur seviyesinde.
**DEVRALDIĞI KAYIT** — Yeni.

#### 14 · `yenitac_kisla_kapici`
**ROL** — Kışla kapısı nöbetçisi. Yamak rütbesinde, sıkılıyor.
**MEKÂN + SAAT** — Tek durak, `isGuard = true` → rutini `guard.c` yönetir. Nöbet değişimi akşam; **değişim aralığında kapı boş kalır** ve bu gizlenerek girmenin kapısıdır.
**MEKANİK BAĞ** — `isGuard`, bounty/tanık zinciri, bölge suç defteri (`crimeRegion`). **GDD §8.2'nin [AÇIK] sorusuna cevabım:** muhafız çoğullanır, hiyerarşi kadrodan gelir — Yenitaç'ın kapı nöbetçileri **kışlaya** bağlı (Kılıçartıkları, mareşalin defteri), `cayir_` köyünün devriyesi **bölgeye** bağlı (yerel, maaşını Yenitaç'tan alır ve gecikir), Karduvar'ınki Ak Nöbet'e bağlı olacak ve krallık defterine hiç yazmayacak. `guards[N]` üç ayrı sahip tanımalı. **[YENİ MEKANİK: çoklu muhafız + sahip alanı — orta.]**
**İLK YİRMİ DAKİKA** — **E tuşu burada öğrenilir**: cümlesini bitirir ve susar; susmuş bir adamın önünde durmak oyuncuya tuşu buldurur. Akşam barkı **mahzene ikinci giriştir** ("Aşağıya kim indi, kapağı kim açık bıraktı"). Konuşmayı hiç istemeyen oyuncu için 15-18. dakika bu ağızdan da açılır.
**TARAF** — Kılıç.
**DEVRALDIĞI KAYIT** — Yeni. *(`dunya_koy_muhafiz` köyde kalıyor — bkz. (C).)*

#### 15 · `yenitac_kisla_seyis`
**ROL** — Ahırın adamı: at, kızak, kışın buz. Şehrin en çok bilen, en az konuşan adamı, çünkü herkesin nereye gittiğini eyerden biliyor.
**MEKÂN + SAAT** — 06 ahır · 09 mahzen kapağı (kızak gelir) · 14 ahır · 20 ahır kapısı. Rutini kısa çünkü işi bir noktada.
**MEKANİK BAĞ** — 9. dakikanın **fiziksel talebi** — *"şu kayışı tut"* — ve bu bir görev değil, günlüğe satır düşmez, kabul edilmez, ödenmez. `npc_bark` yeter. Kayış çözülünce **çuvallar kapağın ağzında kalır**: kimse taşımaz, kimse rica etmez, adam kızağını çeker gider ("Kızak benim işim"). Aşağı inen oyuncu çuvalı götürür ya da götürmez; götürmediyse 19. dakikada para yok (slot 13). Aynı slot **Şirin'in izini** taşıyor: erteleme haberi sabah okunduğunda ahırdaki at gece eyerlenmiş bulunur, üçüncü ertelemede seyis oyuncuya bakar ve bakışını kaçırır. Bir **RoadNode** sahibi: tahıl yolunda devrilmiş kızak (EMANET tipi) — **[YENİ MEKANİK: RoadNode — orta/büyük; GDD §1.2e'de onaylı, kodda yok.]**
**İLK YİRMİ DAKİKA** — **9. dakika: ilk talep.** Aynı adam mahzen kapağını açar; **mahzene birinci giriş** budur. Aşağı inmez.
**TARAF** — Tarafsız. Kılıç'ın atına da bakar, Hatun'un atına da.
**DEVRALDIĞI KAYIT** — Yeni. *(Kart adı "Kıraç" düştü — K6: Kırca Ana'yla ses ikizi. Ad kararı (C) tablosunda: **Ayaz**.)*

#### 16 · `yenitac_kisla_demirci`
**ROL** — Kışlanın demircisi. Yeni silah dövmüyor, kırık silah tamir ediyor; bu şehrin bütün ekonomisini tek cümlede anlatır.
**MEKÂN + SAAT** — 07 ocak başı · 21 `indoors`. İki durak, çünkü adamın hayatı iki durak.
**MEKANİK BAĞ** — **`STATION_SMITH` sahibi** (`craft.c:18`'deki `ownerNpcId` bu slota geçer, `permQuest = demirci_ocak` durur: beş bakır cevheri, ellerine bakmak). Craft istasyonu ilk 20 dakikada **açıklanmaz**; oyuncu ocağı görür, kıvılcımı duyar, kimse ders vermez.
**İLK YİRMİ DAKİKA** — İlk 20 dk DIŞI (kasten). 10. dakika civarı yolun üstünde durur, o kadar.
**TARAF** — Tarafsız; müşterisi Kılıç, borcu Taş'a.
**DEVRALDIĞI KAYIT** — **`dunya_koy_demirci` (Demirci Mahir) taşınır.** Köyde nalbant kalır (dekor + ambient, NPC değil).

---

### ÇARŞI — "alacaklı" nüfusun oturduğu yer

#### 17 · `yenitac_carsi_tuccar`
**ROL** — Esnaf. Veresiye defteri tutuyor ve defteri kapatmıyor, çünkü kapatırsa müşterisi kalmaz.
**MEKÂN + SAAT** — 08 tezgâh · 13 tezgâh (kalabalık saati) · 19 tezgâh önü, defter · 21 `indoors`.
**MEKANİK BAĞ** — `isVendor = true`, alım-satım, bölgesel fiyat. **Tanık kuralının (§1.2c-1) referans örneği bu slot:** oyuncu bir zinciri ilerlettiğinde ilk cümlesi değişir — "evlat" demeyi bırakıp "müşteri" der ya da tersi. Teşekkür etmez. `simya_masa` ondan alınıp hekime geçiyor; yerine veresiye defteri hattı geliyor.
**İLK YİRMİ DAKİKA** — İlk 20 dk DIŞI. Oyuncunun altı akçesi 19. dakikada oluşuyor; dükkânın anlamı 21. dakikada başlıyor.
**TARAF** — Tarafsız, alacaklı. Gazilerin borcu onda; sefer çıkarsa parasını alamaz, çıkmazsa müşterisi yol keser. İki tarafa da hak veren tek esnaf.
**DEVRALDIĞI KAYIT** — **`dunya_koy_tuccar` (Tüccar Baran) taşınır.** GDD §1.2c onu iki ayrı örnekte Yenitaç esnafı olarak kullanıyor; adı da bu yüzden değişmiyor.

#### 18 · `yenitac_carsi_asci`
**ROL** — Akşam kazanı kuran kadın. İşçi kalabalığının tek sıcak noktası.
**MEKÂN + SAAT** — 10 çarşı, malzeme · 17 kazan · 21 kazan (son müşteri) · 24 `indoors`.
**MEKANİK BAĞ** — Kandil hattının **sıcak ucu**: gece kandiller onun kazanının çevresinde en yoğun, sur meşalelerinin bittiği yerde biter. Kaç kişinin yediğini sayar ve sayı düşüyor — AmbientHook ağzı, saymanın rakamının ete kemiğe bindiği yer. Phasing adayı: bir zincirden sonra kazan büyür ya da kalkar.
**İLK YİRMİ DAKİKA** — İlk 20 dk DIŞI, ama oyuncu akşam olmadan bitirdiği için 19. dakikada kazanın **kurulduğunu** görür. Dünyanın ilk yirmi dakikadan sonra da devam ettiğinin işareti.
**TARAF** — Tarafsız.
**DEVRALDIĞI KAYIT** — Yeni. *(Nazife köyde kalıyor — gerekçesi (C)'de.)*

#### 19 · `yenitac_carsi_kesedar`
**ROL** — Kırk Kese'nin Yenitaç yazmanı. İskelenin borcunu satın almaya talip ve bunu gizlemiyor. İşçi yevmiyesini o ödemiyor — kasada üç ay var (slot 5 haklı); onun verdiği şey tek tek kalemler: bir körük, bir kazan, bir at.
**MEKÂN + SAAT** — 09 kese masası (çarşı köşesi, iki sandalye, bir terazi) · 15 iskele dibi (ne kadar iş kaldığını kendi gözüyle sayar) · 18 masa · 21 `indoors`.
**MEKANİK BAĞ** — Vendor **değil** — borç, faiz, senet. Yenitaç'ta **Taş tarafının tek yüzü** ve bir siyasi cümlesi var: mareşalin on iki bini dağılırsa yevmiye ucuzlar, bunu masada söylemez, burada söyler. Kilitli sandığı var (kilit açma), terazisinde ara sıra **güney sikkesi** görünür — kimse açıklamaz, o da nereden geldiğini değil kimin getirdiğini bilir. Bir **RoadNode** sahibi: tahıl yolunda tahsilata giden adam (SATIŞ/BORÇ tipi). Rakamlarını **akçe** ile söyler ve kese'ye çevirmez; bkz. (F).
**İLK YİRMİ DAKİKA** — İlk 20 dk DIŞI.
**TARAF** — Taş.
**DEVRALDIĞI KAYIT** — Yeni.

#### 20 · `yenitac_kapi_bacdar` *(YENİ SLOT — memnun adam)*
**ROL** — Tahıl kapısının bac tahsildarı. Yirmi yıldır aynı masa, aynı terazi. Şehrin tek kesintisiz geliri onun elinden geçiyor: kasa boş olsun, yevmiye gecikirse geciksin, kapıdan giren her arabanın bacı ödenir ve payı **kapı defterinde eski bir temessükle** yazılı. *(Ö-D: "berat" LORE §2/§11'de oyuncunun üçüncü mülk kademesi — bir memurun gelir payına o kelime verilemez.)* Yenitaç'ta parası **yapı gereği** kesilemeyen tek adam — **kesilemeyeceğini sanıyor.** Bacı kesecek tek şey yolun kesilmesi; yolu kesecek adamların oğlu bu şehirde çalışıyor (slot 12) ve saymanın "üç ay" hesabı yol kapanırsa beş haftaya iniyor (slot 5). Memnun adamın güvencesi kadronun en büyük tehdidinin tam hedefinde, ve bunu kimse ona söylemiyor.
Karnı tok, çocuğu sağ, gecesi uykulu. Ve memnuniyeti saldırgan: öğle yemeğini bilerek kışla kuyruğunun gölgesinde yiyor, etini kâğıttan açıyor, kuyruktaki gaziye *"ben halloldum, sen niye halolmadın"* der. Kötülük ettiğini düşünmüyor; nasihat verdiğini düşünüyor. Kadronun tamamı ona kızıyor ve kimse yüzüne söylemiyor, çünkü kapı ondan geçiyor.
**MEKÂN + SAAT** — 07 tahıl kapısı, bac masası · 12 kışla kuyruğunun yanındaki gölge (yemek) · 17 kapı · 21 `indoors`.
**MEKANİK BAĞ** — Kapı bacı: §11 merdiveninin birinci kademesi (**İzinli** = bac muafiyeti) tam olarak onun elinden alınır. Oyuncu unvan aldığı gün adamın tavrı döner ve döndüğü yer somut: para istemez, ama söylediği şey daha kötüdür (*"Sen de halloldun demek"*). Vendor değil, questId yok. Bölgesel fiyat çarpanına dokunmuyor; dokunan slot 17.
**İLK YİRMİ DAKİKA** — Yolun üstünde, 10-11. dakika civarı, ağzı dolu. **Oyuncu ilk kez birinin haklı OLMADIĞINI burada görüyor** — yirmi iki kişinin yirmi ikisinin haklı olduğu bir kadroda bu şart. Konuşmak zorunda değil; adam onunla konuşur, oyuncu yürüyüp geçer, cümle arkadan gelir.
**TARAF** — Hiçbiri, ve bunu bir erdem sayıyor. Pratikte kimin kazandığı umurunda değil, kapının açık kalması umurunda.
**DEVRALDIĞI KAYIT** — Yeni.

---

### MEYDAN — ilanın ve boş taşın olduğu yer

#### 21 · `yenitac_meydan_tellali`
**ROL** — Tellal. Kâğıttan okur — okuyamaz, ezberler; kâğıdı kâtip ona sabah okur, o gün boyunca tekrarlar.
**YERİNE** — Konabilir ve bunu biliyor. Kuyruktaki yirmi adamdan herhangi biri dört akçeye aynı işi yapar; onu tutan şey sesi değil, **okuyamaması**. Okuyamayan tellal metni doğaçlamaz ve kâtibin en sağlam güvencesi bu. Adam bunu biliyor, kâtip de biliyor, ikisi bunu bir kez konuştu ve konuşma kötü geçti — bu şehirde sessizce anlaşılmış bir şey değil, yüksek sesle tartışılmış ve kapanmamış bir hesap.
**MEKÂN + SAAT** — 09 meydan · 12 kışla avlusu · 16 çarşı · 20 `indoors`. Aynı ilanı üç yerde okuyor, çünkü işi bu.
**MEKANİK BAĞ** — **Dünya durumunun sesli sayacı**: erteleme storyFlag'i ilerledikçe okuduğu metin değişir, sebep değişir, ses tonu aynı kalır. Ucuz, tek dosya, büyük etki. `DVAR_FLAG` gerekiyor (slot 3).
**İLK YİRMİ DAKİKA** — 11. dakika. Vinyet paneli 3'te aynı cümleyi duymuştu; şimdi canlı, meydanda, kalabalığa. Kimse durup dinlemiyor. **5. dakikanın çocuğu ilanı ondan DUYUYOR** — kız okuma bilmiyor (kart 11) ve pano önünde hiçbir şey hecelemiyor; tellal bağırıyor, o dinliyor, ezberliyor. *(KR5: bu belgenin eski hâli kızı heceletiyordu ve kartıyla zıt karar veriyordu. Kart kazandı — pislik içindeki çocuğun okuma bilmesi, "aslında soylu" demenin türdeki en tembel işareti. Üstelik yer de tutmuyordu: pano kışlada, tellal meydanda, çocuk çarşı arkasında.)* Tellalın kaybettiği sahnenin yerine kartındaki daha iyi malzeme duruyor: oyuncu ona bir metin okursa Sadık'ın güvencesi çöker.
**TARAF** — Tarafsız.
**DEVRALDIĞI KAYIT** — Yeni.

#### 22 · `yenitac_meydan_tasustasi`
**ROL** — Altı ismin oyulacağı taşı bekleyen usta. Parasını on bir yıl önce aldı.
**MEKÂN + SAAT** — 08 boş taşın yanı · 14 gölgeye çekilir, taş yerinde kalır · 18 taşın yanı · 22 `indoors`. Adam işini değil, taşını bekliyor.
**MEKANİK BAĞ** — Şu an mekanik bağı **yok** ve bu bilerek: oyuncu on bir yıldır oyulmamış bir taşın başında oturan bir adamı görür, sorar, cevap alır, gider. İleride Altı Ad zincirinin fiziksel çıktısı burada oyulacak — sancak sayımı gibi, duvarda asılı bir ilerleme çubuğu. Zincir açıldığında questId alır.
**İLK YİRMİ DAKİKA** — Vinyet paneli 3'te tek repliği var. Canlı olarak 11. dakikada, tellalın on adım ötesinde. İki sesi yan yana duymak yeter. Aynı saatte üçüncü bir ses de var: slot 23 gelip yine aynı şeyi soruyor ve ikisi yine kavga ediyor.
**TARAF** — Tarafsız, alacaklı — ama parasını aldığı için alacaklı olan tek adam. Alacağı iş.
**DEVRALDIĞI KAYIT** — Yeni.

---

### MEZARLIK — kutsanmamış toprak, üç ot ve yıkanan ölü

#### 23 · `yenitac_mezarlik_gassal` *(YENİ SLOT — inançlı)*
**ROL** — Ölü yıkayıcı. Yedisunak'ın usulünü bilen, uygulayan ve uygulanmadığında rahatsız olan tek adam. Derdi kıyamet değil, **usulsüzlük**: Yenitaç kutsanmamış bir şehir, dolayısıyla bu topraktaki her mezar teknik olarak sayılmıyor. *(Ö-F: mezarlık **Yenitaç'ın kendi mezarlığı**, köyün değil — GDD §8.2 güncellenecek. Zemin kartta hazır duruyor: **Sabri'nin yanmış kasabası Yenitaç'ın altında**, yani kırk üç mezar adı olmayan bir kasabanın toprağının üstünde ve o toprak hiç kutsanmadı. Bakıcının "kendi çocukluğunun altında yürüyor" satırıyla bedavaya birleşiyor.)* Kırk üç kişi gömdü ve kırk üçünün de defterde bir eksiği var.
İstediği şey bir kâğıt: Yedisunak'tan tek satır, *"bu toprak sayılır"*. Yazsınlar, o kadar. Dua ediyor — sabah ve akşam, ve gizlemiyor; ama oyuncuya asla dua ettirmez, kimseyi çağırmaz, kimseyi uyarmaz. Uğursuzluktan hiç söz etmiyor: onun sözlüğünde uğur diye bir şey yok, **eksik imza** var.
**MEKÂN + SAAT** — 07 mezarlık, yıkama taşı · 12 meydan (boş taşın yanı — taş ustasıyla tartışmaya gelir) · 16 mezarlık · 21 `indoors`.
**MEKANİK BAĞ** — Üç otun toplandığı yerin sahibi değil, **tanığı**: oyuncu kızılot için mezarlığa indiğinde orada bir adam ölü yıkıyor ve hiçbir şey istemiyor. AmbientHook ağzı. Mahzendeki soruyu ("neden gömülmüyor") tek cevaplayabilen kişi — ama cevabı 25. dakikadan önce vermiyor, çünkü oyuncu ona gitmedikçe konu açılmıyor. Phasing kancası hazır ve bedava: Tuğrul bir gün gömülürse bahçesinde yeni bir mezar ve yanan bir kandil kalır, kalıcı.
**İLK YİRMİ DAKİKA** — DIŞI, tek istisnayla: 11. dakikada meydanda, taş ustasının karşısında, yüksek sesle. Tartışma her gün aynı ve her gün baştan: kutsanmamış bir şehirde altı ismin oyulup oyulamayacağı. Usta *"taş taştır"* diyor, gassal *"taş sayılmaz"* diyor. Oyuncu durup dinleyebilir; kimse ona bakmıyor, kimse onu davet etmiyor.
**TARAF** — Taş'a yakın görünür (tapınak Taş'ın ayaklarından biri), ama kendini taraf saymıyor: usul taraf tutmaz. Kutsama gelirse Kılıç'ın seferi bile ona uyar — o gün sefer ölülerini de kurallı gömecek biri olur.
**DEVRALDIĞI KAYIT** — Yeni.

---

## (C) MEVCUT KADRONUN GÖÇ HARİTASI

`src/npc.c`'de bugün **18 kayıt** var (brifingde 19 deniyordu; `dunya_koy_sifaci` son satır ve `NPC_COUNT` ondan sonra kapanıyor). On sekizinin hepsi aşağıda; **hiçbiri silinmiyor, hiçbiri rafa kalkmıyor.**

Üçüncü sütun yeni: isim havuzu belgesi dinî/tarihî gerçek adların (Kemal, Osman, Fatma, Zehra, Nuh, Selim) oyuncuyu kurgudan çıkardığını söylüyor ve köy artık Yenitaç'ın bir tarla ötesinde, yani oyuncu bu adlarla **ilk saatte** karşılaşıyor. `.id` değişmediği için ad değişikliğinin kod maliyeti sıfır (§8.1).

| # | Bugünkü id | Karar | Yeni id | Ad kararı | Gerekçe |
|---|---|---|---|---|---|
| 1 | `dunya_koy_tuccar` (Baran) | Yenitaç'a taşınır | `yenitac_carsi_tuccar` | **Kalır** | GDD §1.2c onu iki örnekte kanonik olarak "Baran" diye anıyor; değiştirmek belgeyi bozar. `simya_masa` hekime geçer. |
| 2 | `dunya_koy_ihtiyar` (Kemal) | Yerinde kalır | `cayir_koy_ihtiyar` | **Kurak** | Kemal = Atatürk; ilk saatte kurgudan çıkarır. Kurak = kurak yılında doğdu, köyün kendi usulü. `goblin_derdi` duruyor: goblinler **fosilin altındaki merdivenden** çıkıyor (LORE §9-1) — Karaocak galerisi doğuda, çayırın komşusu değil. |
| 3 | `dunya_koy_koylu` (Osman) | Yerinde kalır | `cayir_koy_ciftci` | **Dirlik** | Osmanlı kurucusu + güncel dizi. Dirlik: yolların açıldığı yıl doğdu — ve tarlası şimdi ordunun keseceği yolun üstünde. |
| 4 | `dunya_koy_demirci` (Mahir) | Yenitaç'a taşınır | `yenitac_kisla_demirci` | Kalır, **ocak adı eklenir: Tav** | `STATION_SMITH` başlangıç şehrinde olmalı. Karaocaklı bir demircinin iş adı olur, Yenitaç defteri onu "Demirci Mahir" yazar — iki ad bir adam. |
| 5 | `dunya_koy_muhafiz` | Yerinde kalır | `cayir_koy_muhafiz` | Görünen ad "Muhafız" — **ad verilecek** | Tek muhafızı taşırsak köy kanunsuz kalır. Yenitaç kapıcısı yeni kayıt; çoğullama kararı slot 14'te. |
| 6 | `dunya_koy_kandilci` (Fatma) | Yerinde kalır | `cayir_koy_kandilci` | **Petek** | Peygamber ailesinden ad + Türkiye'nin en yaygın kadın adı. Petek = bal mumu kandilin geldiği yer; kadının işi zaten o. K9 zinciri Eski Geçit'e bağlı, geçit duruyor. |
| 7 | `dunya_koy_sofraci` (Nazife) | Yerinde kalır | `cayir_koy_hanci` | **Kaymak** | GDD §1.2c-4 phasing'in çalışan tek örneği onun sofrası; ama örnek `.id`'ye bağlı, `.name`'e değil. Naz- kümesi Nazlı ile aynı sahnede geçecek. |
| 8 | `dunya_ova_sayan` (Nuh) | Birkaç yüz metre taşınır | `cayir_fosil_sayan` | **Ömür** | Peygamber adı, üstelik tufan çağrışımı dev fosilin yanında yanlış kapıyı açıyor. Ömür yaşlı havuzundan, yani bu adam köylü değil köye **getirilmiş** — fosili sayan adamın oraya niye ait olmadığını ad söylüyor. Silme testine dayanıklı. |
| 9 | `dunya_koy_torun` (Zehra) | Yerinde kalır | `cayir_koy_torun` | **Sulak** | Aynı dinî register. Sulak = sulak yılında doğdu; dedesinin adının tersi, köy buna gülüyor. `SF_S3_HINTS` bozulmuyor. |
| 10 | `dunya_ova_yabanci` | Yerinde kalır | `cayir_kuyu_yabanci` | **Adsız kalır** | Gece bir kez beliren, kim olduğu söylenmeyen adam: Kabukçular'ın "Kav" rütbesi için hazır tohum. Adsızlığı bir tasarım, eksik değil. |
| 11 | `dunya_koy_kervanci` (Selim) | Yerinde, rolü büyür | `cayir_koy_kervanci` | **Kumral** | Üç Osmanlı padişahı. **GEZGİN KADRO'nun ilk adayı** (§1.2e): Yenitaç ↔ Kumkapı rotası. `traveler` + `TravelLeg route[8]` gerekiyor. Nazlı'nın pano önünde hecelediği kervan ilanı bu adamın rotası. |
| 12 | `loca_salon_usta` (Neriman) | Yerinde kalır | değişmez | Kalır | Sessiz El locası = **Kısıklar'ın kuzey hücresi**. "Eski saray celladı" artık gerçek bir saraya işaret ediyor — kanon zenginleşiyor, kod değişmiyor. |
| 13 | `loca_tunel_gozcu` (Çelme) | Yerinde kalır | değişmez | Kalır | İş adı; Kısıklar dokusuna oturuyor. |
| 14 | `loca_hucre_zehirci` (İğne) | Yerinde kalır | değişmez | Kalır | Aynı; damar taşı ezilip zehir olduğu için ileride tedarik hattı bile hazır. |
| 15 | `tasdurak_dukkan_tuccar` (Ferah) | Yerinde kalır | değişmez | Kalır | Taşdurak "Sultanlık'ın ucu" değil, **Kumkapı yolundaki çöl durağı**. **Ayrı harita** (`MAP_DOGU`), kervan yolunun güney ucunda; oyuncu oraya yürüyerek değil **kervanla** varır (LORE §4: çöl güneyin ucunda). Harita, dekor, ekonomi aynı. |
| 16 | `tasdurak_han_hanci` (Duman) | Yerinde kalır | değişmez | Kalır | Aynı. |
| 17 | `tasdurak_konak_katip` (Kâtip Rüstem) | Yerinde kalır, **adı değişir** | değişmez | **Vasfi** | Mareşalle ad çakışması. `.id` rol taşıdığı için tek `.name` düzenlemesi yeter — §8.1'in söktüğü bombanın çalıştığının kanıtı. (Rasim önerisi düştü: taş ustası Rasih kanona girdi.) |
| 18 | `dunya_koy_sifaci` (Ela) | **Yerinde kalır**, rolü keskinleşir | `cayir_koy_otaci` | **Karar bekliyor** — §2.7 bitki havuzu (Mürver / Zufa / Hatmi) | Eski karar (Yenitaç revirine dönüştür) iptal. Otacı, hekimin **rakibi** ve üç otun ikinci alıcısı; ikisinin nefreti GDD §1.2c-7'yi bedavaya karşılıyor. Ot adı taşıyan bir kadının hekim karşısındaki yeri isimden okunur, o yüzden ad değişikliği burada işlevsel. |

**Kaybolan içerik: sıfır — ve bu sefer denetledim.** Rafa kalkan hiçbir şey yok (eski sürümde Ela'nın cadı-şifacı hattı rafa kalkıyordu, artık kalkmıyor: yerinde duruyor ve bir mekanik kararın taşıyıcısı oldu). İki kimlik yeniden okunuyor (Yabancı → Kav tohumu, Taşdurak → çöl durağı), sekiz görünen ad değişiyor ya da karara kalıyor, on iki id yeniden adlandırılıyor, bir NPC birkaç yüz metre taşınıyor, iki NPC şehre taşınıyor. Silinen kayıt, silinen diyalog dosyası, silinen görev yok.

---

## (D) İLK YİRMİ DAKİKA — HANGİ DAKİKADA HANGİ SLOT

### Vinyete işlenecek üç düzeltme

`assets/scenes/intro_demirboyun.txt` şu hâliyle GDD §1.2d'nin en yüksek öncelikli kararını çürütüyor: pano ilanı 19. dakikada değil **birinci dakikada** iş veriyor. Üç düzeltme, hepsi metin düzeyinde:

1. **Panel 4'ten *"Kışla kâtibine sorulur"* silinecek.** O üç kelime düpedüz bir görev yönlendirmesi ve tek başına bütün "pano doğrulamadır" tasarımını götürüyor.
2. **Buz ilanı okunaksız görünecek.** Panelde kâğıt var, çivi var, altı akçe seçilebiliyor, gerisi **yarı yırtık**. Tam metni 19. dakikada okunuyor. Görsel notu buna göre güncellenecek.
3. **Çocuğun hece barkı YOK.** Kız okuma bilmiyor; kervan ilanını **tellaldan duymuş**, ezberlemiş, kendi kendine tekrarlıyor. İlk 20 dakikada tek satır ekonomi dersi verilmiyor ve "aslında okuyor" kalıbı — pislik içindeki çocuğun soylu olduğunun türdeki en tembel işareti — hiç devreye girmiyor. *(KR5: eski hâli kızı heceletiyordu ve kart 11 ile zıt karar veriyordu.)*

Vinyetin sayımı da düzeltilsin: dosyada **on iki satır** var, onu ağızdan ikisi yazıdan; ağızdan gelenlerin **yedisi** kartlarda bark olarak duruyor (mareşal, hekim, hatun, kâtip, tellal, taş ustası, gazi). "Dört replik" yazan başlık yanlış. Panel 1'in üst kat sesi **varise** ait (slot 10), aşağıdan cevap veren kalfa (slot 9); "ipin yediği yer" kalfanın imza deyimi ve yukarıdaki ses onu kullandığı için ikisi çakışmıyor, tersine kadının şantiye dilini bildiği anlaşılıyor.

### Çizelge

| Dakika | Vuruş | Devredeki slot(lar) | Oyuncu ne yapar | Ne öğrenir (metin kutusu yok) |
|---|---|---|---|---|
| 0-2 | Vinyet, canlı salonda ve sur dibinde | `iskele_varis` (panel 1, üst kat sesi), `iskele_kalfa`, `salon_maresal`, `revir_hekim`, `salon_hatun`, `salon_katip`, `salon_bey` (sessiz), `meydan_tellali`, `meydan_tasustasi`, **`revir_gazi`** (panel 4) | Bakar, panelleri ilerletir | Altı ağız var, bir sandalye var, kimse ona bakmıyor |
| ~1.5 (90 sn) | Cevapsız görüntü | Slot yok — **vinç ipinde kıpırdamayan taş taht** | Yukarı bakar | Bu şehirde bir şey on bir yıldır asılı |
| 3 | İlk insan sesi, sana değil | `iskele_kalfa` (AmbientHook: çivi sayımı) | Yanından geçer | Dünyanın kendi işi var; sayı tutuluyor |
| 5 | Seni gören ilk göz | `iskele_civici` | Yaklaşır → kaçar; çömelir → kaçmaz | **Çömelme.** Ve hatırlandığını |
| 7 | Oyunun konusu | `salon_sayman` (ambient, odaya söylenir), arkada beş ağız | Salona girer, dinler | Mesele taç değil; para ve zaman. Duvarda **altı boş çivi** |
| 8 | Kuyruk | `revir_gazi` (görülür, konuşulmaz) | Geçer | Bir kalabalık bir defterin açılmasını bekliyor |
| 9 | **İlk talep** | `kisla_seyis` — "şu kayışı tut" | Kayışı tutar; kayış çözülür, **çuvallar kapağın ağzında kalır** | Talep ≠ görev. Ve mahzen kapağı açık. Çuvalı **kimse istemiyor** |
| 10 | Sessiz araçlar | `kisla_demirci` (ocak), `kisla_kapici` | Görür, isterse konuşur | **E tuşu** (susan adam) |
| 10-11 | Haklı olmayan adam | `kapi_bacdar` (ağzı dolu, kuyruğun gölgesinde) | Yürür geçer; cümle arkadan gelir | Bu şehirde biri halloldu ve halloluşunu bir nasihat sanıyor |
| 11 | Şehrin sesi ve bir kavga | `meydan_tellali` + `meydan_tasustasi` + `mezarlik_gassal` | On adım; isterse durup dinler | Tören ertelendi; taş on bir yıldır boş; ve iki adam bu taşın sayılıp sayılmayacağını tartışıyor |
| 12-13 | Varisin yüzü + yanlış düğüm | `iskele_varis` (yirmi beş metrede), `iskele_makaraci` | İsterse tırmanmasını izler, isterse konuşur | Kimse ondan bir şey istemiyor. Ve bu düğüm 9. dakikadaki düğüm değil |
| 14 | Dolu yatak | `revir_hekim` (revirden geçiş) | Bakar | Birinin eli ezilmiş. Sebebi bir sistem değil, bir adam |
| 15-18 | **İlk ölüm riski** | `salon_bakici` (mahzende, kandili tutan) + tek düşman | İner (çuvalı alarak ya da almadan), dövüşür ya da kaçar; **çizmeyi okur** | Korku sesten gelir: duvarın ardındaki soluk. Ve bir soru: neden gömülmüyor |
| 19 | **Pano — doğrulama** | `kisla_katip` | Panoyu okur; parasını alır **ya da almaz** | Pano iş vermiyor, **iş görüldüğünü söylüyor**. Çuvalı bırakmışsa: *"Çuval kapıda duruyor."* |
| 19+ | Dünya devam ediyor | `carsi_asci` (kazan kurulur), `iskele_kalfa` (surda meşale) | Bakar | Akşam oluyor; kandiller bir yerde bitiyor |

**Mahzene dört giriş, dört slot:** kızak kayışı (`kisla_seyis`, 9. dk) · akşam kapı barkı (`kisla_kapici`) · gece nöbet değişiminde kilitten (`kisla_kapici`'nin rutin boşluğu — refakatçiyi hiç görmez, hikâyeyi yalnız çizmeden alır) · pano (`kisla_katip`, 20. dakikadan sonra, en zayıf yol).

**Buz kimin işi:** kimsenin. Seyis kızağı çeker gider, hekim buzu sipariş etmiştir, kâtibin defterinde bir kalem vardır ve çuvallar kapağın ağzında durur. Oyuncu aşağı inerken alır ya da almaz; ne rica edilir, ne hatırlatılır, ne engellenir. Ödemenin çuvala bağlı olması bu tasarımın en ucuz dersidir: pano yapılmış işi doğruluyor, yapılmamış işi de doğruluyor.

**20. dakikada elinde:** bir soru (neden gömülmüyor), bir kelime (*"Ben rehin değilim"*), bir ses, bir tanıdık (kandili tutan adam), ve kimsenin vermediği iki hedef. Sıfır zincir başlatıldı, sıfır günlük satırı birinin ricası.

**Karşılaşma bütçesi (§1.2e):** ilk 20 dakikada **iki**, asla üç. Bu çizelgede zaten NPC yoğun bir şehir merkezi var; RoadNode karşılaşması ilk yarım saatte **tetiklenmemeli**. Şehir içi rutinler karşılaşma sayılmaz.

---

## (E) BOŞLUKLAR VE RİSKLER

**Silme testini geçti — 23 slotla yeniden denetledim.** Akkemer'i, mühürleri, damar taşını, ejderhaları belgeden tamamen silin: hiçbir slot düşmüyor. Sandalye duruyor, altı ağız duruyor, para bitiyor, çivi çalınıyor, düğüm kayıyor, bacdar etini yiyor, gassal kâğıdını bekliyor, pano 19. dakikada aynı kâğıdı gösteriyor. Fantastiğe dokunan üç yer var ve üçü de çift okumalı: `cayir_fosil_sayan` (fantastiksiz: adımlarını sayan ihtiyar), mahzendeki duvar sesi, ve mezarlıkta ot toplamak (fantastiksiz: mezarlık geceleyin karanlık ve orada köpek var).

**Mahzenin altı mühür DEĞİL — bağlayıcı.** LORE §9 yedi mührü tek tek sayıyor ve Yenitaç'ta olan tek mühür çayırdaki fosil. Kışla mahzeninin altındaki katman LORE §12'nin **Akkemer su yolu ve lağımı**; ses akıntı ve havalandırma. Fantastik okuma "burada sekizinci bir mühür var" değil, "**bu şehir Akkemer'in üstüne kurulmuş**". Bu, Sabri'nin kendi çocukluğunun altında yürüyor olmasıyla birleşiyor ve bedava geliyor. Karaocak'ın duvar ardı nefesi Karaocak'ta kalıyor.

**Beş oyuncu ilk yirmi dakikada ne yapıyor.** Savaşçı mahzene iner ve dövüşür. Hırsız gece kilitten girer, kimseyle konuşmaz, çocuğun cebinden çiviyi alır. Tüccar kayışı tutar, altı akçesini alır, tezgâhı bulur. Şövalye salona girer ve odayı dinler. **Büyücü hâlâ boşta** ve bu bu kadronun en gerçek açığı: Yenitaç'ta büyü hattı yok, çünkü Mühürcüler Yedisunak'ta. Bu turda bir parça iyileşti — mezarlıkta ot toplayan bir otacı, kriptaların üstünde bir yıkama taşı ve rakip bir hekim, iksir hattının başlangıcı olarak Yenitaç'ta duruyor. Ama iksir büyü değil. Kalan kısmi teselli kodda: `smith_asa`, `alch_mana_iksiri`, ve **sahipsiz `STATION_ENCHANT`**. Kodda o istasyonun konumu bugün Karanlık Orman sınırı (`craft.c:25` yorumu: "keşifle bulunur"); **önerim yeni bir konum**, çayırın kenarı — tek satır koordinat değişikliği, sahibi yine kimse olmayacak. Büyücüye Yenitaç'ın verdiği tek şey kimsenin sahiplenmediği bir taş olsun; kadroya "büyücü hocası" eklemek "sen büyücüsün" demenin kibar hâli olurdu.

**Salon ağırlığı biraz düzeldi.** Yirmi üç slotun altısı sarayda (eskiden yirmide altı). Salon artık ayrı harita olduğu için sarayın kalabalığı şehrin kalabalığından ayrıldı: oyuncu kapıdan geçmedikçe altı ağzı görmüyor. Bunun bedeli var — kışla/iskele/çarşı/kapı tarafını ilk sürümde **dekor ve kalabalık** taşımak zorunda: konuşmayan işçi grupları, kuyruk, kazan başındaki sırt. NPC eklemeden yapılmalı.

**Kodda olmayan, tasarımın dayandığı şeyler.** Maliyetler kaba ama koda bakılarak verildi:

- `AmbientHook { npcId, hourFrom, hourTo, dialogNode }` — **küçük/orta**. Yedi slot buna dayanıyor (kalfa, gazi, taş ustası, aşçı, kesedar, makaracı, gassal). Bu olmadan pano yeniden görev kaynağına döner ve 3. dakika vuruşu boşa çıkar. **En yüksek öncelikli eksik bu.**
- **NPC harita geçişi** — **küçük/orta**. (A)'da ayrıntısı var: `NpcStop`'a bir alan, bir yardımcı, `npc.c`'de beş nokta. Salon kararının bütün faturası bu ve dört slotu birden açıyor.
- **Durak gün maskesi** — **küçük**. `NpcStop` yalnız saat taşıyor. Mareşalin "haftada iki kez buz" detayı bugünkü yapıda ya her gün olur (gülünç) ya hiç olmaz. Aynı sorun pazar günü, ateşkes bayramı ve tahsildar sırası için de çıkacak; bir kere çözülsün.
- `DVAR_ITEM` + envanter efekti, ve `DVAR_FLAG` — ikisi de **küçük** (enum + `main.c`'deki değerlendirme; bugün yalnız `DVAR_HINT` var ve tek bayrağa gömülü). Sarp'ın tepsisi, otacının üç otu, tellalın değişen ilanı ve kâtibin takvimi bu ikisine muhtaç. Tepsi bulmacası ayrı bir panel isterse **orta**.
- **Kontrat tarafında iki küçük iş:** storyFlag'le tamamlanan kontrat, ve rotasyona girmeyen sabit ilan slotu. 19. dakika bunlara bağlı.
- `RoadNode` — **orta/büyük**. İlk 20 dakikada gerekmiyor; iki Yenitaç slotu sahiplenmeye hazır.
- `traveler` + `TravelLeg` — **orta**. Kervancı bekliyor, ve Nazlı'nın hecelediği kervan ilanı onun rotası.
- **Çoklu muhafız + sahip alanı** — **orta**. Yenitaç kapıcısı bunsuz köyün muhafızıyla aynı defteri paylaşır.
- **Yaklaşma tetikli kaçış** — **küçük**. `npc_walk_to` var, tetik yok. Beşinci dakikanın tamamı buna bağlı.
- **Cine modunda harita geçişi** — **küçük**. Vinyet iki haritada geçiyor.

**Eski belgedeki iki teknik iddia yanlıştı, düzelttim.**

- *"`contract.c` bugün kabul adımı istiyor, sökülmesi zorunlu."* Sökülmüş. `contract.c:88-113` bunu gerekçesiyle yazıyor; `contract_accept` boşta duruyor (kanon kuralı gereği silinmemiş, zincir görevlerine bağlanabilir).
- *"Revirde tek yatak konumu var, iki adam aynı gün düşerse tanımsız."* Değil. `npc_render_infirmary` yatakları yan yana diziyor (`npc.c:653-656`, `bx = bedX + bed*56`), yani birden fazla hasta zaten çiziliyor. Kalan borç kurgusal değil görsel: yatak sayısı dekorla sınırlı, ve gece revir kapalı olduğu için "gece düşen adam" sorusunu kurgu kapatıyor.

**Ekonomi ölçeği kodla uyumlu çıktı.** (F)'deki merdiveni `item.c` fiyatlarıyla karşılaştırdım: kase 2, kızılot 6, meşale 5, can iksiri 25, uzun kılıç 200, demir kılıç 320. Bir işçinin günlük yevmiyesi 6-8 akçeyse demir kılıç kırk günlük yevmiye eder — bu ortaçağ için doğru bir orandır, rebalance gerekmiyor. Tek göze batan `goblin_derdi`'nin 100 akçesi: bir işçinin on altı günü. **Bilerek böyle kalsın.** Maceracının kazandığı para şehri kızdıracak kadar çok olmalı; kadronun yarısının oyuncuya bakışının sebebi bu ve sistemin çelişkisi değil, oyunun konusu.

**Kalan bir açık.** Yirmi üç slotta hâlâ beş kişinin kartında "kaybolursa kurum çöker" yazıyor (mareşal, saray kâtibi, bakıcı, sayman, kesedar). İkisini çevirdim, beşi kaldı ve dördü savunulabilir (kasa ile arşiv gerçekten tek kişide, işaretler gerçekten tek kişide). Kesedar için savunmam yok — bir tefecinin yerine ikinci bir tefeci gelir. Kartlar yazılırken orası da düşecek.

---

## (F) PARA MERDİVENİ — üç birim, üç ağız

*Bu bölüm GDD'ye taşınacak. Sebebi basit: kadronun altı ağzı sayıyla konuşuyor ve aynı dünyada bir kase çorba iki, bir tefecinin bütün defteri dokuz yüz, mareşalin beslediği adam sayısı on iki bin. Ölçek yazılmadan bu rakamlar birbirini yalanlıyor.*

**akçe — gündelik ölçek.** Elle tutulan para bu. Kase çorba 2, meşale 5, bir ot 6, işçi yevmiyesi 6-8, mahzene buz taşımak 6 (yani tam bir günlük yevmiye — ilanın altı akçesi bu yüzden hem az hem çok). Oyuncunun kesesindeki her şey akçe: kodda tek para alanı var (`player.gold`) ve **ikinci bir para birimi eklenmeyecek.** Kese ile sikke envanterde görünmez; onlar konuşma ölçeğidir.

**kese — bin akçe.** Tuzkapı'nın kesecileri adını buradan alıyor ve "Kırk Kese" bir meclisin adı olmadan önce bir hacimdi. Şehir bütçesi, iskele kalemi, saray kasası kese ile konuşulur. *"Kasada iki yüz kese var, üç ay eder."*

**sikke — devlet ölçeği, yüz kese.** Elde taşınmaz, masada durur; gümrük, bac, kira ve ganimet sikke ile yazılır. Bunun bedava bir yan etkisi var: Karaocak'ın **kendi sikkesini basması** (LORE §4) küçük para basmak değil, **devlet ölçeğinin kendisini** basmak demek — bir şehrin devlet gibi hesap tutması. Rüstem'in sormaması bu yüzden bu kadar ağır. Aynı şekilde kesedarın terazisinde beliren **güney sikkesi**: bir tefecinin masasında devlet ölçeğinde bir para durması, o masada olmaması gereken bir şeyin olduğunu kimse açıklamadan söylüyor.

**LORE'un "kırk gümüş"ü bir birim değil, bir usul.** Karduvar diyet kalemini kırk parça gümüş olarak gönderir; sayılır, tartılmaz. Bir çocuğun bedeli akçeye çevrilmiyor ve çevrilmediği için acıtıyor.

### Karakterizasyon: aynı odada iki kadın, iki birim

**Sayman kese konuşur.** Devlet rakamını asla akçeye bölmez — *"Kırk gün, kırk kese."* Bir sayman için akçe konuşmak, masanın ölçeğini bilmemek demek.

**Kesedar akçe konuşur.** Aynı rakamı *"kırk bin akçe"* diye söyler ve kese'ye yuvarlamayı reddeder, çünkü onun defterinde yuvarlanan her akçe birinin bir günü. İkisi aynı odada aynı sayıyı iki dille söylüyor ve oyuncu hangisinin nereden geldiğini kimse anlatmadan duyuyor.

Ve bedava bir tavan: masaya **kese diliyle** konuşmayan yazmanı Kese Sahibi yapmıyorlar. Kadının kariyerini tıkayan şey bir hata değil, bir birim tercihi — ve tercihinden vazgeçmiyor.

---

## REVİZYON NOTU

**K3 · Salon kararı tersine çevrildi.** Salon `MAP_SALON`, mahzen `MAP_MAHZEN`, ikisi de enum sonuna (save güvenli; `save.c:490` doğruladı). Eski gerekçe bir kod faturasıydı; (A)'da artık "kanon kararı" başlığı altında değil, **MEKANİK BORÇ** başlığı altında ve maliyetiyle duruyor. Raporun sunduğu iki seçenekten birincisini (NPC harita geçişi mekaniği) aldım, ikincisini (avlu/salon iki ayrı durak) **reddettim**: ikincisi salonu dünya haritasında bırakıyor, yani duvarı ve kapıyı geri getirmiyor, yani sorunun kendisini çözmüyor. İkiz kayıt yaması iptal edildi ve bunu (A)'da açıkça yazdım. Sabri'nin mahzen durağını 19'dan 15'e çektim — Ö4'ün "Sarp'ı 21'de kim yatırıyor" sorusu böylece kendiliğinden kapandı.

**Ö1 · Pano.** Üç düzeltmenin üçü de (D)'nin başındaki "vinyete işlenecek" bloğunda talimat olarak duruyor. **Bir yerde eğdim:** çocuk kervan ilanını tam okumuyor, tek kelimesini söküp gerisini tellala soruyor — ve tellal da okuyamıyor. Sebep, klişe raporunun ayrı bir maddesi: pislik içindeki çocuğun okuma yazma bilmesi, "aslında soylu" demenin türdeki en tembel işareti. Kervan ilanı kaldı, hece barkı kaldı, tembel sinyal gitti, üstüne kadronun eksiği olan bir **sohbet** çıktı.

**Ö2 · Buz.** 9. dakika satırına "çuvallar kapağın ağzında kalır" girdi, 19. dakika satırına ödemenin iki dalı girdi, çizelgenin altına "Buz kimin işi: kimsenin" paragrafı eklendi. Slot 15 ve slot 13 aynı şeyi iki taraftan yazıyor.

**Ö19 · Üç ot.** Ela rafa kalkmadı: `cayir_koy_otaci` olarak yerinde, hekimin rakibi ve ikinci alıcı. Toplama yeri mezarlık, mezarlık da yeni slot 23'ün mekânı — bir konum üç iş yapıyor. Göç tablosunun 18. satırı ve slot 7 buna göre yazıldı. Uygulama kararı bende kaldı: ikinci alıcı için `QUESTS[]`'e yeni satır **eklemedim** (görev tek, teslim ağzı iki), bunun yerine iki küçük mekanik işaretledim.

**Ö20 · Vinyet kadrosu.** `revir_gazi` 0-2 satırına girdi. Panel 1'in üst kat sesi **varis** oldu, dolayısıyla slot 10'un "12-13. dakikada görülür" satırı "ilk duyulan ses 0-2, yüz 12-13" oldu. "İpin yediği yer" kalfanın imzası olarak kaldı ve yukarıdan gelen ses onu kullanıyor — çakışma yerine karakter bilgisi çıktı. **Sayıyı ise raporun dediği gibi yazmadım:** dosyayı sayıp yazdım — on iki satır, onu ağızdan, yedisi kart barkı.

**N7, N9, N10, N15 · uygulandı.** Ölü §G referansı `docs/konsept/reboot/22-demirboyun.md`'ye bağlandı. Goblinler fosilin altındaki merdivenden çıkıyor. Taşdurak ayrı harita, güney ucu, kervanla varılıyor. `STATION_ENCHANT` "yeni konum" olarak yazıldı, "zaten öyle" olarak değil.

**N8 · uygulandı ve denetlendi.** "Kaybolan içerik: sıfır" cümlesi artık doğru, çünkü rafa kalkan tek şey (Ela) rafa kalkmıyor. Cümleyi tekrar yazmakla kalmadım, altına neyin ne olduğunu tek tek saydım.

**N11 · Sayımı REDDETTİM ve yeniden saydım.** Raporun düzeltmesi de eksikti: `main.c:1409-1417` aralığında yedi değil **dokuz** `npc_index_by_id` çağrısı var (grep ile doğruladım), sekizi etkileniyor, biri (`loca_salon_usta`) etkilenmiyor. `craft.c` iki. Toplam **10** — raporun dediği 8 değil, eski belgenin dediği 14 de değil. "Hepsi yan yana" ifadesi doğru ama yanıltıcıydı; çağrıların yanı sıra dört id karşılaştırması daha var ve bu turda etkilenmiyor — onları da ayrı satır olarak yazdım ki sonraki tur aramak zorunda kalmasın.

**A4 · Üç yeni slot + iki "yerine biri konabilir".**

- **20 · bac tahsildarı (memnun adam).** Memnuniyetini yapısal bir yere oturttum: kasa boşalsa da kapı bacı kesilmez, yani adamın haklı olmasının bir **sebebi** var ve bu onu daha dayanılmaz yapıyor. Öğle yemeğini kuyruğun gölgesinde yiyor. Mekanik bağı bedava: §11'in "İzinli" kademesi tam olarak onun elinden alınıyor, dolayısıyla oyuncu yükseldiğinde tavrının dönmesi ekstra sistem istemiyor.
- **12 · makaracı (beceriksiz).** Kovulamama sebebini akrabalıktan aldım ama kanona bağladım: mareşalin adamının oğlu, ve onu koruyan şey Rüstem'in kendi kuralı. Sonucu ilk dakikadan itibaren ekranda (revirdeki dolu yatak) ve oyuncu bağlantıyı 14. dakikada kendi kuruyor. Karşılaştırma tasarımı bedava: 9. dakikada doğru bağlanmış bir yükü eliyle tutuyor.
- **23 · gassal (inançlı).** Raporun "taşın boş olmasını **uğursuzluk** sayan biri" önerisini **reddettim**: uğur/uğursuzluk register'ı KORKULUK'a doğru kayıyor. Adamın derdi eksik bir imza — Yedisunak'tan tek satır. Böylece endişesi usulsüzlük kalıyor ve LORE §1.9 güçleniyor. Yer olarak mezarlığı seçtim: Ö19'un üç otu, Tuğrul'un gömülmeme sorusunun tek muhatabı ve LORE §12'nin kripta gerekçesi aynı ekranda buluşuyor. Taş ustasıyla her gün **yüksek sesle** kavga ediyor — klişe raporunun yedi kez saydığı "ikisi de bunu hiç konuşmadılar" tikinin panzehiri.
- **Yerine konabilecek ikisi: 9 (kalfa) ve 21 (tellal).** Gerekçe slotlarının içinde: ikisinin tuttuğu kurum aslında işlemiyor (dinlenmeyen ilan, bitirilmesi yasak inşaat), dolayısıyla vazgeçilmezlikleri baştan yalandı. Tellal için ek bir kazanç var: onu tutan şeyin **okuyamaması** olduğunu kendisi biliyor ve bunu kâtiple bir kez yüksek sesle tartışmışlar — hem replaceable, hem konuşulmuş bir ilişki, hem kâtibi karartıyor.

**Ö3 · Para merdiveni (F) olarak yazıldı** ve kod fiyatlarıyla karşılaştırıldı; rebalance gerekmiyor. Sahibinin kararına iki şey ekledim: (a) sikke = devlet ölçeği tanımı Karaocak'ın kendi sikkesini basmasını yeniden okuyor ve o cümle şimdi çok daha ağır, (b) kesedarın akçe ısrarı kariyer tavanına dönüştü — masaya kese diliyle konuşmayanı Kese Sahibi yapmıyorlar.

**Kendi başıma eklediğim iki şey.** Mahzenin altının mühür DEĞİL, Akkemer su yolu olduğu bağlayıcı satırı (Ö10 — sekizinci mühür riski, (E)'de). Ve kodda yanlış çıkan iki iddianın düzeltmesi: kontrat kabul adımı zaten sökülmüş, revirde birden fazla yatak zaten çiziliyor. İkisinin yerine iki gerçek borç yazdım (storyFlag'le tamamlanan kontrat, rotasyona girmeyen sabit ilan) — 19. dakika vuruşu bugün bu ikisi olmadan çalışmaz ve eski belge bunu görmemişti.

**Bu turda yapmadığım.** Kart metinlerine dokunmadım (K1, K2, K4-K8, Ö5-Ö18, N1-N6, N12-N14 kart dosyalarının işi). Üç yeni slotun adını yazmadım. Yenitaç'ın büyücü açığını kapatmadım — kapatmanın tek dürüst yolu Yedisunak'ı beklemek, ve bunu bir eksik olarak (E)'de bırakıyorum.
