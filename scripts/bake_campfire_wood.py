#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# campfire.png'den ODUN-ONLY varyantı türetir: assets/decor/campfire_wood.png
#
# NEDEN (Faz ALEV, 2026-08-22): kamp ateşinin alevi artık shader'dan geliyor
# (flame.frag) ve sprite'ın GÖMÜLÜ alev animasyonu onunla üst üste yanıyordu.
# Hakan kararı: shader alevi kalır → sprite'tan odun yığını bölümü (satır
# 20-31, çapraz kütükler + kesik uçlar + dip) alınır ve içindeki alev renkleri
# ŞEFFAFLANIR. Ortada kalan delik ÖZELLİK: shader alevi odunların ARKASINA
# çizilir ve bu delikten görünür — "ateş odunların arasında yanıyor" okunur.
#
# Alev rengi ölçümle seçildi (kaynak dosyada üç ton): (255,200,37),
# (255,162,20), (237,118,20) → kural r>=230 AND b<45. Odun kahveleri
# (184,111,80 ve koyusu) ile kesik uç kremleri (234,212,170; 228,166,114)
# kurala TAKILMAZ, yerinde kalır.
#
# Kaynak 8 kare × 16px (128×32 RGBA); çıktı 8 kare × 16px (128×12).
# bake_cave_ring.py deseni: türetilmiş asset, kaynak dosyaya dokunulmaz.
import os, struct, zlib

root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
SRC = os.path.join(root, "assets/decor/campfire.png")
DST = os.path.join(root, "assets/decor/campfire_wood.png")
ROW0, ROWN = 20, 32          # alınan bant: odun yığını satırları
FLAME = lambda r, g, b: r >= 230 and b < 45


def png_read_rgba(path):
    """Minimal PNG okuyucu (8-bit RGBA, tüm filtreler). PIL şart değil —
    bu makinede PIL'siz python'la da koşsun (Xcode python3 vakası)."""
    d = open(path, "rb").read()
    assert d[:8] == b"\x89PNG\r\n\x1a\n", "PNG degil"
    pos, w, h, idat = 8, 0, 0, b""
    while pos < len(d):
        ln = struct.unpack(">I", d[pos:pos + 4])[0]
        tag = d[pos + 4:pos + 8]
        body = d[pos + 8:pos + 8 + ln]
        if tag == b"IHDR":
            w, h, bit, ct = struct.unpack(">IIBB", body[:10])
            assert bit == 8 and ct == 6, f"beklenen 8-bit RGBA, gelen bit={bit} ct={ct}"
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
    out = b"\x89PNG\r\n\x1a\n"
    out += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
    out += chunk(b"IDAT", zlib.compress(raw, 9))
    out += chunk(b"IEND", b"")
    open(path, "wb").write(out)


w, h, pix = png_read_rgba(SRC)
oh = ROWN - ROW0
out = bytearray(w * oh * 4)
stripped = 0
for y in range(oh):
    for x in range(w):
        i = ((y + ROW0) * w + x) * 4
        r, g, b, a = pix[i], pix[i + 1], pix[i + 2], pix[i + 3]
        if a > 8 and FLAME(r, g, b):
            r = g = b = a = 0
            stripped += 1
        o = (y * w + x) * 4
        out[o:o + 4] = bytes((r, g, b, a))
png_write_rgba(DST, w, oh, out)
print(f"OK: {DST}  ({w}x{oh}, {stripped} alev pikseli şeffaflandı)")
