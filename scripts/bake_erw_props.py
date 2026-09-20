#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# EPIC RPG World prop/dekor kesimleri — Faz EPIC-1 dilim 2 (çayır doğa + köy
# evleri ithali), 2026-09-20.
#
# Kaynaklar paketin HAZIR KESİLMİŞ tekil sprite'ları ("-sprites" klasörleri) —
# koordinat çıkarma yalnız Premade_houses sayfasında var, orada da kaba kutu +
# ALFA-TRIM (kesin sınırı alfa tarar, elle piksel sayılmaz).
#
# ANİMASYONLU AĞAÇLAR: GL 1.6'nın 8'er karelik rüzgar animasyonları tek yatay
# şeride paketlenir. Kareler ÖNCE ortak (union) opak kutuya kırpılır —
# decor.c'nin gölge/yansıma yolu şeritlerde "kare kutusu = opak kutu" varsayar
# (r2d'nin alfa taraması tüm şeridi tek kutu görür, kare başına bakamaz).
#
# Çıktılar: assets/decor/erw/ + assets/buildings/erw/ (evler ithal —
# yerleştirme KÖY-YENİDEN diliminde). Kullanım: python3 scripts/bake_erw_props.py
import os, struct, sys, zlib

PACK = sys.argv[1] if len(sys.argv) > 1 else \
    "/Users/hakan/Assets/2D Pixel-Art/Epic RPG"
GL20 = os.path.join(PACK, "ERW - Grass Land 2.0 v2.1/Props/Static props")
GL16 = os.path.join(PACK, "EPIC RPG World Pack - Grass Land V. 1.6/Props")
VIL = os.path.join(PACK, "Epic RPG World - The Village V2.2")
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
DEC = os.path.join(ROOT, "assets/decor/erw")
BLD = os.path.join(ROOT, "assets/buildings/erw")

# --- Saf-python PNG codec (bake_erw_terrain.py ile aynı) -------------------
def png_read_rgba(path):
    d = open(path, "rb").read()
    assert d[:8] == b"\x89PNG\r\n\x1a\n", "PNG degil: " + path
    pos, w, h, idat = 8, 0, 0, b""
    while pos < len(d):
        ln = struct.unpack(">I", d[pos:pos + 4])[0]
        tag = d[pos + 4:pos + 8]
        body = d[pos + 8:pos + 8 + ln]
        if tag == b"IHDR":
            w, h, bit, ct = struct.unpack(">IIBB", body[:10])
            assert bit == 8 and ct == 6, f"beklenen 8-bit RGBA ({path}): bit={bit} ct={ct}"
        elif tag == b"IDAT":
            idat += body
        pos += 12 + ln
    raw = zlib.decompress(idat)
    stride = w * 4
    out = bytearray(h * stride)
    prev = bytearray(stride)
    p = 0
    for y in range(h):
        f = raw[p]; p += 1
        line = bytearray(raw[p:p + stride]); p += stride
        for x in range(stride):
            a = line[x - 4] if x >= 4 else 0
            b_ = prev[x]
            c = prev[x - 4] if x >= 4 else 0
            if f == 1: line[x] = (line[x] + a) & 0xFF
            elif f == 2: line[x] = (line[x] + b_) & 0xFF
            elif f == 3: line[x] = (line[x] + (a + b_) // 2) & 0xFF
            elif f == 4:
                pa, pb, pc = abs(b_ - c), abs(a - c), abs(a + b_ - 2 * c)
                pr = a if (pa <= pb and pa <= pc) else (b_ if pb <= pc else c)
                line[x] = (line[x] + pr) & 0xFF
        out[y * stride:(y + 1) * stride] = line
        prev = line
    return w, h, out

def png_write_rgba(path, w, h, pix):
    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data
                + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))
    stride = w * 4
    raw = b"".join(b"\x00" + bytes(pix[y * stride:(y + 1) * stride]) for y in range(h))
    open(path, "wb").write(b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))

def bbox(px, w, h, x0=0, y0=0, x1=None, y1=None):
    """Alfa>8 sınır kutusu (verilen pencere içinde)."""
    if x1 is None: x1 = w
    if y1 is None: y1 = h
    mnx, mny, mxx, mxy = x1, y1, -1, -1
    for y in range(y0, y1):
        row = (y * w) * 4
        for x in range(x0, x1):
            if px[row + x * 4 + 3] > 8:
                if x < mnx: mnx = x
                if x > mxx: mxx = x
                if y < mny: mny = y
                if y > mxy: mxy = y
    assert mxx >= 0, "pencerede opak piksel yok"
    return mnx, mny, mxx - mnx + 1, mxy - mny + 1

def crop(px, w, h, cx, cy, cw, ch):
    out = bytearray(cw * ch * 4)
    for y in range(ch):
        so = ((cy + y) * w + cx) * 4
        out[y * cw * 4:(y + 1) * cw * 4] = px[so:so + cw * 4]
    return out

os.makedirs(DEC, exist_ok=True)
os.makedirs(BLD, exist_ok=True)
rapor = []

def copy_trim(src, outp):
    w, h, px = png_read_rgba(src)
    x, y, bw, bh = bbox(px, w, h)
    png_write_rgba(outp, bw, bh, crop(px, w, h, x, y, bw, bh))
    rapor.append(f"{os.path.basename(outp)}: {bw}x{bh}")

# 1) Tekil dekorlar (GL 2.0 hazır kesimleri + GL 1.6 kuyu)
copy_trim(os.path.join(GL20, "sheet1-sprites/bush 1.png"), os.path.join(DEC, "bush_1.png"))
copy_trim(os.path.join(GL20, "sheet1-sprites/bush 5.png"), os.path.join(DEC, "bush_2.png"))
copy_trim(os.path.join(GL20, "sheet1-sprites/barrel 1.png"), os.path.join(DEC, "barrel_1.png"))
copy_trim(os.path.join(GL20, "sheet1-sprites/barrel 10.png"), os.path.join(DEC, "barrel_2.png"))
copy_trim(os.path.join(VIL, "tilesets and props/props-sprites/questboard_1.png"),
          os.path.join(DEC, "notice_board.png"))
copy_trim(os.path.join(VIL, "tilesets and props/props-sprites/lamp-post1.png"),
          os.path.join(DEC, "lamp_post.png"))
copy_trim(os.path.join(GL20, "sheet2-sprites/altar - only stones.png"),
          os.path.join(DEC, "ritual_stones.png"))
copy_trim(os.path.join(GL16, "water well-rope.png"),   # Props kökünde (statik hal)
          os.path.join(DEC, "well.png"))

# 2) Çit: 3 düz segment yan yana (tek 32'lik parça direk gibi kalıyordu)
segs = [png_read_rgba(os.path.join(GL20, f"sheet1-sprites/fence - left - right - {i}.png"))
        for i in (1, 2, 3)]
fw = sum(s[0] for s in segs); fh = max(s[1] for s in segs)
fpx = bytearray(fw * fh * 4)
xo = 0
for (sw, sh, spx) in segs:
    for y in range(sh):
        d = (y * fw + xo) * 4
        fpx[d:d + sw * 4] = spx[y * sw * 4:(y + 1) * sw * 4]
    xo += sw
png_write_rgba(os.path.join(DEC, "fence.png"), fw, fh, fpx)
rapor.append(f"fence.png: {fw}x{fh}")

# 3) Animasyonlu ağaçlar: 8 kare → union-trim → tek yatay şerit
def tree_strip(n, outp):
    frames = [png_read_rgba(os.path.join(
        GL16, f"Atlas-Props-individual sprites/Animated Tree{n}_frame{i}.png"))
        for i in range(1, 9)]
    w, h = frames[0][0], frames[0][1]
    ux0, uy0, ux1, uy1 = w, h, 0, 0
    for (fw_, fh_, px) in frames:
        assert fw_ == w and fh_ == h, "kare boyutlari farkli"
        x, y, bw, bh = bbox(px, w, h)
        ux0 = min(ux0, x); uy0 = min(uy0, y)
        ux1 = max(ux1, x + bw); uy1 = max(uy1, y + bh)
    cw, ch = ux1 - ux0, uy1 - uy0
    strip = bytearray(cw * 8 * ch * 4)
    for i, (fw_, fh_, px) in enumerate(frames):
        c = crop(px, w, h, ux0, uy0, cw, ch)
        for y in range(ch):
            d = (y * cw * 8 + i * cw) * 4
            strip[d:d + cw * 4] = c[y * cw * 4:(y + 1) * cw * 4]
    png_write_rgba(outp, cw * 8, ch, strip)
    rapor.append(f"{os.path.basename(outp)}: 8 kare x {cw}x{ch}")

tree_strip(1, os.path.join(DEC, "tree1_anim.png"))
tree_strip(2, os.path.join(DEC, "tree2_anim.png"))

# 4) Evler: Premade_houses sayfasından kaba kutu + alfa-trim.
# Kutular sayfanın 2048 ızgarasında elle seçildi (görsel iterasyonla);
# trim kesin sınırı bulur. KULLANIM: KÖY-YENİDEN dilimi (henüz DECORS'ta yok).
hw, hh, hpx = png_read_rgba(os.path.join(VIL, "buildings/Premade_houses.png"))
HOUSES = [
    ("house_1.png", (20, 20, 420, 400)),     # mor çatılı büyük ev
    ("house_2.png", (455, 15, 385, 400)),    # mavi çatılı ev
    ("house_3.png", (835, 25, 415, 385)),    # yeşil çatılı ev
    ("inn.png",     (1255, 55, 450, 400)),   # kahverengi taş büyük yapı (han)
]
for name, (bx, by, bw_, bh_) in HOUSES:
    x, y, cw, ch = bbox(hpx, hw, hh, bx, by, bx + bw_, by + bh_)
    png_write_rgba(os.path.join(BLD, name), cw, ch, crop(hpx, hw, hh, x, y, cw, ch))
    rapor.append(f"{name}: {cw}x{ch}")

print("OK:")
for r in rapor:
    print("  ", r)
