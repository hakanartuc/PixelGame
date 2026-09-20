#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ERW göçü 4. dalga — BİYOM DEKORLARI: mezarlık (undead), çöl, zindan
# geçit/tuzak, loot sandıkları, maden düğümü, K9 kandili (Faz EPIC-4).
#
# Çöl ev/tente'leri tek sheet'te (props-tents-houses.png) — BAĞLI-BİLEŞEN
# kesici: alfa adaları taranır, en büyük 6 ada ayrı PNG olur (görsel kontrol
# kompoziti scratchpad'e değil docs'a değil — rapor boyut verir, mock'la bakılır).
import os, struct, sys, zlib

PACK = sys.argv[1] if len(sys.argv) > 1 else \
    "/Users/hakan/Assets/2D Pixel-Art/Epic RPG"
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
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

def copy_trim(src, outp):
    w, h, px = png_read_rgba(src)
    x, y, bw, bh = bbox(px, w, h)
    png_write_rgba(outp, bw, bh, crop(px, w, x, y, bw, bh))
    print(f"  {os.path.basename(outp)}: {bw}x{bh}")

def strip_pack(paths, outp):
    """Farklı boyutlu kareleri alt-orta hizayla tek şeride paketler."""
    ts = [png_read_rgba(p) for p in paths]
    tt = []
    for (w, h, px) in ts:
        x, y, bw, bh = bbox(px, w, h)
        tt.append((bw, bh, crop(px, w, x, y, bw, bh)))
    cw = max(t[0] for t in tt); ch = max(t[1] for t in tt)
    strip = bytearray(cw * len(tt) * ch * 4)
    for i, (w, h, px) in enumerate(tt):
        x0 = i * cw + (cw - w) // 2; y0 = ch - h
        for y in range(h):
            for x in range(w):
                s = (y * w + x) * 4
                if px[s + 3] > 8:
                    o = ((y0 + y) * cw * len(tt) + x0 + x) * 4
                    strip[o:o + 4] = px[s:s + 4]
    png_write_rgba(outp, cw * len(tt), ch, strip)
    print(f"  {os.path.basename(outp)}: {len(tt)} kare x {cw}x{ch}")

def islands(src, outbase, keep=6, minpx=800):
    """Alfa adalarını (bağlı bileşen, 8-komşu, 2px köprü toleranslı grid) kes."""
    w, h, px = png_read_rgba(src)
    G = 4   # hücre ızgarası — bitişik sprite'ları ayırır, delikleri köprüler
    gw, gh = (w + G - 1) // G, (h + G - 1) // G
    occ = bytearray(gw * gh)
    for y in range(h):
        for x in range(w):
            if px[(y * w + x) * 4 + 3] > 8:
                occ[(y // G) * gw + (x // G)] = 1
    seen = bytearray(gw * gh)
    comps = []
    for i in range(gw * gh):
        if occ[i] and not seen[i]:
            stack = [i]; seen[i] = 1; cells = []
            while stack:
                c = stack.pop(); cells.append(c)
                cx, cy = c % gw, c // gw
                for dy in (-1, 0, 1):
                    for dx in (-1, 0, 1):
                        nx, ny = cx + dx, cy + dy
                        if 0 <= nx < gw and 0 <= ny < gh:
                            n = ny * gw + nx
                            if occ[n] and not seen[n]:
                                seen[n] = 1; stack.append(n)
            xs = [c % gw for c in cells]; ys = [c // gw for c in cells]
            comps.append((len(cells), min(xs) * G, min(ys) * G,
                          (max(xs) + 1) * G, (max(ys) + 1) * G))
    comps.sort(reverse=True)
    n = 0
    for (sz, x0, y0, x1, y1) in comps[:keep]:
        if sz * G * G < minpx: continue
        x1 = min(x1, w); y1 = min(y1, h)
        cpx = crop(px, w, x0, y0, x1 - x0, y1 - y0)
        bx, by, bw, bh = bbox(cpx, x1 - x0, y1 - y0)
        n += 1
        png_write_rgba(f"{outbase}{n}.png", bw, bh,
                       crop(cpx, x1 - x0, bx, by, bw, bh))
        print(f"  {os.path.basename(outbase)}{n}.png: {bw}x{bh}")

P = lambda *a: os.path.join(PACK, *a)
CE = P("EPIC RPG World Pack - Cemetery V 1.6/Props/props by individual sprites")
DS = P("Epic RPG World - Desert V1.6/props/props-sprites")
DS2 = P("Epic RPG World - Desert V1.6/props/props2-sprites")
CRA = P("EPIC RPG World Pack - Crypt V.1.6/Props/atlas props - individual sprites")
CRAN = P("EPIC RPG World Pack - Crypt V.1.6/Props/animated")
SW = P("EPIC RPG World - Sewers V1.5/Props")
DM = P("Epic RPG World - The dephs of the Mountain V1.5.1/Props/Static/Props-individual sprites")
G20 = P("ERW - Grass Land 2.0 v2.1/Props/Static props")
os.makedirs(DEC, exist_ok=True)
print("OK:")

# MEZARLIK (undead)
copy_trim(f"{CE}/crypt details - statue 1.png", f"{DEC}/mz_lich.png")
copy_trim(f"{G20}/sheet2-sprites/bone - big - 1.png", f"{DEC}/mz_skulls.png")
copy_trim(f"{CE}/tree - darker.png", f"{DEC}/mz_dead_tree1.png")
copy_trim(f"{CE}/tree 5.png", f"{DEC}/mz_dead_tree2.png")
copy_trim(f"{CE}/crypt - 7.png", f"{DEC}/mz_ruin.png")
copy_trim(f"{CE}/tombstone - 1.png", f"{DEC}/mz_grave1.png")
copy_trim(f"{CE}/tombstone - 4.png", f"{DEC}/mz_grave2.png")
copy_trim(f"{G20}/sheet2-sprites/bone - small - 1.png", f"{DEC}/mz_bones1.png")
copy_trim(f"{G20}/sheet2-sprites/bone - small - 5.png", f"{DEC}/mz_bones2.png")
copy_trim(f"{DM}/Crystals1-improved refraction_0.png", f"{DEC}/mz_crystal.png")
copy_trim(f"{CE}/crypt - 3.png", f"{DEC}/mz_gate.png")
copy_trim(f"{CE}/chapel.png", f"{DEC}/mz_chapel.png")

# K9 KANDİLİ
copy_trim(f"{CE}/candle holder - lit.png", f"{DEC}/lantern.png")

# ÇÖL
copy_trim(f"{DS}/cactus2_0.png", f"{DEC}/ds_cactus.png")
copy_trim(f"{DS}/bush_3.png", f"{DEC}/ds_dead_bush.png")
copy_trim(f"{DS}/dead_tree2_.png", f"{DEC}/ds_acacia.png")
copy_trim(f"{DS}/POI3.png", f"{DEC}/ds_temple.png")
copy_trim(f"{DS2}/obelisk2.png", f"{DEC}/ds_obelisk.png")
islands(P("Epic RPG World - Desert V1.6/props/props-tents-houses.png"),
        f"{DEC}/ds_house", keep=6)

# ZİNDAN GEÇİT + TUZAK + SANDIK + MADEN
copy_trim(f"{CE}/crypt - 1.png", f"{DEC}/dungeon_arch.png")
copy_trim(f"{CRAN}/Dungeon exit to Ancient Ruins-stairs.png", f"{DEC}/dungeon_stairs.png")
strip_pack([f"{CRA}/trap - spike - off - 1.png", f"{CRA}/trap - spike - on - 1.png",
            f"{CRA}/trap - spike - on - 2.png", f"{CRA}/trap - spike - on - 4.png"],
           f"{DEC}/floor_spikes.png")
# Sandıklar: rustic 8 kare 64×64 (loot sözleşmesi 8 kare ✓) + küçük kasa
w, h, px = png_read_rgba(f"{SW}/rustic chest-opening animation style1.png")
png_write_rgba(f"{DEC}/chest_anim.png", w, h, px)
print(f"  chest_anim.png: {w}x{h} (8 kare 64x64)")
copy_trim(f"{SW}/atlas props - individual sprites/wooden chests - isometric_18.png",
          f"{DEC}/crate.png")
# maden düğümü: Crystals1 ailesinden EN BÜYÜK kristal (görünürlük)
best, bsz = None, 0
import glob as _g
for f in _g.glob(f"{DM}/Crystals1-improved refraction_*.png"):
    w, h, px = png_read_rgba(f)
    try: x, y, bw, bh = bbox(px, w, h)
    except AssertionError: continue
    if bw * bh > bsz: bsz, best = bw * bh, f
copy_trim(best, f"{DEC}/ore_node.png")
