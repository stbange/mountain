"""Resample the USGS 3DEP n44w111 one-arcsecond tile around Taggart Lake."""

import argparse
from array import array
from collections import deque
from math import cos, floor, radians
from pathlib import Path
import struct
import sys

from PIL import Image

SOURCE = "https://prd-tnm.s3.amazonaws.com/StagedProducts/Elevation/1/TIFF/current/n44w111/USGS_1_n44w111.tif"
# The playable origin is on the eastern moraine. Grid x points east, z south.
LAT, LON = 43.70345, -110.74825
SIDE, SPACING, LAKE = 513, 40, 2101.36
parser = argparse.ArgumentParser(description=__doc__, epilog="Input GeoTIFF: " + SOURCE)
parser.add_argument("geotiff", type=Path)
parser.add_argument("output", type=Path)
args = parser.parse_args()
im = Image.open(args.geotiff)
pixels = im.load()
sx, sy, _ = im.tag_v2[33550]
_, _, _, west, north, _ = im.tag_v2[33922]
dx, dy = SPACING / (111450 * cos(radians(LAT))), SPACING / 111110
heights = array("H")
for j in range(SIDE):
    fy = (north - (LAT - dy * (j - SIDE // 2))) / sy - 0.5
    y = floor(fy)
    for i in range(SIDE):
        fx = (LON + dx * (i - SIDE // 2) - west) / sx - 0.5
        x = floor(fx)
        a = pixels[x, y] + (fx - x) * (pixels[x + 1, y] - pixels[x, y])
        b = pixels[x, y + 1] + (fx - x) * (pixels[x + 1, y + 1] - pixels[x, y + 1])
        heights.append(round(4 * (a + (fy - y) * (b - a))))

# Mark the contiguous hydroflattened lake. This keeps other valley floors dry.
i = SIDE // 2 + round((-110.755 - LON) / dx)
j = SIDE // 2 + round((LAT - 43.7045) / dy)
queue = deque([j * SIDE + i])
while queue:
    k = queue.popleft()
    if abs(heights[k] / 4 - LAKE) < 0.49:
        heights[k] |= 0x8000
        queue.extend((k - 1, k + 1, k - SIDE, k + SIDE))

if sys.byteorder != "little":
    heights.byteswap()
args.output.write_bytes(
    struct.pack("<4sHH", b"MHT1", SIDE, SPACING) + heights.tobytes()
)
