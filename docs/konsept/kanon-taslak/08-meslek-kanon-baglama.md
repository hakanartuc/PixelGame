Kodu okudum, iddiaları grep'le kontrol ettim. Önce envanter, sonra kanona bağlama.

---

# 1. BUGÜN KODDA GERÇEKTEN NE VAR

**23 skill var ve hepsinin canlı bir XP kaynağı var.** `src/skill.h:16`'daki "Sneak, Alchemy... panelde görünür ama level 1'de bekler" yorumu eskimiş; `skill_add_xp` çağrılarının tamamını taradım, XP'siz tek satır kalmamış. Toplama üçlüsü `gather_finish`'ten geliyor (`src/main.c:858`): madencilik düğüm başına 8 XP, oduculuk 6, bitkicilik 5. Craft skilleri tarifin `xp` alanından (`src/main.c:952`). Enchanting'in üç ayrı kaynağı var: tutmayan rün 5, rün basma 10, stat basma 12 (`src/main.c:7598, 7643, 7680`). Panelde isimler hâlâ İngilizce (`skill.c:74-97`) — Mining, Herbalism. Bu bedava bir kanon fırsatı, aşağıda kullanıyorum.

**Beş istasyon var, dördü ayrı tür.** Demirci Ocağı (Mahir, izin görevi `demirci_ocak`), Simya Kazanı (Baran, `simya_masa`), Kadim Sunak (`permQuest = NULL` — kapısız, T(102),T(14), yani harita kenarında keşifle bulunuyor), köy kamp ateşi ve oyuncunun kurduğu kamp ateşi (sabit slot 4, 3 Odun, sönükken haritadan park ediliyor). Kamp ateşinin **kapısı yok** ve bu `craft.c:26`'da açıkça yazılı: "ateş kimsenin değil".

**41 tarif var, tavan 48.** Ocakta 21, kazanda 12, kamp ateşinde 6, sunakta 2. `learned[]` dizisi tarif **indeksiyle** save'lendiği için (`save.c:471`) ortaya ekleme eski kayıtları kaydırır — bu zaten kodda uyarı olarak duruyor.

**Rün ve stat basma sayıları.** Rün *basmak* garantili: 3 saniye tören (`ENCH_RUNE_CAST_TIME`), 1 rün tükenir, silahın `fx` alanı değişir, eski efekt yanar. Rün *üretmek* şanslı: `60 + Enchanting×0.4 + ağaç bonusu`, tavan 95 (`main.c:7584-7587`). Yani level 1'de %60, level 100'de %100 değil %95. Tutmazsa 3 öz yanar. Stat taşları garantili ama pahalı: kademe *n* için *n* taş + *n*×2 Demir Külçe, tavan III, silah başına 2 slot (`craft.c:508`).

**Toplama düğümleri dengesiz:** 36 düğümün 22'si maden, 9'u bitki, 5'i ağaç (`src/resource.c`). Tavan 64.

**Öğretmen sistemi var ama sadece üç skill için:** `train:smithing`, `train:speech`, `train:alchemy` (`main.c:2737`). Altın → XP, günde bir.

---

# 2. MESLEKLERİN KANONDAKİ ANLAMI

## Madencilik — bir gövdenin içi

Kilnmoor'un galerileri bir ejderhanın kaburgaları arasından geçiyor ve madenci bunu kelimeyle öğrenmiyor: duvar önce cüruf, sonra yivli bazalt, sonra **kavis** yapıyor. Kazma sesi bir yerde değişir. Ocakdoğanlar buna "ocak" diyor ve konuşuyorlarmış gibi davranıyorlar.

Kodda değişen şey yok: `ITEM_ORE_COPPER` → **Kor Kırığı**, `ITEM_ORE_IRON` → **Ham Kor**. Ham korun `minSkill = 15` olması artık bir kanon: ham kor sıcak ve kararsız çıkar, acemi tutamaz. Ve oyuncu koru **çıplak elle taşıyamıyor** — Ocakdoğanlar taşıyabiliyor. Bunun kodda hâlihazırdaki karşılığı **Kazma'nın kuşanılı olma şartı** (`toolItem = ITEM_PICKAXE`, `resource.h:27`). Anlatı: kazma değil, kazmanın ucundaki çamurlu testi. Sıfır kod, tek satır ipucu metni.

Riski de hazır: `NEST_POOL` cüruf gölcüğü ve kor ağzı — ısıyı izleyen kör bir şey. Üstünde kor taşıyan madenci avlanır.

## Oduculuk — odun ateşi ölüyü durdurmaz

Bunu bir kusur olarak bırakmıyorum, **kamp ateşinin zaten kapısız olması** üstüne kuruyorum. Oyuncu 3 Odunla ateş kurar, ışık gelir, et pişer, `light_append_campfire` çalışır (`src/light.c:117`) — ve o gece hiçbir şey susmaz. Ateşin çevresi aydınlık ama sessiz. Oyuncu bunu bir tabelayla değil, ateşin dibinde topraktan çıkan bir elle öğrenir. Odun ateşi seni **gösterir ama korumaz**; korun tam tersi.

Odunun asıl işi başka yerde: yakma çukuru. `NEST_GRAVE` ritüeli kodda hazır (bütün mobları öldür, E'ye bas, saha temelli kapanır — `ecology.c:54`). Buna kamp ateşi desenindeki gibi **3 Odun şartı** koymak yeterli. Böylece Pyrewright'ın "on bir gün, kırk bir gümüş" fiyatının odun kalemi oyuncunun elinde bir seçenek olur. Odun ayrıca fitil sapı, tabut, ve iskele — ama bunları söylemek yeterli, tarif yapmak gerekmiyor.

## Bitkicilik — tuz ve fitil

`ITEM_HERB_BLUE` (Ayotu, bataklık kenarı) → **Fitilotu**. Sebebi kodda: kamp ateşinde 2 Ayotu → 1 İp (`craft.c:168`). Bu zaten fitil bükmek. `ITEM_ROPE`'un adını **Büküm** yapıyorum — hem fitil hem kayış, aynı kelime, çanta tariflerini bozmadan.

`ITEM_HERB_RED` (Kızılot, ova çayırı) → **Tuzotu**: Saltmere'den kuzeye kadar tuzlu toprakta yetişen, külü tuz veren ot. Soğuk közün tuzu buradan geliyor. Ve bitkiciliğin hava bağı zaten kurulmuş: yağmurdan sonraki gün ot ×2, karda yarıya iner (`main.c:845-852`). Kanon: kış geceleri uzun, ölü saati uzun, fitil pahalı, ot yok. Kış bu oyunda bir ekonomik kriz.

## Demircilik — kor çeliği bir kadran

Kor çeliği kavgayı kısaltmayı öğretmeli, cezalandırmamalı. Kodda **kadranın iki ucu da hazır**: silahın `fx` alanı (rün) ve `light.c:60-93`'ün oyuncuyu ışık kaynağı yapması. Kor gömülü silah karanlıkta daha ağır vurur *ve* seni iki mil öteden gösterir. Doğru oynanış: gece kılıcı çek, işi bitir, kını kapat.

Dövmenin neden zor olduğu da kodda: **vuruş = iş**. Çekiç kuşanılı olmadan ocak ilerlemiyor, her tıklama bir savurma (`main.c:9056`). Kor çeliği için bunu bir kanona bağlıyorum: ham kor soğutulmadan dövülmez, ve `2 cevher → 1 külçe` tarifi tam olarak soğutmadır. `ITEM_INGOT_COPPER` → **Söndürülmüş Kor** (yumuşak, kandil yakıtı, sikke), `ITEM_INGOT_IRON` → **Kor Çeliği**. Külçe tarifinin `minSkill = 10` olması Sultan'ın tekelinin oyuncu tarafındaki karşılığı: soğutmayı bilmiyorsan Külhan'dan almak zorundasın.

'+basma' kumarı (başarısızlıkta +1 geriler, külçeler her durumda yanar) artık bir kanon cümlesi: her vuruş kordan bir şey alır ve geri koymanın yolu yok.

## Simya — soğuk köz

`ITEM_ESSENCE_TOXIC` → **Soğuk Köz**: tuzda söndürülmüş kor tozu. Yara yok, kan yok, adam üşür ve sabah ölür. Kodda karşılığı hazır: Dilsiz Şişe (3 köz + 2 ot) ve İğne Zehri (1 köz → 3 şarj), ikisi de `blueprintItem 9999` yani sadece zincirle açılıyor. Simya bu dünyada iksir değil **iz bırakmama** üretir; Söndürücü sözleşmesinin makbuzsuz kolu burada.

---

# 3. SUNAK VE RÜN — rün bir harf

Bu bölüm işin kalbi, o yüzden uzun.

**Rün bir harf.** Sunakta bir silaha rün basmakla bir mezar taşına ad kazımak aynı el işi, aynı alet, aynı zanaat. Enchanting bu dünyada büyücülük değil **harf ustalığı** — imparatorluk iki bin yıl isim kaydetti, ve kaydı kazıyan eller hâlâ var.

**Sunak nedir.** Kodda "Kadim Sunak" bir dikilitaş (`obelisk.png`, 32×80), sahipsiz, izin görevi yok, Karanlık Orman sınırında keşifle bulunuyor. Bunu olduğu gibi alıyorum: **bir isim direğinin kaidesi.** İlk Yol'un terk edilmiş bir kolunda kalmış, üstünde sütun sütun isim var, alt üçte biri boş bırakılmış — yeni ölüler için ayrılmıştı, hiç dolmadı. Kimsenin olmaması bir eksiklik değil kanon: batıda bir taşa sahip olan, kandil söndüğünde suçlanan olur. Bu yüzden `permQuest = NULL` doğru.

**Rün üretiminin şanslı olması ne demek.** Öz bir ölünün ağzından çıkmış yarım bir hece (`ITEM_ESSENCE_PALE` → **Hece**, goblin/iskelet/hortlak dropu — kodda beş ayrı loot tablosunda var). Bir rünün tutması, elindeki hecenin taşta bir harfe **oturmasına** bağlı. Oturmazsa hece yanar. `60 + Enchanting×0.4` bu yüzden doğru: acemi rastgele deniyor, usta taşa bakıp hangi hecenin nereye oturacağını görüyor. Tavanın 95 olması da kanon — hiçbir usta bir ölünün ağzından ne çıktığından tam emin olamaz. Ekranda değişen tek şey metin: "Rün tutmadı! Özler yandı" → **"Hece taşa oturmadı."** Sıfır kod.

**Oyuncu bir mezar taşına rün basabilir mi.** Evet, ve bu tek fikir isim ekonomisini bir mesleğe bağlıyor. Kodda sunak panelinin ilk özel satırı `CRAFT_ROW_TARGET` — tıklayınca kuşanılı ekipmanlar arasında dönüyor (`main.c:11614`, `enchTarget`). Hedef listesine bir giriş daha ekleniyor: **Taş.** Oyuncu bir mezar taşının ya da bir kandil direğinin yanında sunak panelini açar, hedefi Taş yapar, aynı 3 saniyelik töreni yapar, aynı 10-12 Enchanting XP'sini alır — ama sonuç bir silahın `fx` alanı değil bir dünya durumu.

İki fiil çıkıyor. **Ad kazımak**: taştan bir adı siler, o ölü bir daha çağrılamaz, saha kapanır. Bunun kodda karşılığı zaten var — `NEST_GRAVE` ritüeli. Tek yapılacak iş ritüelin XP'sini `SKILL_ENCHANTING`'e bağlamak, bir satır. **Ad kazmak**: Tibb'in defterinden satın alınan bir adı boş taşa yazmak. Cardew Halse gecenin ikisinde masasına oturur ve yazar. Bu bir Enchanting işi, kürek işi değil.

Dünya nasıl cevap verir: ad kazıyan oyuncunun bölge itibarı düşer (Pyrewright'lar lanetler, aile taşına dokunulmasını istemez, mevcut `reputation[]` bunu taşıyor), ama kasaba geceyi rahat geçirir ve bunu söyler. Kalıcı iz: o saha bir daha yuva olmaz. Bir kasabayı taşlarını kazıyarak kurtaran oyuncu, orayı adsız bırakmış olur ve otuz yıl sonra kimse kimin gömüldüğünü bilmez.

**Gizli sunak: Sağır Sunak.** Kilnmoor'un mühürlü derin galerisinde, iki kaburganın arasında. Üstünde tek isim yok — hepsi kazınmış, sistematik olarak, batıdan doğuya doğru. Orada basılan rün ters çalışır: silaha gömülü koru **kısar**. Hasar bonusu vermez; ışık ve ses vermez. Islak kül torbasının kalıcı hali, suikastçinin silahı, kervancının karanlık geçiş kararı.

Neden gizli: Ocakdoğanlar orayı kendileri mühürledi, çünkü o taşa basılan harf geri alınmıyor. Orada kendi adını basan biri bir daha çağrılamaz — ölünce kalkmaz, ama Pyrewright de yakamaz, Kurgan Defteri'nde satırı boş kalır. Ocakdoğanların iki tanesi bunu yaptı ve ikisi de artık kimseyle konuşmuyor. Oyuncu kendi adını da basabilir. Bedeli görünmez ve geri dönüşsüz: bir daha hiçbir ölü listesi seni tanımaz, yani kimse seni koruyamaz da.

Oyuncu bunu nasıl öğrenir: her bölgede duran o **ismi kazınmış direk** (03-hikaye-isleyisi §3) yedincide bir örüntüye dönüşüyor. Yedinci direğin keskisi Sağır Sunak'ın harfleriyle aynı ağızda. Kod: yeni bir istasyon girişi (STATIONS'a **sona** eklenmeli, slot 4 sabit), bir tarif, bir item.

---

# 4. BİRBİRİNİ BESLEYEN ZİNCİRLER

**Kor.** `Madencilik: Ham Kor` → `soğutma (2 → 1)` → `Kor Çeliği` → *demirci, kandil, sikke*. Oyuncunun yapamadığı halka soğutma: Smithing 10 altındaysan ya Ocakdoğan çukurunda ya Külhan'ın avlusunda yapılacak, ikisi de itibar istiyor. Tüccarın tek başına tamamlayamadığı ilk zincir bu ve ticaret buradan doğuyor.

**Fitil.** `Bitkicilik: Fitilotu` → `Büküm (kamp ateşinde, 2 → 1)` → `fitil` → *fitil ustası, kandil direği, Fitilci bezi*. Baştan sona oyuncunun elinde, kapısız, ilk saatte yapılabilir — ve fitil ustalığı beş rolün ortak ilk basamağı.

**Yakma.** `Oduculuk: Odun` → `3 Odun` → `yakma çukuru ritüeli` → *Pyrewright makbuzu, bölge kalıcı temiz*. Pyrewright olmadan da yapılabilir ama loncanın kapısı kapanır (kanonda yazılı: büyücü ateşle çözerse Pyrewright kapısı kapanıyor).

**Zehir.** `Kor Kırığı + Tuzotu` → `Soğuk Köz` → `Dilsiz Şişe / İğne Zehri` → *Söndürücü sözleşmesi*. Bu zincirin ortası kilitli: tarifler `blueprintItem 9999`, yani yalnız Sessiz El zinciri açıyor. Simyacı olmak yetmiyor, birine yazılman gerekiyor.

**Harf.** `Hece (ölü lootu)` → `sunakta rün` → `silaha veya taşa` → *silahını güçlendiren savaşçı, taşı kazıyan kasaba, ordusunu silen büyücü*. Tek zanaat, üç müşteri. Ve hecenin satın alınabilir olması iyi: kervan stoğunda `ITEM_ESSENCE_PALE` var (`shop.c:38`), yani avlanmayan oyuncu da girebiliyor.

**Defter.** Tibb'in kopyası → satın al/çal → ad → taş. Tek üretici bir çocuk, tek alıcı Osgar, ve oyuncu açık artırmaya giriyor.

---

# 5. ÜÇ KAPI

Kapılar kodda: izin görevi REWARDED değilse "Önce X ile konuş", bölgede bounty varsa "Önce cezanı öde", itibar `REP_NO_QUESTS` altındaysa "Ustanın sana güveni kalmadı" (`main.c:3358-3389`). `permQuest = NULL` olan istasyonda üçü de yok.

Demirci Ocağı → **Slake'in çukuru**. Tanışma görevi zaten onun repliği: "Sunağa git, yak, gel." Yani ocağın izin görevi bir sunak ziyareti — iki meslek birbirine kapıyla bağlanıyor, sıfır kod.

Simya Kazanı → **Brannock'un çadırı**. İzin görevi bir makbuz: bir baş getir, üç gümüş öde, defterine yazılsın.

Kadim Sunak → kapısız kalıyor, ve kanon bunu şöyle söylüyor: taşın sahibi olmak, karanlık geldiğinde suçlanmak demek. Kimse istemedi.

**Kızıl Kanca üyeliği Ocakdoğan kapısını kapatıyor** ve bunun kodda karşılığı bedava: üyelik Kilnmoor bölge itibarını `REP_NO_QUESTS` altına indirir, mevcut üçüncü kapı kendiliğinden kapanır ve mesaj bile doğru çıkar — "Ustanın sana güveni kalmadı". Kapı kapandığında geri gelmesi de kodda: itibar toparlanınca açılıyor. Yani ihanet geri alınabilir, ucuz değil.

---

# 6. BEŞ ROL, BEŞ SEBEP

Savaşçı **Smithing** öğrenir çünkü '+basma' kumarı onun kumarı, ve kor çeliğinin ifşa bedeli onun problemi. Sunağa da gider — STR ve HP taşları oradadır.

Büyücü **Enchanting** öğrenir çünkü harf işi arşiv merdiveninin ilk basamağı, ve bir hanenin ordusunu bir keskiyle silen kişi o. Bitkiciliği fitil ve kül için yapar.

Suikastçi **Alchemy** öğrenir (Soğuk Köz, iz bırakmayan ölüm) ve Sağır Sunak'ı arar; Lockpicking'i zaten alıyor.

Tüccar **Mining + Speech** alır, çünkü soğutma ekonomisinin iki ucu bu ve öğretmen sistemi tam olarak bu ikisini satıyor. Smithing'i çanta hattı için 40'a kadar götürür — taşıma kapasitesi onun için silahtan önemli.

Şövalye **Woodcutting** ve ad kazma kolunu alır: halk önünde yakma töreni ve halk önünde keski. İkisi de itibar mekaniğine yazıyor.

Hiçbiri tekelli değil. Beşi de sunağa gidiyor, beşi de farklı satıra tıklıyor.

---

# 7. KOD FATURASI

**Sıfır kod (yalnız isim/metin):** dokuz item adı (cevherler, külçeler, iki ot, iki öz, ip), üç istasyon adı + `ownerName`, iki `permQuest` görev metni, "Rün tutmadı" metni, `skill.c`'deki İngilizce skill isimleri, tarif adları. Kor ekonomisinin omurgası — ham kor soğutulmadan dövülmez — **bugün çalışıyor**, sadece adı yanlış.

**Küçük:** `NEST_GRAVE` ritüelinin XP'sini `SKILL_ENCHANTING`'e bağlamak (bir satır). Ritüele 3 Odun şartı (kamp ateşi deseni hazır). Kızıl Kanca üyeliğini bölge itibarına bağlamak. Sağır Sunak: bir istasyon girişi + bir tarif + bir item.

**Orta [YENİ MEKANİK]:** sunak hedef listesine **Taş** eklemek. `enchTarget` bugün bir `EquipSlot` indeksi; dünya nesnesi hedefi almak için hedef seçiminin genelleşmesi ve bir isim-durumu tablosu gerekiyor. Bu bir yarım-gün işi değil, bir günlük iş, ama isim ekonomisinin tamamı buna bağlı.

**Tavanlar — dikkat, bu projede beş kez buradan hata çıktı:**
- `MAX_ITEM_TYPES 96` (`src/item.h:8`), enum'da **92 dolu → 4 boş slot.** En sıkışık tavan bu. Sağır Rün bir slot yiyor. Paralel diziler: `item_textures.h:10-12` (üç dizi), `player.h:184` `itemCd[MAX_ITEM_TYPES]`, `save.c:71` ve `:399` `ITEM_TYPE_COUNT` doğrulaması.
- `MAX_RECIPES 48`, 41 dolu → 7 boş, ama `learned[]` indeksle save'lendiği için **yeni tarif sona** eklenmeli (`save.c:471`).
- İstasyon: `main.c:1196` `station_textures[8]`, 5 dolu → 3 boş. Uyarı: `station_render_all` `STATION_COUNT` boyunca dönüp `textures[i]`'ye erişiyor (`craft.c:268`) ama yükleme döngüsü 8'de klampli — STATION_COUNT 8'i geçerse okuma taşması olur. Yeni istasyon eklerken `station_textures` boyutu da büyütülmeli.
- `MAX_RESOURCES 64`, 36 dolu → 28 boş. Dağılım dengesiz (22/9/5); bitki ve ağaç düğümü eklemek ucuz.
- `SKILL_COUNT 23` — yeni skill gerekmiyor, hepsi kullanılabiliyor.

İlgili dosyalar: `/Users/hakanartuc/PixelGame/src/craft.c`, `/Users/hakanartuc/PixelGame/src/craft.h`, `/Users/hakanartuc/PixelGame/src/resource.c`, `/Users/hakanartuc/PixelGame/src/item.h`, `/Users/hakanartuc/PixelGame/src/skill.c`, `/Users/hakanartuc/PixelGame/src/skilltree.c` (satır 977-1050, meslek mini ağaçları), `/Users/hakanartuc/PixelGame/src/main.c` (7559-7700 sunak, 9034-9120 dövme/toplama, 3358-3396 üç kapı, 824-868 `gather_finish`).

[DOĞRULANMADI] Kor çeliğinin "seni gösterir" ucu için dünyada "burası aydınlık mı" sorgusu yok — ışık yalnız render tarafında yaşıyor. Kadranın ceza yüzü (gizlilik ve tanık yarıçapının ışığı sayması) bu sorgu yazılmadan çalışmaz.