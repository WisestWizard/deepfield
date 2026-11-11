# DEEP FIELD — an ambient drone machine for the M-VAVE FM-1

Every key strikes a voice that swells in and stays, as hard as you struck it: how hard sets its level over 18 dB,
how bright it is and how fast it arrives. Eight of them rise as beams over the keys on the screen, breathe, drift,
and move to neighbouring notes by themselves when GEN runs, through a chord progression that leads them one voice
at a time across the scale you set, or under an arpeggio with Euclidean rhythms that can learn a phrase you played
and carry it on. Four macros shape every voice at once; a held key and a knob give one voice its own level, tone and tune. Thirteen
models, a noise layer, a hall reverb with FREEZE and SHIMMER, a delay of seven kinds and an ensemble. 32 presets,
each a sound: changing preset morphs the sound under the chord you are playing. MIDI over USB, Bluetooth (both
ways) and TRS.

**[deepfield.fog.baby](https://deepfield.fog.baby/)** — play it in the browser, install it from Chrome or Edge,
the manual and the cheat sheet. The firmware is in [`deepfield/`](deepfield/), and
[`deepfield/README.md`](deepfield/README.md) is the whole instrument.

Built as an app on [fm1-base](https://github.com/charlesvestal/fm1-base) (MPL-2.0) over
[felucca-bsp](https://github.com/hugelton/felucca-bsp) (MPL-2.0).

Two firmwares grew up beside it and have their own repositories now: **Pocket Poly**, a stereo polysynth with a
discovery engine ([WisestWizard/pocketpoly](https://github.com/WisestWizard/pocketpoly)), and **GNOMNOM**, a
rhythm game where a gnome eats mushrooms that play a tune
([WisestWizard/gnomnom](https://github.com/WisestWizard/gnomnom)). All three lived here until 2026-11-08; the
history up to that day is in this repository, and each firmware's own carries on in its own.

## Layout

| Path | What |
| --- | --- |
| `deepfield/` | the firmware: `fm1.toml` (build settings), `app.c` / `ui.c` / `engine/` / `scenes.c`, `tests/` (simulator scripts), `docs/MANUAL.md`, `site/`, `tools/` |
| `base/` | fm1-base, a git submodule (felucca-bsp inside it as `base/bsp`) |
| `docs/` | `handoff.md` (where the work stands, read this first), the FM-1 research and the platform decision |
| `firmware.json`, `fmhub.json` | what the firmware catalogues read: RemoteFM1 and M-Flasher the first, The Firmware Hub the second |
| `build.sh` | runs `base/tools/build.py deepfield` |

## Build and test

```
./build.sh --host
deepfield/build/host/deepfield_host deepfield/tests/basic.txt deepfield/build/host
./build.sh
```

The simulator runs the whole firmware on the Mac from a script (commands at the top of `base/host/plat_host.c`),
with WAV and PNG output; `deepfield/tests/screens.txt` writes the manual's screenshots. Device builds run the
JieLi toolchain in a `linux/amd64` container.

```
python3 deepfield/tools/render_scenes.py
```

renders every factory preset to WAV and fails if a render's hash differs from `deepfield/tests/golden.txt`; after
an intended change to the sound, `--golden` rewrites the file.

## Prerequisites (macOS, Apple silicon)

- Homebrew's Python 3.14 and its Pillow, built against libraqm: `brew install python@3.14 pillow` (the font
  generator needs raqm; Xcode's Python and pip's Pillow wheel do not have it).
- A container runtime with a `docker` CLI for the Linux-only JieLi toolchain: `brew install colima docker`, then
  `colima start --vm-type vz --vz-rosetta`.
- The JieLi toolchain: `sh base/tools/get_toolchain.sh` (→ `~/.jieli/toolchain`, 135 MB).
- The JieLi AC79 SDK, three files of which go into every package:
  `git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git ~/fw-AC79_AIoT_SDK` (1 GB).
- For installing over USB from a terminal: `python3 -m pip install --user mido python-rtmidi`.

`JIELI_TOOLCHAIN`, `AC79_SDK` and `PYTHON` override the defaults (`~/.jieli/toolchain`, `~/fw-AC79_AIoT_SDK`,
`/opt/homebrew/bin/python3.14`).

## Install

The easiest way is [the installer page](https://deepfield.fog.baby/install/) in Chrome or Edge. From a terminal:

```
python3 base/tools/fm1_install.py deepfield/build/deepfield.fwsc
```

At your own risk. Hold OCT− (or OCT− and OCT+ for five seconds) at power-on for update mode; the installer and the
Firmware Hub can put M-VAVE's own firmware back. If the FM-1 only shows up as the chip's `WL82 UBOOT1.00` disk,
see `base/bsp/tools/fm1_rescue`.

DEEP FIELD keeps only its own 32 presets and its settings. **It does not read any other firmware's projects,
songs or samples**, and nothing it saves is meant to be read by another firmware. Installing any firmware over
another rewrites the app area, so back up what the firmware you are leaving holds before you install this one.

## The site

```
deepfield/tools/publish_site.sh X.Y            builds the site and pushes it to gh-pages
deepfield/tools/publish_site.sh X.Y --local    builds it into deepfield/build/site to preview
```

Only this script, never `base/tools/publish_site.sh`: the base script replaces the branch without a CNAME and
GitHub Pages then drops the custom domain.

## Licence

MPL-2.0 for this firmware's files (see `docs/platform-decision.md`). fm1-base and felucca-bsp keep their own
MPL-2.0 files; JieLi's SDK files used by the build are Apache-2.0 and are never committed here.
