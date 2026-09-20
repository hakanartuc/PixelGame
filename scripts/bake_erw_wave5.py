#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ERW göçü 5. dalga — SU ANİMASYONU + ÇÖL KAYASI (Hakan: "çöl geçişi nerde?
# denizi ve göletleri de değiştir"). Su tabanı 8 karelik ERW animasyonu olur
# (deniz/gölet/vaha aynı şerit; asıl görünüm su shader'ında — renkleri main'de
# ERW paletine çekildi). Çöl kayası: Cute klif sistemi emekli, ERW rocky-blob.
import os, struct, sys, zlib
import xml.etree.ElementTree as ET
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
PACK = sys.argv[1] if len(sys.argv) > 1 else \
    "/Users/hakan/Assets/2D Pixel-Art/Epic RPG"
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
TIL = os.path.join(ROOT, "assets/Tiles/erw")

# bake_erw_biome'daki codec/araçlar yeniden kullanılır (script çalıştırmadan
# fonksiyon almak için exec — biome script'i sonunda iş listesi koşturuyor,
# o yüzden yalnız tanım bölümü alınır)
src = open(os.path.join(ROOT, "scripts/bake_erw_biome.py")).read()
defs = src.split('P = lambda')[0]
exec(defs)

P = lambda *a: os.path.join(PACK, *a)
DS = P("Epic RPG World - Desert V1.6/tiledmap editor/Tilesets/Terrain-Desert.tsx")
GL16W = P("EPIC RPG World Pack - Grass Land V. 1.6/Tilesets/Animated water tiles (full tile).png")
print("OK:")

# 1) SU ANİMASYON ŞERİDİ — 8 kare 32×32 (deniz + gölet + vaha ortak taban)
w, h, px = png_read_rgba(GL16W)
png_write_rgba(os.path.join(TIL, "water_anim.png"), w, h, px)
print(f"  water_anim.png: {w//32} kare 32x32")

# 2) ÇÖL KAYASI — rocky ground blob'u (klif mimarisi yerine kayalık zemin;
# yüz/plato sistemi emekli — ERW çöl dokusunda rocky yamalar böyle)
interior(DS, "Rocky ground-to-sand", "rock.png")
mask_atlas(DS, "Rocky ground-to-sand", "rocky_atlas.png")

# 3) VAHA KIYISI ANİMASYONLU (Hakan: "kıyıda dalga animasyonu var") —
# Oasis sheet'inde parçalar kolon 1 ve 10'da; animasyon YATAY: kare başına
# 1 kolon sağa, 8 KARE (kolon 9 ve 18 BOŞ AYRAÇ — 9 saymak göleti
# kare başı yok ediyordu, Hakan bulgusu). Maske atlası kare başına 4×4 blok, bloklar DİKEY
# istiflenir (src.y += kare*128); iç su ayrı 9 karelik yatay şerit.
DSW = P("Epic RPG World - Desert V1.6/tiledmap editor/Tilesets/water-to-sand.tsx")
img, cols, bm = wang(DSW, "water-to-sand")
w, h, px = sheet(img)
FR = 8
atlas = bytearray(4 * TILE * (4 * TILE * FR) * 4)
for f in range(FR):
    for m, tids in bm.items():
        tid = sorted(tids)[0] + f          # kare = 1 kolon sağa
        blit_tile(atlas, 4 * TILE, (m % 4) * TILE,
                  f * 4 * TILE + (m // 4) * TILE, px, w, tid, cols)
png_write_rgba(os.path.join(TIL, "oasis_atlas.png"), 4 * TILE, 4 * TILE * FR, atlas)
print(f"  oasis_atlas.png: {FR} kare x 4x4 blok (dikey istif)")
wtid = sorted(bm[15])[0]
strip = bytearray(TILE * FR * TILE * 4)
for f in range(FR):
    blit_tile(strip, TILE * FR, f * TILE, 0, px, w, wtid + f, cols)
png_write_rgba(os.path.join(TIL, "oasis_water_anim.png"), TILE * FR, TILE, strip)
print(f"  oasis_water_anim.png: {FR} kare 32x32")
