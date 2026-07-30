# BECERİ · MESLEK · DAMGA · HAT — v2 (Demirboyun kanonuna göre)

> 2026-07-30 · Dört paralel tasarımcının sentezi · Kanon: `docs/LORE.md` v2
> Hakan direktifi: *"22 olmak zorunluluğumuz yok, skill yapısı baştan sona tekrar çalışabilir,
> mantığı baştan sona değişebilir. Ama varlıkları olmalı."* + balıkçılık + rün basma şart.

**Temel ilke: hiçbir beceri silinmiyor.** 23 mevcut becerinin tamamı kalıyor; üçü yeniden
adlandırılıyor, ikisi ağaç metni düzeltmesiyle kurtuluyor, ikisi yeni eklenıyor → **25 beceri**.

---

## 1. BECERİ LİSTESİ (25)

Sayı hedefi yok, **fiil hedefi** var: *bir istasyon = bir beceri · bir toplama fiili = bir beceri ·
bir silah tutuşu = bir beceri.*

### Değişenler

| Kod | Eski ad | **Yeni ad** | Gerekçe |
|---|---|---|---|
| `SKILL_ENCHANTING` | Büyüleme | **Damgacılık** | Mühür büyük, **damga** onun kılıca sığanı. "Damga basmak" hazır Türkçe fiil |
| `SKILL_ECHO` | Yankı *(v1 terimi)* | **Ejder Sesi** | Ağacın bedeli zaten CAN; Kabukçular'ın "onun ağzından içelim"i birebir aynı mekanik |
| `SKILL_LIGHT` | Nur *(v1 terimi)* | **Kutsama** | Kutsama üç meşruiyet nesnesinden biri (Yedisunak) |
| `SKILL_SPEECH` | Konuşma | **Pazarlık** | Tuzkapı'da pazarlık **yazılı** yapılır; Gökçe'nin işi şart yazmak |
| `SKILL_ASSASSIN` | Suikast | **Kısık** | Örgütün rütbe adı zaten "Kısık" |

### Ağaç metni düzeltmesi (silme yok)

- **`TWO_HANDED`**: kök "Mızrak Eğitimi" → **"Uzun Silah Eğitimi"**; 4 düğümde "mızrak" nötrleşir
  ([skilltree.c:53-77](src/skilltree.c#L53-L77)). Kargı = ordunun talimi, iki elli kılıç = Sarp'ın
  hattı; **aynı ağaç, iki tutuş.**
- **`MEDIUM_ARMOR`**: item'ı yok diye ölü duruyordu → **Ak Nöbet kar zırhı** kimliği alıyor
  (yün + deri + damar pulu; soğuk direnci, karda sessizlik). Eksik item Karduvar'dan gelir.
- **`LIGHT`/Kutsama** ağacında "Kıyamet" düğümü LORE §1.9'u ihlal ediyor → **"Tan Vakti"**
  ([skilltree.c:529](src/skilltree.c#L529)).

### Yeni ikisi

**`SKILL_FISHING` — Balıkçılık.** Kanon bağı sert: Tuğrul'u öldüren delik **balıkçı deliğiydi**
(LORE §8). Oyunun en huzurlu fiili, oyunun ilk cesedini yapan fiil.

**`SKILL_COOKING` — Aşçılık.** Kodda gerçek bir boşluk: `STATION_CAMPFIRE` var ama becerisi yok,
XP'si Simya'ya düşüyor ([craft.c:222](src/craft.c#L222)) — sahipsiz istasyon.

### Altı aile

| Aile | Beceriler | Örgütler |
|---|---|---|
| **KILIÇ** (7) | Tek El, Çift El, Okçuluk, Kalkan, 3 Zırh | Kılıçartıkları, Ak Nöbet |
| **GÖLGE** (4) | Gizlilik, Kilit Açma, Yankesicilik, Kısık | Kısıklar, Kırk Kese |
| **OCAK** (4) | Demircilik, Simya, **Damgacılık**, **Aşçılık** | Karaocak, Mühürcüler |
| **TOPRAK** (4) | Madencilik, Bitkicilik, Oduncuk, **Balıkçılık** | Ak Nöbet, Karaocak, Kırk Kese |
| **SÖZ** (1) | Pazarlık | **altısı birden** — tek herkesin okuduğu beceri |
| **MÜHÜR** (5) | Yıkım, Çağırma, Asa, Kutsama, **Ejder Sesi** | Mühürcüler, Yedisunak, Kabukçular |

---

## 2. STRES EĞRİSİ — rahat oynanış neden işe yarıyor

**Gergin:** Kılıç + Gölge aileleri, Mühür'ün saldırı kolu, zindanlar, gece.
**Rahat:** Balıkçılık, Bitkicilik, Oduncuk, Aşçılık.
**İstisna — Madencilik rahat DEĞİL:** damar taşını kazdığın gece o civarın mezarlığı kalkar.
Oyunun en çok istediği kaynak, haritayı düşmanlaştıran fiile bağlı.

Rahat oynanışın kazancı dinlenme değil, **ışık ve gıda**:
odun → kandil ve ateş → gece ölü kalkmayan bir ada · balık + yemek → soğuk direnci ve gece görüşü
→ **damar kazısını mümkün kılan şey.**

> Döngü: gergin çık → yaralı ve malzemeli dön → ateş kur, pişir, dövdür, damga bas → daha derine
> çık. Oyuncu "mola veriyor" sanıyor; aslında bir sonraki gerginliğin ekipmanını üretiyor.

---

## 3. BALIKÇILIK — tam tasarım

**Mevcut `ResourceDef`'e oturmayacak, ve oturmamalı.** O sistemin sözleşmesi "hareket/hasar süreci
keser" — gerginliğin kaynağı tam olarak bu. Rahat bir fiil o kuralı miras almamalı.

Doğru bölünme: **düğüm ResourceDef gibi, fiil ayrı.** `FishingSpot` yerde durur, respawn'ı
`resource_daily_respawn` ile aynı gün dönümüne bağlanır, ilerlemesi work değil **durum makinesi**:
`BOŞ → ATILMIŞ → GERGİN → ÇEKİLDİ`. Üç timer, bir enum, ~50 satır.

**Oynanış.** E ile at. 4-14 saniye rastgele bekleme. Sonra **görsel değil sesli** işaret (ipin
gerilmesi + şamandıranın batışı). 1.2 saniyelik pencerede E. Kaçırırsan balık gider — **yem gitmez,
olta gitmez, ilerleme sıfırlanmaz.**

**Rahat olmasını sağlayan somut kararlar:**
- İşaret **sesli** → ekrana bakmak zorunda değilsin. "Idle ama uyanık."
- **Kesinti yok:** yürürsen olta yerde kalır, vurulursan süreç ölmez, panel açmak bozmaz.
- **QTE zinciri yok.** Tek tuş, tek pencere; ustalaştıkça kolaylaşır (seviye pencereyi 1.2s→2.0s açar).
- Ritim yavaş ve **düzensiz** → sayaç sayamıyorsun, bırakıyorsun.
- Balık yerinde **mob spawn'ı bastırılır** (~300px). Rahatlık tasarım garantisi olmalı, şans değil.

**Neye bağlı.** Yağmur → ısırma ×1.4 · sis → pencere +%50 · fırtına → liman zincirle kapalı, nehre
ya da vahaya gideceksin · kış → Karduvar gölü donar, **buz deliği** balıkçılığı (deliği kazmayla
açarsın, MINING de gelişir) · gece/gündüz farklı türler · Akkemer su yollarında **yalnız geceleri**
tuhaf şeyler çıkar.

**Ne veriyor.** ① yemek (en ucuz sürdürülebilir buff) ② tuzlanmış balık = Tuzkapı'nın gerçek
ihracatı, kervan dolgusu ③ balık yağı = iksir çözücüsü **ve kandil yakıtı** ④ dipten Akkemer
artıkları: kırık mühür harcı, dinleyici künyesi, ve **damar taşı KIRIĞI** — suya düşmüş, damardan
koparılmamış, yani **mezarlığı kaldırmayan** damar taşı.

**Oyuncu neden ikinci kez balığa gider — üç sebep, hiçbiri koleksiyon değil:**
1. **Yanına biri oturuyor.** Yoldaşlar ve bazı NPC'ler balık tutarken çöküp **sorulmadan** konuşur.
   Deli Sabri'nin Sarp'ın gerçek işaretlerini anlattığı yer burası — oyunun en önemli sırrı, en
   sakin fiilin içinde, diyalog ağacı olmadan. Oyuncu balığa değil, **konuşmaya** gidiyor.
2. **Su gündelik değişiyor** (ekoloji sistemi zaten popülasyon/göç tutuyor). Kaçırmak ceza değil.
3. **Rüstem'in buzu.** Karduvar buz deliğinden çıkan buz mahzendeki tekneye gidiyor: **bir çocuğun
   cesedini eritmemek için balık tutuyorsun.** Ve o çocuğu öldüren şey balıkçıların gece açtığı
   deliklerdi. Kimse bunu söylemez.

**Ahlaki iğne:** açtığın deliği kapatmazsan sabah biri o deliğe basıyor.

---

## 4. DİĞER RAHAT DÖNGÜLER

**Yemek pişirme.** `STATION_CAMPFIRE` zaten **kapısız** — izin yok, itibar kapısı yok; rahat
olmasının sebebi bu. Süre bazlı, vuruş yok. Verdiği: tokluk buff'ları, çiğ etin `rawRisk`ini
temizleme, satılabilir istif. Simya'ya bağlamak yanlış: simya sayısal ve pahalı, aşçılık ucuz ve
sürekli.

**Kandil bakımı** — beceri YOK, kasıtlı. Bakım işine XP çubuğu takarsan iş haline gelir.
Yol kandillerine balık yağı doldurmak = o hattın gece güvenli kalması.
**Bağlayıcı kural: sönmüş kandil ceza değil, yanan kandil bonus.** Hiçbir kandil kendiliğinden
sönüp oyuncuyu cezalandırmaz; yalnız fırtına ve kar söndürür (hava olayı olarak).

**Tarla/bostan.** Yıl 40 gün, mevsim 10 gün → bir ürün tam bir mevsim. Ekersin, gidersin,
döndüğünde olmuştur. **Beklemek zaten oynanış**; sulamaya koşmak yok, kuraklıkta verim düşer ama
ürün ölmez. Beceri: Bitkicilik.

**Kapan hattı** (arıcılık yerine). Ekolojinin yuva/popülasyon verisine bağlı; kurar gidersin,
sabah bakarsın. Rahat çünkü **eşzamansız** — emek ile ödül aynı dakikada değil.
*Arıcılık kesildi: Bitkicilik'in ikizi.*

---

## 5. TOPLAMA — Akkemer'in beş katmanı dikey gradyan

Zindana inmek bir **toplama gradyanı**; her katmanın kendi kaynağı, aracı, becerisi var:

| Katman | Kaynak | Araç | Fiil | Beceri | Ne işe yarar |
|---|---|---|---|---|---|
| Harabe-sunak | Dikilitaş yosunu, damga kırıntısı | Orak / çekiç | Topla / **Kır** | Bitkicilik / Madencilik | Damga özü |
| Bekçi karakolu | Paslı zincir demiri, dinleyici künyesi | Levye | **Sök** | Madencilik | Ucuz demir; künye = Mühürcü itibarı |
| Lağım / su yolu | Küf, engerek zehiri, yeraltı balığı | Elle / olta | Topla / **Tut** | Bitkicilik / Balıkçılık | Zehir, kaplama |
| **Damar** | **DAMAR TAŞI** | Kazma | Kaz | Madencilik (minSkill kapılı) | Her şey |
| Mühür | Kemer kireci, mühür harcı | Kazma | Kaz | Madencilik | **Mühür tamiri** — parayla alınmaz |

Yer üstü şehir kimliğine kilitli: Karaocak demir+kömür+kükürt · Karduvar buz+kar altı otu+trol yağı ·
Kumkapı tuz kabuğu+kamış · Tuzkapı deniz tuzu+kabuk · Yedisunak bal mumu+sunak otu · Yenitaç iskele
odunu+çayır otu. **Hepsi mevcut `ResourceDef` alanlarıyla ifade edilebiliyor** — tek eklenen üç
`verb` metni ("Sök", "Kır", "Tut").

---

## 6. ÜRETİM — dört istasyon yeter, tabela değişiyor

| İstasyon | Girdi → Çıktı | Öğreten |
|---|---|---|
| **SMITH** | külçe + kömür → silah/zırh/kazma/olta/kapan | Karaocak; pratik kademe Kılıçartıkları |
| **ALCH** | ot + zehir + balık yağı → iksir, kaplama, salamura | Yedisunak (alt), **Kısıklar (üst)** — aynı bilgi, iki ahlak |
| **ENCHANT** (Kadim Sunak) | damar taşı + harf → damga, mühür harcı | Mühürcüler; rakip tarif **Kabukçular** |
| **CAMPFIRE** | et/balık/tahıl → yemek + buff | Kimse. **Kapısız kalsın.** |

`RecipeDef.blueprintItem` alanı altın: **örgüt kapısı yeni sistem değil, blueprint dağıtımı.**
Üst tarifler blueprint ister; blueprint üç yoldan gelir (örgüt ödülü, pazar, loot) — yani hiçbir
örgüte girmek zorunlu değil ama bir yol seçmek zorunlu (LORE §15.2 korunur).

**Kendi kendine yetme:** oyuncu her şeyi yapabilir, ama **%70 seviyesinde.** Son %30'un üç kapısı,
hiçbiri seviye duvarı değil: blueprint · istasyon kalitesi (Karaocak ocağı daha iyi; kendi kampında
da dövebilir, sadece daha kötü) · coğrafya (Karduvar buzu Kumkapı'da yok). **Eksik olan bilgi ya da
izin değil, mesafe ve zaman** — ve her eksiği para çözüyor.

---

## 7. DAMGA — silaha basma sistemi

### Kanon gerekçesi
Akkemer'in mührü bir büyü değil, **bir emirdir**: taşa yazılmış tek kelime — *dur*. Kemer sadece
duvarcılık; asıl mühür altındaki **damar taşı örgüsü**. Kanıt örgütün rütbelerinde: Dinleyici →
**Okuyan** → **Mühür Eli**. Mühürcülük bir **okuma** mesleğidir; okunacak bir şey var demek ki.

Bir ejderhayı dokuz yüz yıl tutmak için binlerce ton taş gerekiyor. Bir kılıca sığacak yazı ancak
küçük bir şeyi, ancak bir an tutabilir — **ama kılıcın istediği tam olarak bu.** Kimse kılıçtan
dokuz yüz yıl beklemiyor; adamı yarım saniye tutması yetiyor.

### Adlandırma
- Büyük yazı **mühür**, küçüğü **damga**. Meslek **damgacılık**.
- **"Rün" ve "öz" gidiyor** (v1 terimleri, LORE §15.5 ihlali). Hammadde artık **harf**: damar
  taşından çıkan, üstünde tek okunur iz taşıyan kırık.
- Eşya yeniden adlandırma (silme yok): Soluk Öz → **Sönük Harf** · Zehirli Öz → **Ağılı Harf**.
- İstasyon **Kadim Sunak** kalıyor — LORE §12'nin birinci katmanı, kanon onu çoktan yerine koymuş.

### Beş VFX'in kanondaki karşılığı
Damga hep aynı şeyi yapıyor: **tutuyor.** Değişen, taşın koparıldığı anda neyi tutmakta olduğu.

| VFX | Damga | Nereden | Kim satıyor |
|---|---|---|---|
| `FLAME` | **Ocak Damgası** | Volkan gayzer çanağı, Karaocak dipleri — o taş hiç soğumadı, altındaki uyanık | Karaocak (pahalı, faturalı) · Kabukçu **kavları** (ucuz, kayıtsız — "kav" = çıra) |
| `ICE` | **Buz Damgası** | Karduvar, donmuş göl altındaki mühür. **"Dur" emrinin en dürüst görüntüsü donmaktır** | Mühürcüler + Ak Nöbet — geçitten geçmen gerek, Ak Nöbet Kabukçuyu geçirmez → fiilen taraf seçimi |
| `POISON` | **Ağı Damgası** | Kanon zaten yazmış: damar taşı "ezilip zehir olur" → **harf gerektirmiyor**, toz yeter. Okulu yok, tekeli yok | Her otçu, Tuzkapı lağımı, Kumkapı. Sistemin giriş kapısı |
| `VENOM` | **Kısıklar damgası** | Satılmıyor, suikast loncasından. Vuruşta yere ağı birikintisi | Kısıklar. **Ayırt edici: ışıksız** |
| `PINK` | **Diri Damga** | Sarkaç bir mühür çürüttüğünde çatlağın ağzındaki taş **canlı** ışıyor. Yalnız taze çatlaktan kesilir | Craft yok — **olaya bağlı ve sonlu** |

**Diri damga en zarif çözüm:** kodda en güçlü olan tek damga (+%8, büyüye de işliyor) yedi mühür
olayına bağlanıyor. Her olay bir pencere: o dönemde diri taş var, sonra yok. Pembe ışığın gerekçesi
de bu — çatlak neyle parlıyorsa o parlıyor.

### Oynanış (kod bunu doğru ayırmış, sadece adı değişiyor)
- **Harf dövmek — ŞANSLI.** `60 + Damgacılık×0.4`, tavan 95. Tutmazsa taş çatlar. Mesaj
  *"Rün tutmadı! Özler yandı"* → **"Harf çatladı."** Başarısızlık doğru yerde: yazıyı **okumak**
  zor, basmak değil.
- **Damga basmak — GARANTİLİ.** Kuşanılı eşya sunağa konur, 3 saniyelik tören, **asla başarısız
  olmaz.** Bu korunacak: oyuncunun elindeki eşyayı kumar masasına koymak LORE §15.7'ye aykırı.
- **Seviye ne değiştirir:** harf dövme şansı · damgayı kırmadan **sökme** şansı · yüksek seviyede
  **ikinci yuva**. Üçüncü asla — üç harfi bir bıçakta tutmak için kemer gerekir, kılıç kırılır
  (yapay duvar değil, kurgusal tavan).
- **+plus ile ilişki:** plus demircinin işi (çeliğin gövdesi), damga yazının işi (ne dediği). Ayrı
  sistemler, tek yerde konuşuyorlar: **damganın etkisi plus ile ölçekleniyor.**
- **Yağ vs damga:** mevcut `coatFx` (Zehir Yağı, 60 sn) = ezilmiş damar taşı sürmek; sunak ve harf
  gerekmiyor, geçici. Merdiven kendiliğinden: **yağ giriş, damga taahhüt.**

### Bedel — Sarkaç sorusuna dürüst cevap
**Sıradan üç damga Sarkaç'a DOKUNMAZ.** Bunu ciddi düşündüm ve reddediyorum: her damga basanı
dünyayı bozmakla suçlamak LORE §1.7'nin ağırlığını harcar ve §15.1'in yasakladığı fatura
refleksidir. Bedel zaten dünyada var ve daha iyisi: **damar taşı kazdığın gece mezarlık kalkıyor.**
Yerel, anında, senin başına gelen, atlatılabilir. Damga basan adam gece nöbet tutar. Bu yeter.

**Sarkaç'a dokunan tek şey diri damga — ve basmak değil, HASAT.** Kabukçu yöntemi çatlağı
genişletmek: yedinci mührü sekiz yapmıyor (sayı sabit) ama **takvimi öne çekiyor.** Mühürcüler
tarihi söylüyor, tarihin kaydığını görüyorsun. Cazip, ölçülebilir, geri alınamaz, ve kimse seni
uyarmıyor.

### İki örgüt, iki yol — güvenli/güçlü ayrımı DEĞİL
**Mühürcüler yazıyor.** Okunabilir damga → **sökülebilir, taşınabilir, tamir edilebilir.** Ama
sökülebilen damga verilmiş değil ödünç verilmiştir: etkisi daha düşük. Karşılığında yasal.

**Kabukçular kırıyor.** Harf dövmezler, taşı çatlatıp içindekini bıçağa salarlar: daha güçlü,
anında, **beceri gerektirmiyor.** Karşılığı: sökülemez, çeliği yiyor (her açılışta plus kaybı;
körelir, kırılmaz), gece parlıyor ve **parladığı şeyi çağırıyor** — ölüler bir Kabukçu damgasını
fark ediyor (mevcut ışık sistemi bunu bedava veriyor).

**Ve Kabukçu'nun haklı olduğu yer** (LORE §16.7'ye aday cevap): Mühürcü damgası soluyor ve
yenilenmesi için loncaya **sürekli ödeme** yapman gerekiyor — kira. Kabukçu'nunki bir kere alınır,
senindir, bedava. **Adam yalan söylemiyor.**

### Zırha ve alete basma
Silahta emir dışa, zırhta **içe** dönük — seni tutuyor.
Zırh: Buz = sersemleme/savrulma direnci · Ocak = don direnci (Karduvar) · Ağı = zehir direnci.
**Kandil: en iyisi.** Damgalı kandil sönmüyor — savaşla ilgisi olmayan, kanonun ortasından çıkan
güçlü kullanım. Kazma: Ocak damgalı kazma damar taşını hızlı çıkarır → **daha çok mezarlık gecesi**
(kendi cezasını hızlandıran alet). Olta: Buz damgalı olta donmuş gölde buzu geçer — o gölün ne
olduğunu bilen oyuncu için sessiz bir şey söylüyor.

---

## 8. HAT SİSTEMİ — 13 açık + 2 gizli

### Formül: EL + FİİL + TANIK
"3-4 beceri kesişimi + dünya eylemi" eksikti: kesişim **geriye dönük ve sessiz** (sahne yok),
dünya eylemi ise kimliği **kimseye duyurmuyor.** Oysa kural şu: kimlik davranıştan doğar,
**ad başkasından gelir.** Kanon bunu zaten söylüyor — Kırca Ana unvanını hiç sormaz; §11'in birinci
kademesi "nöbetçinin selamı", yani başkasının davranışı.

- **EL** — esnek kesişim: "şu 5 becerinin 3'ünde 25+". Sabit üçlü değil, havuzdan seçim → iki farklı
  build aynı hattı farklı elle kazanır. Eşik **düşük** (25-40, 70 değil); yoksa seviye duvarı olur.
- **FİİL** — geri alınamaz, tek seferlik, **duvarda görünür** kaydı olan bir iş.
- **TANIK** — seni adlandıran biri. Çoğunlukla örgüt rütbesi, **ama şart değil**: bir köy ihtiyarı,
  bir veraset taşı, bir defter satırı, hatta okuma yazma bilmeyen Deli Sabri.

**Rütbeyi zorunlu yapmamanın sebebi:** zorunlu olsa altı örgüte üyelik zorunlu olur ve LORE §15.2
(rol özgürlüğü) ölür — münzevi oyuncu hiçbir hat kazanamaz. Tanık ayrıca her hatta **bir sahne**
veriyor: biri adı yüksek sesle söylüyor ve o andan sonra NPC'lerin senin hakkındaki kelimesi
değişiyor. Ekranda görünen şey kapıcının yüz ifadesi.

### Neden 13 açık
22 fazla — 22 ayrı fiil, sahne ve kalıcı dünya etkisi gerekir, yoksa yarısı "%5 hasar" olur ve unvan
sisteminin kopyası çıkar. 6 az — hat başına bir örgüt, hattı üyeliğe eşitler.
Mimari: **her örgüte iki hat** (dışa dönük yüz + iç yüz) = 12, **artı bir örgütsüz hat** — sistemin
örgüt gerektirmediğinin kanıtı.

| Hat | El (havuzdan 3) | Fiil (tek seferlik) | Tanık | Verdiği |
|---|---|---|---|---|
| **Siper Tutan** | Kalkan, Ağır Zırh, Tek/Çift El | Yol kesen aç bölüğü geçitte tek başına durdur; köy sağ kalır | Kılıçartıkları, Onbaşı | "Nöbet" duruşu: dostlar kaçmaz, hasarı sen alırsın; gardiyanlar yanında dövüşür |
| **Bölük Ağası** | Pazarlık, Orta/Ağır Zırh, Tek El | Aç bölüğe kalıcı iş bul, Rüstem'in maaş defterine yazdır | Kılıçartıkları, Bayraktar | İki gazi yoldaş, kışla, panoda "bölük kontratı" katmanı |
| **Yol Sahibi** | Pazarlık + iki üretim/toplama | Kapanmış yolu temizle, kervanını geçir; yol haritada kalıcı açılır | Kırk Kese, Kese Sahibi | Kervan, şehirler arası fiyat farkı, satış tavanı kalkar |
| **Lağımcı** | Kilit Açma, Gizlilik, Yankesicilik/Pazarlık | Liman zincirinin altından yük geçir (yazılı pazarlık) | Kırk Kese, Yazman | Kalıcı lağım ağı, yasak mal alıcıları |
| **Kemer Okuyan** | Damgacılık, Madencilik, Asa/Yıkım | Çürümekte olan bir mührü **geciktir** (Sarkaç'ta çentik) | Mühürcüler, Mühür Eli | Mühür odası kapıları, haritada Sarkaç sayımı, damar verimi |
| **Kandil Tutan** | Kutsama, Simya, Pazarlık | Bir kasabanın kandil hattını bir gece boyu ayakta tut | Mühürcüler/tapınak, Okuyan | Alan şifası; o kasaba geceleri kalıcı güvenli; **siviller seni hatırlar** |
| **Kısık Eli** ⚠ | Gizlilik, Kısık, Hafif Zırh | Bir kontratı tanıksız bitir; o makam el değiştirir | Kısıklar, Kısık | Gizli öldürme çarpanı, panonun ikinci yüzü |
| **Zehirci** | Simya, Bitkicilik, Pazarlık/Gizlilik | Şölen tepsisini zehirle **ya da** hazır zehri panzehirle çürüt | Kısıklar veya Hekim Yusuf | Zehirleme fiili, gecikmeli ölüm (mazeret), iksir tavanı |
| **İz Süren** | Okçuluk, Gizlilik, Bitkicilik | Trolün/küçük ejderhanın izini yuvasına sür, yuvayı haritaya işle | Ak Nöbet, Kar Nöbeti | Kalıcı iz katmanı, av çift ürün, yuva erişimi |
| **Buz Balıkçısı** | **Balıkçılık**, Oduncuk, Simya | Gölün gece açılmış deliklerini işaretle — bir daha kimse düşmez | Ak Nöbet, Gedik Beyi | Buz/liman yolları, kışlık ambar; **ve Gedik Beyi'nin sakladığı bilgi (§8)** |
| **Damar Ustası** | Madencilik, Demircilik, Damgacılık | Bir damarı sonuna kadar kaz ve o geceyi mezarlıkla göğüsle | Kabukçular, Çatlak | Damar taşı işleme, en iyi craft, Karaocak derinleri |
| **Ejder Bağlayan** | Çağırma, Yıkım/Asa, Bitkicilik | Bir yumurta çal ve kışı onunla geçir | Kabukçular, Kabuk Kıran | **Binek/yoldaş**, kapılarda yüz ifadesi |
| **Ocak Sahibi** | Bitkicilik, Oduncuk, **Aşçılık**/Demircilik | Bir köyün ambarını bir kışa yetecek kadar doldur; köy büyür | **Örgüt yok** — köy ihtiyarı | Berat merdiveni hızlanır, ev/tarla, yemek pasifleri, yeni dükkânlar |

⚠ **"Sessiz El" adı kullanılmadı** — LORE §10'da Kısıklar'ın tepe rütbesi "Sessiz Bey"; çakışmayı
önlemek için hat adı **Kısık Eli**. (Ayrıca "Sessiz El" v1 kanonunun örgüt adıydı.)

### İki gizli sınıf
Gizliliği "bulunmayan görev" değil, **söylenmeyen kelime** sağlıyor.

**DİNLEYİCİ.** Kapı: Mühürcü rütbesi (Okuyan+) **artı** bir Kabukçu eylemi — kabuğu kırıp taşın
altındakini duymak. **Neden kimse söylemiyor:** "Dinleyici" Mühürcüler'in **en alt rütbesinin adı.**
Sorduğunda "he, Dinleyici, çırak demek" cevabını alırsın — kimse yalan söylemiyor, kelime kamufle
ediyor. Verdiği: Ejder Sesi fısıltıları (bedeli CAN — kodda hazır, `skill_set_echo_visible`), mühür
odasını kazmadan okumak, bir mührü bir kez erteleme. Bedeli: kullandıkça gece ölüleri **seni**
hedefler.
*Yan fayda: dinleyiciler hâlâ bağlı, hâlâ acı çekiyor — LORE §16.7'nin (Kabukçular neden haklı)
ikinci cevabı burada.*

**İŞARET OKUYAN.** LORE'un ikinci adayı (Kısıklar + Ak Nöbet yemini) **reddedildi**: sadece bir
savaş melezi ve gizliliği yapay bir yemine dayanıyor. Yerine: **Sarp'ın odasındaki tepsi bulmacasını
altı ağzın hiçbirine sormadan çöz**; tanık Deli Sabri. **Neden kimse söylemiyor:** kanon zaten
söylüyor — gerçek işaretleri bilen tek adam o ve **kimse ona sormuyor**; üstelik okuma yazması yok,
yani bunun bir sınıf olduğunu kendisi de bilmiyor. Verdiği: dövüş değil, **konuşma katmanı** —
yalan söyleyen NPC'nin "gözü kapanır" (Sarp'ın reddetme kanalının oyuncuya taşınmışı), Sarp adına
konuşabilirsin, kurulda ikinci ses.

İkisi çakışmıyor: biri savaş/mühür tarafı, öbürü siyaset tarafı.
*Reddedilen aday rafta kalıyor — normal bir çapraz hat olarak kullanılabilir (LORE §15.8).*

### Hat kapanması YOK
Bir hat kazanmak başka hattı kapatmıyor. Kapanan şey hat değil **sahne**: dünya eylemleri geri
alınamaz ve tek seferlik. Damarı tükettiysen o damar yok; mührü tamir ettiysen onu kıramazsın
(başkasını kırabilirsin); şölen tepsisi bir kez servis edilir. **Derinlik kısıtlayıcı kuraldan
değil, fiilin tükenmesinden geliyor** — kapıyı tasarımcı değil oyuncunun kendi eli kapatıyor.

İkinci fren yasak değil **tavan**: düşman örgütte rütbe tepesi kapalı kalır (Kısıklar'da Boğaz olan
Kılıçartıkları'nda Söz Sahibi olamaz) — ama girebilir, çalışabilir, ve tavan telafi göreviyle
kalkabilir. Sosyal sonuç, seviye duvarı değil.

Üçüncüsü zaten kodda: **taşınan tek kimlik** — on üçü de kazanılabilir, biri adının yanında durur
ve etkisi ×2.

### Unvan vs Hat
| | Unvan (`title.c`, hazır) | Hat |
|---|---|---|
| Nasıl gelir | Gizli sayaçtan **kendiliğinden** | Eşik + fiil + tanık |
| Ne kadar | Çok | 13 + 2 |
| Ne verir | Pasif yüzde | **Fiil / kapı** |
| Sahne | Yok | Var |
| Cümle | "Ne kadar yaptın" | "Kim oldun" |

Birleşme: her hattın önkoşulunda **bir unvan** olsun (elin alıştığının kanıtı). Hat =
(unvan açık) + (beceri eşikleri) + (quest bayrağı) — üçü de zaten kodda var. Hat kazanılınca
**taşınabilir unvan listesine girer**: aynı UI, aynı ×2.

---

## 9. DÜRÜST UYARILAR (tasarımcıların kendi tespitleri)

### Denge
- **Damgacılık oyunun en güçlü becerisi** — her savaş becerisini çarpıyor. Harf dövmenin şanslı
  olması iyi bir fren ama yetmez; damga başına damar taşı maliyeti **örgüt rütbesine** bağlanmalı.
- **Asa, Yıkım'ı yiyor.** İkisi de büyü hasarı; Asa'nın kendi fiili "asayla vurmak"tan öteye
  geçmiyor. Birleştirme adayı bir numara.
- **Orta Zırh hâlâ en zayıf halka.** Ak Nöbet kar zırhı item'ı yazılmazsa üç zırh becerisi iki buçuk
  kalır → Ağır Zırh'a katılmalı.
- **Kutsama gece dengesini tek başına çözebilir** (ölülere ×1.8). Çarpan tavanı düşmeli ya da maliyet
  mana yerine **tükenir bir kaynağa** (kutsanmış yağ) bağlanmalı.
- **Yankesicilik + Pazarlık aynı keseye çalışıyor** — biri fiyatı indiriyor, öteki parayı bedavaya
  alıyor. Damar taşının altı kat fiyatı bu deliği büyütür.
- **Diri damga yığılması:** bir eşyada en fazla bir diri damga, ömür boyu.
- **Lağımcı hattı tek başına üç sistem açıyor** (hızlı geçiş + gardiyan atlatma + yasak alıcı).
  Birini kes — tercihen hızlı geçişi.
- **Buz Balıkçısı hat olarak zayıf** (fiili "işaretlemek"). Bilgi kapısını korumak istiyorsan hat
  yap, yoksa unvana indir.
- **Kısık Eli / Zehirci aynı şey olma riski** — ayrımı gecikmeli ölüm + mazeret sistemi taşıyor;
  onu yapamayacaksan Zehirci'yi kes.
- **Boşluk:** Çift El ve Kalkan'ın ikinci evi yok; ağır savaş oyuncusuna tek hat düşüyor.

### Grind
- **Risk damar taşında, balıkta değil.** Altı kat fiyat + günlük respawn = "sabah kaz akşam sat"
  makinesi. Gerçek fren fiyat değil, **aynı bölgede tekrarlı kazının mezarlığı tırmandırması**;
  yoksa mezarlık gecesi de farm'a döner (ölü loot'u).
- **Balıkçılığın nadir çıktısı:** suda yalnız damar taşı **kırığı** çıkacak. Tam damar taşı çıkarsa
  oyunun merkez tansiyonu (kaz → mezarlık) bedava atlatılır. Günlük tavan + yalnız-kırık kuralı
  bağlayıcı.
- **Balıkçılık hiçbir zincirin zorunlu düğümü olmamalı.** Bir görev "git balık tut" derse tasarım
  ölür — Sabri'nin konuşması dahil, o konuşma başka yolla da alınabilir olmalı.

### Kod borcu (dikkat)
- ⚠ **`WEAPON_FX_VENOM` şu an `light.c`'de `default` dalına düşüp PEMBE ışık yayıyor.** Gizlenme
  damgası için bu bir hata — Kısıklar damgası ışıksız olmalı.
- ⚠ Damga eşyaları [main.c:4984](src/main.c#L4984) ve [main.c:7833](src/main.c#L7833)'te **iki
  taneye sabitlenmiş**, panel satırları `CRAFT_ROW_RUNE0/RUNE1`. Alev ve buz eklemek yeni satır +
  yeni eşya demek — hafızadaki **paralel dizi tavanları** dersi geçerli: `ITEM_TYPES`, panel
  yerleşimi ve **`save.c` tamponlarını birlikte** gözden geçir.
- `SKILL_FISHING` + `SKILL_COOKING` skill.h sonuna eklenecek (save bloğu sayı-toleranslı).
- `TITLE_COUNT` ve `TitleStat` havuzu büyüyecek; `carriedTitle` aralık kontrolü ve save tamponları
  aynı commit'te.

---

## 10. MVP KESİM LİSTESİ

**Kalır:** 25 beceri (3 ad değişikliği + 2 ağaç metni düzeltmesi) · balıkçılık durum makinesi ·
aşçılık · üç damga (Ocak/Buz/Ağı), eşya başına tek yuva, yalnız silah · dövme şanslı + basma
garantili (zaten kodda) · **4 açık hat + 1 gizli**: Siper Tutan (savaş — ilk saatteki buz
kontratına bağlanır), Lağımcı (para), İz Süren (av), Kemer Okuyan (mühür) + **Dinleyici**.
Dört sütun, dört ayrı şehir, dördü de bugün kodda var olan sistemlerden besleniyor.

**Kesilir:** zırh/alet damgası · damga sökme · ikinci yuva · Kabukçu aşınması · Sarkaç bağı ·
VENOM'un oyuncuya açılması · diri damga craft'ı (yalnız düşman düşürmesi olarak kalır) ·
kandil bakımı kirası · arıcılık (tamamen).
