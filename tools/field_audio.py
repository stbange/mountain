"""Rebuild Mountain's original expedition ambience and pitched field instruments."""

import math
import random
import struct
import wave
from pathlib import Path

root = Path(__file__).resolve().parents[1] / "assets" / "sound"
rate = 16000


def write(name, samples):
    with wave.open(str(root / (name + ".wav")), "wb") as sound:
        sound.setparams((1, 2, rate, 0, "NONE", "not compressed"))
        sound.writeframes(
            b"".join(
                struct.pack("<h", int(max(-1, min(1, s)) * 32767)) for s in samples
            )
        )


def ambience(kind):
    rng = random.Random(597)
    noise = 0
    for i in range(6 * rate):
        t = i / rate
        noise = 0.945 * noise + 0.055 * rng.uniform(-1, 1)
        fade = min(1, t * 3, (6 - t) * 3)
        sea = 0.5 + 0.5 * math.cos(2 * math.pi * t / 6)
        if kind == "surf":
            yield fade * noise * (0.28 + 1.8 * sea**2)
        elif kind == "wind":
            yield fade * (
                0.9 * noise + 0.011 * math.sin(2 * math.pi * (167 * t + 0.2 * sea))
            )
        else:
            yield fade * (
                0.024 * math.sin(2 * math.pi * 55 * t)
                + 0.011 * math.sin(2 * math.pi * 82.5 * t)
                + 0.08 * noise
            )


for kind in ("surf", "wind", "resonance"):
    write(kind, ambience(kind))
for n, pitch in enumerate((261.626, 329.628, 391.995)):
    write(
        "chime-" + str(n),
        (
            min(1, i / (rate * 0.015))
            * math.exp(-4 * i / rate)
            * (
                0.22 * math.sin(2 * math.pi * pitch * i / rate)
                + 0.09 * math.sin(2 * math.pi * pitch * 2.01 * i / rate)
            )
            for i in range(rate)
        ),
    )


def thunder():
    rng = random.Random(2871)
    low = 0.0
    for i in range(4 * rate):
        t = i / rate
        low = 0.988 * low + 0.012 * rng.uniform(-1, 1)
        roll = math.sin(2 * math.pi * (41 * t - 0.85 * t * t))
        echo = 0.58 + 0.42 * math.cos(t * 6.1) ** 2
        yield min(1, t * 3) * (1 - t / 4) ** 2 * echo * (low * 1.2 + roll * 0.09)


write("thunder", thunder())
