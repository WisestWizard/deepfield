# Pocket Poly — custom firmware for the M-VAVE FM-1

This repository holds two firmwares on one fm1-base: **Pocket Poly** (`app/`, below) and **DEEP FIELD**
(`deepfield/`, an ambient drone machine: see `deepfield/README.md`). `./build.sh deepfield ...` builds the latter.

A synthesizer, not a groovebox: a stereo polysynth with a discovery engine (seeded dice / mutate / morph / breed
for patches, an arpeggiator with Turing and Euclidean generators). MIDI is first-class: USB, TRS in, and Bluetooth
MIDI both ways (to a computer or phone, and from a Bluetooth keyboard or controller). In development; this tree is
the scaffold.

Built as an app on [fm1-base](https://github.com/charlesvestal/fm1-base) (MPL-2.0) over
[felucca-bsp](https://github.com/hugelton/felucca-bsp) (MPL-2.0). Research and decisions: `docs/`.

## Layout

| Path | What |
| --- | --- |
| `app/` | the firmware: `fm1.toml` (build settings), `app.h` / `app.c` (the app), `tests/` (simulator scripts), `docs/MANUAL.md`, `site/`, `words.tsv` |
| `base/` | fm1-base, a git submodule (felucca-bsp inside it as `base/bsp`) |
| `docs/` | `fm1-research-findings.md`, `persona-evaluation.md`, `platform-decision.md` |
| `tools/` | `render_all.py` (every factory sound to WAV, checked against `tests/golden.txt`), `cpu_probe.py` (CPU per patch on the device), `fm1_dx7.py` (DX7 banks in) |
| `build.sh` | runs `base/tools/build.py app` |

## Prerequisites (macOS, Apple silicon)

- Homebrew's Python 3.14 and its Pillow, built against libraqm: `brew install python@3.14 pillow` (the font generator
  needs raqm; Xcode's Python and pip's Pillow wheel do not have it).
- A container runtime with a `docker` CLI for the Linux-only JieLi toolchain: `brew install colima docker`, then
  `colima start --vm-type vz --vz-rosetta`.
- The JieLi toolchain: `sh base/tools/get_toolchain.sh` (→ `~/.jieli/toolchain`, 135 MB).
- The JieLi AC79 SDK, three files of which go into every package:
  `git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git ~/fw-AC79_AIoT_SDK` (1 GB).
- For installing over USB from a terminal: `python3 -m pip install --user mido python-rtmidi`.

`JIELI_TOOLCHAIN`, `AC79_SDK` and `PYTHON` override the defaults (`~/.jieli/toolchain`, `~/fw-AC79_AIoT_SDK`,
`/opt/homebrew/bin/python3.14`).

## Build and test

```
./build.sh --host
app/build/host/pocketpoly_host app/tests/basic.txt app/build/host
./build.sh
```

The simulator runs the whole app on the Mac from a script (commands at the top of `base/host/plat_host.c`), with
WAV and PNG output. Device builds run the toolchain in a `linux/amd64` container.

```
python3 tools/render_all.py
```

renders every factory patch and FM voice to `app/build/renders/` and fails if any render's hash differs from
`tests/golden.txt`; after an intended change to the sound, `--golden` rewrites the file.

## Install

```
python3 base/tools/fm1_install.py app/build/pocketpoly.fwsc
```

At your own risk. Hold OCT− (or OCT− and OCT+ for five seconds) at power-on for update mode; the installer and the
Firmware Hub can put M-VAVE's own firmware back. If the FM-1 only shows up as the chip's `WL82 UBOOT1.00` disk,
see `base/bsp/tools/fm1_rescue`.

## Licence

MPL-2.0 for this app's files (proposed; see `docs/platform-decision.md`). fm1-base and felucca-bsp keep their own
MPL-2.0 files; JieLi's SDK files used by the build are Apache-2.0 and are never committed here.
