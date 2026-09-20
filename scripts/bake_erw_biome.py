#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ERW göçü 3. dalga — BİYOM KAROLARI: zindan (Crypt/Depths), mezarlık
# (Cemetery), çöl (Desert kum+vaha), bataklık/diken/taş (Faz EPIC-4).
#
# GENEL ARAÇ: her iş bir tsx wangset'inden gelir —
#   interior  : dört köşesi de hedef renk (m15) olan İLK karo → tek karo PNG
#   atlas     : maske-sıralı 4×4 (hücre = köşe maskesi, m%4/m/4) → dual pass
#               LUT'suz okur (bake_erw_terrain.py deseniyle aynı sözleşme)
# Wangid köşeleri Tiled sırasında [_,NE,_,SE,_,SW,_,NW]; maske bitleri oyunla
# aynı: 1=NW 2=NE 4=SW 8=SE.
#
# BİLİNÇLİ ERTELEME: çöl kaya/klif (rock.png + cliff_atlas.png Cute kalır) —
# ERW klifleri 2-3 karo boylu ayrı mimari, yüz sistemi yeniden tasarım ister.
import os, struct, sys, zlib
import xml.etree.ElementTree as ET

PACK = sys.argv[1] if len(sys.argv) > 1 else \
    "/Users/hakan/Assets/2D Pixel-Art/Epic RPG"
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
TIL = os.path.join(ROOT, "assets/Tiles/erw")
TILE = 32

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

_sheets = {}
def sheet(path):
    if path not in _sheets:
        _sheets[path] = png_read_rgba(path)
    return _sheets[path]

def blit_tile(dst, dw, dx, dy, spx, sw, tileid, cols):
    sx = (tileid % cols) * TILE
    sy = (tileid // cols) * TILE
    for y in range(TILE):
        so = ((sy + y) * sw + sx) * 4
        do = ((dy + y) * dw + dx) * 4
        dst[do:do + TILE * 4] = spx[so:so + TILE * 4]

def wang(tsx, setname):
    t = ET.parse(tsx).getroot()
    cols = int(t.attrib["columns"])
    img = os.path.normpath(os.path.join(os.path.dirname(tsx),
                                        t.find("image").attrib["source"]))
    for ws in t.iter("wangset"):
        if ws.attrib.get("name") != setname:
            continue
        bymask = {}
        for wt in ws.iter("wangtile"):
            ids = [int(v) for v in wt.attrib["wangid"].split(",")]
            ne, se, sw, nw = ids[1], ids[3], ids[5], ids[7]
            if 1 not in (ne, se, sw, nw):
                continue
            m = ((1 if nw == 1 else 0) | (2 if ne == 1 else 0)
               | (4 if sw == 1 else 0) | (8 if se == 1 else 0))
            bymask.setdefault(m, []).append(int(wt.attrib["tileid"]))
        return img, cols, bymask
    raise SystemExit(f"wangset yok: {setname} ({tsx})")

def interior(tsx, setname, out, tint=None, pick=0):
    img, cols, bm = wang(tsx, setname)
    tids = sorted(bm.get(15, []))
    assert tids, f"m15 yok: {setname}"
    w, h, px = sheet(img)
    cell = bytearray(TILE * TILE * 4)
    blit_tile(cell, TILE, 0, 0, px, w, tids[min(pick, len(tids) - 1)], cols)
    if tint:
        for i in range(0, len(cell), 4):
            cell[i] = int(cell[i] * tint[0])
            cell[i + 1] = int(cell[i + 1] * tint[1])
            cell[i + 2] = int(cell[i + 2] * tint[2])
    png_write_rgba(os.path.join(TIL, out), TILE, TILE, cell)
    print(f"  {out} <- {setname} m15[{pick}] ({len(tids)} aday)")

def mask_atlas(tsx, setname, out):
    img, cols, bm = wang(tsx, setname)
    w, h, px = sheet(img)
    atlas = bytearray(4 * TILE * 4 * TILE * 4)
    missing = [m for m in range(1, 16) if m not in bm]
    for m, tids in bm.items():
        blit_tile(atlas, 4 * TILE, (m % 4) * TILE, (m // 4) * TILE,
                  px, w, sorted(tids)[0], cols)
    png_write_rgba(os.path.join(TIL, out), 4 * TILE, 4 * TILE, atlas)
    print(f"  {out} <- {setname} (eksik maskeler: {missing or 'yok'})")

P = lambda *a: os.path.join(PACK, *a)
CR = P("EPIC RPG World Pack - Crypt V.1.6/TiledMap Editor/Tilesets/Tileset-Terrain.tsx")
PR = P("EPIC RPG World Pack - Old Prison V1.7.1/TiledMap Editor New/Tilesets/Tileset-Terrain-old prison.tsx")
CE = P("EPIC RPG World Pack - Cemetery V 1.6/TiledMap Editor/Tilesets/Terrain - cemetery.tsx")
DM = P("Epic RPG World - The dephs of the Mountain V1.5.1/TiledMap Editor/Tilesets/Tileset-Terrain2.tsx")
DMP = P("Epic RPG World - The dephs of the Mountain V1.5.1/TiledMap Editor/Tilesets/The depths-Tileset-Platform2.tsx")
DS = P("Epic RPG World - Desert V1.6/tiledmap editor/Tilesets/Terrain-Desert.tsx")
DSW = P("Epic RPG World - Desert V1.6/tiledmap editor/Tilesets/water-to-sand.tsx")
os.makedirs(TIL, exist_ok=True)
print("OK:")

# ZİNDAN K1 (Crypt) — zemin + duvar (yamaç bloğu duvar dokusu olarak)
interior(CR, "ground1 to ground2", "d1_floor.png")
interior(CR, "slope-ground1", "d1_wall.png")
# ZİNDAN K2 (Depths) — mağara zemini + platform duvarı
interior(DM, "ground-cave-tilesets", "d2_floor.png")
interior(DMP, "plat-2", "d2_wall.png")
# MEZARLIK (Cemetery)
interior(CE, "dirt2 to rocky ground", "mz_ground.png")
interior(CE, "leaves to dirt2", "mz_ground2.png")
interior(CE, "rocky ground to any terrain(transparency)", "mz_rock.png")
mask_atlas(CE, "rocky ground to any terrain(transparency)", "mz_rock_atlas.png")
# LOCA MAĞARA (Crypt ground2 zemin + Crypt duvar ringi)
interior(CR, "ground2 to ground1", "cave_floor.png")
interior(CR, "slope-ground2", "cave_wall.png")
mask_atlas(CR, "ground2 to transparency", "cave_ring_atlas.png")
# ÇÖL — kum + vaha
interior(DS, "sand-to-transparency", "sand.png")
mask_atlas(DSW, "water-to-sand", "oasis_atlas.png")
interior(DSW, "water-to-sand", "oasis_water.png")
# TAŞ / BATAKLIK / DİKEN
interior(CE, "stone ground to any terrain (transparency)", "stone.png")
interior(CE, "dirt1 to rocky ground", "swamp.png")
interior(PR, "spikes", "thorns.png")
# MEZARLIK ÖLÜ SUYU — ERW su karosu koyulaştırılır
w, h, px = png_read_rgba(os.path.join(TIL, "water.png"))
dark = bytearray(px)
for i in range(0, len(dark), 4):
    dark[i] = int(dark[i] * 0.30)
    dark[i + 1] = int(dark[i + 1] * 0.34)
    dark[i + 2] = int(dark[i + 2] * 0.42)
png_write_rgba(os.path.join(TIL, "mz_water.png"), w, h, dark)
print("  mz_water.png <- water.png (karartilmis)")
