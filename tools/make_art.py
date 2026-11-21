"""Rebuild Mountain's original pixel sprites and loopable field recording."""

from pathlib import Path
from math import sin, cos, pi
import random, struct, wave
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1] / "assets"
SIZE = 128
atlas = Image.new("RGBA", (SIZE * 4, SIZE * 4))


def mix(a, b, t):
    return tuple(int(max(0, min(255, x + (y - x) * t))) for x, y in zip(a, b))


def tile(index, painter):
    im = Image.new("RGBA", (SIZE, SIZE))
    painter(ImageDraw.Draw(im), random.Random(4100 + index))
    atlas.paste(im, ((index % 4) * SIZE, (index // 4) * SIZE))


def conifer(d, r, kind):
    sparse = kind == 2
    snowy = kind == 10
    dark = (29, 48, 43) if not sparse else (52, 64, 50)
    mid = (55, 84, 63) if not sparse else (99, 111, 69)
    tip = (98, 126, 77) if not snowy else (199, 213, 197)
    if sparse:
        tip = (159, 155, 81)
    x = 64 + r.randint(-4, 4)
    d.polygon([(x - 1, 4), (x + 2, 5), (68, 124), (60, 124)], fill=(77, 62, 48))
    d.line([(x, 20), (64, 119)], fill=(128, 99, 67), width=1)
    # Build each tier from hanging needle sprays on individual branchlets.
    layers = 36 if sparse else 48
    for j in range(layers):
        f = j / (layers - 1)
        y = 5 + f * (107 if sparse else 114)
        center = x + (64 - x) * f + sin(j * 0.37)
        radius = (4 + f**0.76 * (40 if kind == 1 else 48)) * (0.74 + 0.26 * r.random())
        for side in [-1, 1]:
            reach = radius * (0.74 + 0.30 * r.random())
            drop = 4 + f * 8 + r.uniform(-2, 3)
            endy = y + drop - r.uniform(1, 6)
            d.line(
                [
                    (center, y),
                    (center + side * reach * 0.6, y + drop),
                    (center + side * reach, endy),
                ],
                fill=mix(dark, mid, 0.28),
                width=2 if f > 0.45 else 1,
            )
            for k in range(int(reach * 1.55)):
                u = r.random()
                bx = center + side * (reach * u)
                by = y + sin(u * 1.95) * drop
                length = (
                    (2 + f * (4 if sparse else 8))
                    * (1 - 0.55 * u)
                    * r.uniform(0.4, 1.5)
                )
                light = 0.28 + 0.23 * r.random() + 0.35 * (side == -1) + 0.15 * u
                col = mix(dark, mid, light)
                if r.random() < (0.25 if side == -1 else 0.1):
                    col = mix(mid, tip, r.uniform(0.22, 0.9))
                d.line(
                    [(bx, by - 1), (bx + side * length * 0.85, by + length)],
                    fill=col,
                    width=r.choice([1, 1, 2]),
                )
        if snowy:
            for k in range(int(radius * 0.55)):
                xx = r.uniform(-radius * 0.8, radius * 0.7)
                d.line(
                    [
                        (center + xx, y + abs(xx) * 0.15),
                        (center + xx + 2, y + abs(xx) * 0.15 + 1),
                    ],
                    fill=mix(mid, tip, r.uniform(0.5, 1)),
                    width=2,
                )
    d.line([(x, 1), (x + 1, 9)], fill=mid, width=1)


def broadleaf(d, r, autumn=False):
    bark = (163, 154, 124)
    shadow = (61, 67, 51)
    d.polygon([(59, 125), (66, 125), (69, 44), (65, 41)], fill=bark)
    for j in range(20):
        y = r.randint(43, 121)
        d.line([(62, y), (r.randint(64, 67), y + 1)], fill=shadow)
    for j in range(62):
        a = r.uniform(0, 2 * pi)
        rad = r.random() ** 0.53
        x = 64 + cos(a) * 44 * rad
        y = 46 + sin(a) * 43 * rad
        d.line([(65, 82), (65 + (x - 65) * 0.33, y + 13), (x, y)], fill=shadow, width=2)
        light = 0.28 + (1 - x / 125) * 0.5 + r.random() * 0.22
        lo, hi = (
            ((102, 77, 45), (208, 152, 71))
            if autumn
            else ((54, 82, 53), (151, 165, 90))
        )
        for k in range(55):
            px = x + r.gauss(0, 4.6)
            py = y + r.gauss(0, 4.0)
            col = mix(lo, hi, max(0, min(1, light + r.uniform(-0.42, 0.32))))
            d.rectangle((px, py, px + r.randint(1, 4), py + r.randint(1, 3)), fill=col)


def flowers(d, r, kind):
    for j in range(36):
        x = r.uniform(15, 114)
        y = r.uniform(98, 126)
        h = r.uniform(11, 52)
        bend = r.uniform(-11, 10)
        end = (x + bend, y - h)
        green = r.choice([(66, 88, 45), (91, 116, 54), (121, 139, 67), (74, 103, 68)])
        d.line([(x, y), (x + bend * 0.28, y - h * 0.45), end], fill=green, width=2)
        for side in (-1, 1):
            yy = y - r.uniform(3, h * 0.65)
            xx = x + (y - yy) * bend / h
            d.polygon(
                [
                    (xx, yy + 1),
                    (xx + side * 8, yy - 7),
                    (xx + side * 4, yy),
                    (xx, yy + 3),
                ],
                fill=green,
            )
        if kind == 5:
            continue
        if kind == 8:
            for ax, ay in [(-2, 0), (2, 0), (0, -2), (0, 2)]:
                d.ellipse(
                    (
                        end[0] + ax - 1,
                        end[1] + ay - 1,
                        end[0] + ax + 1,
                        end[1] + ay + 1,
                    ),
                    fill=(240, 226, 190),
                )
            d.point(end, fill=(224, 153, 53))
        else:
            pal = (
                [(121, 79, 129), (163, 111, 173), (192, 145, 191), (218, 179, 205)]
                if kind == 4
                else [(156, 84, 78), (202, 113, 99), (231, 151, 122)]
            )
            for k in range(8):
                yy = end[1] + k * 1.6
                xx = end[0] + sin(k * 2.4) * k * 0.37
                d.rectangle((xx - 1, yy, xx + 1 + k * 0.12, yy + 2), fill=r.choice(pal))


def fern(d, r):
    for j in range(22):
        x = r.uniform(36, 93)
        dx = r.uniform(-38, 38)
        h = r.uniform(24, 68)
        for k in range(12):
            t = k / 12
            px = x + dx * t
            py = 121 - h * sin(t * 1.4)
            col = r.choice([(64, 93, 62), (101, 127, 72), (119, 138, 78)])
            d.line([(px, py), (px + dx * 0.09, py - h * 0.045)], fill=col, width=2)
            for s in (-1, 1):
                d.line([(px, py), (px + s * (1 - t) * 9, py - 4)], fill=col, width=2)


def doe(d, r):
    ink = (88, 65, 50)
    fur = (157, 119, 75)
    sun = (191, 148, 95)
    for x, bend in [(34, -4), (42, 5), (80, -1), (88, 5)]:
        d.polygon(
            [
                (x, 77),
                (x + 5, 77),
                (x + bend + 3, 102),
                (x + bend - 1, 122),
                (x + bend - 4, 122),
                (x + bend - 1, 101),
            ],
            fill=ink if bend < 0 else fur,
        )
    d.polygon(
        [
            (21, 64),
            (32, 58),
            (48, 57),
            (70, 60),
            (83, 53),
            (90, 37),
            (97, 38),
            (93, 64),
            (87, 76),
            (69, 84),
            (43, 84),
            (28, 80),
        ],
        fill=fur,
    )
    d.polygon(
        [
            (22, 65),
            (30, 58),
            (56, 58),
            (72, 60),
            (81, 55),
            (72, 65),
            (44, 64),
            (29, 69),
        ],
        fill=sun,
    )
    d.polygon(
        [
            (80, 65),
            (89, 40),
            (96, 32),
            (108, 34),
            (116, 39),
            (113, 43),
            (102, 42),
            (95, 55),
            (90, 67),
        ],
        fill=fur,
    )
    d.polygon([(92, 35), (85, 22), (89, 22), (98, 31)], fill=ink)
    d.polygon([(97, 32), (101, 19), (104, 20), (102, 34)], fill=sun)
    d.polygon([(25, 64), (14, 59), (17, 66), (26, 71)], fill=(219, 201, 167))
    d.rectangle((113, 38, 116, 40), fill=ink)
    d.point((102, 35), fill=(41, 39, 31))


def alpine(d, r, kind):
    # Nodding glacier lilies, paintbrush bracts and dry autumn seed heads.
    for _ in range(13 if kind == 12 else 19):
        x, y = r.uniform(12, 116), r.uniform(110, 126)
        h, bend = r.uniform(35, 81), r.uniform(-13, 13)
        tip, top = x + bend, y - h
        stem = r.choice([(72, 102, 62), (89, 113, 55), (128, 139, 75)])
        if kind == 14:
            stem = (143, 124, 72)
        d.line([(x, y), (x + bend * 0.4, y - h * 0.55), (tip, top)], fill=stem, width=2)
        for side in (-1, 1):
            d.polygon(
                [
                    (x, y - 2),
                    (x + side * 12, y - h * 0.44),
                    (x + side * 5, y - h * 0.37),
                ],
                fill=stem,
            )
        if kind == 12:
            d.arc((tip - 5, top - 4, tip + 3, top + 5), 185, 357, fill=stem, width=2)
            for off in [-4, 0, 4]:
                d.polygon(
                    [
                        (tip - 4, top),
                        (tip - 4 + off, top - 7),
                        (tip - 3 + off * 1.4, top - 4),
                        (tip - 4, top + 5),
                    ],
                    fill=r.choice([(255, 218, 73), (251, 193, 43), (246, 218, 118)]),
                )
            d.line((tip - 4, top + 2, tip - 4, top + 7), fill=(184, 111, 34), width=2)
        elif kind == 13:
            for k in range(8):
                yy, side = top + k * 2.7, (-1) ** k
                d.polygon(
                    [
                        (tip, yy + 3),
                        (tip + side * (3 + k * 0.6), yy - 6),
                        (tip + side * 2, yy - 5),
                    ],
                    fill=r.choice(
                        [(198, 65, 47), (226, 87, 53), (248, 126, 71), (178, 62, 48)]
                    ),
                )
        else:
            for k in range(12):
                a, rad = r.uniform(0, 2 * pi), r.uniform(2, 7)
                xx, yy = tip + cos(a) * rad, top + sin(a) * rad
                d.line((tip, top, xx, yy), fill=(177, 153, 100))
                d.ellipse(
                    (xx - 1, yy - 1, xx + 1, yy + 1),
                    fill=r.choice([(210, 185, 131), (155, 126, 83)]),
                )


def butterfly_wing(d, r):
    # One articulated wing, with veins and a pale spotted outer margin.
    outline = [
        (4, 63),
        (26, 32),
        (79, 7),
        (114, 3),
        (124, 15),
        (114, 44),
        (91, 63),
        (105, 75),
        (97, 96),
        (72, 117),
        (48, 123),
        (23, 107),
        (8, 77),
    ]
    d.polygon(outline, fill=(63, 48, 37))
    d.polygon(
        [(9, 62), (32, 33), (80, 12), (109, 10), (103, 34), (77, 56), (34, 70)],
        fill=(235, 161, 73),
    )
    d.polygon(
        [(12, 72), (43, 65), (75, 61), (95, 75), (85, 95), (57, 113), (31, 105)],
        fill=(223, 139, 62),
    )
    for tip in [(95, 16), (88, 42), (72, 56), (84, 78), (67, 104), (44, 108)]:
        d.line(
            [(10, 69), ((tip[0] + 14) * 0.5, (tip[1] + 67) * 0.5), tip],
            fill=(124, 83, 48),
            width=2,
        )
    for x, y in [
        (114, 14),
        (109, 26),
        (102, 39),
        (89, 52),
        (94, 80),
        (84, 97),
        (71, 109),
        (51, 117),
    ]:
        d.ellipse((x - 3, y - 2, x + 3, y + 2), fill=(255, 239, 190))


for n in [0, 1, 2, 10]:
    tile(n, lambda d, r, n=n: conifer(d, r, n))
tile(3, broadleaf)
tile(9, lambda d, r: broadleaf(d, r, True))
for n in [4, 5, 8, 11]:
    tile(n, lambda d, r, n=n: flowers(d, r, n))
tile(6, fern)
tile(7, doe)
for n in [12, 13, 14]:
    tile(n, lambda d, r, n=n: alpine(d, r, n))
tile(15, butterfly_wing)
(ROOT / "art").mkdir(exist_ok=True)


def tga(im, path):
    header = struct.pack(
        "<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, im.width, im.height, 32, 0x28
    )
    path.write_bytes(header + im.tobytes("raw", "BGRA"))


tga(atlas, ROOT / "art" / "field.tga")


def sound():
    import numpy as np

    rate = 16000
    duration = 12
    n = rate * duration
    rng = np.random.default_rng(31415)
    t = np.arange(n) / rate
    # Circular filtering makes the wind meet itself at the loop boundary.
    noise = rng.normal(size=n)
    freq = np.fft.rfftfreq(n, 1 / rate)
    wind = np.fft.irfft(np.fft.rfft(noise) / (1 + (freq / 280) ** 1.7))
    track = wind * (0.21 + 0.05 * np.sin(2 * pi * t / duration))
    for start, f0, span in [
        (1.4, 2160, 0.19),
        (1.70, 2510, 0.11),
        (5.8, 1770, 0.26),
        (6.13, 2050, 0.18),
        (9.7, 2450, 0.15),
    ]:
        q = np.maximum(0, t - start)
        env = np.where(q < span, np.sin(pi * np.minimum(q / span, 1)) ** 3, 0)
        track += (
            0.055 * env * np.sin(2 * pi * (f0 * q + 510 * q * q + 5 * np.sin(34 * q)))
        )
    (ROOT / "sound").mkdir(exist_ok=True)
    with wave.open(str(ROOT / "sound" / "ridge.wav"), "wb") as f:
        f.setparams((1, 2, rate, 0, "NONE", "not compressed"))
        f.writeframes((track * 32767).astype("<i2").tobytes())


def cloud_deck():
    """A tileable density field shared by the visible clouds and their ground shadows."""
    import numpy as np

    side, period = 512, 16
    y, x = (np.indices((side, side), dtype=np.float32) + 0.5) * (period / side)
    density = np.zeros((side, side), dtype=np.float32)
    for freq, shift, weight in [
        (1, 0, 0.55),
        (2, 7, 0.27),
        (4, -4, 0.12),
        (8, 0, 0.06),
    ]:
        n = freq * period
        cy, cx = np.indices((n, n), dtype=np.float32)
        a, b, c = (cx * 0.1031) % 1, (cy * 0.1030) % 1, (cx * 0.0973) % 1
        dot = a * (b + 33.33) + b * (c + 33.33) + c * (a + 33.33)
        grid = ((a + b + 2 * dot) * (c + dot)) % 1
        u, v = (x * freq + shift) % n, (y * freq + shift) % n
        i, j = u.astype(int), v.astype(int)
        u, v = u - i, v - j
        u, v = u * u * (3 - 2 * u), v * v * (3 - 2 * v)
        rows = grid[j, i] * (1 - u) + grid[j, (i + 1) % n] * u
        next_rows = grid[(j + 1) % n, i] * (1 - u) + grid[(j + 1) % n, (i + 1) % n] * u
        density += weight * (rows * (1 - v) + next_rows * v)
    header = struct.pack("<BBB9xHHBB", 0, 0, 3, side, side, 8, 0)
    (ROOT / "art" / "clouds.tga").write_bytes(
        header + (density * 255 + 0.5).astype("uint8").tobytes()
    )


if __name__ == "__main__":
    cloud_deck()
    sound()
