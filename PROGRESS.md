# PROGRESS — Yapılanlar ve Kalanlar

Güncelleme: 2026-07-09

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

## 🔜 Kalanlar (öncelik sırası)

1. **GPU-3**: gece ışık sistemi (meşale/pencere ışık kaynakları), screen shake, hasar flash'ı; Vulkan/SPIR-V shader yolu (Windows/Linux/Android için)
2. **Suç & İtibar paketi** (kararlar verildi): Sneak, tanıklı suç, bölge/hizip itibarı, bounty, **muhafız** (yakala → ceza öde / karşı koy)
3. **Diyalog koşulları**: `req:`/`fx:` (Speech skill-check, rüşvet seçenekleri)
4. **Craft**: Smithing (+basma, risk), Alchemy (bitki→iksir), Enchanting (weaponFx instance'a taşınır)
5. **Av & kontrat panosu**: hayvanlar (Cute_Fantasy Animals), deri/et, Witcher tarzı ilan panosu
6. **Mob fazları** (araya serpiştirilir): Slime ailesi → Skeleton ailesi (+ok/mermi sistemi) → Shroomling/Bombschroom → Mummy/Cowling/Flying Skull (yeni biyomlarla)
7. **İç mekanlar/zindanlar**: kapı→ayrı harita, Dungeon_1-3 tileset'leri, sandık/diken kapanı/gate
8. **Yeni biyomlar**: çöl / volkan / mantar diyarı (tiles+NPC+düşman hazır pakette)
9. **Ertelenen küçükler**: HUD perk ikon şeridi, item tooltip'leri, drag&drop, harita editörü akışı (Tiled tileset tanımı),
   CREDITS lisans teyitleri, F10 debug kaldırma, mobil build (SDL_GPU→Vulkan/Metal)

## Karar defteri (değişmez)
- Sınıf yok; kimlik davranıştan: use-based skill + perk (3-seç-1) + bölge/hizip itibarı (global karma YOK)
- Suç = tanık varsa; tam muhafız döngüsü onaylı
- Bölge-sabit zorluk, level-scaling yok · Combat A (ıska yok) · KO auto-attack + hotbar
- Perk eşikleri 25/50/75/100 · Ölüm: köyde diriliş + %10 altın
