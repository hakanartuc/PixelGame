#!/usr/bin/env python3
# Yuva gorselleri (Mob Ekolojisi E1): assets/decor/nests.png — 4 hucre 48x48.
#   0: Ork SAVAS TOTEMI (ahsap direk + kurukafa + savas boyasi + dip kaziklari)
#   1: LANETLI MEZAR (tas + kemikler + mor parilti)
#   2: MUHURLU MEZAR (ayni tas, kemik yok, camgobegi muhur cizgisi)
#   3: SLIME GOLCUGU (acik gri jol — oyunda renk tint'iyle yesil/mavi olur)
# Kaynaklar: Cute Fantasy Desert_Bones + Wood_Spikes kirpmalari (kullanim onayli).
import zlib, struct, math

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

CW, CH = 48, 48
W, H = CW * 4, CH
px = bytearray(W * H * 4)   # tamamen seffaf baslar

def put(x, y, rgb, a=255):
    if x < 0 or x >= W or y < 0 or y >= H: return
    o = (y*W + x)*4
    if a >= 255:
        px[o:o+4] = bytes([rgb[0], rgb[1], rgb[2], 255])
    else:
        oa = px[o+3]
        na = a + oa*(255-a)//255
        for c in range(3):
            px[o+c] = (rgb[c]*a + px[o+c]*oa*(255-a)//255) // max(na,1)
        px[o+3] = na

def rect(x0, y0, w, h, rgb, a=255):
    for y in range(y0, y0+h):
        for x in range(x0, x0+w):
            put(x, y, rgb, a)

def blit(src, sx, sy, sw, sh, dx, dy):
    SW, SH, im = src
    for y in range(sh):
        for x in range(sw):
            so = ((sy+y)*SW + (sx+x))*4
            if im[so+3] < 8: continue
            put(dx+x, dy+y, (im[so], im[so+1], im[so+2]), im[so+3])

BONES  = load_png('/Users/hakanartuc/Cute Fantasy/Cute_Fantasy_Desert/Props/Desert_Bones.png')
SPIKES = load_png('/Users/hakanartuc/Cute Fantasy/Cute_Fantasy_MilitaryCamp/Wood_Spikes.png')

# --- Hücre 0: ORK SAVAŞ TOTEMİ ------------------------------------------------
ox = 0
rect(ox+21, 8, 6, 34, (86, 60, 38))          # ana direk
rect(ox+21, 8, 2, 34, (110, 80, 52))         # sol ışık kenarı
rect(ox+25, 8, 2, 34, (62, 42, 26))          # sağ gölge
rect(ox+16, 14, 16, 3, (74, 50, 32))         # çapraz kol
rect(ox+18, 24, 12, 4, (150, 40, 34))        # savaş boyası bandı (kızıl)
rect(ox+18, 32, 12, 2, (150, 40, 34))
blit(BONES, 40, 12, 30, 26, ox+9, -2)        # tepede kurukafa (Desert_Bones)
blit(SPIKES, 0, 24, 32, 24, ox+4, 30)        # dipte kazıklar
blit(SPIKES, 32, 24, 32, 24, ox+20, 32)
rect(ox+12, 44, 26, 3, (30, 26, 22, ), 130)  # zemin gölgesi

# --- Hücre 1: LANETLİ MEZAR ---------------------------------------------------
gx = CW
rect(gx+18, 16, 12, 22, (96, 98, 106))       # dikili taş
rect(gx+20, 13, 8, 4, (108, 110, 120))       # tepe yuvarlağı hissi
rect(gx+18, 16, 3, 20, (124, 126, 136))      # sol ışık
rect(gx+27, 16, 3, 22, (74, 76, 84))         # sağ gölge
rect(gx+20, 22, 8, 2, (60, 50, 84))          # kazınmış çizgi (eski yazı)
rect(gx+20, 27, 8, 2, (60, 50, 84))
rect(gx+12, 38, 24, 4, (58, 48, 36))         # kabarık toprak
blit(BONES, 4, 92, 26, 18, gx+2, 32)         # kemikler (kaburgalar)
blit(BONES, 118, 60, 26, 20, gx+26, 30)
for i in range(6):                            # mor sızıntı parıltısı
    rect(gx+16+i*3, 12-i%3, 2, 2, (150, 110, 210), 90)

# --- Hücre 2: MÜHÜRLÜ MEZAR (temiz, soğuk, sakin) ------------------------------
sx = CW*2
rect(sx+18, 16, 12, 22, (96, 98, 106))
rect(sx+20, 13, 8, 4, (108, 110, 120))
rect(sx+18, 16, 3, 20, (124, 126, 136))
rect(sx+27, 16, 3, 22, (74, 76, 84))
rect(sx+12, 38, 24, 4, (58, 48, 36))
# mühür işareti: camgöbeği daire + dik çizgi (mühür motifi — intro'yla aynı dil)
rect(sx+22, 20, 4, 10, (140, 220, 235), 200)
rect(sx+20, 24, 8, 3, (140, 220, 235), 160)
rect(sx+21, 18, 6, 2, (200, 245, 255), 120)

# --- Hücre 3: SLİME GÖLCÜĞÜ (açık gri — oyunda tint) ---------------------------
px_ = CW*3
for y in range(28, 45):
    for x in range(6, 42):
        dx, dy = (x - 24) / 18.0, (y - 36.5) / 8.5
        d = dx*dx + dy*dy
        if d < 1.0:
            base = 205 if d < 0.55 else 180
            put(px_+x, y, (base, base+6, base), 235)
rect(px_+14, 31, 8, 3, (245, 250, 246), 160)   # parlama
rect(px_+28, 36, 5, 2, (240, 246, 242), 120)
for bx, by in [(12, 34), (30, 40), (22, 38)]:  # kabarcıklar
    rect(px_+bx, by, 3, 3, (230, 238, 232), 180)

save_png('assets/decor/nests.png', W, H, px)
print("OK assets/decor/nests.png (4 hücre 48x48)")
