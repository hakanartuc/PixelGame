## Kod doğrulaması (iddiaları grep'ledim)

| İddia | Gerçek |
|---|---|
| Mevsim ekonomisi | `item_season_trade_mult` (item.c:554) — **yalnız yiyecek+odun**, kış ×1.35. Yeni mal eklemek küçük iş. Mevsim `gametime_season()` günden türetilir, **save'e girmez** |
| Bölgesel fiyat marjı | Var ama **elle yazılı 5 item** (main.c:5100). Genel sistem değil |
| Kar | **Tileset yok.** `weather_snow_cover()` = tüm ekrana alfa-60 beyaz dikdörtgen (main.c:10879). Kar savaş sinerjisi (buz +%20, herkes −%12) kodda |
| Post-fx grade | `R2DPostFX` **global**; `POSTFX_PRESETS` F7 ile döner. Bölge/chapter'a bağlamak küçük iş |
| Kandil/ocak ışığı | `light_append_point` + `storyFlags[SF_K9_KANDIL]` ile **zaten durum-bağlı sönen bir kandil var**. Dört konseptin de "her evde bir lamba" fikri bedavaya yakın |
| Yuva (ekoloji) | `NEST_SITES[]` **const, 26 saha**. Göç yalnız önceden yazılmış alternatif sahaya olur. Çalışma-zamanı yuva doğumu **yok** |
| Mob bütçesi | `MAX_MOB_TYPES 32`, **26 dolu → 6 boş slot**. `MAX_MOBS 64` dünya geneli aynı anda |
| Bölge tavanı | `MAX_REGIONS 8`; `REGIONS` 6 dolu. `bounty[]`/`reputation[]` Player'da, **save v29'da** |
| Zincir modeli / lore sayfası | **İkisi de yok.** quest.c düz KILL/FETCH, `MAX_QUESTS 16`, önkoşul yok. `storyFlags[16]` Uint8 → **5 boş**. Journal sekmeleri: Görev/Günlük/Ünvan |
| Bölgeye göre suç profili | **Yok.** Bounty/itibar bölgesel ama suç *kuralı* global (crime.c) |
| Madencilik / vinyet / kontrat | Hepsi kodda: `resource.c` (kazma araç şartlı), `scene.c`+`ECHOES[]`, `contract.c`+`assassin.c` |

⚠️ **D ile K aynı konseptin iki kopyası**: ikisi de "kor" ekonomisi, ikisi de lamba/ateş çırağı başlangıcı, ikisinde de Saltmother/uyanan-kuyu ejderhası. K ile G ikisi de kıtaya **Kaldmark** demiş. Bunları iki ayrı seçenek gibi oylamayın.

---

## D — BUZ VE ATEŞ (Korvan)

| Fikir | Taşıyan sistem |
|---|---|
| Kor kazmak | `resource.c` (kazma kuşanma şartı hazır) + `ores.png` |
| Sönen ocak = ölüm | `light_append_point` + storyFlags (K9 deseni birebir) |
| Kuzeye kâr marjı | Bölgesel satış çarpanı (elle, +küçük) |
| Külsar/Kazgan/Karboy | mumya/goblin/ork **reskin yok, sadece isim** |
| Ejderha rüyaları | `scene.c` vinyet + ECHO_ANI |

**Zorunlu yeni iş:** buz hattının ilerlemesi için **bölge durumu modülü** (orta) — yoksa "kış geliyor" yalnız metin kalır, konsept *zayıflar*. Ölülerin ısıya yürümesi = yeni AI hedefi (orta). Ejderha sprite'ı Perde 2'ye ertelenebilir (Perde 1'de ejderha görülmüyor, ısısı görülüyor — dördünün en ucuz ejderha çözümü). **İlk 30 dk:** ocaktan kor al → üç eve yürü, E ile ocak yak (ekran ısınır) → dördüncüye geç kal, adam ölmüş → köy göğsüne çivi çakar, kimse sebebini söylemez → gece rüya vinyeti. **Fiil:** teslimat + kazı; kavga kontrat panosundan gelir. Merdivenin orta basamağı zayıf: "kazma hızı" oylanır, oynanmaz. **Sanat borcu:** 4 bölge tileset'siz (kar/buzul/cam ova). **Puan: 6**

## K — KADİM KAYNAK (Kaldmark)

| Fikir | Taşıyan sistem |
|---|---|
| **Kor yak → çekim sayacı → gece dalgası** | `WE_GRAVE`/`WE_SEAL` + tek sayaç |
| Sönük bölge | Ekoloji yuva yıkımı + grade |
| Fenwake (konuşan ölü) | mumya sprite + diyalog |
| Kül Halkı grevi | itibar hattı + dükkan kilidi |

**Zorunlu yeni iş:** çekim sayacı (**küçük** — dördü içindeki en ucuz+en güçlü tek mekanik: büyü ilk kez bedelli olur). Sönük bölge durumu (orta). Final "kıta çapında ekonomik silahsızlanma" bir **fiil değil**, masa mekaniği — kodda masa yok (büyük). **İlk 30 dk:** rasyon kor dağıt → fitil kes, tavşan avla → **rasyon eksik gelir: 6 hane, 5 pay, kimi üşüteceğini sen seçersin** (oyunun tamamının özeti, ilk 20 dakikada) → bahçede toprak tıkırdar → çıkan şey senin taşıdığın lambaya yürür. **Sanat borcu:** 3 bölge tileset'siz. İngilizce isimler TR birincil dilde sürtünme. **Puan: 7**

## C — ÇÖKEN DÜZEN (Ocakbağı)

| Fikir | Taşıyan sistem |
|---|---|
| Yol/kervan omurgası | `WE_CARAVAN` + bölgesel marj + pano — **hepsi çalışıyor** |
| Mezarlık tarlası = yuva | `NEST_GRAVE` (ritüel mühürleme kodda) |
| Yol kandilleri | light + ECHO_YAZIT taşları |
| Tıkırtı | `ambience.c` — **bir ses, sıfır sistem** |
| Moorfather / Tutuşan | mumya vs iskelet = *iki farklı ölü anlamı*, bedava |

**Zorunlu yeni iş:** en azı. Dokuz nefes sayacı konseptin kendi uyarısının aksine **tek Uint8 storyFlags'e sığar**; asıl eksik onu asacak zincir modeli (A0, ortak). Ejderha sprite'ı yazılı olarak Perde 2'ye ertelenmiş. **İlk 30 dk:** arabayı yükle, üç bölge arası taşı (var olan marj döngüsü) → panodan kontrat, kuyu ağzında Seep temizle → 206 yıldır soğuk kandil ılık bulunur, [E] Dokun → vinyet → taşıdığın yükün *kesik kor* olduğu çıkar, seni tutan adam ölü → Tacın öncüsü: "Kuzeye giden yol kaç gün?" **Merdiven:** teslimat → geçidi/madeni kim tutacak → **dokuz nefes geri sayımı** (dördün en oynanabilir finali: sayılabilir, harcanabilir, geri alınamaz). **Sanat borcu:** en düşük — Holloway/Brackenmoor/Cinderdelve/Stonewatch/Saltreach/Hearthold mevcut çim-bataklık-mağara-klif-kumsal-kaldırım setleriyle karşılanır; yalnız Glasswood + Coldhearth borç. **Puan: 8**

## G — KIRIK GÖK

| Fikir | Taşıyan sistem |
|---|---|
| **Parça düşer → yarış** | Yeni worldevent (küçük) + `contract` — tekrarlanabilir döngü |
| Glim içmek = gizli yol | **Yankıcı deseni birebir**: scene fx `ritual` → sınıf açılır (SF_YANKICI) |
| Glim Yükü + tint | postfx/tint boruları hazır |
| Vettir/Skarn/Draug | goblin/ork/mumya, sıfır reskin |
| Beyazlar | **Sivil-öldürülemezlik kuralını kurgu içinden çözen tek konsept** |

**Zorunlu yeni iş:** ilk 10 dakikanın "yangın söndür / yaralı taşı" fiilleri **kodda yok** (küçük-orta, ama açılışın tamamı buna asılı). Ölülerin kıta boyu kuzeye göçü çalışma-zamanı yuva ister (orta-büyük); olmazsa tehdit *zayıflar*, çökmez. **İlk 30 dk:** gökten parça düşer → söndür/taşı/say → ormanda **kimsenin bilmediği ikinci parçayı** bulursun → gün bitmeden dört taraf kapına gelir → **sat / sakla / rahibe ver / iç.** Dördü de farklı bir kimlik açar, dördü de mevcut diyalog req-fx + kişilik sayaçlarıyla kodlanabilir. **Merdiven:** üç basamağın üçü de aynı nesne (parça) etrafında ve alt basamak *tekrarlanabilir bir döngü* — dördün en temizi. **Sanat borcu:** en ağır — Ribhold (ejderha leşi şehir), Skarnmark, Yarık, iki kar bölgesi; beş **yaşayan** ejderha kritik yolda (Ashling at boyunda diye ucuzlatılmış ama yine de sprite). **Puan: 8**

---

**En az yeni kodla en çok anlatım: K.** Tek sayaç (kor yak → çekim) her büyüyü, her lambayı, her kışı anlatıma çevirir. C çok yakın ikinci ve daha az *varlık* ister; ama C'nin yükü lore sayfalarına biniyor — o sistem kodda yok.

**Erken oyun en güçlü: G**, farkla. Somut, uzamsal, ilk 10 dakikada olan bir olay + gün bitmeden dört yollu kimlik çatalı. İkinci: K (eksik rasyon = kıtlığın tutorial'i).

**Sıralama:** **1) G** — iki tasarım sütununu (kimlik davranıştan doğar; şablonla büyüyen temel: "yeni bölge = yeni parça düştü") dördü içinde en iyi karşılayan, tek tekrarlanabilir yeni döngüsü olan ve sivil kuralını kurguyla çözen tek konsept. **2) C** — en ucuz, en çok hazır sistem kullanan, tileset envanterine en iyi oturan, finali sayılabilir olan. *Karar ölçütü para/sanat ise 1 ile 2 yer değiştirir.* **3) K** — mekanik olarak zarif ama D'nin ikizi ve finali oynanabilir bir fiile bağlanmamış. **4) D** — K ile aynı dünyayı daha pahalı sanat borcuyla anlatıyor; orta basamağı (bölge hedefi) oynanmıyor.

**Hangisi seçilirse seçilsin en riskli 3 nokta:**
1. **`MAX_REGIONS 8` + save v29.** Dördü de 10 bölge istiyor. `bounty[]`/`reputation[]` Player'da ve save'de; `region_of()` de harita-farkında elle yazılmış. Tavanı büyütürken **tüm paralel dizileri ve save.c tamponlarını** birlikte taramak şart — bu depoda daha önce 4 kez ısırmış bir hata sınıfı.
2. **Zincir veri modeli + lore sayfası hiç yok.** quest.c önkoşulsuz düz KILL/FETCH (16 tavan), `storyFlags[16]`'da 5 boş yer, Journal'da Lore sekmesi yok. Dört konsept de chapter eşiği, kilitli düğüm ve "sayfa bulunca zincir açılır" varsayıyor. Bu **maliyetin en büyük tek bloğu ve dördünde eşit** — yani seçimi etkilemez ama A0'da bitmeden hiçbir konsept yazılamaz.
3. **Ekoloji yuvası derleme-zamanı sabit + 6 boş mob slotu.** Dört tehdit de *yayılmak* zorunda (buz hattı iner / sönük dalgası / Tutuşanlar ekonomiyi basar / ölüler kuzeye yürür), ama `NEST_SITES[]` const ve göç yalnız önceden yazılmış sahalara. Çalışma-zamanı yuva doğumu + `MAX_MOB_TYPES 32` bütçesi (yürüyen ailesi + ejderha + yeni siviller 6 slota sığmaz) birlikte planlanmalı.

*Bonus risk:* kar tileset'i yok ve dördü de kış konsepti. Öneri: soğuğu **tile'a hiç bağlamayın** — grade + mevcut kar örtüsü + fiyat + gündüz kısalması zaten kodda ve birlikte "kış geldi"yi ücretsiz anlatıyor.