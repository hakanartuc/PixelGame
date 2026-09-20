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
