#!/usr/bin/env python3
"""Draws the atlas of icons the game menu uses and writes it as an OZT.

The boxes of the game menu take their pictures from one atlas of eight by four cells, each of them
32 pixels square, tinted at draw time - so the atlas is white on transparent and carries no colour
of its own.

OZT is a raw 32 bit TGA with four bytes in front of it, which is what `CGlobalBitmap::OpenTga`
reads: it takes the width at offset 16, the height at 18, the depth at 20 and the pixels from 22,
bottom row first, blue green red alpha.

Run it from the repository root:

    python3 tools/make_menu_icons.py
"""

import math
import os
import struct

CELL = 32
COLUMNS = 8
ROWS = 4
WIDTH = CELL * COLUMNS
HEIGHT = CELL * ROWS
SUPERSAMPLE = 4
STROKE = 2.2

OUTPUT = os.path.join("src", "bin", "Data", "Interface", "newui_menu_icons.OZT")


def arc(cx, cy, r, start_deg, end_deg, steps=24):
    """A polyline along a circle, so an arc needs no primitive of its own."""
    points = []
    for i in range(steps + 1):
        angle = math.radians(start_deg + (end_deg - start_deg) * i / steps)
        points.append((cx + r * math.cos(angle), cy + r * math.sin(angle)))
    return points


def rect(x, y, w, h):
    return [(x, y), (x + w, y), (x + w, y + h), (x, y + h), (x, y)]


def circle(cx, cy, r):
    return arc(cx, cy, r, 0, 360)


def segments_of(polyline):
    return list(zip(polyline[:-1], polyline[1:]))


# Every icon is a list of polylines drawn in a 32 by 32 box, y pointing down.
ICONS = [
    # 0 bank
    [[(4, 13), (16, 6), (28, 13)], [(7, 14), (7, 23)], [(13, 14), (13, 23)], [(19, 14), (19, 23)],
     [(25, 14), (25, 23)], [(4, 26), (28, 26)]],
    # 1 market
    [[(6, 12), (25, 12)], [(21, 8), (25, 12), (21, 16)], [(26, 20), (7, 20)], [(11, 16), (7, 20), (11, 24)]],
    # 2 move map
    [[(5, 9), (12, 6), (20, 9), (27, 6), (27, 23), (20, 26), (12, 23), (5, 26), (5, 9)],
     [(12, 6), (12, 23)], [(20, 9), (20, 26)]],
    # 3 quests
    [rect(8, 6, 16, 18), [(12, 11), (21, 11)], [(12, 15), (21, 15)], [(12, 19), (18, 19)]],
    # 4 helper
    [rect(8, 11, 16, 12), [(16, 6), (16, 11)], circle(12.5, 16, 1.1), circle(19.5, 16, 1.1),
     [(13, 20), (19, 20)]],
    # 5 friends
    [circle(13, 11, 4.5), [(6, 25), (6, 21), (20, 21), (20, 25)], circle(23, 12, 3),
     [(22, 25), (26, 25), (26, 22)]],
    # 6 guild
    [[(16, 5), (27, 9), (27, 16), (16, 27), (5, 16), (5, 9), (16, 5)]],
    # 7 mini map
    [circle(16, 16, 11), [(20, 12), (18, 18), (12, 20), (14, 14), (20, 12)]],
    # 8 character
    [circle(16, 11, 5), [(7, 26), (8, 21), (24, 21), (25, 26)]],
    # 9 inventory
    [[(7, 11), (25, 11), (23, 26), (9, 26), (7, 11)], [(12, 11), (12, 8), (20, 8), (20, 11)]],
    # 10 master level
    [[(8, 16), (16, 8), (24, 16)], [(8, 24), (16, 16), (24, 24)]],
    # 11 pet
    [circle(10, 12, 2.6), circle(15.5, 9, 2.6), circle(21, 10, 2.6), circle(25, 15, 2.6),
     circle(16, 21, 5.2)],
    # 12 party
    [circle(11, 12, 4), circle(21, 12, 4), [(5, 25), (5, 20), (16, 20)], [(16, 20), (27, 20), (27, 25)]],
    # 13 gens
    [[(9, 5), (9, 27)], [(9, 7), (24, 7), (20, 12), (24, 17), (9, 17)]],
    # 14 options
    [circle(16, 16, 5)] + [
        [(16 + 8 * math.cos(math.radians(a)), 16 + 8 * math.sin(math.radians(a))),
         (16 + 11.5 * math.cos(math.radians(a)), 16 + 11.5 * math.sin(math.radians(a)))]
        for a in range(0, 360, 45)
    ],
    # 15 help
    [circle(16, 16, 11), [(12.5, 12.5), (14, 10), (18, 10), (19.5, 12.5), (19, 15), (16, 16.5), (16, 19)],
     circle(16, 22.5, 0.9)],
    # 16 commands
    [rect(4, 10, 24, 12), circle(8, 14, 0.9), circle(12, 14, 0.9), circle(16, 14, 0.9), circle(20, 14, 0.9),
     circle(24, 14, 0.9), circle(8, 18, 0.9), circle(24, 18, 0.9), [(11, 18), (21, 18)]],
    # 17 exit
    [[(16, 6), (16, 15)], arc(16, 17, 9, -60, 240)],
    # 18 show all
    [rect(5, 5, 10, 10), rect(17, 5, 10, 10), rect(5, 17, 10, 10), rect(17, 17, 10, 10)],
]


def distance_to_segment(px, py, ax, ay, bx, by):
    dx = bx - ax
    dy = by - ay
    length = dx * dx + dy * dy
    if length <= 1e-9:
        return math.hypot(px - ax, py - ay)
    t = ((px - ax) * dx + (py - ay) * dy) / length
    t = max(0.0, min(1.0, t))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))


def render_cell(polylines):
    """Coverage of one cell, supersampled, as a list of rows of floats between 0 and 1."""
    size = CELL * SUPERSAMPLE
    half = STROKE / 2.0
    segments = []
    for polyline in polylines:
        segments.extend(segments_of(polyline))

    # Only the pixels near a segment can be covered, so each segment paints its own bounding box
    # instead of every pixel asking every segment.
    coverage = [[0.0] * size for _ in range(size)]
    reach = half + 1.0
    for (ax, ay), (bx, by) in segments:
        x0 = max(0, int((min(ax, bx) - reach) * SUPERSAMPLE))
        x1 = min(size - 1, int((max(ax, bx) + reach) * SUPERSAMPLE) + 1)
        y0 = max(0, int((min(ay, by) - reach) * SUPERSAMPLE))
        y1 = min(size - 1, int((max(ay, by) + reach) * SUPERSAMPLE) + 1)
        for sy in range(y0, y1 + 1):
            py = (sy + 0.5) / SUPERSAMPLE
            row = coverage[sy]
            for sx in range(x0, x1 + 1):
                px = (sx + 0.5) / SUPERSAMPLE
                if row[sx] >= 1.0:
                    continue
                if distance_to_segment(px, py, ax, ay, bx, by) <= half:
                    row[sx] = 1.0
    return coverage


def main():
    # One byte of alpha per pixel; the colour is white everywhere, because the menu tints it.
    alpha = [[0] * WIDTH for _ in range(HEIGHT)]

    for index, polylines in enumerate(ICONS):
        column = index % COLUMNS
        row = index // COLUMNS
        coverage = render_cell(polylines)
        for y in range(CELL):
            for x in range(CELL):
                total = 0.0
                for sy in range(SUPERSAMPLE):
                    line = coverage[y * SUPERSAMPLE + sy]
                    for sx in range(SUPERSAMPLE):
                        total += line[x * SUPERSAMPLE + sx]
                value = int(round(255.0 * total / (SUPERSAMPLE * SUPERSAMPLE)))
                alpha[row * CELL + y][column * CELL + x] = max(0, min(255, value))

    header = bytearray(b"\x00" * 4)
    header += struct.pack(
        "<BBBHHBHHHHBB",
        0,        # no id field
        0,        # no colour map
        2,        # uncompressed true colour
        0, 0, 0,  # colour map specification
        0, 0,     # origin
        WIDTH,
        HEIGHT,
        32,       # bits per pixel
        8,        # eight bits of alpha
    )

    body = bytearray()
    # Bottom row first, which is what a TGA with its origin at the bottom left holds.
    for y in range(HEIGHT - 1, -1, -1):
        for x in range(WIDTH):
            a = alpha[y][x]
            body += bytes((255, 255, 255, a))

    with open(OUTPUT, "wb") as handle:
        handle.write(bytes(header))
        handle.write(bytes(body))

    print("wrote %s (%d bytes, %dx%d, %d icons)" % (OUTPUT, 22 + len(body), WIDTH, HEIGHT, len(ICONS)))


if __name__ == "__main__":
    main()
