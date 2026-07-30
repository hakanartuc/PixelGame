# SUÇ · KELLE · İTİBAR · MUHAFIZ — v2 (Demirboyun)

> 2026-07-30 · Kanon: `docs/LORE.md` v2 · Beceri tasarımı: `beceri-meslek-sinif-v2.md`
> §1-9 paralel tasarımcı çıktısı · §10-13 orkestratör (ikinci ajan harcama limitinde kesildi)

**Tek ilke: kelle parası bir ceza değil, bir BORÇ SENEDİ.** Borç devredilebilir, satılabilir, işle
ödenebilir, affedilebilir, taşınabilir. Oyuncu "yakalandı" değil, **bir alacaklı buldu** — ve altı
şehirde altı farklı alacaklı var.

**Düzeltme:** Bounty'nin şehir başına olması *önerilmedi, zaten kodda* —
`player.bounty[MAX_REGIONS]` + `player.reputation[MAX_REGIONS]` ([player.h:142](src/player.h#L142)).
İş bunu altı şehre ve altı örgüte bağlamak.

---

## 1. ALTI ŞEHİR, ALTI HUKUK

Yakalama paneli bugün üç düğme veriyor: **öde / karşı koy / rüşvet**. Tasarımın merkezi tek ekleme:
**şehir başına değişen dördüncü düğme.** Uygulaması bir enum + altı diyalog dalı; tadı bütün sistemi
taşıyor.

| Şehir | Suç neyle ölçülür | Dördüncü düğme | Sıyrılma yolu |
|---|---|---|---|
| **Yenitaç** | Akçe (mevcut sabitler) | **"Deftere yaz"** — kelle parası kontrat borcuna çevrilir, pano işlerinin ücreti bounty'den düşer | Çalışmak. *Hapsin yerine geçen şey bu* |
| **Tuzkapı** | Akçe, ama kayıtlı | **"Senet imzala"** — bounty anında 0, karşılığı vadeli borç + faiz | Ödemek. Ödemezsen **liman kapanır** ve sonradan ödemek onu AÇMAZ |
| **Yedisunak** | **Kirlilik** — "öde" düğmesi GRİ, akçe geçmez | **"Kefaret"** — oruç/hac/tapınak işi zinciri | Kan suçunda ağır; hırsızlıkta tapınak hiç ilgilenmez |
| **Karaocak** | Kendi sikkesi — krallık akçesiyle ödersen **×1.5** | **"Ocakta çalış"** — bounty cevher teslimiyle kapanır | Madende oynamak (Madencilik gelişir, damar taşı görürsün) |
| **Karduvar** | **Kelle parası HİÇ YOK** — muhafız kovalamaz, **kapı kapanır** | **Mahkeme:** kan bedeli (mal) / sürgün (geçit bir mevsim) / **kefil** | Parayla çıkış yok |
| **Kumkapı** | Vali defter tutar, **aşiret hatırlar** | **"Kan bedeli"** (deve, su, kuyu payı) ya da bir kervanı kurtarmak | Valiye ödersen bounty siliner, aşiret rep'i düşük kalır → kervansız çöl öldürür |

**Tuzkapı'nın özel kuralı** kanondan doğrudan çıkıyor (*"kanun satılık ama pazarlığı yazılı yapılır;
sözünden dönene liman kapanır ve bu ölümden beter sayılır"*): burada rüşvet ayıp değil —
`CRIME_BRIBE_SPEECH` eşiği düşer ve `REP_BRIBE (−5)` **uygulanmaz**, çünkü kayıtlı pazarlık meşru.
Ayıp olan **kayıtsız** rüşvettir (sözlü teklif → rep −10). Kodun rep/bounty ayrımının en güzel
kullanımı: **borç silinir, söz silinmez.**

**Karduvar Tuzkapı'nın tam aynası** — iki şehir birbirini açıklıyor. **Kefil** mekaniği sistemin en
özgün parçası: bir NPC seni kefil alır, ikinci suçta *o* cezalanır. İtibarını başkasının itibarına
bağlıyorsun.

---

## 2. BÖLGE SAYISI — ekseni ayır, tavanı büyütme

Kod bugün **coğrafi bölgeyi** (splash, müzik, mob kampı) ve **yargı çevresini** (bounty, itibar)
aynı diziye bağlamış. Altı şehir + kırsal + uçlar + zindan paletleriyle coğrafi bölge 20'yi aşacak;
`MAX_REGIONS 8` bounty için fazlasıyla yeter ama coğrafya için hemen patlar.

**Çözüm:** `Region`'a `Sint8 lawId` ekle; `bounty[]`/`reputation[]` boyutu **`MAX_LAWS 8`** olsun
(6 şehir + kırsal + kanunsuz uçlar). Coğrafi bölge sayısı serbest kalır, **save tamponuna
dokunmadan.** Hafızadaki *paralel dizi tavanları* dersini baştan çözer.

**Kırsalda kanun iki katmanlı:**
1. En yakın şehrin devriyesi — kırsal suç o çevreye **yarım** yazılır (kanıt zayıf, haber yavaş).
2. **Aç gazi bölükleri** — *"vergi zaten toplanıyor, biz de topluyoruz."* Bunlar haydut değil,
   **gayri resmi gümrük**: panelden ödersin, itibar düşmez, para gider. Bölüğü kesersen o yol kalıcı
   güvenli olur ve Rüstem'in maaş defterindeki açık büyür — **yan etki, ceza değil.**

> ⚠ **KOD HATASI:** [main.c:67](src/main.c#L67) bölge indeksini `-1` iken **0'a düşürüyor** — yani
> ıssız arazide işlenen suç Köy'ün defterine yazılıyor. Yeni modelde `-1 → LAW_NONE`: kanunsuz yerde
> bounty yazılmaz, yalnız itibar dedikodusu işler.

> ✅ **SAVE UYUMU DOĞRULANDI:** [save.c:201](src/save.c#L201) sayıyı dosyaya yazıyor,
> [save.c:455](src/save.c#L455) yalnız `nRegion > MAX_REGIONS` ise reddediyor — tavanı büyütmek eski
> kayıtları okumaya devam eder (tersi değil).

---

## 3. ŞEHİRLER ARASI KAÇMAK — kopyalanan bounty değil, yürüyen kurye

Bounty bölgesel kalsın, **kopyalanmasın.** Yayılan şey **kayıt**: tanıklı suçtan sonra kaynak
şehirden hedef şehre bir **kurye NPC'si** yola çıkar. Haritada görünür, kervanla gider, 1-3 oyun günü
sürer (yıl 40 gün → 2 gün ciddi süre). Ulaşırsa hedef şehirde bounty'nin **~%40'ı** ve rep düşüşünün
yarısı yazılır: *"duyduk."*

**Oyuncu kararı net: kuryeyi kes.** Ama bedava değil — kesmek de bir suç ve üstündeki mühürlü çanta
kanıt olarak kalır; ikinci kurye mühürlü gider (öldürmenin bedeli ×2). Ve kesilen kayıt yok olmuyor,
**sahibi değişiyor**: Kırk Kese onu bilgi olarak alır ve senin üstüne yazar. **Kanundan kurtulup
örgüte borçlanıyorsun.** Sistemin tezi bu.

**Asimetri altı şehri ayırır ve öğrenilebilir bilgi olur:**
Yenitaç ↔ Karaocak hızlı yazışır · Tuzkapı haberi **satar**, bedava vermez · Yedisunak yalnız kan
suçunu taşır · Karduvar haber **alır, vermez** (mektup hattı Kırca Ana'nın) · Kumkapı defter tutmaz,
aşiret hatırlar.

---

## 4. ALTI ÖRGÜT, ALTI ÇIKIŞ YOLU

| Örgüt | Ne yapar | Rütbe | Bedel | Yan etki |
|---|---|---|---|---|
| **Kırk Kese** | Aklar: bounty → altın | Yazman | bounty ×2 akçe | Defterine geçersin; reddettiğin işte bounty geri döner |
| **Kısıklar** | Tanığı sildirir | Gölge | akçe + kan | Kurbanın yakınları rep düşürür; Kılıçartıkları tavanı kapanır |
| **Kılıçartıkları** | Kanunun tarafına alır, **kelle avcısı yetkisi** | Nöbetçi | zaman: 1 iş = 1 dilim borç | En yavaş, en temiz; Kısık geçmişin varsa kabul yok |
| **Mühürcüler / tapınak** | Kan suçunu affeder | Okuyan | **parayla alınmaz** — kutsanmış yağ, mühür harcı | rep +10 ama "kefaretli" damgası; gaziler soğur |
| **Ak Nöbet** | Sürgün eder (Karduvar mahkemesinin infazcısı) | Kar Nöbeti | geçit bir mevsim kapalı | Sürgündeyken Kabukçu işi almışsan bir daha geçmezsin |
| **Kabukçular** | **Hiç umursamaz** | — | — | Hiçbir şehirde temizlenmene yardım etmezler; kaydı olmayan adamsın |

Kabukçular'ın sıfır kapısı bilinçli: **en yüksek bounty'de bile seni içeri alan tek örgüt.** Bu,
LORE §16.7'nin ("Kabukçular nerede haklı") cevabına ikinci katkı — *adam senden kâğıt istemiyor.*

---

## 5. KELLE AVCILIĞI — Kısıklar'dan ayrımı tek kelime: KAYIT

Pano Yenitaç'ta (kanon zaten koymuş) ve **dünyadaki bounty kayıtlarından üretilir.** Üç hedef tipi:
- **Aç gazi bölüğünün reisi** — diri getirmek Rüstem'in defterine iş yazdırır
- **Kaçmış borçlu** (Tuzkapı senedini bozan) — **yalnız diri kabul**, çünkü ölüm borcu siler ve senet
  değersizleşir
- **Başka şehre kaçmış suçlu** — haber sistemi tersten: **kurye sensin**

Diri ×1.5 + itibar · ölü ×1.0 + itibar nötr ve yakınlarından rep düşüşü.

| | Kelle avcılığı | Kısıklar suikastı |
|---|---|---|
| Tanık | **İyidir** | **Ölümdür** |
| Kontrat | Panoda, adın defterde | Elden, kayıt yok |
| Hedef | Bounty'si olmak zorunda | Masum olabilir |
| Öldürme | Suç sayılmaz | Tanık varsa suçlu **sensin** |

Aynı fiil, ters işaretli tanık. Kodun `crime_witnessed` sözleşmesine birebir oturuyor; **yeni sistem
gerekmiyor.**

---

## 6. HAPİS — hayır, ve yerine üç şey

**Klasik hapis eklenmemeli.** Oyuncuyu odada bekletmek LORE §15.7'ye (yapay kapı yok) ve
derinlik+özgürlük ilkesine aykırı; **bekleme oynanış değil.** Kanondaki hücreler, zincirler, kafesler
zindan içeriğidir (Akkemer dinleyicilerini bağladıkları yer) — ceza mekaniği değil, orada kalmalı.

Yerine:
1. **Borç işi** — Yenitaç "deftere yaz" / Karaocak "ocakta öde". Hapsin doğru hâli: oyuncu bir odada
   beklemiyor, madende oynuyor.
2. **Zorla el koyma** — ödeyemiyorsan muhafız çuvalını alır: yasak mal + bounty kadar eşya. Acıtır,
   oyunu durdurmaz.
3. **Sürgün / kapı** — bir şehir kapanır, mevsimle sayılır, oyuncu başka yerde oynar.

**Tek istisna, ceza değil hikâye: ölüm yerine yakalanmak.** Muhafıza yenilirsen ölmek yerine
karakolda uyanırsın, eşyalar vezne sandığında, üç çıkış (kilit açma / pazarlık / kavga). Bu hapsi
**ödül** yapar.

---

## 7. İTİBAR vs KELLE — ve korkunun kendi merdiveni

**Bounty borçtur, silinir. İtibar defterdir, silinmez — yalnız unutulur** (`REP_DAILY +1`).

Derinleşmesi gereken yer **eksi taraf.** Bugün düşük itibar sadece ceza: kimse konuşmuyor, fiyat
kötü. Oysa korkulan adam olmak da bir güç.

**Korku kapıları (rep ≤ −40):** haydut kampları saldırmaz · yol vergisi istenmez · esnaftan **haraç**
diyalogları açılır · Kısıklar/Kabukçular rütbe hızı artar · muhafız tek başına yaklaşmaz,
**takviye bekler.**
**Kaybettikleri:** görev, şifa, kefalet, **Makam.**

Yani düşük itibar bir kapı değil, **bir sınıf: korkulan adam.**

> **En zayıf konum orta itibar.** Ne sevilen ne korkulan adam hiçbir kapıyı açmıyor.
> Tek satırlık gerekli değişiklik: `REP_DAILY` yalnız `|rep| < 50` aralığında çalışsın — ün eşiği
> geçilirse hafıza kalıcı olur, yoksa korkulan adam olmak "itibar bakımı" grind'ine döner.

---

## 8. MAKAM — kanunun kendisi olmak

Kendi kelle parası olan biri kapı ağası olabilir mi? **Şehre göre** — ve bu altı hukuku bir kez daha
kanıtlıyor:

Tuzkapı **evet** (bounty makam fiyatının içinde) · Karaocak **evet**, borç ocak defterinde kalır,
maaşından kesilir · Karduvar kefil şartıyla · Kumkapı vali sormaz, **aşiret sorar** · Yenitaç deftere
yazdırırsan · Yedisunak **hayır** (kirlilik).

Ağa olunca sistem tersine döner: üç gardiyan emrinde, **başkasının bounty'sini görüyorsun** — geçir
ya da çevir. Geçirdiğin adam suç işlerse bounty'nin bir dilimi **sana** yazılır: Karduvar'ın kefil
mantığı, oyuncunun elinde. Af yetkisi var ama denetimli — her af kapının defterine geçer, meclis/vali
ayda N af tanır, aşarsan makam elinden alınır. Ve **kendi adını kendi defterine yazamazsın.**

---

## 9. DÜRÜST UYARILAR

### Sömürülür
1. **Kurye kesme** haberi tamamen durdurur → fren: kayıt Kırk Kese'ye borç olur, ikinci kurye mühürlü.
2. ⚠ **Tanıksız cinayet zaten bedava** (kodda). Altı şehirde bu ücretsiz temizlik makinesine döner.
   **Fren: kayıp mekaniği** — kimse görmese bile bir gün sonra NPC "kayıp" olur, bağlı NPC'ler rep −3.
   Suç değil, **iz**. Ucuz, çok fark eder.
3. **Tuzkapı senediyle sonsuz erteleme** → aynı anda tek açık senet + vade sayacı.
4. **Makam affı** → denetim tavanı (yukarıda).

### Oyuncuyu sıkar
- Altı ayrı panel kuralı ezberlenmez → dördüncü düğme **her zaman tek satır**, şehir adıyla etiketli,
  kuralı gardiyan repliğinde söylenir.
- Süreli cezalar (sürgün, kefaret) **asla 2 şehirden fazlasını kapatmasın** — en az dört şehir her an
  açık.
- Kurye görünmezse oyuncu hile sanar: haritada görünmeli, kaçarken tek satır bildirim gelmeli.

### Mimari
⚠ Tek muhafız (`guard_index()` tek NPC, tek statik state) **altı şehir için yetmez** — ama kalabalık
AI da gerekmiyor. `guard.c`'yi **`GuardUnit guards[12]`** dizisine çevir (şehir başına 2 + kırsal
devriye); mevcut durum makinesi aynen kopyalanır, birim başına `postX/postY` + `lawId` taşır. Panel
yalnız en yakın birimden açılır, HOSTILE tek seferde bir birimde. `rep ≤ −40` iken **iki birim
birlikte** gelir — korku kapısının faturası burada ödenir.

---

# ORKESTRATÖR EKİ — KAOSÇU ÖZGÜRLÜĞÜ

> İkinci tasarımcı harcama limitinde kesildi; bu bölüm orkestratör tarafından yazıldı, ajan
> doğrulaması geçmedi. **Denetlenmemiş sayılmalı.**

## 10. ON KAOS EYLEMİ — kanonun içinden

Kalibre: jenerik "dükkân soy" değil; sistemlerin cevap verdiği, kanona bağlı eylemler.

| # | Eylem | Nasıl (mevcut sistem) | Dünya nasıl cevap verir |
|---|---|---|---|
| 1 | **Taht salonundaki sancakları çal** | Gizlilik + kilit; salon çatısız, iskeleden girilir | Meşruiyet çubuğu **geri sayar** — duvarda görünür. Üç taraf birbirini suçlar |
| 2 | **Mahzendeki buzu sat** | Kontratı al, buzu teslim etme | Tuğrul'un cesedi çürür. Rüstem'in kırılması geri alınamaz |
| 3 | **Damar kazıp mezarlığı komşu köye sal** | Kaz, gece bekleme, kandil hattını boş bırak | O köy kalıcı zarar görür; sen loot toplarsın |
| 4 | **Kuryeyi kes** | Yol pususu | Haber durur, Kırk Kese'ye borçlanırsın (§3) |
| 5 | **Kandil yağını sat** | Bakım işini tersine çevir | O hat gece ölü hattı olur; yağ fiyatı yükselir, sen satıcısın |
| 6 | **Balık deliğini açık bırak** | Buz deliği aç, kapatma | Sabah biri basıyor (kanon §8'in tekrarı — bu kez senin elinle) |
| 7 | **Kabukçu damgası basıp geceye çık** | Damga gece parlıyor ve **çağırıyor** | Ölüleri kasabaya çekersin; muhafız sana koşar |
| 8 | **Ergün'ün kuyusunu zehirle** | Simya + gizlilik | Sürgün hanedanın tek somut iyiliği çürür; Kavurgan'ın parası boşa gider |
| 9 | **Sancağı çal, başka şehri suçla** | 1 + kanıt yerleştirme | İki şehir birbirine girer; sen üçüncü taraf olarak satarsın |
| 10 | **Sarp'ın tepsisine kasten yanlış nesne koy** | Odaya erişim | Altı ağızdan biri seni doğrular; **yanlış bir "beyimiz istiyor" dünyaya yayılır** |

## 11. TANIK OYUNU

"Tanık yoksa suç yok" kuralı kodda ve bir hazine. Dört kaldıraç zaten var:
**zaman** (NPC saat rutinleri — kim ne zaman nerede) · **ışık** (gerçek zamanlı ışık motoru; kandil
söndürmek tanık yarıçapını daraltır) · **örtü** (`decor_conceals` — çalı/saman) · **kalabalık**
(pazar saatinde tanık çok, şafakta yok).

**Tanık öldürmek çözüm mü?** Hayır — ve bu tasarımın en iyi freni: tanık öldürmek **yeni tanık
üretir** (350px yarıçap ikinci kez çalışır), ve kayıp mekaniği (§9.2) tanık olmasa bile iz bırakır.
Doğru çözüm tanığı **öldürmek değil, oradan uzaklaştırmak** — rutinini bozmak, işini vermek, başka
yere çağırmak.

## 12. KAOSUN KARİYERİ

İyi oyuncunun berat/kale merdiveni var; kaosçunun karşılığı:

1. **Yüzü bilinen** — bir şehirde tanınıyorsun, esnaf fiyat kırıyor
2. **Aranan** — kelle ilanı şehir kapısına asılıyor (ekranda görünür sprite)
3. **Korkulan** (rep ≤ −40) — §7'nin korku kapıları açılıyor; muhafız takviye bekliyor
4. **Ağ sahibi** — Kırk Kese/Kısıklar defterinde kendi hattın; sana çalışan NPC'ler
5. **Kaydı olmayan adam** — Kabukçular seni içeri alıyor; hiçbir şehirde temiz değilsin, hiçbir şehir
   seni tutamıyor

Ekranda: kapı ilanı · gardiyanın takviye beklemesi · esnafın haraç diyaloğu · ozanın senin şarkını
**farklı** söylemesi.

## 13. BEDEL — kapı kapatmadan

`REP_TALK_REFUSE −50` şu an **sert bir duvar** ve dürüst değerlendirmem: kısmen kötü. NPC'nin hiç
konuşmaması özgürlüğü değil içeriği kesiyor. Öneri: −50 altında NPC **konuşmaz ama korkar** — kısa,
tek satırlık korku replikleri açılsın (haraç, yalvarma, kaçma). Duvar kalksın, ton değişsin.

**Pişmanlık:** kefaret yolu olmalı ama **iz kalmalı.** Kanon zaten söylüyor: bounty silinir, itibar
unutulur ama defter kalır. Yedisunak seni affeder, gaziler "kefaretli" der; Kırk Kese borcunu kapatır,
defterinde durursun. **Affedilmek ≠ silinmek.**

**MVP'de kalan üç kaos eylemi:** kurye kesme (2 numaralı sistemi kanıtlar) · kandil yağını satmak
(ışık motoru + gece tehdidi) · damar kazıp mezarlığı salmak (kaynak = tehdit bağını kanıtlar).
Sancak çalma ve tepsi sabotajı ana zincire bağlı olduğu için sonraki fazda.
