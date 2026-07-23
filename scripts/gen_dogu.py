#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# TAŞDURAK v2 (Faz MAPGEN): mapgen kütüphanesiyle organik çöl.
# Tile id'leri: 15 kum · 16 kaya · 17 vaha suyu (CSV'ye GID+1 yazılır).
# Sabit noktalar KORUNUR (kod bunlara bağlı): batı girişi y33-37,
# kasaba x8-32/y24-46, vaha ~(15-30,37-46), tapınak vadisi KD, kamplar.
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mapgen import (value_noise, drunk_path, ca_blobs, preview, write_csv,
                    min2x2, fill_pockets)

W, H = 100, 70
SAND, ROCK, WATER = 15, 16, 17
SEED = 88

g = [[SAND] * W for _ in range(H)]

# 1) ORGANİK DIŞ SINIR: noise eşiğiyle kayalık çerçeve (düz duvar yok)
n = value_noise(W, H, 14.0, SEED, octaves=3)
for y in range(H):
    for x in range(W):
        edge = min(x, y, W - 1 - x, H - 1 - y)
        if edge < 5 and n[y][x] + (5 - edge) * 0.18 > 0.72:
            g[y][x] = ROCK
        if edge == 0:
            g[y][x] = ROCK   # en dış halka garanti kapalı

# 2) KUZEY SIRTLARI (tapınak vadisi çevresi): CA kümeleri
blob = ca_blobs(46, 14, 0.44, SEED + 1)
for y in range(14):
    for x in range(46):
        if blob[y][x] and 52 + x < W - 1:
            g[3 + y][52 + x] = ROCK
# vadiye giriş: tapınağa güneyden kıvrımlı yaklaşım
for (px, py) in drunk_path(70, 34, 79, 18, SEED + 2, wobble=0.35, width=2):
    if 1 <= px < W - 1 and 1 <= py < H - 1:
        g[py][px] = SAND

# 3) GÜNEY KAYA KÜMELERİ (akrep yuvaları): CA — ada ada
blob2 = ca_blobs(56, 16, 0.46, SEED + 3)
for y in range(16):
    for x in range(56):
        if blob2[y][x] and 40 + x < W - 1 and 50 + y < H - 1:
            g[50 + y][40 + x] = ROCK

# 4) KERVAN YOLU: girişten kasabaya + kasabadan doğuya kıvrımlı temiz iz
for (px, py) in drunk_path(2, 35, 20, 33, SEED + 4, wobble=0.3, width=2):
    if 1 <= px < W - 1 and 1 <= py < H - 1:
        g[py][px] = SAND
for (px, py) in drunk_path(30, 34, 96, 30, SEED + 5, wobble=0.35, width=2):
    if 1 <= px < W - 1 and 1 <= py < H - 1:
        g[py][px] = SAND

# 5) VAHA: noise kenarlı organik gölet (dikdörtgen değil)
on = value_noise(16, 10, 5.0, SEED + 6, octaves=2)
for y in range(10):
    for x in range(16):
        cx, cy = x - 8.0, y - 5.0
        d = (cx * cx) / 36.0 + (cy * cy) / 12.0   # elips çekirdeği
        if d + (on[y][x] - 0.5) * 0.8 < 0.62:
            gx, gy = 15 + x, 37 + y
            if 1 <= gx < W - 1 and 1 <= gy < H - 1:
                g[gy][gx] = WATER

# 6b) BATI/KUZEYBATI dokusu: kasaba dışı batı çölüne de kümeler
nw = value_noise(W, H, 7.0, SEED + 9, octaves=2)
for y in range(3, 24):
    for x in range(3, 50):
        if nw[y][x] > 0.75 and g[y][x] == SAND:
            g[y][x] = ROCK
for y in range(48, 67):
    for x in range(3, 38):
        if nw[y][x] > 0.73 and g[y][x] == SAND:
            g[y][x] = ROCK

# 6) ORTA ÇÖL: noise eşikli seyrek kaya (kümeli doğal dağılım)
n2 = value_noise(W, H, 6.0, SEED + 7, octaves=2)
for y in range(12, 48):
    for x in range(35, 96):
        if n2[y][x] > 0.74 and g[y][x] == SAND:
            g[y][x] = ROCK

# 7) SABİT ALAN GARANTİLERİ (kod bağımlılıkları)
def apply_guarantees():
    for y in range(33, 38):                    # batı girişi açık
        g[y][1] = SAND; g[y][2] = SAND; g[y][3] = SAND
    for y in range(24, 47):                    # kasaba çekirdeği temiz
        for x in range(8, 33):
            if g[y][x] == ROCK:
                g[y][x] = SAND
    for y in range(24, 32):                    # su binaların altına taşmasın
        for x in range(8, 33):
            if g[y][x] == WATER:
                g[y][x] = SAND
    for y in range(11, 18):                    # tapınak/obelisk tabanları
        for x in range(72, 90):
            if 1 <= x < W - 1 and g[y][x] == ROCK:
                g[y][x] = SAND
    for (cx, cy) in [(78, 17), (88, 24), (62, 56), (84, 52), (52, 34), (70, 38)]:
        for dy in range(-2, 3):                # kamp çevresi yürünür
            for dx in range(-2, 3):
                if 1 <= cx + dx < W - 1 and 1 <= cy + dy < H - 1 and \
                   g[cy + dy][cx + dx] == ROCK:
                    g[cy + dy][cx + dx] = SAND

apply_guarantees()

# 8) AUTOTILE TEMİZLİĞİ: dual-grid seti tek hücrelik benekleri çizebilir ama
#    2x2 altı kaya/su hem oynanış hem görsel olarak cılız — min2x2 süpürür;
#    en dış halka sonra yeniden garanti edilir (min2x2 kemirmesin)
min2x2(g, ROCK, SAND)
min2x2(g, WATER, SAND)
fill_pockets(g, SAND, ROCK, 12)   # mesa içi kum delikleri kapanır (okunurluk)
for y in range(H):
    g[y][0] = ROCK; g[y][W - 1] = ROCK
for x in range(W):
    g[0][x] = ROCK; g[H - 1][x] = ROCK

root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
write_csv(g, os.path.join(root, "assets/maps/dogu.csv"))
prev = os.environ.get("MAPGEN_PREVIEW",
    "/private/tmp/claude-502/-Users-hakanartuc-PixelGame/2746a263-e9f0-4846-867a-e9773c28bfbd/scratchpad/dogu_v2.png")
preview(g, {SAND: (222, 180, 120), ROCK: (150, 100, 62), WATER: (70, 170, 190)},
        prev, cell=6,
        marks=[(4, 35, (255, 255, 0)),                              # giriş
               (13, 27, (255, 0, 0)), (20, 28, (255, 0, 0)),
               (27, 27, (255, 0, 0)),                               # evler
               (80, 14, (200, 0, 255)),                             # tapınak
               (78, 17, (0, 255, 0)), (88, 24, (0, 255, 0)),        # mumya
               (62, 56, (0, 200, 255)), (84, 52, (0, 200, 255)),    # akrep
               (52, 34, (255, 128, 0)), (70, 38, (255, 128, 0))])   # haydut
print("dogu.csv v2 + önizleme yazıldı")
