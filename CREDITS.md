# Asset Credits

Pixe Game'de kullanılan üçüncü parti varlıklar. Mobil/ticari dağıtımdan önce
her satırın lisansı teyit edilmeli (TODO işaretliler özellikle).

## Sprite'lar

### LPC / Universal LPC Spritesheet (oyuncu paper-doll)
- Kaynak: [Universal LPC Spritesheet Generator](https://sanderfrenken.github.io/Universal-LPC-Spritesheet-Character-Generator/)
- Dosyalar: `assets/characters/body|head|face/`, `assets/equipment/`
- Lisanslar: parça başına değişir (CC-BY-SA 3.0 / GPL 3.0 / OGA-BY 3.0 karışık)
- **Longsword** (`assets/equipment/weapon/long_sword_*.png`): OGA-BY 3.0
- TODO: generator'ın ürettiği credits çıktısını parça parça buraya ekle
  (generator "download credits" verir — mobil hedef için şart).

### Cute Fantasy RPG (NPC'ler, evler, hayvanlar, ikonlar, pano)
- Yapımcı: Kenmi — https://kenmi-art.itch.io/cute-fantasy-rpg
- Dosyalar: `assets/characters/npcs/`, `assets/buildings/`,
  `assets/characters/mobs/Sheep|Pig|Boar|Chicken.png` (Animals),
  `assets/characters/mobs/Slime_*.png` (Enemies/Slime — 2 renk × 3 boy),
  `assets/characters/mobs/Skeleton_*.png` (Enemies/Skeleton — Swordman/Bowman/Mage),
  `assets/icons/items/food.png` (Food Icons), `assets/decor/notice_board.png` (Signs),
  `assets/decor/campfire.png` (Outdoor Decor Animations)
- TODO: kullanılan sürümün (ücretsiz/paid) lisans metnini teyit et;
  ücretsiz sürüm kredi ister, ticari kullanım için paid lisans gerekebilir.

### Mob sprite'ları (Goblin, Orc)
- Dosyalar: `assets/characters/mobs/`
- TODO: kaynak/lisans kaydı buraya (indirildiği sayfa bulunup eklenecek).

### Bows and Arrows ikonları (yay/ok envanter ikonları)
- Kaynak: `~/Downloads/opengameart bows/` paketi (Bows.png, Arrows.png kırpmaları)
- Dosyalar: `assets/icons/items/bow_icon.png, arrow_icon.png`
- TODO: opengameart sayfası + yazar + lisans teyidi (mobil dağıtım öncesi şart).

### ULPC Great Bow + nişan oku (yay paper-doll katmanları)
- Yapımcılar: wulax (temel yay), daneeklu (great bow), pvigier (walk animasyonları),
  bluecarrot16 (katman ayrımı, v3 taban uyarlaması)
- Kaynaklar: https://opengameart.org/content/lpc-weapons-two-bows-a-spear-and-a-trident
  ve https://opengameart.org/content/lpc-walk-animations-for-bows
- Dosya: `assets/equipment/weapon/great_bow_140.png`
  (tam sheet'e bizce kompoze edildi; yalnız shoot satırları çizili, nişan oku gömülü)
- Lisans: LPC parçaları (CC-BY-SA 3.0 / GPL 3.0 karışık — generator credits çıktısı
  `lpc_male_item_animations_2026-07-10T17-38-07/credits/` klasöründe)

### ULPC Simple Staff (asa paper-doll katmanları)
- Kaynak: Universal LPC Spritesheet Generator ("simple staff")
- Dosyalar: `assets/equipment/weapon/staff_140.png, staff_009.png`
  (spellcast+thrust+walk satırları tam sheet'e kompoze edildi)
- Lisans: LPC parçaları (generator credits çıktısı
  `lpc_male_item_animations_2026-07-11T13-45-19/credits/` klasöründe)

### LPC Hand Tools (kazma/balta/çekiç smash katmanları)
- Yapımcılar: bluecarrot16, Pierre Vigier, Tuomo Untinen, Johannes Sjölund (wulax), Inboxninja
- Kaynak: https://opengameart.org/content/lpc-hand-tools
- Dosyalar: `assets/equipment/weapon/pickaxe_*.png, axe_*.png, hammer_*.png`
- Lisans: CC-BY-SA 3.0 (axe/pickaxe/hammer ayrıca OGA-BY 3.0 seçeneğiyle —
  mobil dağıtımda OGA-BY 3.0 tercih edilebilir, atıf şart)

### Büyü efektleri (Faz 22 — küre/patlama/alev/kalkan/diken sprite'ları)
- Dosyalar: `assets/fx/orb_fire.png, orb_blue.png, orb_earth.png` (Sog paketi),
  `assets/fx/arcane_burst.png` (Arcane_Effect paketi),
  `assets/fx/flame_small.png` (flames paketi),
  `assets/fx/iceshield.png, spikes.png` (magic_pack)
- Kaynak klasörler: `~/Downloads/Sog.png, Arcane_Effect, flames, magic_pack`
- TODO: her paketin indirildiği sayfa + yazar + lisans teyidi
  (mobil dağıtım öncesi şart — magic_pack LPC uyumlu görünüyor).

### Conjuration dost sprite'ları (M4)
- Dosyalar: `assets/characters/allies/snake.png, turtle.png`
  (128px belirme/hamle efekt sheet'leri — magic_pack ailesi)
- Kaynak: `~/Downloads/snake_right.png, turtleshell_right.png`
- TODO: paket sayfası + yazar + lisans teyidi.

### Büyü ikonları (hotbar/ağaç beceri ikonları)
- Dosya: `assets/icons/abilities.png` (magic_icons.png hücrelerinden türetildi,
  kılıç/yay glifleri prosedürel — `scripts/gen_ability_icons.py`)
- Kaynak: `~/Downloads/magic_icons.png`
- TODO: paketin indirildiği sayfa + yazar + lisans teyidi.

## Font
- **Public Pixel** (`assets/fonts/PublicPixel-rv0pA.ttf`) — GGBotNet,
  CC0 / public domain. https://ggbot.itch.io/public-pixel-font

## Ses
- `assets/audio/*.wav` — bu proje için prosedürel üretildi (script),
  lisans sorunu yok.

## Kütüphaneler (vendored)
- SDL3, SDL3_ttf — zlib lisansı (`vendor/`)
- stb_image — public domain / MIT (`src/stb_image.h`)

## VFX (his cilası fazı, 2026-07-17)
- `assets/fx/hit_weapon.png`, `hit_magic.png`, `cast_circle.png` —
  **Free Pixel Effects Pack** (CodeManu / @DavitMasia, Pixel FX Designer).
  Public domain, kredi gerekmez. Kare 100×100, grid satır-major.
- `assets/fx/splat_small.png` (320×32, 10 kare), `splat_big.png` (640×64,
  10 kare), `spark_block.png` (336×48, 7 kare) — **Super Pixel Effects
  Gigapack (Free Version)** (itch.io). TODO: yazar sayfası + lisans teyidi.
- `assets/fx/slash_up/down/left/right.png` (640×64, 10 kare 64px) —
  **Pixel Art Animations - Slashes** (Frostwindz, itch.io) "Slash 1/color1"
  sheet'inden yön varyantları üretildi (90° döndürme/ayna). Lisans:
  paketteki Frostwindz Asset License Agreement (ticari kullanım serbest,
  yeniden satış yasak).

## Müzik (Müzik fazı, 2026-07-18)
- `assets/audio/music/*.ogg` (8 parça: menu/town/wild/night/danger/dungeon/
  combat/boss) — **"16-Bit Fantasy & Adventure Music (2025)"**.
  **Original music by Marllon Silva (xDeviruchi)** —
  https://www.youtube.com/@xDeviruchi
  Lisans: pakete dahil non-exclusive lisans (ticari kullanım serbest, atıf
  ZORUNLU — oyun kredilerinde yukarıdaki ibare kullanılacak; orijinal
  dosyaların yeniden dağıtımı/satışı yasak). Loop noktaları dokümandaki
  tablodan music.c'ye işlendi (LOOPSTART).
- `src/stb_vorbis.c` — Sean Barrett, public domain (OGG decoder, vendored).

## Mob/Boss sprite'ları (Faz MOB-2, 2026-07-18)
- `assets/characters/mobs/Bringer_of_Death.png` — **Bringer of Death**
  (Clembod, itch.io). Lisans: ticari kullanım serbest, yeniden dağıtım/satış
  yasak (paketteki License.txt). Kompozit sheet Individual Sprite'lardan
  PIL ile üretildi (idle/walk/attack/death satırları); kareler AYNALANDI
  (doğal bakış sol → motor sağ bekler).
- `assets/characters/mobs/Ghoul_Wizard.png` — **Evil Wizard 2** (LuizMelo,
  itch.io; CC0 — License.txt teyitli). Doğal bakış zaten sağ — aynasız.
- `assets/characters/mobs/Blood_Monster.png` — **Tiny RPG Character Asset
  Pack 02** (Blood Monster_A; itch.io ücretsiz sürüm). TODO: yazar adı +
  lisans sayfası teyidi.
- `assets/icons/items/death_sigil.png` — prosedürel üretildi (Nişan ikonu).

## Kutsal VFX (Faz HOLY, 2026-07-18)
- `assets/fx/holy_burst.png` (16 kare @48), `holy_impact.png` (7 @32),
  `holy_beam_init.png`, `holy_beam_mid.png` — **Holy VFX 01-02** (itch.io).
  TODO: yazar sayfası + lisans teyidi (paket arşivinde lisans dosyası yok).
