# PROGRESS — Yapılanlar ve Kalanlar

Güncelleme: 2026-07-10

## ✅ Tamamlanan fazlar

### Temel (GPU geçişi öncesi tarihçe)
- LPC paper-doll oyuncu (katmanlı ekipman, z-order, oversize slash, silah partikül fx)
- Envanter + ekipman UI, loot döngüsü, pause menüsü

### Faz 1 — Skill & Combat çekirdeği
- Use-based skill sistemi (14 kategori, XP eğrisi 100×level^1.5, K paneli)
- Combat A: silah+STR+skill+varyans+crit hasarı; defans azalan getiri 100/(100+def)
- XP yalnız hasar uygulanınca; düşük seviye mobda azalan getiri

### Faz 2 — Loot & Ekipman derinliği
- Item instance (+0..+8 plus, %10/plus), mob başına loot tablosu + geometrik plus roll
- Gold (popup'ta tıklanabilir hücre), Can İksiri (tür başına CD), başlangıç seti
- Save/load (F5/F9 + menü; alan-alan binary, v6'ya kadar evrildi)

### Faz 2.5 — KO Combat & Hotbar
- Space auto-attack (silah hızı döngüsü), 1-8 hotbar, aktif skill'ler (PS/QS, cooldown)
- Pot bara atanır, bardan içilir; CD perdeleri

### Faz 3 — Dünya
- Tiled CSV harita (200×150, GID-1, culling), 4+1 bölge (isim splash, kamplar, mood)

### Faz 4 — NPC & Diyalog & Quest & Vendor
- 3 NPC (Cute_Fantasy), txt diyalog formatı (+eylemler), KILL görevi, al/sat dükkanı

### Faz 5 — Yaşayan Dünya
- Gün/gece (10dk=24s, karartma, gece: algı×1.5 spawn×2), NPC başına saat rutini (gece eve girer)
- Evler (decor + taban collision), mob leash+hızlı heal, grup aggro, tile çarpışması

### Faz 6 — Ses & UI & Ölüm
- Prosedürel SFX (11 adet) — saf SDL3 stream, SDL_mixer YOK
- Adaptif müzik: 4 prosedürel parça (town/wild/danger/combat), bölge/gece/savaş mood'u, crossfade
- Fog-of-war minimap (M), quest log (J), ölüm ekranı + %10 altın cezası, CREDITS.md taslağı

### Faz 7 — UI Reskin
- Cute_Fantasy teması: 9-grid ahşap paneller, slot hücreleri, butonlar (ui_theme tek nokta)

### Faz 8 — Perk sistemi
- 48 perk (One-Handed + 3 zırh skill'i), eşik 25/50/75/100, 3-seçenekten-1, K panelinden seçim
- Zırh sınıfı + zırh XP'si; kanama/kaçınma/yansıtma/ölüm-kurtarışı/oyuncu-knockback mekanikleri
- Perk hissiyatı: 4 özel SFX + partikül patlamaları (kan damlası, crit kıvılcımı, Gazi közleri)
- F10 debug XP (yayında kaldırılacak)

### Faz 9 — SDL_GPU geçişi (GPU-1 + GPU-2)
- **r2d**: SDL_GPU üstünde kendi 2D renderer'ımız — MSL shader, sprite batch, 2 blend pipeline,
  9-grid, render target, ertelenmiş yıkım, src kırpma. SDL_Renderer tamamen söküldü.
- Oyunun tamamı portlandı (text upload, minimap target pass'leri, glow linear örnekleme)
- Duman testi: r2d_test. Bilinen port hataları düzeltildi (sandık smear, ev boyutları, minimap batch sınırı, fx çizim sırası)

### Faz 10 — GPU-3: Işık & His katmanı
- Shader gece aydınlatması: fs_light MSL + multiply pipeline, piksel başına ambient + 32 ışık,
  dither, altın saat; kaynaklar: ev penceresi / meydan direği / meşale (item) / silah efekti; titreşim
- Screen shake (hasar 0.25s/6px, crit 0.15s/3px) + kırmızı hasar vinyeti (0.35s)
- Gün döngüsü test için 5 dk'ya indirildi (tasarım değeri 10 dk — gametime.h)

## 🔜 Kalanlar (öncelik sırası)

1. **Suç & İtibar paketi** (kararlar verildi): Sneak, tanıklı suç, bölge/hizip itibarı, bounty, **muhafız** (yakala → ceza öde / karşı koy)
   - **NPC öldürme** bu paketin parçası olarak eklenecek (değerlendirme 2026-07-10, özgür-dünya vizyonuna uygun):
     - Tanık mekaniğiyle birleşir: tanıklı cinayet = bounty + muhafız; tanıksız iz bırakmaz (Sneak sinerjisi)
     - Gothic/Kenshi yolu: herkes ölebilir, görevler "başarısız" olur, dünya devam eder (Skyrim'in ölümsüz NPC'si YOK;
       ileride ana-akış kritik NPC'leri için "bayılma" ara çözümü düşünülebilir)
     - Kaza koruması şart: KO auto-attack yanlışlıkla NPC'ye kilitlenmemeli — NPC'ye saldırı bilinçli/ayrı eylem
     - İtibar bölgesel düşer (global karma yok) → fiyat, diyalog kapıları, muhafız tepkisi
     - Save'e npcAlive durumu gerekir (kalıcı ölüm); ölen vendor/quest-veren = o içerik kapanır (bilinçli bedel)
2. **Diyalog koşulları**: `req:`/`fx:` (Speech skill-check, rüşvet seçenekleri)
3. **Craft**: Smithing (+basma, risk), Alchemy (bitki→iksir), Enchanting (weaponFx instance'a taşınır)
4. **Av & kontrat panosu**: hayvanlar (Cute_Fantasy Animals), deri/et, Witcher tarzı ilan panosu
5. **Mob fazları** (araya serpiştirilir): Slime ailesi → Skeleton ailesi (+ok/mermi sistemi) → Shroomling/Bombschroom → Mummy/Cowling/Flying Skull (yeni biyomlarla)
6. **İç mekanlar/zindanlar**: kapı→ayrı harita, Dungeon_1-3 tileset'leri, sandık/diken kapanı/gate
7. **Yeni biyomlar**: çöl / volkan / mantar diyarı (tiles+NPC+düşman hazır pakette)
8. **Ertelenen küçükler**: HUD perk ikon şeridi, item tooltip'leri, drag&drop, harita editörü akışı (Tiled tileset tanımı),
   CREDITS lisans teyitleri, F10 debug kaldırma, Vulkan/SPIR-V shader yolu, mobil build (SDL_GPU→Vulkan/Metal)

## Karar defteri (değişmez)
- Sınıf yok; kimlik davranıştan: use-based skill + perk (3-seç-1) + bölge/hizip itibarı (global karma YOK)
- Suç = tanık varsa; tam muhafız döngüsü onaylı
- Bölge-sabit zorluk, level-scaling yok · Combat A (ıska yok) · KO auto-attack + hotbar
- Perk eşikleri 25/50/75/100 · Ölüm: köyde diriliş + %10 altın
