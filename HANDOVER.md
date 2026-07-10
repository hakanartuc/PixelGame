# HANDOVER — Pixe Game

Güncelleme: 2026-07-10 · Dil: konuşma Türkçe, kod/dosya İngilizce, yorumlar Türkçe

## Çalışma kuralları
- **Asistan kodu yazar** (2026-07-09'dan beri), faz faz ilerler; her faz sonunda durur, Hakan test eder, onayla commit atılır.
- **TÜM tasarım kararları Hakan'a sorulur** (AskUserQuestion) — denge değerleri, UX, kapsam. İnisiyatif alınmaz.
- Derleme hep `-Wall -Wextra` sıfır uyarı. Her çalışan aşama sonrası commit önerilir.
- Felsefe "no hidden machinery": sihirli soyutlama yok; sprite batch, shader, parser — hepsi görünürde.
- Oyun proje kökünden çalıştırılır: `./build/game` (asset yolları göreli).
- Test kalıbı: `pkill -x game; (./build/game &); sleep 5; kill -TERM $(pgrep -x game)` — temiz kapanış = cleanup yolu sağlıklı.

## Proje
C + SDL3 (3.4.8 vendored) + SDL_ttf + stb_image + CMake. Top-down 2D pixel-art RPG.
Vizyon: OFFLINE Skyrim-vari açık dünya, KO'dan ilham combat/loot. Sınıf yok — **kimlik davranıştan doğar** (use-based skill + perk + itibar).

**Görsel kaynaklar:** Oyuncu = LPC paper-doll (DOKUNMA). Diğer her şey = Cute_Fantasy paketleri
(`/Users/hakan/Assets/2D Pixel-Art/` — NPC, ev, düşman, zindan, çöl/volkan/mantar biyomları, UI, ikonlar; kullanım onaylı).

## Mimari (modüller)
- **r2d** — SDL_GPU üstünde KENDİ 2D renderer'ımız (SDL_Renderer tamamen söküldü, 2026-07-09):
  MSL shader gömülü (Metal; SPIR-V/Vulkan eklenecek nokta `load_shader`), sprite batch (kare sonunda
  tek upload + tek pass), alpha+additive pipeline, NEAREST/LINEAR sampler, 9-grid, render target
  (`r2d_target_begin/end` — kendi cmdbuf'uyla anında submit; batch'e `forceNewDraw` sınırı koyar),
  kare-içi texture yıkımı ertelenir. SDL kırpma davranışı taklit edilir (taşan src → clip).
  Duman testi: `./build/r2d_test`.
- **Oyun sistemleri:** tilemap (Tiled CSV, 200×150, culling) · region (bölge+mood+kamplar) · decor (evler, taban collision)
  · player (LPC katmanlı, oversize slash) · mob (leash/grup-aggro/tile çarpışma/kanama alanları) · npc (rutinli, saat bazlı)
  · dialog (txt format: `[dugum] / text: / opt: -> hedef|end|shop|accept:ID|complete:ID`) · quest (KILL) · shop (al/sat)
  · item/inventory/equipment (instance +plus, zırh sınıfı) · skill (use-based, XP=hasar) · ability+hotbar (1-8, KO auto-attack)
  · perk (48 perk, eşik 25/50/75/100, 3-seç-1, PerkEffects tek nokta) · combat (Combat A) · gametime (10dk=24s, mood müziği)
  · audio (SFX) + music (4 prosedürel parça, mood crossfade) · minimap (fog, target pass) · save (v11, alan-alan binary)
  · craft (istasyonlar/tarifler/sunak) · resource (toplama düğümleri) · region (bölgesel bounty/itibar)
  · ui_theme (9-grid paneller) · font (ttf→GPU upload).
- **main.c** — tüm bağlantı/orkestrasyon burada (~1500 satır; ileride bölünebilir).

## Tuşlar
WASD hareket · Space auto-attack toggle · Z hedef · 1-8 hotbar · E konuş/istasyon/topla · I envanter · K skill/perk · J görevler
· M minimap · C eğilme · **F10 DEBUG +1M XP + malzeme seti (yayında kaldır!)** · F5/F9 kaydet/yükle · ESC katmanlı kapatma/menü
(F efekt önizleme KALDIRILDI 2026-07-10 — rünün instance efektini eziyordu)

## Kritik bilinenler
- **Kanama/perk VFX'leri olay bazlı**; F10 sonrası moblar tek vuruşta ölürse kanama görülmez (Orc'ta dene).
- Partiküller mob/NPC'lerin ÜSTÜNE çizilir (altında kalıp kayboluyordu).
- CuteFantasy-5x9.ttf glyph seti eksik (boşluk yok) — font PublicPixel'de kaldı.
- Save dünya durumunu (mob/loot) içermez; harita değişirse eski save konumu tuhaf düşebilir.
- CREDITS.md'de lisans TODO'ları var (ULPC parça listesi + Cute_Fantasy ticari lisans teyidi) — mobil yayın öncesi şart.
- Perk eşiği 25'ten başlar; test F10 ile.

## Sıradaki iş
**Meslek 5/5** (PROGRESS #1): Oduculuk ürünleri + minimal yay savaşı (ok craft, Yay kuşanılır,
oyuncu projectile, Archery). Sonrası: av → mob fazları (Slime → Skeleton+mermi → çöl/volkan).

Tamamlananlar (2026-07-10): GPU-3 (shader gece aydınlatması + shake + vinyet),
**Suç & İtibar paketi** (NPC ölümü, muhafız, Sneak + mob şüphesi, bölgesel bounty+itibar; save v8),
**Diyalog koşulları** (req:/fx:, Speech zarı + günlük kilit, muhafız rüşveti [3], Türkçe UI taraması),
**Meslek paketi 1-4/5** (toplama süreci + istasyonlar/craft + blueprint ekonomisi + Kadim Sunak;
save v11 — ItemStack instance fx/stat; ayrıntı PROGRESS Faz 13-16).
Gün döngüsü test için 5 dk (tasarım 10 dk). Tuşlara ek: **C eğilme**, NPC'ye saldırı = tıkla + Space.
