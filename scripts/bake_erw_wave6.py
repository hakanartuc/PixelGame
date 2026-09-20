#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# DENİZ TEST GÖLETİ (Hakan: "deniz asseti — daha mavi, kıyıda dalga; ikinci
# gölet olarak yap"). Kaynak: Sea Adventures "platform - grass2 to coast (sea)"
# — statik çim-geçiş çaprazı + 16 KARELİK animasyonlu kıyı şeritleri (iki grup:
# kolonlar 0-15 ve 17-32; 16 ayraç, 33 artık). İç su = GL16 mavi full-tile
# animasyonu (water_anim.png, 8 kare) — pack tasarımı da iç/kıyıyı ayrı tutuyor.
# Çıktı: Tiles/erw/sea_atlas.png — 4×4 maske bloğu × 16 kare (dikey istif).
# Animasyonsuz maskeler statik çapraz parçasıyla 16 kez doldurulur.
import os, struct, sys, zlib
import xml.etree.ElementTree as ET
PACK = sys.argv[1] if len(sys.argv) > 1 else "/Users/hakan/Assets/2D Pixel-Art/Epic RPG"
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
TIL = os.path.join(ROOT, "assets/Tiles/erw")
src = open(os.path.join(ROOT, "scripts/bake_erw_biome.py")).read()
exec(src.split('P = lambda')[0])

f = os.path.join(PACK, "ERW - Sea Adventures (GL 2.0 expansion) V1.5/TiledMap Editor/Tilesets/platform - grass - coast.tsx")
t = ET.parse(f).getroot()
cols = int(t.attrib["columns"])
img = os.path.normpath(os.path.join(os.path.dirname(f), t.find("image").attrib["source"]))
w, h, px = png_read_rgba(img)
FR = 16
anim, stat = {}, {}
for ws in t.iter("wangset"):
    if "sea" not in ws.attrib["name"]:
        continue
    for wt in ws.iter("wangtile"):
        tid = int(wt.attrib["tileid"])
        ids = [int(v) for v in wt.attrib["wangid"].split(",")]
        ne, se, sw, nw = ids[1], ids[3], ids[5], ids[7]
        m = ((1 if nw == 1 else 0) | (2 if ne == 1 else 0)
           | (4 if sw == 1 else 0) | (8 if se == 1 else 0))
        c, r = tid % cols, tid // cols
        if r >= 16 and c in (0, 17):          # animasyonlu kıyı parçası
            anim.setdefault(m, tid)
        elif r <= 5:                          # statik çapraz parçası
            stat.setdefault(m, tid)
atlas = bytearray(4 * TILE * 4 * TILE * FR * 4)
eksik = []
for m in range(1, 15):
    for fr in range(FR):
        if m in anim:
            tid = anim[m] + fr                # kare = 1 kolon sağa
        elif m in stat:
            tid = stat[m]                     # statik: her karede aynı
        else:
            eksik.append(m); break
        blit_tile(atlas, 4 * TILE, (m % 4) * TILE,
                  fr * 4 * TILE + (m // 4) * TILE, px, w, tid, cols)
png_write_rgba(os.path.join(TIL, "sea_atlas.png"), 4 * TILE, 4 * TILE * FR, atlas)
print(f"OK sea_atlas.png: {FR} kare x 4x4 | animasyonlu maske: {sorted(anim)} | statik: {sorted(set(stat)-set(anim))} | eksik: {sorted(set(eksik))}")
