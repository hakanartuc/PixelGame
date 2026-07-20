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

### Faz B1 — AÇILIŞ PAKETİ (başlık ekranı + ana menü + slot sistemi + Sessiz Gün; save v18)
- **AKIŞ**: atmosferik başlık ekranı ("PIXE" + fısıltı + nabızlı "bir tuşa bas") →
  ANA MENÜ (Yeni Oyun / Yükle / Çıkış) — dünya artık menüden önce başlamaz
- **SLOT SİSTEMİ** (kullanıcı: "1 PC'de birden çok oyuncu — kısıtlama"): 20 manuel +
  1 otomatik slot (`saves/slot_NN.dat`, `saves/auto.dat`; .gitignore'da); save v18
  başına ÖZET BLOĞU (toplam seviye/gün/altın/harita — save_read_summary tam yükleme
  yapmadan slot kartını doldurur); dolu slota yeni oyun = üzerine yazma onayı
- F5/F9 = AKTİF slot (girilen/kaydedilen slot aktif olur; auto'dan yüklemede aktif yok);
  OTOMATİK KAYIT: her şafak + her harita geçişi ("Otomatik kayıt" bildirimi)
- ESC menüsü: Devam / Kaydet (slot) / Yükle (slot) / Ana Menüye Dön (kayıpsız uyarısı) / Çık
- **YENİ OYUN RESET**: playerTemplate şablonu (init'le tek kaynak) + tüm dünya durumu
  sıfırlanır (mob/loot/quest/keşif/yankı/sandık/boss/pano/tarif/debug puanları);
  minimap_reset_explored eklendi
- **SESSİZ GÜN AÇILIŞ VİNYETİ** (scripts/gen_intro.py, 4 panel): donmuş dünya (havada
  asılı tavuk/yapraklar) → dokuz atım (iç içe mor nabız halkaları) → başkasının anısı
  (BULANIK: kazan eller, çukur, bakan şey — tek geçiş blur) → uyanış (şafak + göğüste
  mühür). Metin: assets/scenes/intro_sessiz_gun.txt
- **KÖYE VARIŞ**: yeni oyun spawn'ı köy doğusu yol kenarı (T24,8) — araba yankısının
  yanından geçilir (öğretici doğal yerde); meydanda MUHAFIZ OTOMATİK DURDURUR:
  3 kişilik cevabı → GİZLİ SAYAÇLAR (persKaba/Kibar/Ketum, save v18; dialog fx/req:
  `kaba/kibar/ketum` değişkenleri) — ileride NPC'ler oyuncuyu "tanır"; kibar +2 itibar
- **KEMAL KANCASI**: goblin görevi ödülü sonrası "Eski Geçit iması" düğümü
  (hayvanlar o yöne gitmiyor + gece uğultusu + "mühür bir an ısınıyor")
- **GEZGİN ÇIKINI** (kullanıcı: zırhsız başlangıç iade riski): Uzun Kılıç + DERİ SET
  (başlık/zırh/bot = 15 def, hafif → ağırlık cezasız) kuşanılı + 3 Can İksiri +
  2 Pişmiş Et + Meşale + 50g
- **ARABA GÖRSELİ** (kullanıcı tespiti: pakette cart sprite'ı YOK): gen_diorama.py'a
  piksel araba çizimi — devrik (havada dönen teker + yatık teker + kopmuş ok) ve
  sağlam (anı paneli) varyantları; 4 yankı paneli yeniden üretildi
- **SAHNE TUŞ KİLİDİ** (kullanıcı: immersion): vinyet açıkken TÜM panel/oyun tuşları
  yutulur — yalnız 1-8 (seçim), E/Space (typewriter hızı), ESC (menü) çalışır

### Faz B2 — YOLCULUK GÜNLÜĞÜ (J = kitap; birinci ağız + bildirim dili; save v19)
- **KİTAP PANELİ** (journal.c, Book_UI parşömen 4×): J artık liste değil kitap açar;
  iki yer imi şeritli sekme, [Tab] sekme / [←→] yaprak / [J] kapat; metin sayfalara
  kendiliğinden akar (TTF_GetStringSizeWrapped ölçümüyle sütun taşması)
- **GÖREVLER sekmesi**: üstlenilen işler BİRİNCİ AĞIZ (QuestDef.journal alanı) +
  durum satırı "( 3/5 )" / "Sözü yerine getirdim" / "Tamamlandı" (soluk mürekkep)
- **GÜNLÜK sekmesi** (kronolojik, "Gün N —" damgalı): uyanış (JE_WAKE, yeni oyunun
  ilk satırı) · görev alma/bitirme · YANKILAR (Tanık/Sömüren tonu seçime göre —
  sayı asla yazılmaz, sadece his) · yazıt ilk okuma · zindana ilk adım · boss ·
  ölümler · GÜN DÖNÜMÜ atmosfer satırları (3/7/14/21/30 — seyrek, spam değil)
- **KAYIT MODELİ**: girişler KOD+PARAM+GÜN (JournalEntry, JOURNAL_MAX 96) —
  metin render'da türetilir (dil değişse eski save doğru okunur); save v19
- **BİLDİRİM DİLİ** (ikisi bir arada): "Üstlendim: X" / "Söz yerine geldi —
  haber vermeliyim" / "Sözümü tuttum." kişisel + "+100 Gold"/"+Çekiç" sistemik
- Eski quest_log_render kaldırıldı (quest.c)

### Faz E1 — MOB EKOLOJİSİ ÇEKİRDEĞİ (KO slot spawn'ı söküldü; save v20)
- **YUVA/POPÜLASYON** (ecology.c, 26 saha: eski kamplar + aile başına boş göç
  sahaları): ölen mob GERİ GELMEZ; her şafak yuva +1 doğum (tavana kadar,
  yuva noktasından — göz önünde belirme yok); sürekli respawn dünyada KAPALI;
  popülasyon save v20'de kalıcı; ölüm WIPE'ı artık MAX'a değil MEVCUT nüfusa döner
- **AİLEYE GÖRE YIKIM** (kullanıcı: tekdüze olmasın):
  · GOBLİN: **Goblin Reisi** [MOB 16] (2.8×, kızıl, 260 HP) — ölünce kamp ÇÖKER,
    kalanlar kaçışır (Mob.scatter: kaç + sessiz despawn)
  · ORK: **Savaş Totemi** [MOB 17] (yapı-mob: isStructure, 300 HP, kımıldamaz;
    nests.png hücre 0) — vurunca kamp savunmaya koşar (grup aggro), yıkılınca çöker
  · SLİME: renkli sızıntı gölcüğü (zemin görseli, family tint) — hepsi ölüyken
    [E] kurutma (MEŞALE gerekir)
  · İSKELET: lanetli mezar görseli — hepsi ölüyken [E] MÜHÜRLEME (taş kalıcı
    mühürlü görünüme döner; camgöbeği motif = intro mühür dili); mühür GÖÇ ÜRETMEZ
  · HAYVAN: sürü modeli — tükenirse çayır boşalır, sürü göçer
- **GÖÇ**: çöken yuvanın ailesi 5-15 gün sonra BOŞ alternatif sahaya koloni kurar
  (pop 2'den başlar); iki saha da yok edilirse aile o dünyada TÜKENİR (bedel)
- **MÜHÜR İSTİSNASI**: zindan ekolojiye dahil değil — giriş başına tazelenme sürer
- Her temizlik günlüğe birinci ağız düşer (JE_NEST_CLEANSE ×4 metin)
- Kapasiteler: MAX_MOB_TYPES 16→24, MAX_MOBS 48→64 (taşma bug'ı — reis görünmüyordu)
- Bilinen E1 sadeleştirmeleri: koloni dünya YENİDEN KURULUNCA görünür (zindan
  gidiş-dönüş/ölüm/yükleme tetikler); slime bölünme çocukları ebeveyn zone'unu
  paylaşır (pop 0'da klamplenir — ritüel yine tüm ölüleri ister)
  → İKİSİ DE E2'DE KAPANDI (aşağıda)

### Faz E2 — MOB RUTİNLERİ & HABER AĞI (moblar yaşıyor; save v21)
- **GECE/GÜNDÜZ RİTMİ** (kullanıcı seçimi): goblin+reis GECECİ — gündüz kampta
  uyuklar (baş üstü soluyan "z", algı ×0.35); hayvanlar GECE uyur (otlamaz);
  orklar NÖBETLİ — uyumaz, gece yalnız yarısı devriyede (id çiftliği)
- **UYUYANA SİNSİ VURUŞ** (kullanıcı seçimi): uyuyan moba melee/ok vuruşu
  eğilmeden de sinsi sayılır — mevcut backstabMult + Sneak XP (gece baskını build'i)
- **DEVRİYE**: kamp mobu boştayken yuva çevresinde nokta nokta tur (yarı hız,
  patrolAngle/Timer); şüphe/alarm devriyeyi keser — "slotta bekleme" öldü
- **SU NOKTASI**: ova sürüleri (koyun/domuz) öğlen 10-14 göl kenarına iner
  (NestSiteDef.waterX/Y → mob'a atanır) — avcıya tahmin edilebilir pusu penceresi
- **HABER AĞI**: koloniler panoda "~ Söylentiler ~" bölümünde 7 gün duyurulur
  ("Goblinler Yeşil Ova taraflarına yerleşmiş" — ecology_recent_colonies +
  region_at); yoksa "Ortalık sakin... şimdilik."
- **DİNAMİK KONTRAT** (kullanıcı seçimi): nüfusu tükenen türün kes-kontratı
  çıkmaz — aynı kademeden geçerli ilan geçer; hiç yoksa "Bugün ilan yok"
- E1 kapanışları: koloni DÜNYADAYKEN de şafakta belirir (ecology_dawn zone
  kaydı + anında spawn); slime bölünme çocukları ebeveyn yuvasına sayılır
  (ecology_note_spawn — gölcük ritüeli çocuk canlıyken açılmaz)
- Save v21: NestState.colonizedDay (söylenti süresi)

### Faz CİLA — his + kullanılabilirlik paketi (roadmap YAKIN #1)
- **TOOLTIP** (tooltip.c): envanter/ekipman/dükkan hücresinde tam detay —
  ad(+plus), tür, hasar/defans (instance), rün/stat, tüketim etkileri, değer;
  KUŞANILIYLA KARŞILAŞTIRMA (yeşil ^/kırmızı v fark satırı); ekran içinde çevrilir
- **DRAG&DROP** (sol tık — sağ tık yolları duruyor): çanta içi taşı/takas,
  çantadan ekipmana kuşan, ekipmandan çantaya çıkar, tüketilebilir/tuzağı
  hotbar'a sürükle; yarı saydam ikon imleci izler
- **İKONLAR** (assets/icons/abilities.png ← gen_ability_icons.py): hotbar SKILL
  slotları + ağaç AKTİF düğümleri — büyüler magic_icons.png'den (64→32
  piksel-korumalı), kılıç/yay prosedürel glif
- **MÜZİK ×3** (gen_music.py artık repoda): MOOD_DUNGEON (dron+çan; zindan
  haritaları DANGER'dan ayrıldı), MOOD_NIGHT (gece açık arazi), MOOD_MENU
  (52bpm mühür nabzı — başlık ekranı teması)
- **AMBİYANS** (ambience.c + gen_ambience.py): gündüz kuş/rüzgar, gece cırcır,
  zindan uğultu/damla — müziğin altında, 2sn crossfade, menüde susar
- **SFX VARYASYON**: vuruş/krit/kaçınma/yansıtma/loot ±%8 perde oynaması
  (SDL_SetAudioStreamFrequencyRatio)
- **AYARLAR**: ESC + ana menüde Müzik/SFX kaydırıcıları → settings.cfg (kalıcı)
- **FIX (E2 regresyonu)**: savaş müziği MOB_CHASE'e bakıyordu — devriye/su
  ziyareti tetikliyordu; artık yalnız AGGRO moblar
- **MÜHÜR DALGASI** (kullanıcı tespiti: zindanda 3sn'de respawn): sürekli
  respawn + gece ×2 SÖKÜLDÜ (spawn_zones_update silindi) → her 120sn
  (SEAL_WAVE_SECONDS) toplu diriliş; 3sn önce "Mühür titriyor..." + fısıltı +
  sarsıntı; oyuncuya 280px'ten yakın doğum yok; girişte sayaç sıfırlanır;
  BOSS dalga dışı (haftalık diriliş — farm yok)

### Faz D1a — SAHNE ARAÇLARI + K9 KANDİL (ilk hikaye kartı oyunda; save v22)
- **SAHNE ARAÇLARI (Faz C — 4'ü de geldi):**
  · BARK: npc_bark(i, metin, sn) — baş üstü balon, son 0.5sn solar
  · CINE MODU: letterbox bantları + kamera hedefe süzülür (cineX/Y/Timer);
    oyuncu girdisi kilitli (boş tuş tablosu), dünya akmaya devam eder
  · NPC YÜRÜTME: npc_walk_to/walk_done/walk_cancel — rutini ezer (S3 için hazır)
  · VİNYET ZARI: sahne dosyasında `roll: speech 20 -> fail_id` (opt'a bağlanır);
    [Speech N] etiketi; diyalog zarı eğrisi; scene_set_speech köprüsü
- **K9 KANDİL** (STORY_CARDS K9 — en düşük maliyetli 🜁):
  · Fatma NPC (Bartender_Katy): gündüz meydan, 17:00'de geçit kapısına yürür
  · Kandil objesi (lantern.png + light_append_point) geçit kemeri önünde
  · Kerem yankısı: F2 nöbetçi odası T(20,6) — ECHOES[2]; Tanık (çocuğun kararı) /
    Sömüren ("kandilin nedenini eskisi kadar iyi hatırlamıyorsun") — 3 panel (gen_k9.py)
  · Karar (yankı sonrası Fatma diyaloğu, `k9:N` hedefleri): GERÇEK → tapınak ateşi
    (büyük mavi ışık + kandile cine + bark) / YALAN → kandil söner / SUS → yanar;
    kalıcı (storyFlags[SF_K9_KANDIL], save v22) + günlüğe 3 ayrı metin (JE_K9_CHOICE)
    + Fatma'nın kapanış diyaloğu seçime göre (son1/2/3)
- **DOĞALLIK FİXİ** (kullanıcı tespiti): konuşulan NPC artık DURUR —
  npc_set_talking(dialogNpc) her kare senkron; diyalog bitince rutine döner
- Altyapı: player.storyFlags[16] (kart durum kodları — save v22), light_append_point

### Faz D1b — K2 SON MÜŞTERİ (Sömüren'in ilk somut bedeli)
- **HAN** (köy GD köşesi T17,10 — House_1 red) + **Nazife** (Chef_Chloe): gündüz
  kapıda, 19:00'dan sonra sofra başında
- **SOFRA**: gece kurulur (gen_k2.py masa sprite'ı + mum ışığı) — Nazife gerçeği
  öğrenene kadar HER gece; öğrenince kalkar (görsel + ışık gider)
- **İKİ SIRA DA ÇALIŞIR**: araba yankısı hattı artık kalıcı (SF_K2_CART, seçim
  anında yazılır) — handan önce yankı / yankıdan önce han
- **TANIK hattı**: nasıl söylediğin her şey — YUMUŞAK (süt kokusu kanıtı: "Sütü
  hep unuturdu... o zaman gerçekten oydu", kibar+1) / DÜZ (ketum+1) / SUS
  (kalıcı değil, dönüp söyleyebilirsin)
- **SÖMÜREN hattı**: kanıt YOK — "Kokusunu söyle. Elini. Son sözünü. ...
  Bilmiyorsun." Sofra sonsuza dek kurulur; günlüğe itiraf ("bu benim yüzümden")
- Karar kalıcı (SF_K2_NAZIFE) + JE_K2_CHOICE 3 metin + kapanış diyalogları
- Kişilik fx'leri ilk kez kart diyaloğunda (kibar/ketum — okunma D1c'de genişler)
- **D1b-ek (2026-07-16, merge sonrası port)**: k2:1 artık 2 panellik SON SOFRA
  vinyetiyle kapanır ("ikimize" gecesi + ocakta süt devri + fısıltının ilk kez
  susması — huzur=sessizlik kuralı); han House_1 kopyasından gerçek Inn_Red
  sprite'ına yükseldi (0.8 ölçek, konum aynı); gen_k2.py yol fixi + paneller
  script'e eklendi. NOT: paralel oturum çakışmasında remote D1b esas alındı,
  yereldeki bağımsız D1b atıldı — bu iki parça oradan taşındı.

### Faz M1 — ELEMENT SİNERJİLERİ (mekanik-öncelikli hat #1)
- **TERMAL ŞOK**: yanan hedefe Buz Novası → kalan yanma ANINDA ×1.6 patlar,
  yanma söner (buhar FX + "Termal Şok!" + krit-stili sayı) — DoT'u erken
  bozdurma bedeli tempo seçimi oldu
- **ZEHİR PATLAMASI**: zehirli hedefe Yıldırım → kalan zehir ANINDA + 90px
  çevreye %50'si (yeşil bulut) — kalabalıkta zincir potansiyeli
- **ALEV RÜNÜ YAKIYOR** (kullanıcı onayı): melee + ok vuruşu %20/3sn yakma
  basar — hibrit build'ler (alev kılıç → Nova) sinerjiye girer; rün seçimi
  gerçek trade-off: Pembe %8 anında / Alev %20 DoT+şok / Zehir %25 DoT+patlama
- NOT (2026-07-14 kullanıcı kararı): hat MEKANİK-ÖNCELİKLİ — hikaye mekaniği
  taşıyan/test ettiren araç; D1c (S3) beklemede, "okunma" her faza serpiştirilir

### Faz M2 — PİŞİRME + YEMEK BUFFLARI (av döngüsü kapandı)
- **3 BUFF YEMEĞİ** (kamp ateşi + kazan; baştan bilinir): Avcı Yahnisi
  (2 Et+1 Kızılot → 180sn TÜM hasar +%8 — melee/ok/büyü), Ot Çorbası
  (2 Ayotu+1 Et → mana rejeni ×1.5), Gezgin Kavurması (2 Et+1 Ayotu →
  stamina ×1.6 + hız +%5)
- **TEK "TOK" BUFFU**: yenisi eskisini ezer; HUD sayacı ("Tok: X (Ns)") +
  "Acıktın" bildirimi; süreli buff save'e girmez (iksir deseni)
- **ÇİĞ ET YENİR** (kullanıcı onayı): +6 can ama %25 zehirlenme (8sn) —
  ItemType.rawRisk alanı (bedelli kötü seçenek)
- FoodBuff enum + ItemType foodBuff* alanları; FB_DMG combat_player_hit +
  combat_spell_hit'te; rejenler main'de; tooltip Tok/çiğ satırları; F10 seti
- Av döngüsü: avla → kamp kur → pişir → buff'la yola devam (ekoloji av
  kıtlığıyla et stratejik kaynak)

### Faz M3 — DAVRANIŞ ÜNVANLARI ("kimlik davranıştan doğar" vitrini; save v23)
- **10 ÜNVAN** (title.c): Slime Kasabı/Goblin Belası/Ork Kıran (aile +%5),
  Gölge (sinsi +0.1), Gece Kuşu (gece +%3), Yuva Yıkıcı (reis/totem +%10),
  Usta Avcı (av çift ürün %10 — ikinci kasa), Nişancı (yay +%3),
  Kadeh (iksir +%5), Aşçı (Tok süresi +%20)
- **HEPSİ PASİF + 1 TAŞINAN** (kullanıcı seçimi): açık ünvanların pasifi kalıcı;
  taşınan ünvanın pasifi ×2 + HUD'da isim ("- Gölge -"); kitapta tıkla = taşı/bırak
- **KİTABA 3. SEKME**: J → Tab döngüsü Görevler→Günlük→Ünvanlar; açıklar altın
  (taşınan kızıl), kilitliler ilerleme sayaçlı (87/100); journal_title_at hit testi
- Sayaç kancaları: aile kesimi (reis→goblin, totem→ork sayılır), gece kesimi,
  sinsi vuruş (melee+ok), yuva çöküşü/ritüeli, atılan ok, içilen iksir,
  pişirilen yemek; kazanımda bildirim + SFX + JE_TITLE günlük satırı
- Save v23: titleStats[12] + carriedTitle; F10 sayaçları eşik altına çeker
  (2-3 eylemle ünvan düşer — hızlı test)

### Faz M4 — SUMMON HATTI: CONJURATION ("Öz Bağı"; onaylı mekanikler kapandı)
- **17. SKILL + TAM AĞAÇ**: Conjuration — XP dostların verdiği hasardan (use-based);
  save bump GEREKMEDİ (skill/ağaç blokları sayı-toleranslı)
- **ÖZ YILANI** (bootstrap — Ateş Topu kuralı): 30 mana + %25 REZERVE; hızlı,
  zehir basar (60HP / 6dmg + %25 zehir 4sn); **ÖZ KABUĞU** (ağaçtan AKTİF):
  tank 220HP, 2sn'de taunt nabzı — moblar hedefini kabuğa çevirir ve ONU döver
- Dostlar EFEKT-varlık (yeşil öz; snake/turtleshell 128px belirme+hamle sheet'leri —
  yürüyen pet değil, süzülen büyü); ally.c: takip/hedef seçimi (yalnız SENİN
  aggro'n — sadık, sneak bozmaz)/ışınlanma/can şeridi
- **KURALLAR**: tek dost (yenisi gönderir), hotbar'a tekrar bas = geri çağır
  (rezerv çözülür), ölünce 10sn CD; REZERV bar'da MOR KİLİTLİ bölme (kullanıcı
  tespiti: oransal bar rezervi gizliyordu — görsel bölme eklendi)
- **KEYSTONE'lar**: SÜRÜ EFENDİSİ (iki dost, rezerv dost başına %30) /
  KURBAN BAĞI (dost hasarı +%40, dost ölünce SEN 15 hasar — "Bağ koptu!")
- Ünvan çarpanları dost hasarına da işler; ikon atlası 15 hücre; F10 Conjuration

### Faz D1c — S3 SAYAN ADAM (Dalga 1 kapanışı; MAP_KONAK)
- **NUH + ZEHRA** (Fisherman_Fin / Desert_Person_2, 32px frameSize): Nuh gündüz
  sunak yolunda sayarak volta (T60,18), gece köy kenarı; Zehra meydanda —
  3 ipucu verir (SF_S3_HINTS; yeni `hint` diyalog değişkeni req'te okunur)
- **SAYI BULMACASI** (3 tur): 41 (köyün nefesi) / 9 (Sessiz Gün atımları) /
  14.612 (kırk yıllık nöbet günleri); yanlış = O GÜN kilit (nuhLockDay —
  save'e girmez, zar kilidi kuralı); ipuçlu kolay seçenek + Speech 25 zar turu;
  DIALOG_MAX_OPTS 4→5
- **KONAK YOLculuğu**: 3. doğru → cine (kamera Nuh'u izler) + npc_walk ile
  sayarak yürüyüş (sahne araçlarının ilk gerçek kullanımı)
- **EŞİKÇİ KONAĞI** (MAP_KONAK, T168,45 kemer): loş ama zindan DEĞİL (dalga/mob
  yok; MOOD_NIGHT + 0.55 ambient); giriş salonu/koridor/büyük salon/kilitli
  KAYIT ODASI ("Nuh'suz açılmaz")
- **KARDEŞ YANKISI** (İdris): Tanık = kaydın anlamı; SÖMÜREN = fısıltı İLK KEZ
  yan hikayeye karışır: "...sayan adamdan uzak dur." (açıklanmaz — kanon sinyali)
- **KAYIT ODASI VİNYETİ** (nöbet devri): DEVRAL → "Kayıt Tutan" ünvanı (11. —
  eşiksiz, yalnız bu seçimle; +10 max mana, taşınırsa ×2; TS_RECORD) /
  KAPAT → aile huzuru / [Speech 30 ZAR] üçüncü yol: Zehra'ya öğret (kapı açık
  kalır — vinyet zarının ilk kullanımı; SceneFx.s3 alanı)
- Nuh/Zehra kapanış diyalogları kalıcı; JE_S3_STEP + JE_S3_CHOICE günlük
  metinleri; paneller gen_s3.py (6 panel); npcState tavanı 8→12 (9 NPC)
- **DALGA 1 KAPANDI**: K9 + K2 + S3 oyunda

### Faz KY — KARAKTER YARATMA-LİTE (save v24; roadmap YAKIN kapandı)
- **"Kim olacaksın?" ekranı** (GAME_CREATE): Yeni Oyun slotundan sonra, intro'dan
  önce; menü müziği (mühür nabzı) çalar; [Enter] Başla / [ESC] Geri
- **İSİM**: SDL text input (UTF-8/Türkçe, 23 bayt, yanıp sönen imleç, Backspace);
  boşsa "Gezgin"; save v24 ÖZET BLOĞUNA girer → SLOT KARTI BAŞLIĞI artık isim
- **TEN TONU**: 3 seçenek (açık/buğday/koyu) — LPC base katman color-mod,
  her kare uygulanır (yaratma/yükleme farkı yok)
- **GEÇMİŞLER** (hepsi bedelli — trade-off): Gezgin (nötr) / Eski Asker
  (+2 STR + Demir Miğfer; köy itibarı −5) / Avcı (+2 DEX + Yay + 30 ok;
  KILIÇSIZ) / Çırak Simyacı (+2 INT + 2 mana iksiri + 3 Kızılot; −25 altın);
  stat bonusları TÜRETİLMİŞ statlarda (gear ile doğru toplanır);
  player.background kalıcı — ileride diyaloglar okuyabilir
- Yeni oyun reset bloğu yaratma ekranının "Başla"sına taşındı

## 🔜 ROADMAP (2026-07-14 — RPG-ruhu analiziyle onaylandı)

Analiz özeti: sistem omurgası güçlü (build kimliği, ekonomi, suç/itibar, ekoloji,
ahlak ekseni); üç büyük boşluk → (1) hikaye kartları oyunda değil, (2) seçimler
birikmiyor değil ama OKUNMUYOR (kişilik/itibar/mühür diyaloglara dönmeli),
(3) dünya tek yerleşimli, POI/yankı yoğunluğu düşük. Öncelik: önce his + rol,
sonra genişleme, en son ana hattın kapanışı ("coğrafya hikayeyle büyür").

### Faz WE — DÜNYA OLAYLARI ÇEKİRDEĞİ (save v25)
- **worldevent.c**: her şafak %35 zar → günün olayı (aynısı üst üste gelmez);
  durum save v25'te (aktif olay + başlangıç günü + son olay). 4 olay:
  - **Goblin Baskını (WE_RAID)**: gece inince köyün doğu kenarına 2-3 goblin
    (zoneId -1 — ekolojiye sayılmaz, `Mob.raider`). Öldüren köy itibarı +2
    ("Köyü savundun!"); şafakta sağ kalanlar sessizce çekilir (leş/loot yok).
  - **Gezgin Kervan (WE_CARAVAN)**: 2 gün meydanda Kervancı Selim
    (Desert_Trader_1; `npc_force_hidden` API — rutin görünürlüğü ezilir).
    ÖZEL STOK (`CARAVAN_STOCK`): Bilgelik/Kuvvet Taşı, Soluk/Zehirli Öz, Ok,
    Mana İksiri — hepsi %15 pahalı (pazarlıksız kervan). shop.c aktif-stok
    düzenine geçti (render + tıklama + tooltip stok parametreli).
  - **Bereket (WE_BOUNTY)**: o şafak hayvan sürüleri +2 ekstra doğum
    (`ecology_birth_bonus` — tek şafaklık, hayvan aileleri FAM_SHEEP+).
  - **Mühür Sancısı (WE_SEAL)**: gece yankı parıltıları gözle görülür büyür
    (×1.8) + aralıklı fısıltı SFX'i (40-75 sn).
- **Duyuru ağı**: olay sabahı görünür bir köylü haberi bağırır (bark) + pano
  Söylentiler bölümüne olay satırı düşer (worldevent_rumor, en üst sıra).
- **Dinlenme [R]**: gece kamp ateşi başında `[R] Dinlen` → sabah 07:00'a atla
  (gametime_set — day-rollover DOĞAL tetiklenir: olay zarı/ekoloji/otokayıt).
  HP+STA tam, mana en az %50 (uyku doldurmaz, yarılar). %25 şansla RÜYA
  vinyeti: 3 havuz (ruya_muhur / ruya_cukur / ruya_fisilti — mühür/çukur/
  fısıltı imaları, cevap yok). Paneller `scripts/gen_dreams.py` (prosedürel).
- Kervan görünürlüğü yükleme/yeni oyunda senkronize (worldevent_reset + save).

### Faz WE2 — HAVA SİSTEMİ + DÜNYA OLAYLARI KALANLAR (save v26)
- **weather.c**: her şafak zar — Açık %55 / Yağmur %25 / Sis %15 / Fırtına %5;
  öğlen (12:00) %25 şansla hava döner. Yağmur 2-6 oyun saati sürer; sis 05-10
  arası yoğun, öğlen dağılır. `intensity` 0..1 yumuşak geçiş (bastırır/diner).
  Save v26: aktif hava + kalan saat + "bugün yağdı" bayrağı.
- **Savaş etkileri (M1 sinerjileriyle simetrik)**: ıslakta yanma DoT süresi
  ×0.5, Ateş Topu -%15, Yıldırım +%20 (Zehir Patlaması yağmurda altın çağ);
  siste mob algısı ×0.6 (sinsi fırsatı). FIRTINA: 8-20 sn'de bir yıldırım
  düşer (60px yakına: mob 25 / oyuncu 20 hasar) + şimşek flaşı + gök gürültüsü.
- **Çevre etkileri**: köylüler yağmurda evlerine çekilir (npc_set_shelter;
  kervancı gölgelikte kalır, muhafız nöbette ıslanır + bark); dün yağdıysa
  bugün ot toplaması ×2 ("Yağmur toprağa iyi geldi"); yağmurda kamp ateşi
  kurulamaz (köy ateşi çardaklı — çalışır); meşale ışığı ıslakta -%20;
  fırtınada pencere ışıkları titrer. MOB: yağmurda goblin devriyesi durur
  (yuvaya çekilir), hayvanlar su ziyaretine çıkmaz (su ayağına geldi).
- **Görsel/ses**: ekran-uzayı yağmur damlaları (rüzgar eğimli, 140 tavan),
  iki katmanlı sis örtüsü (nefes alır), fırtına karartması (gündüz loşlaşır),
  şimşek beyaz flaşı; amb_rain.wav yağmur katmanı (day/night'ın üstüne biner)
  + thunder.wav (scripts/gen_weather_audio.py — prosedürel).
- **WE_GRAVE — Huzursuz Mezarlık** (5. dünya olayı): gece mühürlü/boş iskelet
  mezarında 3 iskelet yükselir (risen); şafakta 2+ sağsa mezar YENİDEN yuva
  olur (ekoloji kolonisi + pano söylentisi + JE_GRAVE_RISE günlük girişi),
  sağ kalanlar gün ışığında mezara çekilir.
- **Mob arama (MOB_SEARCH)**: leash'te iz kaybeden avcı mob hemen dönmez —
  8 sn olduğu yerde bakınır (IDLE clip, rastgele yön), oyuncu yakına dönerse
  yeniden alarm; süre dolunca kampa döner. Sis + sinsilikle saklambaç derinleşir.
- **Haber ağı diyaloğa indi**: DVAR_EVENT (`event`) + DVAR_WEATHER (`hava`)
  diyalog değişkenleri (salt okunur; koşul tutmayan seçenek GİZLENİR —
  DialogOptState.hidden). Kemal olay yorumları (5 olay), Osman hava sohbeti
  (3 hava), Baran kervan kıskançlığı.
- **KAR (soğuk cephe)** — 5. hava: havuz Açık %50 / Yağmur %22 / Sis %13 /
  KAR %10 / Fırtına %5; 4-10 saat sürer. SAVAŞ: buz büyüleri +%20, buz
  yavaşlatma süresi ×1.5 (Buz Novası 4→6sn, Buz Oku 3→4.5sn), HERKES %12
  yavaş (oyuncu+mob eşit; av +%15 daha) — ateş/yıldırım NORMAL (kuru soğuk,
  Termal Şok karda altın çağ). ÇEVRE: zemin beyaza bürünür (snowCover —
  yağarken ~20sn birikir, dinince ~75sn erir), sürüler büzüşür (devriye
  70→32px), su ziyareti iptal (dondu), ot toplaması yarıya (doğa uykuda),
  kamp ateşi ömrü 24→12 saat, İZ SÜRME: arama modundaki mob karda bakınmaz —
  12 sn boyunca izinden SANA doğru yürür. Görsel: salınan kar taneleri +
  kapalı gökyüzü karartması; Osman kar sohbeti; F8 döngüsünde kar da var.
  Save v26 UYUMLU (enum sona eklendi — bump yok).

### Faz BD2a — BÜYÜ DALGASI 2, PARÇA 1: TOMAR EKONOMİSİ + 3 BÜYÜ (save v27)
- **Tomar ekonomisi**: kayıp büyü tomarları — envanterde sağ tık = oku →
  büyü KALICI açılır (Yıkım seviyesi yetmiyorsa tomar durur, yanmaz; "Harfler
  kayıyor..."). `Player.spellsLearned` bit maskesi (v27); ağaçtan bağımsız —
  tree_compute_effects sonrası her yerde `spells_apply_learned` köprüsü
  (yoksa hotbar_purge_closed öğrenilmiş büyüyü sökerdi). Yeni oyunda sıfırlanır.
- **3 yeni büyü** (magic_pack sheet'leri — 4×4 grid 128px, 16 kare):
  - **Yıldırım Pençesi** (YP, 35 taban/18 mana/5sn): bakış yönüne 110px koni —
    büyücünün panik butonu. Islakta +%20 (bolt mult). Tomar: Yıkım 15, 350g.
  - **Buz Dikenleri** (BD, 26/24/8sn): hedef alanda (100px) yerden buz —
    hasar + %50 yavaş 3sn. Karda +%20 ve yavaş ×1.5. Tomar: Yıkım 30, 550g.
  - **Hortum** (HO, 8×tick/35/14sn): bakış yönüne süzülen alan (65px/sn, 4sn) —
    0.5sn'de bir tick + %30 yavaş; kalkış karelerinde yerinde döner, son 0.4sn
    dağılır. Fırtınada tick ×1.2 + ömür 5sn. Tomar: Yıkım 40, 650g.
- **Kaynaklar**: kervan özel stoğuna Pençe+Diken tomarları (parça 2'de havuz
  rastgeleleşecek); Hortum tomarı YALNIZ zindan USTA sandığı (%25). F10 dev
  seti 3 tomarı çantaya basar.
- İkon atlası 15→18 hücre (gen_ability_icons.py — sheet karelerinden indirme).
- **Parça 2 TAMAM**: Yılan Isırığı (YS, 12/20/7sn — hedefe küçük vuruş + zehir
  DoT %60/4sn zehir kanalına EKLENİR → Zehir Patlaması salt-büyü kombo; yönlü
  snakebite sheet'i hedefte yükselip ısırır) + Ateş Aslanı (AA, 30/28/9sn —
  bakış yönüne 430px/sn hat dalgası 0.7sn, çarptığı HER moba 1 kez vurur
  [hitIds takibi] + Ateş Topu yanma kanalı basar → Termal Şok'u besler;
  ıslakta -%15; yönlü firelion sheet'leri). Tomarlar: Yılan Isırığı Yıkım 20
  (kervan 400g) / Ateş Aslanı Yıkım 50 (YALNIZ boss leşi %35 — haftalık kural).
  KERVAN TOMAR HAVUZU RASTGELE: her kervanda {Pençe, Isırık, Diken}
  havuzundan 2 FARKLI tomar — worldevent startDay tohumlu (save gerekmez,
  ziyaret boyunca sabit, sonraki kervanda değişir). İkon atlası 18→20.
  Item enum'ları SONA eklendi (v28 kayıtların item indeksleri kaymadı).
  BÜYÜ DALGASI 2 KAPANDI — magic_pack'in 5 büyüsü de oyunda.

### Faz ÇANTA — ÇANTA SEKMELERİ: Eşya + Malzeme (save v28)
- **İki çanta, tek panel**: I aç/kapa, TAB sekme değiştirir (Eşya ↔ Malzeme);
  sekme şeridi tıklanabilir (panel üstü 28px bant). Sürüklenen item DİĞER
  sekmenin başlığına bırakılınca taşınır; şeridin sağ ucundaki ÇÖP hücresine
  bırakılınca İMHA edilir. Craft paneline tekerlek kaydırması geldi (uzun
  tarif listeleri — çanta tarifleri artık görünür); pencere 1280×720 → 1600×900.
- **Otomatik ayıklama**: `ItemType.material` bayrağı (13 tür: cevher×2,
  külçe×2, ot×2, odun, jel, kemik, öz×2, deri, çiğ et + İp) — toplama/loot/
  satın alma hammaddeyi Malzeme sekmesine düşürür. Dolu sekme diğerine taşmaz.
- **bag_* katmanı** (main): bag_add/add_stackp (material yönlendirmesi),
  bag_count/consume/consume_n (İKİ çantadan, craft öncelikli) — ~70 çağrı
  noktası bu katmana geçti; craft/kontrat panelleri iki çantayı birden sayar.
- **Kademeler (craft'la kalıcı büyüme)**: Eşya 30 → +10 (Deri Çanta, Smithing
  15: 6 Deri+2 İp) → +10 (Gezgin Çantası, S40: 10 Deri+Demir Külçe) = max 50;
  Malzeme 25 → +15 (Malzeme Kesesi, S10: 4 Deri+2 İp) → +15 (Usta Kesesi,
  S35: 8 Deri+Demir Külçe) = max 55. Çanta eşyası üretilince OTOMATİK takılır
  (envantere girmez; "Zaten takılı" korumalı). İP yeni ara malzeme: kamp
  ateşinde 2 Ayotu → 1 İp. NOT: gümüş oyunda yok — toka = Demir Külçe.
- `Player.bagUpgrades` (bit0/1 Eşya, bit2/3 Malzeme) + Malzeme çantası içeriği
  save v28; Inventory.cap dinamik (INVENTORY_SIZE 60 dizi tavanı);
  MAX_RECIPES 32→48. Başlık "Eşya 12/30" doluluk gösterir.

### Faz TH-A — TWO-HANDED PARÇA A: MIZRAK (v28 uyumlu)
- **Mızrak** (ITEM_SPEAR, enum sonu): mesafe kontrolü silahı — menzil 130
  (kılıç 80; çoğu mobun saldırı menzilinin DIŞI), tempo 1.5sn, taban 34,
  DELİCİ (hedef defansının %25'i yok sayılır, ağaçla %40, tavan %80),
  HAT VURUŞU (saplama çizgisindeki 2. mob %60 hasar; ağaçla %75 + 3. hedef).
  İki el: WEAPON_TWO_HAND — kalkan kilitli. Kaynak: Demirci Ocağı (S20:
  3 Odun + 2 Demir Külçe) ya da vendor 260g. F10 setine girdi.
- **Animasyon**: PLAYER_THRUST (asa dürtmesiyle ortak state) + thrustOversize
  katmanı — salt-thrust sheet (1536×768, 8 kare × 4 yön 192px, ULPC
  thrust_oversize çıktısı; fg z150 + bg z1). Mızrak katmanı thrust dışında
  çizilmez (sheet'te walk satırı yok).
- **Two-Handed ağacı** (T_TWOHAND, 18 düğüm): Delici/Tempo/Hat dalları;
  aktifler AĞAÇTAN — Saplama (×2.2 + geri itme ×2, 8sn/25 STA), Süpürme
  (ön 140° yay ×1.0 + %30 yavaş 2sn — ağaçla %45; 12sn/35 STA; Nova deseni
  ama melee formülü + TH XP). Keystone KAPI TUTUCU: duruş AÇ/KAPA — hız %40
  düşer ama menzile giren mob otomatik hedeflenir + KO auto devreye girer
  (0.8sn tempo tavanı); mızrak çıkınca duruş düşer. combat_weapon_skill
  zaten TWO_HANDED eşliyordu — XP otomatik. PerkEffects: twoHandDmgMult /
  pierceBonus / lineTargetBonus / lineDmgFrac / sweepSlowMult /
  spearStaminaMult / spearRangeBonus. İkon atlası 20→23.
- **Parça B TAMAM — TWO-HANDED + BLOCK PAKETİ KAPANDI**:
  - Kalkanlar: Tahta (def 6, S10 craft: 4 Odun+1 Bakır Külçe, 120g) + Demir
    (def 12, S25: 3 Demir Külçe+2 Deri, 300g). Görsel: ULPC round_shield
    (z110, iki kalkan paylaşır — long_sword deseni). Prosedürel ikonlar.
  - MIZRAK GÖRSELİ TAM: kompozit sheet (1536×5760) — standart satırlar
    (yürüyüş/idle'da mızrak GÖRÜNÜR) + thrust-oversize bloğu @ y=4992
    (long_sword blok düzeni); player.c thrust'ta oversize bloktan örnekler.
    Eski salt-thrust sheet'leri silindi.
  - PASİF BLOK (kalkan takılıyken her zaman): şans %10 + Block lvl×0.2 + ağaç
    (tavan ~%44); blok = hasarın %60'ı emilir (ağaçla %70) + Block XP.
  - AKTİF BLOK (SHIFT basılı): gelen hasar -%70 (ağaçla -%80), hız yarıya
    (ağaçla %35 ceza / FALANKS sıfır), auto-attack durur, 4 STA/sn (stamina
    biterse duruş düşer). PERFECT: duruşun İLK 0.25sn'inde (ağaçla 0.35)
    gelen vuruş → hasar 0 + melee saldırgan 1sn SERSEMLER (parry); SİPER
    keystone: hasarın %50'si yansır. Mermide sersemletme/yansıtma yok.
  - Blok zarı combat_mob_damage'da (tek nokta) — 3 çağrı yeri (mob melee /
    mermi / muhafız) blok türünü işler; block_feedback ortak (yazı+ses+XP).
  - T_BLOCK ağacı (16 düğüm): Şans/Duruş/Emilim dalları + SİPER + FALANKS +
    AKTİF Kalkan Vuruşu (×0.6 + 2sn sersemletme, 10sn/20 STA — CC aracı).
  - Kule Kalkanı: kalkan slot defansına çarpan (equipment_total_defense).
  - F10: Demir Kalkan + Block ağacı puanı. İkon atlası 23→24.

### Faz PP — PICKPOCKET: GÖLGE ELİ (v28 uyumlu; son mekanik paket)
- **Etkileşim**: C eğil + NPC'ye yaklaş + [P] (eğilmişken '[P] Yankesicilik'
  ipucu). Zar: taban %35 + Pickpocket lvl×0.5 + ağaç; eğilme +%25, hedef
  bark'ta (meşgul) +%15, muhafız -%30; AYNI NPC'de her deneme (o gün) -%15
  — sınırsız ama hızla imkansızlaşır (şafakta sıfırlanır). Tavan %90.
- **Keseler** (role göre): Kervancı 25-60g +%20 öz · vendor 15-40g +%20
  iksir · muhafız 10-25g +%25 Maymuncuk (ironik) · Nuh/Fatma 3-8g (vicdan
  testi) · köylü 5-15g. Başarı persKetum +1 (kişilik izi) + 25 XP (sessiz).
- **Yakalanma**: hedef tanıktır — bounty +15 (ağaçla -%50) + itibar -3 +
  'Hırsız! HIRSIZ var!' bark'ı + lastCrimeDay; MUHAFIZIN cebinde yakalanmak
  = anında yakalama paneli. Başarısızlık da 8 XP (denemek öğretir).
- **T_PICKPOCKET mini ağacı** (10 düğüm): Şans (+%3/+%5/+%5/+%10) · Kese
  (altın +%25×2, item şansı 2x) · Sessizlik (bounty -%50, MASUM YÜZ: günde
  bir başarısızlık fark edilmez) · keystone GÖLGE ELİ (başarıda hedefin
  eşyası da çalınır — ek zar). F10 Pickpocket puanı basar.
- MEKANİK PAKETLER TAMAM: roadmap'in savaş/yaşam eksenindeki tüm onaylı
  sistemler oyunda (M1-M4, Dünya Olayları+hava, Büyü D2, çanta, TH+Block, PP).

### Faz KM — KİRLİ MAL & AKLAMA (save v29)
- **Kirli kese**: yankesicilik altını `player.dirtyGold`'a düşer — HARCANAMAZ
  (HUD'da altının yanında kırmızı "(+N kirli)"). Çalıntı eşyalar
  `ItemStack.stolen` damgalı (tooltip kırmızı ÇALINTI satırı): kuşanılamaz,
  satılamaz, tüketilemez, craft/kontrat SAYMAZ (inventory_count/consume
  çalıntıyı atlar; kirli-temiz aynı yığına girmez).
- **Keseler ×2 + değerli objeler**: Kervancı 50-120g (%15 Cep Saati 150g) ·
  vendor 30-80g (%20 iksir, %15 Yüzük 80g) · muhafız 20-50g (%25 Maymuncuk,
  %10 Broş 100g) · köylü 10-30g (%8 Yüzük) · Nuh/Fatma 5-15g. Objeler
  YALNIZ çalıntı bulunur — normal ekonomide yeri yok, aklayıcı tam öder.
- **MÜSADERE**: yakalama panelinde ceza ödersen kirli altın + tüm çalıntılar
  da gider ("kanıt") — biriktirme gerilimi (push your luck).
- **AKLAMA — iki kanal**:
  - Kervancı Selim "el altından" (kervan günleri, diyalog): kirli altın
    ×0.75, çalıntı eşya ×0.6, DEĞERLİ OBJE ×1.0 — sabırlının kanalı.
  - KÖR KUYU (köy dışı T26,15 — Well.png decor): yalnız GECE [E] — hepsi
    kuyuya, şafakta ×0.5 temiz kese ("Sayılıyor. Hepsi sayılıyor." fısıltısı).
    Aceleci kanal; kim aldığı İLERİDE: yeraltı örgütünün tohumu.
  - Ağaç: Keseci/Güvenilir El düğümleri artık AKLAMA kesintisini düşürür
    (kervan 0.75→0.85, kuyu 0.50→0.60).
- Save v29: ItemStack.stolen (write/read_stack +1 alan) + dirtyGold +
  wellGold/wellDay.

### Faz YK1 — YANKICI: GİZLİ SINIF, PARÇA 1 (v29 uyumlu — storyFlags)
- **Tamamen gizli**: SKILL_ECHO ("Yankı") skill defterinde/K panelinde ritüel
  tamamlanana dek YOK (skill_set_echo_visible — satır enum sonunda, Kader
  eşlemesi korunur). İpucu yok, '???' yok — yalnız fısıltı imaları.
- **Keşif (üç kapı)**: ① 3 yankı SÖMÜR (Tanık kalan hiç görmez) → fısıltı
  değişir: "Taşların eskisine gel. Ay görmesin." (JE_ECHO_CALL) ② GİZLİ TAŞ
  HALKASI: Karanlık Orman derinliği T(108,7) — haritada işaretsiz, prosedürel
  ritual_stones decor'u ③ GECE taşlarda [E] → RİTÜEL vinyeti (2 panel,
  kabul/reddet — reddedersen kapı açık kalır). Kabul: SF_YANKICI=2,
  splash "YANKICI", beceriler açılır, Fısıltı Keskisi taşın dibinde belirir
  (JE_RITUAL: "yabancı gelmiyor").
- **FISILTI KESKİSİ** (unique — satılmaz/craftlanmaz): hançer 16 dmg/0.7sn;
  EKSİK canın her %10'u +%8 hasar (riskle keskinleşir — beceriler can yakar:
  sinerji); vuruşlar Yankı XP'si de akıtır; ritüelsiz kuşanılamaz ("Sapı
  avucunda ISLANIYOR sanki").
- **Beceriler (bedel MANA DEĞİL KAN — AbilityDef.hpFrac)**:
  - Fısıltı Patlaması (FP, 8sn): max HP %10 → 130px gölge dalgası (taban 30 +
    Yankı lvl ×%2); dark_burst animasyonu (48×64, 16 kare). "Kanın yetmiyor!"
    koruması (HP eşiği altında atılmaz).
  - Gölge Formu (GF, 20sn): 5sn algı ×0.05 + hız +%20; 4 HP/sn yakar,
    5 HP'de düşer.
- **F10 test yolu**: sınıfı AÇMAZ — sömürü sayacını 3'e basar, fısıltı çağrısı
  gelir; taşları bulup ritüeli yine oyuncu yapar (keşif korunur).
- Save bump YOK: SF_YANKICI storyFlags[7] (v22 bloğu), echoesConsumed zaten
  kayıtlıydı, shadowTimer runtime. Parça 2 (sonra): Can Emme + Yankı Çağırma
  + T_ECHO ağacı + Dark VFX bolt.

### Faz HG-A — HARİTA GÖRSEL YENİLEME, FAZ A: DÜNYA (v29 uyumlu)
- **Kenar geçişleri (autotile)**: çimen komşu tabana "TAŞAR" — grass_edges.png
  atlası (12 hücre: 4 kenar + 4 dış köşe + 4 iç köşe; gen_tile_edges.py
  Grass_1_Middle'dan DALGALI alpha maskeyle üretir — jilet kenarlar bitti).
  Render: tile çizilirken 4 komşu + 4 çapraz bakılır; yalnız dünya dış mekan
  tabanlarına (yol/su/bataklık/diken) taşar — zindan dokunulmaz.
- **Çimen 4 varyant**: hash(col,row) deterministik — Grass_1..4_Middle
  serpiştirilir, tekdüzelik kırıldı (save'e girmez, hep aynı görünür).
- **Eksik dokular**: taş/bataklık/diken düz renkten kurtuldu (prosedürel
  16px dokular — assets/tiles/terrain/).
- **Süs serpiştirmesi** (tilemap_render_scatter — çarpışmasız, deterministik):
  çimende %6 çiçek/ot (flowers.png 10×10), yol kenarı çimende %10 küçük taş
  (Path_Decoration), su kıyısında %8 saz (Water_Decoration). Tile içi kayma
  hash'le — grid hissi yok. Yalnız dünya haritası.
- **Köy el yerleşimi**: fıçı kümeleri (dükkan+han), meydan/yol bankları,
  ocak arkası taş çit — rutin yollarının dışında.
- Faz B (sıradaki): LANETLİ MEZARLIK haritası (undead tileset).

### Faz HG-B — LANETLİ MEZARLIK HARİTASI (v29 uyumlu; MapId append-safe)
- **MAP_MEZARLIK** (60×45, açık hava zindanı): undead tileset paleti — ölü
  toprak [12] (2 varyant) + dikenli kaya [13] + ölü su [14]; gen_mezarlik.py
  sabit-tohumlu üretir (sınır kayalıkları, iç siperler, doğu gölcüğü).
- **Zindan MEKANİĞİ, açık hava GÖRÜNÜMÜ**: map_is_dungeon=true → mühür
  dalgası respawn, sabit alacakaranlık (ambient 0.45), MOOD_DUNGEON, NPC/hava
  kapalı. Boss YOK (Bringer-of-Death sonraki mob dalgasında buraya oturacak).
- **Portal**: Kayalık Geçit mezar sahaları yanı T(135,88) ↔ mezarlık güney
  girişi T(30,41); giriş görseli skull_door.
- **Map-bazlı dekor**: DecorDef.map alanı (0=dünya — eski girdiler değişmedi);
  render/collision/conceal filtreli; decor_set_map geçişte basılır. Mezarlık:
  DEV İSKELET KALINTISI (Lich, merkez landmark) + mezar taşları + kurumuş
  ağaçlar + harabe + kemik/kafatası yığınları + kristal.
- **İçerik**: MZ_CAMPS 5 iskelet kampı (savaşçı/okçu/büyücü, mühür dalgalı);
  2 sandık — kuzeybatı harabe (Sağlam) + dev iskeletin ardı (USTA → Hortum
  tomarının 2. kaynağı).

### Faz HB-WOW — HOTBAR WoW DÜZENİ (32) + TAB HEDEFLEME + TIKLA-BAS
- WoW düzeni, hepsi hep görünür (sıra/F-tuşu kavramı YOK — Hakan'ın evrilen
  isteği): 2 ana bar 8×1 alt alta (1-8 / Shift+1-8) + sağ dikey 2×4
  (Alt+1-8) + sol dikey 2×4 tuşsuz. slot_rect() tek kaynak (çizim+hit-test).
  Save count-toleranslı — eski 8/30'luk kayıtlar oturur, taşan düşer
  (açık aktifler place_unlocks ile geri gelir).
- TIKLA-BAS: dolu hücreye düz sol tık = tuşla basmakla aynı akış; bar
  üstündeki her tık yutulur (dünyaya geçmez). Taşıma/takas YALNIZ
  SHIFT+sürükle; kaldırma yalnız sürüklerken beliren ✕ hedefine (üst barın
  üstünde ortada) — dışarı bırakmak İPTAL (yanlışlıkla beceri atılamaz).
- Silah uygunluğu görseli: mevcut silaha uymayan beceri barda SOLUK
  (abilityUsable[] main'de her kare; büyüler silahtan bağımsız hep aktif).
- TAB: mob hedefleme (Z KALDIRILDI) — panel önceliği: envanter açıkken çanta
  sekmesi, günlük açıkken günlük sekmesi, yoksa hedef.
- K panelinde ALINMIŞ aktif düğümü bara sürükle = ata; item→hotbar yol korunur.
- NOT: "meslek paneli kısayolu barda" fikri KONUŞULDU ve VAZGEÇİLDİ (craft
  istasyona bağlı tasarımla çelişiyor; ileride Günlük'e Tarifler sekmesi olabilir).
- Crash fix: DECOR 37 > 32 texture dizisi taşması (DECOR_TEX_CAP 64 ortak
  sabit + döngü korumaları + aşım logu); portalTex 8→16 (tam doluydu).

### Faz VFX-1 — VFX HİS CİLASI (tam paket, onaylı sıra ③)
- **AnimFx havuzu** (fx.c/h): genel grid-sheet animasyon havuzu (MAX_ANIM_FX 48,
  satır-major, bir kez oynar söner) — animfx_init/spawn/update/render.
- **İsabet efektleri**: strike_fx_mob köprüsü (g_animFx deseni) — melee'de
  weaponhit (6×6@100), büyüde magickahit (7×7@100) + HER isabette küçük kan
  sıçraması (Hakan onayı); 17 hasar noktasına tek satır çağrı (DoT/yansıtma
  tick'leri hariç — spam yok).
- **Ölüm**: mob_die içinde büyük kan sıçraması + 90ms hit-stop (tüm ölüm
  yolları tek merkezden).
- **HIT-STOP**: g_hitStop — melee 50ms, ok/hat 30ms, ölüm 90ms, PERFECT parry
  120ms; dünya dt'si ×0.08 kısılır (menü/ölüm ekranı gerçek zaman).
- **Blok kıvılcımı**: 3 block_feedback noktasında (guard/mob/proje) sarı çarpma.
- **Cast halkası**: kanalizasyon boyunca ayak altında dönen çember (81 kare).
- **Slash yayı**: savurma başlangıç karesinde yöne göre yay (4 yön sheet'i
  90° döndürmeyle üretildi — slash_up/down/left/right.png).
- Assetler: Free Pixel Effects Pack (public domain) + Gigapack splatter/impact
  + Frostwindz Slash 1 → assets/fx/; CREDITS.md güncellendi.
- UX FIX (aynı commit): otomatik kayıttan yüklemede F5 "Aktif slot yok"
  uyarısında bırakıyordu — artık kayıt slotu ekranını doğrudan açar (seçilen
  slot aktif olur); F9 slotsuz durumda bildirim basar.

### Faz MÜZİK — GERÇEK MÜZİK SETİ (xDeviruchi, onaylı sıra ④)
- **stb_vorbis vendored** (src/stb_vorbis.c, public domain; CMake'te -w —
  vendored uyarıları bize sayılmaz). music.c önce assets/audio/music/*.ogg
  dener, yoksa eski prosedürel WAV'lara düşer (bozulmaz).
- **Eşleme (Hakan onayı)**: MENÜ=Title Theme · KÖY=Definitely Our Town ·
  KIR=Where The Winds Roam · GECE=Silent Forest · TEHLİKE=Calm Before The
  Storm · ZİNDAN/MEZARLIK=Lost Shrine · SAVAŞ=Battle 1 · BOSS=Decisive
  Battle 1 (YENİ MOOD_BOSS — aggro boss COMBAT'ı ezer, enum SONA eklendi).
- **LOOPSTART desteği**: intro'lar loop dışı (dokümandaki tablo) — ilk besleme
  intro dahil, sonraki beslemeler LOOPSTART byte ofsetinden; mood'a her
  girişte intro yeniden çalar (savaş girişi vurgulu).
- Lisans: atıf ZORUNLU — "Original music by Marllon Silva (xDeviruchi)"
  oyun kredilerine girecek (CREDITS.md'de hazır).
- 8 OGG ~37MB; crossfade/duck/volume mevcut sistemden aynen çalışır.

### Faz MOB-2 — MOB DALGASI + ÖLÜM GETİREN (onaylı sıra ⑤)
- **3 yeni tür (SONA — 21/24 MAX_MOB_TYPES)**: [18] Hortlak Büyücü (Evil
  Wizard 2 — mezarlık eliti, ATTACK_MAGIC 30 dmg), [19] Kan Canavarı (Blood
  Monster — ölü su, hızlı melee sürüsü), [20] ÖLÜM GETİREN (Bringer —
  isBoss, lv16 HP1600). Yan görünüşlü kompozit sheet'ler (3 yön aynı klip,
  flip motorda); Wizard kareleri aynalandı.
- **Boss savaşı (Hakan onayı — 2 fazlı)**: dev iskelet kalıntısı önünde
  T(30,20); Faz 1 tırpan + 4sn'de bir KARA PATLAMA (oyuncu altında 1sn
  büyüyen mor halka telegraph → 45 hasar, kaçılabilir); Faz 2 (%50) hız
  ×1.35 (slow kanalı ters kullanım) + BİR KEZ 2 İskelet Savaşçı + çift
  halka. Haftalık diriliş. MOOD_BOSS müziği otomatik.
- **Ödül**: İLK ölümde ÖLÜM GETİREN'İN NİŞANI garanti (unique kolye,
  EQUIP_NECKLACE ilk kullanımı): +%10 hasar (trinketDmgMult — melee+büyü,
  combat.c) + her öldürmede 8 HP (mob_die, g_eqBridge köprüsü). Sonraki
  ölümler: kemik+HP taşı+soluk öz garantili, tomar %60.
- **SAVE: toleranslı kuyruk deseni (YENİ)**: boss2 diriliş kaydı dosya
  SONUNA eklendi — eski v29 save'ler varsayılanla açılır, sürüm bump'sız.
  mob_die boss bloğu Lord/Bringer ayrımına bölündü.
- MZ_CAMPS +3 (2 Hortlak kampı + güney ölü su Kan Canavarı yuvası).
- SATYR + Demon_A sonraki dalgaya kaldı (backlog).

### Faz HOLY-2 — NUR YOLU AĞACI (Hakan onayı — tam 18 düğüm)
- T_HOLYTREE + TREE_DEF(SKILL_LIGHT): Şifa / Işık / Koruma şeritleri + AZİZ
  (şifa+%30, hasar+%15, stamina -15) ve IŞIK TAŞIYICISI (alan 4 HP/sn +
  alanda blok %20 güçlü, Alan CD +6sn) keystone'ları.
- PerkEffects +11 alan (holyDmgMult/healMult/holyManaMult/healCdCut/
  holyUndeadBonus/holyZone* — nötr init'e mult'lar eklendi).
- Uygulama noktaları: mana maliyeti (2 yer), CD ayarı (Rahibe/bedel), şifa
  gücü, hüzme çarpanları (undead 1.6+bonus), alan yarıçap/süre/tick/self-heal
  (holyZone.r alanı), 3 blok noktasında alan-blok sinerjisi.

### Faz HOLY — KUTSAL HATTI (onaylı sıra ⑥, Hakan onayları: 3 büyü + tomar/tapınak + denge)
- **SKILL_LIGHT "Kutsal"** (enum sona): XP = şifa ×1.2 + kutsal hasar.
  Panel satır eşlemesi düzeltildi (row_skill — ECHO gizliyken atlanır; eski
  "son satırı kes" varsayımı LIGHT eklenince kırılıyordu).
- **3 büyü** (ABILITY enum sona — spellsLearned 29/32 bit DOLULUK UYARISI):
  ŞİFA NURU (30 mana → 35 HP taban, INT+Kutsal ölçekler, CD 8sn) ·
  KUTSAL HÜZME (25 mana, 26 taban, UNDEAD ×1.6, CD 5sn, hedefli) ·
  KUTSAL ALAN (40 mana, CD 20sn; 6sn/120px çember — undead %25 yavaş +
  7/sn tick; tek çember, yeniden atım taşır).
- **MobType.undead** alanı: iskelet ailesi + Lord + Hortlak + Bringer işaretli.
- **Tomar gate GENELLEŞTİ**: okuma koşulu ABILITIES[ta].skill seviyesine bakar
  (eskiden Yıkım'a sabitti) — mevcut tomarlar etkilenmez (hepsi Yıkım).
- **Kaynaklar**: Şifa Nuru = kervan SABİT stok + FATMA HEDİYESİ (K9 tapınak
  kararı verenlere son1_tomar düğümü, SF_K9_TOME ile bir kez; çanta doluysa
  bayrak yanmaz) · Hüzme/Alan = Hortlak Büyücü %8/%6 + USTA sandık %15 havuzu.
- İkon atlası 24→29 hücre (gen_ability_icons.py — Yankıcı 24-25 boşluğu da
  dolduruldu) · Holy VFX sheet'leri assets/fx/ · F10 kitine 3 tomar.

### ASSET SIRASI (2026-07-17 — Hakan'ın indirdiği paketler, onaylı işleme sırası)
İndirilenler (~/Downloads):
- **Harita**: Free-Undead-Tileset (açık hava lanetli arazi — LANETLİ MEZARLIK
  haritası onaylı) + Cute Fantasy tile/dekor (dünya görsel yenileme ONAYLI:
  autotile geçişler + varyant + doku + dekor serpiştirme)
- **VFX**: Super Pixel Effects Gigapack (Lightning/Bursts/Spells/Explosions/
  Impacts) + Free Pixel Effects Pack (20 sheet: weaponhit/magickahit/freezing/
  felspell/phantom/flamelash...) + Brackeys VFX + Pixel Fire v1.2 + Ice Effect
  01 + Warped Shooting FX + Holy VFX 01-02 + Dark VFX 01-02 (Yankıcı'da kısmen
  kullanılıyor)
- **Mob/Boss**: Skeleton enemy · Evil Wizard 2 · Bringer-of-Death (boss adayı)
  · SATYR · Tiny RPG Demon_A + Blood Monster_A (2 saldırılı — elit/mini-boss)
- **Müzik**: xDeviruchi 16-bit Fantasy & Adventure (loopable — gerçek müzik seti)
- **UI**: Pixel UI pack 3 (panel/çerçeve yenileme)
- **Diğer**: Dimensional_Portal.png (geçit/Perde 2 görseli) · Destructible
  Objects sheet (kırılabilir varil/kasa — harita fazında loot objesi mekaniği
  adayı) · Pixel Art Slashes (melee vuruş yayları — VFX cilasında weaponhit
  ile birlikte) · 'Free' Part 16-23 (renk varyantlı patlamalar) · DG Fire Zone (volkan/ateş
  bölgesi tile'ları — ileriki ateş biyomu adayı)

ONAYLI SIRA: ① Dünya görsel yenileme (BAŞLADI — keşif bitti) → ② Lanetli
Mezarlık haritası → ③ VFX his cilası (tüm savaş efektleri) → ④ Müzik seti
değişimi → ⑤ Yeni mob dalgası + boss → ⑥ Kutsal hattı (Holy) → UI yenileme
uygun araya. YANKICI Parça 1 iskeleti kodda (tamamlanacak).

### YAKIN — "oyun gibi hissettir" (his + rol)
1. ~~CİLA PAKETİ~~ TAMAM (2026-07-14 — bkz. Faz CİLA bölümü); kalan küçük
   UX borcu: speech zar kilidi save'i (uygun faza serpilir)
2. **HİKAYE KARTLARI DALGA 1** (K9+K2+S3 seçildi 2026-07-14; kart başına commit):
   ~~D1a: sahne araçları + K9 Kandil~~ TAMAM (bkz. Faz D1a bölümü)
   ~~D1b: K2 Son Müşteri~~ TAMAM (bkz. Faz D1b bölümü)
   ~~D1c: S3 Sayan Adam~~ TAMAM (bkz. Faz D1c) — **DALGA 1 KAPANDI**
   + kalan alışkanlık: kişilik+itibar sayaçlarının mevcut köy diyaloglarına
     serpilmesi (`req: kaba >= 3` — her dokunulan diyalogda sürer)
3. ~~KARAKTER YARATMA-LİTE~~ TAMAM (bkz. Faz KY) — **YAKIN KADEMESİ KAPANDI**

### ORTA — "dünyayı büyüt"
4. **İKİNCİ YERLEŞİM + İLK YENİ BİYOM** (çöl ya da mantar): yeni NPC'ler +
   bölgesel ekonomi (fiyat farkları → ticaret), yeni ekoloji aileleri
   (Bombschroom/Mummy vb. buraya), POI + yankı serpiştirme, kartlar 2. dalga;
   mob fazları devamı bu şemsiyede (Shroomling → Mummy/Cowling/Flying Skull;
   kırmızı/sarı/pembe slime; iskelet "mezardan kalkma" animasyonu)
5. **ONAYLI 4 MEKANİK** (2026-07-11) — MEKANİK-ÖNCELİKLİ hat (2026-07-14 pivotu:
   sıra öne çekildi; savaş+yaşam üçlüsü: M1 sinerji ✓ → M2 pişirme → M3 ünvanlar):
   · ~~Element sinerjileri~~ TAMAM (bkz. Faz M1)
   · ~~Pişirme + yemek buffları~~ TAMAM (bkz. Faz M2)
   · ~~Davranış ünvanları~~ TAMAM (bkz. Faz M3) — savaş+yaşam üçlüsü KAPANDI
   · ~~Summon hattı~~ TAMAM (bkz. Faz M4) — ONAYLI 4 MEKANİK TAMAMLANDI
   · Summon hattı (yılan DoT + kaplumbağa tank; bedel: max mananın %25'i rezerve;
     sprite'lar: `~/Downloads/snake_right.png, turtleshell_right.png`)
   · Pişirme + yemek buffları (kamp ateşi + av döngüsü)
   · Davranış ünvanları ("Slime Kasabı" — kimlik davranıştan doğar vitrini)
6. ~~**DÜNYA OLAYLARI tam sistemi**~~ TAMAM (bkz. Faz WE + WE2): çekirdek,
   hava sistemi, haber ağı diyalogda, Huzursuz Mezarlık, mob arama

### UZAK — "derinleştir ve kapat"
7. **ANA HAT PERDE 2**: Lord sonrası fısıltının çağrısı + ikinci büyük zindan/boss
   (boss scroll/taş dropları burada anlam kazanır)
8. **Kalan sistemler**: Two-Handed hattı (spear — asset:
   `~/Downloads/lpc_male_item_animations_2026-07-11T13-46-16`; Block/kalkan aynı aile),
   ~~Pickpocket~~ TAMAM (bkz. Faz PP), Büyü dalgası 2 ~~parça 1 (pençe/diken/hortum + tomar)~~ TAMAM (bkz. Faz BD2a) — ~~kalan parça 2~~ TAMAM (magic_pack kalanları:
   tornado/firelion/icetacle/lightningclaw/snakebite), kalan saga'lar + finale bağları
9. **YAYIN HAZIRLIĞI**: F10 söküm, CREDITS lisans teyitleri, Vulkan/SPIR-V,
   mobil build (SDL_GPU→Metal/Vulkan), harita editörü akışı

Ertelenen küçükler (uygun faza serpilir): yay walk/idle görseli (asset gelince),
Combat'a aktif savunma sorusu (dodge/block — Two-Handed/Block fazında tartışılır).

## Karar defteri (değişmez)
- Sınıf yok; kimlik davranıştan: use-based skill + perk (3-seç-1) + bölge/hizip itibarı (global karma YOK)
- Suç = tanık varsa; tam muhafız döngüsü onaylı
- Bölge-sabit zorluk, level-scaling yok · Combat A (ıska yok) · KO auto-attack + hotbar
- Perk eşikleri 25/50/75/100 · Ölüm: köyde diriliş + %10 altın
