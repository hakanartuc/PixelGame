#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# MAPGEN — el yapımı hissi veren harita üretim kütüphanesi (Faz MAPGEN).
# Teknikler: value-noise (organik sınırlar), drunkard's walk (kıvrımlı yol),
# cellular automata (doğal kaya/mağara kümeleri), Poisson-disk (grid'siz
# serpiştirme), stamp (el yapımı prefab parçalar), PNG önizleme (asistanın
# GÖRSEL iterasyon döngüsü — üret → bak → düzelt → yeniden üret).
# CSV çıktısı Tiled GID kuralıyla yazılır: değer = tile id + 1.
import random, math

# ---------------------------------------------------------------- noise
def value_noise(w, h, scale, seed, octaves=3):
    """0..1 değer gürültüsü — bağımlılıksız (bilinear + oktav toplamı)."""
    rnd = random.Random(seed)
    def grid(gw, gh):
        return [[rnd.random() for _ in range(gw + 2)] for _ in range(gh + 2)]
    out = [[0.0] * w for _ in range(h)]
    amp, total = 1.0, 0.0
    for o in range(octaves):
        cell = max(2.0, scale / (2 ** o))
        gw, gh = int(w / cell) + 2, int(h / cell) + 2
        g = grid(gw, gh)
        for y in range(h):
            for x in range(w):
                fx, fy = x / cell, y / cell
                x0, y0 = int(fx), int(fy)
                tx, ty = fx - x0, fy - y0
                tx = tx * tx * (3 - 2 * tx)   # smoothstep
                ty = ty * ty * (3 - 2 * ty)
                a = g[y0][x0] * (1 - tx) + g[y0][x0 + 1] * tx
                b = g[y0 + 1][x0] * (1 - tx) + g[y0 + 1][x0 + 1] * tx
                out[y][x] += (a * (1 - ty) + b * ty) * amp
        total += amp
        amp *= 0.5
    for y in range(h):
        for x in range(w):
            out[y][x] /= total
    return out

# ------------------------------------------------------------ drunkard
def drunk_path(x0, y0, x1, y1, seed, wobble=0.55, width=1):
    """Kıvrımlı yol: hedefe meyilli sarhoş yürüyüşü. Dönen: (x,y) hücre seti."""
    rnd = random.Random(seed)
    cells = set()
    x, y = float(x0), float(y0)
    guard = 0
    while (abs(x - x1) > 1 or abs(y - y1) > 1) and guard < 20000:
        guard += 1
        for dy in range(-width + 1, width):
            for dx in range(-width + 1, width):
                cells.add((int(x) + dx, int(y) + dy))
        if rnd.random() < wobble:
            ang = rnd.uniform(0, 2 * math.pi)
        else:
            ang = math.atan2(y1 - y, x1 - x) + rnd.uniform(-0.6, 0.6)
        x += math.cos(ang); y += math.sin(ang)
    cells.add((int(x1), int(y1)))
    return cells

# ------------------------------------------------- cellular automata
def ca_blobs(w, h, fill, seed, steps=4):
    """Doğal küme örüntüsü: rastgele doldur → komşu kuralıyla yumuşat.
    Dönen: bool matris (True = dolu)."""
    rnd = random.Random(seed)
    m = [[rnd.random() < fill for _ in range(w)] for _ in range(h)]
    for _ in range(steps):
        n = [[False] * w for _ in range(h)]
        for y in range(h):
            for x in range(w):
                c = 0
                for dy in (-1, 0, 1):
                    for dx in (-1, 0, 1):
                        yy, xx = y + dy, x + dx
                        if 0 <= yy < h and 0 <= xx < w and m[yy][xx]:
                            c += 1
                n[y][x] = c >= 5
        m = n
    return m

# ----------------------------------------------------------- poisson
def poisson(w, h, r, seed, ok=None):
    """Poisson-disk benzeri serpiştirme: min r aralıklı noktalar.
    ok(x,y) verilirse yalnız uygun hücrelere koyar."""
    rnd = random.Random(seed)
    pts = []
    for _ in range(w * h // max(1, int(r * r))):
        x, y = rnd.randint(1, w - 2), rnd.randint(1, h - 2)
        if ok and not ok(x, y):
            continue
        if all((x - px) ** 2 + (y - py) ** 2 >= r * r for px, py in pts):
            pts.append((x, y))
    return pts

# ----------------------------------------------------------- min2x2
def min2x2(grid, tid, repl):
    """tid hücrelerinden hiçbir 2x2 tam-tid karesine girmeyenleri repl yap.
    Autotile 1 genişlikli şerit/çıkıntı sevmez — bu geçiş onları temizler."""
    h, w = len(grid), len(grid[0])
    changed = True
    while changed:
        changed = False
        keep = [[False] * w for _ in range(h)]
        for y in range(h - 1):
            for x in range(w - 1):
                if all(grid[y + dy][x + dx] == tid
                       for dy in (0, 1) for dx in (0, 1)):
                    for dy in (0, 1):
                        for dx in (0, 1):
                            keep[y + dy][x + dx] = True
        for y in range(h):
            for x in range(w):
                if grid[y][x] == tid and not keep[y][x]:
                    grid[y][x] = repl
                    changed = True

# ------------------------------------------------------- fill_pockets
def fill_pockets(grid, bg, tid, min_size):
    """tid kütleleri içinde hapsolmuş KÜÇÜK bg cepleri (< min_size hücre)
    tid ile doldurulur — CA alanlarındaki delikler yüz/kenar kırıntısı
    üretmesin, mesalar büyük ve okunur kalsın."""
    h, w = len(grid), len(grid[0])
    seen = [[False] * w for _ in range(h)]
    for sy in range(h):
        for sx in range(w):
            if seen[sy][sx] or grid[sy][sx] != bg:
                continue
            region, stack, leaks = [], [(sx, sy)], False
            seen[sy][sx] = True
            while stack:
                x, y = stack.pop()
                region.append((x, y))
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    xx, yy = x + dx, y + dy
                    if not (0 <= xx < w and 0 <= yy < h):
                        continue
                    v = grid[yy][xx]
                    if v == bg and not seen[yy][xx]:
                        seen[yy][xx] = True
                        stack.append((xx, yy))
                    elif v != bg and v != tid:
                        leaks = True   # başka türe komşu — hapsolmuş sayma
            if not leaks and len(region) < min_size:
                for (x, y) in region:
                    grid[y][x] = tid

# ------------------------------------------------------------- stamp
def stamp(grid, sx, sy, rows, legend):
    """El yapımı prefab bas: rows = str listesi, legend = {karakter: tileId}.
    '.' = dokunma. Taşan kısımlar sessizce kırpılır."""
    for dy, row in enumerate(rows):
        for dx, ch in enumerate(row):
            if ch == '.':
                continue
            y, x = sy + dy, sx + dx
            if 0 <= y < len(grid) and 0 <= x < len(grid[0]):
                grid[y][x] = legend[ch]

# ----------------------------------------------------------- preview
def preview(grid, colors, path, cell=4, marks=None):
    """Renkli PNG önizleme — asistan Read ile bakar (görsel iterasyon)."""
    from PIL import Image
    h, w = len(grid), len(grid[0])
    im = Image.new("RGB", (w * cell, h * cell))
    px = im.load()
    for y in range(h):
        for x in range(w):
            c = colors.get(grid[y][x], (255, 0, 255))
            for dy in range(cell):
                for dx in range(cell):
                    px[x * cell + dx, y * cell + dy] = c
    if marks:
        for (mx, my, mc) in marks:
            for dy in range(-2, 3):
                for dx in range(-2, 3):
                    xx, yy = mx * cell + dx, my * cell + dy
                    if 0 <= xx < w * cell and 0 <= yy < h * cell:
                        px[xx, yy] = mc
    im.save(path)

def write_csv(grid, path):
    """Tiled GID kuralı: CSV değeri = tile id + 1 (0 = boş)."""
    with open(path, "w") as f:
        f.write("\n".join(",".join(str(c + 1) for c in row) for row in grid) + "\n")
