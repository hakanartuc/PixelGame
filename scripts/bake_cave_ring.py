#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Cave_Walls atlasının KOYU ring/köşe bloklarının düz arka planını (57,31,33)
# şeffaflaştırır — parçalar mezarlığın gri zemininde halo bırakmadan otursun.
# Çıktı: assets/Tiles/Cave/Cave_Walls_Keyed.png (türetilmiş asset).
# Yüz hücreleri (satır 5-7) DOKUNULMAZ: koyu arka plan orada derinlik gölgesi.
import os
from PIL import Image

root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
src = os.path.join(root, "assets/Tiles/Cave/Cave_Walls.png")
dst = os.path.join(root, "assets/Tiles/Cave/Cave_Walls_Keyed.png")

im = Image.open(src).convert("RGBA")
px = im.load()
KEY = (57, 31, 33, 255)
# koyu ring bloğu (4-6, 0-2) + koyu delikli blok (4-6, 3-4) — 16px hücreler
for cy in range(0, 5):
    for cx in range(4, 7):
        for y in range(cy * 16, (cy + 1) * 16):
            for x in range(cx * 16, (cx + 1) * 16):
                if px[x, y] == KEY:
                    px[x, y] = (0, 0, 0, 0)
im.save(dst)
print("Cave_Walls_Keyed.png yazıldı")
