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

### Cute Fantasy RPG (NPC'ler, evler)
- Yapımcı: Kenmi — https://kenmi-art.itch.io/cute-fantasy-rpg
- Dosyalar: `assets/characters/npcs/`, `assets/buildings/`
- TODO: kullanılan sürümün (ücretsiz/paid) lisans metnini teyit et;
  ücretsiz sürüm kredi ister, ticari kullanım için paid lisans gerekebilir.

### Mob sprite'ları (Goblin, Orc)
- Dosyalar: `assets/characters/mobs/`
- TODO: kaynak/lisans kaydı buraya (indirildiği sayfa bulunup eklenecek).

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
