#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# SAF ERW KARŞILAŞTIRMA GÖLETİ (Hakan, 2026-09-20): köyün KD'sinde elips
# gölet — kum halkası (15) + saf ERW suyu (21, shader'sız). Mevcut sulara
# DOKUNMAZ. Amaç: shader'lı su ile ham paket suyunu yan yana görmek.
# CSV kuralı: değer = tile id + 1. Geri almak: bu script'in yamasını çimene
# (1) geri yazmak ya da git'ten world.csv'yi almak.
import os
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
CSV = os.path.join(ROOT, "assets/maps/world.csv")
rows = [l.rstrip("\n").split(",") for l in open(CSV)]
CX, CY, RX, RY = 33.0, 5.5, 4.0, 3.0   # merkez + yarıçaplar (karo)
def put(c, r, tid):
    if 0 <= r < len(rows) and 0 <= c < len(rows[r]) and rows[r][c].strip() != "":
        rows[r][c] = str(tid + 1)
n_sand = n_water = 0
for r in range(int(CY - RY) - 2, int(CY + RY) + 3):
    for c in range(int(CX - RX) - 2, int(CX + RX) + 3):
        d = ((c + 0.5 - CX) / RX) ** 2 + ((r + 0.5 - CY) / RY) ** 2
        if d <= 1.0:
            put(c, r, 21); n_water += 1
        elif d <= 2.1:
            put(c, r, 15); n_sand += 1
open(CSV, "w").write("\n".join(",".join(r) for r in rows) + "\n")
print(f"OK: golet ({CX},{CY}) su={n_water} kum={n_sand}")
