#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 Kyle McCann
"""Render every factory scene of DEEP FIELD to WAV in the host simulator, for auditioning on the Mac and as a
regression check: the simulator is deterministic, so the renders' SHA-256 hashes only change when the sound does.
Each scene lands with its own chord (nothing latched, as PRESET brings a scene's chord in; the simulator's "land"
does it on the millisecond SEL's release once did, so the renders keep their timing) and plays it for 14 s (MASTER
full), then every body is let go and the tail runs 6 s.
Run from the repository after ./build.sh deepfield --host.
  python3 deepfield/tools/render_scenes.py              renders into deepfield/build/renders/ and compares with
                                                        deepfield/tests/golden.txt
  python3 deepfield/tools/render_scenes.py --golden     renders and rewrites the golden file
  python3 deepfield/tools/render_scenes.py --levels     and prints each render's peak and RMS per 2 s"""
import array, hashlib, math, pathlib, subprocess, sys, wave
ROOT = pathlib.Path(__file__).resolve().parents[2]
HOST = ROOT / "deepfield/build/host/deepfield_host"
OUT = ROOT / "deepfield/build/renders"
GOLDEN = ROOT / "deepfield/tests/golden.txt"
SCENES = ["FIRST LIGHT", "VOID TIDE", "GLASS ORBIT", "BOWED SKY", "PULSAR", "NEBULA", "DEEP VOID", "STARLIGHT",
          "CATHEDRAL", "LOW CHOIR", "GAMELAN", "ICE BELLS", "ASCENT", "DESCENT", "FOLDED SUN", "TIDE POOL"]

def script():
    s = ["wait 300", "master 4096", "turn SELECT -1", "wait 200"]
    for i, name in enumerate(SCENES):
        fn = f"{i + 1:02d}-{name.replace(' ', '_')}.wav"
        s += ["wait 60", f"land {i}", "wait 300", f"wav {fn}", "wait 14000", "midi B0 7B 00", "wait 6000", "wavstop",
              "wait 200"]
    return "\n".join(s) + "\n"

def levels(path):
    with wave.open(str(path)) as w:
        n, rate, ch, sw = w.getnframes(), w.getframerate(), w.getnchannels(), w.getsampwidth()
        raw = w.readframes(n)
    a = array.array("h" if sw == 2 else "i", raw)
    if sw == 4:
        a = array.array("h", (x >> 16 for x in a))
    step = rate * ch * 2
    out = []
    for i in range(0, len(a), step):
        seg = a[i:i + step]
        if not seg:
            break
        pk = max(abs(x) for x in seg) / 32768
        rms = math.sqrt(sum(x * x for x in seg) / len(seg)) / 32768
        out.append((20 * math.log10(pk) if pk else -99, 20 * math.log10(rms) if rms else -99))
    return out

def main():
    golden, show = "--golden" in sys.argv, "--levels" in sys.argv
    OUT.mkdir(parents=True, exist_ok=True)
    for f in OUT.glob("*.wav"):
        f.unlink()
    (OUT / "render.txt").write_text(script())
    r = subprocess.run([str(HOST), str(OUT / "render.txt"), str(OUT)], capture_output=True, text=True)
    if r.returncode:
        sys.exit(r.stdout[-2000:] + r.stderr[-2000:])
    hashes = {f.name: hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(OUT.glob("*.wav"))}
    for name, h in hashes.items():
        line = f"{h[:12]}  {name}"
        if show:
            line += "  " + " ".join(f"{pk:5.1f}/{rms:5.1f}" for pk, rms in levels(OUT / name))
        print(line)
    if golden:
        GOLDEN.write_text("".join(f"{h}  {n}\n" for n, h in hashes.items()))
        print(f"golden: {len(hashes)} renders written to {GOLDEN.relative_to(ROOT)}")
        return
    if not GOLDEN.exists():
        sys.exit("no golden file yet: run with --golden")
    old = dict(line.split()[::-1] for line in GOLDEN.read_text().splitlines() if line.strip())
    changed = [n for n, h in hashes.items() if old.get(n) != h]
    missing = [n for n in old if n not in hashes]
    if changed or missing:
        print("CHANGED:", ", ".join(changed + [m + " (missing)" for m in missing]))
        sys.exit(1)
    print(f"all {len(hashes)} renders match golden")

if __name__ == "__main__":
    main()
