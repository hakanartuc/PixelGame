`.claude/agents/klise-avcisi.md`, `docs/konsept/G-kirik-gok.md`, `docs/GDD.md` §1.2b/§4 okundu; `src/common.h:14` (`MAX_REGIONS 8`), `src/player.h:142-143`, `src/worldmap.h:32` (sunriseOffset), `src/r2d.h:100` (postfx) doğrulandı — üçünün de teknik iddiaları gerçek.

# HAKEM RAPORU — HUZUR BÖLGESİ

| Test | V · Sonyaz | O · Loftmark | Y · Kettlemoor |
|---|---|---|---|
| Huzur | **9** | 6 | **8** (girişte) / 10 (kademe 3) |
| Sıkıcılık (yüksek=dolu) | 8 | 6 | **9** |
| Klişe temizliği | **8** TEMİZ⁻ | 4 YIPRANMIŞ⁺ | 6 YIPRANMIŞ |
| Dünya mantığı | 8 | **4** | **9** |
| Aidiyet | 8.5 | 7 | **9** |
| İnsan eli | 8 | **8.5** | 7 |

## 1) HUZUR
**Y kazanıyor, ve sebebi tek bir satır:** buhar vadinin üstünü kapatıyor, **Yarık görünmüyor**. Oyunun sabit dehşet kaynağı fiziksel olarak örtülüyor — "yukarı bakma" cümlesinin nesnesi yok. İkinci vuruş: kanon "glim ısıtmaz" diyor, 214 yıldır kimse ısınmadı, burada suya girip ısınıyorsun. Bunlar kanondan türeyen bedensel rahatlama. "Ayağının altındaki toprak yumuşak — ilk kez kendi adımını duyarsın" cümlesi üç dosyanın en iyi duyusal fikri.

**V** anında ve eksiksiz huzur veriyor: uğultunun kesilmesi (her kasaba glim feneriyle uğuldar, burası uğuldamaz), gerçek karanlık, koku katmanı, turuncu ateş ışığı. Duyusal olarak kuzeyden de doğudan da temiz ayrılıyor. Ama huzuru **kirleniyor**: içinde durduğun bahçe mezarlık, ihracat malı yıkanmış ceset suyu. Muhteşem kurgu, ikinci ziyarette dinlendirici değil.

**O** huzur değil **hayret** veriyor. Gölgesizlik, gecesiz gün, mevsimsiz yıl — bunlar rahatsız edicidir, dinlendirici değil; metnin kendisi "ilk üç gün hasta olursun" diyor ve uyumak için mahzen kazdırıyor. Ayrıca oyunun mevcut oturum döngüsünü (şafak zarı → gün → [R] dinlen) yukarıda kırıyor: gece yoksa ritüel yok.

## 2) SIKICILIK
**Y**: tek fiil (göze açmak) etrafında gerçek bir zincir + **kalıcı, görünür puan tabelası** (kar sırtlara çekiliyor). Ve dâhice olan şey: ısıtmak seni görünür yapıyor — huzur pasif değil, tekrarlanan bir karar. Oyuncunun bir şey **eklediği** tek yer tespiti doğru.
**V**: yüksek marjlı ihracat, nötr oda, Sayım siyaseti, Neriman'ın iplikleri, ve güvenli bölgenin ortasında ölümcül Sürgü sırtı. Dolu. Dönme sebebi ekonomik + sosyal; Y'deki gibi tırmanan değil.
**O**: windfall hasadı iyi bir döngü ama tek döngü. İlerleme ekseni yok — yalnızca sosyal itibar. Güzel kartpostal riski en yüksek olan bu.

## 3) KLİŞE
**O — YIPRANMIŞ⁺, öldürücüye bir adım.** Üç ayrı kalıp üst üste: gökada/Laputa (*Castle in the Sky*, *BioShock Infinite* Columbia, *Skies of Arcadia*), "kaybolan her şeyin gittiği yer" (Ariosto'nun ay vadisi, *Neverwhere* Floating Market), ve en tehlikelisi — **Doğanlar dışarının hikâyelerine inanmıyor**: bu birebir *The Village* (Shyamalan) + *Wayward Pines*. Kubbenin üstüne çıkmak da *The Truman Show*/*Under the Dome* refleksi.
**Y — YIPRANMIŞ, ama içerikte değil sistemde.** "Harabeyi kademe kademe canlandır" = *Stardew Valley* community center, *Tarrey Town*, Fallout 4 yerleşimleri. Kurtarıcı: içerik taze — devletin bir zanaatkârı zorlayıp kaynakları tıkaması Owens Valley / *Chinatown*'dur, fantezi değil. Kademe tablosunu at, aynı fikir klişe kokmaz.
**V — TEMİZ⁻.** "Sırtın ardındaki korunmuş vadi" Rivendell/Shangri-La geometrisi, ama içerik kimseye benzemiyor: dokuz suda yıkanan ölü, üstüne dikilen armut ağacı, kıtanın en pahalı meyvesi. Gizli cennet değil — herkesin bildiği, faturası kesilmiş bir hizmet.

## 4) DÜNYA MANTIĞI
**Y en sağlam.** Cevap üç katmanlı: (a) vadi seni **soyar** — bu dünyada güç glimden geliyor, buraya taşınan lord lord değil, Draug ölüyor; yani kaçmaya gücü yeten herkesin tam olarak kaybedeceği şeyi alıyor. (b) Zaten yarı ölü, kaçılacak kadar iyi değil. (c) Y176'da **bunu isteyen taraf zaten yapmış**. Cevap yapısal değil tarihsel — kurgusal el yok.
**V güçlü ama tek ayağı zayıf.** "Rehine düzeni, ve rehineler zaten ölü" kıyasıya iyi — *işgale* mükemmel cevap. Ama *göçe* cevabı zayıf: kapasite 4000, nüfus 2300; boşluğu ağaçlara ayırmak bir **tercih**, ve tercihler ekonomik baskıyla kırılır. (Kurgu içi gerilim olarak kullanılabilir; savunma olarak sayılmaz.)
**O çöküyor.** Kanon Y202'de bir ejderhanın Yarık'tan yukarı uçtuğunu **herkesin gözü önünde** kaydediyor; Blackholt'un "göğe çıkan kadim yolu" kanon bölge tablosunda macera kancası olarak zaten yazılı; 12 yıldır ip köprü var, 4-5 kişi Vettir'le düzenli takas ediyor. "Aşağıda kimse bilmiyor" tutmuyor. Daha kötüsü: **Vettir kıtanın en fanatik Onarım yanlısı** ve yolun ağzı onların en kutsal mağarası — yukarı ilk onların tırmanması gerekirdi. Dosya bunu [AÇIK] diye bırakıyor; ama bu açık, konseptin taşıyıcı kolonu.

## 5) OYNANABİLİRLİK VE AİDİYET
Üçü de aynı çekirdeği buldu: **Glim Yükü burada düşer** (§4.2/4.4 hazır sayaç, sıfır yeni sistem, gerçek bedel). Bu yakınsama, hangisi seçilirse seçilsin doğru omurga.
- **Y**: respec sunağı (§4.1) buraya oturuyor, hamam = §4.6 dinlenme, tanık→ihbar yerine tanık→sırada sona düşme (§4.4 zarif tercümesi). **Yeni maliyet: orta-büyük** — kalıcı bölge durum kademesi (tileset/hava/grade/spawn/nüfus) + kayıt. Aidiyet en yüksek: kar senin yüzünden çekildi.
- **V**: craft/ticaret (§4.5) en yüksek marjı burada üretiyor, suç→mezar iptali. **Maliyet: küçük** (mevsim + itibar + NPC rutini; "zaman geçirme izni"). Aidiyet: hisse→yer merdiveni ve Perde 1'de içtiğin yudumun 40. saatte iyi ölümünü elinden alması — batchin en iyi geciken bedeli.
- **O**: Sayım = bounty tercümesi temiz, akçesiz ekonomi craft'ı gerçekten anlamlı kılıyor. **Maliyet: küçük.** Ama oyuncunun kaybı sadece bir stat; NPC'ler hafıza kaybediyor, oyuncu yalnız güç kaybediyor — bedel tutarsız.
Üçü de **"ayrı boyut hayır"** diyor ve üçü de haklı; en ikna edici gerekçe Y'de: buhar sırttan görülebilmeli, Kavruk buraya yürüyebilmeli.
*(Asset: V ve Y mevcut tileset+postfx ile çıkar; O gölgesiz/yönsüz ışık için render tarafında ek iş ister.)*

## 6) İNSAN ELİ
**O 8.5** — en iyi ses. "Loftmarklılar birbirleriyle bağırarak konuşur." ve Misafir'i "kayınbirader gibi" anlatması batchin en insan cümlesi.
**V 8** — "Kuzeyliler ona the Plow der ve neyin sürüldüğünü düşünmemeyi tercih eder." Kusur: Türkçe metne serpilmiş İngilizce teknik kelimeler (*sluice*, *windfall*, *hole in the ground*) ve pitch-deck kokan "Neden bu tasarım" listesi.
**Y 7** — prozası iyi ("odunlarını doğru istiflemedikleri için azarlıyor") ama **kademe tablosu §1.2b'nin doğrudan ihlali**: bir yerin canlanmasını tabloyla anlatmak. 1-5 numaralı gerekçe listesi de simetrik.

## SONUÇ

**1. Y — KETTLEMOOR.** En güçlü fikri: *huzur verilmiyor, kazanılıyor — ve ısıttıkça görünür oluyorsun.* En öldürücü kusuru: kademe sistemi tanıdık (Stardew) ve **kademe 3'te savunması buharlaşıyor** — "bir mühendis ve bir gece" tier 0 için doğru, ısınmış ve doğuyla ticaret yapan bir vadi için değil.

**2. V — SONYAZ.** En güçlü fikri: *kıtanın satın aldığı şey manzara değil, kalkmayan bir mezar* — ve rehineler zaten ölü. En öldürücü kusuru: bahçe mezarlık olduğu için ikinci ziyarette huzur bitiyor; brief "rahatlasın" istiyor, bu dosya "tedirgin olsun" veriyor.

**3. O — LOFTMARK.** En güçlü fikri: **Misafir** — göldeki ölmekte olan ejderha, tapılmıyor, "geldi, gitmiyor, çok yiyor, suyun tadını bozuyor". İçinde ejderha olan bir su davası. En öldürücü kusuru: gizlilik tutmuyor (Skyward'ı herkes gördü, yol kanonda yazılı, Vettir orada oturuyor) — dünya mantığı kolonu yok.

**Melezleme (2 öneri):**
1. **V'nin Kantar'ını + rehine mantığını Y'ye tak.** Vadi ısındıkça kıtanın haneleri ölülerini Bekleme Damı'na yollamaya başlasın (Y'de bu tohum zaten var, ölçeklendirilmemiş). Böylece Velgard tier 3'te vadiyi bastığında kıtadaki her hanenin annesini rehin almış olur — Y'nin tek zayıf noktası, kademeyle **birlikte büyüyen** bir savunmayla kapanır.
2. **Misafir'i Y'ye taşı.** Uzun Su yerine sıcak göle yatan bir ejderha; Y'nin siyaseti zaten su sırası kavgası, içine ejderha koyunca hem komedi hem fantastik varlık geliyor — huzur bölgesinin "başka bir tür" gibi hissetmesini sağlayan tek şey bu. Yanına O'nun en iyi döngüsünü de al: **aşağıda katıldığın her savaş yukarıda/vadide bir kıtlık konseyidir.**

**Not:** V ve Y aynı Draug ismini (Sigrun) ve aynı çekirdek mekaniği kullanıyor — seçim sonrası isim çatışması çözülmeli. Ve üçü de `MAX_REGIONS 8` tavanını taşırıyor (`src/common.h:14`; `bounty[]`/`reputation[]` `src/player.h:142-143`, `src/save.c:201` tampon yazımı) — A0'da tek kararla, **paralel dizilerin hepsi grep'lenerek** çözülmeli.