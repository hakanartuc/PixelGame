#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# DENİZ/MAVİ SU TEST GÖLETİ v2 (Hakan: ilk deneme "olmamış" — platform-coast
# seti KLİFLİ sahil çıktı, kaya dudağı setin sanatıydı). Doğru kaynak:
# GL2.0 "water to grass - river orientation" — kayasız çim kıyısı.
#   · statik çapraz: kolon 0-4, satır 1-5 (köşe/kenar parçaları)
#   · animasyonlu kenarlar: kolon 3/12/21 satır 8-13, 8 KARE (1 kolon sağa)
#   · YÖN OTO-DOĞRULAMA: maske 12 (alt yarı su) parçasının alt yarısı üstünden
#     daha mavi olmalı — değilse wang rengi ters demektir, maskeler 15-m çevrilir.
import os, struct, sys, zlib
import xml.etree.ElementTree as ET
PACK = sys.argv[1] if len(sys.argv) > 1 else "/Users/hakan/Assets/2D Pixel-Art/Epic RPG"
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
TIL = os.path.join(ROOT, "assets/Tiles/erw")
src = open(os.path.join(ROOT, "scripts/bake_erw_biome.py")).read()
exec(src.split('P = lambda')[0])

f = os.path.join(PACK, "ERW - Grass Land 2.0 v2.1/TiledMap Editor/Tilesets",
                 "platform - water to grass - river orientation.tsx")
t = ET.parse(f).getroot()
cols = int(t.attrib["columns"])
img = os.path.normpath(os.path.join(os.path.dirname(f), t.find("image").attrib["source"]))
w, h, px = png_read_rgba(img)
FR = 8
anim, stat = {}, {}
for ws in t.iter("wangset"):
    for wt in ws.iter("wangtile"):
        tid = int(wt.attrib["tileid"])
        ids = [int(v) for v in wt.attrib["wangid"].split(",")]
        ne, se, sw, nw = ids[1], ids[3], ids[5], ids[7]
        m = ((1 if nw == 1 else 0) | (2 if ne == 1 else 0)
           | (4 if sw == 1 else 0) | (8 if se == 1 else 0))
        c, r = tid % cols, tid // cols
        if c in (3, 12, 21) and r >= 8:
            anim.setdefault(m, tid)
        elif r <= 5:
            stat.setdefault(m, tid)

def blueness(tid, half):
    """Karonun üst(0)/alt(1) yarısının mavi baskınlığı."""
    sx, sy = (tid % cols) * TILE, (tid // cols) * TILE
    b = r_ = n = 0
    y0 = TILE // 2 if half else 0
    for y in range(y0, y0 + TILE // 2):
        ro = ((sy + y) * w + sx) * 4
        for x in range(TILE):
            if px[ro + x * 4 + 3] > 128:
                r_ += px[ro + x * 4]; b += px[ro + x * 4 + 2]; n += 1
    return (b - r_) / max(n, 1)

probe = anim.get(12, stat.get(12))
inverted = blueness(probe, 1) < blueness(probe, 0)
if inverted:
    anim = {15 - m: tid for m, tid in anim.items()}
    stat = {15 - m: tid for m, tid in stat.items()}
print("yon:", "TERS (15-m cevrildi)" if inverted else "duz")

atlas = bytearray(4 * TILE * 4 * TILE * FR * 4)
eksik = []
for m in range(1, 15):
    for fr in range(FR):
        if m in anim:
            tid = anim[m] + fr
        elif m in stat:
            tid = stat[m]
        else:
            eksik.append(m); break
        blit_tile(atlas, 4 * TILE, (m % 4) * TILE,
                  fr * 4 * TILE + (m // 4) * TILE, px, w, tid, cols)
png_write_rgba(os.path.join(TIL, "sea_atlas.png"), 4 * TILE, 4 * TILE * FR, atlas)
print(f"OK sea_atlas.png: {FR} kare | anim maskeler: {sorted(anim)} | statik: {sorted(set(stat)-set(anim))} | eksik: {sorted(set(eksik))}")
