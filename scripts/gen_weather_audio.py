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

# --- SU KIYISI LOOP (11 sn → dikişli 10 sn): çırpıntı + dalga nefesi ---
# Alçak geçirgen gürültü tabanı + 0.33 Hz yavaş genlik salınımı ("dalga
# nefesi") + arada yumuşak "lap" kabarmaları. Son 1 sn başa crossfade
# edilip kırpılır → dikişsiz döngü.
DUR = 11.0
N = int(SR * DUR)
wash = noise_band(N, 0.12)            # yumuşak çalkantı bandı (alçak geçirgen)
deep = noise_band(N, 0.02)            # çok yavaş taban: derin su gövdesi
smp = [0.60 * wash[i] + 0.30 * deep[i] for i in range(N)]
# dalga nefesi: 0.33 Hz genlik salınımı (0.25-0.5 Hz aralığında)
smp = [smp[i] * (0.65 + 0.35 * math.sin(2 * math.pi * 0.33 * i / SR)) for i in range(N)]
lap = noise_band(N, 0.30)             # lap'ler için biraz daha parlak bant
for _ in range(12):   # yumuşak "lap" kabarmaları: kıyıya vuran küçük dalga
    t0 = random.uniform(0.0, DUR - 0.8)
    s0 = int(t0 * SR)
    dur = random.uniform(0.35, 0.65)
    n = int(dur * SR)
    for j in range(n):
        if s0 + j >= N: break
        env = math.sin(math.pi * j / n) ** 2      # yumuşak kabar-sön zarfı
        smp[s0 + j] += 0.35 * env * lap[s0 + j]
# dikişsiz döngü: son 1 sn'yi başa crossfade et, sonra kırp
CF = int(1.0 * SR)
for i in range(CF):
    a = i / CF
    smp[i] = smp[i] * a + smp[N - CF + i] * (1 - a)
save('assets/audio/amb_water.wav', smp[:N - CF])

# --- GÖLET LOOP (11 sn → dikişli 10 sn): durgun su, deniz kardeşinin sakini ---
# Aynı iskelet, farklı karakter: daha koyu/boğuk bant (düşük kesim), çok daha
# yavaş nefes (0.18 Hz), lap yok denecek kadar az (3 tane, yarı yumuşaklıkta)
# ve genel seviye kısık. Deniz/göl loop'undan SONRA üretilir ki seed düzeni
# bozulmasın (önceki WAV'lar bit-bit aynı kalır).
DUR = 11.0
N = int(SR * DUR)
wash = noise_band(N, 0.05)            # daha koyu çalkantı (kesim daha alçak)
deep = noise_band(N, 0.015)           # neredeyse hareketsiz dip
smp = [0.55 * wash[i] + 0.35 * deep[i] for i in range(N)]
# gölet nefesi: 0.18 Hz — deniz nefesinin neredeyse yarısı, salınım da sığ
smp = [smp[i] * (0.75 + 0.25 * math.sin(2 * math.pi * 0.18 * i / SR)) for i in range(N)]
lap = noise_band(N, 0.18)             # lap bandı da boğuk
for _ in range(3):    # seyrek, yarı yumuşaklıkta minik kıpırtılar
    t0 = random.uniform(0.0, DUR - 1.0)
    s0 = int(t0 * SR)
    dur = random.uniform(0.5, 0.8)
    n = int(dur * SR)
    for j in range(n):
        if s0 + j >= N: break
        env = math.sin(math.pi * j / n) ** 2
        smp[s0 + j] += 0.18 * env * lap[s0 + j]
CF = int(1.0 * SR)
for i in range(CF):
    a = i / CF
    smp[i] = smp[i] * a + smp[N - CF + i] * (1 - a)
save('assets/audio/amb_water_calm.wav', smp[:N - CF], peak_to=0.42)  # kısık
