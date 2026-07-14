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


TAB = load_png('/Users/hakanartuc/Cute Fantasy/Cute_Fantasy/Buildings/House_Decor/Tables.png')

W, H = 40, 44
px = bytearray(W*H*4)
c = Canvas.__new__(Canvas)
c.W, c.H = W, H
c.px = bytearray(W*H*4)   # şeffaf zemin

# beyaz örtülü küçük masa (satır 3, ikinci sütun ~ x48,y192)
c.blit(TAB, 48, 196, 32, 44, 4, 0)
# iki tabak
for cx in (12, 26):
    for y in range(8, 13):
        for x in range(cx-3, cx+4):
            d=(x-cx)**2 + (y-10)**2*2
            if d <= 9: c.px[(y*W+x)*4:(y*W+x)*4+4] = bytes([228,224,214,255])
            if d <= 2: c.px[(y*W+x)*4:(y*W+x)*4+4] = bytes([200,195,185,255])
# mum: küçük gövde + alev
for y in range(4, 9):
    for x in range(18, 21):
        c.px[(y*W+x)*4:(y*W+x)*4+4] = bytes([235,230,210,255])
c.px[(3*W+19)*4:(3*W+19)*4+4] = bytes([255,200,90,255])
c.px[(2*W+19)*4:(2*W+19)*4+4] = bytes([255,160,60,255])

save_png('assets/decor/sofra.png', W, H, c.px)
print("OK assets/decor/sofra.png")
