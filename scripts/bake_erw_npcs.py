#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# EPIC RPG World NPC idle şeritleri — Faz EPIC-1 dilim 3 (NPC göçü), 2026-09-20.
#
# 18 NPC'nin TAMAMI ERW'ye geçer (Hakan: "Cute komple çıkacak"). Kaynaklar:
#   · Village meslek NPC'leri (vendor/blacksmith/wizard/skill master/
#     fisherman/fish vendor) — hazır idle şeritleri (kare 160×160)
#   · Jenerik köylüler — KOMPOZİT: base-idle + cloth-idle alfa bindirme
#     (aynı tuval, kare kare) → tek şerit. Cinsiyet × ten × kıyafet = kadro
#   · GL 1.6 Warrior — muhafız (kare 144×96)
#
# Şeritler ORTAK opak kutuya kırpılır (tüm karelerin union bbox'ı) — böylece
# alt kenar = ayak hizası (npc.c dst.y ayağa hizalar) ve boş marj taşınmaz.
# Çıktı: assets/characters/erw/npc_<ad>_idle.png · rapor: kare sayısı + boyut
# (NPCS[] frameW/frameH/idleFrames alanlarına elle işlenir — script yazmaz,
# tablo insan elinde kalır).
import os, struct, sys, zlib

PACK = sys.argv[1] if len(sys.argv) > 1 else \
    "/Users/hakan/Assets/2D Pixel-Art/Epic RPG"
VN = os.path.join(PACK, "Epic RPG World - The Village V2.2/NPCs")
GL16C = os.path.join(PACK, "EPIC RPG World Pack - Grass Land V. 1.6/Characters")
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
OUT = os.path.join(ROOT, "assets/characters/erw")

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

def over(dst, src):
    """src'yi dst üstüne alfa bindir (kıyafet katmanı — kenar alfası düzgün)."""
    for i in range(0, len(dst), 4):
        sa = src[i + 3]
        if sa == 0: continue
        if sa == 255:
            dst[i:i + 4] = src[i:i + 4]
        else:
            da = dst[i + 3]
            oa = sa + da * (255 - sa) // 255
            for c in range(3):
                dst[i + c] = (src[i + c] * sa + dst[i + c] * da * (255 - sa) // 255) // max(oa, 1)
            dst[i + 3] = oa

def strip_bake(layers, frameW, frameH, outp):
    """layers: aynı boyutta PNG yolları (ilk taban, gerisi alfa bindirme).
    Kaynak yatay şerit YA DA ızgara olabilir (skill master 4×4 640×640);
    kareler soldan sağa, yukarıdan aşağı sayılır. Union-trim + yatay paket."""
    w, h, base = png_read_rgba(layers[0])
    for extra in layers[1:]:
        w2, h2, px2 = png_read_rgba(extra)
        assert (w2, h2) == (w, h), f"katman boyutu farkli: {extra}"
        over(base, px2)
    cols, rows = w // frameW, h // frameH
    n = cols * rows
    # kare başına bbox → union (kare-yerel koordinatta); TAMAMEN boş kareler
    # (ızgara dolgusu) sayılmaz
    ux0, uy0, ux1, uy1 = frameW, frameH, 0, 0
    used = []
    for f in range(n):
        fx, fy = (f % cols) * frameW, (f // cols) * frameH
        empty = True
        for y in range(frameH):
            row = ((fy + y) * w + fx) * 4
            for x in range(frameW):
                if base[row + x * 4 + 3] > 8:
                    empty = False
                    if x < ux0: ux0 = x
                    if x > ux1 - 1: ux1 = x + 1
                    if y < uy0: uy0 = y
                    if y > uy1 - 1: uy1 = y + 1
        if not empty: used.append(f)
    n = len(used)
    cw, ch = ux1 - ux0, uy1 - uy0
    strip = bytearray(cw * n * ch * 4)
    for i, f in enumerate(used):
        fx, fy = (f % cols) * frameW, (f // cols) * frameH
        for y in range(ch):
            so = ((fy + uy0 + y) * w + fx + ux0) * 4
            do = (y * cw * n + i * cw) * 4
            strip[do:do + cw * 4] = base[so:so + cw * 4]
    png_write_rgba(outp, cw * n, ch, strip)
    print(f"  {os.path.basename(outp)}: {n} kare x {cw}x{ch}")

os.makedirs(OUT, exist_ok=True)
V = lambda *p: os.path.join(VN, *p)
vil = lambda cins, skin, parts: [V("villager", cins, skin, f"villager-{p}-idle.png")
                                 for p in parts]

JOBS = [
    # (çıktı adı, kare genişliği, katman listesi)
    ("npc_vendor",      160, 160, [V("Vendor", "vendor-no_carpet-idle.png")]),
    ("npc_vendor_hali", 160, 160, [V("Vendor", "vendor-idle.png")]),          # kilimli (Taşdurak Ferah)
    ("npc_wizard",      160, 160, [V("old wise wizard", "old_wizard-idle.png")]),
    ("npc_blacksmith",  160, 160, [V("blacksmith", "blacksmith-anim-idle.png")]),
    ("npc_skillmaster", 160, 160, [V("skill master", "skill_master-idle.png")]),
    ("npc_fisherman",   192, 192, [V("fisherman", "beard", "fisherman-beard-idle.png")]),
    ("npc_warrior",     144, 96, [os.path.join(GL16C, "Warrior", "warrior-idle.png")]),
    # Köylü kompozitleri: cins × ten × kıyafet
    ("npc_koylu_e11", 160, 160, vil("male",   "skin1", ["base", "cloth1"])),
    ("npc_koylu_e12", 160, 160, vil("male",   "skin1", ["base", "cloth2"])),
    ("npc_koylu_e13", 160, 160, vil("male",   "skin1", ["base", "cloth3"])),
    ("npc_koylu_e21", 160, 160, vil("male",   "skin2", ["base", "cloth1"])),
    ("npc_koylu_e22", 160, 160, vil("male",   "skin2", ["base", "cloth2"])),
    ("npc_koylu_e23", 160, 160, vil("male",   "skin2", ["base", "cloth3"])),
    ("npc_koylu_k11", 160, 160, vil("female", "skin1", ["base", "cloth1-2"])),
    ("npc_koylu_k12", 160, 160, vil("female", "skin1", ["base", "cloth2"])),
    ("npc_koylu_k13", 160, 160, vil("female", "skin1", ["base", "cloth3"])),
    ("npc_koylu_k21", 160, 160, vil("female", "skin2", ["base", "cloth1"])),
    ("npc_koylu_k22", 160, 160, vil("female", "skin2", ["base", "cloth2"])),
]
print("OK:")
for name, fw, fh, layers in JOBS:
    strip_bake(layers, fw, fh, os.path.join(OUT, name + "_idle.png"))
