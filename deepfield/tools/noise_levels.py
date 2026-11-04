#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 Kyle McCann
"""How loud the noise layer is in its settings: renders F1 FIRST LIGHT in the host simulator with the layer on
(LEVEL 80 over CC 20) and again with it off (FX held, C4), both with the same keys, and prints the layer alone
(the difference) in dB RMS: hiss, rain and crackle (TEXTURE), the one band and the chord's breath (BAND), a gust
(GUST at WIND full), and the bed ducked under a chord (DUCK). The bodies are the scene's.
  python3 deepfield/tools/noise_levels.py          (after ./build.sh deepfield --host)"""
import subprocess, wave, array, math, pathlib
ROOT = pathlib.Path(__file__).resolve().parents[2]
S = ROOT / "deepfield/build/nzlevels"; S.mkdir(parents=True, exist_ok=True)
HOST = str(ROOT / "deepfield/build/host/deepfield_host")
def run(name, setup, secs):
    script = "wait 300\nmaster 4096\nmidi C0 00\nwait 400\n" + setup + f"wait 1500\nwav out.wav\nwait {secs}\nwavstop\n"
    d = S / name; d.mkdir(exist_ok=True); (d / "s.txt").write_text(script)
    subprocess.run([HOST, str(d / "s.txt"), str(d)], capture_output=True, text=True)
    with wave.open(str(d / "out.wav")) as w:
        a = array.array("h" if w.getsampwidth() == 2 else "i", w.readframes(w.getnframes()))
    if a.itemsize == 4: a = array.array("h", (x >> 16 for x in a))
    return a
def rms(a): return math.sqrt(sum(x * x for x in a) / len(a)) / 32768 if len(a) else 0
def db(x): return 20 * math.log10(x) if x > 0 else -99
def diff(a, b): return array.array("h", (max(-32768, min(32767, x - y)) for x, y in zip(a, b)))
NOISE_ON = "midi B0 14 50\n"
OFF = "press FX\nwait 100\nkey 7 down\nwait 40\nkey 7 up\nwait 40\nrelease FX\nwait 200\n"
NOBYP = "press FX\nwait 100\nkey 26 down\nwait 40\nkey 26 up\nwait 40\nrelease FX\nwait 200\n"
def page(knobs, second=False):          # EDIT, SELECT (and again) to NOISE (2); knob spins; HOME
    s = "tap EDIT\nwait 30\nspin SELECT 1\nwait 30\n" + ("spin SELECT 1\nwait 30\n" if second else "")
    for k, v in knobs: s += f"spin {k} {v}\nwait 30\n"
    return s + "tap HOME\nwait 30\n"
cases = [
 ("hiss (TEXTURE 0), BAND 30, COLOUR 50", ""),
 ("rain (TEXTURE 50)", page([("K1", 64)], True)),
 ("crackle (TEXTURE 100)", page([("K1", 127)], True)),
 ("COLOUR 0 (brown, rumble)", page([("K2", -50)])),
 ("COLOUR 100 (white)", page([("K2", 77)])),
 ("BAND 50 (one band, narrow)", page([("K4", 34)])),
 ("BAND 100 (the chord's breath)", page([("K4", 97)])),
 ("GUST 100, WIND 100 (8 s)", page([("K3", 87)]) + page([("K2", 127)], True)),
 ("DUCK 100 under the chord", page([("K3", 97)], True)),
]
for label, setup in cases:
    secs = 8000 if "GUST" in label else 4000
    on = run("on", NOISE_ON + setup + NOBYP, secs)
    off = run("off", NOISE_ON + setup + OFF, secs)
    wet = diff(on, off)
    line = f"{label}: layer {db(rms(wet)):.1f} dB RMS, bodies {db(rms(off)):.1f}"
    if "GUST" in label:                  # per second, to see the gust come and go
        n = 44118 * 2
        line += "  by second: " + " ".join(f"{db(rms(wet[i:i + n])):.0f}" for i in range(0, len(wet), n))
    print(line)
