#!/usr/bin/env python3
# DEMIRBOYUN acilis vinyeti panelleri — 4 sahne.
# gen_intro.py'nin yerine gecer (o v0 kanonuna aitti: donmus dunya / dokuz atim /
# baskasinin anisi / uyanis — hepsi oyuncuya SECILMISLIK dayatiyordu).
#
# Yeni dort an (GDD 1.2d, LORE 0):
#   1 bitmemis  catisiz taht salonu, vincte asili tas taht, duvarda ALTI BOS CIVI
#   2 sandalye  tahtin ARKASINDAKI sandalyede oturan adam + alti silüet
#   3 tarih     katibin defteri: uc tarih uzeri cizili, dorduncusu yeni
#   4 disari    sur dibi kalabalik, kontrat panosu, ip ceken isciler
#
# Anlatici yok, oyuncu yok, kimse kameraya bakmiyor. Panellerde MOR/mühür imgesi
# YASAK — o v0 diliydi. Ring/glow yardimcilari da bu yuzden kullanilmiyor.
#
# Cikti: assets/scenes/intro_bitmemis.png, intro_sandalye.png,
#        intro_tarih.png, intro_disari.png
# Kullanim: proje kokunden  python3 scripts/gen_intro_demirboyun.py
import zlib, struct, math, os


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
                b = prev[i]; cc = prev[i-ch] if i >= ch else 0
                pp = a + b - cc
                pa, pb, pc = abs(pp-a), abs(pp-b), abs(pp-cc)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else cc)
                line[i] = (line[i] + pr) & 255
        for x in range(W):
            o = (y*W + x) * 4; s = x * ch
            img[o] = line[s]; img[o+1] = line[s+1]; img[o+2] = line[s+2]
            img[o+3] = line[s+3] if ch == 4 else 255
        prev = line
    return (W, H, img)


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


class Canvas:
    def __init__(self, W, H, rgb=(20, 22, 30)):
        self.W, self.H = W, H
        self.px = bytearray(W*H*4)
        for i in range(W*H):
            self.px[i*4:i*4+4] = bytes([rgb[0], rgb[1], rgb[2], 255])

    def blit(self, src, sx, sy, sw, sh, dx, dy, tint=(255, 255, 255), alpha=255):
        W, H, img = src
        for y in range(sh):
            for x in range(sw):
                if sy+y >= H or sx+x >= W: continue
                so = ((sy+y)*W + (sx+x)) * 4
                a = img[so+3]
                if a < 8: continue
                px, py = dx+x, dy+y
                if px < 0 or px >= self.W or py < 0 or py >= self.H: continue
                do = (py*self.W + px) * 4
                a2 = a * alpha // 255
                for c in range(3):
                    sc = img[so+c] * tint[c] // 255
                    self.px[do+c] = (sc * a2 + self.px[do+c] * (255-a2)) // 255
                self.px[do+3] = 255

    def rect(self, x0, y0, w, h, rgb, alpha=255):
        for y in range(max(0, y0), min(self.H, y0+h)):
            for x in range(max(0, x0), min(self.W, x0+w)):
                o = (y*self.W + x)*4
                for c in range(3):
                    self.px[o+c] = (rgb[c]*alpha + self.px[o+c]*(255-alpha)) // 255

    def glow(self, cx, cy, r, rgb, strength=120):
        for y in range(max(0, cy-r), min(self.H, cy+r)):
            for x in range(max(0, cx-r), min(self.W, cx+r)):
                d = math.hypot(x-cx, y-cy) / r
                if d >= 1.0: continue
                a = int(strength * (1.0-d) * (1.0-d))
                if a <= 0: continue
                o = (y*self.W + x)*4
                for c in range(3):
                    v = self.px[o+c] + rgb[c]*a//255
                    self.px[o+c] = v if v < 255 else 255

    def vignette(self, strength=140):
        cx, cy = self.W/2, self.H/2
        maxd = math.hypot(cx, cy)
        for y in range(self.H):
            for x in range(self.W):
                d = math.hypot(x-cx, y-cy) / maxd
                dark = int(strength * max(0.0, d-0.45) / 0.55)
                if dark <= 0: continue
                o = (y*self.W + x)*4
                for c in range(3):
                    v = self.px[o+c] - dark
                    self.px[o+c] = v if v > 0 else 0


W, H = 208, 117   # 16:9 — oyunda ~5x nearest cizilir (gen_intro.py ile ayni)

OUT = 'assets/scenes'
HOUSE = load_png('assets/buildings/House_1_Limestone_Base_Red.png')   # 96x128
ARCH  = load_png('assets/decor/dungeon_arch.png')                     # 32x32
BENCH = load_png('assets/decor/Benches.png')                          # 64x32
# NPC sayfalari: LPC duzeni, 32x32 kare. Ayakta duran kare = satir 2 (idle guney).
NPCS = [load_png('assets/characters/npcs/%s.png' % n) for n in
        ('Farmer_Bob', 'Bartender_Bruno', 'Chef_Chloe', 'Fisherman_Fin',
         'Guard_Swordman', 'Farmer_Buba')]

STONE_W = (74, 70, 66)     # kirli tas duvar
STONE_D = (40, 38, 36)     # golgeli tas
NIGHT   = (18, 20, 26)     # acik gok (catisiz salon)
LAMP    = (255, 196, 108)  # kandil


def wall(c, y0, h, tint=STONE_W):
    """House sprite'inin tas govdesinden yatay duvar seridi kes ve yan yana bas.
    Her kolonda KAYNAK OFSETI degisir; yoksa ayni kapi/pencere yedi kez tekrarlanip
    duvar degil kolonad gibi okunuyor."""
    offs = (8, 30, 14, 44, 22, 36, 6)
    for i, x in enumerate(range(0, W, 32)):
        sx = offs[i % len(offs)]
        sw = min(32, 96 - sx)
        c.blit(HOUSE, sx, 74, sw, min(48, h), x, y0, tint=tint)


def rain(c, y0, y1, n=34, alpha=52):
    """Catisiz salona giren yagmur: hafif egik ince cizgiler."""
    for i in range(n):
        x = (i * 61) % W
        ln = 5 + (i % 4) * 3
        for k in range(ln):
            c.rect(x + k//3, y0 + ((i*13) % max(1, y1-y0-ln)) + k, 1, 1,
                   (168, 186, 205), alpha=alpha)


def silhouette(c, npc, dx, dy, dark=52, scale2=False):
    """NPC sprite'ini neredeyse siyaha boyayip silüet yap (yuz gorunmez)."""
    t = (dark, dark, int(dark*1.1))
    c.blit(npc, 32, 64, 32, 32, dx, dy, tint=t)


# ---------------------------------------------------------------------------
# 1) BITMEMIS — catisiz taht salonu, vincte asili tas taht, ALTI BOS CIVI
#    Asagidan yukari bakis: gok acik, yagmur iceri giriyor, is bitmemis.
# ---------------------------------------------------------------------------
c = Canvas(W, H, NIGHT)
c.rect(0, 0, W, 46, (26, 30, 40))                  # acik gok (tavan YOK)
for x in range(0, W, 26):                          # kirik duvar ust hizasi
    c.rect(x, 42 + (x//26 % 3), 24, 6, STONE_D, alpha=235)
wall(c, 46, 48)                                    # arka duvar
c.rect(0, 92, W, 25, (52, 48, 44))                 # zemin
# --- vinc: iki dikey iskele diregi + yatay kiris + halat
c.rect(28, 8, 4, 86, (86, 66, 44))                 # sol iskele diregi
c.rect(176, 8, 4, 86, (86, 66, 44))                # sag iskele diregi
c.rect(28, 8, 152, 4, (96, 74, 50))                # kiris
c.rect(102, 12, 2, 22, (128, 118, 96))             # HALAT
# --- asili tas taht: kimildamiyor
c.rect(88, 34, 30, 22, (96, 92, 88))
c.rect(88, 34, 30, 3, (128, 124, 118))             # ust kenar isik alir
c.rect(92, 56, 22, 4, (58, 56, 54), alpha=230)     # altinda golge
# --- SANCAK RAYI + ALTI BOS CIVI (LORE 1.4: ilerleme cubugu duvarda asili).
#     Once duz bir siva bandi ciziyoruz: civiler mesgul tas dokusunda kayboluyordu
#     ve bu panelin BUTUN amaci o alti civinin sayilabilmesi.
c.rect(38, 58, 132, 14, (58, 55, 52), alpha=245)   # siva bandi (duz zemin)
c.rect(38, 58, 132, 1, (86, 82, 78), alpha=200)    # bandin ust kenari isik alir
c.rect(38, 71, 132, 1, (28, 26, 24), alpha=200)    # alt kenar golge
for i in range(6):
    nx = 46 + i*22
    c.rect(nx, 61, 3, 5, (188, 172, 132))          # CIVI — parlak metal
    c.rect(nx+3, 62, 2, 2, (146, 132, 100))        # civinin egik basi
    c.rect(nx-1, 67, 6, 2, (22, 20, 18), alpha=190)  # civinin golgesi
    c.glow(nx+1, 63, 5, (120, 110, 86), strength=26)  # zayif metal parlamasi
# --- iskele diregine tebesir yazisi (okunmaz, VARLIGI yeter)
for i in range(7):
    c.rect(33, 52 + i*3, 5 + (i % 3)*4, 1, (206, 202, 190), alpha=150)
rain(c, 0, 92)
c.glow(196, 74, 16, LAMP, strength=70)             # sagda tek kandil
c.rect(195, 70, 3, 5, (58, 50, 40))
c.vignette(150)
save_png(os.path.join(OUT, 'intro_bitmemis.png'), W, H, c.px)

# ---------------------------------------------------------------------------
# 2) SANDALYE — tahtin ARKASINDAKI sandalyede oturan adam, cevresinde alti silüet.
#    Kimse ona bakmiyor. LORE 0 anlaticisiz kuruluyor: alti agiz, bir sandalye.
# ---------------------------------------------------------------------------
c = Canvas(W, H, NIGHT)
c.rect(0, 0, W, 30, (24, 28, 38))
wall(c, 30, 48)
c.rect(0, 78, W, 39, (48, 44, 42))                 # zemin
# --- tas taht: bos, one dogru (asili taht bu panelde YUKARIDA kaliyor)
c.rect(84, 44, 26, 34, (88, 84, 80))
c.rect(84, 44, 26, 3, (116, 112, 106))
# --- ARKADAKI sade sandalye + oturan adam (tahttan kucuk, tahttan onemli)
c.rect(124, 56, 15, 21, (78, 60, 42))              # sandalye (tahtin SAGINDA)
c.rect(124, 53, 15, 3, (94, 74, 52))               # arkalik
c.blit(NPCS[0], 32, 64, 32, 32, 116, 47, tint=(186, 176, 162))   # oturan adam
c.glow(131, 57, 16, LAMP, strength=72)             # yuzune dusen tek isik
# --- alti silüet: hicbiri sandalyeye donuk degil
poses = [(30, 62), (52, 66), (74, 70), (148, 66), (168, 62), (186, 68)]
for i, (sx, sy) in enumerate(poses):
    silhouette(c, NPCS[i % len(NPCS)], sx, sy, dark=44 + (i % 3)*6)
c.rect(56, 58, 2, 10, (58, 58, 62))               # biri elini kaldirmis
c.rect(54, 56, 6, 2, (58, 58, 62))
rain(c, 0, 30, n=16, alpha=34)
c.vignette(158)
save_png(os.path.join(OUT, 'intro_sandalye.png'), W, H, c.px)

# ---------------------------------------------------------------------------
# 3) TARIH — katibin defteri: uc tarih uzeri cizili, dorduncusu murekkebi islak.
#    Kenarda kurumus keskisiyle tas ustasi ve HIC OYULMAMIS bos tas.
# ---------------------------------------------------------------------------
c = Canvas(W, H, (28, 24, 22))
c.rect(0, 0, W, 40, (34, 29, 26))                  # karanlik ust
c.rect(0, 40, W, 77, (66, 52, 38))                 # ahsap masa yuzeyi
for y in range(44, H, 7):                          # tahta damarlari
    c.rect(0, y, W, 1, (54, 42, 30), alpha=120)
# --- DEFTER: iki sayfa
c.rect(22, 48, 96, 58, (206, 196, 172))
c.rect(22, 48, 46, 58, (196, 186, 162))            # sol sayfa biraz koyu
c.rect(68, 48, 2, 58, (150, 140, 120))             # sirt
for i in range(3):                                 # UC tarih, uzeri CIZILI
    ly = 58 + i*13
    c.rect(78, ly, 32, 2, (58, 46, 34))            # yazi
    c.rect(73, ly, 42, 2, (156, 34, 26))           # UZERI CIZIK (kirmizi, 2px)
c.rect(78, 97, 30, 2, (22, 20, 34))                # DORDUNCU: taze, cizilmemis
c.glow(93, 98, 9, (120, 140, 220), strength=40)    # murekkebi islak parlıyor
c.rect(120, 92, 14, 3, (40, 34, 28))               # kalem
c.rect(133, 90, 5, 6, (86, 80, 62))                # hokka
# --- BOS TAS + kurumus keski (sagda, masanin kenarinda)
c.rect(158, 62, 40, 34, (104, 100, 94))            # hic oyulmamis tas
c.rect(158, 62, 40, 3, (132, 128, 120))
c.rect(150, 88, 14, 3, (72, 66, 54))               # keski, yatiyor
c.rect(148, 87, 4, 5, (46, 42, 36))                # sapi
c.glow(40, 46, 22, LAMP, strength=62)              # solda kandil
c.vignette(168)
save_png(os.path.join(OUT, 'intro_tarih.png'), W, H, c.px)

# ---------------------------------------------------------------------------
# 4) DISARI — sur dibi, KALABALIGIN ICINDEN bakis. Gazi kuyrugu, kontrat panosu,
#    iskelede ip ceken isciler. Hicbir yuz kameraya donuk degil.
# ---------------------------------------------------------------------------
c = Canvas(W, H, (36, 40, 50))
c.rect(0, 0, W, 34, (48, 54, 66))                  # safak gogu
wall(c, 26, 48, tint=(66, 62, 58))                 # SUR
c.rect(0, 74, W, 43, (74, 68, 56))                 # toprak zemin
for i in range(28):                                # zeminde ayak izleri/çamur
    c.rect((i*29) % W, 78 + (i % 5)*7, 3, 2, (60, 54, 44), alpha=150)
# --- iskele: iki direk + ip ceken isciler
c.rect(12, 22, 3, 56, (86, 66, 44))
c.rect(190, 22, 3, 56, (86, 66, 44))
c.rect(12, 22, 181, 3, (96, 74, 50))
c.rect(120, 25, 2, 30, (128, 118, 96))             # inen ip
silhouette(c, NPCS[4], 108, 56, dark=58)           # ipi ceken isci
silhouette(c, NPCS[3], 132, 58, dark=54)
# --- KONTRAT PANOSU: civiyle tutturulmus kagit
c.rect(150, 44, 34, 26, (92, 72, 48))
c.rect(150, 44, 34, 2, (112, 88, 58))
c.rect(157, 49, 19, 15, (214, 206, 184))           # kagit
c.rect(166, 48, 2, 2, (150, 138, 112))             # civi
for i in range(4):
    c.rect(159, 52 + i*3, 14 - (i % 2)*5, 1, (70, 60, 48), alpha=200)
# --- GAZI KUYRUGU: onden arkaya, hicbiri kameraya bakmiyor
queue = [(26, 62), (44, 64), (60, 66), (76, 68), (92, 70)]
for i, (sx, sy) in enumerate(queue):
    silhouette(c, NPCS[(i+1) % len(NPCS)], sx, sy, dark=62 + (i % 3)*8)
c.glow(168, 56, 18, LAMP, strength=54)             # panonun yanindaki fener
c.vignette(140)
save_png(os.path.join(OUT, 'intro_disari.png'), W, H, c.px)

print('4 panel uretildi:')
for n in ('intro_bitmemis', 'intro_sandalye', 'intro_tarih', 'intro_disari'):
    p = os.path.join(OUT, n + '.png')
    print('  %-34s %d bayt' % (p, os.path.getsize(p)))
