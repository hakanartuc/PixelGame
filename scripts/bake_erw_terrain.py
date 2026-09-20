#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# EPIC RPG World (Rafael Matos) arazi karolarını oyunun beklediği dosyalara
# keser — Faz EPIC-1 (çayır + köy), 2026-09-20.
#
# KAYNAK VERİ = paketin Tiled .tsx dosyasındaki WANGSET'ler (köşe tipli).
# Elle koordinat YOK: hangi karonun hangi köşe kombinasyonu olduğunu paketin
# kendi autotile verisi söylüyor. Oyunun dual-grid modeli (tilemap.c
# dsr_dual_pass ailesi) köşe maskesiyle çalışır: bit 1=NW 2=NE 4=SW 8=SE.
# Tiled wangid dizilimi saat yönünde [N,NE,E,SE,S,SW,W,NW] → köşeler
# indeks 1(NE), 3(SE), 5(SW), 7(NW).
#
# ÇIKTILAR (assets/Tiles/erw/ — 32px karo, oyun 2× ölçekle 64'e çizer):
#   grass_1..4.png       çim iç varyantları (grassVar + TILES[0])
#   path.png             yol/toprak iç karosu (TILES[1])
#   path_edge_atlas.png  128×128, 4×4 hücre; HÜCRE = KÖŞE MASKESİ (m%4, m/4)
#                        → tilemap.c'de LUT gerekmez, src doğrudan maskeden.
#
# Paket yolu argümanla değişebilir: python3 scripts/bake_erw_terrain.py [pak_kökü]
import os, re, struct, sys, zlib
import xml.etree.ElementTree as ET

PACK = sys.argv[1] if len(sys.argv) > 1 else \
    "/Users/hakan/Assets/2D Pixel-Art/Epic RPG"
GL = os.path.join(PACK, "ERW - Grass Land 2.0 v2.1")
TSX = os.path.join(GL, "TiledMap Editor/Tilesets",
                   "Tileset-Terrain-new grass - transparency.tsx")
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
OUT = os.path.join(ROOT, "assets/Tiles/erw")
TILE = 32

# --- Saf-python PNG codec (bake_campfire_wood.py ile aynı gerekçe: PIL şart
# değil; 8-bit RGBA, tüm filtreler) --------------------------------------
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
            assert bit == 8 and ct == 6, f"beklenen 8-bit RGBA: bit={bit} ct={ct}"
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

def blit_tile(dstpix, dstw, dx, dy, srcpix, srcw, tileid, cols):
    sx = (tileid % cols) * TILE
    sy = (tileid // cols) * TILE
    for yy in range(TILE):
        so = ((sy + yy) * srcw + sx) * 4
        do = ((dy + yy) * dstw + dx) * 4
        dstpix[do:do + TILE * 4] = srcpix[so:so + TILE * 4]

# --- tsx parse -----------------------------------------------------------
tree = ET.parse(TSX)
ts = tree.getroot()
cols = int(ts.attrib["columns"])
img = ts.find("image").attrib["source"]
sheet = os.path.normpath(os.path.join(os.path.dirname(TSX), img))
w, h, pix = png_read_rgba(sheet)
assert w // TILE == cols, "kolon sayisi uyusmuyor"

def wang_tiles(setname, colorname):
    """(mask -> [tileid]) sözlüğü: mask biti = o köşe HEDEF renk."""
    for ws in ts.iter("wangset"):
        if ws.attrib.get("name") != setname:
            continue
        colors = [c.attrib["name"] for c in ws.iter("wangcolor")]
        cid = colors.index(colorname) + 1     # wangid renkleri 1 tabanlı
        bymask = {}
        for wt in ws.iter("wangtile"):
            ids = [int(v) for v in wt.attrib["wangid"].split(",")]
            ne, se, sw, nw = ids[1], ids[3], ids[5], ids[7]
            if cid not in (ne, se, sw, nw):
                continue
            m = ((1 if nw == cid else 0) | (2 if ne == cid else 0)
               | (4 if sw == cid else 0) | (8 if se == cid else 0))
            bymask.setdefault(m, []).append(int(wt.attrib["tileid"]))
        return colors, bymask
    raise SystemExit("wangset yok: " + setname)

os.makedirs(OUT, exist_ok=True)

# 1) ÇİM İÇLERİ — "grass" setinin ana çim rengi: dört köşesi de çim olan
# karolar (tam çim iç varyantları; "grass to grass" wall2 setine ait, o duvar)
_, grass = wang_tiles("grass", "grass to any terrain but grass")
interior = sorted(grass.get(15, []))
assert len(interior) >= 4, f"cim ic varyanti az: {len(interior)}"
picks = [interior[i * (len(interior) - 1) // 3] for i in range(4)]  # yayılmış 4
for i, tid in enumerate(picks):
    cell = bytearray(TILE * TILE * 4)
    blit_tile(cell, TILE, 0, 0, pix, w, tid, cols)
    png_write_rgba(os.path.join(OUT, f"grass_{i+1}.png"), TILE, TILE, cell)

# 2) YOL/TOPRAK — "dirt1 to main grass": iç (mask 15) + kenar atlası
colors, dirt = wang_tiles("dirt", "dirt1 to main grass")
pin = sorted(dirt.get(15, []))
assert pin, "toprak ic karosu (mask 15) yok"
cell = bytearray(TILE * TILE * 4)
blit_tile(cell, TILE, 0, 0, pix, w, pin[0], cols)
png_write_rgba(os.path.join(OUT, "path.png"), TILE, TILE, cell)

def write_mask_atlas(name, bymask):
    atlas = bytearray(4 * TILE * 4 * TILE * 4)
    missing = []
    for m in range(16):
        tids = bymask.get(m)
        if not tids:
            if m != 0:
                missing.append(m)
            continue
        blit_tile(atlas, 4 * TILE, (m % 4) * TILE, (m // 4) * TILE,
                  pix, w, sorted(tids)[0], cols)
    png_write_rgba(os.path.join(OUT, name), 4 * TILE, 4 * TILE, atlas)
    return missing

miss_d = write_mask_atlas("path_edge_atlas.png", dirt)

# 3) ÇİM SAÇAĞI — "grass to any terrain but grass": çim tarafı opak + saçak
# dışa taşar, öbür taraf ŞEFFAF. Su/bataklık/kum/diken sınırında dual pass
# bunu basar (eski 12 hücreli Cute Fantasy grass_edges.png'nin halefi).
# Maske biti = o köşe ÇİM.
miss_g = write_mask_atlas("grass_edge_atlas.png", grass)

print("OK  cim:", picks, " yol ic:", pin[0])
print("    dirt maskeleri:", {m: len(v) for m, v in sorted(dirt.items())})
print("    grass maskeleri:", {m: len(v) for m, v in sorted(grass.items())})
for nm, mi in (("path", miss_d), ("grass", miss_g)):
    if mi:
        print(f"    EKSIK {nm} maskeleri (hucre bos, pass atlar):", mi)
