# STORY — Anlatı Tasarımı (kanon)

Karar: 2026-07-11 (Hakan) · Omurga: **"Sessiz Gün + Yankı"** (1+2 evliliği)
Sırların tek kaynağı: **LORE_SECRET.md** (⚠️ spoiler — fısıltının gerçeği "Geri Sayan"
orada kilitli; oyuncuya dönük hiçbir metin oradan cümle almaz, tutarlılık alır).
İlke: Anlatı ASSET'E BAĞLI DEĞİL — moblar/NPC'ler/mekânlar değişebilir, omurga değişmez.

## Omurga (tek paragraf)
Bir sabah, dünyanın her yerinde, her şey **dokuz kalp atışı boyunca durdu** — kuşlar havada,
alevler donmuş, sesler yutulmuş. Sonra hayat kaldığı yerden aktı. Herkes yaşadı; kimse
anlamadı. Buna **Sessiz Gün** dendi. O dokuz atımda oyuncu *başka birinin hayatındaydı*:
onun elleriyle bir şey gömüyordu — ve gömdüğü şey ona bakıyordu. Oyuncu döndüğünde
göğsünde soluk bir **mühür** vardı. O günden beri ölülere ve ölümün dokunduğu yerlere
temas ettiğinde **son anlarını görüyor** (Yankılar). Ve her yankı, kimin ölümü olursa
olsun, aynı fısıltıyla bitiyor: ***"...henüz değil."***

## İki katman
- **Dünya yarası (Sessiz Gün)** — herkesin paylaştığı travma. Her NPC'nin bir "o gün
  neredeydin" hikâyesi olabilir (diyalog malzemesi). O günden beri ölüler huzursuz
  (undead içeriğe doğal köprü), tuhaflıklar artıyor. Kimse sebebini bilmiyor.
- **Oyuncunun payı (Yankı)** — yalnız oyuncuda kalan iz. "Seçilmiş kişi" DEĞİL,
  **istemsiz tanık**. Kimseye kolay anlatamaz (deli muamelesi riski — diyaloglarda
  gizleme/açılma seçenekleri).

## Merak motorları (asla erken kapatılmaz)
1. **Yabancı kim?** O dokuz atımda hayatına girdiğin kişi. Ne gömüyordu, gömdüğü şey
   neden BAKIYORDU? → **rüya vinyetleri**: oyuncu her uyuduğunda/dinlendiğinde
   yabancının hayatından bir sahne daha (düzenli anlatım ritmi — uyku = hikâye ilerler).
2. **Mühür neden bende?** Büyüyor mu? Başkasında da var mı? (İleride: ikinci bir
   mühürlü — dost mu düşman mı?)
3. **Fısıltı kimin?** "...henüz değil" — ne henüz değil? Yankıların sahibi ölüler mi,
   yoksa hepsinin ardında dinleyen TEK bir şey mi var?

## Anlatım araçları (uygulama fazları)
- **Faz A — Vinyet sistemi + diyalog yüzü**: tam ekran karartma + parşömen panel +
  statik **pixel diorama** görseli + typewriter anlatı metni + seçenekler (dialog
  `req:`/`fx:`/Speech zarı YENİDEN KULLANILIR). Sahne dosyaları txt (dialog formatının
  kardeşi: `scene:` görsel, `narrate:` anlatıcı). Diyaloglara: portre + isim plakası +
  typewriter. **[Yankı] etiketi**: mühürlü bilgiyle açılan özel seçenekler.
- **Faz B — Açılış senaryosu (ilk 15 dk)**:
  1. *Sessiz Gün vinyeti* (~4 panel): donmuş dünya (pixel dioramanın en güçlü hali —
     normal sahne ama HER ŞEY asılı) → dokuz atım → "başkasının anısı" (bulanık panel:
     eller, kazılmış toprak, bakan şey) → uyanış, göğüste mühür.
  2. *Köye varış* (in-engine mini sahne): ilk seçimli diyalog — kişilik burada
     kurulmaya başlar (kaba/kibar/ketum cevaplar, ufak itibar dokunuşları).
  3. İlk işler doğal öğretici; köyde ilk **yankı noktası** (öğretici yankı vinyeti:
     dokun-gör) + fısıltının ilk duyuluşu.
  4. İlk perde kancası: yankıların yoğunlaştığı yer iması (mevcut haritada Eski Geçit'e
     oturur; harita değişirse kanca taşınır — asset bağımsızlık ilkesi).
- **Faz C — In-engine sahne araçları**: kontrol kilidi, letterbox bantları, kamera pan,
  NPC yürütme scripti, konuşma balonu. (Boss girişi, geçit keşfi, köye ilk varış.)

## ANLATIM İLKELERİ — "NASIL" (her şeyin süzgeci)
Karar (2026-07-11, Hakan): "Önemli olan ne söylediğin değil NASIL söylediğin." Oyuncu
"görev yapıyorum" değil "hikâyenin içindeyim, yaşıyorum" hissetmeli. Her anlatı işi
şu süzgeçten geçer:

1. **Duyular kelimelerden önce** — müzik/ışık/ritim duyguyu metinden önce verir.
   Sessiz Gün: müzik NOTANIN ORTASINDA kesilir, 9 kalp atışı SFX'i sahnenin
   metronomudur, metin o ritimle akar.
2. **Yankı duyusal imzası** — girişte renk çekilir (ışık pass ambient), müzik boğuklaşır,
   fısıltı gerçek bir ses (yakın, alçak); 3 notalık fısıltı motifi (leitmotif) müziğe
   dokunur. Oyuncu imzayı tanıyıp metinden ÖNCE ürperir. Çıkışta renk geri dolar.
3. **Yaşat** — "goblinler saldırganlaştı" CÜMLESİ yasak; devrik yanık araba, otta oyuncak,
   yarıda kesilen çığlık. Çevresel anlatım nesneleri (yankı noktaları) haritanın dilidir.
4. **Vinyet dokunulurdur** — her 3-6 satırda oyuncu eylemi (Dokun/Dinle/Geri çekil);
   Speech zarı + req: vinyet içinde çalışır. Metin duvarı YASAK. Typewriter'da basılı
   tutmak hızlandırır ama atlatmaz.
5. **Kesinti değil süzülme** — kontrol yumuşak alınır/verilir (fade, yavaş letterbox,
   input sönümlemesi); sahne bitince dünya zaten akıyordur.
6. **Görev dili ölür** — quest log = "Yolculuk", birinci ağız günlük tonu
   ("Kemal'e söz verdim..."); bildirimler aynı tonda; Book_UI ile defter görünümü.
7. **Dünya hatırladığını SÖYLER** — bark sistemi (kısa baş üstü replikler): itibar/
   eylemlere sokak tepkisi ("Goblinleri temizleyen bu.").
8. **Sessizliğe güven** — her dakika içerikle dolmaz; tek meşaleli orman yürüyüşü
   başlı başına sahnedir. Boşluk, gerilimin enstrümanıdır.

## AHLAK & SONUÇ MİMARİSİ — "RPG'nin ruhu" (kararlar 2026-07-11)
Temel duruş: **hikâye yargılamaz, sorar.** Global karma YOK (önceki karar geçerli);
oyuncu kötü karakter "seçmez" — kötü şeyler yapar, dünya tepki verir.

- **Mühür aynası (içsel eksen):** her yankı vinyeti aynı sessiz soruyla biter:
  **Dinle/Defnet** (yankıyı huzura erdir — bilgi) vs **Çek/Sömür** (yankıyı tüket —
  bilgi + GERÇEK güç: XP/öz/malzeme; kötülük kârlı olmalı, yoksa dekor). Sayaç GİZLİ:
  etkisi okunmaz, FARK EDİLİR — fısıltının tonu kayar (tanığa mesafeli, sömürene
  neredeyse memnun), rüyalardaki yabancının yüzü değişir.
- **Ölüler konuşur (içerik dönüşümü):** öldürülen görev NPC'sinin kritik bilgisi
  cesedinin yankısından okunur — kötü yol OYNANABİLİR kalır ama ISSIZLAŞIR: bilgiyi
  dostun sesinden değil, öldürdüğünün son korkusundan alırsın. Dünya soğur (kepenkler,
  muhafız, boşalan köy dokusu) ama yol kapanmaz. Ana hikaye asla kilitlenmez.
- **Üç sonuç halkası:** anlık (sahne fx'i) → sistemik (bölgesel itibar/bounty/bark) →
  anlatısal (hikâye bayrakları: fısıltı tonu, rüya içerikleri, kalıcı ölüler, perde
  sonu varyantları).
- **KÖTÜ YOL TAM AYRI HAT (Hakan kararı):** sömüren oyuncu, iyi yolun budanmış hâli
  değil, KENDİ yazılmış sahne hattını yaşar — fısıltının sahibiyle kendine özgü karanlık
  ilişki. Üretim bedeli bilinçli kabul edildi: her anlatı fazında İKİ hat yazılır
  (Tanık hattı + Sömüren hattı); sistemik beat'ler ortak kalır.
- **Epilog kartları (Fallout tarzı):** "iyi/kötü son" etiketi yok. Oyun sonunda bölge
  bölge vinyet dizisi: dünyanın seni nasıl hatırladığı — itibarlar + kalıcı ölüler +
  mühür yolu + son seçimden DERLENİR (vinyet sistemiyle üretilir).

## Yazım ilkeleri
- Amnezi klişesi YOK — hafıza kaybı yalnız o dokuz atımlık "başkasında olma" gizemi.
- Oyuncu "seçilmiş" değil "tanık" — dünya ona görev vermez, o soru sorar.
- Fısıltının sahibi ve gömülen şey UZUN vadeli sırlar; her perde bir katman açar, asla
  hepsini birden değil.
- Yankı vinyetleri KISA (3-6 cümle) ve somut — şiirsellik dozunda, bilgi her zaman var.
- Sessiz Gün'ü NPC'ler farklı yorumlar (tanrı cezası / gökyüzü hastalığı / hiçbir şey
  olmadı diyen inkârcı) — dünya görüşü çeşitliliği bedava karakterizasyon.
- Türkçe metin; ünlemli melodram yok, sükûnetli tekinsizlik ("...henüz değil" tonu).

## DLC / GENİŞLEME KURALLARI (2026-07-11)
- Sessiz Gün KÜRESELDİR — her yeni bölge/biyom/DLC şablonu: *"bu bölge o dokuz atımda
  ne kaybetti + burada ne uyandı?"* Ana omurgaya yara ekler, asla retconlamaz.
- Ana oyunun sonu CEVAP VERİR ama KAPIYI KAPATMAZ (sır katmanlıdır; bkz. LORE_SECRET).
- "Başka mühürlüler var" iması ana oyunda ekilir — hazır genişleme karakter havuzu.
- Yankı noktaları her yeni haritanın hikâye döşeme aracıdır (içerik şablonu).

## YAŞAYAN DÜNYA (onay 2026-07-11 — sistem fazı: Dünya Olayları)
Dünya oyuncudan bağımsız DEĞİŞMELİ (ritim yetmez):
- **Dünya Olayları**: veri-güdümlü tablo (koşul: gün/perde/bölge/olasılık) → olay →
  KALICI iz (baskın izleri, gezgin kervan, kamplar arası çatışma sonuçları).
- **Haber ağı**: olaylar NPC ağzına düşer (bark + diyalog haber düğümleri).
- **Atmosfer saati**: ana hat oyuncuyu bekler ama dünyanın DOKUSU beklemez — zaman/perde
  ilerledikçe Sessiz Gün etkileri kademeli artar (gece tekinsizliği, yeni yankı
  noktaları, NPC tonu).
- **NPC yayları**: 2-3 el yapımı mini-arc (karışmazsan sonuçlanır).
- **Dinlenme + rüya**: kamp ateşinde dinlen → saat ilerler + havuzlar dolar +
  RÜYA VİNYETİ tetiklenir (anlatı ritminin mekanik kapısı).

## İlk perde eskizi (mevcut içerikle gevşek bağ)
Goblinler Sessiz Gün'den beri saldırgan (sebep: geçitteki huzursuzluk). Goblin Derdi
teslimi → Kemal'in iması → geçit keşfi (yankı yoğunluğu) → katlarda yankı vinyetleriyle
"burada ne olduğu"nun kazılması → İskelet Lordu = burayı MÜHÜRLEYEN eski nöbetçi
(yankısı: son nöbetinin görüntüsü) → Lord düşünce fısıltı İLK KEZ cümle kurar.
(1. perde sonu — asıl güç daha derinde.)
