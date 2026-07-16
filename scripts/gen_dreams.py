#!/usr/bin/env python3
# Ruya vinyeti panelleri (Dunya Olaylari): dinlenmede %25 sansla gorulen 3 ruya.
# gen_intro.py'nin kardesi — ayni minimal PNG yaklasimi, dis bagimlilik yok.
# ruya_muhur: isinan muhur / ruya_cukur: dibi olmayan cukur / ruya_fisilti: fisilti
import zlib, struct, math

def save_png(path, W, H, img):
    raw = b''.join(b'\x00' + bytes(img[y*W*4:(y+1)*W*4]) for y in range(H))
    def chunk(typ, data):
        c = struct.pack('>I', len(data)) + typ + data
        return c + struct.pack('>I', zlib.crc32(typ + data) & 0xffffffff)
    png = b'\x89PNG\r\n\x1a\n'
    png += chunk(b'IHDR', struct.pack('>IIBBBBB', W, H, 8, 6, 0, 0, 0))
    png += chunk(b'IDAT', zlib.compress(raw, 9))
    png += chunk(b'IEND', b'')
    open(path, 'wb').write(png)

class Canvas:
    def __init__(self, W, H, rgb=(12, 12, 20)):
        self.W, self.H = W, H
        self.px = bytearray(W*H*4)
        for i in range(W*H):
            self.px[i*4:i*4+4] = bytes([rgb[0], rgb[1], rgb[2], 255])
    def rect(self, x0, y0, w, h, rgb, alpha=255):
        for y in range(max(0, y0), min(self.H, y0+h)):
            for x in range(max(0, x0), min(self.W, x0+w)):
                o = (y*self.W + x)*4
                for c in range(3):
                    self.px[o+c] = (rgb[c]*alpha + self.px[o+c]*(255-alpha)) // 255
    def glow(self, cx, cy, r, rgb, strength=120):
        for y in range(max(0, cy-r), min(self.H, cy+r)):
            for x in range(max(0, cx-r), min(self.W, cx+r)):
                d = math.hypot(x-cx, y-cy) / r
                if d >= 1.0: continue
                a = int(strength * (1.0-d) * (1.0-d))
                if a <= 0: continue
                o = (y*self.W + x)*4
                for c in range(3):
                    v = self.px[o+c] + rgb[c]*a//255
                    self.px[o+c] = v if v < 255 else 255
    def ring(self, cx, cy, r, rgb, alpha=90, thick=1.6):
        for y in range(max(0, cy-r-2), min(self.H, cy+r+2)):
            for x in range(max(0, cx-r-2), min(self.W, cx+r+2)):
                d = math.hypot(x-cx, y-cy)
                if abs(d-r) > thick: continue
                a = int(alpha * (1.0 - abs(d-r)/thick))
                o = (y*self.W + x)*4
                for c in range(3):
                    v = self.px[o+c] + rgb[c]*a//255
                    self.px[o+c] = v if v < 255 else 255
    def blur(self):
        src = bytes(self.px)
        for y in range(1, self.H-1):
            for x in range(1, self.W-1):
                o = (y*self.W + x)*4
                for c in range(3):
                    s = 0
                    for dy in (-1, 0, 1):
                        for dx in (-1, 0, 1):
                            s += src[((y+dy)*self.W + (x+dx))*4 + c]
                    self.px[o+c] = s // 9
    def vignette(self, strength=150):
        cx, cy = self.W/2, self.H/2
        maxd = math.hypot(cx, cy)
        for y in range(self.H):
            for x in range(self.W):
                d = math.hypot(x-cx, y-cy) / maxd
                dark = int(strength * max(0.0, d-0.40) / 0.60)
                if dark <= 0: continue
                o = (y*self.W + x)*4
                for c in range(3):
                    v = self.px[o+c] - dark
                    self.px[o+c] = v if v > 0 else 0

W, H = 208, 117

# --- 1) ruya_muhur: gogusteki muhur — karanlikta isinan turuncu-mor damga ---
c = Canvas(W, H, rgb=(10, 8, 16))
# gogus silueti: alt yariyi dolduran koyu kutle
c.rect(40, 60, 128, 57, (24, 20, 30), alpha=230)
c.rect(60, 48, 88, 20, (20, 17, 26), alpha=220)
# muhur: sicak cekirdek + mor hale + nabiz halkalari
c.glow(104, 72, 34, (255, 140, 40), strength=190)
c.glow(104, 72, 14, (255, 220, 120), strength=230)
c.glow(104, 72, 58, (150, 80, 255), strength=70)
c.ring(104, 72, 22, (255, 170, 90), alpha=120, thick=1.8)
c.ring(104, 72, 40, (170, 110, 255), alpha=70, thick=2.2)
c.blur()
c.vignette(170)
save_png('assets/scenes/ruya_muhur.png', W, H, c.px)

# --- 2) ruya_cukur: dibi olmayan cukur — asagi cagiran soguk isik ---
c = Canvas(W, H, rgb=(14, 16, 24))
# zemin: iki yana acilan taş dudaklar, ortada karanlik yarik
c.rect(0, 40, W, 77, (30, 32, 42), alpha=255)
for i in range(9):   # yarik: ortaya dogru daralan siyah kademeler
    ww = 120 - i*12
    c.rect((W-ww)//2, 46 + i*8, ww, 8, (max(2, 12-i), max(2, 12-i), max(4, 18-i*2)), alpha=255)
# dipten sizen soguk yesil-mavi isik
c.glow(104, 110, 46, (60, 200, 170), strength=110)
c.glow(104, 116, 22, (120, 255, 220), strength=140)
# kenarda duran minik siluet — olcek hissi
c.rect(36, 34, 5, 10, (8, 8, 12), alpha=255)
c.rect(37, 30, 3, 4, (8, 8, 12), alpha=255)
c.blur()
c.vignette(160)
save_png('assets/scenes/ruya_cukur.png', W, H, c.px)

# --- 3) ruya_fisilti: sisin icinde agizsiz sesler — mor lekeler ---
c = Canvas(W, H, rgb=(16, 12, 22))
# sis katmanlari
for y0, a in ((20, 40), (50, 55), (80, 45)):
    c.rect(0, y0, W, 24, (60, 50, 80), alpha=a)
# fisilti kaynaklari: yer yer mor soluklar, hicbiri yuz degil
for (x, y, r, st) in ((40, 46, 18, 120), (96, 70, 24, 140), (150, 40, 15, 110), (170, 84, 20, 100)):
    c.glow(x, y, r, (170, 110, 255), strength=st)
    c.glow(x, y, r//2, (220, 180, 255), strength=st//2)
# aralarindan gecen ince soguk cizgi — "henuz degil" hissi
c.rect(0, 62, W, 1, (200, 170, 255), alpha=60)
c.blur()
c.blur()
c.vignette(150)
save_png('assets/scenes/ruya_fisilti.png', W, H, c.px)

print('ruya panelleri: ruya_muhur / ruya_cukur / ruya_fisilti')
