#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# SESSİZ EL LOCASI v2 (Faz MAPGEN-2): mağara + taş salon karışımı.
# Tile: 6 taş döşeme (salon) · 7 taş duvar (salon) · 10 void ·
#       18 mağara zemini · 19 mağara duvarı (CSV'ye GID+1 yazılır).
# SABİT NOKTALAR (kod bağımlılığı): portal (5.5,5.5)/(5.5,4.2) kuyu dibi,
# Çelme (21,12) tünel, Neriman (28,14) salon, İğne (27,22) hücre,
# kontrat tahtası (32,19).
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mapgen import value_noise, drunk_path, preview, write_csv

W, H = 36, 26
FLOOR_S, WALL_S, VOID, FLOOR_C, WALL_C = 6, 7, 10, 18, 19
SEED = 41

g = [[VOID] * W for _ in range(H)]

# 1) KUYU MAĞARASI: noise kenarlı yuvarlak oda (merkez 5,5 — küçük in)
n = value_noise(W, H, 5.0, SEED, octaves=2)
for y in range(2, 10):
    for x in range(2, 10):
        d = ((x - 5) ** 2 + (y - 5) ** 2) / 9.0
        if d + (n[y][x] - 0.5) * 0.6 < 1.0:
            g[y][x] = FLOOR_C

# 2) TÜNEL: dar ve kıvrımlı — kuyudan Çelme'ye, oradan salon kapısına
for (px, py) in drunk_path(8, 6, 20, 12, SEED + 4, wobble=0.2, width=1):
    for (dx, dy) in ((0, 0), (1, 0), (0, 1)):     # 1.5 genişlik hissi
        if 1 <= px + dx < W - 1 and 1 <= py + dy < H - 1:
            g[py + dy][px + dx] = FLOOR_C
for (px, py) in drunk_path(21, 12, 24, 15, SEED + 2, wobble=0.15, width=2):
    if 1 <= px < W - 1 and 1 <= py < H - 1:
        g[py][px] = FLOOR_C

# 3) TAŞ SALON (inşa edilmiş bölge — dik hatlar BİLEREK):
#    dış duvar ringi + iç döşeme; batı kapısı tünele açılır
SX0, SY0, SX1, SY1 = 24, 11, 35, 24   # duvar dahil sınırlar
for y in range(SY0, SY1 + 1):
    for x in range(SX0, SX1 + 1):
        edge = (x == SX0 or x == SX1 or y == SY0 or y == SY1)
        g[y][x] = WALL_S if edge else FLOOR_S
for y in (14, 15, 16):                 # batı kapısı (tünel ağzı)
    g[y][SX0] = FLOOR_S

# 3b) ZEHİR HÜCRESİ: İğne'nin (27,22) köşesi bölmeyle ayrılır — kapı (26,20)
for x in range(25, 31):
    if x != 26:
        g[20][x] = WALL_S
for y in range(21, 24):
    g[y][30] = WALL_S

# 4) SABİT NOKTA GARANTİLERİ: portal + NPC + tahta çevresi yürünür
def clear(cx, cy, r, tile):
    for dy in range(-r, r + 1):
        for dx in range(-r, r + 1):
            if 0 <= cx + dx < W and 0 <= cy + dy < H:
                g[cy + dy][cx + dx] = tile
clear(5, 5, 2, FLOOR_C)                # kuyu dibi (portal inişi)
clear(21, 12, 1, FLOOR_C)              # Çelme
clear(28, 14, 1, FLOOR_S)              # Neriman (salon içi)
clear(27, 22, 1, FLOOR_S)              # İğne
clear(32, 19, 1, FLOOR_S)              # kontrat tahtası önü

# 5) MAĞARA DUVAR BANDI: mağara zeminine 8-komşu void → mağara duvarı
for y in range(H):
    for x in range(W):
        if g[y][x] != VOID:
            continue
        near = any(0 <= x + dx < W and 0 <= y + dy < H and
                   g[y + dy][x + dx] == FLOOR_C
                   for dy in (-1, 0, 1) for dx in (-1, 0, 1))
        if near:
            g[y][x] = WALL_C

root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
write_csv(g, os.path.join(root, "assets/maps/loca.csv"))
prev = os.environ.get("MAPGEN_PREVIEW",
    "/private/tmp/claude-502/-Users-hakanartuc-PixelGame/2746a263-e9f0-4846-867a-e9773c28bfbd/scratchpad/loca_v2.png")
preview(g, {FLOOR_S: (60, 62, 88), WALL_S: (95, 100, 130), VOID: (7, 7, 13),
            FLOOR_C: (121, 85, 61), WALL_C: (70, 45, 32)},
        prev, cell=14,
        marks=[(5, 5, (255, 255, 0)),     # portal
               (21, 12, (0, 255, 0)),     # Çelme
               (28, 14, (255, 0, 0)),     # Neriman
               (27, 22, (200, 0, 255)),   # İğne
               (32, 19, (0, 200, 255))])  # tahta
print("loca.csv v2 + önizleme yazıldı")
