#!/usr/bin/env python3
# Yeni müzik parçaları (Cila): dungeon / night / menu — mevcut 4 parçayla aynı
# format (44100 Hz, mono, 16-bit, ~32 sn kesintisiz loop). Saf sinüs/zarf sentezi.
import math, struct, wave

SR = 44100
DUR = 32.0
N = int(SR * DUR)

def note(freq, t, dur, vol=0.5, attack=0.02, shape='sine'):
    # t: nota içi zaman; basit ADSR-benzeri zarf (attack + üstel sönüm)
    if t < 0 or t >= dur: return 0.0
    env = min(t / attack, 1.0) * math.exp(-2.2 * t / dur)
    if shape == 'sine':
        v = math.sin(2 * math.pi * freq * t)
    elif shape == 'tri':
        ph = (freq * t) % 1.0
        v = 4.0 * abs(ph - 0.5) - 1.0
    else:  # soft square
        v = 0.6 if math.sin(2 * math.pi * freq * t) > 0 else -0.6
    return v * env * vol

def save(path, samples):
    peak = max(1e-6, max(abs(s) for s in samples))
    k = 0.72 / peak   # tepe -3dB civarı: crossfade'de toplamda taşmasın
    with wave.open(path, 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes(b''.join(
            struct.pack('<h', int(max(-1, min(1, s * k)) * 32767))
            for s in samples))
    print("OK", path)

A2, C3, D3, E3, G3, A3 = 110.0, 130.81, 146.83, 164.81, 196.0, 220.0
C4, D4, E4, G4, A4, C5 = 261.63, 293.66, 329.63, 392.0, 440.0, 523.25

# --- DUNGEON: derin dron + seyrek uzak çan — taşların altında bir şey bekliyor
smp = [0.0] * N
for i in range(N):
    t = i / SR
    # iki detune'lu düşük dron (yavaş vuru)
    d = 0.16 * math.sin(2 * math.pi * 55.0 * t) \
      + 0.14 * math.sin(2 * math.pi * 55.6 * t) \
      + 0.07 * math.sin(2 * math.pi * 110.3 * t)
    d *= 0.8 + 0.2 * math.sin(2 * math.pi * t / 16.0)   # 16sn nefes
    smp[i] = d
# uzak çanlar: 8 vuruş, minör renkler, uzun sönüm
for k, (bt, f) in enumerate([(1.5, A3), (6.0, C4), (11.0, E3), (14.5, D3),
                             (17.5, A3), (22.0, G3), (26.0, C4), (29.5, E3)]):
    s0 = int(bt * SR)
    for j in range(int(4.0 * SR)):
        if s0 + j >= N: break
        tt = j / SR
        smp[s0 + j] += note(f, tt, 4.0, vol=0.20) \
                     + note(f * 2.01, tt, 2.2, vol=0.06)
save('assets/audio/music_dungeon.wav', smp)

# --- NIGHT: yumuşak koyu pad + tek tük yüksek pırıltı — sakin ama tekin değil
smp = [0.0] * N
for i in range(N):
    t = i / SR
    p = 0.11 * math.sin(2 * math.pi * A2 * t) \
      + 0.09 * math.sin(2 * math.pi * E3 * t) \
      + 0.05 * math.sin(2 * math.pi * C3 * (1.0 + 0.0008 * math.sin(t / 5.0)) * t)
    p *= 0.75 + 0.25 * math.sin(2 * math.pi * t / 12.0)
    smp[i] = p
for bt, f in [(3.0, A4), (9.5, E4), (13.0, C5), (20.0, A4), (24.5, G4), (29.0, E4)]:
    s0 = int(bt * SR)
    for j in range(int(2.5 * SR)):
        if s0 + j >= N: break
        smp[s0 + j] += note(f, j / SR, 2.5, vol=0.10, shape='tri')
save('assets/audio/music_night.wav', smp)

# --- MENU: mühür teması — kalp ritminde bas nabız + ağır pad melodisi
smp = [0.0] * N
for i in range(N):
    t = i / SR
    p = 0.10 * math.sin(2 * math.pi * D3 * t) + 0.08 * math.sin(2 * math.pi * A3 * t)
    p *= 0.7 + 0.3 * math.sin(2 * math.pi * t / 10.0)
    smp[i] = p
# nabız: ~52 bpm çift vuruş (lup-dup) — başlık ekranının kalbi
beat = 60.0 / 52.0
bt = 0.0
while bt < DUR:
    for off, v in [(0.0, 0.30), (0.16, 0.18)]:
        s0 = int((bt + off) * SR)
        for j in range(int(0.22 * SR)):
            if s0 + j >= N: break
            smp[s0 + j] += note(58.0, j / SR, 0.22, vol=v, attack=0.004)
    bt += beat
# melodi: yavaş, soru soran (D dorian hissi)
for bt, f, dur in [(2.0, D4, 3.0), (6.0, E4, 2.5), (10.0, A3, 4.0),
                   (16.0, C4, 3.0), (20.0, D4, 2.5), (24.0, G3, 5.0)]:
    s0 = int(bt * SR)
    for j in range(int(dur * SR)):
        if s0 + j >= N: break
        smp[s0 + j] += note(f, j / SR, dur, vol=0.13, attack=0.3, shape='tri')
save('assets/audio/music_menu.wav', smp)
