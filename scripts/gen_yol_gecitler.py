#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# GEÇİT YOLLARI (Hakan: "çölü bulamadım, zindan geçişini bulamadım") —
# kervan yolu col 159'da bitiyordu; geçitlere dal veriliyor (yalnız ÇİM
# üstüne yazar, 2 karo genişlik — ERW yol dili kalın okunuyor):
#   ana yol (159,15) → doğuya (197,19) ÇÖL GEÇİDİ
#   (145,15) → kuzeye (145,7) ESKİ GEÇİT zindanı
#   (168,16) → güneye (168,45) KONAK
#   (122,17) → güneye (122,83) MEZARLIK
import os
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
CSV = os.path.join(ROOT, "assets/maps/world.csv")
rows = [l.rstrip("\n").split(",") for l in open(CSV)]
def put(c, r):
    if 0 <= r < len(rows) and 0 <= c < len(rows[r]) and rows[r][c].strip() != "":
        if rows[r][c] == "1":          # yalnız çim (id 0 → csv '1')
            rows[r][c] = "2"           # yol (id 1 → csv '2')
def hline(r, c0, c1):
    for c in range(min(c0, c1), max(c0, c1) + 1):
        put(c, r); put(c, r + 1)
def vline(c, r0, r1):
    for r in range(min(r0, r1), max(r0, r1) + 1):
        put(c, r); put(c + 1, r)
hline(15, 159, 193); vline(193, 15, 18); hline(18, 193, 196)   # çöl geçidi
vline(145, 8, 15)                                               # Eski Geçit
vline(168, 16, 44)                                              # konak
vline(122, 17, 82)                                              # mezarlık
open(CSV, "w").write("\n".join(",".join(r) for r in rows) + "\n")
print("OK: geçit yolları çizildi")
