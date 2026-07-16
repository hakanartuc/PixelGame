#!/usr/bin/env python3
# Sessiz Gun acilis vinyeti panelleri (Faz B1): 4 sahne.
# gen_diorama.py'nin kardesi — ayni minimal PNG yaklasimi, dis bagimlilik yok.
# [giris] donmus dunya / [atim] dokuz atim / [ani] baskasinin anisi / [uyanis] uyanis
import zlib, struct, math, os

def load_png(path):
    d = open(path, 'rb').read()
    pos = 8; W = H = None; ct = None; idat = b''
    while pos < len(d):
        ln = struct.unpack('>I', d[pos:pos+4])[0]
        typ = d[pos+4:pos+8]; data = d[pos+8:pos+8+ln]
        if typ == b'IHDR': W, H, bd, ct = struct.unpack('>IIBB', data[:10])
        elif typ == b'IDAT': idat += data
        pos += 12 + ln
    raw = zlib.decompress(idat); ch = 4 if ct == 6 else 3; stride = W * ch
    img = bytearray(W * H * 4); prev = bytearray(stride); p = 0
    for y in range(H):
        f = raw[p]; p += 1
        line = bytearray(raw[p:p+stride]); p += stride
        if f == 1:
            for i in range(ch, stride): line[i] = (line[i] + line[i-ch]) & 255
        elif f == 2:
            for i in range(stride): line[i] = (line[i] + prev[i]) & 255
        elif f == 3:
            for i in range(stride):
                a = line[i-ch] if i >= ch else 0
                line[i] = (line[i] + ((a + prev[i]) >> 1)) & 255
        elif f == 4:
            for i in range(stride):
                a = line[i-ch] if i >= ch else 0
                b = prev[i]; c = prev[i-ch] if i >= ch else 0
                pp = a + b - c; pa, pb, pc = abs(pp-a), abs(pp-b), abs(pp-c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                line[i] = (line[i] + pr) & 255
        prev = line
        for x in range(W):
            o = (y*W + x)*4
            img[o]=line[x*ch]; img[o+1]=line[x*ch+1]; img[o+2]=line[x*ch+2]
            img[o+3]=line[x*ch+3] if ch==4 else 255
    return W, H, img

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
    def __init__(self, W, H, rgb=(20,22,30)):
        self.W, self.H = W, H
        self.px = bytearray(W*H*4)
        for i in range(W*H):
            self.px[i*4:i*4+4] = bytes([rgb[0], rgb[1], rgb[2], 255])
    def blit(self, src, sx, sy, sw, sh, dx, dy, tint=(255,255,255), alpha=255):
        W, H, img = src
        for y in range(sh):
            for x in range(sw):
                so = ((sy+y)*W + (sx+x)) * 4
                a = img[so+3]
                if a < 8: continue
                px, py = dx+x, dy+y
                if px < 0 or px >= self.W or py < 0 or py >= self.H: continue
                do = (py*self.W + px) * 4
                a2 = a * alpha // 255
                for c in range(3):
                    sc = img[so+c] * tint[c] // 255
                    self.px[do+c] = (sc * a2 + self.px[do+c] * (255-a2)) // 255
                self.px[do+3] = 255
    def rect(self, x0, y0, w, h, rgb, alpha=255):
        for y in range(max(0,y0), min(self.H, y0+h)):
            for x in range(max(0,x0), min(self.W, x0+w)):
                o = (y*self.W + x)*4
                for c in range(3):
                    self.px[o+c] = (rgb[c]*alpha + self.px[o+c]*(255-alpha)) // 255
    def glow(self, cx, cy, r, rgb, strength=120):
        for y in range(max(0,cy-r), min(self.H, cy+r)):
            for x in range(max(0,cx-r), min(self.W, cx+r)):
                d = math.hypot(x-cx, y-cy) / r
                if d >= 1.0: continue
                a = int(strength * (1.0 - d) * (1.0 - d))
                if a <= 0: continue
                o = (y*self.W + x)*4
                for c in range(3):
                    v = self.px[o+c] + rgb[c]*a//255
                    self.px[o+c] = v if v < 255 else 255
    def ring(self, cx, cy, r, rgb, alpha=90, thick=1.6):
        # Nabiz halkasi: dokuz atim panelinin dili
        for y in range(max(0,cy-r-2), min(self.H, cy+r+2)):
            for x in range(max(0,cx-r-2), min(self.W, cx+r+2)):
                d = math.hypot(x-cx, y-cy)
                if abs(d - r) > thick: continue
                a = int(alpha * (1.0 - abs(d-r)/thick))
                o = (y*self.W + x)*4
                for c in range(3):
                    v = self.px[o+c] + rgb[c]*a//255
                    self.px[o+c] = v if v < 255 else 255
    def blur(self):
        # Tek gecis kutu bulaniklik — "baskasinin anisi" ruya dili
        src = bytes(self.px)
        for y in range(1, self.H-1):
            for x in range(1, self.W-1):
                o = (y*self.W + x)*4
                for c in range(3):
                    s = 0
                    for dy in (-1,0,1):
                        for dx in (-1,0,1):
                            s += src[((y+dy)*self.W + (x+dx))*4 + c]
                    self.px[o+c] = s // 9
    def vignette(self, strength=140):
        cx, cy = self.W/2, self.H/2
        maxd = math.hypot(cx, cy)
        for y in range(self.H):
            for x in range(self.W):
                d = math.hypot(x-cx, y-cy) / maxd
                dark = int(strength * max(0.0, d-0.45) / 0.55)
                if dark <= 0: continue
                o = (y*self.W + x)*4
                for c in range(3):
                    v = self.px[o+c] - dark
                    self.px[o+c] = v if v > 0 else 0

W, H = 208, 117

def stone_floor(c, tint):
    for ty in range(0, H, 16):
        for tx in range(0, W, 16):
            v = 26 + ((tx*7 + ty*13) % 9)
            c.rect(tx, ty, 16, 16, (v*tint[0]//32, v*tint[1]//32, v*tint[2]//32))
            c.rect(tx, ty, 16, 1, (14, 13, 17), 120)
            c.rect(tx, ty, 1, 16, (14, 13, 17), 120)

def shelves(c, x0, n, tint=(96, 72, 46)):
    # kitap rafları: dikey gövde + renkli sırt çizgileri
    for k in range(n):
        x = x0 + k*26
        c.rect(x, 14, 22, 60, tint)
        for row in range(4):
            c.rect(x+2, 18 + row*14, 18, 10, (30, 24, 18))
            for b in range(5):
                col = [(150,60,50),(70,110,150),(160,140,80),(90,130,90),(120,90,140)][ (b+row+k)%5 ]
                c.rect(x+3 + b*4, 19 + row*14, 3, 8, col)

# --- echo_kardes: salon, devrik masa, mor soluk ---
c = Canvas(W, H, rgb=(15, 14, 19))
stone_floor(c, (32, 30, 38))
c.rect(0, 0, W, 22, (10, 9, 14))
shelves(c, 8, 2, (80, 60, 40))
c.rect(120, 66, 40, 8, (86, 64, 42))     # devrik masa (yan)
c.rect(126, 58, 8, 8, (70, 52, 34))      # kırık çekmece
c.rect(136, 60, 6, 3, (40, 38, 44))      # hokka
for i in range(34):
    a = int(70 * (1.0 - i/34))
    c.rect(140, 26 + i, 4, 1, (150, 110, 210), a)
c.glow(142, 62, 24, (150, 110, 210), 70)
c.vignette(160)
save_png('assets/scenes/echo_kardes.png', W, H, c.px)
print("OK echo_kardes")

# --- kardeş anısı: yazan adam, sıcak mum ---
c = Canvas(W, H, rgb=(30, 24, 18))
stone_floor(c, (46, 38, 28))
c.rect(0, 0, W, 24, (16, 12, 10))
shelves(c, 12, 3)
c.rect(96, 62, 44, 10, (110, 84, 54))    # masa (ayakta)
c.rect(104, 46, 10, 16, (170, 140, 110)) # yazan adam (İdris — sırt)
c.rect(106, 41, 6, 6, (185, 152, 122))
c.rect(120, 58, 8, 3, (230, 224, 205))   # açık defter
c.glow(134, 52, 20, (255, 200, 110), 130) # mum
c.rect(0, 0, W, H, (255, 200, 120), 16)
c.vignette(105)
save_png('assets/scenes/echo_kardes_memory.png', W, H, c.px)
print("OK echo_kardes_memory")

# --- sömürü: soğuk, mor akış + fısıltı hissi ---
c = Canvas(W, H, rgb=(11, 10, 17))
stone_floor(c, (24, 22, 30))
c.rect(0, 0, W, 22, (8, 7, 12))
shelves(c, 8, 2, (56, 44, 34))
c.rect(120, 66, 40, 8, (60, 48, 36))
for i in range(50):
    a = int(95 * (1.0 - i/50))
    c.rect(136 + i//7, 18 + i, 6, 1, (165, 115, 235), a)
c.glow(142, 60, 40, (165, 115, 235), 120)
c.rect(0, 0, W, H, (90, 60, 160), 24)
c.vignette(180)
save_png('assets/scenes/echo_kardes_drain.png', W, H, c.px)
print("OK echo_kardes_drain")

# --- kayıt odası: raf duvarı + iki figür ---
c = Canvas(W, H, rgb=(19, 17, 22))
stone_floor(c, (36, 33, 40))
c.rect(0, 0, W, 20, (12, 11, 15))
shelves(c, 6, 7)
c.rect(78, 78, 9, 15, (150, 130, 105))   # oyuncu silüeti
c.rect(80, 73, 5, 6, (165, 142, 115))
c.rect(112, 78, 9, 14, (120, 125, 140))  # Nuh (gri-mavi ton)
c.rect(114, 73, 5, 6, (135, 140, 155))
c.glow(100, 40, 55, (200, 180, 255), 45) # rafların soluk parıltısı
c.vignette(130)
save_png('assets/scenes/kayit_odasi.png', W, H, c.px)
print("OK kayit_odasi")

# --- devir: mühür parlar ---
c = Canvas(W, H, rgb=(16, 13, 24))
stone_floor(c, (30, 27, 38))
shelves(c, 6, 7)
c.rect(94, 74, 9, 16, (150, 130, 105))
c.rect(96, 69, 5, 6, (165, 142, 115))
c.glow(98, 80, 34, (185, 120, 245), 160)  # göğüste mühür ışıması
c.glow(98, 80, 12, (240, 210, 255), 170)
c.rect(0, 0, W, H, (120, 80, 200), 22)
c.vignette(140)
save_png('assets/scenes/kayit_devir.png', W, H, c.px)
print("OK kayit_devir")

# --- kapanış: taş kapı, sıcak çıkış ışığı ---
c = Canvas(W, H, rgb=(20, 18, 22))
stone_floor(c, (38, 34, 40))
c.rect(84, 22, 40, 62, (70, 68, 78))     # taş kapı (kapanmış)
c.rect(88, 26, 32, 54, (58, 56, 66))
c.rect(100, 48, 8, 10, (110, 100, 130), 140)  # sönen mühür izi
c.glow(30, 90, 50, (255, 210, 140), 110) # çıkışa vuran gün ışığı
c.rect(0, 0, W, H, (255, 210, 150), 10)
c.vignette(120)
save_png('assets/scenes/kayit_kapat.png', W, H, c.px)
print("OK kayit_kapat")
