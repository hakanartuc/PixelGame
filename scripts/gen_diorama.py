#!/usr/bin/env python3
# Pixel diorama ureteci: mevcut asset'lerden vinyet gorseli kompoze eder.
# Minimal PNG decode/encode (zlib) — dis bagimlilik yok.
# "Devrik araba" yankisinin 4 paneli: giris / ani (sicak) / defin / somuru (soguk).
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
        # Yumusak radyal isik (gunes / mum / mor soluk)
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

W, H = 208, 117   # 16:9 — oyunda ~5x nearest cizilir

def grass_floor(c, tint):
    gpath = None
    for cand in ['assets/tiles/grass.png', 'assets/Tiles/Grass_1_Middle.png']:
        if os.path.exists(cand): gpath = cand; break
    if gpath:
        g = load_png(gpath)
        gw, gh = g[0], g[1]
        for ty in range(0, H, gh):
            for tx in range(0, W, gw):
                c.blit(g, 0, 0, min(gw,W-tx), min(gh,H-ty), tx, ty, tint=tint)

def road(c, rgb, alpha=200):
    c.rect(0, 62, W, 26, rgb, alpha=alpha)
    edge = tuple(v*3//4 for v in rgb)
    c.rect(0, 60, W, 2, edge, alpha=160)
    c.rect(0, 88, W, 2, edge, alpha=160)

CRATE = load_png('assets/loot/chest_small.png')
SKEL  = load_png('assets/characters/mobs/Skeleton_Swordman.png')

def bones(c, t1=(210,205,190), t2=(180,175,165)):
    c.blit(SKEL, 96, 192, 32, 32, 52, 66, tint=t1)   # olum animasyonu son karesi
    c.blit(SKEL, 96, 192, 32, 32, 146, 72, tint=t2)

def purple_column(c, x=128, w=5, height=46, peak=26):
    for y in range(0, height):
        a = int(peak * (1.0 - y/height))
        c.rect(x, y, w, 1, (150, 110, 210), alpha=a)

# --- [giris]: devrik araba, kemikler, ufukta soluk mor iz -------------------
c = Canvas(W, H, rgb=(24, 27, 24))
grass_floor(c, (70, 80, 66))
road(c, (52, 42, 30))
c.rect(86, 78, 30, 6, (10, 10, 8), alpha=120)               # golge
c.blit(CRATE, 0, 0, 48, 32, 80, 52, tint=(150,130,110))     # devrik kasalar
c.blit(CRATE, 96, 0, 48, 32, 118, 60, tint=(120,105,90))
bones(c)
purple_column(c)
c.vignette(150)
save_png('assets/scenes/echo_cart.png', W, H, c.px)
print("OK assets/scenes/echo_cart.png")

# --- [yanki]: ANI — olunun son sabahi. Sicak, canli: araba ayakta, kemik yok,
#     gun dogumu isigi. Gecmise bakiyoruz — her sey hala yolunda. -----------
c = Canvas(W, H, rgb=(44, 38, 26))
grass_floor(c, (120, 128, 74))                              # sabah cimeni
road(c, (96, 76, 50), alpha=210)                            # aydinlik yol
c.rect(60, 84, 44, 5, (26, 20, 12), alpha=110)              # uzun sabah golgesi
c.blit(CRATE, 0, 0, 48, 32, 66, 56, tint=(225,190,140))     # kasalar AYAKTA, saglam
c.blit(CRATE, 0, 0, 48, 32, 112, 54, tint=(205,175,130))
c.glow(30, 18, 60, (255, 215, 140), strength=170)           # dogan gunes
c.glow(150, 95, 40, (255, 200, 120), strength=40)           # yola vuran isik
c.rect(0, 0, W, H, (255, 205, 130), alpha=22)               # sicak ani tulu (sepya his)
c.vignette(95)                                              # ani yumusak cerceveli
save_png('assets/scenes/echo_cart_memory.png', W, H, c.px)
print("OK assets/scenes/echo_cart_memory.png")

# --- [defin]: mezar. Kemik yok, mor soluk sonmus; toprak yigini + isimsiz tas.
#     Karanlik ama sakin — "artik sahipsiz degil". ---------------------------
c = Canvas(W, H, rgb=(24, 27, 24))
grass_floor(c, (66, 76, 64))
road(c, (52, 42, 30))
c.blit(CRATE, 0, 0, 48, 32, 80, 52, tint=(130,112,96))      # kasalar duruyor (soluk)
c.blit(CRATE, 96, 0, 48, 32, 118, 60, tint=(105,92,80))
# mezar yigini: yol kenarinda kabarik toprak (kat kat, ustte dar)
c.rect(46, 78, 34, 8, (58, 44, 30), alpha=235)
c.rect(50, 74, 26, 5, (66, 50, 34), alpha=235)
c.rect(55, 71, 16, 4, (74, 57, 39), alpha=235)
# isimsiz tas: dikili gri blok + hafif isik yakalamasi
c.rect(60, 58, 7, 14, (98, 100, 106), alpha=255)
c.rect(61, 56, 5, 3, (112, 114, 120), alpha=255)
c.rect(60, 58, 2, 12, (128, 130, 138), alpha=160)           # sol kenara vuran isik
c.glow(63, 66, 22, (255, 220, 160), strength=26)            # tasin ustunde cok hafif sicaklik
purple_column(c, peak=7)                                    # mor iz NEREDEYSE sonmus
c.vignette(140)
save_png('assets/scenes/echo_cart_burial.png', W, H, c.px)
print("OK assets/scenes/echo_cart_burial.png")

# --- [somur]: SOMURU — ayni yer ama soguk ve yanlis. Mor sutun buyumus,
#     isik sana akiyor; kemikler karanliga gomulu. ---------------------------
c = Canvas(W, H, rgb=(15, 15, 22))
grass_floor(c, (42, 48, 52))                                # soguk, morarmis cimen
road(c, (38, 32, 30), alpha=200)
c.blit(CRATE, 0, 0, 48, 32, 80, 52, tint=(95,85,95))        # kasalar renksizlesmis
c.blit(CRATE, 96, 0, 48, 32, 118, 60, tint=(78,70,82))
bones(c, t1=(135,130,145), t2=(110,106,122))                # kemikler soluk/soguk
purple_column(c, x=124, w=13, height=70, peak=95)           # sutun BUYUMUS
purple_column(c, x=120, w=21, height=52, peak=40)
c.glow(130, 62, 44, (165, 110, 235), strength=130)          # cekilen ozun merkezi
c.rect(0, 0, W, H, (110, 70, 180), alpha=20)                # sahneye mor tul
c.vignette(175)                                             # karanlik iyice bastirir
save_png('assets/scenes/echo_cart_drain.png', W, H, c.px)
print("OK assets/scenes/echo_cart_drain.png")

# --- [yazit]: dikilitas yakin plani — koyden eski, tarihsiz. Ay isigi altinda.
OBELISK = load_png('assets/decor/obelisk.png')
c = Canvas(W, H, rgb=(16, 18, 24))
grass_floor(c, (48, 58, 56))                                # gece cimeni (soguk)
c.rect(70, 96, 66, 8, (8, 9, 10), alpha=130)                # tas golgesi
# tas: 32x80 sprite'i merkezde (dev gibi dursun diye alan bos birakilir)
c.blit(OBELISK, 0, 0, 32, 80, 88, 18, tint=(190,195,210))
# kazinmis izler: tasin govdesine yatay cizikler (yazi hissi — okunamayacak kadar eski)
for i, yy in enumerate(range(34, 74, 5)):
    wln = 16 - (i % 3) * 4
    c.rect(96, yy, wln, 1, (60, 64, 78), alpha=200)
c.rect(98, 80, 10, 1, (90, 80, 60), alpha=180)              # dipte YENI kazinti (farkli ton)
c.glow(104, 30, 34, (150, 170, 220), strength=46)           # ay isigi tepede
c.glow(104, 92, 26, (90, 100, 140), strength=24)            # dipte los yansima
c.rect(0, 0, W, H, (70, 90, 140), alpha=14)                 # gece tulu
c.vignette(150)
save_png('assets/scenes/yazit_stone.png', W, H, c.px)
print("OK assets/scenes/yazit_stone.png")
