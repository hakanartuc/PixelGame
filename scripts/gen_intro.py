#!/usr/bin/env python3
# ============================================================================
# HUKUMSUZ — 2026-07-31. Yerine: scripts/gen_intro_demirboyun.py
#
# Bu uretec v0 kanonuna ("Sessiz Gun + Yanki") ait dort paneli uretiyordu:
#   intro_frozen (donmus dunya) / intro_pulse (dokuz atim, mor muhur halkasi) /
#   intro_memory (baskasinin anisi, cukur) / intro_wake (uyanis + gogusteki muhur)
#
# Uc panel de oyuncuya SECILMISLIK dayatiyordu; LORE v2 (DEMIRBOYUN) bunu yasakliyor
# (1.8 fantastik asla zorunluluk degil, 1.9 kimse kiyamet konusmuyor, ve "kimlik
# davranistan dogar" sutunu). "Sessiz Gun" kavrami LORE v2'de HIC gecmiyor.
#
# SILINMEDI (kanon kurali 15.8: bosa cikan is rafa kalkar). Ciktilari da
# assets/scenes/intro_{frozen,pulse,memory,wake}.png olarak duruyor ama artik
# hicbir sahne dosyasi onlara BASVURMUYOR. intro_pulse.png kurtarilamaz
# (esmerkezli mor halka = muhur atimi); digerleri baska sahnelerde kullanilabilir.
#
# BU URETECI CALISTIRMAYIN — ciktilari kanona aykiri.
# ============================================================================
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

W, H = 208, 117   # 16:9 — oyunda ~5x nearest cizilir

def grass_floor(c, tint):
    g = load_png('assets/tiles/Grass/Grass_1_Middle.png')
    gw, gh = g[0], g[1]
    for ty in range(0, H, gh):
        for tx in range(0, W, gw):
            c.blit(g, 0, 0, min(gw,W-tx), min(gh,H-ty), tx, ty, tint=tint)

def road(c, rgb, alpha=200):
    c.rect(0, 62, W, 26, rgb, alpha=alpha)
    edge = tuple(v*3//4 for v in rgb)
    c.rect(0, 60, W, 2, edge, alpha=160)
    c.rect(0, 88, W, 2, edge, alpha=160)

HOUSE  = load_png('assets/buildings/House_1_Limestone_Base_Red.png')
FARMER = load_png('assets/characters/npcs/Farmer_Bob.png')
CHICK  = load_png('assets/characters/mobs/Chicken.png')

# --- [giris]: DONMUS DUNYA — koy sokagi, adim ortasinda insanlar, HAVADA
#     asili kalmis tavuk ve yapraklar. Renk soluk, mavi-gri: nefes tutulmus. --
c = Canvas(W, H, rgb=(30, 33, 40))
grass_floor(c, (58, 66, 72))                                 # soluk soguk cimen
road(c, (48, 44, 44))
c.blit(HOUSE, 0, 0, 96, 96, 8, -6, tint=(120,118,135))       # solda ev (soluk)
c.blit(FARMER, 32, 0, 32, 32, 128, 58, tint=(150,150,165))   # adim ortasinda donmus
c.blit(FARMER, 0, 32, 32, 32, 88, 64, tint=(135,135,152))    # ikinci figur
c.blit(CHICK, 0, 0, 32, 32, 158, 30, tint=(170,170,180))     # tavuk HAVADA asili
c.rect(160, 74, 14, 3, (8, 8, 10), alpha=90)                 # altinda golgesi — yere degmiyor
for lx, ly in [(52,26),(60,38),(146,20),(178,44),(38,50),(112,30),(96,14)]:
    c.rect(lx, ly, 2, 2, (96, 110, 84), alpha=220)           # havada asili yapraklar
c.rect(0, 0, W, H, (120, 140, 175), alpha=26)                # soguk tul: dunya soluk
c.vignette(150)
save_png('assets/scenes/intro_frozen.png', W, H, c.px)
print("OK assets/scenes/intro_frozen.png")

# --- [atim]: DOKUZ ATIM — tam karanlik, ortada muhur nabzi. Dokuz halka:
#     ic ice, disari dogru soluyor. Kalp degil; daha eski bir sey sayiyor. ---
c = Canvas(W, H, rgb=(8, 6, 12))
for i in range(9):
    r = 8 + i * 10
    a = int(95 * (1.0 - i/10.0))
    c.ring(104, 58, r, (150, 100, 220), alpha=a, thick=1.8)
c.glow(104, 58, 26, (190, 120, 255), strength=150)           # cekirdek: muhur
c.glow(104, 58, 9, (255, 210, 235), strength=170)            # ic isik
c.rect(103, 52, 3, 13, (240, 200, 250), alpha=170)           # muhur cizgisi (dik)
c.rect(99, 57, 11, 3, (240, 200, 250), alpha=140)            # muhur cizgisi (yatay)
c.vignette(190)
save_png('assets/scenes/intro_pulse.png', W, H, c.px)
print("OK assets/scenes/intro_pulse.png")

# --- [ani]: BASKASININ ANISI — bulanik: kazan eller, cukur, cukurdan isik,
#     arkada BAKAN sey. Tek gecis blur: ruya/odunc hafiza dili. --------------
c = Canvas(W, H, rgb=(16, 13, 11))
grass_floor(c, (40, 36, 30))                                 # gece topragi
c.rect(70, 66, 66, 30, (24, 17, 12), alpha=245)              # kazilmis cukur
c.rect(66, 62, 74, 5, (52, 40, 26), alpha=220)               # cukur agzi topragi
c.rect(58, 92, 22, 7, (48, 36, 24), alpha=235)               # kenara atilmis toprak
c.rect(130, 90, 18, 6, (48, 36, 24), alpha=235)
c.glow(103, 82, 17, (255, 190, 110), strength=120)           # cukurun dibindeki parilti
# kazan eller: iki soluk ten blok, cukura uzanmis (bilek + parmak imasi)
c.rect(84, 58, 7, 12, (172, 132, 104), alpha=255)
c.rect(83, 68, 9, 4, (150, 112, 88), alpha=255)
c.rect(116, 56, 7, 13, (172, 132, 104), alpha=255)
c.rect(115, 67, 9, 4, (150, 112, 88), alpha=255)
# BAKAN sey: ufukta uzun karanlik silhouette + iki cok soluk goz
c.rect(168, 8, 12, 52, (6, 5, 9), alpha=235)
c.rect(170, 18, 3, 2, (140, 110, 190), alpha=120)
c.rect(175, 18, 3, 2, (140, 110, 190), alpha=120)
c.blur()                                                     # odunc ani: net degil
c.rect(0, 0, W, H, (60, 40, 90), alpha=26)                   # mor ruya tulu
c.vignette(185)
save_png('assets/scenes/intro_memory.png', W, H, c.px)
print("OK assets/scenes/intro_memory.png")

# --- [uyanis]: yol kenarinda sabah. Islak cimen, dogan gun, gogsunde muhur
#     nabzi; batida (solda) koyun dumani. Dunya sicak — ama isaret kalici. ---
c = Canvas(W, H, rgb=(34, 38, 30))
grass_floor(c, (96, 110, 66))                                # sabah cimeni
road(c, (78, 62, 42), alpha=190)
c.blit(HOUSE, 0, 0, 96, 96, -58, 6, tint=(150,140,150))      # solda uzak ev kenari
c.rect(30, 10, 3, 26, (150, 150, 160), alpha=60)             # koy dumani (soluk sutun)
c.rect(32, 4, 3, 10, (160, 160, 170), alpha=40)
c.glow(178, 14, 66, (255, 215, 140), strength=160)           # dogan gun (dogu)
for dx, dy in [(24,84),(58,96),(90,80),(140,92),(170,86),(114,102)]:
    c.rect(dx, dy, 1, 1, (235, 245, 255), alpha=190)         # cimen cig damlalari
c.glow(104, 96, 16, (185, 120, 245), strength=120)           # gogusteki muhur nabzi
c.glow(104, 96, 6, (240, 200, 255), strength=150)
c.rect(0, 0, W, H, (255, 210, 150), alpha=14)                # sicak sabah tulu
c.vignette(110)
save_png('assets/scenes/intro_wake.png', W, H, c.px)
print("OK assets/scenes/intro_wake.png")
