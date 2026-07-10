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

### Faz 11 — Suç & İtibar paketi (TAMAMLANDI: Faz 1-4)
- NPC ölümü (kalıcı, save v7), tıklamayla bilinçli saldırı, tanık tespiti, bounty (+HUD)
- Muhafız döngüsü: yakala → öde/karşı koy, leash kaçışı, ertesi gün diriliş; bounty'liyken NPC konuşmaz
- Sneak: C toggle, algı = eğilme × görüş açısı × siper (dekor LoS) × örtü (conceals tile),
  sinsi vuruş ×2, çift kanal XP (süzülme tick + tanıksız suç); TileType.conceals (çimen)
- Hız çarpanı bug'ı düzeldi: player_update tile hızı dış çarpanı (perk/gece/sneak) eziyordu →
  Player.speedMult (perk hızları da fiilen İLK KEZ çalışıyor)
- Mekanik tamamlama (2026-07-10, "eksik adım bırakma" kararı):
  - Dekor genişledi: ağaçlar (siper/LoS, katı taban) + çalılar/saman (conceals, içine eğilerek girilir) —
    Cute_Fantasy'den kırpıldı (assets/decor/); DecorDef'e solidBase/conceals alanları
  - Mob ŞÜPHE sistemi: algı ikili değil — menzilde birikir (yakında ~0.3sn, sınırda ~2sn), dışında söner;
    dolunca aggro + "!" işareti; şüpheli mob oyuncuya döner (bakınır), baş üstü sarı şüphe çubuğu;
    saldırı yalnız alarmda başlar; leash şüpheyi de sıfırlar
- Bilinen bilinçli tercihler: muhafız sneak'ten etkilenmez (bounty sahibini ismen tanır);
  çim (zemin tile) örtüsü yalnız eğilirken işler
- Bölgesel bounty + itibar (save v8): suç işlendiği bölgeye; itibar -100..+100, ödeme düzeltmez,
  suçsuz gün +1 (0'a doğru); vendor fiyat çarpanı, ≤-50 konuşma reddi, ≤-25 görev kapatma

### Faz 12 — Diyalog koşulları & Speech
- Diyalog txt formatına `req:`/`fx:` satırları (speech/gold/rep/bounty koşulları; gold/rep/bounty efektleri)
- Speech zarı: eşik üstü %100, altı (speech/eşik)×%70; başarısızlık = o gün kilit + itibar -1;
  XP: başarı +15, başarısızlık +3; kilitli seçenek soluk + [Speech N - %şans] etiketi
- Muhafız rüşveti [3]: bounty/2, Speech 25; başarı itibar -5, başarısızlık = karşı koyma
- Saldıran mob otomatik hedeflenir (hedef boşken)
- Türkçe karakterler: tüm UI + diyaloglar düzgün Türkçe (PublicPixel glifleri doğrulandı)
- Not: zar kilidi save'e yazılmıyor (F9 save-scum kapısı — bilinçli erteleme)

### Faz 13 — Meslek paketi 1/5: Kaynak toplama
- 3 toplama skill'i (Mining/Herbalism/Woodcutting), 13 düğüm (günlük yenilenme, riskli yerleşim)
- Toplama süreci: araçla savurma döngüsü (vuruş=iş, düğümde bar) / bitkiler el ile (süre bazlı)
- Kademeli hız: 1 + 0.25×(seviye/20); iş: bakır5/demir10/ağaç7 vuruş, otlar 2.5-4sn; demir Mining 15 kapılı
- Araçlar silah slotuna kuşanılır (WEAPON_TOOL, saldıramaz); LPC Hand Tools smash128 katmanı (elde görünür)
- Kesinti: hareket/hasar/diyalog; yarım ilerleme gün sonuna dek düğümde
- Not: araç idle'da görünmez (sheet'te idle yok); bitki toplama animasyonu bulunursa bağlanacak

### Faz 14 — Meslek paketi 2/5: İstasyonlar & Craft
- Demirci Ocağı + Simya Kazanı; erişim ÜÇ kapılı: tanışma görevi (FETCH) + bounty yok + itibar > -25
- FETCH görev türü + görev item ödülü; Demirci Mahir (yeni NPC): 5 bakır → Çekiç + ocak izni; Baran: 3 Kızılot → kazan
- Süreçli craft: ocakta çekiç savurması / kazanda kaynatma; malzeme tamamlanınca tüketilir (iade güvenli)
- Tarifler: külçeler, Bakır Kılıç, Can/Büyük Can İksiri; '+basma' (şans tablosu + Smithing telafisi, plus-1 riski)
- Mana + Stamina havuzları (HUD) — tüketiciler ve stat basma Faz 4'te
- Save v9 (NPC listesi değişti); F10 debug craft test seti

### Faz 15 — Meslek paketi 3/5: Blueprint ekonomisi & Öğretmenler
- 6 blueprint'li tarif (Demir Kılıç/Zırh/Miğfer, Hız İksiri, Zehir Yağı); sağ tık = kalıcı öğrenme
- Kaynaklar: ork/goblin dropları + Demirci satışı (150g) + Baran dükkânı (100g) — "kesmenin anlamı"
- Öğretici parşömenler (%3, moba göre tür, +250 XP); öğretmen dersleri (seviye×15g, %40, günde 1)
- Buff altyapısı: süreli hız + silah kaplaması (Zehir Yağı → geçici POISON weaponFx)
- Animasyonlar: kazanda oyuncu spellcast (PLAYER_CAST) + kazan fokurdama + kabarcık; ocak alazlanması
- Save v10 (öğrenilen tarifler)

## 🔜 Kalanlar (öncelik sırası)
1. **Meslek paketi 4-5/5** (kararlar verildi, plan 2026-07-10):
   - F4: Enchanting sunağı — İKİ iş (kararlar 2026-07-10):
     a) öz→rün (moba/biyoma bağlı), üzerine basılır, seviyeye bağlı başarısızlık;
        weaponFx ItemStack'e taşınır (save)
     b) STAT BASMA: silah/zırha 2 slot + kademe I-III (aynı stat tekrar basılınca yükselir);
        statlar: STR/DEX/HP/DEF/MP/STA (havuzlar HUD'da hazır) — kombata bağlanır
        (STR→hasar, DEX→crit, HP→max can, DEF→defans, MP/STA→havuz büyümesi);
        malzeme: stat scroll'u + külçe; scroll kaynağı: mob dropu (%6-8, tür moba göre) +
        vendor (düşük kademe, ÇOK nadir + pahalı) + boss (yüksek kademe, mob fazlarıyla)
   - F5: Oduculuk ürünleri + minimal yay savaşı (ok craft, Yay kuşanılır, oyuncu projectile, Archery)
2. **Av & kontrat panosu**: hayvanlar (Cute_Fantasy Animals), deri/et, Witcher tarzı ilan panosu
3. **Mob fazları** (araya serpiştirilir): Slime ailesi → Skeleton ailesi (+ok/mermi sistemi) → Shroomling/Bombschroom → Mummy/Cowling/Flying Skull (yeni biyomlarla)
4. **İç mekanlar/zindanlar**: kapı→ayrı harita, Dungeon_1-3 tileset'leri, sandık/diken kapanı/gate
5. **Yeni biyomlar**: çöl / volkan / mantar diyarı (tiles+NPC+düşman hazır pakette)
6. **Ertelenen küçükler**: HUD perk ikon şeridi, item tooltip'leri, drag&drop, harita editörü akışı (Tiled tileset tanımı),
   CREDITS lisans teyitleri, F10 debug kaldırma, Vulkan/SPIR-V shader yolu, mobil build (SDL_GPU→Vulkan/Metal),
   ikinci yerleşim/muhafız (bölgesel bounty altyapısı hazır), mob "arama" davranışı (şüphe sonrası devriye),
   speech zar kilidinin save'e yazılması

## Karar defteri (değişmez)
- Sınıf yok; kimlik davranıştan: use-based skill + perk (3-seç-1) + bölge/hizip itibarı (global karma YOK)
- Suç = tanık varsa; tam muhafız döngüsü onaylı
- Bölge-sabit zorluk, level-scaling yok · Combat A (ıska yok) · KO auto-attack + hotbar
- Perk eşikleri 25/50/75/100 · Ölüm: köyde diriliş + %10 altın
