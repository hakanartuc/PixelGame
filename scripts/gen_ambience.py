#!/usr/bin/env python3
# Ambiyans loop'ları (Cila): gündüz kuş/rüzgar, gece cırcır, zindan damla/uğultu.
# 44100 Hz mono 16-bit, 24 sn kesintisiz loop — müziğin ALTINDA hafif katman.
import math, struct, wave, random

SR = 44100
DUR = 24.0
N = int(SR * DUR)
random.seed(7)   # deterministik üretim (tekrar üretilebilir asset)

def save(path, samples):
    peak = max(1e-6, max(abs(s) for s in samples))
    k = 0.6 / peak
    with wave.open(path, 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes(b''.join(
            struct.pack('<h', int(max(-1, min(1, s * k)) * 32767))
            for s in samples))
    print("OK", path)

def noise_band(n, smooth):
    # Kaba alçak geçiren gürültü: rüzgar/uğultu tabanı
    out = [0.0] * n
    v = 0.0
    for i in range(n):
        v += (random.random() * 2 - 1 - v) * smooth
        out[i] = v
    return out

# --- GÜNDÜZ: yumuşak rüzgar + tek tük kuş cıvıltısı ---
wind = noise_band(N, 0.02)
smp = [0.35 * wind[i] * (0.7 + 0.3 * math.sin(2 * math.pi * i / SR / 9.0))
       for i in range(N)]
for _ in range(10):   # kuşlar: kısa frekans süpürmeli çift hece
    t0 = random.uniform(0.5, DUR - 1.5)
    f0 = random.uniform(2200, 3400)
    for hece in range(random.randint(2, 3)):
        s0 = int((t0 + hece * 0.18) * SR)
        for j in range(int(0.11 * SR)):
            if s0 + j >= N: break
            tt = j / SR
            f = f0 * (1.0 + 0.25 * math.sin(tt * 60))
            env = math.sin(math.pi * tt / 0.11)
            smp[s0 + j] += 0.12 * env * math.sin(2 * math.pi * f * tt)
save('assets/audio/amb_day.wav', smp)

# --- GECE: cırcır böcekleri + çok hafif rüzgar ---
wind = noise_band(N, 0.012)
smp = [0.18 * wind[i] for i in range(N)]
crick = 4300.0
t = 0.4
while t < DUR - 0.5:            # tril grupları: 3-5 vuruşluk cır-cır
    for k in range(random.randint(3, 5)):
        s0 = int((t + k * 0.062) * SR)
        for j in range(int(0.038 * SR)):
            if s0 + j >= N: break
            tt = j / SR
            env = math.sin(math.pi * tt / 0.038)
            smp[s0 + j] += 0.10 * env * math.sin(2 * math.pi * crick * tt) \
                                 * (0.6 + 0.4 * math.sin(2 * math.pi * 38 * tt))
    t += random.uniform(0.9, 2.2)
save('assets/audio/amb_night.wav', smp)

# --- ZİNDAN: derin uğultu + seyrek su damlası (yankılı) ---
rumble = noise_band(N, 0.004)
smp = [0.30 * rumble[i] * (0.7 + 0.3 * math.sin(2 * math.pi * i / SR / 13.0))
       for i in range(N)]
for _ in range(9):
    t0 = random.uniform(0.5, DUR - 1.0)
    f0 = random.uniform(900, 1500)
    for eko in range(3):        # damla + iki sönük yankı
        s0 = int((t0 + eko * 0.21) * SR)
        vol = 0.16 * (0.55 ** eko)
        for j in range(int(0.09 * SR)):
            if s0 + j >= N: break
            tt = j / SR
            f = f0 * (1.0 - 2.2 * tt)   # düşen perde: "plip"
            env = math.exp(-tt * 40)
            smp[s0 + j] += vol * env * math.sin(2 * math.pi * f * tt)
save('assets/audio/amb_dungeon.wav', smp)
