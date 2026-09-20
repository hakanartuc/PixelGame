#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ERW göçü 2. dalga — çayır serpiştirmesi, su tabanı, istasyonlar, iç dekor
# (Faz EPIC-4, 2026-09-20, "işlerin tamamını bitir").
#
# Üretilenler:
#   Tiles/erw/scatter_atlas.png  10×3 hücre (32px): çiçek/ot/mantar + taşlar —
#                                tilemap_render_scatter'ın flowers.png halefi
#   Tiles/erw/pathdec_atlas.png  3 hücre: yol kenarı taşları
#   Tiles/erw/water.png          ERW su karosu (GL16 animasyonlu setin 0. karesi)
#   decor/erw/herb_red|blue.png  toplama düğümü çiçekleri (renk analiziyle seçilir)
#   decor/erw/bed.png            cabin bed + yastık kompoziti
#   decor/erw/bench.png          vendor bench
#   decor/erw/hay.png            Village hay
#   decor/erw/fire_pot.png       Depths ateş potası 8 kare (demirci ocağı)
#   decor/erw/alch_bench.png     masa + şişeler + kitap kompoziti (simya)
#   decor/erw/obelisk.png        Desert obelisk (Kadim Sunak)
#   decor/erw/campfire_wood2.png Highlands ocak odunu 4 kare (shader alevi altına)
import os, struct, sys, zlib

PACK = sys.argv[1] if len(sys.argv) > 1 else \
    "/Users/hakan/Assets/2D Pixel-Art/Epic RPG"
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
TIL = os.path.join(ROOT, "assets/Tiles/erw")
DEC = os.path.join(ROOT, "assets/decor/erw")

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

def bbox(px, w, h):
    mnx, mny, mxx, mxy = w, h, -1, -1
    for y in range(h):
        row = (y * w) * 4
        for x in range(w):
            if px[row + x * 4 + 3] > 8:
                if x < mnx: mnx = x
                if x > mxx: mxx = x
                if y < mny: mny = y
                if y > mxy: mxy = y
    assert mxx >= 0
    return mnx, mny, mxx - mnx + 1, mxy - mny + 1

def crop(px, w, cx, cy, cw, ch):
    out = bytearray(cw * ch * 4)
    for y in range(ch):
        so = ((cy + y) * w + cx) * 4
        out[y * cw * 4:(y + 1) * cw * 4] = px[so:so + cw * 4]
    return out

def load_trim(path):
    w, h, px = png_read_rgba(path)
    x, y, bw, bh = bbox(px, w, h)
    return bw, bh, crop(px, w, x, y, bw, bh)

def blit_into(dst, dw, px, w, h, x0, y0):
    for y in range(h):
        for x in range(w):
            s = (y * w + x) * 4
            if px[s + 3] > 8:
                o = ((y0 + y) * dw + x0 + x) * 4
                dst[o:o + 4] = px[s:s + 4]

def cell_fit(dst, dw, cellx, celly, cs, px, w, h):
    """Parçayı hücreye ALT-ORTA hizala (taban duruşu); büyükse merkezden kırp."""
    if w > cs or h > cs:
        cx = max(0, (w - cs) // 2); cy = max(0, h - cs)
        px = crop(px, w, cx, cy, min(w, cs), min(h, cs))
        w, h = min(w, cs), min(h, cs)
    blit_into(dst, dw, px, w, h, cellx * cs + (cs - w) // 2,
              celly * cs + (cs - h))

P = lambda *a: os.path.join(PACK, *a)
G20 = P("ERW - Grass Land 2.0 v2.1/Props/Static props")
FLW = os.path.join(G20, "items-flowers-mushrooms-sprites")
VIL = P("Epic RPG World - The Village V2.2/tilesets and props/props-sprites")
os.makedirs(TIL, exist_ok=True); os.makedirs(DEC, exist_ok=True)
rapor = []

# 1) SCATTER ATLASI — 10×3 hücre, 32px (tilemap kodu 3 satırdan seçiyor).
# Satır 0-1: çiçek/ot/mantar karışımı · satır 2: küçük taşlar.
flowers = sorted(f for f in os.listdir(FLW)
                 if f.startswith("items-flowers") and f.endswith(".png"))
shrooms = sorted(f for f in os.listdir(FLW)
                 if "mushroom" in f.lower() and f.endswith(".png"))
rocks = sorted(f for f in os.listdir(G20 + "/sheet1-sprites")
               if f.startswith("rocks - color scheme 1 - "))
CS = 32
atlas = bytearray(10 * CS * 3 * CS * 4)
plants = flowers + shrooms[:4]
for i in range(20):
    src = os.path.join(FLW, plants[i % len(plants)])
    w, h, px = load_trim(src)
    cell_fit(atlas, 10 * CS, i % 10, i // 10, CS, px, w, h)
for i in range(10):
    src = os.path.join(G20, "sheet1-sprites", rocks[(i * 3) % len(rocks)])
    w, h, px = load_trim(src)
    cell_fit(atlas, 10 * CS, i, 2, CS, px, w, h)
png_write_rgba(os.path.join(TIL, "scatter_atlas.png"), 10 * CS, 3 * CS, atlas)
rapor.append(f"scatter_atlas: 10x3x{CS} (bitki {len(plants)}, tas {len(rocks)})")

# 2) YOL KENARI TAŞLARI — 3 hücre
pd = bytearray(3 * CS * CS * 4)
for i in range(3):
    w, h, px = load_trim(os.path.join(G20, "sheet1-sprites", rocks[i * 5]))
    cell_fit(pd, 3 * CS, i, 0, CS, px, w, h)
png_write_rgba(os.path.join(TIL, "pathdec_atlas.png"), 3 * CS, CS, pd)
rapor.append("pathdec_atlas: 3 hucre")

# 3) SU KAROSU — GL16 animasyonlu tam karo setinin 0. karesi (256×32 = 8 kare)
w, h, px = png_read_rgba(P("EPIC RPG World Pack - Grass Land V. 1.6/Tilesets",
                           "Animated water tiles (full tile).png"))
png_write_rgba(os.path.join(TIL, "water.png"), 32, 32, crop(px, w, 0, 0, 32, 32))
rapor.append("water: 32x32 (kare 0)")

# 4) OT DÜĞÜMLERİ — kırmızı/mavi baskın çiçeği renk analiziyle seç
def dominant(pxd, w, h):
    r = g = b = n = 0
    for i in range(0, w * h * 4, 4):
        if pxd[i + 3] > 128:
            r += pxd[i]; g += pxd[i + 1]; b += pxd[i + 2]; n += 1
    return (r / n, g / n, b / n) if n else (0, 0, 0)
best_r, best_b, br_s, bb_s = None, None, -1, -1
for f in flowers:
    w, h, px = load_trim(os.path.join(FLW, f))
    r, g, b = dominant(px, w, h)
    if r - max(g, b) > br_s: br_s = r - max(g, b); best_r = (f, w, h, px)
    if b - max(r, g) > bb_s: bb_s = b - max(r, g); best_b = (f, w, h, px)
for name, pick in (("herb_red.png", best_r), ("herb_blue.png", best_b)):
    f, w, h, px = pick
    png_write_rgba(os.path.join(DEC, name), w, h, px)
    rapor.append(f"{name}: {w}x{h} <- {f}")

# 5) YATAK / BANK / SAMAN
w1, h1, p1 = load_trim(os.path.join(G20, "sheet2-sprites/cabin interior - bed.png"))
w2, h2, p2 = load_trim(os.path.join(G20, "sheet2-sprites/cabin interior - bed - pillow.png"))
bed = bytearray(w1 * h1 * 4)
blit_into(bed, w1, p1, w1, h1, 0, 0)
blit_into(bed, w1, p2, w2, h2, (w1 - w2) // 2, 2)
png_write_rgba(os.path.join(DEC, "bed.png"), w1, h1, bed)
rapor.append(f"bed: {w1}x{h1}")
for src, out in (("vendor - bench.png", "bench.png"),):
    w, h, px = load_trim(os.path.join(G20, "sheet2-sprites", src)) \
        if os.path.exists(os.path.join(G20, "sheet2-sprites", src)) \
        else load_trim(os.path.join(G20, "sheet1-sprites", src))
    png_write_rgba(os.path.join(DEC, out), w, h, px)
    rapor.append(f"{out}: {w}x{h}")
w, h, px = load_trim(os.path.join(VIL, "hay_0.png"))
png_write_rgba(os.path.join(DEC, "hay.png"), w, h, px)
rapor.append(f"hay: {w}x{h}")

# 6) DEMİRCİ OCAĞI — Depths ateş potası, 8 kare şerit (union-trim)
DP = P("Epic RPG World - The dephs of the Mountain V1.5.1/Props/Animated props/individual files")
frames = [png_read_rgba(os.path.join(DP, f"Fire-pot-frame{i}.png")) for i in range(1, 9)]
fw, fh = frames[0][0], frames[0][1]
ux0, uy0, ux1, uy1 = fw, fh, 0, 0
for (w, h, px) in frames:
    x, y, bw, bh = bbox(px, w, h)
    ux0 = min(ux0, x); uy0 = min(uy0, y)
    ux1 = max(ux1, x + bw); uy1 = max(uy1, y + bh)
cw, ch = ux1 - ux0, uy1 - uy0
strip = bytearray(cw * 8 * ch * 4)
for i, (w, h, px) in enumerate(frames):
    blit_into(strip, cw * 8, crop(px, w, ux0, uy0, cw, ch), cw, ch, i * cw, 0)
png_write_rgba(os.path.join(DEC, "fire_pot.png"), cw * 8, ch, strip)
rapor.append(f"fire_pot: 8 kare x {cw}x{ch}")

# 7) SİMYA TEZGAHI — masa + şişeler + kitap kompoziti (ERW'de kazan yok;
# görsel tezgah, istasyon adı 'Simya Kazanı' kaldı — metin borcu rapora)
SW = P("EPIC RPG World - Sewers V1.5/Props/atlas props - individual sprites")
tw, th, tpx = load_trim(os.path.join(VIL, "table--1.png"))
bench = bytearray(tw * (th + 14) * 4)
blit_into(bench, tw, tpx, tw, th, 0, 14)
# şişeler MASA YÜZEYİNE oturur (taban y=20); sığmayan/uzun olan atlanır
xoff = 10
for f in ("laboratory - potions_0.png", "laboratory - potions_2.png",
          "laboratory - book1.png", "laboratory - potions_5.png"):
    w, h, px = load_trim(os.path.join(SW, f))
    y0 = 20 - h
    if xoff + w > tw - 8 or y0 < 0: continue
    blit_into(bench, tw, px, w, h, xoff, y0)
    xoff += w + 8
png_write_rgba(os.path.join(DEC, "alch_bench.png"), tw, th + 14, bench)
rapor.append(f"alch_bench: {tw}x{th + 14}")

# 8) KADİM SUNAK — Desert obelisk
w, h, px = load_trim(P("Epic RPG World - Desert V1.6/props/props2-sprites/obelisk1.png"))
png_write_rgba(os.path.join(DEC, "obelisk.png"), w, h, px)
rapor.append(f"obelisk: {w}x{h}")

# 9) KAMP ATEŞİ ODUNU — Highlands ocak, 4 kare şerit (shader alevi üstüne)
# GÖRSEL İTERASYON BULGUSU: cabin-campfire-1..4 animasyon DEĞİL, 4 ayrı
# ocak tasarımı. Taş çemberli odunlu olan (#2) TEK KARE kullanılır; alev
# gömülü değil — shader alevi üstüne oturur (kamp ateşi dersi korunur).
HP = P("Epic RPG World - Highlands V1.4.1/tilesets and props/props-sprites")
w, h, px = load_trim(os.path.join(HP, "cabin-campfire-2.png"))
png_write_rgba(os.path.join(DEC, "campfire_wood2.png"), w, h, px)
rapor.append(f"campfire_wood2: TEK kare {w}x{h}")

print("OK:")
for r in rapor:
    print("  ", r)
