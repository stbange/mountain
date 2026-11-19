"""Draw Mountain's field symbols, trail stamp and app icon at four-times scale."""

from PIL import Image, ImageDraw
from pathlib import Path
from math import cos, sin, pi

out = Path(__file__).resolve().parents[1] / "web" / "ui"
icons = [
    "trail",
    "journal",
    "book",
    "sound",
    "mute",
    "expand",
    "map",
    "sun",
    "cloud",
    "rain",
    "mist",
    "moon",
    "chevron",
    "altitude",
    "leaf",
    "camera",
    "cabin",
    "move",
    "look",
    "close",
    "download",
    "shuffle",
    "arrow",
    "check",
    "vr",
    "sliders",
    "door",
    "grid",
    "compass",
    "back",
    "keyboard",
    "shutter",
]
S = 96
sheet = Image.new("RGBA", (len(icons) * S, S))
for i, name in enumerate(icons):
    im = Image.new("RGBA", (S, S))
    d = ImageDraw.Draw(im)
    ink = "white"

    def line(*pts):
        p = [(int(x * 4), int(y * 4)) for x, y in pts]
        d.line(p, fill=ink, width=6, joint="curve")
        for x, y in [p[0], p[-1]]:
            d.ellipse((x - 2, y - 2, x + 2, y + 2), fill=ink)

    def ellipse(x, y, X, Y, fill=None):
        d.ellipse((x * 4, y * 4, X * 4, Y * 4), fill=fill, outline=ink, width=6)

    def rect(x, y, X, Y, r=0):
        d.rounded_rectangle(
            (x * 4, y * 4, X * 4, Y * 4), radius=r * 4, outline=ink, width=6
        )

    if name in ("trail", "altitude"):
        line((2, 19), (9, 6), (13, 13), (16, 9), (22, 19), (2, 19))
        line((7, 10), (9, 12), (11, 10))
    elif name == "journal":
        rect(4, 6, 21, 20, 2)
        line((7, 16), (11, 12), (14, 15), (17, 12), (20, 16))
        ellipse(14, 8, 16, 10, "white")
        line((2, 15), (2, 3), (16, 3))
    elif name == "book":
        line(
            (2, 5),
            (8, 4),
            (12, 6),
            (16, 4),
            (22, 5),
            (22, 19),
            (16, 18),
            (12, 20),
            (8, 18),
            (2, 19),
            (2, 5),
        )
        line((12, 6), (12, 20))
    elif name in ("sound", "mute"):
        line((3, 9), (7, 9), (12, 5), (12, 19), (7, 15), (3, 15), (3, 9))
        if name == "mute":
            line((16, 9), (21, 15))
            line((21, 9), (16, 15))
        else:
            line((16, 8), (18, 11), (18, 13), (16, 16))
            line((19, 5), (22, 9), (22, 15), (19, 19))
    elif name == "expand":
        for x, y, s, t in [
            (4, 4, 1, 1),
            (20, 4, -1, 1),
            (4, 20, 1, -1),
            (20, 20, -1, -1),
        ]:
            line((x + s * 5, y), (x, y), (x, y + t * 5))
    elif name == "map":
        line(
            (2, 6),
            (8, 3),
            (15, 6),
            (22, 3),
            (22, 18),
            (15, 21),
            (8, 18),
            (2, 21),
            (2, 6),
        )
        line((8, 3), (8, 18))
        line((15, 6), (15, 21))
    elif name == "sun":
        ellipse(8, 8, 16, 16)
        for j in range(8):
            a = j * pi / 4
            line(
                (12 + 7 * cos(a), 12 + 7 * sin(a)), (12 + 10 * cos(a), 12 + 10 * sin(a))
            )
    elif name in ("cloud", "rain"):
        line(
            (6, 16),
            (4, 15),
            (3, 12),
            (4, 10),
            (6, 9),
            (7, 6),
            (10, 5),
            (13, 6),
            (15, 9),
            (18, 9),
            (21, 11),
            (21, 14),
            (19, 16),
            (6, 16),
        )
        if name == "rain":
            for x in [7, 12, 17]:
                line((x, 18), (x - 1, 21))
    elif name == "mist":
        for y, k in [(7, 0), (11, 4), (15, 1), (19, 5)]:
            line((2 + k, y), (21 - k / 2, y))
    elif name == "moon":
        line(
            (16, 3),
            (13, 4),
            (10, 7),
            (9, 10),
            (10, 14),
            (13, 17),
            (17, 18),
            (21, 17),
            (18, 20),
            (14, 21),
            (9, 20),
            (5, 17),
            (3, 12),
            (4, 8),
            (7, 4),
            (11, 3),
            (16, 3),
        )
    elif name == "chevron":
        line((6, 9), (12, 15), (18, 9))
    elif name == "leaf":
        line((4, 20), (15, 9))
        line(
            (6, 17),
            (5, 11),
            (8, 7),
            (13, 5),
            (21, 3),
            (19, 12),
            (16, 17),
            (11, 19),
            (6, 17),
        )
    elif name == "camera":
        rect(2, 7, 22, 20, 2)
        line((7, 7), (9, 4), (15, 4), (17, 7))
        ellipse(8, 9, 16, 17)
    elif name == "cabin":
        line((2, 11), (12, 3), (22, 11))
        line((4, 10), (4, 21), (20, 21), (20, 10))
        rect(10, 14, 14, 21)
        line((16, 6), (16, 3), (19, 3), (19, 8))
    elif name == "move":
        line((3, 17), (9, 11), (12, 14), (19, 6))
        line((14, 6), (20, 5), (20, 11))
    elif name == "look":
        rect(6, 2, 18, 22, 6)
        line((12, 3), (12, 9))
    elif name == "close":
        line((6, 6), (18, 18))
        line((18, 6), (6, 18))
    elif name == "download":
        line((12, 3), (12, 15))
        line((7, 10), (12, 15), (17, 10))
        line((4, 15), (4, 20), (20, 20), (20, 15))
    elif name == "shuffle":
        line((3, 7), (7, 7), (16, 17), (21, 17))
        line((17, 13), (21, 17), (17, 21))
        line((3, 17), (7, 17), (10, 14))
        line((14, 9), (16, 7), (21, 7))
        line((17, 3), (21, 7), (17, 11))
    elif name == "arrow":
        line((3, 12), (20, 12))
        line((14, 6), (20, 12), (14, 18))
    elif name == "back":
        line((21, 12), (4, 12))
        line((10, 6), (4, 12), (10, 18))
    elif name == "check":
        line((4, 12), (9, 17), (20, 6))
    elif name == "vr":
        line((2, 7), (22, 7), (22, 17), (15, 18), (12, 14), (9, 18), (2, 17), (2, 7))
        ellipse(5, 10, 8, 13)
        ellipse(16, 10, 19, 13)
    elif name == "sliders":
        for y, x in [(5, 8), (12, 16), (19, 10)]:
            line((3, y), (x - 2, y))
            rect(x - 2, y - 2, x + 2, y + 2, 1)
            line((x + 2, y), (21, y))
    elif name == "door":
        line((4, 21), (4, 3), (19, 3), (19, 21))
        line((6, 21), (6, 5), (15, 7), (15, 21), (6, 21))
        ellipse(12, 12, 13, 13, "white")
    elif name == "grid":
        rect(3, 3, 21, 21, 1)
        line((9, 3), (9, 21))
        line((15, 3), (15, 21))
        line((3, 9), (21, 9))
        line((3, 15), (21, 15))
    elif name == "compass":
        ellipse(2, 2, 22, 22)
        line((16, 7), (14, 14), (8, 17), (10, 10), (16, 7))
    elif name == "keyboard":
        rect(2, 5, 22, 19, 2)
        line((7, 15), (17, 15))
    elif name == "shutter":
        ellipse(2, 2, 22, 22)
        ellipse(5, 5, 19, 19)
    sheet.paste(im, (i * S, 0))
sheet.save(out / "symbols.png", optimize=True)
(out / "symbols.css").write_text(
    "\n".join(".i-" + n + "{--i:" + str(i) + "}" for i, n in enumerate(icons)) + "\n"
)
im = Image.new("RGBA", (128, 128))
d = ImageDraw.Draw(im)
d.rounded_rectangle((3, 3, 124, 124), radius=30, fill="#263f33")
d.ellipse((78, 25, 96, 43), fill="#edb06c")
d.line(
    [(15, 89), (51, 40), (74, 71), (86, 55), (113, 91)],
    fill="#eeecd6",
    width=6,
    joint="curve",
)
d.line([(35, 66), (46, 65), (52, 55), (58, 62), (64, 61)], fill="#eeecd6", width=3)
d.line([(23, 99), (52, 90), (70, 92), (99, 84)], fill="#a1ac82", width=3)
im.save(out / "mark.png")
