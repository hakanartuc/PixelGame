#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# LANETLİ MEZARLIK v2 (Faz MAPGEN-2): organik sınır + CA kaya kümeleri +
# kaya çerçeveli ölü su gölcükleri. Tile: 12 ölü toprak · 13 kaya · 14 ölü su.
# SABİT NOKTALAR: portal (30,41-43), dev iskelet (30,15) + boss önü (28-33,
# 18-22), dekorlar, sandıklar (12,9)/(30,13), 8 kamp (MZ_CAMPS).
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mapgen import (value_noise, ca_blobs, min2x2, fill_pockets, preview,
                    write_csv, poisson)

W, H = 60, 45
GROUND, ROCK, WATER = 12, 13, 14
SEED = 66

g = [[GROUND] * W for _ in range(H)]

# 1) ORGANİK DIŞ SINIR: noise eşikli kaya çerçevesi
n = value_noise(W, H, 10.0, SEED, octaves=3)
for y in range(H):
    for x in range(W):
        edge = min(x, y, W - 1 - x, H - 1 - y)
        if edge < 4 and n[y][x] + (4 - edge) * 0.22 > 0.70:
            g[y][x] = ROCK

# 2) KAYA KÜMELERİ (kamp/dekor alanlarından UZAK konumlanır — garanti
#    temizliği kümeleri yutmasın): kuzey sırtı + iki yan yaka (CA)
blob = ca_blobs(26, 7, 0.58, SEED + 1, steps=2)
for y in range(7):
    for x in range(26):
        if blob[y][x]:
            g[2 + y][18 + x] = ROCK       # kuzey sırtı (dev iskeletin ardı)
blob2 = ca_blobs(8, 13, 0.58, SEED + 2, steps=2)
for y in range(13):
    for x in range(8):
        if blob2[y][x]:
            g[26 + y][2 + x] = ROCK       # batı yaka (güney yarı)
blob3 = ca_blobs(7, 9, 0.58, SEED + 3, steps=2)
for y in range(9):
    for x in range(7):
        if blob3[y][x]:
            g[14 + y][52 + x] = ROCK      # doğu yaka

# 2b) ORTA SERPİNTİ: açık alana küçük kaya öbekleri (mezar siperleri)
FIXED = [(30, 41), (30, 15), (30, 20), (12, 9), (30, 13), (27, 17), (10, 16),
         (50, 18), (38, 36), (17, 29), (20, 31), (43, 29), (41, 32), (24, 24),
         (36, 22), (52, 9), (18, 30), (42, 30), (30, 18), (16, 12), (44, 12),
         (50, 26), (10, 22), (34, 40)]
def far_from_fixed(x, y):
    return all((x - fx) ** 2 + (y - fy) ** 2 >= 36 for (fx, fy) in FIXED)
for (px_, py_) in poisson(W - 8, H - 12, 9, SEED + 7,
                          ok=lambda x, y: far_from_fixed(x + 4, y + 6)):
    bx, by = px_ + 4, py_ + 6
    for dy in range(2):
        for dx in range(2 + (bx + by) % 2):    # 2x2 / 3x2 öbek
            if 1 <= bx + dx < W - 1 and 1 <= by + dy < H - 1:
                g[by + dy][bx + dx] = ROCK

# 3) ÖLÜ SU GÖLCÜKLERİ: noise kenarlı + KAYA ÇERÇEVELİ (autotile çerçevesi
#    kayadan gelir — su düz koyu doku kalır)
def pool(cx, cy, rx, ry, seed):
    on = value_noise(2 * rx + 3, 2 * ry + 3, 4.0, seed, octaves=2)
    for y in range(-ry - 1, ry + 2):
        for x in range(-rx - 1, rx + 2):
            d = (x * x) / float(rx * rx) + (y * y) / float(ry * ry)
            gx, gy = cx + x, cy + y
            if 1 <= gx < W - 1 and 1 <= gy < H - 1 and \
               d + (on[y + ry + 1][x + rx + 1] - 0.5) * 0.7 < 1.0:
                g[gy][gx] = WATER
pool(37, 38, 3, 2, SEED + 4)              # güney ölü su (Kan Canavarı kampı yanı)
pool(52, 32, 2, 2, SEED + 5)              # doğu küçük gölcük
# çerçeve: suya 4-komşu toprak → kaya
for y in range(1, H - 1):
    for x in range(1, W - 1):
        if g[y][x] == GROUND and any(
                g[y + dy][x + dx] == WATER
                for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1))):
            g[y][x] = ROCK

# 4) TEMİZLİK: şerit süpür + hapsolmuş cepleri doldur
min2x2(g, ROCK, GROUND)
fill_pockets(g, GROUND, ROCK, 10)

# 5) SABİT ALAN GARANTİLERİ (kod bağımlılıkları — en son)
def clear(cx, cy, r):
    for dy in range(-r, r + 1):
        for dx in range(-r, r + 1):
            if 0 <= cx + dx < W and 0 <= cy + dy < H and \
               g[cy + dy][cx + dx] != GROUND:
                g[cy + dy][cx + dx] = GROUND
for y in range(38, 44):                    # portal koridoru (güney giriş)
    for x in range(28, 33):
        g[y][x] = GROUND
for y in range(17, 23):                    # boss arenası (dev iskelet önü)
    for x in range(26, 36):
        g[y][x] = GROUND
clear(30, 15, 2)                           # dev iskelet kalıntısı (lich dekoru)
for (dx_, dy_) in [(27, 17), (10, 16), (50, 18), (38, 36), (12, 9),
                   (17, 29), (20, 31), (43, 29), (41, 32), (24, 24),
                   (36, 22), (52, 9), (30, 13)]:
    clear(dx_, dy_, 1)                     # dekor + sandık tabanları
for (cx, cy, r) in [(18, 30, 4), (42, 30, 4), (30, 18, 5), (16, 12, 4),
                    (44, 12, 4), (50, 26, 4), (10, 22, 4), (34, 40, 3)]:
    clear(cx, cy, min(r, 3))               # kamp çevreleri yürünür
for y in range(H):                         # en dış halka garanti
    g[y][0] = ROCK; g[y][W - 1] = ROCK
for x in range(W):
    g[0][x] = ROCK; g[H - 1][x] = ROCK

root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
write_csv(g, os.path.join(root, "assets/maps/mezarlik.csv"))
prev = os.environ.get("MAPGEN_PREVIEW",
    "/private/tmp/claude-502/-Users-hakanartuc-PixelGame/2746a263-e9f0-4846-867a-e9773c28bfbd/scratchpad/mezarlik_v2.png")
preview(g, {GROUND: (111, 115, 106), ROCK: (57, 31, 33), WATER: (26, 32, 46)},
        prev, cell=10,
        marks=[(30, 41, (255, 255, 0)),                            # portal
               (30, 15, (200, 0, 255)), (30, 20, (255, 0, 0)),     # lich + boss
               (12, 9, (0, 200, 255)), (30, 13, (0, 200, 255)),    # sandıklar
               (18, 30, (0, 255, 0)), (42, 30, (0, 255, 0)),
               (34, 40, (255, 128, 0))])                           # kamplar
print("mezarlik.csv v2 + önizleme yazıldı")
