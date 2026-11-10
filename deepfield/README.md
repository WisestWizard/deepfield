# DEEP FIELD — an ambient drone machine for the M-VAVE FM-1

Every key strikes a voice that swells in and stays, as hard as you struck it: how hard sets its level over 18 dB,
how bright it is and how fast it arrives. Eight of them rise as beams over the keys on the screen, breathe,
drift, and move to neighbouring notes by themselves when GEN runs, or under an arpeggio with Euclidean rhythms. Four
macros (TIMBRE, DRIFT, SUB, TREMOLO) shape every voice at once; a held key and a knob give one voice its own LEVEL, TONE,
TUNE and a LOCK against GEN; SEL held moves the chord to the keys you touch. Thirteen models (ADDITIVE, SUPERSAW, FM, BOWED, FORMANT, BELL, SHEPARD, WAVEFOLD, from Pocket Poly WAVETABLE,
FM 4-OP and SYNC, then TAPE, a worn loop, and RESONATOR, a singing bowl), a noise layer under them or alone, a hall reverb with FREEZE and SHIMMER, a delay of seven kinds
(ping pong, tape, multi tap, reverse, octave up, octave down, granular) and an ensemble. 32 presets, each a sound:
changing preset morphs the sound under the chord you are playing, each voice crossing into the new model; SEL held
is the chord's button (a key moves the chord, PRESETS brings the preset's chord in over it, PLAY makes GEN's next
move now, HOME lets every voice go). Every knob has a MIDI CC, in and out, and the presets travel as SysEx. The screen,
PHOSPHOR TUBE, lets only sound give off light: beams in their key's colour with white-hot heads, dark-glass keys
lit by their voices, one meaning per colour; at power-on the name spirals into a vortex. MIDI over USB, Bluetooth (both ways: the MIDI page lists the keyboards around and connects one) and TRS.

Built as an app on [fm1-base](https://github.com/charlesvestal/fm1-base) (MPL-2.0). It grew up beside
[Pocket Poly](https://github.com/WisestWizard/pocketpoly), which three of its models come from. The research and
the point of view behind it: `docs/research.md`. The manual: `docs/MANUAL.md`.

## Layout

| Path | What |
| --- | --- |
| `fm1.toml` | build settings (package identity FM-1_1400, Bluetooth both roles, the second core) |
| `app.h`, `app.c` | the contract with fm1-base, MIDI in (the CC map, the switches and triggers queued for the main loop), the bank over SysEx, the simulator's checks and commands (`param`, `land`, `sxloop`) |
| `scenes.c` | settings, the preset bank, the factory presets, the scales |
| `ui.c` | the keys, buttons, pages, the screen that mirrors the panel: beams over the keys strip |
| `engine/drone.c`, `drone.h` | the engine: eight voices, thirteen models, each voice's own values, the hand-over between models, the noise layer, GEN and the arp, the morph (an -O2 unit) |
| `engine/fx.c`, `fx.h` | ensemble, the delay and its seven types (WOW, GATE), an eight-line FDN hall with pre-delay, freeze and shimmer, limiter |
| `tests/basic.txt` | 699 simulator checks; `tests/screens.txt` the manual's screenshots; `tests/golden.txt` the renders' hashes |
| `tools/render_scenes.py` | every factory preset, with its chord, to WAV, compared with the golden file (`--levels` prints peak/RMS); `tools/fx_levels.py` prints each effect's wet level |
| `tools/gen_wt.py` | WAVETABLE's wavetables and their loudness gains, generated at build time (fm1.toml `gen`) into flash |
| `tools/fm1_bank.py` | the presets over USB as SysEx: `dump` a bank to a .syx, `load` one back, `show` a file (needs mido; /usr/bin/python3 has it) |
| `art.h` | the pixel art as text: the thirteen model glyphs and the seven page icons, tinted through the colour ramps |
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
