## 1 · TAVAN ENVANTERİ

Kanonun istediği toplam: **16 bölge** (kuzey 10 + doğu 5 + Kettlemoor), **9 örgüt** (kuzey 6 + doğu 3), **10 halk/ırk**, **6 canlı + 4 ölü ejderha**.

| Kaynak | Mevcut tavan | Kullanılan | Kanonun istediği | Durum |
|---|---|---|---|---|
| `MAX_REGIONS` (`common.h:14`) | **8** | 6 (`region.c:49`) | 16 | **2× taşar** |
| `MAP_COUNT` (`worldmap.h:24`) | 7 | 7 | ~20+ | taşar |
| `MAP_W_MAX/H_MAX` | 256×256 tile | dünya 200×150 | 16 bölge tek CSV'ye sığmaz (~64×64/bölge — bugünkü Yeşil Ova 100×75) | **yeni harita zorunlu** |
| `MAX_MOB_TYPES` (`mob.h:13`) | **32** | 26 | +18–25 (Kavruk, Sırça, Yürüyen, Kalan, Beyazlar, Çekilen kolları, cam ekolojisi, 6 ejderha) | **taşar** |
| NPC: `NPC_STATE_CAP` (`npc.c:227`) | **24** | 18 | ≥120 (16 bölge × ~8) | **5× taşar**; ayrıca `save.c:464` `npcAliveFlag[32]` >32'yi **reddediyor** |
| `storyFlags[16]` (`player.h:201`) | **16** | 11 | 60–120 | **taşar** |
| DVAR (`dialog.h:35-46`) | **10** | 10 | +20; ayrıca bayrak okuma yok — `DVAR_HINT` tek bir bayrağa **sabit kodlu** | **taşar + yapısal eksik** |
| `MAX_ITEM_TYPES` (`item.h:8`) | **96** | **92** | +40 (glim ×3, glimçelik, sus taşı, suluk, kav, ağır su, çan, sırlanmış parça, lore sayfaları) | **4 boş slot** — en dar boğaz |
| `MAX_QUESTS` (`quest.h:19`) | 16 | 3 | 150–250 düğüm | model yok (bkz. §2) |
| `WEATHER_COUNT` | 5 | 5 | 6 (Ak Yel) + bölgesel | taşar |
| Örgüt rütbesi | yok — sadece `assassin.c` + `player.assassinPoints` (tek int) | 1 | 9 | **çerçeve yok** |
| Bölge durumu (kademe) | **yok** | — | Kettlemoor 0–4 | **yok** |

## 2 · A0 İSKELET LİSTESİ (bağımlılık sırasıyla)

| # | Blok | Neden | Boy | Önkoşul |
|---|---|---|---|---|
| 1 | `MAX_REGIONS` 8→32 + tüm paralel diziler | 16 bölge için şart | **küçük** | — |
| 2 | **Bayrak deposu**: `storyFlags[16]` → isimli `flags[128]` | zincir/örgüt/kademe hepsi buna yazar | küçük-orta | — |
| 3 | **Diyalog değişken köprüsü**: `req: flag.<ad> >= n` genel erişimi | bugün her yeni koşul = C'de enum+parser+eval satırı. Kanon yazılamaz | küçük | 2 |
| 4 | **Zincir veri modeli** (düğüm/önkoşul/etki), `quest.c`+`journal.c` göçü | `QuestDef` yalnız KILL/FETCH; 16 bölge zinciri + 3 perde bunun üstüne kurulamaz | **büyük** | 2,3 |
| 5 | **Örgüt çerçevesi** (rütbe dizisi + ilişki matrisi + ifşa riski) | 9 örgüt, çatışan üyelik | orta | 2 |
| 6 | **Bölge kimlik/durum kaydı** (`RegionState`: kademe, grade, hava eğilimi, gündoğumu, nüfus) | Kettlemoor'un belkemiği + Onarım chapter tırmanışı | orta | 1 |
| 7 | Tablo tavanları: item 96→192, mob 32→64, NPC 24→128 (+`save.c` tamponu) | içerik sığmıyor | küçük ×3 | — |
| 8 | Harita tavanı kararı: `MAP_COUNT` + tek dev CSV mi çok harita mı | ilk yeni bölge bundan önce inemez | orta | — |
| 9 | Bölgesel post-fx + bölgesel hava havuzu + Ak Yel | üç metnin de saydığı "ucuz" kalem | küçük-orta | 6 |
| 10 | Göç hedefi (kuzeye yürüyen ölü yuvası) | `NEST_SITES` sabit | küçük (bkz. §4) | — |

## 3 · KANONUN TEKNİK İDDİALARI — DOĞRULAMA

**Doğru:** post-fx geçişi (`r2d.h:81-100`, `fs_postfx`) · mevsim (4, 10 gün) · respec sunağı (`TREE_CLICK_RESPEC`) · tanıklı suç (`crime_witnessed`) · revir/sivil-öldürülemez · ekoloji yuva/göç/reis-totem · mob `tint` (`mob.h:89`).

**Yanlış / eksik:**

1. **Kettlemoor: "bölge kademesi kayıt edilir (tileset/hava/nüfus/grade)"** — `Region` yapısı yalnız `name/rect/mood/camps` (`region.h:22`). Bölge durumu diye bir şey **yok**, `save.c`'de tek satırı yok. Paketin en pahalı tek kalemi; üç doküman da bunu [AÇIK] olarak işaretlemiş — doğru teşhis.
2. **"Bölgesel post-fx grade (F7 altyapısı kodda, ucuz)"** — pass gerçek ama **global**: presetler `main.c:1013`'te statik dizi, F7 sırayla geziyor. Bölgeye bağlama + geçişte lerp = yeni iş (küçük).
3. **Camşar "Ak Yel bölgesel hava olayı (§4.6)"** — hava **global**, 5 tür, yalnız mevsimsel havuz. Bölge kapsamlı hava diye bir şey yok. Bu bir kalem değil iki: 6. tür + bölgesel kapsam.
4. **"Bölgesel gündoğumu"** — var ama **harita başına** (`MapDef.sunriseOffset`), bölge başına değil. Kettlemoor'un "güneş geç doğar"ı ya kendi `MapId`'sini ister ya #6'yı.
5. **"Suluk ekipman yuvası"** — `EQUIP_SLOT_COUNT = 13` sabit; yuva eklemek `EQUIP_SLOT_POS` yerleşimini de tutar (save tarafı sorun değil, bkz. §5).
6. **"Glim Yükü sayacı"** — hiç yok. `echoesWitnessed/Consumed` (mühür ekseni) yeniden anlamlandırılabilir.
7. **§4.4'ün "bölgesel suç"u** — `crime.c` yalnız tanık→bounty/itibar üretir. "Sunnhold'da büyü idam, Velgard'da ruhsatlı" bir **bölgesel yasa tablosu** ister; Kettlemoor'un "tanık ihbar etmez, hamam sırasında sona düşersin"i de ayrı bir toplumsal sonuç katmanı.
8. **Ekoloji göçü** — `plan_migration` (`ecology.c:281`) yalnız **önceden yazılmış**, aynı `family` etiketli boş sahaya taşır. Çalışma zamanında yuva doğurmuyor.

## 4 · EN RİSKLİ 5 NOKTA + UCUZLATMA

1. **Zincir modeli (büyük, her şeyin altında).** → Ucuzlatma: zinciri **veriden** oku (`assets/chains/*.txt`, diyalog parser'ının desenini kopyala). Kod tarafı yalnız düğüm/önkoşul/bayrak makinesi; içerik derlemesiz büyür.
2. **16 bölge × harita üretimi.** → İlk turda 16 bölgeyi **açmayın**: 5 bölgelik dilim (§6). `MAP_W_MAX` 256'yı büyütmek yerine bölge = ayrı `MapId` + ayrı CSV; mevcut portal akışı zaten çalışıyor.
3. **Bölge kademesi (Kettlemoor).** → Kademeyi **tek Uint8** olarak sakla; tileset değiştirme yerine kar örtüsü/grade/spawn tablosu kademeye baksın (`weather_snow_cover` zaten var). Tileset takası v2'ye ertelenebilir.
4. **9 örgüt rütbesi.** → Tek `Sint16 rank[16]` dizisi + statik ilişki matrisi. `assassin.c`'yi genelleştirme, **yanına** çerçeveyi koy, sonra taşı.
5. **Item tavanı 92/96.** → Sabiti 192'ye çıkarmak ucuz ama `item_textures.h`'da üç paralel `R2DTexture*[MAX_ITEM_TYPES]` var; lore sayfalarını **item yapma** — `loreId` ayrı tabloda yaşasın (yüzlerce sayfa item tablosunu boğar).

## 5 · SAVE ETKİSİ — iyi haber

Format beklenenden büyümeye dost. **Sayı-önekli** bloklar (okuyucu `i < COUNT` ile budar): envanter, ekipman, hotbar, skill, **bölge (`save.c:201`/`455`)**, NPC, tarif, yuva, echo. Yani:

- **`MAX_REGIONS` 8→32, `EQUIP_SLOT_COUNT` 13→14, item/mob tavanları: sürüm bump GEREKMEZ.** Eski save `nRegion=8` yazar, yeni okuyucu `8 ≤ 32` kabul eder.
- **İki blok sayı-öneksiz:** `storyFlags` (16 sabit döngü, `save.c:252`/`528`) ve `titleStats` (12). Yeni bayrak deposunu buraya **büyütmeyin** — toleranslı kuyruğa ayrı blok olarak ekleyin; 16'lık döngü miras olarak kalsın. Bump yine gerekmez.
- **Bump gerektiren tek şey:** NPC sayısı 32'yi geçerse `save.c:464` `npcAliveFlag[32]` + `nNpc > 32` reddi. Tamponu 128'e çıkarıp sınırı gevşetmek yeter (v29 korunur, ama **eski binary yeni save'i okumaz** — sorun değil).
- Yeni durumların hepsi (Glim Yükü, bölge kademesi, örgüt rütbeleri, zincir bayrakları) **kuyruk deseniyle**, sürümsüz iner. Kanonun tamamı için **v29 korunabilir.**

## 6 · AŞAMALANDIRMA — ilk 5 saatin minimum seti

**Faz 0 (altyapı, içerik yok):** #1, #2, #3, #7, #8 kararı. Ölçüt: eski save yükleniyor, uyarı yok.

**Faz 1 — Elderdal dilimi (ilk 5 saat):** 1 bölge (Sunnvik + vadi), parça düşüşü açılışı, 4 yollu ilk dallanma, Vardmen (tek örgüt, #5'in kanıtı), Glim Yükü sayacı + tint, 6–8 NPC, Çekilenler + Vettir (mevcut iskelet/goblin), 1 zincir (3–4 düğüm). Yeni harita: **1**. Bu, kanonun §11'ini bire bir karşılar ve mevcut tavanların **hiçbirini** kırmaz.

**Faz 2 — Blackholt + Kettlemoor:** #6 (bölge kademesi) + #9. Sigrun'un güneye yürüyen cesedi kancası, göze açma döngüsü, kademe 0→2. Huzur bölgesi burada **bölge şablonunun ve durum ekseninin kanıtı** olur.

**Faz 3 — Ribhold veya Silverrun:** ilk büyük yerleşim + Soğuk Fener/Işıksızlar (çatışan üyelik gerçekten test edilir) + Ashling.

**Faz 4 — doğu (Tuzbel ağzı → Binkuy):** Ak Yel, suluk, sus taşı, çan. Doğuyu Faz 4'ten önce açmak, kuzey daha yarım dururken ikinci bir tavan dalgası demek.

**Sonra:** Rimewaste/Whitefell/Goldmaw/Stormvik/Skarnmark, Sırçaltı, Yarık.

## 7 · ASSET (kriter değil, bilgi)

Faz 1 mevcut sprite'larla döner (goblin/iskelet/insan + tint). Yeni görsel borcu sırayla: **Ashling** (at boyu ejderha — kanon bunu bilinçli ucuz tutmuş), cam/plaka tileset'i + dikey şaft (doğu, en büyük kalem), sıcak su/buhar + kademeli kar hattı (Kettlemoor, mevcut `weather_snow_cover` üzerine), suluk/çan paper-doll katmanları, Kavruk parlayan çizgi shader'ı (post-fx üstünden yapılabilir).