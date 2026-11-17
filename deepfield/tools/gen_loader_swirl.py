#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 Kyle McCann
"""The update screen's vortex (loader_screen.c) as a table: three arms of dots spiralling in from the rim to the eye,
in the order the install lights them, with each dot's size and its place in the colour ramp; the ring round the eye
that closes at the end; the ramps as RGB565. Writes deepfield/loader_swirl.h:
  python3 deepfield/tools/gen_loader_swirl.py"""
import math, pathlib

CX, CY = 120, 112
ARMS, PER = 3, 64
R0, R1, TURNS = 88.0, 34.0, 0.85
RING_R, RING_N = 27.0, 28
STOPS = [(176, 112, 255), (120, 96, 255), (72, 120, 255), (64, 168, 255), (64, 216, 255), (150, 236, 255),
         (232, 252, 255)]                # the tube's violet, through blue and cyan, to white-hot at the eye
SKY = (0, 4, 16)

def rgb565(c):
    return (c[0] >> 3) << 11 | (c[1] >> 2) << 5 | c[2] >> 3

def ramp(u):
    f = u * (len(STOPS) - 1)
    i = min(int(f), len(STOPS) - 2)
    return tuple(round(STOPS[i][k] + (STOPS[i + 1][k] - STOPS[i][k]) * (f - i)) for k in range(3))

def toward(c, d, k):
    return tuple(round(c[i] * k + d[i] * (1 - k)) for i in range(3))

dots = []
for i in range(PER):
    u = i / (PER - 1)
    r = R0 * (R1 / R0) ** u            # a log spiral, as the boot's arms are: wide apart at the rim, wound tight at the eye
    size = round(5 - 3 * u)
    for a in range(ARMS):
        th = 2 * math.pi * (a / ARMS + TURNS * u) - math.pi / 2
        dots.append((round(CX + r * math.cos(th) - size / 2), round(CY + r * math.sin(th) - size / 2), size, i * 16 // PER))
ring = [(round(CX + RING_R * math.cos(2 * math.pi * k / RING_N - math.pi / 2) - 1.5),
         round(CY + RING_R * math.sin(2 * math.pi * k / RING_N - math.pi / 2) - 1.5)) for k in range(RING_N)]
lit = [ramp(k / 15) for k in range(16)]

def row(name, vals, fmt="%3d"):
    out = "static const %s = {\n" % name
    for i in range(0, len(vals), 16):
        out += "    " + ", ".join(fmt % v for v in vals[i:i + 16]) + ",\n"
    return out + "};\n"

h = "/* SPDX-License-Identifier: MPL-2.0\n * Copyright (C) 2026 Kyle McCann */\n"
h += "/* made by tools/gen_loader_swirl.py: the update screen's vortex. Do not edit */\n"
h += "#define SWIRL_N %du\n#define SWIRL_ARMS %du\n#define SWIRL_RING %du\n" % (len(dots), ARMS, RING_N)
h += "#define SWIRL_CX %du\n#define SWIRL_CY %du\n" % (CX, CY)
h += row("uint8_t SWIRL_X[SWIRL_N]", [d[0] for d in dots])
h += row("uint8_t SWIRL_Y[SWIRL_N]", [d[1] for d in dots])
h += row("uint8_t SWIRL_SC[SWIRL_N]", [d[2] << 4 | d[3] for d in dots], "0x%02X")   # size << 4 | colour
h += row("uint8_t SWIRL_RX[SWIRL_RING]", [p[0] for p in ring])
h += row("uint8_t SWIRL_RY[SWIRL_RING]", [p[1] for p in ring])
h += row("uint16_t SWIRL_LIT[16]", [rgb565(c) for c in lit], "0x%04X")
h += row("uint16_t SWIRL_DIM[16]", [rgb565(toward(c, SKY, 0.32)) for c in lit], "0x%04X")
h += row("uint16_t SWIRL_GHOST[16]", [rgb565(toward(c, SKY, 0.2)) for c in lit], "0x%04X")
h += row("uint16_t SWIRL_WAIT[16]", [rgb565(toward(c, SKY, 0.7)) for c in lit], "0x%04X")
pathlib.Path(__file__).resolve().parent.parent.joinpath("loader_swirl.h").write_text(h)
print("loader_swirl.h: %d dots, %d ring" % (len(dots), RING_N))
