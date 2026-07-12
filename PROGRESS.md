# PROGRESS — Yapılanlar ve Kalanlar

Güncelleme: 2026-07-11

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

### Faz 16 — Meslek paketi 4/5: Kadim Sunak (Enchanting)
- Sunak: Karanlık Orman sınırında (T102,14) sahipsiz harabe — KAPI YOK (keşif ödülü);
  Yaşlı Kemal [sir] düğümü (Speech ≥ 20) yerini söyler; obelisk dekoru
- weaponFx ItemStack instance'ına taşındı (save v11); item_make() zorunlu (compound literal fx siler)
- Rün ÜRETİMİ şanslı: %60 + Enchanting×0.4 (tavan %95), tutmazsa özler yanar;
  BASMA garantili (3 sn tören, mor partikül + spellcast): Pembe (+%8 hasar) / Zehir (4sn %25 DoT, Kanatma ile binmez)
- Özler: goblin Soluk Öz %20 / ork Zehirli Öz %20
- STAT BASMA: 2 slot + kademe I-III; STR/DEX +2, HP +10, DEF +3, MP +8, STA +10 per kademe;
  maliyet: kademe × (1 stat taşı + 2 Demir Külçe); taşlar: mob dropu %2-3 (türe göre) + Baran (Yaşam Taşı 120g)
- Türetilmiş statlar her kare: taban + perk + gear (max can artışı kadar iyileşme); DEF düz defans
- Ekipman paneli: stat rozetleri (mor nokta) + toplam satırı; sunak hedef satırında [Rün: X] + [STAT n]
- Düzeltilen bug'lar: ekipman TAKASI instance siliyordu (rün/stat kaybı); sahipsiz sunak bounty/itibar
  kapısına takılıyordu; perk max-HP hesabı gear HP ile yanlış delta veriyordu; süreç iptali sessizdi
  ("İşlem iptal edildi" eklendi); F efekt önizleme tuşu KALDIRILDI (rün efektini eziyordu);
  Long Sword'un kalıcı PINK tür efekti kaldırıldı (efekt yalnız rün/kaplamadan)

### Faz 17 — Meslek paketi 5/5: Okçuluk & Yay becerileri
- WEAPON_BOW: iki el (kalkan kilidi), hasar DEX ölçekli (+%2/puan), skill Archery (hasar bazlı XP)
- Köklü atış: Space → çekme (LPC shoot 16-19, ~0.9sn, release kare 9); WASD çekmeyi İPTAL eder (ok yanmaz);
  menzil 340px (melee 80) — CD/stamina trigger'da yanar (KO "hedef kaçsa da yanar" kuralı)
- Güdümlü ok: hasar VARIŞTA (crit + sinsi ok ×2 + rün zehir/pembe + aggro + hafif itme); çizgi görseli
- Ok ekonomisi: atış 1 ok; saplananların ~%30'u leşten geri (Mob.arrowsHit); "Ok yok!" + sayaç HUD
- Tarifler (ocak, dizin SONU — save v10 indeks uyumu): Ok x10 = 1 Odun + 1 Bakır Külçe; Yay = 2 Odun + 1 Demir Külçe (Smithing 5)
- Vendor: Yay 180g + Ok 2g; F10: yay + 50 ok + Archery XP
- 5 yay becerisi (Archery merdiveni, hepsi STAMINA tüketir — havuzun ilk gerçek tüketicisi):
  GO(1: ×2.2, 6s, 20) · BO(10: %40 yavaş 3sn, 10s, 15) · GS(20: 140px geri sıçrama, 8s, 25) ·
  YA(30: tek hedefe 3 ok ×0.6, 12s, 30; hedef ölürse yakına) · AO(45: varışta patlama 90px ×0.6, 15s, 35)
- Silah-beceri eşleşmesi: PS/QS↔melee, yay becerileri↔yay ("Yay gerekli!" / CD boşa yanmaz);
  stamina yetmezse "Nefesin yetmiyor!" (CD yanmaz); Mob.slowMult/slowTimer altyapısı
- Envanter 20 → 35 slot (7×5; paneller y=20'ye alındı; save slot sayısını yazdığından uyum bozulmadı)
- ULPC great bow görseli (shoot satırları + gömülü nişan oku); yay yalnız çekmede görünür
  (bu varyantta walk/idle katmanı yok — normal/recurve bow indirilirse idle/walk görseli bağlanır)
- Yayla NPC hedeflenemez (uzaktan tanıksız suç dengesi ayrı iş — ertelendi)

### Faz 18 — Av & Kontrat Panosu & Kamp Ateşi
- 4 hayvan (Cute_Fantasy Animals): Koyun/Domuz/Tavuk pasif — vurulunca "!" + 4sn düz kaçış
  (hız 550 ≈ oyuncu ×1.1; melee köşeye sıkıştırır, yay doğal av aracı); sürü birlikte ürker
  (kamp alarmı kaçışa çevrilir); Yaban Domuzu (Pig_05) orman sınırında saldırgan (HP 90 / hasar 12)
- MobType.passive + dailyRespawn; MOB_FLEE_TIME; hayvan bölgeleri GÜN dönünce dolar
  (spawn_zones_fill artık eksiği sayarak doldurur); kaçış RETURN dokunulmazlığına girmez
- Loot: Koyun/Domuz 1-2 Deri + 1-2 Et; Tavuk 1 Et; Yaban Domuzu 2-3 Deri + 1-2 Et garantili
- Deri seti (ocak, ARMOR_LIGHT İLK item'ları): Başlık 2 Deri/def4, Zırh 4 Deri/def8, Bot 2 Deri/def3
  (görseller mevcut LPC sheet'lerinin deri-kahvesi boyaması — ULPC deri seti gelirse değişir)
- Yemek mekaniği: Pişmiş Et (kazan VE kamp ateşi, 1 Et) → 12sn +3 can/sn (iksire yavaş-ucuz alternatif);
  ItemType.foodRegen/foodDuration + Player.foodRegenRate/foodTimer
- İlan Panosu (köy meydanı, Signs.png): 9 kontratlık havuz, günlük 3 ilan (kademe başına 1);
  kademeler: Temel (herkes) / Güvenilir (itibar ≥10) / Saygın (≥25) — kilitli ???+eşik görünür;
  kes (kabul SONRASI kill sayacı) + getir (teslimde tüketilir); ödül altın + bölge itibarı;
  bounty/dip itibar → "Arananlara iş yok!"; gün dönünce yenilenir; havuz veri tablosu (satır ekle = çeşit)
- Kamp Ateşi: köyde sabit (pano yanı) + oyuncu KURABİLİR — envanterde Odun'a sağ tık,
  3 Odun + 3sn süreç (WASD iptal); tek aktif oyuncu ateşi (yenisi eskisini söndürür);
  ömür kurulumdan 24 OYUN SAATİ (gece yarısı sınırı DEĞİL); kapısız pişirme istasyonu
  (STATION_CAMPFIRE, XP→Alchemy) + gece ışık kaynağı (light_append_campfire, sert titreşim);
  ateş animasyonu HEP oynar; çıkışta söner (save'e yazılmaz — bilinçli)
- contract.c modülü (save v12: pano günü + slot durum/ilerleme; load sonrası lastSeenDay senkronu —
  yoksa günlük blok yüklenen panoyu ezerdi); STATIONS artık const değil (oyuncu ateşi konumu)
- Bilinen küçükler: hayvanlar minimap'te düşman noktası; otlarken gezinmiyorlar

### Faz 19 — Mob fazı 1: Slime ailesi
- 6 tür (Yeşil/Mavi × Büyük/Orta/Küçük; Cute_Fantasy 2 renk kullanıldı, K/S/P yeni biyomlara saklı)
- BÖLÜNME zinciri: Büyük(120/160) → 2 Orta(50/70) → 2'şer Küçük(20/25); yavrular leşin iki
  yanından KIZGIN doğar (aggro+alertFlash); MobType.splitCount/splitChild; mob_die imzası
  mobs/nextMobId aldı; MAX_MOBS dolarsa yavru sessizce doğmaz (backstop)
- Loot YALNIZ Küçük'te (kasa spam'i yok — mob_die loot'u tablo/altın yoksa atlar):
  Jel %80 + altın; Mavi Küçük +%10 Zehirli Öz (sunağa bataklık kaynağı)
- Mavi vuruşu ZEHİRLER: 2 dps × 3sn (Player.poisonDps/poisonTimer; "Zehirlendin!"; süre tazelenir)
- Türe özel gövde boyu (MobType.bodyW/H — Büyük 72×60, Küçük 22×18); sheet: yönsüz blob,
  idle 4 / zıplama 8 / parçalanma-ölüm 8 kare (SLIME_ANIMS makrosu); saldırı = hızlı zıplama
- Slime Jeli (6g) + Yapışkan Tuzağı (kazan: 2 Jel + 1 Ayotu): hotbar'dan ayak önüne kurulur,
  mob basınca 100px alandakiler 5sn %60 yavaşlar; max 4 tuzak (round-robin); zemin jel görseli
- Kamplar HARİTADAN DOĞRULANDI (CSV açıklık taraması — ilk konum gölün içi çıktı!):
  Yeşil T44,20 (domuz otlağı doğusu), Mavi T40,85 (bataklık girişi)
- Not: hammaddeler (jel/cevher/deri) hotbar'a KONMAZ — hotbar yalnız consumable alır (tasarım)

### Faz 20 — Mob fazı 2: İskelet ailesi & mob mermi sistemi & ok kademeleri
- 3 tür (Kayalık Geçit): Savaşçı (HP140/hasar12, 64×64 kılıç savurması) / Okçu (HP90/ok14,
  menzil 280 < oyuncu 340, ~2.2sn tempo) / Büyücü (HP70/büyü22, menzil 240, ~3sn — cam top)
- MOB MERMİ SİSTEMİ: atış anındaki oyuncu konumuna DÜZ hat — yana yürüyerek KAÇILABİLİR
  (oyuncu oklarına bilinçli asimetri: duran hedef yer, hareket eden kaçar); mermi duvara
  takılır → kaya siperi bedavaya LoS mekaniği; isabet melee mob hattından geçer
  (dodge/defans/zırh XP ×0.15); büyü topu mor izli; MobType.attackRecover + projSpeed
- Kamplar (harita doğrulamalı): giriş T118-122,80 (2 Savaşçı + Okçu),
  derin T148-152,95 (Savaşçı + Okçu + Büyücü); gece kuralıyla undead saati
- Kemik ekonomisi: Kemik dropu; parşömenler — Okçu: Archery %4 (TÜRÜN TEK KAYNAĞI),
  Büyücü: Enchanting %4 + Soluk Öz %15; Savaşçı: OneHand %3; scrolls.png +2 hücre (yeşil/mor)
- OK KADEMELERİ (kullanıcı isteği): standart en ucuz, premium'lar sert —
  Ok (1 Odun→10, ×1.0) / Kemik Oku (1 Kemik+1 Odun→10, ×1.15) / Demir Ok (1 Külçe+1 Odun→10,
  ×1.3, Smithing 10); sağ tık = aktif mühimmat, bitince ucuza otomatik düşüş (bildirimli);
  HUD sayacı tür adıyla; kademe çarpanı beceri/sıçrama çarpanlarıyla birleşir;
  leşten geri dönen ok HEP standart (premium uç kırılır — ekonomi koruması)
- Diriliş animasyon satırları sheet'lerde kullanılmadı (ileride "mezardan kalkma" spawn'ı olabilir)

### Faz 21 — DERİN SKILL AĞACI (eşik anketi → puanlı düğüm ağacı; save v13)
- ESKİ sistem söküldü (perk.c silindi, 48 perk düğümlere dönüştü): 25/50/75/100'de 3-seç-1
  yerine GERÇEK ağaç — düğüm/kenar/önkoşul (BİRİ yeter), dallar, keystone'lar
- PUAN: skill başına her 4 seviyede 1 (100'de 25); savaş ağacı ~28-38p → TAMAMI ALINAMAZ
  (bilinçli kıtlık = build kimliği); KADER: TOPLAM seviyenin her 10'unda 1 puan
- SINIF: Küçük 1p / Dikkat çekici 2p / AKTİF 2p (mavi — beceri açar) / KEYSTONE 3p (BEDELLİ)
- 14 AĞAÇ (~200 düğüm, skilltree.c):
  · Savaş/sosyal 7'lisi: OneHanded/Archery/3 zırh/Sneak/Speech (kök + 3 dal + 2 keystone)
  · Meslek mini 6'lısı: Mining/Herbalism/Woodcutting (hız + çift ürün), Smithing (+basma
    şansı), Alchemy (iksir gücü), Enchanting (rün şansı + tören hızı) — 8 düğüm, 12p, dolabilir
  · KADER (karakter-geneli): Beden (HP/stamina/pasif can 0.5/sn) / Zihin (mana + TÜM XP
    +%5/8) / Yaşam (kasa altını/satış/iksir CD); K panelinde mor "* Kader" satırı
- 14 KEYSTONE (güç+feda): BERSERKER, İNFAZCI, KESKİN NİŞANCI↔AKINCI, HAYALET, RÜZGARIN OĞLU,
  YAŞAYAN KALE, FEDAİ, TAŞ BEDEN, ÖLÜMSÜZ, GÖLGE, HORTLAK, GÜMÜŞ DİL, ALTIN AĞIZ,
  AZİM↔ŞANSLI YILDIZ + meslek keystone'ları (DERİN DAMAR, USTA ÇEKİÇ, RÜN HAKİMİ...)
- AKTİFLER AĞAÇTAN: PS/QS + 5 yay becerisi seviyeyle DEĞİL satın alımla açılır →
  hotbar_place_unlocks abilityOpen[] okur; respec kapananları bardan temizler
  (unlockLevel alanı artık kullanılmıyor); başlangıçta aktif yok — normal vuruş taban
- YENİ ETKİ KANCALARI: yay (çekme hızı/menzil/Yaylım+1/Cephaneci/stamina), sneak (algı/
  eğilme hızı/sinsi çarpanı/gece), speech (alış-satış/zar+ceza/kontrat/ders/rüşvet),
  zanaat (toplama-dövme-kaynatma-tören hızları, çift ürün, +basma/rün şansı), kader
  (havuz max+regen, XP çarpanı skill_set_xp_mult, pasif can, kasa altını goldFind)
- UI: düğüm-kenar haritası (ilk tık incele, ikinci tık AL), sınıf renkleri, puan başlığı;
  RESPEC yalnız sunak yakınında: harcanan×50g
- DoT AYRIŞMASI (kullanıcı kararı): zehir + kanama ÜST ÜSTE BİNER (mob'da ayrı kanallar,
  kırmızı damla + yeşil kabarcık) — tam DoT build'i meşru (Kan dalı %75 + zehir %25)
- Sonraki dalga notu: Lockpicking/Pickpocket mini ağaçları MEKANİKLERİYLE gelir
  (zindan + suç genişlemesi); Two-Handed/Block ağaçları item'larıyla

### Faz 22 — BÜYÜ SİSTEMİ (silahtan bağımsız büyüler + Asa + INT + zırh ağırlığı)
- ÖZGÜRLÜK İLKESİ (kullanıcı kararı): büyü SİLAHTAN BAĞIMSIZ — kılıçlı büyücü meşru;
  her zırh sınıfı serbest ama BEDELLİ ("her seçim bir vazgeçiştir")
- 2 YENİ SKILL (save-toleranslı append): **Destruction** (büyü hasarıyla gelişir, tam ağaç:
  3 dal + 2 keystone + 6 AKTİF düğüm) + **Staff** (asa melee'siyle, standart ağaç) → 16 ağaç
- BÜYÜLER (aktif düğümle açılır; Ateş Topu HER ZAMAN açık — büyünün "normal vuruşu"):
  · **Ateş Topu** (15 mana/2s): güdümlü mermi + %30 yakma DoT'u (yanma kanalı AYRI — zehir/
    kanamayla ÜST ÜSTE BİNER, tam DoT build'i)
  · **Buz Novası** (20/6s): çevre 120px hasar + %50 yavaş 4sn
  · **Yıldırım** (30/8s): tek hedef anında yüksek hasar
  · **Alev Zırhı** (25/20s): 15sn — vurana +8 yansıtma, yan alev aurası
  · **Büyü Kalkanı** (25/25s): 12sn hasar HP yerine MANADAN (×1.2 bedel); mana biterse
    kalan cana + kalkan KIRILIR; melee/ok/muhafız emilir, zehir DoT'u deler; buz kubbesi
    görseli (oluşum→dönme→kırılma)
  · **Toprak Dikenleri** (22/10s): çevre 130px hasar + %65 yavaş 1.5sn; yerden fışkıran
    diken animasyonu (zemin katmanı)
- HASAR FORMÜLÜ (kullanıcı kuralı: "MP sadece resource"): baz × INT (+%2/10 üstü puan) ×
  Destruction seviyesi × silah çarpanı (asa spellPower + PEMBE rün ×1.08) × ağaç × varyans;
  DEX krit geçerli — statlar/rünler/silah HEPSİ ölçekler (combat_spell_hit)
- CAST: 0.45sn köklenme (castSpeedMult ölçekler), hareket BOZAR ("Büyü bozuldu"),
  mana commit'te yanar; hedefli büyüler (Ateş Topu/Yıldırım) Z hedefi + 300px menzil ister
- ASA (craft: 2 Odun + 1 Soluk Öz, Smithing 5; vendor'da da var): iki elli; Space =
  Ateş Topu cast'i (mana/menzil yoksa melee dürtme fallback — "Space silahın base atağı"
  kuralı); melee'si SKILL_STAFF XP'si verir; spellPower 0.20; thrust animasyonu (satır 4-7)
- INT STAT: Bilgelik Taşı (sunak töreni, iskelet büyücü %3 drop, scrolls.png cyan hücre)
- ZIRH AĞIRLIĞI (kullanıcı tasarımı): ağır 5 / orta 2 / hafif 1 puan; taşıma = STR−10;
  aşım × %2 hız cezası (tavan %30) + mana rejeni çarpanı (ağır −%6, orta −%3, taban 0.4) —
  STR'siz ağır zırh büyücüsü YAVAŞ + mana rejensiz (bedelli özgürlük)
- MANA İKSİRİ ayrı içecek (40 mana, 10s CD; Alchemy: 2 Ayotu); envanter 32→40 slot (8×5)
- VFX (4 yeni paket, assets/fx/): ateş küresi mermisi (Sog), arcane patlama additive
  (Yıldırım + cast parlaması), yanan mobda 12-kare alev + Alev Zırhı yan alevleri (flames),
  buz kalkanı kubbesi + toprak dikenleri (magic_pack); iskelet büyücü mermisi mavi küre oldu
- DEBUG: F10 → asa + 6 INT taşı + 5 mana iksiri + havuzlar full + **her ağaca 99 puan**
  (tree_set_debug_points — kaydedilmez, restart'ta sıfır; yayında F10 ile birlikte gider)
- Gelecek dalga notu: magic_pack kalanları (tornado/firelion/icetacle/lightningclaw) yeni
  büyüler; snake/turtleshell sprite'ları summon/mob adayı; magic_icons.png ikon sistemine

### Faz 23 — ZİNDAN PAKETİ (çoklu harita + kilit/tuzak + boss; save v14-16)
- **Çoklu harita** (worldmap.c): harita id + geçit tablosu (E ile geçiş, kapı/merdiven
  görselleri); geçiş akışı CSV yükle + mob/loot/partikül temizle + kamp doldur + oyuncu varış
  + splash + ambiyans. Dünya-özel sistemler zindanda kapalı (NPC/dekor/kaynak/istasyon/pano/
  muhafız/bölge). Save v14: aktif harita. Minimap çift keşif tamponu (dünya kalıcı, zindan geçici)
- **Eski Geçit** (Karanlık Orman T145,7 taş kemer; Osman [haber] işaret eder): 2 kat el yapımı
  CSV (scripts/gen_dungeon.py — K1 44×34 Dungeon_1 iskelet aileleri, K2 40×30 Dungeon_2 boss).
  TILE_TYPES 6-10 (zindan döşeme/duvar/void). Sabit loş ambiyans + titreşimli duvar meşaleleri
  (light_collect staticLights bayrağı), DANGER müzik
- **Lockpicking** (lockpick.c + mini ağaç 8 düğüm): zamanlama barı minigame (3 kademe;
  bölge/imleç skill+ağaçla ölçeklenir, görsel=isabet hesabı); başarısızlık maymuncuk kırar +
  260px GÜRÜLTÜ (mob şüphesi dolar — sistem sinerjisi); KİLİT FISILDAYAN keystone.
  Maymuncuk: Baran 15g + ocak (1 Demir Külçe→3). Sandık E ya da sol tık + baş üstü ipucu
- **Zindan sandıkları** (6, günlük yeniden kilitlenir, save v15): kilitsiz/Basit/Sağlam/Usta
  kademe + renk rozeti; Loot.kind/lockTier/chestId; Chest_anim görseli; chest_fill loot taslağı
- **Tuzaklar** (trap.c): periyodik dikenler (uyarı evresi, 15 zırh-delici hasar) + basınç
  plakası/duvar arbaleti (kaçınılabilir bolt 20 hasar, mob mermi havuzu üzerinden)
- **İskelet Lordu** (boss, save v16): 3.4× mor Savaşçı, HP 900/kılıç 28; Faz 2 (%50)
  ok yağmuru + iskelet çağrısı; arena tile kapısı ("Kapı kapandı"); mor boss barı;
  haftalık diriliş; ilk ölümde **Lord'un Kılıcı +2** (%5 can çalma — ItemType.lifesteal)
- **ÖLÜM = WIPE**: zindanda ölünce dünyaya çıkış (eski "zindan içinde köy koordinatı" bug'ı
  çözüldü); tüm moblar tam canla başa döner (boss dahil); dünyada ölünce kamplar tazelenir

## 🔜 Kalanlar (öncelik sırası — 2026-07-11 kullanıcı seçimi)
0. **ANLATI HATTI (aktif — STORY.md + LORE_SECRET.md + STORY_CARDS.md kanon):**
   · Faz A: vinyet/sahne motoru (pixel diorama + typewriter + dokunma beat'leri +
     duyusal direktifler: müzik kısma/kalp atışı ritmi) + diyalog portresi/isim plakası
     + [Yankı]/[Sömür] etiketleri — İLK YANKI PROTOTİPİ (iki hat: Tanık/Sömüren)
     → KOD TAMAM (test bekliyor): sinematik tam ekran vinyet + crossfade, yankı +
     yazıt (ECHO_ANI/ECHO_YAZIT), save v17, diyalog portre+typewriter
   · HİKAYE MİMARİSİ kilitli (2026-07-12): karma düğüm ana hat + 5 saga + 10 kısa
     öykü (kartlar: STORY_CARDS.md — hepsi onaylı; işaretsiz finale bağları)
   · Faz B: açılış senaryosu (ilk 15 dk: Sessiz Gün vinyeti, köye varış, ilk yankı,
     kanca) + quest log → "Yolculuk" günlüğü (birinci ağız + Book_UI) + bildirim dili
   · Faz C: in-engine sahne araçları (letterbox, kamera pan, NPC yürütme, bark sistemi)
   · Dünya Olayları sistemi (yaşayan dünya: olay tablosu + haber ağı + atmosfer saati
     + NPC yayları) + dinlenme/rüya mekaniği (kamp ateşi → rüya vinyeti)
   · Kural: her anlatı fazında İKİ hat yazılır (Tanık + Sömüren — tam ayrı kötü yol)
1. **Mob fazları** (devamı): Shroomling/Bombschroom → Mummy/Cowling/Flying Skull (yeni biyomlarla); slime Kırmızı/Sarı/Pembe yeni biyomlarda; iskelet "mezardan kalkma" spawn animasyonu
3. **Yeni biyomlar**: çöl / volkan / mantar diyarı (tiles+NPC+düşman hazır pakette)
4. **Onaylı yeni mekanikler** (2026-07-11 — uygun fazlarda gelir):
   · **Element sinerjileri**: yanan moba Buz Novası = termal şok patlaması, zehirli moba
     Yıldırım = zehir patlar (DoT kanalları ayrı — altyapı hazır)
   · **Summon hattı (Conjuration)**: yılan (DoT) + kaplumbağa (tank) — sprite'lar hazır
     (`~/Downloads/snake_right.png, turtleshell_right.png`); bedel: max mananın %25'i rezerve
   · **Pişirme + yemek buffları**: kamp ateşinde pişir → kısa süreli buff'lar (av döngüsü)
   · **Davranış ünvanları**: oyun stilinden ünvan + küçük pasif ("Slime Kasabı" %3 slime
     hasarı vb.) — "kimlik davranıştan doğar" felsefesinin görünür hali
5. **Büyü dalgası 2**: magic_pack kalanları (tornado/firelion/icetacle/lightningclaw/snakebite)
6. **Two-Handed hattı**: spear ilk iki elli silah + ağacı (asset hazır:
   `~/Downloads/lpc_male_item_animations_2026-07-11T13-46-16`); Block/kalkan aynı aileden
7. **Pickpocket**: mini ağaç + yankesicilik mekaniği (suç sistemine bağlanır)
8. **Ertelenen küçükler**: hotbar/ağaç ikonları (magic_icons.png elimizde — şu an metin kısaltması),
   item tooltip'leri, drag&drop, harita editörü akışı (Tiled tileset tanımı),
   CREDITS lisans teyitleri, F10 debug kaldırma, Vulkan/SPIR-V shader yolu, mobil build (SDL_GPU→Vulkan/Metal),
   ikinci yerleşim/muhafız (bölgesel bounty altyapısı hazır), mob "arama" davranışı (şüphe sonrası devriye),
   speech zar kilidinin save'e yazılması, yay walk/idle görseli (asset gelince),
   iskelet "mezardan kalkma" spawn animasyonu (sheet satırları hazır)

## Karar defteri (değişmez)
- Sınıf yok; kimlik davranıştan: use-based skill + perk (3-seç-1) + bölge/hizip itibarı (global karma YOK)
- Suç = tanık varsa; tam muhafız döngüsü onaylı
- Bölge-sabit zorluk, level-scaling yok · Combat A (ıska yok) · KO auto-attack + hotbar
- Perk eşikleri 25/50/75/100 · Ölüm: köyde diriliş + %10 altın
