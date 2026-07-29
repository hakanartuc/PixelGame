# FAZ K — KONSEPT DEĞERLENDİRMESİ
*Klişe avcısı + anlatı tasarımcısı çapraz raporu · 2026-07-24*

## Puanlar — iki uzman TERS sıralama verdi

| Konsept | Özgünlük (klişe avcısı) | Oynanabilirlik (anlatı tasarımcısı) |
|---|---|---|
| **A — EMARA** (borç/sözleşme) | **8.0** → 1. sıra | 6.5 → 3. sıra |
| **B — ARDEMIR** (dışarıdan) | 6.0 → 3. sıra | 7.5 → 2. sıra |
| **C — AKBAL** (çürüme) | 7.5 → 2. sıra | **8.5** → 1. sıra |

Gerilim net: **A en zeki kurgu, C en oynanır dünya.**

---

## Kritik bulgu 1 — üçü aynı dilden konuşuyor
Kurgucular birbirinden habersizdi ama **isim havuzunu paylaştılar**: *Damar*
(A ve C'de aynı işlev), *Suvar* (A ve B'de aynı iş), *Berat* (üçünde), *Meram*,
*Kavlak*, *Derbent*, Velda/Velmar. → **Melezleme doğal**, ama seçim sonrası
**isim temizliği şart**.

## Kritik bulgu 2 — üçü aynı dört tuzağa düştü
1. **Düşman bir belgedir** (Berat / tapu / durgu) → "dünya adliye gibi kokar"
   riski. Kural: her hukuki gerçeğin **10 dakika içinde bir bedeni** olmalı.
2. **Doğu = defter, Batı = para** — üç kurgucu da aynı oryantalist varsayıma
   düştü. Kural: en az birini kır (arşivi Batı'ya ver, ya da **tam kaydı
   yalnız bağımsız ortaya**).
3. **Final bir terazidir** — üçü de aynı kelimeyi buldu. Kural: eşik **bir
   odada, adı olan insanlarla** geçilecek.
4. **Masum küçük yeraltı/su halkı** (Suvar / Havran / Meram) = "soylu vahşi".
   Kural: bu halkın **kendi yırtıcı kurumu** olacak.

## Kritik bulgu 3 — konseptlerin "bedava" iddiaları kodda doğrulanmadı
Anlatı tasarımcısı `src/` üzerinde doğruladı:
- **B'nin "kısalan kışlar mevsim sistemiyle bedava"** iddiası **YANLIŞ**:
  `gametime_season()` günden deterministik, 10 gün sabit, save'e bile girmiyor.
  Ayrıca fark edilmesi 40+ oyun günü ister → anlatı olarak ölü.
- **B'nin "dual-grid su altyapısı harita değişimini ifade eder"** iddiası yarı
  doğru: render tarafı var ama `tilemap_set_tile()` **save'e girmiyor**;
  gölü çekmek collision + minimap + decor/resource/yuva koordinatlarını vurur.
- **C'nin iddiaları büyük ölçüde DOĞRULANDI**: `ecology_do_ritual()` (sızıntı
  kapatma) birebir hazır, `drawScale`+`tint` (Semiz), `DialogOptState.hidden`
  (kaybolan seçenek), `skill_set_xp_mult()` (Akbal XP), `TileType.damage`
  (Kavlak), Mummy.png + mezarlık tileset'i (Asmar).

---

## Konsept başına en sert eleştiri ve kurtarma

### A — EMARA
- **En büyük özgünlük**: mobilyayı atmadan fonksiyonu değiştiriyor (mühür =
  sayaç, mumya = personel, ejderha = taşeron). B ve C "suçluyu" değiştiriyor,
  A "nesnelerin ne işe yaradığını" değiştiriyor — bir kat zor manevra.
- **En büyük oynanış sorunu**: Perde 1'de oyuncunun ortak düşmana karşı
  **hiçbir fiili yok** (Sayman'a vurmak yanlış cevap). Tehdide dokunmak
  Ch2'yi bekliyor. **Yüzsüz tehdit riski: yüksek.**
- **En pahalı kalem**: Sayman sprite'ı (hiç karşılığı yok) + Odara/ejderha
  (diğer ikisinin aksine **ertelenemez**, orta oyunda).
- **Klişe kalıntısı**: Suvar ("tornavidalı soylu vahşi") + son perdenin üç
  parçalı MacGuffin avına dönüşmesi.
- **Kurtarma**: Suvar ne yaptığını **biliyor** ve 300 yıldır başkalarının
  payını ödüyor → masum halk değil, kıtanın **en büyük alacaklısı**.

### B — ARDEMIR
- **En büyük özgünlük**: **Andar** (öldürmez — yemini geriye dönük iptal eder,
  hiç var olmamış olursun) ve **besleme düzeni** (400 rehin çocuk) — üç
  dokümandaki en iyi kurum fikirleri; karakter üretebilen tek yapı.
- **En büyük sorun**: GoT haritasının üstünde oturuyor (kuzey/buz/kapı/tampon)
  ve **"80 yıldır kimse fark etmedi"** aptal-olay-örgüsü şartı → ÖLDÜRÜCÜ.
- **Kurtarma**: fark ettiler — biri 40 yıl önce ölçtü, yazdı, **Meram raporu
  sattı**. Kuzey bir keşif değil **örtbas edilmiş dosya** olur.
- **Ayrıca**: tehdidi **öldürülebilir** olan tek konsept (Sedde bir sefer
  heyeti, sistem değil) → Tuzak'ın dediğini yaparsan hikaye biter.

### C — AKBAL
- **En büyük güç**: mekanik omurgası **zaten kodda**; oyuncu 15. dakikada
  tehdidin merkezinde geri alınamaz karar veriyor; ölçek merdiveninin her
  basamağında kılıç sallanacak bir yüz var; kalıcı izin bir kısmı **oyuncunun
  kendi üstünde** (Doygunluk).
- **En büyük sorun**: omurgası **Final Fantasy VII** (Mako=Akbal,
  Shinra=Balyol, AVALANCHE=Tapa) ve alegorisi fazla okunaklı — 20 dakikada
  çözülür.
- **ÖLDÜRÜCÜ bulgu**: "Obur = kıtanın katledilmiş bağışıklık sistemi" = FF7'nin
  WEAPON'ları + Mononoke.
- **Kurtarma**: Obur'lar bilge değildi, **asalaktı**; Obur Avı **meşru ve doğru
  bir karardı**. Trajedi "masumları katlettik" değil **"haklıydık ve bu bizi
  öldürdü"**.
- **İkinci kurtarma**: bir yerde **sürdürülebilir doz** kanıtlanmış olsun →
  kavga "durdur mu durdurma mı" değil **"hangi hızda"** olur; ahlak dersi
  biter, siyaset başlar.

---

## Melezleme — iki rapor da aynı aktarımları önerdi

| Aktarım | Değer |
|---|---|
| **C'nin Doygunluk sayacı → A'ya** | A'nın en büyük eksiği: hiçbir şey oyuncunun bedenine dokunmuyor. A'daki su payı fişi harcanabilir olsun: komşunun payını iç → skill hızlanır, **ama Sayman Defteri'nde başkasının satırına yazılır** |
| **A'nın Sayman Defteri → C'ye** | Obur Defteri: 9 ejderha kaldı, kaçını öldürdün? (Uyarı: **ahlak sayacı değil, mevcut sayacı** — GDD "global karma YOK") |
| **B'nin Andar'ı → hepsine** | Hukuki infaz: hanenin beratı geriye dönük iptal → koca bir NPC sınıfı seni görmez olur. **Sosyal ölüm, mekanik yaptırımla** |
| **B'nin "bölgeye göre suç profili" → hepsine** | Aynı fiil bir blokta suç, ötekinde formalite. **Küçük maliyet, büyük kimlik kazancı** |
| **B'nin besleme sistemi (400 rehin çocuk) → hepsine** | A ve C kurum zengini, **insan fakiri**. Tek başına yüz görev üretir |
| **A'nın Marsen aktüeryası → C'ye** | Felaketi ilan panosundan değil **fiyat listesinden** öğren (Marven yayımlıyor = gürültü; sessizce poliçe yazmayı bırakmak = dehşet) |
| **C'nin Tapa bölünmesi → A'nın Boran'ına, B'nin Tuzak'ına** | Hızlandırmacılar tek notalı; "önce zahire mi, şimdi tıka mı" kavgası örgütü oynanabilir kılar |

---

## Referans çıtası açısından (GoT / Dune / LOTR / Skyrim / Witcher / BG3 / ESO)
- **A → Dune'un YÖNTEMİ** (ekoloji-din-ekonomi aynı kol; CHOAM'vari kaynak
  bağımlılığı krizi) ve Dune'un yüzeyinden hiçbir şey almamış. En iyi
  yöntem/yüzey oranı.
- **B → GoT'un YÜZEYİ** (harita), bir yerde yöntemi (Velmar veraset düğümü).
- **C → LOTR'un YÖNTEMİ** ("uzun yenilgi" — dürüst finali bir hüzünlü küçülme
  olan tek konsept), **Dune'un YÜZEYİ** (madde = melanj).
- **Skyrim/ESO yöntemi** (dünya seni beklemez): **C açık ara birinci**, A ikinci,
  **B sonuncu** (B'nin değişimi senaryolanmış gösteri, yaşayan değişken değil).
- **Witcher/BG3 yöntemi** (doğru cevabı olmayan komşu ölçekli pislik): C birinci,
  B ikinci, **A sonuncu** — "A'nın ikilemleri komşu değil, argüman; okurken
  parlak, oynarken konuşkan."

---

## Hangisi seçilirse seçilsin — teknik uyarılar (kodla doğrulandı)

1. **`MAX_REGIONS 8` sert tavan** (common.h:14) — üç konsept de tam 8 bölge
   sunuyor. `bounty[]`/`reputation[]` anında dolar; "sonsuz genişler" iddiası
   kodda karşılıksız kalır. Büyütmek save formatına dokunur → *paralel dizi
   tavanları dersi (5 vaka)*.
2. **`NEST_SITES` const tablo** — çalışma zamanında yeni yuva doğmuyor. Üçünün
   de "tehdit yayılıyor" katmanı bunu istiyor. **Konsept seçilir seçilmez
   karara bağlanmalı.**
3. **"Bölge durumu" modülü yok** — üçünün de chapter jesti buna bağlı
   (Tuzlanma / Kavlak / sel). Gereken: bölge durumu + **tile override
   kalıcılığı** (şu an save'e girmiyor) + fiyat çarpanı + ekoloji sönmesi.
   *Ucuzlatma: tile değişimi yerine bölgesel post-fx grade + fiyat + ekoloji
   üçlüsü çoğu durumda yeter (F7 altyapısı kodda).*
4. **Sivil-öldürülemez ↔ "insan düşman" sınırı belirsiz** — C'nin Doygunları,
   B'nin Tuzak çeteleri, A'nın Boran'ı hepsi *sivil görünümlü insan düşman*.
   "Bütün bir köy Doygun" hem revir kuralını hem kurguyu kırar. **Kurgu
   yazılmadan karara bağla.**
5. **Diyalog yalnız 10 değişken okuyabiliyor** (`DVAR_*`) ve `storyFlags[16]`
   → **11 dolu, 5 boş**. Üç konsept de onlarca bayrak istiyor. **A0'da
   jenerik "dünya değişkeni tablosu"na çevirmek şimdi ucuz, sonra pahalı.**
6. **"Rota yürüyen öldürülebilir varlık" üçünde de gerekiyor ve yok** (Sayman
   rotası / Pergel ekibi / Balyol konvoyu). NPC ölmez, mob'un uzun rotası yok.
   **Tek seferde jenerik yazılmalı.**
7. **Ejderha üçünde de var, sprite yok** — tek **A'da ertelenemez**.
8. **A0 (İskelet Fazı) hangi konsept seçilirse seçilsin önce inmeli**:
   `quest.c` düz 16 slotluk tablo (yalnız KILL/FETCH, önkoşul/etki bayrağı/
   düğüm zinciri YOK), lore sayfası sistemi yok, örgüt çerçevesi yok
   (`assassinPoints` tek int). Üç konseptin de geç oyunu "çoklu hizip
   itibarının tek teraziye dönmesi" üstüne kurulu — altyapısı **sıfır**.
   **Kurgu yazımına A0'dan önce başlanırsa yazılan her düğüm sonradan taşınır.**
