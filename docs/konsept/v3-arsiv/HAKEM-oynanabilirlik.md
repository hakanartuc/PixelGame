`.claude/agents/anlati-tasarimcisi.md`, `docs/GDD.md` §4 ve `src/` doğrulaması tamam. Kod kontrolü hakemliği değiştirdi — bulguları aşağıda kullanıyorum.

---

# HAKEM RAPORU — ÜÇÜNCÜ TUR

## Önce koda baktım, sonra puanladım

Üç şey öğrendim ve puanların yarısı bunlardan çıktı:

**`src/resource.c` içinde kazı ZATEN var.** `verb = "Kaz"`, `toolItem = ITEM_PICKAXE` (kuşanılı olma şartı), `work` ilerlemesi, hareket edince kesilen süreç, ertesi sabah yenilenme. Yani "kürek işi" yeni mekanik değil, mevcut düğüm sistemine bir satır.

**`src/ecology.h` içinde `NEST_GRAVE` ve `NEST_POOL` var**: *"ritüel: tüm moblar ölü + E"* → yuva kalıcı yıkılır, yıkılmazsa 5-15 gün sonra aile yeni sahaya göçer. Bu birebir şu cümledir: *"canavarı öldürmek yetmez, kökünü sökeceksin; sökmezsen bir mevsim sonra yenisi çıkar."* AV bunu icat etmedi — kodda bulmuş.

**Sıcaklık sistemi kodda YOK.** `grep -ri temperature src/` → boş. `cold` sadece bir SDL_Color. `ITEM_TORCH` var ama `light.c`'de meşale bir `bool torchOn`; yakıt tüketimi yok. KITLIK'in kırağı çubuğu, ısı yarıçapı, "ateş ısıtmaz" ayrımı ve fener yakıtı — dördü de sıfırdan.

Ayrıca: item.h'de yasak-mal/bölgesel yasallık alanı yok (SAVAŞ'ın kaçakçılığı bir alan istiyor, küçük), ama `dirtyGold` + aklama oranı (v29) duruyor.

## PUANLAR

| | Oynanabilirlik | Fantezi | Mekanik | Basitlik | Derinlik | **Toplam** |
|---|---|---|---|---|---|---|
| **AV** | 9 | 9 | 10 | 9 | 9 | **46** |
| **KEŞİF** | 9 | 8 | 8 | 8 | 8 | **41** |
| **SAVAŞ** | 8 | 9 | 8 | 7 | 8 | **40** |
| **KITLIK** | 7 | 8 | 6 | 7 | 7 | **35** |

Elenen yok. Ama aradaki fark 46-35, ve alt uçtaki fark ucuza kapanmıyor.

## 1) OYNANABİLİRLİK

Fiil sayısı dördünde de yeterli (9-10 arası), o yüzden ayırıcı değil. Ayırıcı olan **merkez fiil**.

AV'nin merkez fiili *kaz* — dövüşün bitmediği, dövüşten sonra başlayan bir iş. Oyuncuya "canavarı öldürdüm ama iş bitmedi" hissini veren tek paket. KEŞİF'in merkez fiili de *kaz*, ama aşağı doğru: in, çıkar, taşı, kime vereceğine karar ver. İkisi de sağlam.

SAVAŞ'ın merkez fiili *al-sat-taşı*. İyi bir fiil, ama paketin adı SAVAŞ ve oyuncu savaşmıyor, tedarik ediyor. Bu bir kusur değil (Witcher da öyle) ama isimle içerik uyuşmuyor ve oyuncuya rolünü anlatmak zorlaşıyor.

KITLIK'in merkez fiili *taşı*. Kurye fantazisi. Ve ikinci fiili *ölç* — bir kaleye gir, ocağa çubuk sok, rengi oku, deftere sayı yaz. Bu bir sızma görevidir, tamam; ama üretilen şey bir **rakam**, ve o rakamın ne anlama geldiği ekranda görünmüyor. Kıtanın bütün siyaseti görünmeyen bir sayının etrafında dönüyor.

**İlk 10 dakika** dördünde de somut, özet geçen yok. En zarifi KEŞİF: iki yüz yıl önce batmış teknede hâlâ yanan bir fener, ve aynı çamurdan kalkan dokuz yıllık bir boğulmuş. İki nesne, sıfır metin, kural anlaşıldı.

**Ekran testi.** AV: kararan toprak karosu — `dual-grid autotile` zaten standart (son commit'lerde su geçişleri böyle yapılmış), yani en ucuz görsel. KEŞİF: her fikri bir sprite (pirinç pul, çapraz mızrak, ceketteki köy rengi). KITLIK: alev paletinin soluk beyazdan altına dönmesi 16px'te gerçekten çalışır ve zekice — ama kırağı çubuğu HUD, ocak sayısı defter rakamı. Görünmeyen iki merkezi fikir.

**Metin duvarı.** SAVAŞ'ta en yüksek: ilk on saatte iki ordu, iki hanedan çatlağı, altı örgüt, ruhsat/levazım/mühür terminolojisi. İsim yağmuru. KEŞİF'te en düşük, çünkü tarih fiziksel: derinlik = çağ, eşya kartında "kaç kulaçta bulundu" yazıyor.

## 2) SOYUTLUK

**SAVAŞ — TEMİZ.** Kemik yanar, ölü kalkar. Dördünün en maddisi.

**KEŞİF — TEMİZ.** *"Tuzun değdiği hiçbir şeye dur denmedi"* şiirsel duruyor ama nesne düzeyinde: lamba yanıyor, çan çalıyor, nöbetçi kapıda. 2. turun hatasına benzemiyor.

**AV — TEMİZ, tek çatlakla.** Kural bir kürekle öğreniliyor. Ama finaldeki *"Kes"* seçeneği için yazılan şu satır tasarım kaçamağı: *"Ne olduğunu oyuncu görür, biz burada yazmayız."* Üç seçenekten birinin sonucu tarif edilmemiş.

**KITLIK — RİSKLİ.** İki yerden. Birincisi *"Ateş artık ısıtmaz"* bir fizik iptali; oyuncu ilk saat "neden odun ısıtmıyor" sorusunu taşıyor ve cevap ancak lore'dan geliyor. İkincisi daha ciddi: Hoarwyrm için yazılan cümle **"mekanik olarak bir canavar değil, haritada ilerleyen bir çizgi"**. Oyunun nihai tehdidi dövüşülemez bir minimap çizgisi. 2. turun "oyuncu ne oynayacak" hatasına en yakın nokta bu.

## 3) FANTEZİ

SAVAŞ ve AV başa baş. SAVAŞ GoT'un damarını tutuyor: rehin verilen çocuklar, tedarik siyaseti, Tuzhan'ın iki tarafa birden ruhsat kesmesi (Dune'un baharatı, birebir). AV ise ejderhayı çözmüş — ejderha kökün mahsulü, yüzyıllarca büyümüş, konuşmayı öğrenmiş, ve **konuşan bir mahsul avlanmaz**. Bu, ejderhayı bir boss listesi maddesi olmaktan çıkarıp bir ahlaki sisteme bağlıyor. Aynı fikir goblin/orku da açıklıyor: yaşlanmış canavar, ork totemi aslında mezar taşı, kampta çocuk var. LOTR ırklarına on yıldır kimsenin vermediği gerekçe.

KEŞİF'in undead'i kıtanın en iyisi (Nöbet — ölü değil, *vazifede*), ama siyaseti ince: haneler birer isim, entrika soyut. KITLIK ırklarda güçlü (Ashborn hem karakter hem mekanik) ama siyaseti masabaşı.

## 4) MEKANİK UYUM

AV **10**, çünkü paket koda bakılarak yazılmış: `NEST_GRAVE` ritüeli = kök sökme, `Kaz` düğümü = kürek, tanıklı suç = lordun tarlasını izinsiz kazmak, K1→K4 = Kara Kürek rütbesi, mevsim = kuzey kışı, lore sayfası = gömü defteri (sayfa bir POI ve bir kontrat üretiyor — §4.9'un istediği tam bu). Zorunlu yeni sistem: karo başına toprak kirlilik bayrağı + dual-grid overlay. **Küçük-orta.**

KEŞİF **8**: kaz hazır, tuz sayacı küçük (player.h'ye bir int, spellPower'a bağlanır), mevsime ilk kez gerçek iş veriyor (kışın çamur donar, dip açılır). Ama Nöbet yeni bir düşman durumu (eşya-kapılı geçiş, **orta**) ve asıl yük harita: dört yüz millik sekili çukur.

SAVAŞ **8**: yasak-mal alanı yok, item.h'ye eklenecek (**küçük**); ocak yakınlığının yuvayı beslemesi `NEST_GRAVE`'in tersi (**orta**).

KITLIK **6**: kırağı sistemi + fener yakıtı + ısı yarıçapı + "mana envanterdeki eşyadır" (mevcut spellPower/mana akışını söküp yeniden yazmak) + ölçüm mini-oyunu. Toplam **orta-büyük** ve hiçbiri opsiyonel değil — kuralı kaldırırsan paket kalmaz.

## 5) BASİTLİK

AV en net: *"Kök kazan bir avcısın: dökülen kan toprağın altında kök tutar, kök canavar verir."* İlk yarısı tek başına yeterli, ne oynayacağın belli. KEŞİF en heyecanlısı ama *"hiçbir şeyin durduğu söylenmedi"* ilk okumada bilmece. SAVAŞ üç yan cümle ve oyuncunun kim olduğu bulanık. KITLIK net ama heyecansız: taşıyıcısın, taşıyacaksın.

## 6) DERİNLİK

AV'nin genişleme formülü bedava: her bölgede birileri birilerini gömmüş, yani her bölgede kök var, yani bölge şablonundaki "ortak düşman izi" alanı kendiliğinden dolu. SAVAŞ'ınki en temizi (damar yanıyor mu / mühürlü mü / satılıyor mu / bulunmadı mı) ama sabit bir cepheye bağlı: savaş bitince formül de biter. KEŞİF'te derinlik = çağ otomatiği dahice, ama merkez tek biyom — oyuncu kırk saat çamurda. KITLIK'ta genişleme değil **daralma** eğrisi var: kırağı hattı ilerledikçe bölgeler ölüyor.

## EN GÜÇLÜ SAHNE / EN ÖLDÜRÜCÜ KUSUR

**AV** — Redfurrow'da vekil parayı uzatıp *"tarlada bir şey bulmadın"* derken kürek hâlâ elinde. Oyuncu siyasete kılıçla değil kürekle giriyor. Kusuru: **iki kural var.** "Kan → kök" undead'i açıklamıyor; "donmuş toprak kanı içemez, o yüzden ölüler kalkar" ikinci bir kural ve beş dakika testini zedeliyor.

**SAVAŞ** — Gündüz yağmaladığın tarlada akşam oluyor; iki yüz metre ötede levazımın ocağı bütün gün yandı. Aynı yer, iki oyun. Kusuru: oyuncunun rolü bulanık + isim yükü.

**KEŞİF** — İkinci sekide dört ölü asker, çapraz mızrak, ve cesetten aldığın pirinç pul. Mızraklar kalkıyor. "Yenmek" yerine "geçmek" ilk kez bir seçim. Kusuru: siyaset ince, biyom tek.

**KITLIK** — Elinde dört köz: titreyen mülteci çocuk, sönmek üzere olan dövmeci ocağı, ve birazdan büyü yapman gereken boss odası. Hesap tablosu değil, kapıya gelen adam. Kusuru: dövüşülemez final + en pahalı sistem faturası.

## MELEZ

Merkezi bölme. AV merkezde kalsın, üç transplant yapılsın:

**KEŞİF'ten derinlik = çağ.** AV'nin küreği zaten dikey; "kaç ayak derinde kazdığın hangi çağa indiğini belirler" sıfır yeni sistemle bedava tarih katmanı verir. Bin Dönüm zaten üst üste binmiş bir gömü alanı.

**KEŞİF'ten Nöbet.** AV'nin undead'i şu an "donmuş toprak" yamasıyla ayakta. Onu at, yerine *vazifedeki ölü*yü koy — ama AV'nin kuralına bağla: kök bir cesede ulaşınca onu yürütür, ve yürüyen ölü öldüğü gün ne yapıyorduysa onu yapmaya devam eder. Nöbetçi kapıda durur, çiftçi tarla sürer. Tek kural, iki tehdit.

**SAVAŞ'tan Barrowdale'in tarafsızlık bedeli.** AV'nin Hallowmere'i şu an sadece güzel bir sır; Barrowdale'in "iki ordu da ölüsünü buraya gönderir, karşılığında vadi kendi delikanlısını kimseye veremez ve her yıl rehin çocuk verir" fikri huzurlu bölgeye siyasi diş takar. Huzur bir manzara değil, ödenen bir fatura olur.

**KITLIK'tan merkeze hiçbir şey alınmasın.** Kırağı çubuğu ikinci bir kaynak yönetimi ekseni açar ve küreğin dikkatiyle yarışır. Tek tek ödünç: Ashborn ırkı (Slagmouth'un köz kuyularına birebir oturur) ve "her leş bir zindan" formülü.

## NİHAİ ÖNERİ

**AV, iki düzeltmeyle gitsin.** Kazandığı için değil — dördü içinde kodun bugün yapabildiği tek paket olduğu ve "oyuncu neyi oynayacak" sorusuna tek fiille cevap verdiği için: *kazacaksın*. Canavarı öldürmek işin yarısı, sonra kürek çıkıyor, ve o kürek oyuncuyu lordun tarlasına, mahkeme mahzenine, bataklığa ve sonunda siyasete sokuyor — çünkü kimsenin yapmak istemediği işi yapan adam her kapıdan girer. **Düzeltme 1: ikinci kuralı sil.** Ölü yürüyenler donmuş toprağın istisnası değil, kökün ilk filizi olsun; kök bir cesede ulaşır ve onu öldüğü günkü işine geri gönderir (KEŞİF'in Nöbet'i buraya taşınır). Tek cümle, iki tehdit, beş dakika testi temiz. **Düzeltme 2: finaldeki "Kes" seçeneğini yaz.** Üç seçenekten birini "oyuncu görür" diye boş bırakmak tasarım değil, erteleme — Hakan'ın iki turdur reddettiği şey tam olarak bu tür bir boşluk. Bunlar yapılırsa elde şu var: iki cümlede anlatılabilen, ilk beş dakikada kürekle öğretilen, ejderhayı ve orku aynı kuraldan türeten, `NEST_GRAVE` ile `Kaz` düğümünün zaten beklediği bir dünya.