# HANDOVER — Pixe Game

Güncelleme: 2026-08-04 · Dil: konuşma Türkçe, kod/dosya İngilizce, yorumlar Türkçe

## Çalışma kuralları
- **KOD EN SON GELİR (2026-07-28 Hakan direktifi)**: kod faturası tasarım
  kriteri DEĞİL — kapsam maliyet için asla küçültülmez. Sıra: **lore → GDD →
  yapı → hikaye → görev → kod**. "Bunlar oturmadan kodun önemi yok." Bir
  bölümü demo gibi oynamak yine de iyi fikir.
- **BOŞA ÇIKAN KODU SİLME (2026-07-28)**: kanon değişince sahipsiz kalan
  mekanik/ağaç/bayrak silinmez — gerekçesiyle yorumlanır ya da rafa kalkar.
  Yeniden yazmak yerine **yeniden adlandır/yeniden yorumla**.
- **docs/LORE.md v2 = kanon kilidi (2026-07-30, "DEMİRBOYUN")**: çelişkide LORE kazanır.
  §15 yazım kuralları bağlayıcı (FATURA REFLEKSİ testi, ROL ÖZGÜRLÜĞÜ, isim anlaşılırlığı).
  **v1 "Sönen Kandiller" HÜKÜMSÜZ** — `docs/konsept/arsiv-sonen-kandiller-lore-v1.md`'de rafta.
- **KANON YAPISI — iki katman (bağlayıcı)**: omurga İNSAN siyasi çatışması, fantastik PARALEL
  katman. Fantastik ana zincirin hiçbir düğümünde zorunlu olamaz. Üç tur bu kuralı ihlal ettiği
  için reddedildi. Dünyayı tanıtma cümlesi: "felçli fatihin sandalyesi ve onun adına konuşan
  altı ağız" — asla "taç töreni" diye açma.
- **KANON ÜRETİM YÖNTEMİ**: 6 paralel kurgu mimarı (çapa yasağıyla — eski kanonu görmezler)
  → bağımsız klişe avcıları → tarafsız hakem → Hakan'a sunum. 4 tur, 24 aday. Süreç
  `docs/konsept/reboot/` altında; tekrar gerekirse aynı kalıp kullanılır.
- **KANON DEĞİŞİRSE DOSYALAR DA DEĞİŞİR (2026-07-31 — bugün ÜÇ KEZ ısırdı)**:
  kanon değiştiğinde ona dayanan dosyalar AYNI commit'te ya güncellenir ya
  **HÜKÜMSÜZ** damgası alır. Üç vaka: (1) `docs/konsept/kanon-taslak/` 15 dosya
  v1 LAMPWICK kadrosunu tutuyordu ve adları ("04-npc-kadrosu") hazır sanmaya çok
  müsaitti · (2) oyundaki 18 NPC + diyalogları reboot ÖNCESİ · (3) depo kökünde
  `STORY.md`/`LORE_SECRET.md` başlıklarında "kanon" ve "anayasa" yazıyordu ve
  DEMİRBOYUN'dan İKİ kanon öncesine aitti. Üçünün sebebi aynı: **kanon üç kez
  değişti, ona dayanan içerik yerinde kaldı.** Yeni kanon yazarken `grep -rl` ile
  eski terimleri ara; bulduğun her dosyayı ya düzelt ya damgala.
- **docs/GDD.md = tasarım anayasası (2026-07-24)**: her tasarım kararı önce
  GDD'ye işlenir ([ONAYLI]/[KODDA]/[TASLAK]/[AÇIK] etiketleriyle), kod onu
  izler; yol haritası GDD §9. PROGRESS üretim günlüğü, HANDOVER devir notu
  olarak kalır.
- **Asistan kodu yazar** (2026-07-09'dan beri), faz faz ilerler; her faz sonunda durur, Hakan test eder, onayla commit atılır.
- **TÜM tasarım kararları Hakan'a sorulur** (AskUserQuestion) — denge değerleri, UX, kapsam. İnisiyatif alınmaz.
- Derleme hep `-Wall -Wextra` sıfır uyarı. Her çalışan aşama sonrası commit önerilir.
- **r2d tuzağı:** `flush_to()` vertex sayacını sıfırlamaz (sıfırlama `r2d_begin_frame`'de), aralığı
  `firstVert` ile seçer. Kare içinde ikinci bir flush yapan kod, ayrım noktasındaki `vertCount`'u
  saklayıp `firstVert` olarak vermek zorunda — yoksa aynı çizimler iki kez basılır. Bu bir kez
  ısırdı (sahne-only post-fx). Doğru desen: `r2d_target_begin`'in `savedVertCount`'u.
- Felsefe "no hidden machinery": sihirli soyutlama yok; sprite batch, shader, parser — hepsi görünürde.
- Oyun proje kökünden çalıştırılır: `./build/game` (asset yolları göreli).
- Test kalıbı: `pkill -x game; (PIXE_HIDDEN=1 ./build/game &); sleep 5; kill -TERM $(pgrep -x game)` — temiz kapanış = cleanup yolu sağlıklı.
  **PIXE_HIDDEN=1 (2026-07-31 Hakan isteği)**: duman testi penceresi ekranda GÖRÜNMEZ
  (SDL_WINDOW_HIDDEN; GPU yolu normal işler). `SDL_VIDEODRIVER=dummy` KULLANMA — dummy
  sürücüde SDL_GPU cihazı açılmıyor.

## Proje
C + SDL3 (3.4.8 vendored) + SDL_ttf + stb_image + CMake. Top-down 2D pixel-art RPG.
Vizyon: OFFLINE Skyrim-vari açık dünya, KO'dan ilham combat/loot. Sınıf yok — **kimlik davranıştan doğar** (use-based skill + puanlı ağaç + itibar).

**Görsel kaynaklar (kullanım onaylı):** Oyuncu = LPC paper-doll (DOKUNMA). Diğer her şey =
Cute_Fantasy paketleri (`/Users/hakanartuc/Cute Fantasy` — NPC, ev, düşman, zindan,
çöl/volkan/mantar biyomları, UI, ikonlar) + `~/Downloads/lpc-hand-tools` + ULPC generator
çıktıları (Hakan istek üzerine indirir) + efekt paketleri (assets/fx'e kopyalandı, CREDITS'te).

## Mimari (modüller)
- **r2d** — SDL_GPU üstünde KENDİ 2D renderer'ımız (SDL_Renderer tamamen söküldü, 2026-07-09):
  MSL shader gömülü (Metal; SPIR-V/Vulkan eklenecek nokta `load_shader`), sprite batch (kare sonunda
  tek upload + tek pass), alpha+additive pipeline, NEAREST/LINEAR sampler, 9-grid, render target
  (`r2d_target_begin/end` — kendi cmdbuf'uyla anında submit; batch'e `forceNewDraw` sınırı koyar),
  kare-içi texture yıkımı ertelenir. SDL kırpma davranışı taklit edilir (taşan src → clip).
  Duman testi: `./build/r2d_test`.
- **Oyun sistemleri:** tilemap (Tiled CSV, 200×150, culling) · region (bölge+mood) · ecology (yuva/popülasyon/göç — E1-E2) · decor (evler, taban collision)
  · player (LPC katmanlı, oversize slash) · mob (leash/grup-aggro/tile çarpışma/kanama alanları) · npc (rutinli, saat bazlı)
  · dialog (txt format: `[dugum] / text: / opt: -> hedef|end|shop|accept:ID|complete:ID`) · quest (KILL) · shop (al/sat)
  · item/inventory/equipment (instance +plus/rün/stat, zırh sınıfı, 40 slot) · skill (use-based, XP=hasar, 15 skill)
  · ability+hotbar (1-8, KO auto-attack; aktifler ağaçtan açılır) · skilltree (16 puanlı düğüm ağacı,
  PerkEffects tek nokta — eski perk.c Faz 21'de SÖKÜLDÜ) · combat (Combat A + combat_spell_hit)
  · gametime (10dk=24s, mood müziği) · audio (SFX) + music (4 prosedürel parça, mood crossfade)
  · minimap (fog, target pass) · save (v21, alan-alan binary; skills/masks blokları append-toleranslı)
  · craft (istasyonlar/tarifler/sunak) · resource (toplama düğümleri) · region (bölgesel bounty/itibar)
  · contract (pano: 9 kontrat, 3 kademe, günlük rotasyon) · light (gece ışık kaynakları + kamp ateşi)
  · ui_theme (9-grid paneller) · font (ttf→GPU upload) · tooltip · ambience · journal.
  · **jsonx** (cJSON sarmalayıcı: JSONC yorum temizliği + satır/kolon hatası + varsayılanlı getter)
  · **uilayout** (HUD yerleşimi veriden: çapa/hiza/dikey/boyut — `assets/ui/layout.json`)
  · **uieditor** (oyun içi yerleşim düzenleyicisi, Ctrl+U).
- **main.c** — tüm bağlantı/orkestrasyon burada (~5000 satır; ileride bölünebilir).

## Tuşlar
WASD hareket · Space auto-attack toggle · TAB hedef (Z KALDIRILDI) · 1-8 hotbar
(WoW düzeni 32 yuva: 2 ana bar 8×1 alt alta [1-8 / Shift+1-8] + sağ dikey 2×4
[Alt+1-8] + sol dikey 2×4 tuşsuz; hepsi hep görünür) · SHIFT aktif blok (kalkanla) ·
P yankesicilik (eğilmişken) · R dinlen (ateş başı) / kuyuda İPTEN İN (loca) · E konuş/istasyon/
topla/kuyu · I envanter (TAB Eşya↔Malzeme) · K skill/ağaç (alınmış aktif düğümü
hotbara SÜRÜKLE) · J Yolculuk günlüğü (Tab: Görevler/Günlük/Ünvanlar/Kontrat, ←→ yaprak) · F8 hava döngüsü (dev)
Hotbar: düz TIK = becerine bas (click-to-cast); SHIFT+sürükle = taşı/takas;
kaldırma YALNIZ sürüklerken beliren ✕ hedefine (dışarı=iptal — yanlış atma yok);
silaha uymayan beceri SOLUK çizilir (kullanım engeli hotbarPressed akışında)
· M minimap · C eğilme · **Ctrl+U YERLEŞİM DÜZENLEYİCİSİ** (sürükle/oklar 1px, Shift 10px;
G ızgara 8/16/32, Alt yapıştırmayı kapat; Tab sonraki, A çapa, **E en uygun çapa**, H hiza;
Ctrl+oklar & köşe tutamağı boyut; **P ön izleme** — seçili panel kapalı olsa bile gerçek
içeriğiyle çizilir; Ctrl+S kaydet, R diskten geri, Esc kapat. Açıkken oyuncu girdisi DURUR)
· **Ctrl+B su katmanı izolasyonu** (14 durum: hepsi + 13 katman tek tek; Ctrl+Shift+B geri)
· Ctrl+V su asset akışı/şerit demosu döngüsü (4 durum — demo rafta, tuş duruyor)
· **F7 post-fx filtre döngüsü** (20 tarz + kapalı: SICAK/DARK FANTASY/CRT/VHS/NOIR/GAMEBOY... splash'ta ad; SICAK açılışta aktif — kalıcı tarz seçilmedi) · **F10 DEBUG +1M XP + malzeme/büyü seti + her ağaca 99 puan (yayında kaldır!)** · F5/F9 AKTİF slota kaydet/yükle (slot yoksa — otomatik kayıttan yüklendiyse — F5 kayıt ekranını açar, seçilen slot aktif olur) · ESC katmanlı kapatma/menü
(K = skill listesi + ağaç paneli; ok mühimmatı sağ tıkla seçilir)
(F efekt önizleme KALDIRILDI 2026-07-10 — rünün instance efektini eziyordu)
AÇILIŞ AKIŞI (Faz B1): başlık ekranı → ana menü (Yeni Oyun/Yükle/Çıkış) → 20+1 slot;
vinyet açıkken tuşlar KİLİTLİ (yalnız 1-8 seçim, E/Space hız, ESC menü).

## Kritik bilinenler

### RENDER / VFX (2026-08-03) — bilinmesi gerekenler
- **Shader kaynak dili HLSL** (`shaders/*.hlsl`), çıktılar `assets/shaders/{spirv,msl}/`
  içinde ve **repoda**. `shaders/compile.sh` offline derliyor (glslangValidator +
  spirv-cross). Yeni shader yazan **MSL yazmayacak.**
- **`select()` KULLANMA.** HLSL 2021'in argüman sırası MSL'in tersi ve derleme hatası
  vermiyor — sessizce yanlış sonuç veriyor. Açık `?:` yaz. `light.frag.hlsl`'de gerekçe
  yorumu duruyor.
- **Windows bugün Vulkan/SPIR-V ile çiziyor.** DXIL yok çünkü DXC'nin macOS build'i yok;
  bir Windows makinesinde üretilip `assets/shaders/dxil/` altına konunca D3D12
  kendiliğinden tercih edilir, **kod değişmez**. DXC kullanılırken `dxil.dll`
  `dxc.exe`'nin YANINDA olmalı — yoksa "Shader is corrupt".
- **`src/r2d_test.c` doğrulama harness'ı** ve dört pipeline'ı da sürüyor. Windows'ta ilk
  koşacak şey bu.
- **İKİ KIRILGANLIK, HENÜZ DÜZELTİLMEDİ** (VFX'e engel): `decal_render` ve `shadow.c`
  hiç `r2d_set_blend` çağırmıyor, o anki modu miras alıyor — **ateş efekti eklemek yanık
  lekesini sessizce additive yapar.** Ve `fx_render` her parçacığı ALPHA çiziyor, yani
  kıvılcım/alev **bloom eşiğini geçemiyor**; additive olmadan alev sıcak hissetmez.
- **ASSET YOLU HARFİ:** git dizini `assets/Tiles` diye **büyük T** ile saklıyor. macOS ve
  Windows harf duyarsız, **Linux/Steam Deck duyarlı.** Yeni yol yazarken `git ls-files`
  ile teyit et; yanlış harf burada sessizce çalışır, orada patlar. (Bu tuzağa bu turda
  bir kez düşüldü ve düzeltildi.)
- **`docs/konsept/vfx-teknik-brief.md`** VFX'in tasarım kaynağı: teknikler, iki örnek
  vaka, kanona bağlı efekt listesi, uygulama sırası (0 taşınabilirlik → 1 shader'sız
  kalemler → 2 altyapı → 3 eşikli alev).

### KADRO FAZI (2026-08-03) — bilinmesi gerekenler
- **Yenitaç kadrosu 23 slot, 8 doküman** (`docs/konsept/kadro-*.md`). Kod YAZILMADI;
  tek kod değişikliği gün uzunluğu. Diyalog dosyaları ve `NPCS[]` göçü **sırada**.
- **`GAMETIME_DAY_SECONDS` 1200 → 3600** (Hakan kararı, GDD §1.2g). Sebep çizelge
  değil **okunabilirlik**: 20 dakikalık günde oyuncu bir rutin öğrenemiyor ve
  kadronun tamamı rutinlerin öğrenilebilir olmasına dayanıyor. Yeni rutin/AmbientHook
  penceresi yazan bu ölçeği varsayacak.
- **`dunya_*` öneki ölüyor.** 12 id `yenitac_*` ve `cayir_*` olacak; etkilenen çağrı
  yeri **10** (main.c'de 8, craft.c'de 2). Save'e girmiyor, tek commit. Sayım üç kez
  yanlış yapıldı (14 → 8 → gerçek 10); yeniden sayan grep'lesin.
- **Salon ve mahzen AYRI HARİTA** (`MAP_SALON`, `MAP_MAHZEN`, enum SONUNA — `save.c`
  harita indeksini `MAP_COUNT`'a karşı sınırlıyor, sona ekleme format-güvenli).
  **Bu kararın bir öncesi İPTAL edildi** ve iptal gerekçesi kalıcı bir ders: o karar
  `NpcDef.map`'in tek int olmasından, yani bir **kod faturasından** türetilmişti.
- **[YENİ ALAN GEREKİYOR] `NpcStop.map`** (varsayılan 0, geriye uyumlu) + `npc.c`'de
  beş noktanın ona bağlanması (`NPC_ON_MAP` makrosu 278'de, kullanım 305/416/497/520,
  artı 350'deki elle yazılmış `map == 0`). Durak haritası değişince NPC **yürümez,
  ışınlanır**. **Altı slot buna bağlı** ve tek alan altısını birden açıyor.
- **AmbientHook en yüksek öncelikli eksik.** Yedi slot buna dayanıyor; o olmadan pano
  yeniden görev kaynağına döner ve 3. dakikanın "sana değil dünyaya söylenen ilk
  cümle" vuruşu boşa çıkar.
- **KANON DENETİMİ İKİ TUR ÇALIŞTI** ve ikinci tur revizyonun KENDİ ürettiği beş yeni
  çelişki buldu. Kalan borç `docs/konsept/kadro-kanon-raporu-2.md`'nin sonundaki
  **GDD/LORE BORCU** listesinde (15 madde) — kapatılmadan diyalog yazılmamalı.
  Kapanmayanların en önemlileri: KR3 (Şirin'in dördüncü ertelemesi kimin), KR5 (slot
  21 hâlâ heceliyor, kart okuma bilmiyor), KR6 (mareşalin defterini Ragıp ezberlemiş),
  KR7 (altı ismin taşı meydanda mı salonda mı), KR9 (sikke hem 100 kese hem madeni
  para), Ö-A (Sabri'nin dört durağının hiçbiri `indoors` değil), Ö-B (Sarp yürüyor).
- **Ajanlar üç önerimi reddetti ve üçünde de haklıydılar.** Bu bir desen: ajan
  gerekçeli reddederse gerekçeyi oku, dayatma.
- **YERLEŞİM TUZAĞI (2026-07-31)**: bir konum formülü çizim VE tıklama testini besliyorsa
  TEK yerde durmak zorunda. Hotbar'da üç (`slot_rect` + kaldırma hedefi + main.c ✕),
  envanterde beş kopya vardı; panelleri veriden taşınabilir yapmak onları sessiz sapma
  noktasına çevirdi — panel taşınır, tıklama eski yerde kalır, derleyici SUSAR. Desen:
  modül içinde tek `*_rect()` / `*_origin()`, çizim ve hit-test ikisi de oradan.
  Çıkan gerçek hata: main.c envanter yuva tıklaması `panelY` yerine sabit `20` kullanıyordu.
- **YERLEŞİM: olayı yutmak YETMEZ (2026-07-31)**: hareket/saldırı `SDL_GetKeyboardState`
  ile anlık tuş durumundan okunuyor, olay döngüsünü BAYPAS ediyor. Düzenleyici açıkken
  oyuncu girdisi `!uieditor_active()` ile kapatılıyor — yoksa 'A' (çapa) karakteri sola
  yürütür, 'S' (kaydet) aşağı indirirdi. Yeni bir modal araç eklerken aynı kapı gerekir.
- **YANLIŞ ÇAPANIN BELİRTİSİ GÖRÜNTÜDE YOK**: çapa değiştirince öğe yerinde kalır (ofset
  yeni çapaya göre yeniden hesaplanır), hata ancak pencere boyutlanınca çıkar. Bu bir kez
  ısırdı (saat `sol-orta` + x=1580 kaydedildi → 1280 genişlikte ekran dışı). Koruma:
  düzenleyicide `E` (en uygun çapa) + ofset pencereye göre büyükse kırmızı uyarı.
- **Kanama/perk VFX'leri olay bazlı**; F10 sonrası moblar tek vuruşta ölürse kanama görülmez (Orc'ta dene).
- Partiküller mob/NPC'lerin ÜSTÜNE çizilir (altında kalıp kayboluyordu).
- CuteFantasy-5x9.ttf glyph seti eksik (boşluk yok) — font PublicPixel'de kaldı.
- Save dünya durumunu (mob/loot) içermez; harita değişirse eski save konumu tuhaf düşebilir.
- CREDITS.md'de lisans TODO'ları var (ULPC parça listesi + Cute_Fantasy ticari lisans teyidi) — mobil yayın öncesi şart.
- **MAPGEN + DUAL-GRID AUTOTILE (2026-07-23)**: haritalar scripts/mapgen.py ile
  üretilir (noise/drunk-path/CA/min2x2/fill_pockets/poisson + PNG önizleme —
  asistan üret→BAK→düzelt döngüsü uygular). Çöl kaya(16)/vaha(17) görseli
  tilemap.c dsr_dual_pass; loca mağara duvarı(19) + mezarlık kayası(13)
  cave_dual_pass (parametreli: LUT + wallId; ring yalnız zemine bakan sınırda,
  güney sıra kaya yığını yüzü). Atlaslar: desert/{water,cliff}_atlas.png +
  Tiles/Cave/Cave_Walls_Keyed.png (bake_cave_ring.py türetir — koyu blok
  arka planı şeffaflanır). Üreteçler: gen_dogu/gen_loca/gen_mezarlik.py —
  sabit noktalar (portal/NPC/dekor/kamp) script içinde garanti edilir; harita
  değiştirirken ÖNCE oradaki sabit listesine bak. Yeni eşleme eklerken önce
  scratchpad mock'uyla görsel doğrula. CSV kuralı: değer = tile id + 1.
- **MEVSİMLER (DOGU-P2, 2026-07-24)**: 10 gün/mevsim, günden türetilir (save
  değişmedi). Güneş sınırları mevsimsel (yaz 05-21, kış 08-17) + harita
  gündoğumu offset'i (MapDef.sunriseOffset — Taşdurak -0.75 saat; geçişte
  gametime_set_sun_offset). Etkiler: gündüz ambient mevsim tonu (main ışık
  bloğu), şafak hava havuzu mevsimsel (weather.c roll_pool — kış %38 kar),
  fiyat çarpanı item_season_trade_mult (kış yiyecek+odun ×1.35, sonbahar
  yiyecek ×0.80 — main alım/satım + shop paneli), ekoloji üremesi
  (ilkbahar +1, kış hayvan gün aşırı — ecology_dawn).
- **DEV kancaları (main.c)**: `PIXE_AUTOLOAD=auto|<slotNo>` menüsüz kayıt
  yükler; ek `PIXE_AUTOPLAY=1` dolaş+saldır+6sn'de bir portal geçişi (stres).
  CMake `-DASAN=ON` yalnız game hedefini ASan'lar (build-asan/). Çıkış ses
  crash'i (SDL_QuitAudio) 2. vakası bu araçlarla kovalandı — bizim taşma
  bulunamadı, SDL 3.4.8 kapanış yarışı şüphesi; quit fonksiyonları artık
  stream'leri önce SDL_PauseAudioStreamDevice ile susturuyor.
- Skill sistemi = PUANLI DÜĞÜM AĞACI (Faz 21, skilltree.c): 16 ağaç (7 savaş/sosyal +
  6 meslek mini + Destruction + Staff + Kader). 4 seviye = 1 puan (Kader: 10 TOPLAM seviye = 1).
  Aktif beceriler (PS/QS + 5 yay + 5 büyü) ağaçtan SATIN ALINIR (mavi AKTİF düğüm) —
  hotbar oradan dolar; Ateş Topu HER ZAMAN açık (büyünün "normal vuruşu").
  Keystone'lar bedelli. Respec sunakta (harcanan×50g). Test F10 ile.
- BÜYÜ (Faz 22): silahtan bağımsız cast (0.45sn köklenme, hareket bozar, mana commit'te);
  hasar combat_spell_hit (INT + Destruction + silah spellPower + PEMBE rün). Asa Space'i
  Ateş Topu atar (mana yoksa melee dürtme). Zırh ağırlığı: aşım = hız + mana rejeni cezası.
- SAVE v18 + SLOTLAR (Faz B1): `saves/slot_NN.dat` (20) + `saves/auto.dat` (şafak +
  harita geçişi); dosya başında ÖZET BLOĞU (save_read_summary — slot kartları tam
  yükleme yapmadan dolar). Eski `save.dat` yolu ARTIK KULLANILMIYOR. Yeni oyun reset'i
  playerTemplate'ten; başlangıç = gezgin çıkını (Uzun Kılıç + deri set kuşanılı +
  3 iksir/2 et/meşale/50g). Kişilik sayaçları persKaba/Kibar/Ketum gizli (dialog
  `fx: kaba += 1` biçimiyle işler, req ile okunur).

## Sıradaki iş
**FAZ SU-3 TAMAM (2026-08-04, su fazı KAPANDI)**: oyuncu yansıması (anlık
kare + ekipman; oversize bayrağı ÖZELLİKTİR durum değil — dersi player.c'de),
gövde başına garantili balık, su kenarı fenerleri (light.c + decor.c aynı
koordinat), gece lamba sütunu (LB_LAMP; ambient telafisi TAVANLI ≤0.45 —
3. doyma vakası), yağmur halkaları (weather→su), r2d_test su kapsaması.
Ctrl+B 14 durum. KALAN su borcu: kıyı karo sanatı (Yol A) + NPC yansıması +
vaha/loca deresi görsel doğrulama; Kışyolu buzu ve Tuzkapı limanı kendi
fazlarında.

**FAZ SU-2 TAMAM (2026-08-04)**: dekor yansımaları (r2d_draw_texture_reflect
+ decor_render_reflections, karo karo ıslak kırpma) · balık gölgeleri ·
halka dalgacık (LB_RING, Ctrl+B 13 durum) · nilüferler (su üstü örtüsü —
tilemap_render_water_overlay, su geçişinden SONRA) · göl kuzey kıyısına
ağaçlar (kural: yansıma güneye uzanır → ağaç suyun KUZEYİNDE). Oyuncu/NPC
yansıması anim kare tesisatı bekliyor. Sıradaki yön (Hakan): ateş paketi.

**FAZ SU TAMAM (2026-08-04, save DEĞİŞMEDİ)**: tek geçişli shader su sistemi
(`shaders/water.frag.hlsl` + `r2d_water_flush`). 11 katman, maske RGBA
(derinlik/kıyı/çırpıntı/kuzey — karo başına 2×2 teksel), mizaç kanalıyla
gölet↔deniz tek shader'dan. Köy güneyine gölet (karo [20] sualtı taban).
Ctrl+B katman izolasyonu (12 durum). Kararlar GDD §1.2h; ayrıntı PROGRESS
"Faz SU". **Beş kritik ders koda yorum olarak gömülü:** load_shader sırası
(uniform,sampler) · CSV GID=id+1 · frac(sin()) üç arka uçta farklı → tamsayı
hash · Bayer düz platoda her yerde dama üretir → yapısal geçişler sürekli
değerden · her katman kendi sihirli sayısını kurarsa biri sessizce ölür
(SLOPE_NORM tek kaynak). **Rafta:** duvar prototipi (u.l.w), şerit demosu,
shader köpüğü (foam=0). **SU 2. tur adayları:** sprite yansımaları · balık
gölgesi + halka dalgacık · nilüfer · kıyı karo sanatı · gece lamba yansıması.
**VFX altyapı borcu duruyor:** decal/shadow blend set etmiyor + parçacıklar
ADD kullanamıyor (alev bloom eşiğini geçemez) — ateş efektlerinin ön şartı.

**FAZ UI-LAYOUT TAMAM (2026-07-31, save DEĞİŞMEDİ)**: HUD konumları koddan veriye
(`assets/ui/layout.json`, 23 öğe) + oyun içi düzenleyici (Ctrl+U). Kararlar GDD §7.1'de.
Yeni modüller: jsonx (cJSON + JSONC + satır/kolon hatası), uilayout (çapa/hiza/dikey/boyut),
uieditor (ızgara, hizalama kılavuzu, boyut tutamağı, ön izleme).
Ayrıca: çözünme ölümü (mob.c banded dissolve, 14 şerit) bu commit'te; PIXE_HIDDEN dev kancası.

**FAZ KİMLİK TAMAM (2026-07-31, save DEĞİŞMEDİ)**: içerik tablolarında `.id`
(18 NPC `<harita>_<yer>_<rol>`, 41 tarif `<istasyon>_<slug>`), 13 ad karşılaştırması
+ 2 tarif araması id'ye çevrildi, `StationDef.ownerName` → `ownerNpcId`.
Diyalog tavanı 220→512 / 80→128 + sesli & UTF-8 güvenli kesme (26 replik + 1 seçenek
sessizce kesiliyordu). Kararlar GDD §8.1'de.
**KANON YENİDEN ADLANDIRMASI ARTIK GÜVENLİ** — adlar `.name`'de, kod `.id` kullanıyor.

**FAZ KANON-KAPANIŞ TAMAM (2026-07-31)**: LORE §16'nın yedi açık sorusu kapandı.
§16 artık "KARDUVAR VERASET TAŞI"; açık sorular yeni **§17**'de (dört madde, hepsi
Kavurgan tarafı + "hoca kim"). Ayrıntı ve gerekçeler PROGRESS "Faz KANON-KAPANIŞ".
**TERİM DEĞİŞİKLİĞİ — eski adları kullanma:** "Dinleyici" → **Bağlı** (Skyrim'in
"the Listener"ı ile birebir çakışıyordu), "İşaret Okuyan" → **Yedinci Ağız** ("Okuyan"
zaten Mühürcü rütbesi), taç töreninin adı **Altı Ad**. İngilizce karşılıklar da kararlı:
the Bound / Seventh Mouth / the Six Names.
**S4 KARARI HARİTA FAZI DOĞURDU:** yedi mühür KITAYA YAYILMIŞ seçildi → Yedisunak,
Karaocak derinliği, Tuzkapı körfezi altı, volkan, Karduvar donmuş gölü, Kozmik Tapınak
= **6 yeni harita + kar/volkan/sualtı tileset'leri.** Kendi yol haritası fazını hak ediyor.
**BAĞLAYICI KORKULUK (LORE §9):** mühür çürümesi hiçbir siyasi zincir düğümünün
önkoşulu OLAMAZ — yalnız fiyat/yol/tehdit değiştirir. Kanon editörü bu kuralın kendi
bölümünde iki kez çiğnendiğini buldu (kutsama hattı, Karduvar kapısı); ikisi düzeltildi.
Yeni zincir yazarken bu kural test edilebilir: "şu mühür çürüdüyse" koşulu koyan siyasi
görev kuralı ihlal etmiş olur.

**FAZ SUÇ-DEFTERİ TAMAM (2026-07-31, save FORMATI DEĞİŞMEDİ)**: `MapDef.crimeRegion`.
Bölgesiz haritalarda (zindan/konak/mezarlık/**loca**) işlenen suç Köy'ün defterine
yazılıyordu — locada Neriman'a vurunca köy muhtarı bounty istiyordu. Dört yeni bölge,
`MAX_REGIONS` 8→16 (save.c sayacı yazdığı için toleranslı), bölge indeksleri adlandırıldı.
Kararlar GDD §8.2. Muhafız sistemi ÇOĞULLANMADI — kanon kadrosunu bekliyor (guard.h notu).

**ANLATIM STANDARDI GENİŞLEDİ (2026-07-31, GDD §1.2c — yedi ONAYLI kural)**:
Hakan'ın direktifi *"oyuncu görev yapıyorum değil hikayenin parçasıyım diye
hissetmeli"*. **MİMARİ DEĞİŞMEDİ** — ESO/WoW zinciri + ESO sunumu yerinde; görev
sistemini "dosya/talep" modeliyle değiştirme önerisi REDDEDİLDİ. Bunlar zincirin
İÇİNE giren kalite kuralları. Kısaca:
1. **TANIK KURALI** (bağlayıcı, test edilebilir): ilerlemeyi ondan İSTEMEMİŞ en az
   bir NPC fark etmiş olmalı ve ilk cümlesi değişmeli. Değişen cümle TEŞEKKÜR OLMAZ.
2. **Sayaç** hedefse görünür, yöntemse gizli. "Kaç öldür" ASLA hedef değil. HUD
   sayacı olmayan iş ilerlemeyi DÜNYADA okunabilir bırakmak zorunda.
3. **Kabul adımı** zincirde var (söz), PANODA YOK — `contract_on_kill`'deki
   "kabul edilmiş mi" kontrolü kalkacak, her öldürme sayılacak.
4. **PHASING:** her zincir bir iz bırakır (`storyFlag → decorPatch` + kalıcı
   fiyat/olay/yol). Harita aynı görünüyorsa zincir kapatılmış, bitmemiş.
   ESO bunu YAPAMIYOR (paylaşımlı dünya), WoW hileyle yapıyor, biz offline
   olduğumuz için bedavaya yapıyoruz — **kendi tarzımızın ana adayı bu**.
5. **Duyum kanalı:** `AmbientHook` eklenecek; ünlem işareti EN ZAYIF kanal olur,
   pano ilan değil DOĞRULAMA olur.
6. Her işin ikinci bir DOĞRU tarafı var ve **konuşabilir** →
   **YENİ MEKANİK ONAYLI: `mob.isTalkable` + `dialogFile` + hostile→neutral.**
7. Kes/getir silinmez, HEDEF olmaktan çıkar (sayı direncin ölçüsü).
Ayrıca GDD §1.2c'de "BU OYUNA UYMAZ" listesi: sesli diyalog, dallanan sinematik,
anlatıcı prologu, **ekranda ahlaki seçim kutusu** (sayaçlar gizli oldukları için
çalışıyor), "SÜRE DOLDU" kutusu, harita ikonu çorbası.

**SIRADA — İÇERİK ÜRETİMİ (kod değil, LORE v2 üstüne):**
DEMİRBOYUN kanonu KİLİTLİ ama **hikaye/zincir/NPC kadrosu/diyalog HENÜZ YOK.**
`docs/konsept/kanon-taslak/` (04-npc-kadrosu, 03-hikaye-isleyisi, 07-anlati-tasarimcisi,
08-15 meslek/sınıf hatları) **v1 LAMPWICK üstüne** kurulu — Hald Ironkiln/Coldhearth/
Emberlaw isimleri geçiyor, yani HÜKÜMSÜZ kanona ait. Yeniden üretilmeleri gerekiyor.
Oyundaki mevcut içerik (K9 Kandil, K2 Son Müşteri, S3 Sayan Adam, Suikastçi zinciri,
18 NPC) da reboot ÖNCESİ — kanona göre revize edilecek.
Önce LORE §16'daki 7 açık soru kapanmalı (§15.6: "Kabukçular'ın gerçekten haklı olduğu
yer henüz yazılmadı ve YAZILMALI").

**MOB EKOLOJİSİ TAMAM (E1+E2, save v20-21)**: KO slot spawn'ı dünyadan söküldü
(ecology.c) — yuva/popülasyon (şafakta +1), aileye göre yıkım (Reis/Totem/gölcük/
mezar/sürü), göç kolonileri + panoda söylentiler, RUTİNLER (goblin gececi uyuklar
"z" + uyuyana sinsi vuruş; ork nöbetli devriye; hayvan gece uyur + öğlen su
başına iner), dinamik kontratlar. Zindan = mühür istisnası.
Öncesi B1+B2 (save v18-19): ana menü + 20+1 slot + Sessiz Gün açılışı +
Yolculuk günlüğü (J, iki sekme, birinci ağız).
ROADMAP ONAYLI (2026-07-14, PROGRESS "ROADMAP" bölümü): YAKIN cila → hikaye
kartları D1 + sahne araçları → karakter yaratma-lite; ORTA ikinci yerleşim+biyom →
4 onaylı mekanik → Dünya Olayları; UZAK Perde 2 → kalan sistemler → yayın.
**CİLA PAKETİ TAMAM (2026-07-14)**: tooltip (kuşanılıyla karşılaştırmalı) +
drag&drop (sol tık; sağ tık yolları duruyor) + beceri ikonları (hotbar+ağaç) +
3 yeni müzik (zindan/gece/menü) + ambiyans katmanı + SFX perde varyasyonu +
ses ayarları (settings.cfg) + MÜHÜR DALGASI (zindan respawn'ı: 120sn'de toplu
diriliş, uyarılı; boss haftalık kuralda — spawn_zones_update söküldü).
**D1a TAMAM (2026-07-14, save v22)**: sahne araçları (bark/cine/NPC yürütme/vinyet
zarı) + K9 Kandil (Fatma + Kerem yankısı F2 + üç kalıcı karar: tapınak/söner/yanar;
storyFlags[16]) + konuşulan NPC durur (npc_set_talking).
**D1b TAMAM (2026-07-14)**: K2 Son Müşteri — han (köy GD) + Nazife + gece sofrası;
araba yankısı hattı kalıcı (SF_K2_CART); Tanık = süt kokusu kanıtıyla üç ton,
Sömüren = kanıtsız (ilk somut bedel, sofra sonsuza dek kurulur).
**D1b-ek (2026-07-16)**: k2:1'e son sofra vinyeti (2 panel + fısıltı susması),
han → gerçek Inn_Red sprite'ı, gen_k2.py yol fixi (paneller script'te).
**PİVOT (2026-07-14)**: hat MEKANİK-ÖNCELİKLİ (hikaye = mekanik test aracı).
**M1 TAMAM**: element sinerjileri (Termal Şok ×1.6 / Zehir Patlaması +çevre %50 /
Alev Rünü %20 yakma basar — hibritler sinerjide).
**M2+M3 TAMAM (save v23)**: pişirme (3 buff yemeği, Tok, çiğ et riski) +
davranış ünvanları (10 ünvan, hepsi pasif + 1 taşınan ×2, kitapta 3. sekme,
sayaç kancaları her sistemde). Savaş+yaşam üçlüsü (M1-M3) kapandı.
**M4 TAMAM**: Conjuration — Öz Yılanı/Kabuğu, rezerv barda mor bölme,
Sürü Efendisi/Kurban Bağı. ONAYLI 4 MEKANİK KAPANDI (M1-M4).
**D1c TAMAM — DALGA 1 KAPANDI (K9+K2+S3)**: Nuh+Zehra, 3 turlu sayı bulmacası
(hint diyalog değişkeni + gün kilidi), Eşikçi Konağı (MAP_KONAK), kardeş
yankısı (Sömüren'de fısıltı sinyali), kayıt odası vinyeti (devral = Kayıt Tutan
ünvanı / kapat / Speech-30 zarlı üçüncü yol).
**KARAKTER YARATMA-LİTE TAMAM (save v24)**: "Kim olacaksın?" ekranı — isim
(text input; slot kartı başlığı), 3 ten tonu (LPC tint), 4 geçmiş (bedelli:
Gezgin/Asker/Avcı/Çırak). **ROADMAP YAKIN KADEMESİ KAPANDI.**
**DÜNYA OLAYLARI ÇEKİRDEĞİ TAMAM (save v25)**: her şafak %35 zar → 4 olay
(Goblin Baskını gece 2-3 raider goblin + itibar / Gezgin Kervan 2 gün, Kervancı
Selim + CARAVAN_STOCK %15 pahalı / Bereket sürülere +2 doğum / Mühür Sancısı
gece yankı ×1.8 + fısıltı). Duyuru: köylü bark + pano söylentisi. Dinlenme [R]
gece ateş başında → 07:00 (rollover doğal), HP/STA tam + mana %50, %25 rüya
vinyeti (3 havuz, gen_dreams.py). Harita genişlemeleri (ikinci yerleşim/biyom)
kullanıcı kararıyla "gerçek harita tasarımı gelince" ertelendi.
**HAVA SİSTEMİ + DÜNYA OLAYLARI KALANLAR TAMAM (save v26)**: weather.c (şafak
zarı Açık/Yağmur/Sis/Fırtına %55/25/15/5 + öğlen %25 dönüş; intensity yumuşak);
savaş: ıslakta yanma ×0.5, ateş -%15, yıldırım +%20, siste algı ×0.6, fırtına
yıldırımı (60px: 25/20 hasar); çevre: NPC sığınması, ot ×2 (dün yağdıysa),
yağmurda kamp ateşi kurulamaz, meşale -%20, mob yağmur davranışı; görsel/ses:
damla+sis+şimşek+amb_rain/thunder (gen_weather_audio.py). WE_GRAVE Huzursuz
Mezarlık (gece 3 iskelet, şafakta 2+ sağsa mezar yuva olur); MOB_SEARCH (leash
sonrası 8sn bakınma); haber ağı diyalogda (event/hava değişkenleri, gizlenen
seçenekler; Kemal/Osman/Baran satırları).
**BÜYÜ DALGASI 2 PARÇA 1 TAMAM (save v27)**: tomar ekonomisi (sağ tık oku →
spellsLearned bit maskesi; Yıkım seviye şartı; spells_apply_learned köprüsü
HER tree_compute_effects sonrası şart) + 3 büyü: Yıldırım Pençesi (koni,
ıslakta +%20), Buz Dikenleri (hedef alan, karda +%20 & yavaş ×1.5), Hortum
(süzülen alan, fırtınada güçlü). Kaynak: kervan (Pençe+Diken) + zindan USTA
sandık %25 (Hortum). F10 5 tomar basar.
**PARÇA 2 TAMAM — BÜYÜ DALGASI 2 KAPANDI**: Yılan Isırığı (zehir DoT kanala
eklenir; Zehir Patlaması salt-büyü) + Ateş Aslanı (hat dalgası, tek-vuruş
takibi, yanma basar; YALNIZ boss leşi %35, Yıkım 50). Kervan tomarları
RASTGELE havuz ({Pençe,Isırık,Diken}'den 2 farklı; startDay tohumlu).
Yeni item'lar enum SONUNA (v28 indeks güvenliği).
**ÇANTA SEKMELERİ TAMAM (save v28)**: Eşya + Malzeme çantası — I aç/kapa,
TAB sekme değiştirir, sekme şeridi tıklanır, sürükleyip diğer sekme başlığına
bırak = taşı, şeridin sağındaki ÇÖP'e bırak = imha. Craft paneli tekerlekle
kayar (CRAFT_MAX_VIS 16); pencere 1600×900. material bayrağı (13 hammadde) otomatik Malzeme'ye
düşer; bag_* katmanı (~70 çağrı) iki çantadan sayar/tüketir (craft öncelik);
craft/kontrat panelleri iki çantayı görür. Kademeler: Eşya 30→50 (Deri Çanta
S15 / Gezgin S40), Malzeme 25→55 (Kese S10 / Usta S35) — üretilince OTOMATİK
takılır, İp yeni ara malzeme (2 Ayotu, kamp ateşi). Player.bagUpgrades +
craftBag save v28; Inventory.cap dinamik; MAX_RECIPES 48. DİKKAT: yeni item
eklerken MAX_ITEM_TYPES (96) tavanına bak — 64 taşması crash yaratmıştı.
**HOTBAR WoW DÜZENİ + TAB HEDEFLEME**: 32 yuva hep görünür (2 ana bar 8×1 +
2 yan dikey 2×4; 1-8/Shift/Alt; sol bar tuşsuz); düz tık=bas, SHIFT+sürükle=
taşı; silaha uymayan beceri soluk; TAB mob hedefler (panel önceliği: envanter/
günlük sekmeleri önce; Z kaldırıldı); K panelinden alınmış aktif düğüm bara
sürüklenir + ✕ hedefi (dışarı=iptal). Save toleranslı (8/30→32). DECOR_TEX_CAP 64
+ portalTex 16 (tavan crash'leri — paralel-dizi dersi hafızada).
**VFX HİS CİLASI TAMAM (onaylı sıra ③)**: AnimFx grid-sheet havuzu (fx.c,
48'lik — tek-seferlik efektlerin ortak altyapısı); strike_fx_mob köprüsü
(melee weaponhit / büyü magickahit + her isabette kan, 17 hasar noktası;
DoT/yansıtma hariç); mob_die'da büyük kan + hit-stop (melee 50ms / ok 30ms /
ölüm 90ms / PERFECT parry 120ms — dünya dt ×0.08, menü gerçek zaman); blok
kıvılcımı, cast halkası, yöne göre slash yayı. Assetler assets/fx/ +
CREDITS.md. Sırada: ④ müzik seti (xDeviruchi).
**TAŞDURAK TAMAM (Faz DOGU — Kıta planı ilk adım)**: MAP_DOGU 100×70 çöl
(kum/kaya/vaha tile 15-17); portal dünya T(197,19)↔T(4,35); bölge 6 ayrı
bounty/itibar; NPC Ferah (bölgesel dükkan)/Duman (söylenti)/Kâtip Rüstem
(Sultanlık ilk yüzü); ÇİFT YÖNLÜ ticaret (köy malı çölde ×1.6, hurma/baharat
köyde ×1.5); moblar Mumya/Akrep/Çöl Haydudu (6 kamp). TAVANLAR: MOB 32
(26 dolu), NPC 24 (19 dolu). Gündoğumu ofseti/mevsim → Doğu P2.
**SUİKASTÇİ P3 TAMAM — LONCA KONTRAT/RÜTBE**: assassin.c/h; loca kontrat
tahtası (T28,16, [E], SF≥13) — 5 K1 kontratı (mevcut moblar), üstlen→
tamamla→dön→ödül; rütbe Çırak→Usta El (eşik 0/3/7/13/20); İğne loca dükkanı
rütbe kapılı (İkiz Bıçak sol-el +%15 hız = çift hançer build'i); aklama
oranı rütbeyle iyileşir. Player assassinPoints/contractActive/Progress
(save kuyruk). Kızıl Kanca+haber ağı P4'e ertelendi.
**SUİKASTÇİ P2 TAMAM — SINIF AÇIK**: zincir 3-5 (Dilsiz Şişe simya tarifi →
sahte ölüm kararı [kişiliğe işler] → İlk İmza: Kaçak Tahsildar gece kervan
yolu, MOB 21-22 Soldier — 23/24 tavan DOLU!); SF≥13: SUIKAST skill'i +
5 beceri (Sırttan Bıçak/Gölge Adımı/Duman Perdesi/İğne Batır/Kiriş Kesme)
+ İmzasız Bıçak (arkadan ×1.5 pasif) + Zehirli İğne (sol el, şarj =
İğne Zehri item'ı, kazanda 1 öz→3 şarj). ZEHİR İMZASI: doz sistemi —
3 dozda %20 altı SESSİZ SON (boss hariç); WEAPON_FX_VENOM görseli
(ründen ayrık: mor-yeşil aşağı damlar). npcState 16/16 + MOB 23/24 DOLU.
**SUİKASTÇİ P1 TAMAM (Sessiz El)**: MAP_LOCA "Kuyunun Dibi" (36×26; kuyu↔loca
portal — iniş [R], aklama [E] ayrımı); keşif: launderedTotal≥500 + GECE;
kadro Neriman/Çelme/İğne (NpcDef.map harita farkındalığı — npcState 16/16
DOLU, sıradaki NPC'de 20'ye büyüt!); görev 1 SINAV (gece Mühürlü Kutu,
crime_witnessed victimAlive=false — true geçmek "hep görüldü" bug'ıydı) +
görev 2 TAKİP (Kemal 4 durak, 100-400px bandı, 3 uyarı kuralı); muhafız
GECE DEVRİYESİ (sabit rota, dükkan önünden geçer — hırsızlık penceresi);
ESO sunum v2 TAMAM: sinematik diyalog ekranı (kamera+bant+portre+isim
plakası+hover/tık seçim), *anahtar kelime* vurgusu (markup), J Görevler
sekmesinde zincir metinleri, minimap+dünya hedef işaretçisi.
**REVİR TAMAM**: sivil NPC 1 canda bayılır (0 = ölüm yok) → revire yatar;
Şifacı Ela; masraf 200g×1.25^gün×önem×tekrar; 30 günde köy toplar ama
kırgınlık kalıcı (+%25 zam). sifaci.txt = ESO anlatım ilk örneği.
**KITA VİZYONU SABİT (PROGRESS "VİZYON — KITA PLANI")**: Doğu/Batı blokları
(içlerinde çok krallık/beylik + birer imparatorluk), bağımsız bölge = köy,
bölge haritaları modeli, ilk genişleme DOĞU çöl sultanlığı, olay tabanlı
siyaset, bölgesel gündoğumu + mevsim notu. Kapsam disiplini: fazlar bu
belgeye referansla açılır.
**MOB DALGASI + ÖLÜM GETİREN TAMAM (⑤)**: 3 tür SONA eklendi (18 Hortlak
Büyücü / 19 Kan Canavarı / 20 ÖLÜM GETİREN boss lv16); 2 fazlı boss savaşı
(Kara Patlama telegraph + %50'de hız/çağrı) mezarlıkta T(30,20), haftalık
diriliş boss2_* — SAVE'e TOLERANSLI KUYRUK deseniyle (dosya sonu, bump'sız);
İLK ölüm ödülü Ölüm Getiren'in Nişanı (NECKLACE ilk itemi: +%10 hasar +
öldürmede 8 HP). Yan bakışlı sheet'lerde yön: motor SAĞ bekler (Bringer
aynalandı). HUD barları: sayısal değer + max ile uzayan genişlik.
**LANETLİ MEZARLIK TAMAM (HG-B)**: MAP_MEZARLIK 60×45 açık hava zindanı
(undead palet; mühür dalgası + ambient 0.45); portal Kayalık Geçit T135,88;
map-bazlı dekor (DecorDef.map) — dev iskelet landmark + mezarlar/ağaçlar;
5 iskelet kampı + 2 sandık (USTA = Hortum 2. kaynak). Boss sonraki dalgada.
**HARİTA GÖRSEL YENİLEME FAZ A TAMAM**: çimen kenar taşması (12 hücreli
dalgalı overlay atlası — gen_tile_edges.py), 4 çimen varyantı (hash),
taş/bataklık/diken dokuları, süs serpiştirmesi (çiçek/taş/saz — deterministik
çarpışmasız), köy el yerleşimi (fıçı/bank/çit). Faz B: Lanetli Mezarlık.
**YANKICI GİZLİ SINIF P1 TAMAM (v29 uyumlu)**: skill defterinde görünmez;
keşif: 3 sömürü → fısıltı çağrısı → gizli taş halkası T(108,7) → gece ritüel
vinyeti → sınıf + FISILTI KESKİSİ (eksik-can ölçekli unique hançer) + Fısıltı
Patlaması (max HP %10 bedel) + Gölge Formu (5sn, 4HP/sn). F10 sayaç basar ama
keşfi oyuncu yapar. KALAN P2: Can Emme + Yankı Çağırma + T_ECHO ağacı.
**KİRLİ MAL & AKLAMA TAMAM (save v29)**: çalıntı damgası (kullanım/satış/
craft kilitli, tooltip ÇALINTI) + dirtyGold (HUD kırmızı) + keseler ×2 +
değerli objeler (Yüzük/Broş/Cep Saati — yalnız çalıntı) + MÜSADERE (ceza
ödemede kirli mal gider) + iki aklama kanalı: Kervancı 'el altından' (×0.75,
obje tam) / KÖR KUYU gece bırak → şafak ×0.5 ("Sayılıyor" fısıltısı — yeraltı
örgütü tohumu). Ağaç Keseci düğümleri komisyon düşürür.
**PICKPOCKET TAMAM (v28 uyumlu) — MEKANİK PAKETLER KAPANDI**: C eğil + [P]
(zar %35 + lvl×0.5 + eğilme %25 - deneme×%15/gün); keseler role göre;
yakalanma = suç (bounty 15, itibar -3, bark; muhafızda direkt panel);
T_PICKPOCKET 10 düğüm + GÖLGE ELİ keystone; persKetum izi; F10 puan basar.
**TWO-HANDED PARÇA A TAMAM (v28 uyumlu)**: Mızrak (menzil 130/tempo 1.5/
taban 34, delme %25→%40, hat vuruşu %60→%75 +hedef; craft S20 ya da 260g;
thrustOversize katmanı — salt thrust'ta çizilir) + T_TWOHAND ağacı (18 düğüm;
Saplama ×2.2, Süpürme 140° yay, keystone KAPI TUTUCU duruşu — otomatik hedef
+ 0.8sn tempo). **PARÇA B TAMAM — TH+BLOCK PAKETİ KAPANDI**: kalkanlar (Tahta/Demir, craft+
vendor; görsel ULPC round_shield z110 bağlandı; mızrak kompozit sheet'e
geçti — yürürken de görünür, thrust bloğu y=4992), pasif blok
(%10+lvl×0.2, emilim %60), SHIFT duruşu (-%70, yarı hız, 4 STA/sn), PERFECT
parry (ilk 0.25sn: hasar 0 + 1sn sersemletme; SİPER %50 yansıtma), T_BLOCK
ağacı (16 düğüm + Kalkan Vuruşu aktifi ×0.6+2sn stun). Blok zarı
combat_mob_damage'da tek nokta; block_feedback ortak işleyici.
**KAR (soğuk cephe, v26 uyumlu)**: havuz %50/22/13/10(KAR)/5; buz +%20 +
yavaşlatma ×1.5 + herkes %12 yavaş (av +%15); zemin beyazlar (snowCover),
sürü büzüşür, su donar, ot yarıya, ateş ömrü 12 saat, karda İZ SÜRME (arama
12sn + mob sana yürür); F8 döngüsünde. RNG artık tohumlu (kritik fix).
TAMAM: Meslek 5/5 (13-17) + Av & Pano & Kamp Ateşi (18) + Slime (19) + İskelet &
mermiler & ok kademeleri (20) + DERİN SKILL AĞACI (21, save v13) + BÜYÜ SİSTEMİ
(22: 6 büyü + Asa + INT + zırh ağırlığı + mana iksiri; ayrıntı PROGRESS Faz 22).
Mermiler KAÇILABİLİR — oyuncu okları güdümlü; bilinçli asimetri.
Zehir + kanama + yanma DoT'ları ÜST ÜSTE BİNER.

Tamamlananlar (2026-07-10): GPU-3 (shader gece aydınlatması + shake + vinyet),
**Suç & İtibar paketi** (NPC ölümü, muhafız, Sneak + mob şüphesi, bölgesel bounty+itibar; save v8),
**Diyalog koşulları** (req:/fx:, Speech zarı + günlük kilit, muhafız rüşveti [3], Türkçe UI taraması),
**Meslek paketi 1-4/5** (toplama süreci + istasyonlar/craft + blueprint ekonomisi + Kadim Sunak;
save v11 — ItemStack instance fx/stat; ayrıntı PROGRESS Faz 13-16).
Gün döngüsü test için 5 dk (tasarım 10 dk). Tuşlara ek: **C eğilme**, NPC'ye saldırı = tıkla + Space.
