#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# EPIC RPG World düşman/hayvan sheet'leri — Faz EPIC-2 (mob göçü), 2026-09-20.
#
# Her mob için TEK sheet üretir: satır = durum (idle / run / attack / death),
# kareler soldan sağa. mob.c AnimClip'leri bu satırlara işaret eder (üç facing
# AYNI klibe bakar — Hakan kararı: moblar 2 yönlü, flip zaten kodda).
#
# Kaynak biçimleri karışık (tek satır şerit, çok satırlı grid, "all
# animations" sheet'inin tek satırı) — spec her durumu (dosya, frameW, frameH,
# [row]) olarak verir. Kurallar:
#   · Kare sayısı 12'yi aşan durum EŞİT ARALIKLI 12 kareye indirilir (ilk+son
#     korunur): GPU doku genişliği (moose atk 30×347=10410 > 8192 tavanı) ve
#     VRAM için. Rapor orijinal süreyi korumak için önerilen frameDuration
#     çarpanını yazar.
#   · Tüm durumların TÜM kareleri ortak (union) opak kutuya kırpılır — ayak
#     hizası satırlar arasında oynamaz.
#   · feetY = IDLE satırının en alt opak pikseli (ölüm yatık yatar, o yüzden
#     union altı değil idle altı).
#   · Boş grid kareleri atılır (dolgu hücreleri).
# Çıktı: assets/characters/erw/mob_<ad>.png + tablo raporu (anims değerleri).
import os, struct, sys, zlib

PACK = sys.argv[1] if len(sys.argv) > 1 else \
    "/Users/hakan/Assets/2D Pixel-Art/Epic RPG"
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
OUT = os.path.join(ROOT, "assets/characters/erw")
MAXF = 12   # durum başına kare tavanı

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

def state_frames(path, fw, fh, row=None):
    """Dosyadan kare listesi: [(pix, w)] yerine (kaynakpx, w, [(fx,fy)])."""
    w, h, px = png_read_rgba(path)
    cols, rows = w // fw, h // fh
    cells = []
    rr = range(rows) if row is None else [row]
    for ry in rr:
        for cx in range(cols):
            fx, fy = cx * fw, ry * fh
            # boş kare mi?
            empty = True
            for y in range(0, fh, 2):          # 2'şer atlayarak tara (hız)
                ro = ((fy + y) * w + fx) * 4
                for x in range(0, fw, 2):
                    if px[ro + x * 4 + 3] > 8:
                        empty = False; break
                if not empty: break
            if not empty:
                cells.append((fx, fy))
    return px, w, cells

def downsample(cells):
    if len(cells) <= MAXF:
        return cells, 1.0
    idx = [round(i * (len(cells) - 1) / (MAXF - 1)) for i in range(MAXF)]
    return [cells[i] for i in idx], len(cells) / MAXF

def bake(name, states):
    """states: [(etiket, dosya, fw, fh, row|None)] — sıra: idle, run, atk, death.
    Etiket '=idle' ise o satır idle satırının kopyası olur (klip aynı satıra
    işaret eder — sheet'e ayrıca yazılmaz, raporda belirtilir)."""
    loaded = []   # (etiket, kaynakpx, kaynakw, kareler, süreçarpanı, fw, fh)
    for (tag, path, fw, fh, row) in states:
        if path is None:
            loaded.append((tag, None, 0, [], 1.0, 0, 0)); continue
        px, w, cells = state_frames(path, fw, fh, row)
        cells, mul = downsample(cells)
        loaded.append((tag, px, w, cells, mul, fw, fh))
    # ortak union bbox (kare-yerel)
    fw0 = max(l[5] for l in loaded if l[1] is not None)
    fh0 = max(l[6] for l in loaded if l[1] is not None)
    ux0, uy0, ux1, uy1 = fw0, fh0, 0, 0
    idle_bottom = 0
    for (tag, px, w, cells, mul, fw, fh) in loaded:
        if px is None: continue
        for (fx, fy) in cells:
            for y in range(fh):
                ro = ((fy + y) * w + fx) * 4
                for x in range(fw):
                    if px[ro + x * 4 + 3] > 8:
                        if x < ux0: ux0 = x
                        if x + 1 > ux1: ux1 = x + 1
                        if y < uy0: uy0 = y
                        if y + 1 > uy1: uy1 = y + 1
                        if tag == "idle" and y + 1 > idle_bottom:
                            idle_bottom = y + 1
    cw, ch = ux1 - ux0, uy1 - uy0
    maxn = max(len(l[3]) for l in loaded)
    realrows = [l for l in loaded if l[1] is not None]
    sheet = bytearray(cw * maxn * ch * len(realrows) * 4)
    sw = cw * maxn
    rep = [f"{name}: kare {cw}x{ch}, feetY={idle_bottom - uy0}"]
    ry = 0
    for (tag, px, w, cells, mul, fw, fh) in loaded:
        if px is None:
            rep.append(f"  {tag}: (idle satiri kullanilir)")
            continue
        for i, (fx, fy) in enumerate(cells):
            for y in range(ch):
                so = ((fy + uy0 + y) * w + fx + ux0) * 4
                do = ((ry * ch + y) * sw + i * cw) * 4
                sheet[do:do + cw * 4] = px[so:so + cw * 4]
        rep.append(f"  {tag}: y={ry * ch} kare={len(cells)} sureCarpani={mul:.2f}")
        ry += 1
    png_write_rgba(os.path.join(OUT, f"mob_{name}.png"), sw, ch * len(realrows), sheet)
    print("\n".join(rep))

P = lambda *a: os.path.join(PACK, *a)
GL20 = P("ERW - Grass Land 2.0 v2.1/Characters")
GL16 = P("EPIC RPG World Pack - Grass Land V. 1.6/Characters")
CR = P("EPIC RPG World Pack - Crypt V.1.6/Characters")
PR = P("EPIC RPG World Pack - Old Prison V1.7.1/Characters")
DS = P("Epic RPG World - Desert V1.6/characters")
VO = P("Epic RPG World - Volcano V1.6/Characters")
AR = P("ERW - Ancient Ruins V 2.3/Characters")
SA = P("ERW - Grass Land 2.0 v2.1/Characters/small animals")
VN = P("Epic RPG World - The Village V2.2/NPCs")

E1 = os.path.join(GL16, "Enemy1")
OR = os.path.join(GL20, "orc warrior/orc1")
MO = os.path.join(AR, "Moose")

os.makedirs(OUT, exist_ok=True)
J = [
 ("goblin", [("idle", f"{E1}/enemy1-idle.png", 192,128,None),
             ("run",  f"{E1}/enemy1-run.png", 192,128,None),
             ("atk",  f"{E1}/enemy1-atk1.png", 192,128,None),
             ("death",f"{E1}/enemy1-death.png", 192,128,None)]),
 # Reis: variation1 tek "all animations" sheet — satırlar adda:
 # Idle-run-atk1-atk2-hurt-death (0-5)
 ("goblin_reis", [("idle", f"{E1}/enemy1-variation1-all-animations-from top to bottom(Idle-run-atk1-atk2-hurt-death)-canvas 192x128.png", 192,128,0),
                  ("run",  f"{E1}/enemy1-variation1-all-animations-from top to bottom(Idle-run-atk1-atk2-hurt-death)-canvas 192x128.png", 192,128,1),
                  ("atk",  f"{E1}/enemy1-variation1-all-animations-from top to bottom(Idle-run-atk1-atk2-hurt-death)-canvas 192x128.png", 192,128,2),
                  ("death",f"{E1}/enemy1-variation1-all-animations-from top to bottom(Idle-run-atk1-atk2-hurt-death)-canvas 192x128.png", 192,128,5)]),
 ("ork", [("idle", f"{OR}/orc melee - anims-idle.png", 256,256,None),
          ("run",  f"{OR}/orc melee - anims-walk.png", 256,256,None),
          ("atk",  f"{OR}/orc melee - anims-atk1 with fx.png", 256,256,None),
          ("death",f"{OR}/orc melee - anims-death.png", 256,256,None)]),
 ("iskelet_savasci", [("idle", f"{CR}/Skeleton/skeleton-variation1-idle.png",128,128,None),
                      ("run",  f"{CR}/Skeleton/skeleton-variation1-walk.png",128,128,None),
                      ("atk",  f"{CR}/Skeleton/skeleton-variation1-attack.png",128,128,None),
                      ("death",f"{CR}/Skeleton/skeleton-variation1-death.png",128,128,None)]),
 ("iskelet_lord", [("idle", f"{CR}/Skeleton/skeleton-variation2-idle.png",128,128,None),
                   ("run",  f"{CR}/Skeleton/skeleton-variation2-walk.png",128,128,None),
                   ("atk",  f"{CR}/Skeleton/skeleton-variation2-attack.png",128,128,None),
                   ("death",f"{CR}/Skeleton/skeleton-variation2-death.png",128,128,None)]),
 ("iskelet_okcu", [("idle", f"{PR}/Skeleton 1/no shield/Skeleton 1 - animations-idle.png",220,220,None),
                   ("run",  f"{PR}/Skeleton 1/no shield/Skeleton 1 - animations-walk.png",220,220,None),
                   ("atk",  f"{PR}/Skeleton 1/no shield/Skeleton 1 - animations-atk 1.png",220,220,None),
                   ("death",f"{PR}/Skeleton 1/no shield/Skeleton 1 - animations-death.png",220,220,None)]),
 ("iskelet_buyucu", [("idle", f"{PR}/Mage Skeleton/Mage Skeleton no shield/Mage Skeleton - animations-idle.png",220,220,None),
                     ("run",  f"{PR}/Mage Skeleton/Mage Skeleton no shield/Mage Skeleton - animations-walk.png",220,220,None),
                     ("atk",  f"{PR}/Mage Skeleton/Mage Skeleton no shield/Mage Skeleton - animations-atk2.png",220,220,None),
                     ("death",f"{PR}/Mage Skeleton/Mage Skeleton no shield/Mage Skeleton - animations-death.png",220,220,None)]),
 # elemental kare 256x224 (idle 2048/8; atk 5x4 grid, death 6x6 grid)
 ("hortlak", [("idle", f"{VO}/elemental/elemental-idle_8frames.png",256,224,None),
              ("run",  f"{VO}/elemental/elemental-walk_8frames.png",256,224,None),
              ("atk",  f"{VO}/elemental/elemental-attack1_no-vfx_20frames.png",256,224,None),
              ("death",f"{VO}/elemental/elemental-death_35frames.png",256,224,None)]),
 ("kan_canavari", [("idle", f"{CR}/Big Worm 1/Big worm - 1-idle-8 frames.png",128,128,None),
                   ("run",  None,0,0,None),
                   ("atk",  f"{CR}/Big Worm 1/Big worm - 1-appear-atk-29 frames.png",128,128,None),
                   ("death",f"{CR}/Big Worm 1/Big worm - 1-death-12 frames.png",128,128,None)]),
 ("olum_getiren", [("idle", f"{CR}/Big Worm 2/Big worm - 2-idle-8 frames.png",128,128,None),
                   ("run",  None,0,0,None),
                   ("atk",  f"{CR}/Big Worm 2/Big worm - 2-appear-atk-29 frames.png",128,128,None),
                   ("death",f"{CR}/Big Worm 2/Big worm - 2-death-12 frames.png",128,128,None)]),
 ("asker", [("idle", f"{GL16}/Warrior/warrior-idle.png",144,96,None),
            ("run",  f"{GL16}/Warrior/warrior-run.png",144,96,None),
            ("atk",  f"{GL16}/Warrior/warrior-single swing 1.png",144,96,None),
            ("death",f"{GL16}/Warrior/warrior-death.png",144,96,None)]),
 ("mumya", [("idle", f"{DS}/mummy/mummy-idle-walk.png",224,192,None),
            ("run",  None,0,0,None),
            ("atk",  f"{DS}/mummy/mummy-attack1.png",224,192,None),
            ("death",f"{DS}/mummy/mummy-death.png",224,192,None)]),
 ("haydut", [("idle", f"{DS}/anubis/anubis-idle.png",352,256,None),
             ("run",  f"{DS}/anubis/anubis-run.png",352,256,None),
             ("atk",  f"{DS}/anubis/anubis-attack1.png",352,256,None),
             ("death",f"{DS}/anubis/anubis-death.png",352,256,None)]),
 ("geyik", [("idle", f"{MO}/moose1-idle-347x192.png",347,192,None),
            ("run",  f"{MO}/moose1-run-347x192.png",347,192,None),
            ("atk",  f"{MO}/moose1-attack-all effects-347x192.png",347,192,None),
            ("death",f"{MO}/moose1-death-347x192.png",347,192,None)]),
 ("alageyik", [("idle", f"{MO}/moose2-idle-347x192.png",347,192,None),
               ("run",  f"{MO}/moose2-run-347x192.png",347,192,None),
               ("atk",  f"{MO}/moose2-attack-all effects-347x192.png",347,192,None),
               ("death",f"{MO}/moose2-death-347x192.png",347,192,None)]),
 ("orumcek", [("idle", f"{CR}/Spider/Spider-variation1-idle.png",64,64,None),
              ("run",  f"{CR}/Spider/Spider-variation1-walk.png",64,64,None),
              ("atk",  f"{CR}/Spider/Spider-variation1-atk.png",64,64,None),
              ("death",f"{CR}/Spider/Spider-variation1-death.png",64,64,None)]),
 ("orumcek2", [("idle", f"{CR}/Spider/Spider-variation2-idle.png",64,64,None),
               ("run",  f"{CR}/Spider/Spider-variation2-walk.png",64,64,None),
               ("atk",  f"{CR}/Spider/Spider-variation2-atk.png",64,64,None),
               ("death",f"{CR}/Spider/Spider-variation2-death.png",64,64,None)]),
 ("ordek", [("idle", f"{SA}/ducks/duck1/small animals - duck-duck - idle - sideview.png",96,96,None),
            ("run",  f"{SA}/ducks/duck1/small animals - duck-duck - walk - sideview.png",96,96,None)]),
 ("kurbaga", [("idle", f"{SA}/frogs/frog1/small animals - frog-frog - idle 2 - sideview.png",96,96,None),
              ("run",  f"{SA}/frogs/frog1/small animals - frog-frog - walk - sideview.png",96,96,None)]),
 ("kukla", [("idle", f"{VN}/blacksmith/trainning dummy-hit1-noflash.png",128,128,None)]),
 # sandworm kare 224x192 (loop 8; ambush 5x5 grid; retreat 4x3 grid)
 ("kum_kurdu", [("idle", f"{DS}/sandworm/sandworm-loop.png",224,192,None),
                ("run",  None,0,0,None),
                ("atk",  f"{DS}/sandworm/sandworm-ambush.png",224,192,None),
                ("death",f"{DS}/sandworm/sandworm-retreat.png",224,192,None)]),
]
for name, states in J:
    # AUTO<n>: kare boyutu dosyadan (tek satır, n kare)
    fixed = []
    for (tag, path, fw, fh, row) in states:
        if isinstance(row, str) and row.startswith("AUTO"):
            n = int(row[4:])
            w, h, _ = png_read_rgba(path)
            fixed.append((tag, path, w // n, h, None))
        else:
            fixed.append((tag, path, fw, fh, row))
    bake(name, fixed)
