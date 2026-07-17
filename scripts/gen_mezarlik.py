#!/usr/bin/env python3
# Lanetli Mezarlik haritasi (60x45): olu toprak (12) + dikenli kaya siniri (13)
# + olu su golcugu (14). Yerlesim sabit-tohumlu prosedurel + elle acikliklar.
import random
random.seed(66)
W, H = 60, 45
G, R, S = 12, 13, 14   # ground / rock / su
m = [[G]*W for _ in range(H)]
# sinir kayaliklari (2-3 kalinlik, dalgali)
for x in range(W):
    t = 2 + int(random.random()*2)
    for y in range(t): m[y][x] = R
    b = 2 + int(random.random()*2)
    for y in range(H-b, H): m[H-1-(H-1-y)][x] = R
for y in range(H):
    t = 2 + int(random.random()*2)
    for x in range(t): m[y][x] = R
    b = 2 + int(random.random()*2)
    for x in range(W-b, W): m[y][x] = R
# guney giris agzi (portal T30,41-43)
for y in range(H-5, H):
    for x in range(28, 33): m[y][x] = G
# ic kaya kumeleri (siperler)
for cx, cy, r in [(12,20,3),(48,22,3),(22,36,2),(40,8,2),(52,34,2),(8,32,2)]:
    for y in range(cy-r, cy+r+1):
        for x in range(cx-r, cx+r+1):
            if 0<=x<W and 0<=y<H and (x-cx)**2+(y-cy)**2 <= r*r:
                m[y][x] = R
# olu su golcugu (dogu)
for y in range(24, 31):
    for x in range(46, 55):
        if (x-50)**2/16.0 + (y-27)**2/6.0 <= 1.2: m[y][x] = S
# dev iskelet platosu (merkez-kuzey — dekor buraya oturur; zemin acik kalir)
# CSV yaz (GID = id+1, Tiled uyumu)
with open('assets/maps/mezarlik.csv','w') as f:
    for y in range(H):
        f.write(','.join(str(m[y][x]+1) for x in range(W)) + chr(10))
print('OK assets/maps/mezarlik.csv (60x45)')
