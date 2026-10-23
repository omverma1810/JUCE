"""Generates demo/demo_track.wav: a ~34 s royalty-free chill loop (keys, bass, drums, melody)
for demonstrating Vibe FX. Pure Python, no dependencies."""
import math
import random
import struct
import wave

SR = 44100
BPM = 84
BEAT = 60.0 / BPM
BARS = 12
LENGTH = int(SR * BEAT * 4 * BARS)
random.seed(7)

left = [0.0] * LENGTH
right = [0.0] * LENGTH


def midi_hz(n):
    return 440.0 * 2 ** ((n - 69) / 12)


def add(start_s, samples, pan=0.0):
    start = int(start_s * SR)
    gl = math.cos((pan + 1) * math.pi / 4)
    gr = math.sin((pan + 1) * math.pi / 4)
    for i, s in enumerate(samples):
        j = start + i
        if j >= LENGTH:
            break
        left[j] += s * gl
        right[j] += s * gr


def keys(note, dur, vel=0.12):
    # Electric-piano-like tone: decaying sine stack with slight detune and tremolo
    f = midi_hz(note)
    n = int(dur * SR)
    out = []
    for i in range(n):
        t = i / SR
        env = math.exp(-t * 1.6) * min(1.0, t * 200)
        trem = 1.0 + 0.15 * math.sin(2 * math.pi * 4.5 * t)
        s = (math.sin(2 * math.pi * f * t) + 0.35 * math.sin(2 * math.pi * f * 2.003 * t) * math.exp(-t * 4)
             + 0.12 * math.sin(2 * math.pi * f * 3.01 * t) * math.exp(-t * 7))
        out.append(s * env * trem * vel)
    return out


def bass(note, dur, vel=0.32):
    f = midi_hz(note)
    n = int(dur * SR)
    return [(math.sin(2 * math.pi * f * i / SR) + 0.2 * math.sin(4 * math.pi * f * i / SR))
            * min(1.0, i / 200) * math.exp(-i / SR * 1.2) * vel for i in range(n)]


def kick():
    n = int(0.35 * SR)
    out, phase = [], 0.0
    for i in range(n):
        t = i / SR
        phase += 2 * math.pi * (45 + 110 * math.exp(-t * 28)) / SR
        out.append(math.sin(phase) * math.exp(-t * 9) * 0.6)
    return out


def snare():
    n = int(0.25 * SR)
    return [((random.random() * 2 - 1) * 0.7 + 0.3 * math.sin(2 * math.pi * 190 * i / SR))
            * math.exp(-i / SR * 18) * 0.25 for i in range(n)]


def hat(vel=0.07):
    n = int(0.05 * SR)
    out, prev = [], 0.0
    for i in range(n):
        x = random.random() * 2 - 1
        out.append((x - prev) * math.exp(-i / SR * 70) * vel)  # crude high-pass
        prev = x
    return out


def melody_note(note, dur, vel=0.09):
    f = midi_hz(note)
    n = int(dur * SR)
    out = []
    for i in range(n):
        t = i / SR
        vib = 1 + 0.004 * math.sin(2 * math.pi * 5.5 * t)
        env = min(1.0, t * 40) * math.exp(-t * 1.2)
        out.append((math.sin(2 * math.pi * f * vib * t) + 0.3 * math.sin(4 * math.pi * f * vib * t)) * env * vel)
    return out


# Fmaj7 - Em7 - Dm7 - Cmaj7, 3 times
chords = [[53, 57, 60, 64], [52, 55, 59, 62], [50, 53, 57, 60], [48, 52, 55, 59]]
roots = [41, 40, 38, 36]
melody = [72, 69, 67, 64, 71, 67, 64, 62, 69, 65, 64, 60, 67, 64, 62, 59]

kick_s, snare_s = kick(), snare()

for bar in range(BARS):
    t0 = bar * 4 * BEAT
    c = bar % 4
    for k, note in enumerate(chords[c]):
        add(t0 + k * 0.012, keys(note, 4 * BEAT), pan=(-0.4 + 0.27 * k))
        add(t0 + 2.5 * BEAT + k * 0.012, keys(note + 12, 1.4 * BEAT, 0.05), pan=(0.4 - 0.27 * k))
    add(t0, bass(roots[c], 1.8 * BEAT))
    add(t0 + 2.5 * BEAT, bass(roots[c], 1.4 * BEAT))

    if bar >= 1:
        for b in range(4):
            if b in (0, 2):
                add(t0 + b * BEAT, kick_s)
            if b in (1, 3):
                add(t0 + b * BEAT, snare_s, pan=0.05)
            for h in range(2):
                swing = 0.0 if h == 0 else BEAT * 0.58
                add(t0 + b * BEAT + swing, hat(0.07 if h == 0 else 0.045), pan=0.35)

    if bar >= 4:
        for step in range(4):
            add(t0 + step * BEAT, melody_note(melody[c * 4 + step], BEAT * 0.95), pan=-0.1)

peak = max(max(abs(x) for x in left), max(abs(x) for x in right))
gain = 0.7 / peak
fade = int(1.5 * SR)

with wave.open("demo/demo_track.wav", "wb") as w:
    w.setnchannels(2)
    w.setsampwidth(2)
    w.setframerate(SR)
    frames = bytearray()
    for i in range(LENGTH):
        f = min(1.0, (LENGTH - i) / fade)
        frames += struct.pack("<hh", int(left[i] * gain * f * 32767), int(right[i] * gain * f * 32767))
    w.writeframes(bytes(frames))

print(f"wrote demo/demo_track.wav ({LENGTH / SR:.1f} s)")
