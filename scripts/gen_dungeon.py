#!/usr/bin/env python3
# Zindan kat haritalari — SABIT el yapimi oda duzeni (worldmap.c koordinatlariyla eslesik).
# Tile ID'leri (GID=id+1): 6 d1_floor, 7 d1_wall(kati), 8 d2_floor, 9 d2_wall(kati), 10 void(kati)
VOID = 10

def build(W, H, FLOOR, WALL, rooms, corridors):
    g = [[VOID]*W for _ in range(H)]
    def carve(x0, y0, x1, y1):
        for y in range(y0, y1+1):
            for x in range(x0, x1+1):
                g[y][x] = FLOOR
    for r in rooms: carve(*r)
    for c in corridors: carve(*c)
    # Duvar: floor'a komsu her void
    for y in range(H):
        for x in range(W):
            if g[y][x] != VOID: continue
            near = any(0 <= y+dy < H and 0 <= x+dx < W and g[y+dy][x+dx] == FLOOR
                       for dy in (-1,0,1) for dx in (-1,0,1))
            if near: g[y][x] = WALL
    return g

def write(path, g):
    with open(path, 'w') as f:
        for row in g:
            f.write(','.join(str(t+1) for t in row) + '\n')
    print(path, len(g[0]), 'x', len(g))

# --- KAT 1 (44x34): A giris / B savascilar / C okcular / D dip (merdiven) ---
f1 = build(44, 34, 6, 7,
    rooms=[(5,5,11,9),      # A giris (portal 6,6 / cikis dünyaya)
           (19,5,29,11),    # B
           (7,17,15,25),    # C
           (23,19,37,29)],  # D (merdiven 36,28)
    corridors=[(11,7,19,8),   # A→B
               (25,11,26,19), # B→D
               (8,9,9,17),    # A→C
               (15,21,23,22)])# C→D
write('assets/maps/dungeon_f1.csv', f1)

# --- KAT 2 (40x30): E giris / F salon / G boss odasi ---
f2 = build(40, 30, 8, 9,
    rooms=[(4,4,9,9),       # E giris (merdiven 5.5,5.5)
           (15,4,27,13),    # F salon
           (13,19,31,27)],  # G boss odasi (genis)
    corridors=[(9,6,15,7),    # E→F
               (20,13,21,19)])# F→G
write('assets/maps/dungeon_f2.csv', f2)
