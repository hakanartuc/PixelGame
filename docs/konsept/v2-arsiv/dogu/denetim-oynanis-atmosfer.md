## VERDİKT (özet)

Üçü de kanona sadık ve klişesiz. Ayrım **kamerada** ve **ajanlığın şeklinde** çıkıyor: oyun **top-down 2D**. Bir yaklaşımın imzası yere bakılarak görülüyorsa mekanik olur; göğe bakılarak görülüyorsa ancak *anlatılır*.

## Motor gerçeği (grep'lendi — üçü için de ortak fatura)

| Bulgu | Yer | Etki |
|---|---|---|
| `MAX_REGIONS 8`, `bounty[]`/`reputation[]` bölge başına | `src/common.h:14`, `src/player.h:142-143` | Üçü de taşırıyor → A0 |
| `storyFlags[16]` (16 bayt) | `src/player.h:201` | Üçü de yetmez → A0 |
| `REGIONS[]` sabit const, **tek harita üstünde dikdörtgen** | `src/region.c`, `region.h` | **A'nın ikinci kıtası en pahalı kalem** |
| Mevsim **global**, `SEASON_DAYS 10` × 4 = **40 günlük yıl** | `src/gametime.h:21-28` | S'nin "mevsim durur"u ve A'nın **"60 gün açık yol"**u bölgesel mevsim ister; A'nın 60 günü 40 günlük yıla sığmıyor (sayı düzeltilmeli) |
| `gametime_set_sun_offset()` harita başına | `src/gametime.h:35` | Bölgesel gündoğumu **hazır** — üçü de bedava kullanır |
| 5 hava türü, global | `src/weather.h:19-24` | Ak Yel / tuz fırtınası = 6. tür, **küçük** |
| `NEST_SITES` sabit const tablo | `src/ecology.c:14` | Çalışma zamanı yuva = orta (üçünde de) |
| Oyuncuda `tint` alanı **yok** (`skinTone` var; tint `mob.h:89`'da) | `src/player.h:84` | Glim Yükü görsel tint'i "motorda hazır" **değil** — küçük yeni iş |
| Tek rütbe merdiveni: `assassin.h:19 rankPoints` | — | Çoklu örgüt rütbesi = A0 şablonu (üçü de aynı) |

## A) OYNANABİLİRLİK

| | **S — Akdam** | **T — Camşar** | **A — Tuzova** |
|---|---|---|---|
| Taşıyan mevcut sistem | Glim Yükü (kan muayenesi = **kapı**), bölgesel suç, post-fx | **Glim Yükü (pasif artış = sayaç/baskı)**, hava, suç/tanık, ticaret marjı, ekoloji, ekipman yuvası | Mevsim (yol penceresi), suç, ticaret, gizlilik |
| İkili ajanlık | Kavramsal olarak en şık (**tek hat, iki kıyamet**) ama oyuncu tarafı *gate*: yüksek Glim Yükü = giremezsin. Ceza var, alternatif güç yok | **Sistemik**: bölge seni içirir → **sus taşı** yükü gizler → **çan** sosyal etiket. Üçü de var olan sayaçlara biniyor | **Tek cümlelik en temiz ajanlık**: bir bent haritası çiz, **iki kere sat**. + En iyi ifşa mekaniği: **3 günlük tuz kabuğu** (biri varlığını, öteki yokluğunu kontrol ediyor) |
| Oyuncu neden gider | Hayret → sonra çoğunlukla lore. Ödül zayıf | **Kanon'un 1. saatindeki eşyadan doğuyor** (sırlanmış parça kuzeye kaymıyor) — en zarif on-ramp | **En insani sebep**: "annen yattığı yerde kalır." Müşteri→Kılavuz→Kurye→Broker→Yerbulan merdiveni `assassin.c` şablonuna birebir oturur |
| Yeni iş | **Orta**: bölgesel mevsim kapatma, sınır muayenesi, bölgesel grade | **Küçük-orta**: 6. hava türü, suluk ekipman yuvası, sus taşı item'ı, camaltı katman | **Büyük**: yeni büyü kategorisi (**bent = iptal/nesne**), ikinci kara kütlesi + mevsimlik geçiş, kesme karşı-oyunu |
| Kanon riski | **En düşük** — yeni ejderha yok, "dokuzu düştü" boşluğunu doğru okuyor | 10. ejderha uyduruyor — **en zayıfı** | Aritmetiği **doğru** (kanonda düşen 8 sayılı, 9. slot gerçekten boş) ama Kırılma'yı yerel olaya indiriyor; GDD §3.1 "tek kıta, iki blok" ile sürtüşüyor |

## B) ATMOSFER

| | S | T | A |
|---|---|---|---|
| İmza tek imge | Ufuktan ufka **beyaz tavan**; üstünde yürüyen ejderha gölgeleri; pençe deliğinden **tek yıldız** | **Ayağının altındaki camda, 30 m aşağıda sofrası kurulu şehir**; şafakta çınlayan levha; gece binlerce çan | Tuzda asılı **sağılan ejderha**; kuru denizde dikine duran gemiler |
| Kuzeyden duyusal fark | Sıcak, eski, **fazla düzenli**, boğucu (mükemmel inversiyon) | Parlak, gürültülü, **gurultulu nefes**, öğle kör eder | Kalabalık, pahalı — ama **yıldız var**: varış şoku en zayıf |
| **Top-down'da gösterilebilir mi** | ⚠️ **Hayır** — tavan kamerada yok. Ancak gece yıldızsızlığı, yerdeki gölge geçişi ve diyalogla *anlatılır* | ✅ **Doğal** — saydam kat + altında karanlık şehir katmanı; kamera zaten yere bakıyor | ✅ Evet (set-piece'ler yerde) |
| Görkem / tehlike / merak | 10 / 7 / 9 | 9 / **10** / 9 | 8 / 7 / 9 |

## C) SIRALAMA

| Sıra | Yaklaşım | Oynanabilirlik | Gerekçe |
|---|---|---|---|
| **1** | **T — CAMŞAR** | **8/10** | Atmosferi **kamerayla aynı düzlemde**; imzası her adımda oyuncunun altında. İkili ajanlığı yeni sistem istemiyor — Glim Yükü'nü iradeden **baskıya** çeviriyor, sus taşı/çan ile gizlilik+suç sistemine bağlıyor. Öğle/şafak/gece mevsim-saat sistemini ilk kez **ölümcül** kılıyor. En düşük maliyet, en yüksek dakika-başına atmosfer. Zaafı: 10. ejderha (kanon eklemesi tartışılmalı) ve Kavruk/Sırça ikilisi biraz kalabalık |
| **2** | **A — TUZOVA** | **7/10** | En iyi **tek** ajanlık mekaniği (iki alıcı, bir harita) ve kıtanın en iyi ifşa saati (tuz kabuğu). Doğuya gitme sebebi duygusal ve satın alınabilir. Ama fatura en ağırı: ayrı kara kütlesi `REGIONS` modelini kırıyor, "bent" yeni bir güç ağacı demek, 60 gün 40 günlük yıla sığmıyor, ve **doğuda yıldız olması varış şokunu öldürüyor** — 1. turun reddedilme sebebine en yakın risk |
| **3** | **S — AKDAM** | **6/10** | Kurgusal olarak **en güzeli ve kanona en saygılısı**; sıralamada geride kalmasının sebebi kalite değil **teslimat**: (a) imzası top-down'da görünmez, (b) ikili ajanlığı bir **kapı** (yüksek Glim Yükü = giriş yok) — 40 saatlik build'i cezalandırıyor ama yerine güç koymuyor, (c) soğuk cam/yarım yük satılmadığı için ticaret döngüsü kaçakçılığa iniyor |

## ÖNERİ — T tabanı, S'ten transplantasyon

T'yi taban al; S'ten **ata odaları** (kurutulmuş atalar başını kuzeye çeviriyor — kıtanın en iyi tek dehşet imgesi), **Sarkit** (kimsenin teşekkür etmediği bakım halkı) ve **ejderha gölgesi = takvim** fikrini al; A'dan **tuz kabuğu ifşa saatini** ve **"bir haritayı iki kere sat"** kalıbını doğrudan T'nin Yükçüler/Sırcılar eksenine tak. S'in tavan fikri, T'nin levhası zaten kubbe parçası olduğu için çelişmiyor: Camşar'ın kenarı (Kavyaka) S'in Saçak'ıdır.

**Asset notu (puanlanmadı):** T camaltı katman + suluk/çan portre varyantı ister; S beyaz-tavan aydınlatma modeli + gölge geçişi shader'ı; A yeni kıta tileset'i + asılı ejderha set-piece'i. Üçü de post-fx grade'i (F7) bedavaya kullanır.