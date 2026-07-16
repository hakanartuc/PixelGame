# HANDOVER — Pixe Game

Güncelleme: 2026-07-14 · Dil: konuşma Türkçe, kod/dosya İngilizce, yorumlar Türkçe

## Çalışma kuralları
- **Asistan kodu yazar** (2026-07-09'dan beri), faz faz ilerler; her faz sonunda durur, Hakan test eder, onayla commit atılır.
- **TÜM tasarım kararları Hakan'a sorulur** (AskUserQuestion) — denge değerleri, UX, kapsam. İnisiyatif alınmaz.
- Derleme hep `-Wall -Wextra` sıfır uyarı. Her çalışan aşama sonrası commit önerilir.
- Felsefe "no hidden machinery": sihirli soyutlama yok; sprite batch, shader, parser — hepsi görünürde.
- Oyun proje kökünden çalıştırılır: `./build/game` (asset yolları göreli).
- Test kalıbı: `pkill -x game; (./build/game &); sleep 5; kill -TERM $(pgrep -x game)` — temiz kapanış = cleanup yolu sağlıklı.

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
- **main.c** — tüm bağlantı/orkestrasyon burada (~5000 satır; ileride bölünebilir).

## Tuşlar
WASD hareket · Space auto-attack toggle · Z hedef · 1-8 hotbar · E konuş/istasyon/topla · I envanter · K skill/ağaç · J Yolculuk günlüğü (Tab sekme, ←→ yaprak)
· M minimap · C eğilme · **F10 DEBUG +1M XP + malzeme/büyü seti + her ağaca 99 puan (yayında kaldır!)** · F5/F9 AKTİF slota kaydet/yükle · ESC katmanlı kapatma/menü
(K = skill listesi + ağaç paneli; ok mühimmatı sağ tıkla seçilir)
(F efekt önizleme KALDIRILDI 2026-07-10 — rünün instance efektini eziyordu)
AÇILIŞ AKIŞI (Faz B1): başlık ekranı → ana menü (Yeni Oyun/Yükle/Çıkış) → 20+1 slot;
vinyet açıkken tuşlar KİLİTLİ (yalnız 1-8 seçim, E/Space hız, ESC menü).

## Kritik bilinenler
- **Kanama/perk VFX'leri olay bazlı**; F10 sonrası moblar tek vuruşta ölürse kanama görülmez (Orc'ta dene).
- Partiküller mob/NPC'lerin ÜSTÜNE çizilir (altında kalıp kayboluyordu).
- CuteFantasy-5x9.ttf glyph seti eksik (boşluk yok) — font PublicPixel'de kaldı.
- Save dünya durumunu (mob/loot) içermez; harita değişirse eski save konumu tuhaf düşebilir.
- CREDITS.md'de lisans TODO'ları var (ULPC parça listesi + Cute_Fantasy ticari lisans teyidi) — mobil yayın öncesi şart.
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
**PİVOT (2026-07-14)**: hat MEKANİK-ÖNCELİKLİ (hikaye = mekanik test aracı).
**M1 TAMAM**: element sinerjileri (Termal Şok ×1.6 / Zehir Patlaması +çevre %50 /
Alev Rünü %20 yakma basar — hibritler sinerjide).
**M2+M3 TAMAM (save v23)**: pişirme (3 buff yemeği, Tok, çiğ et riski) +
davranış ünvanları (10 ünvan, hepsi pasif + 1 taşınan ×2, kitapta 3. sekme,
sayaç kancaları her sistemde). Savaş+yaşam üçlüsü (M1-M3) kapandı.
**M4 TAMAM**: Conjuration (17. skill+ağaç) — Öz Yılanı (bootstrap, %25 rezerv,
zehir) + Öz Kabuğu (tank+taunt, ağaçtan); rezerv barda mor kilitli bölme;
keystone'lar: Sürü Efendisi / Kurban Bağı. ONAYLI 4 MEKANİK KAPANDI (M1-M4).
Sıradaki adaylar: **D1c — S3 Sayan Adam** (Dalga 1 kapanışı; npc_walk + vinyet
zarı testleri) / **karakter yaratma-lite** / **Dünya Olayları** / **ikinci
yerleşim** (roadmap ORTA). Kullanıcıya sorulacak.
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
