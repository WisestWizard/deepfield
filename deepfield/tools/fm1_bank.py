#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 Kyle McCann
"""DEEP FIELD's presets over USB MIDI, as SysEx (the manual's section 10). Needs mido (/usr/bin/python3 has it).
  fm1_bank.py dump FILE.syx [--all]         the user slots U1..U16 (--all: F1..F16 too) from the FM-1 into a file
  fm1_bank.py dump FILE.syx --slot U3       one slot
  fm1_bank.py load FILE.syx [--slot U5]     the file's presets to the FM-1 (each into the slot it names; --slot: a
                                            one-preset file into that slot). The FM-1 saves the bank once they stop
  fm1_bank.py show FILE.syx                 the presets in a file
A .syx is the FM-1's own frames one after another: F0 7D 44 46 02 slot 05 <the scene, seven bytes to eight> F7."""
import sys, time

HEAD = [0x7D, 0x44, 0x46]
NAME_LEN = 12

def slot_num(s):
    s = s.upper()
    if s[0] in "FU" and s[1:].isdigit() and 1 <= int(s[1:]) <= 16:
        return int(s[1:]) - 1 + (16 if s[0] == "U" else 0)
    raise SystemExit("slot: F1..F16 or U1..U16")

def slot_name(n):
    return ("F%d" if n < 16 else "U%d") % (n % 16 + 1)

def frames_of(data):
    out, i = [], 0
    while i < len(data):
        if data[i] != 0xF0:
            i += 1
            continue
        j = data.index(0xF7, i)
        out.append(list(data[i + 1:j]))
        i = j + 1
    return out

def unpack(d):
    out = []
    for g in range(0, len(d) - 7, 8):
        hi = d[g]
        out += [d[g + 1 + k] | ((hi >> k) & 1) << 7 for k in range(7)]
    return out

def preset_name(frame):
    if len(frame) < 6 or frame[:3] != HEAD or frame[3] != 2:
        return None, None
    raw = unpack(frame[6:])
    name = bytes(raw[:NAME_LEN]).split(b"\0")[0].decode("ascii", "replace")
    return frame[4], name

def ports():
    import mido
    outs = [n for n in mido.get_output_names() if "FM-1" in n]
    ins = [n for n in mido.get_input_names() if "FM-1" in n]
    if not outs or not ins:
        raise SystemExit("no FM-1 on USB")
    return mido.open_output(outs[0]), mido.open_input(ins[0])

def dump(path, which):
    import mido
    out, inp = ports()
    want = [which] if isinstance(which, int) else list(range(16, 32)) if which == "user" else list(range(32))
    req = mido.Message("sysex", data=HEAD + [1, which if isinstance(which, int) else 0x40 if which == "user" else 0x41])
    got = {}
    time.sleep(0.3)                      # a just-opened input can miss what arrives in its first moments
    for attempt in range(3):             # so ask again if nothing came
        out.send(req)
        t0 = time.time()
        while time.time() - t0 < 2.0 and len(got) < len(want):
            for m in inp.iter_pending():
                if m.type == "sysex" and list(m.data[:3]) == HEAD and m.data[3] == 2:
                    got[m.data[4]] = list(m.data)
            time.sleep(0.02)
        if got:
            break
    missing = [slot_name(s) for s in want if s not in got]
    if missing:
        raise SystemExit("no answer for " + " ".join(missing) + " (is DEEP FIELD 0.10 or later on the unit?)")
    with open(path, "wb") as f:
        for s in want:
            f.write(bytes([0xF0] + got[s] + [0xF7]))
    for s in want:
        print(slot_name(s), preset_name(got[s])[1])
    print("wrote", path)

def load(path, slot):
    import mido
    out, inp = ports()
    frames = [f for f in frames_of(open(path, "rb").read()) if f[:3] == HEAD and len(f) > 6 and f[3] == 2]
    if not frames:
        raise SystemExit("no DEEP FIELD presets in " + path)
    if slot is not None:
        if len(frames) != 1:
            raise SystemExit("--slot needs a one-preset file")
        frames[0][4] = slot
    for f in frames:
        if f[4] < 16:
            print("skipping", slot_name(f[4]), "(a factory slot; load it into a U slot with --slot)")
            continue
        out.send(mido.Message("sysex", data=f))
        print("sent", slot_name(f[4]), preset_name(f)[1])
        time.sleep(0.06)
    print("the FM-1 saves the bank about a second after the last one (BANK SAVED on its screen)")

def show(path):
    for f in frames_of(open(path, "rb").read()):
        s, name = preset_name(f)
        if s is not None:
            print(slot_name(s), name)

def main(argv):
    if len(argv) < 2:
        raise SystemExit(__doc__)
    cmd, args = argv[0], argv[1:]
    slot = None
    if "--slot" in args:
        i = args.index("--slot")
        slot = slot_num(args[i + 1])
        del args[i:i + 2]
    if cmd == "dump":
        dump(args[0], slot if slot is not None else "all" if "--all" in args else "user")
    elif cmd == "load":
        load(args[0], slot)
    elif cmd == "show":
        show(args[0])
    else:
        raise SystemExit(__doc__)

if __name__ == "__main__":
    main(sys.argv[1:])
