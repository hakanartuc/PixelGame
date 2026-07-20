#!/usr/bin/env python3
# Beceri ikon atlası (Cila): assets/icons/abilities.png — AbilityId sırasıyla
# 13 hücre 32×32. Büyüler magic_icons.png'nin çerçeveli 64px hücrelerinden
# TAM YARIYA indirilir (piksel-korumalı); savaş/yay ikonları prosedürel glif.
import zlib, struct

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

N = 29; CELL = 32   # 24-25 Yankıcı, 26-28 Kutsal (Faz HOLY)
W, H = N * CELL, CELL
px = bytearray(W * H * 4)

def put(x, y, rgb, a=255):
    if 0 <= x < W and 0 <= y < H:
        o = (y*W + x)*4
        px[o:o+4] = bytes([rgb[0], rgb[1], rgb[2], a])

def rect(x0, y0, w, h, rgb, a=255):
    for y in range(y0, y0+h):
        for x in range(x0, x0+w):
            put(x, y, rgb, a)

MAGIC = load_png('/Users/hakanartuc/Downloads/magic_icons.png')

def half_blit(sx, sy, cell):
    # 64px çerçeveli hücreyi 2×2 ortalamayla 32'ye indir (kutu filtre)
    MW, MH, im = MAGIC
    for y in range(32):
        for x in range(32):
            r = g = b = a = 0
            for dy in (0, 1):
                for dx in (0, 1):
                    o = ((sy + y*2 + dy)*MW + (sx + x*2 + dx))*4
                    r += im[o]; g += im[o+1]; b += im[o+2]; a += im[o+3]
            put(cell*CELL + x, y, (r//4, g//4, b//4), a//4)

def frame(cell, rgb=(96, 66, 42)):
    # Prosedürel glifler için ahşap çerçeve (büyü ikonlarıyla aynı dil)
    rect(cell*CELL, 0, 32, 32, (60, 40, 26))
    rect(cell*CELL+2, 2, 28, 28, rgb)
    rect(cell*CELL+4, 4, 24, 24, (126, 88, 56))

def sword(cell, blade, hilt=(210, 180, 90)):
    frame(cell)
    c = cell*CELL
    for i in range(14):                       # çapraz namlu (sol-alt → sağ-üst)
        rect(c + 8 + i, 22 - i, 2, 2, blade)
    rect(c + 9, 21, 4, 4, hilt)               # balçak
    rect(c + 6, 24, 4, 4, (120, 80, 50))      # kabza

def bow(cell, string=(230, 230, 240), arrow=(200, 160, 80), mark=None):
    frame(cell)
    c = cell*CELL
    for i in range(18):                       # yay gövdesi (dik yay)
        xoff = int(5 - 4.6 * abs(i - 9) / 9)
        rect(c + 9 + xoff, 7 + i, 2, 1, (150, 105, 60))
    rect(c + 10, 7, 1, 18, string)            # kiriş
    rect(c + 12, 15, 12, 2, arrow)            # ok gövdesi
    rect(c + 24, 14, 2, 4, (235, 235, 245))   # uç
    if mark == 'star':
        rect(c + 22, 8, 3, 3, (255, 220, 90))
    elif mark == 'ice':
        rect(c + 22, 8, 3, 3, (140, 210, 255))
    elif mark == 'volley':
        rect(c + 12, 10, 10, 2, arrow)
        rect(c + 12, 20, 10, 2, arrow)
    elif mark == 'burst':
        for dx, dy in [(-2, 0), (2, 0), (0, -2), (0, 2)]:
            rect(c + 25 + dx, 15 + dy, 2, 2, (255, 170, 70))

# 0 POWER_STRIKE — altın kılıç
sword(0, (245, 215, 130))
# 1 QUICK_SLASH — camgöbeği çift kesik
sword(1, (120, 220, 255))
rect(1*CELL + 20, 6, 2, 10, (120, 220, 255))   # ikinci iz
# 2 POWER_SHOT
bow(2, mark='star')
# 3 ICE_ARROW
bow(3, arrow=(140, 200, 250), mark='ice')
# 4 DISENGAGE — geri sıçrama: sola çift ok başı
frame(4)
for i in range(6):
    rect(4*CELL + 12 + i, 10 + i, 2, 2, (235, 235, 245))
    rect(4*CELL + 12 + i, 22 - i, 2, 2, (235, 235, 245))
    rect(4*CELL + 19 + i, 10 + i, 2, 2, (170, 170, 185))
    rect(4*CELL + 19 + i, 22 - i, 2, 2, (170, 170, 185))
# 5 VOLLEY
bow(5, mark='volley')
# 6 AOE_ARROW
bow(6, mark='burst')
# 7 FIREBOLT — magic_icons ateş (çerçeveli 64: 0,64)
half_blit(0, 64, 7)
# 8 ICE_NOVA — buz kümesi (320,224)
half_blit(320, 224, 8)
# 9 LIGHTNING — yıldırım (192,64)
half_blit(192, 64, 9)
# 10 FLAME_ARMOR — ateş + koruma halkası
half_blit(0, 64, 10)
c = 10*CELL
for i in range(0, 32, 3):                      # kesikli turuncu halka hissi
    put(c + i, 3, (255, 190, 90)); put(c + i, 28, (255, 190, 90))
    put(c + 3, i, (255, 190, 90)); put(c + 28, i, (255, 190, 90))
# 11 MAGIC_SHIELD — küre (320,64)
half_blit(320, 64, 11)
# 12 SPIKES — toprak dikenleri (192,224)
half_blit(192, 224, 12)

SNAKE = load_png('assets/characters/allies/snake.png')
TURTLE = load_png('assets/characters/allies/turtle.png')

def quarter_blit(src, sx, sy, cell):
    # 128px kareyi 4x4 ortalamayla 32'ye indir
    SW, SH, im = src
    for y in range(32):
        for x in range(32):
            r = g = b = a = 0
            for dy in range(4):
                for dx in range(4):
                    o = ((sy + y*4 + dy)*SW + (sx + x*4 + dx))*4
                    r += im[o]; g += im[o+1]; b += im[o+2]; a += im[o+3]
            put(cell*CELL + x, y, (r//16, g//16, b//16), a//16)

# 13 SUMMON_SNAKE — idle baş karesi (satır 2, kare 9)
frame(13, (46, 78, 40))
quarter_blit(SNAKE, 128, 256, 13)
# 14 SUMMON_TURTLE — kabuk karesi (satır 2, kare 9)
frame(14, (46, 78, 40))
quarter_blit(TURTLE, 128, 256, 14)

# Büyü dalgası 2 (tomar büyüleri): sheet karelerinden 32'ye indirme
CLAW = load_png('assets/fx/lightningclaw.png')
ICET = load_png('assets/fx/icetacle.png')
TORN = load_png('assets/fx/tornado.png')

# 15 LIGHTNING_CLAW — dolu pençe karesi (satır 2, kare 2 → 128,256... grid 4x4)
frame(15, (70, 54, 30))
quarter_blit(CLAW, 128, 256, 15)
# 16 ICETACLE — dolu diken karesi (satır 1, kare 3 → 384,128)
frame(16, (40, 62, 84))
quarter_blit(ICET, 384, 128, 16)
# 17 TORNADO — gövde karesi (satır 1, kare 1 → 128,128)
frame(17, (58, 66, 48))
quarter_blit(TORN, 128, 128, 17)

# Büyü dalgası 2 parça 2: yönlü sheet'lerden kareler
SNBI = load_png('assets/fx/snakebite_side.png')
FLIO = load_png('assets/fx/firelion_right.png')

# 18 SNAKEBITE — yılan başı karesi (satır 2, kare 1 → 0,256)
frame(18, (40, 78, 40))
quarter_blit(SNBI, 0, 256, 18)
# 19 FIRELION — dolu aslan karesi (satır 1, kare 2 → 256,128)
frame(19, (84, 48, 30))
quarter_blit(FLIO, 256, 128, 19)

# Two-Handed (Parça A): mızrak glifleri — sword() dilinde prosedürel
def spear(cell, tip=(220, 225, 235), shaft=(150, 105, 60)):
    frame(cell)
    c = cell*CELL
    for i in range(16):                        # çapraz gövde (sol-alt → sağ-üst)
        rect(c + 7 + i, 24 - i, 2, 2, shaft)
    rect(c + 22, 7, 3, 3, tip)                 # uç
    rect(c + 21, 10, 2, 2, tip)

# 20 STAB — mızrak + vuruş izi
spear(20)
rect(20*CELL + 24, 12, 4, 2, (255, 220, 90))
# 21 SWEEP — mızrak + yay izi (alt yarım daire hissi)
spear(21)
for i in range(10):
    rect(21*CELL + 8 + i*2, 26 - abs(i-5)//2, 2, 1, (120, 220, 255))
# 22 GATE_STANCE — mızrak + kapı çerçevesi
spear(22, shaft=(120, 90, 55))
rect(22*CELL + 5, 5, 2, 22, (200, 170, 110))
rect(22*CELL + 25, 5, 2, 22, (200, 170, 110))
rect(22*CELL + 5, 5, 22, 2, (200, 170, 110))

# 23 SHIELD_BASH — kalkan + vuruş yıldızı
frame(23, (60, 66, 80))
c = 23*CELL
for y in range(6, 24):                        # kalkan gövdesi
    w = 7 if y < 15 else 7 - (y - 15)
    if w < 1: continue
    rect(c + 15 - w, y, w*2, 1, (150, 155, 165))
rect(c + 13, 12, 4, 4, (230, 230, 240))       # göbek
for dx, dy in [(-4, 0), (4, 0), (0, -4), (0, 4)]:
    rect(c + 24 + dx//2, 10 + dy//2, 2, 2, (255, 220, 90))   # çarpma kıvılcımı

# 24 WHISPER_BURST — mor gölge patlaması (Yankıcı)
frame(24, (52, 38, 66))
c = 24*CELL
for r_ in range(4, 12, 3):                     # eşmerkezli mor halkalar
    for a_ in range(0, 360, 12):
        import math
        x = int(16 + r_ * math.cos(math.radians(a_)))
        y = int(16 + r_ * math.sin(math.radians(a_)))
        put(c + x, y, (150 + r_*6, 90, 210))
rect(c + 14, 14, 4, 4, (235, 210, 255))       # çekirdek

# 25 SHADOW_FORM — koyu silüet (Yankıcı)
frame(25, (40, 36, 52))
c = 25*CELL
rect(c + 13, 7, 6, 6, (25, 22, 36))           # baş
rect(c + 11, 13, 10, 12, (25, 22, 36))        # gövde
for i in range(5):                             # dağılan etek
    rect(c + 11 + i*2, 25 + (i % 2), 2, 2, (45, 40, 66))
rect(c + 14, 9, 2, 2, (140, 110, 200))        # göz parıltısı

# 26 HEAL_LIGHT — altın artı + ışıma (Kutsal)
frame(26, (96, 84, 46))
c = 26*CELL
rect(c + 13, 7, 6, 18, (255, 232, 150))       # dik kol
rect(c + 7, 13, 18, 6, (255, 232, 150))       # yatay kol
rect(c + 14, 8, 4, 16, (255, 250, 220))       # iç parlaklık
rect(c + 8, 14, 16, 4, (255, 250, 220))
for x_, y_ in [(6, 6), (25, 6), (6, 25), (25, 25)]:
    put(c + x_, y_, (255, 240, 180))           # köşe kıvılcımları

# 27 HOLY_BEAM — gökten inen hüzme (Kutsal)
frame(27, (96, 84, 46))
c = 27*CELL
for y_ in range(5, 24):                        # daralan ışık sütunu
    w_ = 8 - y_ // 5
    rect(c + 16 - w_ // 2, y_, w_, 1, (255, 240, 170))
rect(c + 15, 5, 2, 19, (255, 252, 230))       # çekirdek çizgi
for i in range(6):                             # zemin ışıması
    rect(c + 10 + i*2, 25, 2, 2, (255, 225, 140) if i % 2 else (255, 245, 200))

# 28 HOLY_ZONE — altın çember (Kutsal)
frame(28, (96, 84, 46))
c = 28*CELL
import math
for a_ in range(0, 360, 6):
    x = int(16 + 10 * math.cos(math.radians(a_)))
    y = int(20 + 5 * math.sin(math.radians(a_)))   # perspektifli elips
    put(c + x, y, (255, 232, 150))
    put(c + x, y - 1, (255, 245, 200))
rect(c + 14, 8, 4, 8, (255, 250, 220))        # merkez ışık süzmesi
rect(c + 15, 6, 2, 12, (255, 240, 170))

save_png('assets/icons/abilities.png', W, H, px)
print("OK assets/icons/abilities.png (29 hücre 32x32)")
