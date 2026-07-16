#!/usr/bin/env python3
# Hava sesleri (Dünya Olayları 2): yağmur ambiyans loop'u + gök gürültüsü SFX.
# gen_ambience.py'nin kardeşi — 44100 Hz mono 16-bit, dış bağımlılık yok.
import math, struct, wave, random

SR = 44100
random.seed(11)   # deterministik üretim (tekrar üretilebilir asset)

def save(path, samples, peak_to=0.6):
    peak = max(1e-6, max(abs(s) for s in samples))
    k = peak_to / peak
    with wave.open(path, 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes(b''.join(
            struct.pack('<h', int(max(-1, min(1, s * k)) * 32767))
            for s in samples))
    print("OK", path)

def noise_band(n, smooth):
    out = [0.0] * n
    v = 0.0
    for i in range(n):
        v += (random.random() * 2 - 1 - v) * smooth
        out[i] = v
    return out

# --- YAĞMUR LOOP (24 sn): yüksek bantlı hışırtı + tek tük iri damla ---
DUR = 24.0
N = int(SR * DUR)
hiss = noise_band(N, 0.55)            # hızlı gürültü: yağmur hışırtısı
bed  = noise_band(N, 0.03)            # yavaş taban: uzak uğultu
smp = [0.55 * hiss[i] + 0.25 * bed[i] for i in range(N)]
# dalgalanma: sağanak nefes alır (9 sn periyot)
smp = [smp[i] * (0.8 + 0.2 * math.sin(2 * math.pi * i / SR / 9.0)) for i in range(N)]
for _ in range(70):   # iri damlalar: kısa tok tıklar
    t0 = random.uniform(0.0, DUR - 0.05)
    s0 = int(t0 * SR)
    f = random.uniform(900, 1600)
    for j in range(int(0.018 * SR)):
        if s0 + j >= N: break
        tt = j / SR
        env = math.exp(-tt * 260)
        smp[s0 + j] += 0.30 * env * math.sin(2 * math.pi * f * tt)
save('assets/audio/amb_rain.wav', smp)

# --- GÖK GÜRÜLTÜSÜ SFX (2.8 sn): patlama + yuvarlanan alçak uğultu ---
DUR = 2.8
N = int(SR * DUR)
rum = noise_band(N, 0.006)            # çok alçak bant: gövde
crk = noise_band(N, 0.5)              # çatırtı bandı
smp = [0.0] * N
for i in range(N):
    t = i / SR
    attack = math.exp(-t * 9.0)               # ilk çatlama hızlı söner
    roll = math.exp(-t * 1.4) * (0.6 + 0.4 * math.sin(2 * math.pi * t / 0.7))
    smp[i] = 0.9 * attack * crk[i] + 1.4 * roll * rum[i]
save('assets/audio/thunder.wav', smp, peak_to=0.8)
