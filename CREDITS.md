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

### LPC Hand Tools (kazma/balta/çekiç smash katmanları)
- Yapımcılar: bluecarrot16, Pierre Vigier, Tuomo Untinen, Johannes Sjölund (wulax), Inboxninja
- Kaynak: https://opengameart.org/content/lpc-hand-tools
- Dosyalar: `assets/equipment/weapon/pickaxe_*.png, axe_*.png, hammer_*.png`
- Lisans: CC-BY-SA 3.0 (axe/pickaxe/hammer ayrıca OGA-BY 3.0 seçeneğiyle —
  mobil dağıtımda OGA-BY 3.0 tercih edilebilir, atıf şart)

## Font
- **Public Pixel** (`assets/fonts/PublicPixel-rv0pA.ttf`) — GGBotNet,
  CC0 / public domain. https://ggbot.itch.io/public-pixel-font

## Ses
- `assets/audio/*.wav` — bu proje için prosedürel üretildi (script),
  lisans sorunu yok.

## Kütüphaneler (vendored)
- SDL3, SDL3_ttf — zlib lisansı (`vendor/`)
- stb_image — public domain / MIT (`src/stb_image.h`)
