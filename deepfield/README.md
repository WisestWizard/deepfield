# DEEP FIELD — an ambient drone machine for the M-VAVE FM-1

Every key strikes a body that swells in and stays; eight of them rise as beams over the keys on the screen, breathe,
drift, and move to neighbouring notes by themselves when ORBIT runs, or under an arpeggio with Euclidean rhythms. Four
forces (MASS, DRIFT, SPACE, TIDE) shape every body at once; a held key gives one body its own LEVEL, TONE, TUNE and an
ORBIT pin. Ten models (SOLAR, VOID, GLASS, STRING, CHOIR, BELL, SHEPARD, FOLD, and from Pocket Poly the SCAN wavetables
and four-operator DX), a noise layer under them or alone, a hall reverb with FREEZE and SHIMMER, a delay of seven kinds
(ping-pong, tape, three heads, reverse, octaves up, octaves down, grains) and an ensemble. 32 scenes, each a sound:
changing scene morphs the sound under the chord you are playing, each body crossing into the new model. The screen,
PHOSPHOR TUBE, lets only sound give off light: beams in their key's colour with white-hot heads, dark-glass keys
lit by their bodies, one meaning per colour. MIDI over USB, Bluetooth (both ways) and TRS.

Built as an app on [fm1-base](https://github.com/charlesvestal/fm1-base) (MPL-2.0), beside Pocket Poly in this
repository. The research and the point of view behind it: `docs/research.md`. The manual: `docs/MANUAL.md`.

## Layout

| Path | What |
| --- | --- |
| `fm1.toml` | build settings (package identity FM-1_1400, Bluetooth both roles, the second core) |
| `app.h`, `app.c` | the contract with fm1-base, MIDI in, the simulator's checks |
| `scenes.c` | settings, the scene bank, the factory scenes, the scales |
| `ui.c` | the keys, buttons, pages, the screen that mirrors the panel: beams over the keys strip (and the MOUTH, ORBITS, NEBULA, SCOPE skins) |
| `engine/drone.c`, `drone.h` | the engine: eight bodies, ten models, each body's own values, the hand-over between models, the noise layer, ORBIT and the arp, the morph (an -O2 unit) |
| `engine/fx.c`, `fx.h` | ensemble, the delay and its seven types (WOW, CHOP), an eight-line FDN hall with pre-delay, freeze and shimmer, limiter |
| `tests/basic.txt` | 499 simulator checks; `tests/screens.txt` the manual's screenshots; `tests/golden.txt` the renders' hashes |
| `tools/render_scenes.py` | every factory scene, with its chord, to WAV, compared with the golden file (`--levels` prints peak/RMS); `tools/fx_levels.py` prints each effect's wet level |
| `tools/gen_wt.py` | SCAN's wavetables and their loudness gains, generated at build time (fm1.toml `gen`) into flash |
| `art.h` | the pixel art as text: the ten model glyphs and the seven page icons, tinted through the colour ramps |
| `words.tsv`, `site/`, `docs/` | the screen reader's words, the site's front page, the manual, research, and `docs/BUDGET.md`: what the device can do and what the app uses |

## Build and test

```
./build.sh deepfield --host
deepfield/build/host/deepfield_host deepfield/tests/basic.txt deepfield/build/host
python3 deepfield/tools/render_scenes.py
JIELI_TOOLCHAIN="$HOME/.jieli/toolchain" AC79_SDK="$HOME/fw-AC79_AIoT_SDK" ./build.sh deepfield
python3 base/tools/fm1_install.py deepfield/build/deepfield.fwsc
```

## Licence

MPL-2.0 (proposed, as for Pocket Poly). fm1-base and felucca-bsp keep their own MPL-2.0 files.
