#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 Kyle McCann
"""How loud each effect is: renders F1 FIRST LIGHT in the host simulator with every effect on and again with one
effect switched off (FX held, its key), and prints the wet part (the difference of the two renders) against the
rest, in dB RMS, for a few settings. The two renders get the same key commands so they stay aligned in time.
  python3 deepfield/tools/fx_levels.py          (after ./build.sh deepfield --host)"""
import subprocess, wave, array, math, sys, pathlib
ROOT = pathlib.Path(__file__).resolve().parents[2]
S = ROOT / "deepfield/build/fxlevels"; S.mkdir(parents=True, exist_ok=True)
HOST = str(ROOT / "deepfield/build/host/deepfield_host")
def run(name, setup):
    script = "wait 300\nmaster 4096\nmidi C0 00\nwait 400\n" + setup + "wait 2500\nwav out.wav\nwait 6000\nwavstop\n"
    d = S / name; d.mkdir(exist_ok=True); (d / "s.txt").write_text(script)
    subprocess.run([HOST, str(d / "s.txt"), str(d)], capture_output=True, text=True)
    with wave.open(str(d / "out.wav")) as w:
        a = array.array("h" if w.getsampwidth() == 2 else "i", w.readframes(w.getnframes()))
    if a.itemsize == 4: a = array.array("h", (x >> 16 for x in a))
    return a
def rms(a): return math.sqrt(sum(x * x for x in a) / len(a)) / 32768
def db(x): return 20 * math.log10(x) if x > 0 else -99
def diff(a, b): return array.array("h", (max(-32768, min(32767, x - y)) for x, y in zip(a, b)))
FXK = {"ENS": 0, "DLY": 2, "REV": 4, "SHM": 6}
def bypass(k): return f"press FX\nwait 100\nkey {FXK[k]} down\nwait 40\nkey {FXK[k]} up\nwait 40\nrelease FX\nwait 200\n"
NOBYP = "press FX\nwait 100\nkey 26 down\nwait 40\nkey 26 up\nwait 40\nrelease FX\nwait 200\n"
cases = [
 ("factory F1", ""),
 ("DLY MIX max", "midi B0 5E 7F\n"),
 ("DLY MIX max, DLY FDBK max", "midi B0 5E 7F\ntap FX\nwait 30\nspin SELECT 1\nwait 30\nspin K2 127\nwait 30\ntap HOME\nwait 30\n"),
 ("DLY FDBK 70%", "tap FX\nwait 30\nspin SELECT 1\nwait 30\nspin K2 44\nwait 30\ntap HOME\nwait 30\n"),
 ("ENS on, ENS MIX factory 40", "tap FX\nwait 30\nspin K1 3\nwait 30\ntap HOME\nwait 30\n"),
 ("ENS on, ENS MIX max", "tap FX\nwait 30\nspin K1 3\nwait 30\nmidi B0 5D 7F\ntap HOME\nwait 30\n"),
 ("SPACE max", "midi B0 12 7F\n"),
 ("SPACE 0", "midi B0 12 00\n"),
 ("SHIMMER max", "tap FX\nwait 30\nspin SELECT 2\nwait 30\nspin K3 127\nwait 30\ntap HOME\nwait 30\n"),
]
for label, setup in cases:
    full = run("full", setup + NOBYP)
    print(f"{label}: full {db(rms(full)):.1f} dB RMS")
    for k in ("ENS", "DLY", "REV", "SHM"):
        byp = run("byp", setup + bypass(k))
        wet = diff(full, byp)
        print(f"   {k}: wet-dry difference {db(rms(wet)):.1f} dB RMS, {db(rms(wet)) - db(rms(byp)):+.1f} dB vs the rest")
