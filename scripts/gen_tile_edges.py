#!/usr/bin/env python3
# Cimen kenar kaplamalari (harita gorsel yenileme): Grass_1_Middle'dan
# dalgali alpha maskeli 12 kaplama uretir — cimen komsu tabana "tasar".
# Atlas: 12 hucre x 16px = 192x16 (N,S,W,E, NW,NE,SW,SE dis kose, ic koseler)
# + tas/bataklik/diken 16px dokulari (eksik tile dokulari).
import zlib, struct, math

def load_png(path):
    d=open(path,'rb').read(); pos=8; W=H=None; ct=None; idat=b''
    while pos < len(d):
        ln=struct.unpack('>I',d[pos:pos+4])[0]; typ=d[pos+4:pos+8]; data=d[pos+8:pos+8+ln]
        if typ==b'IHDR': W,H,bd,ct=struct.unpack('>IIBB',data[:10])
        elif typ==b'IDAT': idat+=data
        pos+=12+ln
    raw=zlib.decompress(idat); ch=4 if ct==6 else 3; stride=W*ch
    img=bytearray(W*H*4); prev=bytearray(stride); p=0
    for y in range(H):
        f=raw[p]; p+=1; line=bytearray(raw[p:p+stride]); p+=stride
        if f==1:
            for i in range(ch,stride): line[i]=(line[i]+line[i-ch])&255
        elif f==2:
            for i in range(stride): line[i]=(line[i]+prev[i])&255
        elif f==3:
            for i in range(stride):
                a=line[i-ch] if i>=ch else 0
                line[i]=(line[i]+((a+prev[i])>>1))&255
        elif f==4:
            for i in range(stride):
                a=line[i-ch] if i>=ch else 0
                b=prev[i]; c=prev[i-ch] if i>=ch else 0
                pp=a+b-c; pa,pb,pc=abs(pp-a),abs(pp-b),abs(pp-c)
                pr=a if (pa<=pb and pa<=pc) else (b if pb<=pc else c)
                line[i]=(line[i]+pr)&255
        prev=line
        for x in range(W):
            o=(y*W+x)*4
            img[o]=line[x*ch]; img[o+1]=line[x*ch+1]; img[o+2]=line[x*ch+2]
            img[o+3]=line[x*ch+3] if ch==4 else 255
    return W,H,img

def save_png(path, W, H, px):
    raw=b''.join(b'\x00'+bytes(px[y*W*4:(y+1)*W*4]) for y in range(H))
    def chunk(t,d):
        c=struct.pack('>I',len(d))+t+d
        return c+struct.pack('>I',zlib.crc32(t+d)&0xffffffff)
    png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',W,H,8,6,0,0,0))
    png+=chunk(b'IDAT',zlib.compress(raw,9))+chunk(b'IEND',b'')
    open(path,'wb').write(png)

GW,GH,G = load_png('assets/tiles/Grass/Grass_1_Middle.png')   # 16x16 cimen

def h2(x, y):   # deterministik dalga (koordinat hash)
    v = (x*73856093 ^ y*19349663) & 0xffff
    return v / 65535.0

# Dalga derinligi: kenar boyunca 3-6 px arasi (2px adimlarla — pixel-art dili)
def depth(i):
    return 3 + int(h2(i, 7) * 3.0)

CELL=16; N=12
W,H = CELL*N, CELL
px = bytearray(W*H*4)
def put(cell, x, y):
    src=(y*GW+x)*4; dst=(y*W+cell*CELL+x)*4
    px[dst:dst+4] = G[src:src+4]

# 0 N: komsu YUKARIDA cimen → ust seritte cimen (asagi dalgali)
for x in range(16):
    for y in range(depth(x)): put(0, x, y)
# 1 S
for x in range(16):
    for y in range(16-depth(x+31), 16): put(1, x, y)
# 2 W
for y in range(16):
    for x in range(depth(y+17)): put(2, x, y)
# 3 E
for y in range(16):
    for x in range(16-depth(y+53), 16): put(3, x, y)
# 4-7 DIS koseler (yalniz caprazda cimen): kucuk cimen dili
# NW capraz → sol-ust kosede ceyrek yuvarlak
def corner(cell, cx, cy):
    for y in range(16):
        for x in range(16):
            dx = x - cx; dy = y - cy
            r = 4.5 + h2(x+cell*7, y)*1.5
            if dx*dx + dy*dy < r*r: put(cell, x, y)
corner(4, 0, 0)    # NW
corner(5, 15, 0)   # NE
corner(6, 0, 15)   # SW
corner(7, 15, 15)  # SE
# 8-11 IC koseler: iki kenar birlesimindeki bosluk dolgusu (L kose yumusatma)
def inner(cell, cx, cy):
    for y in range(16):
        for x in range(16):
            dx = x - cx; dy = y - cy
            r = 6.0 + h2(x+cell*13, y)*1.5
            if dx*dx + dy*dy < r*r: put(cell, x, y)
inner(8, 0, 0); inner(9, 15, 0); inner(10, 0, 15); inner(11, 15, 15)
save_png('assets/tiles/Grass/grass_edges.png', W, H, px)
print('OK grass_edges.png (12 hucre)')

# --- Eksik dokular: tas / bataklik / diken (16px, prosedurel ama dokulu) ---
def texture16(path, base, fleck, fleckN, dark_edges=False, seed=1):
    p = bytearray(16*16*4)
    for y in range(16):
        for x in range(16):
            v = h2(x*seed+1, y*seed+3)
            r = int(base[0] * (0.92 + v*0.16))
            g = int(base[1] * (0.92 + v*0.16))
            b = int(base[2] * (0.92 + v*0.16))
            o=(y*16+x)*4; p[o:o+4]=bytes([min(r,255),min(g,255),min(b,255),255])
    for k in range(fleckN):
        fx=int(h2(k,seed)*16); fy=int(h2(k+40,seed)*16)
        o=(fy*16+fx)*4; p[o:o+4]=bytes([fleck[0],fleck[1],fleck[2],255])
    save_png(path, 16, 16, p)

texture16('assets/tiles/terrain/stone.png',  (112,114,126), (140,144,158), 9, seed=3)
texture16('assets/tiles/terrain/swamp.png',  (86,102,64),   (60,74,46),  12, seed=5)
texture16('assets/tiles/terrain/thorns.png', (96,72,52),    (168,52,40), 10, seed=7)
print('OK tas/bataklik/diken dokulari')
